/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA music theory core (see tuna_theory.h). */
#include "tuna_theory.h"

/* ---- scales ------------------------------------------------------------------------------ */
static const struct { uint16_t mask; const char *name; } scales[TS_COUNT] = {
    [TS_IONIAN]     = {0xAB5, "IONIAN"},      /* 0 2 4 5 7 9 11  major */
    [TS_DORIAN]     = {0x6AD, "DORIAN"},      /* 0 2 3 5 7 9 10 */
    [TS_PHRYGIAN]   = {0x5AB, "PHRYGIAN"},    /* 0 1 3 5 7 8 10 */
    [TS_LYDIAN]     = {0xAD5, "LYDIAN"},      /* 0 2 4 6 7 9 11 */
    [TS_MIXOLYDIAN] = {0x6B5, "MIXOLYD"},     /* 0 2 4 5 7 9 10 */
    [TS_AEOLIAN]    = {0x5AD, "AEOLIAN"},     /* 0 2 3 5 7 8 10  natural minor */
    [TS_LOCRIAN]    = {0x56B, "LOCRIAN"},     /* 0 1 3 5 6 8 10 */
    [TS_HARM_MINOR] = {0x9AD, "HARM MIN"},    /* 0 2 3 5 7 8 11 */
    [TS_MEL_MINOR]  = {0xAAD, "MEL MIN"},     /* 0 2 3 5 7 9 11  ascending form */
    [TS_PENT_MAJOR] = {0x295, "PENT MAJ"},    /* 0 2 4 7 9 */
    [TS_PENT_MINOR] = {0x4A9, "PENT MIN"},    /* 0 3 5 7 10 */
    [TS_BLUES]      = {0x4E9, "BLUES"},       /* 0 3 5 6 7 10 */
    [TS_WHOLE_TONE] = {0x555, "WHOLE T"},     /* 0 2 4 6 8 10 */
    [TS_OCT_HW]     = {0x6DB, "OCT H-W"},     /* 0 1 3 4 6 7 9 10 */
    [TS_OCT_WH]     = {0xB6D, "OCT W-H"},     /* 0 2 3 5 6 8 9 11 */
    [TS_MESSIAEN3]  = {0xDDD, "MODE 3"},      /* 0 2 3 4 6 7 8 10 11 */
    [TS_ACOUSTIC]   = {0x6D5, "ACOUSTIC"},    /* 0 2 4 6 7 9 10 */
    [TS_HUNG_MINOR] = {0x9CD, "HUNGARIAN"},   /* 0 2 3 6 7 8 11 */
    [TS_DOUBLE_HARM]= {0x9B3, "BYZANTINE"},   /* 0 1 4 5 7 8 11 */
    [TS_PHRYG_DOM]  = {0x5B3, "PHRYG DOM"},   /* 0 1 4 5 7 8 10 */
};

static int clamp_note(int n) { return n < 0 ? 0 : n > 127 ? 127 : n; }

/* floor division and positive modulo, also for negative a */
static int fdiv(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }
static int pmod(int a, int b) { int m = a % b; return m < 0 ? m + b : m; }

uint16_t tt_scale_mask(int scale) { return (scale >= 0 && scale < TS_COUNT) ? scales[scale].mask : scales[0].mask; }

const char *tt_scale_name(int scale) { return (scale >= 0 && scale < TS_COUNT) ? scales[scale].name : "?"; }

int tt_scale_size(int scale)
{
    uint16_t m = tt_scale_mask(scale);
    int n = 0;
    for (int i = 0; i < 12; i++)
        n += (m >> i) & 1;
    return n;
}

/* semitone offset of scale index idx (0..size-1) */
static int scale_offset(uint16_t mask, int idx)
{
    for (int i = 0; i < 12; i++)
        if ((mask >> i) & 1) {
            if (idx == 0)
                return i;
            idx--;
        }
    return 0;
}

/* semitones above the tonic of any degree (octaves included) */
static int degree_semis(int scale, int degree)
{
    uint16_t m = tt_scale_mask(scale);
    int n = tt_scale_size(scale);
    return 12 * fdiv(degree, n) + scale_offset(m, pmod(degree, n));
}

int tt_degree_note(int scale, int base_note, int degree)
{
    return clamp_note(base_note + degree_semis(scale, degree));
}

