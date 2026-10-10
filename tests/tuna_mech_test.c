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
            uint16_t allowed = tt_scale_mask(g.period_scale >= 0 ? g.period_scale : g.scale) | tt_scale_mask(g.scale) |
                               g.chord_scale | g.mel_scale;
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

/* ---- melody, bass and texture modes ---------------------------------------------------------------- */
static void start_ts(uint32_t seed)
{
    tg_init(&g, seed, SR);
    tg_apply_style_def(&g, &ts);
    tg_reseed(&g, seed);
}

/* all events of a run, for the mode checks */
static void run_log(float secs)
{
    nlg = 0;
    long n = (long)(secs * SR) / BLOCK;
    for (long i = 0; i < n; ++i)
        tg_process(&g, BLOCK, cb, NULL);
}

static void test_melody_modes(void)
{
    hstat h;
    /* chant over a drone: no accents, the bass a held tonic, arrivals on the final */
    style_from(TG_STYLE_MOZART);
    ts.mel_mode = TG_MEL_CHANT;
    ts.harmony = TG_HARM_DRONE;
    ts.vocab = TG_VOC_MODAL;
    ts.scale = TS_DORIAN;
    ts.cycle[0] = 10;
    ts.cycle_len = 1;
    ts.bass_mode = TG_BASS_DRONE;
    ts.pad_mode = TG_PAD_ORGANUM;
    ts.division = 2;
    ts.melody_density = 0.6f;
    start_ts(3);
    run_checked(60, &h, NULL);
    CHECK(h.off_key == 0 && h.arrivals > 2 && h.arr_tonic_ok == h.arrivals, "chant: off key %ld, arrivals %ld/%ld",
          h.off_key, h.arr_tonic_ok, h.arrivals);
    start_ts(3);
    run_log(60);
    int bass_notes = 0, bass_pc_bad = 0, organum = 0, organum_bad = 0;
    int last_mel = -1;
    for (long i = 0; i < nlg; ++i) {
        const tg_event *e = &lg[i].e;
        if (!e->on || e->type != TG_EV_NOTE) continue;
        if (e->part == TG_PART_MELODY) last_mel = e->note;
        if (e->part == TG_PART_BASS) { bass_notes++; bass_pc_bad += pc_of(e->note, g.root) != 0; }
        if (e->part == TG_PART_PAD && last_mel >= 0) {
            organum++;
            bool bad = last_mel - e->note != 7 && last_mel - e->note != 6 && last_mel - e->note != 8;
            if (bad) printf("  organum mel %d pad %d tick %ld\n", last_mel, e->note, lg[i].tick);
            organum_bad += bad;
        }
    }
    CHECK(bass_notes > 0 && bass_notes < 30 && bass_pc_bad == 0, "drone bass: %d notes, %d off the tonic", bass_notes, bass_pc_bad);
    CHECK(organum > 20 && organum_bad == 0, "organum: %d notes, %d not a fifth below", organum, organum_bad);

    /* tintinnabuli: the M-voice moves by step toward the tonic, the T-voice holds to the tonic triad below */
    style_from(TG_STYLE_MOZART);
    ts.mel_mode = TG_MEL_TINTI;
    ts.pad_mode = TG_PAD_TINTI;
    ts.harmony = TG_HARM_DRONE;
    ts.scale = TS_AEOLIAN;
    ts.root = 9;
    ts.bass_mode = TG_BASS_TOLL;
    start_ts(1);
    run_log(90);
    int steps = 0, leaps = 0, tvoice = 0, tbad = 0, ends = 0, ends_tonic = 0;
    last_mel = -1;
    for (long i = 0; i < nlg; ++i) {
        const tg_event *e = &lg[i].e;
        if (e->type != TG_EV_NOTE) continue;
        if (e->part == TG_PART_MELODY && !e->on) {
            bool restarts = false;               /* a release before a new note is not a phrase end */
            for (long j = i + 1; j < nlg && lg[j].tick == lg[i].tick; ++j)
                restarts |= lg[j].e.part == TG_PART_MELODY && lg[j].e.on;
            if (!restarts) {
                ends++;
                ends_tonic += pc_of(e->note, g.root) == 0;
            }
        }
        if (!e->on) continue;
        if (e->part == TG_PART_MELODY) {
            if (last_mel >= 0) { int d = abs(e->note - last_mel); if (d <= 2) steps++; else leaps++; }
            last_mel = e->note;
        }
        if (e->part == TG_PART_PAD) {
            tvoice++;
            int pc = pc_of(e->note, g.root);
            tbad += !(pc == 0 || pc == 3 || pc == 7) || e->note >= last_mel;
        }
    }
    CHECK(steps > 40 && leaps < steps / 3, "tinti M-voice: %d steps, %d leaps (phrase restarts)", steps, leaps);
    CHECK(tvoice > 40 && tbad == 0, "tinti T-voice: %d notes, %d off the triad or not below", tvoice, tbad);
    CHECK(ends > 5 && ends_tonic == ends, "tinti phrases end on the tonic: %d/%d", ends_tonic, ends);

    /* a pattern, and its phase-shifted twin */
    style_from(TG_STYLE_MOZART);
    static const int8_t piano_phase[12] = {0, 1, 4, 5, 6, 1, 0, 5, 4, 1, 6, 5};
    memcpy(ts.pattern, piano_phase, 12);
    ts.pattern_len = 12;
    ts.mel_mode = TG_MEL_PATTERN;
    ts.pad_mode = TG_PAD_PHASE;
    ts.phase_reps = 4;
    ts.harmony = TG_HARM_DRONE;
    ts.scale = TS_DORIAN;
    ts.root = 4;
    ts.bass_mode = TG_BASS_NONE;
    start_ts(2);
    run_log(40);
    int mel[4000], nm = 0, pad[4000], np = 0, bass = 0;
    for (long i = 0; i < nlg; ++i) {
        const tg_event *e = &lg[i].e;
        if (!e->on || e->type != TG_EV_NOTE) continue;
        if (e->part == TG_PART_MELODY && nm < 4000) mel[nm++] = e->note;
        if (e->part == TG_PART_PAD && np < 4000) pad[np++] = e->note;
        bass += e->part == TG_PART_BASS;
    }
    int mbad = 0;
    for (int i = 12; i < nm; ++i) mbad += mel[i] != mel[i - 12];
    CHECK(nm > 100 && mbad == 0, "pattern loops exactly: %d notes, %d off", nm, mbad);
    CHECK(bass == 0, "no bass: %d", bass);
    /* the twin drifts: at first in unison, later ahead */
    int same_early = 0, same_late = 0;
    for (int i = 0; i < 12 && i < np; ++i) same_early += pad[i] == mel[i];
    for (int i = np - 12; i < np && i > 0; ++i) same_late += pad[i] == mel[i];
    CHECK(same_early == 12 && same_late < 12 && g.phase_off > 0, "phase: unison %d/12 at first, %d/12 later, drift %d",
          same_early, same_late, g.phase_off);

    /* birdsong: high, in bursts */
    style_from(TG_STYLE_MOZART);
    ts.mel_mode = TG_MEL_BIRDSONG;
    ts.scale = TS_MESSIAEN3;
    ts.harmony = TG_HARM_DRONE;
    start_ts(4);
    run_checked(40, &h, NULL);
    CHECK(h.off_key == 0, "birdsong in the mode: %ld off", h.off_key);
    start_ts(4);
    run_log(40);
    int low = 0, birds = 0;
    for (long i = 0; i < nlg; ++i)
        if (lg[i].e.on && lg[i].e.part == TG_PART_MELODY && lg[i].e.type == TG_EV_NOTE) {
            birds++;
            low += lg[i].e.note < ts.melody_base + 12 - 2;
        }
    CHECK(birds > 50 && low == 0, "birdsong: %d notes, %d below the high register", birds, low);

    /* the phrase's options: staccato, long notes, a falling line, octave leaps, turns */
    style_from(TG_STYLE_MOZART);
    ts.staccato = 0.8f;
    start_ts(5);
    run_log(30);
    long on_at[128], shorts = 0, longs = 0;
    for (int i = 0; i < 128; ++i) on_at[i] = -1;
    for (long i = 0; i < nlg; ++i) {
        const tg_event *e = &lg[i].e;
        if (e->part != TG_PART_MELODY || e->type != TG_EV_NOTE) continue;
        if (e->on) on_at[e->note] = lg[i].tick;
        else if (on_at[e->note] >= 0) { long len = lg[i].tick - on_at[e->note]; shorts += len <= 1; longs += len > 1; on_at[e->note] = -1; }
    }
    CHECK(shorts > 3 * longs, "staccato: %ld short, %ld long", shorts, longs);

    style_from(TG_STYLE_MOZART);
    ts.mel_min_ticks = 8;
    ts.appoggiatura = 0;
    ts.trill = false;
    start_ts(5);
    run_log(60);
    long prev_tick = -1, too_short = 0, mnotes = 0;
    for (long i = 0; i < nlg; ++i) {
        const tg_event *e = &lg[i].e;
        if (e->part != TG_PART_MELODY || !e->on || e->type != TG_EV_NOTE) continue;
        int period, pos = 0;
        (void)period; (void)pos;
        if (prev_tick >= 0 && lg[i].tick - prev_tick < 8) too_short++;
        prev_tick = lg[i].tick;
        mnotes++;
    }
    CHECK(mnotes > 20 && too_short < mnotes / 5, "long notes: %ld of %ld shorter than 8 ticks (cadences only)", too_short, mnotes);

    for (int d = -1; d <= 1; d += 2) {
        style_from(TG_STYLE_MOZART);
        ts.direction = (float)d;
        start_ts(6);
        run_log(90);
        long up = 0, down = 0;
        int lm = -1;
        for (long i = 0; i < nlg; ++i) {
            const tg_event *e = &lg[i].e;
            if (e->part != TG_PART_MELODY || !e->on || e->type != TG_EV_NOTE) continue;
            if (lm >= 0) { up += e->note > lm; down += e->note < lm; }
            lm = e->note;
        }
        CHECK(d < 0 ? down > up : up > down, "direction %d: %ld up, %ld down", d, up, down);
    }

    style_from(TG_STYLE_MOZART);
    ts.octave = 0.6f;
    ts.melody_range = 24;
    start_ts(7);
    run_checked(60, &h, NULL);
    CHECK(h.off_key == 0 && h.arr_tonic_ok == h.arrivals, "octaves: in key, arrivals %ld/%ld", h.arr_tonic_ok, h.arrivals);
    start_ts(7);
    run_log(60);
    long octs = 0;
    int lm = -1;
    for (long i = 0; i < nlg; ++i) {
        const tg_event *e = &lg[i].e;
        if (e->part != TG_PART_MELODY || !e->on || e->type != TG_EV_NOTE) continue;
        octs += lm >= 0 && abs(e->note - lm) == 12;
        lm = e->note;
    }
    CHECK(octs > 5, "octave leaps: %ld", octs);

    style_from(TG_STYLE_CHOPIN);                       /* division 3: room for a turn */
    ts.turn = 0.5f;
    ts.appoggiatura = 0;
    start_ts(8);
    run_checked(60, &h, NULL);
    CHECK(h.off_key == 0 && h.arr_tonic_ok == h.arrivals, "turns: in key %ld, arrivals %ld/%ld", h.off_key, h.arr_tonic_ok,
          h.arrivals);
    start_ts(8);
    run_log(120);                                       /* (two minutes: a handful a minute, whatever the density) */
    int seq[8], ns = 0;
    long turns = 0;
    for (long i = 0; i < nlg; ++i) {
        const tg_event *e = &lg[i].e;
        if (e->part != TG_PART_MELODY || !e->on || e->type != TG_EV_NOTE) continue;
        if (ns == 4) { memmove(seq, seq + 1, 3 * sizeof(int)); ns = 3; }
        seq[ns++] = e->note;
        if (ns == 4 && seq[0] > seq[1] && seq[2] < seq[1] && seq[3] == seq[1] && seq[0] - seq[1] <= 2 && seq[1] - seq[2] <= 2)
            turns++;
    }
    CHECK(turns > 5, "turns heard: %ld", turns);
}

