#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
ARDUINO_LOCAL_DIR="$ROOT_DIR/.arduino"

mkdir -p "$ARDUINO_LOCAL_DIR/home" "$ARDUINO_LOCAL_DIR/cache"

export HOME="$ARDUINO_LOCAL_DIR/home"
export XDG_CACHE_HOME="$ARDUINO_LOCAL_DIR/cache"

exec "$ROOT_DIR/.tools/bin/arduino-cli" --config-file "$ROOT_DIR/arduino-cli.yaml" "$@"
