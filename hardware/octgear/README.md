# OctGear Hardware

現行8キー + ロータリーエンコーダ版ハードウェア資料です。

## 前提

- 8本のDirect入力ピンを使う
- ロータリーエンコーダはA/B相 + SWの3ピンを使う
- 仮想GND用GPIOは2本
- 仮想GND用GPIOはファームウェア側で `OUTPUT LOW`
- 外付けRGB LED / OLED は廃止
- LED表示は本体LEDのみ
- Key 5を押しながらUSB接続すると、README driveとSerial rescueをその起動だけ表示

## Documents

- `profile.json`: ピン割り当てとWeb表示用metadataの単一ソース
- `pinout.md`: 現行ピン割り当て

`pinout.md`、firmware用 `generated_hardware_config.h`、Web用 `generatedHardwareConfig.ts` は `pnpm hardware:generate` で生成します。
