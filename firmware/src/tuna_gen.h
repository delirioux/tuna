/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA generator: turns the Marbles port (tuna_marbles.c) into music for three synth parts.
 *
 *   part 0  bass    T1 gate + X1     part 1  melody  T2 (master clock) + X2
 *   part 2  pad     T3 gate + X3     Y       slow random value -> velocity
 *
 * Two ways to read Marbles:
 *   raw         Marbles' quantized voltages are the notes (the module's own behaviour).
 *   harmonized  Marbles still makes every random choice, but a musician's rules turn them into
 *               notes (the default):
 *     melody  X2 picks the next *interval*, mostly steps; large leaps are followed by a step back;
 *             downbeats land on chord tones; a density control and a deja-vu rhythm decide which
 *             ticks play; notes are held until the next one (legato). Phrases form periods: a
 *             question that pauses on the fifth or third halfway, and an answer whose last bar
 *             steers home, whose last beat is a neighbour of the tonic, and which lands on the
 *             tonic exactly when the progression returns to its tonic chord.
 *     bass    the chord root on the downbeat and on each chord change, then root, fifth, octave
 *             or third chosen by X1, and a step into the next chord's root just before it.
 *     pad     the chord itself, voiced close to the previous chord (smallest total movement),
 *             struck on each chord change and re-struck on T3.
 *     all     accents by beat position on top of Y's velocity.
 * A composer signature (tuna_style.h, tg_apply_style) replaces the fixed progression with
 * planned functional harmony and can swap the bass (root, walking) and the third part (Alberti
 * figures, a counter-melody), and add ornaments (appoggiatura, suspension, trill).
 * Every Marbles parameter stays reachable through gen->t, gen->x and gen->y. */
#ifndef TUNA_GEN_H
#define TUNA_GEN_H

#include <stdbool.h>
#include <stdint.h>
#include "tuna_marbles.h"
#include "tuna_theory.h"
#include "tuna_style.h"

#define TG_PARTS 3
#define TG_PAD_VOICES 8           /* a chord of four, and a second triad above it (polychords) */

enum { TG_PART_BASS, TG_PART_MELODY, TG_PART_PAD, TG_PART_DRUMS, TG_PART_TIMPANI };
#define TG_EVENT_PARTS 5          /* the three voices, plus drums (GM note numbers) and pitched timpani */

/* General MIDI percussion notes: tuna_style.h */

typedef struct {
    uint8_t part, note, velocity;
    bool on;
    uint16_t offset;              /* sample offset in the processed block */
    uint8_t type;                 /* TG_EV_NOTE, or TG_EV_PARAM: a p-locked sound parameter (param, value) */
    uint8_t param;                /* TG_LK_* (TG_EV_PARAM) */
    int16_t value;                /* the locked value; TG_LK_RESTORE: back to the part's own setting */
} tg_event;

enum { TG_EV_NOTE, TG_EV_PARAM, TG_EV_PROGRAM };   /* PROGRAM: the part's instrument family (value: TI_*) */

/* ---- the sequencer: step locks and trig conditions -------------------------------------------------
 * A step is a tick of the period (generator) or of the frozen pattern. Locks and conditions are an
 * overlay on top of whatever plays: they apply the same to generated and to frozen music. */
