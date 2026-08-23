# NeoGeoSDK 2D Game Engine Layer

> **v1.7.0 — engine occlusion direction**
>
> The boxed rule at the top of `sdk/2d_engine/ng_sprite_pool.h` (and the
> `sdk/2d_engine_plus/ng_sprite_pool.hpp` copy) is authoritative:
> **LOWER hardware slot = drawn BEHIND higher-numbered sprites** when
> they overlap.  The Neo Geo LSPC walks the sprite control blocks
> sequentially from slot 0 upward, so a sprite at slot 1 is written
> first and any later slot covers it.  Backgrounds belong at low
> slots 1–32 (`NG_SPR_BG0_FIRST=1`, `NG_SPR_BG1_FIRST=17`); characters
> at 96–223 (`NG_SPR_CHAR_FIRST..NG_SPR_CHAR_LAST`); FX, particles
> and temporary front effects at higher slots so they draw on top.
> See `sdk/2d_engine/ng_sprite_pool.h` for the canonical layout.
>
> Both `sdk/2d_engine/` (C, `gnu99`) and `sdk/2d_engine_plus/` (C++14,
> `-fno-exceptions -fno-rtti -fno-threadsafe-statics`) ship with the
> same public API surface.  Select with `USE_2D_PLUS=0` or
> `USE_2D_PLUS=1` on the `make` line.

This repository carries a reusable 2D game engine layer under `sdk/2d_engine/ng_*`.

The layer is plain C. No float, no malloc during gameplay, no division in the frame loop. It is not a C++ object system and not an entity-component framework.

## Module overview

### Core engine modules

| Module | Header | Purpose |
|--------|--------|---------|
| Definitions | `ng_defs.h` | Shared types, constants, pool sizes |
| Characters | `ng_chars.h` | Fixed-size character pool, per-kind callbacks |
| Actions | `ng_actions.h` | Action script execution (FRAME, WAIT, MOVE, SFX, …) |
| Level | `ng_level.h` | Level bounds, camera scroll, joystick camera helpers |
| Background | `ng_bg.h` | Sprite-based background layer with parallax ratios |
| FIX cache | `ng_fix.h` | Cached FIX tile writes — only rewrites changed cells |
| NPCs | `ng_npcs.h` | NPC pool with patrol, think hooks, home/bounds |
| Physics | `ng_physics.h` | Fixed-point velocity, gravity, collision solids |
| Game events | `ng_game_events.h` | Fixed ring-buffer event queue |
| Border constraints | `ng_border_constraints.h` | Trigger rects that emit events on enter |
| Status | `ng_status.h` | Bit-flag status set |
| Timers | `ng_timers.h` | Fixed-size timer pool |
| Progress | `ng_progress.h` | Named progress counters |
| Properties | `ng_properties.h` | Runtime shared-value matrix |
| Game time | `ng_game_time.h` | Frame counter |
| Interrupt | `ng_game_interupt.h` | Per-frame hook dispatch |

### Deluxe 2D engine modules (v1.7.0+)

| Module | Header | Purpose |
|--------|--------|---------|
| Sprite groups | `ng_sprite_group.h` | Dirty-flag sticky-bit sprite chains |
| Render queue | `ng_render_queue.h` | 128-slot VBlank-safe deferred VRAM/palette writes |
| Fixed-point | `ng_fixed.h` | 16.16 fixed-point math, sin/cos/shrink lookup tables |
| Camera | `ng_camera.h` | Smooth follow, dead zone, shake, cinematic pan |
| Palette FX | `ng_palette_fx.h` | Fade, flash, pulse, color cycle — queue-safe |
| Particles | `ng_particles.h` | 32-slot pool, 8 types, priority eviction |
| Feedback | `ng_feedback.h` | Hitstop + shake + flash + sound hook in one call |
| Depth FX | `ng_depthfx.h` | NGVec3 perspective projection, Z→shrink/fog |
| Debug HUD | `ng_debug.h` | Fix-layer perf overlay (`NG_DEBUG_PERF=1`) |

## VRAM and CRAM write contract

The Neo Geo LSPC and the 68000 share the VRAM and CRAM buses.  Writes
that land during active video can collide with the LSPC's per-scanline
reads and show up as torn sprites, partial palettes, or "snow" on the
display.

