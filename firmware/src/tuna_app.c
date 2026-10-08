/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA app (see tuna_app.h). */
#include "tuna_app.h"
#include "tuna_sys.h"

/* ---- the library ---------------------------------------------------------------------------------- */
int ta_lib_count(void)
{
    int n = tp_character_count();
    for (int e = 0; e < TP_ERA_COUNT; ++e)
        n += tp_era_count(e);
    return n;
}

int ta_lib_group(int item)
{
    if (item < tp_character_count())
        return 0;
    item -= tp_character_count();
    for (int e = 0; e < TP_ERA_COUNT; ++e) {
        if (item < tp_era_count(e))
            return 1 + e;
        item -= tp_era_count(e);
    }
    return TA_GROUPS - 1;
}

int ta_lib_first(int group)
{
    if (group <= 0)
        return 0;
    int n = tp_character_count();
    for (int e = 0; e < group - 1 && e < TP_ERA_COUNT; ++e)
        n += tp_era_count(e);
    return n;
}

int ta_lib_style(int item)
{
    int gr = ta_lib_group(item);
    if (gr == 0 || item < 0 || item >= ta_lib_count())
        return -1;
    return tp_era_style(gr - 1, item - ta_lib_first(gr));
}

const char *ta_lib_name(int item)
{
    if (item < 0 || item >= ta_lib_count())
        return "";
    int st = ta_lib_style(item);
    return st >= 0 ? tg_style_get(st)->name : tp_character_name(item);
}

const char *ta_group_name(int group) { return group == 0 ? "TUNA" : tp_era_name(group - 1); }

int ta_lib_born(int item)
{
    int st = ta_lib_style(item);
    return st >= 0 ? tp_style_born(st) : 0;
}

static int item_of_style(int style)
{
    for (int i = 0; i < ta_lib_count(); ++i)
        if (ta_lib_style(i) == style)
            return i;
    return -1;
}

/* ---- the sound and the screen ---------------------------------------------------------------------- */
static void on_event(void *ctx, const tg_event *e)
{
    tuna_app *a = ctx;
    if (a->sound && (a->local || (e->type == TG_EV_NOTE && !e->on)))
        a->sound(a->sound_ctx, e);               /* (local off: note-offs still go, nothing hangs) */
    tmi_event(&a->midi, e, a->frame + e->offset);
    if (e->type == TG_EV_NOTE && e->on) {
        ta_note *n = &a->feed[a->feed_w % TA_FEED];
        n->part = e->part;
        n->note = e->note;
        n->vel = e->velocity;
        a->feed_w = (uint8_t)(a->feed_w + 1);
        if ((uint8_t)(a->feed_w - a->feed_r) > TA_FEED)
            a->feed_r = (uint8_t)(a->feed_w - TA_FEED);   /* the screen fell behind: the oldest go */
    }
}

bool ta_feed_pop(tuna_app *a, ta_note *n)
{
    if (a->feed_r == a->feed_w)
        return false;
    *n = a->feed[a->feed_r % TA_FEED];
    a->feed_r = (uint8_t)(a->feed_r + 1);
    return true;
}

static void apply_req(tuna_app *a);

/* ---- loading ------------------------------------------------------------------------------------------ */
static float deja_value(const tuna_app *a)
{
    if (a->deja == TA_DV_OFF)
        return 0.0f;
    if (a->deja == TA_DV_LOCK)
        return 0.5f;                             /* Marbles: the loop locked */
    return a->g.style ? a->g.style->motif : 0.0f;
}

/* ---- the reverb ---- */
static const ta_reverb REVERBS[TG_REV_COUNT] = {
    [TG_REV_ROOM] = {"ROOM", "ROOM", 71, 48, false},           /* Felucca's reverb as designed (SIZE 90, DAMP 60 of 127) */
    [TG_REV_SPRING] = {"SPRING", "SPRING", 71, 48, true},
    [TG_REV_BATHROOM] = {"BATHROOM", "BATH", 22, 8, false},    /* short and bright: hard walls close by */
    [TG_REV_HALL] = {"HALL", "HALL", 86, 45, false},
    [TG_REV_CATHEDRAL] = {"CATHEDRAL", "CHURCH", 97, 30, false}, /* (about four seconds) */
};
static const uint8_t REV_ORDER[TG_REV_COUNT] = {TG_REV_BATHROOM, TG_REV_ROOM, TG_REV_HALL, TG_REV_CATHEDRAL,
                                                TG_REV_SPRING};

