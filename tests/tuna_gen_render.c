/* SPDX-License-Identifier: GPL-3.0-only */
/* Render TUNA generator presets to MIDI and WAV, for listening before there is hardware.
 *   cc -O2 -Ifirmware/src -o build/host/tuna_gen_render tests/tuna_gen_render.c \
 *      firmware/src/tuna_gen.c firmware/src/tuna_marbles.c firmware/src/tuna_marbles_tables.c \
 *      firmware/src/tuna_theory.c -lm
 *   build/host/tuna_gen_render OUTDIR [SECONDS]
 * The WAV uses a tiny sketch synth (sine bass, soft flute-like melody, sine pad, a delay), not Felucca's
 * engines: it is for judging the notes, not the sound. The MIDI has one track per part.
 * Also checks that a seed replays identically. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tuna_gen.h"

#define SR 44100
#define BLOCK 32

typedef struct {
    const char *name;
    float bpm;
    int division, root, scale, prog, chord_ticks;
    int t_model;
    float t_bias, t_jitter, t_deja_vu, pw_mean, pw_std;
    int t_length;
    float x_spread, x_bias, x_steps, x_deja_vu;
    int x_length, x_control;
    uint32_t seed;
    float mel_density;
    int pad_tones;
    bool pad_restrike;
    int style;                /* TG_STYLE_*, -1 = the settings above */
    bool keys;                /* sketch synth: a keyboard sound for the third part (figures) */
} preset;

static const preset presets[] = {
    /* name     bpm  div root scale        prog           chord  T model                   bias  jit   dv    pw    pwstd len  spread xbias steps xdv  xlen ctrl                  seed dens pad restrike */
    {"garden",  72,  2,  2,  TS_DORIAN,     TP_MODAL_VAMP,  16, TM_T_COMPLEMENTARY_BERNOULLI, 0.5f, 0.15f, 0.6f, 0.75f, 0.2f, 8,  0.40f, 0.5f, 0.80f, 0.70f, 8, TM_CONTROL_TILT,      1,  0.35f, 4, false, -1, false},
    {"rhythm",  104, 4,  9,  TS_AEOLIAN,    TP_ANDALUSIAN,  32, TM_T_DRUMS,                 0.8f, 0.0f,  0.75f, 0.3f, 0.0f, 16, 0.55f, 0.5f, 0.68f, 0.80f, 16, TM_CONTROL_IDENTICAL, 7,  0.55f, 3, true, -1, false},
    {"lydian",  84,  2,  5,  TS_LYDIAN,     TP_MODAL_VAMP,  16, TM_T_CLUSTERS,              0.65f, 0.3f, 0.4f, 0.6f, 0.4f, 8,  0.50f, 0.55f, 0.75f, 0.55f, 8, TM_CONTROL_BUMP,      42, 0.40f, 4, false, -1, false},
    /* composer signatures: everything comes from the style; only name, seed and synth matter here */
    {.name = "bach",        .seed = 11, .style = TG_STYLE_BACH,        .keys = true},
    {.name = "mozart",      .seed = 5,  .style = TG_STYLE_MOZART,      .keys = true},
    {.name = "beethoven",   .seed = 3,  .style = TG_STYLE_BEETHOVEN,   .keys = true},
    {.name = "brahms",      .seed = 8,  .style = TG_STYLE_BRAHMS,      .keys = true},
    {.name = "mahler",      .seed = 2,  .style = TG_STYLE_MAHLER,      .keys = false},
    {.name = "chopin",      .seed = 9,  .style = TG_STYLE_CHOPIN,      .keys = true},
    {.name = "satie",       .seed = 4,  .style = TG_STYLE_SATIE,       .keys = true},
    {.name = "debussy",     .seed = 6,  .style = TG_STYLE_DEBUSSY,     .keys = true},
    {.name = "dvorak",      .seed = 12, .style = TG_STYLE_DVORAK,      .keys = false},
    {.name = "saint-saens", .seed = 13, .style = TG_STYLE_SAINT_SAENS, .keys = true},
};

