# Architecture

OctGearは、hardware profile、RP2040 firmware、Webアプリの3領域で構成されます。FirmwareとWebはvendor-defined HID reportを境界に独立し、hardware profileから共有すべき定数だけを生成します。

## System Context

```text
                        ┌──────────────────────────────┐
                        │ Browser                      │
                        │ Home / Remapper / Diagnostics│
                        └──────────────┬───────────────┘
                                       │ WebHID report ID 10
                                       │ 32-byte input/output
                        ┌──────────────▼───────────────┐
Physical controls ─────>│ RP2040 firmware              │─────> Keyboard HID
8 keys + encoder        │ scan / keymap / HID / storage│─────> Consumer HID
                        └──────────────┬───────────────┘
                                       │ EEPROM emulation
                        ┌──────────────▼───────────────┐
                        │ External SPI Flash           │
                        │ persisted keymap             │
                        └──────────────────────────────┘
```

通常動作ではfirmwareだけで入力を処理します。RemapperまたはDiagnosticsがheartbeatを送っている間は、設定中の物理入力がOSへ漏れないようKeyboard / Consumer出力を抑止し、代わりに`KeyEvent`をWebへ送ります。

## Source Of Truth

`hardware/octgear/profile.json`は次の値の単一ソースです。

- 物理キーと仮想GNDのGPIO
- エンコーダのGPIO、方向、1 detentあたりのstep数
- firmware indexとWeb UI上のcontrol順
- layer count、physical key count、control count
- default enabled layers
- default layer RGB colors

`scripts/generate-hardware-config.mjs`がprofileを検証し、次の3ファイルを生成します。

| Generated file | Consumer |
| --- | --- |
| `firmware/octgear/octgear/generated_hardware_config.h` | Firmware compile-time constants |
| `apps/web/src/features/hardware/generatedHardwareConfig.ts` | Web UIのcontrol metadata |
| `hardware/octgear/pinout.md` | 人が読むpinout |

生成スクリプトはキーと仮想GNDのindexが配列順と一致すること、encoder controlが`enc-ccw`、`enc-cw`、`enc-sw`の順で連続することを検証します。生成物は直接編集しません。

## Firmware Modules

Firmware entry pointは`firmware/octgear/octgear/octgear.ino`です。

| Module | Responsibility |
| --- | --- |
| `config.h` | timing、LED、heartbeat、rescue等の手動設定 |
| `generated_hardware_config.h` | profile由来のpin / count定数 |
| `key_scanner.*` | Direct input debounce、quadrature decode、control mask生成 |
| `keymap.*` | RAM上のassignment、active layer、layer RGB color |
| `keymap_storage.*` | EEPROM emulationへのload / save / self-test |
| `key_assignment.*` | assignmentの型とconstructor |
| `hid_device.*` | USB lifecycle、config command、通常HID出力の調停 |
| `hid_output_queue.*` | HID ready待ち中のKeyboard / Consumer report保持 |
| `hid_report_descriptor.*` | Keyboard、Consumer、vendor report descriptor |
| `readme_drive.*` | rescue boot時のread-only FAT12 MSC |
| `serial_rescue.*` | rescue boot時の115200 baud command interface |
| `status_led.*` | mount、layer、remapper、rescue状態の表示 |

### Main Loop

1. Scannerが8物理キーとencoder SWを読み、encoder A/Bの遷移を回転eventへ変換します。
2. 通常時はcontrol mask差分を`sendKeyChanges()`へ渡します。
3. `hid_device`がassignmentを解決し、Keyboard / Consumer reportまたはlayer変更を処理します。
4. Config report受信、queued HID report、USB suspend / remote wakeupを更新します。
5. 状態LEDを更新し、通常時100 us、config mode時1000 us sleepします。

Encoder CCW / CWは保持状態ではなく、1 clickのpress pulseとしてindex 8 / 9に現れます。Encoder SWはindex 10の通常debounced inputです。

### Layer Semantics

Assignmentは`None`、`Keyboard`、`Consumer`、`LayerCycle`、`MomentaryLayer`、`LayerPrevious`の6種類です。

- Layer 0は常時有効で、Layer 1-7の有効状態はFlashへ保存します。既定有効maskはLayer 0/1です。
- `LayerCycle`は有効なlayerだけを次方向へ循環します。
- `LayerPrevious`は有効なlayerだけを前方向へ循環します。
- `MomentaryLayer`はtarget layerが有効な場合だけ押下中に切り替え、releaseで通常layerへ戻ります。
- Keyboard assignmentはmodifier bitmapと最大6 keycodesを持ちます。
- Consumer assignmentは16-bit usageを1つ持ちます。
- Layer colorはRGB各8-bitで、`0,0,0`を消灯としてFlashへ保存します。

起動時はcompile済みdefault keymapをRAMへ作成してから保存領域を読みます。保存領域が無効ならdefault keymapとLayer 0/1有効maskを保存して初期化します。旧6-layer storageはLayer 0-5のassignmentと有効maskを維持し、空のLayer 6/7を追加した8-layer layoutへ自動移行します。

## Web Modules

Web entry pointはページごとに分かれ、共通実装を`src/app`と`src/features`から利用します。

| Path | Responsibility |
| --- | --- |
| `src/pages/*.tsx` | Vite multi-page entry points |
| `src/app/pages/` | Home、Remapper、Diagnosticsの画面状態とworkflow |
| `src/app/components/` | UI panelとkeycap表示 |
| `src/app/hooks/useDeviceSession.ts` | heartbeat、timeout、disconnect cleanup |
| `src/features/device/` | WebHID transport、protocol encode/decode、device commands |
| `src/features/keymap/` | assignment model、normalize、picker option |
| `src/features/hardware/` | profileから生成したUI metadata |
| `src/features/firmware/` | UF2 download / File System Access API書込 |
| `src/shared/i18n/` | 日本語・英語message。現在のdefaultは日本語 |

`WebHidTransport`は1 device / 1 in-flight commandを前提とします。同期commandはcommand byteが一致する最初のinput reportを待ち、1秒でtimeoutします。`KeyEvent`は別listenerで非同期に購読します。

## Runtime Flows

### Connect And Read

1. Browserが`0xCAFE:0xC608` filterでdevice pickerを開きます。
2. Deviceをopenし、最初のheartbeatを送ります。
3. `GetState`でfirmwareのlayer / key countを取得します。
4. `GetKey`をlayer x key分、`GetLayerColor`をlayer分送って全設定を読みます。
5. Webは300 ms間隔でheartbeatを送り、失敗または700 ms timeout時にdisconnect扱いにします。

### Edit And Save

Webは最後に読んだkeymap / layer mask / RGB colorsと、編集中の値を別に保持します。Save時にdeviceが報告した範囲へnormalizeし、変更したlayer設定、色、assignmentだけを順に保存します。同一内容はFlashへ書きません。

### Rescue Boot

起動時にKey 5がLOWならread-onlyの`OCTGEAR` MSCとSerial rescueを有効にします。このmodeでは通常HID出力を行わず、LEDを弱い緑で表示します。USBを抜いて通常接続すると終了します。

## Cross-System Contracts

次の変更は複数領域へ影響します。

| Change | Required checks |
| --- | --- |
| GPIO / control数 / encoder | profile更新、generate、Web build、firmware build |
| HID command / payload | firmware `hid_reports.h`とhandler、Web protocol、`docs/hid-report.md` |
| Assignment kind | firmware model、Web model / UI、storage compatibility、protocol docs |
| USB VID/PID | firmware build env、Web filter、READMEのidentity |
| Rescue script | `rescue.cmd`更新、asset生成、firmware / Web bundle更新 |