enum {
    TG_C_ALWAYS,
    TG_C_PROB,                    /* plays with a chance of a percent */
    TG_C_RATIO,                   /* a:b, plays on loop a of every b (1:2, 2:2, 1:4, 3:4 ...) */
    TG_C_FILL, TG_C_NOT_FILL,     /* only while FILL is held / only while it is not */
    TG_C_FIRST, TG_C_NOT_FIRST,   /* only the first loop / every loop but the first */
    TG_C_PRE, TG_C_NOT_PRE,       /* as the previous conditional step of the part went / the opposite */
    TG_C_COUNT
};
enum {
    /* note locks, applied by Tuna */
    TG_LK_VEL,                    /* velocity 1..127 */
    TG_LK_STEPS,                  /* move the note by scale steps, staying in key */
    TG_LK_OCT,                    /* octaves */
    TG_LK_LEN,                    /* length in ticks */
    /* sound locks, sent as TG_EV_PARAM for the host to apply to the part, restored at its next note */
    TG_LK_E0, TG_LK_E1, TG_LK_E2, TG_LK_E3, TG_LK_E4, TG_LK_E5, TG_LK_E6, TG_LK_E7,   /* engine knobs */
    TG_LK_LEVEL, TG_LK_PAN, TG_LK_DIST, TG_LK_CHO, TG_LK_DLY, TG_LK_REV,
    TG_LK_COUNT
};
#define TG_LK_RESTORE (-32768)
#define TG_STEP_LOCKS 3
#define TG_STEPS 64

typedef struct {
    uint16_t tick;
    uint8_t part, cond, ca, cb, nlock;
    struct { uint8_t param; int16_t value; } lock[TG_STEP_LOCKS];
} tg_step;

/* ---- frozen patterns ------------------------------------------------------------------------------- */
typedef struct {
    uint16_t tick, len;           /* start and length in ticks */
    uint8_t part, note, vel, pad;
} tg_pnote;

#define TG_PAT_MAX 384

typedef struct {
    uint16_t len;                 /* ticks: one period */
    uint16_t n;
    int8_t root, scale;           /* the key it was recorded in (for transposing in key) */
    tg_pnote ev[TG_PAT_MAX];      /* sorted by tick */
} tg_pattern;

typedef void (*tg_event_fn)(void *ctx, const tg_event *e);