/* ---- event log (for MIDI and the synth) ------------------------------------------------------- */
typedef struct { long t; tg_event e; } logged;
typedef struct {
    logged *ev;
    long n, cap, now;
    /* musical checks, harmonized mode */
    long mel_notes, mel_steps, mel_downbeats, mel_downbeat_chord;
    long arrivals, arrivals_tonic, arrivals_tonic_chord, arrivals_by_step;
    long off_key, halves, halves_on_dominant, bass_changes, bass_roots, counter_notes, parallels;
    double motif_share;
    int counter_prev, counter_prev_iv, counter_prev_mel;
    int mel_prev;
} evlog;

static tg_gen *cur;     /* the generator being rendered, for the checks in on_event */

static void on_event(void *ctx, const tg_event *e)
{
    evlog *l = ctx;
    if (l->n == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 1024;
        l->ev = realloc(l->ev, (size_t)l->cap * sizeof *l->ev);
    }
    l->ev[l->n].t = l->now + e->offset;
    l->ev[l->n].e = *e;
    l->n++;
    if (e->on) {
        /* in key: the scale, the current chord, or the scale bent to the chord */
        uint16_t allowed = tt_scale_mask(cur->scale) | cur->chord_scale;
        for (int k = 0; k < cur->chord.n; ++k)
            allowed |= (uint16_t)(1u << ((cur->chord.root + cur->chord.tone[k]) % 12));
        int pc = ((e->note - cur->root) % 12 + 12) % 12;
        bool leaning = e->part == TG_PART_MELODY && cur->mel_pending >= 0;   /* a chromatic appoggiatura */
        l->off_key += !((allowed >> pc) & 1) && !leaning;
    }
    if (e->on && e->part == TG_PART_BASS && cur->style && cur->bass_mode != TG_BASS_PEDAL && cur->chord_ticks > 0 &&
        (cur->ticks - 1) % cur->chord_ticks == 0) {
        l->bass_changes++;
        l->bass_roots += ((e->note - cur->root) % 12 + 12) % 12 == cur->chord.root;
    }
    if (e->on && e->part == TG_PART_PAD && cur->pad_mode == TG_PAD_COUNTER) {
        /* parallel fifths or octaves between melody and counter-melody */
        int mel = cur->mel_note, iv = ((mel - e->note) % 12 + 12) % 12;
        if (l->counter_prev >= 0 && (iv == 0 || iv == 7) && iv == l->counter_prev_iv && mel != l->counter_prev_mel &&
            e->note != l->counter_prev && (mel > l->counter_prev_mel) == (e->note > l->counter_prev))
            l->parallels++;
        l->counter_notes++;
        l->counter_prev = e->note;
        l->counter_prev_iv = iv;
        l->counter_prev_mel = mel;
    }
    if (e->on && e->part == TG_PART_MELODY) {
        int step = l->mel_prev >= 0 ? abs(e->note - l->mel_prev) : 99;
        if (l->mel_prev >= 0) {
            l->mel_steps += step <= 2;
            l->mel_notes++;
        }
        l->mel_prev = e->note;
        int q = (int)(cur->ticks - 1);
        int period = 0, ppos = tg_period_position(cur, &period);
        bool aug = cur->chord.n >= 3 && cur->chord.tone[1] == 4 && cur->chord.tone[2] == 8;
        if (cur->style && ppos == tg_half_position(cur) && !aug) {
            l->halves++;
            l->halves_on_dominant += cur->chord.root == 7 || cur->chord.root == 11;
        }
        if (q > 0 && ppos == 0) {
            l->arrivals++;
            l->arrivals_by_step += step >= 1 && step <= 2;
            l->arrivals_tonic += ((e->note - cur->root) % 12 + 12) % 12 == 0;
            l->arrivals_tonic_chord += cur->chord.root == 0;
        }
        if (q % (cur->division * cur->beats_per_bar) == 0) {
            l->mel_downbeats++;
            int pc = ((e->note - cur->root) % 12 + 12) % 12;
            tt_chord c = cur->chord;
            if (cur->mel_pending >= 0) {
                l->mel_downbeat_chord++;      /* an appoggiatura: a non-chord tone on purpose, resolving */
            } else {
                for (int k = 0; k < c.n && k < 3; ++k)
                    if ((c.root + c.tone[k]) % 12 == pc) {
                        l->mel_downbeat_chord++;
                        break;
                    }
            }
        }
    }
}