int tt_quantize(int note, int scale, int root)
{
    uint16_t m = tt_scale_mask(scale);
    note = clamp_note(note);
    int rel = pmod(note - root, 12);
    for (int d = 0; d < 12; d++) {            /* d < 12: at the MIDI edges one direction may be cut off */
        if (note - d >= 0 && ((m >> pmod(rel - d, 12)) & 1))      /* down first: ties go down */
            return note - d;
        if (note + d <= 127 && ((m >> pmod(rel + d, 12)) & 1))
            return note + d;
    }
    return note;
}

/* ---- chords ------------------------------------------------------------------------------ */
static const struct { uint8_t n; int8_t t[TT_CHORD_MAX]; } qualities[TQ_COUNT] = {
    [TQ_DIATONIC] = {3, {0, 4, 7}},           /* placeholder, never looked up */
    [TQ_MAJ]   = {3, {0, 4, 7}},
    [TQ_MIN]   = {3, {0, 3, 7}},
    [TQ_DIM]   = {3, {0, 3, 6}},
    [TQ_AUG]   = {3, {0, 4, 8}},
    [TQ_SUS2]  = {3, {0, 2, 7}},
    [TQ_SUS4]  = {3, {0, 5, 7}},
    [TQ_MAJ7]  = {4, {0, 4, 7, 11}},
    [TQ_MIN7]  = {4, {0, 3, 7, 10}},
    [TQ_DOM7]  = {4, {0, 4, 7, 10}},
    [TQ_HDIM7] = {4, {0, 3, 6, 10}},
    [TQ_MAJ6]    = {4, {0, 4, 7, 9}},
    [TQ_MIN6]    = {4, {0, 3, 7, 9}},
    [TQ_ADD9]    = {4, {0, 4, 7, 14}},
    [TQ_MADD9]   = {4, {0, 3, 7, 14}},
    [TQ_MAJ9]    = {4, {0, 4, 11, 14}},
    [TQ_MIN9]    = {4, {0, 3, 10, 14}},
    [TQ_DOM9]    = {4, {0, 4, 10, 14}},
    [TQ_DOM13]   = {4, {0, 4, 10, 21}},
    [TQ_DOM7B9]  = {4, {0, 4, 10, 13}},
    [TQ_SUS7]    = {4, {0, 5, 7, 10}},
    [TQ_DIM7]    = {4, {0, 3, 6, 9}},
    [TQ_MINMAJ7] = {4, {0, 3, 7, 11}},
    [TQ_MAJ7S11] = {4, {0, 4, 11, 18}},
    [TQ_SIXNINE] = {4, {0, 4, 9, 14}},
    [TQ_MIN11]   = {4, {0, 3, 10, 17}},
    [TQ_AUG7]    = {4, {0, 4, 8, 10}},
    [TQ_DOM7B5]  = {4, {0, 4, 6, 10}},
    [TQ_QUARTAL] = {4, {0, 5, 10, 15}},
    [TQ_FIFTH]   = {2, {0, 7}},
    [TQ_ADD2]    = {4, {0, 2, 4, 7}},
};

void tt_chord_fixed(tt_chord *c, int root_semi, int quality)
{
    if (quality <= TQ_DIATONIC || quality >= TQ_COUNT)
        quality = TQ_MAJ;
    c->root = (int8_t)pmod(root_semi, 12);
    c->n = qualities[quality].n;
    for (int i = 0; i < TT_CHORD_MAX; i++)
        c->tone[i] = i < c->n ? qualities[quality].t[i] : 0;
}

void tt_chord_diatonic(tt_chord *c, int scale, int degree, int tones)
{
    if (tones < 1)
        tones = 1;
    if (tones > TT_CHORD_MAX)
        tones = TT_CHORD_MAX;
    int base = degree_semis(scale, degree);
    c->root = (int8_t)pmod(base, 12);
    c->n = (uint8_t)tones;
    for (int k = 0; k < TT_CHORD_MAX; k++)
        c->tone[k] = k < tones ? (int8_t)(degree_semis(scale, degree + 2 * k) - base) : 0;
}

int tt_chord_note(const tt_chord *c, int base_note, int k)
{
    int n = c->n ? c->n : 1;
    return clamp_note(base_note + c->root + c->tone[pmod(k, n)] + 12 * fdiv(k, n));
}

/* ---- progressions ------------------------------------------------------------------------ */
/* v = semitones above the tonic for a fixed quality, or the scale degree for TQ_DIATONIC */
typedef struct { int8_t v; uint8_t q; } step;

