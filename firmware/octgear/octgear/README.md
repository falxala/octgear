# Firmware Sketch

Arduino IDE / Arduino CLIで開くOctGear firmware sketchです。利用者向けの更新・救済手順は[`docs/operations.md`](../../../docs/operations.md)、system全体のflowは[`docs/architecture.md`](../../../docs/architecture.md)を参照してください。

## Files

| File | Role |
| --- | --- |
| `octgear.ino` | `setup()` / `loop()`とmode調停 |
| `config.h` | timing、LED、heartbeat、rescue設定 |
| `generated_hardware_config.h` | profileから生成したpin / count定数 |
| `key_scanner.*` | Direct input debounceとencoder decode |
| `keymap.*` | default keymap、RAM assignment、active layer |
| `keymap_storage.*` | EEPROM emulationのload / save / self-test |
| `key_assignment.*` | assignment typeとconstructor |
| `hid_device.*` | USB HID、config command、layer action |
| `hid_output_queue.*` | Keyboard / Consumer report retry queue |
| `hid_report_descriptor.*` | TinyUSB HID report descriptor |
| `hid_reports.h` | report ID、command、status enum |
| `readme_drive.*` | Rescue boot用read-only FAT12 drive |
| `serial_rescue.*` | Rescue boot用command parser |
| `status_led.*` | Device state表示 |
| `rescue.cmd` | Windows offline rescue client source |
| `rescue_cmd_asset.h` | `rescue.cmd`から生成するembedded asset |

## Setup And Loop

`setup()`はLED、keymap、scannerを初期化し、Key 5のboot状態を判定してからUSB deviceを開始します。

`loop()`はscanner、通常HID送信、config report、Serial rescue、status LEDを順に更新します。Remapperまたはrescueがactiveな間は通常HID出力を止め、長いscan sleepへ切り替えます。

## Timing

`config.h`の主なtuning値:

| Setting | Default | Purpose |
| --- | ---: | --- |
| `DEBOUNCE_US` | `5000` | Direct input debounce |
| `IDLE_SCAN_SLEEP_US` | `100` | 通常modeのscan間sleep |
| `REMAPPER_SCAN_SLEEP_US` | `1000` | Config / rescue modeのscan間sleep |
| `REMAPPER_HEARTBEAT_TIMEOUT_MS` | `3000` | 通常出力抑止を解除する期限 |
| `CONFIG_RESPONSE_READY_RETRIES` | `20` | Config input reportのHID ready retry |
| `CONFIG_RESPONSE_RETRY_DELAY_US` | `100` | Retry間隔 |

Input latencyを調整する場合はdebounceとidle sleepの両方を考慮します。Encoder detentは`config.h`ではなくhardware profileの`stepsPerDetent`で調整します。

## Build Invariants

- `usbstack=tinyusb`が必要です。Vendor reportとcomposite USB deviceが依存します。
- CPU frequencyは125 MHzに固定します。
- `generated_hardware_config.h`と`rescue_cmd_asset.h`はsourceを変更後に再生成します。
- HID protocolを変更する場合はWeb implementationと[`docs/hid-report.md`](../../../docs/hid-report.md)を同時に更新します。
- Storage layoutを変える場合は既存deviceのmigrationまたは明示的な初期化方針が必要です。

Repository rootからの標準buildは`pnpm firmware:build`です。直接Arduino CLIを呼ぶ場合も、先に必要な生成処理を実行してください。