static void run(const preset *p, int seconds, evlog *l)
{
    static tg_gen g;
    tg_init(&g, p->seed, (float)SR);
    if (p->style >= 0)
        goto styled;              /* a signature brings its own settings */
    g.bpm = p->bpm;
    g.division = p->division;
    g.root = p->root;
    g.scale = p->scale;
    g.prog = p->prog;
    g.chord_ticks = p->chord_ticks;
    g.t.model = p->t_model;
    g.t.bias = p->t_bias;
    g.t.jitter = p->t_jitter;
    g.t.sequence.deja_vu = p->t_deja_vu;
    tm_seq_set_length(&g.t.sequence, p->t_length);
    g.t.pulse_width_mean = p->pw_mean;
    g.t.pulse_width_std = p->pw_std;
    g.x.spread = p->x_spread;
    g.x.bias = p->x_bias;
    g.x.steps = p->x_steps;
    g.x.deja_vu = p->x_deja_vu;
    g.x.length = p->x_length;
    g.x.control_mode = p->x_control;
    g.melody_density = p->mel_density;
    g.pad_tones = p->pad_tones;
    g.pad_restrike = p->pad_restrike;
styled:
    if (p->style >= 0) {
        tg_apply_style(&g, p->style);
        tg_reseed(&g, p->seed);
    }
    cur = &g;
    memset(l, 0, sizeof *l);
    l->mel_prev = -1;
    l->counter_prev = -1;
    long total = (long)seconds * SR;
    for (l->now = 0; l->now < total; l->now += BLOCK)
        tg_process(&g, BLOCK, on_event, l);
    /* end like a song: play on to the next arrival on the tonic and let it ring for its bar */
    long arrived = -1;
    while (1) {
        tg_process(&g, BLOCK, on_event, l);
        l->now += BLOCK;
        if (arrived < 0 && g.ticks > 1 && tg_period_position(&g, NULL) == 0)
            arrived = g.ticks;
        if (arrived >= 0 && g.ticks >= arrived + g.division * g.beats_per_bar)
            break;
    }
    tg_all_off(&g, on_event, l);
    long notes = g.motif_notes + g.free_notes;
    l->motif_share = notes ? 100.0 * g.motif_notes / notes : 0.0;
}

/* ---- MIDI file -------------------------------------------------------------------------------- */
static void put32(FILE *f, uint32_t v) { fputc(v >> 24, f); fputc(v >> 16, f); fputc(v >> 8, f); fputc(v, f); }
static void put16(FILE *f, uint16_t v) { fputc(v >> 8, f); fputc(v, f); }
static int varlen(uint8_t *b, uint32_t v)
{
    uint8_t tmp[5];
    int n = 0;
    do { tmp[n++] = v & 0x7F; v >>= 7; } while (v);
    for (int i = 0; i < n; ++i)
        b[i] = tmp[n - 1 - i] | (i < n - 1 ? 0x80 : 0);
    return n;
}

