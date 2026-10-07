/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA generator: turns the Marbles port (tuna_marbles.c) into music for three synth parts.
 *
 *   part 0  bass    T1 gate + X1     part 1  melody  T2 (master clock) + X2
 *   part 2  pad     T3 gate + X3     Y       slow random value -> velocity
 *
 * Two ways to read Marbles:
 *   raw         Marbles' quantized voltages are the notes (the module's own behaviour).
 *   harmonized  Marbles still makes every random choice, but a musician's rules turn them into
 *               notes (the default):
 *     melody  X2 picks the next *interval*, mostly steps; large leaps are followed by a step back;
 *             downbeats land on chord tones; a density control and a deja-vu rhythm decide which
 *             ticks play; notes are held until the next one (legato). Phrases form periods: a
 *             question that pauses on the fifth or third halfway, and an answer whose last bar
 *             steers home, whose last beat is a neighbour of the tonic, and which lands on the
 *             tonic exactly when the progression returns to its tonic chord.
 *     bass    the chord root on the downbeat and on each chord change, then root, fifth, octave
 *             or third chosen by X1, and a step into the next chord's root just before it.
 *     pad     the chord itself, voiced close to the previous chord (smallest total movement),
 *             struck on each chord change and re-struck on T3.
 *     all     accents by beat position on top of Y's velocity.
 * A composer signature (tuna_style.h, tg_apply_style) replaces the fixed progression with
 * planned functional harmony and can swap the bass (root, walking) and the third part (Alberti
 * figures, a counter-melody), and add ornaments (appoggiatura, suspension, trill).
 * Every Marbles parameter stays reachable through gen->t, gen->x and gen->y. */
#ifndef TUNA_GEN_H
#define TUNA_GEN_H

#include <stdbool.h>
#include <stdint.h>
#include "tuna_marbles.h"
#include "tuna_theory.h"
#include "tuna_style.h"

#define TG_PARTS 3
#define TG_PAD_VOICES TT_CHORD_MAX

enum { TG_PART_BASS, TG_PART_MELODY, TG_PART_PAD, TG_PART_DRUMS, TG_PART_TIMPANI };
#define TG_EVENT_PARTS 5          /* the three voices, plus drums (GM note numbers) and pitched timpani */

/* General MIDI percussion notes used by the drum part */
#define TG_GM_BASS_DRUM 35
#define TG_GM_KICK 36
#define TG_GM_SIDE_STICK 37
#define TG_GM_SNARE 38
#define TG_GM_HAT_CLOSED 42
#define TG_GM_HAT_OPEN 46
#define TG_GM_CRASH 49
#define TG_GM_TAMBOURINE 54
#define TG_GM_TRIANGLE 81

typedef struct {
    uint8_t part, note, velocity;
    bool on;
    uint16_t offset;              /* sample offset in the processed block */
} tg_event;

typedef void (*tg_event_fn)(void *ctx, const tg_event *e);

