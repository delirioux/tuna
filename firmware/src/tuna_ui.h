/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA screen and controls: Tuna's own pages on Felucca's drawing (gfx.c: canvas, fonts, palettes, icons).
 * Compiled after gfx.c in one unit (as Felucca's ui*.c are): the firmware's unity build, or a host test.
 *
 * The controls
 *   SELECT     tempo                          MASTER     volume
 *   PRESETS    the library: every composer and character, era by era (the list shows while turning;
 *              playing, the choice comes in at the end of the period)
 *   ALGORITHM  the library, a whole era at a time
 *   KNOB 1..4  the four cards of the page
 *   PLAY       start / stop (always from the top: the same seed, the same music)
 *   REC        memory: the phrase locked (deja vu) / the composer's own
 *   OCT- OCT+  the seed: the piece before / after, in the same manner; on SAVE, LOAD and SEQ: back / do it
 *   keys       one key tapped: the root (from the next chord); keys held together: that chord, while held;
 *              on SAVE and LOAD the 16 white keys are the 16 slots
 *   HOME       the chaos page (the notes falling); held: the menu (colour, style)
 *   FX         the library page        SCL   key, scale, tuning, memory
 *   EDIT       time (meter, swing, humanize, percussion), again: the parts (melody, bass, pad, harmony)
 *   ARP        evolution               SEQ   freeze the period into a loop, transpose it
 *   GLO        the mix (four levels), again: reverb       SAVE  save to a slot, again: load
 *   ENV        MIDI out: on, the FM-1's own sound, clock, tunings as bend; the channels and what they play
 *   LFO        the resonator: amount, decay, brightness, what its strings are tuned to */
#ifndef TUNA_UI_H
#define TUNA_UI_H

#include <stdbool.h>
#include <stdint.h>
#include "tuna_app.h"

enum { TU_K_MASTER, TU_K_SELECT, TU_K_PRESETS, TU_K_ALGO, TU_K1, TU_K2, TU_K3, TU_K4, TU_KNOBS };
enum {
    TU_B_FX, TU_B_SCL, TU_B_ENV, TU_B_LFO, TU_B_EDIT, TU_B_GLO,
    TU_B_HOME, TU_B_SAVE, TU_B_ARP, TU_B_SEQ, TU_B_PLAY, TU_B_REC,
    TU_B_OCTDN, TU_B_OCTUP, TU_BUTTONS
};
enum {
    TU_PG_HOME, TU_PG_LIB, TU_PG_KEY, TU_PG_TIME, TU_PG_PARTS, TU_PG_EVOLVE, TU_PG_SEQ,
    TU_PG_MIX, TU_PG_FX, TU_PG_SAVE, TU_PG_LOAD, TU_PG_MENU, TU_PG_MIDI, TU_PG_RESO, TU_PAGES
};
#define TU_KEY_LO 53                      /* F3: the FM-1's lowest key */
#define TU_KEY_HI 79                      /* G5 */
#define TU_PARTICLES 64

typedef struct {
    int16_t x, y, vy;                     /* 1/16 px */
    uint8_t part, size, life;
} tu_particle;

typedef struct {
    tuna_app *app;
    int page;
    int hot, hot_ms;                      /* the knob just turned (its card in the accent) */
    int browse_ms;                        /* the library list shown over the panel */
    int browse;                           /* the item the list points at */
    char msg[32];
    int msg_ms;
    int slot;                             /* SAVE / LOAD: the slot under the cursor */
    int confirm;                          /* SAVE over a used slot: 1 = OCT+ again replaces it */
    uint16_t slots_used;
    char slot_names[TP_SLOTS][TP_NAME + 1];
    bool slots_dirty;
    int home_ms;                          /* HOME held: the menu at 600 ms */
    bool home_down;
    uint32_t keys_down;                   /* bit (note - TU_KEY_LO) */
    int key_gesture;                      /* keys pressed since all were up: 1 = a tap sets the root */
    int key_first;
    uint8_t master;
    int palette, style;
    tu_particle pt[TU_PARTICLES];
    uint32_t rnd;
    uint32_t frame;
    int batt;                             /* 0..3 bars, 4 = USB power */
    /* what is on the screen (redraw only what changed) */
    uint32_t sig_head, sig_card[4], sig_foot;
    bool force;
} tuna_ui;

void tu_init(tuna_ui *u, tuna_app *a);
void tu_knob(tuna_ui *u, int knob, int delta);
void tu_button(tuna_ui *u, int button, bool down);
void tu_key(tuna_ui *u, int note, bool down);
void tu_tick(tuna_ui *u, int ms);         /* timers and the particles; call every frame */
void tu_draw(tuna_ui *u);                 /* what changed; the panel every frame */
void tu_draw_all(tuna_ui *u);             /* everything */
const char *tu_page_name(int page);

#endif
