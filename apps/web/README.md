# Web App

React + TypeScript + Vite のWebHID版ツールです。

## Scope

- Home product page
- Remapper
- Diagnostics
- Firmware updater / bundled UF2 download
- Japanese default text with language files in `src/shared/i18n/`

Web Serial APIは使いません。固定長HID report仕様は `../../docs/hid-report.md` を参照します。

WebHID接続候補は `src/features/device/usbIdentity.ts` で管理します。

## Pages

Viteのmulti-page buildで以下を出力します。

| Page | Source | Output |
| --- | --- | --- |
| Home | `src/main.tsx` | `index.html` |
| Remapper | `src/pages/remapper.tsx` | `remapper.html` |
| Diagnostics | `src/pages/diagnostics.tsx` | `diagnostics.html` |

Homeは製品ページと入口です。RemapperとDiagnosticsは別HTMLとして直接開けます。

## Remapper

Remapperは接続時またはReadボタン押下時だけデバイス内容を読みます。編集後は「書き込む内容」として保持し、Save時に現在の選択キーだけを書き込みます。同一内容の場合は書き込みをスキップします。

接続中はDisconnectボタンでHID接続を閉じます。物理切断イベントも検知してUI状態を未接続へ戻します。

## Diagnostics

Diagnosticsは販売前/出荷前チェック用です。

- WebHID availability
- Device connection
- Key press event
- Diagnostic report send/receive
- Storage write/read/restore
- Report key count

WebHID接続後、物理キーを押すと `KeyEvent` で押下済みが記録されます。Remapper heartbeat中はファームウェア側で通常キー送信が止まるため、検査入力はPCへ流れません。

Storage testは実際のFlash-backed keymap保存領域へ書き込みます。出荷検査など必要な時だけ実行してください。

## Firmware Updater

Updaterは以下を提供します。

- WebHID経由でBOOTSELへ移行
- Bundled UF2のダウンロード
- File System Access API対応ブラウザでBOOTSEL driveへUF2書き込み

直接書き込みできないブラウザではUF2をダウンロードして、OS上でBOOTSEL driveへコピーします。

## 主なディレクトリ

```text
src/
├── app/
├── features/
│   ├── device/
│   ├── firmware/
│   ├── hardware/
│   └── keymap/
├── pages/
└── shared/
```

## 開発コマンド

リポジトリルートから実行します。

```sh
pnpm install
pnpm dev
pnpm typecheck
pnpm build
pnpm firmware:web
```

`pnpm firmware:web` は `apps/web/public/firmware/cyborg-mini-8key.uf2` を更新します。
