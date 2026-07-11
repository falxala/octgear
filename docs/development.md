# Development

この文書はlocal setup、日常的な変更、生成物、検証の手順をまとめます。コマンドはすべてrepository rootで実行します。

## Prerequisites

- Linux x86_64 / arm64、またはmacOS x86_64 / arm64
- Node.jsとCorepack
- `curl`、`tar`、`xxd`
- USB接続したRP2040 boardを扱える環境
- WebHID確認用のChromium系browser

Versionはrepository側で固定しています。

| Tool / dependency | Version source |
| --- | --- |
| pnpm | root `package.json`の`packageManager` |
| Arduino CLI | `scripts/setup-dev-env.sh` |
| RP2040 Arduino core | `scripts/setup-dev-env.sh` |
| Arduino libraries | `scripts/setup-dev-env.sh` |

## Setup

```sh
scripts/setup-dev-env.sh
```

このscriptはArduino CLIを`.tools/bin/`、Arduino data / downloads / user directoryを`.arduino/`へ配置し、pnpm dependenciesをinstallします。system-wide installは不要です。

Options:

```sh
scripts/setup-dev-env.sh --verify     # setup後にWebとfirmwareをbuild
scripts/setup-dev-env.sh --skip-pnpm  # pnpm installを省略
```

`ARDUINO_CLI_VERSION`と`RP2040_CORE_VERSION`は環境変数でoverrideできます。

## Commands

| Command | Output / purpose |
| --- | --- |
| `pnpm dev` | Vite dev serverを全interfaceで起動 |
| `pnpm typecheck` | Web TypeScriptをemitなしで検査 |
| `pnpm build` | typecheck後に`apps/web/dist`を生成 |
| `pnpm preview` | production buildをlocal preview |
| `pnpm hardware:generate` | profile由来のfirmware / Web / pinout生成 |
| `pnpm firmware:build` | Arduino firmwareをcompile |
| `pnpm firmware:web` | UF2と`RESCUE.CMD`をWeb public assetsへ出力 |
| `pnpm rescue-cmd:assets` | `rescue.cmd`からC headerだけを再生成 |

## Development Workflows

### Web-only Change

```sh
pnpm typecheck
pnpm build
```

画面を確認する場合は`pnpm dev`を起動します。ViteはHome、Remapper、Diagnosticsの3 entryを扱います。

### Hardware Profile Change

1. `hardware/octgear/profile.json`を編集します。
2. `pnpm hardware:generate`を実行します。
3. 3つの生成物をdiffで確認します。
4. `pnpm build`と`pnpm firmware:build`を実行します。

生成対象:

```text
firmware/octgear/octgear/generated_hardware_config.h
apps/web/src/features/hardware/generatedHardwareConfig.ts
hardware/octgear/pinout.md
```

Firmware build scriptもgeneratorを実行しますが、profile変更commitには3つの生成物を含めます。

### Firmware Change

```sh
pnpm firmware:build
```

配信用artifactも更新する変更では次を使います。

```sh
pnpm firmware:web
```

出力される`apps/web/public/firmware/octgear.uf2`と`RESCUE.CMD`はWeb buildへ含まれます。既定boardは`rp2040:rp2040:waveshare_rp2040_zero`、board optionsは`usbstack=tinyusb,freq=125`です。

Build identityは環境変数でoverrideできます。

```sh
USB_VID=0x1234 USB_PID=0x5678 \
USB_MANUFACTURER=Example USB_PRODUCT=OctGear \
pnpm firmware:build
```

VID/PIDを変えた場合、WebHID filterも同時に更新しないとRemapperから接続できません。

### Rescue Script Change

`firmware/octgear/octgear/rescue.cmd`がsourceです。Firmwareに埋め込む`rescue_cmd_asset.h`はbuild時に再生成されます。Web配布物まで同期するには`pnpm firmware:web`を実行します。

## Configuration Ownership

| Value | Edit here | Do not edit directly |
| --- | --- | --- |
| pin / control / layer metadata | `hardware/octgear/profile.json` | generated hardware files |
| debounce / sleep / LED / rescue toggle | firmware `config.h` | generated header |
| USB identity defaults | firmware build scripts | compiled UF2 |
| WebHID accepted identity | Web `usbIdentity.ts` | transport code |
| HID wire protocol | firmware + Web protocol modules | one side only |
| UI copy | `src/shared/i18n/ja.ts` and `en.ts` | component-local literals |

## Verification Matrix

| Change area | Minimum verification |
| --- | --- |
| Markdown only | links、commands、generated-file warningの目視確認 |
| Web UI / state | `pnpm typecheck`, `pnpm build` |
| Hardware profile | `pnpm hardware:generate`, Web build, firmware build |
| Firmware logic | `pnpm firmware:build`, target boardで実機確認 |
| HID protocol | Web build、firmware build、Remapper connect/read/save、Diagnostics report |
| Storage / rescue | firmware build、実機でrestore / rebootを含む確認 |

現在repositoryには自動unit test suiteがありません。入力timing、USB lifecycle、Flash persistence、BOOTSEL / MSCは実機確認が必要です。

## Deployment

`.github/workflows/deploy-pages.yml`は`main`へのpushまたはmanual dispatchでGitHub Pages artifactをbuildし、`apps/web/dist`をdeployする構成です。Feature branchやPRではdeploy jobを実行しません。

Pages buildではViteの`github-pages` modeが使われ、base pathは`/octgear/`になります。通常buildとdev serverのbase pathは`/`です。

## Review Checklist

- source-of-truthと生成物が同期している
- Webとfirmwareのlayer / control countが一致する
- HID protocol変更が両実装と文書へ反映されている
- 配布対象firmware変更時にUF2も更新されている
- Storage testを通常のread-only検査として誤解させる表現がない
- USB identity変更時にWebHID filterも更新されている