static void test_bass_pad_modes(void)
{
    hstat h;
    /* ostinato: the habanera rhythm in 2/4 sixteenths: ticks 0, 3, 4, 6 */
    style_from(TG_STYLE_MOZART);
    ts.beats_per_bar = 2;
    ts.division = 4;
    ts.chord_beats = 2;
    ts.period_bars = 8;
    ts.bass_mode = TG_BASS_OSTINATO;
    ts.bass_rhythm = (1u << 0) | (1u << 3) | (1u << 4) | (1u << 6);
    static const int8_t hab[4] = {0, 0, 2, 0};
    memcpy(ts.bass_notes, hab, 4);
    ts.bass_len = 4;
    start_ts(2);
    run_checked(40, &h, NULL);
    CHECK(h.off_key == 0, "ostinato in key: %ld", h.off_key);
    start_ts(2);
    run_log(40);
    int hits = 0, wrong_tick = 0;
    for (long i = 0; i < nlg; ++i)
        if (lg[i].e.on && lg[i].e.part == TG_PART_BASS && lg[i].e.type == TG_EV_NOTE) {
            hits++;
            int t = (int)(lg[i].tick % 8);
            wrong_tick += !(t == 0 || t == 3 || t == 4 || t == 6);
        }
    CHECK(hits > 50 && wrong_tick == 0, "habanera bass: %d hits, %d off the rhythm", hits, wrong_tick);

    /* a lament line over a cycle: G F# F E Eb D */
    style_from(TG_STYLE_MOZART);
    ts.vocab = TG_VOC_MODAL;
    ts.harmony = TG_HARM_CYCLE;
    ts.root = 7;
    ts.scale = TS_AEOLIAN;
    static const int8_t lam_c[6] = {0, 7, 13, 12, 3, 7};       /* i V bVII IV iv V */
    memcpy(ts.cycle, lam_c, 6);
    ts.cycle_len = 6;
    static const int8_t lam_b[6] = {0, -1, -2, -3, -4, -5};
    memcpy(ts.bass_notes, lam_b, 6);
    ts.bass_len = 6;
    ts.bass_mode = TG_BASS_LINE;
    ts.beats_per_bar = 3;
    ts.division = 2;
    ts.chord_beats = 3;
    ts.period_bars = 6;
    start_ts(3);
    run_log(60);
    int line[64], nl = 0;
    for (long i = 0; i < nlg && nl < 64; ++i)
        if (lg[i].e.on && lg[i].e.part == TG_PART_BASS && lg[i].e.type == TG_EV_NOTE)
            line[nl++] = pc_of(lg[i].e.note, 7);
    int lbad = 0;
    for (int i = 0; i < nl; ++i) lbad += line[i] != (12 - i % 6) % 12;
    CHECK(nl >= 12 && lbad == 0, "lament line: %d notes, %d off", nl, lbad);

    /* repeated eighths, tolling, alternating beats */
    int modes[3] = {TG_BASS_REPEAT, TG_BASS_TOLL, TG_BASS_BEAT};
    for (int mi = 0; mi < 3; ++mi) {
        style_from(TG_STYLE_MOZART);
        ts.bass_mode = (uint8_t)modes[mi];
        start_ts(4);
        run_checked(30, &h, NULL);
        CHECK(h.off_key == 0, "bass mode %d in key", modes[mi]);
        start_ts(4);
        run_log(30);
        int n = 0, bad = 0;
        for (long i = 0; i < nlg; ++i) {
            if (!lg[i].e.on || lg[i].e.part != TG_PART_BASS || lg[i].e.type != TG_EV_NOTE) continue;
            n++;
            long t = lg[i].tick;
            if (modes[mi] == TG_BASS_REPEAT) bad += t % 2 != 0;
            if (modes[mi] == TG_BASS_TOLL) bad += t % 32 != 0;
            if (modes[mi] == TG_BASS_BEAT) bad += t % 4 != 0;
        }
        CHECK(n > 5 && bad == 0, "bass mode %d: %d notes, %d off their ticks", modes[mi], n, bad);
    }

    /* oom-pah in 3/4: chords on beats 2 and 3 */
    style_from(TG_STYLE_BRAHMS);
    ts.pad_mode = TG_PAD_OOMPAH;
    ts.hemiola = false;
    start_ts(5);
    run_log(40);
    int pahs = 0, pbad = 0;
    for (long i = 0; i < nlg; ++i)
        if (lg[i].e.on && lg[i].e.part == TG_PART_PAD && lg[i].e.type == TG_EV_NOTE) {
            pahs++;
            int t = (int)(lg[i].tick % 6);
            pbad += !(t == 2 || t == 4);
        }
    CHECK(pahs > 50 && pbad == 0, "oom-pah-pah: %d chord notes, %d off beats 2 and 3", pahs, pbad);

    /* tremolo, figure, additive, canon, spread, polychord, strum: in key, and their shapes */
    style_from(TG_STYLE_MOZART);
    ts.pad_mode = TG_PAD_TREMOLO;
    start_ts(6);
    run_checked(20, &h, NULL);
    CHECK(h.off_key == 0, "tremolo in key");

    style_from(TG_STYLE_MOZART);
    ts.pad_mode = TG_PAD_FIGURE;
    static const int8_t prel[8] = {0, 1, 2, 3, 4, 2, 3, 4};      /* Bach's C major prelude */
    memcpy(ts.pad_figure, prel, 8);
    ts.figure_len = 8;
    start_ts(6);
    run_checked(30, &h, NULL);
    CHECK(h.off_key == 0, "figure in key");

    style_from(TG_STYLE_MOZART);
    ts.pad_mode = TG_PAD_ADDITIVE;
    static const int8_t glass[6] = {0, 1, 2, 3, 2, 1};
    memcpy(ts.pad_figure, glass, 6);
    ts.figure_len = 6;
    ts.phase_reps = 2;
    start_ts(6);
    int lens_seen = 0, seen_mask = 0;
    for (long i = 0; i < (long)(40 * SR) / BLOCK; ++i) {
        tg_process(&g, BLOCK, cb, NULL);
        seen_mask |= 1 << g.add_len;
    }
    for (int k = 0; k < 8; ++k) lens_seen += (seen_mask >> k) & 1;
    CHECK(lens_seen >= 4, "additive: figure lengths seen %x", seen_mask);

    style_from(TG_STYLE_MOZART);
    ts.pad_mode = TG_PAD_CANON;
    ts.canon_ticks = 32;
    ts.canon_steps = -7;
    start_ts(7);
    run_checked(40, &h, NULL);
    CHECK(h.off_key == 0, "canon in key: %ld", h.off_key);
    start_ts(7);
    run_log(40);
    /* the canon voice repeats the melody's pitch classes 32 ticks later (an octave down), mostly */
    long cm = 0, cmatch = 0;
    static int mel_at[20000];
    memset(mel_at, -1, sizeof mel_at);
    for (long i = 0; i < nlg; ++i)
        if (lg[i].e.on && lg[i].e.part == TG_PART_MELODY && lg[i].e.type == TG_EV_NOTE && lg[i].tick < 20000)
            mel_at[lg[i].tick] = lg[i].e.note;
    for (long i = 0; i < nlg; ++i)
        if (lg[i].e.on && lg[i].e.part == TG_PART_PAD && lg[i].e.type == TG_EV_NOTE && lg[i].tick >= 32) {
            int src = mel_at[lg[i].tick - 32];
            if (src < 0) continue;
            cm++;
            cmatch += abs(src - 12 - lg[i].e.note) <= 2;
        }
    CHECK(cm > 30 && cmatch * 10 >= cm * 8, "canon follows the melody an octave down: %ld of %ld", cmatch, cm);

    style_from(TG_STYLE_MOZART);
    ts.pad_mode = TG_PAD_BLOCK;
    ts.spread = true;
    ts.poly_semis = 6;
    start_ts(8);
    run_log(20);
    CHECK(g.pad_n == 6 && g.pad[g.pad_n - 1] - g.pad[0] > 12, "spread polychord: %d notes over %d semitones", g.pad_n,
          g.pad[g.pad_n - 1] - g.pad[0]);

    style_from(TG_STYLE_MOZART);
    ts.pad_mode = TG_PAD_REPEAT;
    ts.strum_ms = 20;
    start_ts(9);
    run_log(10);
    long first = -1, spread_max = 0;
    long tick_now = -1;
    for (long i = 0; i < nlg; ++i)
        if (lg[i].e.on && lg[i].e.part == TG_PART_PAD && lg[i].e.type == TG_EV_NOTE) {
            if (lg[i].tick != tick_now) { tick_now = lg[i].tick; first = lg[i].sample; }
            if (lg[i].sample - first > spread_max) spread_max = lg[i].sample - first;
        }
    CHECK(spread_max >= (long)(0.035f * SR) && spread_max <= (long)(0.045f * SR), "strum spreads a chord over %ld samples",
          spread_max);
}