static void write_midi(const char *path, const evlog *l, float bpm)
{
    const int ppq = 480;
    const double ticks_per_sample = (double)ppq * bpm / 60.0 / SR;
    static const char *names[TG_PARTS] = {"Bass", "Melody", "Pad"};
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fwrite("MThd", 1, 4, f); put32(f, 6); put16(f, 1); put16(f, TG_PARTS + 1); put16(f, (uint16_t)ppq);
    /* tempo track */
    {
        uint32_t us = (uint32_t)(60000000.0 / bpm);
        uint8_t t[] = {0, 0xFF, 0x51, 3, (uint8_t)(us >> 16), (uint8_t)(us >> 8), (uint8_t)us, 0, 0xFF, 0x2F, 0};
        fwrite("MTrk", 1, 4, f); put32(f, sizeof t); fwrite(t, 1, sizeof t, f);
    }
    uint8_t *buf = malloc((size_t)l->n * 8 + 64);
    for (int p = 0; p < TG_PARTS; ++p) {
        int k = 0;
        k += varlen(buf + k, 0);
        buf[k++] = 0xFF; buf[k++] = 0x03; buf[k++] = (uint8_t)strlen(names[p]);
        memcpy(buf + k, names[p], strlen(names[p])); k += (int)strlen(names[p]);
        uint32_t last = 0;
        for (long i = 0; i < l->n; ++i) {
            const logged *ev = &l->ev[i];
            if (ev->e.part != p) continue;
            uint32_t tick = (uint32_t)(ev->t * ticks_per_sample + 0.5);
            k += varlen(buf + k, tick - last);
            last = tick;
            buf[k++] = (uint8_t)((ev->e.on ? 0x90 : 0x80) | p);
            buf[k++] = ev->e.note;
            buf[k++] = ev->e.on ? ev->e.velocity : 64;
        }
        buf[k++] = 0; buf[k++] = 0xFF; buf[k++] = 0x2F; buf[k++] = 0;
        fwrite("MTrk", 1, 4, f); put32(f, (uint32_t)k); fwrite(buf, 1, (size_t)k, f);
    }
    free(buf);
    fclose(f);
}

/* ---- sketch synth ----------------------------------------------------------------------------- */
typedef struct { float freq, amp, env, phase, phase2; bool on; int note; float ks[2048]; int ks_len, ks_pos; } voice;

static float mtof(int n) { return 440.0f * powf(2.0f, (n - 69) / 12.0f); }

