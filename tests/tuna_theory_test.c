/* SPDX-License-Identifier: GPL-3.0-only */
/* Unit tests for the TUNA music theory core.
 *   cc -O2 -Wall -Ifirmware/src -o build/host/tuna_theory_test tests/tuna_theory_test.c firmware/src/tuna_theory.c
 *   build/host/tuna_theory_test        (exit code 0 = all passed; VERBOSE=1 prints every chord) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tuna_theory.h"

static int fails, checks;

#define CHECK(cond, ...)                                       \
    do {                                                       \
        checks++;                                              \
        if (!(cond)) {                                         \
            fails++;                                           \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);        \
            printf(__VA_ARGS__);                               \
            printf("\n");                                      \
        }                                                      \
    } while (0)

static const char *pc_name[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

/* chord notes as a string like "D F A C" (pitch classes, key tonic = root_pc) */
static const char *chord_str(const tt_chord *c, int root_pc)
{
    static char buf[64];
    buf[0] = 0;
    for (int k = 0; k < c->n; k++) {
        if (k)
            strcat(buf, " ");
        strcat(buf, pc_name[(root_pc + c->root + c->tone[k]) % 12]);
    }
    return buf;
}

static uint16_t rotate(uint16_t m, int by) { return (uint16_t)(((m >> by) | (m << (12 - by))) & 0xFFF); }

static void test_scales(void)
{
    /* the seven church modes are the rotations of Ionian on its own degrees */
    static const int modes[7] = {TS_IONIAN, TS_DORIAN, TS_PHRYGIAN, TS_LYDIAN, TS_MIXOLYDIAN, TS_AEOLIAN, TS_LOCRIAN};
    uint16_t ion = tt_scale_mask(TS_IONIAN);
    for (int d = 0; d < 7; d++) {
        int off = tt_degree_note(TS_IONIAN, 0, d);
        CHECK(tt_scale_mask(modes[d]) == rotate(ion, off), "%s is not Ionian rotated by %d", tt_scale_name(modes[d]), off);
    }
    /* sizes */
    for (int s = 0; s < TS_COUNT; s++) {
        int want = s <= TS_MEL_MINOR ? 7 : s == TS_BLUES || s == TS_WHOLE_TONE ? 6 : 5;
        CHECK(tt_scale_size(s) == want, "%s size %d", tt_scale_name(s), tt_scale_size(s));
        CHECK(tt_scale_mask(s) & 1, "%s lacks its tonic", tt_scale_name(s));
    }
    /* C major from C4 */
    static const int cmaj[8] = {60, 62, 64, 65, 67, 69, 71, 72};
    for (int d = 0; d < 8; d++)
        CHECK(tt_degree_note(TS_IONIAN, 60, d) == cmaj[d], "C major degree %d", d);
    CHECK(tt_degree_note(TS_IONIAN, 60, -1) == 59, "degree -1 is B3");
    CHECK(tt_degree_note(TS_IONIAN, 60, -7) == 48, "degree -7 is C3");
    CHECK(tt_degree_note(TS_PENT_MINOR, 57, 5) == 69, "A minor pentatonic wraps after 5 notes");
    /* harmonic minor has the raised 7th, melodic minor the raised 6th and 7th */
    CHECK(tt_degree_note(TS_HARM_MINOR, 57, 6) == 68, "A harmonic minor has G#");
    CHECK(tt_degree_note(TS_MEL_MINOR, 57, 5) == 66, "A melodic minor has F#");
    /* clamping */
    CHECK(tt_degree_note(TS_IONIAN, 120, 14) == 127, "clamps at 127");
}

static void test_quantize(void)
{
    CHECK(tt_quantize(61, TS_IONIAN, 0) == 60, "C# -> C in C major (tie goes down)");
    CHECK(tt_quantize(66, TS_IONIAN, 0) == 65, "F# -> F in C major");
    CHECK(tt_quantize(66, TS_LYDIAN, 0) == 66, "F# stays in C Lydian");
    CHECK(tt_quantize(64, TS_PENT_MINOR, 9) == 64, "E stays in A minor pentatonic");
    CHECK(tt_quantize(65, TS_PENT_MINOR, 9) == 64, "F -> E in A minor pentatonic");
    CHECK(tt_quantize(66, TS_PENT_MINOR, 9) == 67, "F# -> G in A minor pentatonic (nearest)");
    /* every output is in the scale, for every scale, root and note */
    int bad = 0;
    for (int s = 0; s < TS_COUNT; s++)
        for (int r = 0; r < 12; r++)
            for (int n = 0; n < 128; n++) {
                int q = tt_quantize(n, s, r);
                int in = (tt_scale_mask(s) >> (((q - r) % 12 + 12) % 12)) & 1;
                if (!in || abs(q - n) > 2)
                    bad++;
            }
    CHECK(bad == 0, "%d quantized notes off-scale or moved more than 2 semitones", bad);
}

