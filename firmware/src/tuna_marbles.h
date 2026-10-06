/* SPDX-License-Identifier: MIT */
/* TUNA's port of Mutable Instruments Marbles' random section to C.
 *
 * Original: marbles/random/{random_sequence.h, t_generator.cc, x_y_generator.cc, output_channel.cc,
 * quantizer.cc, lag_processor.cc, distributions.h} and marbles/ramp/{slave_ramp.h, ramp_divider.h},
 * Copyright 2015 Emilie Gillet, MIT License (see tuna_marbles_tables.c for the full text).
 *
 * What is ported, line by line (tests/tuna_marbles_ref.cc checks it is bit-exact against the C++):
 *   - the random stream (fallback LCG) and RandomSequence with deja vu, record and replay;
 *   - the T generator on its internal clock: all 7 models, rate, range, bias, jitter, pulse width;
 *   - the X/Y generator on the internal clock sources: spread, bias, steps, deja vu, length,
 *     control modes, voltage ranges, the quantizer and the lag processor, the Y ramp divider.
 * Left out: the external clock (ramp extractor), register mode (sampling an input voltage) and the
 * scale recorder, none of which the FM-1 has an input for.
 *
 * Signals, as on the module:
 *   ramps   0..1 phases per sample; T2 is the master clock, T1 and T3 its slaves
 *   gates   T1 and T3, per sample
 *   X1..X3  voltages (1 V per octave), Y a smoother, slower voltage
 * The C names keep the original's so a reader can follow along. */
#ifndef TUNA_MARBLES_H
#define TUNA_MARBLES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ---- random source and sequences ----------------------------------------------------------- */
typedef struct { uint32_t state; } tm_rng;    /* RandomStream with only its fallback LCG */

void  tm_rng_init(tm_rng *r, uint32_t seed);
float tm_rng_float(tm_rng *r);

#define TM_DEJA_VU_BUFFER 16
#define TM_HISTORY_BUFFER 16

typedef struct {
    tm_rng *rng;
    float loop[TM_DEJA_VU_BUFFER];
    float history[TM_HISTORY_BUFFER];
    int loop_write_head, length, step, record_head, replay_head, replay_start;
    uint32_t replay_hash, replay_shift;
    float deja_vu;
    int redo_read, redo_write, redo_write_history;   /* indices; -1 stands for the original's NULL */
} tm_sequence;

void  tm_seq_init(tm_sequence *s, tm_rng *rng);
float tm_seq_next_value(tm_sequence *s, bool deterministic, float value);
void  tm_seq_next_vector(tm_sequence *s, float *dst, size_t n);
void  tm_seq_set_length(tm_sequence *s, int length);   /* 1..16; other values are ignored */
void  tm_seq_reset(tm_sequence *s);

/* ---- ratios and ramps ------------------------------------------------------------------------- */
typedef struct { int p, q; } tm_ratio;

typedef struct {
    float phase, max_phase, ratio, pulse_width, target;
    int pulse_length;
    bool bernoulli, must_complete;
} tm_slave_ramp;

typedef struct { int num_steps; float hysteresis, scale, offset; int quantized_value; } tm_hyst_quantizer2;
typedef struct { int quantized_value; } tm_hyst_quantizer;

/* ---- T generator -------------------------------------------------------------------------------- */
enum {
    TM_T_COMPLEMENTARY_BERNOULLI, TM_T_CLUSTERS, TM_T_DRUMS, TM_T_INDEPENDENT_BERNOULLI,
    TM_T_DIVIDER, TM_T_THREE_STATES, TM_T_MARKOV, TM_T_MODEL_COUNT
};
enum { TM_T_RANGE_0_25X, TM_T_RANGE_1X, TM_T_RANGE_4X };

#define TM_T_CHANNELS 2
#define TM_MARKOV_HISTORY 16

typedef struct {
    float one_hertz;
    int model, range;
    float rate, bias, jitter, pulse_width_mean, pulse_width_std;
    float rate_hz;                      /* TUNA: > 0 overrides rate/range with a tempo-synced clock */
    float master_phase, jitter_multiplier, phase_difference;
    int32_t divider_pattern_length;
    int32_t streak_counter[TM_MARKOV_HISTORY];
    int32_t markov_history[TM_MARKOV_HISTORY];
    int32_t markov_history_ptr;
    size_t drum_pattern_step, drum_pattern_index;
    tm_sequence sequence;
    tm_slave_ramp slave_ramp[TM_T_CHANNELS];
    tm_hyst_quantizer2 bias_quantizer;
} tm_tgen;