typedef struct {
    /* settings (change any time) */
    float bpm;
    int division;                 /* master ticks per beat: 2 = eighths, 4 = sixteenths */
    int beats_per_bar;            /* meter: 4 = 4/4, 3 = 3/4 */
    int root;                     /* key tonic pitch class, 0 = C */
    int scale;                    /* TS_* */
    int prog;                     /* TP_* */
    int chord_ticks;              /* master ticks per chord */
    int base_note[TG_PARTS];      /* MIDI note of the bottom of each part's range (before the root) */
    float min_gap_s;              /* shortest time between two notes of a part (runaway guard) */
    bool harmonize;               /* false: raw Marbles notes */
    float melody_density;         /* 0..1: share of off-beat ticks the melody plays */
    int melody_range;             /* semitones above base_note the melody may use */
    int phrase_bars;              /* shortest melody period (question + answer) in bars */
    int pad_tones;                /* 3 = triads, 4 = sevenths */
    bool pad_restrike;            /* re-strike the pad chord on T3 */
    int bass_mode;                /* TG_BASS_* */
    int pad_mode;                 /* TG_PAD_* */
    float appoggiatura;           /* chance of an appoggiatura on a melody downbeat */
    float suspension;             /* chance of a suspension where the chord changes */
    bool trill;                   /* trill on the note before the final tonic */
    float mel_profile[4];         /* melody interval thresholds: repeat < step < third < fourth */
    int max_leap;                 /* scale steps of the widest melody leap */
    uint16_t melody_mask;         /* restrict the melody to these pitch classes (0 = no) */
    float syncopation;            /* 0..1: melody notes move from beats to off-beats */
    bool chromatic;               /* appoggiaturas lean from a semitone above */
    int arrival_bars;             /* how long the final tonic is held */
    bool plagal;                  /* periods end IV - I */
    float whole_tone;             /* chance of a whole-tone period */
    int motif_beats;              /* motif length */
    float motif_use;              /* chance a later segment develops the motif */
    float var_invert, var_alter;  /* development: inversion, changed ending */
    bool hemiola;                 /* 3/4: twos against threes before cadences */
    float sforzando;              /* chance of an accent on a weak beat */
    float swell;                  /* crescendo through the period */
    float rubato;                 /* slowing into cadences */
    int perc_mode;                /* TG_PERC_OFF, TG_PERC_PERIOD (the composer's orchestra), TG_PERC_KIT */
    uint8_t perc_w[TG_PI_COUNT];  /* period percussion: 0..10 per instrument */
    const tg_style *style;        /* NULL: the fixed progression `prog` */

    /* Marbles */
    tm_rng rng;
    tm_tgen t;
    tm_xygen xy;
    tm_group x, y;
    int clock_source;

    /* state */
    float sample_rate;
    uint32_t seed;
    long ticks;
    float prev_master;
    int chord_step;
    int loaded_scale, loaded_prog, loaded_step, loaded_root;
    uint16_t allowed_pcs;         /* scale + current chord tones, relative to the tonic */
    uint16_t chord_scale;         /* the scale bent to fit the chord (G -> G# over E major in A minor) */
    tt_chord chord;
    int sounding[TG_PARTS];       /* note held by bass and melody, -1 = none */
    int pad[TG_PAD_VOICES], pad_n;
    bool gate_prev[TG_PARTS];
    uint32_t since_note[TG_PARTS];
    int mel_note, mel_prev_interval, bass_note;
    bool chord_fresh;             /* the chord changed and the bass has not played it yet */
    long mel_hold_until;          /* tick before which the melody holds its note */
    long mel_off_at;              /* tick at which a held melody note is released, -1 = none */
    tm_rng hrng;                  /* the harmonizer's own randomness: Marbles stays untouched */
    tm_sequence mel_rhythm;       /* which off-beat ticks the melody plays, with deja vu */
    int8_t plan[TG_PLAN_MAX];     /* the period's chords, vocabulary indices (with a style) */
    int plan_len;
    bool plan_minor;
    int mel_pending;              /* a melody note due at mel_pending_q (ornament resolution), -1 = none */
    long mel_pending_q;
    int trill_main, trill_upper;
    long trill_start, trill_until;
    int mel_last_dir;             /* direction of the melody's last move: -1, 0, 1 */
    int counter_note, counter_prev_iv, counter_prev_mel;
    bool pad_silent;              /* the pad voicing is a reference for figures, not sounding */
    bool plan_wt;                 /* this period is coloured whole-tone */
    uint16_t mel_scale;           /* the melody's notes: chord scale, limited by melody_mask */
    /* the motif: rhythm and scale-step intervals of the period's first segment */
#define TG_MOTIF_MAX 32
    bool motif_play[TG_MOTIF_MAX];
    int8_t motif_k[TG_MOTIF_MAX];
    int motif_len, motif_seg, motif_start_note, motif_last;
    bool motif_valid, motif_recording, motif_active, motif_invert, motif_alter;
    long motif_notes, free_notes;  /* statistics: notes from development vs free invention */
    /* percussion: its own randomness, so switching drums on or off never changes the notes */
    tm_rng prng;
    int drum_on[8], drum_n;
    int timp_note;
    long timp_off_at, roll_until;
    int roll_note;
    float roll_countdown;          /* samples to the roll's in-between stroke */
    bool march_on, kit_g1_prev, kit_g3_prev;
    long perc_hits;
} tg_gen;

void tg_init(tg_gen *g, uint32_t seed, float sample_rate);
/* Restart from the seed: the same seed and settings play the same music again. */
void tg_reseed(tg_gen *g, uint32_t seed);
/* Render size samples (size <= TG_MAX_BLOCK) and report note events through fn. */
#define TG_MAX_BLOCK 64
void tg_process(tg_gen *g, int size, tg_event_fn fn, void *ctx);
/* Ticks since the melody's last arrival on the tonic, for the current tick (0 = arriving now);
 * *period gets the period's length in ticks. */
int tg_period_position(const tg_gen *g, int *period);
/* Tick within the period where the question ends (the half cadence). */
int tg_half_position(const tg_gen *g);
/* Apply a composer signature (TG_STYLE_*): key, tempo, meter, harmony, accompaniment, ornaments.
 * Takes effect from the next period; call tg_reseed() to start cleanly. */
void tg_apply_style(tg_gen *g, int style);
/* The chord at a chord step (the planned harmony with a style, else the progression). */
void tg_chord_at(const tg_gen *g, int step, int tones, tt_chord *c);
/* Percussion: TG_PERC_OFF, TG_PERC_PERIOD or TG_PERC_KIT. A style sets its default. */
void tg_set_percussion(tg_gen *g, int mode);
/* Release every sounding note (stop). */
void tg_all_off(tg_gen *g, tg_event_fn fn, void *ctx);

/* Marbles scale weighted for a chord inside a key scale (pitch classes relative to the tonic). */
void tg_scale_for_chord(tm_scale *s, uint16_t scale_mask, const tt_chord *chord);
/* The key scale with chord tones forced in: a chord tone outside the scale replaces the scale
 * note a semitone away (A minor over E major: G becomes G#). */
uint16_t tg_chord_scale(uint16_t scale_mask, const tt_chord *chord);

#endif
