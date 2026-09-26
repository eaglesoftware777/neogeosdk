/*
 * ng_move host tests: steering through a medium -- a push settles at a
 * top speed, a coast comes to rest (on the current's own drift), rise
 * floats a still swimmer up and a negative one sinks her, the limit holds,
 * and `top` lands her exactly on the surface, calling a fast arrival a
 * splash but not a bob at the surface.
 */
#include <stdio.h>
#include "../sdk/2d_engine/ng_move.h"

static int failures;
static void expect(int ok, const char *what)
{
    if (!ok) { printf("FAIL %s\n", what); failures++; }
}

int main(void)
{
    NGCharacter c = {0};
    NGMoveParams swim = { 40, 3, 3 * NG_FP_ONE, 0, 0, 0, NG_MOVE_NO_TOP };
    int i;
    uint8_t flags;

    /* A steady push settles near accel * 2^drag. */
    for (i = 0; i < 200; i++) ng_move_steer(&c, 1, 0, &swim);
    expect(c.vx_fp > 300 && c.vx_fp <= 330, "a held push settles at about accel * 2^drag");
    /* Letting go, she coasts to a stop. */
    for (i = 0; i < 200; i++) ng_move_steer(&c, 0, 0, &swim);
    expect(c.vx_fp == 0 && c.vy_fp == 0, "a coast comes to rest");
    /* A current carries her. */
    swim.current_x = -64;
    for (i = 0; i < 200; i++) ng_move_steer(&c, 0, 0, &swim);
    expect(c.vx_fp == -64, "with no push she drifts at the current's speed");
    swim.current_x = 0;
    /* The limit holds whatever the push. */
    swim.accel = 2000;
    ng_move_steer(&c, -1, 1, &swim);
    expect(c.vx_fp == -3 * NG_FP_ONE && c.vy_fp == 3 * NG_FP_ONE, "each axis is held to max_speed");
    swim.accel = 40;
    c.vx_fp = c.vy_fp = 0;
    /* Rise floats her up, a negative rise sinks her. */
    swim.rise = 6;
    for (i = 0; i < 200; i++) ng_move_steer(&c, 0, 0, &swim);
    expect(c.vy_fp < 0, "a positive rise floats her up");
    swim.rise = -6;
    for (i = 0; i < 400; i++) ng_move_steer(&c, 0, 0, &swim);
    expect(c.vy_fp > 0, "a negative rise sinks her");
    /* The top: an upward velocity is cut to land her on it. */
    swim.rise = 0;
    swim.top = 40;
    c.y = 42;
    c.vy_fp = -3 * NG_FP_ONE;
    flags = ng_move_steer(&c, 0, -1, &swim);
    expect(c.vy_fp == -2 * NG_FP_ONE, "an upward speed past the top is cut to land on it");
    expect((flags & (NG_MOVE_AT_TOP | NG_MOVE_SURFACED)) == (NG_MOVE_AT_TOP | NG_MOVE_SURFACED),
           "arriving fast at the top is a splash");
    c.y = 40;
    flags = ng_move_steer(&c, 0, -1, &swim);
    expect(c.vy_fp == 0 && flags == NG_MOVE_AT_TOP, "at the top, pushing up holds her there, no splash");
    c.y = 60;
    c.vy_fp = 0;
    flags = ng_move_steer(&c, 0, -1, &swim);
    expect(flags == 0 && c.vy_fp == -40, "well below the top a push goes up as usual");

    if (failures) {
        printf("ng_move: %d failure(s)\n", failures);
        return 1;
    }
    printf("ng_move: all tests passed\n");
    return 0;
}
