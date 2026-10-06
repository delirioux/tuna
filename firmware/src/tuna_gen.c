/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA generator (see tuna_gen.h). */
#include <math.h>
#include "tuna_gen.h"

/* ---- pitch-class helpers --------------------------------------------------------------------------- */
static int pmod12(int n) { return ((n % 12) + 12) % 12; }

static bool in_mask(uint16_t mask, int note, int root) { return (mask >> pmod12(note - root)) & 1; }

/* the k-th note of mask above (k > 0) or below (k < 0) note; note itself need not be in mask */
static int mask_step(uint16_t mask, int root, int note, int k)
{
    if (!mask)
        return note;
    while (k > 0) {
        do note++; while (!in_mask(mask, note, root));
        k--;
    }
    while (k < 0) {
        do note--; while (!in_mask(mask, note, root));
        k++;
    }
    return note;
}

/* nearest note of mask, ties go down */
static int mask_nearest(uint16_t mask, int root, int note)
{
    if (!mask)
        return note;
    for (int d = 0; d < 12; ++d) {
        if (in_mask(mask, note - d, root))
            return note - d;
        if (in_mask(mask, note + d, root))
            return note + d;
    }
    return note;
}

static uint16_t chord_mask(const tt_chord *c, int tones)
{
    uint16_t m = 0;
    for (int k = 0; k < c->n && k < tones; ++k)
        m |= (uint16_t)(1u << pmod12(c->root + c->tone[k]));
    return m;
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

/* ---- scales ---------------------------------------------------------------------------------------- */
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
        int pc = pmod12(chord->root + chord->tone[k]);
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

uint16_t tg_chord_scale(uint16_t scale_mask, const tt_chord *chord)
{
    uint16_t m = scale_mask;
    for (int k = 0; k < chord->n; ++k) {
        int pc = pmod12(chord->root + chord->tone[k]);
        if ((m >> pc) & 1)
            continue;
        uint16_t cm = chord_mask(chord, chord->n);
        int below = pmod12(pc - 1), above = pmod12(pc + 1);
        if (((m >> below) & 1) && !((cm >> below) & 1))
            m &= (uint16_t)~(1u << below);
        else if (((m >> above) & 1) && !((cm >> above) & 1))
            m &= (uint16_t)~(1u << above);
        m |= (uint16_t)(1u << pc);
    }
    return m;
}

/* ---- setup ----------------------------------------------------------------------------------------- */
static void defaults(tg_gen *g)
{
    g->bpm = 96.0f;
    g->division = 2;
    g->root = 0;
    g->scale = TS_DORIAN;
    g->prog = TP_MODAL_VAMP;
    g->chord_ticks = 16;
    g->base_note[TG_PART_BASS] = 36;     /* bass from C2 */
    g->base_note[TG_PART_MELODY] = 60;   /* melody from C4 */
    g->base_note[TG_PART_PAD] = 52;      /* pad voicings start between E3 and E4 */
    g->min_gap_s = 0.03f;
    g->harmonize = true;
    g->melody_density = 0.5f;
    g->melody_range = 19;
    g->phrase_bars = 4;
    g->pad_tones = 3;
    g->pad_restrike = false;
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
    tm_rng_init(&g->hrng, seed ^ 0x5eed7a4au);
    tm_seq_init(&g->mel_rhythm, &g->hrng);
    g->ticks = 0;
    g->prev_master = 0.0f;
    g->chord_step = 0;
    g->loaded_scale = g->loaded_prog = g->loaded_step = g->loaded_root = -1;
    for (int p = 0; p < TG_PARTS; ++p) {
        g->sounding[p] = -1;
        g->gate_prev[p] = false;
        g->since_note[p] = 1u << 30;
    }
    g->pad_n = 0;
    g->mel_note = -1;
    g->mel_prev_interval = 0;
    g->bass_note = -1;
    g->chord_fresh = true;
    g->mel_hold_until = 0;
    g->mel_off_at = -1;
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
    tt_prog_chord(&g->chord, g->prog, g->chord_step, g->scale, 4);
    tm_scale s;
    tg_scale_for_chord(&s, tt_scale_mask(g->scale), &g->chord);
    tm_xy_load_scale(&g->xy, -1, &s);
    g->allowed_pcs = tt_scale_mask(g->scale) | chord_mask(&g->chord, g->chord.n);
    g->chord_scale = tg_chord_scale(tt_scale_mask(g->scale), &g->chord);
    g->loaded_scale = g->scale;
    g->loaded_prog = g->prog;
    g->loaded_step = g->chord_step;
    g->loaded_root = g->root;
}

/* ---- events ---------------------------------------------------------------------------------------- */
static void emit(tg_event_fn fn, void *ctx, int part, int note, int velocity, bool on, int offset)
{
    tg_event e = {(uint8_t)part, (uint8_t)clampi(note, 0, 127), (uint8_t)clampi(velocity, 1, 127), on,
                  (uint16_t)offset};
    if (!on)
        e.velocity = 0;
    fn(ctx, &e);
}

static void note_off(tg_gen *g, int p, int offset, tg_event_fn fn, void *ctx)
{
    if (g->sounding[p] < 0)
        return;
    emit(fn, ctx, p, g->sounding[p], 0, false, offset);
    g->sounding[p] = -1;
}

static void note_on(tg_gen *g, int p, int note, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    note_off(g, p, offset, fn, ctx);
    note = clampi(note, 0, 127);
    emit(fn, ctx, p, note, velocity, true, offset);
    g->sounding[p] = note;
    g->since_note[p] = 0;
}

static void pad_off(tg_gen *g, int offset, tg_event_fn fn, void *ctx)
{
    for (int i = 0; i < g->pad_n; ++i)
        emit(fn, ctx, TG_PART_PAD, g->pad[i], 0, false, offset);
    g->pad_n = 0;
}

void tg_all_off(tg_gen *g, tg_event_fn fn, void *ctx)
{
    for (int p = 0; p < TG_PARTS; ++p)
        note_off(g, p, 0, fn, ctx);
    pad_off(g, 0, fn, ctx);
}

/* ---- the harmonizer -------------------------------------------------------------------------------- */
typedef struct { bool downbeat, on_beat; int q, bar_ticks; } beat_pos;

static beat_pos position(const tg_gen *g)
{
    beat_pos b;
    b.q = (int)(g->ticks - 1);               /* index of the tick just started */
    b.bar_ticks = g->division * 4;
    b.downbeat = b.q % b.bar_ticks == 0;
    b.on_beat = b.q % g->division == 0;
    return b;
}

static int accent(int velocity, beat_pos b)
{
    return velocity + (b.downbeat ? 16 : b.on_beat ? 6 : -10);
}

/* normalise a channel's voltage to 0..1 within its range */
static float unit(const tg_gen *g, int ch, float v)
{
    float u = (v - g->xy.channel[ch].offset) / g->xy.channel[ch].scale;
    return u < 0.0f ? 0.0f : u > 1.0f ? 1.0f : u;
}

/* pad: voice the chord as close as possible to the previous voicing; common tones keep sounding */
static void pad_voice(tg_gen *g, int offset, int velocity, bool restrike, tg_event_fn fn, void *ctx)
{
    tt_chord c;
    tt_prog_chord(&c, g->prog, g->chord_step, g->scale, g->pad_tones);
    int n = c.n;
    int lo = g->base_note[TG_PART_PAD] + g->root;
    int best[TG_PAD_VOICES], best_cost = 1 << 30;
    for (int inv = 0; inv < n; ++inv) {
        int v[TG_PAD_VOICES];
        int first = g->root + c.root + c.tone[inv];
        while (first < lo) first += 12;
        while (first >= lo + 12) first -= 12;
        v[0] = first;
        for (int k = 1; k < n; ++k) {
            int pc = g->root + c.root + c.tone[(inv + k) % n];
            int x = v[k - 1] + 1;
            while (pmod12(x - pc) != 0) x++;
            v[k] = x;
        }
        int cost = 0;
        if (g->pad_n == n) {
            for (int k = 0; k < n; ++k)
                cost += v[k] > g->pad[k] ? v[k] - g->pad[k] : g->pad[k] - v[k];
        } else {
            int centre = lo + 9, mid = (v[0] + v[n - 1]) / 2;
            cost = mid > centre ? mid - centre : centre - mid;
        }
        if (cost < best_cost) {
            best_cost = cost;
            for (int k = 0; k < n; ++k)
                best[k] = v[k];
        }
    }
    /* release what does not continue, strike what is new (everything on a re-strike) */
    for (int i = 0; i < g->pad_n; ++i) {
        bool keep = false;
        for (int k = 0; k < n && !restrike; ++k)
            keep |= best[k] == g->pad[i];
        if (!keep)
            emit(fn, ctx, TG_PART_PAD, g->pad[i], 0, false, offset);
    }
    for (int k = 0; k < n; ++k) {
        bool held = false;
        for (int i = 0; i < g->pad_n && !restrike; ++i)
            held |= g->pad[i] == best[k];
        if (!held)
            emit(fn, ctx, TG_PART_PAD, best[k], velocity, true, offset);
    }
    for (int k = 0; k < n; ++k)
        g->pad[k] = best[k];
    g->pad_n = n;
    g->since_note[TG_PART_PAD] = 0;
}

static int bass_root_note(const tg_gen *g, int chord_root_pc)
{
    int lo = g->base_note[TG_PART_BASS] + g->root;
    int n = lo + pmod12(chord_root_pc);
    /* stay near the previous bass note rather than jumping by an octave */
    if (g->bass_note >= 0 && n + 12 - g->bass_note < g->bass_note - n && n + 12 <= lo + 16)
        n += 12;
    return n;
}

static void bass_play(tg_gen *g, beat_pos b, float u, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    const tt_chord *c = &g->chord;
    int root = bass_root_note(g, c->root);
    int note;
    int to_change = g->chord_ticks - (b.q % g->chord_ticks);       /* ticks until the next chord */
    if (b.downbeat || g->chord_fresh) {
        note = root;
    } else if (to_change <= g->division && g->chord_ticks > g->division) {
        /* approach: a scale step into the next chord's root */
        tt_chord next;
        tt_prog_chord(&next, g->prog, g->chord_step + 1, g->scale, 3);
        int target = bass_root_note(g, next.root);
        note = mask_step(g->chord_scale, g->root, target, u < 0.5f ? -1 : 1);
    } else if (u < 0.40f) {
        note = root;
    } else if (u < 0.65f) {
        note = root + 7;                                            /* fifth */
    } else if (u < 0.80f) {
        note = root + 12;                                           /* octave */
    } else {
        note = mask_nearest(chord_mask(c, 3), g->root, root + 3);   /* third */
    }
    g->chord_fresh = false;
    g->bass_note = note;
    note_on(g, TG_PART_BASS, note, accent(velocity, b), offset, fn, ctx);
}

/* The melody's period: a question and an answer, as long as whole cycles of the progression
 * (at least phrase_bars bars). It is anchored on the progression's tonic chord, so the answer
 * lands on the tonic exactly when the harmony returns home. */
static void period_of(const tg_gen *g, int bar_ticks, int *period, int *anchor)
{
    int len = tt_prog_len(g->prog);
    int cycle = len * g->chord_ticks;
    int min_ticks = (g->phrase_bars > 0 ? g->phrase_bars : 4) * bar_ticks;
    if (cycle <= 0) {
        *period = min_ticks;
        *anchor = 0;
        return;
    }
    *period = cycle * ((min_ticks + cycle - 1) / cycle);
    int tonic_step = 0;
    for (int s = 0; s < len; ++s) {
        tt_chord c;
        tt_prog_chord(&c, g->prog, s, g->scale, 3);
        if (c.root == 0)
            tonic_step = s;                     /* the last tonic chord: ii-V-I resolves on its I */
    }
    *anchor = tonic_step * g->chord_ticks;
}

int tg_period_position(const tg_gen *g, int *period)
{
    int p, a, bar = g->division * 4;
    period_of(g, bar, &p, &a);
    if (period)
        *period = p;
    int q = (int)(g->ticks - 1);
    return ((q - a) % p + p) % p;
}

static void melody_tick(tg_gen *g, beat_pos b, float x2, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    const int lo = g->base_note[TG_PART_MELODY] + g->root;
    const int hi = lo + g->melody_range;
    const uint16_t chord = chord_mask(&g->chord, 3);
    const uint16_t tonic = 1u;                  /* pitch class 0 relative to the key */
    if (g->mel_note < 0)
        g->mel_note = mask_nearest(chord, g->root, lo + g->melody_range / 2);

    int period;
    int d = tg_period_position(g, &period);     /* ticks since the last arrival on the tonic */
    int to_arrival = period - d;
    bool arrival = d == 0 && b.q > 0;
    bool approach = to_arrival == g->division;
    bool homing = !approach && to_arrival <= b.bar_ticks + g->division;
    bool half = period >= 4 * b.bar_ticks && d == period / 2;

    /* the tonic nearest the current note, inside the range */
    int home = mask_nearest(tonic, g->root, g->mel_note);
    if (home > hi) home -= 12;
    if (home < lo) home += 12;

    if (g->mel_off_at >= 0 && b.q >= g->mel_off_at) {          /* a held note's breath */
        note_off(g, TG_PART_MELODY, offset, fn, ctx);
        g->mel_off_at = -1;
    }
    if (!arrival && b.q < g->mel_hold_until)
        return;

    int note;
    if (arrival) {
        /* the answer: the tonic, over the tonic chord, held for three beats, then a breath */
        note = home;
        g->mel_hold_until = b.q + b.bar_ticks;
        g->mel_off_at = b.q + b.bar_ticks - g->division;
    } else if (approach) {
        /* the beat before: a neighbour of the tonic, the second above or the leading note below */
        int up = mask_step(g->chord_scale, g->root, home, 1);
        int down = mask_step(g->chord_scale, g->root, home, -1);
        int du = up > g->mel_note ? up - g->mel_note : g->mel_note - up;
        int dd = down > g->mel_note ? down - g->mel_note : g->mel_note - down;
        note = (dd <= du && down >= lo) || up > hi ? down : up;
        g->mel_hold_until = b.q + g->division;
    } else if (half) {
        /* the question: halfway, rest on the fifth or third of the chord, not the tonic */
        uint16_t open = (uint16_t)(chord_mask(&g->chord, 3) & ~chord_mask(&g->chord, 1));
        open &= (uint16_t)~tonic;
        if (!open)
            open = chord;
        note = mask_nearest(open, g->root, g->mel_note);
        if (note > hi) note -= 12;
        if (note < lo) note += 12;
        g->mel_hold_until = b.q + 2 * g->division;
        g->mel_off_at = b.q + g->division;
    } else {
        /* density: downbeats always play, other beats mostly, off-beats as the density says;
         * the choice comes from a deja-vu sequence, so rhythms recur like motifs */
        g->mel_rhythm.deja_vu = g->x.deja_vu;
        tm_seq_set_length(&g->mel_rhythm, g->x.length);
        float r = tm_seq_next_value(&g->mel_rhythm, false, 0.0f);
        float p_play = b.downbeat ? 1.0f : b.on_beat ? 0.25f + 0.75f * g->melody_density : g->melody_density;
        if (r >= p_play)
            return;

        /* interval from X2: mostly steps, sometimes a leap, in the chord-bent scale */
        float x = 2.0f * unit(g, TG_PART_MELODY, x2) - 1.0f;
        float m = fabsf(x);
        int dir = x < 0.0f ? -1 : 1;
        /* |x| near 0 repeats the note, then steps, thirds, and rarely a fourth or fifth */
        int k = m < 0.12f ? 0 : m < 0.62f ? 1 : m < 0.86f ? 2 : m < 0.96f ? 3 : 4;
        if (homing) {
            /* the last bar steers toward the tonic, by steps */
            dir = home > g->mel_note ? 1 : home < g->mel_note ? -1 : (x < 0.0f ? -1 : 1);
            k = m < 0.6f ? 1 : 2;
            int dist = home > g->mel_note ? home - g->mel_note : g->mel_note - home;
            if (dist <= 2)
                k = 1;
        } else if (g->mel_prev_interval >= 3 || g->mel_prev_interval <= -3) {
            dir = g->mel_prev_interval > 0 ? -1 : 1;            /* a leap is answered by a step back */
            k = m > 0.5f ? 2 : 1;
        }
        k *= dir;
        note = mask_step(g->chord_scale, g->root, g->mel_note, k);
        if (note > hi || note < lo) {
            k = -k;
            note = mask_step(g->chord_scale, g->root, g->mel_note, k);
        }
        note = clampi(note, lo, hi);
        if (b.downbeat) {
            /* downbeats on chord tones; other beats may carry passing notes, which keeps the line
             * stepwise instead of arpeggiated */
            int snapped = mask_nearest(chord, g->root, note);
            /* do not let the snap undo the motion */
            if (snapped == g->mel_note && k != 0)
                snapped = mask_step(chord, g->root, g->mel_note, k > 0 ? 1 : -1);
            /* at the edges of the range, turn around rather than jump an octave */
            if (snapped > hi)
                snapped = mask_step(chord, g->root, g->mel_note, -1);
            if (snapped < lo)
                snapped = mask_step(chord, g->root, g->mel_note, 1);
            note = snapped;
        }
        g->mel_prev_interval = k;
    }
    if (arrival || approach || half)
        g->mel_prev_interval = note - g->mel_note > 2 ? 3 : note - g->mel_note < -2 ? -3 : 0;
    g->mel_note = note;
    note_on(g, TG_PART_MELODY, note, accent(velocity, b), offset, fn, ctx);   /* legato: held until the next */
}

/* ---- process --------------------------------------------------------------------------------------- */
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
        const float *o = &out[i * TM_CHANNELS];
        int velocity = clampi(50 + (int)(70.0f * (o[3] / 5.0f)), 1, 127);
        bool tick = master[i] < g->prev_master;
        g->prev_master = master[i];
        for (int p = 0; p < TG_PARTS; ++p)
            g->since_note[p]++;

        if (tick) {
            g->ticks++;
            if (g->chord_ticks > 0 && g->ticks > 1 && (g->ticks - 1) % g->chord_ticks == 0) {
                g->chord_step++;
                g->chord_fresh = true;
            }
            reload_scale(g);
        }

        if (!g->harmonize) {
            /* raw Marbles: each part plays its X voltage on its T gate */
            bool gates[TG_PARTS] = {gate[i * 2], master[i] < 0.5f, gate[i * 2 + 1]};
            for (int p = 0; p < TG_PARTS; ++p) {
                bool rising = gates[p] && !g->gate_prev[p];
                bool falling = !gates[p] && g->gate_prev[p];
                g->gate_prev[p] = gates[p];
                if (falling)
                    note_off(g, p, i, fn, ctx);
                if (rising && g->since_note[p] >= min_gap) {
                    int n = g->base_note[p] + g->root + (int)lrintf(o[p] * 12.0f);
                    /* Marbles leaves a channel unquantized at STEPS 0.5 and below, and its lowest
                     * levels are chromatic: a note outside the scale and the chord is snapped in */
                    if (!in_mask(g->allowed_pcs, n, g->root))
                        n = tt_quantize(n, g->scale, g->root);
                    note_on(g, p, n, velocity, i, fn, ctx);
                }
            }
            continue;
        }

        beat_pos b = position(g);
        if (tick) {
            if (g->chord_fresh || g->pad_n == 0)
                pad_voice(g, i, accent(velocity, b) - 20, false, fn, ctx);
            if (g->chord_fresh && g->since_note[TG_PART_BASS] >= min_gap)
                bass_play(g, b, unit(g, TG_PART_BASS, o[0]), velocity, i, fn, ctx);
            melody_tick(g, b, o[1], velocity, i, fn, ctx);
        }
        bool g1 = gate[i * 2], g3 = gate[i * 2 + 1];
        bool bass_rise = g1 && !g->gate_prev[TG_PART_BASS];
        bool bass_fall = !g1 && g->gate_prev[TG_PART_BASS];
        bool pad_rise = g3 && !g->gate_prev[TG_PART_PAD];
        g->gate_prev[TG_PART_BASS] = g1;
        g->gate_prev[TG_PART_PAD] = g3;
        if (bass_fall && g->since_note[TG_PART_BASS] >= min_gap)
            note_off(g, TG_PART_BASS, i, fn, ctx);
        if (bass_rise && g->since_note[TG_PART_BASS] >= min_gap)
            bass_play(g, b, unit(g, TG_PART_BASS, o[0]), velocity, i, fn, ctx);
        if (pad_rise && g->pad_restrike && g->since_note[TG_PART_PAD] >= min_gap)
            pad_voice(g, i, accent(velocity, b) - 26, true, fn, ctx);
    }
}
