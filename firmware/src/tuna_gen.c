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

static void plan_period(tg_gen *g);

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
    g->beats_per_bar = 4;
    g->bass_mode = TG_BASS_MARBLES;
    g->pad_mode = TG_PAD_BLOCK;
    g->appoggiatura = 0.0f;
    g->suspension = 0.0f;
    g->trill = false;
    g->mel_profile[0] = 0.12f;
    g->mel_profile[1] = 0.62f;
    g->mel_profile[2] = 0.86f;
    g->mel_profile[3] = 0.96f;
    g->max_leap = 4;
    g->melody_mask = 0;
    g->syncopation = 0.0f;
    g->chromatic = false;
    g->arrival_bars = 1;
    g->plagal = false;
    g->whole_tone = 0.0f;
    g->motif_beats = 4;
    g->motif_use = 0.5f;
    g->var_invert = 0.15f;
    g->var_alter = 0.3f;
    g->hemiola = false;
    g->sforzando = 0.0f;
    g->swell = 0.0f;
    g->rubato = 0.0f;
    g->style = 0;
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
    g->mel_pending = -1;
    g->mel_pending_q = 0;
    g->trill_until = -1;
    g->trill_start = 0;
    g->mel_last_dir = 0;
    g->counter_note = -1;
    g->counter_prev_iv = -1;
    g->counter_prev_mel = -1;
    g->pad_silent = false;
    g->plan_wt = false;
    g->motif_valid = g->motif_recording = g->motif_active = false;
    g->motif_invert = g->motif_alter = false;
    g->motif_len = 0;
    g->motif_seg = -1;
    g->motif_start_note = -1;
    g->motif_last = -1;
    g->motif_notes = g->free_notes = 0;
    g->plan_len = 0;
    plan_period(g);
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

/* ---- harmony: the fixed progression, or a style's planned periods ---------------------------------- */
static bool key_is_minor(const tg_gen *g) { return (tt_scale_mask(g->scale) >> 3) & 1; }

void tg_chord_at(const tg_gen *g, int step, int tones, tt_chord *c)
{
    if (!g->style || g->plan_len <= 0) {
        tt_prog_chord(c, g->prog, step, g->scale, tones);
        return;
    }
    int slot = ((step % g->plan_len) + g->plan_len) % g->plan_len;
    /* a step in the next period that is not planned yet starts on the tonic */
    int idx = step / g->plan_len > g->chord_step / g->plan_len ? 0 : g->plan[slot];
    const tg_chord_def *d = &tg_vocab(g->plan_minor)[idx];
    tt_chord_fixed(c, d->semis, d->quality);
    if (tones < c->n)
        c->n = (uint8_t)tones;
}

static float hrand(tg_gen *g) { return tm_rng_float(&g->hrng); }

/* pick a vocabulary entry: allowed[i] gives 0/1, weights from the style, root motion down a fifth
 * from prev_semis multiplied by the style's fifth bias */
static int pick_chord(tg_gen *g, const bool *allowed, int prev_semis)
{
    const tg_chord_def *v = tg_vocab(g->plan_minor);
    const uint8_t *w = g->plan_minor ? g->style->w_minor : g->style->w_major;
    float weight[TG_VOCAB], sum = 0.0f;
    for (int i = 0; i < TG_VOCAB; ++i) {
        weight[i] = allowed[i] ? (float)w[i] : 0.0f;
        if (weight[i] > 0.0f && prev_semis >= 0 && pmod12(v[i].semis - prev_semis) == 5)
            weight[i] *= g->style->fifth_bias;
        sum += weight[i];
    }
    if (sum <= 0.0f)
        return 0;
    float r = hrand(g) * sum;
    for (int i = 0; i < TG_VOCAB; ++i) {
        r -= weight[i];
        if (r < 0.0f && weight[i] > 0.0f)
            return i;
    }
    return 0;
}

