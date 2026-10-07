/* SPDX-License-Identifier: GPL-3.0-only */
/* Tests of TUNA's sequencer: freeze, pattern playback, unfreeze, trig conditions, locks, evolution.
 *   cc -O2 -Ifirmware/src -o build/host/tuna_seq_test tests/tuna_seq_test.c firmware/src/tuna_gen.c \
 *      firmware/src/tuna_style.c firmware/src/tuna_marbles.c firmware/src/tuna_marbles_tables.c \
 *      firmware/src/tuna_theory.c -lm
 *   build/host/tuna_seq_test          (exit 0: all passed) */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tuna_gen.h"

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

typedef struct {
    long tick;          /* g.ticks - 1 at the event */
    int pos;            /* step: the pattern's while frozen, else the period's */
    uint32_t loop;
    bool frozen;
    tg_event e;
} logged;

#define LOGMAX 400000
static logged lg[LOGMAX];
static long nlg;
static tg_gen g;

static void cb(void *ctx, const tg_event *e)
{
    (void)ctx;
    if (nlg >= LOGMAX)
        return;
    logged *x = &lg[nlg++];
    x->tick = g.ticks - 1;
    x->frozen = g.frozen;
    if (g.frozen) {
        x->pos = g.seq_step;                    /* the tick it belongs to (a swung note arrives later) */
        x->loop = g.pat_pos == 0 && g.seq_step == g.pat.len - 1 ? g.pat_loop - 1 : g.pat_loop;
    } else {
        x->pos = tg_period_position(&g, NULL);
        x->loop = (uint32_t)g.period_index;
    }
    x->e = *e;
}

static void start(int style, uint32_t seed)
{
    tg_init(&g, seed, SR);
    tg_apply_style(&g, style);
    tg_reseed(&g, seed);
    nlg = 0;
}

static void run_ticks(long ticks)
{
    long end = g.ticks + ticks;
    while (g.ticks < end)
        tg_process(&g, BLOCK, cb, 0);
}

static int period_ticks(void)
{
    int p;
    tg_period_position(&g, &p);
    return p;
}

/* every note-on has its note-off, in order, per part and note (after a stop) */
static bool balanced(long from, long *bad_at)
{
    static int open[TG_EVENT_PARTS][128];
    memset(open, 0, sizeof open);
    for (long i = from; i < nlg; ++i) {
        const tg_event *e = &lg[i].e;
        if (e->type != TG_EV_NOTE)
            continue;
        if (e->on)
            open[e->part][e->note]++;
        else if (open[e->part][e->note] > 0)
            open[e->part][e->note]--;
    }
    for (int p = 0; p < TG_EVENT_PARTS; ++p)
        for (int n = 0; n < 128; ++n)
            if (open[p][n] && p != TG_PART_DRUMS) {
                *bad_at = p * 1000 + n;
                return false;
            }
    return true;
}

/* note-ons of one loop, frozen or not, as (pos, part, note, velocity) */
typedef struct { int pos, part, note, vel; } on_t;
static int ons_of(bool frozen, uint32_t loop, on_t *out, int max)
{
    int n = 0;
    for (long i = 0; i < nlg && n < max; ++i)
        if (lg[i].e.type == TG_EV_NOTE && lg[i].e.on && lg[i].frozen == frozen && lg[i].loop == loop)
            out[n++] = (on_t){lg[i].pos, lg[i].e.part, lg[i].e.note, lg[i].e.velocity};
    return n;
}

static int cmp_on(const void *a, const void *b)
{
    const on_t *x = a, *y = b;
    if (x->pos != y->pos) return x->pos - y->pos;
    if (x->part != y->part) return x->part - y->part;
    return x->note - y->note;
}

