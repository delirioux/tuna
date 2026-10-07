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

/* ---- harmony ---------------------------------------------------------------------------------------- */
static int pc_of(int note, int root) { return ((note - root) % 12 + 12) % 12; }

/* every vocabulary: the tonic first, the whole-tone pair in place, applied chords resolving inside it */
static void test_vocabs(void)
{
    for (int v = 0; v < TG_VOC_COUNT; ++v)
        for (int m = 0; m < 2; ++m) {
            const tg_chord_def *d = tg_vocab_of(v, m);
            CHECK(d[0].func == TF_T && d[0].semis == 0, "vocab %d/%d: index 0 is the tonic", v, m);
            CHECK(d[TG_VOC_AUG_I].quality == TQ_AUG && d[TG_VOC_AUG_BVII].quality == TQ_AUG, "vocab %d/%d: aug pair", v, m);
            for (int k = 0; k < TG_VOCAB; ++k) {
                CHECK(d[k].name != NULL, "vocab %d/%d entry %d unnamed", v, m, k);
                if (d[k].func == TF_SEC)
                    CHECK(d[k].target >= 0 && d[k].target < TG_VOCAB && d[d[k].target].func != TF_SEC,
                          "vocab %d/%d %s resolves to %d", v, m, d[k].name, d[k].target);
                CHECK(d[k].quality < TQ_COUNT, "vocab %d/%d entry %d quality", v, m, k);
            }
        }
}

/* a test signature: Mozart's, changed */
static tg_style ts;
static void style_from(int base)
{
    ts = *tg_style_get(base);
}

/* run and check the notes: in key (the period's scale, the chord, the bent scale, the melody's own) */
typedef struct {
    long notes, off_key, arrivals, arr_tonic_chord, arr_tonic_ok, arr_open_ok, chord_changes;
    long coloured_notes, colour_bad;
} hstat;

static void run_checked(float secs, hstat *h, void (*per_tick)(void))
{
    memset(h, 0, sizeof *h);
    long n = (long)(secs * SR) / BLOCK;
    long last_tick = -1;
    for (long i = 0; i < n; ++i) {
        nlg = 0;
        tg_process(&g, BLOCK, cb, NULL);
        for (long k = 0; k < nlg; ++k) {
            const tg_event *e = &lg[k].e;
            if (!e->on || e->type != TG_EV_NOTE || e->part >= TG_PART_DRUMS)
                continue;
            h->notes++;
            uint16_t allowed = tt_scale_mask(g.period_scale >= 0 ? g.period_scale : g.scale) | g.chord_scale | g.mel_scale;
            for (int c = 0; c < g.chord.n; ++c)
                allowed |= (uint16_t)(1u << ((g.chord.root + g.chord.tone[c]) % 12));
            int pc = pc_of(e->note, g.root);
            bool leaning = e->part == TG_PART_MELODY && g.mel_pending >= 0;
            if (!((allowed >> pc) & 1) && !leaning) {
                if (h->off_key < 3)
                    printf("  off key: part %d note %d pc %d scale %d period_scale %d chord root %d\n", e->part, e->note,
                           pc, g.scale, g.period_scale, g.chord.root);
                h->off_key++;
            }
            if (g.period_scale >= 0) {
                h->coloured_notes++;
                h->colour_bad += !((tt_scale_mask(g.period_scale) | g.chord_scale | g.mel_scale) >> pc & 1);
            }
            if (e->part == TG_PART_MELODY && g.ticks > 1 && tg_period_position(&g, NULL) == 0 && lg[k].tick != last_tick) {
                last_tick = lg[k].tick;
                h->arrivals++;
                int croot = g.chord.root;
                if (croot == 0 && !g.open_prev) {
                    h->arr_tonic_chord++;
                    h->arr_tonic_ok += pc == 0;
                } else {
                    bool tone = false;
                    for (int c = 0; c < 3 && c < g.chord.n; ++c)
                        tone |= (g.chord.root + g.chord.tone[c]) % 12 == pc;
                    h->arr_open_ok += tone;
                }
            }
        }
        if (per_tick)
            per_tick();
    }
}

static void check_plan(void)
{
    /* at each period start: the planned functions are where they belong */
    static long seen = -1;
    if (g.style->harmony != TG_HARM_PLANNED || tg_period_position(&g, NULL) != 0 || g.period_index == seen)
        return;
    seen = g.period_index;
    const tg_chord_def *v = tg_vocab_of(g.style->vocab, g.plan_minor);
    int n = g.plan_len;
    if (n < 4 || g.key_pending)
        return;
    CHECK(v[g.plan[n / 2 - 1]].func == TF_D, "half cadence slot is %s", v[g.plan[n / 2 - 1]].name);
    CHECK(v[g.plan[n - 1]].func == (g.plagal ? TF_S : TF_D), "last slot is %s", v[g.plan[n - 1]].name);
    for (int i = 0; i + 1 < n; ++i)
        if (v[g.plan[i]].func == TF_SEC)
            CHECK(g.plan[i + 1] == v[g.plan[i]].target, "%s goes to %s", v[g.plan[i]].name, v[g.plan[i + 1]].name);
}

