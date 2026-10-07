/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA through Felucca's engines: the generator's events played on the real DSP (the same sources
 * as the firmware, through hostsim.c), rendered to WAV, with the voice budget and the CPU measured.
 *
 *   build: see tests/tuna_felucca.sh
 *   build/host/tuna_felucca OUTDIR [SECONDS]     every preset; ONLY=name renders one
 *
 * Parts to tracks: bass -> track 1, melody -> 2, third part -> 3, drums and timpani -> 4 (the DRUM
 * engine: GM notes; the timpani are its toms, the kit retuned so that GM 45 is the dominant and GM 48
 * the tonic, which keeps them to the two pitches a pair of kettledrums had).
 * CPU: ns per sample of Felucca's mix (mix_block) and of Tuna's generator (tg_process), against
 * Felucca's own heaviest reference mix (regress.c cpu/mix/3parts_full_drums: FM6 8 notes, PHASE 8,
 * VOICE 4, the drums on 16ths), measured the same way on this host. A ratio, not target cycles: the
 * real number comes from the FM-1 in phase 8. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include "tuna_gen.h"

/* Instruction counts (deterministic, unlike time on a shared host): built with -DTUNA_CALLGRIND and run
 * under valgrind --tool=callgrind --collect-atstart=no, collection is switched on around the measured
 * code only (CG_WHAT=mix: Felucca's mix_block, CG_WHAT=gen: Tuna's tg_process) and a dump is written
 * every 0.5 s of audio: each dump's total is that half second's instructions (tests/tuna_cpu.sh). */
#ifdef TUNA_CALLGRIND
#include <valgrind/callgrind.h>
static int cg_mix = 1;
#define CG_ON(what) do { if (cg_mix == (what)) CALLGRIND_TOGGLE_COLLECT; } while (0)
#define CG_OFF(what) CG_ON(what)
#define CG_DUMP() CALLGRIND_DUMP_STATS
#else
#define CG_ON(what) ((void)0)
#define CG_OFF(what) ((void)0)
#define CG_DUMP() ((void)0)
#endif

#define BLOCK CTL

typedef struct { uint8_t engine, preset; } sound_t;

typedef struct {
    const char *name;
    int style;                  /* TG_STYLE_*, -1: the generative settings below */
    uint32_t seed;
    int drums;                  /* 0: the style's default, 1: off, 2: period, 3: kit */
    sound_t s[3];               /* bass, melody, third */
    uint8_t level[4];           /* track levels (default 104) */
    uint8_t rev;                /* reverb send on the three voices, at least */
    /* generative (style -1) */
    float bpm;
    int division, root, scale, prog, chord_ticks, t_model;
    float t_bias, t_jitter, t_dv, x_spread, x_steps, x_dv;
} piece_t;

enum { E_ANALOG = 0, E_PHASE = 2, E_LOFI = 3, E_SAMPLE = 4, E_WHEEL = 7, E_GRAIN = 8, E_PHYS = 9, E_FM6 = 12 };
#define SND(e, p) {(uint8_t)(e), (uint8_t)(p)}
#define SUB_BASS SND(E_ANALOG, 7)
#define SOFT_PAD SND(E_ANALOG, 1)
#define STRINGS SND(E_ANALOG, 11)
#define BRASS SND(E_ANALOG, 9)
#define ACID SND(E_ANALOG, 4)
#define VIOLIN SND(E_PHASE, 2)
#define PULSE_LD SND(E_LOFI, 0)
#define PIANO SND(E_SAMPLE, 0)
#define FLUTE SND(E_SAMPLE, 2)
#define CLOUD SND(E_GRAIN, 0)
#define KALIMBA SND(E_PHYS, 4)
#define HARP SND(E_PHYS, 8)
#define FM_BELL SND(E_FM6, 1)
#define FM_BASS SND(E_FM6, 2)
#define FM_PAD SND(E_FM6, 4)
#define HARPSICHORD SND(E_FM6, 7)

