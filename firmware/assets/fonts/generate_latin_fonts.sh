#!/usr/bin/env bash
# Regenerate Latin-1 supplements. Generated C files are committed, so firmware
# builds do not require Node, a font converter, or a downloaded source font.
# Usage: ./generate_latin_fonts.sh /path/to/Montserrat-Medium.ttf
# Requires lv_font_conv 1.5.3 (npm install -g lv_font_conv@1.5.3).
# Source: https://github.com/google/fonts/tree/main/ofl/montserrat
# Use the Medium (weight 500) instance. License: Montserrat-OFL.txt.
set -euo pipefail
FONT="${1:?Pass the path to Montserrat-Medium.ttf}"
FONT="$(cd "$(dirname "$FONT")" && pwd)/$(basename "$FONT")"
cd "$(dirname "$0")"
for SIZE in 12 14 16 20 24 28; do
  lv_font_conv --font "$FONT" -r 0xA0-0xFF \
    --size "$SIZE" --bpp 4 --format lvgl --no-compress --no-prefilter \
    --lv-font-name "lv_font_latin_${SIZE}" \
    --lv-fallback "lv_font_montserrat_${SIZE}" \
    -o "../../src/ui/fonts/latin_font_${SIZE}.c"
done
