#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
GENERATOR="${GENERATOR:-Xcode}"
CONFIG="${CONFIG:-Release}"
COMMAND="${1:-all}"

configure() {
  cmake -B "$BUILD_DIR" -G "$GENERATOR" "$ROOT_DIR"
}

build_targets() {
  cmake --build "$BUILD_DIR" --config "$CONFIG" --target "$@"
}

case "$COMMAND" in
  configure)
    configure
    ;;
  all)
    configure
    build_targets WomackFX_AU WomackFX_VST3 WomackFX_Standalone
    ;;
  au)
    configure
    build_targets WomackFX_AU
    ;;
  vst3)
    configure
    build_targets WomackFX_VST3
    ;;
  standalone)
    configure
    build_targets WomackFX_Standalone
    ;;
  *)
    echo "Usage: $0 [configure|all|au|vst3|standalone]" >&2
    exit 1
    ;;
esac