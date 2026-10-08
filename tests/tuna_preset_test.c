/* SPDX-License-Identifier: GPL-3.0-only */
/* Tests of TUNA's presets: the record format, the 16 slots (in memory and through Felucca's flash
 * storage on a simulated NOR, power cuts included), the composer library by era, the character presets.
 *   cc -O2 -Ifirmware/src -o build/host/tuna_preset_test tests/tuna_preset_test.c firmware/src/tuna_preset.c \
 *      firmware/src/tuna_gen.c firmware/src/tuna_style.c firmware/src/tuna_marbles.c \
 *      firmware/src/tuna_marbles_tables.c firmware/src/tuna_theory.c -lm
 *   build/host/tuna_preset_test          (exit 0: all passed) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tuna_preset.h"

#define SR 11025.0f
#define BLOCK 16

static int fails, checks;
#define CHECK(cond, ...)                                                   \
    do {                                                                   \
        checks++;                                                          \
        if (!(cond)) {                                                     \
            fails++;                                                       \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                    \
            printf(__VA_ARGS__);                                           \
            printf("\n");                                                  \
        }                                                                  \
    } while (0)

/* ---- the simulated NOR flash and Felucca's storage (as tests/storage_test.c) ---------------------------- */
static uint8_t nor[0x100000];
static int fail_bytes = -1;            /* power loss after this many programmed bytes */
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off) { memset(nor + off, 0xFF, 4096); return 0; }
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    if ((off & 0xFFu) + n > 256u) {
        printf("page wrap at %#x\n", off);
        exit(1);
    }
    for (uint32_t i = 0; i < n; i++) {
        if (fail_bytes == 0) return -9;
        if (fail_bytes > 0) fail_bytes--;
        nor[off + i] &= s[i];
    }
    return 0;
}
#include "../firmware/src/storage.c"

static int flash_load(void *ctx, int group, uint8_t *buf, int max) { (void)ctx; return st_load(OBJ_PROJECT0 + group, buf, (uint32_t)max); }
static int flash_save(void *ctx, int group, const uint8_t *buf, int n)
{
    (void)ctx;
    return st_save(OBJ_PROJECT0 + group, buf, (uint32_t)n);
}
static const tp_store flash = {flash_load, flash_save, 0};

/* ---- an in-memory store ---------------------------------------------------------------------------------- */
static uint8_t mem[TP_GROUPS][TP_GROUP_MAX];
static int mem_n[TP_GROUPS];
static int mem_saves;
static int mem_load(void *ctx, int group, uint8_t *buf, int max)
{
    (void)ctx;
    if (mem_n[group] <= 0) return -1;
    int n = mem_n[group] < max ? mem_n[group] : max;
    memcpy(buf, mem[group], (size_t)n);
    return n;
}
static int mem_save(void *ctx, int group, const uint8_t *buf, int n)
{
    (void)ctx;
    if (n > TP_GROUP_MAX) return -1;
    memcpy(mem[group], buf, (size_t)n);
    mem_n[group] = n;
    mem_saves++;
    return 0;
}
static const tp_store memstore = {mem_load, mem_save, 0};

/* ---- the music: a hash of every event and its moment ----------------------------------------------------- */
static tg_gen ga, gb;
typedef struct { uint64_t h; long notes; tg_gen *g; uint16_t off_key; } hasher;

