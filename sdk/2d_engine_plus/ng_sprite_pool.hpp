#ifndef NG_SPRITE_POOL_HPP
#define NG_SPRITE_POOL_HPP

#include <stdint.h>

/*
 * Hardware sprite slot priority layout.
 *
 *   ╔══════════════════════════════════════════════════════════════════╗
 *   ║  RULE: LOWER hardware slot number = drawn IN FRONT.              ║
 *   ║         HIGHER slot number = drawn BEHIND.                       ║
 *   ║                                                                  ║
 *   ║  Neo Geo LSPC scans the sprite list from slot 0 first; each      ║
 *   ║  later sprite that overlaps is OBSCURED by the earlier-drawn     ║
 *   ║  pixels, so a lower-numbered sprite always wins.                 ║
 *   ╚══════════════════════════════════════════════════════════════════╝
 *
 * Recommended layout (front to back):
 *   Slots   1-15  : HUD / overlays (drawn on top of everything)
 *   Slots  16-95  : foreground sprites — heroes, projectiles
 *   Slots  96-223 : characters / NPCs (assigned automatically by ng_chars)
 *   Slots 224-255 : front effects (in front of chars but behind hero)
 *   Slots 256-287 : particles
 *   Slots 288-299 : transient effects
 *   Slots 300-315 : BACKGROUND layer 0 (full 16 strips for a 256-px wallpaper)
 *   Slots 316-331 : BACKGROUND layer 1 / parallax behind layer 0
 *
 * The sprite_base parameter passed to showScreenN() is the VRAM byte offset
 * for tile data: sprite_base = slot * 64.  SCB1 address for strip S is
 * sprite_base + 64*S, and the hardware sprite slot for SCB2/3/4 is
 * sprite_base/64 + S.
 *
 * COMMON MISTAKE: do NOT draw a background at slot 1 expecting it to sit
 * "behind" the player at slot 96 — slot 1 is the FRONT-most slot and will
 * cover the hero.  Backgrounds belong at slot 300+ where they are drawn
 * BEHIND every char and effect.
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
