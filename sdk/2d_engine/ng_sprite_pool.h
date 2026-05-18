#ifndef NG_SPRITE_POOL_H
#define NG_SPRITE_POOL_H

#include <stdint.h>

/*
 * Hardware sprite slot priority layout.
 * Lower slot number = higher display priority (drawn in front of higher-numbered sprites).
 *
 *   Slots   1-95  : generated title/fullscreen previews
 *   Slots  96-223 : characters and NPCs
 *   Slots 224-255 : front effects
 *   Slots 256-287 : particles and temporary effects
 *   Slots 300-315 : background layer 0 (up to 16 strips = 256 px wide)
 *   Slots 316-331 : background layer 1 / overlay (up to 16 strips)
 *
 * Neo Geo hardware supports 380 sprite slots (0-379).
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
