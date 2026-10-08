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
    int manner;               /* the composer's manner (tg_manner_count) */
    bool keys;                /* sketch synth: a keyboard sound for the third part (figures) */
    int drums;                /* 0: the style's default, 1: off, 2: period, 3: kit */
    bool evolve;              /* the evolution timer: 8-period cycles, full depth, with modulation */
} preset;

static const preset presets[] = {
    /* name     bpm  div root scale        prog           chord  T model                   bias  jit   dv    pw    pwstd len  spread xbias steps xdv  xlen ctrl                  seed dens pad restrike */
    {"garden",  72,  2,  2,  TS_DORIAN,     TP_MODAL_VAMP,  16, TM_T_COMPLEMENTARY_BERNOULLI, 0.5f, 0.15f, 0.6f, 0.75f, 0.2f, 8,  0.40f, 0.5f, 0.80f, 0.70f, 8, TM_CONTROL_TILT,      1,  0.35f, 4, false, -1, false, 0, false},
    {"rhythm",  104, 4,  9,  TS_AEOLIAN,    TP_ANDALUSIAN,  32, TM_T_DRUMS,                 0.8f, 0.0f,  0.75f, 0.3f, 0.0f, 16, 0.55f, 0.5f, 0.68f, 0.80f, 16, TM_CONTROL_IDENTICAL, 7,  0.55f, 3, true, -1, false, 3, false},
    {"lydian",  84,  2,  5,  TS_LYDIAN,     TP_MODAL_VAMP,  16, TM_T_CLUSTERS,              0.65f, 0.3f, 0.4f, 0.6f, 0.4f, 8,  0.50f, 0.55f, 0.75f, 0.55f, 8, TM_CONTROL_BUMP,      42, 0.40f, 4, false, -1, false, 0, false},
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
    /* percussion is a switch on every composer */
    {.name = "bach-timpani", .seed = 11, .style = TG_STYLE_BACH,       .keys = true, .drums = 2},
    {.name = "mozart-kit",   .seed = 5,  .style = TG_STYLE_MOZART,     .keys = true, .drums = 3},
    /* the evolution timer: an arc of energy and a modulation away and home */
    {.name = "mozart-evolve",  .seed = 5,  .style = TG_STYLE_MOZART,  .keys = true, .evolve = true},
    {.name = "bach-evolve",    .seed = 11, .style = TG_STYLE_BACH,    .keys = true, .evolve = true},
    {.name = "mahler-evolve",  .seed = 2,  .style = TG_STYLE_MAHLER,  .evolve = true},
    {.name = "dvorak-evolve",  .seed = 12, .style = TG_STYLE_DVORAK,  .evolve = true},
};

/* ---- event log (for MIDI and the synth) ------------------------------------------------------- */
typedef struct { long t; tg_event e; } logged;
typedef struct {
    logged *ev;
    long n, cap, now;
    /* musical checks, harmonized mode */
    long mel_notes, mel_steps, mel_downbeats, mel_downbeat_chord;
    long arrivals, arrivals_tonic, arrivals_tonic_chord, arrivals_by_step, arrivals_open, arrivals_open_ok;
    long off_key, halves, halves_on_dominant, bass_changes, bass_roots, counter_notes, parallels;
    double motif_share;
    long timp_bad, drum_hits, timp_hits;
    int counter_prev, counter_prev_iv, counter_prev_mel;
    int mel_prev;
    bool mel_prev_phrase;
} evlog;

static tg_gen *cur;
static int gen_div = 1;   /* TUNA_DIV: the generator at SR / gen_div, as the firmware may run it */     /* the generator being rendered, for the checks in on_event */

