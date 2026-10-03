/*
 * ng_trig host tests: sine and cosine within 1 LSB of the host's math
 * library at every angle, cos(a) = sin(a + 64), atan2 within one step
 * of the true angle on a grid of vectors (every sign, both axes, tiny and
 * huge lengths), and the 1.14 multiply.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../sdk/2d_engine/ng_trig.h"

static int failures;

static void fail(const char *what, long a, long b, long c)
{
    if (failures++ < 12) printf("FAIL %s: %ld %ld -> %ld\n", what, a, b, c);
}

int main(void)
{
    static const long grid[] = { -32768, -32767, -20000, -1000, -255, -100, -37, -10, -3, -2, -1,
                                 0, 1, 2, 3, 10, 37, 100, 255, 1000, 20000, 32767 };
    const unsigned n = sizeof grid / sizeof grid[0];
    const double pi = 3.14159265358979323846;
    unsigned a, i, j, vectors = 0;
    double worst_sin = 0;
    int worst_atan = 0;

    for (a = 0; a < 256; a++) {
        double s = sin(2 * pi * a / 256) * NG_TRIG_ONE, c = cos(2 * pi * a / 256) * NG_TRIG_ONE;
        double es = fabs(ng_sin((uint8_t)a) - s), ec = fabs(ng_cos((uint8_t)a) - c);
        if (es > worst_sin) worst_sin = es;
        if (ec > worst_sin) worst_sin = ec;
        if (es > 1.0 || ec > 1.0) fail("sin/cos", (long)a, (long)ng_sin((uint8_t)a), (long)ng_cos((uint8_t)a));
        if (ng_cos((uint8_t)a) != ng_sin((uint8_t)(a + 64))) fail("cos = sin(a + 64)", (long)a, 0, 0);
    }

    srand(12345);
    for (i = 0; i < n + 4000; i++) {
        for (j = 0; j < (i < n ? n : 1u); j++) {
            long dy = i < n ? grid[i] : (long)(rand() % 65536) - 32768;
            long dx = i < n ? grid[j] : (long)(rand() % 65536) - 32768;
            int want, got, d;
            if (dx == 0 && dy == 0) {
                if (ng_atan2(0, 0) != 0) fail("atan2(0, 0)", 0, 0, ng_atan2(0, 0));
                continue;
            }
            want = (int)lround(atan2((double)dy, (double)dx) * 128.0 / pi) & 255;
            got = ng_atan2((int16_t)dy, (int16_t)dx);
            d = (got - want) & 255;
            if (d > 128) d = 256 - d;
            if (d > worst_atan) worst_atan = d;
            if (d > 1) fail("atan2 (dy, dx)", dy, dx, got);
            vectors++;
        }
    }

    if (ng_trig_mul(100, NG_TRIG_ONE) != 100) fail("mul by 1.0", 100, NG_TRIG_ONE, ng_trig_mul(100, NG_TRIG_ONE));
    if (ng_trig_mul(100, ng_sin(64 + 128)) != -100) fail("mul by -1.0", 100, -NG_TRIG_ONE, ng_trig_mul(100, ng_sin(192)));
    if (ng_trig_mul(200, ng_sin(21)) != (int16_t)((200L * ng_sin(21)) >> 14)) fail("mul", 200, ng_sin(21), 0);

    printf("ng_trig: sin/cos worst error %.3f LSB over 256 angles; atan2 worst %d step(s) over %u vectors\n",
           worst_sin, worst_atan, vectors);
    if (failures) {
        printf("ng_trig: %d failure(s)\n", failures);
        return 1;
    }
    printf("ng_trig: all tests passed\n");
    return 0;
}
