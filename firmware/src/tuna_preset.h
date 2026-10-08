/* SPDX-License-Identifier: GPL-3.0-only */
/* TUNA presets: the whole state of Tuna in a small versioned record, 16 storage slots, the composer
 * library by era, and the character presets.
 *
 * A record holds the composer (or none), every generator setting (key, tempo, seed, meter, harmony,
 * melody, accompaniment, percussion, evolution, tuning, Marbles, the chaos knobs), the sequencer overlay (trig
 * conditions and locks), a frozen pattern if there is one, a name, and the host's sounds per track.
 * Loading it gives back exactly the music that was saved: the same seed and settings play the same
 * notes.
 *
 * Storage: the 16 slots live in 4 flash objects (groups) of 4 slots each, written one object at a time
 * through the host's storage (Felucca's A/B sectors: a power cut never loses the previous copy). A group
 * holds 3840 bytes; a slot without a pattern takes about 250, a full 384-note pattern adds 2.3 KB (so a
 * group holds one full pattern, or several smaller ones; a save that does not fit says so). */
#ifndef TUNA_PRESET_H
#define TUNA_PRESET_H

#include <stdbool.h>
#include <stdint.h>
#include "tuna_gen.h"

#define TP_VERSION 3                      /* 2: which parts follow the chaos knobs; 3: the reverb's size, damping, type */
#define TP_SLOTS 16
#define TP_GROUPS 4
#define TP_SLOTS_PER_GROUP (TP_SLOTS / TP_GROUPS)
#define TP_GROUP_MAX 3840                 /* one storage object's payload (Felucca: ST_PAYLOAD_MAX) */
#define TP_NAME 12
#define TP_RECORD_MAX 3800                /* the largest record (a full pattern and all 64 steps) */
#define TP_TRACKS 4

enum {
    TP_OK = 0,
    TP_ERR_SPACE = -1,                    /* the buffer, or the slot's group, is too small */
    TP_ERR_FORMAT = -2,                   /* not a Tuna record, or damaged */
    TP_ERR_VERSION = -3,                  /* written by a newer Tuna */
    TP_ERR_EMPTY = -4,                    /* nothing saved in the slot */
    TP_ERR_STORE = -5,                    /* the storage refused */
    TP_ERR_ARG = -6,
};

/* the host's sound per track (Felucca: engine, preset, level, pan, reverb send) */
typedef struct { uint8_t engine, preset, level, pan, rev; } tp_sound;
typedef struct {
    char name[TP_NAME + 1];
    bool sounds_set;                      /* false: the composer's own instruments */
    tp_sound sound[TP_TRACKS];
    uint8_t reso[4];                      /* the resonator: amount, decay, brightness (0..100), tuning mode */
    uint8_t reverb[3];                    /* version 3: the reverb's size, damping (0..100), type (0 room, 1 spring);
                                             older records: 255, the host's defaults */
} tp_host;

/* record <-> generator */
int tp_encode(const tg_gen *g, const tp_host *host, uint8_t *buf, int max);   /* bytes, or TP_ERR_* */
int tp_decode(tg_gen *g, tp_host *host, const uint8_t *buf, int n);           /* TP_OK or TP_ERR_*; the
                                         generator restarts from the record (keep its sample rate) */
int tp_peek_name(const uint8_t *buf, int n, char *name);                      /* the record's name */

/* storage: one object per group, through the host */
typedef struct {
    int (*load)(void *ctx, int group, uint8_t *buf, int max);       /* bytes, or < 0: empty / unreadable */
    int (*save)(void *ctx, int group, const uint8_t *buf, int n);   /* 0 on success */
    void *ctx;
} tp_store;

int tp_slot_save(const tp_store *s, int slot, const uint8_t *rec, int n);
int tp_slot_load(const tp_store *s, int slot, uint8_t *rec, int max);       /* bytes, or TP_ERR_* */
int tp_slot_clear(const tp_store *s, int slot);
/* which slots hold something (bit per slot), and their names (names may be NULL) */
uint16_t tp_slots_used(const tp_store *s, char names[TP_SLOTS][TP_NAME + 1]);
int tp_group_free(const tp_store *s, int group);                            /* bytes left in a group */

/* the composer library, by era */
enum { TP_ERA_RENAISSANCE, TP_ERA_BAROQUE, TP_ERA_CLASSICAL, TP_ERA_EARLY_ROMANTIC, TP_ERA_ROMANTIC,
       TP_ERA_LATE_ROMANTIC, TP_ERA_MODERN, TP_ERA_MINIMAL, TP_ERA_FILM,
       TP_ERA_1950S, TP_ERA_1960S, TP_ERA_1970S, TP_ERA_1980S, TP_ERA_1990S, TP_ERA_2000S, TP_ERA_2010S, TP_ERA_2020S,
       TP_ERA_COUNT };
const char *tp_era_name(int era);
int tp_era_count(int era);                /* composers in the era */
int tp_era_style(int era, int i);         /* TG_STYLE_* of the i-th composer, by year of birth */
int tp_style_era(int style);
int tp_style_born(int style);             /* year of birth, for the screen */

/* character presets: generative settings without a composer, from ambient to rhythmic */
int tp_character_count(void);
const char *tp_character_name(int i);
void tp_character_apply(tg_gen *g, int i, uint32_t seed);

#endif
