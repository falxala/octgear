# OctGear Hardware

The materials in this directory are provided under the [OctGear Hardware License 1.0](../../HARDWARE-LICENSE.md). Personal, non-commercial builds are permitted. Commercial manufacture or sale of the reference hardware without material hardware changes is prohibited.

現行2 x 4 key matrix + rotary encoder版のhardware metadataです。PCB designそのものではなく、firmwareとWeb UIが共有するlogical control、matrix pin mapping、layer count、default enabled layers、default layer colors、LED輝度既定値 / 上限、外付けWS2812B GPIO / pixel countを管理します。

## Files

| File | Ownership |
| --- | --- |
| `profile.json` | 手動編集するsingle source of truth |
| `pinout.md` | profileから生成する人向けpin table |

同じprofileから次も生成します。

- `firmware/octgear/octgear/generated_hardware_config.h`
- `apps/web/src/features/hardware/generatedHardwareConfig.ts`

## Change Workflow

1. `profile.json`を編集します。
2. Repository rootで`pnpm hardware:generate`を実行します。
3. 3つの生成物のdiffを確認します。
4. Webとfirmwareをbuildします。

```sh
pnpm hardware:generate
pnpm build
pnpm firmware:build
```

Generated filesは直接編集しません。Generatorはindex順とencoder control IDを検証し、不整合時は失敗します。

## Electrical Assumptions

- 2 Row x 4 Columnのmatrixで、ダイオードは使用しない
- Columnは`INPUT_PULLUP`、scan対象Rowだけを`OUTPUT LOW`にする
- ダイオードがないため矩形同時押しは区別できず、Firmwareは曖昧な間のMatrix状態を保持する
- Encoder A/B/SWは`INPUT_PULLUP`、専用Common GPIOは常時`OUTPUT LOW`
- External WS2812Bのdata inputはGPIO 14へ接続し、5 pixels分のdataを送る。4連実機では5番目のdataは無視される
- Status表示は外付けWS2812Bへ出力し、対応boardではonboard WS2812にもミラーする
- OLEDは使用しない
- Key 5は通常inputとrescue boot triggerを兼用

現在のGPIOとfirmware indexは[`pinout.md`](pinout.md)を参照してください。Data flowと影響範囲は[`docs/architecture.md`](../../docs/architecture.md)にあります。