static void test_form(void)
{
    /* contrast: every other period other modes and tempo */
    style_from(TG_STYLE_MOZART);
    ts.alt_every = 1;
    ts.alt.bpm = 0.6f;
    ts.alt.pad_mode = TG_PAD_BLOCK + 1;
    ts.alt.bass_mode = TG_BASS_DRONE + 1;
    start_ts(1);
    int switches = 0;
    bool last = false;
    float bpm_alt = 0;
    for (long i = 0; i < (long)(120 * SR) / BLOCK; ++i) {
        tg_process(&g, BLOCK, cb, NULL);
        if (g.alt_on != last) { switches++; last = g.alt_on; if (last) bpm_alt = g.bpm; }
    }
    CHECK(switches >= 2 && fabsf(bpm_alt - ts.bpm * 0.6f) < 0.01f, "contrast: %d switches, tempo %.1f", switches, bpm_alt);

    /* general pause: a bar of silence after some arrivals */
    style_from(TG_STYLE_MOZART);
    ts.gp = 1.0f;
    start_ts(2);
    run_log(60);
    long silent_ok = 0, gps = 0;
    for (long i = 0; i < nlg; ++i)
        if (lg[i].e.on && lg[i].e.type == TG_EV_NOTE && g.gp_from >= 0 && lg[i].tick >= g.gp_from && lg[i].tick < g.gp_until)
            gps++;
    silent_ok = gps == 0;
    CHECK(silent_ok && g.gp_from > 0, "general pause silent (%ld notes inside)", gps);

    /* echo periods are softer */
    style_from(TG_STYLE_MOZART);
    ts.echo = 0.5f;
    start_ts(3);
    long sum_echo = 0, n_echo = 0, sum_norm = 0, n_norm = 0;
    for (long i = 0; i < (long)(120 * SR) / BLOCK; ++i) {
        nlg = 0;
        tg_process(&g, BLOCK, cb, NULL);
        for (long k = 0; k < nlg; ++k)
            if (lg[k].e.on && lg[k].e.type == TG_EV_NOTE) {
                if (g.echo_vel) { sum_echo += lg[k].e.velocity; n_echo++; }
                else { sum_norm += lg[k].e.velocity; n_norm++; }
            }
    }
    CHECK(n_echo > 20 && n_norm > 20 && sum_echo * n_norm < sum_norm * n_echo - 10 * n_echo * n_norm,
          "echo softer: %.1f against %.1f", n_echo ? (double)sum_echo / n_echo : 0, n_norm ? (double)sum_norm / n_norm : 0);

    /* tuning: each note carries its cents */
    style_from(TG_STYLE_MOZART);
    ts.tuning = TT_TUNE_MEANTONE;
    start_ts(4);
    run_log(20);
    long tn = 0, tbad = 0;
    for (long i = 0; i < nlg; ++i)
        if (lg[i].e.on && lg[i].e.type == TG_EV_NOTE && lg[i].e.part < TG_PART_DRUMS) {
            tn++;
            tbad += lg[i].e.value != tt_tuning_cents10_key(TT_TUNE_MEANTONE, lg[i].e.note - ts.root, 0);
        }
    CHECK(tn > 50 && tbad == 0, "tuning cents on %ld notes, %ld wrong", tn, tbad);

    /* a drum pattern: the Bolero snare on its ticks only */
    style_from(TG_STYLE_MOZART);
    ts.beats_per_bar = 3;
    ts.division = 4;
    ts.perc_mode = TG_PERC_PATTERN;
    ts.drum_pat[0].note = TG_GM_SNARE;
    ts.drum_pat[0].vel = 70;
    ts.drum_pat[0].ticks = 0x0FFF & 0xDBF;  /* a figure over 12 sixteenths */
    start_ts(5);
    run_log(20);
    long dn = 0, dbad = 0;
    for (long i = 0; i < nlg; ++i)
        if (lg[i].e.on && lg[i].e.part == TG_PART_DRUMS && lg[i].e.type == TG_EV_NOTE) {
            dn++;
            dbad += !((0xDBF >> (lg[i].tick % 12)) & 1) || lg[i].e.note != TG_GM_SNARE;
        }
    CHECK(dn > 50 && dbad == 0, "drum pattern: %ld hits, %ld wrong", dn, dbad);
}

