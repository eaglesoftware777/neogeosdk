/*
 * ng_depthfx.c — Depth FX / 2.5D projection (Stage 10)
 *
 * Perspective projection without division:
 *   shrink = ng_shrink_tab[z & 0x7F]   (range 0x20..0xFF)
 *
 *   screen_x = cx + ((world_x - cx) * shrink) >> 8
 *   screen_y = cy + ((world_y - cy) * shrink) >> 8
 *
 * The >> 8 shift replaces the division by 256 that would normalise shrink.
 * Since ng_shrink_tab values range 0x20..0xFF, the shift by 8 gives us
 * roughly 12.5%..100% projection scaling — adequate for arcade depth cues.
 *
 * NeoGeo SCB2 split:
 *   bits 15..8 = horizontal shrink nibble (put in bits 11..8 = high nibble)
 *   bits  7..0 = vertical shrink byte
 *
 * For this projection:
 *   shrink_x = shrink >> 4 (4-bit nibble, 0..15)
 *   shrink_y = shrink       (8-bit byte, 0..255)
 */

#include "ng_depthfx.hpp"
#include "ng_hw.hpp"

extern "C" {


static uint8_t ng_df_fog_near;
static uint8_t ng_df_fog_mid;
static uint8_t ng_df_fog_far;
static uint8_t ng_df_fog_very_far;

/* Cheap xorshift16 for starfield — no stdlib rand */
static uint16_t ng_df_noise = 0xACE1;

static uint16_t NEOGEO_USER ng_df_xorshift(void)
{
    ng_df_noise ^= (uint16_t)(ng_df_noise << 7);
    ng_df_noise ^= (uint16_t)(ng_df_noise >> 9);
    ng_df_noise ^= (uint16_t)(ng_df_noise << 8);
    return ng_df_noise;
}

void NEOGEO_USER ng_depthfx_init(void)
{
    ng_df_fog_near     = 0;
    ng_df_fog_mid      = 0;
    ng_df_fog_far      = 1;
    ng_df_fog_very_far = 2;
}

void NEOGEO_USER ng_depthfx_set_fog_palettes(uint8_t near_offset,
                                              uint8_t mid_offset,
                                              uint8_t far_offset,
                                              uint8_t very_far_offset)
{
    ng_df_fog_near     = near_offset;
    ng_df_fog_mid      = mid_offset;
    ng_df_fog_far      = far_offset;
    ng_df_fog_very_far = very_far_offset;
}

NGProjected NEOGEO_USER ng_depthfx_project(NGVec3 p, uint8_t base_palette)
{
    NGProjected out;
    uint8_t  shrink;
    int16_t  dx;
    int16_t  dy;
    uint8_t  fog_off;

    /* Default: not visible */
    out.visible = 0;

    /* Clamp Z to valid range */
    if (p.z < 0) p.z = 0;
    if (p.z >= NGFX_SHRINK_ENTRIES) p.z = (int16_t)(NGFX_SHRINK_ENTRIES - 1);

    /* Lookup shrink value from table — avoids division */
    shrink = ng_shrink_tab[(uint8_t)p.z];

    if (shrink == 0) {
        /* Sprite too far — invisible */
        return out;
    }

    /*
     * Project world X/Y relative to vanishing point.
     * screen = vp + (world - vp) * shrink / 256
     * = vp + ((world - vp) * shrink) >> 8
     *
     * 68000: one MULS per axis, 16x8 → 24 bit result, shift right 8.
     */
    dx = (int16_t)(p.x - NG_DEPTH_CX);
    dy = (int16_t)(p.y - NG_DEPTH_CY);

    out.screen_x = (int16_t)(NG_DEPTH_CX + ((int32_t)dx * (int32_t)shrink >> 8));
    out.screen_y = (int16_t)(NG_DEPTH_CY + ((int32_t)dy * (int32_t)shrink >> 8));

    /*
     * SCB2 encoding:
     *   bits 15..8 = horizontal shrink nibble (stored in bits 11..8)
     *   bits  7..0 = vertical shrink byte
     * setSCB2 expects: xNibble = shrink >> 4, yShrink = shrink
     */
    out.shrink_x = (uint8_t)(shrink >> 4);    /* 0..15 nibble */
    out.shrink_y = shrink;                     /* 0..255 byte  */

    /* Fog palette selection */
    if (p.z < 32)
        fog_off = ng_df_fog_near;
    else if (p.z < 64)
        fog_off = ng_df_fog_mid;
    else if (p.z < 96)
        fog_off = ng_df_fog_far;
    else
        fog_off = ng_df_fog_very_far;

    out.palette = (uint8_t)(base_palette + fog_off);

    /* Visibility: cull off-screen with small margin */
    if (out.screen_x < -32 || out.screen_x > NG_SCREEN_WIDTH + 32 ||
        out.screen_y < -32 || out.screen_y > NG_SCREEN_HEIGHT + 32) {
        return out; /* visible=0 */
    }

    out.visible = 1;
    return out;
}

void NEOGEO_USER ng_depthfx_draw_group(NGSpriteGroup *group, NGVec3 p, uint8_t base_palette)
{
    NGProjected proj;

    if (!group) return;

    proj = ng_depthfx_project(p, base_palette);

    if (!proj.visible) {
        ng_sprite_group_hide(group);
        return;
    }

    group->x      = proj.screen_x;
    group->y      = proj.screen_y;
    group->xScale = (uint8_t)(proj.shrink_x << 4); /* convert nibble to 8-bit scale */
    group->yScale = proj.shrink_y;
    group->palette = proj.palette;

    ng_sprite_group_update_transform(group);
}

void NEOGEO_USER ng_depthfx_advance_star(NGVec3 *p, int16_t speed, int16_t z_max,
                                          int16_t spread_x, int16_t spread_y)
{
    uint16_t noise;

    if (!p) return;

    p->z -= speed;

    if (p->z <= 0) {
        /* Reset star to far plane with random position */
        p->z = z_max;

        noise = ng_df_xorshift();
        /* Map noise to spread range: noise % spread (avoid modulo via mask trick) */
        /* Use bit masking for powers of 2 spread, or just keep noise in range */
        {
            int16_t rx = (int16_t)(noise & 0x1FF);  /* 0..511 */
            int16_t ry;
            noise = ng_df_xorshift();
            ry = (int16_t)(noise & 0x0FF);           /* 0..255 */

            /* Centre around NG_DEPTH_CX / CY */
            if (spread_x > 0) rx = (int16_t)(NG_DEPTH_CX - spread_x + (rx % (spread_x * 2)));
            else rx = NG_DEPTH_CX;

            if (spread_y > 0) ry = (int16_t)(NG_DEPTH_CY - spread_y + (ry % (spread_y * 2)));
            else ry = NG_DEPTH_CY;

            p->x = rx;
            p->y = ry;
        }
    }
}


} /* extern "C" */
