# Firmware Sketch

Arduino IDE / Arduino CLI で開くスケッチ本体です。

## Files

| File | Role |
| --- | --- |
| `octgear.ino` | `setup()` / `loop()` |
| `config.h` | ピン番号、キー数、reportサイズなどの設定 |
| `key_scanner.*` | 8本Direct入力 + エンコーダA/B/SW + 2本仮想GNDのスキャン |
| `keymap.*` | レイヤー別キーマップ |
| `keymap_storage.*` | EEPROMエミュレーション上のキーマップ永続化 |
| `key_assignment.*` | キー割り当て型 |
| `hid_device.*` | USB HID keyboard / consumer / config report |
| `hid_output_queue.*` | Keyboard / Consumer HID input report retry queue |
| `hid_reports.h` | HID report ID と設定コマンド |
| `readme_drive.*` | Read-only USB MSC README / URL shortcut drive |
| `serial_rescue.*` | Key 5起動時だけ有効な対話式Serial救済コマンド |
| `status_led.*` | 本体LED表示 |

設定用HID report仕様は `../../../docs/hid-report.md` を参照します。

## Before Flashing

`config.h` のピン番号は現行8キー + ロータリーエンコーダ版の配線に合わせています。ハードウェアを変更する場合は、以下と `hardware/octgear/pinout.md` を同時に更新します。

- `KEY_PINS`
- `ENCODER_A_PIN`
- `ENCODER_B_PIN`
- `ENCODER_SWITCH_PIN`
- `VIRTUAL_GROUND_PINS`
- `STATUS_LED_KIND`
- `STATUS_LED_PIN`
- `README_DRIVE_ENABLED`

## Timing Tuning

`config.h` の以下の値で、入力遅延・CPU負荷・WebHID応答待ちを調整します。

| Setting | Default | Purpose |
| --- | ---: | --- |
| `DEBOUNCE_US` | `5000` | キー入力のチャタリング除去時間 |
| `IDLE_SCAN_SLEEP_US` | `100` | 通常キーボード時のスキャン間sleep |
| `REMAPPER_SCAN_SLEEP_US` | `1000` | Remapper / Diagnostics接続中のスキャン間sleep |
| `CONFIG_RESPONSE_READY_RETRIES` | `20` | WebHID設定応答でHID readyを待つ回数 |
| `CONFIG_RESPONSE_RETRY_DELAY_US` | `100` | WebHID設定応答のretry間隔 |

通常入力の遅延を優先する場合は `IDLE_SCAN_SLEEP_US` を小さくします。発熱や消費電力を優先する場合は大きくします。

## Build

リポジトリルートで Arduino CLI wrapper を使います。

```sh
scripts/arduino-cli.sh compile \
  --fqbn rp2040:rp2040:waveshare_rp2040_zero \
  --board-options usbstack=tinyusb,freq=125 \
  firmware/octgear/octgear
```

`--board-options usbstack=tinyusb,freq=125` は必須です。設定用 vendor HID report を Adafruit TinyUSB で扱い、CPU クロックを 125 MHz に固定します。

## Current Scope

- 通常HID keyboard / consumer output
- WebHID向けvendor-defined config reportの受け口
- RAM上のキーマップ更新
- EEPROMエミュレーションへのキーマップ永続化
- Key 5 起動時だけ表示する Read-only README drive with `README.TXT`, `READMEEN.TXT`, `REMAPPER.URL`, and `RESCUE.CMD`
- `README.TXT` / `READMEEN.TXT` are UTF-8 with BOM for Windows Notepad compatibility.
- Key 5 起動時だけ有効な対話式Serial rescue
- README drive / Serial rescue中は弱い緑の本体LED表示
- 通常時はレイヤー別固定色、Remapper接続中はカラーホイールの本体LED状態表示
- 通常時のみ押下を低遅延化し、Remapper接続中は通常キー送信を抑止
- USB suspendからのremote wakeup後にキー状態を再送
- Diagnostics用のreport送受信検査とkeymap storage write/read/restore検査

## README Drive

既定では、READMEドライブは表示しません。`config.h` の `README_DRIVE_ENABLED` は `false` です。

Key 5 を押しながらUSB接続した時だけ、PCに小さいRead-only USBメモリとして `OCTGEAR` ドライブを表示します。Key 5 は firmware index `4`、GPIO `12` です。この起動中だけ、Serial rescueも有効になり、本体LEDは弱い緑で点灯します。

含まれるファイルは以下のみです。

