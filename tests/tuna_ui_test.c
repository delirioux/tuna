/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA's screen and controls on the host: firmware/src/tuna_ui.c drawn by Felucca's gfx.c into a 240 x 240
 * frame, the controls driven as a player would, every page in every palette written out, and a layout lint.
 *   tests/tuna_ui_test.sh [OUTDIR]     (builds this, writes the frames under OUTDIR/ppm, exit 0: all passed)
 * Lint (every text and icon as gfx.c draws it, GFX_HOOK_TEXT): inside the screen and its canvas, not cut,
 * not ellipsised unless it is a name, and no two texts of one canvas overlapping. */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails, checks;
#define CHECK(cond, ...)                                                   \
    do {                                                                   \
        checks++;                                                          \
        if (!(cond)) {                                                     \
            fails++;                                                       \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                    \
            printf(__VA_ARGS__);                                           \
            printf("\n");                                                  \
        }                                                                  \
    } while (0)

/* ---- the layout lint: the ink boxes of one canvas, checked when it is blitted -------------------------- */
typedef struct { int x0, y0, x1, y1, flags; char s[24]; } box;
static box boxes[256];
static int nbox;
static const char *lint_where = "";
static int lint_findings;
static void hook_text(int x0, int y0, int x1, int y1, const char *s, int flags)
{
    if (nbox < 256) {
        box *b = &boxes[nbox++];
        b->x0 = x0; b->y0 = y0; b->x1 = x1; b->y1 = y1; b->flags = flags;
        strncpy(b->s, s, sizeof b->s - 1);
        b->s[sizeof b->s - 1] = 0;
    }
}
#define GFX_HOOK_TEXT(x0, y0, x1, y1, s, flags) hook_text((x0), (y0), (x1), (y1), (s), (flags))
#define GFX_HOOK_BLIT(x, y, r0) ((void)0)
#define GFX_HOOK_BEGIN() (nbox = 0)
#define GFX_HOOK_PIXELS(n) ((void)0)

static uint16_t fb[240 * 240];
static int blit_count;
static void lint_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    for (int i = 0; i < nbox; ++i) {
        box *b = &boxes[i];
        if (b->x1 <= b->x0)
            continue;
        bool name = (b->flags & 8) != 0;            /* free text (a name, a message) may be ellipsised */
        if (b->x0 < 0 || b->y0 < 0 || b->x1 > (int)w || b->y1 > (int)h) {
            printf("LINT %s: \"%s\" leaves its canvas (%d,%d..%d,%d of %ux%u at %u,%u)\n", lint_where, b->s, b->x0,
                   b->y0, b->x1, b->y1, w, h, x, y);
            lint_findings++;
        }
        if ((int)x + b->x1 > 240 || (int)y + b->y1 > 240) {
            printf("LINT %s: \"%s\" off the screen\n", lint_where, b->s);
            lint_findings++;
        }
        if ((b->flags & 1) && !name) {
            printf("LINT %s: \"%s\" ellipsised\n", lint_where, b->s);
            lint_findings++;
        }
        for (int j = i + 1; j < nbox; ++j) {
            box *c = &boxes[j];
            if (c->x1 <= c->x0)
                continue;
            if (b->x0 < c->x1 && c->x0 < b->x1 && b->y0 < c->y1 && c->y0 < b->y1) {
                printf("LINT %s: \"%s\" overlaps \"%s\" (%d,%d..%d,%d / %d,%d..%d,%d)\n", lint_where, b->s, c->s, b->x0,
                       b->y0, b->x1, b->y1, c->x0, c->y0, c->x1, c->y1);
                lint_findings++;
            }
        }
    }
    nbox = 0;
}
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{
    lint_blit(x, y, w, h);
    blit_count++;
    for (uint32_t j = 0; j < h && y + j < 240; j++)
        for (uint32_t i = 0; i < w && x + i < 240; i++)
            fb[(y + j) * 240 + x + i] = p[j * w + i];
}
#include "gfx.c"
#include "tuna_ui.c"

/* ---- the frame out ------------------------------------------------------------------------------------- */
static const char *outdir;
static int frames_written;
static void write_ppm(const char *name)
{
    if (!outdir)
        return;
    char path[512];
    snprintf(path, sizeof path, "%s/ppm/%s.ppm", outdir, name);
    FILE *f = fopen(path, "wb");
    if (!f)
        return;
    fprintf(f, "P6 240 240 255\n");
    for (int i = 0; i < 240 * 240; i++) {
        uint16_t c = (uint16_t)((fb[i] >> 8) | (fb[i] << 8));      /* (stored byte-swapped for the panel) */
        uint8_t px[3] = {(uint8_t)((c >> 11) << 3 | (c >> 13)), (uint8_t)(((c >> 5) & 63) << 2 | ((c >> 9) & 3)),
                         (uint8_t)((c & 31) << 3 | ((c >> 2) & 7))};
        fwrite(px, 1, 3, f);
    }
    fclose(f);
    frames_written++;
}

/* ---- an in-memory slot store ----------------------------------------------------------------------------- */
static uint8_t mem[TP_GROUPS + 1][TP_GROUP_MAX];   /* (the slots' groups, and the preferences: TP_PREFS_GROUP) */
static int mem_n[TP_GROUPS + 1];
static int mem_load(void *ctx, int g, uint8_t *b, int max)
{
    (void)ctx;
    if (mem_n[g] <= 0) return -1;
    int n = mem_n[g] < max ? mem_n[g] : max;
    memcpy(b, mem[g], (size_t)n);
    return n;
}
static int mem_save(void *ctx, int g, const uint8_t *b, int n)
{
    (void)ctx;
    memcpy(mem[g], b, (size_t)n);
    mem_n[g] = n;
    return 0;
}
static const tp_store store = {mem_load, mem_save, 0};

