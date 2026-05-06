# Workflow Tooling Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add five independent workflow tools to womack-audio: a build+validate script, factory preset files, demo video recording scripts, a macOS installer+notarization pipeline, and a GitHub Actions CI workflow.

**Architecture:** All five subsystems are shell scripts, YAML, or static files — no C++ changes required. They operate on the existing build artifacts (AU/VST3 in `~/Library/Audio/Plug-Ins/`) and the existing CMake build system. Each subsystem is independently usable.

**Tech Stack:** bash, cmake, xcodebuild, auval, pluginval, pkgbuild, productbuild, codesign, xcrun notarytool, ffmpeg, GitHub Actions (macos-14 runner)

---

## Files Created / Modified

| File | Purpose |
|---|---|
| `scripts/build-validate.sh` | Build Release → auval → pluginval → report |
| `scripts/package.sh` | pkgbuild → productbuild → codesign → notarytool → staple |
| `scripts/record-demo.sh` | Start ffmpeg screen+audio capture |
| `scripts/compress-demo.sh` | Compress raw recording to web-ready mp4 |
| `presets/factory/01-Classic-Fuzz.xml` | Factory preset: fuzz only |
| `presets/factory/02-Slow-Vibe.xml` | Factory preset: univibe only |
| `presets/factory/03-Tape-Echo.xml` | Factory preset: tape delay only |
| `presets/factory/04-Full-Chain.xml` | Factory preset: all three effects |
| `presets/README.md` | How to install and use presets |
| `distribution/distribution.xml` | productbuild installer definition |
| `distribution/welcome.html` | Installer welcome text |
| `.github/workflows/build.yml` | CI: build + validate on every push/PR |

---

## Task 1: build-and-validate script

**Files:**
- Create: `scripts/build-validate.sh`

- [ ] **Step 1: Create the script**

```bash
cat > /path/to/worktree/scripts/build-validate.sh << 'SCRIPT'
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
cmake --build "$BUILD_DIR" --config Release 2>&1

# ── Kill AU cache so macOS picks up fresh copy ─────────────────────────────
echo "==> refreshing AU cache"
killall -9 AudioComponentRegistrar 2>/dev/null || true
sleep 1

# ── auval ──────────────────────────────────────────────────────────────────
echo "==> auval"
if auval -v aufx Wfx1 Wmck; then
    echo "[PASS] auval"
else
    echo "[FAIL] auval"
    exit 1
fi

# ── pluginval ─────────────────────────────────────────────────────────────
if [ ! -f "$PLUGINVAL_BIN" ]; then
    echo "==> downloading pluginval"
    curl -fsSL "$PLUGINVAL_URL" -o /tmp/pluginval.zip
    unzip -o /tmp/pluginval.zip -d /tmp/ > /dev/null
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
SCRIPT
chmod +x /path/to/worktree/scripts/build-validate.sh
```

> Replace `/path/to/worktree` with the actual worktree path. In the worktree, the script lives at `scripts/build-validate.sh`.

- [ ] **Step 2: Run the script to verify it works end-to-end**

```bash
cd /Users/jameswomack/Projects/github/jameswomack/womack-audio-worktrees/workflow-tooling
bash scripts/build-validate.sh
```

Expected: Build output, then `[PASS] auval`, `[PASS] pluginval AU`, `[PASS] pluginval VST3`, then `✓ All validation checks passed`.

If auval fails with `Component not found`, run `killall -9 AudioComponentRegistrar` manually, wait 2 seconds, and retry.

If pluginval download fails (network), download manually from https://github.com/Tracktion/pluginval/releases and place at `/tmp/pluginval.app/Contents/MacOS/pluginval`.

- [ ] **Step 3: Commit**

```bash
cd /Users/jameswomack/Projects/github/jameswomack/womack-audio-worktrees/workflow-tooling
git add scripts/build-validate.sh
git commit -m "feat(scripts): add build-validate.sh — cmake build + auval + pluginval"
```

---

## Task 2: Factory preset files

**Background:** JUCE's `AudioProcessorValueTreeState` stores state as an XML element named `Parameters` with one attribute per parameter. The format matches exactly what `WomackFXProcessor::getStateInformation` writes. These `.xml` files can be loaded by dragging onto the plugin UI (future feature) or distributed for users to import manually via Logic Pro's "Import Settings" workflow.

**Files:**
- Create: `presets/factory/01-Classic-Fuzz.xml`
- Create: `presets/factory/02-Slow-Vibe.xml`
- Create: `presets/factory/03-Tape-Echo.xml`
- Create: `presets/factory/04-Full-Chain.xml`
- Create: `presets/README.md`

