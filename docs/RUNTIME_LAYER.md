# NeoGeoSDK Functional Runtime Layer

This layer is game-neutral. It is not tied to Maiya. It is meant for any Neo Geo action, platform, adventure, RPG-lite, fighting, or cinematic 2D game.

## Vocabulary

Use old arcade-style functional words:

| Name | Meaning |
|---|---|
| Character | One active table entry: player, enemy, NPC, chest, platform, item, FX, boss |
| Action | Frame/action script: sprite frame, wait, movement, sound, hitbox, FX, event |
| game_interupt | One per-frame update step |
| game_events | Small messages: item collected, boss defeated, door opened |
| Border Constraint | Invisible rectangle/area in a level that sends a game_event |
| Status | Permanent yes/no progress memory |
| Timer | Countdown in frames |
| Progress | 0..max completion value |
| Properties Matrix | Current live values of the game at time T |

## Functional frame process

At each video frame `T`, call:

```c
waitVbl();
game_interupt();
```

The default `game_interupt()` runs:

```text
1. game_time_tick()
2. custom before_logic hook
3. timers_update()
4. border_constraints_update()
5. chars_update()
6. custom collision_logic hook
7. game_events_update()
8. custom after_events hook
9. progress_update()
10. custom before_draw hook
11. chars_draw()
12. custom after_draw hook
```

## Time math

Neo Geo games commonly update gameplay once per VBlank. Treat the game as 60 logic frames per second:

```text
1 second = 60 frames
1 frame  = 1 / 60 second = 16.67 ms
```

If speed is `2 px/frame`, then:

```text
2 * 60 = 120 px/sec
```

The fixed-point layer uses 8 fractional bits:

```text
1 pixel = 256 fixed units
```

So:

```c
vx_fp = 384;
```

means:

```text
384 / 256 = 1.5 px/frame
1.5 * 60 = 90 px/sec
```

## Properties Matrix

The Properties Matrix is the live game snapshot at time T:

```c
prop_set(NG_PROP_GROUP_PLAYER, NG_PROP_PLAYER_X, maiya->x);
prop_set(NG_PROP_GROUP_PLAYER, NG_PROP_PLAYER_HP, maiya->hp);
```

Then any module can read:

```c
int16_t px = prop_get(NG_PROP_GROUP_PLAYER, NG_PROP_PLAYER_X);
```

Status says:

```text
Did it happen?
```

Properties Matrix says:

```text
What is the value now?
```

Timer says:

```text
How long until this ends?
```

Progress says:

```text
How far is this completed?
```

## Files

Copy these files into `sdk/`:

```text
ng_defs.h/c
ng_properties.h/c
ng_game_time.h/c
ng_timers.h/c
ng_progress.h/c
ng_status.h/c
ng_game_events.h/c
ng_sprite_group.h/c
ng_actions.h/c
ng_chars.h/c
ng_border_constraints.h/c
ng_game_interupt.h/c
ng_runtime.h
```

## Makefile objects

Add these to the game build:

```text
sdk/ng_defs.c
sdk/ng_properties.c
sdk/ng_game_time.c
sdk/ng_timers.c
sdk/ng_progress.c
sdk/ng_status.c
sdk/ng_game_events.c
sdk/ng_sprite_group.c
sdk/ng_actions.c
sdk/ng_chars.c
sdk/ng_border_constraints.c
sdk/ng_game_interupt.c
```

Because the current Makefile is a one-line command chain, the safest integration is to add compile/objcopy/link entries for each of the above objects following the existing `sdk/neogeolib.c` pattern.

## Minimal API example

```c
game_runtime_init();

actions_register(ACTION_PLAYER_IDLE, act_player_idle);
chars_set_game_interupt(CHAR_PLAYER, update_player);

game_events_set_handler(game_event_logic);
game_interupt_set_hooks(0, collision_logic, 0, 0, 0);

Character *player = chars_add(CHAR_PLAYER, 40, 160);
char_set_sprite(player, 32, 4, 8, SPR_PLAYER_IDLE_0, NG_PAL_PLAYER_BASE);
char_action(player, ACTION_PLAYER_IDLE);

while (1) {
    waitVbl();
    game_interupt();
}
```

Use `examples/runtime_layer/minimal_runtime_example.c` as a fuller reference.

## This repository

The live NeoGeoSDK tree integrates the runtime layer with a RAM-backed linker
section for its internal tables. The root demo in `main.c` uses:

- `game_runtime_init()`
- `game_interupt()`
- `actions`
- `chars`
- `game_events`
- `border_constraints`
- `status`
- `timers`
- `progress`
- `properties`

That keeps the runtime layer compatible with the SDK's cartridge-oriented
layout, where ordinary C static storage is not otherwise available by default.
