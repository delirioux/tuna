/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA composer signatures: the habits a style gives the harmonizer.
 *
 * A signature sets the harmony (which chords, how they move, how phrases cadence), the
 * accompaniment (bass and the third part), the melody (interval profile, motifs, ornaments,
 * density) and the tempo, meter, dynamics and rubato. It evokes a style; it does not quote works.
 *
 * Harmony is planned one period at a time (question + answer, see tuna_gen.h). The planner fills
 * the period's chord slots with chords from a functional vocabulary, weighted per style:
 *   slot 0             the tonic (the answer of the previous period lands here)
 *   slot N/2-2, N/2-1  pre-dominant, dominant: the half cadence ends the question
 *   slot N-2, N-1      pre-dominant, dominant (or pre-dominant twice: a plagal ending)
 *   the rest           a walk through tonic -> pre-dominant -> dominant -> tonic, with applied
 *                      dominants resolving to their targets and, per style, a pull toward root
 *                      motion by falling fifths.
 * A style may colour some periods with the whole-tone scale (augmented chords) before the
 * cadence brings the music home. */
#ifndef TUNA_STYLE_H
#define TUNA_STYLE_H

#include <stdbool.h>
#include <stdint.h>

enum {
    TG_BASS_MARBLES,   /* T1 decides (no style) */
    TG_BASS_ROOT,      /* the root on each chord change, held */
    TG_BASS_WALKING,   /* a note every beat, stepping into the next root */
    TG_BASS_BAR,       /* the root on every downbeat */
    TG_BASS_MARCH,     /* root on beat 1, fifth on beat 3 */
    TG_BASS_PEDAL,     /* the tonic held under the period (a drone), with its fifth */
};
enum {
    TG_PAD_BLOCK,      /* held chords, voice-led */
    TG_PAD_ALBERTI,    /* low, high, middle, high */
    TG_PAD_COUNTER,    /* a counter-melody */
    TG_PAD_BROKEN,     /* wide broken chords over two octaves */
    TG_PAD_AFTERBEAT,  /* the chord after the downbeat bass (oom-pah, Gymnopédie) */
    TG_PAD_PARALLEL,   /* chords planing under each melody note */
    TG_PAD_DOUBLE,     /* the melody doubled a third or sixth below */
    TG_PAD_REPEAT,     /* the chord repeated in eighths, driving */
    TG_PAD_ARPEGGIO,   /* rising and falling arpeggios in sixteenths */
};
enum { TG_PERC_OFF, TG_PERC_PERIOD, TG_PERC_KIT };
/* period percussion instruments, the order of tg_style.perc */
enum { TG_PI_TIMPANI, TG_PI_SNARE, TG_PI_BASS_DRUM, TG_PI_CYMBAL, TG_PI_TRIANGLE, TG_PI_TAMBOURINE, TG_PI_COUNT };
enum {
    TG_STYLE_BACH, TG_STYLE_MOZART, TG_STYLE_BEETHOVEN, TG_STYLE_BRAHMS, TG_STYLE_MAHLER,
    TG_STYLE_CHOPIN, TG_STYLE_SATIE, TG_STYLE_DEBUSSY, TG_STYLE_DVORAK, TG_STYLE_SAINT_SAENS,
    TG_STYLE_COUNT
};

enum { TF_T, TF_S, TF_D, TF_SEC };          /* tonic, pre-dominant, dominant, applied chord */

#define TG_VOCAB 22
#define TG_PLAN_MAX 16
#define TG_VOC_AUG_I 19                     /* vocabulary indices of the whole-tone colour chords */
#define TG_VOC_AUG_BVII 20

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
    uint8_t meter[8];                /* beat groups in ticks (0-terminated), e.g. {3, 3, 2}; chord_beats then
                                        counts whole cycles of the meter */
    uint8_t meter_bars;              /* bit i: group i starts a new bar (changing meters) */
    float swing, lilt;               /* off-beat eighths late; the waltz's second beat early */
    float melody_density;
    int melody_base, melody_range;
    float profile[4];                /* melody interval thresholds: repeat < step < third < fourth */
    int max_leap;                    /* scale steps of the widest leap (4 = a fifth) */
    uint16_t melody_mask;            /* restrict the melody to these pitch classes (0 = no) */
    float syncopation;               /* 0..1: shifts melody notes from beats to off-beats */
    float appoggiatura, suspension;  /* chances on a downbeat */
    bool chromatic;                  /* appoggiaturas lean from a semitone above */
    bool trill;                      /* trill on the note before the final tonic */
    int arrival_bars;                /* how long the final tonic is held */
    int bass_mode, pad_mode, pad_base, pad_tones;
    float fifth_bias;                /* weight multiplier for root motion down a fifth */
    float repeat;                    /* chance a period repeats the previous period's harmony */
    bool plagal;                     /* end periods IV - I instead of V - I */
    float whole_tone;                /* chance a period is coloured whole-tone */
    float motif;                     /* Marbles deja vu for melody and rhythm */
    int motif_beats;                 /* length of the motif */
    float motif_use;                 /* chance a later bar develops the motif instead of wandering */
    float var_invert, var_alter;     /* development: chance of inversion, of a changed ending */
    bool hemiola;                    /* 3/4: regroup the bars before a cadence in twos */
    float sforzando;                 /* chance of a sudden accent on a weak beat */
    float swell;                     /* 0..1: crescendo through the period into the arrival */
    float rubato;                    /* 0..1: slow into cadences, a tempo after */
    int perc_mode;                   /* TG_PERC_*: the default percussion */
    uint8_t perc[TG_PI_COUNT];       /* period percussion, 0..10 per instrument */
    uint8_t w_major[TG_VOCAB];       /* chord weights, major keys */
    uint8_t w_minor[TG_VOCAB];       /* chord weights, minor keys */
} tg_style;

const tg_style *tg_style_get(int id);
const tg_chord_def *tg_vocab(bool minor);

#endif