/* ---- instrumentation ---------------------------------------------------------------------------------- */
static void test_orchestration(void)
{
    /* fixed: one program per part, at the start */
    style_from(TG_STYLE_MOZART);
    ts.orch_n = 1;
    ts.orch[0][0] = TI_CELLO; ts.orch[0][1] = TI_FLUTE; ts.orch[0][2] = TI_HARP;
    start_ts(1);
    run_log(60);
    int progs = 0, first_tick_ok = 1;
    for (long i = 0; i < nlg; ++i)
        if (lg[i].e.type == TG_EV_PROGRAM) {
            progs++;
            first_tick_ok &= lg[i].tick == 0;
        }
    CHECK(progs == 3 && first_tick_ok, "fixed instruments: %d programs, at the start: %d", progs, first_tick_ok);
    /* the programs come before the first notes */
    long first_note = -1, last_prog = -1;
    for (long i = 0; i < nlg; ++i) {
        if (lg[i].e.type == TG_EV_PROGRAM) last_prog = i;
        else if (lg[i].e.type == TG_EV_NOTE && first_note < 0) first_note = i;
    }
    CHECK(last_prog < first_note, "programs before notes");

    /* by turns: the melody passes from instrument to instrument each period (Bolero) */
    style_from(TG_STYLE_MOZART);
    ts.orch_n = 4;
    ts.orch_mode = TG_ORCH_CYCLE;
    static const uint8_t mel[4] = {TI_FLUTE, TI_CLARINET, TI_BASSOON, TI_SAX};
    for (int k = 0; k < 4; ++k) { ts.orch[k][0] = TI_PIZZ; ts.orch[k][1] = mel[k]; ts.orch[k][2] = TI_STRINGS; }
    start_ts(2);
    run_log(120);
    int melprogs = 0, order_bad = 0, other = 0;
    for (long i = 0; i < nlg; ++i)
        if (lg[i].e.type == TG_EV_PROGRAM) {
            if (lg[i].e.part == TG_PART_MELODY) {
                order_bad += lg[i].e.value != mel[melprogs % 4];
                melprogs++;
            } else {
                other++;
            }
        }
    CHECK(melprogs >= 4 && order_bad == 0 && other == 2, "cycle: %d melody programs (%d out of turn), %d others", melprogs,
          order_bad, other);

    /* contrast periods bring the second set */
    style_from(TG_STYLE_MOZART);
    ts.orch_n = 2;
    ts.orch_mode = TG_ORCH_CONTRAST;
    ts.alt_every = 1;
    ts.orch[0][1] = TI_PIANO; ts.orch[1][1] = TI_CHOIR;
    start_ts(3);
    int mism = 0, seen = 0;
    for (long i = 0; i < (long)(120 * SR) / BLOCK; ++i) {
        tg_process(&g, BLOCK, cb, NULL);
        if (g.orch_cur[TG_PART_MELODY] >= 0) {
            seen++;
            mism += g.orch_cur[TG_PART_MELODY] != (g.alt_on ? TI_CHOIR : TI_PIANO);
        }
    }
    CHECK(seen > 0 && mism == 0, "contrast instruments follow the periods: %d mismatches", mism);
    for (int f = 0; f < TI_COUNT; ++f)
        CHECK(strlen(tg_family_name(f)) <= 8, "family name %s too long", tg_family_name(f));
}


