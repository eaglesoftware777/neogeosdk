# SDK API Guide

This guide covers the public 68000-side SDK helpers declared in [`sdk/neogeo.h`](./sdk/neogeo.h).

For installation, `SDKHOME` layout, WSL usage, and Makefile targets, see:

- [`README.md`](./README.md)
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
| `clearFix()` | Clear the FIX layer. |
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

### Layered sound scene

```c
soundSceneReset();
soundSetADPCMAVolume(0x34);
soundSetADPCMBVolume(0xB8);
soundSetSSGVolume(0x08);

playSFX(SOUND_SFX_COIN_CHIME);
playMusic(SOUND_MUSIC_SAMURAI_GAME_LOOP);
playFMTrack(SOUND_FM_SAMURAI_MINOR);
```

## Notes

- `sdk/neogeo.h` is the intended public call surface for the 68000 side.
- The playable sound driver remains the assembler implementation in `sound/driver/driver.asm`.
- `make m1rom` builds the authoritative ASM sound runtime.
- `make m1rom-c` builds the experimental C-linked sound runtime.
- The current 68000 build no longer links the legacy `softfloat/` runtime by default.