/* ---- the sound: counts what reaches it ---------------------------------------------------------------- */
static long ev_on, ev_off;
/* a stand-in for the FM-1's sound, for the resonator to hear: a decaying sine per note (four at most) */
#include <math.h>
static struct { double ph, inc, amp; } osc[4];
static int osc_next;
static int sounding[8][128];
static void sound(void *ctx, const tg_event *e)
{
    (void)ctx;
    if (e->type != TG_EV_NOTE)
        return;
    if (e->on) {
        ev_on++;
        sounding[e->part][e->note]++;
        if (e->part < 3) {
            osc[osc_next].inc = 2.0 * 3.14159265 * 440.0 * pow(2.0, (e->note - 69) / 12.0) / 11025.0;
            osc[osc_next].amp = 6000.0;
            osc_next = (osc_next + 1) % 4;
        }
    }
    else { ev_off++; if (sounding[e->part][e->note] > 0) sounding[e->part][e->note]--; }
}
static int still_sounding(void)
{
    int n = 0;
    for (int p = 0; p < 8; ++p)
        for (int k = 0; k < 128; ++k)
            n += sounding[p][k];
    return n;
}

#define SR 11025.0f
static tuna_app app;
static tuna_ui ui;

/* play for ms, the screen at 60 frames a second */
static void run_ms(int ms)
{
    for (int t = 0; t < ms; t += 16) {
        int n = (int)(SR * 16 / 1000);
        ta_process(&app, n);
        if (app.playing && app.reso.amount > 0) {      /* the host's part: the mix through the resonator */
            int32_t in[256], o[512] = {0};
            for (int i = 0; i < n; ++i) {
                double v = 0;
                for (int k = 0; k < 4; ++k) {
                    v += osc[k].amp * sin(osc[k].ph);
                    osc[k].ph += osc[k].inc;
                    osc[k].amp *= 0.9997;
                }
                in[i] = (int32_t)v;
            }
            tr_process(&app.reso, in, o, n);
        }
        tu_tick(&ui, 16);
        tu_draw(&ui);
    }
}
static void press(int b) { tu_button(&ui, b, true); tu_button(&ui, b, false); }
static void shot(const char *name)
{
    char w[96];
    snprintf(w, sizeof w, "%s", name);
    lint_where = w;
    tu_draw_all(&ui);
    write_ppm(name);
}

/* ---- every page, every palette ---------------------------------------------------------------------- */
static void all_palettes(const char *page)
{
    for (int p = 0; p < (int)NPALETTES; ++p) {
        for (int st = 0; st < 2; ++st) {
            char name[64];
            ui.palette = p;
            ui.style = st;
            ux.style = (uint8_t)st;
            palette_set((uint32_t)p);
            snprintf(name, sizeof name, "%s%s_%s", st ? "LINE_" : "", UI_PALETTES[p].name, page);
            shot(name);
        }
    }
    ui.palette = 1;
    ui.style = 0;
    ux.style = 0;
    palette_set(1);
}

static void test_screens(void)
{
    /* HOME stopped, then playing with notes falling */
    all_palettes("home_stopped");
    press(TU_B_PLAY);
    run_ms(20000);
    CHECK(app.playing && ev_on > 50, "playing: %ld notes", ev_on);
    int live = 0;
    for (int i = 0; i < TU_PARTICLES; ++i) live += ui.pt[i].life > 0;
    CHECK(live > 3, "falling notes on HOME: %d", live);
    all_palettes("home");
    /* a chaos knob turned: its card hot */
    tu_knob(&ui, TU_K3, 6);
    shot("GREEN_home_hot");
    tu_knob(&ui, TU_K3, -6);
    /* the library while PRESETS turns */
    tu_knob(&ui, TU_K_PRESETS, 1);
    run_ms(100);
    all_palettes("browse");
    run_ms(3000);
    struct { int btn, times; const char *name; } pages[] = {
        {TU_B_FX, 1, "library"}, {TU_B_SCL, 1, "key"}, {TU_B_EDIT, 1, "time"}, {TU_B_EDIT, 1, "parts"},
        {TU_B_ARP, 1, "evolve"}, {TU_B_SEQ, 1, "seq_live"}, {TU_B_GLO, 1, "mix"}, {TU_B_GLO, 1, "reverb"},
        {TU_B_SAVE, 1, "save"}, {TU_B_SAVE, 1, "load"}, {TU_B_ENV, 1, "midi"}, {TU_B_LFO, 1, "reso_off"},
    };
    for (unsigned i = 0; i < sizeof pages / sizeof pages[0]; ++i) {
        press(pages[i].btn);
        run_ms(200);
        all_palettes(pages[i].name);
    }
    /* the resonator turned up: its strings shake */
    for (int i = 0; i < 20; ++i) tu_knob(&ui, TU_K1, 1);
    run_ms(2000);
    CHECK(app.reso_amount == 40, "AMOUNT 40 (%d)", app.reso_amount);
    {
        int32_t mx = 0;
        for (int k = 0; k < TR_STRINGS; ++k) mx = abs(app.reso.lp[k]) > mx ? abs(app.reso.lp[k]) : mx;
        CHECK(mx > (64 << 8), "the strings ring (%d)", mx >> 8);
    }
    all_palettes("reso");
    ta_reso_set(&app, 0, app.reso_decay, app.reso_bright, app.reso_mode);
    /* SEQ frozen (after the piece chosen above has come in: a new piece starts live) */
    for (int i = 0; i < 60 && app.pending >= 0; ++i)
        run_ms(1000);
    press(TU_B_SEQ);
    press(TU_B_OCTUP);
    for (int i = 0; i < 60 && !app.g.frozen; ++i)
        run_ms(1000);
    CHECK(app.g.frozen, "frozen from the SEQ page (playing %d, req %d, msg %s, rec_valid %d, ticks %ld)", app.playing,
          app.g.freeze_req, ui.msg, app.g.rec_valid, app.g.ticks);
    run_ms(1500);
    all_palettes("seq_frozen");
    press(TU_B_OCTUP);                                /* live again at the boundary */
    for (int i = 0; i < 60 && app.g.frozen; ++i)
        run_ms(1000);
    CHECK(!app.g.frozen, "unfrozen");
    /* the menu: HOME held */
    tu_button(&ui, TU_B_HOME, true);
    run_ms(700);
    tu_button(&ui, TU_B_HOME, false);
    CHECK(ui.page == TU_PG_MENU, "HOME held opens the menu (page %d)", ui.page);
    all_palettes("menu");
    press(TU_B_HOME);
    CHECK(ui.page == TU_PG_HOME, "HOME closes the menu");
    /* a character preset, the parts page empty */
    ta_play(&app, false);
    ta_select(&app, 0);
    press(TU_B_EDIT);
    press(TU_B_EDIT);
    shot("GREEN_parts_character");
    press(TU_B_HOME);
    shot("GREEN_home_character");
}