const ta_reverb *ta_reverb_type(int type) { return &REVERBS[type >= 0 && type < TG_REV_COUNT ? type : TG_REV_ROOM]; }

int ta_reverb_step(int type, int d)
{
    int i = 0;
    while (i < TG_REV_COUNT - 1 && REV_ORDER[i] != type)
        ++i;
    i += d;
    return REV_ORDER[i < 0 ? 0 : i >= TG_REV_COUNT ? TG_REV_COUNT - 1 : i];
}

int ta_piece_reverb(int style)
{
    const tg_style *s = style >= 0 ? tg_style_get(style) : 0;
    if (!s)
        return TG_REV_ROOM;
    if (s->reverb)
        return s->reverb - 1;
    switch (tp_style_era(style)) {
    case TP_ERA_RENAISSANCE: return TG_REV_CATHEDRAL;   /* (written for churches) */
    case TP_ERA_EARLY_ROMANTIC: case TP_ERA_ROMANTIC: case TP_ERA_LATE_ROMANTIC: case TP_ERA_FILM:
        return TG_REV_HALL;                             /* (the concert hall's orchestra) */
    default: return TG_REV_ROOM;
    }
}

void ta_set_reverb_type(tuna_app *a, int type)
{
    const ta_reverb *r = ta_reverb_type(type);
    a->rev_type = (uint8_t)(r - REVERBS);
    a->rev_size = r->size;
    a->rev_damp = r->damp;
}

/* a fresh generator for the item: the piece's own tempo, key, percussion and evolution; the player's chaos
 * knobs (and the parts they move) and memory stay as they were */
static void load_item(tuna_app *a, int item)
{
    tg_gen *g = &a->g;
    float c[4] = {g->chaos_rhythm, g->chaos_pitch, g->chaos_density, g->chaos_macro};
    unsigned parts = g->chaos_parts;
    int st = ta_lib_style(item);
    if (st >= 0) {
        tg_init(g, a->seed, a->sample_rate);
        tg_apply_style(g, st);
    } else {
        tp_character_apply(g, item, a->seed);   /* (tg_init inside) */
    }
    tg_set_chaos(g, c[0], c[1], c[2], c[3]);
    tg_set_chaos_parts(g, parts);
    g->x.deja_vu = deja_value(a);
    tg_reseed(g, a->seed);
    ta_set_reverb_type(a, ta_piece_reverb(st));   /* (its reverb too; the send is the player's) */
    a->item = item;
    a->req_on = false;
    a->req.meter = a->req.mel = a->req.bass = a->req.pad = a->req.harm = a->req.scale = -1;
}

void ta_init(tuna_app *a, float sample_rate, uint32_t seed)
{
    memset(a, 0, sizeof *a);
    a->sample_rate = sample_rate;
    a->seed = seed % TA_SEEDS;
    a->rnd = seed * 2654435761u + 0x6d2b79f5u;
    a->pending = -1;
    a->seed_req = -1;
    a->req.meter = a->req.mel = a->req.bass = a->req.pad = a->req.harm = a->req.scale = -1;
    a->deja = TA_DV_STYLE;
    for (int t = 0; t < TP_TRACKS; ++t)
        a->level[t] = 80;
    a->reverb = 30;
    ta_set_reverb_type(a, TG_REV_ROOM);
    a->local = true;
    tmi_init(&a->midi, sample_rate);
    tr_init(&a->reso, sample_rate);
    a->reso_decay = 50;
    a->reso_bright = 50;
    a->reso_mode = TR_TUNE_CHORD;
    tg_init(&a->g, seed, sample_rate);
    int home = 0;                                /* the first piece: TUNA's GARDEN (no strcmp in the firmware) */
    for (int i = 0; i < ta_lib_count(); ++i) {
        const char *n = ta_lib_name(i), *w = "GARDEN";
        while (*n && *n == *w) n++, w++;
        if (ta_lib_group(i) == 0 && *n == *w)
            home = i;
    }
    load_item(a, home);
}