/* ---- the chaos knobs on bass and pad ----------------------------------------------------------------------- */
typedef struct {
    long notes[3], ints[3], int_sum[3], off_key[3], spread_sum, spread_n, anchors;
    uint64_t h_anchor, h_bp, h_all;
    int prev[3];
    long tick_pad;
    int pad_lo, pad_hi;
    int active[3][128];
} part_stat;
static part_stat ps;
static void parts_cb(void *ctx, const tg_event *e)
{
    (void)ctx;
    if (e->type != TG_EV_NOTE || e->part > TG_PART_PAD)
        return;
    int p = e->part;
    uint8_t b[6] = {(uint8_t)g.ticks, (uint8_t)(g.ticks >> 8), e->part, e->note, e->velocity, e->on};
    for (int i = 0; i < 6; ++i) {
        ps.h_all = (ps.h_all ^ b[i]) * 1099511628211ull;
        if (p != TG_PART_MELODY)
            ps.h_bp = (ps.h_bp ^ b[i]) * 1099511628211ull;
    }
    if (!e->on) {
        if (ps.active[p][e->note] > 0)
            ps.active[p][e->note]--;
        return;
    }
    ps.active[p][e->note]++;
    ps.notes[p]++;
    if (ps.prev[p] >= 0) {
        ps.int_sum[p] += abs(e->note - ps.prev[p]);
        ps.ints[p]++;
    }
    ps.prev[p] = e->note;
    uint16_t allowed = tt_scale_mask(g.period_scale >= 0 ? g.period_scale : g.scale) | tt_scale_mask(g.scale) |
                       g.chord_scale | g.mel_scale;
    for (int k = 0; k < g.chord.n; ++k)
        allowed |= (uint16_t)(1u << ((g.chord.root + g.chord.tone[k]) % 12));
    if (p != TG_PART_MELODY)
        ps.off_key[p] += !((allowed >> pc_of(e->note, g.root)) & 1);
    if (p == TG_PART_BASS && g.cx_fresh) {               /* the bass on the chord change */
        ps.anchors++;
        uint8_t a[3] = {(uint8_t)g.ticks, (uint8_t)(g.ticks >> 8), e->note};
        for (int i = 0; i < 3; ++i)
            ps.h_anchor = (ps.h_anchor ^ a[i]) * 1099511628211ull;
    }
    if (p == TG_PART_PAD) {                              /* the voicing's span, chord by chord */
        if (ps.tick_pad != g.ticks) {
            if (ps.tick_pad >= 0 && ps.pad_hi > ps.pad_lo) {
                ps.spread_sum += ps.pad_hi - ps.pad_lo;
                ps.spread_n++;
            }
            ps.tick_pad = g.ticks;
            ps.pad_lo = ps.pad_hi = e->note;
        }
        if (e->note < ps.pad_lo) ps.pad_lo = e->note;
        if (e->note > ps.pad_hi) ps.pad_hi = e->note;
    }
}