static void hash_cb(void *ctx, const tg_event *e)
{
    hasher *x = ctx;
    uint8_t b[16] = {(uint8_t)x->g->ticks, (uint8_t)(x->g->ticks >> 8), (uint8_t)(x->g->ticks >> 16), (uint8_t)e->offset,
                     (uint8_t)(e->offset >> 8), e->type, e->part, e->note, e->velocity, e->on, e->param,
                     (uint8_t)e->value, (uint8_t)(e->value >> 8)};
    for (int i = 0; i < 13; ++i)
        x->h = (x->h ^ b[i]) * 1099511628211ull;
    if (e->type == TG_EV_NOTE && e->on) {
        x->notes++;
        const tg_gen *cur = x->g;
        if (e->part < TG_PART_DRUMS) {      /* in key, as tests/tuna_gen_render.c judges it */
            uint16_t allowed = tt_scale_mask(cur->period_scale >= 0 ? cur->period_scale : cur->scale) |
                               tt_scale_mask(cur->scale) | cur->chord_scale | cur->mel_scale;
            for (int k = 0; k < cur->chord.n; ++k)
                allowed |= (uint16_t)(1u << ((cur->chord.root + cur->chord.tone[k]) % 12));
            int pc = ((e->note - cur->root) % 12 + 12) % 12;
            bool leaning = e->part == TG_PART_MELODY && cur->mel_pending >= 0;
            x->off_key += !((allowed >> pc) & 1) && !leaning;
        }
    }
}

static hasher play(tg_gen *g, float secs)
{
    hasher x = {1469598103934665603ull, 0, g, 0};
    long blocks = (long)(secs * SR / BLOCK);
    for (long i = 0; i < blocks; ++i)
        tg_process(g, BLOCK, hash_cb, &x);
    return x;
}

static void fresh(tg_gen *g, int style, uint32_t seed)
{
    tg_init(g, seed, SR);
    if (style >= 0)
        tg_apply_style(g, style);
    tg_reseed(g, seed);
}

/* freeze takes effect at the period's end */
static bool freeze_now(tg_gen *g)
{
    if (!tg_freeze(g))
        return false;
    for (long i = 0; i < (long)(600 * SR / BLOCK) && !g->frozen; ++i)
        tg_process(g, BLOCK, hash_cb, &(hasher){0, 0, g, 0});
    return g->frozen;
}

static uint8_t rec[TP_RECORD_MAX], rec2[TP_RECORD_MAX], back[TP_RECORD_MAX];

