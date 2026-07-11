# OctGear Hardware

現行8キー + rotary encoder版のhardware metadataです。PCB designそのものではなく、firmwareとWeb UIが共有するlogical control、pin mapping、layer count、default enabled layers、default layer colorsを管理します。

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

- 8 Direct inputは`INPUT_PULLUP`で、press時にLOW
- VGND1 / VGND2はfirmwareが常時`OUTPUT LOW`に設定
- EncoderはA/B quadratureとpull-upされたswitch input
- External RGB LED / OLEDは使用しない
- Status表示はboard onboard LEDのみ
- Key 5は通常inputとrescue boot triggerを兼用

現在のGPIOとfirmware indexは[`pinout.md`](pinout.md)を参照してください。Data flowと影響範囲は[`docs/architecture.md`](../../docs/architecture.md)にあります。
