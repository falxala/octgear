# Operations

通常のキーマップ設定、firmware更新、出荷検査、offline rescueの手順です。

## Remapper

RemapperはWebHIDでOctGearへ接続します。Web Serialは使いません。

### Connect And Edit

1. OctGearを通常起動でUSB接続します。
2. `/remapper.html`をChromium系browserで開きます。
3. Connectから`OctGear`を選択します。
4. 接続時に全layer / controlのkeymapが読み込まれます。
5. layerとcontrolを選び、割り当てを編集します。
6. Saveで、最後に読み込んだ内容との差分だけをdeviceへ書き込みます。

Readはdeviceから全keymapを再読込し、未保存の編集内容を置き換えます。Saveは全layer / controlを比較しますが、変更がないassignmentへのFlash writeは行いません。

物理キーやencoderを操作すると、対応するcontrol tileが選択されます。接続中はheartbeatにより通常のKeyboard / Consumer出力がfirmware側で抑止されるため、設定操作がPC入力として流れません。

### Assignment Types

| Type | Behavior |
| --- | --- |
| None | 出力しない |
| Keyboard | modifier + 最大6 keycodesを送る |
| Consumer | volume、media等の16-bit usageを送る |
| Layer Cycle | 通常layerを`0 -> ... -> 5 -> 0`で切り替える |
| Momentary Layer | 押している間だけtarget layerを使う |

## Firmware Update

Remapperのfirmware panelには、BOOTSEL移行、同梱UF2 download、browserからのUF2書込があります。

### Browser Write

1. OctGearへWebHID接続します。
2. BOOTSELへの切替を実行します。
3. OSに`RPI-RP2`またはUF2 bootloader driveが現れるのを待ちます。
4. Browserのfolder pickerでそのdriveを選びます。
5. 同梱`octgear.uf2`の書込完了を待ちます。

Folder pickerによる直接書込はFile System Access APIが必要です。未対応browserではUF2をdownloadし、OS上でbootloader driveへcopyしてください。

WebHIDからBOOTSELへ移行できない場合は、boardのBOOTSEL操作を使って手動でbootloader driveを表示します。

## Diagnostics

`/diagnostics.html`は出荷前・販売前の個体検査用です。

検査項目:

- BrowserのWebHID support
- OctGearへの接続
- 物理キー8個、Encoder CCW / CW / SWのevent
- Vendor reportのrequest / response
- Firmware-reported layer / key count
- Keymap storageのwrite / read / restore

### Recommended Order

1. Connectして、firmwareが`keyCount = 11`を返すことを確認します。
2. 8キーとencoder 3操作をそれぞれ入力し、全controlが記録されることを確認します。
3. Diagnostic report testを実行します。
4. 必要な個体だけStorage testを実行します。
5. Disconnectして通常HID入力へ戻ることを確認します。

Diagnostic report testは固定nonceを送り、`RPT` signature、protocol version、echoを検証します。

> Storage testは実際のFlash-backed keymap保存領域へtest patternを書きます。検証後に元のkeymapを復元しますが、書込回数を伴うため、必要な検査時だけ実行してください。電源断やUSB切断をtest中に行わないでください。

## Rescue Boot

WebHIDを利用できない場合、Key 5 bootでoffline rescueを起動できます。

1. OctGearをUSBから外します。
2. Key 5を押したままUSB接続します。
3. LEDが弱い緑になり、read-onlyの`OCTGEAR` driveが現れるまで待ちます。
4. Windowsでは`RESCUE.CMD`を実行します。
5. `help`でcommand一覧を確認します。
6. 終了後はUSBを抜き、キーを押さずに再接続します。

Driveには`README.TXT`、`READMEEN.TXT`、`REMAPPER.URL`、`RESCUE.CMD`が含まれます。Serialは115200 baud、8-N-1、LF終端です。`RESCUE.CMD`は`OCTGEAR ACK`を返すportを自動検出し、見つからない場合は手動選択へ移ります。

## Serial Rescue Commands

Layerは`0-5`、key番号は`1-11`です。数値はdecimalまたは`0x`付きhexを受け付けます。

| Command | Purpose | Persists |
| --- | --- | --- |
| `probe` | OctGear rescue identityとcountを返す | No |
| `help` / `?` | command一覧 | No |
| `state` | active layerとcountを表示 | No |
| `dump` | 全assignmentを表示 | No |
| `layer <layer>` | active layerを変更 | No |
| `get <layer> <key>` | assignmentを表示 | No |
| `none <layer> <key>` | assignmentを消去 | Yes |
| `key <layer> <key> <modifier> <keycode...>` | Keyboard assignmentを設定 | Yes |
| `consumer <layer> <key> <usage>` | Consumer assignmentを設定 | Yes |
| `cycle <layer> <key>` | Layer Cycleを設定 | Yes |
| `hold <layer> <key> <target>` | Momentary Layerを設定 | Yes |
| `diag` | Storage self-testを実行 | Test writes |
| `bootloader` | UF2 bootloaderへ再起動 | No |

Examples:

```text
state
get 0 1
key 1 1 0x00 0x04
key 1 2 0x01 0x06
consumer 0 1 0x00e2
cycle 0 8
hold 0 5 2
```

Keyboard modifier bitmap:

| Bit | Modifier |
| ---: | --- |
| `0x01` | Left Ctrl |
| `0x02` | Left Shift |
| `0x04` | Left Alt |
| `0x08` | Left GUI |
| `0x10` | Right Ctrl |
| `0x20` | Right Shift |
| `0x40` | Right Alt |
| `0x80` | Right GUI |

`none`、`key`、`consumer`、`cycle`、`hold`は成功時にそのassignmentを即座にFlashへ保存します。`diag`はWeb DiagnosticsのStorage testと同じ保存領域を検査します。

## Troubleshooting

| Symptom | Check |
| --- | --- |
| Device pickerにOctGearがない | 対応browser、HTTPS / localhost、USB cable、VID/PIDを確認 |
| 接続直後に切断表示になる | heartbeat timeout、device再起動、別tabの接続を確認 |
| Remapper接続中に通常キーが出ない | 設計どおり。Disconnectすると通常出力へ戻る |
| Encoder方向またはdetent数が違う | profileの`reversed` / `stepsPerDetent`を確認して再build |
| UF2 driveへ直接書けない | UF2をdownloadしてOSからcopy |
| `OCTGEAR` driveが出ない | USB接続前からKey 5を保持しているか確認 |
| Rescue portを自動検出できない | OSのserial port権限とmanual selectionを確認 |

