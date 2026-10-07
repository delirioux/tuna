#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Build and run TUNA through Felucca's engines (tests/tuna_felucca.c).
#   tests/tuna_felucca.sh OUTDIR [SECONDS]      ONLY=name renders one piece
# Needs the generated headers in build/gen (./build.sh once, or tools/gen_tables.py,
# gen_fm6_patches.py and gen_samples.py into build/gen).
set -eu
OUT=${1:?usage: tests/tuna_felucca.sh OUTDIR [SECONDS]}
SECS=${2:-60}
mkdir -p build/host "$OUT"
for f in tuna_gen tuna_style tuna_marbles tuna_marbles_tables tuna_theory tuna_reso; do
    cc -O2 -c -Ifirmware/src "firmware/src/$f.c" -o "build/host/$f.o"
done
cc -O2 -w -Ibuild/gen -Ifirmware/src -o build/host/tuna_felucca tests/tuna_felucca.c \
    build/host/tuna_gen.o build/host/tuna_style.o build/host/tuna_marbles.o build/host/tuna_marbles_tables.o \
    build/host/tuna_theory.o build/host/tuna_reso.o -lm
build/host/tuna_felucca "$OUT" "$SECS"
