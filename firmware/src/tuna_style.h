/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA composer signatures: the habits a style gives the harmonizer.
 *
 * A signature sets the harmony (which chords, how they move, how phrases cadence), the
 * accompaniment (bass and the third part), the melody (interval profile, motifs, ornaments,
 * density) and the tempo, meter, dynamics and rubato. It evokes a style; it does not quote works.
 *
 * Harmony is planned one period at a time (question + answer, see tuna_gen.h). The planner fills
 * the period's chord slots with chords from a functional vocabulary, weighted per style:
 *   slot 0             the tonic (the answer of the previous period lands here)
 *   slot N/2-2, N/2-1  pre-dominant, dominant: the half cadence ends the question
 *   slot N-2, N-1      pre-dominant, dominant (or pre-dominant twice: a plagal ending)
 *   the rest           a walk through tonic -> pre-dominant -> dominant -> tonic, with applied
 *                      dominants resolving to their targets and, per style, a pull toward root
 *                      motion by falling fifths.
 * A style may colour some periods with the whole-tone scale (augmented chords) before the
 * cadence brings the music home. */
#ifndef TUNA_STYLE_H
#define TUNA_STYLE_H

#include <stdbool.h>
#include <stdint.h>

enum {
    TG_BASS_MARBLES,   /* T1 decides (no style) */
    TG_BASS_ROOT,      /* the root on each chord change, held */
    TG_BASS_WALKING,   /* a note every beat, stepping into the next root */
    TG_BASS_BAR,       /* the root on every downbeat */
    TG_BASS_MARCH,     /* root on beat 1, fifth on beat 3 */
    TG_BASS_PEDAL,     /* the tonic held under the period (a drone), with its fifth */
    TG_BASS_DRONE,     /* the tonic held without end (an ison), struck again only at period starts */
    TG_BASS_OSTINATO,  /* a rhythm (bass_rhythm, ticks of the bar) on chord tones (bass_notes, ladder steps) */
    TG_BASS_LINE,      /* a fixed line per chord step (bass_notes: semitones from the tonic), e.g. a lament */
    TG_BASS_REPEAT,    /* the root in repeated eighths (Vivaldi's drum bass) */
    TG_BASS_TOLL,      /* the root struck low every two bars and left to ring (bells) */
    TG_BASS_BEAT,      /* every beat, root and fifth in turn (pizzicato) */
    TG_BASS_NONE,      /* no bass */
};
enum {
    TG_PAD_BLOCK,      /* held chords, voice-led */
    TG_PAD_ALBERTI,    /* low, high, middle, high */
    TG_PAD_COUNTER,    /* a counter-melody */
    TG_PAD_BROKEN,     /* wide broken chords over two octaves */
    TG_PAD_AFTERBEAT,  /* the chord after the downbeat bass (oom-pah, Gymnopedie) */
    TG_PAD_PARALLEL,   /* chords planing under each melody note */
    TG_PAD_DOUBLE,     /* the melody doubled a third or sixth below (or pad_interval semitones) */
    TG_PAD_REPEAT,     /* the chord repeated in eighths, driving */
    TG_PAD_ARPEGGIO,   /* rising and falling arpeggios in sixteenths */
    TG_PAD_OOMPAH,     /* the chord on the beats the bass leaves: 2 and 3 in 3/4, 2 and 4 in 4/4 */
    TG_PAD_TREMOLO,    /* the chord re-struck on every tick */
    TG_PAD_ORGANUM,    /* the melody in parallel, pad_interval semitones below */
    TG_PAD_TINTI,      /* tintinnabuli: the tonic-triad note nearest below (or above) the melody */
    TG_PAD_CANON,      /* the melody again, canon_ticks later, canon_steps scale steps away */
    TG_PAD_FIGURE,     /* a figure over the chord (pad_figure, ladder steps), one note per tick */
    TG_PAD_ADDITIVE,   /* the figure growing and shrinking by one note (Glass) */
    TG_PAD_PHASE,      /* the melody's pattern, drifting a step ahead every phase_reps loops (Reich) */
    TG_PAD_NONE,       /* no third part */
};
enum {
    TG_MEL_PHRASE,     /* periods of question and answer (the default) */
    TG_MEL_CHANT,      /* the same, unmetered: no accents, no bar lines */
    TG_MEL_TINTI,      /* tintinnabuli M-voice: scale steps toward the tonic, one more each phrase */
    TG_MEL_PATTERN,    /* the style's pattern, looped (pattern: scale steps from the tonic, or ladder steps) */
    TG_MEL_BIRDSONG,   /* bursts of quick high notes, leaps and repeated notes, then silence */
};
enum { TG_HARM_PLANNED, TG_HARM_CYCLE, TG_HARM_DRONE };
enum { TG_VOC_CLASSIC, TG_VOC_ROMANTIC, TG_VOC_JAZZ, TG_VOC_MODAL, TG_VOC_MODERN, TG_VOC_POP, TG_VOC_COUNT };
/* instrument families: the host maps them to its sounds (Felucca presets) */
enum {
    TI_DEFAULT, TI_PIANO, TI_HARPSICHORD, TI_ORGAN, TI_STRINGS, TI_VIOLIN, TI_CELLO, TI_CONTRABASS, TI_PIZZ,
    TI_FLUTE, TI_OBOE, TI_CLARINET, TI_BASSOON, TI_HORN, TI_TRUMPET, TI_BRASS, TI_TUBA, TI_HARP, TI_GUITAR,
    TI_CHOIR, TI_VOICE, TI_BELLS, TI_CELESTA, TI_MARIMBA, TI_BANDONEON, TI_SAX, TI_SYNTH, TI_PAD, TI_WHISTLE,
    TI_DRONE, TI_EPIANO, TI_COUNT
};
enum { TG_ORCH_FIXED, TG_ORCH_CYCLE, TG_ORCH_RANDOM, TG_ORCH_ENERGY, TG_ORCH_CONTRAST };
enum { TG_MOD_RELATED, TG_MOD_UP_SEMITONE, TG_MOD_UP_TONE, TG_MOD_MEDIANT };
#define TG_REST (-128)
/* General MIDI percussion notes used by the drum part */
#define TG_GM_BASS_DRUM 35
#define TG_GM_KICK 36
#define TG_GM_SIDE_STICK 37
#define TG_GM_SNARE 38
#define TG_GM_HAT_CLOSED 42
#define TG_GM_HAT_OPEN 46
#define TG_GM_CRASH 49
#define TG_GM_TAMBOURINE 54
#define TG_GM_TRIANGLE 81
#define TG_GM_HAND_CLAP 39
#define TG_GM_LOW_TOM 45
#define TG_GM_RIDE 51
#define TG_GM_CASTANETS 85

