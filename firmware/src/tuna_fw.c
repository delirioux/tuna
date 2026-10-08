/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA on the FM-1: the glue between Tuna (its generator, app and screen) and Felucca's firmware (the engines,
 * voices and effects, the panel, the screen driver, USB MIDI, the flash). Included by felucca.c after Felucca's
 * own sources when FELUCCA_TUNA is 1; Tuna's other modules (tuna_gen.c and the rest) are their own translation
 * units, linked beside felucca.o (tools/build.py).
 *
 * Who runs what:
 *   the audio ISR (audio.c audio_block -> tf_audio_pre / tf_audio_post): Tuna's generator at a quarter of the
 *     audio rate, its notes straight into Felucca's tracks (trk_note_on / off), its sound locks into their
 *     parameters, its MIDI into the USB out ring; after Felucca's mix, the resonator on the master;
 *   the main loop (main.c -> tf_frame, tf_input): the panel into Tuna's controls (with the audio ISR held off
 *     while one acts, so the generator is never seen half changed), what the ISR asked for that needs the main
 *     loop (an instrument change loads a Felucca sound: set_engine_of, apply_preset_to), the mixer into the
 *     tracks, the LEDs, the screen.
 * Parts to tracks: bass 1, melody 2, the third part 3, drums and timpani 4 (the DRUM engine: GM notes; the
 * timpani are its toms, the kit retuned so that GM 45 sounds the dominant and GM 48 the tonic), as in
 * tests/tuna_felucca.c, which renders the same mapping on the host. */
#include "tuna_app.h"
#include "tuna_ui.h"
#include "tuna_ui.c"

#define TF_DIV 4                                   /* Tuna at a quarter of the audio rate (11 kHz) */

static tuna_app tf_app;
static tuna_ui tf_ui;

/* ---- Tuna's instrument families on Felucca's sounds: tuna_sounds.h (shared with tests/tuna_felucca.c) ---- */
#include "tuna_sounds.h"

/* asked for by the ISR, done by the main loop */
static volatile int8_t tf_family_req[3] = {-1, -1, -1};
static int8_t tf_family_cur[3] = {-1, -1, -1};
static int16_t tf_base[NTRK][TG_LK_COUNT];        /* a track's own values under the sound locks */
static uint8_t tf_level_set[NTRK], tf_rev_set;     /* the mixer last put into the tracks */
static int tf_timp_root = -1;

static int tf_lk_param(int lk)
{
    if (lk >= TG_LK_E0 && lk <= TG_LK_E7) return P_E0 + (lk - TG_LK_E0);
    switch (lk) {
    case TG_LK_LEVEL: return P_LEVEL;
    case TG_LK_PAN:   return P_PAN;
    case TG_LK_DIST:  return P_DIST;
    case TG_LK_CHO:   return P_CHOR;
    case TG_LK_DLY:   return P_DLY;
    case TG_LK_REV:   return P_REV;
    default:          return -1;
    }
}

static void tf_snapshot(uint32_t t)
{
    for (int lk = 0; lk < TG_LK_COUNT; ++lk)
        if (tf_lk_param(lk) >= 0)
            tf_base[t][lk] = trk[t].p[tf_lk_param(lk)];
}

/* a part's settings over its new sound: chords polyphonic, the mixer's level and reverb, short releases for
 * quick figures (a long tail would hold one of the 8 shared voices) */