static long parts_hanging;
static part_stat parts_run(int style, unsigned parts, float r, float pi, float d, float macro, float secs)
{
    tg_init(&g, 5, SR);
    tg_apply_style(&g, style);
    tg_set_chaos(&g, r, pi, d, macro);
    tg_set_chaos_parts(&g, parts);
    g.humanize = g.swing = g.lilt = 0.0f;                /* (events at once, so the callback sees their tick) */
    g.strum_ms = 0;
    tg_reseed(&g, 5);
    memset(&ps, 0, sizeof ps);
    ps.h_anchor = ps.h_bp = ps.h_all = 1469598103934665603ull;
    ps.prev[0] = ps.prev[1] = ps.prev[2] = -1;
    ps.tick_pad = -1;
    long n = (long)(secs * SR) / BLOCK;
    for (long i = 0; i < n; ++i)
        tg_process(&g, BLOCK, parts_cb, NULL);
    part_stat out = ps;
    tg_all_off(&g, parts_cb, NULL);                      /* everything released, moved notes too */
    parts_hanging = 0;
    for (int p = 0; p < 3; ++p)
        for (int k = 0; k < 128; ++k)
            parts_hanging += ps.active[p][k];
    return out;
}

static float mean_int(const part_stat *s, int p) { return (float)s->int_sum[p] / (float)(s->ints[p] ? s->ints[p] : 1); }
static float mean_spread(const part_stat *s) { return (float)s->spread_sum / (float)(s->spread_n ? s->spread_n : 1); }

static void test_hands_parts(void)
{
    const unsigned B = 1u << TG_PART_BASS, M = 1u << TG_PART_MELODY, P = 1u << TG_PART_PAD;
    static const int held[] = {TG_STYLE_BEETHOVEN, TG_STYLE_MAHLER, TG_STYLE_GERSHWIN, TG_STYLE_TCHAIKOVSKY};
    for (unsigned si = 0; si < sizeof held / sizeof held[0]; ++si) {
        int st = held[si];
        const char *nm = tg_style_get(st)->name;
        part_stat base = parts_run(st, TG_CHAOS_ALL, 0.5f, 0.5f, 0.5f, 0.5f, 120.0f);
        /* the knobs turned, but no part follows them: the composer exactly */
        part_stat none = parts_run(st, 0, 1.0f, 1.0f, 1.0f, 0.5f, 120.0f);
        CHECK(none.h_all == base.h_all, "%s: no part follows: not the composer", nm);
        /* melody only: the melody moves, nothing is added to bass or pad (they share its random draws, so
         * their choices may differ) */
        part_stat mel = parts_run(st, M, 1.0f, 0.0f, 1.0f, 0.5f, 120.0f);
        CHECK(mel.h_all != base.h_all && parts_hanging == 0, "%s: melody only", nm);
        (void)M;
        /* bass: DENSITY, PITCH, and the chord changes untouched */
        part_stat bd0 = parts_run(st, B, 0.5f, 0.5f, 0.0f, 0.5f, 120.0f);
        CHECK(parts_hanging == 0, "%s: bass density down: %ld notes left hanging", nm, parts_hanging);
        part_stat bd1 = parts_run(st, B, 0.5f, 0.5f, 1.0f, 0.5f, 120.0f);
        CHECK(parts_hanging == 0, "%s: bass density up: %ld notes left hanging", nm, parts_hanging);
        /* (a bass that only plays the chord changes has nothing to leave out) */
        CHECK(bd0.notes[0] <= base.notes[0] && base.notes[0] < bd1.notes[0], "%s: bass density %ld <= %ld < %ld", nm,
              bd0.notes[0], base.notes[0], bd1.notes[0]);
        part_stat bp1 = parts_run(st, B, 0.5f, 1.0f, 0.5f, 0.5f, 120.0f);
        part_stat bp0 = parts_run(st, B, 0.5f, 0.0f, 0.5f, 0.5f, 120.0f);
        CHECK(mean_int(&bp0, 0) <= mean_int(&base, 0) &&
                  (mean_int(&base, 0) < mean_int(&bp1, 0) || bp1.notes[0] == bp1.anchors),
              "%s: bass pitch: mean interval %.2f <= %.2f < %.2f", nm, mean_int(&bp0, 0), mean_int(&base, 0),
              mean_int(&bp1, 0));
        part_stat br1 = parts_run(st, B, 1.0f, 0.5f, 0.5f, 0.5f, 120.0f);
        CHECK(br1.h_bp != base.h_bp, "%s: bass rhythm up changed nothing", nm);
        part_stat bw = parts_run(st, B, 1.0f, 1.0f, 0.0f, 0.5f, 120.0f);
        CHECK(bw.h_anchor == base.h_anchor && bw.anchors == base.anchors && base.anchors > 0,
              "%s: the bass on the chord changes moved (%ld, the composer %ld)", nm, bw.anchors, base.anchors);
        CHECK(bd0.off_key[0] + bd1.off_key[0] + bp1.off_key[0] + br1.off_key[0] + bw.off_key[0] == 0,
              "%s: bass off key", nm);
        /* pad: DENSITY, RHYTHM, PITCH (the voicing's span) */
        part_stat pd0 = parts_run(st, P, 0.5f, 0.5f, 0.0f, 0.5f, 120.0f);
        part_stat pd1 = parts_run(st, P, 0.5f, 0.5f, 1.0f, 0.5f, 120.0f);
        CHECK(pd0.notes[2] <= base.notes[2] && base.notes[2] < pd1.notes[2], "%s: pad density %ld <= %ld < %ld", nm,
              pd0.notes[2], base.notes[2], pd1.notes[2]);
        part_stat pr1 = parts_run(st, P, 1.0f, 0.5f, 0.5f, 0.5f, 120.0f);
        CHECK(parts_hanging == 0 && pr1.h_bp != base.h_bp, "%s: pad rhythm up (%ld hanging)", nm, parts_hanging);
        part_stat pp1 = parts_run(st, P, 0.5f, 1.0f, 0.5f, 0.5f, 120.0f);
        CHECK(parts_hanging == 0, "%s: pad pitch up: %ld notes left hanging", nm, parts_hanging);
        part_stat pp0 = parts_run(st, P, 0.5f, 0.0f, 0.5f, 0.5f, 120.0f);
        /* (a voicing already close, folded into the bottom octave, is another inversion: half a semitone's grace) */
        CHECK(mean_spread(&pp0) <= mean_spread(&base) + 0.5f && mean_spread(&base) < mean_spread(&pp1),
              "%s: pad voicing span %.1f <= %.1f < %.1f", nm, mean_spread(&pp0), mean_spread(&base), mean_spread(&pp1));
        CHECK(pd0.off_key[2] + pd1.off_key[2] + pr1.off_key[2] + pp1.off_key[2] + pp0.off_key[2] == 0, "%s: pad off key",
              nm);
        /* the macro: all three knobs at once, every part */
        part_stat all = parts_run(st, TG_CHAOS_ALL, 1.0f, 1.0f, 1.0f, 0.5f, 60.0f);
        part_stat mac = parts_run(st, TG_CHAOS_ALL, 0.5f, 0.5f, 0.5f, 1.0f, 60.0f);
        CHECK(all.h_all == mac.h_all && parts_hanging == 0, "%s: the macro is not the three knobs together", nm);
    }
    /* the other modes (figures, drones, ostinatos, lines): wild and sparse, nothing hangs, nothing off key */
    static const int other[] = {TG_STYLE_MOZART, TG_STYLE_HILDEGARD, TG_STYLE_GLASS, TG_STYLE_PURCELL, TG_STYLE_REICH,
                                TG_STYLE_PART, TG_STYLE_DEBUSSY, TG_STYLE_BACH};
    for (unsigned si = 0; si < sizeof other / sizeof other[0]; ++si) {
        int st = other[si];
        const char *nm = tg_style_get(st)->name;
        for (int k = 0; k < 2; ++k) {
            float v = k ? 1.0f : 0.0f;
            part_stat x = parts_run(st, TG_CHAOS_ALL, v, v, v, v, 90.0f);
            CHECK(parts_hanging == 0, "%s: chaos %.0f: %ld notes left hanging", nm, v, parts_hanging);
            CHECK(x.off_key[0] + x.off_key[2] == 0, "%s: chaos %.0f: %ld bass, %ld pad notes off key", nm, v, x.off_key[0],
                  x.off_key[2]);
        }
    }
}

