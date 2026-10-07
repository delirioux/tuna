#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# All of TUNA's tests on the host: theory, mechanisms, sequencer, presets and slots, the screen, and the composers swept over seeds.
#   tests/tuna_tests.sh [SEEDS] [SECONDS]        (defaults 10 seeds, 240 s; exit 0: all passed)
set -eu
SEEDS=${1:-10}
SECS=${2:-240}
mkdir -p build/host
SRC="firmware/src/tuna_gen.c firmware/src/tuna_style.c firmware/src/tuna_marbles.c firmware/src/tuna_marbles_tables.c firmware/src/tuna_theory.c"
for t in tuna_theory_test tuna_mech_test tuna_seq_test tuna_gen_render; do
    cc -O2 -Wall -Ifirmware/src -o build/host/$t tests/$t.c $SRC -lm
done
build/host/tuna_theory_test
build/host/tuna_mech_test
build/host/tuna_seq_test
cc -O2 -Wall -Ifirmware/src -o build/host/tuna_preset_test tests/tuna_preset_test.c firmware/src/tuna_preset.c $SRC -lm
build/host/tuna_preset_test
sh tests/tuna_ui_test.sh build/tuna_ui
out=$(mktemp -d)
TUNA_DIV=4 SWEEP=$SEEDS build/host/tuna_gen_render "$out" "$SECS"
TUNA_DIV=4 SWEEP=$SEEDS STYLES=10-999 build/host/tuna_gen_render "$out" "$SECS"
rm -rf "$out"
echo "tuna: all tests passed"