static void write_wav(const char *path, const evlog *l, int seconds, bool keys)
{
    (void)seconds;
    long total = (l->n ? l->ev[l->n - 1].t : 0) + 4L * SR;    /* to the last note, plus its tail */
    float *mix = calloc((size_t)total * 2, sizeof(float));
    /* bass and melody mono, pad 4 voices */
    static voice v[6];
    memset(v, 0, sizeof v);
    long e = 0;
    float mel_age = 0.0f, mel_freq = 0.0f;
    for (long t = 0; t < total; ++t) {
        while (e < l->n && l->ev[e].t <= t) {
            const tg_event *ev = &l->ev[e].e;
            if (ev->on) {
                int vi = ev->part == 0 ? 0 : 1;
                if (ev->part == 2) {                 /* a free pad voice, else the quietest */
                    vi = 2;
                    for (int i = 2; i < 6; ++i)
                        if (!v[i].on && (v[vi].on || v[i].env < v[vi].env)) vi = i;
                }
                voice *x = &v[vi];
                x->on = true;
                x->note = ev->note;
                x->freq = mtof(ev->note);
                x->amp = ev->velocity / 127.0f;
                if (ev->part == 1) mel_age = 0.0f;
            } else {
                for (int i = 0; i < 6; ++i) {
                    int part = i == 0 ? 0 : i == 1 ? 1 : 2;
                    if (v[i].on && v[i].note == ev->note && part == ev->part)
                        v[i].on = false;
                }
            }
            e++;
        }
        float out_l = 0, out_r = 0;
        /* bass: sine + a little second harmonic, quick attack, release */
        {
            voice *x = &v[0];
            float target = x->on ? x->amp : 0.0f;
            x->env += (target - x->env) * (x->on ? 0.01f : 0.0008f);
            x->phase += x->freq / SR; x->phase -= floorf(x->phase);
            float s = sinf(6.2831853f * x->phase) + 0.25f * sinf(12.566371f * x->phase);
            out_l += 0.35f * s * x->env; out_r += 0.35f * s * x->env;
        }
        /* melody: a soft flute-like voice, short glide between legato notes, delayed vibrato */
        {
            voice *x = &v[1];
            float target = x->on ? x->amp : 0.0f;
            x->env += (target - x->env) * (x->on ? 0.004f : 0.0015f);
            if (mel_freq <= 0.0f) mel_freq = x->freq;
            mel_freq += (x->freq - mel_freq) * 0.006f;
            mel_age += 1.0f / SR;
            float vib = mel_age > 0.25f ? 0.004f * sinf(6.2831853f * 5.2f * mel_age) : 0.0f;
            x->phase += mel_freq * (1.0f + vib) / SR; x->phase -= floorf(x->phase);
            float s = sinf(6.2831853f * x->phase) + 0.12f * sinf(18.849556f * x->phase);
            out_l += 0.30f * s * x->env; out_r += 0.24f * s * x->env;
        }
        /* pad: two detuned sines, slow envelope */
        for (int k = 2; k < 6 && keys; ++k) {
            /* keys: fast attack, decay while held, short release; a few harmonics */
            voice *x = &v[k];
            if (x->on && x->phase2 == 0.0f) { x->env = x->amp * 0.8f; x->phase2 = 1.0f; }
            if (!x->on) x->phase2 = 0.0f;
            x->env *= x->on ? 0.99985f : 0.9985f;
            x->phase += x->freq / SR; x->phase -= floorf(x->phase);
            float ph = 6.2831853f * x->phase;
            float s = sinf(ph) + 0.45f * sinf(2.0f * ph) + 0.2f * sinf(3.0f * ph) + 0.08f * sinf(5.0f * ph);
            out_l += 0.16f * s * x->env * (k % 2 ? 1.0f : 0.7f);
            out_r += 0.16f * s * x->env * (k % 2 ? 0.7f : 1.0f);
        }
        for (int k = 2; k < 6 && !keys; ++k) {
            voice *x = &v[k];
            float target = x->on ? x->amp * 0.6f : 0.0f;
            x->env += (target - x->env) * (x->on ? 0.00008f : 0.00004f);
            x->phase += x->freq / SR; x->phase -= floorf(x->phase);
            x->phase2 += x->freq * 1.004f / SR; x->phase2 -= floorf(x->phase2);
            float s = 0.5f * (sinf(6.2831853f * x->phase) + sinf(6.2831853f * x->phase2));
            out_l += 0.16f * s * x->env * (k % 2 ? 1.0f : 0.6f);
            out_r += 0.16f * s * x->env * (k % 2 ? 0.6f : 1.0f);
        }
        mix[t * 2] = out_l;
        mix[t * 2 + 1] = out_r;
    }
    /* ping-pong delay, about 3/8 of a second */
    long d = (long)(0.375f * SR);
    for (long t = d; t < total; ++t) {
        mix[t * 2] += 0.35f * mix[(t - d) * 2 + 1];
        mix[t * 2 + 1] += 0.35f * mix[(t - d) * 2];
    }
    float peak = 1e-6f;
    for (long i = 0; i < total * 2; ++i) if (fabsf(mix[i]) > peak) peak = fabsf(mix[i]);
    float gain = 0.89f / peak;
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    uint32_t data = (uint32_t)(total * 4);
    fwrite("RIFF", 1, 4, f);
    uint32_t v32 = 36 + data; fwrite(&v32, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f);
    v32 = 16; fwrite(&v32, 4, 1, f);
    uint16_t v16 = 1; fwrite(&v16, 2, 1, f);
    v16 = 2; fwrite(&v16, 2, 1, f);
    v32 = SR; fwrite(&v32, 4, 1, f);
    v32 = SR * 4; fwrite(&v32, 4, 1, f);
    v16 = 4; fwrite(&v16, 2, 1, f);
    v16 = 16; fwrite(&v16, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&data, 4, 1, f);
    for (long i = 0; i < total * 2; ++i) {
        int s = (int)lrintf(mix[i] * gain * 32767.0f);
        int16_t s16 = (int16_t)(s > 32767 ? 32767 : s < -32768 ? -32768 : s);
        fwrite(&s16, 2, 1, f);
    }
    fclose(f);
    free(mix);
}

