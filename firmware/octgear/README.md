# OctGear Firmware

RP2040 Arduino coreとAdafruit TinyUSBを使う、現行8キー + rotary encoder版firmwareです。Sketch本体は[`octgear/`](octgear/)にあります。

## Responsibilities

- ダイオードなし2 x 4 key matrixとencoder A/B/SWのscan
- 8 layers x 11 controlsのkeymap解決
- Layer 1-7の有効状態を保存し、layer遷移から無効layerを除外。既定有効はLayer 0/1
- LayerごとのRGB LED色を保存。`0,0,0`で消灯
- Keyboard / Consumer Control HID output
- WebHID vendor reportによる設定とDiagnostics
- 3-sector Flash journalへのkeymap保存
- UF2 bootloaderへのreboot
- Key 5 boot時のread-only README drive / Serial rescue
- onboard LEDによるlayer、remapper、rescue状態表示

通常時は低遅延scanを行い、Remapper / Diagnostics heartbeat中は通常HID出力を抑止します。Rescue boot中も通常HID出力は行いません。

Key matrixにはダイオードがありません。複数Row間で2列以上が同時に導通する曖昧な状態では、scannerは直前の安定したMatrix状態を維持し、phantom keyを出力しません。

## Build

Repository rootから実行します。

```sh
pnpm firmware:build
pnpm firmware:web
```

既定FQBNは`rp2040:rp2040:waveshare_rp2040_zero`、board optionsは`usbstack=tinyusb,freq=125`です。`pnpm firmware:web`は配信用の次のassetsも更新します。

```text
apps/web/public/firmware/octgear.uf2
apps/web/public/firmware/RESCUE.CMD
```

Build scriptsはhardware configとrescue command assetをcompile前に再生成します。環境構築、identity override、検証手順は[`docs/development.md`](../../docs/development.md)を参照してください。

## Configuration

| Configuration | Source |
| --- | --- |
| Matrix GPIO、control / layer count、encoder tuning、default layer colors | `hardware/octgear/profile.json` |
| debounce、scan sleep、LED brightness、heartbeat、rescue toggle | `octgear/config.h` |
| USB identity defaults | `scripts/compile-firmware.sh`, `scripts/build-web-firmware.sh` |
| HID command IDs / status | `octgear/hid_reports.h` |

Generated headerは直接編集しません。Hardware profile変更後は`pnpm hardware:generate`を実行します。

## Default Keymap

未初期化の保存領域と`ResetConfiguration`では、次のcompile済み既定値を使用します。

| Control | Layer 0 | Layer 1 |
| --- | --- | --- |
| K1 | 次レイヤー | 次レイヤー |
| K2 | Mute | Q |
| K3 | Volume Down | W |
| K4 | Volume Up | E |
| K5 | Momentary Layer 1 | None |
| K6 | Previous Track | A |
| K7 | Play / Pause | S |
| K8 | Next Track | D |
| Encoder CCW | Volume Down | None |
| Encoder CW | Volume Up | None |
| Encoder SW | None | None |

Layer 2〜7は全controlが`None`です。既定で有効なのはLayer 0/1だけで、Layer 0は無効化できません。既に保存済みの設定はFirmware更新だけでは変更されず、初期化したときにこの既定値が適用されます。

## Storage

Keymap、layer enable mask、layer RGB colorsはexternal SPI Flash上の独立した3つの4KB sectorへ循環保存します。各slotはgenerationとCRCを持ち、起動時はCRCが正常な最新generationを読み込みます。保存中に電源が切れて新slotが不完全になっても、直前の正常slotへfallbackします。

標準buildはArduino coreがfilesystem用として扱う64KBをFirmware領域から分離して予約します。Filesystemはmountせず、その先頭12KBをjournalに直接使用します。設定変更時はRAM上の設定全体を次slotへ書き、1回の保存で消去するsectorを1つに限定します。保存形式が無効または未初期化ならcompile済みdefaultで初期化します。

Diagnostics / Serial rescueのstorage self-testはtest patternのwrite、readback、元keymapのrestoreを行います。実Flashへ書くため、必要な検査時だけ実行します。

## Further Reading

- [Firmware module map](octgear/README.md)
- [Architecture](../../docs/architecture.md)
- [HID report protocol](../../docs/hid-report.md)
- [Update, diagnostics, and rescue](../../docs/operations.md)
