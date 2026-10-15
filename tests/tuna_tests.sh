#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# All of TUNA's tests on the host: theory, mechanisms, sequencer, presets and slots, resonator, MIDI out,
# the maths, the screen, the browser build, and the composers swept over seeds.
#   tests/tuna_tests.sh [SEEDS] [SECONDS]        (defaults 10 seeds, 240 s; exit 0: all passed)
set -eu
SEEDS=${1:-10}
SECS=${2:-240}
mkdir -p build/host
python3 tools/tuna_genres.py --check      # the genres' C is what tools/genres holds
python3 tools/tuna_fm6.py --check      # the FM6 patches' C is what tools/tuna_fm6.py holds
python3 tools/tuna_manners.py --check   # the manners' C is what tools/manners holds
# the packed library (the firmware's) is the table, byte for byte
cc -O1 -Wall -Ifirmware/src -o build/host/tuna_style_pack tests/tuna_style_pack.c
cc -O1 -Wall -DTUNA_PACKED_STYLES=1 -Ifirmware/src -o build/host/tuna_style_pack_p tests/tuna_style_pack.c
build/host/tuna_style_pack --check firmware/src/tuna_styles_packed.inc
build/host/tuna_style_pack --print > build/host/styles_table.txt
build/host/tuna_style_pack_p --print > build/host/styles_packed.txt
cmp -s build/host/styles_table.txt build/host/styles_packed.txt || { echo "tuna: the packed styles differ from the table"; exit 1; }
SRC="firmware/src/tuna_gen.c firmware/src/tuna_style.c firmware/src/tuna_marbles.c firmware/src/tuna_marbles_tables.c firmware/src/tuna_theory.c"
for t in tuna_theory_test tuna_mech_test tuna_seq_test tuna_gen_render; do
    cc -O2 -Wall -Ifirmware/src -o build/host/$t tests/$t.c $SRC -lm
done
build/host/tuna_theory_test
build/host/tuna_mech_test
build/host/tuna_seq_test
cc -O2 -Wall -Ifirmware/src -o build/host/tuna_preset_test tests/tuna_preset_test.c firmware/src/tuna_preset.c $SRC -lm
build/host/tuna_preset_test
cc -O2 -Wall -Ifirmware/src -o build/host/tuna_reso_test tests/tuna_reso_test.c firmware/src/tuna_reso.c -lm
build/host/tuna_reso_test
cc -O2 -Wall -Ifirmware/src -o build/host/tuna_midi_test tests/tuna_midi_test.c firmware/src/tuna_app.c \
    firmware/src/tuna_midi.c firmware/src/tuna_reso.c firmware/src/tuna_preset.c $SRC -lm
build/host/tuna_midi_test
cc -O2 -Wall -fno-builtin -o build/host/tuna_libm_test tests/tuna_libm_test.c -lm
build/host/tuna_libm_test
sh tests/tuna_ui_test.sh build/tuna_ui
# the firmware's side: Tuna's modules as the FM-1 builds them (no C library, single-precision floats only), for
# a CPU like it; they may need nothing from outside but memcpy and memset (Felucca's libc.c)
LLD=$(command -v ld.lld || command -v ld.lld-18 || true)
NM=$(command -v llvm-nm || command -v llvm-nm-18 || true)
if command -v clang >/dev/null 2>&1 && [ -n "$LLD" ] && [ -n "$NM" ]; then
    fs=build/host/freestanding
    mkdir -p $fs
    for m in tuna_gen tuna_style tuna_theory tuna_marbles tuna_marbles_tables tuna_preset tuna_app tuna_midi tuna_reso tuna_libm; do
        clang --target=riscv32 -march=rv32imf -mabi=ilp32f -Os -fno-builtin -ffreestanding -nostdinc \
            -isystem "$(clang -print-resource-dir)/include" -DTUNA_FREESTANDING=1 -DTUNA_PACKED_STYLES=1 -Wall -Werror -Wno-unused-function \
            -Ifirmware/src -c firmware/src/$m.c -o $fs/$m.o
    done
    "$LLD" -r -o $fs/tuna.o $fs/tuna_*.o
    need=$("$NM" -u $fs/tuna.o | awk '{print $2}' | grep -vx 'memcpy\|memset' || true)
    [ -z "$need" ] || { echo "tuna: the firmware build would need: $need"; exit 1; }
    echo "tuna_freestanding: only memcpy and memset needed"
else
    echo "tuna: freestanding check skipped (no clang, ld.lld or llvm-nm)"
fi
# the browser build, when a WebAssembly compiler is at hand (emcc, or clang with WASI_SYSROOT) and Node to run it
if command -v node >/dev/null 2>&1 && { command -v emcc >/dev/null 2>&1 || [ -n "${WASI_SYSROOT:-}" ]; }; then
    sh web/emu/tuna_build.sh
    node web/emu/tuna_emu_test.mjs build/tuna_emu/tuna.wasm
else
    echo "tuna: browser build skipped (no emcc, no WASI_SYSROOT, or no node)"
fi
out=$(mktemp -d)
TUNA_DIV=4 SWEEP=$SEEDS build/host/tuna_gen_render "$out" "$SECS"
TUNA_DIV=4 SWEEP=$SEEDS STYLES=10-999 build/host/tuna_gen_render "$out" "$SECS"
rm -rf "$out"
echo "tuna: all tests passed"
