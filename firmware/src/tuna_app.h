/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA app: what the screen and the controls act on. It owns the generator and holds what the player
 * set: the piece from the library (a character preset or a composer), the seed, play and stop, the chaos
 * knobs, the memory (deja vu), the mixer, the 16 slots. It renders the music in blocks and hands every
 * event to the sound (Felucca's engines on the FM-1, a test on the host) and the note-ons to the screen.
 *
 * The library: the 8 character presets first (group 0, "TUNA"), then the 89 composers and styles by era and year of
 * birth (groups 1..9). Choosing while playing waits for the end of the period, so the music turns over
 * at a cadence; stopped, it is at once. Play starts the piece from its beginning (the same seed plays
 * the same music); OCT+ / OCT- step the seed: another piece. A composer's 256 seeds are sorted by the type of
 * work: a block of seeds per manner, in the manners' order (BACH: the fugues, then the chorales, then the gigues).
 *
 * Every piece has a code to share it ("BACH-FUGUE-17": the piece, the manner, the number in it), the same piece
 * on any FM-1 and in the browser. Favourites (up to TA_FAVS pieces) and the radio (another piece every few minutes)
 * are kept with the player's preferences in a storage object of their own, written while stopped. */
#ifndef TUNA_APP_H
#define TUNA_APP_H

#include <stdbool.h>
#include <stdint.h>
#include "tuna_gen.h"
#include "tuna_preset.h"
#include "tuna_midi.h"
#include "tuna_reso.h"

#define TA_SEEDS 256                      /* seeds 0..255: a full turn of steps */
#define TA_GROUPS (1 + TP_ERA_COUNT)      /* TUNA's characters, then the eras */
#define TA_FEED 32                        /* note-ons waiting for the screen */
#define TA_FAVS 32                        /* favourites (in the pool, 19 B each: its headroom) */
#define TA_FAV_NAME 18                    /* a library name (17) and its end */
#define TA_CODE 40                        /* a share code and its end */

enum { TA_DV_OFF, TA_DV_STYLE, TA_DV_LOCK, TA_DV_COUNT };   /* memory: fresh, the composer's own, locked */
enum { TA_RADIO_OFF, TA_RADIO_ERA, TA_RADIO_ALL, TA_RADIO_COUNT };   /* the radio: off, within the era, anywhere */
enum { TA_CLK_OFF, TA_CLK_OUT, TA_CLK_IN, TA_CLK_COUNT };   /* MIDI clock: none, sent, followed */

typedef struct { char name[TA_FAV_NAME]; uint8_t seed; } ta_fav;

typedef struct { uint8_t part, note, vel; } ta_note;

/* settings that change the form (the meter, the parts' modes, the harmony, the scale): asked for now, they
 * come in where the period ends (stopped: at once), the piece starting over in them. -1: no change asked */
typedef struct { int8_t meter, mel, bass, pad, harm, scale; } ta_edit;
enum { TA_M_44, TA_M_34, TA_M_24, TA_M_54, TA_M_68, TA_M_78, TA_M_332, TA_M_COUNT };
typedef struct {
    tg_gen g;
    float sample_rate;
    bool playing;
    int item;                             /* the library item playing (or ready) */
    int pending;                          /* chosen while playing: loads at the end of the period; -1 none */
    ta_edit req;                          /* form settings waiting for the period's end */
    bool req_on;
    uint32_t seed;
    int deja;                             /* TA_DV_* */
    uint8_t level[TP_TRACKS];             /* 0..100: bass, melody, pad, drums */
    uint8_t reverb;                       /* 0..100: the reverb send of the mix */
    uint8_t rev_size, rev_damp, rev_type; /* the reverb itself: size and damping 0..100, TG_REV_* (the piece's) */
    ta_note feed[TA_FEED];                /* note-ons for the screen (a ring) */
    uint8_t feed_w, feed_r;
    void (*sound)(void *ctx, const tg_event *e);
    void *sound_ctx;
    bool local;                           /* the FM-1's own sound (off: MIDI only) */
    tmi_out midi;                         /* MIDI out (tuna_midi.h): the host sends what it queues */
    uint32_t frame;                       /* audio frames since start, the MIDI time stamps */
    tr_reso reso;                         /* the resonator (tuna_reso.h): the host runs it on its mix */
    uint8_t reso_amount, reso_decay, reso_bright, reso_mode;   /* 0..100, TR_TUNE_* */
    uint32_t reso_sig;                    /* the harmony it is tuned to */
    tp_store store;
    long blocks;
    int seed_req;                         /* a seed asked for while playing (the next half period), -1 none */
    uint32_t rnd;                         /* draws a new piece's seed (ta_stir: the host's entropy) */
    int manner_fix;                       /* a slot's manner off its seed's block (an older slot), -1: the seed's */
    /* the player's preferences (their own storage object, TP_PREFS_GROUP): favourites and the radio */
    ta_fav fav[TA_FAVS];
    int fav_n;
    bool prefs_dirty;                     /* changed since written (written while stopped: ta_prefs_flush) */
    uint8_t radio, radio_min;             /* TA_RADIO_*, minutes a piece plays */
    uint32_t radio_frames;                /* frames the piece has played */
    /* MIDI clock in: tempo and phase follow the clock's pulses */
    bool clock_in;
    uint32_t ck_pulses;                   /* pulses since Start */
    uint32_t ck_last, ck_beat0;           /* frame of the last pulse, of the pulse 24 before */
    uint32_t ck_beat_n;                   /* pulses counted toward the beat */
    float ck_bpm;                         /* the clock's tempo (0: not yet known) */
    bool ck_run;                          /* Start (or Continue) came, Stop did not */
    uint32_t ck_idle;                     /* frames played since the last pulse */
} tuna_app;

/* the reverb types: each a size and damping (0..100) on Felucca's ROOM or SPRING; ROOM and SPRING as designed */
typedef struct { const char *name, *card; uint8_t size, damp; bool spring; } ta_reverb;   /* (card: at most 6) */
const ta_reverb *ta_reverb_type(int type);
int ta_reverb_step(int type, int d);      /* the next type on the knob, small spaces to large, then SPRING */
int ta_piece_reverb(int style);           /* TG_REV_*: the style's own, else its era's (-1: a character, ROOM) */
void ta_set_reverb_type(tuna_app *a, int type);   /* the type with its size and damping */

void ta_init(tuna_app *a, float sample_rate, uint32_t seed);
void ta_stir(tuna_app *a, uint32_t v);   /* entropy for the next piece's seed (the host: a timer at each touch) */
void ta_set_manner(tuna_app *a, int manner);     /* the first seed of that manner's block */
const char *ta_manner_name(const tuna_app *a);   /* the manner playing, "" for a style without manners */
/* sorted seeds: a style's manner for a seed, the first seed of a manner's block, the block's size */
int ta_seed_manner(int style, uint32_t seed);
uint32_t ta_manner_first(int style, int manner);
uint32_t ta_manner_size(int style, int manner);
/* "FUGUE 3/40" (the manner, the seed's number in its block), "SEED 147" without manners: the seed asked for */
void ta_seed_label(const tuna_app *a, char *out, int max);
/* the share code: the piece, its manner, the seed's number ("BACH-FUGUE-17", "REGGAE-148"); for an item and seed,
 * and the one playing (or coming). ta_code_go plays (or selects) a code's piece: 0, or -1 when it is not one */
void ta_code_of(int item, uint32_t seed, char *out, int max);
void ta_code(const tuna_app *a, char *out, int max);
int ta_code_go(tuna_app *a, const char *code);
/* favourites: the piece playing (or coming) added or taken out: its index + 1 when added, 0 taken out, -1 full */
int ta_fav_toggle(tuna_app *a);
int ta_fav_find(const tuna_app *a, int item, uint32_t seed);   /* index, or -1 */
int ta_fav_item(const tuna_app *a, int i);                     /* the favourite's library item, -1: not (any more) */
void ta_fav_play(tuna_app *a, int i);                          /* select it (playing: at the next half period) */
/* the radio: TA_RADIO_*, minutes a piece plays (1..30) */
void ta_radio_set(tuna_app *a, int mode, int minutes);
/* the preferences: read at start (ta_set_store), written while stopped when changed (the host's main loop):
 * TP_OK, nothing to do or not now (playing) 1, or TP_ERR_* */
int ta_prefs_flush(tuna_app *a);
/* MIDI clock in: on, and a real-time message (0xF8 clock, 0xFA start, 0xFB continue, 0xFC stop) at frame */
void ta_clock_in_set(tuna_app *a, bool on);
void ta_clock_msg(tuna_app *a, uint8_t status, uint32_t frame);
void ta_select_seed(tuna_app *a, int item, uint32_t seed);   /* a piece with this seed */
void ta_set_sound(tuna_app *a, void (*fn)(void *ctx, const tg_event *e), void *ctx);
void ta_set_store(tuna_app *a, const tp_store *s);

/* the library */
int ta_lib_count(void);
int ta_lib_group(int item);               /* 0: TUNA, 1..9: the eras */
int ta_lib_first(int group);              /* the group's first item */
const char *ta_lib_name(int item);
const char *ta_group_name(int group);
int ta_lib_born(int item);                /* a composer's year of birth, 0 for a character */
int ta_lib_style(int item);               /* TG_STYLE_*, -1 for a character */

void ta_select(tuna_app *a, int item);    /* at once (stopped) or at the period's end (playing) */
void ta_play(tuna_app *a, bool on);
void ta_seed_step(tuna_app *a, int delta);
void ta_set_seed(tuna_app *a, uint32_t seed);
void ta_set_deja(tuna_app *a, int mode);
void ta_set_chaos(tuna_app *a, float rhythm, float pitch, float density, float macro);
/* the player's settings (each piece brings its own again) */
void ta_set_chaos_parts(tuna_app *a, unsigned mask);   /* which parts follow the chaos knobs */
void ta_set_tempo(tuna_app *a, float bpm);
void ta_set_root(tuna_app *a, int pc);
void ta_set_percussion(tuna_app *a, int mode);
void ta_set_evolve(tuna_app *a, bool on, int periods, float depth, bool modulate);
/* form settings (ta_edit): ta_edit_get gives what will play (asked for, or playing now) */
void ta_edit_set(tuna_app *a, const ta_edit *e);
void ta_edit_get(const tuna_app *a, ta_edit *e);
const char *ta_meter_name(int m);
int ta_meter_now(const tg_gen *g);        /* TA_M_*, or -1: the composer's own (another) meter */
void ta_process(tuna_app *a, int frames); /* render (any size; nothing while stopped) */
bool ta_feed_pop(tuna_app *a, ta_note *n);

/* MIDI out and the resonator */
void ta_midi_set(tuna_app *a, bool on, bool clock, bool bend);
void ta_reso_set(tuna_app *a, int amount, int decay, int bright, int mode);   /* 0..100, TR_TUNE_* */

/* the 16 slots: the whole state (tuna_preset.h) */
int ta_slot_save(tuna_app *a, int slot);  /* TP_OK or TP_ERR_* */
int ta_slot_load(tuna_app *a, int slot);
uint16_t ta_slots_used(tuna_app *a, char names[TP_SLOTS][TP_NAME + 1]);

#endif
