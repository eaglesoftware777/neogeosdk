# SDK API Guide

> **v1.3.0 quick reference**
>
> - Sprite slot priority: **LOWER slot number = drawn IN FRONT** (see the
>   boxed rule in `sdk/2d_engine/ng_sprite_pool.h` and the `_plus.hpp` copy).
>   Backgrounds belong at slot 300+, hero/HUD at slot 1–95.
> - C++14 engine: build with `USE_2D_PLUS=1` (engine in `sdk/2d_engine_plus/`).
>   New `games/demo_plus` (ID 778) is a smoke-test target that links only
>   against the C++ engine.
> - `Makefile` accepts `GAME_EXTRA_INCLUDES` (set by a game's `game.mk`)
>   so a game can pull in another game's artbox/header path without
>   duplicating data.

This guide covers the public 68000-side SDK helpers declared in [`sdk/neogeo.h`](./sdk/neogeo.h).

For installation, `SDKHOME` layout, WSL usage, and Makefile targets, see:

- [`README.md`](./README.md)
- [`docs/GAME_ENGINE_LAYER.md`](./docs/GAME_ENGINE_LAYER.md)
- [`docs/ARTBOX_PIPELINE.md`](./docs/ARTBOX_PIPELINE.md)
- [`docs/MAKEFILE_INTEGRATION.md`](./docs/MAKEFILE_INTEGRATION.md)
- the repository wiki home page

The SDK is organized in five layers:

1. Sprite control block builders
2. Palette and VRAM helpers
3. FIX layer and HUD helpers
4. Timing and input helpers
5. YM2610 sound helpers

## Sprite Control Block Helpers

Use these helpers before calling `vram_sprite()` or the sprite move helpers.

| Function | Purpose |
| --- | --- |
| `setSCB2(x_shrink, y_shrink)` | Build the sprite shrink word. |
| `setSCB3(y_pos, sticky_flag, height_factor)` | Build the sprite Y/sticky/height word. |
| `setSCB4(x_pos)` | Build the sprite X position word. |
| `setSCB1_2(pal_offset, tile_offset, bit3_autoanim, bit2_autoanim, vflip, hflip)` | Build the per-tile attribute word. |
| `setFIXDATA(palette_index, tile_number)` | Build a FIX layer tile word. |

## Palette and VRAM Helpers

These functions write directly to Neo Geo VRAM and palette RAM.

| Function | Purpose |
| --- | --- |
| `setBACKDROP(backdrop_color)` | Set the backdrop color register. |
| `load_palettes(palette, palette_offset)` | Copy 16 palette entries into the palette RAM address passed in `palette_offset`. |
| `vram_init(start, vram_inc)` | Set the VRAM address and auto-increment registers. |
| `setpal(...)` | Fill a 16-entry palette buffer in RAM. |
| `vram_SCB1(scb1_tiles, scb1_attrs, tiles_number)` | Write SCB1 tile/attribute pairs. |
| `vram_sfix(vram_inc, fix_addr, fix_data)` | Write a FIX tile with explicit increment and address. |
| `vram_sfix1(fix_data)` | Write a FIX tile using the current VRAM cursor. |
| `vram_SCB234(scb_addr, scb_value)` | Write a single SCB2/3/4 value. |
| `vram_sprite(...)` | Upload a full sprite entry with SCB1/2/3/4 data. |
| `vram_sprite_mvx(vram_offset, scb4)` | Update a sprite X position in VRAM. |
| `vram_sprite_mvy(vram_offset, scb3)` | Update a sprite Y/height word in VRAM. |

## FIX Layer and HUD Helpers

Use these helpers for text, counters, debug overlays, and BIOS message coordination.

| Function | Purpose |
| --- | --- |
| `fixtext_out(x, y, text, pal)` | Write an ASCII string to the FIX layer. |
| `fixtext_out1(x, y, tiles, pal, count)` | Write a tile array to the FIX layer. |
| `fixtext_out2(x, y, a, b, c, mod, tiles, pal, count)` | Variant helper used for generated FIX strings. |
| `fix_svalue1(x, y, value, pal, offset)` | Display a numeric value using a custom digit tile offset. |
| `fix_svalue(x, y, value, pal)` | Display a numeric value using the default ASCII digit offset. |
| `display_digit(x, y, value, pal, offset)` | Display a `uint32_t` value without floating-point helpers. |
| `displayCreditP1()` / `displayCreditP2()` | Show credit counters using BIOS memory. |
| `mess_outtest()` | Demo helper for the BIOS message output path. |
| `setsfix()` | Enable the S-FIX border behavior used by the SDK demo. |
| `setBIOSMESSBusy()` / `setBIOSMESSReady()` | Gate access to the BIOS message output path. |

## Timing and Housekeeping Helpers

| Function | Purpose |
| --- | --- |
| `clearRAM()` | Clear the SDK work RAM area. |
| `clearSprs()` | Clear the visible sprite list. |
| `clearFix()` | Clear the FIX layer. Restores BRDFIX (game S ROM) after the BIOS clear call. |
| `waitVbl()` | Wait for the next VBlank. |
| `cycle10ms()` | Rough 10 ms delay helper. |
| `cycle1s()` | Rough 1 second delay helper. |
| `cyclexms1(ticks_10ms)` | Delay in coarse 10 ms units. |
| `cyclexs(seconds)` | Delay in coarse second units. |
| `cyclexms(milliseconds)` | Delay in millisecond-style SDK units. |
| `kickWatchDog()` | Service the hardware watchdog register. |
| `sleep1FFF()` | Short busy-wait helper used by low-level routines. |

## Input and Status Helpers

| Function | Purpose |
| --- | --- |
| `poll_joystick()` | Read the BIOS-managed joystick state. |
| `read_p1credit()` | Read the player 1 credit counter. |
| `read_joy1()` | Read the raw joystick hardware ports directly. |

## Sound API

The full command map, ranges, track indices, and driver behavior live in:

- [`sound/SOUND_DRIVER_GUIDE.txt`](./sound/SOUND_DRIVER_GUIDE.txt)
- [`sound/driver/readme`](./sound/driver/readme)

High-level sound control categories:

| Category | Functions |
| --- | --- |
| Core control | `soundInit`, `soundReset`, `soundStopAll`, `soundStopMusic`, `soundCancelFade`, `soundSceneReset` |
| Raw command path | `playSoundtest`, `soundCommand`, `isZ80Ready` |
| Music and samples | `playMusic`, `playSFX`, `playSFXB` |
| FM | `playFMDebug`, `playFMTrack`, `soundSetFMVolume` |
| SSG | `playSSGTrack`, `soundSetSSGPreset`, `playInsertCoinSSG` |
| Voice cues | `playVoiceCue`, `playGetReadyVoice`, `playAttackVoice`, `playCoinThenReady` |
| Fade / mix | `soundFadeOut`, `soundFadeIn`, `soundFadeOutSpeed`, `soundFadeInSpeed`, `soundSetTempo`, `soundSetADPCMAVolume`, `soundSetADPCMBVolume`, `soundSetSSGVolume`, `soundApplyMix` |
| Scene helpers | `soundPlayDemoFM`, `soundPlayTitleMusic`, `soundPlayGameLoop` |

Current shipped helper behavior:

- `soundPlayTitleMusic(track)` resets the scene, plays the title gong, then starts the ADPCM-B title theme
- `soundPlayGameLoop(track)` resets the scene and maps the requested loop to a direct ADPCM-B stage or ending TRACK
- `playMusic(track)` remains the explicit path for MML/SSG playback when you want the music engine directly

## 2D Game Engine Layer

The repository also includes a reusable game engine layer under `sdk/2d_engine/ng_*`.

Primary entry header:

```c
#include "sdk/2d_engine/ng_game_engine.h"
```

Core startup:

```c
game_engine_init();
```

Per-frame entry:

```c
waitVbl();
game_engine_frame();
```

Main runtime modules:

| Module | Purpose |
| --- | --- |
| `ng_actions` | const action scripts with commands like `FRAME`, `WAIT`, `MOVE`, `SFX`, `MUSIC`, `EVENT`, `LOOP` |
| `ng_chars` | fixed-size character pool with fixed-point motion and sprite binding |
| `ng_game_events` | small queued event system |
| `ng_timers` | frame timers |
| `ng_progress` | progress counters |
| `ng_status` | boolean status flags |
| `ng_properties` | live shared values grouped by system |
| `ng_level` | level state, world bounds, camera follow, and joystick camera scroll |
| `ng_fix` | cached FIX-layer text and numeric output |
| `ng_npcs` | fixed-size NPC slots bound to characters and think callbacks |
| `ng_physics` | fixed-point gravity, drag, world bounds, and solid-rectangle resolution |
| `ng_border_constraints` | invisible trigger rectangles that emit events |
| `ng_sprite_group` | grouped Neo Geo sprite upload helpers |

Important current build note:

- the linker scripts reserve `game_engine_bss` for `out/ng_*0.o`
- `Makefile` and `MakefileWin32.mak` compile and link the `sdk/2d_engine/ng_*` modules by default
- use [`docs/MAKEFILE_INTEGRATION.md`](./docs/MAKEFILE_INTEGRATION.md) to inspect the exact build and linker wiring

Camera helpers:

```c
level_set_world_bounds(0, 0, 639, 447);
level_camera_follow(player->x, player->y, 320, 224);
level_camera_joystick(2, NG_CAMERA_AXIS_X, 320, 224);    /* horizontal scroll */
level_camera_joystick(2, NG_CAMERA_AXIS_Y, 320, 224);    /* vertical scroll */
level_camera_joystick(2, NG_CAMERA_AXIS_BOTH, 320, 224); /* free camera */
```

Characters keep world-space `x`/`y` values for physics and collision. The engine subtracts the current level scroll when drawing sprites.

Action-state pattern:

```c
static const NGActionCmd player_run[] = {
    SPEED(0, 0),
    FRAME(tile_run_0, 3),
    FRAME(tile_run_1, 3),
    LOOP()
};

actions_register(ACT_PLAYER_RUN, player_run);
char_action(player, ACT_PLAYER_RUN);
```

Use a per-character callback to map joystick combinations to actions. The demo
uses the priority attack, jump, run, idle so combinations produce deterministic
results.

NPC and opponent pattern:

```c
NGNpc *opponent = npc_spawn(OPPONENT_KIND, CHAR_OPPONENT, 240, 180);
npc_set_think(opponent, opponent_think, 1);

if (ng_rect_hit(char_body_rect(player), char_body_rect(npc_char(opponent)))) {
    char_damage(player, 1);
}
```

Artbox asset categories:

| Category | Typical PNG names | Use |
| --- | --- | --- |
| `background` | `background_*.png`, `zz_npc_forest_alley.png`, legacy numbered pages | full-screen scene/background pages |
| `main_character` | `sprite_*.png` | player/main-character animation frames |
| `opponent` | `opponent_*.png` | enemy/opponent sprite frames |
| `npc` | `z_npc_*.png` | non-player character sprite frames |

The category is written into `artbox/assets_manifest.json` and generated into
`artbox/sprite_meta.h` as `NG_ASSET_CATEGORY_*`.

See [`docs/ARTBOX_PIPELINE.md`](./docs/ARTBOX_PIPELINE.md) for the full rule and
category reference.

## Typical Call Patterns

### One-time startup

```c
soundInit();
clearSprs();
clearFix();
```

### Draw a sprite and move it

```c
uint16_t scb2 = setSCB2(0x0F, 0xAF);
uint16_t scb3 = setSCB3(496 - y, 1, 9);
uint16_t scb4 = setSCB4(x);

vram_sprite(vram_start, 1, vram_offset, tiles, attrs, tile_count, scb2, scb3, scb4);
vram_sprite_mvx(vram_offset, setSCB4(x + 8));
```

### FIX text and number output

```c
fixtext_out(10, 10, "INSERT COIN", 0);
display_digit(20, 15, score, 0, 48);
```

### Direct title or stage theme

```c
soundPlayTitleMusic(0);
soundPlayGameLoop(SOUND_MUSIC_A);
```

### Manual layered sound scene

```c
soundSceneReset();
soundSetADPCMAVolume(0x34);
soundSetADPCMBVolume(0xB8);
soundSetSSGVolume(0x08);
soundSetFMVolume(0x0C);

playSFX(SOUND_SFX_1);
playMusic(SOUND_MUSIC_A);
playFMTrack(SOUND_FM_B);
```

## Notes

- `sdk/neogeo.h` is the intended public call surface for the 68000 side.
- The playable sound driver is the assembler implementation in `sound/driver/driver.asm`.
- `make m1rom` / `make m1rom-asm` builds the authoritative ASM sound runtime.
- `make m1rom-c` builds the experimental C-linked sound runtime for comparison only.
- The current 68000 build no longer links the legacy `softfloat/` runtime by default.
- `clearFix()` calls the BIOS `SYS_FIX_CLEAR` routine which resets BRDFIX to 0. The SDK now restores BRDFIX (`BSET.B #0,REG_BRDFIX`) immediately after that call so the game S ROM is always selected. Never call `clearFix()` without following up with text draws — the game S ROM is restored automatically.
- `soundCommand()` no longer calls `isZ80Ready()` after writing the command byte. The trailing poll created a race condition with the Z80 NMI handler that caused a permanent 68k deadlock. Callers that need to verify Z80 readiness before sending a second command should call `isZ80Ready()` explicitly between commands.