static int check(const preset *p, const evlog *pa, bool same, bool verbose)
{
    const evlog a = *pa;
    int fails = 0;
        long count[TG_PARTS] = {0};
        int lo = 127, hi = 0;
        for (long k = 0; k < a.n; ++k)
            if (a.ev[k].e.on) {
                count[a.ev[k].e.part]++;
                int n = a.ev[k].e.note;
                lo = n < lo ? n : lo;
                hi = n > hi ? n : hi;
            }
        if (verbose) printf("%-8s %ld events (bass %ld, melody %ld, third %ld), notes %d..%d, off-key %ld, seed replays: %s\n"
               "         melody: %.0f%% steps (<= 2 semitones), downbeats on chord tones %ld/%ld,"
               " arrivals on the tonic %ld/%ld (over the tonic chord %ld, by step %ld)\n",
               p->name, a.n, count[0], count[1], count[2], lo, hi, a.off_key, same ? "yes" : "NO",
               a.mel_notes ? 100.0 * a.mel_steps / a.mel_notes : 0.0, a.mel_downbeat_chord, a.mel_downbeats,
               a.arrivals_tonic, a.arrivals, a.arrivals_tonic_chord, a.arrivals_by_step);
        if (p->style >= 0)
            if (verbose) printf("         style: half cadences on the dominant %ld/%ld, bass on the root at chord changes %ld/%ld,"
                   " counter-melody parallels %ld/%ld, melody from the motif %.0f%%\n",
                   a.halves_on_dominant, a.halves, a.bass_roots, a.bass_changes, a.parallels, a.counter_notes,
                   a.motif_share);
        fails += !same || a.off_key || !count[0] || !count[1] || !count[2];
        fails += a.mel_downbeat_chord != a.mel_downbeats;
        fails += !a.arrivals || a.arrivals_tonic != a.arrivals || a.arrivals_tonic_chord != a.arrivals ||
                 a.arrivals_by_step != a.arrivals;
        if (p->style >= 0)
            fails += a.halves_on_dominant != a.halves || a.bass_roots != a.bass_changes || a.parallels;
    return fails;
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    int seconds = argc > 2 ? atoi(argv[2]) : 60;
    int fails = 0;
    if (getenv("SWEEP")) {
        /* the same checks over many seeds per preset, no files */
        int seeds = atoi(getenv("SWEEP"));
        for (size_t i = 0; i < sizeof presets / sizeof presets[0]; ++i) {
            int bad = 0;
            for (int sd = 1; sd <= seeds; ++sd) {
                preset q = presets[i];
                q.seed = (uint32_t)(sd * 7919 + 17);
                evlog a;
                run(&q, seconds, &a);
                int f = check(&q, &a, true, false);
                if (f && bad < 2) {
                    printf("  %s seed %u fails:\n", q.name, q.seed);
                    check(&q, &a, true, true);
                }
                bad += f > 0;
                free(a.ev);
            }
            printf("%-12s %d/%d seeds pass\n", presets[i].name, seeds - bad, seeds);
            fails += bad;
        }
        return fails ? 1 : 0;
    }
    for (size_t i = 0; i < sizeof presets / sizeof presets[0]; ++i) {
        const preset *p = &presets[i];
        evlog a, b;
        run(p, seconds, &a);
        run(p, seconds, &b);
        bool same = a.n == b.n;     /* field by field: the structs carry padding bytes */
        for (long k = 0; same && k < a.n; ++k)
            same = a.ev[k].t == b.ev[k].t && a.ev[k].e.part == b.ev[k].e.part && a.ev[k].e.note == b.ev[k].e.note &&
                   a.ev[k].e.velocity == b.ev[k].e.velocity && a.ev[k].e.on == b.ev[k].e.on;
        fails += check(p, &a, same, true);
        char path[512];
        snprintf(path, sizeof path, "%s/tuna_%s.mid", dir, p->name);
        write_midi(path, &a, p->style >= 0 ? tg_style_get(p->style)->bpm : p->bpm);
        snprintf(path, sizeof path, "%s/tuna_%s.wav", dir, p->name);
        write_wav(path, &a, seconds, p->keys);
        free(a.ev);
        free(b.ev);
    }
    return fails ? 1 : 0;
}
