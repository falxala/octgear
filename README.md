# OctGear

RP2040 Zero / compatible boardで動く、8キー + ロータリーエンコーダの小型キーボードです。通常のKeyboard / Consumer Control HIDに加え、WebHIDベースのRemapper、製造検査、ブラウザからのファームウェア更新を提供します。

## Features

- ダイオードなし2 x 4 key matrixと、独立GPIOのロータリーエンコーダCCW / CW / SW
- 8 layers x 11 controlsのキーマップ
- Layer 1-7の個別有効化。Layer 0は常時有効、既定有効はLayer 0/1
- LayerごとのRGB LED色設定。`0,0,0`で消灯
- 外付けWS2812Bと内蔵mirrorの輝度上限を`0-128`で設定、Flash保存
- Encoder回転方向の反転設定とFlash保存
- Keyboard、Consumer Control、次／前レイヤー、Momentary Layer割り当て
- WebHID Remapperによる読込、編集、差分保存
- 物理入力、設定report、Flash保存領域のDiagnostics
- WebHIDからのBOOTSEL移行と、同梱UF2の更新
- Key 5起動によるREADME drive / Serial rescue

## Web Pages

| Page | Path | Role |
| --- | --- | --- |
| Home | `/` | 製品情報と各ツールへの入口 |
| Remapper | `/remapper.html` | キーマップ設定とファームウェア更新 |
| Diagnostics | `/diagnostics.html` | 出荷前・販売前の個体検査 |

RemapperとDiagnosticsは独立したHTMLとして直接開けます。WebHIDを使うため、対応ブラウザかつsecure contextで実行してください。通常はChromium系ブラウザのHTTPS配信またはlocalhostを想定しています。

## Quick Start

開発環境を初期化します。Arduino CLI、RP2040 core、Arduino librariesはリポジトリ内の`.tools/`と`.arduino/`へ導入されます。

```sh
scripts/setup-dev-env.sh
pnpm dev
```

主な検証・生成コマンド:

```sh
pnpm typecheck          # Web TypeScript
pnpm build              # Web production build
pnpm firmware:build     # Firmware compile
pnpm firmware:web       # Firmware compile + 配信用UF2/RESCUE.CMD更新
pnpm hardware:generate  # Hardware profileから3つの生成物を更新
```

初回セットアップと全ビルドをまとめて確認する場合は`scripts/setup-dev-env.sh --verify`を使います。

## Architecture

```text
hardware/octgear/profile.json
        │
        ├─> firmware/.../generated_hardware_config.h
        ├─> apps/web/.../generatedHardwareConfig.ts
        └─> hardware/octgear/pinout.md

Browser ── WebHID config report ──> RP2040 firmware ──> 3-slot Flash journal
   │                                      │
   └─ Remapper / Diagnostics              └─ Keyboard / Consumer HID
```

`hardware/octgear/profile.json`がピン割り当て、コントロール順、レイヤー数の単一ソースです。生成ファイルは直接編集せず、profile変更後に`pnpm hardware:generate`を実行します。Firmware buildでも生成処理は自動実行されます。

詳しいモジュール境界とデータフローは[Architecture](docs/architecture.md)を参照してください。

## Repository Layout

```text
apps/web/                  React + TypeScript + Vite multi-page app
docs/                      横断仕様、開発、運用手順
firmware/octgear/          RP2040 Arduino firmware
hardware/octgear/          Hardware profileと生成pinout
scripts/                   環境構築、生成、firmware build
```

## Hardware Summary

現行構成はRow 2本、Column 4本のダイオードなしkey matrixと、A/Common/B/SWを独立GPIOへ接続するロータリーエンコーダです。ダイオードがないため、一部の矩形同時押しは電気的に区別できません。Firmwareは曖昧な間のMatrix状態を更新せず、phantom keyの出力を防ぎます。正確なGPIOは生成された[pinout](hardware/octgear/pinout.md)を参照してください。

Key 5を押しながらUSB接続すると、その起動だけread-onlyの`OCTGEAR`ドライブとSerial rescueが有効になります。通常起動では表示されません。詳しい復旧手順は[Operations](docs/operations.md)にあります。

## USB Identity

| Field | Default |
| --- | --- |
| Vendor ID | `0xCAFE` |
| Product ID | `0xC608` |
| Manufacturer | `OctGear` |
| Product | `OctGear` |

`0xCAFE`はproject-localな開発用VIDです。商用・広範囲な配布では、割り当て済みVID/PID、許可されたsublicense、またはpid.codes等で取得したPIDへ置き換えてください。Web側の接続filterは`apps/web/src/features/device/usbIdentity.ts`で管理します。

## Documentation

| Document | 内容 |
| --- | --- |
| [Architecture](docs/architecture.md) | システム構成、責務、実行時データフロー |
| [Development](docs/development.md) | 環境構築、コマンド、変更別ワークフロー |
| [Operations](docs/operations.md) | Remapper、更新、Diagnostics、Serial rescue |
| [HID Report Protocol](docs/hid-report.md) | WebHID vendor reportのwire format |
| [Web App](apps/web/README.md) | Webアプリ内部の構成 |
| [Firmware](firmware/octgear/README.md) | Firmwareの構成とbuild |
| [Hardware](hardware/octgear/README.md) | Hardware profileと生成物 |

## Project Policy

OctGearは個人プロジェクトとして管理しています。IssueやPull Requestは送信できますが、確認、返信、レビュー、マージ、対応時期は保証しません。対応まで長期間かかる場合や、対応しない場合があります。必要に応じてForkし、各Forkで管理してください。

## License

Copyright 2026 falxala

Software and software documentation are licensed under the [Apache License 2.0](LICENSE).

`hardware/`以下のhardware design materialsは[OctGear Hardware License 1.0](HARDWARE-LICENSE.md)の対象です。個人による非商用の自作は許可されますが、基準となるハードウェア構成を実質的に変更せず製造・販売することは禁止されています。このため、リポジトリ全体をOSI準拠のオープンソースとして提供するものではありません。

名称、ロゴ、公式性の表示については[Trademark Policy](TRADEMARKS.md)を参照してください。