/* ---- the controls -------------------------------------------------------------------------------------- */
/* a new piece brings its own tempo, key, percussion and evolution (the chaos knobs stay); a seed asked for while
 * playing comes in at the next half of the period; 256 seeds */
static void test_reset(void)
{
    ta_play(&app, false);
    ta_select(&app, ta_lib_first(2));
    float own = app.g.bpm;
    int own_root = app.g.root, own_perc = app.g.perc_mode;
    tu_knob(&ui, TU_K_SELECT, 7);
    ta_set_root(&app, (own_root + 3) % 12);
    ta_set_percussion(&app, own_perc == TG_PERC_KIT ? TG_PERC_OFF : TG_PERC_KIT);
    ta_set_evolve(&app, true, 6, 0.8f, true);
    ta_set_chaos(&app, 0.7f, 0.5f, 0.5f, 0.5f);
    ta_select(&app, ta_lib_first(2) + 1);
    ta_select(&app, ta_lib_first(2));
    own = app.g.style->bpm;                       /* (its own in the manner the new seed brought) */
    own_root = app.g.style->root;
    own_perc = app.g.style->perc_mode;
    CHECK(app.g.bpm == own && app.g.root == own_root && app.g.perc_mode == own_perc && !app.g.evolve,
          "the piece's own again (bpm %.0f/%.0f, root %d/%d)", app.g.bpm, own, app.g.root, own_root);
    CHECK(app.g.chaos_rhythm > 0.69f, "the chaos knobs stay (%.2f)", app.g.chaos_rhythm);
    ta_set_chaos(&app, 0.5f, 0.5f, 0.5f, 0.5f);
    /* a new piece, a new seed: stopped at once; playing with the piece; turning back cancels both */
    {
        unsigned seen = 0;
        int differ = 0;
        for (int i = 0; i < 12; ++i) {
            uint32_t s0 = app.seed;
            ta_select(&app, ta_lib_first(2) + (i & 1));
            differ += app.seed != s0;
            seen |= 1u << (app.seed % 32);
        }
        CHECK(differ >= 10 && __builtin_popcount(seen) >= 6, "a new piece draws a new seed (%d of 12 new)", differ);
        ta_play(&app, true);
        run_ms(100);
        int it = app.item;
        ta_select(&app, it + 1);
        CHECK(app.pending == it + 1 && app.seed_req >= 0, "playing: the piece and its seed wait (%d, %d)", app.pending,
              app.seed_req);
        ta_select(&app, it);
        CHECK(app.pending < 0 && app.seed_req < 0, "turned back: nothing waits (%d, %d)", app.pending, app.seed_req);
        ta_play(&app, false);
        ta_select(&app, ta_lib_first(2));
    }
    /* seeds */
    ta_set_seed(&app, 255);
    ta_seed_step(&app, 1);
    CHECK(app.seed == 0, "seed 255 + 1 wraps to 0 (%u)", app.seed);
    ta_seed_step(&app, -1);
    CHECK(app.seed == 255, "and back (%u)", app.seed);
    ta_play(&app, true);
    run_ms(300);
    uint32_t before = app.seed;
    ta_seed_step(&app, 1);
    CHECK(app.seed == before && app.seed_req == (int)((before + 1) % 256), "playing: the seed waits (%u, asked %d)",
          app.seed, app.seed_req);
    int half = tg_half_position(&app.g), d0 = tg_period_position(&app.g, NULL);
    long guard = 0;
    while (app.seed_req >= 0 && guard++ < 20000)
        ta_process(&app, 64);
    CHECK(app.seed == (before + 1) % 256, "it came in (%u)", app.seed);
    CHECK(app.g.ticks < 4, "the new seed starts its piece from the top (tick %ld; asked at %d, half %d)", app.g.ticks, d0,
          half);
    /* on the beat: the new seed's first tick a tick after the old one's last, as if nothing changed (it went in
     * before the boundary tick, the clock's phase kept); a piece and a form change likewise */
    for (int kind = 0; kind < 3; ++kind) {
        long f = 0, t_prev = -1, t_last = -1, t_new = -1, gap_old = -1;
        while (app.g.ticks < 3) { ta_process(&app, 1); f++; }
        if (kind == 0) ta_seed_step(&app, 1);
        else if (kind == 1) ta_select(&app, ta_lib_first(2) + 1);
        else { ta_edit e; ta_edit_get(&app, &e); e.mel = e.mel == 0 ? 1 : 0; ta_edit_set(&app, &e); }
        long tk = app.g.ticks;
        bool waiting = true;
        for (guard = 0; guard < 400000 && t_new < 0; ++guard) {
            ta_process(&app, 1);
            f++;
            waiting = waiting && (app.seed_req >= 0 || app.pending >= 0 || app.req_on);
            if (app.g.ticks != tk) {
                if (app.g.ticks == 1 && !waiting) {
                    t_new = f;
                } else {
                    t_prev = t_last;
                    t_last = f;
                }
                tk = app.g.ticks;
            }
        }
        gap_old = t_last - t_prev;
        long gap = t_new - t_last, tol = gap_old / 20 + 2;
        static const char *const what[3] = {"seed", "piece", "form"};
        CHECK(t_new > 0 && t_prev > 0 && labs(gap - gap_old) <= tol + (kind == 1 ? gap_old : 0),
              "%s: no hiccup, the first new tick %ld frames after the last old one (a tick: %ld)", what[kind], gap, gap_old);
    }
    ta_play(&app, false);
    ta_select(&app, 0);
}

