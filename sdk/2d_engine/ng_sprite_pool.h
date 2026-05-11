#ifndef NG_SPRITE_POOL_H
#define NG_SPRITE_POOL_H

#include <stdint.h>

/*
 * Hardware sprite slot priority layout.
 * Lower slot number = higher display priority (drawn in front of higher-numbered sprites).
 *
 *   Slots   0-299 : Characters, effects, NPCs, projectiles (caller manages)
 *   Slots 300-315 : Background layer 0 (up to 16 strips = 256 px wide)
 *   Slots 316-331 : Background layer 1 / overlay (up to 16 strips)
 *
 * Neo Geo hardware supports 380 sprite slots (0-379).
 *
 * The sprite_base parameter passed to showScreenN() is the VRAM byte offset
 * for tile data: sprite_base = slot * 64.  SCB1 address for strip S is
 * sprite_base + 64*S, and the hardware sprite slot for SCB2/3/4 is
 * sprite_base/64 + S.
 */
#define NG_SPR_CHAR_FIRST    0
#define NG_SPR_CHAR_LAST     299
#define NG_SPR_BG0_FIRST     300
#define NG_SPR_BG1_FIRST     316
#define NG_SPR_BG_STRIPS     16

/* sprite_base value for showScreenN: slot * 64 */
#define NG_SPR_VRAM_BASE(slot)   ((uint16_t)((uint16_t)(slot) * 64u))

#endif