### Current (v1.3.x) contract

| Subsystem | Where it writes | Status |
|---|---|---|
| `ng_bg_draw()` | Direct SCB1/SCB2/SCB3/SCB4 writes | Direct, must run inside the vblank window after `waitVbl()` |
| `ng_chars_draw()` | Direct SCB1/SCB2/SCB3/SCB4 writes | Direct, must run inside the vblank window after `waitVbl()` |
| `ng_particles_draw()` | Direct SCB writes | Direct, must run inside the vblank window |
| `ng_render_queue_flush()` | Drains queued SCB / palette commands | Vblank-safe by design |
| `ng_palette_fx_*` | Queues via `ng_render_queue` | Vblank-safe |
| `ng_feedback_*` | State only; rendering rides ng_render_queue | Vblank-safe |
| `demo_draw_sprite_screen{,_flip}` | Queues into the demo's sprite queue | Vblank-safe (drained at uframe top) |

What this means in practice:

* **CRAM writes (palette RAM at `0x400000`) MUST go through the render
  queue.**  Use `ng_rq_palette_upload()` or `ng_palette_fx_*`.  Writing
  the palette directly from game logic during active video produces
  rolling "snow" pixels on every line currently being scanned out.
* **The big draws (`ng_bg_draw`, `ng_chars_draw`, `ng_particles_draw`)
  are still direct VRAM writes**, but the engine drives them only from
  inside the per-frame pump immediately after `waitVbl()` returns, so
  the writes complete before the LSPC starts scanning the next frame.
* A future v1.4.0 may move those big draws fully through the queue;
  until then, keep custom rendering code aligned with the pump order
  (`waitVbl()` -> draws -> `ng_render_queue_flush()` -> updates).

## Headers

Use the aggregate include:

```c
#include "sdk/2d_engine/ng_engine.h"
```

Or include only the modules you need:

- `sdk/2d_engine/ng_defs.h`
- `sdk/2d_engine/ng_actions.h`
- `sdk/2d_engine/ng_chars.h`
- `sdk/2d_engine/ng_game_events.h`
- `sdk/2d_engine/ng_game_interupt.h`
- `sdk/2d_engine/ng_game_time.h`
- `sdk/2d_engine/ng_progress.h`
- `sdk/2d_engine/ng_properties.h`
- `sdk/2d_engine/ng_level.h`
- `sdk/2d_engine/ng_fix.h`
- `sdk/2d_engine/ng_npcs.h`
- `sdk/2d_engine/ng_physics.h`
- `sdk/2d_engine/ng_status.h`
- `sdk/2d_engine/ng_timers.h`
- `sdk/2d_engine/ng_border_constraints.h`
- `sdk/2d_engine/ng_sprite_group.h`
- `sdk/2d_engine/ng_render_queue.h`
- `sdk/2d_engine/ng_fixed.h`
- `sdk/2d_engine/ng_camera.h`
- `sdk/2d_engine/ng_palette_fx.h`
- `sdk/2d_engine/ng_particles.h`
- `sdk/2d_engine/ng_feedback.h`
- `sdk/2d_engine/ng_depthfx.h`
- `sdk/2d_engine/ng_debug.h`

## Startup

Initialize the game engine once during game startup:

```c
game_engine_init();
```

That resets:

- game time
- timers
- progress slots
- status flags
- event queue
- level and camera state
- cached FIX text state
- border constraints
- action table
- character pool
- NPC pool
- physics bodies and solids

## Per-frame call

Call the game engine once per frame after `waitVbl()`:

```c
waitVbl();
game_engine_frame();
```

`game_engine_frame()` runs this sequence:

1. `game_time_tick()`
2. optional `before_logic` hook
3. `timers_update()`
4. `level_update()`
5. `npcs_update()`
6. `physics_update_pre()`
7. `border_constraints_update()`
8. `chars_update()`
9. `physics_resolve()`
10. optional `collision_logic` hook
11. `game_events_update()`
12. optional `after_events` hook
13. `progress_update()`
14. optional `before_draw` hook
15. `chars_draw()`
16. optional `after_draw` hook

