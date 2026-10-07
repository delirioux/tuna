/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA generator (see tuna_gen.h). */
#include <math.h>
#include "tuna_gen.h"

/* no libc here: the firmware brings its own (libc.c) */
static void zero(void *p, unsigned n)
{
    unsigned char *b = p;
    while (n--)
        *b++ = 0;
}

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
static void apply_modes(tg_gen *g, bool alt);

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
    g->meter[0] = 0;
    g->meter_bars = 0;
    g->swing = 0.0f;
    g->lilt = 0.0f;
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
    g->mel_add = 0;
    g->period_scale = -1;
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
    g->humanize = 0.0f;
    g->mel_mode = TG_MEL_PHRASE;
    g->harmony = TG_HARM_PLANNED;
    g->mel_min_ticks = 0;
    g->direction = g->staccato = g->turn = g->octave = 0.0f;
    g->pad_interval = g->poly_semis = g->strum_ms = 0;
    g->spread = false;
    g->gp = g->echo = 0.0f;
    g->tuning = TT_TUNE_EQUAL;
    g->tune_root = 0;
    g->perc_mode = TG_PERC_OFF;
    for (int i = 0; i < TG_PI_COUNT; ++i)
        g->perc_w[i] = 0;
    g->perc_w[TG_PI_TIMPANI] = 6;
    g->perc_w[TG_PI_CYMBAL] = 3;
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
    g->pad_poly = 0;
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
    tm_rng_init(&g->prng, seed ^ 0xd7a3b1c5u);
    g->drum_n = 0;
    g->timp_note = -1;
    g->timp_off_at = -1;
    g->roll_until = -1;
    g->roll_note = -1;
    g->roll_countdown = 0.0f;
    g->march_on = g->kit_g1_prev = g->kit_g3_prev = false;
    g->perc_hits = 0;
    tm_rng_init(&g->hum_rng, seed ^ 0x48756d61u);
    g->hq_n = 0;
    g->clock = 0;
    g->tick_delay = 0;
    /* sequencer: recording from the start; the overlay's steps and FILL are settings and stay */
    g->rec_cur = 0;
    g->rec_valid = false;
    g->rec[0].n = g->rec[1].n = 0;
    g->rec[0].len = g->rec[1].len = 0;
    for (int p = 0; p < TG_EVENT_PARTS; ++p) {
        for (int n = 0; n < 128; ++n)
            g->rec_open[p][n] = g->xmap[p][n] = -1;
        g->sound_locked[p] = 0;
        g->cond_tick[p] = -1;
        g->cond_loop[p] = 0;
        g->cond_pass[p] = true;
        g->cond_pre[p] = false;
    }
    zero(g->lenlocked, sizeof g->lenlocked);
    g->loff_n = 0;
    g->frozen = g->freeze_req = g->unfreeze_req = false;
    g->pat_pos = 0;
    g->pat_loop = 0;
    g->play_n = 0;
    tm_rng_init(&g->srng, seed ^ 0x53746570u);
    g->period_index = 0;
    g->energy_vel = 0;
    g->energy = 0.0f;
    g->key_pending = g->key_away = false;
    g->periods_away = 0;
    g->key_set_tick = 0;
    g->perc_quiet = false;
    if (g->evolve) {                                /* a restart starts the cycle at home */
        g->root = g->base_root;
        g->scale = g->base_scale;
    }
    g->plan_len = 0;
    g->period_scale = -1;
    g->open_end = g->open_prev = false;
    g->open_chord = g->open_chord_prev = 0;
    g->cycle_sel = 0;
    g->style_away = false;
    g->stat_open = g->stat_colour = g->stat_slips = g->stat_modulations = 0;
    g->tinti_k = 1;
    g->tinti_i = g->tinti_rest = 0;
    g->tinti_dir = -1;
    g->pat_i = 0;
    g->bird_left = g->bird_rest = 0;
    g->orn_n = 0;
    g->bass_hit = g->bass_alt = 0;
    g->pad_note = -1;
    for (int i = 0; i < TG_CANON_MAX; ++i)
        g->canon_buf[i] = -1;
    g->add_len = 1;
    g->add_rep = g->add_pos = 0;
    g->add_dir = 1;
    g->phase_off = g->phase_rep = 0;
    g->oompah_off = -1;
    g->gp_from = g->gp_until = -1;
    g->echo_vel = 0;
    for (int p = 0; p < TG_PARTS; ++p)
        g->orch_cur[p] = -1;
    g->orch_set = 0;
    tm_rng_init(&g->orng, seed ^ 0x4f726368u);
    if (g->style && g->alt_on)
        apply_modes(g, false);                      /* a restart begins outside any contrast period */
    g->alt_on = false;
    for (int i = 0; i < TG_PLAN_MAX; ++i)
        g->plan_slip[i] = 0;
    if (g->style && g->style_away) {
        g->root = g->home_root;
        g->scale = g->home_scale;
    }
    g->style_away = false;
    plan_period(g);
}

void tg_init(tg_gen *g, uint32_t seed, float sample_rate)
{
    defaults(g);
    g->nsteps = 0;
    g->fill = false;
    g->evolve = false;
    g->evolve_modulate = false;
    g->evolve_periods = 8;
    g->evolve_depth = 0.0f;
    g->pat.n = 0;
    g->pat.len = 0;
    g->sample_rate = sample_rate;
    tm_rng_init(&g->rng, seed);
    tm_t_init(&g->t, &g->rng, sample_rate);
    g->t.model = TM_T_COMPLEMENTARY_BERNOULLI;
    g->t.pulse_width_mean = 0.5f;
    tg_reseed(g, seed);
}

/* ---- meter ------------------------------------------------------------------------------------------ */
int tg_meter_cycle(const tg_gen *g, int *bars)
{
    if (!g->meter[0]) {
        if (bars) *bars = 1;
        return g->division * g->beats_per_bar;
    }
    int sum = 0, nb = 0;
    for (int i = 0; i < TG_METER_MAX && g->meter[i]; ++i) {
        sum += g->meter[i];
        nb += i == 0 || ((g->meter_bars >> i) & 1);
    }
    if (bars) *bars = nb;
    return sum > 0 ? sum : 1;
}

/* the average bar: what "a bar" means for holds, homing and periods */
static int bar_ticks_of(const tg_gen *g)
{
    int nb, c = tg_meter_cycle(g, &nb);
    return nb > 1 ? c / nb : c;
}

void tg_meter_at(const tg_gen *g, long q, tg_meter_pos *m)
{
    if (!g->meter[0]) {
        int bar = g->division * g->beats_per_bar;
        int pos = (int)(q % bar);
        m->group = m->beat = pos / g->division;
        m->sub = pos % g->division;
        m->glen = g->division;
        m->bar_pos = pos;
        m->bar_len = bar;
        m->nbeats = g->beats_per_bar;
        m->bar_start = pos == 0;
        return;
    }
    int cycle = tg_meter_cycle(g, NULL);
    int pos = (int)(q % cycle), at = 0, bar_at = 0, beat = 0, cur = -1;
    m->bar_start = false;
    for (int i = 0; i < TG_METER_MAX && g->meter[i]; ++i) {
        bool starts_bar = i == 0 || ((g->meter_bars >> i) & 1);
        if (starts_bar) {
            if (cur >= 0)
                break;                              /* the bar holding pos has ended */
            bar_at = at;
            beat = 0;
        }
        if (cur < 0 && pos < at + g->meter[i]) {
            cur = i;
            m->group = i;
            m->beat = beat;
            m->sub = pos - at;
            m->glen = g->meter[i];
            m->bar_start = starts_bar && pos == at;
        }
        at += g->meter[i];
        beat++;
    }
    m->bar_pos = pos - bar_at;
    m->bar_len = at - bar_at;
    m->nbeats = beat;
}

/* ---- harmony: the fixed progression, or a style's planned periods ---------------------------------- */
static bool key_is_minor(const tg_gen *g) { return (tt_scale_mask(g->scale) >> 3) & 1; }
static bool scale_is_minor(int scale) { return (tt_scale_mask(scale) >> 3) & 1; }

/* the period's scale: the key's, or a colour (another scale, the parallel mode) for this period */
static int scale_now(const tg_gen *g) { return g->period_scale >= 0 ? g->period_scale : g->scale; }

static const tg_chord_def *voc(const tg_gen *g, bool minor)
{
    return tg_vocab_of(g->style ? g->style->vocab : TG_VOC_CLASSIC, minor);
}

/* a vocabulary chord's root in semitones above the tonic (a modal entry names a scale degree) */
static int voc_semis_in(int scale, const tg_chord_def *d)
{
    if (d->quality != TQ_DIATONIC)
        return d->semis;
    return pmod12(tt_degree_note(scale, 60, d->semis) - 60);
}
static int voc_semis(const tg_gen *g, const tg_chord_def *d) { return voc_semis_in(scale_now(g), d); }

void tg_chord_at(const tg_gen *g, int step, int tones, tt_chord *c)
{
    /* a modulation decided at the period's start applies from its second chord on: later steps are
     * already in the new key (relative to the old tonic until it applies) */
    bool future = g->key_pending && step > g->chord_step;
    int shift = future ? g->next_root - g->root : 0;
    if (!g->style || g->plan_len <= 0) {
        tt_prog_chord(c, g->prog, step, future ? g->next_scale : g->scale, tones);
        c->root = (int8_t)pmod12(c->root + shift);
        return;
    }
    int slot = ((step % g->plan_len) + g->plan_len) % g->plan_len;
    /* a step in the next period that is not planned yet starts on the tonic (or the open chord) */
    bool next = step / g->plan_len > g->chord_step / g->plan_len;
    int idx = next ? (g->open_end ? g->open_chord : 0) : g->plan[slot];
    bool minor = g->plan_minor;
    if (g->key_pending && !future) {
        idx = 0;                                    /* the period's first chord: the old key's tonic */
        minor = g->plan_minor0;
    }
    const tg_chord_def *d = &voc(g, minor)[idx];
    if (d->quality == TQ_DIATONIC) {
        tt_chord_diatonic(c, future ? g->next_scale : scale_now(g), d->semis, tones > 0 ? tones : 3);
        c->root = (int8_t)pmod12(c->root + shift);
    } else {
        tt_chord_fixed(c, d->semis + shift, d->quality);
    }
    if (!next && g->plan_slip[slot])
        c->root = (int8_t)pmod12(c->root + g->plan_slip[slot]);
    if (tones < c->n)
        c->n = (uint8_t)tones;
}

static float hrand(tg_gen *g) { return tm_rng_float(&g->hrng); }

/* pick a vocabulary entry: allowed[i] gives 0/1, weights from the style, root motion down a fifth
 * from prev_semis multiplied by the style's fifth bias */