static void tf_part_settings(uint32_t k)
{
    const tg_gen *g = &tf_app.g;
    if (k != 1u)
        trk[k].p[P_VOICE] = V_POLY;
    trk[k].p[P_LEVEL] = tsn_level(tf_app.level[k] * 127 / 100, k < 3u ? tf_family_cur[k] : TI_DEFAULT);
    trk[k].p[P_REV] = (int16_t)(tf_app.reverb * 127 / 100);
    if (k == 2u && (g->pad_mode == TG_PAD_ALBERTI || g->pad_mode == TG_PAD_BROKEN || g->pad_mode == TG_PAD_ARPEGGIO ||
                    g->pad_mode == TG_PAD_REPEAT || g->pad_mode == TG_PAD_COUNTER || g->pad_mode == TG_PAD_TREMOLO ||
                    g->pad_mode == TG_PAD_FIGURE || g->pad_mode == TG_PAD_ADDITIVE || g->pad_mode == TG_PAD_OOMPAH ||
                    g->pad_mode == TG_PAD_PHASE) && trk[2].p[P_REL] > 34)
        trk[2].p[P_REL] = 34;
    if (k == 0u && (g->bass_mode == TG_BASS_REPEAT || g->bass_mode == TG_BASS_OSTINATO || g->bass_mode == TG_BASS_BEAT) &&
        trk[0].p[P_REL] > 40)
        trk[0].p[P_REL] = 40;
    if (k == 2u && g->pad_mode == TG_PAD_RHYTHM && trk[2].p[P_REL] > 30)
        trk[2].p[P_REL] = 30;                      /* struck chords: short */
    /* what the family's preset does not hold: distortion, glide, the voice mode */
    int fam = tf_family_cur[k];
    if (k < 3u && fam > TI_DEFAULT && fam < TI_COUNT) {
        const tsn_family *f = &TSN_FAMILY[fam];
        if (f->dist != TSN_KEEP) trk[k].p[P_DIST] = f->dist;
        if (f->glide != TSN_KEEP) trk[k].p[P_GLIDE] = f->glide;
        if (f->voice != TSN_KEEP) trk[k].p[P_VOICE] = f->voice;
    }
}

/* the main loop: a family the ISR asked for becomes the track's Felucca sound (the engine fades over, as a
 * preset load from the panel does) */
static void tf_load_family(uint32_t k, int fam)
{
    if (fam <= TI_DEFAULT || fam >= TI_COUNT || fam == tf_family_cur[k])
        return;
    tf_family_cur[k] = (int8_t)fam;
    undo_depth++;                                  /* (no undo copy of Tuna's loads) */
    set_engine_of(&trk[k], TSN_FAMILY[fam].engine);
    apply_preset_to(&trk[k], TSN_FAMILY[fam].preset);
    undo_depth--;
    fm1_irq_off();
    if (TSN_FAMILY[fam].own)
        tsn_apply_own(trk[k].p, TSN_FAMILY[fam].own);
    tf_part_settings(k);
    tf_snapshot(k);
    fm1_irq_on();
}

/* the DRUM kit retuned (E1) so that its tom at GM 45 sounds the dominant, at GM 48 the tonic */
static void tf_tune_timpani(int root)
{
    int dom = (root + 7) % 12, n = dom - 9;
    if (n > 5) n -= 12;
    if (n < -6) n += 12;
    int e1 = 64 + (n * 16 + (n >= 0 ? 1 : -1)) / 3;   /* 3/16 semitone a step, rounded */
    trk[3].p[P_E0 + 1] = (int16_t)(e1 < 0 ? 0 : e1 > 127 ? 127 : e1);
}

/* a tuned note: the voice just started moves by the note's cents (1/16 semitone steps) */
static void tf_tune_voice(track_t *t, uint32_t note, int cents10)
{
    if (!cents10 || t->eng_req != t->engine || t->xf_on)
        return;
    voice_t *v = trk_vmode(t) == V_POLY ? &t->v[t->m_vi] : &t->v[0];
    if (v->note != note)
        return;
    int d = (cents10 * 16 + (cents10 >= 0 ? 500 : -500)) / 1000;
    v->pitch16 += d;
    if (!v->gstep)
        v->pitch_cur = v->pitch16;
}

/* ---- the audio ISR ---------------------------------------------------------------------------------- */
static void tf_sound(void *ctx, const tg_event *e)
{
    (void)ctx;
    uint32_t tr = e->part >= TG_PART_DRUMS ? 3u : e->part;
    if (e->type == TG_EV_PROGRAM) {
        if (tr < 3u)
            tf_family_req[tr] = (int8_t)e->value;  /* (the main loop loads it) */
        return;
    }
    if (e->type == TG_EV_PARAM) {
        int prm = tf_lk_param(e->param);
        if (prm >= 0)
            trk[tr].p[prm] = e->value == TG_LK_RESTORE ? tf_base[tr][e->param] : e->value;
        return;
    }
    uint32_t note = e->note;
    if (e->part == TG_PART_TIMPANI) {
        int pc = ((e->note - tf_app.g.root) % 12 + 12) % 12;
        note = pc == 0 ? 48u : 45u;
    }
    if (e->on) {
        trk_note_on(&trk[tr], note, e->velocity);
        if (tr != 3u)
            tf_tune_voice(&trk[tr], note, e->value);
    } else if (tr != 3u) {
        trk_note_off(&trk[tr], note);              /* a drum hit plays on through its decay */
    }
}

