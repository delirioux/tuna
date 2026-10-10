/* SPDX-License-Identifier: GPL-3.0-only */
/* The melody a composer's manner plays, as notes in beats, for tools/tuna_corpus.py to measure as it measures a
 * score's. Jobs on stdin, one a line:
 *     STYLE MANNER SEED SECONDS  P0 P1 P2 P3  MAX_LEAP OCTAVE DENSITY SYNCOPATION DIRECTION RANGE
 * the ten settings replacing the manner's own (-9: its own). Per job on stdout:
 *     J STYLE MANNER MEL_MODE DIVISION  and the ten as played
 *     N ON OFF PITCH        (beats)
 *     E
 * With the argument "list": every composer's manners, as ID MANNER COMPOSER<tab>NAME. */
#include <stdio.h>
#include <string.h>
#include "tuna_gen.h"

#define SR 44100
#define DIV 4                     /* the generator at a quarter of the audio rate, as on the FM-1 */
#define BLOCK 32

static tg_gen g;
static long seen_tick;            /* the last tick counted */
static long beats;                /* beats begun up to it */
static double pos;                /* its place in beats */
static double on_at;
static int on_note;

static void advance(void)
{
    while (seen_tick < g.ticks - 1) {              /* (ticks - 1: the tick now sounding) */
        tg_meter_pos m;
        tg_meter_at(&g, ++seen_tick, &m);
        if (m.sub == 0)
            beats++;
        pos = (double)(beats - 1) + (double)m.sub / (double)m.glen;
    }
}

static void on_event(void *ctx, const tg_event *e)
{
    (void)ctx;
    if (e->type != TG_EV_NOTE || e->part != TG_PART_MELODY)
        return;
    advance();
    if (on_note >= 0 && (e->on || e->note == on_note)) {
        printf("N %.4f %.4f %d\n", on_at, pos > on_at ? pos : on_at + 0.01, on_note);
        on_note = -1;
    }
    if (e->on) {
        on_note = e->note;
        on_at = pos;
    }
}

int main(int argc, char **argv)
{
    int style, manner, seconds;
    if (argc > 1 && !strcmp(argv[1], "list")) {        /* ID MANNER COMPOSER<tab>MANNER NAME */
        for (style = 0; style < TG_STYLE_COUNT; ++style)
            for (manner = 0; manner < tg_manner_count(style); ++manner)
                printf("%d %d %s\t%s\n", style, manner, tg_style_get(style)->name, tg_manner_name(style, manner));
        return 0;
    }
    unsigned seed;
    float v[10];
    while (scanf("%d %d %u %d %f %f %f %f %f %f %f %f %f %f", &style, &manner, &seed, &seconds, &v[0], &v[1], &v[2],
                 &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9]) == 14) {
        if (!tg_style_get(style) || manner < 0 || manner >= tg_manner_count(style))
            return 1;
        tg_init(&g, seed, (float)SR / DIV);
        tg_apply_manner(&g, style, manner);
        tg_style *st = &g.style_buf;
        for (int i = 0; i < 4; ++i)
            if (v[i] > -8.0f) st->profile[i] = v[i];
        if (v[4] > -8.0f) st->max_leap = (int)v[4];
        if (v[5] > -8.0f) st->octave = v[5];
        if (v[6] > -8.0f) st->melody_density = v[6];
        if (v[7] > -8.0f) st->syncopation = v[7];
        if (v[8] > -8.0f) st->direction = v[8];
        if (v[9] > -8.0f) st->melody_range = (int)v[9];
        tg_apply_style_def(&g, st);
        g.style_id = style;
        g.manner = manner;
        tg_reseed(&g, seed);
        printf("J %d %d %d %d %g %g %g %g %d %g %g %g %g %d\n", style, manner, st->mel_mode, st->division,
               (double)st->profile[0], (double)st->profile[1], (double)st->profile[2], (double)st->profile[3],
               st->max_leap, (double)st->octave, (double)st->melody_density, (double)st->syncopation,
               (double)st->direction, st->melody_range);
        seen_tick = -1;
        beats = 0;
        pos = 0.0;
        on_note = -1;
        for (long now = 0; now < (long)seconds * SR; now += BLOCK)
            tg_process(&g, BLOCK / DIV, on_event, 0);
        printf("E\n");
    }
    return 0;
}
