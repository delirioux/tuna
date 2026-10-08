#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Every instrument family on Felucca's engines, one by one (tests/tuna_audition.c), then measured against each
# other (tools/tuna_audition.py): which sound alike.
#   tests/tuna_audition.sh OUTDIR        ONLY=NAME renders one
#   NOTRIM=1 tests/tuna_audition.sh OUTDIR && python3 tools/tuna_audition.py OUTDIR --trims
#                                        evens the families' levels out again (tuna_sounds.h TSN_TRIM)
set -eu
OUT=${1:?usage: tests/tuna_audition.sh OUTDIR}
mkdir -p build/host "$OUT"
cc -O2 -w -Ibuild/gen -Ifirmware/src -o build/host/tuna_audition tests/tuna_audition.c firmware/src/tuna_style.c \
    firmware/src/tuna_theory.c -lm
build/host/tuna_audition "$OUT"
[ -n "${ONLY:-}" ] || python3 tools/tuna_audition.py "$OUT"
# CPU=1: instructions of Felucca's mix per sample of each family's phrase (valgrind callgrind)
if [ "${CPU:-0}" = 1 ]; then
    cc -O2 -w -DTUNA_CALLGRIND -Ibuild/gen -Ifirmware/src -o build/host/tuna_audition_cg tests/tuna_audition.c \
        firmware/src/tuna_style.c firmware/src/tuna_theory.c -lm
    rm -f "$OUT"/callgrind.out.*
    (cd "$OUT" && valgrind --tool=callgrind --collect-atstart=no --callgrind-out-file=callgrind.out.%p \
        "$OLDPWD/build/host/tuna_audition_cg" . >/dev/null 2>&1)
    python3 tools/tuna_audition.py "$OUT" --cpu
fi