/* MIDI out: Tuna's queue into USB-MIDI packets (cable 0) */
static void tf_midi_out(void)
{
    tmi_msg m;
    while (tmi_pop(&tf_app.midi, &m)) {
        uint32_t st = m.b[0], cin = st >= 0xF0u ? 0x0Fu : st >> 4;
        midi_out_event(cin | st << 8 | (uint32_t)m.b[1] << 16 | (uint32_t)m.b[2] << 24);
    }
}

/* before Felucca's mix: Tuna for this block */
static void tf_audio_pre(uint32_t n)
{
    /* the drum kit: a groove's own, or Felucca's with the toms tuned as timpani (the main loop sets the kit:
     * tf_kit) */
    if (tf_app.g.perc_mode != TG_PERC_GROOVE && tf_app.g.root != tf_timp_root) {
        tf_tune_timpani(tf_app.g.root);
        tf_timp_root = tf_app.g.root;
    }
    ta_process(&tf_app, (int)(n / TF_DIV));
    tf_midi_out();
}

/* after it: the resonator on the master (Q15 stereo, before audio.c's shift to 24 bits) */
static void tf_audio_post(int32_t *out, uint32_t n)
{
    if (tf_app.reso.amount <= 0)
        return;
    int32_t in[CTL];
    uint32_t i;
    for (i = 0; i < n && i < CTL; i++)
        in[i] = (out[2u * i] + out[2u * i + 1u]) >> 1;
    tr_process(&tf_app.reso, in, out, (int)i);
    for (i = 0; i < 2u * n; i++)
        out[i] = out[i] > 32767 ? 32767 : out[i] < -32768 ? -32768 : out[i];
}

/* ---- the slots: Felucca's flash objects of the projects, a group of four slots each ---------------------- */
#if FELUCCA_FLASH
static int tf_store_load(void *ctx, int group, uint8_t *buf, int max)
{
    (void)ctx;
    return st_load(OBJ_PROJECT0 + (uint32_t)group, buf, (uint32_t)max);
}
static int tf_store_save(void *ctx, int group, const uint8_t *buf, int n)
{
    (void)ctx;
    return st_save(OBJ_PROJECT0 + (uint32_t)group, buf, (uint32_t)n);
}
static const tp_store tf_store = {tf_store_load, tf_store_save, 0};
#endif

/* a seed of its own at every power-on: the ADC's low bits (noise on the battery divider, where the MASTER pot
 * stands) and the timer between the reads, through FNV-1a */
static uint32_t tf_seed_boot;
static bool tf_seed_mixed;
static uint32_t tf_mix(uint32_t h, uint32_t v)
{
    for (int i = 0; i < 4; i++, v >>= 8)
        h = (h ^ (v & 0xFFu)) * 16777619u;
    return h;
}
static uint32_t tf_entropy(void)
{
    uint32_t h = 2166136261u;
    for (int i = 0; i < 64; i++) {
        h = tf_mix(h, (uint32_t)fm1_adc_read(FM1_ADC_BATT));
        h = tf_mix(h, (uint32_t)fm1_adc_read(FM1_ADC_MASTER));
        h = tf_mix(h, fm1_ticks());
    }
    return h ? h : 1u;
}

/* ---- start ------------------------------------------------------------------------------------------- */
static void tf_init(void)
{
    uint32_t i;
    for (i = 0; i < G_COUNT; i++)
        song.g[i] = GP[i].def;
    undo_depth++;
    fm6_init();
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        track_defaults(t);
        set_engine_of(t, i == 3u ? ENGI_DRUM : TSN_ANALOG);   /* the drums on track 4, the rest Tuna's */
        apply_preset_to(t, i == 3u ? 0u : 1u);
        t->engine = t->eng_req;
        track_defaults_steps(t);                   /* (Felucca's sequencer stays empty) */
    }
    undo_depth--;
    song.sel = 0;
    song.master_q12 = 2048;
    tf_seed_boot = tf_entropy() % TA_SEEDS;        /* another piece at every power-on */
    ta_init(&tf_app, (float)FS / (float)TF_DIV, tf_seed_boot);
    tr_init(&tf_app.reso, (float)FS);              /* (the resonator runs at the audio rate) */
    ta_reso_set(&tf_app, tf_app.reso_amount, tf_app.reso_decay, tf_app.reso_bright, tf_app.reso_mode);
    ta_set_sound(&tf_app, tf_sound, 0);
#if FELUCCA_FLASH
    ta_set_store(&tf_app, &tf_store);