static void test_controls(void)
{
    ta_play(&app, false);
    ta_select(&app, ta_lib_first(2));                /* Baroque's first */
    tu_goto_page(&ui, TU_PG_HOME);
    /* chaos knobs: two per click, around 0.5 */
    tu_knob(&ui, TU_K1, 5);
    CHECK(app.g.chaos_rhythm > 0.59f && app.g.chaos_rhythm < 0.61f, "RHYTHM +5 clicks: %.2f", app.g.chaos_rhythm);
    tu_knob(&ui, TU_K1, -100);
    CHECK(app.g.chaos_rhythm == 0.0f, "RHYTHM stops at 0");
    tu_knob(&ui, TU_K1, 25);
    CHECK(app.g.chaos_rhythm == 0.5f, "back to the composer's");
    /* PRESETS stopped: at once; playing: at the period's end */
    int before = app.item;
    tu_knob(&ui, TU_K_PRESETS, 1);
    CHECK(app.item == before + 1 && app.pending < 0, "stopped: loads at once (%d)", app.item);
    press(TU_B_PLAY);
    run_ms(3000);
    int at = app.item;
    tu_knob(&ui, TU_K_PRESETS, 2);
    CHECK(app.item == at && app.pending == at + 2, "playing: waits (%d -> %d)", app.item, app.pending);
    for (int i = 0; i < 400 && app.pending >= 0; ++i)
        run_ms(100);
    CHECK(app.item == at + 2 && app.pending < 0, "loaded at the period's end");
    CHECK(app.g.ticks < 200, "the new piece starts from its beginning (tick %ld)", app.g.ticks);
    /* ALGORITHM: a whole era */
    tu_knob(&ui, TU_K_ALGO, 1);
    CHECK(app.pending == ta_lib_first(ta_lib_group(at + 2) + 1), "ALGORITHM: the next era's first");
    run_ms(100);
    /* keys: a tap sets the root (at the next chord), two held a chord */
    int root = app.g.root;
    tu_key(&ui, 55, true);                           /* G3 */
    tu_key(&ui, 55, false);
    CHECK(app.g.root_req == 7 || app.g.root == 7, "a tap asks for G");
    for (int i = 0; i < 100 && app.g.root != 7; ++i)
        run_ms(50);
    CHECK(app.g.root == 7, "root G applied (was %d)", root);
    tu_key(&ui, 60, true);                           /* C4 E4 G4 */
    tu_key(&ui, 64, true);
    tu_key(&ui, 67, true);
    CHECK(app.g.hold_mask == (1u << 0 | 1u << 4 | 1u << 7) && app.g.hold_root == 0, "chord C held");
    tu_key(&ui, 64, false);
    tu_key(&ui, 67, false);
    CHECK(app.g.hold_mask == 0, "released");
    tu_key(&ui, 60, false);
    CHECK(app.g.root == 7 && app.g.root_req < 0, "a chord is not a root tap");
    /* REC: memory locked and back */
    press(TU_B_REC);
    CHECK(app.deja == TA_DV_LOCK && app.g.x.deja_vu == 0.5f, "REC locks the phrase");
    press(TU_B_REC);
    CHECK(app.deja == TA_DV_STYLE, "REC again: the composer's own");
    /* OCT+: another seed */
    uint32_t s0 = app.seed;
    press(TU_B_OCTUP);
    CHECK(app.playing ? app.seed_req == (int)((s0 + 1) % 256) : app.seed == (s0 + 1) % 256,
          "OCT+ steps the seed (playing: at the next half period)");
    run_ms(500);
    /* form settings wait for the period's end */
    tu_goto_page(&ui, TU_PG_TIME);
    tu_knob(&ui, TU_K1, 1);
    CHECK(app.req_on, "a meter asked for while playing waits");
    for (int i = 0; i < 400 && app.req_on; ++i)
        run_ms(100);
    CHECK(!app.req_on, "the meter came in");
    /* stop: everything off */
    press(TU_B_PLAY);
    CHECK(!app.playing && still_sounding() == 0, "stop: %d notes still sounding", still_sounding());

    /* SAVE and LOAD through the UI: a slot by its white key */
    ta_select(&app, ta_lib_first(5) + 3);
    tu_knob(&ui, TU_K_PRESETS, 0);
    tu_goto_page(&ui, TU_PG_HOME);
    tu_knob(&ui, TU_K2, 10);                         /* PITCH +20 */
    press(TU_B_SAVE);
    tu_key(&ui, 57, true);                           /* A3: the third white key, slot 3 */
    tu_key(&ui, 57, false);
    CHECK(ui.slot == 2, "white key A3 = slot 3 (%d)", ui.slot + 1);
    tu_draw(&ui);
    press(TU_B_OCTUP);
    CHECK(strstr(ui.msg, "SAVED") != NULL, "saved: %s", ui.msg);
    tu_draw(&ui);
    CHECK((ui.slots_used >> 2) & 1u, "slot 3 used");
    press(TU_B_OCTUP);                               /* a used slot: asks */
    CHECK(ui.confirm == 1, "saving over asks first");
    press(TU_B_OCTDN);
    CHECK(ui.confirm == 0 && ui.page == TU_PG_SAVE, "OCT- cancels");
    int saved_item = app.item;
    float saved_pitch = app.g.chaos_pitch;
    ta_select(&app, 1);
    tu_goto_page(&ui, TU_PG_HOME);
    tu_knob(&ui, TU_K2, -10);
    press(TU_B_SAVE);
    press(TU_B_SAVE);
    CHECK(ui.page == TU_PG_LOAD, "SAVE again: LOAD");
    ui.slot = 2;
    press(TU_B_OCTUP);
    CHECK(app.item == saved_item && app.g.chaos_pitch == saved_pitch, "loaded slot 3 (%s, pitch %.2f)",
          ta_lib_name(app.item), app.g.chaos_pitch);
    ui.slot = 9;
    press(TU_B_OCTUP);
    CHECK(strstr(ui.msg, "EMPTY") != NULL, "an empty slot says so");
    press(TU_B_HOME);

    /* the library list: every item named, groups in order */
    int last_group = 0;
    for (int i = 0; i < ta_lib_count(); ++i) {
        int gr = ta_lib_group(i);
        CHECK(gr >= last_group && ta_lib_name(i)[0], "library item %d", i);
        last_group = gr;
    }
    CHECK(ta_lib_count() == tp_character_count() + TG_STYLE_COUNT, "library size %d", ta_lib_count());
}

