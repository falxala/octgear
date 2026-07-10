#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
FQBN="${FQBN:-rp2040:rp2040:waveshare_rp2040_zero}"
USB_VID="${USB_VID:-0xCAFE}"
USB_PID="${USB_PID:-0xC608}"
USB_MANUFACTURER="${USB_MANUFACTURER:-OctGear}"
USB_PRODUCT="${USB_PRODUCT:-OctGear}"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/.arduino/build/octgear}"
EXPORT_DIR="${EXPORT_DIR:-$ROOT_DIR/.arduino/export/octgear}"
PUBLIC_DIR="$ROOT_DIR/apps/web/public/firmware"
SKETCH_DIR="$ROOT_DIR/firmware/octgear/octgear"

mkdir -p "$BUILD_DIR" "$EXPORT_DIR" "$PUBLIC_DIR"

node "$ROOT_DIR/scripts/generate-hardware-config.mjs"
"$ROOT_DIR/scripts/build-rescue-cmd-asset.sh"

"$ROOT_DIR/scripts/arduino-cli.sh" compile \
  --fqbn "$FQBN" \
  --board-options usbstack=tinyusb,freq=125 \
  --build-property "build.usbvid=-DUSBD_VID=$USB_VID -DUSB_VID=$USB_VID" \
  --build-property "build.usbpid=-DUSBD_PID=$USB_PID -DUSB_PID=$USB_PID" \
  --build-property "build.usb_manufacturer=\"$USB_MANUFACTURER\"" \
  --build-property "build.usb_product=\"$USB_PRODUCT\"" \
  --build-path "$BUILD_DIR" \
  --output-dir "$EXPORT_DIR" \
  "$SKETCH_DIR"

cp "$EXPORT_DIR/octgear.ino.uf2" "$PUBLIC_DIR/octgear.uf2"
cp "$SKETCH_DIR/rescue.cmd" "$PUBLIC_DIR/RESCUE.CMD"
