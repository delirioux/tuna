/* SPDX-License-Identifier: GPL-3.0-only */
/* Tests of TUNA's MIDI out (firmware/src/tuna_midi.c through tuna_app.c): the channels, every note on with its
 * note off, programs, controllers, tunings as bend, clock, start and stop, the FM-1's own sound switched
 * off. With an OUTDIR it also writes what went out as Standard MIDI Files (one track per channel), for a DAW.
 *   cc -O2 -Ifirmware/src -o build/host/tuna_midi_test tests/tuna_midi_test.c firmware/src/tuna_app.c \
 *      firmware/src/tuna_midi.c firmware/src/tuna_reso.c firmware/src/tuna_preset.c firmware/src/tuna_gen.c \
 *      firmware/src/tuna_style.c firmware/src/tuna_marbles.c firmware/src/tuna_marbles_tables.c \
 *      firmware/src/tuna_theory.c -lm
 *   build/host/tuna_midi_test [OUTDIR]          (exit 0: all passed) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tuna_app.h"

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

#define SR 11025.0f
static tuna_app app;

/* everything that went out, in order */
#define MAXMSG 400000
static tmi_msg out[MAXMSG];
static long nout;
static long local_on;                                /* notes the FM-1's own sound got */
static void sound(void *ctx, const tg_event *e)
{
    (void)ctx;
    if (e->type == TG_EV_NOTE && e->on)
        local_on++;
}

static void drain(void)
{
    tmi_msg m;
    while (tmi_pop(&app.midi, &m))
        if (nout < MAXMSG)
            out[nout++] = m;
}

static void run_seconds(float s)
{
    long blocks = (long)(s * SR / 64);
    for (long i = 0; i < blocks; ++i) {
        ta_process(&app, 64);
        drain();
    }
}

/* ---- a Standard MIDI File: track 0 the tempo, then a track per channel used ---------------------------- */
static void vlq(FILE *f, uint32_t v)
{
    uint8_t b[5];
    int n = 0;
    b[n++] = v & 127;
    while (v >>= 7)
        b[n++] = (uint8_t)(128 | (v & 127));
    while (n)
        fputc(b[--n], f);
}
static void be32(FILE *f, uint32_t v) { fputc(v >> 24, f); fputc(v >> 16, f); fputc(v >> 8, f); fputc(v, f); }
static void write_smf(const char *path, float bpm, const char *title)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return;
    int used[16] = {0}, ntr = 1;
    for (long i = 0; i < nout; ++i)
        if (out[i].b[0] < 0xF0 && !used[out[i].b[0] & 15]) {
            used[out[i].b[0] & 15] = 1;
            ntr++;
        }
    const int ppq = 480;
    fwrite("MThd", 1, 4, f);
    be32(f, 6);
    fputc(0, f); fputc(1, f); fputc(0, f); fputc(ntr, f); fputc(ppq >> 8, f); fputc(ppq & 255, f);
    static const char *const NAMES[16] = {"Bass", "Melody", "Pad", "Timpani", "", "", "", "", "", "Drums"};
    for (int t = -1; t < 16; ++t) {
        if (t >= 0 && !used[t])
            continue;
        uint8_t buf[1 << 20];
        long n = 0;
        uint32_t last = 0;
#define PUT(c) (buf[n++] = (uint8_t)(c))
        if (t < 0) {                                   /* tempo and name */
            uint32_t us = (uint32_t)(60000000.0f / bpm);
            PUT(0); PUT(0xFF); PUT(0x51); PUT(3); PUT(us >> 16); PUT(us >> 8); PUT(us);
            int len = (int)strlen(title);
            PUT(0); PUT(0xFF); PUT(0x03); PUT(len);
            for (int i = 0; i < len; ++i) PUT(title[i]);
        } else {
            const char *nm = NAMES[t][0] ? NAMES[t] : "Part";
            int len = (int)strlen(nm);
            PUT(0); PUT(0xFF); PUT(0x03); PUT(len);
            for (int i = 0; i < len; ++i) PUT(nm[i]);
            for (long i = 0; i < nout && n < (1 << 20) - 16; ++i) {
                const tmi_msg *m = &out[i];
                if (m->b[0] >= 0xF0 || (m->b[0] & 15) != t)
                    continue;
                uint32_t tick = (uint32_t)((double)(m->frame - out[0].frame) / SR * bpm / 60.0 * ppq);
                if (tick < last) tick = last;
                uint8_t tmp[5];
                int k = 0;
                uint32_t d = tick - last;
                tmp[k++] = d & 127;
                while (d >>= 7) tmp[k++] = (uint8_t)(128 | (d & 127));
                while (k) PUT(tmp[--k]);
                last = tick;
                for (int j = 0; j < m->n; ++j) PUT(m->b[j]);
            }
        }
        PUT(0); PUT(0xFF); PUT(0x2F); PUT(0);
        fwrite("MTrk", 1, 4, f);
        be32(f, (uint32_t)n);
        fwrite(buf, 1, (size_t)n, f);
    }
    fclose(f);
    (void)vlq;
}