static void on_event(void *ctx, const tg_event *e)
{
    evlog *l = ctx;
    if (e->type != TG_EV_NOTE)
        return;                                   /* sound locks: for the host, not the notes */
    if (l->n == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 1024;
        l->ev = realloc(l->ev, (size_t)l->cap * sizeof *l->ev);
    }
    l->ev[l->n].t = l->now + (long)e->offset * gen_div;
    l->ev[l->n].e = *e;
    l->n++;
    if (e->on && e->part == TG_PART_TIMPANI) {
        int pc = ((e->note - cur->root) % 12 + 12) % 12;
        l->timp_bad += pc != 0 && pc != 7;       /* kettledrums: tonic and dominant only */
        l->timp_hits++;
    }
    if (e->on && e->part == TG_PART_DRUMS)
        l->drum_hits++;
    if (e->on && e->part < TG_PART_DRUMS) {
        /* in key: the scale, the current chord, or the scale bent to the chord */
        uint16_t allowed = tt_scale_mask(cur->period_scale >= 0 ? cur->period_scale : cur->scale) | tt_scale_mask(cur->scale) |
                           cur->chord_scale | cur->mel_scale;     /* a coloured period is still in its key */
        for (int k = 0; k < cur->chord.n; ++k)
            allowed |= (uint16_t)(1u << ((cur->chord.root + cur->chord.tone[k]) % 12));
        if (cur->poly_semis && e->part == TG_PART_PAD)      /* a polychord's upper triad, bitonal on purpose */
            for (int k = 0; k < 3; ++k)
                allowed |= (uint16_t)(1u << ((cur->chord.root + cur->poly_semis + (k == 0 ? 0 : k == 1 ? 4 : 7)) % 12));
        int pc = ((e->note - cur->root) % 12 + 12) % 12;
        bool leaning = e->part == TG_PART_MELODY && cur->mel_pending >= 0;   /* a chromatic appoggiatura */
        if (!((allowed >> pc) & 1) && !leaning && getenv("OFFKEY"))
            printf("    off key: tick %ld part %d note %d pc %d key %d scale %d period_scale %d chord %d/%d n%d tones %d %d %d, "
                   "pending %d key_pending %d\n", cur->ticks - 1, e->part, e->note, pc, cur->root, cur->scale,
                   cur->period_scale, cur->chord.root, cur->chord_step, cur->chord.n, cur->chord.tone[0], cur->chord.tone[1],
                   cur->chord.tone[2], cur->mel_pending, cur->key_pending);
        l->off_key += !((allowed >> pc) & 1) && !leaning;
    }
    bool root_bass = cur->bass_mode == TG_BASS_ROOT || cur->bass_mode == TG_BASS_BAR || cur->bass_mode == TG_BASS_WALKING ||
                     cur->bass_mode == TG_BASS_MARCH || cur->bass_mode == TG_BASS_REPEAT || cur->bass_mode == TG_BASS_BEAT;
    if (e->on && e->part == TG_PART_BASS && cur->style && root_bass && cur->chord_ticks > 0 &&
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
        /* the phrase checks hold for phrases (and chant); the other melody modes have their own tests; an
         * arrival counts when the note before it was a phrase's too (contrast periods change the mode) */
        bool phrase_now = cur->mel_mode == TG_MEL_PHRASE || cur->mel_mode == TG_MEL_CHANT;
        bool phrase = phrase_now && l->mel_prev_phrase;
        l->mel_prev_phrase = phrase_now;
        int q = (int)(cur->ticks - 1);
        int period = 0, ppos = tg_period_position(cur, &period);
        bool aug = cur->chord.n >= 3 && cur->chord.tone[1] == 4 && cur->chord.tone[2] == 8;
        bool octa = cur->period_scale == TS_OCT_HW || cur->period_scale == TS_OCT_WH;
        if (phrase && cur->style && cur->harmony == TG_HARM_PLANNED && ppos == tg_half_position(cur) && !aug && !octa &&
            !cur->plan_wt && cur->plan_len >= 4) {
            l->halves++;
            const tg_chord_def *v = tg_vocab_of(cur->style->vocab, cur->plan_minor);
            l->halves_on_dominant += v[cur->plan[cur->plan_len / 2 - 1]].func == TF_D;
        }
        if (phrase && q > 0 && ppos == 0) {
            int pc = ((e->note - cur->root) % 12 + 12) % 12;
            l->arrivals++;
            l->arrivals_by_step += step >= 1 && step <= 2;
            if (cur->chord.root == 0 && !cur->open_prev) {
                l->arrivals_tonic += pc == 0;
                l->arrivals_tonic_chord++;
            } else {
                bool tone = false;              /* an open ending: a tone of the chord it lands on */
                for (int c = 0; c < cur->chord.n && c < 3; ++c)
                    tone |= (cur->chord.root + cur->chord.tone[c]) % 12 == pc;
                l->arrivals_open++;
                l->arrivals_open_ok += tone;
            }
        }
        tg_meter_pos m;
        tg_meter_at(cur, q, &m);
        if (cur->mel_mode == TG_MEL_PHRASE && m.bar_start) {
            l->mel_downbeats++;
            int pc = ((e->note - cur->root) % 12 + 12) % 12;
            tt_chord c = cur->chord;
            if (cur->mel_pending >= 0 || cur->orn_n > 0) {
                l->mel_downbeat_chord++;      /* an appoggiatura or a turn: a non-chord tone on purpose */
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
    tg_init(&g, p->seed, (float)SR / (float)gen_div);
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
        tg_apply_manner(&g, p->style, p->manner);
        tg_reseed(&g, p->seed);
    }
    if (p->drums)
        tg_set_percussion(&g, p->drums - 1);
    if (p->evolve)
        tg_evolve(&g, true, 8, 1.0f, true);
    cur = &g;
    memset(l, 0, sizeof *l);
    l->mel_prev = -1;
    l->mel_prev_phrase = true;
    l->counter_prev = -1;
    long total = (long)seconds * SR;
    for (l->now = 0; l->now < total; l->now += BLOCK)
        tg_process(&g, BLOCK / gen_div, on_event, l);
    /* end like a song: play on to the next arrival on the tonic and let it ring for its bar */
    long arrived = -1;
    while (1) {
        tg_process(&g, BLOCK / gen_div, on_event, l);
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
    static const char *names[TG_EVENT_PARTS] = {"Bass", "Melody", "Third", "Drums", "Timpani"};
    static const uint8_t channel[TG_EVENT_PARTS] = {0, 1, 2, 9, 4};      /* drums on GM channel 10 */
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fwrite("MThd", 1, 4, f); put32(f, 6); put16(f, 1); put16(f, TG_EVENT_PARTS + 1); put16(f, (uint16_t)ppq);
    /* tempo track */
    {
        uint32_t us = (uint32_t)(60000000.0 / bpm);
        uint8_t t[] = {0, 0xFF, 0x51, 3, (uint8_t)(us >> 16), (uint8_t)(us >> 8), (uint8_t)us, 0, 0xFF, 0x2F, 0};
        fwrite("MTrk", 1, 4, f); put32(f, sizeof t); fwrite(t, 1, sizeof t, f);
    }
    uint8_t *buf = malloc((size_t)l->n * 8 + 64);
    for (int p = 0; p < TG_EVENT_PARTS; ++p) {
        int k = 0;
        k += varlen(buf + k, 0);
        buf[k++] = 0xFF; buf[k++] = 0x03; buf[k++] = (uint8_t)strlen(names[p]);
        memcpy(buf + k, names[p], strlen(names[p])); k += (int)strlen(names[p]);
        if (p == TG_PART_TIMPANI) {
            k += varlen(buf + k, 0);
            buf[k++] = (uint8_t)(0xC0 | channel[p]);
            buf[k++] = 47;                      /* GM program 48: timpani */
        }
        uint32_t last = 0;
        for (long i = 0; i < l->n; ++i) {
            const logged *ev = &l->ev[i];
            if (ev->e.part != p) continue;
            uint32_t tick = (uint32_t)(ev->t * ticks_per_sample + 0.5);
            k += varlen(buf + k, tick - last);
            last = tick;
            buf[k++] = (uint8_t)((ev->e.on ? 0x90 : 0x80) | channel[p]);
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

/* percussion for the sketch synth: one-shot strokes */
typedef struct { int kind; float age, vel, freq; bool live; } stroke;
#define MAX_STROKES 32
static stroke strokes[MAX_STROKES];
static uint32_t drum_noise_state = 12345;
static float drum_noise(void)
{
    drum_noise_state = drum_noise_state * 1664525u + 1013904223u;
    return (float)(drum_noise_state >> 8) / 8388608.0f - 1.0f;
}

static void stroke_start(int part, int note, int velocity)
{
    int slot = 0;
    for (int i = 0; i < MAX_STROKES; ++i) {
        if (!strokes[i].live) { slot = i; break; }
        if (strokes[i].age > strokes[slot].age) slot = i;
    }
    stroke *k = &strokes[slot];
    k->kind = part == TG_PART_TIMPANI ? -1 : note;
    k->freq = part == TG_PART_TIMPANI ? mtof(note) : 0.0f;
    k->vel = velocity / 127.0f;
    k->age = 0.0f;
    k->live = true;
}

/* one sample of all strokes, stereo */
static void strokes_render(float *out_l, float *out_r)
{
    static float hp_prev;
    float n = drum_noise();
    float hp = n - hp_prev;          /* a crude high-pass for metal */
    hp_prev = n;
    for (int i = 0; i < MAX_STROKES; ++i) {
        stroke *k = &strokes[i];
        if (!k->live)
            continue;
        float a = k->age, v = k->vel, x = 0.0f, pan = 0.5f;
        switch (k->kind) {
        case -1: {                      /* timpani: partials 1, 1.5, 1.98, a slight pitch drop, a felt attack */
            float f = k->freq * (1.0f + 0.03f * expf(-a * 25.0f));
            x = (sinf(6.2831853f * f * a) + 0.5f * sinf(6.2831853f * 1.5f * f * a) +
                 0.25f * sinf(6.2831853f * 1.98f * f * a)) * expf(-a * 1.4f) * 0.55f + n * expf(-a * 70.0f) * 0.25f;
            pan = 0.35f;
            if (a > 3.0f) k->live = false;
            break;
        }
        case TG_GM_KICK: case TG_GM_BASS_DRUM: {
            bool big = k->kind == TG_GM_BASS_DRUM;
            float f0 = big ? 38.0f : 45.0f, f1 = big ? 60.0f : 120.0f;
            float ph = 6.2831853f * (f0 * a + (f1 - f0) / 30.0f * (1.0f - expf(-a * 30.0f)));
            x = sinf(ph) * expf(-a * (big ? 3.5f : 8.0f)) * 0.9f;
            if (a > (big ? 1.5f : 0.6f)) k->live = false;
            break;
        }
        case TG_GM_SNARE:
            x = (n * expf(-a * 16.0f) * 0.55f + sinf(6.2831853f * 190.0f * a) * expf(-a * 25.0f) * 0.35f);
            pan = 0.55f;
            if (a > 0.5f) k->live = false;
            break;
        case TG_GM_SIDE_STICK:
            x = (n * expf(-a * 70.0f) * 0.4f + sinf(6.2831853f * 820.0f * a) * expf(-a * 50.0f) * 0.3f);
            pan = 0.6f;
            if (a > 0.15f) k->live = false;
            break;
        case TG_GM_HAT_CLOSED: case TG_GM_HAT_OPEN:
            x = hp * expf(-a * (k->kind == TG_GM_HAT_OPEN ? 7.0f : 45.0f)) * 0.3f;
            pan = 0.7f;
            if (a > 0.8f) k->live = false;
            break;
        case TG_GM_CRASH:
            x = hp * expf(-a * 1.8f) * 0.32f;
            pan = 0.3f;
            if (a > 3.0f) k->live = false;
            break;
        case TG_GM_TAMBOURINE:
            x = hp * expf(-a * 11.0f) * (0.6f + 0.4f * sinf(6.2831853f * 26.0f * a)) * 0.35f;
            pan = 0.75f;
            if (a > 0.5f) k->live = false;
            break;
        case TG_GM_TRIANGLE:
            x = (sinf(6.2831853f * 1250.0f * a) + 0.6f * sinf(6.2831853f * 3140.0f * a)) * expf(-a * 2.2f) * 0.12f;
            pan = 0.8f;
            if (a > 2.5f) k->live = false;
            break;
        default:
            k->live = false;
            break;
        }
        x *= v;
        *out_l += x * (1.0f - pan) * 1.4f;
        *out_r += x * pan * 1.4f;
        k->age += 1.0f / SR;
    }
}

static void write_wav(const char *path, const evlog *l, int seconds, bool keys)
{
    memset(strokes, 0, sizeof strokes);
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
            if (ev->part >= TG_PART_DRUMS) {         /* percussion: one-shot strokes, offs ignored */
                if (ev->on)
                    stroke_start(ev->part, ev->note, ev->velocity);
                e++;
                continue;
            }
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
        strokes_render(&out_l, &out_r);
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

/* switching percussion must not change one note of the three voices */
static bool voices_identical(const evlog *a, const evlog *b)
{
    long i = 0, j = 0;
    while (1) {
        while (i < a->n && a->ev[i].e.part >= TG_PART_DRUMS) i++;
        while (j < b->n && b->ev[j].e.part >= TG_PART_DRUMS) j++;
        if (i >= a->n || j >= b->n)
            return i >= a->n && j >= b->n;
        const logged *x = &a->ev[i], *y = &b->ev[j];
        if (x->t != y->t || x->e.part != y->e.part || x->e.note != y->e.note || x->e.on != y->e.on ||
            (x->e.on && x->e.velocity != y->e.velocity))
            return false;
        i++, j++;
    }
}

/* the preset's style as it plays: the composer's profile with its manner's changes */
static const tg_style *style_of(const preset *p)
{
    static tg_style buf;
    if (p->style < 0)
        return NULL;
    tg_style_manner(&buf, p->style, p->manner);
    return &buf;
}

static int check(const preset *p, const evlog *pa, bool same, bool verbose)
{
    const evlog a = *pa;
    int fails = 0;
        long count[TG_EVENT_PARTS] = {0};
        int lo = 127, hi = 0;
        for (long k = 0; k < a.n; ++k)
            if (a.ev[k].e.on) {
                count[a.ev[k].e.part]++;
                if (a.ev[k].e.part >= TG_PART_DRUMS)
                    continue;
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
        if (verbose && a.arrivals_open)
            printf("         open endings: %ld, on a tone of their chord %ld\n", a.arrivals_open, a.arrivals_open_ok);
        if (p->style >= 0)
            if (verbose) printf("         style: half cadences on the dominant %ld/%ld, bass on the root at chord changes %ld/%ld,"
                   " counter-melody parallels %ld/%ld, melody from the motif %.0f%%\n",
                   a.halves_on_dominant, a.halves, a.bass_roots, a.bass_changes, a.parallels, a.counter_notes,
                   a.motif_share);
        if (verbose && (a.drum_hits || a.timp_hits))
            printf("         percussion: %ld drum strokes, %ld timpani strokes (%ld off tonic/dominant)\n",
                   a.drum_hits, a.timp_hits, a.timp_bad);
        {
            const tg_style *st = style_of(p);
            bool no_bass = st && st->bass_mode == TG_BASS_NONE, no_pad = st && st->pad_mode == TG_PAD_NONE;
            fails += !same || a.off_key || (!count[0] && !no_bass) || !count[1] || (!count[2] && !no_pad) || a.timp_bad;
        }
        fails += a.mel_downbeat_chord != a.mel_downbeats;
        {
            const tg_style *st = style_of(p);
            bool phrase = !st || st->mel_mode == TG_MEL_PHRASE || st->mel_mode == TG_MEL_CHANT;
            if (phrase) {
                fails += (!a.arrivals && !(st && st->alt.mel_mode)) || a.arrivals_tonic != a.arrivals_tonic_chord ||
                         a.arrivals_by_step != a.arrivals ||
                         a.arrivals_open_ok != a.arrivals_open;
                if (!st || (st->open <= 0.0f && st->harmony == TG_HARM_PLANNED))
                    fails += a.arrivals_tonic_chord != a.arrivals;   /* no open endings: every arrival on the tonic */
            }
        }
        if (getenv("WHY"))
            printf("    why: same %d off %ld counts %ld %ld %ld timp %ld down %ld/%ld arr %ld tonic %ld/%ld step %ld open %ld/%ld halves %ld/%ld roots %ld/%ld par %ld\n",
                   same, a.off_key, count[0], count[1], count[2], a.timp_bad, a.mel_downbeat_chord, a.mel_downbeats, a.arrivals,
                   a.arrivals_tonic, a.arrivals_tonic_chord, a.arrivals_by_step, a.arrivals_open_ok, a.arrivals_open,
                   a.halves_on_dominant, a.halves, a.bass_roots, a.bass_changes, a.parallels);
        if (p->style >= 0)
            fails += a.halves_on_dominant != a.halves || a.bass_roots != a.bass_changes || a.parallels;
    return fails;
}

/* STYLES=a-b (or one id): every composer in the range as a preset, instead of the list above; each of its
 * manners a preset of its own (bach, bach-chorale, bach-gigue) */
static preset style_presets[TG_STYLE_COUNT * 4];
static char style_names[TG_STYLE_COUNT * 4][36];

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    int seconds = argc > 2 ? atoi(argv[2]) : 60;
    int fails = 0;
    const preset *list = presets;
    size_t nlist = sizeof presets / sizeof presets[0];
    if (getenv("STYLES")) {
        int a = 0, b = TG_STYLE_COUNT - 1;
        if (sscanf(getenv("STYLES"), "%d-%d", &a, &b) == 1) b = a;
        nlist = 0;
        for (int i = a; i <= b && i < TG_STYLE_COUNT; ++i)
            for (int m = 0; m < tg_manner_count(i) && m < 4; ++m) {
                char *o = style_names[nlist];
                const char *n = tg_style_get(i)->name;
                int k = 0;
                for (; n[k] && k < 23; ++k) o[k] = (char)(n[k] >= 'A' && n[k] <= 'Z' ? n[k] + 32 : n[k] == ' ' ? '-' : n[k]);
                if (m) {                                  /* the first manner keeps the plain name */
                    o[k++] = '-';
                    for (const char *t = tg_manner_name(i, m); *t && k < 35; ++t)
                        o[k++] = (char)(*t >= 'A' && *t <= 'Z' ? *t + 32 : *t == ' ' ? '-' : *t);
                }
                o[k] = 0;
                preset q = {.name = o, .seed = 1, .style = i, .manner = m, .keys = true};
                style_presets[nlist++] = q;
            }
        list = style_presets;
    }
    const char *only = getenv("ONLY");
    if (getenv("TUNA_DIV"))
        gen_div = atoi(getenv("TUNA_DIV"));
    if (gen_div != 1 && gen_div != 2 && gen_div != 4 && gen_div != 8)
        gen_div = 1;
    if (getenv("SWEEP")) {
        /* the same checks over many seeds per preset, no files */
        int seeds = atoi(getenv("SWEEP"));
        for (size_t i = 0; i < nlist; ++i) {
            if (only && strcmp(only, list[i].name))
                continue;
            int bad = 0;
            for (int sd = 1; sd <= seeds; ++sd) {
                preset q = list[i];
                q.seed = (uint32_t)(sd * 7919 + 17);
                evlog a;
                run(&q, seconds, &a);
                int f = check(&q, &a, true, false);
                if (a.drum_hits || a.timp_hits) {
                    preset off = q;
                    off.drums = 1;
                    evlog z;
                    run(&off, seconds, &z);
                    if (!voices_identical(&a, &z)) {
                        f++;
                        if (bad < 2) printf("  %s seed %u: percussion changed the notes\n", q.name, q.seed);
                    }
                    free(z.ev);
                }
                if (f && bad < 2) {
                    printf("  %s seed %u fails:\n", q.name, q.seed);
                    check(&q, &a, true, true);
                }
                bad += f > 0;
                free(a.ev);
            }
            printf("%-14s %d/%d seeds pass\n", list[i].name, seeds - bad, seeds);
            fails += bad;
        }
        return fails ? 1 : 0;
    }
    for (size_t i = 0; i < nlist; ++i) {
        const preset *p = &list[i];
        if (only && strcmp(only, p->name))
            continue;
        evlog a, b;
        run(p, seconds, &a);
        run(p, seconds, &b);
        bool same = a.n == b.n;     /* field by field: the structs carry padding bytes */
        for (long k = 0; same && k < a.n; ++k)
            same = a.ev[k].t == b.ev[k].t && a.ev[k].e.part == b.ev[k].e.part && a.ev[k].e.note == b.ev[k].e.note &&
                   a.ev[k].e.velocity == b.ev[k].e.velocity && a.ev[k].e.on == b.ev[k].e.on;
        fails += check(p, &a, same, true);
        if (a.drum_hits || a.timp_hits) {
            preset off = *p;
            off.drums = 1;
            evlog z;
            run(&off, seconds, &z);
            bool ident = voices_identical(&a, &z);
            printf("         notes identical with percussion off: %s\n", ident ? "yes" : "NO");
            fails += !ident;
            free(z.ev);
        }
        char path[512];
        snprintf(path, sizeof path, "%s/tuna_%s.mid", dir, p->name);
        write_midi(path, &a, p->style >= 0 ? tg_style_get(p->style)->bpm : p->bpm);
        snprintf(path, sizeof path, "%s/tuna_%s.wav", dir, p->name);
        if (!getenv("NOWAV"))
            write_wav(path, &a, seconds, p->keys);
        free(a.ev);
        free(b.ev);
    }
    return fails ? 1 : 0;
}