**Parameter reference** (from `PluginProcessor.cpp`):
| ParamID | Range | Default |
|---|---|---|
| fuzzEnabled | bool | false |
| fuzzDrive | 1.0–20.0 | 5.0 |
| fuzzTone | 200–20000 | 4000 |
| fuzzMix | 0–1 | 1.0 |
| fuzzType | 0=Tanh, 1=Hard Clip, 2=Asymmetric | 0 |
| vibeEnabled | bool | false |
| vibeRate | 0.1–10 | 2.0 |
| vibeDepth | 0–1 | 0.5 |
| vibeFeedback | -1–1 | 0.3 |
| vibeMix | 0–1 | 0.5 |
| delayEnabled | bool | false |
| delayTime | 10–2000 ms | 350 |
| delayFeedback | 0–0.95 | 0.4 |
| delayMod | 0–1 | 0.3 |
| delaySat | 0–1 | 0.3 |
| delayMix | 0–1 | 0.5 |

- [ ] **Step 1: Create 01-Classic-Fuzz.xml** (fuzz only, hard clip type, high drive)

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Parameters fuzzEnabled="1" fuzzDrive="12.0" fuzzTone="6000.0" fuzzMix="1.0" fuzzType="1"
            vibeEnabled="0" vibeRate="2.0" vibeDepth="0.5" vibeFeedback="0.3" vibeMix="0.5"
            delayEnabled="0" delayTime="350.0" delayFeedback="0.4" delayMod="0.3" delaySat="0.3" delayMix="0.5"/>
```

Save to `presets/factory/01-Classic-Fuzz.xml`.

- [ ] **Step 2: Create 02-Slow-Vibe.xml** (univibe only, slow dreamy sweep)

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Parameters fuzzEnabled="0" fuzzDrive="5.0" fuzzTone="4000.0" fuzzMix="1.0" fuzzType="0"
            vibeEnabled="1" vibeRate="0.4" vibeDepth="0.85" vibeFeedback="0.5" vibeMix="0.9"
            delayEnabled="0" delayTime="350.0" delayFeedback="0.4" delayMod="0.3" delaySat="0.3" delayMix="0.5"/>
```

Save to `presets/factory/02-Slow-Vibe.xml`.

- [ ] **Step 3: Create 03-Tape-Echo.xml** (tape delay only, slap-back echo)

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Parameters fuzzEnabled="0" fuzzDrive="5.0" fuzzTone="4000.0" fuzzMix="1.0" fuzzType="0"
            vibeEnabled="0" vibeRate="2.0" vibeDepth="0.5" vibeFeedback="0.3" vibeMix="0.5"
            delayEnabled="1" delayTime="120.0" delayFeedback="0.25" delayMod="0.2" delaySat="0.5" delayMix="0.6"/>
```

Save to `presets/factory/03-Tape-Echo.xml`.

- [ ] **Step 4: Create 04-Full-Chain.xml** (all three effects together)

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Parameters fuzzEnabled="1" fuzzDrive="7.0" fuzzTone="5000.0" fuzzMix="0.8" fuzzType="2"
            vibeEnabled="1" vibeRate="1.2" vibeDepth="0.6" vibeFeedback="0.4" vibeMix="0.7"
            delayEnabled="1" delayTime="430.0" delayFeedback="0.35" delayMod="0.4" delaySat="0.4" delayMix="0.45"/>
```

Save to `presets/factory/04-Full-Chain.xml`.

- [ ] **Step 5: Create presets/README.md**

```markdown
# Factory Presets

Four factory presets for Womack FX in JUCE APVTS XML format.

## Installing in Logic Pro

1. Open Logic Pro with Womack FX inserted on a track.
2. In the plugin window, click the preset menu (top center of the AU window).
3. Choose **"Import Settings…"** and navigate to a `.xml` file from this folder.

## Manual parameter import

If Logic Pro does not show an import option, use the Standalone app:
1. Open `Womack FX.app` (in `/Applications` or the build folder).
2. Right-click the plugin area → **Load state from XML file** (future: add this button to the UI).

## Format

Each file is the raw APVTS state XML. The root element is `<Parameters>` with one attribute per parameter ID matching `Source/Parameters.h`.
```

- [ ] **Step 6: Commit**

```bash
cd /Users/jameswomack/Projects/github/jameswomack/womack-audio-worktrees/workflow-tooling
git add presets/
git commit -m "feat(presets): add four factory presets in APVTS XML format"
```