enum { TG_PERC_OFF, TG_PERC_PERIOD, TG_PERC_KIT, TG_PERC_PATTERN };
/* period percussion instruments, the order of tg_style.perc */
enum { TG_PI_TIMPANI, TG_PI_SNARE, TG_PI_BASS_DRUM, TG_PI_CYMBAL, TG_PI_TRIANGLE, TG_PI_TAMBOURINE, TG_PI_COUNT };
enum {
    TG_STYLE_BACH, TG_STYLE_MOZART, TG_STYLE_BEETHOVEN, TG_STYLE_BRAHMS, TG_STYLE_MAHLER,
    TG_STYLE_CHOPIN, TG_STYLE_SATIE, TG_STYLE_DEBUSSY, TG_STYLE_DVORAK, TG_STYLE_SAINT_SAENS,
    /* 1: medieval and renaissance */
    TG_STYLE_HILDEGARD, TG_STYLE_TALLIS, TG_STYLE_PALESTRINA, TG_STYLE_ALLEGRI, TG_STYLE_MONTEVERDI, TG_STYLE_SWEELINCK,
    /* 2: baroque (Bach above) */
    TG_STYLE_BUXTEHUDE, TG_STYLE_PACHELBEL, TG_STYLE_PURCELL, TG_STYLE_VIVALDI, TG_STYLE_ALBINONI, TG_STYLE_HANDEL,
    TG_STYLE_SCARLATTI, TG_STYLE_RAMEAU, TG_STYLE_PERGOLESI,
    /* 3: classical (Mozart, Beethoven above) */
    TG_STYLE_GLUCK, TG_STYLE_HAYDN,
    /* 4: early romantic (Chopin above) */
    TG_STYLE_SCHUBERT, TG_STYLE_ROSSINI, TG_STYLE_BERLIOZ, TG_STYLE_FELIX_MENDELSSOHN, TG_STYLE_FANNY_MENDELSSOHN,
    TG_STYLE_ROBERT_SCHUMANN, TG_STYLE_CLARA_SCHUMANN, TG_STYLE_LISZT,
    /* 5: romantic (Brahms, Saint-Saens, Dvorak above) */
    TG_STYLE_VERDI, TG_STYLE_WAGNER, TG_STYLE_GOUNOD, TG_STYLE_JOHANN_STRAUSS, TG_STYLE_BRUCKNER, TG_STYLE_BIZET,
    TG_STYLE_FRANCK, TG_STYLE_BRUCH, TG_STYLE_MUSSORGSKY, TG_STYLE_TCHAIKOVSKY, TG_STYLE_SMETANA, TG_STYLE_GRIEG,
    TG_STYLE_MASSENET, TG_STYLE_RIMSKY_KORSAKOV,
    /* 6: late romantic and impressionism (Mahler, Debussy, Satie above) */
    TG_STYLE_WIDOR, TG_STYLE_FAURE, TG_STYLE_ELGAR, TG_STYLE_PUCCINI, TG_STYLE_RICHARD_STRAUSS, TG_STYLE_SIBELIUS,
    TG_STYLE_RACHMANINOFF, TG_STYLE_HOLST, TG_STYLE_VAUGHAN_WILLIAMS, TG_STYLE_RAVEL,
    /* 7: modern */
    TG_STYLE_BARTOK, TG_STYLE_STRAVINSKY, TG_STYLE_PROKOFIEV, TG_STYLE_POULENC, TG_STYLE_GERSHWIN, TG_STYLE_ORFF,
    TG_STYLE_RODRIGO, TG_STYLE_COPLAND, TG_STYLE_KHACHATURIAN, TG_STYLE_KORNGOLD, TG_STYLE_DURUFLE,
    TG_STYLE_SHOSTAKOVICH, TG_STYLE_MESSIAEN, TG_STYLE_BRITTEN, TG_STYLE_BARBER, TG_STYLE_PIAZZOLLA,
    /* 8: minimalism and holy minimalism */
    TG_STYLE_TEN_HOLT, TG_STYLE_GLASS, TG_STYLE_REICH, TG_STYLE_PART, TG_STYLE_GORECKI, TG_STYLE_TAVENER,
    TG_STYLE_VASKS, TG_STYLE_WHITACRE, TG_STYLE_RUTTER,
    /* 9: film and musical */
    TG_STYLE_MORRICONE, TG_STYLE_JOHN_WILLIAMS, TG_STYLE_HOWARD_SHORE, TG_STYLE_PREISNER, TG_STYLE_LLOYD_WEBBER,
    TG_STYLE_COUNT
};