/* ---- record round trips ------------------------------------------------------------------------------------- */
static void test_roundtrip(void)
{
    int worst = 0;
    for (int style = 0; style < TG_STYLE_COUNT; ++style) {
        uint32_t seed = 1000u + (uint32_t)style * 7u;
        fresh(&ga, style, seed);
        tp_host h = {"COMPOSER", false, {{0}}};
        int n = tp_encode(&ga, &h, rec, sizeof rec);
        CHECK(n > 0, "%s: encode %d", tg_style_get(style)->name, n);
        if (n <= 0) continue;
        if (n > worst) worst = n;
        /* the settings are saved, not their moment: after minutes of evolution and contrast, the same bytes */
        play(&ga, 150.0f);
        int m = tp_encode(&ga, &h, rec2, sizeof rec2);
        CHECK(m == n && !memcmp(rec, rec2, (size_t)n), "%s: record changed while playing (%d/%d bytes)",
              tg_style_get(style)->name, m, n);
        if (m == n)
            for (int i = 0; i < n; ++i)
                if (rec[i] != rec2[i]) {
                    printf("    first difference at byte %d: %d -> %d\n", i, rec[i], rec2[i]);
                    break;
                }
        /* decoding gives the same record back, and the same music as a fresh start */
        gb.sample_rate = SR;
        tp_host h2;
        CHECK(tp_decode(&gb, &h2, rec, n) == TP_OK, "%s: decode", tg_style_get(style)->name);
        CHECK(!strcmp(h2.name, "COMPOSER") && !h2.sounds_set, "%s: host part", tg_style_get(style)->name);
        m = tp_encode(&gb, &h2, rec2, sizeof rec2);
        CHECK(m == n && !memcmp(rec, rec2, (size_t)n), "%s: re-encode differs", tg_style_get(style)->name);
        fresh(&ga, style, seed);
        hasher a = play(&ga, 60.0f), b = play(&gb, 60.0f);
        CHECK(a.h == b.h && a.notes == b.notes && a.notes > 0, "%s: music after loading differs (%ld/%ld notes)",
              tg_style_get(style)->name, a.notes, b.notes);
    }
    printf("largest composer record: %d bytes\n", worst);

    /* edited settings, sounds and a name come back; the music is that of the same edits from the start */
    fresh(&ga, TG_STYLE_DEBUSSY, 77);
    ga.bpm = 63.0f;
    ga.root = 3;
    ga.melody_density = 0.6f;
    ga.mel_mode = TG_MEL_CHANT;
    ga.swing = 0.4f;
    ga.tuning = TT_TUNE_MEANTONE;
    ga.x.spread = 0.8f;
    ga.t.bias = 0.3f;
    ga.perc_mode = TG_PERC_OFF;
    tg_reseed(&ga, 77);
    tp_host h = {"MY DEBUSSY", true, {{1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15}, {16, 17, 18, 19, 20}}, {33, 44, 55, 2}, {60, 40, 1}};
    int n = tp_encode(&ga, &h, rec, sizeof rec);
    tp_host h2;
    CHECK(tp_decode(&gb, &h2, rec, n) == TP_OK && !memcmp(&h2, &h, sizeof h), "edited: host part back");
    CHECK(gb.bpm == 63.0f && gb.root == 3 && gb.mel_mode == TG_MEL_CHANT && gb.tuning == TT_TUNE_MEANTONE &&
              gb.x.spread == 0.8f && gb.t.bias == 0.3f && gb.style_id == TG_STYLE_DEBUSSY,
          "edited: settings back");
    hasher a = play(&ga, 60.0f), b = play(&gb, 60.0f);
    CHECK(a.h == b.h && a.notes > 0, "edited: music after loading differs");
    char name[TP_NAME + 1];
    CHECK(tp_peek_name(rec, n, name) == TP_OK && !strcmp(name, "MY DEBUSSY"), "peek name");
    CHECK(tp_encode(&ga, &h, rec, 100) == TP_ERR_SPACE, "small buffer refused");

    /* a composer's manner: saved and restored (record version 4) */
    if (tg_manner_count(TG_STYLE_BACH) > 1) {
        fresh(&ga, TG_STYLE_BACH, 9);
        tg_apply_manner(&ga, TG_STYLE_BACH, 1);
        tg_reseed(&ga, 9);
        n = tp_encode(&ga, NULL, rec, sizeof rec);
        CHECK(tp_decode(&gb, NULL, rec, n) == TP_OK && gb.manner == 1 && gb.style_id == TG_STYLE_BACH &&
                  gb.bpm == ga.bpm && gb.division == ga.division, "manner: back (%d, bpm %.0f)", gb.manner, gb.bpm);
        hasher x = play(&ga, 60.0f), y = play(&gb, 60.0f);
        CHECK(x.h == y.h && x.notes > 0, "manner: the same music after loading");
    }

    /* evolution on, from the start: saved and restored */
    fresh(&ga, -1, 5);
    tg_evolve(&ga, true, 2, 1.0f, true);
    n = tp_encode(&ga, NULL, rec, sizeof rec);
    play(&ga, 200.0f);                                 /* evolution has moved the key and density by now */
    int m = tp_encode(&ga, NULL, rec2, sizeof rec2);
    CHECK(m == n && !memcmp(rec, rec2, (size_t)n), "evolving: record stays at the resting values");
    CHECK(tp_decode(&gb, NULL, rec, n) == TP_OK && gb.evolve && gb.evolve_periods == 2 && gb.evolve_modulate,
          "evolving: evolution back");
    fresh(&ga, -1, 5);
    tg_evolve(&ga, true, 2, 1.0f, true);
    a = play(&ga, 200.0f);
    b = play(&gb, 200.0f);
    CHECK(a.h == b.h, "evolving: music after loading differs");
}

