#!/usr/bin/env bash
# Builds, packages, signs, and notarizes Womack FX.
# Required env vars: APPLE_ID, APPLE_TEAM_ID, APPLE_APP_PASSWORD,
#                    INSTALLER_CERT_NAME, APP_CERT_NAME
# Usage: scripts/package.sh [version]
# Output: dist/WomackFX-<version>-installer.pkg

set -euo pipefail

VERSION="${1:-1.0.0}"
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DIST_DIR="$REPO_ROOT/dist"
STAGING_DIR="$DIST_DIR/staging"
BUILD_DIR="$REPO_ROOT/build"

mkdir -p "$DIST_DIR" "$STAGING_DIR"

# ── 0. Preflight: validate required env vars ───────────────────────────────
for var in APPLE_ID APPLE_TEAM_ID APPLE_APP_PASSWORD INSTALLER_CERT_NAME APP_CERT_NAME; do
    if [ -z "${!var:-}" ]; then
        echo "Error: required environment variable $var is not set"
        exit 1
    fi
done

# ── 1. Build Release ───────────────────────────────────────────────────────
echo "==> Building Release..."
cmake -B "$BUILD_DIR" -G Xcode -S "$REPO_ROOT" -Wno-dev
cmake --build "$BUILD_DIR" --config Release

# ── 2. Locate build artifacts ─────────────────────────────────────────────
AU_SRC="$HOME/Library/Audio/Plug-Ins/Components/Womack FX.component"
VST3_SRC="$HOME/Library/Audio/Plug-Ins/VST3/Womack FX.vst3"

if [ ! -d "$AU_SRC" ]; then
    echo "Error: AU component not found at $AU_SRC"
    exit 1
fi

if [ ! -d "$VST3_SRC" ]; then
    echo "Error: VST3 plugin not found at $VST3_SRC"
    exit 1
fi

# ── 3. Sign the plugins ────────────────────────────────────────────────────
echo "==> Signing AU..."
codesign --force --deep --options runtime \
    --sign "${APP_CERT_NAME}" \
    "$AU_SRC"

echo "==> Signing VST3..."
codesign --force --deep --options runtime \
    --sign "${APP_CERT_NAME}" \
    "$VST3_SRC"

# ── 4. Build component packages ───────────────────────────────────────────
echo "==> Building AU package..."
pkgbuild \
    --component "$AU_SRC" \
    --install-location "/Library/Audio/Plug-Ins/Components" \
    --identifier "com.womack.womackfx.au" \
    --version "$VERSION" \
    --sign "${INSTALLER_CERT_NAME}" \
    "$STAGING_DIR/WomackFX-AU.pkg"

echo "==> Building VST3 package..."
pkgbuild \
    --component "$VST3_SRC" \
    --install-location "/Library/Audio/Plug-Ins/VST3" \
    --identifier "com.womack.womackfx.vst3" \
    --version "$VERSION" \
    --sign "${INSTALLER_CERT_NAME}" \
    "$STAGING_DIR/WomackFX-VST3.pkg"

# ── 5. Build distribution installer ──────────────────────────────────────
INSTALLER="$DIST_DIR/WomackFX-${VERSION}-installer.pkg"
echo "==> Building distribution installer..."
productbuild \
    --distribution "$REPO_ROOT/distribution/distribution.xml" \
    --resources "$REPO_ROOT/distribution" \
    --package-path "$STAGING_DIR" \
    --sign "${INSTALLER_CERT_NAME}" \
    "$INSTALLER"

# ── 6. Notarize ───────────────────────────────────────────────────────────
echo "==> Submitting for notarization (this takes 1-5 minutes)..."
xcrun notarytool submit "$INSTALLER" \
    --apple-id "${APPLE_ID}" \
    --team-id "${APPLE_TEAM_ID}" \
    --password "${APPLE_APP_PASSWORD}" \
    --wait

# ── 7. Staple ─────────────────────────────────────────────────────────────
echo "==> Stapling notarization ticket..."
xcrun stapler staple "$INSTALLER"

echo ""
echo "✓ Installer ready: $INSTALLER"