typedef struct {
    /* settings (change any time) */
    float bpm;
    int division;                 /* master ticks per beat: 2 = eighths, 4 = sixteenths */
    int beats_per_bar;            /* meter: 4 = 4/4, 3 = 3/4 */
#define TG_METER_MAX 12
    uint8_t meter[TG_METER_MAX];  /* odd meters: beat groups in ticks, 0-terminated (3+3+2, 2+2+3, 3+2 ...);
                                     meter[0] == 0: beats_per_bar beats of `division` ticks */
    uint16_t meter_bars;          /* bit i: group i starts a bar (bit 0 implied): changing meters as a cycle */
    float swing;                  /* 0..1: the off-beat eighth late (1 = triplet swing, 2:1) */
    float lilt;                   /* 0..1: Viennese waltz: the second beat comes early */
    int root;                     /* key tonic pitch class, 0 = C */
    int scale;                    /* TS_* */
    int prog;                     /* TP_* */
    int chord_ticks;              /* master ticks per chord */
    int base_note[TG_PARTS];      /* MIDI note of the bottom of each part's range (before the root) */
    float min_gap_s;              /* shortest time between two notes of a part (runaway guard) */
    bool harmonize;               /* false: raw Marbles notes */
    float melody_density;         /* 0..1: share of off-beat ticks the melody plays */
    int melody_range;             /* semitones above base_note the melody may use */
    int phrase_bars;              /* shortest melody period (question + answer) in bars */
    int pad_tones;                /* 3 = triads, 4 = sevenths */
    bool pad_restrike;            /* re-strike the pad chord on T3 */
    int bass_mode;                /* TG_BASS_* */
    int pad_mode;                 /* TG_PAD_* */
    float appoggiatura;           /* chance of an appoggiatura on a melody downbeat */
    float suspension;             /* chance of a suspension where the chord changes */
    bool trill;                   /* trill on the note before the final tonic */
    float mel_profile[4];         /* melody interval thresholds: repeat < step < third < fourth */
    int max_leap;                 /* scale steps of the widest melody leap */
    uint16_t melody_mask;         /* restrict the melody to these pitch classes (0 = no) */
    uint16_t mel_add;             /* pitch classes the melody may add to its scale (blue notes) */
    float syncopation;            /* 0..1: melody notes move from beats to off-beats */
    bool chromatic;               /* appoggiaturas lean from a semitone above */
    int arrival_bars;             /* how long the final tonic is held */
    bool plagal;                  /* periods end IV - I */
    float whole_tone;             /* chance of a whole-tone period */
    int motif_beats;              /* motif length */
    float motif_use;              /* chance a later segment develops the motif */
    float var_invert, var_alter;  /* development: inversion, changed ending */
    bool hemiola;                 /* 3/4: twos against threes before cadences */
    float sforzando;              /* chance of an accent on a weak beat */
    float swell;                  /* crescendo through the period */
    float rubato;                 /* slowing into cadences */
    /* composer mechanisms (tuna_style.h): copied by tg_apply_style, changed by contrast periods */
    int mel_mode;                 /* TG_MEL_* */
    int harmony;                  /* TG_HARM_* (with a style) */
    int mel_min_ticks;            /* the shortest free melody note */
    float direction;              /* -1..1: the melody leans downward or upward */
    float staccato;               /* 0..1: melody notes shorter than their time */
    float turn;                   /* chance of a turn on a downbeat */
    float octave;                 /* chance a wide leap is an octave */
    int pad_interval;             /* ORGANUM, DOUBLE: semitones below the melody (0: default) */
    int poly_semis;               /* BLOCK: a second major triad above (0: none) */
    bool spread;                  /* BLOCK: open voicing */
    int strum_ms;                 /* chords spread low to high */
    float gp;                     /* chance of a general pause after an arrival */
    float echo;                   /* chance a period is an echo (softer) */
    int tuning;                   /* TT_TUNE_*: cents sent with each note (tg_event.value) */
    int tune_root;                /* the pitch class the tuning is laid on */
    int perc_mode;                /* TG_PERC_OFF, TG_PERC_PERIOD (the composer's orchestra), TG_PERC_KIT */
    uint8_t perc_w[TG_PI_COUNT];  /* period percussion: 0..10 per instrument */
    const tg_style *style;        /* NULL: the fixed progression `prog` */

    /* Marbles */
    tm_rng rng;
    tm_tgen t;
    tm_xygen xy;
    tm_group x, y;
    int clock_source;

    /* state */
    float sample_rate;
    uint32_t seed;
    long ticks;
    float prev_master;
    int chord_step;
    int loaded_scale, loaded_prog, loaded_step, loaded_root;
    uint16_t allowed_pcs;         /* scale + current chord tones, relative to the tonic */
    uint16_t chord_scale;         /* the scale bent to fit the chord (G -> G# over E major in A minor) */
    tt_chord chord;
    int sounding[TG_PARTS];       /* note held by bass and melody, -1 = none */
    int pad[TG_PAD_VOICES], pad_n;
    int pad_poly;                 /* of pad_n, the notes of a polychord's upper triad */
    bool gate_prev[TG_PARTS];
    uint32_t since_note[TG_PARTS];
    int mel_note, mel_prev_interval, bass_note;
    bool chord_fresh;             /* the chord changed and the bass has not played it yet */
    long mel_hold_until;          /* tick before which the melody holds its note */
    long mel_off_at;              /* tick at which a held melody note is released, -1 = none */
    tm_rng hrng;                  /* the harmonizer's own randomness: Marbles stays untouched */
    tm_sequence mel_rhythm;       /* which off-beat ticks the melody plays, with deja vu */
    int8_t plan[TG_PLAN_MAX];     /* the period's chords, vocabulary indices (with a style) */
    int8_t plan_slip[TG_PLAN_MAX];/* semitones a slot's chord is moved (side-slips, octatonic colour) */
    int period_scale;             /* the period's colour scale (TS_*), -1: the key's own */
    bool open_end;                /* this period ends away from the tonic, on open_chord */
    bool open_prev;               /* this period began on the previous period's open chord */
    int8_t open_chord;            /* vocabulary index the next period starts on when open_end */
    int8_t open_chord_prev;
    int8_t plan_tonic;            /* the tonic chord the planner chose for slot 0 */
    int cycle_sel;                /* TG_HARM_CYCLE: 0 or 1 (cycle2) */
    int home_root, home_scale;    /* the style's key, for its own modulations */
    bool style_away;              /* a style modulation is away from home */
    long stat_open, stat_colour, stat_slips, stat_modulations;   /* statistics for the checks */
    /* melody modes */
    int tinti_k, tinti_i, tinti_rest, tinti_dir;   /* tintinnabuli: phrase length, note, rest, direction */
    int pat_i;                    /* PATTERN: the next element */
    int bird_left, bird_rest;     /* BIRDSONG: notes left in the burst, ticks of silence */
    int orn_q[3], orn_n;          /* a turn's remaining notes */
    /* bass and third-part modes */
    int bass_hit, bass_alt;       /* OSTINATO: hit of the bar; BEAT: root or fifth */
    int pad_note;                 /* figures, canon, phase: the third part's sounding note (-1 none) */
#define TG_CANON_MAX 128
    int16_t canon_buf[TG_CANON_MAX];   /* the melody by tick: a note, -1 held, -2 released */
    int add_len, add_rep, add_dir, add_pos;   /* ADDITIVE: active figure length, loops, growing or not */
    int phase_off, phase_rep;     /* PHASE: the drift and its loop count */
    long oompah_off;              /* OOMPAH: the tick the short chord ends */
    /* form */
    long gp_from, gp_until;       /* a general pause */
    int echo_vel;                 /* an echo period's velocity offset */
    bool alt_on;                  /* a contrast period */
    /* instrumentation */
    int orch_cur[TG_PARTS];       /* the family each part plays now (TI_*), -1: not sent yet */
    int orch_set;
    tm_rng orng;                  /* the orchestrator's own randomness */
    int plan_len;
    bool plan_minor;
    int mel_pending;              /* a melody note due at mel_pending_q (ornament resolution), -1 = none */
    long mel_pending_q;
    int trill_main, trill_upper;
    long trill_start, trill_until;
    int mel_last_dir;             /* direction of the melody's last move: -1, 0, 1 */
    int counter_note, counter_prev_iv, counter_prev_mel;
    bool pad_silent;              /* the pad voicing is a reference for figures, not sounding */
    bool plan_wt;                 /* this period is coloured whole-tone */
    uint16_t mel_scale;           /* the melody's notes: chord scale, limited by melody_mask */
    /* the motif: rhythm and scale-step intervals of the period's first segment */
#define TG_MOTIF_MAX 32
    bool motif_play[TG_MOTIF_MAX];
    int8_t motif_k[TG_MOTIF_MAX];
    int motif_len, motif_seg, motif_start_note, motif_last;
    bool motif_valid, motif_recording, motif_active, motif_invert, motif_alter;
    long motif_notes, free_notes;  /* statistics: notes from development vs free invention */
    /* percussion: its own randomness, so switching drums on or off never changes the notes */
    tm_rng prng;
    int drum_on[8], drum_n;
    int timp_note;
    long timp_off_at, roll_until;
    int roll_note;
    float roll_countdown;          /* samples to the roll's in-between stroke */
    bool march_on, kit_g1_prev, kit_g3_prev;
    long perc_hits;
    /* humanization: a player's small lateness and touch, on its own randomness */
    float humanize;                /* 0 = machine-exact (the default), 1 = a natural amount */
#define TG_HQ 160
    struct { long due; uint32_t order; tg_event e; } hq[TG_HQ];
    int hq_n;
    uint32_t hq_order;            /* arrival order: events due together leave in the order they came */
    long clock;                    /* samples rendered */
    int tick_delay;                /* swing and lilt: samples the events of this tick come late */
    uint16_t hdelay[TG_EVENT_PARTS][128];
    long hdue[TG_EVENT_PARTS][128];  /* when the note's last start or end leaves the queue: a note never
                                        ends before it starts, nor starts again before it ended */
    tm_rng hum_rng;
    tg_event_fn user_fn;
    void *user_ctx;

    /* sequencer (tuna_gen.c, "sequencer"): the last complete period is always recorded, so FREEZE can
     * keep what was just heard; the frozen pattern loops until UNFREEZE */
    tg_pattern rec[2];            /* recording and the last complete period, swapped at each boundary */
    int rec_cur;
    bool rec_valid;               /* rec[!rec_cur] holds a complete period */
    int16_t rec_open[TG_EVENT_PARTS][128];   /* index of a recorded note still sounding, -1 none */
    tg_pattern pat;               /* the frozen pattern */
    bool frozen, freeze_req, unfreeze_req;
    uint16_t pat_pos;             /* the pattern's next tick */
    int seq_step;                 /* the step of the tick now sounding (pattern or period), for displays */
    uint32_t pat_loop;
    struct { uint8_t part, note; uint16_t left; } play[48];   /* pattern notes sounding */
    int play_n;
    /* step overlay */
    tg_step steps[TG_STEPS];
    int nsteps;
    bool fill;                    /* FILL held */
    int16_t xmap[TG_EVENT_PARTS][128];       /* a passed note -> the note sent (locks), -1 none, -2 dropped */
    uint8_t lenlocked[TG_EVENT_PARTS][16];   /* bit per note: its release comes from a LEN lock */
    struct { uint8_t part, note; uint16_t left; } loff[16];  /* LEN-locked releases due */
    int loff_n;
    uint32_t sound_locked[TG_EVENT_PARTS];   /* sound locks active per part (bit per TG_LK_*) */
    int cond_tick[TG_EVENT_PARTS];           /* the step a part's condition was last evaluated for */
    uint32_t cond_loop[TG_EVENT_PARTS];
    bool cond_pass[TG_EVENT_PARTS], cond_pre[TG_EVENT_PARTS];
    tm_rng srng;                  /* conditions' chances */

    /* evolution: an arc of energy over a cycle of periods, and a modulation away and home */
    bool evolve, evolve_modulate;
    int mod_kind;                 /* TG_MOD_*: where the evolution modulates to */
    int evolve_periods;           /* periods per cycle */
    float evolve_depth;           /* 0..1 */
    float base_density;
    int base_root, base_scale;
    uint8_t base_perc[TG_PI_COUNT];
    int energy_vel;               /* velocity offset now */
    float energy;                 /* 0..1 now */
    long period_index;
    bool key_pending, key_away;
    int next_root, next_scale;
    bool plan_minor0;             /* slot 0's key while a modulation is pending */
    long periods_away;            /* statistics: periods that started in the away key */
    long key_set_tick;            /* the tick a modulation was decided (it applies at the next chord) */
    bool perc_quiet;              /* evolution: the kit waits for the energy to build */

    /* the player's hands (the UI): chaos around the composer's own settings, a root from a key, a chord
     * held on the keys. Chaos 0.5 is the composer exactly; the macro moves all three together. */
    float chaos_rhythm;           /* 0..1: on the beat .. syncopated, varied rhythm */
    float chaos_pitch;            /* 0..1: steps and repeats .. leaps */
    float chaos_density;          /* 0..1: sparse .. busy (the melody, x0.35 .. x2.8) */
    float chaos_macro;            /* 0..1: added to the three (0.5: nothing) */
    int root_req;                 /* a new tonic from a key, applied at the next chord change; -1 none */
    uint16_t hold_mask;           /* a chord held on the keys: its pitch classes (absolute), 0 none */
    int8_t hold_root;             /* its lowest key's pitch class (the chord's root) */
} tg_gen;