/* ---- the player's hands: chaos knobs, a root from a key, a chord held ------------------------------------ */
typedef struct { long notes, on_beat, leaps_sum, intervals, off_key; uint64_t h; } hand_stat;
static hand_stat hs;
static int hs_prev = -1;
static void hands_cb(void *ctx, const tg_event *e)
{
    (void)ctx;
    if (e->type != TG_EV_NOTE)
        return;
    uint8_t b[6] = {(uint8_t)g.ticks, (uint8_t)(g.ticks >> 8), e->part, e->note, e->velocity, e->on};
    for (int i = 0; i < 6; ++i)
        hs.h = (hs.h ^ b[i]) * 1099511628211ull;
    if (!e->on || e->part != TG_PART_MELODY)
        return;
    hs.notes++;
    tg_meter_pos m;
    tg_meter_at(&g, g.ticks - 1, &m);
    hs.on_beat += m.sub == 0;
    if (hs_prev >= 0) {
        hs.leaps_sum += abs(e->note - hs_prev);
        hs.intervals++;
    }
    hs_prev = e->note;
    uint16_t allowed = tt_scale_mask(g.period_scale >= 0 ? g.period_scale : g.scale) | tt_scale_mask(g.scale) |
                       g.chord_scale | g.mel_scale;
    for (int k = 0; k < g.chord.n; ++k)
        allowed |= (uint16_t)(1u << ((g.chord.root + g.chord.tone[k]) % 12));
    hs.off_key += !((allowed >> pc_of(e->note, g.root)) & 1) && g.mel_pending < 0;
}

static hand_stat hands_run(int style, uint32_t seed, float r, float p, float d, float macro, float secs)
{
    tg_init(&g, seed, SR);
    tg_apply_style(&g, style);
    tg_set_chaos(&g, r, p, d, macro);
    tg_reseed(&g, seed);
    memset(&hs, 0, sizeof hs);
    hs.h = 1469598103934665603ull;
    hs_prev = -1;
    long n = (long)(secs * SR) / BLOCK;
    for (long i = 0; i < n; ++i)
        tg_process(&g, BLOCK, hands_cb, NULL);
    return hs;
}

