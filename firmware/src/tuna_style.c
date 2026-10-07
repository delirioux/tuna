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
    {5, TQ_MIN, TF_S, -1, "iv"},          /* borrowed from minor */
    {8, TQ_MAJ, TF_S, -1, "bVI"},         /* borrowed from minor */
    {5, TQ_MAJ7, TF_S, -1, "IVmaj7"},
    {0, TQ_MAJ7, TF_T, -1, "Imaj7"},
    {9, TQ_MIN7, TF_T, -1, "vi7"},
    {10, TQ_MAJ, TF_S, -1, "bVII"},       /* mixolydian colour */
    {0, TQ_AUG, TF_T, -1, "I+"},          /* whole-tone colour */
    {10, TQ_AUG, TF_T, -1, "bVII+"},      /* whole-tone colour */
    {4, TQ_MIN7, TF_T, -1, "iii7"},
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
    {0, TQ_MIN7, TF_T, -1, "i7"},
    {3, TQ_MAJ7, TF_T, -1, "IIImaj7"},
    {3, TQ_DOM7, TF_SEC, 1, "V7/VI"},
    {0, TQ_AUG, TF_T, -1, "I+"},          /* whole-tone colour */
    {10, TQ_AUG, TF_T, -1, "bVII+"},      /* whole-tone colour */
    {0, TQ_MAJ, TF_T, -1, "I"},           /* the major tonic: a warm shift (or a Picardy third) */
};

const tg_chord_def *tg_vocab(bool minor) { return minor ? vocab_minor : vocab_major; }

/* Column order of the weight tables:
 *  major: I vi iii IV ii ii7 V V7 vii° V/V V/vi V/IV V/ii iv bVI IVM7 IM7 vi7 bVII I+ bVII+ iii7
 *  minor: i VI III iv ii° iiø V V7 vii° V/V V/III V/iv VII bII iv7 VIM7 i7 IIIM7 V/VI I+ bVII+ I */