- `README.TXT`
- `READMEEN.TXT`
- `REMAPPER.URL`
- `RESCUE.CMD`

Windowsでは `RESCUE.CMD` を実行すると、PowerShellの `System.IO.Ports.SerialPort` を使う対話プロンプトを開きます。`probe` の `OCTGEAR ACK` 応答でCOMポートを自動検出し、見つからない場合だけ手動選択へ落ちます。`help` でコマンド一覧を表示できます。
同じ `RESCUE.CMD` は `pnpm firmware:web` 実行時に `apps/web/public/firmware/` にも書き出されます。

主なSerial rescueコマンド:

```text
state
probe
dump
layer 0
get 0 1
none 0 1
key 1 1 0x00 0x04
consumer 0 1 0x00e2
cycle 0 1
hold 0 5 2
diag
bootloader
```

`none` / `key` / `consumer` / `cycle` / `hold` は指定キーの割り当てをすぐ保存します。key番号は `1-11`、layerは `0-5` です。

### Serial Rescue Examples

`key` はKeyboard assignmentを書き込みます。形式は以下です。

```text
key <layer> <key 1-11> <modifier> <keycode> [keycode...]
```

例:

```text
# Layer 1 / Key 1 を A にする
key 1 1 0x00 0x04

# Layer 1 / Key 2 を Ctrl+C にする
key 1 2 0x01 0x06

# Layer 1 / Key 3 を Ctrl+Shift+Esc にする
key 1 3 0x03 0x29
```

`modifier` はUSB HID keyboard modifier bitmapです。

| Modifier | Value |
| --- | ---: |
| Left Ctrl | `0x01` |
| Left Shift | `0x02` |
| Left Alt | `0x04` |
| Left GUI | `0x08` |
| Right Ctrl | `0x10` |
| Right Shift | `0x20` |
| Right Alt | `0x40` |
| Right GUI | `0x80` |

`consumer` はConsumer Control assignmentを書き込みます。

```text
# Layer 0 / Key 1 をMuteにする
consumer 0 1 0x00e2

# Layer 0 / Key 2 をVolume Upにする
consumer 0 2 0x00e9

# Layer 0 / Key 3 をVolume Downにする
consumer 0 3 0x00ea
```

`none` は割り当てを消します。

```text
none 2 8
```

`cycle` は押すたびに通常レイヤを `0 -> 1 -> 2 -> 3 -> 4 -> 5 -> 0` の順で切り替えます。

```text
# Layer 0 / Key 1 をレイヤローテーションにする
cycle 0 1
```

`hold` は押している間だけ指定レイヤへ一時切り替えし、離すと元の通常レイヤへ戻します。

```text
# Layer 0 / Key 5 を押している間だけ Layer 2 にする
hold 0 5 2
```

PowerShellから直接送る場合は、LF (`0x0A`) でコマンドを終端します。`WriteLine()` ではなくbyte配列で送ると安定します。

```powershell
$port = "COM5"
$s = [System.IO.Ports.SerialPort]::new($port, 115200, "None", 8, "One")
$s.NewLine = [string][char]10
$s.DtrEnable = $true
$s.RtsEnable = $true
$s.Open()

$bytes = [Text.Encoding]::ASCII.GetBytes("probe" + [char]10)
$s.Write($bytes, 0, $bytes.Length)
Start-Sleep -Milliseconds 200
$s.ReadLine()

$bytes = [Text.Encoding]::ASCII.GetBytes("key 1 1 0x00 0x04" + [char]10)
$s.Write($bytes, 0, $bytes.Length)
Start-Sleep -Milliseconds 200
$s.ReadLine()

$s.Close()
```

常時表示したい場合は `config.h` の `README_DRIVE_ENABLED` を `true` にします。

## Diagnostics Mode

Webの `diagnostics.html` は販売前/出荷前チェック用です。

- WebHID接続状態
- 診断用 `DiagnosticReport` の送受信確認
- キーマップ保存領域を使った `DiagnosticStorage` の書込/読込/復元確認
- 物理キー8個 + エンコーダ3操作の入力確認
- `KeyEvent` 受信確認
- firmwareが返す report key count

Diagnostics接続中は Remapper heartbeat が有効になり、通常のkeyboard / consumer outputは止まります。検査中のキー押下はPC入力として送信されず、Web UIの押下チェックだけに使われます。

Storage testは実際のFlash-backed keymap保存領域へ書き込みます。出荷検査など必要な時だけ実行してください。テスト後は元のキーマップを復元して保存します。