/* ---- the sequencer and a frozen pattern ---------------------------------------------------------------------- */
static void test_pattern(void)
{
    fresh(&ga, TG_STYLE_BACH, 9);
    play(&ga, 30.0f);
    CHECK(freeze_now(&ga), "freeze");
    tg_step_cond(&ga, TG_PART_MELODY, 4, TG_C_RATIO, 1, 2);
    tg_step_lock(&ga, TG_PART_MELODY, 4, TG_LK_VEL, 40);
    tg_step_lock(&ga, TG_PART_BASS, 8, TG_LK_OCT, -1);
    tg_step_lock(&ga, TG_PART_PAD, 12, TG_LK_REV, 90);
    int n = tp_encode(&ga, NULL, rec, sizeof rec);
    CHECK(n > 0, "frozen: encode %d", n);
    CHECK(tp_decode(&gb, NULL, rec, n) == TP_OK, "frozen: decode");
    CHECK(gb.frozen && gb.pat.n == ga.pat.n && gb.pat.len == ga.pat.len && gb.pat.root == ga.pat.root &&
              gb.pat.scale == ga.pat.scale, "frozen: pattern back (%d/%d notes)", gb.pat.n, ga.pat.n);
    bool same = gb.nsteps == ga.nsteps;
    for (int i = 0; i < ga.pat.n && same; ++i) {
        const tg_pnote *x = &ga.pat.ev[i], *y = &gb.pat.ev[i];
        same = x->tick == y->tick && x->len == y->len && x->part == y->part && x->note == y->note && x->vel == y->vel;
    }
    for (int i = 0; i < ga.nsteps && same; ++i) {
        const tg_step *x = &ga.steps[i], *y = &gb.steps[i];
        same = x->tick == y->tick && x->part == y->part && x->cond == y->cond && x->ca == y->ca && x->cb == y->cb &&
               x->nlock == y->nlock;
        for (int k = 0; k < x->nlock && same; ++k)
            same = x->lock[k].param == y->lock[k].param && x->lock[k].value == y->lock[k].value;
    }
    CHECK(same && ga.nsteps == 3, "frozen: notes, conditions and locks back (%d steps)", gb.nsteps);
    /* two loads play alike, and play the pattern */
    static tg_gen gc;
    gc.sample_rate = SR;
    tp_decode(&gc, NULL, rec, n);
    hasher a = play(&gb, 40.0f), b = play(&gc, 40.0f);
    CHECK(a.h == b.h && a.notes > 0, "frozen: loads play alike");

    /* the worst case: a full pattern and every step with three locks fits a record and a group */
    fresh(&ga, TG_STYLE_GLASS, 3);
    play(&ga, 5.0f);
    freeze_now(&ga);
    if (ga.pat.len < 2) ga.pat.len = 384;
    ga.pat.n = 0;
    for (int i = 0; i < TG_PAT_MAX; ++i)
        tg_pat_add(&ga, i % ga.pat.len, i % TG_EVENT_PARTS, 40 + i % 60, 100, 2);
    for (int i = 0; i < TG_STEPS; ++i)
        for (int k = 0; k < TG_STEP_LOCKS; ++k)
            tg_step_lock(&ga, i % TG_PARTS, i, TG_LK_E0 + k, i);
    CHECK(ga.pat.n == TG_PAT_MAX && ga.nsteps == TG_STEPS, "full: %d notes, %d steps", ga.pat.n, ga.nsteps);
    tp_host h = {"FULL", true, {{0}}};
    n = tp_encode(&ga, &h, rec, sizeof rec);
    printf("largest record (full pattern, 64 steps of 3 locks): %d bytes\n", n);
    CHECK(n > 0 && n <= TP_RECORD_MAX && n + 7 <= TP_GROUP_MAX, "full record fits (%d)", n);
    CHECK(tp_decode(&gb, NULL, rec, n) == TP_OK && gb.pat.n == TG_PAT_MAX && gb.nsteps == TG_STEPS &&
              gb.pat.ev[383].note == ga.pat.ev[383].note && gb.pat.ev[200].tick == ga.pat.ev[200].tick,
          "full record back");
    int m = tp_encode(&gb, &h, rec2, sizeof rec2);
    CHECK(m == n && !memcmp(rec, rec2, (size_t)n), "full record: re-encode identical");
}

