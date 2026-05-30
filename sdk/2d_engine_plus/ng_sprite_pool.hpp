#ifndef NG_SPRITE_POOL_HPP
#define NG_SPRITE_POOL_HPP

#include <stdint.h>

/*
 * Hardware sprite slot priority layout.
 *
 * Lower sprite slot numbers draw first, so higher-numbered slots draw in
 * front. Keep backgrounds in low ranges and active characters/effects above
 * them so large background strips cannot cover gameplay sprites.
 *
 * Recommended layout (back to front):
 *   Slots   1-32  : background layers / parallax
 *   Slots  33-95  : title/front one-off generated screens
 *   Slots  96-223 : characters / NPCs assigned by ng_chars
 *   Slots 224-255 : effects
 *   Slots 256-287 : particles
 *   Slots 288-379 : temporary/front effects
 *
 * The sprite_base parameter passed to showScreenN() is the VRAM byte offset
 * for tile data: sprite_base = slot * 64.  SCB1 address for strip S is
 * sprite_base + 64*S, and the hardware sprite slot for SCB2/3/4 is
 * sprite_base/64 + S.
 */
#define NG_SPR_TOTAL         380
#define NG_SPR_LAST          379
#define NG_SPR_TITLE_FIRST   33
#define NG_SPR_TITLE_LAST    95
#define NG_SPR_CHAR_FIRST    96
#define NG_SPR_CHAR_LAST     223
#define NG_SPR_FX_FIRST      224
#define NG_SPR_FX_LAST       255
#define NG_SPR_PART_FIRST    256
#define NG_SPR_PART_LAST     287
#define NG_SPR_TEMP_FIRST    288
#define NG_SPR_TEMP_LAST     379
#define NG_SPR_BG0_FIRST     1
#define NG_SPR_BG1_FIRST     17
#define NG_SPR_BG_STRIPS     16

/* sprite_base value for showScreenN: slot * 64 */
#define NG_SPR_VRAM_BASE(slot)   ((uint16_t)((uint16_t)(slot) * 64u))

#endif
