/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA presets (see tuna_preset.h). */
#include "tuna_preset.h"

/* ---- the byte stream: one field list serves writing and reading ----------------------------------------
 * Little-endian, fixed sizes; floats as their IEEE bits. Record layout:
 *   "TUNA" version(1) flags(1) length(2)  name(12)  fields...  crc32(4)
 * where length counts everything before the CRC and the CRC covers it all. */
typedef struct {
    uint8_t *buf;
    int pos, max;
    bool write, err;
    int version;                          /* the record's (reading); TP_VERSION (writing) */
} tp_io;

static void io_byte(tp_io *io, uint8_t *v)
{
    if (io->pos >= io->max) {
        io->err = true;
        return;
    }
    if (io->write)
        io->buf[io->pos] = *v;
    else
        *v = io->buf[io->pos];
    io->pos++;
}

static void io_u32(tp_io *io, uint32_t *v)
{
    uint8_t b[4] = {(uint8_t)*v, (uint8_t)(*v >> 8), (uint8_t)(*v >> 16), (uint8_t)(*v >> 24)};
    for (int i = 0; i < 4; ++i)
        io_byte(io, &b[i]);
    if (!io->write)
        *v = (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
}

static void io_u16(tp_io *io, uint16_t *v)
{
    uint8_t b[2] = {(uint8_t)*v, (uint8_t)(*v >> 8)};
    io_byte(io, &b[0]);
    io_byte(io, &b[1]);
    if (!io->write)
        *v = (uint16_t)(b[0] | b[1] << 8);
}

static void io_f32(tp_io *io, float *v)
{
    union { float f; uint32_t u; } x;
    x.f = *v;
    io_u32(io, &x.u);
    if (!io->write)
        *v = x.f;
}

/* an int stored in one byte, signed or not */
static void io_int8(tp_io *io, int *v, bool is_signed)
{
    uint8_t b = (uint8_t)*v;
    io_byte(io, &b);
    if (!io->write)
        *v = is_signed ? (int)(int8_t)b : (int)b;
}
#define IO_U8(io, x)  io_int8((io), &(x), false)
#define IO_S8(io, x)  io_int8((io), &(x), true)

static void io_int16(tp_io *io, int *v)
{
    uint16_t u = (uint16_t)*v;
    io_u16(io, &u);
    if (!io->write)
        *v = (int)(int16_t)u;
}

static void io_bool(tp_io *io, bool *v)
{
    uint8_t b = *v ? 1 : 0;
    io_byte(io, &b);
    if (!io->write)
        *v = b != 0;
}

static uint32_t crc32(const uint8_t *p, int n)        /* zlib CRC-32, as Felucca's storage */
{
    static const uint32_t T[16] = {
        0x00000000u, 0x1DB71064u, 0x3B6E20C8u, 0x26D930ACu, 0x76DC4190u, 0x6B6B51F4u, 0x4DB26158u, 0x5005713Cu,
        0xEDB88320u, 0xF00F9344u, 0xD6D6A3E8u, 0xCB61B38Cu, 0x9B64C2B0u, 0x86D3D2D4u, 0xA00AE278u, 0xBDBDF21Cu};
    uint32_t c = 0xFFFFFFFFu;
    while (n-- > 0) {
        c ^= *p++;
        c = (c >> 4) ^ T[c & 15u];
        c = (c >> 4) ^ T[c & 15u];
    }
    return ~c;
}

/* ---- the generator's settings ---------------------------------------------------------------------------
 * The values saved are the settings, not their moment: while the evolution or a contrast period has moved
 * the key, the tempo or the density, the record keeps the values they return to. */
typedef struct {
    int style;                            /* TG_STYLE_*, -1: none */
    uint32_t seed;
    bool evolve, evolve_modulate;
    int evolve_periods;
    float evolve_depth;
    bool frozen;
} tp_head;

static void settings(tp_io *io, tg_gen *g, tp_head *h)
{
    const tg_style *st = g->style;
    bool alt = io->write && st && g->alt_on;
    /* the values that evolution, modulation and contrast move: their resting values when writing */
    int root = g->root, scale = g->scale;
    float bpm = g->bpm, density = g->melody_density;
    int mel_mode = g->mel_mode, bass_mode = g->bass_mode, pad_mode = g->pad_mode, harmony = g->harmony;
    uint8_t perc[TG_PI_COUNT];
    for (int i = 0; i < TG_PI_COUNT; ++i)
        perc[i] = g->perc_w[i];
    if (io->write) {
        if (g->evolve) {
            root = g->base_root;
            scale = g->base_scale;
            density = g->base_density;
            for (int i = 0; i < TG_PI_COUNT; ++i)
                perc[i] = g->base_perc[i];
        } else if (st && (g->style_away || g->key_pending)) {
            root = g->home_root;
            scale = g->home_scale;
        }
        if (alt) {
            if (st->alt.bpm > 0.0f) bpm /= st->alt.bpm;
            if (st->alt.density > 0.0f) density /= st->alt.density;
            mel_mode = st->mel_mode;
            bass_mode = st->bass_mode;
            pad_mode = st->pad_mode;
            harmony = st->harmony;
        }
    }
    IO_U8(io, root);
    IO_U8(io, scale);
    io_f32(io, &bpm);
    io_f32(io, &density);
    IO_U8(io, mel_mode);
    IO_U8(io, bass_mode);
    IO_U8(io, pad_mode);
    IO_U8(io, harmony);
    for (int i = 0; i < TG_PI_COUNT; ++i)
        io_byte(io, &perc[i]);
    if (!io->write) {
        g->root = root;
        g->scale = scale;
        g->bpm = bpm;
        g->melody_density = density;
        g->mel_mode = mel_mode;
        g->bass_mode = bass_mode;
        g->pad_mode = pad_mode;
        g->harmony = harmony;
        for (int i = 0; i < TG_PI_COUNT; ++i)
            g->perc_w[i] = perc[i];
    }
    /* meter and time */
    IO_U8(io, g->division);
    IO_U8(io, g->beats_per_bar);
    for (int i = 0; i < TG_METER_MAX; ++i)
        io_byte(io, &g->meter[i]);
    io_u16(io, &g->meter_bars);
    io_f32(io, &g->swing);
    io_f32(io, &g->lilt);
    IO_U8(io, g->prog);
    io_int16(io, &g->chord_ticks);
    for (int p = 0; p < TG_PARTS; ++p)
        IO_S8(io, g->base_note[p]);
    io_f32(io, &g->min_gap_s);
    io_bool(io, &g->harmonize);
    IO_U8(io, g->melody_range);
    IO_U8(io, g->phrase_bars);
    IO_U8(io, g->pad_tones);
    io_bool(io, &g->pad_restrike);
    /* melody */
    io_f32(io, &g->appoggiatura);
    io_f32(io, &g->suspension);
    io_bool(io, &g->trill);
    for (int i = 0; i < 4; ++i)
        io_f32(io, &g->mel_profile[i]);
    IO_U8(io, g->max_leap);
    io_u16(io, &g->melody_mask);
    io_u16(io, &g->mel_add);
    io_f32(io, &g->syncopation);
    io_bool(io, &g->chromatic);
    IO_U8(io, g->arrival_bars);
    io_bool(io, &g->plagal);
    io_f32(io, &g->whole_tone);
    IO_U8(io, g->motif_beats);
    io_f32(io, &g->motif_use);
    io_f32(io, &g->var_invert);
    io_f32(io, &g->var_alter);
    io_bool(io, &g->hemiola);
    io_f32(io, &g->sforzando);
    io_f32(io, &g->swell);
    io_f32(io, &g->rubato);
    IO_U8(io, g->mel_min_ticks);
    io_f32(io, &g->direction);
    io_f32(io, &g->staccato);
    io_f32(io, &g->turn);
    io_f32(io, &g->octave);
    /* accompaniment, form, sound */
    IO_S8(io, g->pad_interval);
    IO_U8(io, g->poly_semis);
    io_bool(io, &g->spread);
    IO_U8(io, g->strum_ms);
    io_f32(io, &g->gp);
    io_f32(io, &g->echo);
    IO_U8(io, g->tuning);
    IO_U8(io, g->tune_root);
    IO_U8(io, g->mod_kind);
    IO_U8(io, g->perc_mode);
    io_f32(io, &g->humanize);
    /* Marbles */
    IO_U8(io, g->t.model);
    IO_U8(io, g->t.range);
    io_f32(io, &g->t.rate);
    io_f32(io, &g->t.bias);
    io_f32(io, &g->t.jitter);
    io_f32(io, &g->t.pulse_width_mean);
    io_f32(io, &g->t.pulse_width_std);
    io_f32(io, &g->t.sequence.deja_vu);
    IO_U8(io, g->t.sequence.length);
    tm_group *grp[2] = {&g->x, &g->y};
    for (int k = 0; k < 2; ++k) {
        IO_U8(io, grp[k]->control_mode);
        IO_U8(io, grp[k]->voltage_range);
        io_f32(io, &grp[k]->spread);
        io_f32(io, &grp[k]->bias);
        io_f32(io, &grp[k]->steps);
        io_f32(io, &grp[k]->deja_vu);
        IO_U8(io, grp[k]->length);
        IO_U8(io, grp[k]->ratio.p);
        IO_U8(io, grp[k]->ratio.q);
    }
    IO_U8(io, g->clock_source);
    /* the chaos knobs */
    io_f32(io, &g->chaos_rhythm);
    io_f32(io, &g->chaos_pitch);
    io_f32(io, &g->chaos_density);
    io_f32(io, &g->chaos_macro);
    /* version 2: which parts follow them (version 1: the melody alone, as it was then) */
    int parts = g->chaos_parts;
    if (io->version >= 2)
        IO_U8(io, parts);
    else
        parts = 1 << TG_PART_MELODY;
    if (!io->write)
        tg_set_chaos_parts(g, (unsigned)parts);
    (void)h;
}

/* the sequencer overlay and the frozen pattern */
static void sequencer(tp_io *io, tg_gen *g, tp_head *h)
{
    int n = g->nsteps;
    IO_U8(io, n);
    if (!io->write) {
        if (n > TG_STEPS) { io->err = true; return; }
        g->nsteps = n;
    }
    for (int i = 0; i < n && !io->err; ++i) {
        tg_step *s = &g->steps[i];
        int part = s->part, cond = s->cond, ca = s->ca, cb = s->cb, nl = s->nlock;
        io_u16(io, &s->tick);
        IO_U8(io, part);
        IO_U8(io, cond);
        IO_U8(io, ca);
        IO_U8(io, cb);
        IO_U8(io, nl);
        if (!io->write) {
            if (nl > TG_STEP_LOCKS || part >= TG_EVENT_PARTS || cond >= TG_C_COUNT) { io->err = true; return; }
            s->part = (uint8_t)part; s->cond = (uint8_t)cond; s->ca = (uint8_t)ca; s->cb = (uint8_t)cb; s->nlock = (uint8_t)nl;
        }
        for (int k = 0; k < nl; ++k) {
            int prm = s->lock[k].param, val = s->lock[k].value;
            IO_U8(io, prm);
            io_int16(io, &val);
            if (!io->write) {
                if (prm >= TG_LK_COUNT) { io->err = true; return; }
                s->lock[k].param = (uint8_t)prm;
                s->lock[k].value = (int16_t)val;
            }
        }
    }
    io_bool(io, &h->frozen);
    if (!h->frozen)
        return;
    tg_pattern *pt = &g->pat;
    int root = pt->root, scale = pt->scale;
    io_u16(io, &pt->len);
    io_u16(io, &pt->n);
    IO_S8(io, root);
    IO_S8(io, scale);
    if (!io->write) {
        if (pt->n > TG_PAT_MAX || pt->len == 0) { io->err = true; return; }
        pt->root = (int8_t)root;
        pt->scale = (int8_t)scale;
    }
    if (io->write && pt->len >= 8192) { io->err = true; return; }   /* never: a period is far shorter */
    for (int i = 0; i < pt->n && !io->err; ++i) {
        tg_pnote *x = &pt->ev[i];
        uint16_t tp = (uint16_t)(x->tick | x->part << 13);   /* 6 bytes a note: the part beside the tick */
        io_u16(io, &tp);
        io_u16(io, &x->len);
        io_byte(io, &x->note);
        io_byte(io, &x->vel);
        if (!io->write) {
            x->tick = tp & 0x1FFF;
            x->part = (uint8_t)(tp >> 13);
            x->pad = 0;
            if (x->part >= TG_EVENT_PARTS || x->note > 127 || x->tick >= pt->len) { io->err = true; return; }
        }
    }
}

static void host_part(tp_io *io, tp_host *hp)
{
    tp_host dummy = {0};
    if (!hp) hp = &dummy;
    io_bool(io, &hp->sounds_set);
    for (int t = 0; t < TP_TRACKS; ++t) {
        tp_sound *s = &hp->sound[t];
        io_byte(io, &s->engine);
        io_byte(io, &s->preset);
        io_byte(io, &s->level);
        io_byte(io, &s->pan);
        io_byte(io, &s->rev);
    }
    for (int i = 0; i < 4; ++i)
        io_byte(io, &hp->reso[i]);
}

/* ---- encode / decode ------------------------------------------------------------------------------------ */
int tp_encode(const tg_gen *gc, const tp_host *host, uint8_t *buf, int max)
{
    tg_gen *g = (tg_gen *)gc;                     /* written only when reading: never here */
    tp_io io = {buf, 0, max - 4, true, false, TP_VERSION};
    tp_head h;
    h.style = g->style ? tg_style_index(g->style) : -1;
    h.seed = g->seed;
    h.evolve = g->evolve;
    h.evolve_modulate = g->evolve_modulate;
    h.evolve_periods = g->evolve_periods;
    h.evolve_depth = g->evolve_depth;
    h.frozen = g->frozen;
    static const uint8_t magic[4] = {'T', 'U', 'N', 'A'};
    for (int i = 0; i < 4; ++i) {
        uint8_t m = magic[i];
        io_byte(&io, &m);
    }
    uint8_t version = TP_VERSION, flags = 0;
    uint16_t len = 0;
    io_byte(&io, &version);
    io_byte(&io, &flags);
    io_u16(&io, &len);                            /* filled in below */
    tp_host blank = {{0}, false, {{0}}, {0}};
    const tp_host *hp = host ? host : &blank;
    for (int i = 0; i < TP_NAME; ++i) {
        uint8_t c = (uint8_t)hp->name[i];
        io_byte(&io, &c);
    }
    io_int16(&io, &h.style);
    io_u32(&io, &h.seed);
    io_bool(&io, &h.evolve);
    io_bool(&io, &h.evolve_modulate);
    IO_U8(&io, h.evolve_periods);
    io_f32(&io, &h.evolve_depth);
    settings(&io, g, &h);
    sequencer(&io, g, &h);
    host_part(&io, (tp_host *)hp);
    if (io.err)
        return TP_ERR_SPACE;
    int n = io.pos;
    buf[6] = (uint8_t)n;
    buf[7] = (uint8_t)(n >> 8);
    uint32_t c = crc32(buf, n);
    buf[n] = (uint8_t)c;
    buf[n + 1] = (uint8_t)(c >> 8);
    buf[n + 2] = (uint8_t)(c >> 16);
    buf[n + 3] = (uint8_t)(c >> 24);
    return n + 4;
}

static int check_record(const uint8_t *buf, int n)
{
    if (n < 24 || buf[0] != 'T' || buf[1] != 'U' || buf[2] != 'N' || buf[3] != 'A')
        return TP_ERR_FORMAT;
    if (buf[4] > TP_VERSION)
        return TP_ERR_VERSION;
    int len = buf[6] | buf[7] << 8;
    if (len + 4 > n || len < 20)
        return TP_ERR_FORMAT;
    uint32_t c = (uint32_t)buf[len] | (uint32_t)buf[len + 1] << 8 | (uint32_t)buf[len + 2] << 16 |
                 (uint32_t)buf[len + 3] << 24;
    if (c != crc32(buf, len))
        return TP_ERR_FORMAT;
    return len;
}

int tp_peek_name(const uint8_t *buf, int n, char *name)
{
    int len = check_record(buf, n);
    if (len < 0)
        return len;
    for (int i = 0; i < TP_NAME; ++i)
        name[i] = (char)buf[8 + i];
    name[TP_NAME] = 0;
    return TP_OK;
}

int tp_decode(tg_gen *g, tp_host *host, const uint8_t *buf, int n)
{
    int len = check_record(buf, n);
    if (len < 0)
        return len;
    tp_io io = {(uint8_t *)buf, 8 + TP_NAME, len, false, false, buf[4]};
    tp_head h = {0};
    io_int16(&io, &h.style);
    io_u32(&io, &h.seed);
    io_bool(&io, &h.evolve);
    io_bool(&io, &h.evolve_modulate);
    IO_U8(&io, h.evolve_periods);
    io_f32(&io, &h.evolve_depth);
    if (io.err || h.style < -1 || h.style >= TG_STYLE_COUNT)
        return TP_ERR_FORMAT;
    /* a fresh generator: the composer, then every setting over it, then the seed */
    float sr = g->sample_rate;
    tg_init(g, h.seed, sr);
    if (h.style >= 0)
        tg_apply_style(g, h.style);
    g->evolve = false;                            /* switched on below, from the settings saved */
    settings(&io, g, &h);
    if (io.err)
        return TP_ERR_FORMAT;
    tg_reseed(g, h.seed);
    if (h.evolve)
        tg_evolve(g, true, h.evolve_periods, h.evolve_depth, h.evolve_modulate);
    sequencer(&io, g, &h);
    tp_host hp;
    host_part(&io, &hp);
    if (io.err)
        return TP_ERR_FORMAT;
    if (h.frozen) {
        g->frozen = true;
        g->pat_pos = 0;
        g->pat_loop = 0;
        g->play_n = 0;
        g->rec_valid = false;
    }
    if (host) {
        for (int i = 0; i < TP_NAME; ++i)
            hp.name[i] = (char)buf[8 + i];
        hp.name[TP_NAME] = 0;
        *host = hp;
    }
    return TP_OK;
}

/* ---- the slots ------------------------------------------------------------------------------------------
 * A group's object: "TBNK", then per slot it holds: slot(1) length(2) record. */
static uint8_t gbuf[TP_GROUP_MAX];

static int group_read(const tp_store *s, int group)
{
    int n = s->load(s->ctx, group, gbuf, TP_GROUP_MAX);
    if (n < 4 || gbuf[0] != 'T' || gbuf[1] != 'B' || gbuf[2] != 'N' || gbuf[3] != 'K')
        n = 4;                                    /* empty (or not ours): start afresh */
    gbuf[0] = 'T'; gbuf[1] = 'B'; gbuf[2] = 'N'; gbuf[3] = 'K';
    /* walk it: a damaged tail is dropped */
    int pos = 4;
    while (pos + 3 <= n) {
        int len = gbuf[pos + 1] | gbuf[pos + 2] << 8;
        if (gbuf[pos] >= TP_SLOTS || pos + 3 + len > n)
            break;
        pos += 3 + len;
    }
    return pos;
}

/* where a slot's entry is in the group buffer (its record at +3), -1 if absent */
static int group_find(int n, int slot, int *len)
{
    int pos = 4;
    while (pos + 3 <= n) {
        int l = gbuf[pos + 1] | gbuf[pos + 2] << 8;
        if (gbuf[pos] == slot) {
            *len = l;
            return pos;
        }
        pos += 3 + l;
    }
    return -1;
}

static int group_remove(int n, int slot)
{
    int len, at = group_find(n, slot, &len);
    if (at < 0)
        return n;
    int cut = 3 + len;
    for (int i = at; i + cut < n; ++i)
        gbuf[i] = gbuf[i + cut];
    return n - cut;
}

int tp_slot_save(const tp_store *s, int slot, const uint8_t *rec, int n)
{
    if (slot < 0 || slot >= TP_SLOTS || n <= 0 || !s)
        return TP_ERR_ARG;
    if (check_record(rec, n) < 0)
        return TP_ERR_FORMAT;
    int group = slot / TP_SLOTS_PER_GROUP;
    int used = group_remove(group_read(s, group), slot);
    if (used + 3 + n > TP_GROUP_MAX)
        return TP_ERR_SPACE;
    gbuf[used] = (uint8_t)slot;
    gbuf[used + 1] = (uint8_t)n;
    gbuf[used + 2] = (uint8_t)(n >> 8);
    for (int i = 0; i < n; ++i)
        gbuf[used + 3 + i] = rec[i];
    used += 3 + n;
    return s->save(s->ctx, group, gbuf, used) == 0 ? TP_OK : TP_ERR_STORE;
}

int tp_slot_load(const tp_store *s, int slot, uint8_t *rec, int max)
{
    if (slot < 0 || slot >= TP_SLOTS || !s)
        return TP_ERR_ARG;
    int n = group_read(s, slot / TP_SLOTS_PER_GROUP), len;
    int at = group_find(n, slot, &len);
    if (at < 0)
        return TP_ERR_EMPTY;
    if (len > max)
        return TP_ERR_SPACE;
    if (check_record(gbuf + at + 3, len) < 0)
        return TP_ERR_FORMAT;
    for (int i = 0; i < len; ++i)
        rec[i] = gbuf[at + 3 + i];
    return len;
}

int tp_slot_clear(const tp_store *s, int slot)
{
    if (slot < 0 || slot >= TP_SLOTS || !s)
        return TP_ERR_ARG;
    int group = slot / TP_SLOTS_PER_GROUP;
    int n = group_read(s, group), len;
    if (group_find(n, slot, &len) < 0)
        return TP_OK;
    n = group_remove(n, slot);
    return s->save(s->ctx, group, gbuf, n) == 0 ? TP_OK : TP_ERR_STORE;
}

uint16_t tp_slots_used(const tp_store *s, char names[TP_SLOTS][TP_NAME + 1])
{
    uint16_t used = 0;
    for (int group = 0; group < TP_GROUPS; ++group) {
        int n = group_read(s, group);
        for (int k = 0; k < TP_SLOTS_PER_GROUP; ++k) {
            int slot = group * TP_SLOTS_PER_GROUP + k, len;
            int at = group_find(n, slot, &len);
            if (at < 0 || check_record(gbuf + at + 3, len) < 0)
                continue;
            used |= (uint16_t)(1u << slot);
            if (names)
                tp_peek_name(gbuf + at + 3, len, names[slot]);
        }
    }
    return used;
}

int tp_group_free(const tp_store *s, int group)
{
    if (group < 0 || group >= TP_GROUPS)
        return 0;
    return TP_GROUP_MAX - group_read(s, group);
}

/* ---- the composer library by era ------------------------------------------------------------------------ */
static const char *const era_names[TP_ERA_COUNT] = {
    "RENAISSANCE", "BAROQUE", "CLASSICAL", "EARLY ROMANTIC", "ROMANTIC", "LATE ROMANTIC", "MODERN", "MINIMAL",
    "FILM & STAGE",
};

/* composers by era, in order of birth (year) */
typedef struct { uint8_t style; int16_t born; } tp_composer;
static const tp_composer era1[] = {
    {TG_STYLE_HILDEGARD, 1098}, {TG_STYLE_TALLIS, 1505}, {TG_STYLE_PALESTRINA, 1525}, {TG_STYLE_SWEELINCK, 1562},
    {TG_STYLE_MONTEVERDI, 1567}, {TG_STYLE_ALLEGRI, 1582}};
static const tp_composer era2[] = {
    {TG_STYLE_BUXTEHUDE, 1637}, {TG_STYLE_PACHELBEL, 1653}, {TG_STYLE_PURCELL, 1659}, {TG_STYLE_ALBINONI, 1671},
    {TG_STYLE_VIVALDI, 1678}, {TG_STYLE_RAMEAU, 1683}, {TG_STYLE_BACH, 1685}, {TG_STYLE_HANDEL, 1685},
    {TG_STYLE_SCARLATTI, 1685}, {TG_STYLE_PERGOLESI, 1710}};
static const tp_composer era3[] = {
    {TG_STYLE_GLUCK, 1714}, {TG_STYLE_HAYDN, 1732}, {TG_STYLE_MOZART, 1756}, {TG_STYLE_BEETHOVEN, 1770}};
static const tp_composer era4[] = {
    {TG_STYLE_ROSSINI, 1792}, {TG_STYLE_SCHUBERT, 1797}, {TG_STYLE_BERLIOZ, 1803}, {TG_STYLE_FANNY_MENDELSSOHN, 1805},
    {TG_STYLE_FELIX_MENDELSSOHN, 1809}, {TG_STYLE_CHOPIN, 1810}, {TG_STYLE_ROBERT_SCHUMANN, 1810},
    {TG_STYLE_LISZT, 1811}, {TG_STYLE_CLARA_SCHUMANN, 1819}};
static const tp_composer era5[] = {
    {TG_STYLE_WAGNER, 1813}, {TG_STYLE_VERDI, 1813}, {TG_STYLE_GOUNOD, 1818}, {TG_STYLE_FRANCK, 1822},
    {TG_STYLE_SMETANA, 1824}, {TG_STYLE_BRUCKNER, 1824}, {TG_STYLE_JOHANN_STRAUSS, 1825}, {TG_STYLE_BRAHMS, 1833},
    {TG_STYLE_SAINT_SAENS, 1835}, {TG_STYLE_BRUCH, 1838}, {TG_STYLE_BIZET, 1838}, {TG_STYLE_MUSSORGSKY, 1839},
    {TG_STYLE_TCHAIKOVSKY, 1840}, {TG_STYLE_DVORAK, 1841}, {TG_STYLE_MASSENET, 1842}, {TG_STYLE_GRIEG, 1843},
    {TG_STYLE_RIMSKY_KORSAKOV, 1844}};
static const tp_composer era6[] = {
    {TG_STYLE_WIDOR, 1844}, {TG_STYLE_FAURE, 1845}, {TG_STYLE_ELGAR, 1857}, {TG_STYLE_PUCCINI, 1858},
    {TG_STYLE_MAHLER, 1860}, {TG_STYLE_DEBUSSY, 1862}, {TG_STYLE_RICHARD_STRAUSS, 1864}, {TG_STYLE_SIBELIUS, 1865},
    {TG_STYLE_SATIE, 1866}, {TG_STYLE_VAUGHAN_WILLIAMS, 1872}, {TG_STYLE_RACHMANINOFF, 1873}, {TG_STYLE_HOLST, 1874},
    {TG_STYLE_RAVEL, 1875}};
static const tp_composer era7[] = {
    {TG_STYLE_BARTOK, 1881}, {TG_STYLE_STRAVINSKY, 1882}, {TG_STYLE_PROKOFIEV, 1891}, {TG_STYLE_ORFF, 1895},
    {TG_STYLE_KORNGOLD, 1897}, {TG_STYLE_GERSHWIN, 1898}, {TG_STYLE_POULENC, 1899}, {TG_STYLE_COPLAND, 1900},
    {TG_STYLE_RODRIGO, 1901}, {TG_STYLE_DURUFLE, 1902}, {TG_STYLE_KHACHATURIAN, 1903}, {TG_STYLE_SHOSTAKOVICH, 1906},
    {TG_STYLE_MESSIAEN, 1908}, {TG_STYLE_BARBER, 1910}, {TG_STYLE_BRITTEN, 1913}, {TG_STYLE_PIAZZOLLA, 1921}};
static const tp_composer era8[] = {
    {TG_STYLE_TEN_HOLT, 1923}, {TG_STYLE_GORECKI, 1933}, {TG_STYLE_PART, 1935}, {TG_STYLE_REICH, 1936},
    {TG_STYLE_GLASS, 1937}, {TG_STYLE_TAVENER, 1944}, {TG_STYLE_RUTTER, 1945}, {TG_STYLE_VASKS, 1946},
    {TG_STYLE_WHITACRE, 1970}};
static const tp_composer era9[] = {
    {TG_STYLE_MORRICONE, 1928}, {TG_STYLE_JOHN_WILLIAMS, 1932}, {TG_STYLE_HOWARD_SHORE, 1946},
    {TG_STYLE_LLOYD_WEBBER, 1948}, {TG_STYLE_PREISNER, 1955}};

#define N_OF(a) ((int)(sizeof a / sizeof a[0]))
static const struct { const tp_composer *c; int n; } eras[TP_ERA_COUNT] = {
    {era1, N_OF(era1)}, {era2, N_OF(era2)}, {era3, N_OF(era3)}, {era4, N_OF(era4)}, {era5, N_OF(era5)},
    {era6, N_OF(era6)}, {era7, N_OF(era7)}, {era8, N_OF(era8)}, {era9, N_OF(era9)},
};

const char *tp_era_name(int era) { return era >= 0 && era < TP_ERA_COUNT ? era_names[era] : "?"; }
int tp_era_count(int era) { return era >= 0 && era < TP_ERA_COUNT ? eras[era].n : 0; }
int tp_era_style(int era, int i) { return i >= 0 && i < tp_era_count(era) ? eras[era].c[i].style : -1; }

int tp_style_era(int style)
{
    for (int e = 0; e < TP_ERA_COUNT; ++e)
        for (int i = 0; i < eras[e].n; ++i)
            if (eras[e].c[i].style == style)
                return e;
    return -1;
}

int tp_style_born(int style)
{
    for (int e = 0; e < TP_ERA_COUNT; ++e)
        for (int i = 0; i < eras[e].n; ++i)
            if (eras[e].c[i].style == style)
                return eras[e].c[i].born;
    return 0;
}

/* ---- character presets ------------------------------------------------------------------------------------
 * Tuna's own generative music, no composer: Marbles' randomness shaped by the harmonizer, from still
 * and ambient to driving and rhythmic. */
typedef struct {
    const char *name;
    float bpm;
    uint8_t division, root, scale, prog, chord_ticks, t_model, pad_tones, pad_restrike, perc, x_control, bass_mode;
    float t_bias, t_jitter, t_dv, x_spread, x_steps, x_dv, density;
} tp_char;

static const tp_char characters[] = {
    /* name        bpm  div root scale        prog          chord model                        pad rs perc         xctl                 bass            bias  jit   dv    spread steps xdv   dens */
    {"STILL",       52, 2, 9,  TS_AEOLIAN,    TP_MODAL_VAMP, 32, TM_T_COMPLEMENTARY_BERNOULLI, 4, 0, TG_PERC_OFF,    TM_CONTROL_TILT,      TG_BASS_DRONE,  0.30f, 0.30f, 0.80f, 0.30f, 0.90f, 0.85f, 0.18f},
    {"GARDEN",      72, 2, 2,  TS_DORIAN,     TP_MODAL_VAMP, 16, TM_T_COMPLEMENTARY_BERNOULLI, 4, 0, TG_PERC_OFF,    TM_CONTROL_TILT,      TG_BASS_MARBLES, 0.50f, 0.15f, 0.60f, 0.40f, 0.80f, 0.70f, 0.35f},
    {"LYDIAN",      84, 2, 5,  TS_LYDIAN,     TP_MODAL_VAMP, 16, TM_T_CLUSTERS,                4, 0, TG_PERC_OFF,    TM_CONTROL_BUMP,      TG_BASS_MARBLES, 0.65f, 0.30f, 0.40f, 0.50f, 0.75f, 0.55f, 0.40f},
    {"DRIFT",       66, 2, 4,  TS_PENT_MINOR, TP_EPIC,       32, TM_T_COMPLEMENTARY_BERNOULLI, 3, 0, TG_PERC_OFF,    TM_CONTROL_IDENTICAL, TG_BASS_ROOT,   0.45f, 0.20f, 0.70f, 0.45f, 0.85f, 0.75f, 0.30f},
    {"FOLK",        96, 2, 7,  TS_IONIAN,     TP_FOLK,       16, TM_T_COMPLEMENTARY_BERNOULLI, 3, 0, TG_PERC_PERIOD, TM_CONTROL_IDENTICAL, TG_BASS_BAR,    0.50f, 0.05f, 0.50f, 0.50f, 0.80f, 0.60f, 0.50f},
    {"JAZZ",       116, 2, 10, TS_IONIAN,     TP_JAZZ,       16, TM_T_COMPLEMENTARY_BERNOULLI, 4, 1, TG_PERC_KIT,    TM_CONTROL_IDENTICAL, TG_BASS_WALKING, 0.55f, 0.10f, 0.50f, 0.55f, 0.70f, 0.50f, 0.55f},
    {"RHYTHM",     104, 4, 9,  TS_AEOLIAN,    TP_ANDALUSIAN, 32, TM_T_DRUMS,                   3, 1, TG_PERC_KIT,    TM_CONTROL_IDENTICAL, TG_BASS_MARBLES, 0.80f, 0.00f, 0.75f, 0.55f, 0.68f, 0.80f, 0.55f},
    {"DRIVE",      124, 4, 4,  TS_MIXOLYDIAN, TP_MIXO_ROCK,  32, TM_T_DRUMS,                   3, 1, TG_PERC_KIT,    TM_CONTROL_IDENTICAL, TG_BASS_REPEAT, 0.85f, 0.00f, 0.80f, 0.50f, 0.75f, 0.85f, 0.65f},
};

int tp_character_count(void) { return N_OF(characters); }
const char *tp_character_name(int i) { return i >= 0 && i < N_OF(characters) ? characters[i].name : "?"; }

void tp_character_apply(tg_gen *g, int i, uint32_t seed)
{
    if (i < 0 || i >= N_OF(characters))
        return;
    const tp_char *c = &characters[i];
    float sr = g->sample_rate;
    tg_init(g, seed, sr);
    g->bpm = c->bpm;
    g->division = c->division;
    g->root = c->root;
    g->scale = c->scale;
    g->prog = c->prog;
    g->chord_ticks = c->chord_ticks;
    g->t.model = c->t_model;
    g->t.bias = c->t_bias;
    g->t.jitter = c->t_jitter;
    g->t.sequence.deja_vu = c->t_dv;
    g->x.control_mode = c->x_control;
    g->x.spread = c->x_spread;
    g->x.steps = c->x_steps;
    g->x.deja_vu = c->x_dv;
    g->melody_density = c->density;
    g->pad_tones = c->pad_tones;
    g->pad_restrike = c->pad_restrike != 0;
    g->bass_mode = c->bass_mode;
    g->perc_mode = c->perc;
    tg_reseed(g, seed);
}