---

## Task 3: Demo video tooling

**Context:** Uses `ffmpeg` and macOS's `screencapture`. The record script starts a background ffmpeg capture of the primary display plus system audio (via BlackHole or Soundflower virtual audio device). The compress script converts the raw recording to a web-ready H.264 mp4 at 2 Mbit/s.

**Prerequisite:** `ffmpeg` installed: `brew install ffmpeg`. For system audio capture: `brew install blackhole-2ch` and configure a Multi-Output Device in Audio MIDI Setup.

**Files:**
- Create: `scripts/record-demo.sh`
- Create: `scripts/compress-demo.sh`

- [ ] **Step 1: Create scripts/record-demo.sh**

```bash
#!/usr/bin/env bash
# Records primary display + system audio (via BlackHole virtual device).
# Usage: scripts/record-demo.sh [output-name]
# Output: recordings/<name>-<timestamp>.mkv
# Stop: press Ctrl+C or kill the ffmpeg process (its PID is printed).

set -euo pipefail

NAME="${1:-demo}"
TIMESTAMP="$(date +%Y%m%d-%H%M%S)"
RECORDINGS_DIR="$(cd "$(dirname "$0")/.." && pwd)/recordings"
mkdir -p "$RECORDINGS_DIR"
OUTPUT="$RECORDINGS_DIR/${NAME}-${TIMESTAMP}.mkv"

# Display index (0 = primary). Adjust if you have multiple monitors.
DISPLAY_IDX=1

# Audio device — must match the device name in Audio MIDI Setup.
# After installing blackhole-2ch, create a Multi-Output Device named "BlackHole 2ch".
AUDIO_DEVICE="BlackHole 2ch"

echo "Recording to: $OUTPUT"
echo "Press Ctrl+C to stop."

ffmpeg \
  -f avfoundation \
  -framerate 30 \
  -capture_cursor 1 \
  -capture_mouse_clicks 1 \
  -i "${DISPLAY_IDX}:${AUDIO_DEVICE}" \
  -c:v libx264 -preset ultrafast -crf 0 \
  -c:a pcm_s16le \
  "$OUTPUT" &

FFMPEG_PID=$!
echo "ffmpeg PID: $FFMPEG_PID"
wait $FFMPEG_PID
echo "Saved: $OUTPUT"
```

Save to `scripts/record-demo.sh`, then `chmod +x scripts/record-demo.sh`.

- [ ] **Step 2: Create scripts/compress-demo.sh**

```bash
#!/usr/bin/env bash
# Compresses a raw recording to a web-ready H.264 mp4.
# Usage: scripts/compress-demo.sh <input.mkv> [output.mp4]
# Output defaults to same directory as input with .mp4 extension.

set -euo pipefail

INPUT="${1:?Usage: compress-demo.sh <input.mkv> [output.mp4]}"
OUTPUT="${2:-${INPUT%.*}.mp4}"

if [ ! -f "$INPUT" ]; then
    echo "Error: input file not found: $INPUT"
    exit 1
fi

echo "Compressing: $INPUT -> $OUTPUT"

ffmpeg -i "$INPUT" \
  -c:v libx264 \
  -preset slow \
  -crf 23 \
  -vf "scale=trunc(iw/2)*2:trunc(ih/2)*2" \
  -c:a aac \
  -b:a 192k \
  -movflags +faststart \
  "$OUTPUT"

SIZE=$(du -sh "$OUTPUT" | cut -f1)
echo "Done: $OUTPUT ($SIZE)"
```

Save to `scripts/compress-demo.sh`, then `chmod +x scripts/compress-demo.sh`.

- [ ] **Step 3: Verify ffmpeg is available and scripts are executable**

```bash
which ffmpeg || echo "Install with: brew install ffmpeg"
ls -la scripts/record-demo.sh scripts/compress-demo.sh
```

Expected: both scripts listed with `x` permission bits.

- [ ] **Step 4: Test compress script on any existing video file**

```bash
# Create a 3-second test clip to verify ffmpeg works
ffmpeg -f lavfi -i testsrc=duration=3:size=1280x720:rate=30 /tmp/test-input.mkv -y
bash scripts/compress-demo.sh /tmp/test-input.mkv /tmp/test-output.mp4
ls -lh /tmp/test-output.mp4
```

Expected: `/tmp/test-output.mp4` created, size in kilobytes.

- [ ] **Step 5: Add recordings/ to .gitignore**

Append to the repo's `.gitignore`:
```
recordings/
```

- [ ] **Step 6: Commit**

