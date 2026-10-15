/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA screen and controls (see tuna_ui.h). Drawn with gfx.c (included before this file): Felucca's
 * canvas, Inter Tight, the Fukiai icons and the palettes, in Felucca's layout:
 *   header 0..24    transport, memory, tempo | a message | freeze, battery
 *   cards 28..72    four cards (57 x 44 at x 3 + 59 c): what KNOB 1..4 do on this page
 *   panel 76..198   the falling notes (HOME), or the page's picture; the library list while it is turned
 *   footer 202..240 the period's progress, the piece and its era, the page */
#include "tuna_ui.h"
#include "ui_icons.h"

#define TU_HEAD 24
#define TU_CARD_Y 28
#define TU_CARD_W 57
#define TU_CARD_H 44
#define TU_CARD_X(c) (3 + 59 * (int32_t)(c))
#define TU_PANEL_Y 76
#define TU_PANEL_H 122
#define TU_FOOT_Y 202
#define TU_FOOT_H 38
#define TU_HOT_MS 700
#define TU_BROWSE_MS 2500
#define TU_MENU_HOLD_MS 600

/* ---- small helpers ------------------------------------------------------------------------------------ */
static int tu_clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static int tu_wrap(int v, int n) { return n > 0 ? ((v % n) + n) % n : 0; }

/* strings, bounded (the firmware has no string library: libc.c) */
static void tu_cpy(char *d, const char *s, unsigned n)
{
    unsigned i = 0;
    while (s[i] && i + 1u < n) {
        d[i] = s[i];
        i++;
    }
    if (n)
        d[i] = 0;
}
static void tu_cat(char *d, const char *s, unsigned n)
{
    unsigned i = 0;
    while (i < n && d[i])
        i++;
    if (i < n)
        tu_cpy(d + i, s, n - i);
}

