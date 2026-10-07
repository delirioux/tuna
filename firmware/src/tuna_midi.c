/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA MIDI out (see tuna_midi.h). */
#include "tuna_midi.h"
#include "tuna_sys.h"

static void put(tmi_out *m, uint32_t frame, uint8_t a, uint8_t b, uint8_t c, uint8_t n)
{
    if ((uint16_t)(m->w - m->r) >= TMI_QUEUE) {
        m->dropped++;
        return;
    }
    tmi_msg *x = &m->q[m->w % TMI_QUEUE];
    x->frame = frame;
    x->b[0] = a;
    x->b[1] = b;
    x->b[2] = c;
    x->n = n;
    m->w++;
}

bool tmi_pop(tmi_out *m, tmi_msg *msg)
{
    if (m->r == m->w)
        return false;
    *msg = m->q[m->r % TMI_QUEUE];
    m->r++;
    return true;
}

void tmi_init(tmi_out *m, float sample_rate)
{
    memset(m, 0, sizeof *m);
    m->sr = sample_rate;
    m->on = true;
    m->clock = true;
    m->bend = true;
    m->ch[TG_PART_BASS] = 0;
    m->ch[TG_PART_MELODY] = 1;
    m->ch[TG_PART_PAD] = 2;
    m->ch[TG_PART_TIMPANI] = 3;
    m->ch[TG_PART_DRUMS] = 9;
    for (int c = 0; c < 16; ++c)
        m->prog[c] = -1;
}

/* Tuna's families as General MIDI programs (0-based) */
int tmi_gm_program(int f)
{
    static const uint8_t GM[TI_COUNT] = {
        [TI_DEFAULT] = 0, [TI_PIANO] = 0, [TI_HARPSICHORD] = 6, [TI_ORGAN] = 19, [TI_STRINGS] = 48,
        [TI_VIOLIN] = 40, [TI_CELLO] = 42, [TI_CONTRABASS] = 43, [TI_PIZZ] = 45, [TI_FLUTE] = 73, [TI_OBOE] = 68,
        [TI_CLARINET] = 71, [TI_BASSOON] = 70, [TI_HORN] = 60, [TI_TRUMPET] = 56, [TI_BRASS] = 61, [TI_TUBA] = 58,
        [TI_HARP] = 46, [TI_GUITAR] = 24, [TI_CHOIR] = 52, [TI_VOICE] = 53, [TI_BELLS] = 14, [TI_CELESTA] = 8,
        [TI_MARIMBA] = 12, [TI_BANDONEON] = 23, [TI_SAX] = 65, [TI_SYNTH] = 81, [TI_PAD] = 89, [TI_WHISTLE] = 78,
        [TI_DRONE] = 95, [TI_EPIANO] = 4,
    };
    return f >= 0 && f < TI_COUNT ? GM[f] : 0;
}

/* a sound lock as a controller: Felucca's percentages (0..100) to 0..127 */
static int lock_cc(int param)
{
    switch (param) {
    case TG_LK_LEVEL: return 7;
    case TG_LK_PAN: return 10;
    case TG_LK_REV: return 91;
    case TG_LK_CHO: return 93;
    case TG_LK_DLY: return 94;
    case TG_LK_DIST: return 76 + 4;            /* 80: general purpose 5 */
    default:
        return param >= TG_LK_E0 && param <= TG_LK_E7 ? 70 + (param - TG_LK_E0) : -1;
    }
}

void tmi_event(tmi_out *m, const tg_event *e, uint32_t frame)
{
    if (!m->on || e->part >= TG_EVENT_PARTS)
        return;
    uint8_t ch = m->ch[e->part] & 15u;
    if (e->type == TG_EV_PROGRAM) {
        if (e->part == TG_PART_DRUMS)
            return;                            /* channel 10 is the kit */
        int p = tmi_gm_program(e->value);
        if (m->prog[ch] != p) {
            put(m, frame, (uint8_t)(0xC0 | ch), (uint8_t)p, 0, 2);
            m->prog[ch] = (int8_t)p;
        }
        return;
    }
    if (e->type == TG_EV_PARAM) {
        int cc = lock_cc(e->param);
        if (cc < 0 || e->value == TG_LK_RESTORE)
            return;
        int v = e->param == TG_LK_PAN ? 64 + e->value * 63 / 100 : e->value * 127 / 100;
        v = v < 0 ? 0 : v > 127 ? 127 : v;
        put(m, frame, (uint8_t)(0xB0 | ch), (uint8_t)cc, (uint8_t)v, 3);
        return;
    }
    uint8_t note = e->note & 127u;
    if (e->on) {
        bool mono = e->part == TG_PART_BASS || e->part == TG_PART_MELODY || e->part == TG_PART_TIMPANI;
        if (m->bend && mono) {
            int c10 = e->value;                    /* cents x 10 off equal temperament */
            int b = 8192 + c10 * 8192 / 2000;      /* a bend range of 2 semitones */
            b = b < 0 ? 0 : b > 16383 ? 16383 : b;
            if (b != m->bend_now[ch] + 8192) {
                put(m, frame, (uint8_t)(0xE0 | ch), (uint8_t)(b & 127), (uint8_t)(b >> 7), 3);
                m->bend_now[ch] = (int16_t)(b - 8192);
            }
        }
        put(m, frame, (uint8_t)(0x90 | ch), note, e->velocity ? (uint8_t)(e->velocity & 127u) : 1u, 3);
        if (m->held[ch][note] < 255)
            m->held[ch][note]++;
        m->act[ch] = 255;
    } else {
        if (!m->held[ch][note])
            return;                            /* (never sent, or already off) */
        m->held[ch][note]--;
        put(m, frame, (uint8_t)(0x80 | ch), note, 64, 3);
    }
}

void tmi_start(tmi_out *m, uint32_t frame)
{
    m->clock_next = frame;
    m->clock_frac = 0;
    if (m->on && m->clock)
        put(m, frame, 0xFA, 0, 0, 1);
}

void tmi_panic(tmi_out *m, uint32_t frame)
{
    for (int c = 0; c < 16; ++c) {
        bool any = false;
        for (int n = 0; n < 128; ++n)
            while (m->held[c][n]) {
                if (m->on)
                    put(m, frame, (uint8_t)(0x80 | c), (uint8_t)n, 64, 3);
                m->held[c][n]--;
                any = true;
            }
        if (m->on && (any || m->prog[c] >= 0))
            put(m, frame, (uint8_t)(0xB0 | c), 123, 0, 3);   /* All Notes Off */
    }
}

void tmi_stop(tmi_out *m, uint32_t frame)
{
    tmi_panic(m, frame);
    if (m->on && m->clock)
        put(m, frame, 0xFC, 0, 0, 1);
}

void tmi_clock(tmi_out *m, uint32_t frame, int frames, float bpm)
{
    if (!m->on || !m->clock || bpm <= 0.0f)
        return;
    float step = m->sr * 60.0f / (bpm * 24.0f);          /* frames per clock */
    uint32_t whole = (uint32_t)step, frac = (uint32_t)((step - (float)whole) * 65536.0f);
    if ((int32_t)(m->clock_next - frame) < 0) {          /* (fell behind: from now) */
        m->clock_next = frame;
        m->clock_frac = 0;
    }
    while ((int32_t)(m->clock_next - (frame + (uint32_t)frames)) < 0) {
        put(m, m->clock_next, 0xF8, 0, 0, 1);
        m->clock_frac += frac;
        m->clock_next += whole + (m->clock_frac >> 16);
        m->clock_frac &= 0xFFFFu;
    }
}