/* ---- damage --------------------------------------------------------------------------------------------- */
static void test_damage(void)
{
    fresh(&ga, TG_STYLE_MOZART, 4);
    tp_host h = {"SAFE", false, {{0}}};
    int n = tp_encode(&ga, &h, rec, sizeof rec);
    int bad = 0;
    for (int i = 0; i < n; ++i)
        for (int bit = 0; bit < 8; bit += 3) {
            memcpy(rec2, rec, (size_t)n);
            rec2[i] ^= (uint8_t)(1u << bit);
            gb.sample_rate = SR;
            int r = tp_decode(&gb, NULL, rec2, n);
            bad += r == TP_OK;
        }
    CHECK(bad == 0, "every flipped bit is refused (%d accepted)", bad);
    CHECK(tp_decode(&gb, NULL, rec, n - 1) == TP_ERR_FORMAT, "short record refused");
    CHECK(tp_decode(&gb, NULL, rec, 10) == TP_ERR_FORMAT, "fragment refused");
    memcpy(rec2, "TUNX", 4);
    CHECK(tp_decode(&gb, NULL, rec2, n) == TP_ERR_FORMAT, "wrong magic refused");
    /* a newer version, intact: said so */
    memcpy(rec2, rec, (size_t)n);
    rec2[4] = TP_VERSION + 1;
    CHECK(tp_decode(&gb, NULL, rec2, n) == TP_ERR_VERSION, "newer version refused");
    /* the generator was not touched by the refusals: a good record still loads */
    CHECK(tp_decode(&gb, NULL, rec, n) == TP_OK, "good record after refusals");
}

/* ---- slots ---------------------------------------------------------------------------------------------------- */
static int make(int style, uint32_t seed, const char *name, uint8_t *buf)
{
    fresh(&ga, style, seed);
    tp_host h = {{0}, false, {{0}}};
    snprintf(h.name, sizeof h.name, "%s", name);
    return tp_encode(&ga, &h, buf, TP_RECORD_MAX);
}

