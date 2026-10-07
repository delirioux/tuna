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
    {11, TQ_DIM7, TF_D, -1, "vii°7"},
    {1, TQ_MAJ, TF_S, -1, "bII"},         /* Neapolitan */
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
    {11, TQ_DIM7, TF_D, -1, "vii°7"},
    {6, TQ_DIM7, TF_S, -1, "#iv°7"},
};

/* Further vocabularies, one per harmonic language. Every one keeps the tonic at 0 and the whole-tone
 * colour chords at TG_VOC_AUG_I and TG_VOC_AUG_BVII (19, 20). Applied chords (TF_SEC) name their target. */

/* romantic: chromatic mediants, diminished sevenths, the augmented sixth, the half-diminished
 * (Tristan) chord, the Neapolitan */
static const tg_chord_def rom_major[TG_VOCAB] = {
    {0, TQ_MAJ, TF_T, -1, "I"},           {9, TQ_MIN, TF_T, -1, "vi"},          {4, TQ_MIN, TF_T, -1, "iii"},
    {5, TQ_MAJ, TF_S, -1, "IV"},          {2, TQ_MIN, TF_S, -1, "ii"},          {2, TQ_MIN7, TF_S, -1, "ii7"},
    {7, TQ_MAJ, TF_D, -1, "V"},           {7, TQ_DOM7, TF_D, -1, "V7"},         {11, TQ_DIM7, TF_D, -1, "vii°7"},
    {2, TQ_DOM7, TF_SEC, 6, "V7/V"},      {4, TQ_DOM7, TF_SEC, 1, "V7/vi"},     {0, TQ_DOM7, TF_SEC, 3, "V7/IV"},
    {9, TQ_DOM7, TF_SEC, 4, "V7/ii"},     {5, TQ_MIN, TF_S, -1, "iv"},          {8, TQ_MAJ, TF_S, -1, "bVI"},
    {3, TQ_MAJ, TF_T, -1, "bIII"},        {4, TQ_MAJ, TF_T, -1, "III"},         {9, TQ_MAJ, TF_T, -1, "VI"},
    {8, TQ_DOM7, TF_SEC, 6, "Ger+6"},     {0, TQ_AUG, TF_T, -1, "I+"},          {10, TQ_AUG, TF_T, -1, "bVII+"},
    {2, TQ_HDIM7, TF_S, -1, "iiø7"},      {1, TQ_MAJ, TF_S, -1, "bII"},         {6, TQ_DIM7, TF_S, -1, "#iv°7"},
};
static const tg_chord_def rom_minor[TG_VOCAB] = {
    {0, TQ_MIN, TF_T, -1, "i"},           {8, TQ_MAJ, TF_T, -1, "VI"},          {3, TQ_MAJ, TF_T, -1, "III"},
    {5, TQ_MIN, TF_S, -1, "iv"},          {2, TQ_DIM, TF_S, -1, "ii°"},         {2, TQ_HDIM7, TF_S, -1, "iiø7"},
    {7, TQ_MAJ, TF_D, -1, "V"},           {7, TQ_DOM7, TF_D, -1, "V7"},         {11, TQ_DIM7, TF_D, -1, "vii°7"},
    {2, TQ_DOM7, TF_SEC, 6, "V7/V"},      {10, TQ_DOM7, TF_SEC, 2, "V7/III"},   {0, TQ_DOM7, TF_SEC, 3, "V7/iv"},
    {10, TQ_MAJ, TF_SEC, 2, "VII"},       {1, TQ_MAJ, TF_S, -1, "bII"},         {5, TQ_MIN7, TF_S, -1, "iv7"},
    {8, TQ_DOM7, TF_SEC, 6, "Ger+6"},     {0, TQ_MAJ, TF_T, -1, "I"},           {6, TQ_DIM7, TF_S, -1, "#iv°7"},
    {3, TQ_DOM7, TF_SEC, 1, "V7/VI"},     {0, TQ_AUG, TF_T, -1, "I+"},          {10, TQ_AUG, TF_T, -1, "bVII+"},
    {3, TQ_AUG, TF_T, -1, "III+"},        {5, TQ_MIN6, TF_S, -1, "iv6"},        {4, TQ_MAJ, TF_T, -1, "#III"},
};

/* jazz: sevenths as the norm, ninths and thirteenths, altered dominants, tritone substitutes,
 * the backdoor bVII9, sixth chords */
static const tg_chord_def jazz_major[TG_VOCAB] = {
    {0, TQ_MAJ7, TF_T, -1, "Imaj7"},      {9, TQ_MIN7, TF_T, -1, "vi7"},        {4, TQ_MIN7, TF_T, -1, "iii7"},
    {5, TQ_MAJ7, TF_S, -1, "IVmaj7"},     {2, TQ_MIN7, TF_S, -1, "ii7"},        {2, TQ_MIN9, TF_S, -1, "ii9"},
    {7, TQ_DOM9, TF_D, -1, "V9"},         {7, TQ_DOM13, TF_D, -1, "V13"},       {7, TQ_DOM7B9, TF_D, -1, "V7b9"},
    {2, TQ_DOM9, TF_SEC, 6, "II9"},       {4, TQ_DOM7B9, TF_SEC, 1, "III7b9"},  {0, TQ_DOM7, TF_SEC, 3, "I7"},
    {9, TQ_DOM7B9, TF_SEC, 4, "VI7b9"},   {5, TQ_MIN6, TF_S, -1, "iv6"},        {10, TQ_DOM9, TF_S, -1, "bVII9"},
    {1, TQ_DOM7, TF_D, -1, "bII7"},       {0, TQ_MAJ6, TF_T, -1, "I6"},         {0, TQ_SIXNINE, TF_T, -1, "I6/9"},
    {6, TQ_HDIM7, TF_S, -1, "#ivø7"},     {0, TQ_AUG, TF_T, -1, "I+"},          {10, TQ_AUG, TF_T, -1, "bVII+"},
    {3, TQ_MAJ7, TF_S, -1, "bIIImaj7"},   {7, TQ_SUS7, TF_D, -1, "V7sus"},      {8, TQ_MAJ7, TF_S, -1, "bVImaj7"},
};
static const tg_chord_def jazz_minor[TG_VOCAB] = {
    {0, TQ_MIN6, TF_T, -1, "i6"},         {8, TQ_MAJ7, TF_T, -1, "VImaj7"},     {3, TQ_MAJ7, TF_T, -1, "IIImaj7"},
    {5, TQ_MIN7, TF_S, -1, "iv7"},        {2, TQ_HDIM7, TF_S, -1, "iiø7"},      {5, TQ_MIN6, TF_S, -1, "iv6"},
    {7, TQ_DOM7B9, TF_D, -1, "V7b9"},     {7, TQ_SUS7, TF_D, -1, "V7sus"},      {11, TQ_DIM7, TF_D, -1, "vii°7"},
    {2, TQ_DOM7, TF_SEC, 6, "V7/V"},      {10, TQ_DOM7, TF_SEC, 2, "V7/III"},   {0, TQ_DOM7B9, TF_SEC, 3, "V7/iv"},
    {10, TQ_DOM9, TF_S, -1, "bVII9"},     {1, TQ_DOM7, TF_D, -1, "bII7"},       {5, TQ_MIN9, TF_S, -1, "iv9"},
    {0, TQ_MINMAJ7, TF_T, -1, "imaj7"},   {0, TQ_MIN9, TF_T, -1, "im9"},        {8, TQ_DOM7, TF_S, -1, "bVI7"},
    {3, TQ_DOM7, TF_SEC, 1, "V7/VI"},     {0, TQ_AUG, TF_T, -1, "I+"},          {10, TQ_AUG, TF_T, -1, "bVII+"},
    {0, TQ_MAJ6, TF_T, -1, "I6"},         {9, TQ_HDIM7, TF_S, -1, "viø7"},      {2, TQ_MIN7, TF_S, -1, "ii7"},
};

/* modal: chords on the mode's own degrees (TQ_DIATONIC: semis is the degree), as the mode makes them
 * (Dorian IV is major, Phrygian II is the flat second), with the cadential chords of modal music:
 * the raised V (musica ficta), the flat VII, the Picardy third, the open fifth */
static const tg_chord_def modal_both[TG_VOCAB] = {
    {0, TQ_DIATONIC, TF_T, -1, "i"},      {5, TQ_DIATONIC, TF_T, -1, "vi"},     {2, TQ_DIATONIC, TF_T, -1, "iii"},
    {3, TQ_DIATONIC, TF_S, -1, "iv"},     {1, TQ_DIATONIC, TF_S, -1, "ii"},     {4, TQ_DIATONIC, TF_D, -1, "v"},
    {6, TQ_DIATONIC, TF_D, -1, "vii"},    {7, TQ_MAJ, TF_D, -1, "V"},           {1, TQ_DIATONIC, TF_D, -1, "II"},
    {0, TQ_MAJ, TF_T, -1, "I"},           {0, TQ_FIFTH, TF_T, -1, "5"},         {0, TQ_SUS4, TF_T, -1, "isus4"},
    {5, TQ_MAJ, TF_S, -1, "IV"},          {10, TQ_MAJ, TF_D, -1, "bVII"},       {8, TQ_MAJ, TF_S, -1, "bVI"},
    {3, TQ_MAJ, TF_T, -1, "bIII"},        {0, TQ_SUS2, TF_T, -1, "isus2"},      {7, TQ_SUS4, TF_D, -1, "Vsus4"},
    {2, TQ_MAJ, TF_S, -1, "II"},          {0, TQ_AUG, TF_T, -1, "I+"},          {10, TQ_AUG, TF_T, -1, "bVII+"},
    {7, TQ_FIFTH, TF_D, -1, "V5"},        {5, TQ_FIFTH, TF_S, -1, "IV5"},       {3, TQ_DIATONIC, TF_T, -1, "iv as T"},
};

/* modern: quartal chords, the octatonic family (major triads a minor third apart, dominant and
 * diminished sevenths), tritone pairs, bare fifths */
static const tg_chord_def modern_both[TG_VOCAB] = {
    {0, TQ_MAJ, TF_T, -1, "I"},           {0, TQ_MIN, TF_T, -1, "i"},           {0, TQ_QUARTAL, TF_T, -1, "I4"},
    {3, TQ_MAJ, TF_T, -1, "bIII"},        {6, TQ_MAJ, TF_T, -1, "#IV"},         {9, TQ_MAJ, TF_T, -1, "VI"},
    {7, TQ_DOM7, TF_D, -1, "V7"},         {1, TQ_DOM7, TF_D, -1, "bII7"},       {11, TQ_DIM7, TF_D, -1, "vii°7"},
    {0, TQ_DIM7, TF_T, -1, "i°7"},        {5, TQ_QUARTAL, TF_S, -1, "IV4"},     {2, TQ_QUARTAL, TF_S, -1, "II4"},
    {8, TQ_MAJ, TF_S, -1, "bVI"},         {10, TQ_MAJ, TF_S, -1, "bVII"},       {0, TQ_FIFTH, TF_T, -1, "5"},
    {7, TQ_FIFTH, TF_D, -1, "V5"},        {1, TQ_MAJ, TF_S, -1, "bII"},         {5, TQ_MIN, TF_S, -1, "iv"},
    {7, TQ_SUS4, TF_D, -1, "Vsus4"},      {0, TQ_AUG, TF_T, -1, "I+"},          {10, TQ_AUG, TF_T, -1, "bVII+"},
    {0, TQ_DOM7B9, TF_T, -1, "I7b9"},     {3, TQ_DOM7, TF_T, -1, "bIII7"},      {6, TQ_DOM7, TF_D, -1, "bV7"},
};

/* pop and choral: added ninths, suspensions, the flat VII and VI, close clusters, 6/9 */
static const tg_chord_def pop_major[TG_VOCAB] = {
    {0, TQ_MAJ, TF_T, -1, "I"},           {0, TQ_ADD9, TF_T, -1, "Iadd9"},      {9, TQ_MIN, TF_T, -1, "vi"},
    {9, TQ_MIN7, TF_T, -1, "vi7"},        {5, TQ_MAJ, TF_S, -1, "IV"},          {5, TQ_ADD9, TF_S, -1, "IVadd9"},
    {5, TQ_MAJ7, TF_S, -1, "IVmaj7"},     {2, TQ_MIN7, TF_S, -1, "ii7"},        {7, TQ_MAJ, TF_D, -1, "V"},
    {7, TQ_SUS4, TF_D, -1, "Vsus4"},      {7, TQ_SUS7, TF_D, -1, "V7sus"},      {4, TQ_MIN, TF_T, -1, "iii"},
    {10, TQ_MAJ, TF_S, -1, "bVII"},       {8, TQ_MAJ, TF_S, -1, "bVI"},         {0, TQ_ADD2, TF_T, -1, "Iadd2"},
    {0, TQ_SIXNINE, TF_T, -1, "I6/9"},    {4, TQ_MAJ, TF_SEC, 2, "V/vi"},       {0, TQ_SUS2, TF_T, -1, "Isus2"},
    {5, TQ_SUS2, TF_S, -1, "IVsus2"},     {0, TQ_AUG, TF_T, -1, "I+"},          {10, TQ_AUG, TF_T, -1, "bVII+"},
    {2, TQ_MAJ, TF_SEC, 8, "V/V"},        {2, TQ_MIN, TF_S, -1, "ii"},          {3, TQ_MAJ, TF_S, -1, "bIII"},
};
static const tg_chord_def pop_minor[TG_VOCAB] = {
    {0, TQ_MIN, TF_T, -1, "i"},           {0, TQ_MADD9, TF_T, -1, "iadd9"},     {8, TQ_MAJ, TF_T, -1, "VI"},
    {8, TQ_MAJ7, TF_T, -1, "VImaj7"},     {5, TQ_MIN, TF_S, -1, "iv"},          {5, TQ_MIN7, TF_S, -1, "iv7"},
    {7, TQ_MAJ, TF_D, -1, "V"},           {7, TQ_MIN, TF_D, -1, "v"},           {10, TQ_MAJ, TF_D, -1, "VII"},
    {3, TQ_MAJ, TF_T, -1, "III"},         {2, TQ_HDIM7, TF_S, -1, "iiø7"},      {10, TQ_SUS2, TF_S, -1, "VIIsus2"},
    {7, TQ_SUS4, TF_D, -1, "Vsus4"},      {3, TQ_ADD9, TF_T, -1, "IIIadd9"},    {8, TQ_ADD9, TF_T, -1, "VIadd9"},
    {5, TQ_MAJ, TF_S, -1, "IV"},          {0, TQ_MIN7, TF_T, -1, "i7"},         {0, TQ_SUS2, TF_T, -1, "isus2"},
    {7, TQ_MIN7, TF_D, -1, "v7"},         {0, TQ_AUG, TF_T, -1, "I+"},          {10, TQ_AUG, TF_T, -1, "bVII+"},
    {0, TQ_MAJ, TF_T, -1, "I"},           {1, TQ_MAJ, TF_S, -1, "bII"},         {10, TQ_DOM7, TF_S, -1, "VII7"},
};

const tg_chord_def *tg_vocab(bool minor) { return minor ? vocab_minor : vocab_major; }

const tg_chord_def *tg_vocab_of(int vocab, bool minor)
{
    switch (vocab) {
    case TG_VOC_ROMANTIC: return minor ? rom_minor : rom_major;
    case TG_VOC_JAZZ:     return minor ? jazz_minor : jazz_major;
    case TG_VOC_MODAL:    return modal_both;
    case TG_VOC_MODERN:   return modern_both;
    case TG_VOC_POP:      return minor ? pop_minor : pop_major;
    default:              return minor ? vocab_minor : vocab_major;
    }
}