static void test_harmony_models(void)
{
    hstat h;
    /* each vocabulary, planned, every chord allowed: functions in place, everything in key */
    for (int voc = 0; voc < TG_VOC_COUNT; ++voc)
        for (int minor = 0; minor < 2; ++minor) {
            style_from(TG_STYLE_MOZART);
            ts.vocab = (uint8_t)voc;
            ts.scale = minor ? TS_AEOLIAN : TS_IONIAN;
            if (voc == TG_VOC_MODAL)
                ts.scale = minor ? TS_DORIAN : TS_MIXOLYDIAN;
            ts.pad_tones = 4;
            for (int k = 0; k < TG_VOCAB; ++k)
                ts.w_major[k] = ts.w_minor[k] = k == TG_VOC_AUG_I || k == TG_VOC_AUG_BVII ? 0 : 5;
            for (int sd = 1; sd <= 3; ++sd) {
                tg_init(&g, (uint32_t)sd, SR);
                tg_apply_style_def(&g, &ts);
                tg_reseed(&g, (uint32_t)sd);
                run_checked(90, &h, check_plan);
                CHECK(h.off_key == 0, "vocab %d %s seed %d: %ld of %ld notes off key", voc, minor ? "minor" : "major", sd,
                      h.off_key, h.notes);
                CHECK(h.arrivals > 3 && h.arr_tonic_ok == h.arr_tonic_chord && h.arr_tonic_chord == h.arrivals,
                      "vocab %d seed %d: arrivals %ld, on the tonic %ld/%ld", voc, sd, h.arrivals, h.arr_tonic_ok,
                      h.arr_tonic_chord);
            }
        }

    /* a modal vocabulary builds its chords from the mode */
    {
        style_from(TG_STYLE_MOZART);
        ts.vocab = TG_VOC_MODAL;
        ts.scale = TS_PHRYGIAN;
        tg_init(&g, 1, SR);
        tg_apply_style_def(&g, &ts);
        tg_reseed(&g, 1);
        tt_chord c;
        g.plan[1] = 8;                                   /* the Phrygian cadence chord: II on the flat second */
        tg_chord_at(&g, 1, 3, &c);
        CHECK(c.root == 1 && c.tone[1] == 4 && c.tone[2] == 7, "Phrygian II is the major flat second: %d %d %d",
              c.root, c.tone[1], c.tone[2]);
    }

    /* a fixed cycle, and a second one taking over every two periods */
    {
        style_from(TG_STYLE_MOZART);
        ts.harmony = TG_HARM_CYCLE;
        ts.vocab = TG_VOC_CLASSIC;
        static const int8_t pach[8] = {0, 6, 1, 2, 3, 0, 3, 6};    /* I V vi iii IV I IV V */
        memcpy(ts.cycle, pach, 8);
        ts.cycle_len = 8;
        ts.cycle2[0] = 0; ts.cycle2[1] = 3; ts.cycle2_len = 2;
        ts.cycle_switch = 2;
        ts.chord_beats = 2;
        ts.period_bars = 4;
        tg_init(&g, 4, SR);
        tg_apply_style_def(&g, &ts);
        tg_reseed(&g, 4);
        int bad = 0, changes = 0;
        long last_step = -1;
        for (long i = 0; i < (long)(120 * SR) / BLOCK; ++i) {
            tg_process(&g, BLOCK, cb, NULL);
            if (g.chord_step != last_step) {
                last_step = g.chord_step;
                const tg_chord_def *v = tg_vocab_of(TG_VOC_CLASSIC, false);
                int sel = (int)((g.period_index / 2) % 2);
                int want = sel ? ts.cycle2[g.chord_step % 2] : pach[g.chord_step % 8];
                bad += g.chord.root != v[want].semis;
                changes++;
            }
        }
        CHECK(changes > 50 && bad == 0, "cycle followed: %d of %d chords off", bad, changes);
    }

    /* a drone: one chord throughout */
    {
        style_from(TG_STYLE_MOZART);
        ts.harmony = TG_HARM_DRONE;
        ts.vocab = TG_VOC_MODAL;
        ts.scale = TS_DORIAN;
        ts.cycle[0] = 10;                                  /* the open fifth */
        ts.cycle_len = 1;
        tg_init(&g, 2, SR);
        tg_apply_style_def(&g, &ts);
        tg_reseed(&g, 2);
        run_checked(60, &h, NULL);
        CHECK(g.chord.root == 0 && g.chord.n >= 2 && g.chord.tone[1] == 7, "the drone is the open fifth");
        CHECK(h.off_key == 0 && h.arrivals > 2 && h.arr_tonic_ok == h.arrivals, "drone: in key %ld, arrivals %ld/%ld",
              h.off_key, h.arr_tonic_ok, h.arrivals);
    }

    /* open endings: some periods land on another chord, on one of its tones */
    {
        style_from(TG_STYLE_MOZART);
        ts.vocab = TG_VOC_ROMANTIC;
        ts.open = 0.5f;
        long open_total = 0, arrivals = 0, ok = 0, tonic_ok = 0, tonic = 0;
        for (int sd = 1; sd <= 4; ++sd) {
            tg_init(&g, (uint32_t)sd, SR);
            tg_apply_style_def(&g, &ts);
            tg_reseed(&g, (uint32_t)sd);
            run_checked(120, &h, check_plan);
            CHECK(h.off_key == 0, "open seed %d: %ld off key", sd, h.off_key);
            open_total += g.stat_open;
            arrivals += h.arrivals;
            ok += h.arr_open_ok;
            tonic_ok += h.arr_tonic_ok;
            tonic += h.arr_tonic_chord;
        }
        CHECK(open_total > 3, "open endings happen: %ld", open_total);
        CHECK(tonic_ok == tonic && ok == arrivals - tonic, "open: tonic arrivals %ld/%ld, open arrivals on chord tones %ld/%ld",
              tonic_ok, tonic, ok, arrivals - tonic);
    }

    /* colour: octatonic periods, Lydian periods, the parallel mode */
    static const int colours[3] = {TS_OCT_HW, TS_LYDIAN, -1};
    for (int ci = 0; ci < 3; ++ci) {
        style_from(TG_STYLE_MOZART);
        if (colours[ci] >= 0) {
            ts.color_scale = (uint8_t)colours[ci];
            ts.color = 0.6f;
        } else {
            ts.mixture = 0.5f;
        }
        long coloured = 0;
        for (int sd = 1; sd <= 3; ++sd) {
            tg_init(&g, (uint32_t)sd, SR);
            tg_apply_style_def(&g, &ts);
            tg_reseed(&g, (uint32_t)sd);
            run_checked(120, &h, NULL);
            CHECK(h.off_key == 0 && h.colour_bad == 0, "colour %d seed %d: off key %ld, outside the colour %ld",
                  colours[ci], sd, h.off_key, h.colour_bad);
            CHECK(h.arr_tonic_ok == h.arr_tonic_chord && h.arr_tonic_chord == h.arrivals,
                  "colour %d seed %d: arrivals on the tonic %ld/%ld", colours[ci], sd, h.arr_tonic_ok, h.arrivals);
            coloured += h.coloured_notes;
        }
        CHECK(coloured > 50, "colour %d: %ld notes in coloured periods", colours[ci], coloured);
    }

    /* side-slips */
    {
        style_from(TG_STYLE_MOZART);
        ts.sideslip = 0.3f;
        tg_init(&g, 5, SR);
        tg_apply_style_def(&g, &ts);
        tg_reseed(&g, 5);
        run_checked(120, &h, check_plan);
        CHECK(g.stat_slips > 3 && h.off_key == 0, "side-slips %ld, off key %ld", g.stat_slips, h.off_key);
    }

    /* the style's own modulations: away and home, always in the key of the moment */
    {
        style_from(TG_STYLE_MOZART);
        ts.modulate = 0.4f;
        long mods = 0;
        for (int sd = 1; sd <= 3; ++sd) {
            tg_init(&g, (uint32_t)sd, SR);
            tg_apply_style_def(&g, &ts);
            tg_reseed(&g, (uint32_t)sd);
            run_checked(180, &h, check_plan);
            CHECK(h.off_key == 0, "modulate seed %d: %ld off key", sd, h.off_key);
            CHECK(h.arr_tonic_ok == h.arrivals, "modulate seed %d: arrivals on the tonic %ld/%ld", sd, h.arr_tonic_ok,
                  h.arrivals);
            mods += g.stat_modulations;
        }
        CHECK(mods > 4, "modulations happen: %ld", mods);
    }
}

int main(void)
{
    test_meters();
    test_swing_lilt();
    test_vocabs();
    test_harmony_models();
    printf("tuna_mech: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