/* CHAOS (EDIT a third time): which parts the HOME knobs move; kept from piece to piece and in a slot */
static void test_chaos_page(void)
{
    ta_play(&app, false);
    tu_goto_page(&ui, TU_PG_HOME);
    press(TU_B_EDIT);
    press(TU_B_EDIT);
    press(TU_B_EDIT);
    CHECK(ui.page == TU_PG_CHAOS, "EDIT, EDIT, EDIT: the CHAOS page (%d)", ui.page);
    CHECK(app.g.chaos_parts == TG_CHAOS_ALL, "all parts follow the knobs at first (%u)", app.g.chaos_parts);
    tu_knob(&ui, TU_K2, -1);                         /* BASS off */
    CHECK(app.g.chaos_parts == ((1u << TG_PART_MELODY) | (1u << TG_PART_PAD)), "BASS off (%u)", app.g.chaos_parts);
    lint_where = "chaos";
    all_palettes("chaos");
    ta_select(&app, ta_lib_first(3));
    CHECK(app.g.chaos_parts == ((1u << TG_PART_MELODY) | (1u << TG_PART_PAD)), "kept for the next piece (%u)",
          app.g.chaos_parts);
    CHECK(ta_slot_save(&app, 5) == TP_OK, "saved slot 6");
    tu_knob(&ui, TU_K1, -1);
    tu_knob(&ui, TU_K3, -1);
    CHECK(app.g.chaos_parts == 0, "all off (%u)", app.g.chaos_parts);
    CHECK(ta_slot_load(&app, 5) == TP_OK && app.g.chaos_parts == ((1u << TG_PART_MELODY) | (1u << TG_PART_PAD)),
          "the slot brings its parts back (%u)", app.g.chaos_parts);
    tu_knob(&ui, TU_K2, 1);
    CHECK(app.g.chaos_parts == TG_CHAOS_ALL, "BASS on again (%u)", app.g.chaos_parts);
    press(TU_B_EDIT);
    CHECK(ui.page == TU_PG_TIME, "EDIT again: back to TIME (%d)", ui.page);
    tu_goto_page(&ui, TU_PG_HOME);
}

/* REVERB (GLO, GLO): the send, and Felucca's reverb: size, damping, type; each piece brings its own type */
static void test_reverb_page(void)
{
    ta_play(&app, false);
    tu_goto_page(&ui, TU_PG_HOME);
    press(TU_B_GLO);
    press(TU_B_GLO);
    CHECK(ui.page == TU_PG_FX, "GLO, GLO: the REVERB page (%d)", ui.page);
    int st = ta_lib_style(app.item), own = ta_piece_reverb(st);
    CHECK(app.rev_type == own && app.rev_size == ta_reverb_type(own)->size, "the piece's own reverb, %s (%d %d)",
          ta_reverb_type(own)->name, app.rev_type, app.rev_size);
    ta_set_reverb_type(&app, TG_REV_ROOM);
    CHECK(app.rev_size == 71 && app.rev_damp == 48, "ROOM: Felucca's reverb as designed (%d %d)", app.rev_size, app.rev_damp);
    tu_knob(&ui, TU_K4, 1);
    CHECK(app.rev_type == TG_REV_HALL && app.rev_size == ta_reverb_type(TG_REV_HALL)->size,
          "K4: ROOM -> HALL, with its size (%d %d)", app.rev_type, app.rev_size);
    tu_knob(&ui, TU_K4, 1);
    tu_knob(&ui, TU_K4, 1);
    tu_knob(&ui, TU_K4, 1);
    CHECK(app.rev_type == TG_REV_SPRING, "K4 on: CATHEDRAL, then SPRING, the last (%d)", app.rev_type);
    for (int i = 0; i < 6; ++i)
        tu_knob(&ui, TU_K4, -1);
    CHECK(app.rev_type == TG_REV_BATHROOM, "K4 back: BATHROOM, the first (%d)", app.rev_type);
    lint_where = "reverb";
    all_palettes("reverb_bath");
    ta_set_reverb_type(&app, TG_REV_CATHEDRAL);
    all_palettes("reverb_cathedral");
    ta_set_reverb_type(&app, TG_REV_SPRING);
    tu_knob(&ui, TU_K2, -5);
    tu_knob(&ui, TU_K3, 4);
    CHECK(app.rev_size == 61 && app.rev_damp == 56 && app.rev_type == TG_REV_SPRING, "K2 SIZE, K3 DAMP (%d %d)",
          app.rev_size, app.rev_damp);
    all_palettes("reverb");
    CHECK(ta_slot_save(&app, 6) == TP_OK, "saved slot 7");
    ta_set_reverb_type(&app, TG_REV_ROOM);
    CHECK(ta_slot_load(&app, 6) == TP_OK && app.rev_size == 61 && app.rev_damp == 56 && app.rev_type == TG_REV_SPRING,
          "the slot brings the reverb back (%d %d %d)", app.rev_size, app.rev_damp, app.rev_type);
    /* a new piece: its own reverb, the send kept */
    app.reverb = 44;
    int it = -1, ren = -1, rom = -1;
    for (int i = 0; i < ta_lib_count() && (it < 0 || ren < 0 || rom < 0); ++i) {
        int s2 = ta_lib_style(i);
        if (s2 < 0) continue;
        if (it < 0 && tg_style_get(s2)->reverb == TG_REV_SPRING + 1) it = i;
        if (ren < 0 && tp_style_era(s2) == TP_ERA_RENAISSANCE) ren = i;
        if (rom < 0 && tp_style_era(s2) == TP_ERA_ROMANTIC) rom = i;
    }
    CHECK(it >= 0 && ren >= 0 && rom >= 0, "a SPRING genre, a renaissance and a romantic composer (%d %d %d)", it, ren, rom);
    ta_select(&app, it);
    CHECK(app.rev_type == TG_REV_SPRING && app.rev_size == 71 && app.reverb == 44, "%s: SPRING, the send kept (%d %d)",
          ta_lib_name(it), app.rev_type, app.reverb);
    ta_select(&app, ren);                         /* (a manner may bring a reverb of its own) */
    ta_set_manner(&app, 0);
    CHECK(app.rev_type == TG_REV_CATHEDRAL || app.g.style->reverb, "%s: CATHEDRAL (%d)", ta_lib_name(ren), app.rev_type);
    ta_select(&app, rom);
    ta_set_manner(&app, 0);
    CHECK(app.rev_type == TG_REV_HALL || app.g.style->reverb, "%s: HALL (%d)", ta_lib_name(rom), app.rev_type);
    ta_set_manner(&app, -1);
    app.reverb = 30;
    ta_set_reverb_type(&app, TG_REV_ROOM);
    tu_goto_page(&ui, TU_PG_HOME);
}

