# Tuna

<p align="center"><img src="docs/tuna-logo.png" alt="Tuna" width="256"></p>

[![License: GPL-3.0-only](https://img.shields.io/badge/license-GPL--3.0--only-blue.svg)](LICENSE)

Generative music firmware for the M-VAVE FM-1, built on [Felucca](docs/FELUCCA.md).

Tuna composes as it plays: Mutable Instruments Marbles' randomness, shaped by music theory into
melody, bass, accompaniment and percussion, through Felucca's synthesis engines. Pick a composer
and Tuna plays in that composer's manner, endlessly and never twice the same; or pick a character
preset and let it drift, from still and ambient to driving and rhythmic.

> **Status: in development, not yet on hardware.** Everything below runs and is tested on a
> computer. There is no installable Tuna build for the FM-1 yet. The Felucca web installer
> linked from Felucca's documentation installs Felucca, not Tuna.

## What is in it

- **86 popular genres in 8 decades**, the 1950s to now: rock & roll and doo-wop, Motown, ska and funk, disco,
  reggae, punk and Bronx block parties, house, techno and new jack swing, grunge, jungle and trance, dubstep and
  grime, trap, drill, amapiano and hyperpop, phonk and Jersey club. Each has its own drum groove and kit, bass line,
  chord loop, comping and instruments; vocal genres play as their beat, bass, chords and a lead line.
- **89 composers and styles in 10 eras**, from Hildegard von Bingen to today: Renaissance, Baroque,
  Classical, Early Romantic, Romantic, Late Romantic, Modern, Minimal, Film & Stage, Contemporary. Each has its
  own harmony, melody, rhythm, form and instrumentation: chant on a drone, Bach's counterpoint,
  Satie's parallel chords, Debussy's whole tones, Gershwin's jazz chords, phasing patterns,
  tintinnabuli bells, Messiaen's octatonic modes and birdsong.
- **Tuned from the scores**: the melodies of 45 composers lean toward what 3,486 public-domain scores of theirs
  measure: how often a line repeats a note, steps or leaps, how wide and how high it goes, how busy it is.
- **285 manners**: most composers wrote in several ways, and Tuna plays them all: Bach's fugues, chorales and
  gigues, Chopin's nocturnes, mazurkas, polonaises and études, Mozart's sonatas, arias, serenades and masses,
  Verdi's arias, choruses and Requiem. The seed picks the manner (each seed its own); LIBRARY's fourth knob fixes
  one.
- **93 instruments** on Felucca's engines, each composer and genre with sets of its own: besides the orchestra,
  piano and band, a pipe organ and harmonium, harpsichord and lute, cimbalom and koto, sitar and oud, erhu and
  shakuhachi, pan flute and harmonica, accordion and bandoneon, vibraphone, glockenspiel and tubular bells, music box
  and toy piano, upright, slap and fretless bass, string machine, synth brass and a handful of 8-bit leads. Their levels are
  evened out, so any of them sits right in the mix.
- **Odd meters and grouped beats**: 3+3+2, 5/4, 7/8 and the like.
- **Tunings**: equal, meantone, Werckmeister, Pythagorean, just, and user-defined.
- **8 character presets** without a composer, from STILL to DRIVE.
- **Sequencer**: freeze the period you hear into an editable pattern of up to 384 notes; trig
  conditions and up to 3 parameter locks per step; an evolution timer that builds an energy arc
  and modulates.
- **16 slots** that save the whole state and play back exactly the music that was saved.
- **MIDI out**, every part on its own channel (bass 1, melody 2, pad 3, timpani 4, drums 10), with General MIDI
  programs for the instruments, the sound locks as controllers, historical tunings as pitch bend, and clock.
- **A resonator**: sympathetic strings on the mix in the manner of Rings, tuned to the chord, the key or the
  root, retuning as the harmony moves.

## Controls

| Control | Does |
| --- | --- |
| PLAY | start / stop; always from the top (the same seed plays the same music) |
| SELECT / MASTER | tempo / volume |
| PRESETS / ALGORITHM | the library: piece by piece, or a whole era (or decade) at a time; playing, the new piece comes in at the next half of the period. Each piece brings its own tempo, key, percussion, evolution and reverb type, and a new random seed |
| KNOB 1–4 | the page's four cards; on HOME the chaos knobs: RHYTHM, PITCH, DENSITY and MACRO (0 is the composer exactly), on melody, bass and pad, each in its own way |
| keys | tap one: the root (from the next chord); hold several: that chord, while held; on SAVE / LOAD the 16 white keys are the slots |
| OCT− / OCT+ | the seed (256, a new one at every power-on): another piece in the same manner, playing from the next half of the period; on SAVE, LOAD and SEQ: back / do it |
| REC | memory: the phrase locked (deja vu) / the composer's own |
| HOME | the falling notes; held: the menu (colour, style) |
| FX · SCL · EDIT · ARP · SEQ · GLO · SAVE | library · key, scale, tuning, memory · time, then the parts, then which parts follow the chaos knobs · evolution · freeze · mix, then reverb (send, size, damping, type: BATHROOM, ROOM, HALL, CATHEDRAL, SPRING) · save, then load |
| ENV · LFO | MIDI out · the resonator |

## Roadmap

| Phase | | |
| --- | --- | --- |
| 1 | Music theory core | done |
| 2 | Generator (Marbles port, harmonizer) | done |
| 3 | Coupling to Felucca's engines | done |
| 4 | Sequencer | done |
| 5 | Presets, slots, composers by era | done |
| 6 | Screen and controls | done |
| 7 | MIDI out per part, resonator | done |
| 8 | On the FM-1: flash, measure CPU, then fine-tune every composer from their music | prepared: firmware glue, maths, browser build; waiting for the hardware |

## Building and testing

Clone with Felucca's engines (a submodule, MIT): `git clone --recurse-submodules https://github.com/delirioux/tuna.git`,
or `git submodule update --init` in a clone made without it. Then, on Linux or macOS with a C compiler:

```
sh tests/tuna_tests.sh            # all of Tuna's tests: theory, generator, sequencer, presets, screen, 89 composers and styles
sh tests/tuna_ui_test.sh          # the screen alone: every page as PNG, two GIFs of HOME playing (build/tuna_ui)
sh tests/tuna_felucca.sh out 60   # render every composer through Felucca's engines to WAV
```

Tuna in the browser, the whole firmware on a simulated FM-1 (needs `emcc`, or clang with a WASI sysroot in
`WASI_SYSROOT`; run `./build.sh` once first for `build/gen`):

```
sh web/emu/tuna_build.sh                                # build/tuna_emu: serve it and open index.html
node web/emu/tuna_emu_test.mjs build/tuna_emu/tuna.wasm  # boot, play, browse, root, pages, stop, save
```

For the FM-1 itself: `FELUCCA_TUNA=1` with Felucca's build, described in [BUILDING.md](BUILDING.md). Untested
on hardware so far.

## Documentation

- [docs/SPEC.md](docs/SPEC.md): the specification and all 89 composers and styles, era by era
- [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md): the working notes: agreements, workflow, design decisions, risks,
  and the steps for the first build on the FM-1

## Credits and licence

Tuna is a modified version of **Felucca** by Leo Kuroshita (@kurogedelic), Hügelton Instruments
(<https://github.com/hugelton/Felucca>); Felucca's own README is kept in
[docs/FELUCCA.md](docs/FELUCCA.md). The Tuna files are `firmware/src/tuna_*` and `tests/tuna_*`.

Tuna's random section is a port of **Marbles** by Emilie Gillet
(<https://github.com/pichenettes/eurorack>), MIT ([LICENSES/MIT-Marbles.txt](LICENSES/MIT-Marbles.txt)).

Tuna is free software under the GNU General Public License, version 3 only ([LICENSE](LICENSE)).
Bundled and ported parts keep their own licences: see [LICENSING.md](LICENSING.md).
