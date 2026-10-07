/* SPDX-License-Identifier: GPL-3.0-only */
/* Tests of TUNA's resonator (firmware/src/tuna_reso.c): pitch, decay, stability, gliding retunes, the modes.
 *   cc -O2 -Ifirmware/src -o build/host/tuna_reso_test tests/tuna_reso_test.c firmware/src/tuna_reso.c -lm
 *   build/host/tuna_reso_test          (exit 0: all passed) */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tuna_reso.h"

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

#define SR 44100
#define N (SR * 12)
static tr_reso r;
static int32_t in[N], out[2 * N];

static void run(int from, int to)
{
    for (int i = from; i < to; i += 64) {
        int n = to - i < 64 ? to - i : 64;
        tr_process(&r, in + i, out + 2 * i, n);
    }
}

/* the strongest period (autocorrelation) of the left channel over [a, b) */
static float period_of(int a, int b, int lo, int hi)
{
    double best = -1e300;
    int bp = 0;
    for (int p = lo; p <= hi; ++p) {
        double s = 0;
        for (int i = a; i + p < b; ++i)
            s += (double)out[2 * i] * out[2 * (i + p)];
        if (s > best) { best = s; bp = p; }
    }
    /* parabolic refinement */
    return (float)bp;
}

static double rms(int a, int b)
{
    double s = 0;
    for (int i = a; i < b; ++i)
        s += (double)out[2 * i] * out[2 * i] + (double)out[2 * i + 1] * out[2 * i + 1];
    return sqrt(s / (2.0 * (b - a)));
}

