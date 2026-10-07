# Tuna

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

- **89 composers in 9 eras**, from Hildegard von Bingen to Eric Whitacre: Renaissance, Baroque,
  Classical, Early Romantic, Romantic, Late Romantic, Modern, Minimal, Film & Stage. Each has its
  own harmony, melody, rhythm, form and instrumentation: chant on a drone, Bach's counterpoint,
  Satie's parallel chords, Debussy's whole tones, Gershwin's jazz chords, Reich's phasing, Pärt's
  tintinnabuli, Messiaen's octatonic modes and birdsong.
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
| PRESETS / ALGORITHM | the library: piece by piece, or a whole era at a time; playing, the new piece comes in at the end of the period |
| KNOB 1–4 | the page's four cards; on HOME the chaos knobs: RHYTHM, PITCH, DENSITY and MACRO (0 is the composer exactly) |
| keys | tap one: the root (from the next chord); hold several: that chord, while held; on SAVE / LOAD the 16 white keys are the slots |
| OCT− / OCT+ | the seed: another piece in the same manner; on SAVE, LOAD and SEQ: back / do it |
| REC | memory: the phrase locked (deja vu) / the composer's own |
| HOME | the falling notes; held: the menu (colour, style) |
| FX · SCL · EDIT · ARP · SEQ · GLO · SAVE | library · key, scale, tuning, memory · time, then the parts · evolution · freeze · mix, then reverb · save, then load |
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
| 8 | On the FM-1: flash, measure CPU, then fine-tune every composer from their music | next |

## Building and testing

On Linux or macOS with a C compiler:

```
sh tests/tuna_tests.sh            # all of Tuna's tests: theory, generator, sequencer, presets, screen, 89 composers
sh tests/tuna_ui_test.sh          # the screen alone: every page as PNG, two GIFs of HOME playing (build/tuna_ui)
sh tests/tuna_felucca.sh out 60   # render every composer through Felucca's engines to WAV
```

Building Felucca itself for the FM-1 is described in [BUILDING.md](BUILDING.md).

## Credits and licence

Tuna is a modified version of **Felucca** by Leo Kuroshita (@kurogedelic), Hügelton Instruments
(<https://github.com/hugelton/Felucca>); Felucca's own README is kept in
[docs/FELUCCA.md](docs/FELUCCA.md). The Tuna files are `firmware/src/tuna_*` and `tests/tuna_*`.

Tuna's random section is a port of **Marbles** by Emilie Gillet
(<https://github.com/pichenettes/eurorack>), MIT ([LICENSES/MIT-Marbles.txt](LICENSES/MIT-Marbles.txt)).

Tuna is free software under the GNU General Public License, version 3 only ([LICENSE](LICENSE)).
Bundled and ported parts keep their own licences: see [LICENSING.md](LICENSING.md).
