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
  -y \
  "$OUTPUT"

SIZE=$(du -sh "$OUTPUT" | cut -f1)
echo "Done: $OUTPUT ($SIZE)"