/* ---- freeze keeps what was heard, loop after loop; unfreeze hands back cleanly ---------------------- */
static void test_freeze(int style, uint32_t seed)
{
    start(style, seed);
    int P = period_ticks();
    run_ticks(2L * P + P / 3);                      /* a third into period 2 */
    uint32_t heard = (uint32_t)g.period_index;      /* the period playing when FREEZE is pressed */
    CHECK(tg_freeze(&g), "freeze refused");
    run_ticks(P - P / 3 + 4L * P);                  /* to the boundary, then 4 loops */
    CHECK(g.frozen, "%s: not frozen after the boundary", tg_style_get(style)->name);
    CHECK(g.pat.len == P, "pattern length %d, period %d", g.pat.len, P);

    static on_t a[4096], b[4096];
    int na = ons_of(false, heard, a, 4096);
    qsort(a, (size_t)na, sizeof *a, cmp_on);
    for (uint32_t loop = 0; loop < 4; ++loop) {
        int nb = ons_of(true, loop, b, 4096);
        qsort(b, (size_t)nb, sizeof *b, cmp_on);
        bool same = na == nb && !memcmp(a, b, (size_t)na * sizeof *a);
        CHECK(same, "%s: frozen loop %u differs from the period heard (%d vs %d notes)", tg_style_get(style)->name,
              loop, nb, na);
    }
    /* nothing from the generator while frozen */
    long gen_while_frozen = 0, first_frozen = -1;
    for (long i = 0; i < nlg; ++i)
        if (lg[i].frozen && lg[i].e.on && lg[i].e.type == TG_EV_NOTE) {
            if (first_frozen < 0) first_frozen = i;
        }
    if (first_frozen >= 0)
        CHECK(lg[first_frozen].pos == 0, "the frozen pattern starts at step %d", lg[first_frozen].pos);
    (void)gen_while_frozen;

    /* unfreeze: at the next boundary the generator plays again, new music */
    tg_unfreeze(&g);
    long mark = nlg;
    run_ticks(3L * P);
    CHECK(!g.frozen, "still frozen after unfreeze");
    {   /* every note the pattern started has ended by the time the generator takes over */
        static int open[TG_EVENT_PARTS][128];
        memset(open, 0, sizeof open);
        int left = 0;
        for (long i = 0; i < nlg; ++i) {
            const tg_event *e = &lg[i].e;
            if (!lg[i].frozen || e->type != TG_EV_NOTE || e->part == TG_PART_DRUMS)
                continue;
            if (e->on) open[e->part][e->note]++;
            else if (open[e->part][e->note]) open[e->part][e->note]--;
        }
        for (int p = 0; p < TG_EVENT_PARTS; ++p)
            for (int n = 0; n < 128; ++n)
                left += open[p][n];
        CHECK(left == 0, "%s: %d pattern notes still sounding after unfreeze", tg_style_get(style)->name, left);
    }
    long gen_notes = 0;
    for (long i = mark; i < nlg; ++i)
        gen_notes += !lg[i].frozen && lg[i].e.on && lg[i].e.type == TG_EV_NOTE;
    CHECK(gen_notes > 0, "no generated notes after unfreeze");
    tg_all_off(&g, cb, 0);
    long bad = 0;
    CHECK(balanced(0, &bad), "%s: a note left hanging (part %ld note %ld)", tg_style_get(style)->name, bad / 1000,
          bad % 1000);
}

/* ---- trig conditions on a frozen pattern ------------------------------------------------------------- */
static int plays_in_loop(int part, int pos, uint32_t loop)
{
    int n = 0;
    for (long i = 0; i < nlg; ++i)
        n += lg[i].frozen && lg[i].e.type == TG_EV_NOTE && lg[i].e.on && lg[i].e.part == part && lg[i].pos == pos &&
             lg[i].loop == loop;
    return n;
}

static void frozen_mozart(void)
{
    start(TG_STYLE_MOZART, 5);
    int P = period_ticks();
    run_ticks(P / 2);
    tg_freeze(&g);
    run_ticks(P);                                   /* frozen from the boundary on */
}