static void plan_period(tg_gen *g)
{
    if (!g->style)
        return;
    const tg_style *st = g->style;
    int n = st->period_bars * g->beats_per_bar / (st->chord_beats > 0 ? st->chord_beats : 1);
    if (n < 2) n = 2;
    if (n > TG_PLAN_MAX) n = TG_PLAN_MAX;
    bool minor = key_is_minor(g);
    if (g->plan_len == n && g->plan_minor == minor && hrand(g) < st->repeat)
        return;                                     /* the period repeats its harmony: A A */
    g->plan_len = n;
    g->plan_minor = minor;
    const tg_chord_def *v = tg_vocab(minor);

    /* -1: free; TF_* + 10: a slot fixed to a function */
    int want[TG_PLAN_MAX];
    for (int i = 0; i < n; ++i) want[i] = -1;
    if (n >= 4) {
        want[n / 2 - 2] = TF_S + 10;
        want[n / 2 - 1] = TF_D + 10;                /* half cadence */
        want[n - 2] = TF_S + 10;
        want[n - 1] = (g->plagal ? TF_S : TF_D) + 10;   /* into the next tonic: V - I, or IV - I */
    } else {
        want[n - 1] = TF_D + 10;
    }
    /* the tonic: the plain triad or a coloured one (Imaj7, i7, a major tonic in minor) */
    {
        bool allowed[TG_VOCAB];
        for (int k = 0; k < TG_VOCAB; ++k)
            allowed[k] = v[k].semis == 0 && v[k].func == TF_T && v[k].quality != TQ_AUG;
        g->plan[0] = (int8_t)pick_chord(g, allowed, -1);
    }
    /* a whole-tone period: augmented chords drift until the cadence brings the key back */
    g->plan_wt = n >= 4 && st->whole_tone > 0.0f && hrand(g) < st->whole_tone;
    if (g->plan_wt) {
        for (int i = 0; i < n - 2; ++i)
            g->plan[i] = (int8_t)(i % 2 ? TG_VOC_AUG_BVII : TG_VOC_AUG_I);
        for (int i = 0; i < n; ++i)
            if (i < n - 2) want[i] = -2;            /* taken */
    }
    for (int i = 1; i < n; ++i) {
        if (want[i] == -2)
            continue;
        const tg_chord_def *prev = &v[g->plan[i - 1]];
        bool allowed[TG_VOCAB];
        if (prev->func == TF_SEC) {                 /* an applied chord resolves to its target */
            g->plan[i] = prev->target;
            continue;
        }
        int next_want = i + 1 < n ? want[i + 1] : TF_T + 10;
        for (int k = 0; k < TG_VOCAB; ++k) {
            const tg_chord_def *c = &v[k];
            bool ok;
            if (want[i] >= 10) {
                int f = want[i] - 10;
                ok = c->func == f;
                if (f == TF_S && c->func == TF_SEC && v[c->target].func == TF_D)
                    ok = true;                      /* V7/V before the dominant */
                if (f == TF_D)
                    ok = c->func == TF_D;
            } else if (prev->func == TF_D || prev->quality == TQ_AUG) {
                ok = c->func == TF_T;               /* dominants resolve */
            } else if (prev->func == TF_S) {
                ok = c->func == TF_S || c->func == TF_D;
            } else {
                ok = true;
            }
            if (k == g->plan[i - 1])
                ok = false;                         /* change the chord */
            if (c->quality == TQ_AUG)
                ok = false;                         /* only in whole-tone periods */
            if (c->func == TF_SEC && want[i] < 10 && (i + 1 >= n || want[i + 1] >= 10))
                ok = false;                         /* its target would land on a fixed slot */
            if (c->func == TF_D && next_want == TF_S + 10)
                ok = false;                         /* no dominant right before a pre-dominant */
            allowed[k] = ok;
        }
        g->plan[i] = (int8_t)pick_chord(g, allowed, prev->semis);
    }
}

void tg_apply_style(tg_gen *g, int id)
{
    const tg_style *st = tg_style_get(id);
    g->style = st;
    if (!st)
        return;
    g->root = st->root;
    g->scale = st->scale;
    g->bpm = st->bpm;
    g->division = st->division;
    g->beats_per_bar = st->beats_per_bar;
    g->chord_ticks = st->chord_beats * st->division;
    g->phrase_bars = st->period_bars;
    g->melody_density = st->melody_density;
    g->base_note[TG_PART_MELODY] = st->melody_base - st->root;
    g->melody_range = st->melody_range;
    for (int i = 0; i < 4; ++i)
        g->mel_profile[i] = st->profile[i];
    g->appoggiatura = st->appoggiatura;
    g->suspension = st->suspension;
    g->trill = st->trill;
    g->bass_mode = st->bass_mode;
    g->pad_mode = st->pad_mode;
    g->base_note[TG_PART_PAD] = st->pad_base - st->root;
    g->pad_tones = st->pad_tones;
    g->max_leap = st->max_leap > 0 ? st->max_leap : 4;
    g->melody_mask = st->melody_mask;
    g->syncopation = st->syncopation;
    g->chromatic = st->chromatic;
    g->arrival_bars = st->arrival_bars > 0 ? st->arrival_bars : 1;
    g->plagal = st->plagal;
    g->whole_tone = st->whole_tone;
    g->motif_beats = st->motif_beats > 0 ? st->motif_beats : 4;
    g->motif_use = st->motif_use;
    g->var_invert = st->var_invert;
    g->var_alter = st->var_alter;
    g->hemiola = st->hemiola;
    g->sforzando = st->sforzando;
    g->swell = st->swell;
    g->rubato = st->rubato;
    /* Marbles as the style wants it: a centred, moderately wide X, motifs recurring by deja vu */
    g->x.control_mode = TM_CONTROL_IDENTICAL;
    g->x.spread = 0.5f;
    g->x.bias = 0.5f;
    g->x.steps = 0.75f;
    g->x.deja_vu = st->motif;
    g->x.length = st->beats_per_bar * 2;
    g->plan_len = 0;
    plan_period(g);
}

static void reload_scale(tg_gen *g)
{
    if (g->loaded_scale == g->scale && g->loaded_prog == g->prog && g->loaded_step == g->chord_step &&
        g->loaded_root == g->root)
        return;
    tg_chord_at(g, g->chord_step, 4, &g->chord);
    tm_scale s;
    tg_scale_for_chord(&s, tt_scale_mask(g->scale), &g->chord);
    tm_xy_load_scale(&g->xy, -1, &s);
    g->allowed_pcs = tt_scale_mask(g->scale) | chord_mask(&g->chord, g->chord.n);
    g->chord_scale = tg_chord_scale(tt_scale_mask(g->scale), &g->chord);
    bool aug = g->chord.n >= 3 && g->chord.tone[1] == 4 && g->chord.tone[2] == 8;
    if (aug) {
        g->chord_scale = tt_scale_mask(TS_WHOLE_TONE);  /* an augmented chord floats in whole tones */
        g->allowed_pcs |= g->chord_scale;
    }
    g->mel_scale = g->chord_scale;
    if (g->melody_mask && !aug) {
        uint16_t m = (uint16_t)(g->chord_scale & g->melody_mask);
        int bits = 0;
        for (int i = 0; i < 12; ++i) bits += (m >> i) & 1;
        if (bits >= 4)
            g->mel_scale = m;
    }
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
    for (int i = 0; i < g->pad_n && !g->pad_silent; ++i)
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
    b.bar_ticks = g->division * g->beats_per_bar;
    b.downbeat = b.q % b.bar_ticks == 0;
    b.on_beat = b.q % g->division == 0;
    return b;
}