static const piece_t pieces[] = {
    {.name = "garden", .style = -1, .seed = 1, .s = {SUB_BASS, FM_BELL, CLOUD}, .level = {96, 104, 92, 96}, .rev = 40,
     .bpm = 72, .division = 2, .root = 2, .scale = TS_DORIAN, .prog = TP_MODAL_VAMP, .chord_ticks = 16,
     .t_model = TM_T_COMPLEMENTARY_BERNOULLI, .t_bias = 0.5f, .t_jitter = 0.15f, .t_dv = 0.6f,
     .x_spread = 0.4f, .x_steps = 0.8f, .x_dv = 0.7f},
    {.name = "rhythm", .style = -1, .seed = 7, .drums = 3, .s = {ACID, PULSE_LD, SOFT_PAD}, .level = {100, 100, 84, 100},
     .rev = 20, .bpm = 104, .division = 4, .root = 9, .scale = TS_AEOLIAN, .prog = TP_ANDALUSIAN, .chord_ticks = 32,
     .t_model = TM_T_DRUMS, .t_bias = 0.8f, .t_dv = 0.75f, .x_spread = 0.55f, .x_steps = 0.68f, .x_dv = 0.8f},
    {.name = "bach",         .style = TG_STYLE_BACH,        .seed = 11, .s = {HARPSICHORD, HARPSICHORD, HARPSICHORD},
     .level = {100, 106, 96, 96}, .rev = 28},
    {.name = "bach-timpani", .style = TG_STYLE_BACH,        .seed = 11, .drums = 2,
     .s = {HARPSICHORD, HARPSICHORD, HARPSICHORD}, .level = {100, 106, 96, 100}, .rev = 28},
    {.name = "mozart",       .style = TG_STYLE_MOZART,      .seed = 5,  .s = {PIANO, PIANO, PIANO},
     .level = {98, 108, 88, 96}, .rev = 30},
    {.name = "mozart-kit",   .style = TG_STYLE_MOZART,      .seed = 5,  .drums = 3, .s = {PIANO, PIANO, PIANO},
     .level = {98, 108, 88, 92}, .rev = 24},
    {.name = "beethoven",    .style = TG_STYLE_BEETHOVEN,   .seed = 3,  .s = {PIANO, PIANO, PIANO},
     .level = {100, 108, 86, 100}, .rev = 30},
    {.name = "brahms",       .style = TG_STYLE_BRAHMS,      .seed = 8,  .s = {SUB_BASS, VIOLIN, STRINGS},
     .level = {96, 106, 92, 96}, .rev = 40},
    {.name = "mahler",       .style = TG_STYLE_MAHLER,      .seed = 2,  .s = {SUB_BASS, BRASS, STRINGS},
     .level = {96, 104, 92, 100}, .rev = 48},
    {.name = "chopin",       .style = TG_STYLE_CHOPIN,      .seed = 9,  .s = {PIANO, PIANO, PIANO},
     .level = {98, 108, 88, 96}, .rev = 36},
    {.name = "satie",        .style = TG_STYLE_SATIE,       .seed = 4,  .s = {PIANO, PIANO, PIANO},
     .level = {100, 106, 90, 96}, .rev = 44},
    {.name = "debussy",      .style = TG_STYLE_DEBUSSY,     .seed = 6,  .s = {SOFT_PAD, FLUTE, HARP},
     .level = {92, 106, 96, 96}, .rev = 52},
    {.name = "dvorak",       .style = TG_STYLE_DVORAK,      .seed = 12, .s = {SUB_BASS, VIOLIN, STRINGS},
     .level = {96, 106, 90, 96}, .rev = 40},
    {.name = "saint-saens",  .style = TG_STYLE_SAINT_SAENS, .seed = 13, .s = {PIANO, VIOLIN, PIANO},
     .level = {96, 108, 86, 98}, .rev = 34},
};

/* ---- events into the tracks ---------------------------------------------------------------------- */
static tg_gen gen;
/* Tuna runs at a fraction of the audio rate: it only decides when notes start, and 4x decimation
 * (11 kHz, 0.09 ms) is far below what anyone hears, at a quarter of the cost. TUNA_DIV overrides. */
static int tuna_div = 4;
static int timp_lo, timp_hi;          /* GM notes of the dominant and tonic kettledrums */
static long n_events;

static void on_event(void *ctx, const tg_event *e)
{
    (void)ctx;
    uint32_t tr = e->part >= TG_PART_DRUMS ? 3u : e->part;
    uint32_t note = e->note;
    if (e->part == TG_PART_TIMPANI) {
        int pc = ((e->note - gen.root) % 12 + 12) % 12;
        note = (uint32_t)(pc == 0 ? timp_hi : timp_lo);
    }
    if (e->on) {
        trk_note_on(&trk[tr], note, e->velocity);
        n_events++;
    } else if (tr != 3u) {
        trk_note_off(&trk[tr], note);    /* a drum hit plays on through its decay */
    }
}

/* the DRUM kit retuned (TUNE, E1) so that its tom at GM 45 (110 Hz as designed) sounds the dominant */
static void tune_timpani(track_t *t, int root)
{
    int dom = (root + 7) % 12, n = dom - 9;          /* A, the tom's designed pitch class */
    if (n > 5) n -= 12;
    if (n < -6) n += 12;
    int e1 = 64 + (int)lrintf((float)n * 16.0f / 3.0f);   /* 3/16 semitone per step */
    t->p[P_E0 + 1] = (int16_t)(e1 < 0 ? 0 : e1 > 127 ? 127 : e1);
    timp_lo = 45;
    timp_hi = 48;                                     /* the tom five semitones up: the tonic */
}