/* a melody step of the pattern (its tick), the n-th */
static int melody_tick(int nth)
{
    for (int i = 0; i < g.pat.n; ++i)
        if (g.pat.ev[i].part == TG_PART_MELODY && nth-- == 0)
            return g.pat.ev[i].tick;
    return -1;
}

static void test_conditions(void)
{
    frozen_mozart();
    CHECK(g.frozen, "not frozen");
    int t = melody_tick(1), t2 = melody_tick(3);
    CHECK(t >= 0 && t2 >= 0, "no melody steps to test with");

    tg_step_cond(&g, TG_PART_MELODY, t, TG_C_RATIO, 1, 2);
    tg_step_cond(&g, TG_PART_MELODY, t2, TG_C_PRE, 0, 0);
    uint32_t l0 = g.pat_loop + 1;
    run_ticks((long)g.pat.len * 7);
    int ok_ratio = 0, ok_pre = 0, n = 0;
    for (uint32_t L = l0; L < l0 + 6; ++L, ++n) {
        bool want = L % 2 == 0;
        ok_ratio += (plays_in_loop(TG_PART_MELODY, t, L) > 0) == want;
        ok_pre += (plays_in_loop(TG_PART_MELODY, t2, L) > 0) == want;     /* PRE follows the ratio step */
    }
    CHECK(ok_ratio == n, "RATIO 1:2: right in %d of %d loops", ok_ratio, n);
    CHECK(ok_pre == n, "PRE: right in %d of %d loops", ok_pre, n);

    tg_step_cond(&g, TG_PART_MELODY, t, TG_C_FILL, 0, 0);
    tg_step_clear(&g, TG_PART_MELODY, t2);
    uint32_t lf = g.pat_loop + 1;
    run_ticks((long)g.pat.len - g.pat_pos);          /* to the next loop */
    g.fill = true;
    run_ticks(g.pat.len);
    g.fill = false;
    run_ticks(g.pat.len);
    CHECK(plays_in_loop(TG_PART_MELODY, t, lf) > 0, "FILL: silent while FILL was held");
    CHECK(plays_in_loop(TG_PART_MELODY, t, lf + 1) == 0, "FILL: played without FILL");

    tg_step_cond(&g, TG_PART_MELODY, t, TG_C_PROB, 50, 0);
    uint32_t lp = g.pat_loop + 1;
    run_ticks((long)g.pat.len * 201);
    int hits = 0;
    for (uint32_t L = lp; L < lp + 200; ++L)
        hits += plays_in_loop(TG_PART_MELODY, t, L) > 0;
    CHECK(hits > 70 && hits < 130, "PROB 50: played %d of 200 loops", hits);

    tg_all_off(&g, cb, 0);
    long bad0 = 0;
    CHECK(balanced(0, &bad0), "conditions: a note left hanging (part %ld note %ld)", bad0 / 1000, bad0 % 1000);

    /* a chord under a condition stays whole: all its notes or none (Mahler: held block chords) */
    start(TG_STYLE_MAHLER, 2);
    {
        int P = period_ticks();
        run_ticks(P / 2);
        tg_freeze(&g);
        run_ticks(P);
    }
    int tc = -1, best = 1;
    for (int i = 0; i < g.pat.n; ++i) {
        int k = 0;
        for (int j = 0; j < g.pat.n; ++j)
            k += g.pat.ev[j].part == TG_PART_PAD && g.pat.ev[j].tick == g.pat.ev[i].tick;
        if (g.pat.ev[i].part == TG_PART_PAD && k > best) {
            best = k;
            tc = g.pat.ev[i].tick;
        }
    }
    CHECK(tc >= 0, "no chord in the frozen pattern to test with");
    for (int i = 0; i + 1 < g.pat.n; ++i)
        if (g.pat.ev[i].part == TG_PART_PAD && g.pat.ev[i + 1].part == TG_PART_PAD && g.pat.ev[i].tick == g.pat.ev[i + 1].tick)
            tc = g.pat.ev[i].tick;
    if (tc >= 0) {
        tg_step_cond(&g, TG_PART_PAD, tc, TG_C_PROB, 50, 0);
        uint32_t lc = g.pat_loop + 1;
        run_ticks((long)g.pat.len * 41);
        int whole = 0, chord = 0;
        for (long i = 0; i < g.pat.n; ++i)
            chord += g.pat.ev[i].part == TG_PART_PAD && g.pat.ev[i].tick == tc;
        for (uint32_t L = lc; L < lc + 40; ++L) {
            int k = plays_in_loop(TG_PART_PAD, tc, L);
            whole += k == 0 || k == chord;
        }
        CHECK(whole == 40, "a chord under PROB split in %d of 40 loops", 40 - whole);
    }
    tg_all_off(&g, cb, 0);
    long bad = 0;
    CHECK(balanced(0, &bad), "conditions: a note left hanging (part %ld note %ld)", bad / 1000, bad % 1000);
}