void ta_set_sound(tuna_app *a, void (*fn)(void *ctx, const tg_event *e), void *ctx)
{
    a->sound = fn;
    a->sound_ctx = ctx;
}

void ta_set_store(tuna_app *a, const tp_store *s) { a->store = *s; }

/* every sounding note off, through the sound */
static void all_off(tuna_app *a) { tg_all_off(&a->g, on_event, a); }

void ta_stir(tuna_app *a, uint32_t v) { a->rnd = (a->rnd ^ v) * 16777619u + 1u; }

/* a new piece's seed, one of 256: drawn (xorshift, stirred by the host and the time played) */
static uint32_t new_seed(tuna_app *a)
{
    uint32_t x = a->rnd ^ (a->frame * 2246822519u);
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    a->rnd = x ? x : 1u;
    return (a->rnd >> 13) % TA_SEEDS;
}

/* another piece: with a new seed (a seed means nothing across pieces; this way each visit plays something
 * new); playing, both at the next half of the period */
void ta_select(tuna_app *a, int item)
{
    if (item < 0 || item >= ta_lib_count())
        return;
    if (a->playing) {
        if (item == a->item) {                    /* (turned back: nothing changes) */
            a->pending = -1;
            a->seed_req = -1;
            return;
        }
        a->pending = item;
        a->seed_req = (int)new_seed(a);
        return;
    }
    a->pending = -1;
    a->seed_req = -1;
    a->seed = new_seed(a);
    load_item(a, item);
}

void ta_set_chaos_parts(tuna_app *a, unsigned mask) { tg_set_chaos_parts(&a->g, mask); }

void ta_set_tempo(tuna_app *a, float bpm) { a->g.bpm = bpm; }
void ta_set_root(tuna_app *a, int pc) { tg_set_root(&a->g, ((pc % 12) + 12) % 12); }
void ta_set_percussion(tuna_app *a, int mode) { tg_set_percussion(&a->g, mode); }
void ta_set_evolve(tuna_app *a, bool on, int periods, float depth, bool modulate)
{
    tg_evolve(&a->g, on, periods, depth, modulate);
}

void ta_play(tuna_app *a, bool on)
{
    if (on == a->playing)
        return;
    if (on) {
        if (a->pending >= 0) {
            load_item(a, a->pending);
            a->pending = -1;
        } else if (a->req_on) {
            apply_req(a);
        } else {
            tg_reseed(&a->g, a->seed);           /* from the top: the same seed, the same music */
        }
        a->blocks = 0;
        tmi_start(&a->midi, a->frame);
    } else {
        all_off(a);
        tmi_stop(&a->midi, a->frame);
        if (a->pending >= 0) {
            load_item(a, a->pending);
            a->pending = -1;
        } else if (a->req_on) {
            apply_req(a);
        }
    }
    a->playing = on;
}

/* 256 seeds (a full turn of steps); stopped: at once; playing: at the next half of the period */
void ta_seed_step(tuna_app *a, int delta)
{
    uint32_t from = a->seed_req >= 0 ? (uint32_t)a->seed_req : a->seed;
    ta_set_seed(a, (from + (uint32_t)(delta % TA_SEEDS) + TA_SEEDS) % TA_SEEDS);
}

void ta_set_seed(tuna_app *a, uint32_t seed)
{
    seed %= TA_SEEDS;
    if (a->playing) {
        a->seed_req = seed == a->seed ? -1 : (int)seed;
        return;
    }
    a->seed_req = -1;
    a->seed = seed;
    tg_reseed(&a->g, a->seed);
}

void ta_set_deja(tuna_app *a, int mode)
{
    a->deja = mode < 0 ? 0 : mode >= TA_DV_COUNT ? TA_DV_COUNT - 1 : mode;
    a->g.x.deja_vu = deja_value(a);
}

/* ---- form settings --------------------------------------------------------------------------------- */
static const char *const METER_NAMES[TA_M_COUNT] = {"4/4", "3/4", "2/4", "5/4", "6/8", "7/8", "3+3+2"};
const char *ta_meter_name(int m) { return m >= 0 && m < TA_M_COUNT ? METER_NAMES[m] : "OWN"; }

