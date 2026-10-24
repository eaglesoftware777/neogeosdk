# Game Time

## What It Does

`ng_game_time` (`sdk/2d_engine/ng_game_time.h`) keeps the engine's clocks.
`ng_game_engine_frame()` ticks them once per game frame:

```c
uint32_t ng_game_time_frame(void);        /* frames since ng_game_time_init */
uint16_t ng_game_time_second(void);       /* ... in whole seconds (60 frames) */
uint32_t ng_game_time_stage_frame(void);  /* the stage clock                 */
void     ng_game_time_reset_stage(void);  /* stage clock back to 0           */
void     ng_game_time_stage_run(uint8_t on);
```

## The Stage Clock

The stage clock counts only while it runs. It runs from
`ng_game_time_init()` (every `ng_game_engine_init()`), so a game that never
touches it sees it count every frame. A game stops it for what it doesn't
want timed, such as a mission card or a scene change, and starts it again.

Like every engine timer, it holds:

- during a pause (`ng_pause_set`), because the engine frame doesn't tick
  while paused;
- during a hitstop, for a game that opted in with
  `ng_game_engine_set_hitstop_freeze`.

Maiya's stage clear timer runs it only while she is on the road. That
covers the whole stage and its guardian, and leaves out the mission card,
the warp, the guardian's entrance, a fall and the clear. It is shown as
`STAGE mm:ss:ff` on the HUD, and as the clear time, with the best time
(kept in RAM until power-off), on the clear screen.

## Frames, Not Seconds

Every clock here counts **frames**, and turns them into seconds at 60 a
second. MAME runs the Neo Geo at 60 frames a second. A real board's video
runs at about **59.18 Hz**, so its "seconds" last a little longer: a stage
shown as 5:00.00 took about 5:04 of wall-clock time on hardware.

Times recorded on one kind of machine compare fairly with each other, but
not across MAME and hardware. The same holds for anything else timed in
frames, such as a stage's countdown.

A game that runs slower than 60 frames a second in a busy scene also counts
fewer frames there. The clock counts the game's own frames, the same ones
its movement and timers advance by.
