/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA's instrument families on Felucca's sounds: the engine (engines.c's order), its factory preset, and what a
 * preset does not hold (the distortion send, glide, the voice mode). Shared by the firmware (tuna_fw.c) and the
 * renders (tests/tuna_felucca.c), so both play the same sounds. And a groove's kit as the DRUM engine's
 * parameters. Needs tuna_style.h (TI_*, tg_groove) before it. */
#ifndef TUNA_SOUNDS_H
#define TUNA_SOUNDS_H

enum { TSN_ANALOG = 0, TSN_PHASE = 2, TSN_LOFI = 3, TSN_SAMPLE = 4, TSN_VOICE = 5, TSN_TRIO = 6, TSN_WHEEL = 7,
       TSN_GRAIN = 8, TSN_PHYS = 9, TSN_FM6 = 12 };
enum { TSN_KEEP = -1 };                   /* dist, glide, voice: as the preset leaves them */
enum { TSN_POLY = 0, TSN_MONO = 1, TSN_LEGATO = 2 };   /* core.h's V_POLY V_MONO V_LEGATO */

typedef struct { uint8_t engine, preset; int8_t dist, glide, voice; } tsn_family;

#define TSN(e, p) {(uint8_t)(e), (uint8_t)(p), TSN_KEEP, TSN_KEEP, TSN_KEEP}
#define TSN_X(e, p, d, gl, v) {(uint8_t)(e), (uint8_t)(p), (int8_t)(d), (int8_t)(gl), (int8_t)(v)}
static const tsn_family TSN_FAMILY[TI_COUNT] = {
    [TI_DEFAULT] = TSN(TSN_ANALOG, 1),     [TI_PIANO] = TSN(TSN_SAMPLE, 0),     [TI_HARPSICHORD] = TSN(TSN_FM6, 7),
    [TI_ORGAN] = TSN(TSN_WHEEL, 0),        [TI_STRINGS] = TSN(TSN_ANALOG, 11),  [TI_VIOLIN] = TSN(TSN_PHASE, 2),
    [TI_CELLO] = TSN(TSN_PHASE, 2),        [TI_CONTRABASS] = TSN(TSN_ANALOG, 7), [TI_PIZZ] = TSN(TSN_PHYS, 2),
    [TI_FLUTE] = TSN(TSN_SAMPLE, 2),       [TI_OBOE] = TSN(TSN_ANALOG, 10),     [TI_CLARINET] = TSN(TSN_WHEEL, 3),
    [TI_BASSOON] = TSN(TSN_LOFI, 1),       [TI_HORN] = TSN(TSN_PHASE, 0),       [TI_TRUMPET] = TSN(TSN_ANALOG, 9),
    [TI_BRASS] = TSN(TSN_FM6, 3),          [TI_TUBA] = TSN(TSN_FM6, 2),         [TI_HARP] = TSN(TSN_PHYS, 8),
    [TI_GUITAR] = TSN(TSN_PHYS, 2),        [TI_CHOIR] = TSN(TSN_VOICE, 0),      [TI_VOICE] = TSN(TSN_VOICE, 1),
    [TI_BELLS] = TSN(TSN_FM6, 1),          [TI_CELESTA] = TSN(TSN_PHASE, 4),    [TI_MARIMBA] = TSN(TSN_FM6, 5),
    [TI_BANDONEON] = TSN(TSN_WHEEL, 2),    [TI_SAX] = TSN(TSN_SAMPLE, 3),       [TI_SYNTH] = TSN(TSN_ANALOG, 0),
    [TI_PAD] = TSN(TSN_ANALOG, 1),         [TI_WHISTLE] = TSN(TSN_WHEEL, 3),    [TI_DRONE] = TSN(TSN_PHYS, 7),
    [TI_EPIANO] = TSN(TSN_FM6, 0),
    /* the popular genres' */
    [TI_EGUITAR] = TSN_X(TSN_PHYS, 2, 14, TSN_KEEP, TSN_KEEP),       /* a plucked string, a little hair on it */
    [TI_DISTGUITAR] = TSN_X(TSN_PHYS, 2, 100, TSN_KEEP, TSN_KEEP),   /* the same through the distortion */
    [TI_EBASS] = TSN(TSN_FM6, 2),                                    /* FM BASS: a fingered electric bass */
    [TI_SYNTHBASS] = TSN(TSN_ANALOG, 2),                             /* SQR BASS */
    [TI_808] = TSN_X(TSN_ANALOG, 7, 18, 34, TSN_LEGATO),             /* SUB BASS, gliding, a little grit */
    [TI_ACID] = TSN_X(TSN_ANALOG, 4, TSN_KEEP, 18, TSN_LEGATO),      /* the 303 */
    [TI_SUPERSAW] = TSN(TSN_ANALOG, 6),                              /* RAVE */
    [TI_FATBASS] = TSN(TSN_TRIO, 0),                                 /* three oscillators: reese, wobble */
    [TI_SYNCLEAD] = TSN(TSN_TRIO, 2),
    [TI_CHIP] = TSN(TSN_LOFI, 0),                                    /* PULSE LD */
    [TI_CLAV] = TSN(TSN_FM6, 7),                                     /* FM PLUCK: the clavinet's bite */
    [TI_ROCKORGAN] = TSN(TSN_WHEEL, 4),                              /* ROCK DRIVE */
    [TI_GOSPEL] = TSN(TSN_WHEEL, 2),
    [TI_TALKBASS] = TSN_X(TSN_VOICE, 2, TSN_KEEP, 24, TSN_LEGATO),   /* WOW BASS: the talk box, G-funk's whine */
    [TI_GLITCH] = TSN(TSN_GRAIN, 1),
    [TI_SINEKEY] = TSN(TSN_ANALOG, 5),
    [TI_SHIMMER] = TSN(TSN_GRAIN, 3),
    [TI_KALIMBA] = TSN(TSN_PHYS, 4),
    [TI_LOGDRUM] = TSN_X(TSN_PHYS, 5, TSN_KEEP, 20, TSN_KEEP),       /* HAND DRUM: amapiano's log drum, bending */
    [TI_PLUCKSYNTH] = TSN(TSN_ANALOG, 8),
    [TI_SAWLEAD] = TSN_X(TSN_ANALOG, 0, TSN_KEEP, 10, TSN_KEEP),
    [TI_WAVEBASS] = TSN(TSN_LOFI, 1),
    [TI_STEELDRUM] = TSN(TSN_FM6, 5),
};
#undef TSN
#undef TSN_X

/* the DRUM engine's eight parameters (E0 KIT, E1 TUNE, E2 TONE, E3 DECY, E4 SNAP, E5 ACC, E6 KICK, E7 DRV) for a
 * groove's kit, or Felucca's own kit as designed (no groove) */
static inline void tsn_kit(const tg_groove *gr, int16_t e[8])
{
    e[0] = gr ? gr->kit : 0;
    e[1] = gr ? gr->tune : 64;
    e[2] = gr ? gr->tone : 64;
    e[3] = gr ? gr->decay : 64;
    e[4] = gr ? gr->snap : 64;
    e[5] = 100;
    e[6] = gr ? gr->kick : 0;
    e[7] = gr ? gr->drive : 0;
}

#endif
