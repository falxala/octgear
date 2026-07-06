# Cyborg Mini 8-Key Firmware

現行8キー版のRP2040ファームウェアです。

Arduinoスケッチ本体は `cyborg_mini_8key/` にあります。

## Current Scope

- 物理キーは8キー
- ロータリーエンコーダは搭載しない
- 外付けRGB LED / OLED は使わない
- 表示は本体LEDのみ。通常のキーボード接続時はレイヤー別固定色、Remapper接続中だけカラーホイールで状態を示す
- 設定通信はSerialではなくUSB HID / WebHID用reportを使う
- USB identity は開発用の `0xCAFE:0xC608` を使う
- WebHIDからUF2ブートローダへ入る更新用reportを持つ
- Remapper接続中は通常のKeyboard / Consumer送信を止める
- Key 5起動時だけ、オフライン救済用の対話式Serialコマンドを有効にする
- Serial rescue / README drive起動中は本体LEDを弱い緑で点灯する
- Diagnostics用のHID report送受信検査とstorage write/read/restore検査を持つ

## キースキャン

- `keyPins[8]` を `INPUT_PULLUP` として読む
- `virtualGroundPins[2]` は常時 `OUTPUT LOW`
- 押下時はDirect入力が `LOW`
- 8キーの状態は1バイトのビットマスクで扱う
- マトリクススキャンは行わない

## Build

リポジトリルートから実行します。

```sh
pnpm firmware:build
pnpm firmware:web
```

既定のFQBNは `rp2040:rp2040:waveshare_rp2040_zero` です。`usbstack=tinyusb,freq=125` を指定し、CPU clockを125MHzに固定します。

`pnpm firmware:web` はWeb配信用の `apps/web/public/firmware/cyborg-mini-8key.uf2` を更新します。
同時に、Windows offline rescue用の `apps/web/public/firmware/RESCUE.CMD` も出力します。

`cyborg_mini_8key/rescue.cmd` を変更した場合は、ファームウェアビルド時に `cyborg_mini_8key/rescue_cmd_asset.h` が再生成されます。手動で生成だけ行う場合は、リポジトリルートで `pnpm rescue-cmd:assets` を実行します。

## Storage

キーマップはRP2040 Arduino coreのEEPROMエミュレーションに保存します。実体は外付けSPI Flashです。

通常運用ではSave時だけ書き込みます。DiagnosticsのStorage testも実際の保存領域へ書きます。全個体検査で必要な回数だけ実行してください。

## 参照元

旧6キー版ファームウェアは `legacy/firmware/cyborg-mini-6key/` にあります。