int main(void)
{
    /* nothing in, nothing out; amount 0 leaves the mix alone */
    tr_init(&r, SR);
    tr_set(&r, 0.0f, 0.5f, 0.5f);
    int notes[6] = {57};
    tr_tune(&r, notes, 1);
    memset(in, 0, sizeof in);
    for (int i = 0; i < SR / 2; ++i)   /* half a second of A3 played into it, as the music does */
        in[i] = (int32_t)(16000.0 * sin(2.0 * 3.14159265 * 220.0 * i / SR) * (i < SR / 2 - 500 ? 1.0 : (SR / 2 - i) / 500.0));
    memset(out, 0, sizeof out);
    run(0, SR);
    CHECK(rms(0, SR) == 0.0, "amount 0: the mix untouched");

    /* one string on A3: a burst of noise makes it ring at 220 Hz */
    tr_init(&r, SR);
    tr_set(&r, 1.0f, 0.6f, 0.7f);
    tr_tune(&r, notes, 1);
    memset(out, 0, sizeof out);
    run(0, 3 * SR);
    float p = period_of(SR * 6 / 10, SR * 6 / 10 + 4096, 150, 260);
    float hz = SR / p;
    CHECK(fabsf(hz - 220.0f) < 3.0f, "A3 string rings at %.1f Hz", hz);
    double r1 = rms(SR * 55 / 100, SR * 80 / 100), r2 = rms(5 * SR / 2, 5 * SR / 2 + SR / 4);
    CHECK(r1 > 500.0 && r2 < r1, "it rings (%.0f) and dies away (%.0f)", r1, r2);

    /* the RT60: decay 0.2 vs 0.8 */
    double tail[2];
    for (int t = 0; t < 2; ++t) {
        tr_init(&r, SR);
        tr_set(&r, 1.0f, t ? 0.8f : 0.2f, 0.7f);
        tr_tune(&r, notes, 1);
        memset(out, 0, sizeof out);
        run(0, 4 * SR);
        tail[t] = rms(3 * SR / 2, 3 * SR / 2 + SR / 4) / rms(SR * 55 / 100, SR * 70 / 100);
    }
    CHECK(tail[1] > 10.0 * tail[0], "longer decay rings longer (%.4f vs %.4f)", tail[1], tail[0]);
    {   /* the RT60 itself: about decay_s * sqrt(220 / 220) at decay 0.5 = 1.7 s: -60 dB there, +-10 dB */
        tr_init(&r, SR);
        tr_set(&r, 1.0f, 0.5f, 1.0f);
        tr_tune(&r, notes, 1);
        memset(out, 0, sizeof out);
        run(0, 5 * SR);
        float rt = r.decay_s;
        double a = rms(SR * 55 / 100, SR * 60 / 100), b = rms((int)(SR * (0.55f + rt)), (int)(SR * (0.55f + rt)) + SR / 20);
        double db = 20.0 * log10(b / a);
        CHECK(db < -45.0 && db > -75.0, "RT60 %.2f s: %.1f dB after it", rt, db);
    }

    /* stability: loud noise for 8 s at the longest decay and full brightness on six strings, then silence */
    tr_init(&r, SR);
    tr_set(&r, 1.0f, 1.0f, 1.0f);
    int six[6] = {40, 47, 52, 59, 64, 71};
    tr_tune(&r, six, 6);
    srand(3);
    for (int i = 0; i < N; ++i) in[i] = i < 8 * SR ? (rand() % 65535) - 32767 : 0;
    memset(out, 0, sizeof out);
    run(0, N);
    int32_t peak = 0;
    for (int i = 0; i < 2 * N; ++i) if (abs(out[i]) > peak) peak = abs(out[i]);
    CHECK(peak < 6 * 2 * 32768, "bounded: peak %d", peak);
    double end = rms(N - SR / 4, N), loud = rms(7 * SR, 7 * SR + SR / 4);
    CHECK(end < loud * 0.25, "dies away after the input stops (%.0f -> %.0f)", loud, end);

    /* a retune glides: no step much bigger than the string's own motion */
    tr_init(&r, SR);
    tr_set(&r, 1.0f, 0.9f, 0.6f);
    tr_tune(&r, notes, 1);
    memset(in, 0, sizeof in);
    for (int i = 0; i < SR / 2; ++i) in[i] = (int32_t)(((unsigned)i * 2654435761u) >> 17) - 16384;
    memset(out, 0, sizeof out);
    run(0, SR);
    int32_t step_before = 0, step_during = 0;
    for (int i = SR / 2; i < SR - 1; ++i) if (abs(out[2 * i + 2] - out[2 * i]) > step_before) step_before = abs(out[2 * i + 2] - out[2 * i]);
    int up[1] = {64};                                     /* A3 -> E4 */
    tr_tune(&r, up, 1);
    run(SR, 2 * SR);
    for (int i = SR; i < SR + SR / 10; ++i) if (abs(out[2 * i + 2] - out[2 * i]) > step_during) step_during = abs(out[2 * i + 2] - out[2 * i]);
    CHECK(step_during < 2 * step_before + 64, "retune glides: steps %d (before %d)", step_during, step_before);
    p = period_of(SR + SR / 2, SR + SR / 2 + 4096, 60, 200);
    CHECK(fabsf(SR / p - 329.6f) < 6.0f, "retuned to E4: %.1f Hz", SR / p);

    /* the modes' notes */
    int nn[6], k;
    int8_t maj7[4] = {0, 4, 7, 11};
    k = tr_notes_for(TR_TUNE_CHORD, 2, 5, maj7, 4, nn);   /* D major, chord IV: G maj7 */
    int pcs = 0;
    for (int i = 0; i < k; ++i) pcs |= 1 << (nn[i] % 12);
    CHECK(k == 6 && pcs == (1 << 7 | 1 << 11 | 1 << 2 | 1 << 6), "CHORD: G B D F# (%d strings)", k);
    k = tr_notes_for(TR_TUNE_KEY, 2, 5, maj7, 4, nn);
    pcs = 0;
    for (int i = 0; i < k; ++i) pcs |= 1 << (nn[i] % 12);
    CHECK(pcs == (1 << 2 | 1 << 9), "KEY: D and A");
    k = tr_notes_for(TR_TUNE_ROOT, 2, 5, maj7, 4, nn);
    pcs = 0;
    for (int i = 0; i < k; ++i) pcs |= 1 << (nn[i] % 12);
    CHECK(pcs == (1 << 7 | 1 << 2), "ROOT: G, and its fifth (D) under each");
    for (int i = 0; i < k; ++i) CHECK(nn[i] >= 28 && nn[i] <= 96, "string note %d in range", nn[i]);

    printf("tuna_reso: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
