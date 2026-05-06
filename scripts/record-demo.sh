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

# Display index (1 = primary on macOS). Adjust if you have multiple monitors.
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
wait $FFMPEG_PID || true
echo "Saved: $OUTPUT"
