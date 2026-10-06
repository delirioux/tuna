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
 *             strong beats land on chord tones; phrases of two bars end on a chord tone and
 *             breathe for a beat; a density control and a deja-vu rhythm decide which ticks play;
 *             notes are held until the next one (legato).
 *     bass    the chord root on the downbeat and on each chord change, then root, fifth, octave
 *             or third chosen by X1, and a step into the next chord's root just before it.
 *     pad     the chord itself, voiced close to the previous chord (smallest total movement),
 *             struck on each chord change and re-struck on T3.
 *     all     accents by beat position on top of Y's velocity.
 * Every Marbles parameter stays reachable through gen->t, gen->x and gen->y. */
#ifndef TUNA_GEN_H
#define TUNA_GEN_H

#include <stdbool.h>
#include <stdint.h>
#include "tuna_marbles.h"
#include "tuna_theory.h"

#define TG_PARTS 3
#define TG_PAD_VOICES TT_CHORD_MAX

enum { TG_PART_BASS, TG_PART_MELODY, TG_PART_PAD };

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
    int root;                     /* key tonic pitch class, 0 = C */
    int scale;                    /* TS_* */
    int prog;                     /* TP_* */
    int chord_ticks;              /* master ticks per chord */
    int base_note[TG_PARTS];      /* MIDI note of the bottom of each part's range (before the root) */
    float min_gap_s;              /* shortest time between two notes of a part (runaway guard) */
    bool harmonize;               /* false: raw Marbles notes */
    float melody_density;         /* 0..1: share of off-beat ticks the melody plays */
    int melody_range;             /* semitones above base_note the melody may use */
    int phrase_bars;              /* melody phrase length; 0 = no phrasing */
    int pad_tones;                /* 3 = triads, 4 = sevenths */
    bool pad_restrike;            /* re-strike the pad chord on T3 */

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
    tm_rng hrng;                  /* the harmonizer's own randomness: Marbles stays untouched */
    tm_sequence mel_rhythm;       /* which off-beat ticks the melody plays, with deja vu */
} tg_gen;

void tg_init(tg_gen *g, uint32_t seed, float sample_rate);
/* Restart from the seed: the same seed and settings play the same music again. */
void tg_reseed(tg_gen *g, uint32_t seed);
/* Render size samples (size <= TG_MAX_BLOCK) and report note events through fn. */
#define TG_MAX_BLOCK 64
void tg_process(tg_gen *g, int size, tg_event_fn fn, void *ctx);
/* Release every sounding note (stop). */
void tg_all_off(tg_gen *g, tg_event_fn fn, void *ctx);

/* Marbles scale weighted for a chord inside a key scale (pitch classes relative to the tonic). */
void tg_scale_for_chord(tm_scale *s, uint16_t scale_mask, const tt_chord *chord);
/* The key scale with chord tones forced in: a chord tone outside the scale replaces the scale
 * note a semitone away (A minor over E major: G becomes G#). */
uint16_t tg_chord_scale(uint16_t scale_mask, const tt_chord *chord);

#endif
