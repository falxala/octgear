# Web App

React 19 + TypeScript + Viteで構成する、OctGearのWebHID toolsです。Home、Remapper、Diagnosticsをmulti-page buildで出力します。

システム全体の関係は[`docs/architecture.md`](../../docs/architecture.md)、利用手順は[`docs/operations.md`](../../docs/operations.md)を参照してください。

## Entry Points

| Page | HTML | React entry | Page component |
| --- | --- | --- | --- |
| Home | `index.html` | `src/main.tsx` | `src/app/pages/HomePage.tsx` |
| Remapper | `remapper.html` | `src/pages/remapper.tsx` | `src/app/pages/RemapperApp.tsx` |
| Diagnostics | `diagnostics.html` | `src/pages/diagnostics.tsx` | `src/app/pages/DiagnosticsApp.tsx` |

GitHub Pages modeではVite baseが`/octgear/`、通常のdevelopment / buildでは`/`です。内部linkとfirmware asset URLはbaseを考慮して組み立てます。

## Source Layout

```text
src/
├── app/
│   ├── components/       pageを構成するUI
│   ├── hooks/            device session lifecycle
│   ├── pages/            page-level stateとworkflow
│   └── remapper/         editor固有の変換
├── features/
│   ├── device/           WebHID transportとcommand codec
│   ├── firmware/         UF2 download / install
│   ├── hardware/         generated control metadata
│   └── keymap/           assignment modelとpicker data
├── pages/                multi-page entry points
└── shared/               i18n等の小さい共通module
```

Dependency directionはpageからfeatureへ向けます。TransportはUI stateを持たず、protocol moduleはDOMを参照しません。特定featureだけで使う処理は`shared`へ移さず、そのfeature内に置きます。

## Device Session

接続時は次の順で処理します。

1. `usbIdentity.ts`のVID/PID filterでdeviceを選択
2. HID deviceをopen
3. heartbeatを送信
4. `GetState`でdevice dimensionsを取得
5. `GetKey`で全keymapを読込
6. 300 ms間隔のheartbeatとphysical disconnect listenerを開始

Heartbeat sendが失敗するか700 msでtimeoutするとsessionを閉じます。Firmware側のheartbeat有効期限は3000 msです。

WebHID commandは固定32-byte reportです。同期requestは同じcommand byteのresponseを1つ待つ設計なので、同じtransport上でcommandを並列送信しないでください。詳細は[`docs/hid-report.md`](../../docs/hid-report.md)にあります。

## Remapper State

Remapperは2つのkeymapを保持します。

- `readKeymap`: 最後にdeviceから正常に読んだ内容
- `writeKeymap`: UIで編集中の内容

Save時はdeviceが返したlayer / key countへnormalizeし、両者の差分だけを`SetKey`で逐次保存します。Readは全keymapを再取得し、編集中内容を置き換えます。Physical `KeyEvent`のpressは対応するlayer / controlを選択します。

## Firmware Updater

`src/features/firmware/firmwareUpdater.ts`は次を担当します。

- `${BASE_URL}firmware/octgear.uf2`のdownload
- File System Access API support判定
- Userが選択したUF2 driveへの`octgear.uf2`書込

BOOTSEL移行command自体はdevice featureにあり、page componentがsession closeとUI stateを調停します。

## Localization

`src/shared/i18n/ja.ts`をshapeの基準にし、`en.ts`はその型を満たします。現在のdefault localeは`ja`です。表示文言を追加する場合は両方を更新します。

## Development

Repository rootから実行します。

```sh
pnpm dev
pnpm typecheck
pnpm build
```

Firmware bundleを更新する場合は`pnpm firmware:web`を使います。より詳しい変更別手順は[`docs/development.md`](../../docs/development.md)を参照してください。