static void setup(const piece_t *p)
{
    host_tracks_init();
    tg_init(&gen, p->seed, (float)FS / (float)tuna_div);
    if (p->style >= 0) {
        tg_apply_style(&gen, p->style);
        tg_reseed(&gen, p->seed);
    } else {
        gen.bpm = p->bpm;
        gen.division = p->division;
        gen.root = p->root;
        gen.scale = p->scale;
        gen.prog = p->prog;
        gen.chord_ticks = p->chord_ticks;
        gen.t.model = p->t_model;
        gen.t.bias = p->t_bias;
        gen.t.jitter = p->t_jitter;
        gen.t.sequence.deja_vu = p->t_dv;
        gen.x.spread = p->x_spread;
        gen.x.steps = p->x_steps;
        gen.x.deja_vu = p->x_dv;
    }
    if (p->drums)
        tg_set_percussion(&gen, p->drums - 1);
    gen.humanize = 1.0f;
    song.g[G_BPM] = (int16_t)lrintf(gen.bpm);
    for (uint32_t k = 0; k < 3u; ++k) {
        host_preset(&trk[k], p->s[k].engine, p->s[k].preset);
        if (k != 1u)
            trk[k].p[P_VOICE] = V_POLY;               /* chords and figures; the melody keeps its preset's mode */
        trk[k].p[P_LEVEL] = p->level[k];
        if (trk[k].p[P_REV] < p->rev)
            trk[k].p[P_REV] = p->rev;
    }
    /* fast figures (Alberti, broken chords, arpeggios, repeated chords) release sooner: a piano
     * note's tail would otherwise hold a voice of the shared 8 long after the next note sounds */
    if (gen.pad_mode == TG_PAD_ALBERTI || gen.pad_mode == TG_PAD_BROKEN || gen.pad_mode == TG_PAD_ARPEGGIO ||
        gen.pad_mode == TG_PAD_REPEAT || gen.pad_mode == TG_PAD_COUNTER) {
        if (trk[2].p[P_REL] > 34)
            trk[2].p[P_REL] = 34;
    }
    host_drums(&trk[3]);
    trk[3].p[P_LEVEL] = p->level[3];
    tune_timpani(&trk[3], gen.root);
}

/* ---- render ---------------------------------------------------------------------------------------- */
typedef struct {
    double mix_ns, gen_ns, worst_mix_ns;   /* per sample: mean, and the worst 0.5 s of the mix */
    uint32_t busy_max, kills, clips;
    long samples;
    int32_t peak;
} stats_t;

static int render(const piece_t *p, const char *dir, int seconds, stats_t *st)
{
    char path[512];
    snprintf(path, sizeof path, "%s/tuna_%s.wav", dir, p->name);
    FILE *w = fopen(path, "wb");
    if (!w) {
        fprintf(stderr, "cannot write %s\n", path);
        return 1;
    }
    memset(st, 0, sizeof *st);
    setup(p);
    n_events = 0;
    uint32_t kills0 = voice_kills;
    long total = (long)seconds * FS, f = 0, arrived = -1, tail_left = -1;
    const long seg = FS / 2;
    uint64_t seg_ns = 0, mix_ns = 0, gen_ns = 0;
    long seg_n = 0;
    wav_hdr(w, 0);
    while (1) {
        int32_t o[2 * BLOCK];
        uint64_t t0 = now_ns();
        CG_ON(0);
        if (tail_left < 0)
            tg_process(&gen, BLOCK / tuna_div, on_event, 0);
        CG_OFF(0);
        uint64_t t1 = now_ns();
        CG_ON(1);
        mix_block(o, BLOCK);
        CG_OFF(1);
        uint64_t t2 = now_ns();
        gen_ns += t1 - t0;
        mix_ns += t2 - t1;
        seg_ns += t2 - t1;
        seg_n += BLOCK;
        if (seg_n >= seg) {
            CG_DUMP();
            double v = (double)seg_ns / (double)seg_n;
            if (v > st->worst_mix_ns)
                st->worst_mix_ns = v;
            seg_ns = 0;
            seg_n = 0;
        }
        uint32_t b = busy_now();
        if (b > st->busy_max)
            st->busy_max = b;
        for (int i = 0; i < 2 * BLOCK; ++i) {
            int32_t v = o[i] < 0 ? -o[i] : o[i];
            if (v > st->peak) st->peak = v;
            if (v >= 32700) st->clips++;
        }
        for (int i = 0; i < BLOCK; ++i)
            wav_put(w, o[2 * i], o[2 * i + 1]);
        f += BLOCK;
        /* end like a song: after the time asked, on to the next arrival on the tonic, its bar, a tail */
        if (tail_left < 0 && f >= total) {
            if (arrived < 0 && gen.ticks > 1 && tg_period_position(&gen, NULL) == 0)
                arrived = gen.ticks;
            if (arrived >= 0 && gen.ticks >= arrived + gen.division * gen.beats_per_bar) {
                tg_all_off(&gen, on_event, 0);
                tail_left = 3L * FS;
            }
        }
        if (tail_left >= 0 && (tail_left -= BLOCK) <= 0)
            break;
    }
    /* the header with the real length */
    fseek(w, 0, SEEK_SET);
    wav_hdr(w, (uint32_t)f);
    fclose(w);
    st->samples = f;
    st->mix_ns = (double)mix_ns / (double)f;
    st->gen_ns = (double)gen_ns / (double)f;
    st->kills = voice_kills - kills0;
    return 0;
}