void tm_t_init(tm_tgen *t, tm_rng *rng, float sample_rate);
/* Render size samples. reset may be NULL. master[size], slave[ch][size], gate[size * 2] (T1, T3
 * interleaved), exactly as Marbles' Ramps and gate buffers. */
void tm_t_process(tm_tgen *t, bool *reset, float *master, float *slave0, float *slave1, bool *gate,
                  size_t size);

/* ---- quantizer ---------------------------------------------------------------------------------- */
#define TM_MAX_DEGREES 16
#define TM_NUM_THRESHOLDS 7

typedef struct {
    float base_interval;
    int num_degrees;
    struct { float voltage; uint8_t weight; } degree[TM_MAX_DEGREES];
} tm_scale;

void tm_scale_init_major(tm_scale *s);          /* Marbles' default: chromatic with major weights */
/* TUNA: a 12-tone scale from a pitch-class mask (bit 0 = tonic, as in tuna_theory.h). Scale notes
 * get weights by musical importance (tonic, fifth, third, seventh, second, sixth, fourth), so
 * STEPS from 0.5 to 1 goes: chromatic, the scale, then fewer and stronger notes, down to the
 * tonic alone. */
void tm_scale_from_mask(tm_scale *s, uint16_t mask);

typedef struct {
    float voltage[TM_MAX_DEGREES];
    struct { uint16_t bitmask; uint8_t first, last; } level[TM_NUM_THRESHOLDS];
    float feedback[TM_NUM_THRESHOLDS];
    float base_interval, base_interval_reciprocal;
    int num_degrees;
    tm_hyst_quantizer level_quantizer;
} tm_quantizer;

void  tm_quantizer_init(tm_quantizer *q, const tm_scale *s);
float tm_quantizer_process(tm_quantizer *q, float value, float amount, bool hysteresis);

/* ---- X/Y generator ------------------------------------------------------------------------------ */
enum { TM_RANGE_NARROW, TM_RANGE_POSITIVE, TM_RANGE_FULL };     /* +2 V, +5 V, -5..+5 V */
enum { TM_CLOCK_T1_T2_T3, TM_CLOCK_T1, TM_CLOCK_T2, TM_CLOCK_T3 };
enum { TM_CONTROL_IDENTICAL, TM_CONTROL_BUMP, TM_CONTROL_TILT };

#define TM_X_CHANNELS 3
#define TM_CHANNELS 4                   /* X1..X3 and Y */

typedef struct {
    int control_mode, voltage_range;
    float spread, bias, steps, deja_vu;
    int length;
    tm_ratio ratio;                     /* Y only: Y's clock as a ratio of the X2 clock */
} tm_group;

typedef struct {
    float ramp_start, ramp_value, lp_state, previous_phase;
} tm_lag;

typedef struct {
    float spread, bias, steps;
    float previous_steps, previous_phase, previous_voltage, voltage, quantized_voltage;
    float scale, offset;                /* ScaleOffset */
    tm_lag lag;
    tm_quantizer quantizer;             /* TUNA: one scale per channel instead of six slots */
} tm_output_channel;

typedef struct {
    float phase, train_phase, max_train_phase, f_ratio;
    int reset_counter;
    bool reset_at_next_pulse;
} tm_ramp_divider;

typedef struct {
    tm_sequence sequence[TM_CHANNELS];
    tm_output_channel channel[TM_CHANNELS];
    tm_ramp_divider ramp_divider;
    bool use_shifted_sequences[TM_CHANNELS];
} tm_xygen;

void tm_xy_init(tm_xygen *g, tm_rng *rng);
void tm_xy_load_scale(tm_xygen *g, int channel, const tm_scale *s);   /* channel -1: X1..X3 */
/* Render size samples from T's ramps. y_ramp[size] is scratch (Marbles' ramps.external).
 * output[size * 4]: X1, X2, X3, Y interleaved. tick[size * 4] (may be NULL) is set where a channel
 * drew a new value: TUNA's note-on points. */
void tm_xy_process(tm_xygen *g, int clock_source, const tm_group *x, const tm_group *y, bool *reset,
                   const float *master, const float *slave0, const float *slave1, float *y_ramp,
                   float *output, bool *tick, size_t size);

#endif
