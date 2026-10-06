/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA generator (see tuna_gen.h). */
#include <math.h>
#include "tuna_gen.h"

void tg_scale_for_chord(tm_scale *s, uint16_t scale_mask, const tt_chord *chord)
{
    /* chord tones first (root, fifth, third, seventh), then the other scale notes by importance;
     * the weights line up with the quantizer's thresholds 255 / 224 / 128 / 32 / 16 */
    static const uint8_t chord_weight[TT_CHORD_MAX] = {255, 192, 224, 160};   /* by chord tone index */
    static const uint8_t order[12] = {0, 7, 4, 3, 11, 10, 2, 1, 9, 8, 5, 6};
    static const uint8_t other_weight[] = {120, 96, 72, 48, 40, 36, 34, 33};
    s->base_interval = 1.0f;
    s->num_degrees = 12;
    for (int i = 0; i < 12; ++i) {
        s->degree[i].voltage = (float)i * 0.0833333333f;
        s->degree[i].weight = 16;
    }
    bool taken[12] = {false};
    for (int k = 0; k < chord->n; ++k) {
        int pc = (chord->root + chord->tone[k]) % 12;
        if (!taken[pc]) {
            s->degree[pc].weight = chord_weight[k];
            taken[pc] = true;
        }
    }
    int rank = 0;
    for (int k = 0; k < 12; ++k) {
        int pc = order[k];
        if (((scale_mask >> pc) & 1) && !taken[pc]) {
            s->degree[pc].weight = other_weight[rank < 7 ? rank : 7];
            rank++;
        }
    }
}

static void defaults(tg_gen *g)
{
    g->bpm = 96.0f;
    g->division = 2;
    g->root = 0;
    g->scale = TS_DORIAN;
    g->prog = TP_MODAL_VAMP;
    g->chord_ticks = 16;
    g->base_note[0] = 36;     /* bass from C2 */
    g->base_note[1] = 60;     /* melody from C4 */
    g->base_note[2] = 48;     /* pad from C3 */
    g->min_gap_s = 0.03f;
    g->clock_source = TM_CLOCK_T1_T2_T3;
    tm_group x = {TM_CONTROL_IDENTICAL, TM_RANGE_NARROW, 0.5f, 0.5f, 0.75f, 0.0f, 8, {1, 1}};
    tm_group y = {TM_CONTROL_IDENTICAL, TM_RANGE_POSITIVE, 0.5f, 0.5f, 0.0f, 0.0f, 8, {1, 4}};
    g->x = x;
    g->y = y;
}

void tg_reseed(tg_gen *g, uint32_t seed)
{
    g->seed = seed;
    /* keep the T settings across a reseed, restart its state */
    tm_tgen keep = g->t;
    tm_rng_init(&g->rng, seed);
    tm_t_init(&g->t, &g->rng, g->sample_rate);
    g->t.model = keep.model;
    g->t.range = keep.range;
    g->t.rate = keep.rate;
    g->t.bias = keep.bias;
    g->t.jitter = keep.jitter;
    g->t.pulse_width_mean = keep.pulse_width_mean;
    g->t.pulse_width_std = keep.pulse_width_std;
    g->t.sequence.deja_vu = keep.sequence.deja_vu;
    tm_seq_set_length(&g->t.sequence, keep.sequence.length);
    tm_xy_init(&g->xy, &g->rng);
    g->ticks = 0;
    g->chord_step = 0;
    g->prev_master = 0.0f;
    g->loaded_scale = g->loaded_prog = g->loaded_step = g->loaded_root = -1;
    for (int p = 0; p < TG_PARTS; ++p) {
        g->sounding[p] = -1;
        g->gate_prev[p] = false;
        g->since_note[p] = 1u << 30;
    }
}

void tg_init(tg_gen *g, uint32_t seed, float sample_rate)
{
    defaults(g);
    g->sample_rate = sample_rate;
    tm_rng_init(&g->rng, seed);
    tm_t_init(&g->t, &g->rng, sample_rate);
    g->t.model = TM_T_COMPLEMENTARY_BERNOULLI;
    g->t.pulse_width_mean = 0.5f;
    tg_reseed(g, seed);
}