static void test_slots(const tp_store *s, const char *what)
{
    static uint8_t recs[TP_SLOTS][TP_RECORD_MAX];
    int lens[TP_SLOTS];
    char names[TP_SLOTS][TP_NAME + 1];
    CHECK(tp_slots_used(s, names) == 0, "%s: empty at first", what);
    CHECK(tp_slot_load(s, 3, back, sizeof back) == TP_ERR_EMPTY, "%s: empty slot", what);
    for (int i = 0; i < TP_SLOTS; ++i) {
        char nm[16];
        snprintf(nm, sizeof nm, "SLOT %d", i + 1);
        lens[i] = make((i * 5) % TG_STYLE_COUNT, (uint32_t)i, nm, recs[i]);
        CHECK(tp_slot_save(s, i, recs[i], lens[i]) == TP_OK, "%s: save slot %d", what, i);
    }
    CHECK(tp_slots_used(s, names) == 0xFFFF && !strcmp(names[0], "SLOT 1") && !strcmp(names[15], "SLOT 16"),
          "%s: all slots used, names", what);
    bool ok = true;
    for (int i = 0; i < TP_SLOTS; ++i)
        ok &= tp_slot_load(s, i, back, sizeof back) == lens[i] && !memcmp(back, recs[i], (size_t)lens[i]);
    CHECK(ok, "%s: every slot loads its own record", what);
    /* overwrite one: its neighbours keep theirs */
    lens[5] = make(TG_STYLE_SATIE, 99, "NEW", recs[5]);
    CHECK(tp_slot_save(s, 5, recs[5], lens[5]) == TP_OK, "%s: overwrite", what);
    ok = true;
    for (int i = 0; i < TP_SLOTS; ++i)
        ok &= tp_slot_load(s, i, back, sizeof back) == lens[i] && !memcmp(back, recs[i], (size_t)lens[i]);
    CHECK(ok, "%s: overwrite keeps the others", what);
    CHECK(tp_slot_clear(s, 6) == TP_OK && tp_slot_load(s, 6, back, sizeof back) == TP_ERR_EMPTY &&
              tp_slot_load(s, 7, back, sizeof back) == lens[7] && tp_slots_used(s, NULL) == (0xFFFF & ~(1u << 6)),
          "%s: clear", what);
    CHECK(tp_slot_clear(s, 6) == TP_OK, "%s: clear twice", what);
    CHECK(tp_slot_save(s, 16, recs[0], lens[0]) == TP_ERR_ARG && tp_slot_load(s, -1, back, sizeof back) == TP_ERR_ARG,
          "%s: slot range", what);
    CHECK(tp_slot_save(s, 2, rec, 12) == TP_ERR_FORMAT, "%s: a damaged record is not saved", what);
    CHECK(tp_slot_load(s, 0, back, 20) == TP_ERR_SPACE, "%s: small buffer", what);

    /* space: a full pattern takes most of a group; the group says so and keeps what it holds */
    fresh(&ga, TG_STYLE_GLASS, 3);
    play(&ga, 5.0f);
    freeze_now(&ga);
    if (ga.pat.len < 2) ga.pat.len = 384;
    ga.pat.n = 0;
    for (int i = 0; i < TG_PAT_MAX; ++i)
        tg_pat_add(&ga, i % ga.pat.len, i % TG_EVENT_PARTS, 40 + i % 60, 100, 2);
    tp_host h = {"BIG", false, {{0}}};
    int big = tp_encode(&ga, &h, rec, sizeof rec);
    int free8 = tp_group_free(s, 2);
    CHECK(tp_slot_save(s, 8, rec, big) == (big + 3 <= free8 + lens[8] + 3 ? TP_OK : TP_ERR_SPACE),
          "%s: big save as space allows", what);
    tp_slot_clear(s, 9);
    tp_slot_clear(s, 10);
    tp_slot_clear(s, 11);
    CHECK(tp_slot_save(s, 8, rec, big) == TP_OK, "%s: big save in an emptied group", what);
    CHECK(tp_slot_save(s, 9, rec, big) == TP_ERR_SPACE, "%s: a second big one does not fit", what);
    CHECK(tp_slot_load(s, 8, back, sizeof back) == big && !memcmp(back, rec, (size_t)big),
          "%s: the refusal kept the first", what);
    CHECK(tp_slot_save(s, 9, recs[9], lens[9]) == TP_OK, "%s: a small one still fits beside it", what);
    CHECK(tp_slot_load(s, 4, back, sizeof back) == lens[4], "%s: other groups untouched", what);
    /* a loaded slot plays */
    int n = tp_slot_load(s, 8, back, sizeof back);
    gb.sample_rate = SR;
    CHECK(n > 0 && tp_decode(&gb, NULL, back, n) == TP_OK && gb.frozen && play(&gb, 20.0f).notes > 0,
          "%s: the big slot plays", what);
    for (int i = 0; i < TP_SLOTS; ++i)
        tp_slot_clear(s, i);
    CHECK(tp_slots_used(s, NULL) == 0, "%s: all cleared", what);
}

/* a power cut at any byte of a slot save leaves either the old group or the new one, never a broken one */
static void test_power_cut(void)
{
    memset(nor, 0xFF, sizeof nor);
    uint8_t a[TP_RECORD_MAX], b[TP_RECORD_MAX], c[TP_RECORD_MAX];
    int na = make(TG_STYLE_BACH, 1, "A", a), nb = make(TG_STYLE_RAVEL, 2, "B", b), nc = make(TG_STYLE_PART, 3, "C", c);
    tp_slot_save(&flash, 0, a, na);
    tp_slot_save(&flash, 1, b, nb);
    uint32_t off = st_sector(OBJ_PROJECT0, 0);
    static uint8_t copies[2 * ST_SECTOR];
    memcpy(copies, nor + off, sizeof copies);
    int total = 4 + 3 * 3 + na + nb + nc + (int)sizeof(st_hdr_t) + 16, olds = 0, news = 0, broken = 0;
    for (int cut = 0; cut < total; cut += 3) {
        memcpy(nor + off, copies, sizeof copies);
        fail_bytes = cut;
        int r = tp_slot_save(&flash, 2, c, nc);
        fail_bytes = -1;
        int la = tp_slot_load(&flash, 0, back, sizeof back);
        bool a_ok = la == na && !memcmp(back, a, (size_t)na);
        int lb = tp_slot_load(&flash, 1, back, sizeof back);
        bool b_ok = lb == nb && !memcmp(back, b, (size_t)nb);
        int lc = tp_slot_load(&flash, 2, back, sizeof back);
        bool c_new = lc == nc && !memcmp(back, c, (size_t)nc), c_old = lc == TP_ERR_EMPTY;
        if (a_ok && b_ok && c_new) news++;
        else if (a_ok && b_ok && c_old && r != TP_OK) olds++;
        else broken++;
    }
    CHECK(broken == 0 && olds > 0 && news > 0, "power cuts: %d old, %d new, %d broken", olds, news, broken);
}