## Optional frame hooks

You can inject high-level callbacks around the engine frame pass:

```c
game_engine_set_hooks(
    before_logic,
    collision_logic,
    after_events,
    before_draw,
    after_draw
);
```

## Characters

Characters are stored in a fixed pool of `NG_MAX_CHARS`.

Core calls:

```c
NGCharacter *c = chars_add(kind, x, y);
chars_remove(c);
chars_clear_kind(kind);
chars_find(kind);
chars_at(index);
chars_count();
```

Each `NGCharacter` holds:

- active/visible state
- kind, state, facing
- integer and fixed-point position
- velocity
- current action-script position
- sprite binding and palette
- body rectangle
- hit rectangle
- HP
- general-purpose scratch fields `data0..data2`

### Character logic callback

Register one per-kind callback:

```c
chars_set_game_interupt(DEMO_CHAR_PLAYER, player_tick);
```

That callback is executed once per frame for each active character of that kind.

## Sprite binding

Attach a Neo Geo sprite strip group to a character:

```c
char_set_sprite(c, 32, 6, 16, tile_base, NG_PAL_PLAYER_BASE);
char_set_body(c, -8, -24, 16, 32);
char_set_pos(c, 120, 180);
char_set_speed(c, 1, 0);
```

Useful helpers:

- `char_set_sprite`
- `char_set_body`
- `char_set_pos`
- `char_set_speed`
- `char_damage`
- `char_heal`
- `char_body_rect`
- `char_hit_rect`

Sprite-group level helpers live in `sdk/2d_engine/ng_sprite_group.h`.

Characters store world-space `x`/`y` coordinates. `chars_draw()` subtracts the
current level scroll before writing sprite transforms, so physics, collisions,
and AI stay in world coordinates while rendering happens in screen space.

## Action scripts

Action scripts are const arrays of `NGActionCmd`.

Register them:

```c
actions_register(ACT_PLAYER_WALK, player_walk_script);
char_action(player, ACT_PLAYER_WALK);
```

Useful commands:

- `FRAME(tile, ticks)`
- `WAIT(ticks)`
- `MOVE(dx, dy)`
- `SPEED(vx, vy)`
- `SCALE(sx, sy)`
- `PALETTE(bank)`
- `FLIP(hflip, vflip)`
- `SFX(id)`
- `MUSIC(id)`
- `FX(kind, dx, dy)`
- `HITBOX(x, y, w, h)`
- `CLEAR_HITBOX()`
- `EVENT(id, a, b)`
- `GOTO(action_id)`
- `LOOP()`
- `END()`

Sound hooks are externalized:

```c
actions_set_sound_hooks(sfx_hook, music_hook);
actions_set_fx_hook(fx_hook);
```

That keeps the game engine layer independent from a specific game sound policy.

### Input-driven action state

Use the per-kind character callback to translate input into action scripts. Keep
the priority explicit when several buttons are pressed at once:

```c
static void player_tick(NGCharacter *player)
{
    uint16_t input = poll_joystick();

    if (input & (1u << CNT_A)) {
        char_action(player, ACT_PLAYER_ATTACK);
    } else if (input & (1u << CNT_UP)) {
        char_action(player, ACT_PLAYER_JUMP);
    } else if (input & ((1u << CNT_LEFT) | (1u << CNT_RIGHT))) {
        char_action(player, ACT_PLAYER_RUN);
    } else {
        char_action(player, ACT_PLAYER_IDLE);
    }
}
```

The demo follows this finite-state-machine style for idle, run, jump, hit, and
attack. One active action owns the current animation, hitbox, speed changes, and
sound cues.

## Level and Camera

`ng_level` owns level metadata, scroll values, camera properties, and world
bounds:

```c
level_set_world_bounds(0, 0, 639, 447);
level_set_camera(0, 0, 320, 224);
level_move_camera(2, 0, 320, 224);
level_camera_follow(player->x, player->y, 320, 224);
```

Joystick camera helpers:

```c
level_camera_joystick(2, NG_CAMERA_AXIS_X, 320, 224);
level_camera_joystick(2, NG_CAMERA_AXIS_Y, 320, 224);
level_camera_joystick(2, NG_CAMERA_AXIS_BOTH, 320, 224);
```