#endif
    for (i = 0; i < 3u; i++) {
        tf_part_settings(i);
        tf_snapshot(i);
    }
    trk[3].p[P_LEVEL] = (int16_t)(tf_app.level[3] * 127 / 100);
    tf_snapshot(3);
    tu_init(&tf_ui, &tf_app);
    tf_ui.palette = (int)settings.palette;
    palette_set(settings.palette);
}

/* ---- the main loop ---------------------------------------------------------------------------------- */
/* the panel into Tuna's controls; the audio ISR held off while each acts */
static uint32_t tf_notes_prev;
static void tf_input(void)
{
    uint32_t released = 0, pressed = fm1_input_edges(&released), b, k;
    static const uint8_t KNOB[NE] = {TU_K_SELECT, TU_K_ALGO, TU_K_PRESETS, TU_K1, TU_K2, TU_K3, TU_K4};
    fm1_input_note_edges();                        /* (the keys: from their state below) */
    for (b = 0; b < NB; b++) {                     /* (Felucca's B_* are Tuna's TU_B_*, in the same order) */
        uint32_t bit = 1u << panel.btn[b];
        if (pressed & bit) {
            fm1_irq_off();
            ta_stir(&tf_app, fm1_ticks());        /* (the moment of each touch: the next piece's seed) */
            /* the first PLAY mixes in when it came (the ADC may be quiet), unless a seed was chosen since */
            if (b == TU_B_PLAY && !tf_seed_mixed && !tf_app.playing && tf_app.seed == tf_seed_boot)
                ta_set_seed(&tf_app, tf_mix(tf_seed_boot, fm1_ticks()));
            if (b == TU_B_PLAY)
                tf_seed_mixed = true;
            tu_button(&tf_ui, (int)b, true);
            fm1_irq_on();
        }
        if (released & bit) {
            fm1_irq_off();
            tu_button(&tf_ui, (int)b, false);
            fm1_irq_on();
        }
    }
    for (k = 0; k < NE; k++) {
        int32_t s = panel_enc(k);
        if (s) {
            fm1_irq_off();
            ta_stir(&tf_app, fm1_ticks());
            tu_knob(&tf_ui, KNOB[k], s);
            fm1_irq_on();
        }
    }
    uint32_t notes = fm1_in.notes, ch = notes ^ tf_notes_prev;
    for (k = 0; ch && k < 27u; k++)
        if ((ch >> k) & 1u) {
            fm1_irq_off();
            tu_key(&tf_ui, TU_KEY_LO + (int)k, (notes >> k) & 1u);
            fm1_irq_on();
        }
    tf_notes_prev = notes;
}

/* what the ISR asked for, the mixer into the tracks */
static void tf_service(void)
{
    uint32_t k;
    for (k = 0; k < 3u; k++) {
        int fam = tf_family_req[k];
        if (fam >= 0) {
            tf_family_req[k] = -1;
            tf_load_family(k, fam);
        }
    }
    for (k = 0; k < NTRK; k++)
        if (tf_app.level[k] != tf_level_set[k] || tf_app.reverb != tf_rev_set) {
            fm1_irq_off();
            trk[k].p[P_LEVEL] = tsn_level(tf_app.level[k] * 127 / 100, k < 3u ? tf_family_cur[k] : TI_DEFAULT);
            if (k < 3u)
                trk[k].p[P_REV] = (int16_t)(tf_app.reverb * 127 / 100);
            tf_snapshot(k);
            fm1_irq_on();
            tf_level_set[k] = tf_app.level[k];
        }
    tf_rev_set = tf_app.reverb;
    /* the reverb itself: Felucca's shared bus (changing the type fades the old one out, fx.c) */
    int16_t rs = (int16_t)(tf_app.rev_size * 127 / 100), rd = (int16_t)(tf_app.rev_damp * 127 / 100);
    int16_t rt = ta_reverb_type(tf_app.rev_type)->spring ? 1 : 0;   /* (G_RTYPE: 0 ROOM, 1 SPRING) */
    if (song.g[G_RSIZE] != rs || song.g[G_RDAMP] != rd || song.g[G_RTYPE] != rt) {
        fm1_irq_off();
        song.g[G_RSIZE] = rs;
        song.g[G_RDAMP] = rd;
        song.g[G_RTYPE] = rt;
        fm1_irq_on();
    }
    /* the drum kit follows the style: a groove's own kit, else Felucca's (its toms tuned as timpani) */
    static const tg_groove *kit_for = (const tg_groove *)1;
    const tg_groove *gr = tf_app.g.perc_mode == TG_PERC_GROOVE && tf_app.g.style && tf_app.g.style->groove
                              ? tg_groove_get(tf_app.g.style->groove - 1) : 0;
    if (gr != kit_for) {
        int16_t e[8];
        tsn_kit(gr, e);
        fm1_irq_off();
        for (k = 0; k < 8u; k++)
            trk[3].p[P_E0 + k] = e[k];
        tf_timp_root = -1;                         /* (Felucca's kit: retuned as timpani again) */
        tf_snapshot(3);
        fm1_irq_on();
        kit_for = gr;
    }
    song.playing = tf_app.playing;                 /* (PLAY's green LED, Felucca's meters) */
}

