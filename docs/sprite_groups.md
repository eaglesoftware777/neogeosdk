# Sprite Groups

## What It Does

`NGSpriteGroup` wraps one or more adjacent NeoGeo hardware sprite slots into a
single logical object.  A "group" is a multi-strip character (e.g. 4 strips × 16 px
wide = 64 px wide character).  The group tracks position, scale, palette, flip flags,
and animation tile base in one struct.

## Why It Matters on NeoGeo

NeoGeo sprites are 16 px wide strips.  A 64 px wide character requires 4 adjacent
strips.  Strips 1..N must set the "sticky" (chain) bit in SCB3 so hardware glues
them to the driver strip.  Without correct sticky chaining, strips split apart
when the character moves.

## Hardware Limitation It Solves

- Prevents strip-split on move by always writing driver SCB3 first, then sticky
  SCB3 for chained strips.
- Dirty-flag system avoids redundant VRAM writes: only changed attributes are
  uploaded each frame.
- `ng_sprite_group_flush()` checks `g->dirty` and skips unchanged SCB regions.

## Dirty Flags

```c
#define NG_SGF_DIRTY_POS      0x01  // position changed
#define NG_SGF_DIRTY_TILE     0x02  // tile base / stride changed
#define NG_SGF_DIRTY_PALETTE  0x04  // palette slot changed
#define NG_SGF_DIRTY_SHRINK   0x08  // scale changed
#define NG_SGF_DIRTY_VIS      0x10  // visibility changed
```

All setters (`ng_sprite_group_set_pos`, `ng_sprite_group_set_palette`, etc.)
automatically set the appropriate dirty bit.

## Example

```c
NGSpriteGroup player;

// Init: 4-strip, 8-tile-high, tile base 0x100, palette 2
ng_sprite_group_init(&player, 0, 4, 8, 0x100, 2);

// Game loop:
ng_sprite_group_set_pos(&player, x, y);   // sets DIRTY_POS automatically
ng_sprite_group_flush(&player);            // writes only SCB3/4; zero wasted writes
```

## Common Mistakes

- Calling `ng_sprite_group_upload()` every frame: this re-writes all SCB1 data
  every frame even when the tile hasn't changed.  Use `ng_sprite_group_flush()`
  after init and only call `ng_sprite_group_upload()` when tile data changes.
- Forgetting to set `tileStride`: if the sprite sheet is laid out in rows, stride
  must equal the number of strips.  Call `ng_sprite_group_set_tile_stride()`.

## Performance Advice

- For static backgrounds: call `ng_sprite_group_upload()` once at scene load.
  Never call it again unless the tile set changes.
- For moving characters: use `ng_sprite_group_set_pos()` + `ng_sprite_group_flush()`
  each frame (writes SCB3+SCB4 only = 2 × 2 words = 8 bytes per strip).
- Limit groups to ≤ 380 total hardware slots across all active groups.
