/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA generator: turns the Marbles port (tuna_marbles.c) into notes for three synth parts.
 *
 *   part 0  bass    T1 gate + X1     part 1  melody  T2 (master clock) + X2
 *   part 2  pad     T3 gate + X3     Y       slow random value -> velocity
 *
 * Pitches are quantized to the key's scale, with the current chord of the progression weighted
 * highest: STEPS from the middle up goes chromatic -> scale -> chord tones -> root and fifth.
 * Every tm_* parameter of Marbles stays reachable through gen->t, gen->x and gen->y. */
#ifndef TUNA_GEN_H
#define TUNA_GEN_H

#include <stdbool.h>
#include <stdint.h>
#include "tuna_marbles.h"
#include "tuna_theory.h"

#define TG_PARTS 3

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
    int base_note[TG_PARTS];      /* MIDI note of 0 V per part (before the key's root) */
    float min_gap_s;              /* shortest time between two notes of a part (runaway guard) */

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
    int sounding[TG_PARTS];       /* note held by each part, -1 = none */
    bool gate_prev[TG_PARTS];
    uint32_t since_note[TG_PARTS];
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

#endif
