/* SPDX-License-Identifier: GPL-3.0-only */
/* Tests of TUNA's composer mechanisms: beat-group meters, swing and lilt, harmony models, resolution,
 * melody, bass and texture modes, instrumentation and tuning.
 *   cc -O2 -Ifirmware/src -o build/host/tuna_mech_test tests/tuna_mech_test.c firmware/src/tuna_gen.c \
 *      firmware/src/tuna_style.c firmware/src/tuna_marbles.c firmware/src/tuna_marbles_tables.c \
 *      firmware/src/tuna_theory.c -lm
 *   build/host/tuna_mech_test          (exit 0: all passed) */
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

static tg_gen g;

typedef struct {
    long tick;
    long sample;         /* g.clock + offset: when it sounds */
    tg_event e;
} logged;

#define LOGMAX 200000
static logged lg[LOGMAX];
static long nlg;

static void cb(void *ctx, const tg_event *e)
{
    (void)ctx;
    if (nlg >= LOGMAX)
        return;
    lg[nlg].tick = g.ticks - 1;
    lg[nlg].sample = g.clock + e->offset;
    lg[nlg].e = *e;
    nlg++;
}

static void run_seconds(float secs)
{
    long n = (long)(secs * SR) / BLOCK;
    for (long i = 0; i < n; ++i)
        tg_process(&g, BLOCK, cb, NULL);
}

/* ---- meters ----------------------------------------------------------------------------------------- */
static void set_meter(const uint8_t *groups, uint16_t bars)
{
    int i = 0;
    for (; groups[i]; ++i)
        g.meter[i] = groups[i];
    g.meter[i] = 0;
    g.meter_bars = bars;
}

static void test_meters(void)
{
    tg_init(&g, 1, SR);
    tg_meter_pos m;
    /* regular 4/4 in sixteenths */
    g.division = 4;
    g.beats_per_bar = 4;
    tg_meter_at(&g, 0, &m);
    CHECK(m.bar_start && m.beat == 0 && m.glen == 4 && m.bar_len == 16, "4/4 downbeat");
    tg_meter_at(&g, 22, &m);
    CHECK(!m.bar_start && m.beat == 1 && m.sub == 2 && m.bar_pos == 6, "4/4 tick 22: beat %d sub %d", m.beat, m.sub);

    /* 3+3+2 eighths */
    static const uint8_t tango[] = {3, 3, 2, 0};
    set_meter(tango, 0);
    g.division = 2;
    int bars;
    CHECK(tg_meter_cycle(&g, &bars) == 8 && bars == 1, "3+3+2 is one bar of 8");
    static const int beat_of[8] = {0, 0, 0, 1, 1, 1, 2, 2};
    static const int sub_of[8] = {0, 1, 2, 0, 1, 2, 0, 1};
    for (int q = 0; q < 24; ++q) {
        tg_meter_at(&g, q, &m);
        CHECK(m.beat == beat_of[q % 8] && m.sub == sub_of[q % 8] && m.bar_start == (q % 8 == 0) && m.nbeats == 3,
              "3+3+2 tick %d: beat %d sub %d", q, m.beat, m.sub);
    }

    /* Promenade: 5/4 then 6/4, a two-bar cycle of quarters (division 1) */
    static const uint8_t prom[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0};
    set_meter(prom, 1u << 5);
    g.division = 1;
    CHECK(tg_meter_cycle(&g, &bars) == 11 && bars == 2, "5/4 + 6/4 cycle: 11 beats, 2 bars");
    tg_meter_at(&g, 4, &m);
    CHECK(!m.bar_start && m.beat == 4 && m.bar_len == 5 && m.nbeats == 5, "5/4 bar: beat 5 of 5");
    tg_meter_at(&g, 5, &m);
    CHECK(m.bar_start && m.beat == 0 && m.bar_len == 6 && m.nbeats == 6, "6/4 bar starts at beat 6");
    tg_meter_at(&g, 16, &m);
    CHECK(m.bar_start, "the cycle repeats at 11");
    g.meter[0] = 0;
    g.meter_bars = 0;
}

/* swing delays the off-beat eighth, lilt the waltz's first and third beats; the notes stay the same */
typedef struct { uint8_t part, note; long sample, tick; } onset;
static int collect_onsets(onset *o, int max)
{
    int n = 0;
    for (long i = 0; i < nlg && n < max; ++i)
        if (lg[i].e.on && lg[i].e.type == TG_EV_NOTE) {
            o[n].part = lg[i].e.part;
            o[n].note = lg[i].e.note;
            o[n].sample = lg[i].sample;
            o[n].tick = lg[i].tick;
            n++;
        }
    return n;
}

static void test_swing_lilt(void)
{
    static onset a[20000], b[20000];
    for (int mode = 0; mode < 2; ++mode) {
        const char *what = mode ? "lilt" : "swing";
        int style = mode ? TG_STYLE_SATIE : TG_STYLE_MOZART;
        tg_init(&g, 3, SR);
        tg_apply_style(&g, style);
        tg_reseed(&g, 3);
        nlg = 0;
        run_seconds(20);
        int na = collect_onsets(a, 20000);
        tg_init(&g, 3, SR);
        tg_apply_style(&g, style);
        tg_reseed(&g, 3);
        if (mode) g.lilt = 1.0f; else g.swing = 1.0f;
        nlg = 0;
        run_seconds(20);
        int nb = collect_onsets(b, 20000);
        CHECK(na == nb, "%s: %d onsets, straight %d", what, nb, na);
        float tickp = SR / g.t.rate_hz;
        int bad = 0, shifted = 0;
        static bool used[20000];
        memset(used, 0, sizeof used);
        int from = 0;
        for (int i = 0; i < na; ++i) {
            /* the same onset in the shifted run: same part and note, at or after it, within a tick */
            int j = -1;
            while (from < nb && used[from]) from++;
            for (int k = from; k < nb && b[k].sample <= a[i].sample + (long)tickp; ++k)
                if (!used[k] && b[k].part == a[i].part && b[k].note == a[i].note && b[k].sample >= a[i].sample) {
                    j = k;
                    break;
                }
            if (j < 0) { bad++; continue; }
            used[j] = true;
            tg_meter_pos m;
            tg_meter_at(&g, a[i].tick, &m);
            float want;
            if (mode)
                want = tickp * (float)g.division * (m.beat == 0 ? 0.15f : m.beat == 2 ? 0.07f : 0.0f);
            else
                want = m.sub == g.division / 2 ? tickp * (float)g.division / 6.0f : 0.0f;
            long d = b[j].sample - a[i].sample;
            if (labs(d - (long)want) > 1 + (long)(0.04f * want)) bad++;
            shifted += d > 0;
        }
        CHECK(bad == 0, "%s: %d onsets not where they belong", what, bad);
        CHECK(shifted > 10 && shifted < na - 10, "%s: %d of %d onsets shifted", what, shifted, na);
    }
}

int main(void)
{
    test_meters();
    test_swing_lilt();
    printf("tuna_mech: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
