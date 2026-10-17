# Tuna

<p align="center"><img src="docs/tuna-logo.png" alt="Tuna" width="256"></p>

[![License: GPL-3.0-only](https://img.shields.io/badge/license-GPL--3.0--only-blue.svg)](LICENSE)

Generative music firmware for the M-VAVE FM-1, built on [Felucca](docs/FELUCCA.md).

Tuna composes as it plays: Mutable Instruments Marbles' randomness, shaped by music theory into
melody, bass, accompaniment and percussion, through Felucca's synthesis engines. Pick a composer
and Tuna plays in that composer's manner, endlessly and never twice the same; or pick a character
preset and let it drift, from still and ambient to driving and rhythmic.

> **Version 1.0: runs on the FM-1.** Install it from the web installer at <https://delirioux.github.io/tuna/>
> (Chrome or Edge, USB), or try it in the browser there first. Built for fun and with no warranty: Felucca's
> installer, or the official V15, takes the FM-1 back.

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
- **95 instruments** on Felucca's engines, each composer and genre with sets of its own: besides the orchestra,
  piano and band, a pipe organ and harmonium, harpsichord and lute, cimbalom and koto, sitar and oud, erhu and
  shakuhachi, pan flute and harmonica, accordion and bandoneon, vibraphone, glockenspiel and tubular bells, music box
  and toy piano, upright, slap and fretless bass, string machine, synth brass and a handful of 8-bit leads; WIND and
  RAIN on Felucca's NOISE engine (wind through the chords, a record's crackle). Their levels are evened out, so any of
  them sits right in the mix. Only the piano is sampled; Tuna's flute and sax are FM patches of its own, which leaves
  the flash room for the rest.
- **Odd meters and grouped beats**: 3+3+2, 5/4, 7/8 and the like.
- **Tunings**: equal, meantone, Werckmeister, Pythagorean, just, and user-defined.
- **8 character presets** without a composer, from STILL to DRIVE.
- **Sequencer**: freeze the period you hear into an editable pattern of up to 384 notes; trig
  conditions and up to 3 parameter locks per step; an evolution timer that builds an energy arc
  and modulates.
- **16 slots** that save the whole state and play back exactly the music that was saved.
- **Seeds sorted by the type of work**: each composer's 256 pieces in a block per manner, so OCT+ walks through
  the fugues, then the chorales, then the gigues, the screen saying where you are ("FUGUE 17/86").
- **Favourites**: hold REC on a piece you like; they wait below TUNA in the library (up to 32), kept across power-offs.
- **Share codes**: every piece has a code, `BACH-FUGUE-17`, the same piece on any FM-1 and in the browser. A link
  plays it: `https://delirioux.github.io/tuna/try/?piece=BACH-FUGUE-17`.
- **Radio**: after a few minutes (1 to 30) another piece, from the same era or the whole library, coming in at a
  cadence.
- **MOOD**: one knob from calm to busy, every part at once: fewer or more notes, softer or louder, a little slower or
  faster; the pitch stays the composer's.
- **MIDI out**, every part on its own channel (bass 1, melody 2, pad 3, timpani 4, drums 10), with General MIDI
  programs for the instruments, the sound locks as controllers, historical tunings as pitch bend, and clock.
- **MIDI clock in**: Tuna follows another device's tempo, start and stop.
- **A resonator**: sympathetic strings on the mix in the manner of Rings, tuned to the chord, the key or the
  root, retuning as the harmony moves.

## Controls

| Control | Does |
| --- | --- |
| PLAY | start / stop; always from the top (the same seed plays the same music) |
| SELECT / MASTER | tempo / volume |
| PRESETS / ALGORITHM | the library: piece by piece, or a whole era (or decade) at a time; playing, the new piece comes in at the next half of the period. Each piece brings its own tempo, key, percussion, evolution and reverb type, and a new random seed |
| KNOB 1–4 | the page's four cards; on HOME the chaos knobs RHYTHM, PITCH, DENSITY (0 is the composer exactly, on melody, bass and pad, each in its own way) and MOOD, calm to busy |
| keys | tap one: the root (from the next chord); hold several: that chord, while held; on SAVE / LOAD the 16 white keys are the slots |
| OCT− / OCT+ | the seed (256, a new one at every power-on, sorted by the type of work): the next piece, playing from the next half of the period; on SAVE, LOAD and SEQ: back / do it |
| REC | memory: the phrase locked (deja vu) / the composer's own; held: the piece a favourite (held again: no longer) |
| HOME | the falling notes; held: the menu (colour, style, radio and its minutes) |
| FX · SCL · EDIT · ARP · SEQ · GLO · SAVE | library (ERA turned below TUNA: the favourites; K4 the manner; the share code under the list) · key, scale, tuning, memory · time, then the parts, then which parts follow the chaos knobs · evolution · freeze · mix, then reverb (send, size, damping, type: BATHROOM, ROOM, HALL, CATHEDRAL, SPRING) · save, then load |
| ENV · LFO | MIDI out, and CLOCK: off, out or in · the resonator |

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
| 8 | On the FM-1: firmware, web installer, boot logo | done |
| 9 | The player: seeds sorted by the type of work, favourites, share codes, radio, MOOD, MIDI clock in | done, untested on the FM-1 |
| 10 | 1.0 on the FM-1: CPU measured, every era played | next |
| 11 | Consonant by default: dissonance only where it is the composer's signature | |
| 12 | Longer form: half and full cadences, themes that return, sections, an arc over minutes | |
| 13 | Hands on the instruments: the knobs on one part (level, brightness, envelope, instrument), mute and solo | |
| 14 | Synth session: a button held, the keys play in the key | |
| 15 | Sound: Felucca's HALL reverb, the second core, a lighter choir | |

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

For the FM-1 itself: `FELUCCA_TUNA=1 ./build.sh`, with Felucca's toolchain and SDK as [BUILDING.md](BUILDING.md)
describes. Every push to `main` builds the package and the site on GitHub Actions
([.github/workflows/firmware.yml](.github/workflows/firmware.yml)).

## Documentation

- [docs/SPEC.md](docs/SPEC.md): the specification and all 89 composers and styles, era by era
- [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md): the working notes: agreements, workflow, design decisions, risks
  and what comes next

## Credits and licence

Tuna is a modified version of **Felucca** by Leo Kuroshita (@kurogedelic), Hügelton Instruments
(<https://github.com/hugelton/Felucca>); Felucca's own README is kept in
[docs/FELUCCA.md](docs/FELUCCA.md). The Tuna files are `firmware/src/tuna_*` and `tests/tuna_*`.

Tuna's random section is a port of **Marbles** by Emilie Gillet
(<https://github.com/pichenettes/eurorack>), MIT ([LICENSES/MIT-Marbles.txt](LICENSES/MIT-Marbles.txt)).

Tuna is free software under the GNU General Public License, version 3 only ([LICENSE](LICENSE)).
Bundled and ported parts keep their own licences: see [LICENSING.md](LICENSING.md).
