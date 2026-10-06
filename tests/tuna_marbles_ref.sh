#!/bin/sh
# SPDX-License-Identifier: MIT
# Build and run the bit-exact comparison of TUNA's Marbles port against Mutable Instruments' C++.
#   tests/tuna_marbles_ref.sh EURORACK_DIR [SCENARIOS]
# EURORACK_DIR: a checkout of https://github.com/pichenettes/eurorack with stmlib/ populated
# (git submodule, or a link to https://github.com/pichenettes/stmlib).
#
# One deliberate change is applied to the original, the same one the port makes: the lag
# processor clamps its raised-cosine table index to 0..1. Without it, a T ramp that ran negative
# under extreme jitter makes the original read before the table, and the result depends on
# whatever memory lies there.
set -eu
EURO=${1:?usage: tests/tuna_marbles_ref.sh EURORACK_DIR [SCENARIOS]}
N=${2:-2000}
OUT=build/host/marbles_ref
mkdir -p "$OUT"

sed 's/float warped_phase = Interpolate(lut_raised_cosine, phase, 256.0f);/float table_phase = phase < 0.0f ? 0.0f : phase > 1.0f ? 1.0f : phase; float warped_phase = Interpolate(lut_raised_cosine, table_phase, 256.0f);/' \
    "$EURO/marbles/random/lag_processor.cc" > "$OUT/lag_processor.cc"
grep -q table_phase "$OUT/lag_processor.cc" || { echo "patch did not apply"; exit 1; }

FLAGS="-O2 -ffp-contract=off"
cc $FLAGS -c -Ifirmware/src firmware/src/tuna_marbles.c -o "$OUT/tuna_marbles.o"
cc $FLAGS -c -Ifirmware/src firmware/src/tuna_marbles_tables.c -o "$OUT/tuna_marbles_tables.o"
cc $FLAGS -c -Ifirmware/src firmware/src/tuna_theory.c -o "$OUT/tuna_theory.o"
c++ $FLAGS -w -I"$EURO" -Ifirmware/src tests/tuna_marbles_ref.cc \
    "$EURO/marbles/random/t_generator.cc" "$EURO/marbles/random/x_y_generator.cc" \
    "$EURO/marbles/random/output_channel.cc" "$EURO/marbles/random/quantizer.cc" \
    "$OUT/lag_processor.cc" "$EURO/marbles/ramp/ramp_extractor.cc" "$EURO/marbles/resources.cc" \
    "$EURO/stmlib/dsp/units.cc" \
    "$OUT/tuna_marbles.o" "$OUT/tuna_marbles_tables.o" "$OUT/tuna_theory.o" -o "$OUT/tuna_marbles_ref"
"$OUT/tuna_marbles_ref" "$N"
