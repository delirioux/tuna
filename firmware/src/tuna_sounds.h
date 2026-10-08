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

/* own: a sound of Tuna's on the engine (Felucca's preset layout, core.h preset_t), in place of the factory
 * preset's values; 0: the factory preset as it is */
typedef struct { uint8_t engine, preset; int8_t dist, glide, voice; const preset_t *own; } tsn_family;

#define TSN(e, p) {(uint8_t)(e), (uint8_t)(p), TSN_KEEP, TSN_KEEP, TSN_KEEP, 0}
#define TSN_X(e, p, d, gl, v) {(uint8_t)(e), (uint8_t)(p), (int8_t)(d), (int8_t)(gl), (int8_t)(v), 0}
#define TSN_OWN(e, p, own) {(uint8_t)(e), (uint8_t)(p), TSN_KEEP, TSN_KEEP, TSN_KEEP, &(own)}
#define TSN_OWNX(e, p, own, d, gl, v) {(uint8_t)(e), (uint8_t)(p), (int8_t)(d), (int8_t)(gl), (int8_t)(v), &(own)}

/* ---- Tuna's own sounds (TSN_OWN): Felucca's engines set otherwise. e: the engine's E0..E7 (eng_*.c .edit),
 * env: ATK DEC SUS REL, fenv: ENV -> FILTER, mono, FX(dist, chorus, delay, reverb) ---- */