static int accent_plain(int velocity, beat_pos b)
{
    return velocity + (b.downbeat ? 16 : b.on_beat ? 6 : -10);
}

int tg_period_position(const tg_gen *g, int *period);

/* true in the two bars before an arrival of a 3/4 style with hemiola: beats regroup in twos */
static bool hemiola_zone(const tg_gen *g, beat_pos b, int *strong)
{
    if (!g->hemiola || g->beats_per_bar != 3)
        return false;
    int period, d = tg_period_position(g, &period);
    int to_arrival = period - d;
    if (to_arrival > 2 * b.bar_ticks)
        return false;
    int since = 2 * b.bar_ticks - to_arrival;          /* ticks into the six beats */
    *strong = since % (2 * g->division) == 0;
    return true;
}

/* accents: beat position, hemiola, sforzandi, and the swell through the period */
static int accent_g(tg_gen *g, int velocity, beat_pos b)
{
    int strong;
    int v;
    if (hemiola_zone(g, b, &strong))
        v = velocity + (strong ? 14 : -8);
    else
        v = accent_plain(velocity, b);
    if (g->sforzando > 0.0f && !b.on_beat && hrand(g) < g->sforzando)
        v += 34;
    if (g->swell > 0.0f) {
        int period, d = tg_period_position(g, &period);
        v += (int)(g->swell * 26.0f * (float)d / (float)period) - (int)(g->swell * 10.0f);
    }
    return v;
}
#define accent(v, b) accent_g(g, (v), (b))

/* normalise a channel's voltage to 0..1 within its range */
static float unit(const tg_gen *g, int ch, float v)
{
    float u = (v - g->xy.channel[ch].offset) / g->xy.channel[ch].scale;
    return u < 0.0f ? 0.0f : u > 1.0f ? 1.0f : u;
}

