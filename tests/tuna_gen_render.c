/* SPDX-License-Identifier: GPL-3.0-only */
/* Render TUNA generator presets to MIDI and WAV, for listening before there is hardware.
 *   cc -O2 -Ifirmware/src -o build/host/tuna_gen_render tests/tuna_gen_render.c \
 *      firmware/src/tuna_gen.c firmware/src/tuna_marbles.c firmware/src/tuna_marbles_tables.c \
 *      firmware/src/tuna_theory.c -lm
 *   build/host/tuna_gen_render OUTDIR [SECONDS]
 * The WAV uses a tiny sketch synth (sine bass, plucked melody, soft pad, a delay), not Felucca's
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
} preset;

static const preset presets[] = {
    /* name     bpm  div root scale        prog           chord  T model                   bias  jit   dv    pw    pwstd len  spread xbias steps xdv  xlen ctrl                  seed */
    {"tuin",    72,  2,  2,  TS_DORIAN,     TP_MODAL_VAMP,  16, TM_T_COMPLEMENTARY_BERNOULLI, 0.5f, 0.15f, 0.6f, 0.75f, 0.2f, 8,  0.40f, 0.5f, 0.80f, 0.70f, 8, TM_CONTROL_TILT,      1},
    {"ritme",   104, 4,  9,  TS_AEOLIAN,    TP_ANDALUSIAN,  32, TM_T_DRUMS,                 0.8f, 0.0f,  0.75f, 0.3f, 0.0f, 16, 0.55f, 0.5f, 0.68f, 0.80f, 16, TM_CONTROL_IDENTICAL, 7},
    {"lydisch", 84,  2,  5,  TS_LYDIAN,     TP_MODAL_VAMP,  16, TM_T_CLUSTERS,              0.65f, 0.3f, 0.4f, 0.6f, 0.4f, 8,  0.50f, 0.55f, 0.75f, 0.55f, 8, TM_CONTROL_BUMP,      42},
};

/* ---- event log (for MIDI and the synth) ------------------------------------------------------- */
typedef struct { long t; tg_event e; } logged;
typedef struct { logged *ev; long n, cap; long now; } evlog;

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
}

