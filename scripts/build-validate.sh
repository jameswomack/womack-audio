#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build"
PLUGINVAL_URL="https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_macOS.zip"
PLUGINVAL_BIN="/tmp/pluginval.app/Contents/MacOS/pluginval"

# ── Build ──────────────────────────────────────────────────────────────────
echo "==> cmake configure"
cmake -B "$BUILD_DIR" -G Xcode -S "$REPO_ROOT" -Wno-dev

echo "==> cmake build (Release)"
cmake --build "$BUILD_DIR" --config Release

# ── Kill AU cache so macOS picks up fresh copy ─────────────────────────────
echo "==> refreshing AU cache"
killall -9 AudioComponentRegistrar 2>/dev/null || true
sleep 2

# ── auval ──────────────────────────────────────────────────────────────────
echo "==> auval"
if auval -v aufx Wfx1 Wmck; then
    echo "[PASS] auval"
else
    echo "[FAIL] auval"
    exit 1
fi

# ── pluginval ─────────────────────────────────────────────────────────────
if [ ! -x "$PLUGINVAL_BIN" ]; then
    echo "==> downloading pluginval"
    curl -fsSL "$PLUGINVAL_URL" -o /tmp/pluginval.zip
    unzip -o /tmp/pluginval.zip -d /tmp/ > /dev/null
    [ -x "$PLUGINVAL_BIN" ] || { echo "[ERROR] pluginval binary not found or not executable after extraction"; exit 1; }
fi

AU_PATH="$HOME/Library/Audio/Plug-Ins/Components/Womack FX.component"
echo "==> pluginval (AU)"
if "$PLUGINVAL_BIN" --strictness-level 5 --validate "$AU_PATH"; then
    echo "[PASS] pluginval AU"
else
    echo "[FAIL] pluginval AU"
    exit 1
fi

VST3_PATH="$HOME/Library/Audio/Plug-Ins/VST3/Womack FX.vst3"
echo "==> pluginval (VST3)"
if "$PLUGINVAL_BIN" --strictness-level 5 --validate "$VST3_PATH"; then
    echo "[PASS] pluginval VST3"
else
    echo "[FAIL] pluginval VST3"
    exit 1
fi

echo ""
echo "✓ All validation checks passed"