```bash
cd /Users/jameswomack/Projects/github/jameswomack/womack-audio-worktrees/workflow-tooling
git add scripts/record-demo.sh scripts/compress-demo.sh .gitignore
git commit -m "feat(scripts): add demo recording and compression scripts (ffmpeg)"
```

---

## Task 4: Package + notarization pipeline

**Context:** Creates a signed macOS installer (`.pkg`) that installs the AU component and VST3 to `/Library/Audio/Plug-Ins/` (system-wide). Requires an Apple Developer account with a valid "Developer ID Installer" certificate in Keychain. Notarization uses `xcrun notarytool` (Xcode 13+).

**Required environment variables** (set in shell or CI secrets):
- `APPLE_ID` — developer Apple ID email
- `APPLE_TEAM_ID` — 10-char team ID from developer.apple.com
- `APPLE_APP_PASSWORD` — app-specific password generated at appleid.apple.com
- `INSTALLER_CERT_NAME` — e.g. `"Developer ID Installer: James Womack (XXXXXXXXXX)"`
- `APP_CERT_NAME` — e.g. `"Developer ID Application: James Womack (XXXXXXXXXX)"`

**Files:**
- Create: `distribution/distribution.xml`
- Create: `distribution/welcome.html`
- Create: `scripts/package.sh`

- [ ] **Step 1: Create distribution/distribution.xml**

```xml
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>Womack FX</title>
    <welcome file="welcome.html" mime-type="text/html"/>
    <license file="license.txt" mime-type="text/plain"/>
    <options customize="never" require-scripts="false" rootVolumeOnly="true"/>
    <domains enable_localSystem="true" enable_userHome="false"/>
    <pkg-ref id="com.womack.womackfx.au" version="1.0.0" auth="Root">WomackFX-AU.pkg</pkg-ref>
    <pkg-ref id="com.womack.womackfx.vst3" version="1.0.0" auth="Root">WomackFX-VST3.pkg</pkg-ref>
    <choices-outline>
        <line choice="com.womack.womackfx.au"/>
        <line choice="com.womack.womackfx.vst3"/>
    </choices-outline>
    <choice id="com.womack.womackfx.au" title="Womack FX AU" description="Audio Unit for Logic Pro and GarageBand" selected="true">
        <pkg-ref id="com.womack.womackfx.au"/>
    </choice>
    <choice id="com.womack.womackfx.vst3" title="Womack FX VST3" description="VST3 for Ableton Live, Reaper, and others" selected="true">
        <pkg-ref id="com.womack.womackfx.vst3"/>
    </choice>
</installer-gui-script>
```

- [ ] **Step 2: Create distribution/welcome.html**

```html
<!DOCTYPE html>
<html>
<body style="font-family: -apple-system, sans-serif; padding: 20px;">
<h2>Womack FX</h2>
<p>This installer will install Womack FX as an Audio Unit (AU) and/or VST3 plugin.</p>
<p>After installation, restart your DAW. In Logic Pro, Womack FX appears under
<strong>Audio FX → Womack → Womack FX</strong>.</p>
<p>System requirements: macOS 12 or later, 64-bit DAW.</p>
</body>
</html>
```

- [ ] **Step 3: Create scripts/package.sh**

```bash
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
```

Save to `scripts/package.sh`, then `chmod +x scripts/package.sh`.

- [ ] **Step 4: Dry-run the script without signing (verify logic)**

```bash
# Check the script parses without errors
bash -n scripts/package.sh && echo "Syntax OK"

# Verify required tools exist
for tool in cmake pkgbuild productbuild codesign; do
    which $tool && echo "$tool: OK" || echo "$tool: MISSING"
done
xcrun notarytool --version && echo "notarytool: OK"
```

Expected: All tools found. `notarytool` reports a version string.

- [ ] **Step 5: Add dist/ to .gitignore**

Append to `.gitignore`:
```
dist/
```

- [ ] **Step 6: Commit**

```bash
cd /Users/jameswomack/Projects/github/jameswomack/womack-audio-worktrees/workflow-tooling
git add scripts/package.sh distribution/ .gitignore
git commit -m "feat(scripts): add macOS installer + notarization pipeline"
```

---

## Task 5: GitHub Actions CI

**Context:** macOS-14 runners have Xcode 15+ and support `auval`. JUCE is a git submodule — the checkout action must use `submodules: recursive`. We skip code signing in CI (ad-hoc signing only). pluginval is downloaded and cached per run. The build is cached by hashing `CMakeLists.txt` and `JUCE/` commit SHA.

