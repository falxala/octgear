# OctGear

RP2040 Zero / 互換ボードを使う8キー + ロータリーエンコーダ小型キーボードと、WebHIDベースの設定ツールです。

## Current State

- 8キー Direct input + rotary encoder A/B/SW
- 6 layers x 11 controls のキーマップ
- Keyboard / Consumer Control HID output
- WebHID Remapper
- Diagnostics for production inspection
- Browser-based firmware updater / bundled UF2 download
- Key 5 boot serial rescue for offline recovery
- GitHub Pages deployment from `main`

## Pages

Vite multi-page buildで以下を出力します。

| Page | Path | Purpose |
| --- | --- | --- |
| Home | `/` | 製品ページと入口 |
| Remapper | `/remapper.html` | キーマップ読込、編集、保存、ファームウェア更新 |
| Diagnostics | `/diagnostics.html` | 出荷前/販売前の個体検査 |

Remapper / Diagnostics は別HTMLとして直接開けます。ブランドクリックはHomeへ戻ります。

## Repository Layout

```text
apps/web/                       React + TypeScript + Vite web app
firmware/octgear/      RP2040 firmware
hardware/octgear/      現行8キー + エンコーダ版ハードウェアメモ
docs/hid-report.md              WebHID config report protocol
scripts/                        Arduino CLI wrapper / firmware build scripts
```

Hardware pinout and UI-facing control metadata are sourced from `hardware/octgear/profile.json`.
Run `pnpm hardware:generate` after editing it, or use the firmware build scripts which run generation automatically.

## Hardware Summary

現在のピンマップは `hardware/octgear/profile.json` を正とします。詳細表は `hardware/octgear/pinout.md` に生成します。

Key 5を押しながらUSB接続すると、Read-only README driveとSerial rescueをその起動だけ表示します。通常は非表示です。Windowsではドライブ内の `RESCUE.CMD` から対話式の救済プロンプトを開けます。このモード中は本体LEDを弱い緑で点灯します。

## Firmware

Sketch:

```text
firmware/octgear/octgear/
```

主な実装:

- `key_scanner.*`: 8本Direct入力 + エンコーダA/B/SW + 2本仮想GND
- `keymap.*`: RAM上の6 layer x 11 control assignment
- `keymap_storage.*`: Flash-backed EEPROM emulationへの永続化
- `hid_device.*`: Keyboard / Consumer / vendor config report
- `hid_output_queue.*`: Keyboard / Consumer HID input report retry queue
- `readme_drive.*`: Key 5起動時だけ表示するRead-only drive
- `serial_rescue.*`: Key 5起動時だけ有効な対話式Serial救済コマンド
- `status_led.*`: 本体LED状態表示

Firmware buildはRP2040 Arduino coreの `waveshare_rp2040_zero` を使い、CPU clockを `125 MHz` に固定します。

```sh
pnpm firmware:build
pnpm firmware:web
```

`pnpm firmware:web` は `apps/web/public/firmware/octgear.uf2` も更新します。

## Diagnostics

Diagnosticsは全個体検査向けです。

- WebHID support
- Device connection
- Physical key / encoder input event
- Diagnostic HID report send/receive
- Keymap storage write/read/restore
- Firmware-reported key count

Storage testは実際のFlash-backed keymap保存領域へ書き込みます。出荷検査など必要な時だけ実行してください。検査はテストパターンを書き、再読込検証後に元のキーマップを復元します。

## Development

初回は `scripts/setup-dev-env.sh` で、Web依存関係、Arduino CLI、RP2040 core、ファームウェア用Arduino librariesをローカルの `.tools/` / `.arduino/` に導入します。セットアップ後に検証まで行う場合は `scripts/setup-dev-env.sh --verify` を使います。

```sh
scripts/setup-dev-env.sh
```

通常の開発コマンド:

```sh
pnpm dev
pnpm typecheck
pnpm build
pnpm firmware:build
pnpm firmware:web
```

`pnpm firmware:web` は配信用UF2に加えて、Windows offline rescue用の `apps/web/public/firmware/RESCUE.CMD` も更新します。

## Deployment

GitHub Pages deploy workflowは `main` へのpush、または手動実行で動きます。PR / feature branchではPages deployを走らせません。

The workflow uses Node.js 24 and builds `apps/web/dist`.

## USB Identity

Default firmware identity:

| Field | Value |
| --- | --- |
| Vendor ID | `0xCAFE` |
| Product ID | `0xC608` |
| Manufacturer | `OctGear` |
| Product | `OctGear` |

`0xCAFE` is used as the project-local development/vendor ID. Keep project PIDs under the `0xC6xx` range to avoid accidental reuse inside this repository.

Reserved project PIDs:

| PID | Use |
| --- | --- |
| `0xC608` | OctGear normal firmware |
| `0xC609` | Reserved for rescue-mode identity if split from normal firmware |

For broader commercial distribution, replace this with an assigned VID/PID, a permitted vendor sublicense, or an allocated pid.codes PID.

WebHID filter is managed in `apps/web/src/features/device/usbIdentity.ts`.

## Reference

- HID protocol: `docs/hid-report.md`
- Web app notes: `apps/web/README.md`
- Firmware notes: `firmware/octgear/README.md`
- Sketch notes: `firmware/octgear/octgear/README.md`
- Hardware pinout: `hardware/octgear/pinout.md`