/* Felucca's reference mix, in-process, for the instruction counts: 1 s to settle, then 4 s in 0.5 s dumps */
static void reference_counts(void)
{
    static const uint8_t NOTES[8] = {48, 52, 55, 59, 60, 64, 67, 71};
    static const uint8_t REF[3][3] = {{ENGI_FM6, 0, 8}, {2, 0, 8}, {5, 0, 4}};
    host_tracks_init();
    for (uint32_t p = 0; p < 3u; p++) {
        host_preset(&trk[p], REF[p][0], REF[p][1]);
        trk[p].p[P_VOICE] = V_POLY;
        trk[p].p[P_SUS] = 127;
        trk[p].p[P_AMODE] = 0;
        for (uint32_t i = 0; i < REF[p][2]; i++)
            trk_note_on(&trk[p], NOTES[i] + 12u * p, 100);
    }
    host_drums(&trk[3]);
    long n = 5L * FS / CTL;
    for (long k = 0; k < n; k++) {
        int32_t o[2 * CTL];
        if ((k * CTL) % (FS / 8u) < CTL)
            drum_hit((uint32_t)((k * CTL) / (FS / 8u)));
        if (k >= FS / CTL)
            CG_ON(1);
        mix_block(o, CTL);
        if (k >= FS / CTL) {
            CG_OFF(1);
            if (((k + 1 - FS / CTL) * CTL) % (FS / 2) < CTL)
                CG_DUMP();
        }
    }
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    int seconds = argc > 2 ? atoi(argv[2]) : 60;
    const char *only = getenv("ONLY");
    if (getenv("TUNA_DIV"))
        tuna_div = atoi(getenv("TUNA_DIV"));
    if (tuna_div != 1 && tuna_div != 2 && tuna_div != 4 && tuna_div != 8)
        tuna_div = 4;
#ifdef TUNA_CALLGRIND
    cg_mix = !(getenv("CG_WHAT") && !strcmp(getenv("CG_WHAT"), "gen"));
    if (getenv("REF")) {
        reference_counts();
        return 0;
    }
#endif
    /* Felucca's reference: its heaviest regression mix, measured here the same way */
    static const uint8_t REF[NPART][3] = {{ENGI_FM6, 0, 8}, {2, 0, 8}, {5, 0, 4}, {ENGI_DRUM, 0, DRUM_HITS}};
    uint32_t ref_busy;
    double ref = tracks_cost(REF, &ref_busy);
    printf("reference: Felucca's full mix (FM6 8 + PHASE 8 + VOICE 4 + drums): %.0f ns per sample on this host\n", ref);
    printf("%-13s %6s %5s %6s %6s %7s %7s %8s %6s\n", "piece", "events", "peak", "voices", "steals", "mix/ref",
           "worst", "tuna ns", "clips");
    int fails = 0;
    for (size_t i = 0; i < sizeof pieces / sizeof pieces[0]; ++i) {
        const piece_t *p = &pieces[i];
        if (only && strcmp(only, p->name))
            continue;
        stats_t st;
        fails += render(p, dir, seconds, &st);
        printf("%-13s %6ld %5d %3u/%-2u %6u %6.2fx %6.2fx %8.1f %6u\n", p->name, n_events, st.peak, st.busy_max, NVOICE,
               st.kills, st.mix_ns / ref, st.worst_mix_ns / ref, st.gen_ns, st.clips);
        fails += st.busy_max > NVOICE || st.peak == 0;
    }
    return fails ? 1 : 0;
}