/* Column order of the weight tables (classic):
 *  major: I vi iii IV ii ii7 V V7 vii° V/V V/vi V/IV V/ii iv bVI IVM7 IM7 vi7 bVII I+ bVII+ iii7 vii°7 bII
 *  minor: i VI III iv ii° iiø V V7 vii° V/V V/III V/iv VII bII iv7 VIM7 i7 IIIM7 V/VI I+ bVII+ I vii°7 #iv°7
 * romantic:
 *  major: I vi iii IV ii ii7 V V7 vii°7 V/V V/vi V/IV V/ii iv bVI bIII III VI Ger+6 I+ bVII+ iiø7 bII #iv°7
 *  minor: i VI III iv ii° iiø V V7 vii°7 V/V V/III V/iv VII bII iv7 Ger+6 I #iv°7 V/VI I+ bVII+ III+ iv6 #III
 * jazz:
 *  major: IM7 vi7 iii7 IVM7 ii7 ii9 V9 V13 V7b9 II9 III7b9 I7 VI7b9 iv6 bVII9 bII7 I6 I6/9 #ivø7 I+ bVII+
 *         bIIIM7 V7sus bVIM7
 *  minor: i6 VIM7 IIIM7 iv7 iiø7 iv6 V7b9 V7sus vii°7 V/V V/III V/iv bVII9 bII7 iv9 iM7 im9 bVI7 V/VI I+
 *         bVII+ I6 viø7 ii7
 * modal (both): i vi iii iv ii v vii V II(cad) I 5 isus4 IV bVII bVI bIII isus2 Vsus4 II I+ bVII+ V5 IV5 iv(T)
 * modern (both): I i I4 bIII #IV VI V7 bII7 vii°7 i°7 IV4 II4 bVI bVII 5 V5 bII iv Vsus4 I+ bVII+ I7b9
 *         bIII7 bV7
 * pop:
 *  major: I Iadd9 vi vi7 IV IVadd9 IVM7 ii7 V Vsus4 V7sus iii bVII bVI Iadd2 I6/9 V/vi Isus2 IVsus2 I+ bVII+
 *         V/V ii bIII
 *  minor: i iadd9 VI VIM7 iv iv7 V v VII III iiø7 VIIsus2 Vsus4 IIIadd9 VIadd9 IV i7 isus2 v7 I+ bVII+ I
 *         bII VII7 */
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
        .tuning = TT_TUNE_WERCKMEISTER,
        .orch = {{TI_HARPSICHORD, TI_HARPSICHORD, TI_HARPSICHORD}, {TI_ORGAN, TI_ORGAN, TI_ORGAN},
                 {TI_CELLO, TI_VIOLIN, TI_VIOLIN}, {TI_CELLO, TI_FLUTE, TI_HARPSICHORD}}, .orch_n = 4,
        .orch_mode = TG_ORCH_RANDOM,
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
        .orch = {{TI_PIANO, TI_PIANO, TI_PIANO}, {TI_CELLO, TI_VIOLIN, TI_STRINGS}, {TI_CELLO, TI_FLUTE, TI_STRINGS},
                 {TI_CONTRABASS, TI_OBOE, TI_STRINGS}}, .orch_n = 4, .orch_mode = TG_ORCH_RANDOM,
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
        .orch = {{TI_PIANO, TI_PIANO, TI_PIANO}, {TI_CELLO, TI_VIOLIN, TI_STRINGS}, {TI_CONTRABASS, TI_HORN, TI_STRINGS},
                 {TI_CELLO, TI_CLARINET, TI_STRINGS}}, .orch_n = 4, .orch_mode = TG_ORCH_RANDOM,
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
        .orch = {{TI_CELLO, TI_VIOLIN, TI_STRINGS}, {TI_CELLO, TI_CLARINET, TI_STRINGS}, {TI_CELLO, TI_HORN, TI_STRINGS},
                 {TI_PIANO, TI_PIANO, TI_PIANO}}, .orch_n = 4, .orch_mode = TG_ORCH_RANDOM,
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
        .orch = {{TI_CONTRABASS, TI_HORN, TI_STRINGS}, {TI_TUBA, TI_TRUMPET, TI_BRASS}, {TI_CONTRABASS, TI_OBOE, TI_STRINGS},
                 {TI_CONTRABASS, TI_VIOLIN, TI_HARP}}, .orch_n = 4, .orch_mode = TG_ORCH_RANDOM,
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
        .orch = {{TI_PIANO, TI_PIANO, TI_PIANO}}, .orch_n = 1,
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
        .orch = {{TI_PIANO, TI_PIANO, TI_PIANO}}, .orch_n = 1,
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
        .orch = {{TI_CELLO, TI_FLUTE, TI_HARP}, {TI_CONTRABASS, TI_OBOE, TI_STRINGS}, {TI_PIANO, TI_PIANO, TI_PIANO}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
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
        .orch = {{TI_CONTRABASS, TI_OBOE, TI_STRINGS}, {TI_CONTRABASS, TI_VIOLIN, TI_STRINGS},
                 {TI_CONTRABASS, TI_HORN, TI_STRINGS}, {TI_CELLO, TI_FLUTE, TI_STRINGS}}, .orch_n = 4,
        .orch_mode = TG_ORCH_RANDOM,
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
        .orch = {{TI_PIANO, TI_VIOLIN, TI_PIANO}, {TI_ORGAN, TI_ORGAN, TI_ORGAN}, {TI_CONTRABASS, TI_CELLO, TI_HARP},
                 {TI_CONTRABASS, TI_FLUTE, TI_STRINGS}}, .orch_n = 4, .orch_mode = TG_ORCH_RANDOM,
    },

    /* ---- 1: medieval and renaissance ------------------------------------------------------------------
     * modal vocabulary: i vi iii iv ii v vii V II(cad) I 5 isus4 IV bVII bVI bIII isus2 Vsus4 II I+ bVII+ V5 IV5 iv */
    [TG_STYLE_HILDEGARD] = {
        /* a single soaring line over a drone; no meter, Pythagorean fifths */
        .name = "HILDEGARD", .root = 2, .scale = TS_DORIAN,                          /* D Dorian */
        .bpm = 66, .division = 2, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 4,
        .melody_density = 0.55f, .melody_base = 57, .melody_range = 22,
        .profile = {0.08f, 0.62f, 0.80f, 0.90f}, .max_leap = 4,                        /* a fifth up, steps down */
        .arrival_bars = 1,
        .bass_mode = TG_BASS_DRONE, .pad_mode = TG_PAD_BLOCK, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.3f,
        .motif = 0.3f, .motif_beats = 4, .motif_use = 0.4f, .var_invert = 0.2f, .var_alter = 0.4f,
        .swell = 0.3f, .rubato = 0.2f,
        .perc_mode = TG_PERC_OFF,
        .vocab = TG_VOC_MODAL, .harmony = TG_HARM_DRONE, .cycle = {10}, .cycle_len = 1,
        .mel_mode = TG_MEL_CHANT, .tuning = TT_TUNE_PYTHAGOREAN,
        .orch = {{TI_DRONE, TI_VOICE, TI_DRONE}}, .orch_n = 1,
    },
    [TG_STYLE_TALLIS] = {
        /* Tudor polyphony: Dorian, false relations between the modal and the raised third, Amen endings */
        .name = "TALLIS", .root = 7, .scale = TS_DORIAN,                             /* G Dorian */
        .bpm = 58, .division = 2, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.35f, .melody_base = 62, .melody_range = 14,
        .profile = {0.05f, 0.76f, 0.92f, 0.98f}, .max_leap = 4,
        .suspension = 0.5f, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_COUNTER, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.3f, .plagal = true,
        .motif = 0.5f, .motif_beats = 4, .motif_use = 0.6f, .var_invert = 0.2f, .var_alter = 0.3f,
        .swell = 0.4f, .rubato = 0.15f,
        .perc_mode = TG_PERC_OFF,
        .w_minor = {10, 0, 6, 7, 3, 5, 6, 8, 0, 4, 0, 2, 0, 3, 3, 3, 0, 3, 0, 0, 0, 0, 0, 0},
        .w_major = {10, 6, 3, 7, 5, 6, 0, 0, 0, 0, 0, 2, 0, 3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0},
        .vocab = TG_VOC_MODAL, .tuning = TT_TUNE_MEANTONE,
        .orch = {{TI_CHOIR, TI_CHOIR, TI_CHOIR}}, .orch_n = 1,
    },
    [TG_STYLE_PALESTRINA] = {
        /* the model of smooth counterpoint: steps, leaps recovered, prepared suspensions, Ionian clarity */
        .name = "PALESTRINA", .root = 5, .scale = TS_IONIAN,                         /* F */
        .bpm = 64, .division = 2, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.4f, .melody_base = 60, .melody_range = 12,
        .profile = {0.03f, 0.84f, 0.95f, 0.99f}, .max_leap = 4,
        .suspension = 0.55f, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_COUNTER, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 2.0f, .repeat = 0.35f,
        .motif = 0.5f, .motif_beats = 4, .motif_use = 0.7f, .var_invert = 0.1f, .var_alter = 0.2f,
        .swell = 0.3f, .rubato = 0.1f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 6, 3, 7, 6, 8, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 4, 2, 0, 0, 0, 0, 0},
        .w_minor = {10, 5, 5, 6, 0, 4, 5, 8, 0, 3, 0, 2, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0},
        .vocab = TG_VOC_MODAL, .tuning = TT_TUNE_MEANTONE,
        .orch = {{TI_CHOIR, TI_CHOIR, TI_CHOIR}}, .orch_n = 1,
    },
    [TG_STYLE_ALLEGRI] = {
        /* Miserere: a harmonised verse with a high ornamented soprano, then a chanted verse on one chord */
        .name = "ALLEGRI", .root = 7, .scale = TS_AEOLIAN,                           /* G minor */
        .bpm = 54, .division = 3, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 4,
        .melody_density = 0.3f, .melody_base = 67, .melody_range = 17,                 /* up to the high C */
        .profile = {0.06f, 0.70f, 0.88f, 0.96f}, .max_leap = 5,
        .suspension = 0.3f, .arrival_bars = 1,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_BLOCK, .pad_base = 55, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.5f,
        .motif = 0.6f, .motif_beats = 4, .motif_use = 0.6f, .var_invert = 0.0f, .var_alter = 0.3f,
        .swell = 0.3f, .rubato = 0.3f,
        .perc_mode = TG_PERC_OFF,
        .vocab = TG_VOC_MODAL, .harmony = TG_HARM_CYCLE, .cycle = {0, 3, 15, 7}, .cycle_len = 4,
        .turn = 0.35f, .tuning = TT_TUNE_MEANTONE,
        .alt = {.density = 1.4f, .mel_mode = TG_MEL_CHANT + 1, .harmony = TG_HARM_DRONE + 1, .bass_mode = TG_BASS_DRONE + 1},
        .alt_every = 1,
        .orch = {{TI_CHOIR, TI_VOICE, TI_CHOIR}, {TI_CHOIR, TI_CHOIR, TI_CHOIR}}, .orch_n = 2, .orch_mode = TG_ORCH_CONTRAST,
    },
    [TG_STYLE_MONTEVERDI] = {
        /* seconda prattica: an expressive voice over basso continuo, repeated-note concitato in the ritornelli */
        .name = "MONTEVERDI", .root = 2, .scale = TS_AEOLIAN,                        /* D minor */
        .bpm = 72, .division = 4, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 4,
        .melody_density = 0.35f, .melody_base = 62, .melody_range = 15,
        .profile = {0.25f, 0.70f, 0.88f, 0.96f}, .max_leap = 5,                        /* repeated notes, leaps of grief */
        .appoggiatura = 0.3f, .suspension = 0.3f, .arrival_bars = 1,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_BLOCK, .pad_base = 52, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.3f,
        .motif = 0.4f, .motif_beats = 4, .motif_use = 0.5f, .var_invert = 0.1f, .var_alter = 0.4f,
        .swell = 0.4f, .rubato = 0.3f,
        .perc_mode = TG_PERC_OFF,
        .w_minor = {10, 5, 6, 6, 0, 3, 6, 9, 0, 3, 0, 2, 2, 0, 0, 0, 0, 4, 2, 0, 0, 0, 0, 0},
        .w_major = {10, 6, 3, 7, 5, 8, 0, 0, 0, 0, 0, 2, 0, 3, 0, 0, 0, 4, 3, 0, 0, 0, 0, 0},
        .vocab = TG_VOC_MODAL, .tuning = TT_TUNE_MEANTONE,
        .alt = {.bpm = 1.25f, .density = 1.8f, .pad_mode = TG_PAD_REPEAT + 1}, .alt_every = 2,
        .orch = {{TI_CELLO, TI_VOICE, TI_HARPSICHORD}, {TI_CELLO, TI_TRUMPET, TI_STRINGS}}, .orch_n = 2,
        .orch_mode = TG_ORCH_CONTRAST,
    },
    [TG_STYLE_SWEELINCK] = {
        /* the Amsterdam organist: running figuration over modal counterpoint, echoes, a Picardy close */
        .name = "SWEELINCK", .root = 9, .scale = TS_AEOLIAN,                         /* A Aeolian */
        .bpm = 76, .division = 4, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.75f, .melody_base = 64, .melody_range = 17,
        .profile = {0.02f, 0.85f, 0.95f, 0.99f}, .max_leap = 4,
        .suspension = 0.25f, .arrival_bars = 1,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_COUNTER, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 2.0f, .repeat = 0.3f,
        .motif = 0.45f, .motif_beats = 2, .motif_use = 0.8f, .var_invert = 0.3f, .var_alter = 0.2f,
        .swell = 0.2f, .rubato = 0.05f,
        .perc_mode = TG_PERC_OFF,
        .w_minor = {10, 6, 6, 6, 0, 4, 6, 8, 0, 4, 0, 2, 2, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0},
        .w_major = {10, 6, 3, 7, 5, 8, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 3, 2, 0, 0, 0, 0, 0},
        .vocab = TG_VOC_MODAL, .echo = 0.35f, .tuning = TT_TUNE_MEANTONE,
        .orch = {{TI_ORGAN, TI_ORGAN, TI_ORGAN}, {TI_HARPSICHORD, TI_HARPSICHORD, TI_HARPSICHORD}}, .orch_n = 2,
        .orch_mode = TG_ORCH_RANDOM,
    },

    /* ---- 2: baroque ------------------------------------------------------------------------------------
     * classic vocabulary:
     *  major: I vi iii IV ii ii7 V V7 vii° V/V V/vi V/IV V/ii iv bVI IVM7 IM7 vi7 bVII I+ bVII+ iii7 vii°7 bII
     *  minor: i VI III iv ii° iiø V V7 vii° V/V V/III V/iv VII bII iv7 VIM7 i7 IIIM7 V/VI I+ bVII+ I vii°7 #iv°7 */
    [TG_STYLE_BUXTEHUDE] = {
        /* stylus phantasticus: fugal periods and free toccata periods over a pedal, dramatic pauses */
        .name = "BUXTEHUDE", .root = 4, .scale = TS_AEOLIAN,                         /* E minor */
        .bpm = 84, .division = 4, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.55f, .melody_base = 64, .melody_range = 19,
        .profile = {0.03f, 0.75f, 0.90f, 0.97f}, .max_leap = 5,
        .suspension = 0.3f, .trill = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_WALKING, .pad_mode = TG_PAD_COUNTER, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 3.0f, .repeat = 0.25f,
        .motif = 0.4f, .motif_beats = 2, .motif_use = 0.8f, .var_invert = 0.3f, .var_alter = 0.25f,
        .swell = 0.3f, .rubato = 0.15f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 6, 3, 6, 5, 6, 6, 8, 4, 6, 4, 3, 4, 1, 0, 2, 0, 2, 0, 0, 0, 2, 3, 0},
        .w_minor = {10, 6, 5, 7, 3, 6, 6, 8, 5, 4, 3, 4, 5, 3, 3, 1, 0, 0, 2, 0, 0, 0, 4, 2},
        .gp = 0.15f, .tuning = TT_TUNE_WERCKMEISTER,
        .alt = {.bpm = 1.15f, .density = 1.5f, .bass_mode = TG_BASS_PEDAL + 1, .pad_mode = TG_PAD_BLOCK + 1},
        .alt_every = 1,
        .orch = {{TI_ORGAN, TI_ORGAN, TI_ORGAN}}, .orch_n = 1,
    },
    [TG_STYLE_PACHELBEL] = {
        /* the Canon: eight chords over a ground, violins in canon two bars apart, the figuration growing */
        .name = "PACHELBEL", .root = 2, .scale = TS_IONIAN,                          /* D major */
        .bpm = 58, .division = 4, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.5f, .melody_base = 66, .melody_range = 14,
        .profile = {0.05f, 0.80f, 0.93f, 0.98f}, .max_leap = 4,
        .suspension = 0.2f, .arrival_bars = 1,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_CANON, .pad_base = 54, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.0f,
        .motif = 0.6f, .motif_beats = 4, .motif_use = 0.7f, .var_invert = 0.1f, .var_alter = 0.3f,
        .swell = 0.2f,
        .perc_mode = TG_PERC_OFF,
        .harmony = TG_HARM_CYCLE, .cycle = {0, 6, 1, 2, 3, 0, 3, 6}, .cycle_len = 8,    /* I V vi iii IV I IV V */
        .canon_ticks = 32, .canon_steps = 0,
        .evolve_periods = 8, .evolve_depth = 0.8f,
        .tuning = TT_TUNE_WERCKMEISTER,
        .orch = {{TI_CELLO, TI_VIOLIN, TI_VIOLIN}}, .orch_n = 1,
    },
    [TG_STYLE_PURCELL] = {
        /* Dido's lament: the chromatic ground falling from tonic to dominant, sighs over it, in 3/2 */
        .name = "PURCELL", .root = 7, .scale = TS_AEOLIAN,                           /* G minor */
        .bpm = 60, .division = 2, .beats_per_bar = 3, .chord_beats = 3, .period_bars = 6,
        .melody_density = 0.35f, .melody_base = 62, .melody_range = 15,
        .profile = {0.08f, 0.75f, 0.90f, 0.97f}, .max_leap = 5,
        .appoggiatura = 0.4f, .chromatic = true, .suspension = 0.4f, .arrival_bars = 1,
        .bass_mode = TG_BASS_LINE, .pad_mode = TG_PAD_BLOCK, .pad_base = 55, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.0f,
        .motif = 0.5f, .motif_beats = 3, .motif_use = 0.6f, .var_invert = 0.1f, .var_alter = 0.3f,
        .swell = 0.4f, .rubato = 0.2f,
        .perc_mode = TG_PERC_OFF,
        .vocab = TG_VOC_MODAL, .harmony = TG_HARM_CYCLE, .cycle = {0, 7, 13, 12, 3, 7}, .cycle_len = 6, /* i V bVII IV iv V */
        .bass_notes = {0, -1, -2, -3, -4, -5}, .bass_len = 6,                              /* G F# F E Eb D */
        .direction = -0.4f, .tuning = TT_TUNE_MEANTONE,
        .orch = {{TI_CELLO, TI_VOICE, TI_STRINGS}, {TI_CELLO, TI_VIOLIN, TI_HARPSICHORD}}, .orch_n = 2,
        .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_VIVALDI] = {
        /* motor rhythm: a drum bass, sequences round the circle of fifths, tutti and solo in turn, terraces */
        .name = "VIVALDI", .root = 9, .scale = TS_AEOLIAN,                           /* A minor */
        .bpm = 112, .division = 4, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.7f, .melody_base = 64, .melody_range = 19,
        .profile = {0.10f, 0.60f, 0.82f, 0.94f}, .max_leap = 5,
        .trill = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_REPEAT, .pad_mode = TG_PAD_REPEAT, .pad_base = 52, .pad_tones = 3,
        .fifth_bias = 4.0f, .repeat = 0.3f,
        .motif = 0.6f, .motif_beats = 2, .motif_use = 0.9f, .var_invert = 0.1f, .var_alter = 0.2f,
        .swell = 0.2f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 6, 4, 7, 5, 5, 6, 8, 4, 6, 5, 4, 5, 1, 0, 2, 0, 2, 0, 0, 0, 3, 2, 0},
        .w_minor = {10, 6, 6, 7, 3, 5, 6, 8, 4, 5, 5, 4, 6, 1, 3, 2, 0, 2, 2, 0, 0, 0, 2, 0},
        .echo = 0.3f, .tuning = TT_TUNE_WERCKMEISTER,
        .alt = {.density = 1.2f, .bass_mode = TG_BASS_ROOT + 1, .pad_mode = TG_PAD_ARPEGGIO + 1}, .alt_every = 1,
        .orch = {{TI_CELLO, TI_VIOLIN, TI_STRINGS}, {TI_CELLO, TI_VIOLIN, TI_HARPSICHORD}}, .orch_n = 2,
        .orch_mode = TG_ORCH_CONTRAST,
    },
    [TG_STYLE_ALBINONI] = {
        /* the Adagio: slow pizzicato walking bass, organ chords, a long sighing violin line */
        .name = "ALBINONI", .root = 7, .scale = TS_AEOLIAN,                          /* G minor */
        .bpm = 46, .division = 2, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.3f, .melody_base = 67, .melody_range = 17,
        .profile = {0.05f, 0.70f, 0.88f, 0.95f}, .max_leap = 5,
        .appoggiatura = 0.35f, .suspension = 0.4f, .trill = true, .arrival_bars = 2,
        .bass_mode = TG_BASS_WALKING, .pad_mode = TG_PAD_BLOCK, .pad_base = 53, .pad_tones = 3,
        .fifth_bias = 2.0f, .repeat = 0.35f,
        .motif = 0.5f, .motif_beats = 4, .motif_use = 0.6f, .var_invert = 0.1f, .var_alter = 0.3f,
        .swell = 0.5f, .rubato = 0.25f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 6, 3, 7, 5, 6, 6, 8, 3, 5, 4, 3, 3, 2, 2, 3, 2, 3, 0, 0, 0, 2, 2, 0},
        .w_minor = {10, 7, 4, 8, 3, 6, 6, 9, 4, 4, 3, 4, 3, 4, 4, 3, 0, 0, 2, 0, 0, 0, 2, 0},
        .orch = {{TI_PIZZ, TI_VIOLIN, TI_ORGAN}}, .orch_n = 1,
    },
    [TG_STYLE_HANDEL] = {
        /* ceremonial D major: trumpets and drums, plain strong harmony, marching eighths, the grand pause */
        .name = "HANDEL", .root = 2, .scale = TS_IONIAN,                             /* D major */
        .bpm = 92, .division = 4, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.45f, .melody_base = 64, .melody_range = 17,
        .profile = {0.15f, 0.55f, 0.82f, 0.95f}, .max_leap = 5,                        /* repeated notes, fanfares */
        .trill = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_REPEAT, .pad_mode = TG_PAD_REPEAT, .pad_base = 52, .pad_tones = 3,
        .fifth_bias = 2.0f, .repeat = 0.45f,
        .motif = 0.5f, .motif_beats = 2, .motif_use = 0.7f, .var_invert = 0.1f, .var_alter = 0.3f,
        .sforzando = 0.05f, .swell = 0.4f,
        .perc_mode = TG_PERC_PERIOD, .perc = {8, 0, 0, 0, 0, 0},                       /* trumpets and drums */
        .w_major = {10, 4, 2, 8, 5, 3, 9, 8, 2, 5, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
        .w_minor = {10, 5, 4, 7, 2, 4, 8, 8, 3, 4, 2, 2, 3, 2, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0},
        .gp = 0.2f, .tuning = TT_TUNE_WERCKMEISTER,
        .orch = {{TI_CELLO, TI_TRUMPET, TI_STRINGS}, {TI_CELLO, TI_VIOLIN, TI_STRINGS}, {TI_CELLO, TI_OBOE, TI_HARPSICHORD}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_SCARLATTI] = {
        /* keyboard sonatas: fast 3/8, leaps across the keyboard, repeated notes, strummed Spanish chords */
        .name = "SCARLATTI", .root = 2, .scale = TS_IONIAN,                          /* D major */
        .bpm = 180, .division = 2, .beats_per_bar = 3, .chord_beats = 3, .period_bars = 8,
        .melody_density = 0.75f, .melody_base = 62, .melody_range = 24,
        .profile = {0.20f, 0.55f, 0.75f, 0.88f}, .max_leap = 7,
        .trill = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_BAR, .pad_mode = TG_PAD_REPEAT, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.8f,                                             /* each half again */
        .motif = 0.5f, .motif_beats = 3, .motif_use = 0.6f, .var_invert = 0.2f, .var_alter = 0.3f,
        .sforzando = 0.08f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 4, 2, 7, 5, 3, 8, 9, 3, 4, 2, 2, 1, 2, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0},
        .w_minor = {10, 5, 5, 7, 2, 4, 8, 9, 4, 3, 2, 2, 4, 4, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0},
        .color_scale = TS_PHRYG_DOM, .color = 0.2f, .mixture = 0.2f,
        .octave = 0.12f, .strum_ms = 9,
        .orch = {{TI_HARPSICHORD, TI_HARPSICHORD, TI_HARPSICHORD}, {TI_HARPSICHORD, TI_HARPSICHORD, TI_GUITAR}},
        .orch_n = 2, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_RAMEAU] = {
        /* French ornament, the added-sixth chord, a tambourin over a drone and a tabor, gavottes between */
        .name = "RAMEAU", .root = 2, .scale = TS_IONIAN,                             /* D major, 2/4 */
        .bpm = 132, .division = 4, .beats_per_bar = 2, .chord_beats = 2, .period_bars = 8,
        .melody_density = 0.55f, .melody_base = 66, .melody_range = 17,
        .profile = {0.10f, 0.68f, 0.88f, 0.96f}, .max_leap = 4,
        .appoggiatura = 0.25f, .trill = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_PEDAL, .pad_mode = TG_PAD_BLOCK, .pad_base = 54, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.5f,
        .motif = 0.55f, .motif_beats = 2, .motif_use = 0.7f, .var_invert = 0.1f, .var_alter = 0.3f,
        .perc_mode = TG_PERC_PATTERN,
        .w_major = {10, 5, 3, 7, 4, 9, 8, 7, 2, 5, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},  /* ii7: the sixte ajoutee */
        .w_minor = {10, 5, 4, 7, 2, 8, 8, 7, 3, 4, 2, 2, 3, 2, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0},
        .turn = 0.3f, .tuning = TT_TUNE_WERCKMEISTER,
        .alt = {.bpm = 0.7f, .bass_mode = TG_BASS_ROOT + 1, .pad_mode = TG_PAD_ALBERTI + 1}, .alt_every = 2,
        .orch = {{TI_BASSOON, TI_FLUTE, TI_HARPSICHORD}, {TI_CELLO, TI_OBOE, TI_HARPSICHORD}}, .orch_n = 2,
        .orch_mode = TG_ORCH_CONTRAST,
        .drum_pat = {{TG_GM_SNARE, 58, 0x75}},                                          /* the tabor */
    },
    [TG_STYLE_PERGOLESI] = {
        /* Stabat Mater: two high voices in thirds, chains of suspensions, a walking bass, sighs */
        .name = "PERGOLESI", .root = 5, .scale = TS_AEOLIAN,                         /* F minor */
        .bpm = 56, .division = 4, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.4f, .melody_base = 65, .melody_range = 14,
        .profile = {0.04f, 0.82f, 0.94f, 0.98f}, .max_leap = 4,
        .appoggiatura = 0.3f, .chromatic = true, .suspension = 0.6f, .arrival_bars = 1,
        .bass_mode = TG_BASS_WALKING, .pad_mode = TG_PAD_DOUBLE, .pad_base = 58, .pad_tones = 3,
        .fifth_bias = 2.5f, .repeat = 0.35f,
        .motif = 0.5f, .motif_beats = 2, .motif_use = 0.7f, .var_invert = 0.1f, .var_alter = 0.3f,
        .swell = 0.4f, .rubato = 0.2f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 6, 3, 7, 5, 6, 6, 8, 4, 5, 4, 3, 4, 2, 1, 2, 0, 2, 0, 0, 0, 2, 3, 0},
        .w_minor = {10, 6, 5, 7, 3, 6, 6, 9, 5, 4, 3, 4, 4, 3, 4, 1, 0, 0, 2, 0, 0, 0, 3, 0},
        .direction = -0.3f, .tuning = TT_TUNE_WERCKMEISTER,
        .orch = {{TI_CELLO, TI_VOICE, TI_VOICE}, {TI_CELLO, TI_VIOLIN, TI_VIOLIN}}, .orch_n = 2,
        .orch_mode = TG_ORCH_RANDOM,
    },

    /* ---- 3: classical ------------------------------------------------------------------------------------ */
    [TG_STYLE_GLUCK] = {
        /* noble simplicity: a long plain flute melody over strings in triplets */
        .name = "GLUCK", .root = 2, .scale = TS_AEOLIAN,                             /* D minor */
        .bpm = 60, .division = 3, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.35f, .melody_base = 62, .melody_range = 17,
        .profile = {0.05f, 0.72f, 0.90f, 0.97f}, .max_leap = 4,
        .appoggiatura = 0.25f, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_FIGURE, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.5f,
        .motif = 0.55f, .motif_beats = 4, .motif_use = 0.6f, .var_invert = 0.05f, .var_alter = 0.3f,
        .swell = 0.3f, .rubato = 0.15f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 5, 2, 8, 5, 3, 8, 8, 2, 4, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
        .w_minor = {10, 6, 4, 8, 3, 5, 8, 9, 3, 3, 2, 3, 2, 2, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0},
        .pad_figure = {0, 2, 1, 0, 2, 1}, .figure_len = 6,
        .orch = {{TI_CELLO, TI_FLUTE, TI_STRINGS}, {TI_CELLO, TI_FLUTE, TI_HARP}}, .orch_n = 2,
        .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_HAYDN] = {
        /* wit and surprise: a tune of repeated notes worked from one idea, sudden accents, sudden quiet, pauses */
        .name = "HAYDN", .root = 7, .scale = TS_IONIAN,                              /* G major, 2/4 */
        .bpm = 120, .division = 4, .beats_per_bar = 2, .chord_beats = 2, .period_bars = 8,
        .melody_density = 0.45f, .melody_base = 67, .melody_range = 17,
        .profile = {0.25f, 0.60f, 0.85f, 0.95f}, .max_leap = 4,
        .appoggiatura = 0.1f, .trill = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_BAR, .pad_mode = TG_PAD_REPEAT, .pad_base = 55, .pad_tones = 3,
        .fifth_bias = 1.2f, .repeat = 0.45f,
        .motif = 0.7f, .motif_beats = 2, .motif_use = 0.9f, .var_invert = 0.25f, .var_alter = 0.35f,
        .sforzando = 0.03f, .swell = 0.2f,
        .perc_mode = TG_PERC_OFF, .perc = {5, 0, 3, 3, 4, 0},                          /* the Military symphony */
        .w_major = {10, 4, 2, 7, 5, 4, 8, 9, 2, 5, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
        .w_minor = {10, 5, 4, 7, 2, 5, 8, 9, 3, 4, 2, 2, 2, 2, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0},
        .gp = 0.2f, .echo = 0.25f,
        .orch = {{TI_CELLO, TI_VIOLIN, TI_STRINGS}, {TI_CELLO, TI_OBOE, TI_STRINGS}, {TI_CONTRABASS, TI_FLUTE, TI_STRINGS}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },

    /* ---- 4: early romantic ------------------------------------------------------------------------------
     * romantic vocabulary:
     *  major: I vi iii IV ii ii7 V V7 vii°7 V/V V/vi V/IV V/ii iv bVI bIII III VI Ger+6 I+ bVII+ iiø7 bII #iv°7
     *  minor: i VI III iv ii° iiø V V7 vii°7 V/V V/III V/iv VII bII iv7 Ger+6 I #iv°7 V/VI I+ bVII+ III+ iv6 #III */
    [TG_STYLE_SCHUBERT] = {
        /* the Lied: a singable line, major and minor trading places, third-related chords, repeated triplets */
        .name = "SCHUBERT", .root = 10, .scale = TS_IONIAN,                          /* B-flat major */
        .bpm = 72, .division = 3, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.35f, .melody_base = 62, .melody_range = 14,
        .profile = {0.08f, 0.75f, 0.90f, 0.97f}, .max_leap = 5,
        .appoggiatura = 0.15f, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_REPEAT, .pad_base = 53, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.45f,
        .motif = 0.55f, .motif_beats = 4, .motif_use = 0.6f, .var_invert = 0.1f, .var_alter = 0.3f,
        .swell = 0.3f, .rubato = 0.2f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 5, 2, 7, 4, 4, 7, 8, 2, 5, 3, 2, 2, 3, 5, 2, 2, 2, 1, 0, 0, 1, 1, 1},
        .w_minor = {10, 6, 4, 7, 2, 4, 7, 8, 3, 3, 3, 2, 2, 2, 3, 1, 4, 1, 2, 0, 0, 0, 2, 1},
        .vocab = TG_VOC_ROMANTIC, .mixture = 0.35f, .turn = 0.15f,
        .orch = {{TI_PIANO, TI_CLARINET, TI_PIANO}, {TI_PIANO, TI_VOICE, TI_PIANO}, {TI_CELLO, TI_VIOLIN, TI_STRINGS}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_ROSSINI] = {
        /* the galop and the Rossini crescendo: I and V, staccato repeated notes, more players with each round */
        .name = "ROSSINI", .root = 4, .scale = TS_IONIAN,                            /* E major, 2/4 */
        .bpm = 152, .division = 4, .beats_per_bar = 2, .chord_beats = 2, .period_bars = 8,
        .melody_density = 0.65f, .melody_base = 64, .melody_range = 15,
        .profile = {0.30f, 0.60f, 0.85f, 0.95f}, .max_leap = 4,
        .trill = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_BAR, .pad_mode = TG_PAD_OOMPAH, .pad_base = 56, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.6f,
        .motif = 0.7f, .motif_beats = 2, .motif_use = 0.85f, .var_invert = 0.05f, .var_alter = 0.2f,
        .swell = 1.0f,
        .perc_mode = TG_PERC_PERIOD, .perc = {5, 3, 5, 4, 4, 0},                       /* the banda */
        .w_major = {10, 2, 1, 5, 3, 2, 10, 9, 1, 4, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        .w_minor = {10, 3, 3, 5, 2, 2, 10, 9, 2, 3, 1, 1, 2, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0},
        .staccato = 0.6f,
        .evolve_periods = 6, .evolve_depth = 1.0f,
        .orch = {{TI_PIZZ, TI_FLUTE, TI_STRINGS}, {TI_CELLO, TI_CLARINET, TI_STRINGS}, {TI_TUBA, TI_TRUMPET, TI_BRASS}},
        .orch_n = 3, .orch_mode = TG_ORCH_ENERGY,
    },
    [TG_STYLE_BERLIOZ] = {
        /* the idee fixe returning again and again, bold changing orchestra, many timpani, sudden contrasts */
        .name = "BERLIOZ", .root = 0, .scale = TS_IONIAN,                            /* C major */
        .bpm = 88, .division = 2, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 8,
        .melody_density = 0.45f, .melody_base = 64, .melody_range = 19,
        .profile = {0.06f, 0.60f, 0.82f, 0.93f}, .max_leap = 6,
        .appoggiatura = 0.2f, .chromatic = true, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_BLOCK, .pad_base = 52, .pad_tones = 3,
        .fifth_bias = 1.3f, .repeat = 0.2f,
        .motif = 0.85f, .motif_beats = 4, .motif_use = 0.9f, .var_invert = 0.1f, .var_alter = 0.2f,
        .sforzando = 0.08f, .swell = 0.6f, .rubato = 0.2f,
        .perc_mode = TG_PERC_PERIOD, .perc = {9, 4, 5, 6, 0, 0},
        .w_major = {10, 5, 3, 6, 5, 4, 7, 8, 3, 5, 3, 3, 2, 3, 4, 3, 3, 2, 2, 0, 0, 2, 2, 2},
        .w_minor = {10, 6, 4, 7, 3, 5, 7, 8, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 0, 0, 1, 2, 1},
        .vocab = TG_VOC_ROMANTIC, .mixture = 0.2f, .echo = 0.2f,
        .alt = {.density = 1.3f, .pad_mode = TG_PAD_TREMOLO + 1}, .alt_every = 2,
        .orch = {{TI_CONTRABASS, TI_FLUTE, TI_STRINGS}, {TI_CELLO, TI_OBOE, TI_HARP}, {TI_TUBA, TI_BRASS, TI_STRINGS},
                 {TI_CONTRABASS, TI_CLARINET, TI_BELLS}}, .orch_n = 4, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_FELIX_MENDELSSOHN] = {
        /* the Italian symphony: a running 6/8, light staccato, repeated woodwind chords, elegant harmony */
        .name = "FELIX MENDELSSOHN", .root = 9, .scale = TS_IONIAN,                  /* A major, 6/8 */
        .bpm = 116, .division = 3, .beats_per_bar = 2, .chord_beats = 2, .period_bars = 8,
        .melody_density = 0.6f, .melody_base = 64, .melody_range = 17,
        .profile = {0.15f, 0.60f, 0.85f, 0.95f}, .max_leap = 5,
        .trill = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_BAR, .pad_mode = TG_PAD_REPEAT, .pad_base = 57, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.4f,
        .motif = 0.6f, .motif_beats = 2, .motif_use = 0.8f, .var_invert = 0.1f, .var_alter = 0.3f,
        .swell = 0.3f,
        .perc_mode = TG_PERC_OFF, .perc = {4, 0, 0, 0, 0, 0},
        .w_major = {10, 5, 3, 7, 5, 5, 7, 8, 3, 6, 4, 3, 4, 2, 1, 1, 0, 1, 0, 0, 0, 2, 2, 0},
        .w_minor = {10, 6, 4, 7, 3, 5, 7, 8, 4, 5, 3, 3, 3, 2, 2, 1, 0, 0, 2, 0, 0, 0, 2, 0},
        .staccato = 0.5f,
        .orch = {{TI_CONTRABASS, TI_VIOLIN, TI_FLUTE}, {TI_CELLO, TI_FLUTE, TI_CLARINET}, {TI_CELLO, TI_VIOLIN, TI_STRINGS}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_FANNY_MENDELSSOHN] = {
        /* piano lieder: broad lines, chromatic inner harmony, bold turns to other keys, wide broken chords */
        .name = "FANNY MENDELSSOHN", .root = 8, .scale = TS_IONIAN,                  /* A-flat major */
        .bpm = 66, .division = 4, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.4f, .melody_base = 63, .melody_range = 17,
        .profile = {0.05f, 0.72f, 0.90f, 0.97f}, .max_leap = 5,
        .appoggiatura = 0.25f, .chromatic = true, .suspension = 0.2f, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_BROKEN, .pad_base = 44, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.3f,
        .motif = 0.45f, .motif_beats = 4, .motif_use = 0.55f, .var_invert = 0.1f, .var_alter = 0.4f,
        .swell = 0.6f, .rubato = 0.3f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 5, 3, 6, 5, 5, 6, 8, 4, 6, 5, 4, 4, 4, 5, 3, 3, 3, 2, 0, 0, 3, 2, 2},
        .w_minor = {10, 6, 4, 7, 3, 6, 6, 8, 5, 5, 4, 3, 3, 3, 3, 2, 3, 2, 3, 0, 0, 1, 2, 2},
        .vocab = TG_VOC_ROMANTIC, .modulate = 0.3f,
        .orch = {{TI_PIANO, TI_PIANO, TI_PIANO}}, .orch_n = 1,
    },
    [TG_STYLE_ROBERT_SCHUMANN] = {
        /* Eusebius and Florestan in turn: dreamy and impetuous periods, syncopation, off-beat accents, inner voices */
        .name = "ROBERT SCHUMANN", .root = 5, .scale = TS_IONIAN,                    /* F major */
        .bpm = 66, .division = 4, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.35f, .melody_base = 60, .melody_range = 17,
        .profile = {0.06f, 0.70f, 0.88f, 0.96f}, .max_leap = 5,
        .syncopation = 0.45f, .appoggiatura = 0.2f, .suspension = 0.25f, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_COUNTER, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.35f,
        .motif = 0.55f, .motif_beats = 2, .motif_use = 0.7f, .var_invert = 0.2f, .var_alter = 0.3f,
        .sforzando = 0.08f, .swell = 0.5f, .rubato = 0.35f, .swing = 0.6f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 6, 3, 6, 5, 6, 6, 8, 4, 6, 4, 3, 4, 3, 2, 1, 1, 2, 1, 0, 0, 2, 1, 1},
        .w_minor = {10, 6, 5, 7, 3, 6, 6, 8, 5, 5, 3, 3, 3, 2, 3, 2, 2, 2, 2, 0, 0, 0, 2, 1},
        .vocab = TG_VOC_ROMANTIC,
        .alt = {.bpm = 1.55f, .density = 1.8f, .pad_mode = TG_PAD_REPEAT + 1}, .alt_every = 1,
        .orch = {{TI_PIANO, TI_PIANO, TI_PIANO}}, .orch_n = 1,
    },
    [TG_STYLE_CLARA_SCHUMANN] = {
        /* Romanzes: a lyrical minor-key line over triplet arpeggios, chromatic appoggiaturas, octave leaps */
        .name = "CLARA SCHUMANN", .root = 7, .scale = TS_AEOLIAN,                    /* G minor */
        .bpm = 76, .division = 3, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.45f, .melody_base = 67, .melody_range = 17,
        .profile = {0.05f, 0.70f, 0.88f, 0.96f}, .max_leap = 6,
        .appoggiatura = 0.3f, .chromatic = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_ARPEGGIO, .pad_base = 43, .pad_tones = 3,
        .fifth_bias = 1.8f, .repeat = 0.35f,
        .motif = 0.5f, .motif_beats = 4, .motif_use = 0.6f, .var_invert = 0.1f, .var_alter = 0.35f,
        .swell = 0.5f, .rubato = 0.3f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 5, 3, 6, 5, 5, 7, 8, 3, 5, 4, 3, 3, 2, 2, 1, 1, 1, 1, 0, 0, 1, 1, 1},
        .w_minor = {10, 6, 4, 7, 3, 6, 7, 8, 5, 4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 0, 0, 0, 2, 1},
        .vocab = TG_VOC_ROMANTIC, .octave = 0.1f,
        .orch = {{TI_PIANO, TI_PIANO, TI_PIANO}, {TI_PIANO, TI_VIOLIN, TI_PIANO}}, .orch_n = 2, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_LISZT] = {
        /* Liebestraum: the melody in the middle wrapped in wide arpeggios, diminished sevenths, Hungarian colour */
        .name = "LISZT", .root = 1, .scale = TS_IONIAN,                              /* D-flat major */
        .bpm = 60, .division = 3, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.4f, .melody_base = 60, .melody_range = 19,
        .profile = {0.05f, 0.65f, 0.86f, 0.95f}, .max_leap = 6,
        .appoggiatura = 0.25f, .chromatic = true, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_ARPEGGIO, .pad_base = 44, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.3f, .whole_tone = 0.1f,
        .motif = 0.5f, .motif_beats = 4, .motif_use = 0.6f, .var_invert = 0.15f, .var_alter = 0.4f,
        .sforzando = 0.04f, .swell = 0.8f, .rubato = 0.5f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 5, 3, 6, 5, 5, 6, 8, 5, 6, 4, 3, 3, 4, 5, 4, 3, 3, 2, 0, 0, 3, 2, 3},
        .w_minor = {10, 6, 4, 7, 3, 6, 6, 8, 6, 5, 3, 3, 3, 3, 3, 2, 3, 3, 2, 0, 0, 2, 2, 2},
        .vocab = TG_VOC_ROMANTIC, .color_scale = TS_HUNG_MINOR, .color = 0.25f, .turn = 0.2f,
        .orch = {{TI_PIANO, TI_PIANO, TI_PIANO}}, .orch_n = 1,
    },

    /* ---- 5: romantic -------------------------------------------------------------------------------------- */
    [TG_STYLE_VERDI] = {
        /* the opera house: oom-pah-pah, a big singable tune, brass, emphatic cadences, the banda */
        .name = "VERDI", .root = 3, .scale = TS_IONIAN,                              /* E-flat major, 3/4 */
        .bpm = 132, .division = 2, .beats_per_bar = 3, .chord_beats = 3, .period_bars = 8,
        .melody_density = 0.5f, .melody_base = 62, .melody_range = 15,
        .profile = {0.12f, 0.60f, 0.82f, 0.94f}, .max_leap = 5,
        .appoggiatura = 0.15f, .arrival_bars = 1,
        .bass_mode = TG_BASS_BAR, .pad_mode = TG_PAD_OOMPAH, .pad_base = 55, .pad_tones = 3,
        .fifth_bias = 1.2f, .repeat = 0.5f,
        .motif = 0.6f, .motif_beats = 3, .motif_use = 0.7f, .var_invert = 0.05f, .var_alter = 0.3f,
        .sforzando = 0.05f, .swell = 0.5f, .rubato = 0.15f,
        .perc_mode = TG_PERC_PERIOD, .perc = {5, 3, 5, 5, 2, 0},
        .w_major = {10, 4, 2, 7, 4, 3, 9, 9, 2, 5, 2, 2, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0},
        .w_minor = {10, 5, 4, 7, 2, 4, 9, 9, 3, 4, 2, 2, 2, 2, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0},
        .orch = {{TI_CONTRABASS, TI_VOICE, TI_PIZZ}, {TI_TUBA, TI_TRUMPET, TI_BRASS}, {TI_CELLO, TI_CLARINET, TI_PIZZ}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_WAGNER] = {
        /* endless melody: the half-diminished chord, deceptive endings, leitmotifs, swells that never land */
        .name = "WAGNER", .root = 9, .scale = TS_AEOLIAN,                            /* A minor (Tristan) */
        .bpm = 52, .division = 2, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 8,
        .melody_density = 0.35f, .melody_base = 60, .melody_range = 22,
        .profile = {0.04f, 0.55f, 0.78f, 0.90f}, .max_leap = 6,
        .appoggiatura = 0.5f, .chromatic = true, .suspension = 0.3f, .arrival_bars = 3,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_TREMOLO, .pad_base = 50, .pad_tones = 4,
        .fifth_bias = 1.0f, .repeat = 0.15f,
        .motif = 0.75f, .motif_beats = 4, .motif_use = 0.8f, .var_invert = 0.1f, .var_alter = 0.3f,
        .swell = 1.0f, .rubato = 0.35f,
        .perc_mode = TG_PERC_PERIOD, .perc = {7, 0, 0, 3, 0, 0},
        .w_major = {6, 6, 3, 5, 3, 4, 4, 6, 6, 5, 5, 4, 4, 5, 6, 5, 4, 4, 6, 0, 0, 8, 3, 4},
        .w_minor = {6, 6, 4, 6, 2, 8, 4, 6, 6, 5, 3, 4, 3, 4, 3, 6, 3, 4, 3, 0, 0, 3, 4, 4},
        .vocab = TG_VOC_ROMANTIC, .open = 0.75f, .modulate = 0.25f,
        .orch = {{TI_TUBA, TI_HORN, TI_STRINGS}, {TI_CONTRABASS, TI_CELLO, TI_STRINGS}, {TI_TUBA, TI_BRASS, TI_STRINGS},
                 {TI_CONTRABASS, TI_OBOE, TI_STRINGS}}, .orch_n = 4, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_GOUNOD] = {
        /* Ave Maria over Bach's prelude: a long plain melody above the broken-chord figure, organ, Amen */
        .name = "GOUNOD", .root = 0, .scale = TS_IONIAN,                             /* C major */
        .bpm = 66, .division = 4, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 4,
        .melody_density = 0.25f, .melody_base = 64, .melody_range = 15,
        .profile = {0.05f, 0.78f, 0.92f, 0.98f}, .max_leap = 5,
        .appoggiatura = 0.1f, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_FIGURE, .pad_base = 52, .pad_tones = 3,
        .fifth_bias = 2.0f, .repeat = 0.3f, .plagal = true,
        .motif = 0.4f, .motif_beats = 4, .motif_use = 0.5f, .var_invert = 0.05f, .var_alter = 0.3f,
        .swell = 0.4f, .rubato = 0.2f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 6, 3, 7, 6, 5, 6, 8, 3, 6, 4, 4, 4, 2, 1, 1, 0, 1, 0, 0, 0, 2, 2, 0},
        .w_minor = {10, 6, 4, 7, 3, 5, 6, 8, 4, 4, 3, 3, 3, 2, 2, 1, 0, 0, 2, 0, 0, 0, 2, 0},
        .pad_figure = {0, 1, 2, 3, 4, 2, 3, 4}, .figure_len = 8, .mel_min_ticks = 4,
        .orch = {{TI_ORGAN, TI_VIOLIN, TI_PIANO}, {TI_ORGAN, TI_VOICE, TI_HARP}}, .orch_n = 2, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_JOHANN_STRAUSS] = {
        /* the Viennese waltz: bass on one, chords on two and three, the second beat early, a leaping tune */
        .name = "JOHANN STRAUSS", .root = 2, .scale = TS_IONIAN,                     /* D major, 3/4 */
        .bpm = 168, .division = 2, .beats_per_bar = 3, .chord_beats = 3, .period_bars = 8,
        .melody_density = 0.45f, .melody_base = 62, .melody_range = 19,
        .profile = {0.10f, 0.50f, 0.75f, 0.90f}, .max_leap = 5,
        .appoggiatura = 0.15f, .chromatic = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_BAR, .pad_mode = TG_PAD_OOMPAH, .pad_base = 57, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.6f,
        .motif = 0.7f, .motif_beats = 3, .motif_use = 0.8f, .var_invert = 0.05f, .var_alter = 0.25f,
        .swell = 0.3f, .lilt = 0.8f,
        .perc_mode = TG_PERC_PERIOD, .perc = {3, 2, 3, 3, 4, 0},
        .w_major = {10, 3, 1, 6, 4, 4, 9, 9, 2, 5, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
        .w_minor = {10, 4, 3, 6, 2, 4, 9, 9, 3, 4, 2, 2, 2, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0},
        .octave = 0.05f,
        .orch = {{TI_CONTRABASS, TI_VIOLIN, TI_HORN}, {TI_CELLO, TI_FLUTE, TI_PIZZ}, {TI_CONTRABASS, TI_STRINGS, TI_BRASS}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_BRUCKNER] = {
        /* cathedrals of sound: tremolo from nothing, brass chorales between, pauses, pedal points, octave themes */
        .name = "BRUCKNER", .root = 2, .scale = TS_AEOLIAN,                          /* D minor */
        .bpm = 56, .division = 3, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 8,
        .melody_density = 0.3f, .melody_base = 57, .melody_range = 19,
        .profile = {0.05f, 0.50f, 0.75f, 0.90f}, .max_leap = 6,
        .arrival_bars = 3,
        .bass_mode = TG_BASS_PEDAL, .pad_mode = TG_PAD_TREMOLO, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 1.8f, .repeat = 0.3f, .plagal = true,
        .motif = 0.6f, .motif_beats = 4, .motif_use = 0.7f, .var_invert = 0.2f, .var_alter = 0.3f,
        .swell = 1.0f, .rubato = 0.1f,
        .perc_mode = TG_PERC_PERIOD, .perc = {8, 0, 0, 4, 0, 0},
        .w_major = {10, 5, 3, 8, 5, 4, 7, 8, 3, 5, 3, 3, 3, 3, 4, 4, 3, 3, 2, 0, 0, 2, 2, 1},
        .w_minor = {10, 6, 4, 8, 3, 5, 7, 8, 4, 4, 3, 3, 3, 4, 3, 3, 2, 2, 2, 0, 0, 1, 2, 2},
        .vocab = TG_VOC_ROMANTIC, .modulate = 0.2f, .gp = 0.35f, .octave = 0.15f,
        .alt = {.pad_mode = TG_PAD_BLOCK + 1, .bass_mode = TG_BASS_ROOT + 1}, .alt_every = 2,
        .orch = {{TI_CONTRABASS, TI_HORN, TI_STRINGS}, {TI_TUBA, TI_BRASS, TI_BRASS}}, .orch_n = 2,
        .orch_mode = TG_ORCH_CONTRAST,
    },
    [TG_STYLE_BIZET] = {
        /* the Habanera: the dotted bass, a melody sliding down by semitones, minor turning major, castanets */
        .name = "BIZET", .root = 2, .scale = TS_AEOLIAN,                             /* D minor, 2/4 */
        .bpm = 72, .division = 4, .beats_per_bar = 2, .chord_beats = 2, .period_bars = 8,
        .melody_density = 0.45f, .melody_base = 62, .melody_range = 15,
        .profile = {0.05f, 0.80f, 0.92f, 0.97f}, .max_leap = 4,
        .arrival_bars = 1,
        .bass_mode = TG_BASS_OSTINATO, .pad_mode = TG_PAD_BLOCK, .pad_base = 55, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.6f,
        .motif = 0.7f, .motif_beats = 2, .motif_use = 0.8f, .var_invert = 0.05f, .var_alter = 0.25f,
        .swell = 0.3f, .rubato = 0.15f,
        .perc_mode = TG_PERC_PATTERN,
        .w_major = {10, 4, 2, 7, 4, 3, 8, 9, 2, 4, 2, 2, 1, 2, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0},
        .w_minor = {10, 6, 4, 7, 3, 4, 8, 9, 3, 4, 2, 2, 3, 3, 2, 0, 0, 0, 1, 0, 0, 0, 2, 0},
        .mixture = 0.4f, .direction = -0.5f, .mel_add = (1u << 11) | (1u << 9),        /* C# and B: the chromatic slide */
        .bass_rhythm = (1u << 0) | (1u << 3) | (1u << 4) | (1u << 6), .bass_notes = {0, 0, 2, 0}, .bass_len = 4,
        .orch = {{TI_PIZZ, TI_VOICE, TI_STRINGS}, {TI_PIZZ, TI_OBOE, TI_STRINGS}, {TI_CELLO, TI_FLUTE, TI_STRINGS}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
        .drum_pat = {{TG_GM_CASTANETS, 52, 0x59}, {TG_GM_TAMBOURINE, 40, 0x44}},
    },
    [TG_STYLE_FRANCK] = {
        /* chromatic organ harmony: keys melting into keys, sliding voices, a canon between the instruments */
        .name = "FRANCK", .root = 9, .scale = TS_IONIAN,                             /* A major, 9/8 */
        .bpm = 52, .division = 3, .beats_per_bar = 3, .chord_beats = 3, .period_bars = 8,
        .melody_density = 0.5f, .melody_base = 64, .melody_range = 19,
        .profile = {0.05f, 0.65f, 0.85f, 0.95f}, .max_leap = 5,
        .appoggiatura = 0.25f, .chromatic = true, .suspension = 0.25f, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_BROKEN, .pad_base = 45, .pad_tones = 3,
        .fifth_bias = 1.2f, .repeat = 0.2f,
        .motif = 0.6f, .motif_beats = 3, .motif_use = 0.7f, .var_invert = 0.15f, .var_alter = 0.3f,
        .swell = 0.7f, .rubato = 0.3f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 6, 3, 5, 5, 5, 6, 8, 6, 6, 5, 4, 4, 5, 5, 4, 4, 4, 4, 0, 0, 4, 3, 3},
        .w_minor = {10, 6, 4, 6, 3, 6, 6, 8, 6, 5, 4, 3, 3, 4, 3, 4, 3, 4, 3, 0, 0, 2, 3, 3},
        .vocab = TG_VOC_ROMANTIC, .open = 0.35f, .modulate = 0.45f, .sideslip = 0.1f,
        .canon_ticks = 9, .canon_steps = -7,
        .alt = {.pad_mode = TG_PAD_CANON + 1}, .alt_every = 2,
        .orch = {{TI_PIANO, TI_VIOLIN, TI_PIANO}, {TI_ORGAN, TI_VIOLIN, TI_ORGAN}, {TI_ORGAN, TI_ORGAN, TI_ORGAN}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_BRUCH] = {
        /* a rhapsodic singing violin (or cello) over warm sustained strings and horn */
        .name = "BRUCH", .root = 7, .scale = TS_AEOLIAN,                             /* G minor */
        .bpm = 60, .division = 4, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.35f, .melody_base = 62, .melody_range = 24,
        .profile = {0.05f, 0.60f, 0.82f, 0.93f}, .max_leap = 6,
        .appoggiatura = 0.2f, .suspension = 0.2f, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_BLOCK, .pad_base = 53, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.35f,
        .motif = 0.5f, .motif_beats = 4, .motif_use = 0.6f, .var_invert = 0.1f, .var_alter = 0.35f,
        .swell = 0.6f, .rubato = 0.45f,
        .perc_mode = TG_PERC_OFF, .perc = {4, 0, 0, 0, 0, 0},
        .w_major = {10, 6, 3, 7, 5, 5, 7, 8, 3, 5, 4, 3, 3, 3, 3, 2, 1, 2, 0, 0, 0, 2, 2, 0},
        .w_minor = {10, 7, 5, 7, 3, 5, 7, 8, 4, 5, 3, 3, 3, 3, 3, 2, 0, 0, 2, 0, 0, 0, 2, 0},
        .mixture = 0.3f, .octave = 0.1f, .turn = 0.2f,
        .orch = {{TI_CONTRABASS, TI_VIOLIN, TI_STRINGS}, {TI_CONTRABASS, TI_CELLO, TI_STRINGS},
                 {TI_CONTRABASS, TI_VIOLIN, TI_HORN}}, .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_MUSSORGSKY] = {
        /* Promenade: 5/4 then 6/4, a pentatonic folk tune, planing chords, the Great Gate's bells between */
        .name = "MUSSORGSKY", .root = 10, .scale = TS_IONIAN,                        /* B-flat */
        .bpm = 92, .division = 2, .beats_per_bar = 5, .chord_beats = 1, .period_bars = 4,
        .meter = {6, 4, 6, 6}, .meter_bars = 1u << 2,                                   /* 5/4 (3+2), 6/4 (3+3) */
        .melody_density = 0.45f, .melody_base = 58, .melody_range = 15,
        .profile = {0.15f, 0.75f, 0.90f, 0.97f}, .max_leap = 4,
        .melody_mask = 0x295, .arrival_bars = 1,
        .bass_mode = TG_BASS_BAR, .pad_mode = TG_PAD_PARALLEL, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.4f,
        .motif = 0.7f, .motif_beats = 5, .motif_use = 0.8f, .var_invert = 0.1f, .var_alter = 0.3f,
        .swell = 0.5f,
        .perc_mode = TG_PERC_OFF, .perc = {6, 0, 3, 5, 0, 0},
        .w_major = {10, 6, 4, 7, 6, 6, 0, 6, 0, 0, 2, 0, 0, 5, 3, 0, 2, 2, 3, 0, 0, 0, 0, 0},
        .w_minor = {10, 6, 5, 7, 4, 5, 3, 6, 0, 3, 2, 0, 3, 3, 3, 3, 2, 2, 0, 0, 0, 0, 0, 0},
        .vocab = TG_VOC_MODAL,
        .alt = {.bpm = 0.8f, .pad_mode = TG_PAD_BLOCK + 1, .bass_mode = TG_BASS_TOLL + 1}, .alt_every = 2,
        .orch = {{TI_CONTRABASS, TI_TRUMPET, TI_BRASS}, {TI_TUBA, TI_BRASS, TI_BELLS}}, .orch_n = 2,
        .orch_mode = TG_ORCH_CONTRAST,
    },
    [TG_STYLE_TCHAIKOVSKY] = {
        /* sequences rising to a climax, falling scale lines, harp and celesta colour, big swells */
        .name = "TCHAIKOVSKY", .root = 11, .scale = TS_AEOLIAN,                      /* B minor */
        .bpm = 72, .division = 4, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.4f, .melody_base = 62, .melody_range = 19,
        .profile = {0.04f, 0.80f, 0.92f, 0.97f}, .max_leap = 5,
        .appoggiatura = 0.25f, .chromatic = true, .suspension = 0.3f, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_BLOCK, .pad_base = 52, .pad_tones = 3,
        .fifth_bias = 1.8f, .repeat = 0.3f,
        .motif = 0.65f, .motif_beats = 4, .motif_use = 0.9f, .var_invert = 0.05f, .var_alter = 0.25f,
        .sforzando = 0.03f, .swell = 0.9f, .rubato = 0.35f,
        .perc_mode = TG_PERC_PERIOD, .perc = {7, 0, 2, 6, 2, 0},
        .w_major = {10, 6, 3, 7, 5, 5, 7, 8, 4, 6, 4, 3, 3, 3, 3, 2, 2, 2, 2, 0, 0, 2, 1, 1},
        .w_minor = {10, 7, 4, 7, 3, 6, 7, 8, 5, 5, 3, 3, 3, 3, 3, 2, 2, 2, 2, 0, 0, 0, 2, 1},
        .vocab = TG_VOC_ROMANTIC, .direction = -0.4f,
        .alt = {.pad_mode = TG_PAD_ARPEGGIO + 1}, .alt_every = 2,
        .orch = {{TI_CONTRABASS, TI_VIOLIN, TI_STRINGS}, {TI_CONTRABASS, TI_BASSOON, TI_STRINGS},
                 {TI_CONTRABASS, TI_FLUTE, TI_CELESTA}, {TI_TUBA, TI_BRASS, TI_STRINGS}}, .orch_n = 4,
        .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_SMETANA] = {
        /* Vltava: the river's flowing figure in the flutes, a broad folk tune with its raised seventh */
        .name = "SMETANA", .root = 4, .scale = TS_HARM_MINOR,                        /* E minor, 6/8 */
        .bpm = 66, .division = 3, .beats_per_bar = 2, .chord_beats = 2, .period_bars = 8,
        .melody_density = 0.4f, .melody_base = 64, .melody_range = 15,
        .profile = {0.05f, 0.70f, 0.90f, 0.97f}, .max_leap = 5,
        .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_FIGURE, .pad_base = 59, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.5f,
        .motif = 0.6f, .motif_beats = 4, .motif_use = 0.7f, .var_invert = 0.05f, .var_alter = 0.3f,
        .swell = 0.5f, .rubato = 0.15f,
        .perc_mode = TG_PERC_PERIOD, .perc = {4, 0, 0, 2, 3, 0},
        .w_major = {10, 5, 3, 7, 5, 4, 7, 8, 2, 4, 3, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0},
        .w_minor = {10, 6, 4, 7, 2, 4, 7, 8, 3, 3, 3, 3, 2, 1, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0},
        .pad_figure = {0, 1, 2, 3, 2, 1}, .figure_len = 6,
        .orch = {{TI_CONTRABASS, TI_VIOLIN, TI_FLUTE}, {TI_CELLO, TI_STRINGS, TI_FLUTE}}, .orch_n = 2,
        .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_GRIEG] = {
        /* Morning: a pentatonic pastoral tune passed between flute and oboe, a drone fifth, Lydian light */
        .name = "GRIEG", .root = 4, .scale = TS_IONIAN,                              /* E major, 6/8 */
        .bpm = 58, .division = 3, .beats_per_bar = 2, .chord_beats = 2, .period_bars = 8,
        .melody_density = 0.4f, .melody_base = 64, .melody_range = 15,
        .profile = {0.06f, 0.70f, 0.90f, 0.97f}, .max_leap = 4,
        .melody_mask = 0x295, .arrival_bars = 2,
        .bass_mode = TG_BASS_PEDAL, .pad_mode = TG_PAD_BLOCK, .pad_base = 56, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.5f,
        .motif = 0.75f, .motif_beats = 4, .motif_use = 0.8f, .var_invert = 0.05f, .var_alter = 0.25f,
        .swell = 0.6f, .rubato = 0.15f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 5, 4, 6, 4, 4, 6, 6, 2, 4, 4, 3, 3, 3, 2, 2, 1, 2, 2, 0, 0, 2, 1, 0},
        .w_minor = {10, 6, 5, 6, 2, 4, 6, 6, 2, 3, 3, 3, 3, 2, 2, 2, 1, 2, 2, 0, 0, 2, 1, 0},
        .color_scale = TS_LYDIAN, .color = 0.25f, .mixture = 0.15f,
        .orch = {{TI_CELLO, TI_FLUTE, TI_STRINGS}, {TI_CELLO, TI_OBOE, TI_STRINGS}}, .orch_n = 2,
        .orch_mode = TG_ORCH_CYCLE,
    },
    [TG_STYLE_MASSENET] = {
        /* Meditation: a slow violin with chromatic appoggiaturas and expressive leaps over harp arpeggios */
        .name = "MASSENET", .root = 2, .scale = TS_IONIAN,                           /* D major */
        .bpm = 54, .division = 4, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.3f, .melody_base = 66, .melody_range = 19,
        .profile = {0.04f, 0.60f, 0.85f, 0.95f}, .max_leap = 6,
        .appoggiatura = 0.35f, .chromatic = true, .suspension = 0.2f, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_ARPEGGIO, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.4f,
        .motif = 0.55f, .motif_beats = 4, .motif_use = 0.6f, .var_invert = 0.05f, .var_alter = 0.35f,
        .swell = 0.5f, .rubato = 0.45f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {8, 6, 5, 4, 6, 4, 5, 6, 7, 3, 4, 3, 1, 2, 1, 3, 4, 0, 0, 0, 0, 5, 3, 1},
        .w_minor = {10, 4, 6, 3, 6, 4, 7, 2, 2, 4, 3, 1, 3, 2, 2, 2, 2, 0, 2, 0, 0, 2, 1, 1},
        .vocab = TG_VOC_POP, .turn = 0.15f, .mel_min_ticks = 3,
        .orch = {{TI_CONTRABASS, TI_VIOLIN, TI_HARP}, {TI_CELLO, TI_VIOLIN, TI_HARP}}, .orch_n = 2,
        .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_RIMSKY_KORSAKOV] = {
        /* Scheherazade: violin arabesques over harp, augmented seconds, octatonic colour, a glittering orchestra */
        .name = "RIMSKY-KORSAKOV", .root = 4, .scale = TS_HARM_MINOR,                /* E minor */
        .bpm = 76, .division = 4, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 4,
        .melody_density = 0.5f, .melody_base = 67, .melody_range = 19,
        .profile = {0.03f, 0.70f, 0.88f, 0.96f}, .max_leap = 5,
        .appoggiatura = 0.2f, .chromatic = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_ARPEGGIO, .pad_base = 48, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.4f,
        .motif = 0.6f, .motif_beats = 4, .motif_use = 0.7f, .var_invert = 0.1f, .var_alter = 0.3f,
        .swell = 0.6f, .rubato = 0.3f,
        .perc_mode = TG_PERC_PERIOD, .perc = {5, 0, 0, 4, 5, 5},
        .w_major = {10, 5, 3, 7, 5, 4, 7, 8, 3, 4, 3, 3, 3, 3, 2, 1, 0, 1, 2, 0, 0, 0, 3, 1},
        .w_minor = {10, 6, 4, 7, 3, 5, 7, 8, 4, 4, 3, 3, 4, 4, 2, 1, 0, 1, 2, 0, 0, 0, 3, 1},
        .color_scale = TS_OCT_HW, .color = 0.3f, .turn = 0.3f,
        .orch = {{TI_CONTRABASS, TI_VIOLIN, TI_HARP}, {TI_CELLO, TI_CLARINET, TI_HARP}, {TI_TUBA, TI_BRASS, TI_STRINGS},
                 {TI_CONTRABASS, TI_FLUTE, TI_STRINGS}}, .orch_n = 4, .orch_mode = TG_ORCH_RANDOM,
    },

    /* ---- 6: late romantic and impressionism ------------------------------------------------------------ */
    [TG_STYLE_WIDOR] = {
        /* the Toccata: unbroken staccato sixteenths above, the theme in long pedal notes below, full organ */
        .name = "WIDOR", .root = 5, .scale = TS_IONIAN,                              /* F major */
        .bpm = 116, .division = 4, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.25f, .melody_base = 41, .melody_range = 14,                /* the pedal theme */
        .profile = {0.05f, 0.60f, 0.85f, 0.95f}, .max_leap = 5,
        .arrival_bars = 1,
        .bass_mode = TG_BASS_NONE, .pad_mode = TG_PAD_FIGURE, .pad_base = 60, .pad_tones = 3,
        .fifth_bias = 2.0f, .repeat = 0.4f,
        .motif = 0.6f, .motif_beats = 4, .motif_use = 0.7f, .var_invert = 0.1f, .var_alter = 0.3f,
        .swell = 0.6f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 5, 4, 7, 5, 5, 6, 8, 3, 6, 4, 3, 4, 2, 2, 2, 0, 2, 1, 0, 0, 2, 3, 1},
        .w_minor = {10, 6, 4, 7, 3, 5, 6, 8, 4, 5, 3, 3, 3, 2, 2, 1, 0, 0, 2, 0, 0, 0, 3, 1},
        .pad_figure = {5, 3, 4, 2, 5, 3, 4, 2}, .figure_len = 8, .mel_min_ticks = 4,
        .orch = {{TI_ORGAN, TI_ORGAN, TI_ORGAN}}, .orch_n = 1,
    },
    [TG_STYLE_FAURE] = {
        /* the Pavane: a pizzicato pulse, off-beat chords, a flute line, the modal minor dominant, side-steps */
        .name = "FAURE", .root = 6, .scale = TS_AEOLIAN,                             /* F-sharp minor */
        .bpm = 88, .division = 2, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.4f, .melody_base = 66, .melody_range = 15,
        .profile = {0.05f, 0.72f, 0.90f, 0.97f}, .max_leap = 5,
        .appoggiatura = 0.15f, .suspension = 0.2f, .arrival_bars = 2,
        .bass_mode = TG_BASS_BEAT, .pad_mode = TG_PAD_OOMPAH, .pad_base = 57, .pad_tones = 3,
        .fifth_bias = 1.3f, .repeat = 0.4f,
        .motif = 0.55f, .motif_beats = 4, .motif_use = 0.65f, .var_invert = 0.1f, .var_alter = 0.3f,
        .swell = 0.2f, .rubato = 0.15f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 6, 4, 7, 5, 7, 0, 3, 0, 0, 0, 2, 0, 3, 2, 0, 1, 2, 0, 0, 0, 0, 0, 0},
        .w_minor = {10, 6, 5, 7, 0, 7, 5, 3, 0, 0, 0, 2, 3, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0},
        .vocab = TG_VOC_MODAL, .sideslip = 0.12f, .modulate = 0.25f,
        .orch = {{TI_PIZZ, TI_FLUTE, TI_PIZZ}, {TI_CELLO, TI_OBOE, TI_STRINGS}}, .orch_n = 2, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_ELGAR] = {
        /* nobilmente: a broad melody of rising sixths and sevenths over a stepping bass, warm strings */
        .name = "ELGAR", .root = 3, .scale = TS_IONIAN,                              /* E-flat major, 3/4 */
        .bpm = 50, .division = 2, .beats_per_bar = 3, .chord_beats = 3, .period_bars = 8,
        .melody_density = 0.35f, .melody_base = 58, .melody_range = 19,
        .profile = {0.05f, 0.55f, 0.75f, 0.90f}, .max_leap = 6,
        .appoggiatura = 0.2f, .suspension = 0.35f, .arrival_bars = 2,
        .bass_mode = TG_BASS_WALKING, .pad_mode = TG_PAD_BLOCK, .pad_base = 51, .pad_tones = 4,
        .fifth_bias = 1.5f, .repeat = 0.35f,
        .motif = 0.6f, .motif_beats = 3, .motif_use = 0.7f, .var_invert = 0.05f, .var_alter = 0.3f,
        .swell = 0.9f, .rubato = 0.3f,
        .perc_mode = TG_PERC_PERIOD, .perc = {7, 0, 3, 6, 0, 0},
        .w_major = {10, 6, 4, 7, 5, 7, 6, 8, 3, 6, 5, 4, 4, 2, 2, 4, 2, 4, 0, 0, 0, 3, 3, 1},
        .w_minor = {10, 6, 5, 7, 3, 6, 6, 8, 4, 5, 4, 3, 3, 2, 3, 3, 1, 2, 2, 0, 0, 1, 3, 1},
        .octave = 0.05f,
        .orch = {{TI_CONTRABASS, TI_VIOLIN, TI_STRINGS}, {TI_CONTRABASS, TI_HORN, TI_STRINGS}, {TI_TUBA, TI_BRASS, TI_STRINGS}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_PUCCINI] = {
        /* verismo: a soaring tune doubled an octave below, octave leaps, pentatonic colour, added-note chords */
        .name = "PUCCINI", .root = 8, .scale = TS_IONIAN,                            /* A-flat major, 6/8 */
        .bpm = 52, .division = 3, .beats_per_bar = 2, .chord_beats = 2, .period_bars = 8,
        .melody_density = 0.4f, .melody_base = 63, .melody_range = 17,
        .profile = {0.08f, 0.60f, 0.82f, 0.93f}, .max_leap = 7,
        .appoggiatura = 0.25f, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_DOUBLE, .pad_base = 48, .pad_tones = 3,
        .fifth_bias = 1.3f, .repeat = 0.35f,
        .motif = 0.6f, .motif_beats = 4, .motif_use = 0.7f, .var_invert = 0.05f, .var_alter = 0.3f,
        .swell = 0.6f, .rubato = 0.5f,
        .perc_mode = TG_PERC_OFF, .perc = {4, 0, 0, 3, 0, 0},
        .w_major = {10, 4, 6, 3, 6, 3, 4, 6, 7, 2, 3, 3, 2, 2, 2, 3, 4, 1, 0, 0, 0, 4, 4, 1},
        .w_minor = {10, 4, 6, 3, 6, 4, 7, 2, 3, 4, 3, 1, 3, 2, 2, 2, 2, 0, 2, 0, 0, 2, 1, 1},
        .vocab = TG_VOC_POP, .open = 0.15f, .color_scale = TS_PENT_MAJOR, .color = 0.2f, .octave = 0.12f,
        .pad_interval = 12,
        .orch = {{TI_CONTRABASS, TI_VOICE, TI_STRINGS}, {TI_CONTRABASS, TI_VIOLIN, TI_HARP}, {TI_CELLO, TI_FLUTE, TI_STRINGS}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_RICHARD_STRAUSS] = {
        /* Don Juan: leaping horn themes, chromatic mediants, sudden side-slips, tremolo strings, timpani */
        .name = "RICHARD STRAUSS", .root = 4, .scale = TS_IONIAN,                    /* E major */
        .bpm = 84, .division = 4, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 4,
        .melody_density = 0.45f, .melody_base = 59, .melody_range = 24,
        .profile = {0.04f, 0.45f, 0.70f, 0.85f}, .max_leap = 7,
        .appoggiatura = 0.3f, .chromatic = true, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_TREMOLO, .pad_base = 50, .pad_tones = 4,
        .fifth_bias = 1.0f, .repeat = 0.2f,
        .motif = 0.6f, .motif_beats = 4, .motif_use = 0.7f, .var_invert = 0.15f, .var_alter = 0.35f,
        .sforzando = 0.05f, .swell = 0.9f, .rubato = 0.25f,
        .perc_mode = TG_PERC_PERIOD, .perc = {8, 0, 3, 6, 0, 0},
        .w_major = {10, 5, 3, 6, 5, 5, 6, 7, 4, 5, 4, 3, 3, 4, 6, 6, 6, 5, 3, 0, 0, 4, 3, 2},
        .w_minor = {10, 6, 4, 6, 3, 5, 6, 7, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 2, 3, 4},
        .vocab = TG_VOC_ROMANTIC, .open = 0.35f, .sideslip = 0.15f, .modulate = 0.3f, .octave = 0.15f,
        .orch = {{TI_TUBA, TI_HORN, TI_STRINGS}, {TI_CONTRABASS, TI_VIOLIN, TI_STRINGS}, {TI_TUBA, TI_BRASS, TI_STRINGS},
                 {TI_CONTRABASS, TI_OBOE, TI_HARP}}, .orch_n = 4, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_SIBELIUS] = {
        /* slow harmony on long pedals, an oscillating string figure, a swinging horn motif, slow growth */
        .name = "SIBELIUS", .root = 3, .scale = TS_IONIAN,                           /* E-flat major, 3/4 */
        .bpm = 72, .division = 2, .beats_per_bar = 3, .chord_beats = 6, .period_bars = 8,
        .melody_density = 0.35f, .melody_base = 58, .melody_range = 15,
        .profile = {0.08f, 0.60f, 0.85f, 0.95f}, .max_leap = 4,
        .arrival_bars = 2,
        .bass_mode = TG_BASS_PEDAL, .pad_mode = TG_PAD_FIGURE, .pad_base = 55, .pad_tones = 3,
        .fifth_bias = 1.2f, .repeat = 0.4f,
        .motif = 0.75f, .motif_beats = 3, .motif_use = 0.85f, .var_invert = 0.1f, .var_alter = 0.3f,
        .swell = 0.8f,
        .perc_mode = TG_PERC_PERIOD, .perc = {7, 0, 0, 3, 0, 0},
        .w_major = {10, 6, 3, 7, 5, 7, 0, 0, 0, 0, 3, 2, 0, 5, 3, 2, 3, 4, 2, 0, 0, 0, 0, 0},
        .w_minor = {10, 6, 4, 7, 4, 6, 4, 4, 0, 2, 2, 2, 2, 4, 3, 2, 3, 4, 0, 0, 0, 0, 0, 0},
        .vocab = TG_VOC_MODAL, .pad_figure = {0, 1, 0, 1, 2, 1}, .figure_len = 6,
        .evolve_periods = 8, .evolve_depth = 0.9f,
        .orch = {{TI_CONTRABASS, TI_HORN, TI_STRINGS}, {TI_CONTRABASS, TI_FLUTE, TI_STRINGS}, {TI_TUBA, TI_BRASS, TI_STRINGS}},
        .orch_n = 3, .orch_mode = TG_ORCH_ENERGY,
    },
    [TG_STYLE_RACHMANINOFF] = {
        /* the second concerto: tolling low octaves, wide chords, a narrow long-breathed melody, sevenths */
        .name = "RACHMANINOFF", .root = 0, .scale = TS_AEOLIAN,                      /* C minor */
        .bpm = 66, .division = 3, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.3f, .melody_base = 60, .melody_range = 14,
        .profile = {0.05f, 0.85f, 0.95f, 0.98f}, .max_leap = 4,
        .appoggiatura = 0.2f, .chromatic = true, .suspension = 0.3f, .arrival_bars = 2,
        .bass_mode = TG_BASS_TOLL, .pad_mode = TG_PAD_BLOCK, .pad_base = 45, .pad_tones = 4,
        .fifth_bias = 1.5f, .repeat = 0.3f,
        .motif = 0.55f, .motif_beats = 4, .motif_use = 0.65f, .var_invert = 0.05f, .var_alter = 0.3f,
        .swell = 0.9f, .rubato = 0.45f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 6, 4, 6, 5, 6, 6, 8, 4, 6, 4, 3, 3, 4, 4, 3, 3, 2, 2, 0, 0, 3, 2, 2},
        .w_minor = {10, 7, 5, 7, 3, 6, 6, 8, 5, 5, 4, 4, 3, 3, 4, 2, 2, 2, 2, 0, 0, 1, 3, 2},
        .vocab = TG_VOC_ROMANTIC, .mixture = 0.15f, .spread = true, .mel_min_ticks = 3,
        .alt = {.pad_mode = TG_PAD_ARPEGGIO + 1}, .alt_every = 2,
        .orch = {{TI_PIANO, TI_VIOLIN, TI_PIANO}, {TI_PIANO, TI_STRINGS, TI_PIANO}, {TI_PIANO, TI_CELLO, TI_PIANO}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_HOLST] = {
        /* Mars: 5/4, the col legno ostinato on one note, a slow brass theme of fourths, dissonance building */
        .name = "HOLST", .root = 7, .scale = TS_PHRYGIAN,                            /* on G, 5/4 */
        .bpm = 160, .division = 2, .beats_per_bar = 5, .chord_beats = 1, .period_bars = 4,
        .meter = {6, 4},                                                                /* 3 + 2 */
        .melody_density = 0.3f, .melody_base = 55, .melody_range = 15,
        .profile = {0.10f, 0.55f, 0.80f, 0.92f}, .max_leap = 5,
        .arrival_bars = 1,
        .bass_mode = TG_BASS_OSTINATO, .pad_mode = TG_PAD_BLOCK, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.3f,
        .motif = 0.7f, .motif_beats = 5, .motif_use = 0.8f, .var_invert = 0.1f, .var_alter = 0.3f,
        .swell = 1.0f,
        .perc_mode = TG_PERC_PATTERN,
        .w_major = {6, 0, 4, 3, 4, 3, 3, 5, 3, 3, 3, 3, 4, 3, 6, 4, 5, 2, 2, 0, 0, 4, 3, 4},
        .w_minor = {6, 0, 4, 3, 4, 3, 3, 5, 3, 3, 3, 3, 4, 3, 6, 4, 5, 2, 2, 0, 0, 4, 3, 4},
        .vocab = TG_VOC_MODERN, .open = 0.5f, .color_scale = TS_OCT_HW, .color = 0.2f, .mel_min_ticks = 4,
        .bass_rhythm = 0x1D7, .bass_notes = {0}, .bass_len = 1,
        .evolve_periods = 6, .evolve_depth = 1.0f,
        .orch = {{TI_CONTRABASS, TI_BRASS, TI_BRASS}, {TI_TUBA, TI_TRUMPET, TI_BRASS}}, .orch_n = 2,
        .orch_mode = TG_ORCH_RANDOM,
        .drum_pat = {{TG_GM_SIDE_STICK, 55, 0x1D7}, {TG_GM_LOW_TOM, 70, 0x001}},
    },
    [TG_STYLE_VAUGHAN_WILLIAMS] = {
        /* The Lark Ascending: a high pentatonic violin over planing modal triads */
        .name = "VAUGHAN WILLIAMS", .root = 7, .scale = TS_DORIAN,                   /* G Dorian */
        .bpm = 60, .division = 4, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 4,
        .melody_density = 0.55f, .melody_base = 72, .melody_range = 19,
        .profile = {0.05f, 0.70f, 0.88f, 0.96f}, .max_leap = 4,
        .melody_mask = 0x4A9, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_PARALLEL, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.35f,
        .motif = 0.5f, .motif_beats = 4, .motif_use = 0.6f, .var_invert = 0.15f, .var_alter = 0.35f,
        .swell = 0.5f, .rubato = 0.3f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 6, 3, 7, 5, 6, 0, 2, 0, 3, 2, 2, 0, 5, 3, 0, 3, 2, 2, 0, 0, 0, 0, 0},
        .w_minor = {10, 0, 6, 7, 5, 6, 7, 2, 0, 3, 2, 2, 0, 0, 4, 0, 3, 2, 2, 0, 0, 0, 0, 0},
        .vocab = TG_VOC_MODAL,
        .orch = {{TI_CONTRABASS, TI_VIOLIN, TI_STRINGS}, {TI_CELLO, TI_FLUTE, TI_HARP}}, .orch_n = 2,
        .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_RAVEL] = {
        /* Bolero's snare and its melody passed from instrument to instrument in one crescendo, over the
         * seventh and ninth chords of the G major concerto */
        .name = "RAVEL", .root = 0, .scale = TS_IONIAN,                              /* C major, 3/4 */
        .bpm = 72, .division = 4, .beats_per_bar = 3, .chord_beats = 3, .period_bars = 8,
        .melody_density = 0.45f, .melody_base = 62, .melody_range = 15,
        .profile = {0.06f, 0.70f, 0.88f, 0.96f}, .max_leap = 5,
        .appoggiatura = 0.15f, .arrival_bars = 1,
        .bass_mode = TG_BASS_OSTINATO, .pad_mode = TG_PAD_BLOCK, .pad_base = 52, .pad_tones = 4,
        .fifth_bias = 1.5f, .repeat = 0.6f,
        .motif = 0.85f, .motif_beats = 3, .motif_use = 0.9f, .var_invert = 0.0f, .var_alter = 0.2f,
        .swell = 0.3f,
        .perc_mode = TG_PERC_PATTERN,
        .w_major = {10, 5, 4, 7, 6, 5, 6, 5, 3, 4, 3, 2, 3, 3, 3, 2, 4, 4, 2, 0, 0, 2, 4, 3},
        .w_minor = {10, 5, 4, 6, 5, 4, 6, 4, 3, 3, 3, 2, 3, 2, 3, 2, 3, 2, 2, 0, 0, 2, 2, 2},
        .vocab = TG_VOC_JAZZ, .mel_add = (1u << 3) | (1u << 10),
        .bass_rhythm = (1u << 0) | (1u << 4) | (1u << 8), .bass_notes = {0, 2, 0}, .bass_len = 3,
        .evolve_periods = 8, .evolve_depth = 1.0f,
        .orch = {{TI_PIZZ, TI_FLUTE, TI_HARP}, {TI_PIZZ, TI_CLARINET, TI_PIZZ}, {TI_PIZZ, TI_BASSOON, TI_STRINGS},
                 {TI_PIZZ, TI_SAX, TI_STRINGS}}, .orch_n = 4, .orch_mode = TG_ORCH_CYCLE,
        .drum_pat = {{TG_GM_SNARE, 50, 0x5DD}},
    },

    /* ---- 7: modern ------------------------------------------------------------------------------------------
     * modern vocabulary (both modes): I i I4 bIII #IV VI V7 bII7 vii°7 i°7 IV4 II4 bVI bVII 5 V5 bII iv Vsus4
     *                                 I+ bVII+ I7b9 bIII7 bV7
     * jazz vocabulary:
     *  major: IM7 vi7 iii7 IVM7 ii7 ii9 V9 V13 V7b9 II9 III7b9 I7 VI7b9 iv6 bVII9 bII7 I6 I6/9 #ivø7 I+ bVII+
     *         bIIIM7 V7sus bVIM7
     *  minor: i6 VIM7 IIIM7 iv7 iiø7 iv6 V7b9 V7sus vii°7 V/V V/III V/iv bVII9 bII7 iv9 iM7 im9 bVI7 V/VI I+
     *         bVII+ I6 viø7 ii7
     * pop vocabulary:
     *  major: I Iadd9 vi vi7 IV IVadd9 IVM7 ii7 V Vsus4 V7sus iii bVII bVI Iadd2 I6/9 V/vi Isus2 IVsus2 I+ bVII+
     *         V/V ii bIII
     *  minor: i iadd9 VI VIM7 iv iv7 V v VII III iiø7 VIIsus2 Vsus4 IIIadd9 VIadd9 IV i7 isus2 v7 I+ bVII+ I
     *         bII VII7 */
    [TG_STYLE_BARTOK] = {
        /* Bulgarian 3+3+2, the acoustic scale, fourth chords, percussive chords on the beats, a narrow folk tune */
        .name = "BARTOK", .root = 9, .scale = TS_ACOUSTIC,                           /* on A, 3+3+2 */
        .bpm = 150, .division = 2, .beats_per_bar = 4, .chord_beats = 1, .period_bars = 8,
        .meter = {3, 3, 2},
        .melody_density = 0.5f, .melody_base = 64, .melody_range = 12,
        .profile = {0.25f, 0.70f, 0.88f, 0.96f}, .max_leap = 4,
        .arrival_bars = 1,
        .bass_mode = TG_BASS_OSTINATO, .pad_mode = TG_PAD_OOMPAH, .pad_base = 53, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.4f,
        .motif = 0.7f, .motif_beats = 3, .motif_use = 0.8f, .var_invert = 0.3f, .var_alter = 0.3f,
        .sforzando = 0.1f, .swell = 0.4f,
        .perc_mode = TG_PERC_PERIOD, .perc = {6, 3, 3, 2, 0, 0},
        .w_major = {4, 6, 6, 3, 2, 3, 3, 3, 2, 2, 5, 4, 3, 4, 5, 4, 3, 3, 3, 0, 0, 2, 2, 2},
        .w_minor = {4, 6, 6, 3, 2, 3, 3, 3, 2, 2, 5, 4, 3, 4, 5, 4, 3, 3, 3, 0, 0, 2, 2, 2},
        .vocab = TG_VOC_MODERN, .open = 0.55f, .color_scale = TS_OCT_HW, .color = 0.25f,
        .bass_notes = {0, 2, 0}, .bass_len = 3,
        .orch = {{TI_PIZZ, TI_VIOLIN, TI_PIANO}, {TI_CELLO, TI_CLARINET, TI_PIANO}, {TI_PIZZ, TI_VIOLIN, TI_MARIMBA}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_STRAVINSKY] = {
        /* the Rite: changing meters, octatonic polychords pounding, narrow folk cells, irregular accents */
        .name = "STRAVINSKY", .root = 4, .scale = TS_OCT_HW,                        /* on E, octatonic */
        .bpm = 126, .division = 2, .beats_per_bar = 3, .chord_beats = 1, .period_bars = 8,
        .meter = {3, 2, 3, 2, 2}, .meter_bars = (1u << 1) | (1u << 2) | (1u << 3),  /* 3/8 2/8 3/8 2/4 */
        .melody_density = 0.6f, .melody_base = 65, .melody_range = 9,
        .profile = {0.30f, 0.80f, 0.95f, 0.99f}, .max_leap = 3,
        .arrival_bars = 1,
        .bass_mode = TG_BASS_OSTINATO, .pad_mode = TG_PAD_OOMPAH, .pad_base = 52, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.5f,
        .motif = 0.8f, .motif_beats = 2, .motif_use = 0.9f, .var_invert = 0.2f, .var_alter = 0.3f,
        .sforzando = 0.3f, .swell = 0.5f,
        .perc_mode = TG_PERC_PERIOD, .perc = {8, 2, 7, 4, 0, 0},
        .w_major = {6, 3, 2, 6, 6, 6, 4, 3, 3, 3, 1, 1, 1, 1, 3, 2, 2, 1, 1, 0, 0, 5, 4, 4},
        .w_minor = {6, 3, 2, 6, 6, 6, 4, 3, 3, 3, 1, 1, 1, 1, 3, 2, 2, 1, 1, 0, 0, 5, 4, 4},
        .vocab = TG_VOC_MODERN, .open = 0.75f, .poly_semis = 6,
        .bass_notes = {0, 2, 1, 2}, .bass_len = 4,
        .orch = {{TI_CONTRABASS, TI_BASSOON, TI_BRASS}, {TI_TUBA, TI_HORN, TI_STRINGS}, {TI_CONTRABASS, TI_FLUTE, TI_STRINGS}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_PROKOFIEV] = {
        /* the Montagues: a dotted march, leaps over the octave, chords that slip to a wrong key and back */
        .name = "PROKOFIEV", .root = 4, .scale = TS_AEOLIAN,                         /* E minor */
        .bpm = 104, .division = 2, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.5f, .melody_base = 59, .melody_range = 24,
        .profile = {0.08f, 0.45f, 0.70f, 0.85f}, .max_leap = 7,
        .arrival_bars = 1,
        .bass_mode = TG_BASS_MARCH, .pad_mode = TG_PAD_REPEAT, .pad_base = 52, .pad_tones = 3,
        .fifth_bias = 1.5f, .repeat = 0.35f,
        .motif = 0.65f, .motif_beats = 4, .motif_use = 0.8f, .var_invert = 0.2f, .var_alter = 0.3f,
        .sforzando = 0.08f, .swell = 0.5f, .swing = 1.4f,                              /* dotted */
        .perc_mode = TG_PERC_PERIOD, .perc = {6, 5, 4, 5, 0, 0},
        .w_major = {10, 5, 3, 7, 5, 4, 7, 8, 3, 5, 3, 3, 3, 3, 3, 0, 0, 0, 2, 0, 0, 2, 3, 3},
        .w_minor = {10, 6, 4, 7, 3, 4, 7, 8, 4, 4, 3, 3, 3, 4, 2, 0, 0, 0, 2, 0, 0, 0, 3, 2},
        .sideslip = 0.3f, .staccato = 0.4f, .octave = 0.15f,
        .orch = {{TI_TUBA, TI_VIOLIN, TI_STRINGS}, {TI_CONTRABASS, TI_BASSOON, TI_STRINGS}, {TI_CONTRABASS, TI_CLARINET, TI_PIZZ}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_POULENC] = {
        /* music-hall cheek and sacred calm in turn: stride chords and sevenths, then choir and organ */
        .name = "POULENC", .root = 0, .scale = TS_IONIAN,                            /* C major */
        .bpm = 132, .division = 2, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.55f, .melody_base = 64, .melody_range = 15,
        .profile = {0.12f, 0.65f, 0.85f, 0.95f}, .max_leap = 5,
        .arrival_bars = 1,
        .bass_mode = TG_BASS_MARCH, .pad_mode = TG_PAD_OOMPAH, .pad_base = 55, .pad_tones = 4,
        .fifth_bias = 1.5f, .repeat = 0.35f,
        .motif = 0.6f, .motif_beats = 2, .motif_use = 0.75f, .var_invert = 0.1f, .var_alter = 0.35f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 6, 4, 7, 6, 4, 6, 4, 2, 5, 3, 3, 3, 3, 3, 2, 5, 3, 2, 0, 0, 3, 2, 3},
        .w_minor = {10, 6, 4, 6, 5, 4, 6, 4, 3, 4, 3, 3, 3, 3, 3, 3, 3, 3, 2, 0, 0, 3, 2, 2},
        .vocab = TG_VOC_JAZZ, .sideslip = 0.15f, .staccato = 0.3f,
        .alt = {.bpm = 0.5f, .density = 0.6f, .pad_mode = TG_PAD_BLOCK + 1, .bass_mode = TG_BASS_ROOT + 1}, .alt_every = 1,
        .orch = {{TI_PIANO, TI_CLARINET, TI_PIANO}, {TI_ORGAN, TI_CHOIR, TI_ORGAN}}, .orch_n = 2,
        .orch_mode = TG_ORCH_CONTRAST,
    },
    [TG_STYLE_GERSHWIN] = {
        /* blue notes, swing and syncopation, stride and walking basses, ninths and thirteenths, a 6/9 ending */
        .name = "GERSHWIN", .root = 10, .scale = TS_IONIAN,                          /* B-flat major */
        .bpm = 120, .division = 2, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.55f, .melody_base = 62, .melody_range = 17,
        .profile = {0.15f, 0.60f, 0.82f, 0.94f}, .max_leap = 5,
        .syncopation = 0.5f, .appoggiatura = 0.2f, .chromatic = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_WALKING, .pad_mode = TG_PAD_BLOCK, .pad_base = 55, .pad_tones = 4,
        .fifth_bias = 2.5f, .repeat = 0.4f,
        .motif = 0.65f, .motif_beats = 2, .motif_use = 0.8f, .var_invert = 0.1f, .var_alter = 0.35f,
        .swell = 0.3f, .swing = 1.0f,
        .perc_mode = TG_PERC_KIT,
        .w_major = {10, 6, 5, 6, 7, 5, 6, 5, 4, 5, 4, 3, 4, 3, 3, 3, 4, 4, 2, 0, 0, 2, 3, 2},
        .w_minor = {10, 5, 4, 6, 6, 4, 6, 4, 3, 4, 3, 3, 3, 3, 3, 3, 3, 3, 2, 0, 0, 3, 2, 2},
        .vocab = TG_VOC_JAZZ, .mel_add = (1u << 3) | (1u << 10) | (1u << 6),          /* the blue third, seventh, fifth */
        .orch = {{TI_CONTRABASS, TI_CLARINET, TI_PIANO}, {TI_CONTRABASS, TI_TRUMPET, TI_PIANO},
                 {TI_CONTRABASS, TI_PIANO, TI_PIANO}, {TI_CONTRABASS, TI_STRINGS, TI_PIANO}}, .orch_n = 4,
        .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_ORFF] = {
        /* O Fortuna: a driving ostinato, pounding choir chords, a narrow chant-like tune, drums, pp to ff */
        .name = "ORFF", .root = 2, .scale = TS_AEOLIAN,                              /* D minor, 3/4 */
        .bpm = 150, .division = 2, .beats_per_bar = 3, .chord_beats = 3, .period_bars = 8,
        .melody_density = 0.65f, .melody_base = 62, .melody_range = 9,
        .profile = {0.35f, 0.85f, 0.95f, 0.99f}, .max_leap = 3,
        .arrival_bars = 1,
        .bass_mode = TG_BASS_OSTINATO, .pad_mode = TG_PAD_OOMPAH, .pad_base = 55, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 1.0f,
        .motif = 0.85f, .motif_beats = 3, .motif_use = 0.9f, .var_invert = 0.0f, .var_alter = 0.2f,
        .swell = 0.6f,
        .perc_mode = TG_PERC_PERIOD, .perc = {9, 0, 8, 7, 0, 0},
        .harmony = TG_HARM_CYCLE, .cycle = {0, 0, 12, 1, 12, 0, 6, 0}, .cycle_len = 8,   /* i i VII VI VII i V i */
        .bass_notes = {0, -1}, .bass_len = 2, .echo = 0.4f,
        .evolve_periods = 4, .evolve_depth = 1.0f,
        .orch = {{TI_CONTRABASS, TI_CHOIR, TI_CHOIR}, {TI_TUBA, TI_CHOIR, TI_BRASS}}, .orch_n = 2,
        .orch_mode = TG_ORCH_ENERGY,
    },
    [TG_STYLE_RODRIGO] = {
        /* the Adagio of Aranjuez: strummed guitar, the Andalusian descent, an ornamented cor anglais */
        .name = "RODRIGO", .root = 11, .scale = TS_AEOLIAN,                          /* B minor */
        .bpm = 50, .division = 4, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 8,
        .melody_density = 0.35f, .melody_base = 62, .melody_range = 15,
        .profile = {0.06f, 0.75f, 0.90f, 0.97f}, .max_leap = 4,
        .appoggiatura = 0.2f, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_REPEAT, .pad_base = 50, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.5f,
        .motif = 0.6f, .motif_beats = 4, .motif_use = 0.7f, .var_invert = 0.05f, .var_alter = 0.3f,
        .swell = 0.4f, .rubato = 0.35f,
        .perc_mode = TG_PERC_OFF,
        .harmony = TG_HARM_CYCLE, .cycle = {0, 12, 1, 6}, .cycle_len = 4,             /* i VII VI V */
        .color_scale = TS_PHRYG_DOM, .color = 0.3f, .turn = 0.4f, .strum_ms = 14,
        .orch = {{TI_PIZZ, TI_OBOE, TI_GUITAR}, {TI_PIZZ, TI_GUITAR, TI_STRINGS}}, .orch_n = 2, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_COPLAND] = {
        /* Americana: open fifths and wide spacing, sus and add9 chords, leaps of fourths and fifths, fanfares */
        .name = "COPLAND", .root = 0, .scale = TS_IONIAN,                            /* C major */
        .bpm = 92, .division = 2, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 4,
        .melody_density = 0.4f, .melody_base = 62, .melody_range = 19,
        .profile = {0.06f, 0.45f, 0.75f, 0.92f}, .max_leap = 5,
        .syncopation = 0.25f, .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_BLOCK, .pad_base = 43, .pad_tones = 4,
        .fifth_bias = 1.2f, .repeat = 0.4f,
        .motif = 0.6f, .motif_beats = 4, .motif_use = 0.7f, .var_invert = 0.1f, .var_alter = 0.3f,
        .swell = 0.6f,
        .perc_mode = TG_PERC_PERIOD, .perc = {8, 0, 5, 4, 0, 0},
        .w_major = {8, 6, 3, 2, 7, 5, 3, 3, 6, 5, 3, 3, 4, 1, 4, 3, 0, 3, 3, 0, 0, 1, 4, 1},
        .w_minor = {9, 5, 6, 3, 5, 3, 6, 3, 4, 3, 2, 3, 4, 3, 3, 3, 3, 3, 2, 0, 0, 2, 1, 2},
        .vocab = TG_VOC_POP, .spread = true,
        .orch = {{TI_CONTRABASS, TI_TRUMPET, TI_BRASS}, {TI_CONTRABASS, TI_CLARINET, TI_STRINGS},
                 {TI_CONTRABASS, TI_VIOLIN, TI_STRINGS}}, .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_KHACHATURIAN] = {
        /* the Sabre Dance: furious repeated notes on the xylophone, chromatic oscillation, Armenian colour */
        .name = "KHACHATURIAN", .root = 7, .scale = TS_IONIAN,                       /* G, 2/4 */
        .bpm = 160, .division = 2, .beats_per_bar = 2, .chord_beats = 2, .period_bars = 8,
        .melody_density = 0.8f, .melody_base = 67, .melody_range = 12,
        .profile = {0.45f, 0.85f, 0.95f, 0.99f}, .max_leap = 3,
        .arrival_bars = 1,
        .bass_mode = TG_BASS_REPEAT, .pad_mode = TG_PAD_OOMPAH, .pad_base = 55, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.6f,
        .motif = 0.8f, .motif_beats = 2, .motif_use = 0.9f, .var_invert = 0.1f, .var_alter = 0.2f,
        .sforzando = 0.05f, .swell = 0.4f,
        .perc_mode = TG_PERC_PERIOD, .perc = {5, 5, 4, 4, 0, 2},
        .w_major = {10, 4, 3, 6, 4, 3, 9, 8, 3, 4, 2, 2, 2, 3, 2, 0, 0, 0, 2, 0, 0, 0, 2, 3},
        .w_minor = {10, 5, 4, 6, 3, 3, 9, 8, 4, 3, 2, 2, 3, 3, 2, 0, 0, 0, 2, 0, 0, 0, 3, 2},
        .mel_add = (1u << 1) | (1u << 6), .color_scale = TS_HUNG_MINOR, .color = 0.25f,
        .orch = {{TI_TUBA, TI_MARIMBA, TI_BRASS}, {TI_CONTRABASS, TI_MARIMBA, TI_STRINGS}, {TI_TUBA, TI_TRUMPET, TI_BRASS}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_KORNGOLD] = {
        /* swashbuckling Hollywood from Vienna: heroic horn leaps, chromatic added-note harmony, tremolo strings */
        .name = "KORNGOLD", .root = 2, .scale = TS_IONIAN,                           /* D major */
        .bpm = 112, .division = 4, .beats_per_bar = 4, .chord_beats = 2, .period_bars = 4,
        .melody_density = 0.5f, .melody_base = 60, .melody_range = 22,
        .profile = {0.05f, 0.50f, 0.75f, 0.90f}, .max_leap = 7,
        .appoggiatura = 0.25f, .chromatic = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_MARCH, .pad_mode = TG_PAD_TREMOLO, .pad_base = 52, .pad_tones = 3,
        .fifth_bias = 1.3f, .repeat = 0.3f,
        .motif = 0.65f, .motif_beats = 4, .motif_use = 0.8f, .var_invert = 0.1f, .var_alter = 0.3f,
        .swell = 0.8f,
        .perc_mode = TG_PERC_PERIOD, .perc = {7, 3, 4, 6, 2, 0},
        .w_major = {10, 5, 3, 6, 5, 5, 7, 8, 4, 6, 4, 3, 3, 4, 5, 5, 4, 4, 2, 0, 0, 3, 2, 2},
        .w_minor = {10, 6, 4, 6, 3, 5, 7, 8, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 2, 3, 3},
        .vocab = TG_VOC_ROMANTIC, .open = 0.2f, .modulate = 0.2f, .octave = 0.1f,
        .orch = {{TI_TUBA, TI_HORN, TI_STRINGS}, {TI_CONTRABASS, TI_VIOLIN, TI_HARP}, {TI_TUBA, TI_TRUMPET, TI_BRASS}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_DURUFLE] = {
        /* the Requiem: Gregorian chant over modal ninth chords, the meter following the words, organ */
        .name = "DURUFLE", .root = 2, .scale = TS_DORIAN,                            /* D Dorian */
        .bpm = 66, .division = 2, .beats_per_bar = 4, .chord_beats = 1, .period_bars = 6,
        .meter = {4, 4, 6, 4}, .meter_bars = (1u << 2) | (1u << 3),                     /* 4/4 3/4 2/4 */
        .melody_density = 0.55f, .melody_base = 62, .melody_range = 14,
        .profile = {0.08f, 0.75f, 0.90f, 0.97f}, .max_leap = 4,
        .arrival_bars = 1,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_BLOCK, .pad_base = 50, .pad_tones = 4,
        .fifth_bias = 1.0f, .repeat = 0.3f, .plagal = true,
        .motif = 0.4f, .motif_beats = 4, .motif_use = 0.5f, .var_invert = 0.2f, .var_alter = 0.4f,
        .swell = 0.3f, .rubato = 0.2f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {8, 6, 3, 2, 6, 4, 4, 5, 6, 3, 4, 3, 3, 1, 2, 4, 0, 3, 3, 0, 0, 1, 4, 1},
        .w_minor = {8, 6, 0, 0, 0, 0, 2, 5, 4, 3, 2, 2, 2, 3, 0, 5, 6, 4, 5, 0, 0, 2, 0, 1},
        .vocab = TG_VOC_POP, .mel_mode = TG_MEL_CHANT,
        .orch = {{TI_ORGAN, TI_CHOIR, TI_ORGAN}}, .orch_n = 1,
    },
    [TG_STYLE_SHOSTAKOVICH] = {
        /* the second Waltz: oom-pah-pah under a saxophone, chromatic and dark, Phrygian shadows, sarcasm */
        .name = "SHOSTAKOVICH", .root = 0, .scale = TS_AEOLIAN,                      /* C minor, 3/4 */
        .bpm = 152, .division = 2, .beats_per_bar = 3, .chord_beats = 3, .period_bars = 8,
        .melody_density = 0.4f, .melody_base = 60, .melody_range = 15,
        .profile = {0.06f, 0.65f, 0.85f, 0.95f}, .max_leap = 5,
        .appoggiatura = 0.3f, .chromatic = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_BAR, .pad_mode = TG_PAD_OOMPAH, .pad_base = 55, .pad_tones = 3,
        .fifth_bias = 1.3f, .repeat = 0.5f,
        .motif = 0.65f, .motif_beats = 3, .motif_use = 0.8f, .var_invert = 0.1f, .var_alter = 0.3f,
        .sforzando = 0.08f, .swell = 0.4f,
        .perc_mode = TG_PERC_PERIOD, .perc = {4, 4, 3, 4, 0, 0},
        .w_major = {10, 5, 3, 7, 4, 4, 8, 8, 3, 4, 3, 2, 2, 3, 3, 0, 0, 0, 2, 0, 0, 0, 3, 3},
        .w_minor = {10, 7, 5, 7, 3, 5, 7, 8, 4, 4, 3, 3, 3, 4, 3, 2, 0, 0, 2, 0, 0, 0, 3, 2},
        .mel_add = (1u << 1) | (1u << 6), .color_scale = TS_PHRYGIAN, .color = 0.25f,
        .orch = {{TI_TUBA, TI_SAX, TI_PIZZ}, {TI_CONTRABASS, TI_TRUMPET, TI_PIZZ}, {TI_CONTRABASS, TI_STRINGS, TI_PIZZ}},
        .orch_n = 3, .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_MESSIAEN] = {
        /* modes of limited transposition, added-value rhythm, an infinitely slow praise and birdsong in turn */
        .name = "MESSIAEN", .root = 6, .scale = TS_IONIAN,                           /* F-sharp major */
        .bpm = 46, .division = 2, .beats_per_bar = 5, .chord_beats = 1, .period_bars = 4,
        .meter = {2, 3, 2, 3, 3},                                                       /* added values */
        .melody_density = 0.25f, .melody_base = 66, .melody_range = 17,
        .profile = {0.04f, 0.65f, 0.85f, 0.95f}, .max_leap = 5,
        .arrival_bars = 1,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_BLOCK, .pad_base = 50, .pad_tones = 4,
        .fifth_bias = 0.8f, .repeat = 0.3f,
        .motif = 0.5f, .motif_beats = 5, .motif_use = 0.5f, .var_invert = 0.3f, .var_alter = 0.4f,
        .swell = 0.5f, .rubato = 0.2f,
        .perc_mode = TG_PERC_PERIOD, .perc = {0, 0, 0, 5, 4, 0},
        .w_major = {5, 3, 3, 4, 3, 4, 4, 5, 6, 3, 4, 2, 4, 3, 4, 4, 8, 8, 3, 0, 0, 3, 3, 4},
        .w_minor = {2, 5, 4, 3, 3, 3, 6, 3, 4, 3, 3, 3, 3, 4, 3, 2, 3, 4, 3, 0, 0, 8, 2, 2},
        .vocab = TG_VOC_JAZZ, .open = 0.5f, .color_scale = TS_OCT_HW, .color = 0.35f, .mel_min_ticks = 6,
        .alt = {.mel_mode = TG_MEL_BIRDSONG + 1, .bpm = 2.2f}, .alt_every = 1,
        .orch = {{TI_ORGAN, TI_SYNTH, TI_BELLS}, {TI_PIANO, TI_FLUTE, TI_PIANO}}, .orch_n = 2, .orch_mode = TG_ORCH_CONTRAST,
    },
    [TG_STYLE_BRITTEN] = {
        /* Dawn: a high thin line far above a low brass chorale, harp between, Lydian light, the sea */
        .name = "BRITTEN", .root = 9, .scale = TS_IONIAN,                            /* A major */
        .bpm = 60, .division = 4, .beats_per_bar = 4, .chord_beats = 4, .period_bars = 4,
        .melody_density = 0.35f, .melody_base = 76, .melody_range = 12,
        .profile = {0.06f, 0.65f, 0.85f, 0.95f}, .max_leap = 5,
        .arrival_bars = 2,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_BLOCK, .pad_base = 41, .pad_tones = 3,
        .fifth_bias = 1.0f, .repeat = 0.4f,
        .motif = 0.6f, .motif_beats = 4, .motif_use = 0.7f, .var_invert = 0.15f, .var_alter = 0.3f,
        .swell = 0.6f,
        .perc_mode = TG_PERC_PERIOD, .perc = {5, 0, 0, 4, 0, 0},
        .w_major = {10, 5, 4, 6, 4, 3, 5, 5, 2, 3, 3, 3, 2, 4, 5, 3, 2, 2, 4, 0, 0, 2, 2, 3},
        .w_minor = {10, 6, 5, 6, 3, 4, 5, 5, 3, 3, 3, 3, 3, 3, 3, 2, 0, 0, 3, 0, 0, 2, 2, 2},
        .color_scale = TS_LYDIAN, .color = 0.35f, .mel_min_ticks = 3,
        .alt = {.pad_mode = TG_PAD_ARPEGGIO + 1}, .alt_every = 2,
        .orch = {{TI_TUBA, TI_VIOLIN, TI_BRASS}, {TI_CONTRABASS, TI_FLUTE, TI_HARP}}, .orch_n = 2,
        .orch_mode = TG_ORCH_CONTRAST,
    },
    [TG_STYLE_BARBER] = {
        /* the Adagio for Strings: 3/2 and 4/2 in turn, a stepwise line in long notes, suspensions, one arc */
        .name = "BARBER", .root = 10, .scale = TS_AEOLIAN,                           /* B-flat minor */
        .bpm = 50, .division = 2, .beats_per_bar = 4, .chord_beats = 1, .period_bars = 8,
        .meter = {6, 8}, .meter_bars = 1u << 1,                                         /* 3/2, 4/2 */
        .melody_density = 0.3f, .melody_base = 65, .melody_range = 15,
        .profile = {0.03f, 0.90f, 0.97f, 0.99f}, .max_leap = 3,
        .suspension = 0.5f, .arrival_bars = 3,
        .bass_mode = TG_BASS_ROOT, .pad_mode = TG_PAD_BLOCK, .pad_base = 53, .pad_tones = 3,
        .fifth_bias = 1.2f, .repeat = 0.3f,
        .motif = 0.6f, .motif_beats = 4, .motif_use = 0.7f, .var_invert = 0.05f, .var_alter = 0.3f,
        .swell = 1.0f, .rubato = 0.2f,
        .perc_mode = TG_PERC_OFF,
        .w_major = {10, 6, 3, 7, 5, 6, 6, 8, 3, 4, 3, 3, 3, 4, 4, 2, 0, 0, 2, 0, 0, 0, 2, 0},
        .w_minor = {10, 6, 4, 7, 3, 6, 6, 8, 3, 4, 3, 3, 3, 4, 4, 2, 0, 0, 2, 0, 0, 0, 2, 0},
        .open = 0.4f, .mel_min_ticks = 4,
        .evolve_periods = 6, .evolve_depth = 1.0f,
        .orch = {{TI_CONTRABASS, TI_VIOLIN, TI_STRINGS}, {TI_CONTRABASS, TI_CELLO, TI_STRINGS}}, .orch_n = 2,
        .orch_mode = TG_ORCH_RANDOM,
    },
    [TG_STYLE_PIAZZOLLA] = {
        /* nuevo tango: 3+3+2, a bass falling by semitones, percussive chords and golpes, bandoneon */
        .name = "PIAZZOLLA", .root = 9, .scale = TS_AEOLIAN,                         /* A minor, 3+3+2 */
        .bpm = 116, .division = 2, .beats_per_bar = 4, .chord_beats = 1, .period_bars = 6,
        .meter = {3, 3, 2},
        .melody_density = 0.5f, .melody_base = 64, .melody_range = 15,
        .profile = {0.10f, 0.70f, 0.88f, 0.96f}, .max_leap = 5,
        .syncopation = 0.4f, .appoggiatura = 0.3f, .chromatic = true, .arrival_bars = 1,
        .bass_mode = TG_BASS_LINE, .pad_mode = TG_PAD_OOMPAH, .pad_base = 55, .pad_tones = 4,
        .fifth_bias = 1.0f, .repeat = 0.5f,
        .motif = 0.7f, .motif_beats = 3, .motif_use = 0.8f, .var_invert = 0.1f, .var_alter = 0.3f,
        .sforzando = 0.08f, .swell = 0.5f,
        .perc_mode = TG_PERC_PATTERN,
        .vocab = TG_VOC_JAZZ, .harmony = TG_HARM_CYCLE, .cycle = {0, 6, 16, 22, 17, 6}, .cycle_len = 6,
        .bass_notes = {0, -1, -2, -3, -4, -5}, .bass_len = 6,                              /* A G# G F# F E */
        .orch = {{TI_CONTRABASS, TI_BANDONEON, TI_PIANO}, {TI_CONTRABASS, TI_VIOLIN, TI_PIANO}}, .orch_n = 2,
        .orch_mode = TG_ORCH_RANDOM,
        .drum_pat = {{TG_GM_SIDE_STICK, 60, 0x49}, {TG_GM_BASS_DRUM, 66, 0x01}},
    },
};

static const char *const family_names[TI_COUNT] = {
    "DEFAULT", "PIANO", "HARPSICH", "ORGAN", "STRINGS", "VIOLIN", "CELLO", "CONTRABS", "PIZZ",
    "FLUTE", "OBOE", "CLARINET", "BASSOON", "HORN", "TRUMPET", "BRASS", "TUBA", "HARP", "GUITAR",
    "CHOIR", "VOICE", "BELLS", "CELESTA", "MARIMBA", "BANDONEO", "SAX", "SYNTH", "PAD", "WHISTLE",
    "DRONE", "E.PIANO",
};

const char *tg_family_name(int f) { return f >= 0 && f < TI_COUNT ? family_names[f] : "?"; }

const tg_style *tg_style_get(int id) { return id >= 0 && id < TG_STYLE_COUNT ? &styles[id] : 0; }
