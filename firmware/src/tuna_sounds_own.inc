/* SPDX-License-Identifier: GPL-3.0-only */
/* Tuna's own sounds on Felucca's engines (tuna_sounds.h TSN_OWN): Felucca's preset layout (core.h preset_t),
 * {name, E0..E7, ATK DEC SUS REL, ENV->FLT, mono, FX(dist, chorus, delay, reverb)}. The E values per engine:
 *   ANALOG  WAVE(SAW SQR TRI SIN PWM) DTN MIX NOIS CUT RES DRV KTR
 *   PHASE   WAVE WAVE2 DCW ENV DTN LINE SUB -
 *   LOFI    CHIP WAVE DUTY CRSH SWP VIB ARP TONE
 *   TRIO    WAVE INT2 INT3 DTN MODE CUT RES PW
 *   WHEEL   REG(FLUTE MELLO HOLLW SMOOT 3BAR BLUES GOSPL ROCK TOPS CLARI REED STRNG CHAPL BRITE BASS FULL)
 *           SUB BODY TOP PERC CLICK DRV ROTR
 *   PHYS    MODEL(0 MODAL 1 STRING 2 MEMB 3 SYMP) STRC|CHRD|HARM BRIT DAMP POS|SYMP ACC BOW|BUZZ|BEND EXC
 * Measured against each other with tests/tuna_audition.sh. */

/* the sound-alike pairs, apart */
static const preset_t TSN_CELLO = {"CELLO", {0, 6, 34, 30, 8, 0, 0, 0}, {55, 90, 105, 75}, 0, 0, FX(0, 40, 15, 60)};
static const preset_t TSN_CONTRABASS = {"CONTRABASS", {0, 0, 26, 25, 5, 0, 30, 0}, {30, 90, 100, 55}, 0, 0,
                                        FX(0, 20, 10, 40)};
static const preset_t TSN_TUBA = {"TUBA", {0, 0, 22, 70, 0, 0, 70, 0}, {20, 80, 100, 45}, 0, 0, FX(0, 10, 10, 40)};
static const preset_t TSN_CLAV = {"CLAV", {1, 40, 120, 70, 8, 115, 0, 30}, {0, 100, 127, 25}, 0, 0, FX(10, 20, 15, 25)};
static const preset_t TSN_WHISTLE = {"WHISTLE", {2, 1, 0, 0, 0, 30, 0, 70}, {10, 64, 120, 30}, 0, 1, FX(0, 0, 25, 45)};
static const preset_t TSN_BANDONEON = {"BANDONEON", {10, 0, 0, 1, 0, 0, 15, 0}, {25, 64, 127, 40}, 0, 0,
                                       FX(0, 30, 10, 40)};
static const preset_t TSN_BASSOON = {"BASSOON", {0, 0, 0, 6, 50, 45, 10, 30}, {25, 80, 100, 40}, 10, 0,
                                     FX(0, 10, 10, 40)};
static const preset_t TSN_SAWLEAD = {"SAW LEAD", {0, 0, 12, 12, 0, 96, 25, 64}, {2, 70, 100, 35}, 20, 1,
                                     FX(0, 10, 35, 25)};
static const preset_t TSN_CLARINET = {"CLARINET", {9, 0, 0, 0, 0, 0, 0, 0}, {20, 64, 127, 40}, 0, 0, FX(0, 10, 10, 45)};
static const preset_t TSN_PIZZ = {"PIZZ", {1, 56, 45, 45, 50, 90, 0, 0}, {0, 100, 127, 30}, 0, 0, FX(0, 10, 10, 45)};
static const preset_t TSN_EGUITAR = {"E.GUITAR", {1, 36, 85, 90, 20, 100, 0, 10}, {0, 100, 127, 40}, 0, 0,
                                     FX(0, 40, 25, 30)};

/* new instruments */
static const preset_t TSN_ACCORDION = {"ACCORDION", {10, 0, 0, 2, 0, 0, 10, 0}, {15, 64, 127, 35}, 0, 0,
                                       FX(0, 90, 10, 35)};
static const preset_t TSN_KOTO = {"KOTO", {1, 45, 100, 75, 15, 110, 0, 25}, {0, 100, 127, 50}, 0, 0, FX(0, 10, 20, 50)};
static const preset_t TSN_SITAR = {"SITAR", {3, 20, 110, 95, 90, 100, 100, 10}, {0, 100, 127, 70}, 0, 0,
                                   FX(0, 10, 20, 50)};
static const preset_t TSN_OUD = {"OUD", {3, 0, 70, 80, 40, 100, 30, 15}, {0, 100, 127, 50}, 0, 0, FX(0, 10, 15, 45)};
static const preset_t TSN_BANJO = {"BANJO", {1, 30, 127, 55, 8, 120, 0, 40}, {0, 100, 127, 30}, 0, 0, FX(0, 0, 15, 30)};
static const preset_t TSN_DULCIMER = {"DULCIMER", {1, 40, 100, 100, 30, 110, 0, 60}, {0, 100, 127, 80}, 0, 0,
                                      FX(0, 60, 20, 50)};
static const preset_t TSN_VIBES = {"VIBES", {0, 34, 40, 100, 20, 90, 0, 10}, {0, 100, 127, 85}, 0, 0, FX(0, 50, 20, 50)};
static const preset_t TSN_XYLO = {"XYLO", {0, 34, 60, 55, 0, 110, 0, 30}, {0, 100, 127, 40}, 0, 0, FX(0, 0, 15, 40)};
static const preset_t TSN_GLOCK = {"GLOCK", {0, 36, 90, 90, 10, 110, 0, 20}, {0, 100, 127, 80}, 0, 0, FX(0, 10, 20, 55)};
static const preset_t TSN_TUBULAR = {"TUBULAR", {0, 90, 70, 122, 20, 100, 0, 10}, {0, 100, 127, 90}, 0, 0,
                                     FX(0, 10, 15, 60)};
static const preset_t TSN_MUSICBOX = {"MUSICBOX", {0, 30, 80, 80, 5, 100, 0, 40}, {0, 100, 127, 70}, 0, 0,
                                      FX(0, 10, 25, 60)};
static const preset_t TSN_TABLA = {"TABLA", {2, 110, 80, 60, 40, 100, 30, 30}, {0, 100, 127, 50}, 0, 0, FX(0, 0, 10, 35)};
static const preset_t TSN_STEELPAN = {"STEELPAN", {0, 33, 85, 70, 12, 110, 0, 25}, {0, 100, 127, 50}, 0, 0, FX(0, 10, 15, 45)};
