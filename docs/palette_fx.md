# Palette FX

## What It Does

`ng_palette_fx` (Stage 7) manages animated palette effects: fade in/out, white
flash, red/blue tint flash, brightness pulse, and colour cycle.

All effects modify a 16-colour work palette in RAM and upload it via the render
queue — no direct palette RAM writes during game logic.

## Why It Matters on NeoGeo

NeoGeo palette RAM at 0x400000 is writable any time, but writing mid-frame can
cause visible colour tears on sprites being rendered on that scanline.  The queue
defers all palette writes to VBlank for tear-free colour changes.

## Hardware Limitation It Solves

- Safe palette upload timing: all writes happen via `ng_render_queue_flush()`.
- Multiple simultaneous effects on up to `NG_PALFX_MAX_SLOTS = 8` palette slots.
- No blending hardware — effects are CPU-computed once per frame.

## NeoGeo Colour Format

```
Bit 15: dark flag (halves all channels when set)
Bits 14-10: red (0..31)
Bits 9-5:   green (0..31)
Bits 4-0:   blue (0..31)
Value 0x8000 = transparent (colour index 0)
```

## Example

```c
// At scene load:
ng_palfx_upload_base(PAL_PLAYER, player_palette);

// On hit:
ng_palfx_flash_white(PAL_PLAYER, player_palette, 6);

// Per frame:
ng_palette_fx_update();        // computes work palette, queues upload
ng_render_queue_flush();       // applies at VBlank
```

## Available Effects

| Function               | Description |
|-----------------------|-------------|
| `ng_palfx_fade_in`    | Black → base palette over N frames |
| `ng_palfx_fade_out`   | Base palette → black over N frames |
| `ng_palfx_flash_white`| Brief white tint, decays to normal |
| `ng_palfx_flash_red`  | Red damage tint |
| `ng_palfx_flash_blue` | Blue magic tint |
| `ng_palfx_pulse`      | Looping brightness oscillation |
| `ng_palfx_cycle`      | Rotate palette entries [start..end] |

## Common Mistakes

- Writing to palette RAM directly: `*(uint16_t*)0x400000 = colour;`
  This bypasses the queue and can cause mid-frame tears.
  Always use `ng_rq_palette_upload()` or the palfx helpers.
- Forgetting to call `ng_palette_fx_update()` per frame: effects will not advance.

## Performance Advice

- Each active slot blends 15 colours: 15 × 3 multiply+shift = ~45 operations.
  With 8 slots active simultaneously = 360 operations — acceptable in VBlank.
- For scene-wide fade (all palettes), loop over all slots and call
  `ng_palfx_fade_out()` on each.  Budget accordingly.