#include "tuna_sounds_own.inc"
static const tsn_family TSN_FAMILY[TI_COUNT] = {
    [TI_DEFAULT] = TSN(TSN_ANALOG, 1),     [TI_PIANO] = TSN(TSN_SAMPLE, 0),     [TI_HARPSICHORD] = TSN(TSN_FM6, 7),
    [TI_ORGAN] = TSN(TSN_WHEEL, 0),        [TI_STRINGS] = TSN(TSN_ANALOG, 11),  [TI_VIOLIN] = TSN(TSN_PHASE, 2),
    [TI_CELLO] = TSN_OWN(TSN_PHASE, 2, TSN_CELLO),        [TI_CONTRABASS] = TSN_OWN(TSN_PHASE, 2, TSN_CONTRABASS), [TI_PIZZ] = TSN_OWN(TSN_PHYS, 2, TSN_PIZZ),
    [TI_FLUTE] = TSN(TSN_SAMPLE, 2),       [TI_OBOE] = TSN(TSN_ANALOG, 10),     [TI_CLARINET] = TSN_OWN(TSN_WHEEL, 3, TSN_CLARINET),
    [TI_BASSOON] = TSN_OWN(TSN_ANALOG, 10, TSN_BASSOON),       [TI_HORN] = TSN(TSN_PHASE, 0),       [TI_TRUMPET] = TSN(TSN_ANALOG, 9),
    [TI_BRASS] = TSN(TSN_FM6, 3),          [TI_TUBA] = TSN_OWN(TSN_PHASE, 0, TSN_TUBA),         [TI_HARP] = TSN(TSN_PHYS, 8),
    [TI_GUITAR] = TSN(TSN_PHYS, 2),        [TI_CHOIR] = TSN(TSN_VOICE, 0),      [TI_VOICE] = TSN(TSN_VOICE, 1),
    [TI_BELLS] = TSN(TSN_FM6, 1),          [TI_CELESTA] = TSN(TSN_PHASE, 4),    [TI_MARIMBA] = TSN(TSN_FM6, 5),
    [TI_BANDONEON] = TSN_OWN(TSN_WHEEL, 2, TSN_BANDONEON),    [TI_SAX] = TSN(TSN_SAMPLE, 3),       [TI_SYNTH] = TSN(TSN_ANALOG, 0),
    [TI_PAD] = TSN(TSN_GRAIN, 0),         [TI_WHISTLE] = TSN_OWN(TSN_LOFI, 0, TSN_WHISTLE),    [TI_DRONE] = TSN(TSN_PHYS, 7),
    [TI_EPIANO] = TSN(TSN_FM6, 0),
    /* the popular genres' */
    [TI_EGUITAR] = TSN_OWNX(TSN_PHYS, 2, TSN_EGUITAR, 14, TSN_KEEP, TSN_KEEP),       /* a plucked string, a little hair on it */
    [TI_DISTGUITAR] = TSN_OWNX(TSN_PHYS, 2, TSN_EGUITAR, 100, TSN_KEEP, TSN_KEEP),   /* the same through the distortion */
    [TI_EBASS] = TSN(TSN_FM6, 2),                                    /* FM BASS: a fingered electric bass */
    [TI_SYNTHBASS] = TSN(TSN_ANALOG, 2),                             /* SQR BASS */
    [TI_808] = TSN_X(TSN_ANALOG, 7, 18, 34, TSN_LEGATO),             /* SUB BASS, gliding, a little grit */
    [TI_ACID] = TSN_X(TSN_ANALOG, 4, TSN_KEEP, 18, TSN_LEGATO),      /* the 303 */
    [TI_SUPERSAW] = TSN(TSN_ANALOG, 6),                              /* RAVE */
    [TI_FATBASS] = TSN(TSN_TRIO, 0),                                 /* three oscillators: reese, wobble */
    [TI_SYNCLEAD] = TSN(TSN_TRIO, 2),
    [TI_CHIP] = TSN(TSN_LOFI, 0),                                    /* PULSE LD */
    [TI_CLAV] = TSN_OWN(TSN_PHYS, 2, TSN_CLAV),                                     /* FM PLUCK: the clavinet's bite */
    [TI_ROCKORGAN] = TSN(TSN_WHEEL, 4),                              /* ROCK DRIVE */
    [TI_GOSPEL] = TSN(TSN_WHEEL, 2),
    [TI_TALKBASS] = TSN_X(TSN_VOICE, 2, TSN_KEEP, 24, TSN_LEGATO),   /* WOW BASS: the talk box, G-funk's whine */
    [TI_GLITCH] = TSN(TSN_GRAIN, 1),
    [TI_SINEKEY] = TSN(TSN_ANALOG, 5),
    [TI_SHIMMER] = TSN(TSN_GRAIN, 3),
    [TI_KALIMBA] = TSN(TSN_PHYS, 4),
    [TI_LOGDRUM] = TSN_X(TSN_PHYS, 5, TSN_KEEP, 20, TSN_KEEP),       /* HAND DRUM: amapiano's log drum, bending */
    [TI_PLUCKSYNTH] = TSN(TSN_ANALOG, 8),
    [TI_SAWLEAD] = TSN_OWNX(TSN_TRIO, 0, TSN_SAWLEAD, TSN_KEEP, 10, TSN_KEEP),
    [TI_WAVEBASS] = TSN(TSN_LOFI, 1),
    [TI_STEELDRUM] = TSN_OWN(TSN_PHYS, 1, TSN_STEELPAN),
    /* more colours */
    [TI_STRMACH] = TSN(TSN_ANALOG, 3),         [TI_JAZZORGAN] = TSN(TSN_WHEEL, 1),
    [TI_ACCORDION] = TSN_OWN(TSN_WHEEL, 2, TSN_ACCORDION),
    [TI_RESOSYNTH] = TSN(TSN_PHASE, 3),        [TI_METALLEAD] = TSN(TSN_PHASE, 5),
    [TI_WAVELEAD] = TSN(TSN_LOFI, 3),          [TI_STEPLEAD] = TSN(TSN_LOFI, 4),
    [TI_WHISPER] = TSN(TSN_VOICE, 3),          [TI_ARPLEAD] = TSN(TSN_TRIO, 1),
    [TI_RINGMOD] = TSN(TSN_TRIO, 3),           [TI_CHIPCHOIR] = TSN(TSN_TRIO, 4),
    [TI_FROZEN] = TSN(TSN_GRAIN, 2),           [TI_BELLTREE] = TSN(TSN_PHYS, 0),
    [TI_GLASS] = TSN(TSN_PHYS, 3),             [TI_REEDORGAN] = TSN(TSN_PHASE, 1),
    [TI_KOTO] = TSN_OWN(TSN_PHYS, 2, TSN_KOTO),     [TI_SITAR] = TSN_OWN(TSN_PHYS, 7, TSN_SITAR),
    [TI_OUD] = TSN_OWN(TSN_PHYS, 7, TSN_OUD),       [TI_BANJO] = TSN_OWN(TSN_PHYS, 2, TSN_BANJO),
    [TI_DULCIMER] = TSN_OWN(TSN_PHYS, 2, TSN_DULCIMER), [TI_VIBES] = TSN_OWN(TSN_PHYS, 1, TSN_VIBES),
    [TI_XYLOPHONE] = TSN_OWN(TSN_PHYS, 1, TSN_XYLO), [TI_GLOCKEN] = TSN_OWN(TSN_PHYS, 0, TSN_GLOCK),
    [TI_TUBULAR] = TSN_OWN(TSN_PHYS, 0, TSN_TUBULAR), [TI_MUSICBOX] = TSN_OWN(TSN_PHYS, 0, TSN_MUSICBOX),
    [TI_TABLA] = TSN_OWN(TSN_PHYS, 5, TSN_TABLA),
};
#undef TSN
#undef TSN_X
#undef TSN_OWN
#undef TSN_OWNX

