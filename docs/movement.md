# Movement Modes: Swimming and Flight

`ng_move` (`sdk/2d_engine/ng_move.h`) steers a character through a medium
instead of walking it: in water, or on the wing. It is in the on-demand
library, so only a game that calls it carries it (C engine only).

```c
static const NGMoveParams swim = {
    48,              /* accel: 8.8 px/frame added while a direction is held */
    3,               /* drag: 1/8 of the gap to the current closes a frame  */
    3 * NG_FP_ONE,   /* max_speed, each axis                                */
    -9,              /* rise: with no vertical push; negative sinks slowly  */
    0, 0,            /* current_x, current_y: the medium's own drift        */
    84,              /* top: the surface, a world y she can't rise above    */
};

ng_physics_set_gravity(c, 0, 3 * NG_FP_ONE);        /* the steer is the pull */
/* each frame, before the physics step; dx, dy are -1, 0 or 1 */
uint8_t flags = ng_move_steer(c, dx, dy, &swim);
if (flags & NG_MOVE_SURFACED) { /* a splash */ }
```

Each frame, per axis, the velocity closes `1/2^drag` of its gap to the
current, a held direction adds `accel`, and the result is held to
`max_speed`; with no vertical push `rise` is added upward. A steady push
settles at about `accel * 2^drag` (48 and 3: 1.5 px a frame); letting go,
she coasts to the current's speed. Gaps smaller than the drag can close are
closed outright, so a coast comes to a real stop.

`top` stops her at a surface: an upward velocity that would take her past
it is cut to land her on it (`NG_MOVE_AT_TOP`), and arriving faster than a
pixel a frame also returns `NG_MOVE_SURFACED`. `NG_MOVE_NO_TOP` turns it off.

Only the velocity changes. The physics step still moves the character and
collides it with the world, so she can stand on a ledge or the sea floor.
For flight, give no rise, a wind for the current and the top of the sky for
`top`.

## Maiya

Maiya swims the Sunken Reef's road (stage mechanic "water"): the stick
steers her eight ways, A is a stroke (a kick up and on in a spray of
bubbles), down takes her through a one-way ledge, still she sinks to the
floor, the tide (`current_x`, a slow sine) rocks her back and forth, and
breaking the surface throws up a spray. Vines aren't climbed while
swimming. The guardian's arena is fought standing on the floor, as before.

`tests/move_test.c` covers the settle speed, the coast, the current, the
limit, rise and sink, and the surface.