int ta_meter_now(const tg_gen *g)
{
    if (g->meter[0]) {
        if (g->meter[0] == 3 && g->meter[1] == 3 && g->meter[2] == 2 && !g->meter[3]) return TA_M_332;
        if (g->meter[0] == 2 && g->meter[1] == 2 && g->meter[2] == 3 && !g->meter[3]) return TA_M_78;
        return -1;
    }
    if (g->division == 3 && g->beats_per_bar == 2) return TA_M_68;
    switch (g->beats_per_bar) {
    case 4: return TA_M_44;
    case 3: return TA_M_34;
    case 2: return TA_M_24;
    case 5: return TA_M_54;
    default: return -1;
    }
}

/* a meter over the composer's music: the beats, the grid (6/8 in threes, the grouped meters in eighths) and
 * the chords (as often as the composer changes them, at most a bar; a grouped meter: a chord a bar) */
static void apply_meter(tg_gen *g, int m)
{
    int bar_old = tg_meter_cycle(g, NULL), chord_bars_x2 = bar_old > 0 ? 2 * g->chord_ticks / bar_old : 2;
    static const uint8_t g78[3] = {2, 2, 3}, g332[3] = {3, 3, 2};
    for (int i = 0; i < TG_METER_MAX; ++i)
        g->meter[i] = 0;
    g->meter_bars = 0;
    switch (m) {
    case TA_M_44: g->beats_per_bar = 4; break;
    case TA_M_34: g->beats_per_bar = 3; break;
    case TA_M_24: g->beats_per_bar = 2; break;
    case TA_M_54: g->beats_per_bar = 5; break;
    case TA_M_68: g->beats_per_bar = 2; g->division = 3; break;
    case TA_M_78: case TA_M_332:
        g->division = 2;
        g->beats_per_bar = 3;
        for (int i = 0; i < 3; ++i)
            g->meter[i] = m == TA_M_78 ? g78[i] : g332[i];
        break;
    default: return;
    }
    int bar = tg_meter_cycle(g, NULL);
    if (g->meter[0])
        g->chord_ticks = bar;
    else
        g->chord_ticks = chord_bars_x2 >= 2 ? bar : bar / 2 > 0 ? bar / 2 : bar;   /* a bar, or half a bar */
}

static void apply_req(tuna_app *a)
{
    tg_gen *g = &a->g;
    const ta_edit *e = &a->req;
    if (e->meter >= 0) apply_meter(g, e->meter);
    if (e->mel >= 0) g->mel_mode = e->mel;
    if (e->bass >= 0) g->bass_mode = e->bass;
    if (e->pad >= 0) g->pad_mode = e->pad;
    if (e->harm >= 0) g->harmony = e->harm;
    if (e->scale >= 0) {
        g->scale = e->scale;
        g->home_scale = g->base_scale = e->scale;
    }
    a->req_on = false;
    a->req.meter = a->req.mel = a->req.bass = a->req.pad = a->req.harm = a->req.scale = -1;
    tg_reseed(g, a->seed);                       /* the piece starts over in them */
}

void ta_edit_set(tuna_app *a, const ta_edit *e)
{
    a->req = *e;
    a->req_on = true;
    if (!a->playing)
        apply_req(a);
}

void ta_edit_get(const tuna_app *a, ta_edit *e)
{
    const tg_gen *g = &a->g;
    e->meter = (int8_t)(a->req_on && a->req.meter >= 0 ? a->req.meter : ta_meter_now(g));
    e->mel = (int8_t)(a->req_on && a->req.mel >= 0 ? a->req.mel : g->mel_mode);
    e->bass = (int8_t)(a->req_on && a->req.bass >= 0 ? a->req.bass : g->bass_mode);
    e->pad = (int8_t)(a->req_on && a->req.pad >= 0 ? a->req.pad : g->pad_mode);
    e->harm = (int8_t)(a->req_on && a->req.harm >= 0 ? a->req.harm : g->harmony);
    e->scale = (int8_t)(a->req_on && a->req.scale >= 0 ? a->req.scale : g->scale);
}

