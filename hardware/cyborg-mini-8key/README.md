# Cyborg Mini 8-Key Hardware

現行8キー版ハードウェア資料です。

## 前提

- 8本のDirect入力ピンを使う
- 仮想GND用GPIOは2本
- 仮想GND用GPIOはファームウェア側で `OUTPUT LOW`
- 外付けRGB LED / OLED は廃止
- LED表示は本体LEDのみ
- Key 5を押しながらUSB接続すると、README driveとSerial rescueをその起動だけ表示

## Documents

- `pinout.md`: 現行ピン割り当て

旧6キー版のケース、PCB、ボトムプレート資料は `legacy/hardware/cyborg-mini-6key/` にあります。
