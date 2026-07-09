#!/usr/bin/env bash
# Builds, packages, signs, and notarizes the Womack Audio plugin suite
# (Womack FX + Womack Resonote).
# Required env vars: APPLE_ID, APPLE_TEAM_ID, APPLE_APP_PASSWORD,
#                    INSTALLER_CERT_NAME, APP_CERT_NAME
# Usage: scripts/package.sh [version]
# Output: dist/WomackAudio-<version>-installer.pkg

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

# ── 1. Build Release (builds every plugin in the suite) ────────────────────
echo "==> Building Release..."
cmake -B "$BUILD_DIR" -G Xcode -S "$REPO_ROOT" -Wno-dev
cmake --build "$BUILD_DIR" --config Release

# ── helper: sign a built plugin bundle and build its component .pkg ────────
# args: <product name> <AU|VST3> <pkg identifier> <output pkg path>
build_component_pkg () {
    local product_name="$1" kind="$2" identifier="$3" out_pkg="$4"
    local src install_loc

    if [ "$kind" = "AU" ]; then
        src="$HOME/Library/Audio/Plug-Ins/Components/${product_name}.component"
        install_loc="/Library/Audio/Plug-Ins/Components"
    else
        src="$HOME/Library/Audio/Plug-Ins/VST3/${product_name}.vst3"
        install_loc="/Library/Audio/Plug-Ins/VST3"
    fi

    if [ ! -d "$src" ]; then
        echo "Error: ${kind} bundle not found at $src"
        exit 1
    fi

    echo "==> Signing ${product_name} ${kind}..."
    codesign --force --deep --options runtime --sign "${APP_CERT_NAME}" "$src"

    echo "==> Building ${product_name} ${kind} package..."
    pkgbuild \
        --component "$src" \
        --install-location "$install_loc" \
        --identifier "$identifier" \
        --version "$VERSION" \
        --sign "${INSTALLER_CERT_NAME}" \
        "$out_pkg"
}

# ── 2-4. Sign + package each plugin/format. The staging pkg filenames must
#         match the <pkg-ref> entries in distribution/distribution.xml. ─────
build_component_pkg "Womack FX"       "AU"   "com.womack.womackfx.au"   "$STAGING_DIR/WomackFX-AU.pkg"
build_component_pkg "Womack FX"       "VST3" "com.womack.womackfx.vst3" "$STAGING_DIR/WomackFX-VST3.pkg"
build_component_pkg "Womack Resonote" "AU"   "com.womack.resonote.au"   "$STAGING_DIR/Resonote-AU.pkg"
build_component_pkg "Womack Resonote" "VST3" "com.womack.resonote.vst3" "$STAGING_DIR/Resonote-VST3.pkg"

# ── 5. Build distribution installer ──────────────────────────────────────
INSTALLER="$DIST_DIR/WomackAudio-${VERSION}-installer.pkg"
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
