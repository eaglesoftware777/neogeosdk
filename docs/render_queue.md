# Render Queue

## What It Does

`ng_render_queue` (Stage 3) is a deferred VRAM / palette write buffer.
Game logic posts `NGRenderCmd` entries during the active frame; at VBlank the
queue is drained in a single burst via `ng_render_queue_flush()`.

## Why It Matters on NeoGeo

Direct VRAM writes during active display can corrupt the currently-displayed
scanline.  The safe window is the 16-line VBlank period.  The queue separates
the "what to draw" decision (game logic time) from the "when to write hardware"
moment (VBlank time).

## Hardware Limitation It Solves

- Race condition between CPU writes and the LSPC sprite engine reading VRAM.
- Palette flickering caused by mid-frame colour updates.
- Ensures all sprite moves for a given frame are applied atomically.

## Queue Capacities

| Queue         | Slots | Memory     |
|---------------|-------|------------|
| Sprite cmds   | 128   | 768 bytes  |
| Palette uploads | 8   | 4 bytes + pointer per slot |

## Example

```c
// Game logic (safe to call any time):
ng_rq_sprite_pos(slot, scb3, scb4);
ng_rq_sprite_shrink(slot, scb2);
ng_rq_palette_upload(pal_slot, my_palette);

// VBlank handler / after waitVbl():
ng_render_queue_flush();
```

## Common Mistakes

- Calling `ng_render_queue_flush()` inside the game logic loop (before VBlank):
  This defeats the purpose.  Always flush at or after `waitVbl()`.
- Overflowing the queue: if `ng_rq_free() == 0`, new commands are silently
  dropped.  Increase `NG_RQ_MAX_CMDS` if needed.

## Performance Advice

- The flush loop is a tight SCB234 write burst; 128 commands costs approximately
  256 VRAM_ADDR + VRAM_RW pairs, well within the VBlank budget.
- For palette uploads, batch palette changes and avoid uploading the same slot
  twice in one frame.
