#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Instruction counts of TUNA on Felucca's engines, per piece, against Felucca's reference mix
# (tests/tuna_felucca.c; needs valgrind). Deterministic: the same numbers every run.
#   tests/tuna_cpu.sh [SECONDS] [PIECE ...]
# Prints per piece: Felucca mix instructions per sample, mean and worst half second, as a ratio of
# the reference mix (FM6 8 + PHASE 8 + VOICE 4 + drums), and Tuna's own generator per sample.
set -eu
SECS=${1:-30}
shift || true
OUT=build/host/cpu
rm -rf "$OUT"; mkdir -p "$OUT"
for f in tuna_gen tuna_style tuna_marbles tuna_marbles_tables tuna_theory; do
    cc -O2 -c -Ifirmware/src "firmware/src/$f.c" -o "build/host/$f.o"
done
cc -O2 -w -DTUNA_CALLGRIND -Ibuild/gen -Ifirmware/src -o build/host/tuna_felucca_cg tests/tuna_felucca.c \
    build/host/tuna_gen.o build/host/tuna_style.o build/host/tuna_marbles.o build/host/tuna_marbles_tables.o \
    build/host/tuna_theory.o -lm
cg() {   # cg TAG [env ...]
    tag=$1; shift
    env "$@" valgrind -q --tool=callgrind --collect-atstart=no --callgrind-out-file="$OUT/$tag.%p" \
        build/host/tuna_felucca_cg "$OUT/wav" "$SECS" > /dev/null
}
mkdir -p "$OUT/wav"
cg ref REF=1
PIECES=${*:-"garden rhythm bach bach-timpani mozart mozart-kit beethoven brahms mahler chopin satie debussy dvorak saint-saens"}
for p in $PIECES; do
    cg "mix-$p" ONLY="$p" CG_WHAT=mix ${TUNA_DIV:+TUNA_DIV=$TUNA_DIV}
    cg "gen-$p" ONLY="$p" CG_WHAT=gen ${TUNA_DIV:+TUNA_DIV=$TUNA_DIV}
done
python3 - "$OUT" $PIECES <<'PY'
import glob, re, sys
out, pieces = sys.argv[1], sys.argv[2:]
SEG = 44100 // 2
def segs(tag):
    vals = []
    for f in sorted(glob.glob(f"{out}/{tag}.*"), key=lambda s: [int(x) if x.isdigit() else x for x in re.split(r'(\d+)', s)]):
        for line in open(f):
            if line.startswith("summary:") or line.startswith("totals:"):
                vals.append(int(line.split()[1])); break
    return [v for v in vals if v > 0]
ref = segs("ref")
r = sum(ref) / len(ref) / SEG
print(f"reference mix: {r:.0f} instructions per sample (x86, -O2)")
print(f"{'piece':13s} {'mean':>7s} {'worst':>7s} {'tuna':>6s}")
for p in pieces:
    m, g = segs(f"mix-{p}"), segs(f"gen-{p}")
    if not m: print(p, "no data"); continue
    mean, worst = sum(m) / len(m) / SEG, max(m) / SEG
    gen = sum(g) / max(1, len(g)) / SEG if g else 0
    print(f"{p:13s} {mean/r:6.2f}x {worst/r:6.2f}x {gen:5.0f}i   ({mean:.0f} / {worst:.0f} instructions per sample)")
PY
