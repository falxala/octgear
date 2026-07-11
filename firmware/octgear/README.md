# OctGear Firmware

RP2040 Arduino coreとAdafruit TinyUSBを使う、現行8キー + rotary encoder版firmwareです。Sketch本体は[`octgear/`](octgear/)にあります。

## Responsibilities

- 8 Direct inputとencoder A/B/SWのscan
- 8 layers x 11 controlsのkeymap解決
- Layer 1-7の有効状態を保存し、layer遷移から無効layerを除外。既定有効はLayer 0/1
- Keyboard / Consumer Control HID output
- WebHID vendor reportによる設定とDiagnostics
- Flash-backed EEPROM emulationへのkeymap保存
- UF2 bootloaderへのreboot
- Key 5 boot時のread-only README drive / Serial rescue
- onboard LEDによるlayer、remapper、rescue状態表示

通常時は低遅延scanを行い、Remapper / Diagnostics heartbeat中は通常HID出力を抑止します。Rescue boot中も通常HID出力は行いません。

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
| GPIO、control / layer count、encoder tuning | `hardware/octgear/profile.json` |
| debounce、scan sleep、LED、heartbeat、rescue toggle | `octgear/config.h` |
| USB identity defaults | `scripts/compile-firmware.sh`, `scripts/build-web-firmware.sh` |
| HID command IDs / status | `octgear/hid_reports.h` |

Generated headerは直接編集しません。Hardware profile変更後は`pnpm hardware:generate`を実行します。

## Storage

KeymapはRP2040 Arduino coreのEEPROM emulationを通してexternal SPI Flashへ保存します。起動時にstorageが無効ならcompile済みdefault keymapで初期化します。通常の変更はassignment単位で保存されます。

Diagnostics / Serial rescueのstorage self-testはtest patternのwrite、readback、元keymapのrestoreを行います。実Flashへ書くため、必要な検査時だけ実行します。

## Further Reading

- [Firmware module map](octgear/README.md)
- [Architecture](../../docs/architecture.md)
- [HID report protocol](../../docs/hid-report.md)
- [Update, diagnostics, and rescue](../../docs/operations.md)
