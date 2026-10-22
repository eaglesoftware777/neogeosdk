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
- The group remembers its last SCB1 footprint. Frame changes rewrite occupied
  tiles; blank padding is written again only when the footprint grows or a
  hidden group is restored.

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
ng_sprite_group_flush(&player);            // writes the driver position when moved
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
- For moving characters: keep the same group between frames and use
  `ng_sprite_group_set_pos()` + `ng_sprite_group_flush()`. Position-only updates
  write the driver strip's SCB3 and SCB4; scale or visibility changes also
  refresh the chained strips.
- `ng_chars_draw()` retains one group per character and hides an old slot window
  before assigning a new one. Call `ng_chars_set_depth_sort(0)` for a scene
  whose priority bands already define a fixed order; restore `1` when Y-depth
  sorting is needed.
- Fixed-art projectiles in the C engine can opt into
  `ng_char_set_render_mode(c, NG_CHAR_RENDER_MOVE_ONLY)`. Position, offsets,
  camera, visibility and VRAM priority continue to update. Sprite setters still
  invalidate the cached model. Direct writes to tile, palette, palette map,
  stride, dimensions, active rows, scale or flip require `c->sprite_dirty = 1`.
  The default `NG_CHAR_RENDER_FULL` keeps detecting those direct field changes;
  reused character slots return to that default. This option saves CPU model
  comparisons, not the hardware cost of actually moving a sprite.
- Limit groups to ≤ 380 total hardware slots across all active groups.
- For scenery placed every frame, use `ng_sprite_group_show_at(g, tile, palette, x, y)`.
  It does what `set_tile_base`, `set_palette`, `set_pos`, `set_visible(1)` and
  `flush` do, in one call. `ng_sprite_groups_hide_all(first, count)` hides the
  unused rest of a pool and touches only the groups still shown.
- For a layer wider than the 512 pixels the hardware wraps in, give the group
  the whole art's width as its tile stride and point each strip at the column it
  should show with `ng_sprite_group_set_strip_column(g, strip, column)`, while the
  strip is off screen. It writes the strip's tile map at once (holding off raster
  bands while it does). Maiya's far layer streams its paintings this way.

## Writing in the vertical blank (`NG_VRAM_DEFER`)

With `-DNG_VRAM_DEFER=1` in a game's `GAME_ENGINE_DEFINES`,
`ng_sprite_group_flush()` only lists the group. `ng_vram_commit()`, called
right after the frame's wait for the vertical blank, writes every listed group
in the state it has then: hides first, then the groups, then the FIX text
cells. A group that only moved costs two words. See
[`FRAMEWORK_V1.md`](FRAMEWORK_V1.md) for the whole frame.

## Reserving a small actor before bulk work

The optional C engine `ng_sprite_group_commit_small(g)` consumes one completed
published group before ordinary queued work. `ng_char_commit_small(c)` uses the
character renderer's existing published group and slot ownership. Neither call
changes the model or allocates slots. Default queue policy and all other games
remain unchanged until they opt in.

Use these only in the exclusive VBlank consumer after taking a completed frame,
for example before a large full-screen palette fade. They reject reentrant writers,
active display, insufficient whole-group budget, pending hides, streaming-column
ownership, wide tile-address windows and footprints beyond eight strips/sixteen rows.
Wide maps retain their existing normal/staged writer. The same conservative
cost estimate plus50% margin applies. Pending work remains untouched on rejection.
Successful uploads detach all queued references, so the ordinary commit cannot
repeat an old request or resurrect removed ownership. Existing queued hides must
still run first when they overlap the actor.

An upload which reserves time for the actor may defer a whole-screen palette step.
The palette writer keeps its own all-bank deadline and never splits that fade to
force it through. This path does not make large scenery transfers safe: use hidden
staging for those. Verify actual write windows, queue latency, palette behavior,
scene transitions and physical hardware for each adopter. See [frame_sync.md](frame_sync.md)
for publication ownership and [the host regression](../tests/sprite_deferred_test.c).
