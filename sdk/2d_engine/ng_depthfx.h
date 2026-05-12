#ifndef NG_DEPTHFX_H
#define NG_DEPTHFX_H

#include "ng_defs.h"
#include "ng_chars.h"

/*
 * Optical depth FX for NeoGeo 2D sprites.
 * This is not polygonal 3D.  It maps a character's Y position or virtual Z
 * value to shrink registers and priority offset, creating SNK-style depth by
 * 2D illusion: small/far at the horizon, full/near at the bottom.
 */
typedef struct {
    int16_t far_y;
    int16_t mid_y;
    int16_t near_y;
    uint8_t far_scale;
    uint8_t mid_scale;
    uint8_t near_scale;
    uint8_t preserve_x_scale;
    uint8_t priority_band;
} NGDepthFXConfig;

void NEOGEO_USER ng_depthfx_default(NGDepthFXConfig *cfg);
uint8_t NEOGEO_USER ng_depthfx_scale_for_y(const NGDepthFXConfig *cfg, int16_t y);
void NEOGEO_USER ng_depthfx_apply_y(NGCharacter *c, const NGDepthFXConfig *cfg);
void NEOGEO_USER ng_depthfx_apply_virtual_z(NGCharacter *c, const NGDepthFXConfig *cfg, int16_t z);

#endif
