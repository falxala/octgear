#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
SKETCH_DIR="$ROOT_DIR/firmware/octgear/octgear"
SRC_FILE="$SKETCH_DIR/rescue.cmd"
OUT_FILE="$SKETCH_DIR/rescue_cmd_asset.h"
TMP_FILE="$OUT_FILE.tmp"

{
  printf '#pragma once\n\n'
  printf '#include <Arduino.h>\n\n'
  printf 'namespace RescueCmdAsset {\n\n'
  xxd -i -n RESCUE_CMD_TEXT "$SRC_FILE" | sed 's/^unsigned char /const uint8_t /; s/^unsigned int /const uint32_t /'
  printf '\nconstexpr uint32_t RESCUE_CMD_TEXT_SIZE = RESCUE_CMD_TEXT_len;\n\n'
  printf '}  // namespace RescueCmdAsset\n'
} > "$TMP_FILE"

mv "$TMP_FILE" "$OUT_FILE"