void ta_set_chaos(tuna_app *a, float rhythm, float pitch, float density, float macro)
{
    tg_set_chaos(&a->g, rhythm, pitch, density, macro);
}

/* the resonator follows the harmony: retuned when the chord, the key or the mode changes */
static void retune(tuna_app *a)
{
    const tg_gen *g = &a->g;
    uint32_t sig = (uint32_t)g->root * 7u + (uint32_t)g->chord.root * 131u + (uint32_t)a->reso_mode * 3331u;
    for (int k = 0; k < g->chord.n; ++k)
        sig = sig * 31u + (uint32_t)(g->chord.tone[k] + 1);
    if (sig == a->reso_sig)
        return;
    a->reso_sig = sig;
    int notes[TR_STRINGS];
    int n = tr_notes_for(a->reso_mode, g->root, g->chord.root, g->chord.tone, g->chord.n, notes);
    tr_tune(&a->reso, notes, n);
}

void ta_midi_set(tuna_app *a, bool on, bool clock, bool bend)
{
    if (a->midi.on && !on)
        tmi_panic(&a->midi, a->frame);           /* nothing left hanging on the other side */
    a->midi.on = on;
    a->midi.clock = clock;
    a->midi.bend = bend;
}

void ta_reso_set(tuna_app *a, int amount, int decay, int bright, int mode)
{
    a->reso_amount = (uint8_t)(amount < 0 ? 0 : amount > 100 ? 100 : amount);
    a->reso_decay = (uint8_t)(decay < 0 ? 0 : decay > 100 ? 100 : decay);
    a->reso_bright = (uint8_t)(bright < 0 ? 0 : bright > 100 ? 100 : bright);
    a->reso_mode = (uint8_t)(mode < 0 ? 0 : mode >= TR_TUNE_COUNT ? TR_TUNE_COUNT - 1 : mode);
    tr_set(&a->reso, a->reso_amount / 100.0f, a->reso_decay / 100.0f, a->reso_bright / 100.0f);
    a->reso_sig = 0;
    retune(a);
}

/* a piece, a seed or a form chosen while playing: in at the boundary. With keep_clock the clock's phase
 * carries over, so the new piece's first tick falls where the old one's next tick would have: on the beat */
static void apply_changes(tuna_app *a, bool keep_clock)
{
    float phase = a->g.t.master_phase, prev = a->g.prev_master;
    all_off(a);
    if (a->seed_req >= 0) {
        a->seed = (uint32_t)a->seed_req;
        a->seed_req = -1;
        if (a->pending < 0 && !a->req_on)
            tg_reseed(&a->g, a->seed);            /* (with a new piece: its seed too) */
    }
    if (a->pending >= 0) {
        load_item(a, a->pending);                 /* (a new piece: the form settings asked for go) */
        a->pending = -1;
        a->req_on = false;
    } else if (a->req_on) {
        apply_req(a);
    }
    if (keep_clock && a->g.ticks == 0) {          /* (started over: the old tick's phase) */
        a->g.t.master_phase = phase;
        a->g.prev_master = prev;
    }
}

/* samples before the generator's next tick (its clock's ramp wraps), or -1 */
static int samples_to_tick(const tg_gen *g)
{
    float f = g->t.rate_hz * g->t.one_hertz * g->t.jitter_multiplier;
    if (!(f > 0.0f) || g->t.master_phase > 1.0f)
        return -1;
    return (int)((1.0f - g->t.master_phase) / f) + 1;   /* the wrap comes on this sample (1 = the next) */
}

void ta_process(tuna_app *a, int frames)
{
    while (a->playing && frames > 0) {
        int n = frames > TG_MAX_BLOCK ? TG_MAX_BLOCK : frames;
        /* a piece, a seed or a form chosen while playing comes in at the next half of the period: where it
         * begins (on the cadence) or halfway (on the half cadence). It goes in just before that tick, so
         * nothing of the old one sounds there and the beat goes on; if the tick came first, just after it */
        if (a->pending >= 0 || a->req_on || a->seed_req >= 0) {
            if (a->g.ticks == 0) {
                apply_changes(a, true);           /* (nothing played yet) */
            } else {
                int p, d = tg_period_position(&a->g, &p), h = tg_half_position(&a->g), next = (d + 1) % p;
                int k = next == 0 || next == h ? samples_to_tick(&a->g) : -1;
                if (k == 1)
                    apply_changes(a, true);
                else if (k > 1 && k <= n)
                    n = k - 1;                    /* up to the sample before the tick */
                else if (d == 0 || d == h)
                    apply_changes(a, false);
            }
        }
        tmi_clock(&a->midi, a->frame, n, a->g.bpm);
        tg_process(&a->g, n, on_event, a);
        a->blocks++;
        a->frame += (uint32_t)n;
        frames -= n;
        retune(a);
    }
}

