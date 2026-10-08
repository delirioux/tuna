/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA's instrument families on Felucca's engines, one by one: each plays the same short phrase, so they can
 * be heard (and measured, tools/tuna_audition.py) side by side.
 *
 *   build and run: tests/tuna_audition.sh OUTDIR
 *   OUTDIR/NN_NAME.wav     the phrase in the family's own register, with its reverb (to listen to)
 *   OUTDIR/dry/NN_NAME.wav a held C4, then a staccato C5, mono and dry (to measure: same pitch for all)
 *   OUTDIR/all.wav         every family in turn (2 s of silence between)
 *   OUTDIR/families.txt    number, name, engine, preset, own sound, register
 * ONLY=NAME renders one. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include "tuna_style.h"
#include "tuna_sounds.h"
/* instruction counts per family (tests/tuna_audition.sh with CPU=1: valgrind callgrind, collection on around
 * Felucca's mix only, one dump per family's phrase) */
#ifdef TUNA_CALLGRIND
#include <valgrind/callgrind.h>
#define CG_TOGGLE() CALLGRIND_TOGGLE_COLLECT
#define CG_DUMP() CALLGRIND_DUMP_STATS
#else
#define CG_TOGGLE() ((void)0)
#define CG_DUMP() ((void)0)
#endif

/* bass families sound two octaves lower, the low ones one octave lower */
static int family_shift(int f)
{
    switch (f) {
    case TI_CONTRABASS: case TI_TUBA: case TI_EBASS: case TI_SYNTHBASS: case TI_808: case TI_FATBASS:
    case TI_TALKBASS: case TI_WAVEBASS: case TI_ACID: case TI_SLAPBASS: case TI_FRETLESS: case TI_UPRIGHT:
        return -24;
    case TI_CELLO: case TI_BASSOON: case TI_HORN: case TI_DRONE: case TI_LOGDRUM:
        return -12;
    default:
        return 0;
    }
}

static void load_family(track_t *t, int f, bool dry)
{
    const tsn_family *fm = &TSN_FAMILY[f];
    host_preset(t, fm->engine, fm->preset);
    if (fm->own)
        tsn_apply_own(t->p, fm->own);
    if (fm->fm6)
        tsn_fm6_load(t, fm->fm6);
    if (fm->dist != TSN_KEEP) t->p[P_DIST] = fm->dist;
    if (fm->glide != TSN_KEEP) t->p[P_GLIDE] = fm->glide;
    if (fm->voice != TSN_KEEP) t->p[P_VOICE] = fm->voice;
    t->p[P_LEVEL] = getenv("NOTRIM") ? 104 : tsn_level(104, f);
    t->p[P_REV] = dry ? 0 : 36;
    if (dry)
        t->p[P_DLY] = 0;
}

typedef struct { uint32_t at, len; uint8_t note, vel; } ev_t;

/* render events on track 0 for frames; mono (dry) or stereo into f (and into all, if given) */
static void play(const ev_t *ev, int n, uint32_t frames, FILE *f, FILE *all, bool mono)
{
    track_t *t = &trk[0];
    for (uint32_t pos = 0; pos < frames; pos += CTL) {
        for (int i = 0; i < n; ++i) {
            if (ev[i].at >= pos && ev[i].at < pos + CTL)
                trk_note_on(t, ev[i].note, ev[i].vel);
            if (ev[i].at + ev[i].len >= pos && ev[i].at + ev[i].len < pos + CTL)
                trk_note_off(t, ev[i].note);
        }
        int32_t o[2 * CTL];
        if (!mono) CG_TOGGLE();
        mix_block(o, CTL);
        if (!mono) CG_TOGGLE();
        for (int i = 0; i < CTL; ++i) {
            if (mono) {
                int32_t m = (o[2 * i] + o[2 * i + 1]) / 2;
                int16_t s = (int16_t)(m > 32767 ? 32767 : m < -32768 ? -32768 : m);
                fwrite(&s, 2, 1, f);
            } else {
                wav_put(f, o[2 * i], o[2 * i + 1]);
                if (all)
                    wav_put(all, o[2 * i], o[2 * i + 1]);
            }
        }
    }
}

static void mono_hdr(FILE *f, uint32_t frames)
{
    uint32_t v;
    fwrite("RIFF", 1, 4, f); v = 36 + frames * 2; fwrite(&v, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f); v = 16; fwrite(&v, 4, 1, f);
    uint16_t a = 1, ch = 1, ba = 2, bits = 16; uint32_t sr = FS, br = FS * 2;
    fwrite(&a, 2, 1, f); fwrite(&ch, 2, 1, f); fwrite(&sr, 4, 1, f); fwrite(&br, 4, 1, f);
    fwrite(&ba, 2, 1, f); fwrite(&bits, 2, 1, f);
    fwrite("data", 1, 4, f); v = frames * 2; fwrite(&v, 4, 1, f);
}

