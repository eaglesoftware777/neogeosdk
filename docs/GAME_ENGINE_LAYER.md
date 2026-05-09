# NeoGeoSDK 2D Game Engine Layer

This repository carries a reusable 2D game engine layer under `sdk/ng_*`.

The layer is plain C. It is not a C++ object system and it is not an
entity-component framework. The model is simple:

- fixed-size arrays
- character pool
- action scripts
- timers
- status flags
- progress slots
- properties matrix
- event queue
- border constraints
- one per-frame game engine entry point

## Headers

Use the aggregate include:

```c
#include "sdk/ng_game_engine.h"
```

Or include only the modules you need:

- `sdk/ng_defs.h`
- `sdk/ng_actions.h`
- `sdk/ng_chars.h`
- `sdk/ng_game_events.h`
- `sdk/ng_game_interupt.h`
- `sdk/ng_game_time.h`
- `sdk/ng_progress.h`
- `sdk/ng_properties.h`
- `sdk/ng_status.h`
- `sdk/ng_timers.h`
- `sdk/ng_border_constraints.h`
- `sdk/ng_sprite_group.h`

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
- border constraints
- action table
- character pool

## Per-frame call

Call the game engine once per frame after `waitVbl()`:

```c
waitVbl();
game_engine_frame();
```

`game_engine_frame()` runs this sequence:

1. `game_time_tick()`
2. `timers_update()`
3. `border_constraints_update()`
4. `chars_update()`
5. `game_events_update()`
6. `progress_update()`
7. `chars_draw()`

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

Sprite-group level helpers live in `sdk/ng_sprite_group.h`.

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

See `sdk/ng_defs.h` for the fixed capacities:

- `NG_MAX_CHARS`
- `NG_MAX_ACTIONS`
- `NG_MAX_GAME_EVENTS`
- `NG_MAX_TIMERS`
- `NG_MAX_PROGRESS`
- `NG_MAX_STATUS`
- `NG_MAX_BORDER_CONSTRAINTS`

## Important build note

The engine source files are present in the repository, the linker scripts
reserve `game_engine_bss` for `out/ng_*0.o`, and the default makefiles now
compile and link the `sdk/ng_*.c` modules automatically.

For the current build wiring, follow:

- [`docs/MAKEFILE_INTEGRATION.md`](./MAKEFILE_INTEGRATION.md)

## Compatibility aliases

The old names are still accepted for existing code:

- `#include "sdk/ng_runtime.h"`
- `game_runtime_init()`
- `game_interupt_set_hooks()`
- `game_interupt()`
