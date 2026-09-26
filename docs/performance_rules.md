# Performance Rules

## NeoGeo 68000 Hardware Budget

| Resource        | Limit |
|----------------|-------|
| CPU clock       | ~12 MHz (68000) |
| VBlank window   | ~16 scanlines ≈ 1.1 ms |
| Hardware sprites | 380 total (slots 0-379) |
| Palette slots   | 16 per bank × 2 banks |
| FIX layer cells | 40 × 32 |

## Rule 1: No malloc/free During Gameplay

Use fixed-size pools for everything: particles, sprite groups, game objects.
`ng_particles`, `ng_chars`, `ng_npcs` all use static arrays.
Dynamic allocation fragments RAM and causes non-deterministic pause spikes.

## Rule 2: No float/double

The 68000 has no FPU.  Any float triggers a software emulation routine costing
hundreds of cycles.  Use `NGFixed` (16.16) or 8.8 fixed-point from `ng_defs.h`.

## Rule 3: Avoid Division in the Frame Loop

Integer division on the 68000 (`DIVS/DIVU`) is 76-158 cycles per call.
In a 60 Hz frame with ~200,000 available cycles, 10 divisions = 1,580 cycles
wasted.  Use:
- Pre-computed lookup tables (`ng_shrink_tab`, `ng_sin_tab`)
- Right-shift instead of divide-by-power-of-2
- Fixed-point multiply (`NGFX_MUL`) instead of divide

## Rule 4: Use Lookup Tables

`ng_fixed.c` provides:
- `ng_sin_tab[256]` / `ng_cos_tab[256]`: sine/cosine without trig
- `ng_shrink_tab[128]`: depth → SCB2 shrink without perspective divide

## Rule 5: Fixed-Size Pools

| System     | Pool size | Adjustable |
|-----------|-----------|------------|
| Characters | 64        | `NG_MAX_CHARS` |
| NPCs       | 32        | `NG_MAX_NPCS` |
| Particles  | 32        | `NG_PART_MAX_PARTICLES` |
| Palette FX | 8         | `NG_PALFX_MAX_SLOTS` |
| Render queue | 128     | `NG_RQ_MAX_CMDS` |

## Rule 6: Use Dirty Flags

`NGSpriteGroup.dirty` tracks which SCB regions need updating.
Only set bits get written to VRAM.  A static sprite costs zero VRAM writes
per frame after initial upload.

## Rule 7: VBlank-Safe Queues

All VRAM and palette writes go through `ng_render_queue`.
Game logic posts commands; `ng_render_queue_flush()` applies them at VBlank.
Never write directly to `VRAM_ADDR`/`VRAM_RW` in game logic.

## Rule 8: Do Not Update Unchanged Sprites

The dirty-flag system in `NGSpriteGroup` and the render queue enforce this.
The Y-depth sorter in `ng_chars.c` re-assigns slots only when sort order changes.

## Rule 9: Prefer Pre-Baked ROM Assets

Animation tile sequences, palette data, and audio samples should be in ROM.
Do not decompress at runtime; do not compute palette at runtime.

## Rule 10: Keep Particles Droppable by Priority

`NG_PART_PRI_OPTIONAL` particles are silently skipped when sprite budget > 200.
Use `NG_PART_PRI_CRITICAL` only for essential effects (hit sparks, death burst).

## Rule 11: Keep Large Objects Sticky-Bit Chained

Multi-strip sprites MUST use the chain bit (SCB3 bit 6) for strips 1..N.
`ng_sprite_group_upload()` and `ng_sprite_group_flush()` handle this correctly.
Never write sticky-strip SCB3 without bit 6 set.

## Rule 12: Keep Demos at Stable 60 FPS

Monitor `VertBlank` at 0x100000: if the flag is not set when your game logic
finishes, VBlank was missed.  Set `ng_dbg_vblank_overflow = 1` in the debug HUD.
Target: game logic + VRAM writes < 60% of the inter-VBlank window.

## Rule 13: Build a Busy Game With GAME_OPTIMIZE

The tree builds at -O0.  A game whose frame doesn't fit sets, in its
`game.mk`:

```makefile
GAME_OPTIMIZE = -O2
```

The engine, the on-demand SDK library and the game's scene files are then
built at that level; the start-up sources (cart header, `user.c`, `main.c`,
`eyecatcher.c`, `neogeolib.c`) stay at -O0.  Maiya measured, in MAME, a walk
and fight through stages 1, 2, 4 and 6: at -O0 22-24 frames a second; with
`-O2` 28-30; with `-O2` and the streamed sprite writes below 43-49; and with
her per-frame library divisions gone (see the next rule) 52-55.

## Rule 14: Divide in 16 Bits

The 68000 divides 32 bits by 16 in one `divu.w`; a division GCC can't prove
fits that calls the library instead, several times slower.  GCC uses
`divu.w` only when it sees both operands are 16-bit, so cast a product
first: `(uint16_t)(t * 255u) / (uint16_t)d`, not `(t * 255u) / d`.  A
signed or 32-bit `%` or `/` in a frame's work (an animation phase, a wrap)
is worth a look in the listing for `__divsi3`, `__modsi3`, `__udivsi3` and
`__umodsi3`.

`ng_sprite_group` (built at -O2 in every game) writes the video RAM itself:
a map is streamed a strip at a time with the tile stepped a row at a time
(no multiply per row), and the SCB2/3/4 words of a group's strips go out as
one run each, the auto-increment at 1.  Every write to the data port lands
at least 12 clocks after the one before.

## Sprite Budget Allocation Guide

| Use             | Slots  | Notes |
|----------------|--------|-------|
| Player character | 0-7   | up to 8 strips |
| Enemy characters | 8-63  | 2-8 strips each |
| Particles        | 64-95 | up to 32 strips |
| NPCs / objects   | 96-199| varied |
| Background layer 0 | 300-315 | 16 strips |
| Background layer 1 | 316-331 | 16 strips |
| Reserved/free    | 332-379 | |

Total active hardware sprites must not exceed 380.