/* the LEDs: the page's button lit, PLAY green while playing, REC with the memory locked, the keys held and
 * the notes of the chord sounding */
static void tf_leds(void)
{
    static const uint8_t PAGE_BTN[TU_PAGES] = {
        [TU_PG_HOME] = B_HOME, [TU_PG_LIB] = B_FX, [TU_PG_KEY] = B_SCL, [TU_PG_TIME] = B_EDIT, [TU_PG_PARTS] = B_EDIT,
        [TU_PG_EVOLVE] = B_ARP, [TU_PG_SEQ] = B_SEQ, [TU_PG_MIX] = B_GLO, [TU_PG_FX] = B_GLO, [TU_PG_SAVE] = B_SAVE,
        [TU_PG_LOAD] = B_SAVE, [TU_PG_MENU] = B_HOME, [TU_PG_MIDI] = B_ENV, [TU_PG_RESO] = B_LFO,
        [TU_PG_CHAOS] = B_EDIT};
    static uint8_t ready;
    uint8_t nl[FM1_NCOL] = {0}, nd[FM1_NCOL] = {0};
    uint32_t k, c;
    if (!ready) {
        led_pos_init();
        ready = 1;
    }
    led_put(nl, panel.btn[PAGE_BTN[tf_ui.page % TU_PAGES]], 1);
    led_put(nl, panel.btn[B_REC], tf_app.deja == TA_DV_LOCK);
    for (k = 0; k < NB; k++)
        led_put(nd, panel.btn[k], 1);
    {
        const tg_gen *g = &tf_app.g;
        uint16_t chord = 0;
        for (c = 0; tf_app.playing && c < g->chord.n; c++)
            chord |= (uint16_t)(1u << ((g->chord.root + g->chord.tone[c] + g->root) % 12));
        for (k = 0; k < 27u; k++) {
            uint32_t pc = (TU_KEY_LO + k) % 12u;
            led_put(nl, 14u + k, (int)(((fm1_in.notes >> k) & 1u) || ((chord >> pc) & 1u)));
            led_put(nd, 14u + k, 1);
        }
    }
    if (tf_app.playing)
        led_clear(nd, panel.btn[B_PLAY]);
    for (c = 0; c < FM1_NCOL; c++)
        nd[c] = settings_leds == LEDS_DIM || settings_leds == LEDS_DIM_LO ? (uint8_t)(nd[c] & ~nl[c]) : 0u;
    if (tf_app.playing)
        nl[LED_PLAY_GREEN >> 3] |= (uint8_t)(1u << (LED_PLAY_GREEN & 7u));
    fm1_led_dim_level(settings_leds == LEDS_DIM_LO);
    for (c = 0; c < FM1_NCOL; c++) {
        fm1_led_dim[c] = nd[c];
        fm1_led_breath[c] = 0;
        fm1_led[c] = nl[c];
    }
}

/* a frame: the screen, its clock */
static uint32_t tf_last_ms;
static void tf_frame(void)
{
    uint32_t now = fm1_ms, dt = now - tf_last_ms;
    static uint8_t first = 1;
    tf_last_ms = now;
    tf_service();
    tf_leds();
    tf_ui.batt = (int)batt_shown();
    if (tf_ui.palette != (int)settings.palette) {  /* (the menu's colour, kept as Felucca keeps its own) */
        settings.palette = (uint32_t)tf_ui.palette;
        settings_save();
    }
    tu_tick(&tf_ui, dt > 100u ? 100 : (int)dt);
    if (first) {
        tu_draw_all(&tf_ui);
        first = 0;
    } else {
        tu_draw(&tf_ui);
    }
}