static const struct { uint8_t len; const char *name; step s[TT_PROG_MAX]; } progs[TP_COUNT] = {
    [TP_POP]        = {4, "POP",        {{0, TQ_MAJ}, {7, TQ_MAJ}, {9, TQ_MIN}, {5, TQ_MAJ}}},
    [TP_JAZZ]       = {3, "JAZZ",       {{2, TQ_MIN7}, {7, TQ_DOM7}, {0, TQ_MAJ7}}},
    [TP_EPIC]       = {4, "EPIC",       {{0, TQ_MIN}, {8, TQ_MAJ}, {3, TQ_MAJ}, {10, TQ_MAJ}}},
    [TP_ANDALUSIAN] = {4, "ANDALUS",    {{0, TQ_MIN}, {10, TQ_MAJ}, {8, TQ_MAJ}, {7, TQ_MAJ}}},
    [TP_FOLK]       = {4, "FOLK",       {{0, TQ_MAJ}, {5, TQ_MAJ}, {0, TQ_MAJ}, {7, TQ_MAJ}}},
    [TP_MODAL_VAMP] = {2, "VAMP",       {{0, TQ_DIATONIC}, {1, TQ_DIATONIC}}},
    [TP_MIXO_ROCK]  = {3, "MIXO ROCK",  {{0, TQ_MAJ}, {10, TQ_MAJ}, {5, TQ_MAJ}}},
};

int tt_prog_len(int prog) { return (prog >= 0 && prog < TP_COUNT) ? progs[prog].len : 1; }

const char *tt_prog_name(int prog) { return (prog >= 0 && prog < TP_COUNT) ? progs[prog].name : "?"; }

void tt_prog_chord(tt_chord *c, int prog, int step_i, int scale, int tones)
{
    if (prog < 0 || prog >= TP_COUNT)
        prog = 0;
    step s = progs[prog].s[pmod(step_i, progs[prog].len)];
    if (s.q == TQ_DIATONIC) {
        tt_chord_diatonic(c, scale, s.v, tones);
        return;
    }
    tt_chord_fixed(c, s.v, s.q);
    if (tones < c->n)                         /* triad requested: drop the 7th of fixed 7th chords */
        c->n = (uint8_t)tones;
}

/* ---- tunings ------------------------------------------------------------------------------- */
/* cents x10 from equal temperament, by pitch class above the tuning root (the root itself exact):
 * meantone        quarter-comma, fifths of 696.58 cents from E-flat to G-sharp
 * Werckmeister    III (1691)
 * Pythagorean     pure fifths from E-flat to G-sharp
 * just            5-limit: 16/15 9/8 6/5 5/4 4/3 45/32 3/2 8/5 5/3 16/9 15/8 */
static const int16_t tunings[TT_TUNE_USER][12] = {
    [TT_TUNE_EQUAL]       = {0},
    [TT_TUNE_MEANTONE]    = {0, -240, -68, 103, -137, 34, -205, -34, -274, -103, 68, -171},
    [TT_TUNE_WERCKMEISTER]= {0, -98, -78, -59, -98, -20, -117, -39, -78, -117, -39, -78},
    [TT_TUNE_PYTHAGOREAN] = {0, 137, 39, -59, 78, -20, 117, 20, 156, 59, -39, 98},
    [TT_TUNE_JUST]        = {0, 117, 39, 156, -137, -20, -98, 20, 137, -156, -39, -117},
};
static int16_t user_tuning[12];
static const char *const tuning_names[TT_TUNE_COUNT] = {"EQUAL", "MEANTONE", "WERCKM", "PYTHAG", "JUST", "USER"};

const char *tt_tuning_name(int t) { return t >= 0 && t < TT_TUNE_COUNT ? tuning_names[t] : "?"; }

int tt_tuning_cents10(int t, int pc_rel)
{
    pc_rel = pmod(pc_rel, 12);
    if (t == TT_TUNE_USER)
        return user_tuning[pc_rel];
    return t > 0 && t < TT_TUNE_USER ? tunings[t][pc_rel] : 0;
}

void tt_tuning_set_user(const int16_t *c)
{
    for (int i = 0; i < 12; i++)
        user_tuning[i] = c[i];
}

/* ---- held keys ---------------------------------------------------------------------------- */
uint16_t tt_held_set(const uint8_t *notes, int n, int *bass_pc)
{
    uint16_t set = 0;
    int low = 128;
    for (int i = 0; i < n; i++) {
        set |= (uint16_t)(1u << (notes[i] % 12));
        if (notes[i] < low)
            low = notes[i];
    }
    if (bass_pc)
        *bass_pc = low < 128 ? low % 12 : -1;
    return set;
}
