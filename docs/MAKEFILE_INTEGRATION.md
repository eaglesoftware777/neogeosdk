# Game Engine Makefile Integration for `sdk/2d_engine/ng_*`

> **v1.3.1 additions**
>
> - `USE_2D_PLUS=1` switches the engine source set to
>   `sdk/2d_engine_plus/ng_*.cpp` (C++14, no exceptions, no RTTI, no
>   threadsafe statics).  Default (`USE_2D_PLUS=0`) builds the
>   `sdk/2d_engine/ng_*.c` modules.
> - `GAME_EXTRA_INCLUDES` is a per-game variable that game.mk can set
>   to add additional `-I…` paths to both CFLAGS and CXXFLAGS without
>   modifying the top-level `Makefile`.  Used by `games/demo_plus` to
>   pull in `games/demo`'s artbox tables via `#include "../demo/main.c"`.
>
> ```make
> # games/demo_plus/game.mk
> GAME_NAME           = NeoGeo SDK Demo Plus
> GAME_ID             = 778
> GAME_SCENES         = demo_plus_main
> GAME_EXTRA_INCLUDES = -Igames/demo -Igames/demo/artbox
> ```
>
> - New game `games/demo_plus` (ID 778) — builds clean with
>   `make GAME=demo_plus p1 USE_2D_PLUS=1`.

The 2D game engine layer is built from the `sdk/2d_engine/ng_*.c` modules.

Current state on `main`:

- `Makefile` compiles the engine modules into `out/ng_*0.o`
- `MakefileWin32.mak` compiles the same engine modules into `out\\ng_*0.o`
- both link those objects into the main 68000 game binary
- both linker scripts place engine state in `game_engine_bss`
- P1 generation crops the linked program to the full 512 KB P-ROM window before byte swap and padding
- debug targets can emit symbol, map, readelf, disassembly, and GDB batch-trace files

This note documents the current wiring so you can extend it safely.

## Current object lists

Linux:

```make
NG_ENGINE_NAMES=ng_defs ng_properties ng_game_time ng_timers ng_progress ng_status ng_game_events ng_level ng_fix ng_sprite_group ng_actions ng_chars ng_npcs ng_physics ng_border_constraints ng_game_interupt
NG_ENGINE_OBJ0=$(addprefix out/,$(addsuffix 0.o,$(NG_ENGINE_NAMES)))
```

Windows:

```make
NG_ENGINE_OBJ0=out\ng_defs0.o out\ng_properties0.o out\ng_game_time0.o out\ng_timers0.o out\ng_progress0.o out\ng_status0.o out\ng_game_events0.o out\ng_level0.o out\ng_fix0.o out\ng_sprite_group0.o out\ng_actions0.o out\ng_chars0.o out\ng_npcs0.o out\ng_physics0.o out\ng_border_constraints0.o out\ng_game_interupt0.o
```

## Current linker state

Already prepared:

- `sdk/neogeo.ld`
- `sdk/neogeo_win.ld`

Both scripts place the engine modules into:

- `neogeo_user` for code and rodata
- `game_engine_bss` for BSS, COMMON, and data sections

The P1 path keeps the linked game in the `0x000000..0x080000` program ROM
window. This is important once the 68000 program grows beyond 128 KB; cropping at
`0x020000` would truncate code and can reset the game when execution reaches the
missing region.

## Minimal game-engine-enabled game pattern

In your 68000 source:

```c
#include "sdk/2d_engine/ng_game_engine.h"

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

- `#include "sdk/2d_engine/ng_runtime.h"` still works
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
- `ng_level.c`
- `ng_fix.c`
- `ng_sprite_group.c`
- `ng_actions.c`
- `ng_chars.c`
- `ng_npcs.c`
- `ng_physics.c`
- `ng_border_constraints.c`
- `ng_game_interupt.c`

That gives you the full engine layer by default.

When adding a new engine module, update both `NG_ENGINE_NAMES` in `Makefile` and
the explicit `NG_ENGINE_OBJ0` list in `MakefileWin32.mak`, then ensure the source
is included by `sdk/2d_engine/ng_game_engine.h` if it is part of the public aggregate API.

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

## Debug and GDB Trace Targets

Linux:

```bash
make debug-build
make debug-artifacts
make gdb-trace
make gdb
make gdb-remote GDB_REMOTE=localhost:1234
```

Windows:

```bat
make -f MakefileWin32.mak debug-build
make -f MakefileWin32.mak debug-artifacts
make -f MakefileWin32.mak gdb-trace
```

`DEBUG=1` adds:

- `-g3`
- `-gdwarf-2`
- `-DNG_DEBUG=1`
- linker map output at `out/game.map`

Generated debug files:

- `dump/game.size.txt`
- `dump/game.sym`
- `dump/game.readelf`
- `dump/game.debug.dump`
- `dump/game.map`
- `dump/gdb_trace.gdb`
- `dump/gdb_trace.txt`

If the bundled cross-GDB cannot run on the host, pass another compatible GDB:

```bash
make gdb-trace GDB=/path/to/m68k-gdb
```