/* KEY, K2: the scales in a player's order, MAJ and MIN side by side, every scale once in a full turn */
static void test_scale_order(void)
{
    ta_play(&app, false);
    tu_goto_page(&ui, TU_PG_KEY);
    ta_edit e;
    ta_edit_get(&app, &e);
    e.scale = TS_IONIAN;
    ta_edit_set(&app, &e);
    tu_knob(&ui, TU_K2, 1);
    ta_edit_get(&app, &e);
    CHECK(e.scale == TS_AEOLIAN, "MAJ, one click: MIN (%d)", e.scale);
    unsigned seen = 0;
    for (int i = 0; i < TS_COUNT; ++i) {
        tu_knob(&ui, TU_K2, 1);
        ta_edit_get(&app, &e);
        seen |= 1u << e.scale;
    }
    CHECK(seen == (1u << TS_COUNT) - 1 && e.scale == TS_AEOLIAN, "a full turn: every scale once, back to MIN (%x)", seen);
    tu_knob(&ui, TU_K2, -2);
    ta_edit_get(&app, &e);
    CHECK(e.scale == TS_MESSIAEN3, "MAJ, one click back: the last, MESS3 (%d)", e.scale);
    tu_knob(&ui, TU_K2, 1);
    tu_goto_page(&ui, TU_PG_HOME);
}

/* a composer's manners: the 256 seeds sorted by them (a block each, in the manners' order), LIBRARY's K4 jumps to the
 * next block, OCT walks through them, a slot keeps its manner */
static void test_manners(void)
{
    ta_play(&app, false);
    int bach = -1;
    for (int i = 0; i < ta_lib_count() && bach < 0; ++i)
        if (ta_lib_style(i) == TG_STYLE_BACH) bach = i;
    const int B = TG_STYLE_BACH, n = tg_manner_count(B);
    CHECK(bach >= 0 && n >= 2, "Bach has manners (%d)", n);
    ta_select(&app, bach);
    unsigned seen = 0;
    int prev = 0, unsorted = 0, off = 0;
    for (int s = 0; s < 256; ++s) {
        ta_set_seed(&app, (uint32_t)s);
        seen |= 1u << app.g.manner;
        unsorted += app.g.manner < prev;
        off += app.g.manner != ta_seed_manner(B, (uint32_t)s);
        prev = app.g.manner;
    }
    CHECK(seen == (1u << n) - 1 && !unsorted && !off, "the 256 seeds: every manner, in blocks in order (%x, %d, %d)", seen,
          unsorted, off);
    uint32_t sum = 0;
    for (int m = 0; m < n; ++m) {
        sum += ta_manner_size(B, m);
        CHECK(ta_manner_size(B, m) + 1u >= 256u / (uint32_t)n, "block %d: %u seeds", m, ta_manner_size(B, m));
        CHECK(ta_seed_manner(B, ta_manner_first(B, m)) == m && (m == 0 || ta_seed_manner(B, ta_manner_first(B, m) - 1u) == m - 1),
              "block %d starts at %u", m, ta_manner_first(B, m));
    }
    CHECK(sum == 256u, "the blocks hold the 256 seeds (%u)", sum);
    ta_set_seed(&app, 7);
    int m7 = app.g.manner;
    ta_set_seed(&app, 8);
    ta_set_seed(&app, 7);
    CHECK(app.g.manner == m7, "the same seed, the same manner (%d)", app.g.manner);
    /* K4: the first seed of the next manner's block */
    tu_goto_page(&ui, TU_PG_LIB);
    ta_set_seed(&app, 0);
    tu_knob(&ui, TU_K4, 1);
    const tg_style *base = tg_style_get(B);
    CHECK(app.g.manner == 1 && app.seed == ta_manner_first(B, 1) && !strcmp(ta_manner_name(&app), tg_manner_name(B, 1)),
          "K4: the second manner, %s, from its first seed (%u)", ta_manner_name(&app), app.seed);
    CHECK(app.g.style != base && app.g.style->name == base->name, "it plays the profile with its changes");
    char lb[32], want[32];
    ta_seed_label(&app, lb, sizeof lb);
    snprintf(want, sizeof want, "%s 1/%u", tg_manner_name(B, 1), ta_manner_size(B, 1));
    CHECK(!strcmp(lb, want), "the label: %s (%s)", lb, want);
    tu_knob(&ui, TU_K4, -1);
    CHECK(app.g.manner == 0 && app.seed == 0, "K4 back: the first manner's first seed (%u)", app.seed);
    /* OCT+ at a block's last seed: the next manner */
    ta_set_seed(&app, ta_manner_first(B, 1) - 1u);
    tu_goto_page(&ui, TU_PG_HOME);
    press(TU_B_OCTUP);
    CHECK(app.g.manner == 1 && !strncmp(ui.msg, tg_manner_name(B, 1), strlen(tg_manner_name(B, 1))),
          "OCT+ past the fugues: the next manner (%d, %s)", app.g.manner, ui.msg);
    tu_goto_page(&ui, TU_PG_LIB);
    lint_where = "manner";
    all_palettes("lib_manner");
    CHECK(ta_slot_save(&app, 8) == TP_OK, "saved slot 9");
    ta_set_seed(&app, 0);
    CHECK(ta_slot_load(&app, 8) == TP_OK && app.g.manner == 1, "the slot brings the manner back (%d)", app.g.manner);
    /* playing: a manner chosen comes in at the next half period */
    ta_set_seed(&app, 0);
    ta_play(&app, true);
    run_ms(200);
    int target = 2 < n ? 2 : 1;
    ta_set_manner(&app, target);
    CHECK(app.g.manner == 0 && app.seed_req == (int)ta_manner_first(B, target), "playing: it waits (%d, seed %d)",
          app.g.manner, app.seed_req);
    long guard = 0;
    while ((app.pending >= 0 || app.seed_req >= 0) && guard++ < 20000)
        ta_process(&app, 64);
    CHECK(app.g.manner == target, "and comes in (%d)", app.g.manner);
    ta_play(&app, false);
    ta_select(&app, 0);
    tu_goto_page(&ui, TU_PG_HOME);
}

