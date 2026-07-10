# HID Report Protocol

WebHID版リマッパーで使う固定長の設定用reportです。

通常のキーボード/Consumer Control HID reportとは別に、vendor-defined reportを1つ持ちます。

## Report

| Field | Value |
| --- | --- |
| Report ID | `10` |
| Direction | Web -> device: Output report |
| Direction | Device -> Web: Input report |
| Size | `32 bytes` |

WebHIDの `sendReport(reportId, data)` では、`reportId` は引数で渡すため、`data[0]` はcommandです。

## Request Layout

```text
byte 0      command
byte 1..31  command payload
```

## Response Layout

```text
byte 0      command
byte 1      status
byte 2      payload length
byte 3..31  response payload
```

## Status

| Value | Name | Meaning |
| ---: | --- | --- |
| `0x00` | `Ok` | Success |
| `0x01` | `InvalidLength` | Request payload is too short |
| `0x02` | `OutOfRange` | Layer or key index is invalid |
| `0x03` | `StorageError` | Device could not persist the updated assignment |
| `0x04` | `Unsupported` | Command is not supported on this MCU/firmware build |
| `0xff` | `UnknownCommand` | Command is not supported |

## Commands

| Value | Name | Request payload | Response payload |
| ---: | --- | --- | --- |
| `0x01` | `GetState` | none | `activeLayer, layerCount, keyCount, virtualGroundCount` |
| `0x02` | `SetLayer` | `layer` | `layer` |
| `0x03` | `GetKey` | `layer, keyIndex` | key assignment payload |
| `0x04` | `SetKey` | key assignment payload | `layer, keyIndex` |
| `0x05` | `EnterBootloader` | none | none, then reboot to UF2 bootloader |
| `0x06` | `RemapperHeartbeat` | none | no response |
| `0x07` | `KeyEvent` | not supported | `layer, keyIndex, pressed` |
| `0x08` | `DiagnosticReport` | `0x43, 0x59, 0x42, 0x38` | `0x52, 0x50, 0x54, version, echoed request bytes[4]` |
| `0x09` | `DiagnosticStorage` | none | `ok, layerCount, keyCount` |

`KeyEvent` is an asynchronous device-to-Web input report. The firmware emits it when a physical key, encoder rotation, or encoder switch press is detected while the remapper heartbeat is active, so the UI can select the matching tile.

`RemapperHeartbeat` is sent periodically by Remapper and Diagnostics. While the heartbeat is active, normal keyboard / consumer output is suppressed by firmware.

`DiagnosticReport` is a synchronous send/receive self-test for Diagnostics. The Web UI sends a fixed nonce and verifies that the firmware returns the `RPT` signature plus the same nonce.

`DiagnosticStorage` writes a test pattern across the keymap storage area, reads it back, verifies it, and restores the original keymap. It is intended for production inspection of the same flash-backed storage used by normal remapping. Run it only when needed because it writes the external SPI Flash through EEPROM emulation.

## Key Indexes

Current firmware reports `keyCount = 11`.

| Index | Control |
| ---: | --- |
| `0..7` | Physical keys 1-8 |
| `8` | Encoder CCW rotation |
| `9` | Encoder CW rotation |
| `10` | Encoder SW |

## Key Assignment Payload

`GetKey` response and `SetKey` request use the same assignment shape.

```text
byte 0      layer
byte 1      keyIndex
byte 2      kind
byte 3      modifier
byte 4..9   keyboard keycodes[6]
byte 10     consumer usage low byte
byte 11     consumer usage high byte
byte 12     target layer
```

`target layer` is used when `kind` is `MomentaryLayer`; otherwise it is `0`.

`kind` values:

| Value | Name |
| ---: | --- |
| `0` | None |
| `1` | Keyboard |
| `2` | Consumer |
| `3` | LayerCycle |
| `4` | MomentaryLayer |