static void test_hands(void)
{
    static const int styles[] = {TG_STYLE_MOZART, TG_STYLE_DEBUSSY, TG_STYLE_BACH, TG_STYLE_JOHN_WILLIAMS};
    for (unsigned si = 0; si < sizeof styles / sizeof styles[0]; ++si) {
        int st = styles[si];
        const char *nm = tg_style_get(st)->name;
        hand_stat base = hands_run(st, 7, 0.5f, 0.5f, 0.5f, 0.5f, 120.0f);
        tg_init(&g, 7, SR);
        tg_apply_style(&g, st);
        tg_reseed(&g, 7);
        memset(&hs, 0, sizeof hs);
        hs.h = 1469598103934665603ull;
        hs_prev = -1;
        for (long i = 0; i < (long)(120.0f * SR) / BLOCK; ++i)
            tg_process(&g, BLOCK, hands_cb, NULL);
        CHECK(hs.h == base.h, "%s: chaos 0.5 is not the composer exactly", nm);
        /* density */
        hand_stat d0 = hands_run(st, 7, 0.5f, 0.5f, 0.0f, 0.5f, 120.0f);
        hand_stat d1 = hands_run(st, 7, 0.5f, 0.5f, 1.0f, 0.5f, 120.0f);
        CHECK(d0.notes < base.notes && base.notes < d1.notes, "%s: density %ld < %ld < %ld", nm, d0.notes, base.notes,
              d1.notes);
        /* pitch: the mean interval grows */
        hand_stat p0 = hands_run(st, 7, 0.5f, 0.0f, 0.5f, 0.5f, 120.0f);
        hand_stat p1 = hands_run(st, 7, 0.5f, 1.0f, 0.5f, 0.5f, 120.0f);
        float m0 = (float)p0.leaps_sum / (float)(p0.intervals ? p0.intervals : 1);
        float mb = (float)base.leaps_sum / (float)(base.intervals ? base.intervals : 1);
        float m1 = (float)p1.leaps_sum / (float)(p1.intervals ? p1.intervals : 1);
        CHECK(m0 < mb && mb < m1, "%s: pitch: mean interval %.2f < %.2f < %.2f", nm, m0, mb, m1);
        /* rhythm: the share on the beat falls */
        hand_stat r0 = hands_run(st, 7, 0.0f, 0.5f, 0.5f, 0.5f, 120.0f);
        hand_stat r1 = hands_run(st, 7, 1.0f, 0.5f, 0.5f, 0.5f, 120.0f);
        CHECK(r0.notes < base.notes, "%s: rhythm down: longer notes (%ld, the composer %ld)", nm, r0.notes, base.notes);
        CHECK(r1.h != base.h && r1.notes > r0.notes, "%s: rhythm up: another rhythm (%ld notes)", nm, r1.notes);
        /* the macro moves all three alike */
        hand_stat all = hands_run(st, 7, 1.0f, 1.0f, 1.0f, 0.5f, 60.0f);
        hand_stat mac = hands_run(st, 7, 0.5f, 0.5f, 0.5f, 1.0f, 60.0f);
        CHECK(all.h == mac.h, "%s: the macro is not the three knobs together", nm);
        /* in key at every extreme */
        long off = d0.off_key + d1.off_key + p0.off_key + p1.off_key + r0.off_key + r1.off_key + all.off_key;
        CHECK(off == 0, "%s: %ld melody notes off key at the extremes", nm, off);
        hand_stat lo = hands_run(st, 3, 0.0f, 0.0f, 0.0f, 0.0f, 60.0f);
        CHECK(lo.off_key == 0 && lo.notes > 0, "%s: all chaos off: %ld notes, %ld off key", nm, lo.notes, lo.off_key);
    }

    /* a root from a key: stopped, at once; playing, at the next chord change, all of it moved */
    tg_init(&g, 11, SR);
    tg_apply_style(&g, TG_STYLE_MOZART);
    tg_reseed(&g, 11);
    int r0 = g.root;
    tg_set_root(&g, r0 + 2);
    CHECK(g.root == (r0 + 2) % 12 && g.home_root == (r0 + 2) % 12, "root when stopped: %d", g.root);
    nlg = 0;
    run_seconds(10.0f);
    int before = g.root;
    tg_set_root(&g, before + 5);
    CHECK(g.root == before, "root moved before the chord change");
    long asked = g.ticks;
    while (g.root == before && g.ticks < asked + 200)
        tg_process(&g, BLOCK, cb, NULL);
    CHECK(g.root == (before + 5) % 12 && (g.ticks - 1) % g.chord_ticks == 0,
          "root applied at tick %ld (chord every %d)", g.ticks - 1, g.chord_ticks);
    CHECK(g.home_root == (before + 5) % 12 && g.root_req == -1, "home moved with it");
    nlg = 0;
    run_seconds(30.0f);
    long offk = 0, mel = 0;
    for (long i = 0; i < nlg; ++i)
        if (lg[i].e.type == TG_EV_NOTE && lg[i].e.on && lg[i].e.part == TG_PART_BASS) {
            mel++;
            offk += !((tt_scale_mask(g.scale) >> pc_of(lg[i].e.note, g.root)) & 1);
        }
    CHECK(mel > 0 && offk * 10 <= mel, "after the root change: %ld of %ld bass notes outside the new key", offk, mel);

    /* a chord held on the keys: the pad plays it, until released */
    tg_init(&g, 5, SR);
    tg_apply_style(&g, TG_STYLE_SATIE);
    tg_reseed(&g, 5);
    run_seconds(8.0f);
    uint16_t held = (uint16_t)(1u << 1 | 1u << 5 | 1u << 8);           /* C# F G#: D-flat major */
    tg_hold_chord(&g, held, 1);
    tg_process(&g, BLOCK, cb, NULL);
    nlg = 0;
    run_seconds(12.0f);
    long pad = 0, padoff = 0;
    for (long i = 0; i < nlg; ++i)
        if (lg[i].e.type == TG_EV_NOTE && lg[i].e.on && lg[i].e.part == TG_PART_PAD) {
            pad++;
            padoff += !((held >> (lg[i].e.note % 12)) & 1);
        }
    CHECK(pad > 0 && padoff == 0, "held chord: %ld pad notes, %ld not in it", pad, padoff);
    CHECK(g.chord.root == pc_of(1, g.root), "held chord root %d", g.chord.root);
    tg_hold_chord(&g, 0, 0);
    run_seconds(20.0f);
    CHECK(g.hold_root == -1, "released");
}

int main(void)
{
    test_meters();
    test_swing_lilt();
    test_vocabs();
    test_harmony_models();
    test_melody_modes();
    test_bass_pad_modes();
    test_form();
    test_orchestration();
    test_hands();
    test_hands_parts();
    printf("tuna_mech: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