**Note on cost:** GitHub-hosted `macos-14` runners cost ~10× more per minute than Linux. Each full build takes ~8-12 minutes. For a small project, this is fine; for heavy iteration, consider only running CI on PRs and pushes to `main`.

**Files:**
- Create: `.github/workflows/build.yml`

- [ ] **Step 1: Create .github/workflows/build.yml**

```yaml
name: Build & Validate

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  build:
    name: Build and Validate (macOS)
    runs-on: macos-14

    steps:
      - name: Checkout (with JUCE submodule)
        uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Cache CMake build
        uses: actions/cache@v4
        with:
          path: build
          key: cmake-${{ runner.os }}-${{ hashFiles('CMakeLists.txt', 'plugins/**/CMakeLists.txt') }}-${{ hashFiles('.gitmodules') }}
          restore-keys: |
            cmake-${{ runner.os }}-

      - name: Configure CMake
        run: cmake -B build -G Xcode -Wno-dev

      - name: Build Release
        run: cmake --build build --config Release

      - name: Refresh AU registry
        run: |
          killall -9 AudioComponentRegistrar 2>/dev/null || true
          sleep 2

      - name: Validate with auval
        run: auval -v aufx Wfx1 Wmck

      - name: Download pluginval
        run: |
          curl -fsSL https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_macOS.zip \
            -o /tmp/pluginval.zip
          unzip -o /tmp/pluginval.zip -d /tmp/

      - name: Validate AU with pluginval
        run: |
          /tmp/pluginval.app/Contents/MacOS/pluginval \
            --strictness-level 5 \
            --validate "$HOME/Library/Audio/Plug-Ins/Components/Womack FX.component"

      - name: Validate VST3 with pluginval
        run: |
          /tmp/pluginval.app/Contents/MacOS/pluginval \
            --strictness-level 5 \
            --validate "$HOME/Library/Audio/Plug-Ins/VST3/Womack FX.vst3"

      - name: Upload build artifacts
        uses: actions/upload-artifact@v4
        if: success()
        with:
          name: WomackFX-plugins
          path: |
            ~/Library/Audio/Plug-Ins/Components/Womack FX.component
            ~/Library/Audio/Plug-Ins/VST3/Womack FX.vst3
          retention-days: 7
```

- [ ] **Step 2: Validate YAML syntax locally**

```bash
# Check syntax with Python (available on macOS without install)
python3 -c "
import yaml, sys
with open('.github/workflows/build.yml') as f:
    yaml.safe_load(f)
print('YAML syntax OK')
"
```

Expected: `YAML syntax OK`

- [ ] **Step 3: Push branch and verify CI runs**

```bash
# From the main repo root (not worktree), after merging:
# Push and open the Actions tab on GitHub
git push origin workflow-tooling
# Then: https://github.com/jameswomack/womack-audio/actions
```

Expected: A new workflow run appears and all steps pass (build ~8-12 min on macos-14).

If `auval` fails in CI with "Component not found", increase the `sleep` in the "Refresh AU registry" step from `2` to `5`.

If pluginval download times out, add a retry:
```yaml
- name: Download pluginval
  run: |
    for i in 1 2 3; do
      curl -fsSL https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_macOS.zip \
        -o /tmp/pluginval.zip && break || sleep 5
    done
    unzip -o /tmp/pluginval.zip -d /tmp/
```

- [ ] **Step 4: Commit**

```bash
cd /Users/jameswomack/Projects/github/jameswomack/womack-audio-worktrees/workflow-tooling
git add .github/workflows/build.yml
git commit -m "feat(ci): add GitHub Actions build + auval + pluginval on macos-14"
```

---

## Self-Review

**Spec coverage:**
1. build-and-validate — Task 1 ✓ (cmake, auval, pluginval)
2. Preset sharing — Task 2 ✓ (4 factory presets + README)
3. Demo video tooling — Task 3 ✓ (record + compress scripts)
4. Package + notarization — Task 4 ✓ (pkgbuild + productbuild + notarytool)
5. GitHub Actions CI — Task 5 ✓ (macos-14 + submodules + cache + auval + pluginval + artifact upload)

**Placeholder scan:** No TBDs, TODOs, or vague steps found. All code blocks are complete.

**Type consistency:** No shared types across tasks (all shell scripts / YAML). No cross-task naming conflicts.

**Gap found:** `.gitignore` gets two appends (Tasks 3 and 4). Both steps include the same instructions — the second append (`dist/`) won't conflict with the first (`recordings/`). No issue.