static void test_chords(void)
{
    tt_chord c;
    /* diatonic triads of C major: I ii iii IV V vi vii° */
    static const char *triads[7] = {"C E G", "D F A", "E G B", "F A C", "G B D", "A C E", "B D F"};
    for (int d = 0; d < 7; d++) {
        tt_chord_diatonic(&c, TS_IONIAN, d, 3);
        CHECK(!strcmp(chord_str(&c, 0), triads[d]), "C major degree %d: %s", d, chord_str(&c, 0));
    }
    tt_chord_diatonic(&c, TS_IONIAN, 4, 4);
    CHECK(!strcmp(chord_str(&c, 0), "G B D F"), "V7 in C: %s", chord_str(&c, 0));
    /* chord notes wrap by octaves */
    tt_chord_fixed(&c, 0, TQ_MAJ);
    CHECK(tt_chord_note(&c, 60, 0) == 60 && tt_chord_note(&c, 60, 3) == 72 && tt_chord_note(&c, 60, -1) == 55,
          "chord_note wraps: %d %d %d", tt_chord_note(&c, 60, 0), tt_chord_note(&c, 60, 3), tt_chord_note(&c, 60, -1));
    /* every fixed quality starts on its root and ascends */
    for (int q = TQ_MAJ; q < TQ_COUNT; q++) {
        tt_chord_fixed(&c, 5, q);
        int ok = c.root == 5 && c.tone[0] == 0;
        for (int k = 1; k < c.n; k++)
            ok &= c.tone[k] > c.tone[k - 1];
        CHECK(ok, "quality %d malformed", q);
    }
}

static void test_progressions(int verbose)
{
    tt_chord c;
    struct { int prog, scale, root_pc, tones; const char *want[4]; } cases[] = {
        {TP_POP,        TS_IONIAN,     0, 3, {"C E G", "G B D", "A C E", "F A C"}},
        {TP_JAZZ,       TS_IONIAN,     0, 4, {"D F A C", "G B D F", "C E G B"}},
        {TP_JAZZ,       TS_IONIAN,     0, 3, {"D F A", "G B D", "C E G"}},
        {TP_EPIC,       TS_AEOLIAN,    9, 3, {"A C E", "F A C", "C E G", "G B D"}},
        {TP_ANDALUSIAN, TS_AEOLIAN,    9, 3, {"A C E", "G B D", "F A C", "E G# B"}},
        {TP_FOLK,       TS_IONIAN,     7, 3, {"G B D", "C E G", "G B D", "D F# A"}},
        {TP_MODAL_VAMP, TS_DORIAN,     2, 3, {"D F A", "E G B"}},          /* i - ii */
        {TP_MODAL_VAMP, TS_PHRYGIAN,   4, 3, {"E G B", "F A C"}},          /* i - bII */
        {TP_MODAL_VAMP, TS_LYDIAN,     5, 3, {"F A C", "G B D"}},          /* I - II */
        {TP_MODAL_VAMP, TS_MIXOLYDIAN, 7, 3, {"G B D", "A C E"}},          /* I - ii */
        {TP_MIXO_ROCK,  TS_MIXOLYDIAN, 4, 3, {"E G# B", "D F# A", "A C# E"}},
    };
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        int len = tt_prog_len(cases[i].prog);
        for (int s = 0; s < len; s++) {
            tt_prog_chord(&c, cases[i].prog, s, cases[i].scale, cases[i].tones);
            const char *got = chord_str(&c, cases[i].root_pc);
            if (verbose)
                printf("  %-9s %-8s in %-2s step %d: %s\n", tt_prog_name(cases[i].prog), tt_scale_name(cases[i].scale),
                       pc_name[cases[i].root_pc], s, got);
            CHECK(!strcmp(got, cases[i].want[s]), "%s in %s %s step %d: %s, want %s", tt_prog_name(cases[i].prog),
                  pc_name[cases[i].root_pc], tt_scale_name(cases[i].scale), s, got, cases[i].want[s]);
        }
    }
    /* steps wrap */
    tt_chord a, b;
    tt_prog_chord(&a, TP_POP, 1, TS_IONIAN, 3);
    tt_prog_chord(&b, TP_POP, 5, TS_IONIAN, 3);
    CHECK(!memcmp(&a, &b, sizeof a), "progression steps wrap");
}

static void test_held(void)
{
    int bass;
    uint8_t one[] = {62};
    CHECK(tt_held_set(one, 1, &bass) == (1u << 2) && bass == 2, "single key D");
    uint8_t chord[] = {67, 60, 64};
    CHECK(tt_held_set(chord, 3, &bass) == ((1u << 0) | (1u << 4) | (1u << 7)) && bass == 0, "C E G, bass C");
    CHECK(tt_held_set(NULL, 0, &bass) == 0 && bass == -1, "nothing held");
}

int main(void)
{
    int verbose = getenv("VERBOSE") != NULL;
    test_scales();
    test_quantize();
    test_chords();
    test_progressions(verbose);
    test_held();
    printf("tuna_theory: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