static void silence(uint32_t frames)
{
    for (int k = 0; k < NTRK; ++k)
        for (int i = 0; i < NVOICE; ++i)
            trk[k].v[i].active = 0;
    int32_t o[2 * CTL];
    for (uint32_t pos = 0; pos < frames; pos += CTL)
        mix_block(o, CTL);
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    const char *only = getenv("ONLY");
    char path[512];
    const uint32_t S = FS;                       /* frames a second */
    /* to listen: a held note, a run up in eighths (140 bpm), a held triad */
    const uint32_t q = S * 60 / 140 / 2;
    ev_t demo[12];
    int nd = 0;
    demo[nd++] = (ev_t){0, S * 6 / 5, 60, 100};
    static const uint8_t RUN[8] = {60, 62, 64, 65, 67, 69, 71, 72};
    for (int i = 0; i < 8; ++i)
        demo[nd++] = (ev_t){S * 2 + (uint32_t)i * q, q * 3 / 4, RUN[i], (uint8_t)(i % 2 ? 84 : 104)};
    demo[nd++] = (ev_t){S * 4, S * 3 / 2, 60, 96};
    demo[nd++] = (ev_t){S * 4, S * 3 / 2, 64, 90};
    demo[nd++] = (ev_t){S * 4, S * 3 / 2, 67, 90};
    const uint32_t demo_len = S * 6 + S / 2;
    /* to measure: a held C4 (1.2 s, then 0.8 s of its release), a staccato C5 (0.1 s, 0.9 s after) */
    const ev_t dry[2] = {{0, S * 6 / 5, 60, 100}, {S * 2, S / 10, 72, 100}};
    const uint32_t dry_len = S * 3;

    snprintf(path, sizeof path, "%s/dry", dir);
    mkdir(path, 0755);
    snprintf(path, sizeof path, "%s/families.txt", dir);
    FILE *list = fopen(path, "w");
    FILE *all = 0;
    int count = 0;
    for (int f = 1; f < TI_COUNT; ++f)
        count += !only || !strcmp(only, tg_family_name(f));
    if (!only) {
        snprintf(path, sizeof path, "%s/all.wav", dir);
        all = fopen(path, "wb");
        wav_hdr(all, (uint32_t)count * (demo_len + 2 * S));
    }
    for (int f = 1; f < TI_COUNT; ++f) {
        const char *name = tg_family_name(f);
        if (only && strcmp(only, name))
            continue;
        char fn[32];
        int k = 0;
        for (; name[k] && k < 31; ++k)
            fn[k] = name[k] == ' ' || name[k] == '.' || name[k] == '/' ? '_' : name[k];
        fn[k] = 0;
        const tsn_family *fm = &TSN_FAMILY[f];
        int sh = family_shift(f);
        fprintf(list, "%02d %-10s engine %2u preset %2u %s shift %d\n", f, name, fm->engine, fm->preset,
                fm->own ? fm->own->name : fm->fm6 ? "fm6" : "-", sh);
        /* listen */
        host_tracks_init();
        silence(S / 4);
        load_family(&trk[0], f, false);
        ev_t d[12];
        for (int i = 0; i < nd; ++i) {
            d[i] = demo[i];
            d[i].note = (uint8_t)(demo[i].note + sh);
        }
        snprintf(path, sizeof path, "%s/%02d_%s.wav", dir, f, fn);
        FILE *w = fopen(path, "wb");
        wav_hdr(w, demo_len);
        play(d, nd, demo_len, w, all, false);
        CG_DUMP();
        fclose(w);
        if (all) {
            int32_t o[2 * CTL];
            for (uint32_t pos = 0; pos < 2 * S; pos += CTL) {
                mix_block(o, CTL);
                for (int i = 0; i < CTL; ++i)
                    wav_put(all, o[2 * i], o[2 * i + 1]);
            }
        }
        /* measure */
        host_tracks_init();
        silence(S / 4);
        load_family(&trk[0], f, true);
        snprintf(path, sizeof path, "%s/dry/%02d_%s.wav", dir, f, fn);
        w = fopen(path, "wb");
        mono_hdr(w, dry_len);
        play(dry, 2, dry_len, w, 0, true);
        fclose(w);
    }
    if (all)
        fclose(all);
    fclose(list);
    printf("tuna audition: %d families into %s\n", count, dir);
    return 0;
}
