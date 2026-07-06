#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
FQBN="${FQBN:-rp2040:rp2040:waveshare_rp2040_zero}"
USB_VID="${USB_VID:-0xCAFE}"
USB_PID="${USB_PID:-0xC608}"
USB_MANUFACTURER="${USB_MANUFACTURER:-OctGear}"
USB_PRODUCT="${USB_PRODUCT:-OctGear}"

"$ROOT_DIR/scripts/build-rescue-cmd-asset.sh"

exec "$ROOT_DIR/scripts/arduino-cli.sh" compile \
  --fqbn "$FQBN" \
  --board-options usbstack=tinyusb,freq=125 \
  --build-property "build.usbvid=-DUSBD_VID=$USB_VID -DUSB_VID=$USB_VID" \
  --build-property "build.usbpid=-DUSBD_PID=$USB_PID -DUSB_PID=$USB_PID" \
  --build-property "build.usb_manufacturer=\"$USB_MANUFACTURER\"" \
  --build-property "build.usb_product=\"$USB_PRODUCT\"" \
  "$ROOT_DIR/firmware/cyborg-mini-8key/cyborg_mini_8key"
