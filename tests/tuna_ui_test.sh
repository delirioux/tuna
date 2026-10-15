#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# TUNA's screen on the host: builds tests/tuna_ui_test.c on Felucca's gfx.c, runs it (the controls, the layout
# lint over every page, palette and composer) and turns its frames into PNGs and two GIFs of HOME playing.
#   tests/tuna_ui_test.sh [OUTDIR]     (default build/tuna_ui; needs the generated headers in build/gen: they
#                                       come from tools/build.py's generate step; Python with Pillow for the PNGs)
set -eu
OUT=${1:-build/tuna_ui}
mkdir -p build/host "$OUT/ppm"
SRC="firmware/src/tuna_app.c firmware/src/tuna_midi.c firmware/src/tuna_reso.c firmware/src/tuna_preset.c firmware/src/tuna_gen.c firmware/src/tuna_style.c
     firmware/src/tuna_marbles.c firmware/src/tuna_marbles_tables.c firmware/src/tuna_theory.c"
[ -f build/gen/ui_fonts.h ] || python3 -c "import sys; sys.argv=['x']; sys.path.insert(0,'tools'); import build; build.generate()"
cc -O2 -Wall -Wno-unused-function -Ibuild/gen -Ifirmware/src -o build/host/tuna_ui_test tests/tuna_ui_test.c $SRC -lm
build/host/tuna_ui_test "$OUT"
python3 - "$OUT" <<'PY' || echo "(no Pillow: the PPM frames are in $1/ppm)"
import glob, os, sys
from PIL import Image
out = sys.argv[1]
os.makedirs(out + "/png", exist_ok=True)
for p in sorted(glob.glob(out + "/ppm/*.ppm")):
    n = os.path.basename(p)[:-4]
    if n.startswith("anim_"):
        continue
    Image.open(p).resize((480, 480), Image.NEAREST).save(out + "/png/" + n + ".png")
for tag in ("debussy", "phasing"):
    fr = [Image.open(p).resize((480, 480), Image.NEAREST) for p in sorted(glob.glob(out + "/ppm/anim_" + tag + "_*.ppm"))]
    if fr:
        fr[0].save(out + "/tuna_home_" + tag + ".gif", save_all=True, append_images=fr[1:], duration=64, loop=0)
PY