/* ---- the slots ------------------------------------------------------------------------------------------- */
static uint8_t rec_buf[TP_RECORD_MAX];

static bool same_name(const char *a, const char *b)
{
    int i = 0;
    for (; i < TP_NAME && a[i] && a[i] == b[i]; ++i)
        ;
    return i == TP_NAME || a[i] == b[i];
}

int ta_slot_save(tuna_app *a, int slot)
{
    tp_host h;
    memset(&h, 0, sizeof h);
    const char *nm = ta_lib_name(a->item);
    const char *src = nm[0] ? nm : "TUNA";
    for (int i = 0; i < TP_NAME && src[i]; ++i)
        h.name[i] = src[i];                       /* (h zeroed: the name ends) */
    h.sounds_set = false;                         /* the composer's own instruments; the mix: */
    for (int t = 0; t < TP_TRACKS; ++t) {
        h.sound[t].level = a->level[t];
        h.sound[t].rev = a->reverb;
    }
    h.reso[0] = a->reso_amount;
    h.reso[1] = a->reso_decay;
    h.reso[2] = a->reso_bright;
    h.reso[3] = a->reso_mode;
    h.reverb[0] = a->rev_size;
    h.reverb[1] = a->rev_damp;
    h.reverb[2] = a->rev_type;
    int n = tp_encode(&a->g, &h, rec_buf, sizeof rec_buf);
    if (n < 0)
        return n;
    return a->store.save ? tp_slot_save(&a->store, slot, rec_buf, n) : TP_ERR_STORE;
}

int ta_slot_load(tuna_app *a, int slot)
{
    if (!a->store.load)
        return TP_ERR_STORE;
    int n = tp_slot_load(&a->store, slot, rec_buf, sizeof rec_buf);
    if (n < 0)
        return n;
    tp_host h;
    bool was = a->playing;
    if (was)
        all_off(a);
    a->g.sample_rate = a->sample_rate;
    int r = tp_decode(&a->g, &h, rec_buf, n);
    if (r != TP_OK)
        return r;
    a->seed = a->g.seed;
    a->pending = -1;
    int st = a->g.style ? tg_style_index(a->g.style) : -1;
    if (st >= 0) {
        a->item = item_of_style(st);
    } else {                                      /* a character preset: found by its name */
        a->item = -1;
        for (int i = 0; i < tp_character_count(); ++i)
            if (same_name(h.name, tp_character_name(i)))
                a->item = i;
    }
    float dv = a->g.x.deja_vu;
    a->deja = dv == 0.0f ? TA_DV_OFF : dv == 0.5f && !(a->g.style && a->g.style->motif == 0.5f) ? TA_DV_LOCK
                                                                                                 : TA_DV_STYLE;
    for (int t = 0; t < TP_TRACKS; ++t)
        a->level[t] = h.sound[t].level;
    a->reverb = h.sound[0].rev;
    if (h.reverb[0] <= 100 && h.reverb[1] <= 100 && h.reverb[2] < TG_REV_COUNT) {
        a->rev_type = h.reverb[2];
        a->rev_size = h.reverb[0];
        a->rev_damp = h.reverb[1];
    } else {                                      /* (an older record: the piece's own) */
        ta_set_reverb_type(a, ta_piece_reverb(st));
    }
    ta_reso_set(a, h.reso[0], h.reso[1], h.reso[2], h.reso[3]);
    a->blocks = 0;
    return TP_OK;
}

uint16_t ta_slots_used(tuna_app *a, char names[TP_SLOTS][TP_NAME + 1])
{
    return a->store.load ? tp_slots_used(&a->store, names) : 0;
}
