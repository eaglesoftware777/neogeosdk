/*
 * ng_move.h — Movement modes: swimming and free flight
 *
 * Walking is the physics module's gravity and a game's own run code. In
 * water or on the wing a character is steered instead: the stick pushes her
 * any of eight ways, the medium holds her back, and without a push she
 * drifts where it carries her.
 *
 *   NGMoveParams swim = { 40, 3, 3 * NG_FP_ONE, -10, 0, 0, 24 };
 *   ng_physics_set_gravity(c, 0, 3 * NG_FP_ONE);   // the steer is the pull now
 *   ...every frame, before the physics step:
 *   flags = ng_move_steer(c, dx, dy, &swim);          // dx, dy: -1, 0 or 1
 *
 * Each frame, on each axis:
 *
 *   drag      the velocity closes 1/2^drag of its gap to the current's
 *             (water holds a swimmer back and carries her along with it);
 *   push      a held direction adds accel (so a steady push settles at
 *             about accel * 2^drag faster than the current);
 *   drift     with no vertical push, `rise` is added upward (buoyancy; a
 *             negative rise sinks her slowly to the floor);
 *   limit     each axis is held to +-max_speed.
 *
 * `top` is a world y she can't rise above (the water's surface, the top of
 * the sky): an upward velocity that would carry her past it is cut to land
 * her on it, and NG_MOVE_AT_TOP is returned -- with NG_MOVE_SURFACED too if
 * she got there moving up faster than a pixel a frame (a splash, say;
 * bobbing at the surface doesn't set it). NG_MOVE_NO_TOP turns it off.
 *
 * Only the velocity changes: the physics step still moves her and collides
 * her with the world, so she can stand on a ledge or the sea floor. Units
 * are the engine's 8.8 fixed point (NG_FP_ONE is one pixel a frame).
 *
 * The same call flies: a mount in the sky is steered with no rise, a wind
 * for the current and the top of the screen for `top`. On-demand library:
 * only a game that calls it carries it. C engine only.
 */

#ifndef NG_MOVE_H
#define NG_MOVE_H

#include <stdint.h>
#include "ng_defs.h"
#include "ng_chars.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int16_t accel;       /* 8.8 px/frame added a frame a direction is held      */
    uint8_t drag;        /* shift: 1/2^drag of the gap to the current closes    */
    int16_t max_speed;   /* 8.8 px/frame, each axis                             */
    int16_t rise;        /* 8.8 px/frame added upward with no vertical push     */
    int16_t current_x;   /* 8.8 px/frame: the medium's own drift                */
    int16_t current_y;
    int16_t top;         /* world y she can't rise above; NG_MOVE_NO_TOP: none  */
} NGMoveParams;

#define NG_MOVE_NO_TOP    (-32768)

#define NG_MOVE_AT_TOP    0x01u   /* held at `top` this frame                */
#define NG_MOVE_SURFACED  0x02u   /* ...arriving faster than 1 px a frame    */

uint8_t NEOGEO_USER ng_move_steer(NGCharacter *c, int8_t dx, int8_t dy, const NGMoveParams *p);

#ifdef __cplusplus
}
#endif
#endif