/* ---- the checks on what went out --------------------------------------------------------------------- */
typedef struct { long ons, offs, clocks, starts, stops, progs, bends, ccs, bad_order; int open; } tally;
static tally count(long from, int bend_ch_ok_mask)
{
    tally t;
    memset(&t, 0, sizeof t);
    static int held[16][128];
    memset(held, 0, sizeof held);
    uint32_t last = 0;
    for (long i = from; i < nout; ++i) {
        const tmi_msg *m = &out[i];
        uint8_t st = m->b[0];
        if (m->frame + 2000 < last) t.bad_order++;    /* (a humanized note may be a little earlier) */
        if (m->frame > last) last = m->frame;
        if (st == 0xF8) t.clocks++;
        else if (st == 0xFA) t.starts++;
        else if (st == 0xFC) t.stops++;
        else if ((st & 0xF0) == 0x90) { t.ons++; held[st & 15][m->b[1]]++; }
        else if ((st & 0xF0) == 0x80) {
            t.offs++;
            if (held[st & 15][m->b[1]] > 0) held[st & 15][m->b[1]]--;
            else t.bad_order += 1000;                 /* an off without its on */
        }
        else if ((st & 0xF0) == 0xC0) t.progs++;
        else if ((st & 0xF0) == 0xE0) { t.bends++; if (!((bend_ch_ok_mask >> (st & 15)) & 1)) t.bad_order += 100000; }
        else if ((st & 0xF0) == 0xB0) t.ccs++;
    }
    for (int c = 0; c < 16; ++c)
        for (int n = 0; n < 128; ++n)
            t.open += held[c][n];
    return t;
}

