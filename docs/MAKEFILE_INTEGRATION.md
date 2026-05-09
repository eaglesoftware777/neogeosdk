# Makefile Integration for `sdk/ng_*`

The runtime-layer source files are present in `sdk/ng_*.c`, and the linker
scripts already reserve `runtime_bss` for `out/ng_*0.o`, but the default
`Makefile` and `MakefileWin32.mak` do not compile those files automatically.

This note shows the minimal integration path.

## Current linker state

Already prepared:

- `sdk/neogeo.ld`
- `sdk/neogeo_win.ld`

Both scripts already include:

- `out/ng_*0.o(neogeo_user)`
- `out/ng_*0.o(.rodata)`
- `out/ng_*0.o(.bss)`
- `out/ng_*0.o(.data)`

So the missing step is compilation of the runtime C files into `out/ng_*0.o`.

## Linux `Makefile`

Add compile steps in the `game:` recipe for the runtime files you want to use.

Example:

```make
	$(CC) $(CFLAGS) sdk/ng_actions.c -o out/ng_actions0.o
	$(CC) $(CFLAGS) sdk/ng_border_constraints.c -o out/ng_border_constraints0.o
	$(CC) $(CFLAGS) sdk/ng_chars.c -o out/ng_chars0.o
	$(CC) $(CFLAGS) sdk/ng_defs.c -o out/ng_defs0.o
	$(CC) $(CFLAGS) sdk/ng_game_events.c -o out/ng_game_events0.o
	$(CC) $(CFLAGS) sdk/ng_game_interupt.c -o out/ng_game_interupt0.o
	$(CC) $(CFLAGS) sdk/ng_game_time.c -o out/ng_game_time0.o
	$(CC) $(CFLAGS) sdk/ng_progress.c -o out/ng_progress0.o
	$(CC) $(CFLAGS) sdk/ng_properties.c -o out/ng_properties0.o
	$(CC) $(CFLAGS) sdk/ng_sprite_group.c -o out/ng_sprite_group0.o
	$(CC) $(CFLAGS) sdk/ng_status.c -o out/ng_status0.o
	$(CC) $(CFLAGS) sdk/ng_timers.c -o out/ng_timers0.o
```

## Native Windows `MakefileWin32.mak`

Add equivalent compile lines in the `game:` recipe:

```make
	$(CC) $(CFLAGS) sdk\ng_actions.c -o out\ng_actions0.o
	$(CC) $(CFLAGS) sdk\ng_border_constraints.c -o out\ng_border_constraints0.o
	$(CC) $(CFLAGS) sdk\ng_chars.c -o out\ng_chars0.o
	$(CC) $(CFLAGS) sdk\ng_defs.c -o out\ng_defs0.o
	$(CC) $(CFLAGS) sdk\ng_game_events.c -o out\ng_game_events0.o
	$(CC) $(CFLAGS) sdk\ng_game_interupt.c -o out\ng_game_interupt0.o
	$(CC) $(CFLAGS) sdk\ng_game_time.c -o out\ng_game_time0.o
	$(CC) $(CFLAGS) sdk\ng_progress.c -o out\ng_progress0.o
	$(CC) $(CFLAGS) sdk\ng_properties.c -o out\ng_properties0.o
	$(CC) $(CFLAGS) sdk\ng_sprite_group.c -o out\ng_sprite_group0.o
	$(CC) $(CFLAGS) sdk\ng_status.c -o out\ng_status0.o
	$(CC) $(CFLAGS) sdk\ng_timers.c -o out\ng_timers0.o
```

## Minimal runtime-enabled game pattern

In your 68000 source:

```c
#include "sdk/ng_runtime.h"

void game_boot(void)
{
    game_runtime_init();
}

void game_frame(void)
{
    waitVbl();
    game_interupt();
}
```

## Recommended first integration set

If you want the smallest useful runtime subset, start with:

- `ng_game_interupt.c`
- `ng_game_time.c`
- `ng_timers.c`
- `ng_progress.c`
- `ng_status.c`
- `ng_game_events.c`
- `ng_properties.c`
- `ng_chars.c`
- `ng_actions.c`
- `ng_sprite_group.c`

Add `ng_border_constraints.c` when your project starts using authored trigger
zones driven from player properties.

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