void tg_init(tg_gen *g, uint32_t seed, float sample_rate);
/* Restart from the seed: the same seed and settings play the same music again. */
void tg_reseed(tg_gen *g, uint32_t seed);
/* Render size samples (size <= TG_MAX_BLOCK) and report note events through fn. */
#define TG_MAX_BLOCK 64
void tg_process(tg_gen *g, int size, tg_event_fn fn, void *ctx);
/* Ticks since the melody's last arrival on the tonic, for the current tick (0 = arriving now);
 * *period gets the period's length in ticks. */
int tg_period_position(const tg_gen *g, int *period);
/* Tick within the period where the question ends (the half cadence). */
int tg_half_position(const tg_gen *g);
/* The meter: ticks in one cycle of beat groups (one bar unless the meter changes), and bars in it. */
int tg_meter_cycle(const tg_gen *g, int *bars);
/* Where a tick falls: the beat group, the tick in it, and whether it starts a bar. */
typedef struct { int group, beat, sub, glen, bar_pos, bar_len, nbeats; bool bar_start; } tg_meter_pos;
void tg_meter_at(const tg_gen *g, long q, tg_meter_pos *m);
/* Apply a composer signature (TG_STYLE_*): key, tempo, meter, harmony, accompaniment, ornaments.
 * Takes effect from the next period; call tg_reseed() to start cleanly. */
