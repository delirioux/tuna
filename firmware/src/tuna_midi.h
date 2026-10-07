/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA MIDI out: every part on its own channel, for a DAW or other synths.
 *   bass 1, melody 2, pad 3, timpani 4, drums 10 (General MIDI's drum channel and key map, as Tuna's own)
 * Notes with their velocities; the part's instrument family as a General MIDI program change (a piano part
 * is program 1, strings 49, and so on); the sound locks of the sequencer as controllers (level 7, pan 10,
 * reverb 91, chorus 93, delay 94, the engine knobs E0..E7 as 70..77); a historical tuning as pitch bend
 * (the one-note parts only: bass, melody, timpani; a bend range of 2 semitones); MIDI clock (24 a quarter
 * note) with start and stop. Stopping turns every note off that is still on, and sends All Notes Off.
 *
 * Messages go into a queue stamped with the audio frame they belong to; the host sends them (Felucca: USB
 * MIDI) or, on the host, writes a MIDI file. No allocation, no output of its own. */
#ifndef TUNA_MIDI_H
#define TUNA_MIDI_H

#include <stdbool.h>
#include <stdint.h>
#include "tuna_gen.h"

#define TMI_QUEUE 256

typedef struct {
    uint32_t frame;                       /* when (audio frames since start) */
    uint8_t b[3], n;                      /* the message, its length */
} tmi_msg;

typedef struct {
    bool on;                              /* MIDI out at all */
    bool clock;                           /* clock, start and stop */
    bool bend;                            /* tunings as pitch bend */
    uint8_t ch[TG_EVENT_PARTS];           /* channel of each part, 0..15 */
    uint8_t held[16][128];                /* notes on, per channel (counted) */
    int16_t bend_now[16];
    int8_t prog[16];                      /* the program last sent, -1 none */
    tmi_msg q[TMI_QUEUE];
    uint16_t w, r;
    uint32_t dropped;                     /* messages lost to a full queue */
    uint32_t clock_next, clock_frac;      /* the frame of the next clock, and its fraction (Q16): no doubles,
                                           * the FM-1's FPU is single precision */
    float sr;
    uint8_t act[16];                      /* activity, for the screen (set on a note, the screen fades it) */
} tmi_out;

void tmi_init(tmi_out *m, float sample_rate);
void tmi_event(tmi_out *m, const tg_event *e, uint32_t frame);
void tmi_start(tmi_out *m, uint32_t frame);
void tmi_stop(tmi_out *m, uint32_t frame);  /* every held note off, All Notes Off, stop */
void tmi_panic(tmi_out *m, uint32_t frame); /* every held note off, All Notes Off (before MIDI is switched off) */
void tmi_clock(tmi_out *m, uint32_t frame, int frames, float bpm);   /* the clocks within these frames */
bool tmi_pop(tmi_out *m, tmi_msg *msg);
int tmi_gm_program(int family);           /* TI_* -> General MIDI program 0..127 */

#endif