`level_set_camera()` clamps to the current world bounds and writes both
`NG_PROP_GROUP_LEVEL` scroll values and `NG_PROP_GROUP_CAMERA` values.

## NPCs

`ng_npcs` binds small NPC records to normal `NGCharacter` slots.

```c
NGNpc *npc = npc_spawn(NPC_KIND, CHAR_KIND, 160, 180);
npc_set_home(npc, 160, 180);
npc_set_patrol_bounds(npc, 80, 240, 180, 180);
npc_set_think(npc, npc_think_patrol, 1);
```

Use `npc_char(npc)` to access the backing character for body rectangles,
sprites, HP, and actions.

## Physics

`ng_physics` supplies optional fixed-point movement support:

```c
physics_attach(player, NG_PHYSICS_GRAVITY | NG_PHYSICS_WORLD | NG_PHYSICS_SOLIDS);
physics_set_gravity(player, NG_FP_FROM_FRAC(1, 4), NG_TO_FP(4));
physics_add_solid(0, 208, 320, 16, 0);
```

The frame pass calls `physics_update_pre()` before character updates and
`physics_resolve()` after character updates.

## FIX Cache

`ng_fix` wraps FIX text writes with a small cache so repeated HUD/debug strings
do not rewrite unchanged cells every frame:

```c
ng_fix_puts(2, 1, "READY", 0);
ng_fix_put_u16(10, 1, lives, 0, 48);
```

## Game events

Events are queued in a fixed ring buffer:

```c
game_events_set_handler(my_handler);
game_events_send(id, a, b, c);
game_events_read(&event);
game_events_count();
```

Use events for:

- trigger volumes
- combat notifications
- script progression
- UI or stage transitions

## Timers, progress, status, properties

Timers:

```c
timer_start(id, frames);
timer_stop(id);
timer_active(id);
timer_done(id);
timer_value(id);
timer_percent_left(id);
```

Progress:

```c
progress_start(id, max);
progress_set(id, value);
progress_add(id, amount);
progress_value(id);
progress_percent(id);
progress_done(id);
```

Status:

```c
status_set(id);
status_clear(id);
status_has(id);
status_toggle(id);
```

Properties matrix:

```c
prop_set(group, id, value);
prop_get(group, id);
prop_add(group, id, amount);
prop_clear_group(group);
```

The properties matrix is the runtime's shared live-value table. It is a good fit
for player position, stage scroll, level mode, boss phase, timers, and debug
values that many systems need to read.

## Border constraints

Border constraints are invisible rectangles that emit game events when the
player rectangle enters them.

```c
border_constraints_load(list, count);
border_constraints_update();
border_constraints_reset_used();
```

Each `NGBorderConstraint` carries:

- `x, y, w, h`
- `event_id`
- payload `a, b`
- `once`
- `used`

Use them for:

- boss-room start triggers
- camera locks
- cutscene triggers
- tutorial or message gates

## Runtime limits

See `sdk/2d_engine/ng_defs.h` for the fixed capacities:

- `NG_MAX_CHARS`
- `NG_MAX_ACTIONS`
- `NG_MAX_GAME_EVENTS`
- `NG_MAX_TIMERS`
- `NG_MAX_PROGRESS`
- `NG_MAX_STATUS`
- `NG_MAX_BORDER_CONSTRAINTS`
- `NG_MAX_NPCS`
- `NG_MAX_SOLIDS`
- `NG_FIX_WIDTH`
- `NG_FIX_HEIGHT`

## Important build note

The engine source files are present in the repository, the linker scripts
reserve `game_engine_bss` for `out/ng_*0.o`, and the default makefiles now
compile and link the `sdk/2d_engine/ng_*.c` modules automatically.

For the current build wiring, follow:

- [`docs/MAKEFILE_INTEGRATION.md`](./MAKEFILE_INTEGRATION.md)

## Compatibility aliases

The old names are still accepted for existing code:

- `#include "sdk/2d_engine/ng_runtime.h"`
- `game_runtime_init()`
- `game_interupt_set_hooks()`
- `game_interupt()`