void tg_apply_style(tg_gen *g, int style);
/* The same from a signature table (a user's own composer, or a test's). It must outlive its use. */
void tg_apply_style_def(tg_gen *g, const tg_style *st);
/* The chord at a chord step (the planned harmony with a style, else the progression). */
void tg_chord_at(const tg_gen *g, int step, int tones, tt_chord *c);
/* ---- sequencer ----
 * FREEZE: at the next period boundary the period just heard becomes a loop (returns false if no
 * complete period was recorded yet). UNFREEZE: the generator takes over again at the next boundary. */
bool tg_freeze(tg_gen *g);
void tg_unfreeze(tg_gen *g);
/* The frozen pattern, to edit: notes sorted by tick */
int tg_pat_add(tg_gen *g, int tick, int part, int note, int vel, int len);   /* index, or -1 if full */
void tg_pat_remove(tg_gen *g, int index);
void tg_pat_transpose(tg_gen *g, int part, int steps);   /* scale steps in the pattern's key; part -1: all
                                                            pitched parts (not drums, not timpani) */
/* Step overlay: a condition and up to TG_STEP_LOCKS locks per (part, tick). 0 on success. */
int tg_step_cond(tg_gen *g, int part, int tick, int cond, int a, int b);
int tg_step_lock(tg_gen *g, int part, int tick, int param, int value);
void tg_step_clear(tg_gen *g, int part, int tick);
/* Evolution: an energy arc over `periods` periods (density, dynamics, percussion), with an optional
 * modulation to the dominant (major) or the relative major (minor) and back, by pivot chords. */
