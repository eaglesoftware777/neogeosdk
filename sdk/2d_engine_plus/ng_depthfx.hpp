/*
 * ng_depthfx.h — 2.5D depth projection helper (Stage 10)
 *
 * Projects a 3D (x, y, z) world position onto the NeoGeo screen using
 * a simple perspective divide approximation.  Z is used to:
 *   1. Scale the sprite (via SCB2 shrink) using a lookup table.
 *   2. Select a fog/distance palette.
 *   3. Sort draw order (higher Z = farther = drawn behind lower Z).
 *
 * No floating point.  No runtime division for the common case.
 * The perspective project uses the ng_shrink_tab[] lookup for Z → shrink.
 * Screen X/Y offsets use fixed-point shifts.
 *
 * Coordinate system:
 *   X: left-right,   positive = right
 *   Y: up-down,      positive = down (screen space)
 *   Z: depth,        positive = into screen (farther away)
 *
 * Perspective formula (approximated with integer shift):
 *   screen_x = (x * PROJ_DIST) / (z + PROJ_DIST)   … but we use reciprocal table.
 *   screen_y = (y * PROJ_DIST) / (z + PROJ_DIST)
 *
 * For a fixed vanishing point at screen centre:
 *   screen_x = cx + (world_x - cx) * shrink / 255
 *   screen_y = cy + (world_y - cy) * shrink / 255
 *
 * where shrink = ng_shrink_tab[z & 0x7F] and cx/cy = screen centre.
 */

#ifndef NG_DEPTHFX_HPP
#define NG_DEPTHFX_HPP

#include "ng_defs.hpp"
#include "ng_sprite_group.hpp"
#include "ng_fixed.hpp"

#ifdef __cplusplus
extern "C" {
#endif


/* Screen vanishing point (default: screen centre) */
#define NG_DEPTH_CX   160
#define NG_DEPTH_CY   112

/* Palette fog bands: Z range [0..127] mapped to 4 palette offset bands */
#define NG_DEPTH_PAL_NEAR    0   /* z = 0..31:  use palette + 0 */
#define NG_DEPTH_PAL_MID     1   /* z = 32..63: use palette + 1 */
#define NG_DEPTH_PAL_FAR     2   /* z = 64..95: use palette + 2 */
#define NG_DEPTH_PAL_VERY_FAR 3  /* z = 96..127: use palette + 3 */

/* 3D world position */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;   /* 0 = camera plane, 127 = far clip */
} NGVec3;

/* Projected screen output */
typedef struct {
    int16_t screen_x;
    int16_t screen_y;
    uint8_t shrink_x;    /* SCB2 horizontal shrink byte (0x0F nibble = full) */
    uint8_t shrink_y;    /* SCB2 vertical shrink byte (0xFF = full) */
    uint8_t palette;     /* fog-adjusted palette index */
    uint8_t visible;     /* 0 if behind camera or off-screen */
} NGProjected;

void NEOGEO_USER ng_depthfx_init(void);

/*
 * Project a 3D world position to screen coordinates.
 * base_palette: the sprite's base palette slot; fog offset is added.
 */
NGProjected NEOGEO_USER ng_depthfx_project(NGVec3 p, uint8_t base_palette);

/*
 * Draw a sprite group at 3D world position.
 * Sets the group's x, y, xScale, yScale, and palette from projection,
 * then calls ng_sprite_group_update_transform().
 */
void NEOGEO_USER ng_depthfx_draw_group(NGSpriteGroup *group, NGVec3 p, uint8_t base_palette);

/*
 * Configure the depth system's fog palette range.
 * Each fog band maps to an additional palette offset.
 * band_offset[0..3]: how much to add to base_palette for each depth band.
 */
void NEOGEO_USER ng_depthfx_set_fog_palettes(uint8_t near_offset,
                                              uint8_t mid_offset,
                                              uint8_t far_offset,
                                              uint8_t very_far_offset);

/*
 * Simplified starfield / flying object helper.
 * Moves p->z towards camera by speed per frame.
 * When p->z <= 0, resets to z_max with random x/y spread.
 * Uses a cheap xorshift noise (no rand(), no stdlib).
 */
void NEOGEO_USER ng_depthfx_advance_star(NGVec3 *p, int16_t speed, int16_t z_max,
                                          int16_t spread_x, int16_t spread_y);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif