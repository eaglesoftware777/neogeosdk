# Game Engine Makefile Integration for `sdk/ng_*`

The 2D game engine layer is built from the `sdk/ng_*.c` modules.

Current state on `main`:

- `Makefile` compiles the engine modules into `out/ng_*0.o`
- `MakefileWin32.mak` compiles the same engine modules into `out\\ng_*0.o`
- both link those objects into the main 68000 game binary
- both linker scripts place engine state in `game_engine_bss`

This note documents the current wiring so you can extend it safely.

## Current object lists

Linux:

```make
NG_ENGINE_NAMES=ng_defs ng_properties ng_game_time ng_timers ng_progress ng_status ng_game_events ng_sprite_group ng_actions ng_chars ng_border_constraints ng_game_interupt
NG_ENGINE_OBJ0=$(addprefix out/,$(addsuffix 0.o,$(NG_ENGINE_NAMES)))
```

Windows:

```make
NG_ENGINE_OBJ0=out\ng_defs0.o out\ng_properties0.o out\ng_game_time0.o out\ng_timers0.o out\ng_progress0.o out\ng_status0.o out\ng_game_events0.o out\ng_sprite_group0.o out\ng_actions0.o out\ng_chars0.o out\ng_border_constraints0.o out\ng_game_interupt0.o
```

## Current linker state

Already prepared:

- `sdk/neogeo.ld`
- `sdk/neogeo_win.ld`

Both scripts place the engine modules into:

- `neogeo_user` for code and rodata
- `game_engine_bss` for BSS, COMMON, and data sections

## Minimal game-engine-enabled game pattern

In your 68000 source:

```c
#include "sdk/ng_game_engine.h"

void game_boot(void)
{
    game_engine_init();
}

void game_frame(void)
{
    waitVbl();
    game_engine_frame();
}
```

Compatibility aliases remain available:

- `#include "sdk/ng_runtime.h"` still works
- `game_runtime_init()` still calls `game_engine_init()`
- `game_interupt()` still calls `game_engine_frame()`

Use the game-engine names for new code.

## Default module set

The current default build already includes:

- `ng_defs.c`
- `ng_properties.c`
- `ng_game_time.c`
- `ng_timers.c`
- `ng_progress.c`
- `ng_status.c`
- `ng_game_events.c`
- `ng_sprite_group.c`
- `ng_actions.c`
- `ng_chars.c`
- `ng_border_constraints.c`
- `ng_game_interupt.c`

That gives you the full engine layer by default.

## Sound hook integration

The action system is designed to call back into the existing SDK sound API:

```c
static void my_sfx(uint16_t id)   { playSFX((uint8_t)id); }
static void my_music(uint16_t id) { playMusic((uint8_t)id); }

actions_set_sound_hooks(my_sfx, my_music);
```

You can also point music hooks at higher-level helpers such as:

- `soundPlayTitleMusic()`
- `soundPlayGameLoop()`

when your action scripts are scene-oriented rather than note-oriented.