static int item_of(int style)
{
    for (int i = 0; i < ta_lib_count(); ++i)
        if (ta_lib_style(i) == style)
            return i;
    return 0;
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : NULL;
    ta_init(&app, SR, 1);
    ta_set_sound(&app, sound, NULL);

    /* the channels: every composer a minute, everything on its part's channel, every note closed by stop */
    static const int styles[] = {TG_STYLE_BACH, TG_STYLE_MOZART, TG_STYLE_MAHLER, TG_STYLE_DEBUSSY, TG_STYLE_GERSHWIN,
                                 TG_STYLE_REICH, TG_STYLE_PART, TG_STYLE_JOHN_WILLIAMS, TG_STYLE_ORFF};
    for (unsigned s = 0; s < sizeof styles / sizeof styles[0]; ++s) {
        const char *nm = tg_style_get(styles[s])->name;
        nout = 0;
        ta_select(&app, item_of(styles[s]));
        float bpm = app.g.bpm;
        ta_play(&app, true);
        run_seconds(60.0f);
        ta_play(&app, false);
        drain();
        tally t = count(0, 1 << 0 | 1 << 1 | 1 << 3);
        CHECK(t.ons > 30 && t.ons == t.offs && t.open == 0, "%s: %ld on, %ld off, %d open", nm, t.ons, t.offs, t.open);
        CHECK(t.bad_order == 0, "%s: out of order / off without on / bend on a chord part: %ld", nm, t.bad_order);
        CHECK(t.starts == 1 && t.stops == 1, "%s: start %ld stop %ld", nm, t.starts, t.stops);
        long want = (long)(60.0f * bpm / 60.0f * 24.0f);
        CHECK(labs(t.clocks - want) <= 2, "%s: %ld clocks for %ld", nm, t.clocks, want);
        CHECK(t.progs >= 1, "%s: programs sent (%ld)", nm, t.progs);
        CHECK(app.midi.dropped == 0, "%s: %u dropped", nm, app.midi.dropped);
        int chans = 0;
        for (long i = 0; i < nout; ++i)
            if ((out[i].b[0] & 0xF0) == 0x90) chans |= 1 << (out[i].b[0] & 15);
        CHECK((chans & ~(1 << 0 | 1 << 1 | 1 << 2 | 1 << 3 | 1 << 9)) == 0 && (chans & 7) != 0,
              "%s: notes on channels %#x", nm, chans);
        if (dir) {
            char path[512], title[64];
            snprintf(path, sizeof path, "%s/tuna_%s.mid", dir, nm);
            for (char *p = path + strlen(dir); *p; ++p) if (*p == ' ') *p = '_';
            snprintf(title, sizeof title, "Tuna: %s", nm);
            write_smf(path, bpm, title);
        }
    }

    /* drums only on 10 */
    nout = 0;
    ta_select(&app, item_of(TG_STYLE_LLOYD_WEBBER));
    ta_play(&app, true);
    run_seconds(30.0f);
    ta_play(&app, false);
    drain();
    long ch10 = 0;
    for (long i = 0; i < nout; ++i)
        if (out[i].b[0] == 0x99) ch10++;
    CHECK(ch10 > 20, "the kit on channel 10: %ld notes", ch10);

    /* a historical tuning: bends on the one-note parts, none on the pad */
    nout = 0;
    ta_select(&app, item_of(TG_STYLE_BACH));
    app.g.tuning = TT_TUNE_MEANTONE;
    ta_play(&app, true);
    run_seconds(30.0f);
    ta_play(&app, false);
    drain();
    tally t = count(0, 1 << 0 | 1 << 1 | 1 << 3);
    CHECK(t.bends > 10 && t.bad_order == 0, "meantone: %ld bends, all on bass / melody / timpani", t.bends);
    nout = 0;
    app.g.tuning = TT_TUNE_EQUAL;
    ta_play(&app, true);
    run_seconds(10.0f);
    ta_play(&app, false);
    drain();
    t = count(0, 0xFFFF);
    CHECK(t.bends <= 2, "equal temperament: no bends after the first (%ld)", t.bends);

    /* BEND off; CLOCK off; OUT off mid-play closes what is open */
    ta_midi_set(&app, true, false, false);
    nout = 0;
    app.g.tuning = TT_TUNE_MEANTONE;
    ta_play(&app, true);
    run_seconds(10.0f);
    t = count(0, 0xFFFF);
    CHECK(t.clocks == 0 && t.starts == 0 && t.bends == 0, "clock and bend off: %ld clocks, %ld bends", t.clocks, t.bends);
    ta_midi_set(&app, false, false, false);
    drain();
    long before = nout;
    t = count(0, 0xFFFF);
    CHECK(t.open == 0, "OUT off: %d notes left open", t.open);
    run_seconds(5.0f);
    CHECK(nout == before, "OUT off: nothing more (%ld)", nout - before);
    ta_play(&app, false);
    app.g.tuning = TT_TUNE_EQUAL;

    /* SOUND off: MIDI only */
    ta_midi_set(&app, true, true, true);
    app.local = false;
    local_on = 0;
    nout = 0;
    ta_play(&app, true);
    run_seconds(10.0f);
    ta_play(&app, false);
    drain();
    t = count(0, 0xFFFF);
    CHECK(local_on == 0 && t.ons > 5, "SOUND off: %ld notes inside, %ld out", local_on, t.ons);
    app.local = true;

    /* General MIDI programs for every family */
    for (int f = 0; f < TI_COUNT; ++f)
        CHECK(tmi_gm_program(f) >= 0 && tmi_gm_program(f) < 128, "family %d -> program %d", f, tmi_gm_program(f));
    CHECK(tmi_gm_program(TI_STRINGS) == 48 && tmi_gm_program(TI_FLUTE) == 73 && tmi_gm_program(TI_CHOIR) == 52,
          "strings 49, flute 74, choir 53 (1-based)");

    printf("tuna_midi: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
