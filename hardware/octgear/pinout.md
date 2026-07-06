# Pinout

現行8キー版のファームウェアとPCB設計を合わせるためのピン割り当てです。

この表と `firmware/octgear/octgear/config.h` を同時に管理します。

## Key Inputs

Direct入力は8本です。各入力はファームウェアで `INPUT_PULLUP` に設定します。

| Logical key | Firmware index | GPIO | Virtual ground rail | Notes |
| --- | ---: | --- | --- | --- |
| Key 1 | 0 | 7 | VGND1 | Direct input |
| Key 2 | 1 | 6 | VGND1 | Direct input |
| Key 3 | 2 | 5 | VGND1 | Direct input |
| Key 4 | 3 | 4 | VGND1 | Direct input |
| Key 5 | 4 | 12 | VGND2 | Direct input / README drive trigger |
| Key 6 | 5 | 11 | VGND2 | Direct input |
| Key 7 | 6 | 10 | VGND2 | Direct input |
| Key 8 | 7 | 9 | VGND2 | Direct input |

## Virtual Ground

仮想GND用GPIOは2本です。どちらもファームウェアで常時 `OUTPUT LOW` に設定します。

| Rail | Firmware index | GPIO | Notes |
| --- | ---: | --- | --- |
| VGND1 | 0 | 1 | Key 1-Key 4 physical group |
| VGND2 | 1 | 8 | Key 5-Key 8 physical group |

## Removed Parts

新ハードウェアでは以下を使いません。

- Rotary encoder
- External RGB LED / WS2812B
- OLED

状態表示は本体LEDのみで行います。