enum { TF_T, TF_S, TF_D, TF_SEC };          /* tonic, pre-dominant, dominant, applied chord */

#define TG_VOCAB 24
#define TG_PLAN_MAX 16
#define TG_VOC_AUG_I 19                     /* vocabulary indices of the whole-tone colour chords */
#define TG_VOC_AUG_BVII 20

typedef struct {
    int8_t semis;      /* chord root, semitones above the tonic (TQ_DIATONIC: the scale degree) */
    uint8_t quality;   /* TQ_* */
    uint8_t func;      /* TF_* */
    int8_t target;     /* TF_SEC: the vocabulary index it resolves to */
    const char *name;  /* roman numeral, for logs */
} tg_chord_def;

typedef struct {
    const char *name;
    int root, scale;                 /* default key */
    float bpm;
    int division, beats_per_bar, chord_beats, period_bars;
    uint8_t meter[8];                /* beat groups in ticks (0-terminated), e.g. {3, 3, 2}; chord_beats then
                                        counts whole cycles of the meter */
    uint8_t meter_bars;              /* bit i: group i starts a new bar (changing meters) */
    float swing, lilt;               /* off-beat eighths late; the waltz's second beat early */
    float melody_density;
    int melody_base, melody_range;
    float profile[4];                /* melody interval thresholds: repeat < step < third < fourth */
    int max_leap;                    /* scale steps of the widest leap (4 = a fifth) */
    uint16_t melody_mask;            /* restrict the melody to these pitch classes (0 = no) */
    float syncopation;               /* 0..1: shifts melody notes from beats to off-beats */
    float appoggiatura, suspension;  /* chances on a downbeat */
    bool chromatic;                  /* appoggiaturas lean from a semitone above */
    bool trill;                      /* trill on the note before the final tonic */
    int arrival_bars;                /* how long the final tonic is held */
    int bass_mode, pad_mode, pad_base, pad_tones;
    float fifth_bias;                /* weight multiplier for root motion down a fifth */
    float repeat;                    /* chance a period repeats the previous period's harmony */
    bool plagal;                     /* end periods IV - I instead of V - I */
    float whole_tone;                /* chance a period is coloured whole-tone */
    float motif;                     /* Marbles deja vu for melody and rhythm */
    int motif_beats;                 /* length of the motif */
    float motif_use;                 /* chance a later bar develops the motif instead of wandering */
    float var_invert, var_alter;     /* development: chance of inversion, of a changed ending */
    bool hemiola;                    /* 3/4: regroup the bars before a cadence in twos */
    float sforzando;                 /* chance of a sudden accent on a weak beat */
    float swell;                     /* 0..1: crescendo through the period into the arrival */
    float rubato;                    /* 0..1: slow into cadences, a tempo after */
    int perc_mode;                   /* TG_PERC_*: the default percussion */
    uint8_t perc[TG_PI_COUNT];       /* period percussion, 0..10 per instrument */
    uint8_t w_major[TG_VOCAB];       /* chord weights, major keys */
    uint8_t w_minor[TG_VOCAB];       /* chord weights, minor keys */

    /* ---- the composer expansion: all zero gives the behaviour above ---- */
    /* harmony */
    uint8_t vocab;                   /* TG_VOC_*: the chord vocabulary the weights refer to */
    uint8_t harmony;                 /* TG_HARM_*: planned periods, a fixed cycle, or a drone */
    int8_t cycle[8], cycle2[8];      /* TG_HARM_CYCLE: vocabulary indices; a second cycle for contrast */
    uint8_t cycle_len, cycle2_len, cycle_switch;   /* cycle_switch: periods per cycle before the other */
    float open;                      /* chance a period ends away from the tonic (deceptive, open, on V) */
    uint8_t color_scale;             /* TS_*: the colour of coloured periods (0: whole-tone, as whole_tone) */
    float color;                     /* chance a period takes the colour scale */
    float mixture;                   /* chance a period turns to the parallel mode */
    float sideslip;                  /* chance a chord slips a semitone (and the next one back) */
    float modulate;                  /* chance a period moves to a related key; home again later */
    uint8_t mod_kind;                /* TG_MOD_*: what the evolution timer modulates to */
    uint8_t tuning;                  /* TT_TUNE_* */
    /* melody */
    uint8_t mel_mode;                /* TG_MEL_* */
    int8_t pattern[16];              /* TG_MEL_PATTERN (and PHASE): steps, TG_REST for a rest */
    uint8_t pattern_len;
    bool pattern_chord;              /* pattern steps are chord-ladder steps, not scale steps */
    uint8_t mel_min_ticks;           /* the shortest melody note (long-note styles) */
    float direction;                 /* -1..1: the melody's tendency to fall or rise */
    float staccato;                  /* 0..1: notes shorter than their time */
    uint16_t mel_add;                /* pitch classes (from the tonic) the melody may add: blue notes */
    float turn;                      /* chance of a turn (upper, main, lower, main) on a downbeat */
    float octave;                    /* chance a free leap is an octave */
    /* bass and third part */
    uint16_t bass_rhythm;            /* TG_BASS_OSTINATO: ticks of the bar that sound */
    int8_t bass_notes[8];            /* OSTINATO: chord-ladder steps in turn; LINE: semitones per chord step */
    uint8_t bass_len;
    int8_t pad_figure[16];           /* TG_PAD_FIGURE / ADDITIVE: chord-ladder steps */
    uint8_t figure_len;
    int8_t pad_interval;             /* ORGANUM and DOUBLE: semitones below the melody (0: the default) */
    uint8_t canon_ticks;             /* CANON: delay */
    int8_t canon_steps;              /* CANON: scale steps away (-7: an octave below in a 7-note scale) */
    uint8_t phase_reps;              /* PHASE: loops before the next step of drift */
    uint8_t poly_semis;              /* BLOCK: a second major triad this far above the chord's root */
    bool spread;                     /* BLOCK: open voicing over two octaves */
    uint8_t strum_ms;                /* chords spread from low to high, ms per string */
    /* form and dynamics */
    float gp;                        /* chance of a general pause after an arrival */
    float echo;                      /* chance a period is played softer, as an echo */
    struct {                         /* contrast: every alt_every periods the music turns to this */
        float bpm, density;          /* factors (0: unchanged) */
        uint8_t mel_mode, bass_mode, pad_mode;   /* +1 (0: unchanged) */
        uint8_t harmony;             /* +1 (0: unchanged) */
    } alt;
    uint8_t alt_every;
    uint8_t evolve_periods;          /* the evolution timer on by default (Bolero, Pachelbel) */
    float evolve_depth;
    bool evolve_modulate;
    /* instruments */
    uint8_t orch[4][3];              /* sets of TI_* for bass, melody, third part */
    uint8_t orch_n, orch_mode;       /* TG_ORCH_*: how the sets change between periods */
    /* percussion pattern (TG_PERC_PATTERN): up to three instruments, ticks of the bar */
    struct { uint8_t note, vel; uint16_t ticks; } drum_pat[3];
} tg_style;

const tg_style *tg_style_get(int id);
int tg_style_index(const tg_style *st);           /* TG_STYLE_* of a library style, or -1 */
const char *tg_family_name(int family);       /* TI_*: a short name for the screen */
const tg_chord_def *tg_vocab(bool minor);               /* the classic vocabulary */
const tg_chord_def *tg_vocab_of(int vocab, bool minor);

#endif
