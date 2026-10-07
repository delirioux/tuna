# Developing Tuna

The working notes behind Tuna: the agreements, how the work flows, why the code is shaped as it is, where it
stands, and what happens when the FM-1 arrives. The specification and the 89 composers are in
[SPEC.md](SPEC.md); what Tuna is and how to play it is in the [README](../README.md).

Status: 7 October 2026. Phases 1 to 7 done, phase 8 prepared; the FM-1 is on its way.

## Agreements

| What | Agreed |
| --- | --- |
| Home | Codeberg, `codeberg.org/delirioux/tuna`, branch `tuna`. Not GitHub. |
| Upstream | Tuna is a fork of [Felucca](https://github.com/hugelton/Felucca) (GPL-3.0). Never push to Felucca. Upstream releases are merged in by hand (`Merge Felucca 1.0.5.2 into Tuna`), Felucca's own README kept as [FELUCCA.md](FELUCCA.md). |
| Language | English throughout: code, comments, commits, docs. |
| Licence | GPL-3.0-only, as Felucca; Marbles parts MIT ([LICENSES/MIT-Marbles.txt](../LICENSES/MIT-Marbles.txt)). |
| Commits | Author `delirioux <340923392+delirioux@users.noreply.github.com>`. Work done with Claude ends with the `Co-Authored-By` and `Claude-Session` lines. |
| Publishing | Personal use first. |

## How changes reach Codeberg

Claude works in a sandbox that cannot push. Changes travel as git bundles:

1. Claude commits on `tuna` and writes `tuna.bundle` (`git bundle create tuna.bundle codeberg/tuna..tuna`).
2. Download it, then `Tuna-Sync.ps1 -Push`: it imports the newest `tuna*.bundle` from Downloads and pushes it.
3. For a fresh session, `Tuna-Sync.ps1 -Fetch` writes `Downloads\tuna-full.bundle` of the whole repo; upload
   that so the session starts from what is on Codeberg.

`Tuna-Sync.ps1` with no argument shows a menu. A bundle that repeats commits Codeberg already has is harmless.

## Where Tuna lives

Tuna's files are `firmware/src/tuna_*`, `tests/tuna_*` and `web/emu/tuna_*`. Everything else is Felucca's,
touched only where Tuna hooks in (`felucca.c`, `main.c`, `audio.c`, `tools/build.py`, `web/emu/index.html`).

| File | Does |
| --- | --- |
| `tuna_theory` | scales, chords, progressions, tunings |
| `tuna_marbles`, `tuna_marbles_tables` | the Marbles port (T and X/Y generators, deja vu, quantizer), bit-exact |
| `tuna_gen` | the generator: harmonizer, parts, motifs, meters, chaos knobs, root and held chord, sequencer |
| `tuna_style` | the 89 composer signatures and 8 character presets |
| `tuna_preset` | the slot record (versioned, CRC-checked) and the 16 slots in Felucca's project objects |
| `tuna_app` | the app state: library, playing, edits at the period's end, MIDI, resonator |
| `tuna_midi` | MIDI out per part |
| `tuna_reso` | the resonator, fixed point |
| `tuna_ui` | Tuna's pages, drawn with Felucca's `gfx.c` |
| `tuna_fw` | the firmware glue: panel, encoders, keys, LEDs, flash, USB MIDI, the audio hooks |
| `tuna_libm`, `tuna_sys.h` | the five maths functions the firmware needs, and the freestanding declarations |

`FELUCCA_TUNA=1` builds Tuna instead of Felucca's own screens: `felucca.c` includes `tuna_fw.c` (with
`tuna_ui.c`), `tools/build.py` compiles the other modules as objects of their own and links them in.

## Decisions and why

- **Felucca's engines, Tuna's everything else.** Rewriting the hardware layer and the synthesis would take
  weeks and add no sound. Felucca's screens are replaced ("Tuna only"): no engine or FX editing on the device.
- **No doubles, no libm.** The AC79's FPU is single precision and the firmware links no C library. Tuna uses
  floats only, and `tuna_libm.c` supplies `exp2f`, `sinf`, `sqrtf`, `lrintf`, `fabsf` (within 5e-7 of the
  host's). The MIDI clock runs in Q16 fixed point. Compiled for a single-precision-only CPU, Tuna's modules
  need nothing outside but `memcpy` and `memset`, which Felucca's `libc.c` provides.
- **The generator runs in the audio interrupt** at a quarter of the audio rate (11 kHz). The panel's actions
  run with the audio interrupt off; sound changes that load presets are deferred to the main loop.
- **Saving only while stopped.** Felucca runs from flash (XIP); writing it while playing would stall the
  audio. Tuna says `STOP TO SAVE`.
- **Deterministic.** The same seed plays the same music; a slot loads exactly the music that was saved. The
  chaos knobs at 0 are the composer byte for byte. The tests depend on this.
- **The resonator in fixed point** at half rate: about 0.12× of Felucca's reference mix.

## Building and testing

On Linux or macOS with a C compiler (Windows: WSL2). `./build.sh` once first, for `build/gen`.

```
sh tests/tuna_tests.sh            # everything: theory, generator, sequencer, presets, resonator, MIDI, maths, screen, composers
sh tests/tuna_ui_test.sh          # the screen: every page as PNG, GIFs of HOME (build/tuna_ui)
sh tests/tuna_felucca.sh out 60   # every composer through Felucca's engines to WAV
sh tests/tuna_cpu.sh 30           # instruction counts against Felucca's reference mix (valgrind)
sh web/emu/tuna_build.sh          # Tuna in the browser (emcc, or clang with WASI_SYSROOT)
node web/emu/tuna_emu_test.mjs build/tuna_emu/tuna.wasm
```

`tuna_tests.sh` runs the browser build and its test too when `node` and `emcc` or `WASI_SYSROOT` are there. A
WASI sysroot comes with the npm package `@yowasp/clang` (`gen/llvm-resources.tar`).

Last full run, 7 Oct 2026: theory 230, mechanisms 1499, sequencer 1096, presets 1127, resonator 18, MIDI 102,
maths 6, screen 134 checks, the browser test, and every composer over 10 seeds of 4 minutes: all pass.

## Open risks

| Risk | What happens | Check |
| --- | --- | --- |
| CPU on the chip | dropouts at the loudest moments; choir-heavy composers peak at 1.06–1.15× the reference mix | measure on the FM-1; organ or pad in place of the choir |
| The JieLi compiler wants doubles or a maths function the host build never showed | link errors, or slow software floats | the first build's errors name the function; add it to `tuna_libm.c` |
| Flash space | the image may not fit the ~568 KB program area | the first build says; the Marbles tables (74 KB) can shrink |
| Chip and clock unconfirmed | estimates off | read the board on delivery |

None of these is measured yet; they wait for the hardware.

## When the FM-1 arrives

1. Check the stock firmware works, and that the M-VAVE updater can put it back.
2. Install Felucca with its web installer, and confirm the way back to the stock firmware.
3. Read the chip markings.
4. Set up WSL2 with the JieLi toolchain (`tools/get_toolchain.sh`, see [BUILDING.md](../BUILDING.md)).
5. Build: `FELUCCA_TUNA=1 ./build.sh`. Send any errors as they are.
6. Flash Tuna, play every era, measure the CPU.
7. Then fine-tune the composers from their music (MIDI files); this can also start before, in the browser.