static void run(const preset *p, int seconds, evlog *l)
{
    static tg_gen g;
    tg_init(&g, p->seed, (float)SR);
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
    memset(l, 0, sizeof *l);
    long total = (long)seconds * SR;
    for (l->now = 0; l->now < total; l->now += BLOCK)
        tg_process(&g, BLOCK, on_event, l);
    tg_all_off(&g, on_event, l);
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

static void write_wav(const char *path, const evlog *l, int seconds)
{
    long total = (long)(seconds + 3) * SR;
    float *mix = calloc((size_t)total * 2, sizeof(float));
    /* pad is polyphonic-ish: 3 voices round robin; bass and melody mono */
    static voice v[5];
    memset(v, 0, sizeof v);
    int pad_next = 0;
    long e = 0;
    uint32_t noise = 22222;
    for (long t = 0; t < total; ++t) {
        while (e < l->n && l->ev[e].t <= t) {
            const tg_event *ev = &l->ev[e].e;
            int vi = ev->part == 0 ? 0 : ev->part == 1 ? 1 : 2 + pad_next;
            if (ev->on) {
                if (ev->part == 2) pad_next = (pad_next + 1) % 3;
                voice *x = &v[vi];
                x->on = true;
                x->note = ev->note;
                x->freq = mtof(ev->note);
                x->amp = ev->velocity / 127.0f;
                if (ev->part == 1) {        /* Karplus-Strong pluck */
                    x->ks_len = (int)(SR / x->freq);
                    if (x->ks_len > 2047) x->ks_len = 2047;
                    for (int i = 0; i < x->ks_len; ++i) {
                        noise = noise * 1664525u + 1013904223u;
                        x->ks[i] = ((noise >> 9) / 4194304.0f - 1.0f) * x->amp;
                    }
                    x->ks_pos = 0;
                }
            } else {
                for (int i = 0; i < 5; ++i)
                    if (v[i].on && v[i].note == ev->note && ((ev->part == 2) == (i >= 2)) && ((ev->part == 0) == (i == 0)))
                        v[i].on = false;
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
        /* melody: pluck */
        {
            voice *x = &v[1];
            if (x->ks_len) {
                int i = x->ks_pos, j = (i + 1) % x->ks_len;
                float s = x->ks[i];
                x->ks[i] = 0.5f * (x->ks[i] + x->ks[j]) * 0.996f;
                x->ks_pos = j;
                out_l += 0.30f * s; out_r += 0.22f * s;
            }
        }
        /* pad: two detuned sines, slow envelope */
        for (int k = 2; k < 5; ++k) {
            voice *x = &v[k];
            float target = x->on ? x->amp * 0.6f : 0.0f;
            x->env += (target - x->env) * (x->on ? 0.00008f : 0.00004f);
            x->phase += x->freq / SR; x->phase -= floorf(x->phase);
            x->phase2 += x->freq * 1.004f / SR; x->phase2 -= floorf(x->phase2);
            float s = 0.5f * (sinf(6.2831853f * x->phase) + sinf(6.2831853f * x->phase2));
            out_l += 0.22f * s * x->env * (k == 2 ? 1.0f : 0.7f);
            out_r += 0.22f * s * x->env * (k == 4 ? 1.0f : 0.7f);
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

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    int seconds = argc > 2 ? atoi(argv[2]) : 60;
    int fails = 0;
    for (size_t i = 0; i < sizeof presets / sizeof presets[0]; ++i) {
        const preset *p = &presets[i];
        evlog a, b;
        run(p, seconds, &a);
        run(p, seconds, &b);
        bool same = a.n == b.n;     /* field by field: the structs carry padding bytes */
        for (long k = 0; same && k < a.n; ++k)
            same = a.ev[k].t == b.ev[k].t && a.ev[k].e.part == b.ev[k].e.part && a.ev[k].e.note == b.ev[k].e.note &&
                   a.ev[k].e.velocity == b.ev[k].e.velocity && a.ev[k].e.on == b.ev[k].e.on;
        /* allowed pitch classes: the scale plus every chord tone of the progression */
        uint16_t allowed = tt_scale_mask(p->scale);
        for (int s = 0; s < tt_prog_len(p->prog); ++s) {
            tt_chord c;
            tt_prog_chord(&c, p->prog, s, p->scale, 4);
            for (int k = 0; k < c.n; ++k)
                allowed |= (uint16_t)(1u << ((c.root + c.tone[k]) % 12));
        }
        long count[TG_PARTS] = {0}, off_key = 0;
        int lo = 127, hi = 0;
        for (long k = 0; k < a.n; ++k)
            if (a.ev[k].e.on) {
                count[a.ev[k].e.part]++;
                int n = a.ev[k].e.note;
                lo = n < lo ? n : lo;
                hi = n > hi ? n : hi;
                off_key += !((allowed >> (((n - p->root) % 12 + 12) % 12)) & 1);
            }
        printf("%-8s %ld events (bass %ld, melody %ld, pad %ld), notes %d..%d, off-key %ld, seed replays: %s\n",
               p->name, a.n, count[0], count[1], count[2], lo, hi, off_key, same ? "yes" : "NO");
        fails += !same || off_key || !count[0] || !count[1] || !count[2];
        char path[512];
        snprintf(path, sizeof path, "%s/tuna_%s.mid", dir, p->name);
        write_midi(path, &a, p->bpm);
        snprintf(path, sizeof path, "%s/tuna_%s.wav", dir, p->name);
        write_wav(path, &a, seconds);
        free(a.ev);
        free(b.ev);
    }
    return fails ? 1 : 0;
}