/* favourites (REC held), the share code, the radio, MOOD, the clock in */
static void test_player(void)
{
    ta_play(&app, false);
    int bach = -1, reggae = -1;
    for (int i = 0; i < ta_lib_count(); ++i) {
        if (ta_lib_style(i) == TG_STYLE_BACH) bach = i;
        if (!strcmp(ta_lib_name(i), "REGGAE")) reggae = i;
    }
    CHECK(bach >= 0 && reggae >= 0, "Bach and reggae in the library");
    /* the share code */
    const int B = TG_STYLE_BACH;
    char code[TA_CODE], want[TA_CODE];
    ta_code_of(bach, ta_manner_first(B, 1) + 16u, code, sizeof code);
    snprintf(want, sizeof want, "BACH-%s-17", tg_manner_name(B, 1));
    CHECK(!strcmp(code, want), "Bach's code: %s (%s)", code, want);
    ta_code_of(reggae, 147, code, sizeof code);
    CHECK(!strcmp(code, "REGGAE-148"), "a genre's code: %s", code);
    int bad = 0;
    for (int i = 0; i < ta_lib_count(); ++i)
        for (uint32_t s = 0; s < 256; s += 37) {
            ta_code_of(i, s, code, sizeof code);
            if (ta_code_go(&app, code) != 0 || app.item != i || app.seed != s) {
                if (!bad++) printf("  code %s: item %d seed %u (want %d %u)\n", code, app.item, app.seed, i, s);
            }
        }
    CHECK(bad == 0, "every code plays its piece (%d not)", bad);
    snprintf(want, sizeof want, "bach %s 17", tg_manner_name(B, 1));
    for (char *c = want; *c; ++c) if (*c >= 'A' && *c <= 'Z') *c = (char)(*c + 32);
    CHECK(ta_code_go(&app, want) == 0 && app.item == bach && app.seed == ta_manner_first(B, 1) + 16u,
          "typed by hand: %s", want);
    CHECK(ta_code_go(&app, "NOBODY-1") < 0 && ta_code_go(&app, "BACH-FUGUE-999") < 0, "not a code: refused");
    /* favourites: REC held */
    tu_goto_page(&ui, TU_PG_HOME);
    ta_select_seed(&app, bach, 40);
    int deja = app.deja;
    tu_button(&ui, TU_B_REC, true);
    for (int t = 0; t < 1000; t += 16) tu_tick(&ui, 16);
    tu_button(&ui, TU_B_REC, false);
    CHECK(app.fav_n == 1 && app.deja == deja && ta_fav_find(&app, bach, 40) == 0, "REC held: a favourite, memory as it was");
    ta_select_seed(&app, reggae, 3);
    tu_button(&ui, TU_B_REC, true);
    for (int t = 0; t < 1000; t += 16) tu_tick(&ui, 16);
    tu_button(&ui, TU_B_REC, false);
    CHECK(app.fav_n == 2, "a second favourite (%d)", app.fav_n);
    CHECK(app.prefs_dirty, "kept to be written");
    for (int t = 0; t < 2500; t += 16) tu_tick(&ui, 16);
    CHECK(!app.prefs_dirty && mem_n[TP_PREFS_GROUP] > 0, "written, stopped, after 2 s");
    /* the favourites on LIBRARY: ERA below TUNA */
    tu_goto_page(&ui, TU_PG_LIB);
    ta_select(&app, 3);
    ui.browse = 3;
    tu_knob(&ui, TU_K1, -1);
    CHECK(ui.fav_browse == 0 && app.item == bach && app.seed == 40, "ERA below TUNA: the favourites, the first plays");
    shot("lib_favs");
    tu_knob(&ui, TU_K2, 1);
    CHECK(ui.fav_browse == 1 && app.item == reggae && app.seed == 3, "PIECE: the next favourite");
    tu_knob(&ui, TU_K1, 1);
    CHECK(ui.fav_browse < 0 && ta_lib_group(app.item) == 0, "ERA up: TUNA again");
    /* taken out again, REC held once more */
    ta_select_seed(&app, bach, 40);
    tu_button(&ui, TU_B_REC, true);
    for (int t = 0; t < 1000; t += 16) tu_tick(&ui, 16);
    tu_button(&ui, TU_B_REC, false);
    CHECK(app.fav_n == 1 && ta_fav_find(&app, bach, 40) < 0, "REC held on a favourite: taken out");
    /* the preferences come back from the store */
    ta_radio_set(&app, TA_RADIO_ERA, 7);
    for (int t = 0; t < 2500; t += 16) tu_tick(&ui, 16);
    static tuna_app app2;
    ta_init(&app2, SR, 3);
    ta_set_store(&app2, &store);
    CHECK(app2.fav_n == 1 && ta_fav_item(&app2, 0) == reggae && app2.fav[0].seed == 3 && app2.radio == TA_RADIO_ERA &&
              app2.radio_min == 7, "the favourites and the radio after a restart");
    /* the radio: another piece of the era after its minutes, at a cadence */
    ta_radio_set(&app, TA_RADIO_ERA, 1);
    ta_select(&app, bach);
    int from = app.item;
    ta_play(&app, true);
    long guard = 0;
    while (app.item == from && guard++ < 400000)
        ta_process(&app, 64);
    CHECK(app.item != from && ta_lib_group(app.item) == ta_lib_group(from), "the radio: the next piece, the same era (%s)",
          ta_lib_name(app.item));
    CHECK(app.blocks > 0 && (float)guard * 64.0f / SR >= 60.0f, "after its minute (%.0f s)", (double)guard * 64.0 / SR);
    ta_play(&app, false);
    ta_radio_set(&app, TA_RADIO_OFF, 5);
    /* MOOD: the card */
    tu_goto_page(&ui, TU_PG_HOME);
    tu_knob(&ui, TU_K4, 10);
    CHECK(app.g.chaos_macro > 0.5f, "K4 on HOME: MOOD up (%.2f)", (double)app.g.chaos_macro);
    shot("home_mood");
    tu_knob(&ui, TU_K4, -10);
    /* the clock in: CLOCK on ENV to IN, the tempo and the start from the clock */
    tu_goto_page(&ui, TU_PG_MIDI);
    tu_knob(&ui, TU_K3, 1);
    tu_knob(&ui, TU_K3, 1);
    CHECK(app.clock_in && !app.midi.clock, "CLOCK: IN (and nothing sent)");
    shot("midi_clock_in");
    ta_select(&app, bach);
    uint32_t fr = 0, per = (uint32_t)(SR * 60.0 / 100.0 / 24.0);   /* 100 BPM */
    ta_clock_msg(&app, 0xFA, fr);
    CHECK(app.playing, "Start: it plays");
    long ticks0 = app.g.ticks;
    for (int p = 0; p < 24 * 16; ++p) {                 /* 16 beats */
        ta_clock_msg(&app, 0xF8, fr);
        ta_process(&app, (int)per);
        fr += per;
    }
    float beats = (float)(app.g.ticks - ticks0) / (float)app.g.division;
    CHECK(app.g.bpm > 99.0f && app.g.bpm < 101.0f, "the clock's tempo: %.1f", (double)app.g.bpm);
    CHECK(beats > 15.5f && beats < 16.5f, "16 beats of clock, %.2f of Tuna", (double)beats);
    ta_clock_msg(&app, 0xFC, fr);
    CHECK(!app.playing, "Stop: it stops");
    ta_clock_msg(&app, 0xFA, fr);
    for (int i = 0; i < 20; ++i) ta_process(&app, 1024);   /* no pulses: the clock lost */
    CHECK(!app.playing, "the clock lost: it stops");
    tu_knob(&ui, TU_K3, -2);
    CHECK(!app.clock_in && !app.midi.clock, "CLOCK: OFF");
    ta_select(&app, 0);
    tu_goto_page(&ui, TU_PG_HOME);
}

