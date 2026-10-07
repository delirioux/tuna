/* SPDX-License-Identifier: GPL-3.0-only */
/* firmware/src/tuna_libm.c (the FM-1's own exp2f, sinf, sqrtf, lrintf, fabsf) against the host's libm.
 *   cc -O2 -o build/host/tuna_libm_test tests/tuna_libm_test.c -lm      (exit 0: all passed) */
#include <math.h>
#include <stdio.h>
#define fabsf tl_fabsf
#define lrintf tl_lrintf
#define sqrtf tl_sqrtf
#define exp2f tl_exp2f
#define sinf tl_sinf
#include "../firmware/src/tuna_libm.c"
#undef fabsf
#undef lrintf
#undef sqrtf
#undef exp2f
#undef sinf

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

int main(void)
{
    double worst_exp = 0, worst_sin = 0, worst_sqrt = 0;
    for (int i = -400000; i <= 400000; ++i) {
        float x = (float)i / 20000.0f;                         /* -20 .. 20 */
        double e = fabs((double)tl_exp2f(x) / exp2((double)x) - 1.0);
        if (e > worst_exp) worst_exp = e;
        double s = fabs((double)tl_sinf(x) - sin((double)x));
        if (s > worst_sin) worst_sin = s;
        float y = (float)(i + 400001) * 3.1f;
        double q = fabs((double)tl_sqrtf(y) / sqrt((double)y) - 1.0);
        if (q > worst_sqrt) worst_sqrt = q;
        if (tl_lrintf(x * 7.3f) != lrintf(x * 7.3f)) { CHECK(0, "lrintf(%g)", x * 7.3f); break; }
        if (tl_fabsf(x) != fabsf(x)) { CHECK(0, "fabsf(%g)", x); break; }
    }
    CHECK(worst_exp < 3e-7, "exp2f: worst relative error %.2e", worst_exp);
    CHECK(worst_sin < 1e-6, "sinf: worst error %.2e", worst_sin);
    CHECK(worst_sqrt < 2e-7, "sqrtf: worst relative error %.2e", worst_sqrt);
    CHECK(tl_lrintf(2.5f) == 2 && tl_lrintf(3.5f) == 4 && tl_lrintf(-2.5f) == -2 && tl_lrintf(-0.4f) == 0, "ties to even");
    CHECK(tl_sqrtf(0.0f) == 0.0f && tl_sqrtf(-1.0f) != tl_sqrtf(-1.0f), "sqrtf(0), sqrtf(-1)");
    CHECK(tl_exp2f(-200.0f) == 0.0f && tl_exp2f(10.0f) == 1024.0f && tl_exp2f(0.0f) == 1.0f, "exp2f edges");
    printf("tuna_libm: exp2f %.1e, sinf %.1e, sqrtf %.1e; %d checks, %d failed\n", worst_exp, worst_sin, worst_sqrt,
           checks, fails);
    return fails ? 1 : 0;
}
