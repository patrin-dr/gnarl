#!/bin/sh -e
# Render all GNARL screens on a computer (needs a C compiler; Python with
# Pillow optional, for PNG output). Usage, from the top-level directory:
#   tools/screen_preview/run.sh [ru|en] [output-dir]
lang=${1:-ru}
out=${2:-screen_preview_$lang}
top=$(pwd)
mkdir -p "$out/include"
cp include/module.h "$out/include/module.h"
printf '#include "boards/heltec_v2.h"\n' > "$out/include/board.h"
if [ "$lang" = en ]; then
    sed -i.bak 's|^#define DISPLAY_LANG_RU|// #define DISPLAY_LANG_RU|' "$out/include/module.h"
fi
cc -std=gnu11 -O1 -w -o "$out/preview" \
    -I"$out/include" -Iinclude -Ilib/oled -Isrc/gnarl -Ilib/u8g2/csrc \
    tools/screen_preview/preview.c src/gnarl/screens.c lib/u8g2/csrc/*.c
cd "$out" && ./preview
python3 - <<'PY' 2>/dev/null || echo "(install Pillow for PNG output; PBM files written)"
import glob
from PIL import Image
for p in sorted(glob.glob("*.pbm")):
    im = Image.open(p).convert("L").point(lambda v: 255 - v)
    im = im.resize((im.width * 4, im.height * 4), Image.NEAREST)
    big = Image.new("L", (im.width + 16, im.height + 16), 0)
    big.paste(im, (8, 8))
    rgb = Image.merge("RGB", (big.point(lambda v: v * 0.55), big.point(lambda v: v * 0.85), big))
    rgb.save(p[:-4] + ".png")
PY
