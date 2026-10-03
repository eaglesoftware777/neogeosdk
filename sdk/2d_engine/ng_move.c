/*
 * ng_move.c — Swimming and free flight (see ng_move.h). In the on-demand
 * library: only a game that steers a character carries it.
 */

#include "ng_move.h"

/* One axis: close 1/2^drag of the gap to the current, push, then limit.
 * A gap smaller than the drag can take is closed outright, so a coast
 * really comes to rest instead of creeping on at a pixel a minute. */
static int32_t NEOGEO_USER ng_move_axis(int32_t v, int8_t push, const NGMoveParams *p, int16_t current)
{
    int32_t gap = v - current;

    if (p->drag) {
        if (gap < ((int32_t)1 << p->drag) && gap > -((int32_t)1 << p->drag)) gap = 0;
        else gap -= gap >> p->drag;
    }
    v = current + gap;
    if (push > 0) v += p->accel;
    else if (push < 0) v -= p->accel;
    if (v > p->max_speed) v = p->max_speed;
    else if (v < -p->max_speed) v = -p->max_speed;
    return v;
}

uint8_t NEOGEO_USER ng_move_steer(NGCharacter *c, int8_t dx, int8_t dy, const NGMoveParams *p)
{
    uint8_t flags = 0;
    int32_t vy;

    if (!c || !p) return 0;
    c->vx_fp = ng_move_axis(c->vx_fp, dx, p, p->current_x);
    vy = ng_move_axis(c->vy_fp, dy, p, p->current_y);
    if (!dy) vy -= p->rise;
    if (p->top != NG_MOVE_NO_TOP && vy < 0) {
        int32_t room = ((int32_t)c->y - p->top) << NG_FP_SHIFT;   /* how far up she can still go */
        if (-vy >= room) {
            if (room > 0 && -vy > NG_FP_ONE) flags |= NG_MOVE_SURFACED;
            vy = room > 0 ? -room : 0;
            flags |= NG_MOVE_AT_TOP;
        }
    }
    c->vy_fp = vy;
    return flags;
}
