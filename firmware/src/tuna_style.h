/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA composer signatures: the habits a style gives the harmonizer.
 *
 * A signature sets the harmony (which chords, how they move, how phrases cadence), the
 * accompaniment (bass and the third part), the melody (interval profile, density, ornaments)
 * and the tempo and meter. It evokes a style; it does not quote works.
 *
 * Harmony is planned one period at a time (question + answer, see tuna_gen.h). The planner fills
 * the period's chord slots with chords from a functional vocabulary, weighted per style:
 *   slot 0           the tonic (the answer of the previous period lands here)
 *   slot N/2-2, N/2-1  pre-dominant, dominant: the half cadence ends the question
 *   slot N-2, N-1    pre-dominant, dominant: the authentic cadence into the next tonic
 *   the rest         a walk through tonic -> pre-dominant -> dominant -> tonic, with secondary
 *                    dominants resolving to their targets and, per style, a pull toward
 *                    root motion by falling fifths (Bach's sequences). */
#ifndef TUNA_STYLE_H
#define TUNA_STYLE_H

#include <stdbool.h>
#include <stdint.h>

enum { TG_BASS_MARBLES, TG_BASS_ROOT, TG_BASS_WALKING };
enum { TG_PAD_BLOCK, TG_PAD_ALBERTI, TG_PAD_COUNTER };
enum { TG_STYLE_BACH, TG_STYLE_MOZART, TG_STYLE_COUNT };

enum { TF_T, TF_S, TF_D, TF_SEC };          /* tonic, pre-dominant, dominant, applied chord */

#define TG_VOCAB 16
#define TG_PLAN_MAX 16

typedef struct {
    int8_t semis;      /* chord root, semitones above the tonic */
    uint8_t quality;   /* TQ_* */
    uint8_t func;      /* TF_* */
    int8_t target;     /* TF_SEC: the vocabulary index it resolves to */
    const char *name;  /* roman numeral, for logs */
} tg_chord_def;

typedef struct {
    const char *name;
    int root, scale;                 /* default key */
    float bpm;
    int division, beats_per_bar, chord_beats, period_bars;
    float melody_density;
    int melody_base, melody_range;
    float profile[4];                /* melody interval thresholds: repeat < step < third < fourth */
    float appoggiatura, suspension;  /* chances on a downbeat */
    bool trill;                      /* trill on the note before the final tonic */
    int bass_mode, pad_mode, pad_base, pad_tones;
    float fifth_bias;                /* weight multiplier for root motion down a fifth */
    float motif;                     /* Marbles deja vu for melody and rhythm: how much motifs recur */
    float repeat;                    /* chance a period repeats the previous period's harmony */
    uint8_t w_major[TG_VOCAB];       /* chord weights, major keys */
    uint8_t w_minor[TG_VOCAB];       /* chord weights, minor keys */
} tg_style;

const tg_style *tg_style_get(int id);
const tg_chord_def *tg_vocab(bool minor);

#endif
