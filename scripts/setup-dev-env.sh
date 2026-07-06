#!/usr/bin/env sh
set -eu

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
ARDUINO_CLI_VERSION="${ARDUINO_CLI_VERSION:-1.5.1}"
RP2040_CORE_VERSION="${RP2040_CORE_VERSION:-5.6.1}"
VERIFY=false
SKIP_PNPM=false

usage() {
  cat <<'EOF'
Usage: scripts/setup-dev-env.sh [--verify] [--skip-pnpm]

Sets up the local development environment:
  - installs Arduino CLI into .tools/bin/
  - installs the RP2040 Arduino core into .arduino/
  - installs firmware Arduino libraries into .arduino/
  - installs pnpm workspace dependencies

Options:
  --verify     run typecheck, web build, and firmware web build after setup
  --skip-pnpm  skip pnpm install

Environment overrides:
  ARDUINO_CLI_VERSION  default: 1.5.1
  RP2040_CORE_VERSION  default: 5.6.1
EOF
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --verify)
      VERIFY=true
      ;;
    --skip-pnpm)
      SKIP_PNPM=true
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

need_command() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required command: $1" >&2
    exit 1
  fi
}

install_arduino_cli() {
  if [ -x "$ROOT_DIR/.tools/bin/arduino-cli" ]; then
    "$ROOT_DIR/.tools/bin/arduino-cli" version
    return
  fi

  need_command curl
  need_command tar

  os="$(uname -s)"
  arch="$(uname -m)"
  case "$os:$arch" in
    Linux:x86_64|Linux:amd64)
      asset_os_arch="Linux_64bit"
      ;;
    Linux:aarch64|Linux:arm64)
      asset_os_arch="Linux_ARM64"
      ;;
    Darwin:x86_64|Darwin:amd64)
      asset_os_arch="macOS_64bit"
      ;;
    Darwin:aarch64|Darwin:arm64)
      asset_os_arch="macOS_ARM64"
      ;;
    *)
      echo "Unsupported platform for automatic Arduino CLI install: $os $arch" >&2
      echo "Install arduino-cli manually at .tools/bin/arduino-cli, then rerun this script." >&2
      exit 1
      ;;
  esac

  url="https://downloads.arduino.cc/arduino-cli/arduino-cli_${ARDUINO_CLI_VERSION}_${asset_os_arch}.tar.gz"
  tmp_dir="$(mktemp -d)"
  trap 'rm -rf "$tmp_dir"' EXIT INT TERM

  echo "Downloading Arduino CLI $ARDUINO_CLI_VERSION..."
  curl -fsSL "$url" -o "$tmp_dir/arduino-cli.tar.gz"
  tar -xzf "$tmp_dir/arduino-cli.tar.gz" -C "$tmp_dir"

  mkdir -p "$ROOT_DIR/.tools/bin"
  cp "$tmp_dir/arduino-cli" "$ROOT_DIR/.tools/bin/arduino-cli"
  chmod +x "$ROOT_DIR/.tools/bin/arduino-cli"
  "$ROOT_DIR/.tools/bin/arduino-cli" version
}

install_firmware_dependencies() {
  need_command xxd

  "$ROOT_DIR/scripts/arduino-cli.sh" core update-index
  "$ROOT_DIR/scripts/arduino-cli.sh" core install "rp2040:rp2040@$RP2040_CORE_VERSION"
  "$ROOT_DIR/scripts/arduino-cli.sh" lib install "Adafruit TinyUSB Library@3.7.7"
  "$ROOT_DIR/scripts/arduino-cli.sh" lib install "Adafruit SPIFlash@5.1.1"
  "$ROOT_DIR/scripts/arduino-cli.sh" lib install "Adafruit NeoPixel@1.15.5"
  "$ROOT_DIR/scripts/arduino-cli.sh" lib install "SdFat - Adafruit Fork@2.3.103"
  "$ROOT_DIR/scripts/arduino-cli.sh" lib install "MIDI Library@5.0.2"
}

install_web_dependencies() {
  if [ "$SKIP_PNPM" = true ]; then
    return
  fi

  if ! command -v pnpm >/dev/null 2>&1; then
    if command -v corepack >/dev/null 2>&1; then
      corepack enable
    fi
  fi
  need_command pnpm

  (cd "$ROOT_DIR" && pnpm install --frozen-lockfile)
}

run_verification() {
  if [ "$VERIFY" != true ]; then
    return
  fi

  (cd "$ROOT_DIR" && pnpm typecheck)
  (cd "$ROOT_DIR" && pnpm build)
  (cd "$ROOT_DIR" && pnpm firmware:web)
}

install_arduino_cli
install_firmware_dependencies
install_web_dependencies
run_verification

echo "Development environment setup complete."
