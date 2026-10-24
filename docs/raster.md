# Raster bands

`sdk/2d_engine/ng_raster.h` changes video state part way down the screen: colours, or sprite-table words such as a background's X position, every 8 or 16 lines. It runs on the LSPC timer interrupt (IRQ2).

- **Opt-in:** a game sets `-DNG_RASTER=1` in its `GAME_ENGINE_DEFINES`. The module is then built and linked; without it, nothing changes.
- **C engine only.**
- **Framework v1.x addition** (`FRAMEWORK_V1.md`).

## The hardware

The LSPC timer counts pixels (6 MHz, 384 a line) down from a load value and interrupts at 0. Its controls are bits of `REG_LSPCMODE` ($3C0006); the load value is in `REG_TIMERHIGH`/`REG_TIMERLOW` ($3C0008/$3C000A).

| `REG_LSPCMODE` bit | Effect |
|---|---|
| 4 | interrupt on |
| 5 | load as soon as `REG_TIMERLOW` is written |
| 6 | load at the start of each frame's blanking, line 240 |
| 7 | load again at 0 |

- Bit 1 of `REG_IRQACK` acknowledges the interrupt.
- A load value must stay above 4; a smaller one floods the CPU.
- The bits 15–8 of `REG_LSPCMODE` are the sprite auto-animation speed. The module keeps them: `ng_raster_set_anim_speed()`, 0 by default.
- Source: the neogeodev wiki pages "Timer interrupt" and "Memory mapped registers".

There is no interrupt per line, and a line is only 768 CPU cycles. So the module works in bands, and runs the timer only over the bands a frame uses:
1. The frame's load (bit 6, line 240) holds the delay to the first band.
2. That band's interrupt loads the band period at once and repeats it.
3. The last band's interrupt parks the counter and leaves the next frame's delay for the next load.

A frame therefore costs one interrupt per band, about 470 cycles each in MAME. The handler finds its band from the raster line counter, so an interrupt held back or missed doesn't shift the later ones.

## Use

```c
ng_raster_start(8);                      /* bands of 8 lines (or 16); at scene start */

/* each frame, after the scenery is placed: */
uint8_t first = ng_raster_band_at(64);   /* the band in place from screen line 64 */
uint8_t last  = ng_raster_band_at(136);
ng_raster_vram_bands(first, n, SCB4_ADDR + g->firstSprite, words);   /* X per band */
ng_raster_color(band, palette * 16 + index, color);                  /* or a colour */

/* frame end, right after the wait for the vertical blank: */
ng_vram_commit();
ng_raster_vblank();                      /* this frame's table goes in use */
```

The game's IRQ2 handler calls `ng_raster_irq()`. Or IRQ2 can be a jump to `ng_raster_irq_handler`, which is an interrupt routine itself; Maiya does this (`games/maiya/user.c`).

Rules:
- **Bands run from 1 to `ng_raster_last_band()`.** That is 32 for 8-line bands and 16 for 16-line bands; the last one starts on line 232. Band *b* starts on screen line *b* × lines − 40.
- **Put a moved sprite back.** A band that moves a sprite should be followed by one that restores it, so the next frame starts from the position the commit wrote.
- **Sprite-table words need `NG_VRAM_DEFER`.** They are skipped while the engine itself writes video memory (`ng_vram_busy`: the commit, a full list's flush, a hide, an upload). A game that writes VRAM directly during play must not use them.
- **Colours are written when the interrupt comes**, near the end of the band's previous line. A colour written while a line is being drawn shows as a stray pixel ("snow"). Keep to a few colours per band, and check them on hardware.
- **An empty table turns the timer off.**

## Maiya

`mg_raster_step()`, the **Golden Savanna:** heat haze over the horizon, screen lines 64–136.
- The painting's driving strip moves ±1 pixel in 8-line bands: 11 interrupts a frame.
- It runs only in play on the road: not in an arena, the vault, or a cutscene.

Two other uses were tried and taken out:
- **The Sunken Reef's whole painting swayed** ±2 pixels in 16-line bands, 14 interrupts a frame. It cost the heaviest stage about 16,000 cycles a frame (phase 5).
- **The Sky Road scrolled its clouds slower than its mountains.** The clouds that reach below the split line sheared along a seam.

## Not done

**Swapping palette banks** (`REG_PALBANK0`/`REG_PALBANK1`) for whole-screen changes is not provided.
- The bank select switches both what the CPU writes and what the screen shows. A spare bank can only be filled during a blank, at about 10 cycles a colour.
- Maiya's whole-screen changes are fades, which `NG_PALFX_SCREEN`'s two-frame blend already covers.

**Colour cycling** is `ng_palfx_cycle()` (`ng_palette_fx.h`). Maiya's scenery swaps two colours of a painting every 16 frames (`mg_animate_scenery`).

## Not verified

- Classic MVS and AES hardware. MAME's timer timing is known to be off by a couple of lines.
- Where a band's writes land in the line on hardware.