void tg_evolve(tg_gen *g, bool on, int periods, float depth, bool modulate);

/* Percussion: TG_PERC_OFF, TG_PERC_PERIOD or TG_PERC_KIT. A style sets its default. */
void tg_set_percussion(tg_gen *g, int mode);
/* The player's hands. Chaos: 0..1 each, 0.5 = the composer's own (tg_init and a style leave 0.5).
 * A root (pitch class 0..11): stopped, at once; playing, from the next chord change, the music moved
 * there whole. A chord held on the keys (absolute pitch classes, its root the lowest key): the harmony
 * until released (mask 0). */
void tg_set_chaos(tg_gen *g, float rhythm, float pitch, float density, float macro);
void tg_set_root(tg_gen *g, int pc);
void tg_hold_chord(tg_gen *g, uint16_t mask, int root_pc);

/* Release every sounding note (stop). */
void tg_all_off(tg_gen *g, tg_event_fn fn, void *ctx);

/* Marbles scale weighted for a chord inside a key scale (pitch classes relative to the tonic). */
void tg_scale_for_chord(tm_scale *s, uint16_t scale_mask, const tt_chord *chord);
/* The key scale with chord tones forced in: a chord tone outside the scale replaces the scale
 * note a semitone away (A minor over E major: G becomes G#). */
uint16_t tg_chord_scale(uint16_t scale_mask, const tt_chord *chord);

#endif