/* ---- the library ---------------------------------------------------------------------------------------- */
static void test_eras(void)
{
    int seen[TG_STYLE_COUNT] = {0}, total = 0;
    for (int e = 0; e < TP_ERA_COUNT; ++e) {
        int prev = -10000;
        CHECK(tp_era_count(e) > 0 && strcmp(tp_era_name(e), "?"), "era %d", e);
        for (int i = 0; i < tp_era_count(e); ++i) {
            int st = tp_era_style(e, i);
            CHECK(st >= 0 && st < TG_STYLE_COUNT, "era %d: style %d", e, st);
            if (st < 0 || st >= TG_STYLE_COUNT) continue;
            seen[st]++;
            total++;
            CHECK(tp_style_era(st) == e, "%s: era", tg_style_get(st)->name);
            int born = tp_style_born(st);
            CHECK(born >= prev && born > 1000 && born < 2100, "%s: born %d after %d", tg_style_get(st)->name, born, prev);
            prev = born;
        }
        CHECK(tp_era_style(e, tp_era_count(e)) == -1, "era %d: past the end", e);
    }
    for (int st = 0; st < TG_STYLE_COUNT; ++st)
        CHECK(seen[st] == 1, "%s: in %d eras", tg_style_get(st)->name, seen[st]);
    CHECK(total == TG_STYLE_COUNT, "all %d composers in the eras (%d)", TG_STYLE_COUNT, total);
    CHECK(tp_style_era(-1) == -1 && tp_style_born(TG_STYLE_COUNT) == 0, "unknown style");
}

static void test_characters(void)
{
    CHECK(tp_character_count() >= 6, "characters");
    for (int i = 0; i < tp_character_count(); ++i)
        for (uint32_t seed = 1; seed <= 3; ++seed) {
            ga.sample_rate = SR;
            tp_character_apply(&ga, i, seed);
            CHECK(ga.style == NULL, "%s: no composer", tp_character_name(i));
            int n = tp_encode(&ga, NULL, rec, sizeof rec);
            gb.sample_rate = SR;
            CHECK(n > 0 && tp_decode(&gb, NULL, rec, n) == TP_OK, "%s: saves", tp_character_name(i));
            hasher a = play(&ga, 90.0f), b = play(&gb, 90.0f);
            CHECK(a.notes > 20, "%s seed %u: plays (%ld notes)", tp_character_name(i), seed, a.notes);
            CHECK(a.off_key == 0, "%s seed %u: %d notes off key", tp_character_name(i), seed, a.off_key);
            CHECK(a.h == b.h, "%s seed %u: loads to the same music", tp_character_name(i), seed);
        }
    CHECK(!strcmp(tp_character_name(-1), "?"), "character range");
}

int main(void)
{
    test_roundtrip();
    test_pattern();
    test_damage();
    test_slots(&memstore, "memory");
    memset(nor, 0xFF, sizeof nor);
    test_slots(&flash, "flash");
    test_power_cut();
    test_eras();
    test_characters();
    printf("tuna_preset_test: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