static void tu_fmt_int(char *d, int v)
{
    char t[12];
    int n = 0;
    unsigned u = v < 0 ? (unsigned)-v : (unsigned)v;
    do {
        t[n++] = (char)('0' + u % 10u);
        u /= 10u;
    } while (u);
    if (v < 0)
        *d++ = '-';
    while (n)
        *d++ = t[--n];
    *d = 0;
}
static void tu_fmt_signed(char *d, int v)
{
    if (v > 0)
        *d++ = '+';
    tu_fmt_int(d, v);
}
static uint32_t tu_hash(uint32_t h, const char *s)
{
    while (*s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h;
}
static uint32_t tu_rand(tuna_ui *u)
{
    u->rnd = u->rnd * 1103515245u + 12345u;
    return u->rnd >> 8;
}

/* sharps only: the fonts have capitals only (gfx.c fold), and EB would not read as E-flat */
static const char *const TU_NOTE_NAMES[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

/* ---- icons (the Fukiai cells of ui_icons.h; as icons.c draws them, without its parameter tables) ---------- */
static int32_t tu_icon_cell(uint32_t *size, uint32_t id)
{
    const uint8_t *idx = *size == 24u ? AI24_IDX : *size == 16u ? AI16_IDX : AI12_IDX;
    if (id >= ICON_COUNT)
        return -1;
    if (idx[id] == 0xFFu) {
        if (*size == 12u || AI12_IDX[id] == 0xFFu)
            return -1;
        *size = 12u;
        idx = AI12_IDX;
    }
    return idx[id];
}
/* the icon's ink centred up and down on the line cy */
static int32_t tu_icon_mid(int32_t x, int32_t cy, uint32_t size, uint32_t id, uint16_t fg, uint16_t bg)
{
    uint32_t sz = size;
    int32_t k = tu_icon_cell(&sz, id), y0, y1;
    if (k < 0)
        return 0;
    if (sz == 24u) {
        uint32_t v = AI24_INK[k];
        y0 = (int32_t)(v >> 8 & 255u);
        y1 = (int32_t)(v >> 24 & 255u);
    } else {
        uint32_t v = sz == 16u ? AI16_INK[k] : AI12_INK[k];
        y0 = (int32_t)(v >> 4 & 15u);
        y1 = (int32_t)(v >> 12 & 15u) + 1;
    }
    int32_t y = cy - HALF_UP(y1 - y0) - y0;
    cv_alpha_hc(x, y, sz, sz, sz == 24u ? AI24_DATA + AI24_OFF[k] : sz == 16u ? AI16_DATA + AI16_OFF[k] :
                AI12_DATA + AI12_OFF[k], AI_HC, ramp(fg, bg));         /* (Felucca 1.6: the cells Huffman-coded) */
    GFX_HOOK_TEXT(x, y + cv_oy, x + (int32_t)sz, y + cv_oy + (int32_t)sz, "icon", 4u);
    return (int32_t)sz;
}

/* ---- names ------------------------------------------------------------------------------------------- */
static const char *const TU_MEL_NAMES[] = {"PHRASE", "CHANT", "TINTI", "PATTERN", "BIRDS"};
/* (a card's value fits 49 px: in M, or in S; the long names shortened as Felucca's are) */
static const char *const TU_BASS_NAMES[] = {"FREE", "ROOT", "WALK", "BAR", "MARCH", "PEDAL", "DRONE", "OSTIN",
                                         "LINE", "REPEAT", "TOLL", "BEAT", "NONE", "808"};
static const char *const TU_PAD_NAMES[] = {"BLOCK", "ALBERTI", "CNTR", "BROKEN", "AFTER", "PARAL", "DOUBLE",
                                        "REPEAT", "ARP", "OOMPAH", "TREM", "ORGNM", "TINTI", "CANON",
                                        "FIGURE", "ADDIT", "PHASE", "NONE", "RHYTHM"};
static const char *const TU_HARM_NAMES[] = {"PLAN", "CYCLE", "DRONE"};
static const char *const TU_TUNE_NAMES[TT_TUNE_COUNT] = {"EQUAL", "MEAN", "WERCK", "PYTH", "JUST", "USER"};
static const char *const TU_SCALE_SHORT[TS_COUNT] = {"MAJ", "DOR", "PHRY", "LYD", "MIX", "MIN", "LOC", "HARM", "MEL",
                                                  "PEN", "MPEN", "BLUES", "WHOLE", "DIMHW", "DIMWH", "MESS3",
                                                  "ACOUS", "HUNG", "DHARM", "PDOM"};
static const char *const TU_PERC_NAMES[] = {"OFF", "PERIOD", "KIT", "PATTERN", "GROOVE"};
static const char *const TU_DEJA_NAMES[TA_DV_COUNT] = {"OFF", "OWN", "LOCK"};
static const char *const TU_PAGE_NAMES[TU_PAGES] = {"HOME", "LIBRARY", "KEY", "TIME", "PARTS", "EVOLVE", "FREEZE",
                                                 "MIX", "REVERB", "SAVE", "LOAD", "MENU", "MIDI", "RESONATOR",
                                                 "CHAOS"};
const char *tu_page_name(int page) { return page >= 0 && page < TU_PAGES ? TU_PAGE_NAMES[page] : ""; }
#define TU_NAMES_N(a) ((int)(sizeof a / sizeof a[0]))

/* modes a part can take over any composer; the ones that need the composer's own material (a pattern, a
 * figure, a line, a canon) only where the composer has them, i.e. as its own setting */
static bool tu_mode_ok(const tg_gen *g, int kind, int m)
{
    const tg_style *st = g->style;
    if (!st)
        return false;
    switch (kind) {
    case 0: return m == TG_MEL_PATTERN ? st->mel_mode == m : m >= 0 && m < TU_NAMES_N(TU_MEL_NAMES);
    case 1: return m == TG_BASS_MARBLES ? false : m == TG_BASS_OSTINATO || m == TG_BASS_LINE ? st->bass_mode == m
                 : m == TG_BASS_808 ? st->groove || st->bass_rhythm
                                                                                             : m >= 0 && m < TU_NAMES_N(TU_BASS_NAMES);
    case 2: return (m >= TG_PAD_CANON && m <= TG_PAD_PHASE) || m == TG_PAD_RHYTHM ? st->pad_mode == m
                                                                     : m >= 0 && m < TU_NAMES_N(TU_PAD_NAMES);
    case 3: return m == TG_HARM_CYCLE ? st->cycle_len > 0 : m >= 0 && m < TU_NAMES_N(TU_HARM_NAMES);
    }
    return false;
}
static int tu_mode_step(const tg_gen *g, int kind, int m, int d)
{
    int n = kind == 0 ? TU_NAMES_N(TU_MEL_NAMES) : kind == 1 ? TU_NAMES_N(TU_BASS_NAMES) : kind == 2 ? TU_NAMES_N(TU_PAD_NAMES)
                                                                                       : TU_NAMES_N(TU_HARM_NAMES);
    for (int i = 0; i < n; ++i) {
        m = tu_wrap(m + (d > 0 ? 1 : -1), n);
        if (tu_mode_ok(g, kind, m))
            return m;
    }
    return m;
}

/* a chord's name: root and quality from its tones */
static void tu_chord_name(char *d, const tt_chord *c, int key_root)
{
    int root = (c->root + key_root) % 12;
    uint16_t m = 0;
    for (int k = 0; k < c->n; ++k)
        m |= (uint16_t)(1u << (c->tone[k] % 12));
    const char *q = "";
    bool third = m & (1u << 4), mthird = m & (1u << 3), fifth = m & (1u << 7), dfifth = m & (1u << 6),
         afifth = m & (1u << 8), sev = m & (1u << 10), maj7 = m & (1u << 11), six = m & (1u << 9);
    /* (capitals only: minor spelled out, as "DM" would read as D major) */
    if (mthird && dfifth)
        q = six ? " DIM7" : sev ? " HALF-DIM" : " DIM";
    else if (third && afifth)
        q = sev ? " AUG7" : " AUG";
    else if (mthird)
        q = maj7 ? " MIN MAJ7" : sev ? " MIN7" : six ? " MIN6" : " MIN";
    else if (third)
        q = maj7 ? " MAJ7" : sev ? "7" : six ? "6" : "";
    else if (m & (1u << 5))
        q = sev ? "7 SUS4" : " SUS4";
    else if (m & (1u << 2) && fifth)
        q = " SUS2";
    else if (fifth && c->n <= 2)
        q = "5";
    tu_cpy(d, TU_NOTE_NAMES[root], 24);
    tu_cat(d, q, 24);
}

/* ---- the cards -------------------------------------------------------------------------------------- */
typedef struct {
    const char *label;
    char val[16];
    int32_t ratio;                        /* 0..1000 gauge, -1 none */
    bool bipolar;                         /* the gauge from its middle (the chaos knobs) */
    bool off;                             /* nothing to turn here */
    bool name;                            /* a name: may be ellipsised */
} tu_card;

static int tu_chaos_val(float v) { return (int)(v * 100.0f + 0.5f); }

static void tu_card_chaos(tu_card *c, const char *label, float v)
{
    c->label = label;
    int x = tu_chaos_val(v) - 50;
    tu_fmt_signed(c->val, x);
    c->ratio = tu_chaos_val(v) * 10;
    c->bipolar = true;
}
static void tu_card_text(tu_card *c, const char *label, const char *v, int32_t ratio)
{
    c->label = label;
    tu_cpy(c->val, v, sizeof c->val);
    c->ratio = ratio;
}
static void tu_card_int(tu_card *c, const char *label, int v, int lo, int hi)
{
    c->label = label;
    tu_fmt_int(c->val, v);
    c->ratio = hi > lo ? (v - lo) * 1000 / (hi - lo) : -1;
}

static void tu_page_cards(tuna_ui *u, tu_card *c)
{
    tuna_app *a = u->app;
    tg_gen *g = &a->g;
    ta_edit e;
    ta_edit_get(a, &e);
    memset(c, 0, 4 * sizeof *c);
    for (int i = 0; i < 4; ++i) {
        c[i].label = "";
        c[i].ratio = -1;
    }
    switch (u->page) {
    case TU_PG_HOME:
        tu_card_chaos(&c[0], "RHYTHM", g->chaos_rhythm);
        tu_card_chaos(&c[1], "PITCH", g->chaos_pitch);
        tu_card_chaos(&c[2], "DENSITY", g->chaos_density);
        tu_card_chaos(&c[3], "MACRO", g->chaos_macro);
        break;
    case TU_PG_LIB: {
        int item = a->pending >= 0 ? a->pending : a->item, gr = item >= 0 ? ta_lib_group(item) : 0;
        char t[8];
        tu_fmt_int(t, gr);
        tu_card_text(&c[0], "ERA", gr == 0 ? "TUNA" : t, gr * 1000 / (TA_GROUPS - 1));
        tu_card_text(&c[1], "PIECE", item >= 0 ? ta_lib_name(item) : "SLOT", -1);
        c[1].name = true;
        tu_card_int(&c[2], "SEED", a->seed_req >= 0 ? a->seed_req : (int)(a->seed % TA_SEEDS), 0, -1);
        c[2].ratio = -1;
        /* the composer's manner: chosen, or the seed's (AUTO) */
        int st = item >= 0 ? ta_lib_style(item) : -1, nm = tg_manner_count(st);
        if (nm > 1 && item == a->item) {
            tu_card_text(&c[3], a->manner_fix < 0 ? "AUTO" : "MANNER", ta_manner_name(a),
                         (g->manner + 1) * 1000 / nm);
            c[3].name = true;
        } else if (nm > 1) {
            tu_card_text(&c[3], "MANNER", a->manner_fix < 0 ? "AUTO" : tg_manner_name(st, a->manner_fix), -1);
        } else {
            c[3].off = true;
        }
        break;
    }
    case TU_PG_KEY: {
        int root = g->root_req >= 0 ? g->root_req : g->root;
        tu_card_text(&c[0], "ROOT", TU_NOTE_NAMES[root], root * 1000 / 11);
        tu_card_text(&c[1], "SCALE", TU_SCALE_SHORT[e.scale], e.scale * 1000 / (TS_COUNT - 1));
        tu_card_text(&c[2], "TUNE", TU_TUNE_NAMES[g->tuning], g->tuning * 1000 / (TT_TUNE_COUNT - 1));
        tu_card_text(&c[3], "DEJA", TU_DEJA_NAMES[a->deja], a->deja * 500);
        break;
    }
    case TU_PG_TIME:
        tu_card_text(&c[0], "METER", ta_meter_name(e.meter), e.meter < 0 ? -1 : e.meter * 1000 / (TA_M_COUNT - 1));
        tu_card_int(&c[1], "SWING", (int)(g->swing * 100.0f + 0.5f), 0, 100);
        tu_card_int(&c[2], "HUMAN", (int)(g->humanize * 100.0f + 0.5f), 0, 100);
        tu_card_text(&c[3], "PERC", TU_PERC_NAMES[tu_clampi(g->perc_mode, 0, 4)], g->perc_mode * 250);
        break;
    case TU_PG_PARTS:
        if (!g->style) {                          /* a character preset: Marbles' own parts */
            for (int i = 0; i < 4; ++i)
                c[i].off = true;
            break;
        }
        tu_card_text(&c[0], "MELODY", TU_MEL_NAMES[tu_clampi(e.mel, 0, TU_NAMES_N(TU_MEL_NAMES) - 1)], -1);
        tu_card_text(&c[1], "BASS", TU_BASS_NAMES[tu_clampi(e.bass, 0, TU_NAMES_N(TU_BASS_NAMES) - 1)], -1);
        tu_card_text(&c[2], "PAD", TU_PAD_NAMES[tu_clampi(e.pad, 0, TU_NAMES_N(TU_PAD_NAMES) - 1)], -1);
        tu_card_text(&c[3], "HARM", TU_HARM_NAMES[tu_clampi(e.harm, 0, TU_NAMES_N(TU_HARM_NAMES) - 1)], -1);
        break;
    case TU_PG_CHAOS: {
        static const char *const L[3] = {"MELODY", "BASS", "PAD"};
        static const uint8_t part[3] = {TG_PART_MELODY, TG_PART_BASS, TG_PART_PAD};
        for (int i = 0; i < 3; ++i) {
            bool on = (g->chaos_parts >> part[i]) & 1u;
            tu_card_text(&c[i], L[i], on ? "ON" : "OFF", on ? 1000 : 0);
        }
        c[3].off = true;
        break;
    }
    case TU_PG_EVOLVE:
        tu_card_text(&c[0], "EVOLVE", g->evolve ? "ON" : "OFF", g->evolve ? 1000 : 0);
        tu_card_int(&c[1], "LEN", g->evolve_periods, 2, 16);
        tu_card_int(&c[2], "DEPTH", (int)(g->evolve_depth * 100.0f + 0.5f), 0, 100);
        tu_card_text(&c[3], "MOD", g->evolve_modulate ? "ON" : "OFF", g->evolve_modulate ? 1000 : 0);
        break;
    case TU_PG_SEQ:
        tu_card_text(&c[0], "STATE", g->frozen ? (g->unfreeze_req ? "ENDING" : "LOOP") : g->freeze_req ? "WAIT" : "LIVE", -1);
        tu_card_int(&c[1], "NOTES", g->frozen ? g->pat.n : 0, 0, -1);
        c[1].ratio = g->frozen ? g->pat.n * 1000 / TG_PAT_MAX : -1;
        tu_card_int(&c[2], "LOCKS", g->nsteps, 0, TG_STEPS);
        c[3].off = true;
        if (g->frozen)
            tu_card_text(&c[3], "TRANSP", "+/-", -1), c[3].off = false;
        break;
    case TU_PG_MIX: {
        static const char *const L[4] = {"BASS", "MELODY", "PAD", "DRUMS"};
        for (int i = 0; i < 4; ++i)
            tu_card_int(&c[i], L[i], a->level[i], 0, 100);
        break;
    }
    case TU_PG_FX:
        tu_card_int(&c[0], "SEND", a->reverb, 0, 100);
        tu_card_int(&c[1], "SIZE", a->rev_size, 0, 100);
        tu_card_int(&c[2], "DAMP", a->rev_damp, 0, 100);
        tu_card_text(&c[3], "TYPE", ta_reverb_type(a->rev_type)->card, -1);
        break;
    case TU_PG_SAVE:
    case TU_PG_LOAD: {
        char t[8];
        tu_fmt_int(t, u->slot + 1);
        tu_card_text(&c[0], "SLOT", t, u->slot * 1000 / (TP_SLOTS - 1));
        bool used = (u->slots_used >> u->slot) & 1u;
        tu_card_text(&c[1], "NAME", used ? u->slot_names[u->slot] : "EMPTY", -1);
        c[1].name = true;
        c[2].off = c[3].off = true;
        break;
    }
    case TU_PG_MIDI:
        tu_card_text(&c[0], "OUT", a->midi.on ? "ON" : "OFF", a->midi.on ? 1000 : 0);
        tu_card_text(&c[1], "SOUND", a->local ? "ON" : "OFF", a->local ? 1000 : 0);
        tu_card_text(&c[2], "CLOCK", a->midi.clock ? "ON" : "OFF", a->midi.clock ? 1000 : 0);
        tu_card_text(&c[3], "BEND", a->midi.bend ? "ON" : "OFF", a->midi.bend ? 1000 : 0);
        break;
    case TU_PG_RESO: {
        static const char *const TN[TR_TUNE_COUNT] = {"CHORD", "KEY", "ROOT"};
        tu_card_int(&c[0], "AMOUNT", a->reso_amount, 0, 100);
        tu_card_int(&c[1], "DECAY", a->reso_decay, 0, 100);
        tu_card_int(&c[2], "BRIGHT", a->reso_bright, 0, 100);
        tu_card_text(&c[3], "TUNE", TN[a->reso_mode % TR_TUNE_COUNT], a->reso_mode * 500);
        break;
    }
    case TU_PG_MENU:
        tu_card_text(&c[0], "COLOR", UI_PALETTES[u->palette % NPALETTES].name, u->palette * 1000 / (NPALETTES - 1));
        tu_card_text(&c[1], "STYLE", u->style ? "LINE" : "FLAT", u->style * 1000);
        c[2].off = c[3].off = true;
        break;
    }
}

static void tu_draw_card(tuna_ui *u, int i, const tu_card *c)
{
    bool hot = u->hot == i && u->hot_ms > 0;
    uint16_t lc = hot ? T_ACCENT : T_MID, vc = hot ? T_ACCENT : T_THEME;
    uint32_t sig = tu_hash(2166136261u + (uint32_t)i * 7u + ux.gen * 131u + (uint32_t)hot * 3u + (uint32_t)u->page * 977u,
                           c->label);
    sig = tu_hash(sig, c->val) + (uint32_t)(c->ratio + 1) * 31u + (uint32_t)c->off * 5u + (uint32_t)c->name * 11u;
    if (!u->force && sig == u->sig_card[i])
        return;
    u->sig_card[i] = sig;
    cv_begin(TU_CARD_W, TU_CARD_H, T_BG);
    cv_rrect(0, 0, TU_CARD_W, TU_CARD_H, 4, T_SURF, T_BG);
    if (!c->off && c->label[0]) {
        cv_text_on(5, 3, &AF_S, c->label, lc, T_SURF);
        const aafont_t *vf = text_w(&AF_M, c->val) <= TU_CARD_W - 8 ? &AF_M : &AF_S;
        int32_t vy = vf == &AF_M ? 17 : 19;
        if (c->name)
            cv_free_text(5, vy, vf, c->val, vc, T_SURF, TU_CARD_W - 8);
        else
            cv_text_fit(5, vy, vf, c->val, vc, T_SURF, TU_CARD_W - 8);
        if (c->ratio >= 0) {
            int32_t w = TU_CARD_W - 10, x0 = 5, y = 37;
            cv_rect(x0, y, w, 3, T_LINE);
            int32_t r = tu_clampi(c->ratio, 0, 1000) * w / 1000;
            if (c->bipolar) {
                int32_t mid = x0 + w / 2;
                if (r >= w / 2)
                    cv_rect(mid, y, r - w / 2 + 1, 3, vc);
                else
                    cv_rect(x0 + r, y, mid - x0 - r + 1, 3, vc);
                cv_rect(mid, y - 2, 1, 7, hot ? T_ACCENT : T_MID);   /* the composer's own value */
            } else {
                cv_rect(x0, y, r > 0 ? r : 1, 3, vc);
            }
        }
    } else if (c->off) {
        cv_rect(TU_CARD_W / 2 - 6, TU_CARD_H / 2, 12, 1, T_DIM);
    }
    cv_blit((uint32_t)TU_CARD_X(i), TU_CARD_Y);
}

/* ---- the header -------------------------------------------------------------------------------------- */
static void tu_draw_head(tuna_ui *u)
{
    tuna_app *a = u->app;
    tg_gen *g = &a->g;
    char b[40];
    int bpm = (int)(g->bpm + 0.5f);
    uint32_t sig = (uint32_t)a->playing * 3u + (uint32_t)a->deja * 7u + (uint32_t)bpm * 101u + ux.gen * 13u +
                   (uint32_t)g->frozen * 17u + (uint32_t)g->freeze_req * 19u + (uint32_t)u->batt * 7777u +
                   (uint32_t)(a->pending + 2) * 3313u + (uint32_t)(u->hot == TU_K_SELECT && u->hot_ms > 0) * 23u +
                   (u->msg_ms > 0 ? tu_hash(29u, u->msg) : 0u);
    if (!u->force && sig == u->sig_head)
        return;
    u->sig_head = sig;
    cv_begin(240, TU_HEAD, T_BG);
    tu_icon_mid(8, TU_HEAD / 2, 16, a->playing ? ICON_X_PLAY : ICON_X_STOP, a->playing ? T_THEME : T_MID, T_BG);
    if (a->deja == TA_DV_LOCK)
        tu_icon_mid(28, TU_HEAD / 2, 16, ICON_X_LOCK, T_REC, T_BG);
    tu_icon_mid(54, TU_HEAD / 2, 16, ICON_TEMPO, T_MID, T_BG);
    tu_fmt_int(b, bpm);
    cv_text_on(72, CAP_IN(M, TU_HEAD), &AF_M, b, u->hot == TU_K_SELECT && u->hot_ms > 0 ? T_ACCENT : T_THEME, T_BG);
    if (u->msg_ms > 0) {
        cv_free_text(106, CAP_IN(S, TU_HEAD), &AF_S, u->msg, T_TEXT, T_BG, 186 - 106);
    } else if (a->pending >= 0) {
        int32_t x = cv_text_on(106, CAP_IN(S, TU_HEAD), &AF_S, "NEXT", T_ACCENT, T_BG) + 5;
        cv_free_text(x, CAP_IN(S, TU_HEAD), &AF_S, ta_lib_name(a->pending), T_TEXT, T_BG, 186 - x);
    }
    if (g->frozen || g->freeze_req)
        tu_icon_mid(190, TU_HEAD / 2, 16, ICON_X_FREEZE, g->frozen ? T_THEME : T_MID, T_BG);
    {
        static const uint8_t BAT[5] = {ICON_X_BAT0, ICON_X_BAT1, ICON_X_BAT3, ICON_X_BAT4, ICON_X_BAT_CHG};
        int lv = tu_clampi(u->batt, 0, 4);
        tu_icon_mid(214, TU_HEAD / 2, 24, BAT[lv], lv == 1 ? T_ACCENT : lv == 0 ? T_MID : T_THEME, T_BG);
    }
    cv_blit(0, 0);
}

/* ---- the footer -------------------------------------------------------------------------------------- */
static void tu_draw_foot(tuna_ui *u)
{
    tuna_app *a = u->app;
    tg_gen *g = &a->g;
    int period = 1, pos = tg_period_position(g, &period);
    int filled = a->playing && period > 0 ? (pos * 16) / period : 0;
    int item = a->item;
    const char *name = item >= 0 ? ta_lib_name(item) : "SLOT";
    const char *manner = ta_manner_name(a);       /* (a composer's manner where it has them, else the era) */
    const char *era = manner[0] ? manner : item >= 0 ? ta_group_name(ta_lib_group(item)) : "";
    uint32_t sig = tu_hash(tu_hash((uint32_t)filled * 7u + (uint32_t)a->playing * 3u + ux.gen * 11u + (uint32_t)u->page * 13u,
                                   name), era);
    if (!u->force && sig == u->sig_foot)
        return;
    u->sig_foot = sig;
    cv_begin(240, TU_FOOT_H, T_BG);
    for (int i = 0; i < 16; ++i) {                /* the period: 16 dashes, the part heard so far lit */
        int32_t x = 10 + i * 14;
        cv_rect(x, 6, 10, 2, i < filled ? T_THEME : i == filled && a->playing ? T_ACCENT : T_LINE);
    }
    cv_free_text(10, 17, &AF_M, name, T_THEME, T_BG, 130);
    int32_t nx = 10 + text_w(&AF_M, name);
    if (nx > 140) nx = 140;
    int32_t px = 230 - text_w(&AF_S, tu_page_name(u->page)) - 8;   /* the page's name at the right */
    if (era[0] && px - nx - 6 > 24)
        cv_free_text(nx + 6, 17 + AF_M_CAP_Y + AF_M_CAP_H - AF_S_CAP_Y - AF_S_CAP_H, &AF_S, era, T_MID, T_BG, px - nx - 6);
    cv_text_r(230, 17 + AF_M_CAP_Y + AF_M_CAP_H - AF_S_CAP_Y - AF_S_CAP_H, &AF_S, tu_page_name(u->page), T_MID, T_BG);
    cv_blit(0, TU_FOOT_Y);
}

/* ---- the panels ---------------------------------------------------------------------------------------- */
static uint16_t tu_part_color(int part)
{
    switch (part) {
    case TG_PART_MELODY: return T_ACCENT;
    case TG_PART_BASS: return T_THEME;
    case TG_PART_PAD: return T_TEXT;
    default: return T_MID;
    }
}

static void tu_panel_begin(void)
{
    cv_begin(240, TU_PANEL_H, T_BG);
    cv_rrect(3, 0, 234, TU_PANEL_H, 4, T_SURF, T_BG);
}

/* HOME: the notes falling, the chord and the key on top */
static void tu_panel_home(tuna_ui *u)
{
    tuna_app *a = u->app;
    tg_gen *g = &a->g;
    for (int i = 0; i < TU_PARTICLES; ++i) {
        tu_particle *p = &u->pt[i];
        if (!p->life)
            continue;
        uint16_t c = tu_part_color(p->part);
        if (p->life < 64)
            c = ux_mix(T_SURF, c, p->life * 100 / 64);
        int32_t x = p->x >> 4, y = p->y >> 4, s = p->size;
        if (p->vy > 0 && p->life >= 64) {         /* a trail while falling, longer the faster */
            int32_t tl = 3 + p->vy / 4;
            cv_rect(x + s / 2, y - tl, 1, tl / 2, ux_mix(T_SURF, c, 25));
            cv_rect(x + s / 2, y - tl / 2, 1, tl / 2, ux_mix(T_SURF, c, 55));
        }
        cv_rrect(x, y, s, s, s > 4 ? 2 : 1, c, T_SURF);
    }
    cv_rect(8, TU_PANEL_H - 3, 224, 1, T_LINE);    /* the floor they land on */
    char t[24];
    if (a->playing) {
        tu_chord_name(t, &g->chord, g->root);
        cv_text_on(10, 6, &AF_M, t, T_TEXT, T_SURF);
    }
    tu_cpy(t, TU_NOTE_NAMES[g->root], sizeof t);
    tu_cat(t, " ", sizeof t);
    tu_cat(t, tt_scale_name(g->scale), sizeof t);
    cv_text_r(230, 6 + AF_M_CAP_Y + AF_M_CAP_H - AF_S_CAP_Y - AF_S_CAP_H, &AF_S, t, T_MID, T_SURF);
    if (!a->playing) {
        GFX_HOOK_ALIGN(3, 0, 237, TU_PANEL_H, AL_H, "home: press play");
        int32_t w = kc_w(KC_PLAY) + 5 + text_w(&AF_S, "TO START");
        int32_t x = 3 + HALF_UP(234 - w);
        cv_keycap(x, TU_PANEL_H / 2 - KC_H / 2, KC_PLAY, T_KEY, T_INK, T_SURF);
        cv_text_on(x + kc_w(KC_PLAY) + 5, TU_PANEL_H / 2 + CAP_IN(S, 0), &AF_S, "TO START", T_MID, T_SURF);
    }
}

/* the library list (the PRESETS knob, the LIBRARY page) */
static void tu_panel_browse(tuna_ui *u)
{
    tuna_app *a = u->app;
    int item = u->browse, gr = ta_lib_group(item), first = ta_lib_first(gr);
    int n = (gr + 1 < TA_GROUPS ? ta_lib_first(gr + 1) : ta_lib_count()) - first;
    char t[16], c[8];
    cv_text_on(10, 4, &AF_S, ta_group_name(gr), T_THEME, T_SURF);
    tu_fmt_int(t, item - first + 1);
    tu_cat(t, "/", sizeof t);
    tu_fmt_int(c, n);
    tu_cat(t, c, sizeof t);
    cv_text_r(230, 4, &AF_S, t, T_MID, T_SURF);
    const int rows = 5, rh = 20, y0 = 20;
    int top = item - first - rows / 2;
    top = tu_clampi(top, 0, n > rows ? n - rows : 0);
    for (int r = 0; r < rows && top + r < n; ++r) {
        int it = first + top + r;
        int32_t y = y0 + r * rh;
        bool sel = it == item;
        uint16_t bg = sel ? T_SEL : T_SURF, fg = sel ? T_INK : T_TEXT;
        if (sel)
            cv_rrect(6, y, 228, rh - 2, 3, T_SEL, T_SURF);
        if (it == a->item)
            tu_icon_mid(10, y + (rh - 2) / 2, 16, a->playing ? ICON_X_PLAY : ICON_X_STOP, sel ? T_INK : T_THEME, bg);
        cv_free_text(30, y + CAP_IN(M, rh - 2), &AF_M, ta_lib_name(it), fg, bg, 150);
        if (it == a->pending) {
            cv_text_r(228, y + CAP_IN(S, rh - 2), &AF_S, "NEXT", sel ? T_INK : T_ACCENT, bg);
        } else if (ta_lib_born(it)) {
            tu_fmt_int(t, ta_lib_born(it));
            cv_text_r(228, y + CAP_IN(S, rh - 2), &AF_S, t, sel ? T_INK : T_MID, bg);
        }
    }
}

/* KEY: one octave of keys, the scale's notes marked, the tonic and the chord sounding */
static void tu_panel_key(tuna_ui *u)
{
    tuna_app *a = u->app;
    tg_gen *g = &a->g;
    ta_edit e;
    ta_edit_get(a, &e);
    uint16_t mask = tt_scale_mask(e.scale);
    int root = g->root_req >= 0 ? g->root_req : g->root;
    uint16_t chord = 0;
    for (int k = 0; k < g->chord.n; ++k)
        chord |= (uint16_t)(1u << ((g->chord.root + g->chord.tone[k] + g->root) % 12));
    static const int8_t white[7] = {0, 2, 4, 5, 7, 9, 11}, black[5] = {1, 3, 6, 8, 10};
    static const int8_t bx[5] = {1, 2, 4, 5, 6};             /* a black key right of white key bx - 1 */
    const int32_t x0 = 15, kw = 30, ky = 24, kh = 70;
    for (int i = 0; i < 7; ++i) {
        int pc = white[i];
        bool in = (mask >> tu_wrap(pc - root, 12)) & 1u;
        uint16_t f = pc == root ? T_ACCENT : in ? T_THEME : T_RAISE;
        cv_rrect(x0 + i * kw, ky, kw - 2, kh, 3, f, T_SURF);
        if (a->playing && ((chord >> pc) & 1u))
            cv_rrect(x0 + i * kw + kw / 2 - 5, ky + kh - 14, 8, 8, 4, T_INK, f);
    }
    for (int i = 0; i < 5; ++i) {
        int pc = black[i];
        bool in = (mask >> tu_wrap(pc - root, 12)) & 1u;
        uint16_t f = pc == root ? T_ACCENT : in ? T_SEL : T_DIM;
        int32_t x = x0 + bx[i] * kw - 9;
        cv_rrect(x, ky, 16, 42, 3, f, T_SURF);
        if (a->playing && ((chord >> pc) & 1u))
            cv_rrect(x + 4, ky + 30, 8, 8, 4, T_INK, f);
    }
    char t[32];
    tu_cpy(t, TU_NOTE_NAMES[root], sizeof t);
    tu_cat(t, " ", sizeof t);
    tu_cat(t, tt_scale_name(e.scale), sizeof t);
    cv_free_text(15, 4, &AF_S, t, T_TEXT, T_SURF, 140);
    cv_text_r(228, 4, &AF_S, TU_TUNE_NAMES[g->tuning], T_MID, T_SURF);
    cv_text_on(15, 100, &AF_S, "KEYS: TAP A ROOT, HOLD A CHORD", T_MID, T_SURF);
}

/* TIME and PARTS: the meter as big figures, or what each part does */
static void tu_panel_time(tuna_ui *u)
{
    tuna_app *a = u->app;
    tg_gen *g = &a->g;
    ta_edit e;
    ta_edit_get(a, &e);
    const char *m = e.meter >= 0 ? ta_meter_name(e.meter) : "OWN";
    char t[24];
    if (e.meter < 0) {                            /* the composer's own grouping, spelled out */
        t[0] = 0;
        for (int i = 0; i < TG_METER_MAX && g->meter[i]; ++i) {
            char n[4];
            tu_fmt_int(n, g->meter[i]);
            if (i) tu_cat(t, "+", sizeof t);
            tu_cat(t, n, sizeof t);
        }
        m = t;
    }
    GFX_HOOK_ALIGN(3, 0, 237, 0, AL_H, "time: the meter");
    cv_text_in(3, 22, 234, &AF_L, m, T_THEME, T_SURF);
    /* the bar's beats as dots, the beat now lit */
    int bars, cyc = tg_meter_cycle(g, &bars);
    tg_meter_pos mp;
    tg_meter_at(g, g->ticks > 0 ? g->ticks - 1 : 0, &mp);
    int n = cyc / (g->division > 0 ? g->division : 1);
    if (g->meter[0]) {
        n = 0;
        while (n < TG_METER_MAX && g->meter[n]) n++;
    }
    n = tu_clampi(n, 1, 12);
    int32_t w = n * 16, x0 = 3 + HALF_UP(234 - w);
    int now = g->meter[0] ? mp.beat : (int)((g->ticks > 0 ? g->ticks - 1 : 0) % cyc) / (g->division > 0 ? g->division : 1);
    for (int i = 0; i < n; ++i)
        cv_rrect(x0 + i * 16 + 3, 72, 10, 10, 5, a->playing && i == now % n ? T_ACCENT : i == 0 ? T_THEME : T_RAISE, T_SURF);
    if (a->req_on)
        cv_text_in(3, 96, 234, &AF_S, "AT THE PERIOD'S END", T_ACCENT, T_SURF);
}

static void tu_panel_parts(tuna_ui *u)
{
    tuna_app *a = u->app;
    tg_gen *g = &a->g;
    if (!g->style) {
        cv_text_in(3, 50, 234, &AF_S, "A CHARACTER PRESET: MARBLES PLAYS", T_MID, T_SURF);
        return;
    }
    static const char *const P[4] = {"BASS", "MELODY", "PAD", "DRUMS"};
    static const uint8_t part[4] = {TG_PART_BASS, TG_PART_MELODY, TG_PART_PAD, TG_PART_DRUMS};
    for (int i = 0; i < 4; ++i) {
        int32_t y = 8 + i * 24;
        cv_rrect(10, y + 4, 8, 8, 4, tu_part_color(part[i]), T_SURF);
        cv_text_on(24, y + CAP_IN(S, 16), &AF_S, P[i], T_MID, T_SURF);
        const char *fam = i < TG_PARTS && g->orch_cur[part[i]] >= 0 ? tg_family_name(g->orch_cur[part[i]]) : i == 3 ? TU_PERC_NAMES[tu_clampi(g->perc_mode, 0, 4)] : "";
        cv_free_text(90, y + CAP_IN(M, 16), &AF_M, fam, T_TEXT, T_SURF, 140);
    }
    if (a->req_on)
        cv_text_in(3, 104, 234, &AF_S, "AT THE PERIOD'S END", T_ACCENT, T_SURF);
}

/* CHAOS: which parts the HOME knobs move, and how far they are turned */
static void tu_panel_chaos(tuna_ui *u)
{
    tg_gen *g = &u->app->g;
    static const char *const P[3] = {"MELODY", "BASS", "PAD"};
    static const uint8_t part[3] = {TG_PART_MELODY, TG_PART_BASS, TG_PART_PAD};
    for (int i = 0; i < 3; ++i) {
        int32_t y = 10 + i * 26;
        bool on = (g->chaos_parts >> part[i]) & 1u;
        cv_rrect(10, y + 4, 8, 8, 4, on ? tu_part_color(part[i]) : T_DIM, T_SURF);
        cv_text_on(24, y + CAP_IN(S, 16), &AF_S, P[i], T_MID, T_SURF);
        cv_text_on(100, y + CAP_IN(M, 16), &AF_M, on ? "THE KNOBS" : "AS WRITTEN", on ? T_TEXT : T_DIM, T_SURF);
    }
    cv_text_in(3, 92, 234, &AF_S, "HOME: RHYTHM PITCH DENSITY MACRO", T_MID, T_SURF);
}

/* EVOLVE: the energy arc over the cycle, the period now */
static void tu_panel_evolve(tuna_ui *u)
{
    tg_gen *g = &u->app->g;
    int n = tu_clampi(g->evolve_periods, 2, 16);
    int now = (int)(g->period_index % n);
    int32_t w = 214 / n, x0 = 3 + HALF_UP(234 - w * n);
    for (int i = 0; i < n; ++i) {
        /* the arc: up through the cycle to its height two thirds in, then down */
        float t = (float)i / (float)(n - 1), e = t < 0.67f ? t / 0.67f : (1.0f - t) / 0.33f;
        e = 0.15f + 0.85f * e * (g->evolve ? g->evolve_depth : 0.0f);
        int32_t h = (int32_t)(e * 80.0f);
        uint16_t c = !g->evolve ? T_RAISE : i == now && u->app->playing ? T_ACCENT : T_THEME;
        cv_rrect(x0 + i * w + 1, 100 - h, w - 3, h, 2, c, T_SURF);
    }
    cv_text_on(10, 4, &AF_S, g->evolve ? (g->evolve_modulate ? "ARC AND MODULATION" : "ARC") : "OFF: THE COMPOSER AS IS",
               T_MID, T_SURF);
    cv_rect(10, 104, 220, 1, T_LINE);
}

/* FREEZE: the loop as a piano roll, the playhead; live, how to freeze */
static void tu_panel_seq(tuna_ui *u)
{
    tg_gen *g = &u->app->g;
    if (!g->frozen) {
        int32_t w = kc_w(KC_OCTUP) + 5 + text_w(&AF_S, g->freeze_req ? "WAITING FOR THE PERIOD'S END" : "FREEZE THIS PERIOD");
        int32_t x = 3 + HALF_UP(234 - w);
        cv_keycap(x, 50, KC_OCTUP, g->freeze_req ? T_DIM : T_KEY, T_INK, T_SURF);
        cv_text_on(x + kc_w(KC_OCTUP) + 5, 50 + HALF_UP(KC_H - AF_S_CAP_H) - AF_S_CAP_Y,
                   &AF_S, g->freeze_req ? "WAITING FOR THE PERIOD'S END" : "FREEZE THIS PERIOD", T_MID, T_SURF);
        return;
    }
    const tg_pattern *pt = &g->pat;
    int lo = 127, hi = 0;
    for (int i = 0; i < pt->n; ++i)
        if (pt->ev[i].part < TG_PART_DRUMS) {
            if (pt->ev[i].note < lo) lo = pt->ev[i].note;
            if (pt->ev[i].note > hi) hi = pt->ev[i].note;
        }
    if (hi < lo) { lo = 48; hi = 72; }
    int span = hi - lo + 1 < 12 ? 12 : hi - lo + 1;
    const int32_t x0 = 10, w = 220, y0 = 8, h = 96;
    int len = pt->len > 0 ? pt->len : 1;
    for (int i = 0; i < pt->n; ++i) {
        const tg_pnote *p = &pt->ev[i];
        int32_t x = x0 + p->tick * w / len, xw = p->len * w / len;
        int32_t y = p->part >= TG_PART_DRUMS ? y0 + h - 3 : y0 + h - 8 - (p->note - lo) * (h - 12) / span;
        cv_rect(x, y, xw > 2 ? xw - 1 : 2, 3, tu_part_color(p->part));
    }
    int32_t px = x0 + g->pat_pos * w / len;
    cv_rect(px, y0, 1, h, T_ACCENT);
    int32_t kx = cv_keycap(10, 106, KC_OCTUP, T_KEY, T_INK, T_SURF);
    cv_text_on(10 + kx + 4, 106 + HALF_UP(KC_H - AF_S_CAP_H) - AF_S_CAP_Y, &AF_S, g->unfreeze_req ? "ENDING" : "LIVE AGAIN", T_MID, T_SURF);
}

/* MIX: four level bars, what each part plays now */
static void tu_panel_mix(tuna_ui *u)
{
    tuna_app *a = u->app;
    tg_gen *g = &a->g;
    static const uint8_t part[4] = {TG_PART_BASS, TG_PART_MELODY, TG_PART_PAD, TG_PART_DRUMS};
    for (int i = 0; i < 4; ++i) {
        int32_t x = TU_CARD_X(i) + 8, h = a->level[i] * 80 / 100;
        cv_rrect(x + 14, 12, 12, 80, 3, T_RAISE, T_SURF);
        if (h > 0)
            cv_rrect(x + 14, 92 - h, 12, h, 3, tu_part_color(part[i]), T_RAISE);
        const char *fam = i < 3 && g->orch_cur[part[i]] >= 0 ? tg_family_name(g->orch_cur[part[i]]) : i == 3 ? "DRUMS" : "";
        GFX_HOOK_ALIGN(TU_CARD_X(i), 0, TU_CARD_X(i) + TU_CARD_W, 0, AL_H, "mix: the part's family");
        int32_t fw = text_w(&AF_S, fam);
        cv_free_text(TU_CARD_X(i) + 2 + (fw < TU_CARD_W - 4 ? ink_in(&AF_S, fam, TU_CARD_W - 4) : 0), 100, &AF_S, fam,
                     T_MID, T_SURF, TU_CARD_W - 4);
    }
}

/* REVERB: the send, the size, the damping as bars; the model below (Felucca's shared reverb bus) */
static void tu_panel_fx(tuna_ui *u)
{
    tuna_app *a = u->app;
    static const char *const L[3] = {"SEND", "SIZE", "DAMP"};
    const int v[3] = {a->reverb, a->rev_size, a->rev_damp};
    for (int i = 0; i < 3; ++i) {
        int32_t y = 6 + i * 28, w = 200 * v[i] / 100;
        cv_text_on(20, y, &AF_S, L[i], T_MID, T_SURF);
        cv_rrect(20, y + 14, 200, 8, 4, T_RAISE, T_SURF);
        if (w > 0)
            cv_rrect(20, y + 14, w < 8 ? 8 : w, 8, 4, T_THEME, T_RAISE);
    }
    cv_text_on(20, 92, &AF_M, ta_reverb_type(a->rev_type)->name, T_THEME, T_SURF);
}

/* SAVE and LOAD: the 16 slots as the 16 white keys, in 4 rows */
static void tu_panel_slots(tuna_ui *u)
{
    for (int s = 0; s < TP_SLOTS; ++s) {
        int r = s / 4, c = s % 4;
        int32_t x = 6 + c * 57, y = 4 + r * 24, w = 55, h = 22;
        bool used = (u->slots_used >> s) & 1u, sel = s == u->slot;
        uint16_t f = sel ? (u->confirm ? T_REC : T_SEL) : used ? T_RAISE : T_SURF;
        cv_rrect(x, y, w, h, 3, f, T_SURF);
        if (!sel && !used)
            cv_frame(x, y, w, h, T_LINE);
        char t[4];
        tu_fmt_int(t, s + 1);
        uint16_t fg = sel ? T_INK : used ? T_TEXT : T_DIM;
        cv_text_on(x + 3, y + CAP_IN(S, h), &AF_S, t, sel ? T_INK : T_MID, f);
        if (used)
            cv_free_text(x + 17, y + CAP_IN(S, h), &AF_S, u->slot_names[s], fg, f, w - 19);
    }
    const char *act = u->page == TU_PG_SAVE ? (u->confirm ? "AGAIN: REPLACE" : "SAVE") : "LOAD";
    int32_t kx = cv_keycap(6, 104, KC_OCTUP, T_KEY, T_INK, T_SURF);
    cv_text_on(6 + kx + 4, 104 + HALF_UP(KC_H - AF_S_CAP_H) - AF_S_CAP_Y, &AF_S, act, u->confirm ? T_REC : T_MID, T_SURF);
    cv_text_r(232, 104 + HALF_UP(KC_H - AF_S_CAP_H) - AF_S_CAP_Y, &AF_S, "WHITE KEYS: SLOT", T_DIM, T_SURF);
}

static void tu_panel_menu(tuna_ui *u)
{
    (void)u;
    for (int i = 0; i < 5; ++i) {                 /* the palette's colours */
        cv_rrect(20 + i * 42, 12, 34, 20, 3, T_LINE, T_SURF);
        cv_rrect(21 + i * 42, 13, 32, 18, 3, i == 0 ? T_BG : i == 1 ? T_SURF : i == 2 ? T_TEXT : i == 3 ? T_THEME : T_ACCENT,
                 T_LINE);
    }
    cv_text_on(20, 46, &AF_M, "TUNA", T_THEME, T_SURF);
    cv_text_on(20, 66, &AF_S, "GENERATIVE MUSIC ON FELUCCA", T_MID, T_SURF);
    cv_text_on(20, 82, &AF_S, "MARBLES: EMILIE GILLET (MIT)", T_MID, T_SURF);
    cv_text_on(20, 98, &AF_S, "GPL-3.0, HOME CLOSES", T_DIM, T_SURF);
}

/* MIDI: the parts, their channels, a light as each plays */
static void tu_panel_midi(tuna_ui *u)
{
    tuna_app *a = u->app;
    static const char *const P[5] = {"BASS", "MELODY", "PAD", "TIMPANI", "DRUMS"};
    static const uint8_t part[5] = {TG_PART_BASS, TG_PART_MELODY, TG_PART_PAD, TG_PART_TIMPANI, TG_PART_DRUMS};
    for (int i = 0; i < 5; ++i) {
        int32_t y = 6 + i * 21;
        int ch = a->midi.ch[part[i]] & 15;
        uint8_t act = a->midi.act[ch];
        uint16_t led = !a->midi.on ? T_DIM : act ? ux_mix(T_RAISE, tu_part_color(part[i]), 30 + act * 70 / 255) : T_RAISE;
        cv_rrect(12, y + 4, 10, 10, 5, led, T_SURF);
        cv_text_on(30, y + CAP_IN(S, 18), &AF_S, P[i], T_MID, T_SURF);
        char t[8];
        tu_cpy(t, "CH ", sizeof t);
        char n[4];
        tu_fmt_int(n, ch + 1);
        tu_cat(t, n, sizeof t);
        cv_text_on(110, y + CAP_IN(M, 18), &AF_M, t, a->midi.on ? T_THEME : T_DIM, T_SURF);
        if (i < 3 && a->g.orch_cur[part[i]] >= 0) {
            char gm[8];
            tu_cpy(gm, "PRG ", sizeof gm);
            tu_fmt_int(n, tmi_gm_program(a->g.orch_cur[part[i]]) + 1);
            tu_cat(gm, n, sizeof gm);
            cv_text_r(228, y + CAP_IN(S, 18), &AF_S, gm, T_MID, T_SURF);
        } else if (i == 4) {
            cv_text_r(228, y + CAP_IN(S, 18), &AF_S, "GM KIT", T_MID, T_SURF);
        }
    }
}

/* RESONATOR: its strings, each a line that shakes as it rings, its note at the left */
static void tu_panel_reso(tuna_ui *u)
{
    tuna_app *a = u->app;
    const tr_reso *r = &a->reso;
    for (int k = 0; k < TR_STRINGS; ++k) {
        int32_t y = 12 + k * 18;
        int note = r->note[k];
        uint16_t c = note < 0 || !a->reso_amount ? T_DIM : k & 1 ? T_THEME : T_ACCENT;
        if (note >= 0) {
            char t[8];
            tu_cpy(t, TU_NOTE_NAMES[note % 12], sizeof t);
            char o[4];
            tu_fmt_int(o, note / 12 - 1);
            tu_cat(t, o, sizeof t);
            cv_text_on(12, y - 6 + CAP_IN(S, 12), &AF_S, t, note < 0 ? T_DIM : T_MID, T_SURF);
        }
        int32_t amp = r->lp[k] < 0 ? -r->lp[k] : r->lp[k];
        amp = a->reso_amount ? (amp >> 8) / 64 : 0;       /* a sample of 64: 1 px, up to 6 px */
        if (amp > 6) amp = 6;
        int32_t prev = y;
        for (int32_t x = 48; x < 230; x += 2) {
            /* a standing wave: still at both ends, its amplitude in the middle */
            int32_t env = (x - 48) * (230 - x) * 1024 / ((230 - 48) * (230 - 48));   /* 0..256 */
            int32_t ph = (int32_t)((x * (note > 0 ? note : 40) / 8 + (int32_t)u->frame * 3) % 32);
            int32_t sw = ph < 16 ? ph - 8 : 24 - ph;              /* a triangle -8..8 */
            int32_t yy = y + sw * amp * env / (8 * 256);
            cv_line(x - 2, prev, x, yy, c);
            prev = yy;
        }
    }
}

static void tu_draw_panel(tuna_ui *u)
{
    tu_panel_begin();
    if (u->browse_ms > 0 || u->page == TU_PG_LIB) {
        tu_panel_browse(u);
    } else {
        switch (u->page) {
        case TU_PG_HOME: tu_panel_home(u); break;
        case TU_PG_KEY: tu_panel_key(u); break;
        case TU_PG_TIME: tu_panel_time(u); break;
        case TU_PG_PARTS: tu_panel_parts(u); break;
        case TU_PG_EVOLVE: tu_panel_evolve(u); break;
        case TU_PG_SEQ: tu_panel_seq(u); break;
        case TU_PG_MIX: tu_panel_mix(u); break;
        case TU_PG_FX: tu_panel_fx(u); break;
        case TU_PG_SAVE: case TU_PG_LOAD: tu_panel_slots(u); break;
        case TU_PG_MENU: tu_panel_menu(u); break;
        case TU_PG_MIDI: tu_panel_midi(u); break;
        case TU_PG_RESO: tu_panel_reso(u); break;
        case TU_PG_CHAOS: tu_panel_chaos(u); break;
        }
    }
    cv_blit(0, TU_PANEL_Y);
}

static void tu_background(void)
{
    cv_begin(240, 40, T_BG);                      /* the background, gaps and all */
    for (int y = 0; y < 240; y += 40)
        cv_blit(0, (uint32_t)y);
    if (ux.style == ST_LINE) {                    /* LINE: no cards, 1 px rules between the areas */
        cv_begin(240, 1, T_RULE);
        cv_blit(0, TU_HEAD + 1);
        cv_blit(0, TU_CARD_Y + TU_CARD_H + 1);
        cv_blit(0, TU_FOOT_Y - 3);
        cv_begin(1, TU_CARD_H, T_RULE);
        for (int c = 1; c < 4; ++c)
            cv_blit((uint32_t)(TU_CARD_X(c) - 2), TU_CARD_Y);
    }
}

void tu_draw(tuna_ui *u)
{
    tu_card c[4];
    if (u->force)                                 /* a page, a palette, a style: everything again */
        tu_background();
    if (u->slots_dirty && (u->page == TU_PG_SAVE || u->page == TU_PG_LOAD)) {
        u->slots_used = ta_slots_used(u->app, u->slot_names);
        u->slots_dirty = false;
    }
    tu_draw_head(u);
    tu_page_cards(u, c);
    for (int i = 0; i < 4; ++i)
        tu_draw_card(u, i, &c[i]);
    tu_draw_panel(u);
    tu_draw_foot(u);
    u->force = false;
    u->frame++;
}

void tu_draw_all(tuna_ui *u)
{
    u->force = true;
    tu_draw(u);
}

/* ---- input ----------------------------------------------------------------------------------------- */
static void tu_msg(tuna_ui *u, const char *m)
{
    tu_cpy(u->msg, m, sizeof u->msg);
    u->msg_ms = 1500;
}

static void tu_goto_page(tuna_ui *u, int pg)
{
    if (pg != u->page) {
        u->page = pg;
        u->confirm = 0;
        u->force = true;
        if (pg == TU_PG_SAVE || pg == TU_PG_LOAD)
            u->slots_dirty = true;
    }
    u->browse_ms = 0;
}

void tu_init(tuna_ui *u, tuna_app *a)
{
    memset(u, 0, sizeof *u);
    u->app = a;
    u->page = TU_PG_HOME;
    u->hot = -1;
    u->palette = (int)TU_PALETTE_DEFAULT;         /* ICE, Tuna's own (Felucca starts GREY) */
    u->rnd = 0x7475u;
    u->batt = 3;
    u->browse = a->item >= 0 ? a->item : 0;
    u->slots_dirty = true;
    u->force = true;
    ux.style = (uint8_t)u->style;
    palette_set((uint32_t)u->palette);
}

static float tu_chaos_add(float v, int d) { return (float)tu_clampi(tu_chaos_val(v) + 2 * d, 0, 100) / 100.0f; }

/* the scales in the knob's order: major and minor first, side by side; the minor variants; pentatonics and
 * blues; the church modes from bright to dark (each click changes one note); the colourful; the symmetric */
static const uint8_t TU_SCALE_ORDER[TS_COUNT] = {
    TS_IONIAN, TS_AEOLIAN, TS_HARM_MINOR, TS_MEL_MINOR, TS_PENT_MAJOR, TS_PENT_MINOR, TS_BLUES,
    TS_LYDIAN, TS_MIXOLYDIAN, TS_DORIAN, TS_PHRYGIAN, TS_LOCRIAN,
    TS_ACOUSTIC, TS_PHRYG_DOM, TS_DOUBLE_HARM, TS_HUNG_MINOR,
    TS_WHOLE_TONE, TS_OCT_HW, TS_OCT_WH, TS_MESSIAEN3};

static int tu_scale_step(int scale, int d)
{
    int i = 0;
    while (i < TS_COUNT - 1 && TU_SCALE_ORDER[i] != scale)
        ++i;
    return TU_SCALE_ORDER[tu_wrap(i + d, TS_COUNT)];
}

static void tu_set_edit(tuna_ui *u, int which, int d)
{
    tuna_app *a = u->app;
    ta_edit e, cur;
    ta_edit_get(a, &cur);
    e.meter = e.mel = e.bass = e.pad = e.harm = e.scale = -1;
    if (a->req_on)
        e = a->req;
    switch (which) {
    case 0: e.meter = (int8_t)tu_wrap((cur.meter < 0 ? (d > 0 ? -1 : TA_M_COUNT) : cur.meter) + d, TA_M_COUNT); break;
    case 1: e.mel = (int8_t)tu_mode_step(&a->g, 0, cur.mel, d); break;
    case 2: e.bass = (int8_t)tu_mode_step(&a->g, 1, cur.bass, d); break;
    case 3: e.pad = (int8_t)tu_mode_step(&a->g, 2, cur.pad, d); break;
    case 4: e.harm = (int8_t)tu_mode_step(&a->g, 3, cur.harm, d); break;
    case 5: e.scale = (int8_t)tu_scale_step(cur.scale, d); break;
    }
    ta_edit_set(a, &e);
}

static void tu_lib_turn(tuna_ui *u, int d, bool era)
{
    tuna_app *a = u->app;
    int from = u->browse_ms > 0 || u->page == TU_PG_LIB ? u->browse : (a->pending >= 0 ? a->pending : a->item);
    if (from < 0) from = 0;
    int to;
    if (era) {
        int gr = tu_wrap(ta_lib_group(from) + d, TA_GROUPS);
        to = ta_lib_first(gr);
    } else {
        to = tu_wrap(from + d, ta_lib_count());
    }
    u->browse = to;
    u->browse_ms = TU_BROWSE_MS;
    ta_select(a, to);
}

void tu_knob(tuna_ui *u, int k, int d)
{
    tuna_app *a = u->app;
    tg_gen *g = &a->g;
    if (!d)
        return;
    u->hot = k - TU_K1;
    u->hot_ms = TU_HOT_MS;
    switch (k) {
    case TU_K_MASTER: u->hot = -1; return;      /* (the MASTER knob is Felucca's: the output level, main.c) */
    case TU_K_SELECT: ta_set_tempo(a, (float)tu_clampi((int)(g->bpm + 0.5f) + d, 20, 300)); u->hot = TU_K_SELECT; return;
    case TU_K_PRESETS: tu_lib_turn(u, d, false); u->hot = -1; return;
    case TU_K_ALGO: tu_lib_turn(u, d, true); u->hot = -1; return;
    }
    int c = k - TU_K1;
    switch (u->page) {
    case TU_PG_HOME: {
        float v[4] = {g->chaos_rhythm, g->chaos_pitch, g->chaos_density, g->chaos_macro};
        v[c] = tu_chaos_add(v[c], d);
        ta_set_chaos(a, v[0], v[1], v[2], v[3]);
        break;
    }
    case TU_PG_LIB:
        if (c == 0) tu_lib_turn(u, d, true);
        else if (c == 1) tu_lib_turn(u, d, false);
        else if (c == 2) ta_seed_step(a, d);
        else ta_set_manner(a, a->manner_fix + d);  /* AUTO, then each manner */
        break;
    case TU_PG_KEY:
        if (c == 0) ta_set_root(a, (g->root_req >= 0 ? g->root_req : g->root) + d);
        else if (c == 1) tu_set_edit(u, 5, d);
        else if (c == 2) g->tuning = tu_wrap(g->tuning + d, TT_TUNE_COUNT);
        else ta_set_deja(a, tu_clampi(a->deja + d, 0, TA_DV_COUNT - 1));
        break;
    case TU_PG_TIME:
        if (c == 0) tu_set_edit(u, 0, d);
        else if (c == 1) g->swing = (float)tu_clampi((int)(g->swing * 100.0f + 0.5f) + 2 * d, 0, 100) / 100.0f;
        else if (c == 2) g->humanize = (float)tu_clampi((int)(g->humanize * 100.0f + 0.5f) + 2 * d, 0, 100) / 100.0f;
        else {
            /* OFF, PERIOD, KIT, and the style's own PATTERN or GROOVE where it has one */
            int m = g->perc_mode;
            for (int i = 0; i < 5; ++i) {
                m = tu_clampi(m + (d > 0 ? 1 : -1), TG_PERC_OFF, TG_PERC_GROOVE);
                bool ok = m <= TG_PERC_KIT || (m == TG_PERC_PATTERN && g->style && g->style->drum_pat[0].note) ||
                          (m == TG_PERC_GROOVE && g->style && g->style->groove);
                if (ok)
                    break;
            }
            bool ok = m <= TG_PERC_KIT || (m == TG_PERC_PATTERN && g->style && g->style->drum_pat[0].note) ||
                      (m == TG_PERC_GROOVE && g->style && g->style->groove);
            if (ok)
                ta_set_percussion(a, m);
        }
        break;
    case TU_PG_PARTS:
        if (g->style)
            tu_set_edit(u, 1 + c, d);
        break;
    case TU_PG_CHAOS:
        if (c < 3) {
            static const uint8_t part[3] = {TG_PART_MELODY, TG_PART_BASS, TG_PART_PAD};
            unsigned m = g->chaos_parts;
            m = d > 0 ? m | (1u << part[c]) : m & ~(1u << part[c]);
            ta_set_chaos_parts(a, m);
        }
        break;
    case TU_PG_EVOLVE:
        if (c == 0) ta_set_evolve(a, d > 0, g->evolve_periods, g->evolve_depth, g->evolve_modulate);
        else if (c == 1) ta_set_evolve(a, g->evolve, tu_clampi(g->evolve_periods + d, 2, 16), g->evolve_depth, g->evolve_modulate);
        else if (c == 2) ta_set_evolve(a, g->evolve, g->evolve_periods, (float)tu_clampi((int)(g->evolve_depth * 100.0f + 0.5f) + 2 * d, 0, 100) / 100.0f, g->evolve_modulate);
        else ta_set_evolve(a, g->evolve, g->evolve_periods, g->evolve_depth, d > 0);
        break;
    case TU_PG_SEQ:
        if (c == 3 && g->frozen)
            tg_pat_transpose(g, -1, d);
        break;
    case TU_PG_MIX:
        a->level[c] = (uint8_t)tu_clampi(a->level[c] + 2 * d, 0, 100);
        break;
    case TU_PG_FX:
        if (c == 0) a->reverb = (uint8_t)tu_clampi(a->reverb + 2 * d, 0, 100);
        else if (c == 1) a->rev_size = (uint8_t)tu_clampi(a->rev_size + 2 * d, 0, 100);
        else if (c == 2) a->rev_damp = (uint8_t)tu_clampi(a->rev_damp + 2 * d, 0, 100);
        else ta_set_reverb_type(a, ta_reverb_step(a->rev_type, d > 0 ? 1 : -1));   /* (its size and damping too) */
        break;
    case TU_PG_SAVE: case TU_PG_LOAD:
        if (c == 0) {
            u->slot = tu_wrap(u->slot + d, TP_SLOTS);
            u->confirm = 0;
        }
        break;
    case TU_PG_MIDI:
        if (c == 0) ta_midi_set(a, d > 0, a->midi.clock, a->midi.bend);
        else if (c == 1) a->local = d > 0;
        else if (c == 2) ta_midi_set(a, a->midi.on, d > 0, a->midi.bend);
        else ta_midi_set(a, a->midi.on, a->midi.clock, d > 0);
        break;
    case TU_PG_RESO: {
        int v[4] = {a->reso_amount, a->reso_decay, a->reso_bright, a->reso_mode};
        if (c < 3) v[c] = tu_clampi(v[c] + 2 * d, 0, 100);
        else v[3] = tu_wrap(v[3] + d, TR_TUNE_COUNT);
        ta_reso_set(a, v[0], v[1], v[2], v[3]);
        break;
    }
    case TU_PG_MENU:
        if (c == 0) {
            u->palette = tu_wrap(u->palette + d, (int)NPALETTES);
            palette_set((uint32_t)u->palette);
            u->force = true;
        } else if (c == 1) {
            u->style = d > 0;
            ux.style = (uint8_t)u->style;
            palette_set((uint32_t)u->palette);
            u->force = true;
        }
        break;
    }
}

/* SAVE / LOAD: OCT+ does it */
static void tu_slot_action(tuna_ui *u)
{
    tuna_app *a = u->app;
    bool used = (u->slots_used >> u->slot) & 1u;
    char m[32], n[4];
    tu_fmt_int(n, u->slot + 1);
    if (u->page == TU_PG_SAVE) {
        if (a->playing) {                         /* (the FM-1 runs from its flash: no writing it while playing) */
            tu_msg(u, "STOP TO SAVE");
            return;
        }
        if (used && !u->confirm) {
            u->confirm = 1;
            return;
        }
        u->confirm = 0;
        int r = ta_slot_save(a, u->slot);
        tu_cpy(m, r == TP_OK ? "SAVED IN SLOT " : r == TP_ERR_SPACE ? "NO ROOM IN SLOT " : "NOT SAVED: SLOT ", sizeof m);
        tu_cat(m, n, sizeof m);
        tu_msg(u, m);
    } else {
        if (!used) {
            tu_msg(u, "THAT SLOT IS EMPTY");
            return;
        }
        int r = ta_slot_load(a, u->slot);
        tu_cpy(m, r == TP_OK ? "LOADED SLOT " : "NOT LOADED: SLOT ", sizeof m);
        tu_cat(m, n, sizeof m);
        tu_msg(u, m);
        u->browse = a->item >= 0 ? a->item : u->browse;
    }
    u->slots_dirty = true;
}

void tu_button(tuna_ui *u, int b, bool down)
{
    tuna_app *a = u->app;
    tg_gen *g = &a->g;
    if (b == TU_B_HOME) {
        if (down) {
            u->home_down = true;
            u->home_ms = 0;
        } else if (u->home_down) {
            u->home_down = false;
            if (u->home_ms < TU_MENU_HOLD_MS)
                tu_goto_page(u, TU_PG_HOME);
        }
        return;
    }
    if (!down)
        return;
    switch (b) {
    case TU_B_PLAY:
        ta_play(a, !a->playing);
        for (int i = 0; i < TU_PARTICLES; ++i)
            if (!a->playing) u->pt[i].life = 0;
        break;
    case TU_B_REC:
        ta_set_deja(a, a->deja == TA_DV_LOCK ? TA_DV_STYLE : TA_DV_LOCK);
        tu_msg(u, a->deja == TA_DV_LOCK ? "MEMORY: THE PHRASE LOCKED" : "MEMORY: THE COMPOSER'S OWN");
        break;
    case TU_B_OCTUP: case TU_B_OCTDN: {
        int d = b == TU_B_OCTUP ? 1 : -1;
        if (u->page == TU_PG_SAVE || u->page == TU_PG_LOAD) {
            if (d > 0) tu_slot_action(u);
            else if (u->confirm) u->confirm = 0;
            else tu_goto_page(u, TU_PG_HOME);
        } else if (u->page == TU_PG_SEQ) {
            if (d > 0) {
                if (g->frozen) tg_unfreeze(g);
                else if (!tg_freeze(g)) tu_msg(u, "PLAY A PERIOD FIRST");
            } else {
                tu_goto_page(u, TU_PG_HOME);
            }
        } else if (u->page == TU_PG_MENU) {
            if (d < 0) tu_goto_page(u, TU_PG_HOME);
        } else {
            char m[24];
            ta_seed_step(a, d);
            tu_cpy(m, "SEED ", sizeof m);
            tu_fmt_int(m + 5, a->seed_req >= 0 ? a->seed_req : (int)(a->seed % TA_SEEDS));
            tu_msg(u, m);
        }
        break;
    }
    case TU_B_FX: tu_goto_page(u, TU_PG_LIB); u->browse = a->pending >= 0 ? a->pending : a->item >= 0 ? a->item : 0; break;
    case TU_B_SCL: tu_goto_page(u, TU_PG_KEY); break;
    case TU_B_EDIT: tu_goto_page(u, u->page == TU_PG_TIME ? TU_PG_PARTS : u->page == TU_PG_PARTS ? TU_PG_CHAOS : TU_PG_TIME); break;
    case TU_B_ARP: tu_goto_page(u, TU_PG_EVOLVE); break;
    case TU_B_SEQ: tu_goto_page(u, TU_PG_SEQ); break;
    case TU_B_GLO: tu_goto_page(u, u->page == TU_PG_MIX ? TU_PG_FX : TU_PG_MIX); break;
    case TU_B_SAVE: tu_goto_page(u, u->page == TU_PG_SAVE ? TU_PG_LOAD : TU_PG_SAVE); break;
    case TU_B_ENV: tu_goto_page(u, TU_PG_MIDI); break;
    case TU_B_LFO: tu_goto_page(u, TU_PG_RESO); break;
    }
}

/* the keys: a tap sets the root; two or more held, a chord; SAVE / LOAD: the white keys are the slots */
static int tu_white_index(int note)
{
    static const int8_t w[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
    int octave = note / 12, k = w[note % 12];
    if (k < 0)
        return -1;
    int base = (TU_KEY_LO / 12) * 7 + w[TU_KEY_LO % 12];
    return octave * 7 + k - base;
}

void tu_key(tuna_ui *u, int note, bool down)
{
    tuna_app *a = u->app;
    if (note < TU_KEY_LO || note > TU_KEY_HI)
        return;
    uint32_t bit = 1u << (note - TU_KEY_LO);
    if (u->page == TU_PG_SAVE || u->page == TU_PG_LOAD) {
        int wi = tu_white_index(note);
        if (down && wi >= 0 && wi < TP_SLOTS) {
            u->slot = wi;
            u->confirm = 0;
        }
        return;
    }
    if (down) {
        if (!u->keys_down) {
            u->key_gesture = 0;
            u->key_first = note;
        }
        u->keys_down |= bit;
        u->key_gesture++;
    } else {
        u->keys_down &= ~bit;
    }
    int held = 0, low = -1;
    uint16_t mask = 0;
    for (int i = 0; i <= TU_KEY_HI - TU_KEY_LO; ++i)
        if ((u->keys_down >> i) & 1u) {
            held++;
            if (low < 0) low = TU_KEY_LO + i;
            mask |= (uint16_t)(1u << ((TU_KEY_LO + i) % 12));
        }
    if (held >= 2)
        tg_hold_chord(&a->g, mask, low % 12);
    else if (a->g.hold_mask)
        tg_hold_chord(&a->g, 0, 0);
    if (!down && !u->keys_down && u->key_gesture == 1) {   /* a single key, tapped: the root */
        char m[16];
        ta_set_root(a, u->key_first % 12);
        tu_cpy(m, "ROOT ", sizeof m);
        tu_cat(m, TU_NOTE_NAMES[u->key_first % 12], sizeof m);
        tu_msg(u, m);
    }
}

/* ---- time --------------------------------------------------------------------------------------------- */
static void tu_spawn(tuna_ui *u, const ta_note *n)
{
    int best = 0, life = 256;
    for (int i = 0; i < TU_PARTICLES; ++i) {     /* a free one, or the faintest */
        if (!u->pt[i].life) { best = i; break; }
        if (u->pt[i].life < life) { life = u->pt[i].life; best = i; }
    }
    tu_particle *p = &u->pt[best];
    int lo = 33, hi = 90;                          /* bass to the melody's top: the width of the panel */
    int note = n->part >= TG_PART_DRUMS ? 30 + (n->note % 48) : n->note;
    int32_t x = 10 + (tu_clampi(note, lo, hi) - lo) * 214 / (hi - lo);
    p->x = (int16_t)((x + (int32_t)(tu_rand(u) % 5u) - 2) << 4);
    p->y = (int16_t)(26 << 4);
    p->vy = (int16_t)(4 + n->vel / 20);
    p->part = n->part;
    p->size = (uint8_t)(n->part == TG_PART_BASS ? 7 : n->part == TG_PART_MELODY ? 6 : n->part == TG_PART_PAD ? 5 : 4);
    if (n->vel > 100)
        p->size++;
    p->life = 255;
}

void tu_tick(tuna_ui *u, int ms)
{
    tuna_app *a = u->app;
    ta_note n;
    while (ta_feed_pop(a, &n))
        tu_spawn(u, &n);
    for (int i = 0; i < TU_PARTICLES; ++i) {
        tu_particle *p = &u->pt[i];
        if (!p->life)
            continue;
        int floor = (TU_PANEL_H - 4 - p->size) << 4;
        if (p->y < floor) {
            p->y = (int16_t)(p->y + p->vy * ms / 16);
            p->vy = (int16_t)(p->vy + (ms + 32) / 64);
            if (p->y >= floor) {
                p->y = (int16_t)floor;
                p->vy = 0;
                if (p->life > 120) p->life = 120;  /* landed: it fades */
            }
        } else {
            int f = ms / 12 > 0 ? ms / 12 : 1;
            p->life = (uint8_t)(p->life > f ? p->life - f : 0);
        }
    }
    for (int c = 0; c < 16; ++c)                  /* the MIDI lights fade */
        a->midi.act[c] = (uint8_t)(a->midi.act[c] > ms ? a->midi.act[c] - ms : 0);
    if (u->hot_ms > 0) u->hot_ms -= ms;
    if (u->browse_ms > 0 && (u->browse_ms -= ms) <= 0) {
        u->browse_ms = 0;
        u->force = true;
    }
    if (u->msg_ms > 0) u->msg_ms -= ms;
    if (u->home_down) {
        u->home_ms += ms;
        if (u->home_ms >= TU_MENU_HOLD_MS && u->page != TU_PG_MENU) {
            tu_goto_page(u, TU_PG_MENU);
            u->home_down = false;
        }
    }
}
