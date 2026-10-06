/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA composer signatures (see tuna_style.h). */
#include "tuna_style.h"
#include "tuna_theory.h"

/* functional vocabulary; the order is the column order of the style weight tables */
static const tg_chord_def vocab_major[TG_VOCAB] = {
    {0, TQ_MAJ, TF_T, -1, "I"},
    {9, TQ_MIN, TF_T, -1, "vi"},
    {4, TQ_MIN, TF_T, -1, "iii"},
    {5, TQ_MAJ, TF_S, -1, "IV"},
    {2, TQ_MIN, TF_S, -1, "ii"},
    {2, TQ_MIN7, TF_S, -1, "ii7"},
    {7, TQ_MAJ, TF_D, -1, "V"},
    {7, TQ_DOM7, TF_D, -1, "V7"},
    {11, TQ_DIM, TF_D, -1, "vii°"},
    {2, TQ_DOM7, TF_SEC, 6, "V7/V"},
    {4, TQ_DOM7, TF_SEC, 1, "V7/vi"},
    {0, TQ_DOM7, TF_SEC, 3, "V7/IV"},
    {9, TQ_DOM7, TF_SEC, 4, "V7/ii"},
    {5, TQ_MIN, TF_S, -1, "iv"},
    {8, TQ_MAJ, TF_S, -1, "bVI"},
    {5, TQ_MAJ7, TF_S, -1, "IVmaj7"},
};

static const tg_chord_def vocab_minor[TG_VOCAB] = {
    {0, TQ_MIN, TF_T, -1, "i"},
    {8, TQ_MAJ, TF_T, -1, "VI"},
    {3, TQ_MAJ, TF_T, -1, "III"},
    {5, TQ_MIN, TF_S, -1, "iv"},
    {2, TQ_DIM, TF_S, -1, "ii°"},
    {2, TQ_HDIM7, TF_S, -1, "iiø7"},
    {7, TQ_MAJ, TF_D, -1, "V"},
    {7, TQ_DOM7, TF_D, -1, "V7"},
    {11, TQ_DIM, TF_D, -1, "vii°"},
    {2, TQ_DOM7, TF_SEC, 6, "V7/V"},
    {10, TQ_DOM7, TF_SEC, 2, "V7/III"},
    {0, TQ_DOM7, TF_SEC, 3, "V7/iv"},
    {10, TQ_MAJ, TF_SEC, 2, "VII"},      /* the subtonic leads to III, as in the falling-fifth chain */
    {1, TQ_MAJ, TF_S, -1, "bII"},        /* Neapolitan */
    {5, TQ_MIN7, TF_S, -1, "iv7"},
    {8, TQ_MAJ7, TF_T, -1, "VImaj7"},
};

const tg_chord_def *tg_vocab(bool minor) { return minor ? vocab_minor : vocab_major; }

static const tg_style styles[TG_STYLE_COUNT] = {
    [TG_STYLE_BACH] = {
        .name = "BACH",
        .root = 2, .scale = TS_AEOLIAN,                 /* D minor */
        .bpm = 76, .division = 4, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.85f, .melody_base = 62, .melody_range = 17,
        .profile = {0.04f, 0.72f, 0.90f, 0.97f},        /* running sixteenths: steps, some arpeggio */
        .appoggiatura = 0.0f, .suspension = 0.45f, .trill = false,
        .bass_mode = TG_BASS_WALKING, .pad_mode = TG_PAD_COUNTER, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 4.0f, .repeat = 0.25f, .motif = 0.35f,
        /*            I  vi iii IV ii ii7 V V7 vii V/V V/vi V/IV V/ii iv bVI IVM7 */
        .w_major = {10, 6, 3,  6, 5, 6,  6, 8, 4,  6,   4,   3,   4,   0, 0,  2},
        /*            i  VI III iv ii° iiø V V7 vii V/V V/III V/iv VII bII iv7 VIM7 */
        .w_minor = {10, 6, 5,  7, 3,  6,  6, 8, 5,  4,   3,    4,   5,  2,  4,  2},
    },
    [TG_STYLE_MOZART] = {
        .name = "MOZART",
        .root = 0, .scale = TS_IONIAN,                  /* C major */
        .bpm = 112, .division = 4, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 8,
        .melody_density = 0.30f, .melody_base = 64, .melody_range = 19,
        .profile = {0.10f, 0.62f, 0.88f, 0.97f},
        .appoggiatura = 0.35f, .suspension = 0.0f, .trill = true,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_ALBERTI, .pad_base = 48, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.5f, .motif = 0.5f,
        /*            I  vi iii IV ii ii7 V V7 vii V/V V/vi V/IV V/ii iv bVI IVM7 */
        .w_major = {10, 4, 0,  8, 6, 4,  8, 9, 2,  5,   2,   1,   1,   1, 0,  0},
        /*            i  VI III iv ii° iiø V V7 vii V/V V/III V/iv VII bII iv7 VIM7 */
        .w_minor = {10, 5, 4,  8, 2,  5,  8, 9, 3,  4,   1,    1,   1,  2,  2,  0},
    },
};

const tg_style *tg_style_get(int id) { return id >= 0 && id < TG_STYLE_COUNT ? &styles[id] : 0; }
