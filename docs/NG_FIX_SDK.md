# NeoGeo FIX SDK

`sdk/ng_fix` is the low-level FIX-layer helper module for the NeoGeo SDK.

The FIX layer is the 8x8 tile plane sourced from the S1 ROM.  It is best used
for HUD text, scores, captions, debug overlays, menu frames, bars, simple UI
icons, and scene labels.  It is not a scrolling background layer.

## Files

```c
#include "sdk/ng_fix/ng_fix.h"
```

The module is intentionally separate from `sdk/2d_engine`.  The 2D engine may
use it, but the FIX SDK can also be used directly by title screens, sound tests,
the BIOS/demo flow, and diagnostic tools.

## Core ideas

- Screen size: 40x32 FIX cells.
- Each cell is one 8x8 S1 tile.
- Palette is limited to 0..15.
- Blank cells use tile `$00FF` by default, matching the BIOS clear pattern.
- Cached writes avoid rewriting unchanged FIX cells every frame.

## Basic text

```c
ngfix_init();
ngfix_clear();
ngfix_puts(2, 2, "NEO GEO SDK", 0);
ngfix_center(4, "FIX LAYER TEST", 1);
```

## Raw tile output

Use raw tile writes when the S1 ROM contains icons, Japanese glyphs, custom
borders, meters, or font tiles that are not ASCII text.

```c
ngfix_put_tile(10, 8, 0x0120, 2);  /* tile $120, palette 2 */
ngfix_write_tile(11, 8, 0x0121, 2); /* forced hardware write */
```

## Custom font mapping

The default mapping uses the character byte as the tile number, preserving the
existing SDK behavior.  For a custom S1 font, set `ascii_base`.

```c
NGFixFont font = { 0x0200, 0x00FF, '?', 1 };
ngfix_set_font(&font);
ngfix_puts(2, 2, "CUSTOM FONT", 0);
```

## Numbers, boxes, bars

```c
ngfix_put_u16(2, 5, score, 0, 5);
ngfix_put_hex16(2, 6, 0x7777, 1);
ngfix_box(1, 1, 38, 6, 0);
ngfix_bar_u8(4, 8, 16, hp, max_hp, 2, '#', '-');
```

## BRDFIX — Game S ROM vs BIOS S ROM

The BRDFIX register (`$3A0003` bit 0) controls which S ROM the hardware uses for
FIX tile lookups:

- `0` = BIOS S ROM (default at boot, used by BIOS routines)
- `1` = Game S ROM (required for any game tile output)

`ngfix_init()` calls `setsfix()` which sets BRDFIX to 1.

`ngfix_clear()` calls `clearFix()` which internally calls the BIOS `SYS_FIX_CLEAR`
routine.  That BIOS routine resets BRDFIX to 0.  `clearFix()` now immediately
restores BRDFIX to 1 after the BIOS call so that all tile output that follows uses
the game S ROM.

You do not need to call `setsfix()` manually after `ngfix_clear()`.  The restoration
is handled inside `clearFix()`.

## Compatibility

`sdk/2d_engine/ng_fix.c` now delegates to this module, so existing calls such as
`ng_fix_puts()` continue to work while the standalone FIX SDK becomes the common
implementation.