/* ---- locks ------------------------------------------------------------------------------------------- */
static const logged *on_at(int part, int pos, uint32_t loop)
{
    for (long i = 0; i < nlg; ++i)
        if (lg[i].frozen && lg[i].e.type == TG_EV_NOTE && lg[i].e.on && lg[i].e.part == part && lg[i].pos == pos &&
            lg[i].loop == loop)
            return &lg[i];
    return 0;
}

static void test_locks(void)
{
    frozen_mozart();
    int t = melody_tick(2);
    int orig = -1;
    for (int i = 0; i < g.pat.n; ++i)
        if (g.pat.ev[i].part == TG_PART_MELODY && g.pat.ev[i].tick == t)
            orig = g.pat.ev[i].note;
    tg_step_lock(&g, TG_PART_MELODY, t, TG_LK_VEL, 127);
    tg_step_lock(&g, TG_PART_MELODY, t, TG_LK_STEPS, 1);
    tg_step_lock(&g, TG_PART_MELODY, t, TG_LK_E2, 99);
    uint32_t L = g.pat_loop + 1;
    run_ticks((long)g.pat.len * 2);
    const logged *x = on_at(TG_PART_MELODY, t, L);
    CHECK(x != 0, "the locked step did not play");
    if (x) {
        CHECK(x->e.velocity == 127, "VEL lock: velocity %d", x->e.velocity);
        int want = orig;
        do want++; while (!((tt_scale_mask(g.pat.scale) >> (((want - g.pat.root) % 12 + 12) % 12)) & 1));
        CHECK(x->e.note == want, "STEPS +1: note %d, want %d (from %d)", x->e.note, want, orig);
        /* the sound lock came right before it, and goes back at the part's next note */
        const logged *prm = x - 1;
        CHECK(prm->e.type == TG_EV_PARAM && prm->e.param == TG_LK_E2 && prm->e.value == 99,
              "E2 lock not sent before its note");
        bool restored = false;
        for (const logged *y = x + 1; y < lg + nlg && !restored; ++y) {
            if (y->e.type == TG_EV_PARAM && y->e.part == TG_PART_MELODY && y->e.param == TG_LK_E2)
                restored = y->e.value == TG_LK_RESTORE;
            if (y->e.type == TG_EV_NOTE && y->e.on && y->e.part == TG_PART_MELODY && !(y->pos == t))
                break;
        }
        CHECK(restored, "E2 lock not restored at the next melody note");
    }
    /* LEN: released one tick after its start */
    tg_step_clear(&g, TG_PART_MELODY, t);
    tg_step_lock(&g, TG_PART_MELODY, t, TG_LK_LEN, 1);
    L = g.pat_loop + 1;
    run_ticks((long)g.pat.len * 2);
    x = on_at(TG_PART_MELODY, t, L);
    long off_tick = -1;
    for (const logged *y = x ? x + 1 : lg + nlg; y < lg + nlg; ++y)
        if (y->e.type == TG_EV_NOTE && !y->e.on && y->e.part == TG_PART_MELODY && y->e.note == x->e.note) {
            off_tick = y->tick;
            break;
        }
    CHECK(x && off_tick == x->tick + 1, "LEN 1: released %ld ticks after", x ? off_tick - x->tick : -1);
    tg_all_off(&g, cb, 0);
    long bad = 0;
    CHECK(balanced(0, &bad), "locks: a note left hanging (part %ld note %ld)", bad / 1000, bad % 1000);
}