static int pick_chord(tg_gen *g, const bool *allowed, int prev_semis)
{
    const tg_chord_def *v = voc(g, g->plan_minor);
    const uint8_t *w = g->plan_minor ? g->style->w_minor : g->style->w_major;
    float weight[TG_VOCAB], sum = 0.0f;
    for (int i = 0; i < TG_VOCAB; ++i) {
        weight[i] = allowed[i] ? (float)w[i] : 0.0f;
        if (weight[i] > 0.0f && prev_semis >= 0 && pmod12(voc_semis(g, &v[i]) - prev_semis) == 5)
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

/* the style's own modulation: now and then a period moves to a related key, and later home again
 * (the move applies from the period's second chord, through the pivot as with evolution) */
static void style_modulate(tg_gen *g)
{
    const tg_style *st = g->style;
    if (st->modulate <= 0.0f || g->evolve || g->key_pending || g->open_prev)
        return;
    if (g->style_away) {
        if (hrand(g) < 0.5f) {
            g->next_root = g->home_root;
            g->next_scale = g->home_scale;
            g->style_away = false;
        } else {
            return;
        }
    } else {
        if (hrand(g) >= st->modulate)
            return;
        bool minor = scale_is_minor(g->scale);
        /* major: the dominant, the subdominant, the relative minor; minor: the relative major, the
         * minor dominant, the subdominant, the submediant major */
        static const int8_t maj_to[3] = {7, 5, 9}, min_to[4] = {3, 7, 5, 8};
        int k = (int)(hrand(g) * (minor ? 4.0f : 3.0f));
        int to = minor ? min_to[k > 3 ? 3 : k] : maj_to[k > 2 ? 2 : k];
        bool to_minor = minor ? (to == 7 || to == 5) : to == 9;
        g->next_root = (g->root + to) % 12;
        g->next_scale = to_minor ? (minor ? g->scale : TS_AEOLIAN) : (minor ? TS_IONIAN : g->scale);
        g->style_away = true;
    }
    g->key_pending = true;
    g->key_set_tick = g->ticks;
    g->stat_modulations++;
}

/* the chord the next period opens on when this one ends away from the tonic: a tonic-function chord
 * other than I (a deceptive cadence), or a dominant (the music stops on V) */
static int pick_open(tg_gen *g)
{
    const tg_chord_def *v = voc(g, g->plan_minor);
    bool allowed[TG_VOCAB];
    for (int k = 0; k < TG_VOCAB; ++k)
        allowed[k] = v[k].quality != TQ_AUG && v[k].func != TF_SEC && v[k].func != TF_S &&
                     voc_semis(g, &v[k]) != 0;
    int c = pick_chord(g, allowed, -1);
    return voc_semis(g, &v[c]) != 0 ? c : 0;
}

static void plan_period(tg_gen *g)
{
    if (!g->style)
        return;
    const tg_style *st = g->style;
    int n = st->period_bars * bar_ticks_of(g) / (g->chord_ticks > 0 ? g->chord_ticks : 1);
    if (n < 2) n = 2;
    if (n > TG_PLAN_MAX) n = TG_PLAN_MAX;
    /* the previous period's ending decides how this one begins */
    g->open_prev = g->open_end;
    g->open_chord_prev = g->open_chord;
    g->open_end = false;
    style_modulate(g);

    /* fixed harmony: a cycle repeated through the period, or one chord held (a drone) */
    if (g->harmony == TG_HARM_CYCLE || g->harmony == TG_HARM_DRONE) {
        if (g->key_pending)
            g->plan_minor0 = key_is_minor(g);
        g->plan_len = n;
        g->plan_minor = scale_is_minor(g->key_pending ? g->next_scale : g->scale);
        g->period_scale = -1;
        if (st->cycle_switch > 0 && st->cycle2_len > 0)
            g->cycle_sel = (int)((g->period_index / st->cycle_switch) % 2);
        const int8_t *cy = g->cycle_sel ? st->cycle2 : st->cycle;
        int len = g->cycle_sel ? st->cycle2_len : st->cycle_len;
        for (int i = 0; i < n; ++i) {
            g->plan[i] = g->harmony == TG_HARM_DRONE ? (int8_t)(len ? cy[0] : 0) : (int8_t)(len ? cy[i % len] : 0);
            g->plan_slip[i] = 0;
        }
        return;
    }

    bool minor = scale_is_minor(scale_now(g));
    bool fresh = st->open > 0.0f && (g->open_prev || g->plan[0] != g->plan_tonic);
    if (g->key_pending) {
        g->plan_minor0 = key_is_minor(g);           /* slot 0 stays the old key's tonic (the pivot) */
        g->period_scale = -1;
        minor = scale_is_minor(g->next_scale);
    } else if (g->plan_len == n && g->plan_minor == minor && !fresh && hrand(g) < st->repeat) {
        goto ending;                                /* the period repeats its harmony: A A */
    } else {
        /* a coloured period: the parallel mode, or another scale */
        g->period_scale = -1;
        if (st->mixture > 0.0f && hrand(g) < st->mixture) {
            g->period_scale = key_is_minor(g) ? TS_IONIAN : TS_AEOLIAN;
            g->stat_colour++;
        } else if (st->color > 0.0f && hrand(g) < st->color) {
            g->period_scale = st->color_scale;
            g->stat_colour++;
        }
        minor = scale_is_minor(scale_now(g));
    }
    g->plan_len = n;
    g->plan_minor = minor;
    {
        const tg_chord_def *v = voc(g, minor);
        for (int i = 0; i < n; ++i)
            g->plan_slip[i] = 0;

        /* -1: free; TF_* + 10: a slot fixed to a function */
        int want[TG_PLAN_MAX];
        for (int i = 0; i < n; ++i) want[i] = -1;
        if (n >= 4) {
            want[n / 2 - 2] = TF_S + 10;
            want[n / 2 - 1] = TF_D + 10;            /* half cadence */
            want[n - 2] = TF_S + 10;
            want[n - 1] = (g->plagal ? TF_S : TF_D) + 10;   /* into the next tonic: V - I, or IV - I */
        } else {
            want[n - 1] = TF_D + 10;
        }
        /* the tonic: the plain triad or a coloured one (Imaj7, i7, a major tonic in minor) */
        {
            bool allowed[TG_VOCAB];
            for (int k = 0; k < TG_VOCAB; ++k)
                allowed[k] = voc_semis(g, &v[k]) == 0 && v[k].func == TF_T && v[k].quality != TQ_AUG;
            g->plan[0] = (int8_t)pick_chord(g, allowed, -1);
            g->plan_tonic = g->plan[0];
            if (g->open_prev && !g->key_pending) {
                g->plan[0] = g->open_chord_prev;    /* the deceptive (or open) arrival of the last period */
                g->stat_open++;
            }
        }
        /* a whole-tone period: augmented chords drift until the cadence brings the key back */
        g->plan_wt = n >= 4 && st->whole_tone > 0.0f && hrand(g) < st->whole_tone;
        if (g->plan_wt) {
            for (int i = 0; i < n - 2; ++i)
                g->plan[i] = (int8_t)(i % 2 ? TG_VOC_AUG_BVII : TG_VOC_AUG_I);
            for (int i = 0; i < n; ++i)
                if (i < n - 2) want[i] = -2;        /* taken */
        }
        /* an octatonic period: the tonic chord moved by minor thirds (I, bIII, #IV, VI), then the cadence */
        if (n >= 4 && (scale_now(g) == TS_OCT_HW || scale_now(g) == TS_OCT_WH) && g->period_scale >= 0) {
            int last = 0;
            for (int i = 0; i < n - 2; ++i) {
                g->plan[i] = g->plan_tonic;
                int k = i ? 1 + (int)(hrand(g) * 3.0f) : 0;
                last = (last + 3 * k) % 12;
                g->plan_slip[i] = (int8_t)(i ? last : 0);
                want[i] = -2;
            }
        }
        for (int i = 1; i < n; ++i) {
            if (want[i] == -2)
                continue;
            const tg_chord_def *prev = &v[g->plan[i - 1]];
            bool allowed[TG_VOCAB];
            if (prev->func == TF_SEC) {             /* an applied chord resolves to its target */
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
                        ok = true;                  /* V7/V before the dominant */
                    if (f == TF_D)
                        ok = c->func == TF_D;
                } else if (prev->func == TF_D || prev->quality == TQ_AUG) {
                    ok = c->func == TF_T;           /* dominants resolve */
                } else if (prev->func == TF_S) {
                    ok = c->func == TF_S || c->func == TF_D;
                } else {
                    ok = true;
                }
                if (k == g->plan[i - 1])
                    ok = false;                     /* change the chord */
                if (c->quality == TQ_AUG)
                    ok = false;                     /* only in whole-tone periods */
                if (c->func == TF_SEC && want[i] < 10 && (i + 1 >= n || want[i + 1] >= 10))
                    ok = false;                     /* its target would land on a fixed slot */
                if (c->func == TF_D && next_want == TF_S + 10 && want[i] < 10)
                    ok = false;                     /* no free dominant right before a pre-dominant */
                allowed[k] = ok;
            }
            g->plan[i] = (int8_t)pick_chord(g, allowed, voc_semis(g, prev));
        }
        /* side-slips: a free chord moved a semitone, the music sliding off and back */
        if (st->sideslip > 0.0f)
            for (int i = 1; i < n - 2; ++i)
                if (want[i] == -1 && !g->plan_slip[i - 1] && hrand(g) < st->sideslip) {
                    g->plan_slip[i] = (int8_t)(hrand(g) < 0.5f ? 1 : -1);
                    g->stat_slips++;
                }
    }
ending:
    /* does this period end away from the tonic? */
    if (st->open > 0.0f && !g->key_pending && !(g->evolve && g->evolve_modulate) && hrand(g) < st->open) {
        g->open_end = true;
        g->open_chord = (int8_t)pick_open(g);
        if (g->open_chord == 0)
            g->open_end = false;
    }
}

/* the style's modes, or a contrast period's (alt) */
static void apply_modes(tg_gen *g, bool alt)
{
    const tg_style *st = g->style;
    g->mel_mode = alt && st->alt.mel_mode ? st->alt.mel_mode - 1 : st->mel_mode;
    g->bass_mode = alt && st->alt.bass_mode ? st->alt.bass_mode - 1 : st->bass_mode;
    g->pad_mode = alt && st->alt.pad_mode ? st->alt.pad_mode - 1 : st->pad_mode;
    g->harmony = alt && st->alt.harmony ? st->alt.harmony - 1 : st->harmony;
    g->bpm = st->bpm * (alt && st->alt.bpm > 0.0f ? st->alt.bpm : 1.0f);
    if (!g->evolve)
        g->melody_density = st->melody_density * (alt && st->alt.density > 0.0f ? st->alt.density : 1.0f);
}

void tg_apply_style(tg_gen *g, int id) { tg_apply_style_def(g, tg_style_get(id)); }

void tg_apply_style_def(tg_gen *g, const tg_style *st)
{
    g->style = st;
    if (!st)
        return;
    g->root = st->root;
    g->scale = st->scale;
    g->bpm = st->bpm;
    g->division = st->division;
    g->beats_per_bar = st->beats_per_bar;
    for (int i = 0; i < TG_METER_MAX; ++i)
        g->meter[i] = i < 8 ? st->meter[i] : 0;
    g->meter_bars = st->meter_bars;
    g->swing = st->swing;
    g->lilt = st->lilt;
    /* a chord lasts chord_beats beats, or with beat groups chord_beats whole cycles of the meter */
    g->chord_ticks = g->meter[0] ? st->chord_beats * tg_meter_cycle(g, NULL) : st->chord_beats * st->division;
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
    g->mel_add = st->mel_add;
    g->home_root = st->root;
    g->home_scale = st->scale;
    g->mel_min_ticks = st->mel_min_ticks;
    g->direction = st->direction;
    g->staccato = st->staccato;
    g->turn = st->turn;
    g->octave = st->octave;
    g->pad_interval = st->pad_interval;
    g->poly_semis = st->poly_semis;
    g->spread = st->spread;
    g->strum_ms = st->strum_ms;
    g->gp = st->gp;
    g->echo = st->echo;
    g->tuning = st->tuning;
    g->tune_root = st->root;
    apply_modes(g, false);
    g->style_away = false;
    g->period_scale = -1;
    g->open_end = g->open_prev = false;
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
    g->perc_mode = st->perc_mode;
    for (int i = 0; i < TG_PI_COUNT; ++i)
        g->perc_w[i] = st->perc[i];
    /* Marbles as the style wants it: a centred, moderately wide X, motifs recurring by deja vu */
    g->x.control_mode = TM_CONTROL_IDENTICAL;
    g->x.spread = 0.5f;
    g->x.bias = 0.5f;
    g->x.steps = 0.75f;
    g->x.deja_vu = st->motif;
    g->x.length = st->beats_per_bar * 2;
    g->plan_len = 0;
    plan_period(g);
    if (st->evolve_periods > 0)
        tg_evolve(g, true, st->evolve_periods, st->evolve_depth, st->evolve_modulate);
}

static void reload_scale(tg_gen *g)
{
    int sc = scale_now(g);
    if (g->loaded_scale == sc && g->loaded_prog == g->prog && g->loaded_step == g->chord_step &&
        g->loaded_root == g->root)
        return;
    tg_chord_at(g, g->chord_step, 4, &g->chord);
    tm_scale s;
    uint16_t mask = tt_scale_mask(sc);
    tg_scale_for_chord(&s, mask, &g->chord);
    tm_xy_load_scale(&g->xy, -1, &s);
    g->allowed_pcs = mask | chord_mask(&g->chord, g->chord.n);
    g->chord_scale = tg_chord_scale(mask, &g->chord);
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
    if (g->mel_add && !aug)
        g->mel_scale |= g->mel_add;                    /* blue notes */
    g->loaded_scale = sc;
    g->loaded_prog = g->prog;
    g->loaded_step = g->chord_step;
    g->loaded_root = g->root;
}

/* ---- events ---------------------------------------------------------------------------------------- */
static void emit(tg_event_fn fn, void *ctx, int part, int note, int velocity, bool on, int offset)
{
    tg_event e = {(uint8_t)part, (uint8_t)clampi(note, 0, 127), (uint8_t)clampi(velocity, 1, 127), on,
                  (uint16_t)offset, TG_EV_NOTE, 0, 0};
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
    g->pad_poly = 0;
}

static void drums_release(tg_gen *g, int offset, tg_event_fn fn, void *ctx);

static void gen_release(tg_gen *g, int offset, bool sounding);
static void pat_release(tg_gen *g, int offset);
static void out_param(tg_gen *g, int part, int param, int value, int offset);

void tg_all_off(tg_gen *g, tg_event_fn fn, void *ctx)
{
    /* queued by the humanizer: releases go out now, notes not yet started never will */
    for (int i = 0; i < g->hq_n; ++i)
        if (!g->hq[i].e.on) {
            tg_event e = g->hq[i].e;
            e.offset = 0;
            fn(ctx, &e);
        }
    g->hq_n = 0;
    float hum = g->humanize;                        /* the rest at once, past the humanizer */
    g->humanize = 0.0f;
    g->user_fn = fn;
    g->user_ctx = ctx;
    if (!g->frozen)
        gen_release(g, 0, true);
    pat_release(g, 0);
    for (int i = 0; i < g->loff_n; ++i) {
        tg_event e = {g->loff[i].part, g->loff[i].note, 0, false, 0, TG_EV_NOTE, 0, 0};
        fn(ctx, &e);
    }
    g->loff_n = 0;
    zero(g->lenlocked, sizeof g->lenlocked);
    for (int p = 0; p < TG_EVENT_PARTS; ++p) {
        for (int prm = TG_LK_E0; prm < TG_LK_COUNT; ++prm)
            if ((g->sound_locked[p] >> prm) & 1u)
                out_param(g, p, prm, TG_LK_RESTORE, 0);
        g->sound_locked[p] = 0;
        for (int n = 0; n < 128; ++n)
            g->xmap[p][n] = -1;
    }
    g->humanize = hum;
}

/* ---- the harmonizer -------------------------------------------------------------------------------- */
/* where the tick just started falls: bar_ticks is the average bar (one bar unless the meter changes);
 * beats are the meter's groups, so in 3+3+2 the beats are three, of 3, 3 and 2 ticks */
typedef struct { bool downbeat, on_beat; int q, bar_ticks; tg_meter_pos m; } beat_pos;

static beat_pos position(const tg_gen *g)
{
    beat_pos b;
    b.q = (int)(g->ticks - 1);               /* index of the tick just started */
    b.bar_ticks = bar_ticks_of(g);
    tg_meter_at(g, b.q, &b.m);
    b.downbeat = b.m.bar_start;
    b.on_beat = b.m.sub == 0;
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
    if (!g->hemiola || g->beats_per_bar != 3 || g->meter[0])
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

/* pad: voice the chord as close as possible to the previous voicing; common tones keep sounding.
 * Options: an open (spread) voicing, a second major triad above (a polychord), strummed onsets. */
static void pad_voice(tg_gen *g, int offset, int velocity, bool restrike, bool silent, tg_event_fn fn, void *ctx)
{
    tt_chord c;
    tg_chord_at(g, g->chord_step, g->pad_tones, &c);
    int n = c.n;
    int lo = g->base_note[TG_PART_PAD] + g->root;
    int best[TG_PAD_VOICES], best_cost = 1 << 30;
    if (g->spread && n >= 3) {
        /* open position: root, fifth, third an octave up, then the rest, each at least a third apart */
        static const uint8_t order4[4] = {0, 2, 1, 3};
        int prev = -1;
        for (int k = 0; k < n; ++k) {
            int pc = g->root + c.root + c.tone[order4[k < 4 ? k : 3]];
            int x = k == 0 ? lo : prev + 3;
            while (pmod12(x - pc) != 0) x++;
            best[k] = prev = x;
        }
    } else {
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
            if (g->pad_n - g->pad_poly == n) {
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
    }
    if (g->poly_semis && n + 3 <= TG_PAD_VOICES) {
        /* a polychord: a major triad on the chord's root + poly_semis, above the chord */
        int top = best[n - 1], r = g->root + c.root + g->poly_semis;
        while (r <= top) r += 12;
        best[n] = r;
        best[n + 1] = r + 4;
        best[n + 2] = r + 7;
        n += 3;
    }
    g->pad_poly = n - c.n;
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
    int keep_delay = g->tick_delay, struck = 0;
    float strum = g->strum_ms * 0.001f * g->sample_rate;
    for (int k = 0; k < n; ++k) {
        bool held = false;
        for (int i = 0; i < g->pad_n && !restrike; ++i)
            held |= g->pad[i] == best[k];
        if (!held) {
            if (g->strum_ms)
                g->tick_delay = keep_delay + (int)(strum * (float)struck);   /* a strum: low to high */
            emit(fn, ctx, TG_PART_PAD, best[k], velocity, true, offset);
            struck++;
        }
    }
    g->tick_delay = keep_delay;
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
    int p, a, bar = bar_ticks_of(g);
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
    if (g->mel_mode == TG_MEL_CHANT) {
        b.downbeat = false;                      /* chant has no bar lines: no accents, no strong beats */
        b.on_beat = false;
    }
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

    /* where the period lands: the tonic, or after an open ending a tone of the chord it opens on (not
     * its root: the ear should hear the music go on); at the arrival that chord is sounding already */
    uint16_t target = tonic;
    if (arrival ? g->open_prev : g->open_end) {
        tt_chord nc;
        if (arrival)
            nc = g->chord;
        else
            tg_chord_at(g, (g->chord_step / g->plan_len + 1) * g->plan_len, 3, &nc);
        uint16_t cm = chord_mask(&nc, 3);
        uint16_t upper = (uint16_t)(cm & ~(1u << pmod12(nc.root)));
        target = upper ? upper : cm;
    }
    /* the target nearest the current note, inside the range */
    int home = mask_nearest(target, g->root, g->mel_note);
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
        g->orn_n = 0;
    }
    if (!arrival && g->orn_n > 0) {
        /* a turn: its remaining notes, one per tick */
        int n = g->orn_q[3 - g->orn_n];
        g->orn_n--;
        g->mel_last_dir = n > g->mel_note ? 1 : n < g->mel_note ? -1 : 0;
        g->mel_note = n;
        note_on(g, TG_PART_MELODY, n, accent(velocity, b) - 12, offset, fn, ctx);
        return;
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
        if (up - home > 2) up = mask_step(tt_scale_mask(scale_now(g)), g->root, home, 1);
        if (home - down > 2) down = mask_step(tt_scale_mask(scale_now(g)), g->root, home, -1);
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

            /* interval from X2: mostly steps, sometimes a leap; a style may lean downward or upward */
            int dir = x + g->direction * 0.6f < 0.0f ? -1 : 1;
            const float *pr = g->mel_profile;
            k = m < pr[0] ? 0 : m < pr[1] ? 1 : m < pr[2] ? 2 : m < pr[3] ? 3 : g->max_leap;
            bool oct = !homing && g->octave > 0.0f && m >= pr[0] && hrand(g) < g->octave;
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
            if (oct) {
                /* an octave leap, the other way if it would leave the range */
                note = g->mel_note + 12 * (k < 0 ? -1 : 1);
                if (note > hi || note < lo)
                    note = g->mel_note - 12 * (k < 0 ? -1 : 1);
                if (note > hi || note < lo)
                    note = g->mel_note;
                k = note > g->mel_note ? 3 : note < g->mel_note ? -3 : 0;   /* answered by a step back */
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
            /* a turn: the note above, the note, the note below, the note, quickly */
            if (g->turn > 0.0f && g->mel_pending < 0 && !homing && g->division >= 3 && to_arrival > 3 * g->division &&
                hrand(g) < g->turn) {
                int up = mask_step(g->chord_scale, g->root, note, 1), down = mask_step(g->chord_scale, g->root, note, -1);
                if (up <= hi + 2 && down >= lo - 2) {
                    g->orn_q[0] = note;
                    g->orn_q[1] = down;
                    g->orn_q[2] = note;
                    g->orn_n = 3;
                    note = up;
                }
            }
        }
        if (g->mel_min_ticks > 1 && b.q + g->mel_min_ticks > g->mel_hold_until)
            g->mel_hold_until = b.q + g->mel_min_ticks;     /* long notes */
        if (g->staccato > 0.0f && g->orn_n == 0 && g->mel_pending < 0) {
            int len = (int)lrintf((float)g->division * (1.0f - g->staccato));
            g->mel_off_at = b.q + (len < 1 ? 1 : len);
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
    bool strong = b.on_beat;
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
    int root = bass_root_note(g, g->chord.root);
    int note;
    if (b.downbeat || g->chord_fresh)
        note = root;
    else if (b.on_beat && b.m.nbeats >= 4 && b.m.beat == b.m.nbeats / 2)
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
    if ((!b.downbeat || b.q % (2 * b.bar_ticks) >= b.m.bar_len) && g->sounding[TG_PART_BASS] >= 0)
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
    /* pitch classes from the root upward (a ninth counts as a second), then stacked by octaves */
    int pcs[TT_CHORD_MAX], np = 0;
    for (int k = 0; k < c.n; ++k) {
        int s = c.tone[k] % 12, at = np;
        while (at > 0 && pcs[at - 1] > s) { pcs[at] = pcs[at - 1]; --at; }
        pcs[at] = s;
        np++;
    }
    int n = 0;
    for (int oct = 0; n < max && oct < 5; ++oct)
        for (int k = 0; k < np && n < max; ++k)
            if (k == 0 || pcs[k] != pcs[k - 1])
                out[n++] = root + 12 * oct + pcs[k];
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
    if (b.downbeat)
        pad_off(g, offset, fn, ctx);
    else if (b.on_beat && b.m.beat == 1)
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
    g->pad_poly = 0;
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

/* ---- melody modes beyond the phrase ------------------------------------------------------------------ */
/* the k-th note of the chord ladder from lo (k may be negative: below, by octaves) */
static int ladder_note(const tg_gen *g, int lo, int k)
{
    int lad[16], n = chord_ladder(g, lo, lad, 16);
    if (n <= 0)
        return lo;
    int per = 0;                                    /* notes per octave in the ladder */
    while (per < n && lad[per] < lad[0] + 12) per++;
    if (per <= 0) per = 1;
    int oct = k >= 0 ? k / per : -((-k + per - 1) / per);
    int i = k - oct * per;
    return lad[i] + 12 * oct;
}

/* tintinnabuli M-voice: phrases of k+1 notes that step toward the tonic (from below, then, after a
 * full round, from above), one beat each, the tonic held, a beat of silence; k grows by one each phrase */
static void melody_tinti(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    if (!b.on_beat)
        return;
    const int lo = g->base_note[TG_PART_MELODY] + g->root, hi = lo + g->melody_range;
    int centre = mask_nearest(1u, g->root, (lo + hi) / 2);
    int k = g->tinti_k, i = g->tinti_i;
    if (i <= k) {
        int note = mask_step(tt_scale_mask(scale_now(g)), g->root, centre, g->tinti_dir * (k - i));
        g->mel_last_dir = note > g->mel_note ? 1 : note < g->mel_note ? -1 : 0;
        g->mel_note = note;
        note_on(g, TG_PART_MELODY, note, velocity - 6 + (i == 0 ? 6 : 0), offset, fn, ctx);
        g->free_notes++;
    } else if (i == k + 2) {
        note_off(g, TG_PART_MELODY, offset, fn, ctx);
    }
    if (++g->tinti_i > k + 2) {
        g->tinti_i = 0;
        if (++g->tinti_k > 7) {
            g->tinti_k = 1;
            g->tinti_dir = -g->tinti_dir;
        }
    }
}

/* a pattern element as a note: scale steps from the tonic at lo, or steps of the chord ladder; -1: rest */
static int pattern_note(const tg_gen *g, int idx, int lo)
{
    const tg_style *st = g->style;
    int len = st ? st->pattern_len : 0;
    if (len <= 0)
        return -1;
    int e = st->pattern[((idx % len) + len) % len];
    if (e == TG_REST)
        return -1;
    if (st->pattern_chord)
        return ladder_note(g, lo, e);
    int tonic = mask_nearest(1u, g->root, lo);
    if (tonic < lo) tonic += 12;
    return mask_step(tt_scale_mask(scale_now(g)), g->root, tonic, e);
}

static void melody_pattern(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    int note = pattern_note(g, g->pat_i, g->base_note[TG_PART_MELODY] + g->root);
    g->pat_i++;
    if (note < 0) {
        note_off(g, TG_PART_MELODY, offset, fn, ctx);
        return;
    }
    g->mel_note = note;
    note_on(g, TG_PART_MELODY, note, accent(velocity, b) - 6, offset, fn, ctx);
    g->free_notes++;
}

/* birdsong: bursts of quick notes high up, repeated notes, steps and wide leaps, then silence */
static void melody_bird(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    (void)b;
    int lo = g->base_note[TG_PART_MELODY] + g->root + 12, hi = lo + g->melody_range;
    if (hi > 108) hi = 108;
    if (g->bird_rest > 0) {
        g->bird_rest--;
        note_off(g, TG_PART_MELODY, offset, fn, ctx);
        return;
    }
    bool first = g->bird_left <= 0;
    if (first)
        g->bird_left = 3 + (int)(hrand(g) * 8.0f);
    uint16_t ms = g->mel_scale ? g->mel_scale : g->chord_scale;
    int note = g->mel_note >= lo && g->mel_note <= hi ? g->mel_note : mask_nearest(ms, g->root, (lo + hi) / 2);
    float r = hrand(g);
    if (r >= 0.3f) {
        int dir = hrand(g) < 0.5f ? -1 : 1;
        int k = r < 0.65f ? 1 + (int)(hrand(g) * 2.0f) : 3 + (int)(hrand(g) * 4.0f);
        note = mask_step(ms, g->root, note, dir * k);
        if (note > hi || note < lo)
            note = mask_step(ms, g->root, note, -2 * dir * k);
        note = clampi(note, lo, hi);
    }
    g->mel_note = note;
    note_on(g, TG_PART_MELODY, note, velocity + (first ? 10 : -6), offset, fn, ctx);
    g->free_notes++;
    if (--g->bird_left <= 0)
        g->bird_rest = 2 + (int)(hrand(g) * 10.0f);
}

/* ---- bass modes ---------------------------------------------------------------------------------------- */
static void bass_mode_tick(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    const tg_style *st = g->style;
    const int lo = g->base_note[TG_PART_BASS] + g->root;
    int note = -1, vel = accent(velocity, b) - 6;
    switch (g->bass_mode) {
    case TG_BASS_DRONE:
        if (g->sounding[TG_PART_BASS] < 0 || (tg_period_position(g, NULL) == 0 && b.q > 0 && b.on_beat)) {
            note = lo;
            vel = velocity - 14;
        }
        break;
    case TG_BASS_OSTINATO: {
        if (b.downbeat)
            g->bass_hit = 0;
        uint16_t r = st ? st->bass_rhythm : 0;
        bool hit = r ? ((r >> b.m.bar_pos) & 1) : b.on_beat;
        if (!hit)
            break;
        int k = st && st->bass_len ? st->bass_notes[g->bass_hit % st->bass_len] : 0;
        g->bass_hit++;
        tt_chord c;
        tg_chord_at(g, g->chord_step, 3, &c);
        int root = bass_root_note(g, c.root);
        note = ladder_note(g, root, k);
        break;
    }
    case TG_BASS_LINE:
        if (g->chord_fresh || g->sounding[TG_PART_BASS] < 0) {
            int len = st ? st->bass_len : 0;
            int slot = g->plan_len > 0 ? g->chord_step % g->plan_len : g->chord_step;
            note = lo + 12 + (len ? st->bass_notes[slot % len] : 0);
        }
        break;
    case TG_BASS_REPEAT:
        if (b.q % (g->division >= 2 ? g->division / 2 : 1) == 0)
            note = bass_root_note(g, g->chord.root);
        vel -= 6;
        break;
    case TG_BASS_TOLL:
        if ((b.downbeat && (b.q / (b.bar_ticks > 0 ? b.bar_ticks : 1)) % 2 == 0) || g->sounding[TG_PART_BASS] < 0) {
            note = bass_root_note(g, g->chord.root);
            vel = velocity + 4;
        }
        break;
    case TG_BASS_BEAT:
        if (g->chord_fresh)
            g->bass_alt = 0;
        if (b.on_beat) {
            int root = bass_root_note(g, g->chord.root);
            note = g->bass_alt++ % 2 ? mask_nearest(chord_mask(&g->chord, 3), g->root, root + 7) : root;
        }
        break;
    default:
        break;
    }
    if (note >= 0) {
        g->bass_note = note;
        g->chord_fresh = false;
        note_on(g, TG_PART_BASS, note, vel, offset, fn, ctx);
    } else if (g->bass_mode == TG_BASS_LINE || g->bass_mode == TG_BASS_DRONE || g->bass_mode == TG_BASS_TOLL) {
        g->chord_fresh = false;
    }
}

/* ---- third-part modes -------------------------------------------------------------------------------- */
/* per tick, before the melody: chords on the off-beats, tremolo, figures */
static void pad_mode_tick(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    const tg_style *st = g->style;
    switch (g->pad_mode) {
    case TG_PAD_OOMPAH: {
        if (g->oompah_off >= 0 && b.q >= g->oompah_off) {
            pad_off(g, offset, fn, ctx);
            g->oompah_off = -1;
        }
        bool pah = b.on_beat && !b.downbeat && (b.m.nbeats != 4 || b.m.beat % 2 == 1);
        if (pah) {
            pad_voice(g, offset, accent(velocity, b) - 20, true, false, fn, ctx);
            g->oompah_off = b.q + (b.m.glen >= 2 ? b.m.glen / 2 : 1);
        }
        break;
    }
    case TG_PAD_TREMOLO:
        pad_voice(g, offset, accent(velocity, b) - 28, true, false, fn, ctx);
        break;
    case TG_PAD_FIGURE:
    case TG_PAD_ADDITIVE: {
        int len = st ? st->figure_len : 0;
        if (len <= 0)
            break;
        int k;
        if (g->pad_mode == TG_PAD_FIGURE) {
            int pos = g->chord_ticks > 0 ? b.q % g->chord_ticks : b.q;
            k = st->pad_figure[pos % len];
        } else {
            int L = g->add_len < 1 ? 1 : g->add_len > len ? len : g->add_len;
            k = st->pad_figure[g->add_pos % L];
            if (++g->add_pos >= L) {
                g->add_pos = 0;
                if (++g->add_rep >= (st->phase_reps ? st->phase_reps : 2)) {
                    g->add_rep = 0;
                    L += g->add_dir;
                    if (L > len) { L = len - 1; g->add_dir = -1; }
                    if (L < 2) { L = 2; g->add_dir = 1; }
                    g->add_len = L;
                }
            }
        }
        if (k == TG_REST) {
            note_off(g, TG_PART_PAD, offset, fn, ctx);
            break;
        }
        int note = ladder_note(g, g->base_note[TG_PART_PAD] + g->root, k);
        note_on(g, TG_PART_PAD, note, accent(velocity, b) - 24, offset, fn, ctx);
        break;
    }
    default:
        break;
    }
}

/* after the melody: the parts that follow it */
static void pad_follow(tg_gen *g, beat_pos b, int velocity, bool new_note, int offset, tg_event_fn fn, void *ctx)
{
    const tg_style *st = g->style;
    int mel = g->sounding[TG_PART_MELODY] >= 0 ? g->sounding[TG_PART_MELODY] : g->mel_note;   /* what sounds (a trill) */
    switch (g->pad_mode) {
    case TG_PAD_ORGANUM:
    case TG_PAD_TINTI:
        if (g->sounding[TG_PART_MELODY] < 0) {
            note_off(g, TG_PART_PAD, offset, fn, ctx);
            break;
        }
        if (!new_note)
            break;
        if (g->pad_mode == TG_PAD_ORGANUM) {
            int n = mel - (g->pad_interval ? g->pad_interval : 7);
            uint16_t sc = tt_scale_mask(scale_now(g)) | g->chord_scale;
            if (!in_mask(sc, n, g->root))
                n = mask_nearest(g->chord_scale, g->root, n);
            note_on(g, TG_PART_PAD, n, accent(velocity, b) - 16, offset, fn, ctx);
        } else {
            /* the tonic triad's note nearest below the melody */
            uint16_t triad = (uint16_t)(1u | (1u << (key_is_minor(g) ? 3 : 4)) | (1u << 7));
            int n = mel - 1;
            while (!in_mask(triad, n, g->root)) n--;
            note_on(g, TG_PART_PAD, n, accent(velocity, b) - 14, offset, fn, ctx);
        }
        break;
    case TG_PAD_CANON: {
        int delay = st && st->canon_ticks ? st->canon_ticks : 2 * b.bar_ticks;
        if (delay >= TG_CANON_MAX) delay = TG_CANON_MAX - 1;
        g->canon_buf[b.q % TG_CANON_MAX] = (int16_t)(new_note ? mel : g->sounding[TG_PART_MELODY] < 0 ? -2 : -1);
        if (b.q < delay)
            break;
        int v = g->canon_buf[(b.q - delay) % TG_CANON_MAX];
        if (v == -2) {
            note_off(g, TG_PART_PAD, offset, fn, ctx);
        } else if (v >= 0) {
            int n = mask_step(tt_scale_mask(scale_now(g)), g->root, v, st ? st->canon_steps : -7);
            if (!in_mask(g->chord_scale, n, g->root))
                n = mask_nearest(g->chord_scale, g->root, n);
            note_on(g, TG_PART_PAD, n, accent(velocity, b) - 18, offset, fn, ctx);
        }
        break;
    }
    case TG_PAD_PHASE: {
        int len = st ? st->pattern_len : 0;
        if (len <= 0)
            break;
        int n = pattern_note(g, g->pat_i - 1 + g->phase_off, g->base_note[TG_PART_MELODY] + g->root);
        if (n < 0)
            note_off(g, TG_PART_PAD, offset, fn, ctx);
        else
            note_on(g, TG_PART_PAD, n, accent(velocity, b) - 10, offset, fn, ctx);
        if (g->pat_i % len == 0 && ++g->phase_rep >= (st->phase_reps ? st->phase_reps : 4)) {
            g->phase_rep = 0;
            g->phase_off = (g->phase_off + 1) % len;
        }
        break;
    }
    default:
        break;
    }
}

/* ---- percussion -------------------------------------------------------------------------------------- */
/* Percussion never draws from the harmonizer's randomness (g->hrng): it has its own (g->prng), so
 * switching it on or off leaves every note of the three voices exactly as it was. */
static float prand(tg_gen *g) { return tm_rng_float(&g->prng); }

void tg_set_percussion(tg_gen *g, int mode) { g->perc_mode = mode; }

static void drum_hit(tg_gen *g, int note, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    for (int i = 0; i < g->drum_n; ++i)
        if (g->drum_on[i] == note)
            return;                                 /* one stroke per instrument per tick */
    if (g->drum_n < 8)
        g->drum_on[g->drum_n++] = note;
    emit(fn, ctx, TG_PART_DRUMS, note, velocity, true, offset);
    g->perc_hits++;
}

static void drums_release(tg_gen *g, int offset, tg_event_fn fn, void *ctx)
{
    for (int i = 0; i < g->drum_n; ++i)
        emit(fn, ctx, TG_PART_DRUMS, g->drum_on[i], 0, false, offset);
    g->drum_n = 0;
}

/* the two kettledrums: the tonic, and the dominant a fourth below it, in the timpani's range */
static int timp_pitch(const tg_gen *g, bool dominant)
{
    int tonic = 40 + pmod12(g->root - 4);           /* E2 .. D#3 */
    if (!dominant)
        return tonic;
    int d = tonic - 5;
    return d < 36 ? d + 12 : d;
}

static void timp_hit(tg_gen *g, int note, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    if (g->timp_note >= 0)
        emit(fn, ctx, TG_PART_TIMPANI, g->timp_note, 0, false, offset);
    emit(fn, ctx, TG_PART_TIMPANI, note, velocity, true, offset);
    g->timp_note = note;
    g->timp_off_at = g->ticks - 1 + 2 * g->division;
    g->perc_hits++;
}

/* the composer's own percussion */
static void perc_period(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    const uint8_t *w = g->perc_w;
    int period, d = tg_period_position(g, &period);
    int to_arrival = period - d;
    bool arrival = d == 0 && b.q > 0;
    bool chord_change = g->chord_ticks > 0 && b.q % g->chord_ticks == 0;
    int beat = b.m.beat, sub = b.m.sub;
    uint16_t cm = chord_mask(&g->chord, 3);
    bool has_tonic = cm & 1, has_dom = (cm >> 7) & 1;
    float swell = (float)d / (float)period;
    if (d == 0)
        g->march_on = w[TG_PI_SNARE] && prand(g) * 10.0f < (float)w[TG_PI_SNARE];   /* decided per period */

    /* timpani: tonic and dominant only, as the instrument was tuned */
    if (w[TG_PI_TIMPANI]) {
        float wt = (float)w[TG_PI_TIMPANI] / 10.0f;
        int tonic = timp_pitch(g, false), dom = timp_pitch(g, true);
        if (arrival) {
            g->roll_until = -1;
            timp_hit(g, tonic, velocity + 18, offset, fn, ctx);
        } else if (b.q < g->roll_until) {
            timp_hit(g, g->roll_note, velocity - 18 + 24 * (g->division - (int)(g->roll_until - b.q)) / g->division,
                     offset, fn, ctx);
            g->roll_countdown = g->sample_rate / g->t.rate_hz * 0.5f;
        } else if (to_arrival <= g->division && to_arrival > 0 && wt >= 0.5f && (has_tonic || has_dom)) {
            /* a roll into the arrival, on the dominant if the chord has it */
            g->roll_note = has_dom ? dom : tonic;
            g->roll_until = b.q + to_arrival;
            timp_hit(g, g->roll_note, velocity - 18, offset, fn, ctx);
            g->roll_countdown = g->sample_rate / g->t.rate_hz * 0.5f;
        } else if (d == tg_half_position(g) && d > 0 && has_dom) {
            timp_hit(g, dom, velocity + 8, offset, fn, ctx);           /* the half cadence */
        } else if (b.downbeat && (has_tonic || has_dom) && prand(g) < wt * (chord_change ? 0.55f : 0.15f)) {
            timp_hit(g, has_tonic && (!has_dom || prand(g) < 0.6f) ? tonic : dom, velocity - 4, offset, fn, ctx);
        } else if (wt >= 0.8f && b.on_beat && swell > 0.75f && (has_tonic || has_dom) && prand(g) < 0.25f) {
            timp_hit(g, has_dom ? dom : tonic, velocity - 10, offset, fn, ctx);    /* building to the end */
        }
    }

    /* the march: snare 1 2-and 3 4, bass drum on 1 and 3, a snare roll into the arrival */
    if (g->march_on) {
        int sv = velocity - 16;
        if (to_arrival <= b.bar_ticks && to_arrival > 0)
            drum_hit(g, TG_GM_SNARE, sv + 20 * (b.bar_ticks - to_arrival) / b.bar_ticks, offset, fn, ctx);
        else if (sub == 0 || (beat == 1 && b.m.glen >= 2 && sub == b.m.glen / 2))
            drum_hit(g, TG_GM_SNARE, sv + (beat == 0 ? 14 : 0), offset, fn, ctx);
        if (w[TG_PI_BASS_DRUM] && sub == 0 && (beat == 0 || (b.m.nbeats == 4 && beat == 2)))
            drum_hit(g, TG_GM_BASS_DRUM, velocity - 4, offset, fn, ctx);
    } else if (w[TG_PI_BASS_DRUM]) {
        float wb = (float)w[TG_PI_BASS_DRUM] / 10.0f;
        if (arrival || (b.downbeat && chord_change && prand(g) < wb * 0.4f))
            drum_hit(g, TG_GM_BASS_DRUM, velocity - (arrival ? 0 : 14), offset, fn, ctx);
    }

    /* cymbal: a crash at the arrival, soft strokes for the colourists */
    if (w[TG_PI_CYMBAL]) {
        float wc = (float)w[TG_PI_CYMBAL] / 10.0f;
        bool soft = g->whole_tone > 0.0f || g->style == tg_style_get(TG_STYLE_SATIE);
        if (arrival && prand(g) < wc * 1.4f)
            drum_hit(g, TG_GM_CRASH, soft ? velocity - 30 : velocity + 14, offset, fn, ctx);
        else if (soft && b.downbeat && chord_change && prand(g) < wc * 0.4f)
            drum_hit(g, TG_GM_CRASH, velocity - 40, offset, fn, ctx);
    }

    /* triangle: the arrival and a few downbeats */
    if (w[TG_PI_TRIANGLE]) {
        float wt = (float)w[TG_PI_TRIANGLE] / 10.0f;
        if ((arrival && prand(g) < wt * 1.5f) || (b.downbeat && prand(g) < wt * 0.35f))
            drum_hit(g, TG_GM_TRIANGLE, velocity - 18, offset, fn, ctx);
    }

    /* tambourine: off-beats in the second half of the period, as the music gathers */
    if (w[TG_PI_TAMBOURINE]) {
        float wt = (float)w[TG_PI_TAMBOURINE] / 10.0f;
        bool off = b.m.glen >= 2 ? sub == b.m.glen / 2 : !b.on_beat;
        if ((swell > 0.5f && off && prand(g) < wt * 0.8f) || (arrival && prand(g) < wt))
            drum_hit(g, TG_GM_TAMBOURINE, velocity - 20, offset, fn, ctx);
    }
}

/* a modern kit: kick and backbeat, hats, Marbles adding kicks (T1) and ghost notes (T3),
 * a fill into each arrival and a crash on it */
static void perc_kit(tg_gen *g, beat_pos b, int velocity, bool t1, bool t3, int offset, tg_event_fn fn, void *ctx)
{
    if (g->perc_quiet)
        return;
    int period, d = tg_period_position(g, &period);
    int to_arrival = period - d;
    bool arrival = d == 0 && b.q > 0;
    int beat = b.m.beat, sub = b.m.sub;
    bool eighth = g->meter[0] ? true : sub == 0 || (g->division >= 2 && sub == g->division / 2);
    if (arrival) {
        drum_hit(g, TG_GM_CRASH, velocity + 14, offset, fn, ctx);
        drum_hit(g, TG_GM_KICK, velocity + 14, offset, fn, ctx);
        return;
    }
    if (to_arrival <= g->division && to_arrival > 0) {                  /* the fill */
        drum_hit(g, TG_GM_SNARE, velocity - 12 + 20 * (g->division - to_arrival) / g->division, offset, fn, ctx);
        return;
    }
    if (g->meter[0]) {
        /* beat groups: kick on the bar, snare on its last group; between, kicks in short bars (3+3+2:
         * kick kick snare, the tresillo), kick and snare in turn in long ones */
        if (sub == 0) {
            bool last = beat == b.m.nbeats - 1 && b.m.nbeats > 1;
            bool kick = beat == 0 || (!last && (b.m.nbeats <= 3 || beat % 2 == 0));
            drum_hit(g, kick ? TG_GM_KICK : TG_GM_SNARE, velocity + (beat == 0 ? 8 : 0), offset, fn, ctx);
        }
    } else if (g->beats_per_bar == 3) {                                 /* waltz: kick on one, side stick after */
        if (b.downbeat)
            drum_hit(g, TG_GM_KICK, velocity + 6, offset, fn, ctx);
        else if (sub == 0)
            drum_hit(g, TG_GM_SIDE_STICK, velocity - 14, offset, fn, ctx);
    } else {
        if (sub == 0 && (beat == 0 || beat == 2))
            drum_hit(g, TG_GM_KICK, velocity + (beat == 0 ? 8 : 0), offset, fn, ctx);
        if (sub == 0 && (beat == 1 || beat == 3))
            drum_hit(g, TG_GM_SNARE, velocity + 2, offset, fn, ctx);
    }
    if (t1 && eighth && sub != 0 && prand(g) < 0.5f)
        drum_hit(g, TG_GM_KICK, velocity - 10, offset, fn, ctx);       /* Marbles' syncopated kicks */
    if (t3 && !b.on_beat && prand(g) < 0.3f)
        drum_hit(g, TG_GM_SNARE, velocity - 38, offset, fn, ctx);      /* ghost notes */
    if (eighth) {
        bool open = beat == b.m.nbeats - 1 && sub != 0 && prand(g) < 0.35f;
        drum_hit(g, open ? TG_GM_HAT_OPEN : TG_GM_HAT_CLOSED, velocity - (sub == 0 ? 18 : 28), offset, fn, ctx);
    }
}

/* a style's own drum pattern: up to three instruments on fixed ticks of the bar (Bolero's snare) */
static void perc_pattern(tg_gen *g, beat_pos b, int velocity, int offset, tg_event_fn fn, void *ctx)
{
    const tg_style *st = g->style;
    if (!st)
        return;
    for (int k = 0; k < 3; ++k) {
        int note = st->drum_pat[k].note;
        if (note && ((st->drum_pat[k].ticks >> b.m.bar_pos) & 1))
            drum_hit(g, note, st->drum_pat[k].vel ? st->drum_pat[k].vel + (velocity - 85) / 3 : velocity, offset, fn, ctx);
    }
}

/* ---- humanization ---------------------------------------------------------------------------------- */
/* Every event can come a little late, as from a player: up to a few milliseconds per part (each note
 * of a chord on its own, which spreads it slightly), and velocities vary a little. A note's release
 * keeps its note's delay, so lengths stay as written. Off by default: the checks see exact time. */
static const float HUMAN_MS[TG_EVENT_PARTS] = {6.0f, 10.0f, 8.0f, 3.0f, 4.0f};   /* bass melody third drums timpani */

static void hum_collect(void *ctx, const tg_event *e)
{
    tg_gen *g = ctx;
    if (g->hq_n >= TG_HQ) {                         /* full: deliver as is (never happens at sane rates) */
        g->user_fn(g->user_ctx, e);
        return;
    }
    tg_event x = *e;
    long d;
    if (x.type != TG_EV_NOTE) {
        d = 0;                                      /* a sound lock: before its note, never late */
    } else if (x.on) {
        float r = tm_rng_float(&g->hum_rng);
        d = (long)(r * r * HUMAN_MS[x.part] * 0.001f * g->sample_rate * g->humanize);   /* mostly small */
        d += g->tick_delay;                         /* swing, lilt */
        g->hdelay[x.part][x.note] = (uint16_t)d;
        int dv = (int)lrintf((tm_rng_float(&g->hum_rng) * 2.0f - 1.0f) * 8.0f * g->humanize);
        x.velocity = (uint8_t)clampi(x.velocity + dv, 1, 127);
    } else {
        d = g->hdelay[x.part][x.note];
    }
    g->hq[g->hq_n].due = g->clock + x.offset + d;
    g->hq[g->hq_n].e = x;
    g->hq_n++;
}

/* deliver the queued events due before `end`, earliest first, with offsets inside this block */
static void hum_deliver(tg_gen *g, long end)
{
    while (1) {
        int best = -1;
        for (int i = 0; i < g->hq_n; ++i)
            if (g->hq[i].due < end && (best < 0 || g->hq[i].due < g->hq[best].due))
                best = i;
        if (best < 0)
            return;
        tg_event e = g->hq[best].e;
        long off = g->hq[best].due - g->clock;
        e.offset = (uint16_t)(off < 0 ? 0 : off);
        g->hq[best] = g->hq[--g->hq_n];
        g->user_fn(g->user_ctx, &e);
    }
}

static void process_block(tg_gen *g, int size, tg_event_fn fn, void *ctx);
static void rec_sink(void *ctx, const tg_event *e);
static void seq_tick(tg_gen *g, int offset);
static void key_apply(tg_gen *g);
static void form_step(tg_gen *g, int offset);

void tg_process(tg_gen *g, int size, tg_event_fn fn, void *ctx)
{
    if (size > TG_MAX_BLOCK)
        size = TG_MAX_BLOCK;
    g->user_fn = fn;
    g->user_ctx = ctx;
    process_block(g, size, rec_sink, g);            /* -> recording -> overlay -> humanizer -> fn */
    if (g->hq_n)
        hum_deliver(g, g->clock + size);
    g->clock += size;
}

/* ---- sequencer ---------------------------------------------------------------------------------------
 * Event path: the generator -> rec_sink (records the period; drops everything while frozen) -> the step
 * overlay (conditions, locks, evolution's dynamics) -> the humanizer -> the caller. The frozen pattern
 * enters at the overlay, so locks and conditions treat both alike. */
static void out_event(tg_gen *g, const tg_event *e)
{
    if (g->humanize > 0.0f || g->hq_n || g->tick_delay)
        hum_collect(g, e);
    else
        g->user_fn(g->user_ctx, e);
}

static void out_param(tg_gen *g, int part, int param, int value, int offset)
{
    tg_event e = {(uint8_t)part, 0, 0, false, (uint16_t)offset, TG_EV_PARAM, (uint8_t)param, (int16_t)value};
    out_event(g, &e);
}

/* the step's position and loop: the pattern's while frozen, else the period's */
static int seq_pos(const tg_gen *g, uint32_t *loop)
{
    if (g->frozen) {
        *loop = g->pat_loop;
        return g->pat_pos;
    }
    *loop = (uint32_t)g->period_index;
    return tg_period_position(g, NULL);
}

static tg_step *step_find(tg_gen *g, int part, int tick)
{
    for (int i = 0; i < g->nsteps; ++i)
        if (g->steps[i].part == part && g->steps[i].tick == tick)
            return &g->steps[i];
    return 0;
}

static bool cond_eval(tg_gen *g, const tg_step *st, int part, int pos, uint32_t loop)
{
    if (g->cond_tick[part] == pos && g->cond_loop[part] == loop)
        return g->cond_pass[part];                  /* one verdict per step: a chord stays whole */
    bool r = true;
    switch (st->cond) {
    case TG_C_PROB:      r = tm_rng_float(&g->srng) * 100.0f < (float)st->ca; break;
    case TG_C_RATIO:     r = st->cb ? (int)(loop % st->cb) == (st->ca + st->cb - 1) % st->cb : true; break;
    case TG_C_FILL:      r = g->fill; break;
    case TG_C_NOT_FILL:  r = !g->fill; break;
    case TG_C_FIRST:     r = loop == 0; break;
    case TG_C_NOT_FIRST: r = loop > 0; break;
    case TG_C_PRE:       r = g->cond_pre[part]; break;
    case TG_C_NOT_PRE:   r = !g->cond_pre[part]; break;
    default: break;
    }
    if (st->cond != TG_C_ALWAYS && st->cond != TG_C_PRE && st->cond != TG_C_NOT_PRE)
        g->cond_pre[part] = r;
    g->cond_tick[part] = pos;
    g->cond_loop[part] = loop;
    g->cond_pass[part] = r;
    return r;
}

static bool len_bit(const tg_gen *g, int part, int note) { return (g->lenlocked[part][note >> 3] >> (note & 7)) & 1; }
static void len_set(tg_gen *g, int part, int note, bool on)
{
    if (on) g->lenlocked[part][note >> 3] |= (uint8_t)(1u << (note & 7));
    else g->lenlocked[part][note >> 3] &= (uint8_t)~(1u << (note & 7));
}

static void loff_cancel(tg_gen *g, int part, int note)
{
    for (int i = 0; i < g->loff_n; ++i)
        if (g->loff[i].part == part && g->loff[i].note == note) {
            g->loff[i] = g->loff[--g->loff_n];
            len_set(g, part, note, false);
            return;
        }
}

/* the overlay: everything that plays passes here */
static void overlay_in(tg_gen *g, const tg_event *ein)
{
    if (ein->type != TG_EV_NOTE || ein->part >= TG_EVENT_PARTS) {
        out_event(g, ein);
        return;
    }
    tg_event e = *ein;
    int part = e.part;
    if (!e.on) {
        int m = g->xmap[part][e.note];
        g->xmap[part][e.note] = -1;
        if (m == -2)
            return;                                 /* its start was skipped by a condition */
        if (m >= 0) {
            if (len_bit(g, part, m))
                return;                             /* a LEN lock releases it */
            e.note = (uint8_t)m;
        }
        out_event(g, &e);
        return;
    }
    uint32_t loop;
    int pos = seq_pos(g, &loop);
    tg_step *st = g->nsteps ? step_find(g, part, pos) : 0;
    /* sound locks of the part's previous note go back, unless this step locks them again */
    if (g->sound_locked[part]) {
        uint32_t keep = 0;
        for (int k = 0; st && k < st->nlock; ++k)
            keep |= 1u << st->lock[k].param;
        for (int prm = TG_LK_E0; prm < TG_LK_COUNT; ++prm)
            if ((g->sound_locked[part] >> prm) & 1u && !((keep >> prm) & 1u)) {
                out_param(g, part, prm, TG_LK_RESTORE, e.offset);
                g->sound_locked[part] &= ~(1u << prm);
            }
    }
    if (st && !cond_eval(g, st, part, pos, loop)) {
        g->xmap[part][e.note] = -2;
        return;
    }
    int note = e.note, vel = e.velocity + (g->evolve ? g->energy_vel : 0) + g->echo_vel, len = 0;
    bool pitched = part != TG_PART_DRUMS && part != TG_PART_TIMPANI;
    for (int k = 0; st && k < st->nlock; ++k) {
        int prm = st->lock[k].param, v = st->lock[k].value;
        switch (prm) {
        case TG_LK_VEL:   vel = v; break;
        case TG_LK_STEPS: if (pitched) note = mask_step(tt_scale_mask(g->scale), g->root, note, v); break;
        case TG_LK_OCT:   if (part != TG_PART_DRUMS) note += 12 * v; break;
        case TG_LK_LEN:   len = v; break;
        default:
            out_param(g, part, prm, v, e.offset);
            g->sound_locked[part] |= 1u << prm;
            break;
        }
    }
    note = clampi(note, 0, 127);
    loff_cancel(g, part, note);
    if (len > 0 && g->loff_n < 16) {
        g->loff[g->loff_n].part = (uint8_t)part;
        g->loff[g->loff_n].note = (uint8_t)note;
        g->loff[g->loff_n].left = (uint16_t)len;
        g->loff_n++;
        len_set(g, part, note, true);
    }
    g->xmap[part][e.note] = (int16_t)note;
    e.note = (uint8_t)note;
    e.velocity = (uint8_t)clampi(vel, 1, 127);
    if (g->tuning && pitched)
        e.value = (int16_t)tt_tuning_cents10(g->tuning, note - g->tune_root);   /* cents x10 off equal */
    out_event(g, &e);
}

/* LEN-locked releases, once per tick */
static void loff_tick(tg_gen *g, int offset)
{
    for (int i = 0; i < g->loff_n;) {
        if (--g->loff[i].left == 0) {
            tg_event e = {g->loff[i].part, g->loff[i].note, 0, false, (uint16_t)offset, TG_EV_NOTE, 0, 0};
            len_set(g, e.part, e.note, false);
            g->loff[i] = g->loff[--g->loff_n];
            out_event(g, &e);
        } else {
            ++i;
        }
    }
}

/* recording: every note the generator plays, by tick of the period */
static void rec_sink(void *ctx, const tg_event *e)
{
    tg_gen *g = ctx;
    if (g->frozen)
        return;                                     /* the generator runs on silently */
    if (e->on && e->type == TG_EV_NOTE && g->gp_from >= 0 && g->ticks - 1 >= g->gp_from && g->ticks - 1 < g->gp_until)
        return;                                     /* a general pause: everything is silent */
    if (e->type == TG_EV_NOTE && e->part < TG_EVENT_PARTS) {
        tg_pattern *r = &g->rec[g->rec_cur];
        int d = tg_period_position(g, NULL);
        int open = g->rec_open[e->part][e->note];
        if (open >= 0) {                            /* its release, or a new start of the same note */
            int l = d - r->ev[open].tick;
            r->ev[open].len = (uint16_t)(l > 0 ? l : 1);
            g->rec_open[e->part][e->note] = -1;
        }
        if (e->on && r->n < TG_PAT_MAX) {
            tg_pnote *x = &r->ev[r->n];
            x->tick = (uint16_t)d;
            x->len = 0;
            x->part = e->part;
            x->note = e->note;
            x->vel = e->velocity;
            x->pad = 0;
            g->rec_open[e->part][e->note] = (int16_t)r->n++;
        }
    }
    overlay_in(g, e);
}

/* release what the generator holds (through the overlay, so locked notes end right) and forget it */
static void gen_release(tg_gen *g, int offset, bool sounding)
{
    tg_event e = {0, 0, 0, false, (uint16_t)offset, TG_EV_NOTE, 0, 0};
    for (int p = 0; p < TG_PARTS; ++p) {
        if (sounding && g->sounding[p] >= 0) {
            e.part = (uint8_t)p;
            e.note = (uint8_t)g->sounding[p];
            overlay_in(g, &e);
        }
        g->sounding[p] = -1;
    }
    for (int i = 0; i < g->pad_n; ++i)
        if (sounding && !g->pad_silent) {
            e.part = TG_PART_PAD;
            e.note = (uint8_t)g->pad[i];
            overlay_in(g, &e);
        }
    g->pad_n = 0;
    g->pad_poly = 0;
    for (int i = 0; i < g->drum_n; ++i)
        if (sounding) {
            e.part = TG_PART_DRUMS;
            e.note = (uint8_t)g->drum_on[i];
            overlay_in(g, &e);
        }
    g->drum_n = 0;
    if (sounding && g->timp_note >= 0) {
        e.part = TG_PART_TIMPANI;
        e.note = (uint8_t)g->timp_note;
        overlay_in(g, &e);
    }
    g->timp_note = -1;
    g->roll_until = -1;
    g->counter_note = -1;
    g->trill_until = -1;
    g->mel_pending = -1;
}

static void pat_release(tg_gen *g, int offset)
{
    for (int i = 0; i < g->play_n; ++i) {
        tg_event e = {g->play[i].part, g->play[i].note, 0, false, (uint16_t)offset, TG_EV_NOTE, 0, 0};
        overlay_in(g, &e);
    }
    g->play_n = 0;
}

/* one tick of the frozen pattern */
static void pat_tick(tg_gen *g, int offset)
{
    for (int i = 0; i < g->play_n;) {
        if (--g->play[i].left == 0) {
            tg_event e = {g->play[i].part, g->play[i].note, 0, false, (uint16_t)offset, TG_EV_NOTE, 0, 0};
            g->play[i] = g->play[--g->play_n];
            overlay_in(g, &e);
        } else {
            ++i;
        }
    }
    for (int i = 0; i < g->pat.n; ++i) {
        const tg_pnote *x = &g->pat.ev[i];
        if (x->tick != g->pat_pos)
            continue;
        for (int k = 0; k < g->play_n; ++k)         /* the same note again: end it first */
            if (g->play[k].part == x->part && g->play[k].note == x->note) {
                tg_event e = {x->part, x->note, 0, false, (uint16_t)offset, TG_EV_NOTE, 0, 0};
                g->play[k] = g->play[--g->play_n];
                overlay_in(g, &e);
                break;
            }
        tg_event e = {x->part, x->note, x->vel, true, (uint16_t)offset, TG_EV_NOTE, 0, 0};
        overlay_in(g, &e);
        if (g->play_n < 48) {
            g->play[g->play_n].part = x->part;
            g->play[g->play_n].note = x->note;
            g->play[g->play_n].left = (uint16_t)(x->len ? x->len : 1);
            g->play_n++;
        }
    }
}

/* evolution: the energy and the key of the period about to start */
static void evolve_step(tg_gen *g)
{
    int P = g->evolve_periods > 1 ? g->evolve_periods : 8;
    int k = (int)(g->period_index % P);
    float e = sinf(3.14159265f * ((float)k + 0.5f) / (float)P);
    float dpt = g->evolve_depth;
    g->energy = e;
    float dens = g->base_density * (1.0f - 0.5f * dpt + dpt * e);
    g->melody_density = dens < 0.05f ? 0.05f : dens > 1.0f ? 1.0f : dens;
    g->energy_vel = (int)lrintf((e - 0.5f) * 24.0f * dpt);
    for (int i = 0; i < TG_PI_COUNT; ++i) {
        int w = (int)lrintf((float)g->base_perc[i] * (1.0f - 0.7f * dpt + 1.0f * dpt * e));
        g->perc_w[i] = (uint8_t)(w > 10 ? 10 : w);
    }
    g->perc_quiet = g->perc_mode == TG_PERC_KIT && e < 0.35f * dpt;
    if (g->evolve_modulate && P >= 4) {
        int away = P / 2, home = P / 2 + (P >= 8 ? P / 4 : 1);
        bool minor = (tt_scale_mask(g->base_scale) >> 3) & 1;
        if (k == away && !g->key_away) {            /* to the dominant, or the relative major */
            g->next_root = minor ? (g->base_root + 3) % 12 : (g->base_root + 7) % 12;
            g->next_scale = minor ? TS_IONIAN : g->base_scale;
            g->key_pending = true;
            g->key_away = true;
            g->key_set_tick = g->ticks;
        } else if (k == home && g->key_away) {
            g->next_root = g->base_root;
            g->next_scale = g->base_scale;
            g->key_pending = true;
            g->key_away = false;
            g->key_set_tick = g->ticks;
        }
    }
    if (g->key_away)
        g->periods_away++;
}

/* instrumentation: the style's sets of families, changed at period starts (by turns, at random, with the
 * energy of the evolution, or with the contrast periods); a PROGRAM event per part that changes */
static void orch_step(tg_gen *g, int offset)
{
    const tg_style *st = g->style;
    int n = st->orch_n;
    if (n <= 0)
        return;
    int set = g->orch_set;
    switch (st->orch_mode) {
    case TG_ORCH_CYCLE:    set = (int)(g->period_index % n); break;
    case TG_ORCH_RANDOM:
        if (g->period_index > 0 && tm_rng_float(&g->orng) < 0.5f)
            set = (int)(tm_rng_float(&g->orng) * (float)n);
        break;
    case TG_ORCH_ENERGY:
        set = g->evolve ? (int)(g->energy * (float)n) : (int)(g->period_index % n);
        break;
    case TG_ORCH_CONTRAST: set = g->alt_on && n > 1 ? 1 : 0; break;
    default:               set = 0; break;
    }
    if (set >= n) set = n - 1;
    if (set < 0) set = 0;
    g->orch_set = set;
    for (int p = 0; p < TG_PARTS; ++p) {
        int f = st->orch[set][p];
        if (f == TI_DEFAULT || f == g->orch_cur[p])
            continue;
        g->orch_cur[p] = f;
        tg_event e = {(uint8_t)p, 0, 0, false, (uint16_t)offset, TG_EV_PROGRAM, 0, (int16_t)f};
        out_event(g, &e);
    }
}

/* the form at a period's start: a contrast period (other modes, tempo, density), an echo, a general
 * pause after the arrival */
static void form_step(tg_gen *g, int offset)
{
    const tg_style *st = g->style;
    if (st->alt_every) {
        bool on = (g->period_index / st->alt_every) % 2 == 1;
        if (on != g->alt_on) {
            g->alt_on = on;
            gen_release(g, offset, true);           /* the parts change their ways: start clean */
            apply_modes(g, on);
            if (g->evolve)
                g->base_density = st->melody_density * (on && st->alt.density > 0.0f ? st->alt.density : 1.0f);
        }
    }
    orch_step(g, offset);
    g->echo_vel = g->echo > 0.0f && hrand(g) < g->echo ? -22 : 0;
    if (g->gp > 0.0f && hrand(g) < g->gp) {
        int bar = bar_ticks_of(g);
        g->gp_from = g->ticks - 1 + (long)g->arrival_bars * bar;
        g->gp_until = g->gp_from + bar;
    }
}

/* a modulation applies at the first chord change after the period's start */
static void key_apply(tg_gen *g)
{
    if (!g->key_pending || g->ticks <= g->key_set_tick)
        return;
    g->root = g->next_root;
    g->scale = g->next_scale;
    g->plan_minor = (tt_scale_mask(g->scale) >> 3) & 1;
    g->key_pending = false;
}

/* every tick, before the parts: period boundaries (recording, freeze, evolution), LEN releases, the
 * frozen pattern */
static void seq_tick(tg_gen *g, int offset)
{
    int period, d = tg_period_position(g, &period);
    if (d == 0 && g->ticks > 1) {
        /* the period just heard is complete */
        tg_pattern *r = &g->rec[g->rec_cur];
        for (int p = 0; p < TG_EVENT_PARTS; ++p)
            for (int n = 0; n < 128; ++n)
                if (g->rec_open[p][n] >= 0) {
                    tg_pnote *x = &r->ev[g->rec_open[p][n]];
                    x->len = (uint16_t)(r->len > x->tick ? r->len - x->tick : 1);
                    g->rec_open[p][n] = -1;
                }
        g->rec_valid = !g->frozen && r->len > 0;
        g->rec_cur ^= 1;
        if (g->unfreeze_req && g->frozen) {
            pat_release(g, offset);
            g->frozen = false;
            g->unfreeze_req = false;
            gen_release(g, offset, false);          /* it never sounded: just forget it */
            g->rec_valid = false;
        }
        if (g->freeze_req && !g->frozen && g->rec_valid) {
            gen_release(g, offset, true);
            g->pat = g->rec[!g->rec_cur];
            g->frozen = true;
            g->pat_pos = 0;
            g->pat_loop = 0;
            g->play_n = 0;
        }
        g->freeze_req = false;
        g->period_index++;
        if (g->evolve && !g->frozen)
            evolve_step(g);
        if (g->style && !g->frozen)
            form_step(g, offset);
        /* the next recording */
        r = &g->rec[g->rec_cur];
        r->n = 0;
        r->len = (uint16_t)period;
        r->root = (int8_t)(g->key_pending ? g->next_root : g->root);
        r->scale = (int8_t)(g->key_pending ? g->next_scale : g->scale);
    } else if (g->ticks == 1) {
        if (g->style)
            orch_step(g, offset);                   /* the opening instruments */
        g->rec[g->rec_cur].n = 0;
        g->rec[g->rec_cur].len = (uint16_t)period;
        g->rec[g->rec_cur].root = (int8_t)g->root;
        g->rec[g->rec_cur].scale = (int8_t)g->scale;
    }
    if (g->loff_n)
        loff_tick(g, offset);
    if (g->frozen) {
        pat_tick(g, offset);
        if (++g->pat_pos >= g->pat.len) {
            g->pat_pos = 0;
            g->pat_loop++;
        }
    }
}

bool tg_freeze(tg_gen *g)
{
    if (g->frozen)
        return false;
    g->freeze_req = true;
    g->unfreeze_req = false;
    return true;
}

void tg_unfreeze(tg_gen *g)
{
    if (g->frozen)
        g->unfreeze_req = true;
    g->freeze_req = false;
}

int tg_pat_add(tg_gen *g, int tick, int part, int note, int vel, int len)
{
    tg_pattern *pt = &g->pat;
    if (pt->n >= TG_PAT_MAX || tick < 0 || tick >= pt->len || part < 0 || part >= TG_EVENT_PARTS)
        return -1;
    int i = pt->n;
    while (i > 0 && pt->ev[i - 1].tick > tick) {
        pt->ev[i] = pt->ev[i - 1];
        --i;
    }
    pt->ev[i].tick = (uint16_t)tick;
    pt->ev[i].len = (uint16_t)(len > 0 ? len : 1);
    pt->ev[i].part = (uint8_t)part;
    pt->ev[i].note = (uint8_t)clampi(note, 0, 127);
    pt->ev[i].vel = (uint8_t)clampi(vel, 1, 127);
    pt->ev[i].pad = 0;
    pt->n++;
    return i;
}

void tg_pat_remove(tg_gen *g, int index)
{
    tg_pattern *pt = &g->pat;
    if (index < 0 || index >= pt->n)
        return;
    for (int i = index; i + 1 < pt->n; ++i)
        pt->ev[i] = pt->ev[i + 1];
    pt->n--;
}

void tg_pat_transpose(tg_gen *g, int part, int steps)
{
    tg_pattern *pt = &g->pat;
    uint16_t mask = tt_scale_mask(pt->scale);
    for (int i = 0; i < pt->n; ++i) {
        tg_pnote *x = &pt->ev[i];
        if (x->part == TG_PART_DRUMS || x->part == TG_PART_TIMPANI || (part >= 0 && x->part != part))
            continue;
        x->note = (uint8_t)clampi(mask_step(mask, pt->root, x->note, steps), 0, 127);
    }
}

int tg_step_cond(tg_gen *g, int part, int tick, int cond, int a, int b)
{
    tg_step *st = step_find(g, part, tick);
    if (!st) {
        if (g->nsteps >= TG_STEPS)
            return -1;
        st = &g->steps[g->nsteps++];
        zero(st, sizeof *st);
        st->part = (uint8_t)part;
        st->tick = (uint16_t)tick;
    }
    st->cond = (uint8_t)(cond >= 0 && cond < TG_C_COUNT ? cond : TG_C_ALWAYS);
    st->ca = (uint8_t)a;
    st->cb = (uint8_t)b;
    return 0;
}

int tg_step_lock(tg_gen *g, int part, int tick, int param, int value)
{
    tg_step *st = step_find(g, part, tick);
    if (!st) {
        if (tg_step_cond(g, part, tick, TG_C_ALWAYS, 0, 0))
            return -1;
        st = step_find(g, part, tick);
    }
    for (int k = 0; k < st->nlock; ++k)
        if (st->lock[k].param == param) {
            st->lock[k].value = (int16_t)value;
            return 0;
        }
    if (st->nlock >= TG_STEP_LOCKS || param < 0 || param >= TG_LK_COUNT)
        return -1;
    st->lock[st->nlock].param = (uint8_t)param;
    st->lock[st->nlock].value = (int16_t)value;
    st->nlock++;
    return 0;
}

void tg_step_clear(tg_gen *g, int part, int tick)
{
    tg_step *st = step_find(g, part, tick);
    if (st)
        *st = g->steps[--g->nsteps];
}

void tg_evolve(tg_gen *g, bool on, int periods, float depth, bool modulate)
{
    if (on && !g->evolve) {                         /* the settings now are the cycle's home */
        g->base_density = g->melody_density;
        g->base_root = g->root;
        g->base_scale = g->scale;
        for (int i = 0; i < TG_PI_COUNT; ++i)
            g->base_perc[i] = g->perc_w[i];
    }
    if (!on && g->evolve) {
        g->melody_density = g->base_density;
        for (int i = 0; i < TG_PI_COUNT; ++i)
            g->perc_w[i] = g->base_perc[i];
        g->energy_vel = 0;
        g->perc_quiet = false;
        if (g->key_away || g->key_pending) {        /* home at the next period */
            g->next_root = g->base_root;
            g->next_scale = g->base_scale;
            g->key_pending = true;
            g->key_away = false;
            g->key_set_tick = g->ticks;
        }
    }
    g->evolve = on;
    g->evolve_periods = periods > 1 ? periods : 8;
    g->evolve_depth = depth < 0.0f ? 0.0f : depth > 1.0f ? 1.0f : depth;
    g->evolve_modulate = modulate;
}

/* ---- process --------------------------------------------------------------------------------------- */
static void process_block(tg_gen *g, int size, tg_event_fn fn, void *ctx)
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
        int bar = bar_ticks_of(g), to_arrival = period - d;
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
            seq_tick(g, i);                          /* boundaries, freeze, evolution, the pattern */
            if (g->gp_from >= 0 && g->ticks - 1 == g->gp_from)
                gen_release(g, i, true);             /* the general pause begins */
            if (g->chord_ticks > 0 && g->ticks > 1 && (g->ticks - 1) % g->chord_ticks == 0) {
                g->chord_step++;
                g->chord_fresh = true;
                key_apply(g);
                if (g->style && g->plan_len > 0 && g->chord_step % g->plan_len == 0)
                    plan_period(g);                  /* a new period: plan its harmony */
            }
            reload_scale(g);
            g->tick_delay = 0;
            if ((g->swing > 0.0f || g->lilt > 0.0f) && !g->meter[0]) {
                /* swing: the off-beat eighth late by up to a sixth of a beat (2:1); lilt: in 3/4 the
                 * first beat holds back and the second comes early */
                float tickp = g->sample_rate / g->t.rate_hz;
                tg_meter_pos m;
                tg_meter_at(g, g->ticks - 1, &m);
                float d = 0.0f;
                if (g->swing > 0.0f && g->division % 2 == 0 && m.sub == g->division / 2)
                    d += g->swing * tickp * (float)g->division / 6.0f;
                if (g->lilt > 0.0f && g->beats_per_bar == 3)
                    d += g->lilt * tickp * (float)g->division * (m.beat == 0 ? 0.15f : m.beat == 2 ? 0.07f : 0.0f);
                g->tick_delay = (int)d;
            }
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
            default:
                pad_mode_tick(g, b, velocity, i, fn, ctx);
                break;
            }
            switch (g->bass_mode) {
            case TG_BASS_WALKING: if (b.on_beat) bass_walk(g, b, velocity, i, fn, ctx); break;
            case TG_BASS_ROOT:    if (g->chord_fresh) bass_root(g, b, velocity, i, fn, ctx); break;
            case TG_BASS_BAR:     if (b.downbeat || g->chord_fresh) bass_bar(g, b, velocity, i, fn, ctx); break;
            case TG_BASS_MARCH:   if (b.on_beat) bass_march(g, b, velocity, i, fn, ctx); break;
            case TG_BASS_PEDAL:   bass_pedal(g, b, velocity, i, fn, ctx); break;
            case TG_BASS_DRONE: case TG_BASS_OSTINATO: case TG_BASS_LINE: case TG_BASS_REPEAT:
            case TG_BASS_TOLL: case TG_BASS_BEAT: case TG_BASS_NONE:
                bass_mode_tick(g, b, velocity, i, fn, ctx);
                break;
            default:
                if (g->chord_fresh && g->since_note[TG_PART_BASS] >= min_gap)
                    bass_play(g, b, unit(g, TG_PART_BASS, o[0]), velocity, i, fn, ctx);
                break;
            }
            int before = g->sounding[TG_PART_MELODY];
            long before_count = g->motif_notes + g->free_notes;
            switch (g->mel_mode) {
            case TG_MEL_TINTI:    melody_tinti(g, b, velocity, i, fn, ctx); break;
            case TG_MEL_PATTERN:  melody_pattern(g, b, velocity, i, fn, ctx); break;
            case TG_MEL_BIRDSONG: melody_bird(g, b, velocity, i, fn, ctx); break;
            default:              melody_tick(g, b, o[1], velocity, i, fn, ctx); break;
            }
            bool new_note = g->sounding[TG_PART_MELODY] >= 0 &&
                            (g->sounding[TG_PART_MELODY] != before || g->motif_notes + g->free_notes != before_count);
            if (g->pad_mode == TG_PAD_COUNTER)
                pad_counter(g, b, velocity, i, fn, ctx);
            else if (g->pad_mode == TG_PAD_PARALLEL && new_note && g->since_note[TG_PART_MELODY] == 0)
                pad_parallel(g, accent(velocity, b), i, fn, ctx);
            else if (g->pad_mode == TG_PAD_DOUBLE && new_note && g->since_note[TG_PART_MELODY] == 0)
                pad_double(g, accent(velocity, b), i, fn, ctx);
            else if (g->pad_mode >= TG_PAD_ORGANUM)
                pad_follow(g, b, velocity, new_note && g->since_note[TG_PART_MELODY] == 0, i, fn, ctx);

            /* percussion, last, on its own randomness */
            drums_release(g, i, fn, ctx);
            if (g->timp_note >= 0 && b.q >= g->timp_off_at) {
                emit(fn, ctx, TG_PART_TIMPANI, g->timp_note, 0, false, i);
                g->timp_note = -1;
            }
            if (g->perc_mode == TG_PERC_PERIOD)
                perc_period(g, b, velocity, i, fn, ctx);
            else if (g->perc_mode == TG_PERC_KIT)
                perc_kit(g, b, velocity, gate[i * 2] && !g->kit_g1_prev, gate[i * 2 + 1] && !g->kit_g3_prev, i, fn, ctx);
            else if (g->perc_mode == TG_PERC_PATTERN)
                perc_pattern(g, b, velocity, i, fn, ctx);
        }
        g->kit_g1_prev = gate[i * 2];
        g->kit_g3_prev = gate[i * 2 + 1];
        if (g->roll_countdown > 0.0f) {
            g->roll_countdown -= 1.0f;
            if (g->roll_countdown <= 0.0f && g->roll_note >= 0 && g->ticks - 1 < g->roll_until)
                timp_hit(g, g->roll_note, velocity - 16, i, fn, ctx);   /* the roll's in-between stroke */
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