/* each family's level, evened out: half-dB steps added to the part's level (LVL), so that a plucked guitar and
 * held strings come out alike at the same mix setting (from tests/tuna_audition.sh: the peak of a held C4 to
 * -12 dB, at most +9 dB) */
static const int8_t TSN_TRIM[TI_COUNT] = {
    [TI_PIANO] = -2, [TI_HARPSICHORD] = 9, [TI_ORGAN] = 5, [TI_STRINGS] = -5, [TI_VIOLIN] = -4, [TI_CELLO] = -3,
    [TI_CONTRABASS] = -6, [TI_PIZZ] = 18, [TI_FLUTE] = 0, [TI_OBOE] = -2, [TI_CLARINET] = 1, [TI_BASSOON] = -5,
    [TI_HORN] = -7, [TI_TRUMPET] = -2, [TI_BRASS] = 2, [TI_TUBA] = -4, [TI_HARP] = 16, [TI_GUITAR] = 18,
    [TI_CHOIR] = -4, [TI_VOICE] = -1, [TI_BELLS] = 11, [TI_CELESTA] = 6, [TI_MARIMBA] = 9, [TI_BANDONEON] = 3,
    [TI_SAX] = 0, [TI_SYNTH] = -3, [TI_PAD] = 3, [TI_WHISTLE] = 4, [TI_DRONE] = 5, [TI_EPIANO] = -4,
    [TI_EGUITAR] = 18, [TI_DISTGUITAR] = 8, [TI_EBASS] = 8, [TI_SYNTHBASS] = -2, [TI_808] = 8, [TI_ACID] = 1,
    [TI_SUPERSAW] = -8, [TI_FATBASS] = 8, [TI_SYNCLEAD] = 0, [TI_CHIP] = -1, [TI_CLAV] = 18, [TI_ROCKORGAN] = 0,
    [TI_GOSPEL] = -2, [TI_TALKBASS] = -7, [TI_GLITCH] = 9, [TI_SINEKEY] = -5, [TI_SHIMMER] = 5, [TI_KALIMBA] = 17,
    [TI_LOGDRUM] = 10, [TI_PLUCKSYNTH] = 1, [TI_SAWLEAD] = -1, [TI_WAVEBASS] = 7, [TI_STEELDRUM] = 12,
    [TI_STRMACH] = -7, [TI_JAZZORGAN] = 3, [TI_ACCORDION] = 2, [TI_RESOSYNTH] = 7, [TI_METALLEAD] = 0,
    [TI_WAVELEAD] = 11, [TI_STEPLEAD] = -3, [TI_WHISPER] = -2, [TI_ARPLEAD] = -2, [TI_RINGMOD] = 18,
    [TI_CHIPCHOIR] = 2, [TI_FROZEN] = 18, [TI_BELLTREE] = 15, [TI_GLASS] = 18, [TI_REEDORGAN] = -7, [TI_KOTO] = 18,
    [TI_SITAR] = -3, [TI_OUD] = 17, [TI_BANJO] = 18, [TI_DULCIMER] = 13, [TI_VIBES] = 8, [TI_XYLOPHONE] = 10,
    [TI_GLOCKEN] = 11, [TI_TUBULAR] = 18, [TI_MUSICBOX] = 7, [TI_TABLA] = 12,
};

/* the part's level (0..127) with its family's trim; 0 stays 0 (a part turned off) */
static inline int16_t tsn_level(int level, int fam)
{
    if (level <= 0)
        return 0;
    int v = level + (fam > TI_DEFAULT && fam < TI_COUNT ? TSN_TRIM[fam] : 0);
    return (int16_t)(v < 1 ? 1 : v > 127 ? 127 : v);
}

/* a family's own sound into a track's parameters, after its factory preset was loaded (as ui.c
 * apply_preset_to puts a preset in) */
static inline void tsn_apply_own(int16_t *p, const preset_t *pr)
{
    static const uint8_t FX_DEF[4] = {0, 24, 28, 36};
    for (int i = 0; i < 8; i++)
        p[P_E0 + i] = pr->e[i];
    p[P_ATK] = pr->env[0];
    p[P_DEC] = pr->env[1];
    p[P_SUS] = pr->env[2];
    p[P_REL] = pr->env[3];
    p[P_ED_FLT] = pr->fenv;
    p[P_VOICE] = pr->mono ? V_LEGATO : V_POLY;
    for (int i = 0; i < 4; i++)
        p[P_DIST + i] = (int16_t)(pr->fx[i] ? pr->fx[i] - 1 : FX_DEF[i]);
}

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