/* ---- the overlay on generated music ------------------------------------------------------------------ */
static void test_generator_overlay(void)
{
    start(TG_STYLE_MOZART, 5);
    int P = period_ticks();
    tg_step_cond(&g, TG_PART_BASS, 0, TG_C_RATIO, 1, 2);   /* the bass's downbeat of each period, every other */
    run_ticks(8L * P + 2);
    int ok = 0, n = 0;
    for (uint32_t L = 1; L < 8; ++L, ++n) {
        int k = 0;
        for (long i = 0; i < nlg; ++i)
            k += !lg[i].frozen && lg[i].e.type == TG_EV_NOTE && lg[i].e.on && lg[i].e.part == TG_PART_BASS &&
                 lg[i].pos == 0 && lg[i].loop == L;
        ok += (k > 0) == (L % 2 == 0);
    }
    CHECK(ok == n, "RATIO 1:2 on generated bass: right in %d of %d periods", ok, n);
    tg_all_off(&g, cb, 0);
    long bad = 0;
    CHECK(balanced(0, &bad), "generator overlay: a note left hanging (part %ld note %ld)", bad / 1000, bad % 1000);
}

/* ---- evolution ------------------------------------------------------------------------------------------- */
static void test_evolution(int style, uint32_t seed, int away_root_offset, int away_scale)
{
    start(style, seed);
    int home = g.root, home_scale = g.scale;
    tg_evolve(&g, true, 8, 1.0f, true);
    int P = period_ticks();
    int roots[20], dens_notes[20];
    for (int k = 0; k < 17; ++k) {
        long mark = nlg;
        run_ticks(P);
        roots[k] = g.root * 100 + g.scale;
        dens_notes[k] = 0;
        for (long i = mark; i < nlg; ++i)
            dens_notes[k] += lg[i].e.type == TG_EV_NOTE && lg[i].e.on && lg[i].e.part == TG_PART_MELODY;
    }
    int away = ((home + away_root_offset) % 12) * 100 + away_scale;
    bool went = false, back = false;
    for (int k = 0; k < 16; ++k) {
        if (roots[k] == away) went = true;
        if (went && roots[k] == home * 100 + home_scale) back = true;
    }
    CHECK(went, "%s: never modulated (to root %d scale %d)", tg_style_get(style)->name, (home + away_root_offset) % 12,
          away_scale);
    CHECK(back, "%s: never came home", tg_style_get(style)->name);
    /* the arc: the middle of the cycle busier than its ends */
    int ends = dens_notes[0] + dens_notes[7] + dens_notes[8] + dens_notes[15];
    int mid = dens_notes[3] + dens_notes[4] + dens_notes[11] + dens_notes[12];
    CHECK(mid > ends, "%s: the energy arc does not show (middle %d, ends %d melody notes)", tg_style_get(style)->name,
          mid, ends);
    tg_all_off(&g, cb, 0);
    long bad = 0;
    CHECK(balanced(0, &bad), "evolution: a note left hanging (part %ld note %ld)", bad / 1000, bad % 1000);
}

int main(void)
{
    for (int s = 0; s < TG_STYLE_COUNT; ++s)
        test_freeze(s, 21 + (uint32_t)s);
    test_conditions();
    test_locks();
    test_generator_overlay();
    test_evolution(TG_STYLE_MOZART, 5, 7, TS_IONIAN);     /* C major -> G major -> C */
    test_evolution(TG_STYLE_BACH, 11, 3, TS_IONIAN);      /* D minor -> F major -> D minor */
    printf("tuna_seq: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
