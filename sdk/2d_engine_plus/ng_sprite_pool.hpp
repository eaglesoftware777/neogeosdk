#ifndef NG_SPRITE_POOL_HPP
#define NG_SPRITE_POOL_HPP

#include <stdint.h>

/*
 * Hardware sprite slot priority layout.
 *
 *   ╔══════════════════════════════════════════════════════════════════╗
 *   ║  OBSERVED HARDWARE RULE: HIGHER slot number = drawn IN FRONT.    ║
 *   ║                          LOWER slot number  = drawn BEHIND.      ║
 *   ║                                                                  ║
 *   ║  This contradicts older SDK comments that claimed the opposite.  ║
 *   ║  The direction was determined empirically — putting a background ║
 *   ║  at slot 300 covered chars at slot 96, confirming HIGHER = TOP.  ║
 *   ╚══════════════════════════════════════════════════════════════════╝
 *
 * Recommended layout (back to front):
 *   Slots   1-15  : BACKGROUND back layer  (drawn behind everything)
 *   Slots  16-31  : BACKGROUND parallax mid-layer
 *   Slots  96-223 : characters / NPCs (assigned automatically by ng_chars)
 *   Slots 224-255 : front effects on top of chars
 *   Slots 256-287 : particles
 *   Slots 288-299 : transient effects
 *   Slots 300-345 : foreground sprites / hero
 *   Slots 346-379 : HUD / overlays (drawn on top of everything)
 *
 * The legacy names NG_SPR_BG0_FIRST = 300 / NG_SPR_BG1_FIRST = 316 are
 * MISLEADING — those slots are at the FRONT, not the back.  New code
 * that needs a true background should use slots 1..15.  The existing
 * names are kept for source compatibility but should be considered
 * deprecated.
 *
 * The sprite_base parameter passed to showScreenN() is the VRAM byte offset
 * for tile data: sprite_base = slot * 64.  SCB1 address for strip S is
 * sprite_base + 64*S, and the hardware sprite slot for SCB2/3/4 is
 * sprite_base/64 + S.
 */
#define NG_SPR_TOTAL         380
#define NG_SPR_LAST          379
#define NG_SPR_TITLE_FIRST   1
#define NG_SPR_TITLE_LAST    95
#define NG_SPR_CHAR_FIRST    96
#define NG_SPR_CHAR_LAST     223
#define NG_SPR_FX_FIRST      224
#define NG_SPR_FX_LAST       255
#define NG_SPR_PART_FIRST    256
#define NG_SPR_PART_LAST     287
#define NG_SPR_TEMP_FIRST    288
#define NG_SPR_TEMP_LAST     299
#define NG_SPR_BG0_FIRST     300
#define NG_SPR_BG1_FIRST     316
#define NG_SPR_BG_STRIPS     16

/* sprite_base value for showScreenN: slot * 64 */
#define NG_SPR_VRAM_BASE(slot)   ((uint16_t)((uint16_t)(slot) * 64u))

#endif