/* pad: voice the chord as close as possible to the previous voicing; common tones keep sounding */
static void pad_voice(tg_gen *g, int offset, int velocity, bool restrike, bool silent, tg_event_fn fn, void *ctx)
{
    tt_chord c;
    tg_chord_at(g, g->chord_step, g->pad_tones, &c);
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
    g->pad_silent = silent;
    if (silent) {
        for (int k = 0; k < n; ++k)
            g->pad[k] = best[k];
        g->pad_n = n;
        return;
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
        tg_chord_at(g, g->chord_step + 1, 3, &next);
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
    if (g->style && g->plan_len > 0) {
        *period = g->plan_len * g->chord_ticks;     /* a planned period starts on its tonic */
        *anchor = 0;
        return;
    }
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
    int p, a, bar = g->division * g->beats_per_bar;
    period_of(g, bar, &p, &a);
    if (period)
        *period = p;
    int q = (int)(g->ticks - 1);
    return ((q - a) % p + p) % p;
}

int tg_half_position(const tg_gen *g)
{
    int period;
    tg_period_position(g, &period);
    /* a planned period: the question ends on the half-cadence dominant (bar 4 of 8); else halfway */
    if (g->style && g->plan_len >= 4)
        return (g->plan_len / 2 - 1) * g->chord_ticks;
    return period / 2;
}

/* scale steps from a to b within mask (signed) */
static int mask_steps_between(uint16_t mask, int root, int a, int b)
{
    int k = 0, n = a;
    while (n < b && k < 24) { n = mask_step(mask, root, n, 1); k++; }
    while (n > b && k > -24) { n = mask_step(mask, root, n, -1); k--; }
    return k;
}

/* The motif engine. Each period's first segment (motif_beats long, after the arrival's hold)
 * is invented freely and recorded: which ticks play and the interval, in scale steps, of each
 * note. Later segments develop it, like a composer working with one idea:
 *   sequence     the same rhythm and intervals, starting on the chord tone nearest the last
 *                note, so the figure follows the harmony;
 *   inversion    the intervals upside down;
 *   new ending   the last interval invented afresh;
 *   restatement  after the half cadence the answer starts with the motif at its original pitch.
 * The cadence rules (half cadence, homing, approach, arrival) always take precedence. */
static void motif_segment(tg_gen *g, beat_pos b, int *seg, int *t)
{
    int period, d = tg_period_position(g, &period);
    int len = g->motif_beats * g->division;
    if (len < 1) len = 1;
    if (len > TG_MOTIF_MAX) len = TG_MOTIF_MAX;
    g->motif_len = len;
    int start = b.q >= period ? g->arrival_bars * b.bar_ticks : 0;    /* after the arrival's hold */
    if (start > period / 2) start = 0;
    int rel = d - start;
    *seg = rel < 0 ? -1 : rel / len;
    *t = rel < 0 ? 0 : rel % len;
}

static void melody_tick(tg_gen *g, beat_pos b, float x2, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    const int lo = g->base_note[TG_PART_MELODY] + g->root;
    const int hi = lo + g->melody_range;
    const uint16_t chord = chord_mask(&g->chord, 3);
    const uint16_t tonic = 1u;                  /* pitch class 0 relative to the key */
    const uint16_t ms = g->mel_scale ? g->mel_scale : g->chord_scale;
    if (g->mel_note < 0)
        g->mel_note = mask_nearest(chord, g->root, lo + g->melody_range / 2);

    int period;
    int d = tg_period_position(g, &period);     /* ticks since the last arrival on the tonic */
    int to_arrival = period - d;
    bool arrival = d == 0 && b.q > 0;
    bool approach = to_arrival == g->division;
    bool homing = !approach && to_arrival <= b.bar_ticks + g->division;
    int half_at = tg_half_position(g);
    bool half = period >= 4 * b.bar_ticks && d == half_at;

    /* motif bookkeeping happens on every tick, held or not, so recordings stay aligned */
    int seg, t;
    motif_segment(g, b, &seg, &t);
    if (seg != g->motif_seg && seg >= 0) {
        g->motif_seg = seg;
        if (seg == 0 && t == 0 && !(g->motif_valid && hrand(g) < (g->style ? g->style->repeat : 0.3f))) {
            for (int i = 0; i < TG_MOTIF_MAX; ++i) {
                g->motif_play[i] = false;
                g->motif_k[i] = 0;
            }
            g->motif_recording = true;                 /* a new idea */
            g->motif_start_note = -1;
            g->motif_active = false;
        } else if (seg >= 1) {
            if (g->motif_last < -1)
                g->motif_last = -g->motif_last - 2;   /* an unused restatement marker */
            if (g->motif_recording) {
                g->motif_recording = false;
                g->motif_last = -1;
                for (int i = 0; i < g->motif_len; ++i)
                    if (g->motif_play[i]) g->motif_last = i;
                g->motif_valid = g->motif_last >= 0;
            }
            /* develop it here? the segment after the half cadence restates it literally */
            bool restate = g->motif_len > 0 && seg * g->motif_len >= half_at && (seg - 1) * g->motif_len < half_at;
            g->motif_active = g->motif_valid && (restate || hrand(g) < g->motif_use);
            g->motif_invert = !restate && hrand(g) < g->var_invert;
            g->motif_alter = !restate && hrand(g) < g->var_alter;
            if (restate && g->motif_start_note >= 0)
                g->motif_last = -g->motif_last - 2;    /* marker: restart at the original pitch */
        } else if (seg == 0) {
            g->motif_active = g->motif_valid;          /* the idea is kept: it opens the period again */
            g->motif_invert = g->motif_alter = false;
        }
    }

    /* the tonic nearest the current note, inside the range */
    int home = mask_nearest(tonic, g->root, g->mel_note);
    if (home > hi) home -= 12;
    if (home < lo) home += 12;

    if (g->mel_off_at >= 0 && b.q >= g->mel_off_at) {          /* a held note's breath */
        note_off(g, TG_PART_MELODY, offset, fn, ctx);
        g->mel_off_at = -1;
    }
    if (approach) {
        /* the cadence outranks any ornament or hold still running: the approach must sound */
        g->mel_pending = -1;
        g->trill_until = -1;
        g->mel_hold_until = 0;
    }
    if (!arrival && b.q < g->trill_until) {
        /* a trill: upper and main note alternate, ending on the main note whatever the length */
        int left = (int)(g->trill_until - 1 - b.q);
        int n = left % 2 ? g->trill_upper : g->trill_main;
        note_on(g, TG_PART_MELODY, n, accent(velocity, b) - 8, offset, fn, ctx);
        return;
    }
    if (!arrival && g->mel_pending >= 0 && b.q >= g->mel_pending_q) {
        /* an ornament resolves: appoggiatura or suspension, a step down to the chord tone */
        g->mel_last_dir = g->mel_pending > g->mel_note ? 1 : g->mel_pending < g->mel_note ? -1 : 0;
        g->mel_note = g->mel_pending;
        g->mel_pending = -1;
        note_on(g, TG_PART_MELODY, g->mel_note, accent(velocity, b) - 10, offset, fn, ctx);
        return;
    }
    if (!arrival && b.q < g->mel_hold_until)
        return;
    g->mel_pending = -1;

    int note;
    bool from_motif = false;
    if (arrival) {
        /* the answer: the tonic, over the tonic chord, held, then a breath */
        note = home;
        g->mel_hold_until = b.q + g->arrival_bars * b.bar_ticks;
        g->mel_off_at = g->mel_hold_until - g->division;
    } else if (approach) {
        /* the beat before: a neighbour of the tonic, the second above or the leading note below */
        int up = mask_step(g->chord_scale, g->root, home, 1);
        int down = mask_step(g->chord_scale, g->root, home, -1);
        /* a real step: over a borrowed chord the bent scale can put a third there (A-flat over
         * D-flat major in F); then take the key's own neighbour */
        if (up - home > 2) up = mask_step(tt_scale_mask(g->scale), g->root, home, 1);
        if (home - down > 2) down = mask_step(tt_scale_mask(g->scale), g->root, home, -1);
        if (up - home > 2) up = home + 2;
        if (home - down > 2) down = home - 1;
        int du = up > g->mel_note ? up - g->mel_note : g->mel_note - up;
        int dd = down > g->mel_note ? down - g->mel_note : g->mel_note - down;
        note = (dd <= du && down >= lo) || up > hi ? down : up;
        g->mel_hold_until = b.q + g->division;
        if (g->trill && g->division >= 2) {
            /* the classical cadential trill: start on the upper note, end on the main note */
            g->trill_main = note;
            g->trill_upper = mask_step(g->chord_scale, g->root, note, 1);
            g->trill_start = b.q;
            g->trill_until = b.q + g->division;
            note = (g->division - 1) % 2 ? g->trill_upper : g->trill_main;
        }
    } else if (half) {
        /* the question: rest on the fifth or third of the chord, not the tonic */
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
        /* a suspension: where the chord changes, hold the melody note over it if it is not a
         * chord tone but the step below is, and resolve down on the next beat */
        bool chord_change = g->chord_ticks > 0 && b.q % g->chord_ticks == 0 && b.q > 0;
        if (chord_change && g->sounding[TG_PART_MELODY] == g->mel_note && g->suspension > 0.0f &&
            !in_mask(chord, g->mel_note, g->root)) {
            int below = mask_step(g->chord_scale, g->root, g->mel_note, -1);
            if (in_mask(chord, below, g->root) && below >= lo && hrand(g) < g->suspension) {
                g->mel_pending = below;
                g->mel_pending_q = b.q + g->division;
                g->mel_hold_until = g->mel_pending_q;
                return;
            }
        }

        float x = 2.0f * unit(g, TG_PART_MELODY, x2) - 1.0f;
        float m = fabsf(x);
        int k;
        bool develop = !homing && g->motif_active && !g->motif_recording && seg >= 0 && t < g->motif_len;
        if (develop) {
            /* the motif's rhythm decides whether this tick plays, its intervals where to */
            if (!g->motif_play[t])
                return;
            int first = 0;
            while (first < g->motif_len && !g->motif_play[first]) first++;
            int last = g->motif_last < -1 ? -g->motif_last - 2 : g->motif_last;
            if (t == first) {
                if (g->motif_last < -1 && g->motif_start_note >= 0) {
                    note = g->motif_start_note;                     /* restatement */
                    if (!in_mask(ms, note, g->root) && !in_mask(chord, note, g->root))
                        note = mask_nearest(chord, g->root, note);  /* recorded over another chord */
                    g->motif_last = last;
                } else {
                    note = mask_nearest(chord, g->root, g->mel_note);  /* sequence: on the harmony */
                }
                k = 0;
            } else {
                k = g->motif_k[t];
                if (g->motif_invert) k = -k;
                if (g->motif_alter && t == last)
                    k = (x < 0.0f ? -1 : 1) * (m < 0.5f ? 1 : 2);
                note = mask_step(ms, g->root, g->mel_note, k);
            }
            if (note > hi || note < lo) {
                k = -k;
                note = t == first ? note + (note > hi ? -12 : 12) : mask_step(ms, g->root, g->mel_note, k);
            }
            from_motif = true;
        } else {
            /* density: downbeats always play, other beats mostly, off-beats as the density says;
             * syncopation moves weight from beats to off-beats; a hemiola plays its twos */
            g->mel_rhythm.deja_vu = g->x.deja_vu;
            tm_seq_set_length(&g->mel_rhythm, g->x.length);
            float r = tm_seq_next_value(&g->mel_rhythm, false, 0.0f);
            float on = (0.25f + 0.75f * g->melody_density) * (1.0f - 0.6f * g->syncopation);
            float off = g->melody_density * (1.0f + g->syncopation);
            int strong;
            float p_play = b.downbeat ? 1.0f : b.on_beat ? on : off;
            if (hemiola_zone(g, b, &strong))
                p_play = strong ? 1.0f : 0.15f * g->melody_density;
            if (r >= p_play) {
                if (g->motif_recording && t < TG_MOTIF_MAX) g->motif_play[t] = false;
                return;
            }

            /* interval from X2: mostly steps, sometimes a leap */
            int dir = x < 0.0f ? -1 : 1;
            const float *pr = g->mel_profile;
            k = m < pr[0] ? 0 : m < pr[1] ? 1 : m < pr[2] ? 2 : m < pr[3] ? 3 : g->max_leap;
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
            note = mask_step(homing ? g->chord_scale : ms, g->root, g->mel_note, k);
            if (note > hi || note < lo) {
                k = -k;
                note = mask_step(homing ? g->chord_scale : ms, g->root, g->mel_note, k);
            }
        }
        note = clampi(note, lo, hi);
        if (b.downbeat) {
            /* downbeats on chord tones; other beats may carry passing notes */
            int snapped = mask_nearest(chord, g->root, note);
            if (snapped == g->mel_note && k != 0)
                snapped = mask_step(chord, g->root, g->mel_note, k > 0 ? 1 : -1);
            /* at the edges of the range, turn around rather than jump an octave */
            if (snapped > hi)
                snapped = mask_step(chord, g->root, g->mel_note, -1);
            if (snapped < lo)
                snapped = mask_step(chord, g->root, g->mel_note, 1);
            note = snapped;
            /* an appoggiatura: lean on the note above the chord tone, resolve half a beat later */
            if (g->appoggiatura > 0.0f && hrand(g) < g->appoggiatura) {
                int lean = g->chromatic ? note + 1 : mask_step(g->chord_scale, g->root, note, 1);
                if (!in_mask(chord, lean, g->root) && lean <= hi + 2) {
                    g->mel_pending = note;
                    g->mel_pending_q = b.q + (g->division >= 2 ? g->division / 2 : 1);
                    g->mel_hold_until = g->mel_pending_q;
                    note = lean;
                }
            }
        }
        if (g->motif_recording && t < TG_MOTIF_MAX && seg == 0) {
            int prev = g->mel_note;
            g->motif_play[t] = true;
            int target = g->mel_pending >= 0 ? g->mel_pending : note;
            g->motif_k[t] = (int8_t)mask_steps_between(ms, g->root, prev, target);
            if (g->motif_start_note < 0)
                g->motif_start_note = target;
        }
        g->mel_prev_interval = k;
        if (from_motif) g->motif_notes++; else g->free_notes++;
    }
    if (arrival || approach || half)
        g->mel_prev_interval = note - g->mel_note > 2 ? 3 : note - g->mel_note < -2 ? -3 : 0;
    g->mel_last_dir = note > g->mel_note ? 1 : note < g->mel_note ? -1 : 0;
    g->mel_note = note;
    note_on(g, TG_PART_MELODY, note, accent(velocity, b), offset, fn, ctx);   /* legato: held until the next */
}

/* ---- accompaniment figures -------------------------------------------------------------------------- */
/* bass, root mode: the chord root on each chord change, held */
static void bass_root(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    int note = bass_root_note(g, g->chord.root);
    g->bass_note = note;
    g->chord_fresh = false;
    note_on(g, TG_PART_BASS, note, accent(velocity, b) - 6, offset, fn, ctx);
}

/* bass, walking: a note every beat, the root on the chord change, steps toward the next root,
 * a chord tone mid-chord, and a neighbour of the next root on the last beat */
static void bass_walk(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    const int lo = g->base_note[TG_PART_BASS] + g->root, hi = lo + 19;
    int ct = g->chord_ticks > 0 ? g->chord_ticks : b.bar_ticks;
    int pos = b.q % ct;
    int beats_left = (ct - pos) / g->division;
    int note;
    if (pos == 0 || g->bass_note < 0) {
        note = bass_root_note(g, g->chord.root);
    } else {
        tt_chord next;
        tg_chord_at(g, g->chord_step + 1, 3, &next);
        int target = bass_root_note(g, next.root);
        int dir = target > g->bass_note ? 1 : target < g->bass_note ? -1 : (hrand(g) < 0.5f ? 1 : -1);
        if (beats_left <= 1) {
            note = mask_step(g->chord_scale, g->root, target, -dir);        /* the neighbour of the next root */
            if (note == g->bass_note)
                note = mask_step(g->chord_scale, g->root, target, dir);
        } else {
            note = mask_step(g->chord_scale, g->root, g->bass_note, dir);
            if (pos == ct / 2)                                              /* mid-chord: a chord tone */
                note = mask_nearest(chord_mask(&g->chord, 3), g->root, note);
        }
    }
    if (note > hi) note -= 12;
    if (note < lo) note += 12;
    g->bass_note = note;
    g->chord_fresh = false;
    note_on(g, TG_PART_BASS, note, accent(velocity, b) - 4, offset, fn, ctx);
}

/* third part, Alberti: low, high, middle, high through the chord, every tick */
static void pad_alberti(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    if (g->pad_n < 3)
        return;
    static const uint8_t figure[4] = {0, 2, 1, 2};
    int note = g->pad[figure[b.q % 4]];
    note_on(g, TG_PART_PAD, note, accent(velocity, b) - 30, offset, fn, ctx);
}

/* third part, counter-melody: below the melody, in contrary motion, consonant on strong beats,
 * no parallel fifths or octaves with the melody */
static void pad_counter(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    int every = g->division >= 2 ? g->division / 2 : 1;            /* eighth notes */
    if (b.q % every)
        return;
    const int lo = g->base_note[TG_PART_PAD] + g->root, hi = lo + 19;
    const int mel = g->mel_note;
    const uint16_t chord = chord_mask(&g->chord, 3);
    if (mel < 0)
        return;
    if (g->counter_note < 0)
        g->counter_note = mask_nearest(chord, g->root, clampi(mel - 9, lo, hi));
    int dir = g->mel_last_dir ? -g->mel_last_dir : (g->counter_note < (lo + hi) / 2 ? 1 : -1);
    int cand = mask_step(g->chord_scale, g->root, g->counter_note, dir);
    bool strong = b.q % g->division == 0;
    if (strong) {
        int best = cand, best_score = 1 << 20;
        bool mel_moved = g->counter_prev_mel >= 0 && mel != g->counter_prev_mel;
        int mel_dir = mel > g->counter_prev_mel ? 1 : -1;
        for (int n = cand - 4; n <= cand + 4; ++n) {
            if (!in_mask(chord, n, g->root) || n < lo || n > hi || n > mel - 3)
                continue;
            int iv = pmod12(mel - n);
            int score = (iv == 3 || iv == 4 || iv == 8 || iv == 9) ? 0 : (iv == 0 || iv == 7) ? 2 : iv == 5 ? 4 : 8;
            bool moved = n != g->counter_note;
            if ((iv == 0 || iv == 7) && iv == g->counter_prev_iv && mel_moved && moved &&
                (n > g->counter_note ? 1 : -1) == mel_dir)
                score += 20;                                         /* parallel fifth or octave */
            score += (n > cand ? n - cand : cand - n);
            if (score < best_score) {
                best_score = score;
                best = n;
            }
        }
        cand = best;
    }
    if (!strong) {
        /* between beats too: no parallel fifth or octave; try the other way, else hold */
        int mel_dir = mel > g->counter_prev_mel ? 1 : -1;
        for (int attempt = 0; attempt < 2; ++attempt) {
            int iv = pmod12(mel - cand);
            bool parallel = (iv == 0 || iv == 7) && iv == g->counter_prev_iv && g->counter_prev_mel >= 0 &&
                            mel != g->counter_prev_mel && cand != g->counter_note &&
                            (cand > g->counter_note ? 1 : -1) == mel_dir;
            if (!parallel)
                break;
            cand = attempt == 0 ? mask_step(g->chord_scale, g->root, g->counter_note, -dir) : g->counter_note;
        }
    }
    while (cand > mel - 3 && cand - 12 >= lo)
        cand -= 12;
    if (cand > hi) cand = mask_step(g->chord_scale, g->root, g->counter_note, -1);
    if (cand < lo) cand = mask_step(g->chord_scale, g->root, g->counter_note, 1);
    {
        /* last word: never a parallel fifth or octave; hold the note instead (oblique motion) */
        int iv = pmod12(mel - cand);
        int mel_dir = mel > g->counter_prev_mel ? 1 : -1;
        if ((iv == 0 || iv == 7) && iv == g->counter_prev_iv && g->counter_prev_mel >= 0 &&
            mel != g->counter_prev_mel && cand != g->counter_note && (cand > g->counter_note ? 1 : -1) == mel_dir)
            cand = g->counter_note;
    }
    g->counter_prev_iv = pmod12(mel - cand);
    g->counter_prev_mel = mel;
    g->counter_note = cand;
    note_on(g, TG_PART_PAD, cand, accent(velocity, b) - (strong ? 14 : 22), offset, fn, ctx);
}

/* bass, every bar: the root on each downbeat */
static void bass_bar(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    int note = bass_root_note(g, g->chord.root);
    g->bass_note = note;
    g->chord_fresh = false;
    note_on(g, TG_PART_BASS, note, accent(velocity, b) - 4, offset, fn, ctx);
}

/* bass, march: root on beat 1, fifth on beat 3 */
static void bass_march(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    int pos = b.q % b.bar_ticks;
    int root = bass_root_note(g, g->chord.root);
    int note;
    if (pos == 0 || g->chord_fresh)
        note = root;
    else if (g->beats_per_bar == 4 && pos == 2 * g->division)
        note = mask_nearest(chord_mask(&g->chord, 3), g->root, root + 7 > g->base_note[TG_PART_BASS] + g->root + 14 ? root - 5 : root + 7);
    else
        return;
    g->bass_note = note;
    g->chord_fresh = false;
    note_on(g, TG_PART_BASS, note, accent(velocity, b) - 2, offset, fn, ctx);
}

/* bass, pedal: the tonic held under the harmony, struck again every two bars */
static void bass_pedal(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    if (b.q % (2 * b.bar_ticks) != 0 && g->sounding[TG_PART_BASS] >= 0)
        return;
    int note = g->base_note[TG_PART_BASS] + g->root;
    g->bass_note = note;
    g->chord_fresh = false;
    note_on(g, TG_PART_BASS, note, accent(velocity, b) - 14, offset, fn, ctx);
}

/* the chord's tones from low upward: root, third, fifth (a seventh if voiced), spanning octaves */
static int chord_ladder(const tg_gen *g, int lo, int *out, int max)
{
    tt_chord c;
    tg_chord_at(g, g->chord_step, g->pad_tones, &c);
    int root = g->root + c.root;
    while (root < lo) root += 12;
    while (root >= lo + 12) root -= 12;
    int n = 0;
    for (int oct = 0; n < max && oct < 4; ++oct)
        for (int k = 0; k < c.n && n < max; ++k)
            out[n++] = root + 12 * oct + c.tone[k];
    return n;
}

/* third part, broken chords (Chopin): root, fifth, octave, tenth, twelfth, tenth over two octaves */
static void pad_broken(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    int lad[12], n = chord_ladder(g, g->base_note[TG_PART_PAD] + g->root, lad, 12);
    if (n < 6)
        return;
    tt_chord c;
    tg_chord_at(g, g->chord_step, 3, &c);
    /* ladder order: root, third, fifth, root+12, third+12, fifth+12 (triads) */
    static const uint8_t figure[6] = {0, 2, 3, 4, 5, 4};
    int pos = (g->chord_ticks > 0 ? b.q % g->chord_ticks : b.q) % 6;
    int note = lad[figure[pos] < n ? figure[pos] : n - 1];
    note_on(g, TG_PART_PAD, note, accent(velocity, b) - (pos ? 30 : 20), offset, fn, ctx);
}

/* third part, afterbeat (Gymnopédie, oom-pah): the bass takes beat 1, the chord comes on beat 2 */
static void pad_afterbeat(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    int pos = b.q % b.bar_ticks;
    if (pos == 0)
        pad_off(g, offset, fn, ctx);
    else if (pos == g->division)
        pad_voice(g, offset, accent(velocity, b) - 22, true, false, fn, ctx);
}

/* third part, parallel chords (Debussy): each new melody note gets the same chord shape under it,
 * a third and a fifth below inside the scale, sliding along with the tune */
static void pad_parallel(tg_gen *g, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    int mel = g->mel_note;
    uint16_t sc = g->chord_scale;
    pad_off(g, offset, fn, ctx);
    int a = mask_step(sc, g->root, mel, -2), bb = mask_step(sc, g->root, mel, -4);
    g->pad[0] = bb;
    g->pad[1] = a;
    g->pad_n = 2;
    g->pad_silent = false;
    emit(fn, ctx, TG_PART_PAD, bb, velocity - 26, true, offset);
    emit(fn, ctx, TG_PART_PAD, a, velocity - 22, true, offset);
    g->since_note[TG_PART_PAD] = 0;
}

/* third part, doubling (Brahms): the melody a third below, or a sixth below when it sits high */
static void pad_double(tg_gen *g, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    int lo = g->base_note[TG_PART_PAD] + g->root;
    int mel = g->mel_note;
    int steps = mel - lo > 16 ? -5 : -2;
    int note = mask_step(g->mel_scale ? g->mel_scale : g->chord_scale, g->root, mel, steps);
    if (note < lo)
        note = mask_step(g->chord_scale, g->root, mel, -2);
    note_on(g, TG_PART_PAD, note, velocity - 16, offset, fn, ctx);
}

/* third part, arpeggios (Saint-Saëns): sixteenths up and down the chord over two octaves */
static void pad_arpeggio(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    int lad[12], n = chord_ladder(g, g->base_note[TG_PART_PAD] + g->root, lad, 7);
    if (n < 3)
        return;
    int cycle = 2 * n - 2;
    int pos = (g->chord_ticks > 0 ? b.q % g->chord_ticks : b.q) % cycle;
    int note = pos < n ? lad[pos] : lad[cycle - pos];
    note_on(g, TG_PART_PAD, note, accent(velocity, b) - (pos == 0 ? 18 : 28), offset, fn, ctx);
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
    float bpm = g->bpm;
    if (g->harmonize && g->rubato > 0.0f && g->ticks > 0) {
        /* rubato: broaden through the last bar into the arrival, a tempo after it */
        int period, d = tg_period_position(g, &period);
        int bar = g->division * g->beats_per_bar, to_arrival = period - d;
        if (to_arrival <= bar)
            bpm *= 1.0f - 0.32f * g->rubato * (1.0f - (float)to_arrival / (float)bar);
        else if (d < g->division)
            bpm *= 1.0f - 0.2f * g->rubato;
    }
    g->t.rate_hz = bpm / 60.0f * (float)g->division;
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
                if (g->style && g->plan_len > 0 && g->chord_step % g->plan_len == 0)
                    plan_period(g);                  /* a new period: plan its harmony */
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
            /* held chords voice-lead on each change; Alberti keeps a silent voicing to draw its
             * figure from; the other figures fetch the chord themselves */
            if ((g->chord_fresh || g->pad_n == 0) && (g->pad_mode == TG_PAD_BLOCK || g->pad_mode == TG_PAD_ALBERTI))
                pad_voice(g, i, accent(velocity, b) - 20, false, g->pad_mode == TG_PAD_ALBERTI, fn, ctx);
            switch (g->pad_mode) {
            case TG_PAD_ALBERTI:   pad_alberti(g, b, velocity, i, fn, ctx); break;
            case TG_PAD_BROKEN:    pad_broken(g, b, velocity, i, fn, ctx); break;
            case TG_PAD_AFTERBEAT: pad_afterbeat(g, b, velocity, i, fn, ctx); break;
            case TG_PAD_ARPEGGIO:  pad_arpeggio(g, b, velocity, i, fn, ctx); break;
            case TG_PAD_REPEAT:
                if (b.q % (g->division >= 2 ? g->division / 2 : 1) == 0)
                    pad_voice(g, i, accent(velocity, b) - 24, true, false, fn, ctx);
                break;
            default: break;
            }
            switch (g->bass_mode) {
            case TG_BASS_WALKING: if (b.on_beat) bass_walk(g, b, velocity, i, fn, ctx); break;
            case TG_BASS_ROOT:    if (g->chord_fresh) bass_root(g, b, velocity, i, fn, ctx); break;
            case TG_BASS_BAR:     if (b.downbeat || g->chord_fresh) bass_bar(g, b, velocity, i, fn, ctx); break;
            case TG_BASS_MARCH:   if (b.on_beat) bass_march(g, b, velocity, i, fn, ctx); break;
            case TG_BASS_PEDAL:   bass_pedal(g, b, velocity, i, fn, ctx); break;
            default:
                if (g->chord_fresh && g->since_note[TG_PART_BASS] >= min_gap)
                    bass_play(g, b, unit(g, TG_PART_BASS, o[0]), velocity, i, fn, ctx);
                break;
            }
            int before = g->sounding[TG_PART_MELODY];
            long before_count = g->motif_notes + g->free_notes;
            melody_tick(g, b, o[1], velocity, i, fn, ctx);
            bool new_note = g->sounding[TG_PART_MELODY] >= 0 &&
                            (g->sounding[TG_PART_MELODY] != before || g->motif_notes + g->free_notes != before_count);
            if (g->pad_mode == TG_PAD_COUNTER)
                pad_counter(g, b, velocity, i, fn, ctx);
            else if (g->pad_mode == TG_PAD_PARALLEL && new_note && g->since_note[TG_PART_MELODY] == 0)
                pad_parallel(g, accent(velocity, b), i, fn, ctx);
            else if (g->pad_mode == TG_PAD_DOUBLE && new_note && g->since_note[TG_PART_MELODY] == 0)
                pad_double(g, accent(velocity, b), i, fn, ctx);
        }
        if (g->bass_mode != TG_BASS_MARBLES && g->pad_mode != TG_PAD_BLOCK)
            continue;                                /* the figures run on the clock, not on T1/T3 */
        bool g1 = gate[i * 2], g3 = gate[i * 2 + 1];
        bool bass_rise = g1 && !g->gate_prev[TG_PART_BASS];
        bool bass_fall = !g1 && g->gate_prev[TG_PART_BASS];
        bool pad_rise = g3 && !g->gate_prev[TG_PART_PAD];
        g->gate_prev[TG_PART_BASS] = g1;
        g->gate_prev[TG_PART_PAD] = g3;
        if (g->bass_mode == TG_BASS_MARBLES) {
            if (bass_fall && g->since_note[TG_PART_BASS] >= min_gap)
                note_off(g, TG_PART_BASS, i, fn, ctx);
            if (bass_rise && g->since_note[TG_PART_BASS] >= min_gap)
                bass_play(g, b, unit(g, TG_PART_BASS, o[0]), velocity, i, fn, ctx);
        }
        if (g->pad_mode == TG_PAD_BLOCK && pad_rise && g->pad_restrike && g->since_note[TG_PART_PAD] >= min_gap)
            pad_voice(g, i, accent(velocity, b) - 26, true, false, fn, ctx);
    }
}
