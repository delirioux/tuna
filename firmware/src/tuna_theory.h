/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA music theory core: scales, diatonic chords, chord progressions, held-key chords.
 * Pure integer code, no allocation, no floats: runs unchanged on the AC79 and on the host.
 *
 * Conventions
 *   pitch class (pc)  0..11, C = 0
 *   root              pitch class of the key's tonic
 *   scale mask        12-bit set of pitch classes relative to the root (bit 0 = tonic)
 *   MIDI note         0..127, C4 = 60 */
#ifndef TUNA_THEORY_H
#define TUNA_THEORY_H

#include <stdint.h>

/* ---- scales ------------------------------------------------------------------------------ */
enum {
    TS_IONIAN, TS_DORIAN, TS_PHRYGIAN, TS_LYDIAN, TS_MIXOLYDIAN, TS_AEOLIAN, TS_LOCRIAN,  /* church modes */
    TS_HARM_MINOR, TS_MEL_MINOR,                                                          /* minor variants */
    TS_PENT_MAJOR, TS_PENT_MINOR,                                                         /* pentatonic */
    TS_BLUES,
    TS_COUNT
};

uint16_t    tt_scale_mask(int scale);              /* 12-bit pitch-class set, relative to the root */
int         tt_scale_size(int scale);              /* number of notes per octave (5..7) */
const char *tt_scale_name(int scale);              /* short UI name, e.g. "DORIAN" */

/* Scale degree -> MIDI note. degree may be negative or span octaves (degree 7 of a 7-note scale
 * is the tonic one octave up). base_note = MIDI note of degree 0 (must be a tonic). */
int tt_degree_note(int scale, int base_note, int degree);

/* Snap any MIDI note to the nearest note of the scale on this root (ties go down). */
int tt_quantize(int note, int scale, int root);

/* ---- chords ------------------------------------------------------------------------------ */
enum { TQ_DIATONIC, TQ_MAJ, TQ_MIN, TQ_DIM, TQ_AUG, TQ_SUS2, TQ_SUS4, TQ_MAJ7, TQ_MIN7, TQ_DOM7, TQ_HDIM7, TQ_COUNT };

#define TT_CHORD_MAX 4

typedef struct {
    int8_t  root;                  /* semitones above the key's tonic, 0..11 */
    uint8_t n;                     /* number of tones */
    int8_t  tone[TT_CHORD_MAX];    /* semitones above the chord root, ascending, tone[0] = 0 */
} tt_chord;

/* Fixed-quality chord on a root (semitones above the tonic). */
void tt_chord_fixed(tt_chord *c, int root_semi, int quality);

/* Diatonic chord on a scale degree: stack thirds inside the scale. tones = 3 (triad) or 4 (7th).
 * Pentatonic and blues scales stack every other scale note, which gives their usual open voicings. */
void tt_chord_diatonic(tt_chord *c, int scale, int degree, int tones);

/* Chord tone k (any k, wraps upward by octaves) as a MIDI note, with the chord built on base_note
 * (= MIDI note of the key's tonic). */
int tt_chord_note(const tt_chord *c, int base_note, int k);

/* ---- progressions ------------------------------------------------------------------------ */
enum {
    TP_POP,          /* I-V-vi-IV */
    TP_JAZZ,         /* ii7-V7-Imaj7 */
    TP_EPIC,         /* i-VI-III-VII */
    TP_ANDALUSIAN,   /* i-VII-VI-V (V major) */
    TP_FOLK,         /* I-IV-I-V */
    TP_MODAL_VAMP,   /* diatonic degree 1-2: Dorian i-ii, Phrygian i-bII, Lydian I-II, Mixolydian I-ii */
    TP_MIXO_ROCK,    /* I-bVII-IV */
    TP_COUNT
};

#define TT_PROG_MAX 8

int         tt_prog_len(int prog);
const char *tt_prog_name(int prog);

/* Chord at step (wraps) of a progression. Steps marked diatonic follow the current scale, so a
 * modal vamp changes colour with the mode; fixed steps keep their quality in any scale. */
void tt_prog_chord(tt_chord *c, int prog, int step, int scale, int tones);

/* ---- held keys ---------------------------------------------------------------------------- */
/* Pitch-class set of the held MIDI notes (bit pc). The lowest held note is reported as the bass
 * pitch class through *bass_pc (or -1 when nothing is held). One key = root, more keys = chord. */
uint16_t tt_held_set(const uint8_t *notes, int n, int *bass_pc);

#endif
