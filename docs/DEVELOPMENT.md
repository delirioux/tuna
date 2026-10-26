# Developing Tuna

The working notes behind Tuna: the agreements, how the work flows, why the code is shaped as it is, where it
stands, and what happens when the FM-1 arrives. The specification and the 89 composers and styles are in
[SPEC.md](SPEC.md); what Tuna is and how to play it is in the [README](../README.md).

Status: 7 October 2026. Phases 1 to 7 done, phase 8 prepared; the FM-1 is on its way.

## Agreements

| What | Agreed |
| --- | --- |
| Home | GitHub, `github.com/delirioux/tuna`, branch `main` (private until the release). |
| Upstream | Tuna is a fork of [Felucca](https://github.com/hugelton/Felucca) (GPL-3.0). Never push to Felucca. Upstream releases are merged in by hand (`Merge Felucca 1.6.1 into Tuna`; its five engines are a submodule, `firmware/engines`), Felucca's own README kept as [FELUCCA.md](FELUCCA.md). |
| Language | English throughout: code, comments, commits, docs. |
| Licence | GPL-3.0-only, as Felucca; Marbles parts MIT ([LICENSES/MIT-Marbles.txt](../LICENSES/MIT-Marbles.txt)). |
| Commits | Author `delirioux <340923392+delirioux@users.noreply.github.com>`. Work done with Claude ends with the `Co-Authored-By` line (no links into Claude). |
| Publishing | Public, GPL-3.0, for everyone, as soon as it works on the FM-1 (release checklist below). |

## How changes reach GitHub

Claude works in a sandbox that cannot push. Changes travel as git bundles:

1. Claude commits on `main` and writes `tuna.bundle` (`git bundle create tuna.bundle github/main..main`).
2. Download it, then `Tuna-Sync.ps1 -Push`: it imports the newest `tuna*.bundle` from Downloads and pushes it.
3. For a fresh session, `Tuna-Sync.ps1 -Fetch` writes `Downloads\tuna-full.bundle` of the whole repo; upload
   that so the session starts from what is on GitHub.

`Tuna-Sync.ps1` with no argument shows a menu. A bundle that repeats commits GitHub already has is harmless.

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
  need nothing outside but `memcpy` and `memset`, which Felucca's `libc.c` provides. The FPU needs JieLi's
  `-mcpu=r3 -mfprev1` (`tools/build.py` adds them to Tuna's build): without them every float operation becomes a
  call to a soft-float routine the firmware does not have.
- **The flash: 568 KB for Felucca and Tuna together**, and 72 KB of RAM beside a larger pool. Tuna's build leaves
  out Felucca's SLICE engine (and its break loop), serial console and web editor (Felucca's sounds and projects;
  updates and the installer's handshake are `ota.c`'s and stay); its state lives in the pool, but for the
  generator's two recordings (`TUNA_REC_OUTSIDE`, in RAM), to keep Felucca's margins (4 KB of RAM, 8 KB of pool,
  `tools/build.py`); the Marbles
  inverse CDFs are 16-bit; and the style library is packed: `tuna_styles_packed.inc`, each profile as its
  differences from a base, written from `tuna_style.c`'s table by `tests/tuna_style_pack.c`
  (`build/host/tuna_style_pack firmware/src/tuna_styles_packed.inc` after a change to the table, the genres or the
  manners' profiles; `tests/tuna_tests.sh` checks it is up to date and expands to the table byte for byte).
- **The generator runs in the audio interrupt** at a quarter of the audio rate (11 kHz). The panel's actions
  run with the audio interrupt off; sound changes that load presets are deferred to the main loop.
- **Saving only while stopped.** Felucca runs from flash (XIP); writing it while playing would stall the
  audio. Tuna says `STOP TO SAVE`.
- **A new seed at every power-on, then deterministic.** The seed comes from the ADC's low bits and the timer
  at start (the browser: the page's random number), mixed with the moment of the first PLAY unless a seed was
  chosen by then. From there the same seed plays the same music; a slot loads exactly the music that was saved. The
  chaos knobs at 0 are the composer byte for byte. The tests depend on this.
- **The chaos knobs move melody, bass and pad**, each in its own way: the melody inside its generator; bass
  and pad through a filter on their events that leaves out, moves and adds notes (the bass keeps its root on
  every chord change). The filter has its own random numbers, and added notes go out beside the parts, so the
  harmony and everything else stays the composer's. At 0 nothing changes: the composer byte for byte.
  The CHAOS page (EDIT a third time) chooses which parts follow; slots keep it (record version 2).
- **Each piece brings its own settings.** A new composer or genre loads its own tempo, key, scale, meter,
  percussion, evolution, sounds and reverb type: keeping the player's would lose what makes it itself. The chaos knobs (and
  the parts they move), memory, mix, reverb, resonator and MIDI stay.
- **The reverb is Felucca's.** The REVERB page (GLO twice) sets the send into Felucca's shared reverb bus, and its
  SIZE, DAMP and TYPE as Felucca's own globals. The types are a size and damping on Felucca's two models, no new
  DSP: BATHROOM, ROOM (as designed), HALL, CATHEDRAL on ROOM, and SPRING. Each piece brings its type (a genre's
  `reverb`, else by era: Renaissance CATHEDRAL, the Romantics and film HALL, the rest ROOM; the surf, ska, reggae
  and garage genres SPRING); the send stays the player's. Slots keep all four (record version 3; older records take
  the piece's). Volume is the MASTER knob alone, read by Felucca.
- **Changes land on half periods.** A piece, a seed (OCT) or a form setting chosen while playing comes in at the
  next half of the period: its start (on the cadence) or its middle (on the half cadence). Seeds run 0..255.
  The change goes in just before that tick, with the clock's phase kept: the new music starts on the beat, with no
  gap and no clipped notes from the old.
- **A new piece, a new seed.** A seed means nothing across pieces, so choosing a piece draws one of the 256 (stirred
  by the moment of each touch on the panel): every visit plays something new. Turn back to the piece playing and
  nothing changes; a slot still loads exactly what was saved.
- **Manners are changes.** A composer's profile (`tuna_style.c`) is its first manner; `tools/manners/*.py` (one
  file per era) add the others as changes to it, and `python3 tools/tuna_manners.py` checks them against the
  fields of `tg_style` and writes `tuna_manners.inc` (only what differs from the profile: 285 manners in about
  27 KB) and the field table `tuna_style_fields.inc`; `--show NAME` prints a profile in the manners' syntax.
  The generator plays a library style through a copy with its manner applied (`tg_apply_manner`,
  `style_id`). The seed picks the manner (each seed its own, so a new seed may bring a chorale where a fugue was);
  LIBRARY's K4 fixes one; a slot keeps it (record version 4). The render sweeps play every manner as a piece of
  its own. What the fields cannot say yet, by the composers' manners: dotted and other fixed melodic rhythm
  cells, a stress on a given beat, a drum pattern per manner, glissandi, a one-time form (one crescendo, march
  then trio), a cantus firmus in the bass, a choir without accompaniment.
- **Composers are tuned from their scores.** `tools/tuna_corpus.py` measures the top line of public-domain
  scores (PDMX: its deduplicated subset, no licence conflict, rated scores at 4 or better; 3,486 scores of 79 of
  the composers, 45 of them with enough to tune from) and, the same way, the melody each manner plays (`tests/tuna_corpus_dump.c`): the shares of
  repeated notes, steps, thirds, fourths and wider leaps, how wide the leaps are, octaves, rising against
  falling, the range, notes a beat. `tune` then moves a manner's `profile`, `max_leap`, `octave`, `direction`
  and `melody_range` toward the scores, playing and measuring again each round, and writes them into
  `tuna_style.c` and `tools/manners`. A manner with at least 8 scores of its own by title (Bach's fugues,
  Chopin's nocturnes) follows those, its `melody_density` too; the others follow the composer's whole work
  (at least 15 scores) half as far and keep their rhythm, which belongs to the manner. Trust grows with the
  number of scores, and no setting moves far from what was written by hand (an interval's chance at most
  doubled or halved, the range 6 semitones wider, the density 1.3 times): the scores lean a manner, they do
  not rewrite it. A top line is not always the tune (a right hand's figuration counts as melody), the
  generator turns a profile into notes through its own rules (a leap is answered by a step, motifs return),
  and patterns, tintinnabuli and birdsong have no free line to tune. The measurements are kept in
  `tools/corpus/composers.json`; PDMX itself is not in the repository. Each `tune` moves on from where the
  sources stand, so it is run once per corpus, not repeatedly.
- **Genres are data.** The 86 popular genres live in `tools/genres/d1950.py` .. `d2020.py` (readable: grooves as
  tick strings, chords by name); `python3 tools/tuna_genres.py` writes `firmware/src/tuna_genres.inc` and checks
  them (chord names against the vocabularies, lengths, names); `tuna_tests.sh` fails if the two differ. A
  genre's drums are a groove (up to 8 instruments over two bars, accents, ghosts, hat rolls) with its own kit
  on Felucca's DRUM engine; bass can be a riff in semitones or an 808 sliding from kick to kick; the third part
  can strike chords on a rhythm (skanks, stabs) or play power chords. Freezing a genre loops its pitched parts
  (whole bars if a period has more notes than a pattern holds) while its groove plays on.
- **Instruments are Felucca's sounds, some set Tuna's way.** A family (TI_*) is a Felucca engine and factory preset
  (`tuna_sounds.h`), or a sound of Tuna's own on an engine (`tuna_sounds_own.inc`, Felucca's preset layout), or an
  FM6 patch of Tuna's own (`tools/tuna_fm6.py` writes `tuna_fm6.inc`). Each has a level trim, so a pluck and a held
  string come out alike. Popular genres take the plucked UPRIGHT for a double bass, the composers the bowed
  CONTRABASS. `tests/tuna_audition.sh` plays every family on the same phrase (WAVs to listen to), measures
  them against each other (sound-alikes), counts their instructions (CPU=1) and re-derives the trims. The 175
  composers and genres were orchestrated for variety: each its own signature colour, no family everywhere (the
  most used, STRINGS, in a quarter of them). The heavy ones (long ringing modal or FM tails) go in the melody, at
  most one per set, never in a third part striking chords on every beat.
- **The resonator in fixed point** at half rate: about 0.12× of Felucca's reference mix.

## Building and testing

On Linux or macOS with a C compiler (Windows: WSL2). `./build.sh` once first, for `build/gen`.

```
sh tests/tuna_tests.sh            # everything: theory, generator, sequencer, presets, resonator, MIDI, maths, screen, composers
sh tests/tuna_ui_test.sh          # the screen: every page as PNG, GIFs of HOME (build/tuna_ui)
sh tests/tuna_felucca.sh out 60   # every composer through Felucca's engines to WAV
sh tests/tuna_cpu.sh 30           # instruction counts against Felucca's reference mix (valgrind)
sh tests/tuna_audition.sh out     # every instrument on the same phrase: WAVs, sound-alikes; CPU=1 its cost
python3 tools/tuna_corpus.py report   # what the scores say of each composer (stats PDMX_DIR measures them again)
sh web/emu/tuna_build.sh          # Tuna in the browser (emcc, or clang with WASI_SYSROOT)
node web/emu/tuna_emu_test.mjs build/tuna_emu/tuna.wasm
```

`tuna_tests.sh` runs the browser build and its test too when `node` and `emcc` or `WASI_SYSROOT` are there. A
WASI sysroot comes with the npm package `@yowasp/clang` (`gen/llvm-resources.tar`).

Last full run, 11 Oct 2026: theory 230, mechanisms 1837, sequencer 2128, presets 2005, resonator 18, MIDI 165,
maths 6, screen 466 checks, and every composer and manner over 10 seeds of 4 minutes: all pass (built with
clang; the freestanding check and the browser test were not at hand that day, last run 7 Oct).

## Open risks

| Risk | What happens | Check |
| --- | --- | --- |
| CPU on the chip | dropouts at the loudest moments; all 175 styles measured (instruction counts, 30 s each): mean at most 1.03×, the worst half second 1.30× the reference mix (JAZZ 50S), most below 1.1× | measure on the FM-1; organ or pad in place of the choir |
| Soft floats | without `-mcpu=r3 -mfprev1` floats become library calls that crash the FM-1 at boot | settled: tools/build.py sets them for Tuna, and CI fails on any soft-float routine in the image |
| Flash and RAM | the image did not fit at first | settled: packed styles, uint16 Marbles tables, no editor, recordings in the pool |

CPU is the one left open: it is measured on the FM-1 next.

## Next

What the composers' manners could not say (the agents who wrote them met these again and again). Gaps in the
generator's design, not the hardware's; in order of value:

1. **Rhythm figures.** A melody's rhythm is Marbles' pulse shaped by density and syncopation; a fixed figure per
   bar (dotted, long-short-short, the mazurka's, the polonaise's, the sicilienne's lilt) cannot be set.
2. **Beat stress.** Accents follow a general rule plus random sforzandi; an accent table per beat is needed (the
   mazurka on 2 or 3, the sarabande on 2).
3. **A drum pattern per manner.** `drum_pat` was left out of the manner tool (a shortcut); add it (the Boléro
   snare, the Mars side drum, castanets).
4. **Glides into notes.** Felucca's tracks glide in mono only and the generator never asks for a slide per note
   (Gershwin's clarinet, Bartók, Khachaturian).
5. **A shape across the whole piece.** The generator thinks one period at a time; a single crescendo
   (Zarathustra, the Mountain King's speed-up) or a march that turns once into its trio needs a piece timeline.
6. **A melody in the bass.** Every bass mode follows the chord roots; a cantus firmus in the bass under running
   upper voices needs a bass mode of its own.
7. **A lighter, lower choir.** CHOIR is heavy and mid-register, so a choir cannot sing all three parts
   (a cappella motets, Rachmaninoff's basses).

## Release checklist

Before the first public release (GPL-3.0, on GitHub):

1. **It works on the FM-1**: built, flashed, played through every era; flash and CPU measured; the way back to the
   stock firmware tried.
2. **The name**: search "Tuna" in the EUIPO and BOIP registers (classes 9 and 15). An iOS tuner (Aurora DSP), a
   Windows tuner suite (Tuna+) and an OBS plugin already use it; none is a composer or a synth firmware. Release
   as "Tuna for FM-1" at least.
3. **Licences and credits**: Tuna stays GPL-3.0-only, as Felucca; Felucca's copyright lines stay; the MIT parts
   (Marbles, DaisySP, Rings, Fukiai), msfa's Apache-2.0 (FM6) and the OFL fonts keep their texts in LICENSES/ and
   their credits in the README and the menu. The samples are Versilian Studios' CC0 library (tools/fetch_cc0.py):
   free to use.
4. **Other names**: "for the M-VAVE FM-1" in words only, no logos, nothing that suggests it is official.
5. **A courtesy note** to Leo Kuroshita (Felucca), not required.

## When the FM-1 arrives

1. Check the stock firmware works, and that the M-VAVE updater can put it back.
2. Install Felucca with its web installer, and confirm the way back to the stock firmware.
3. Read the chip markings.
4. Set up WSL2 with the JieLi toolchain (`tools/get_toolchain.sh`, see [BUILDING.md](../BUILDING.md)).
5. Build: `FELUCCA_TUNA=1 ./build.sh`. Send any errors as they are.
6. Flash Tuna, play every era, measure the CPU.
7. Then fine-tune the composers by ear. Their melodies were leaned toward their scores on 10 Oct 2026
   (`tools/tuna_corpus.py`); harmony and rhythm figures still wait (chord transitions need a harmonic analysis
   of the scores, the figures the generator's first two next steps above).
