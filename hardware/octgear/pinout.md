# Pinout

現行8キー + ロータリーエンコーダ版のファームウェアとPCB設計を合わせるためのピン割り当てです。

この表は `hardware/octgear/profile.json` から生成します。

## Key Matrix

8 keys form a 2 x 4 matrix without diodes. Columns use `INPUT_PULLUP`; firmware drives only the selected row `OUTPUT LOW` and leaves the other row high impedance.

| Logical key | Firmware index | Matrix position | Notes |
| --- | ---: | --- | --- |
| Key 1 | 0 | Row 1 / Column 1 | Matrix Row 1 / Column 1 |
| Key 2 | 1 | Row 1 / Column 2 | Matrix Row 1 / Column 2 |
| Key 3 | 2 | Row 1 / Column 3 | Matrix Row 1 / Column 3 |
| Key 4 | 3 | Row 1 / Column 4 | Matrix Row 1 / Column 4 |
| Key 5 | 4 | Row 2 / Column 1 | Matrix Row 2 / Column 1 / README drive trigger |
| Key 6 | 5 | Row 2 / Column 2 | Matrix Row 2 / Column 2 |
| Key 7 | 6 | Row 2 / Column 3 | Matrix Row 2 / Column 3 |
| Key 8 | 7 | Row 2 / Column 4 | Matrix Row 2 / Column 4 |

| Row | Index | GPIO | Mode |
| --- | ---: | ---: | --- |
| Row 1 | 0 | 0 | Scan output; selected row is OUTPUT LOW |
| Row 2 | 1 | 9 | Scan output; selected row is OUTPUT LOW |

| Column | Index | GPIO | Mode |
| --- | ---: | ---: | --- |
| Column 1 | 0 | 5 | INPUT_PULLUP |
| Column 2 | 1 | 6 | INPUT_PULLUP |
| Column 3 | 2 | 7 | INPUT_PULLUP |
| Column 4 | 3 | 8 | INPUT_PULLUP |

Because the matrix has no diodes, rectangular multi-key combinations are electrically ambiguous. Firmware holds the previous stable matrix state while an ambiguous combination is present, preventing phantom key output.

## Rotary Encoder

Encoder A/B and SW are independent from the key matrix. A/B/SW use `INPUT_PULLUP`; Common is a dedicated `OUTPUT LOW` on GPIO 12.

The compiled default direction is reversed. Remapper can change and persist the direction without rebuilding firmware.

| Signal | GPIO | Firmware control index |
| --- | ---: | ---: |
| A | 11 | CCW/CW: 8/9 |
| Common | 12 | - |
| B | 13 | CCW/CW: 8/9 |
| SW | 10 | 10 |

## Removed Parts

新ハードウェアでは以下を使いません。

- External RGB LED / WS2812B
- OLED

状態表示は本体LEDのみで行います。