static void reload_scale(tg_gen *g)
{
    if (g->loaded_scale == g->scale && g->loaded_prog == g->prog && g->loaded_step == g->chord_step &&
        g->loaded_root == g->root)
        return;
    tt_chord c;
    tt_prog_chord(&c, g->prog, g->chord_step, g->scale, 4);
    tm_scale s;
    tg_scale_for_chord(&s, tt_scale_mask(g->scale), &c);
    tm_xy_load_scale(&g->xy, -1, &s);
    g->allowed_pcs = tt_scale_mask(g->scale);
    for (int k = 0; k < c.n; ++k)
        g->allowed_pcs |= (uint16_t)(1u << ((c.root + c.tone[k]) % 12));
    g->loaded_scale = g->scale;
    g->loaded_prog = g->prog;
    g->loaded_step = g->chord_step;
    g->loaded_root = g->root;
}

static void note_off(tg_gen *g, int p, int offset, tg_event_fn fn, void *ctx)
{
    if (g->sounding[p] < 0)
        return;
    tg_event e = {(uint8_t)p, (uint8_t)g->sounding[p], 0, false, (uint16_t)offset};
    fn(ctx, &e);
    g->sounding[p] = -1;
}

void tg_all_off(tg_gen *g, tg_event_fn fn, void *ctx)
{
    for (int p = 0; p < TG_PARTS; ++p)
        note_off(g, p, 0, fn, ctx);
}

void tg_process(tg_gen *g, int size, tg_event_fn fn, void *ctx)
{
    static float master[TG_MAX_BLOCK], s0[TG_MAX_BLOCK], s1[TG_MAX_BLOCK], yr[TG_MAX_BLOCK];
    static float out[TG_MAX_BLOCK * TM_CHANNELS];
    static bool gate[TG_MAX_BLOCK * TM_T_CHANNELS];
    if (size > TG_MAX_BLOCK)
        size = TG_MAX_BLOCK;

    reload_scale(g);
    g->t.rate_hz = g->bpm / 60.0f * (float)g->division;
    tm_t_process(&g->t, NULL, master, s0, s1, gate, (size_t)size);
    tm_xy_process(&g->xy, g->clock_source, &g->x, &g->y, NULL, master, s0, s1, yr, out, NULL, (size_t)size);

    const uint32_t min_gap = (uint32_t)(g->min_gap_s * g->sample_rate);
    for (int i = 0; i < size; ++i) {
        if (master[i] < g->prev_master) {            /* a master clock tick */
            g->ticks++;
            if (g->chord_ticks > 0 && g->ticks % g->chord_ticks == 0) {
                g->chord_step++;
                /* the new chord applies from the next block: the quantizers are per block */
            }
        }
        g->prev_master = master[i];
        bool gates[TG_PARTS] = {gate[i * 2], master[i] < 0.5f, gate[i * 2 + 1]};
        float yv = out[i * TM_CHANNELS + 3];
        int velocity = 50 + (int)(70.0f * (yv / 5.0f));
        if (velocity < 1) velocity = 1;
        if (velocity > 127) velocity = 127;
        for (int p = 0; p < TG_PARTS; ++p) {
            g->since_note[p]++;
            bool rising = gates[p] && !g->gate_prev[p];
            bool falling = !gates[p] && g->gate_prev[p];
            g->gate_prev[p] = gates[p];
            if (falling)
                note_off(g, p, i, fn, ctx);
            if (rising && g->since_note[p] >= min_gap) {
                note_off(g, p, i, fn, ctx);
                int n = g->base_note[p] + g->root + (int)lrintf(out[i * TM_CHANNELS + p] * 12.0f);
                /* Tuna stays in key. Marbles leaves a channel unquantized at STEPS 0.5 and below
                 * (which the control modes' tilt and bump produce on some channels), and its
                 * lowest levels are chromatic: a note outside both the scale and the current chord
                 * is snapped into the scale. Chord tones outside the scale (the major V in minor)
                 * are kept. */
                int pc = ((n - g->root) % 12 + 12) % 12;
                if (!((g->allowed_pcs >> pc) & 1))
                    n = tt_quantize(n, g->scale, g->root);
                if (n < 0) n = 0;
                if (n > 127) n = 127;
                tg_event e = {(uint8_t)p, (uint8_t)n, (uint8_t)velocity, true, (uint16_t)i};
                fn(ctx, &e);
                g->sounding[p] = n;
                g->since_note[p] = 0;
            }
        }
    }
}