/* every composer drawn on HOME and LIBRARY without a lint finding (names, chords, keys) */
static void test_every_composer(void)
{
    ta_play(&app, false);
    for (int i = 0; i < ta_lib_count(); ++i) {
        ta_select(&app, i);
        ui.browse = i;
        tu_goto_page(&ui, TU_PG_HOME);
        press(TU_B_PLAY);
        run_ms(1500);
        char w[48];
        snprintf(w, sizeof w, "home:%s", ta_lib_name(i));
        lint_where = w;
        tu_draw_all(&ui);
        tu_goto_page(&ui, TU_PG_LIB);
        snprintf(w, sizeof w, "library:%s", ta_lib_name(i));
        lint_where = w;
        tu_draw_all(&ui);
        tu_goto_page(&ui, TU_PG_KEY);
        tu_draw_all(&ui);
        tu_goto_page(&ui, TU_PG_TIME);
        tu_draw_all(&ui);
        tu_goto_page(&ui, TU_PG_PARTS);
        tu_draw_all(&ui);
        tu_goto_page(&ui, TU_PG_MIX);
        tu_draw_all(&ui);
        press(TU_B_PLAY);
        /* each of its other manners: HOME (the footer names it) and LIBRARY (its card) */
        int st = ta_lib_style(i);
        for (int m = 1; m < tg_manner_count(st); ++m) {
            ta_set_manner(&app, m);
            CHECK(app.g.manner == m, "%s: manner %d plays (%d)", ta_lib_name(i), m, app.g.manner);
            snprintf(w, sizeof w, "manner:%s %s", ta_lib_name(i), tg_manner_name(st, m));
            lint_where = w;
            press(TU_B_PLAY);
            run_ms(600);
            tu_goto_page(&ui, TU_PG_HOME);
            tu_draw_all(&ui);
            tu_goto_page(&ui, TU_PG_LIB);
            tu_draw_all(&ui);
            press(TU_B_PLAY);
        }
        ta_set_manner(&app, -1);
    }
}

/* frames of HOME playing, for an animation (tests/tuna_ui_test.sh makes the GIF) */
static void anim(int item, const char *tag)
{
    if (!outdir)
        return;
    ta_play(&app, false);
    ta_select(&app, item);
    ta_set_chaos(&app, 0.5f, 0.5f, 0.5f, 0.5f);
    tu_goto_page(&ui, TU_PG_HOME);
    ui.palette = 1;
    palette_set(1);
    ta_play(&app, true);
    run_ms(4000);
    for (int f = 0; f < 120; ++f) {
        char name[64];
        for (int k = 0; k < 4; ++k) {
            ta_process(&app, (int)(SR * 16 / 1000));
            tu_tick(&ui, 16);
        }
        if (f == 40) tu_knob(&ui, TU_K3, 12);        /* DENSITY up, for a while */
        if (f == 80) tu_knob(&ui, TU_K3, -12);
        lint_where = "anim";
        tu_draw_all(&ui);
        snprintf(name, sizeof name, "anim_%s_%03d", tag, f);
        write_ppm(name);
    }
    ta_play(&app, false);
}

int main(int argc, char **argv)
{
    CHECK(!strcmp(UI_PALETTES[TU_PALETTE_DEFAULT].name, "ICE"), "Tuna's colour: ICE (%s)", UI_PALETTES[TU_PALETTE_DEFAULT].name);
    outdir = argc > 1 ? argv[1] : NULL;
    ta_init(&app, SR, 1);
    ta_set_sound(&app, sound, NULL);
    ta_set_store(&app, &store);
    tu_init(&ui, &app);
    test_screens();
    test_controls();
    test_reset();
    test_chaos_page();
    test_reverb_page();
    test_scale_order();
    test_manners();
    test_player();
    test_every_composer();
    for (int i = 0; i < ta_lib_count(); ++i) {
        if (ta_lib_style(i) == TG_STYLE_DEBUSSY) anim(i, "debussy");
        if (ta_lib_style(i) == TG_STYLE_PHASING) anim(i, "phasing");
    }
    CHECK(lint_findings == 0, "%d layout findings", lint_findings);
    printf("tuna_ui: %d checks, %d failed, %d frames written\n", checks, fails, frames_written);
    return fails ? 1 : 0;
}