static const tg_style styles[TG_STYLE_COUNT] = {
    [TG_STYLE_BACH] = {
        .name = "BACH", .root = 2, .scale = TS_AEOLIAN,                              /* D minor */
        .bpm = 76, .division = 4, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.85f, .melody_base = 62, .melody_range = 17,
        .profile = {0.04f, 0.72f, 0.90f, 0.97f}, .max_leap = 4,         /* running sixteenths */
        .suspension = 0.45f, .arrival_bars = 1,
        .bass_mode = TG_BASS_WALKING, .pad_mode = TG_PAD_COUNTER, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 4.0f, .repeat = 0.25f,
        .motif = 0.35f, .motif_beats = 2, .motif_use = 0.85f, .var_invert = 0.2f, .var_alter = 0.25f,
        .perc_mode = TG_PERC_OFF, .perc = {6, 0, 0, 0, 0, 0}, /* festive works: trumpets and drums */
        .w_major = {10, 6, 3, 6, 5, 6, 6, 8, 4, 6, 4, 3, 4, 0, 0, 2, 0, 2, 0, 0, 0, 2},
        .w_minor = {10, 6, 5, 7, 3, 6, 6, 8, 5, 4, 3, 4, 5, 2, 4, 2, 0, 0, 2, 0, 0, 0},
    },
    [TG_STYLE_MOZART] = {
        .name = "MOZART", .root = 0, .scale = TS_IONIAN,                             /* C major */
        .bpm = 112, .division = 4, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 8,
        .melody_density = 0.30f, .melody_base = 64, .melody_range = 19,
        .profile = {0.10f, 0.62f, 0.88f, 0.97f}, .max_leap = 4,
        .appoggiatura = 0.35f, .trill = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_ALBERTI, .pad_base = 48, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.5f,
        .motif = 0.5f, .motif_beats = 4, .motif_use = 0.65f, .var_invert = 0.1f, .var_alter = 0.3f,
        .perc_mode = TG_PERC_OFF, .perc = {5, 0, 0, 0, 0, 0}, /* the classical orchestra: timpani */
        .w_major = {10, 4, 0, 8, 6, 4, 8, 9, 2, 5, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
        .w_minor = {10, 5, 4, 8, 2, 5, 8, 9, 3, 4, 1, 1, 1, 2, 2, 0, 0, 0, 1, 0, 0, 0},
    },
    [TG_STYLE_BEETHOVEN] = {
        .name = "BEETHOVEN", .root = 0, .scale = TS_AEOLIAN,                         /* C minor */
        .bpm = 100, .division = 2, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 8,
        .melody_density = 0.55f, .melody_base = 60, .melody_range = 19,
        .profile = {0.18f, 0.55f, 0.80f, 0.93f}, .max_leap = 5,         /* repeated notes, then leaps */
        .syncopation = 0.1f, .suspension = 0.1f, .arrival_bars = 2,
        .bass_mode = TG_BASS_BAR, .pad_mode = TG_PAD_REPEAT, .pad_base = 48, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.3f,
        .motif = 0.6f, .motif_beats = 2, .motif_use = 0.85f, .var_invert = 0.3f, .var_alter = 0.35f,
        .sforzando = 0.12f, .swell = 0.8f, .rubato = 0.1f,
        .perc_mode = TG_PERC_PERIOD, .perc = {9, 0, 0, 0, 0, 0}, /* timpani as a voice of its own */
        .w_major = {10, 5, 2, 7, 5, 3, 8, 10, 4, 6, 3, 2, 2, 3, 3, 0, 0, 0, 0, 0, 0, 0},
        .w_minor = {10, 7, 4, 8, 3, 5, 8, 10, 6, 5, 3, 4, 2, 5, 2, 0, 0, 0, 2, 0, 0, 0},
    },
    [TG_STYLE_BRAHMS] = {
        .name = "BRAHMS", .root = 5, .scale = TS_IONIAN,                             /* F major, 3/4 */
        .bpm = 84, .division = 2, .beats_per_bar = 3, .chord_beats = 3, .period_bars = 8,
        .melody_density = 0.5f, .melody_base = 60, .melody_range = 17,
        .profile = {0.08f, 0.60f, 0.85f, 0.95f}, .max_leap = 5,
        .syncopation = 0.2f, .appoggiatura = 0.15f, .suspension = 0.2f, .arrival_bars = 2,
        .bass_mode = TG_BASS_BAR, .pad_mode = TG_PAD_DOUBLE, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.4f, .plagal = true,
        .motif = 0.45f, .motif_beats = 3, .motif_use = 0.6f, .var_invert = 0.15f, .var_alter = 0.3f,
        .hemiola = true, .swell = 0.4f, .rubato = 0.2f,
        .perc_mode = TG_PERC_OFF, .perc = {6, 0, 0, 0, 2, 0}, /* timpani; the Fourth has a triangle */
        .w_major = {10, 6, 3, 8, 6, 5, 6, 7, 2, 4, 3, 2, 2, 6, 4, 2, 0, 2, 0, 0, 0, 0},
        .w_minor = {10, 6, 4, 8, 3, 6, 6, 8, 3, 4, 2, 3, 2, 3, 4, 2, 0, 0, 2, 0, 0, 3},
    },
    [TG_STYLE_MAHLER] = {
        .name = "MAHLER", .root = 2, .scale = TS_IONIAN,                             /* D major */
        .bpm = 60, .division = 2, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 8,
        .melody_density = 0.4f, .melody_base = 62, .melody_range = 24,
        .profile = {0.06f, 0.45f, 0.70f, 0.85f}, .max_leap = 6,         /* long lines, wide leaps */
        .appoggiatura = 0.4f, .chromatic = true, .suspension = 0.25f, .arrival_bars = 3,
        .bass_mode = TG_BASS_MARCH, .pad_mode = TG_PAD_BLOCK, .pad_base = 52, .pad_tones = 4,
        .fifth_bias = 1.5f, .repeat = 0.2f,
        .motif = 0.4f, .motif_beats = 4, .motif_use = 0.5f, .var_invert = 0.2f, .var_alter = 0.4f,
        .sforzando = 0.05f, .swell = 0.6f, .rubato = 0.35f,
        .perc_mode = TG_PERC_PERIOD, .perc = {8, 7, 6, 5, 2, 0}, /* march snare, bass drum, cymbals */
        .w_major = {10, 7, 3, 7, 5, 5, 6, 7, 2, 5, 4, 3, 3, 7, 6, 3, 3, 3, 2, 0, 0, 0},
        .w_minor = {10, 7, 4, 7, 3, 5, 6, 8, 3, 4, 3, 3, 3, 3, 3, 3, 2, 2, 3, 0, 0, 5},
    },
    [TG_STYLE_CHOPIN] = {
        .name = "CHOPIN", .root = 3, .scale = TS_IONIAN,                             /* E-flat major */
        .bpm = 60, .division = 3, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.45f, .melody_base = 63, .melody_range = 19,
        .profile = {0.08f, 0.62f, 0.85f, 0.95f}, .max_leap = 5,
        .syncopation = 0.15f, .appoggiatura = 0.3f, .chromatic = true, .suspension = 0.1f,
        .trill = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_BROKEN, .pad_base = 46, .pad_tones = 3,
        .fifth_bias = 2.0f, .repeat = 0.4f,
        .motif = 0.4f, .motif_beats = 4, .motif_use = 0.5f, .var_invert = 0.1f, .var_alter = 0.4f,
        .swell = 0.4f, .rubato = 0.5f,
        .perc_mode = TG_PERC_OFF, .perc = {3, 0, 0, 0, 0, 0}, /* only in the concertos */
        .w_major = {10, 6, 3, 6, 6, 6, 6, 9, 3, 6, 4, 3, 4, 4, 3, 2, 2, 2, 0, 0, 0, 0},
        .w_minor = {10, 6, 5, 7, 3, 6, 6, 9, 4, 5, 3, 3, 3, 4, 3, 2, 0, 2, 3, 0, 0, 2},
    },
    [TG_STYLE_SATIE] = {
        .name = "SATIE", .root = 2, .scale = TS_IONIAN,                              /* D major, 3/4 */
        .bpm = 70, .division = 2, .beats_per_bar = 3, .chord_beats = 3, .period_bars = 8,
        .melody_density = 0.18f, .melody_base = 66, .melody_range = 14,
        .profile = {0.05f, 0.70f, 0.90f, 0.98f}, .max_leap = 4,         /* plain and slow */
        .arrival_bars = 2,
        .bass_mode = TG_BASS_BAR, .pad_mode = TG_PAD_AFTERBEAT, .pad_base = 54, .pad_tones = 4,
        .fifth_bias = 0.5f, .repeat = 0.6f,
        .motif = 0.7f, .motif_beats = 3, .motif_use = 0.6f, .var_invert = 0.1f, .var_alter = 0.2f,
        .rubato = 0.1f,
        .perc_mode = TG_PERC_OFF, .perc = {0, 0, 2, 2, 0, 0}, /* a soft bass drum and cymbal, as in Parade */
        .w_major = {4, 2, 0, 2, 2, 4, 3, 1, 0, 0, 0, 0, 0, 0, 0, 10, 10, 4, 0, 0, 0, 2},
        .w_minor = {6, 3, 2, 4, 1, 3, 3, 1, 0, 0, 0, 0, 0, 0, 4, 6, 8, 4, 0, 0, 0, 0},
    },
    [TG_STYLE_DEBUSSY] = {
        .name = "DEBUSSY", .root = 1, .scale = TS_IONIAN,                            /* D-flat, 9/8 feel */
        .bpm = 58, .division = 3, .beats_per_bar = 3, .chord_beats = 3, .period_bars = 8,
        .melody_density = 0.35f, .melody_base = 61, .melody_range = 19,
        .profile = {0.06f, 0.55f, 0.82f, 0.95f}, .max_leap = 4,
        .syncopation = 0.3f, .appoggiatura = 0.1f, .suspension = 0.1f, .arrival_bars = 2,
        .bass_mode = TG_BASS_PEDAL, .pad_mode = TG_PAD_PARALLEL, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 0.7f, .repeat = 0.3f, .plagal = true, .whole_tone = 0.4f,
        .motif = 0.5f, .motif_beats = 3, .motif_use = 0.5f, .var_invert = 0.2f, .var_alter = 0.3f,
        .swell = 0.3f, .rubato = 0.3f,
        .perc_mode = TG_PERC_OFF, .perc = {3, 0, 0, 4, 3, 0}, /* soft rolls, suspended and antique cymbals */
        .w_major = {6, 5, 4, 6, 5, 7, 2, 1, 0, 1, 0, 0, 0, 2, 3, 8, 8, 6, 6, 0, 0, 5},
        .w_minor = {6, 5, 5, 5, 2, 5, 2, 1, 0, 1, 1, 1, 4, 2, 5, 6, 6, 6, 0, 0, 0, 0},
    },
    [TG_STYLE_DVORAK] = {
        .name = "DVORAK", .root = 2, .scale = TS_IONIAN,                             /* D major */
        .bpm = 66, .division = 2, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 8,
        .melody_density = 0.4f, .melody_base = 62, .melody_range = 16,
        .profile = {0.06f, 0.60f, 0.85f, 0.96f}, .max_leap = 4,
        .melody_mask = 0x295,                                           /* major pentatonic */
        .syncopation = 0.35f, .appoggiatura = 0.1f, .suspension = 0.1f, .arrival_bars = 2,
        .bass_mode = TG_BASS_PEDAL, .pad_mode = TG_PAD_BLOCK, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 1.2f, .repeat = 0.5f,
        .motif = 0.6f, .motif_beats = 4, .motif_use = 0.7f, .var_invert = 0.1f, .var_alter = 0.3f,
        .sforzando = 0.03f, .swell = 0.4f, .rubato = 0.25f,
        .perc_mode = TG_PERC_PERIOD, .perc = {6, 0, 0, 3, 6, 4}, /* triangle and tambourine, as in the Slavonic Dances */
        .w_major = {10, 7, 3, 7, 5, 3, 6, 7, 1, 3, 3, 2, 2, 5, 3, 0, 0, 3, 0, 0, 0, 0},
        .w_minor = {10, 6, 5, 6, 2, 4, 6, 7, 2, 3, 2, 2, 3, 1, 3, 2, 2, 2, 2, 0, 0, 4},
    },
    [TG_STYLE_SAINT_SAENS] = {
        .name = "SAINT-SAENS", .root = 7, .scale = TS_AEOLIAN,                       /* G minor */
        .bpm = 96, .division = 4, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 8,
        .melody_density = 0.4f, .melody_base = 67, .melody_range = 19,
        .profile = {0.08f, 0.60f, 0.86f, 0.96f}, .max_leap = 5,
        .syncopation = 0.1f, .appoggiatura = 0.2f, .suspension = 0.1f, .trill = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_ARPEGGIO, .pad_base = 43, .pad_tones = 3,
        .fifth_bias = 2.5f, .repeat = 0.55f,
        .motif = 0.5f, .motif_beats = 4, .motif_use = 0.7f, .var_invert = 0.2f, .var_alter = 0.25f,
        .sforzando = 0.05f, .swell = 0.4f, .rubato = 0.1f,
        .perc_mode = TG_PERC_PERIOD, .perc = {7, 0, 3, 4, 3, 0}, /* timpani, cymbals, bass drum */
        .w_major = {10, 5, 3, 7, 6, 5, 7, 9, 4, 5, 3, 3, 3, 3, 4, 2, 0, 2, 0, 0, 0, 2},
        .w_minor = {10, 6, 5, 7, 3, 5, 7, 9, 5, 5, 4, 3, 4, 4, 2, 2, 0, 0, 3, 0, 0, 0},
    },
};

const tg_style *tg_style_get(int id) { return id >= 0 && id < TG_STYLE_COUNT ? &styles[id] : 0; }
