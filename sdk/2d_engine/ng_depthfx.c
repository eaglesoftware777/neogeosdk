#include "ng_depthfx.h"

void NEOGEO_USER ng_depthfx_default(NGDepthFXConfig *cfg)
{
    if (!cfg) return;
    cfg->far_y = 88;
    cfg->mid_y = 124;
    cfg->near_y = 176;
    cfg->far_scale = 0xB8;
    cfg->mid_scale = 0xD0;
    cfg->near_scale = 0xEC;
    cfg->preserve_x_scale = 0;
    cfg->priority_band = NG_RENDER_BAND_PLAYER;
}

static uint8_t NEOGEO_USER ng_depthfx_lerp_u8(uint8_t a, uint8_t b, uint8_t t)
{
    /* >>8 instead of /255: avoids DIVS, error ≤1 LSB at typical scale values */
    return (uint8_t)((uint16_t)a + (((int16_t)((int16_t)b - (int16_t)a) * (int16_t)t) >> 8));
}

uint8_t NEOGEO_USER ng_depthfx_scale_for_y(const NGDepthFXConfig *cfg, int16_t y)
{
    int16_t num;
    int16_t den;
    uint8_t t;
    NGDepthFXConfig local;

    if (!cfg) {
        ng_depthfx_default(&local);
        cfg = &local;
    }

    if (y <= cfg->far_y) return cfg->far_scale;
    if (y >= cfg->near_y) return cfg->near_scale;

    if (y < cfg->mid_y) {
        num = (int16_t)(y - cfg->far_y);
        den = (int16_t)(cfg->mid_y - cfg->far_y);
        if (den <= 0) return cfg->mid_scale;
        t = (uint8_t)((num * 255) / den);
        return ng_depthfx_lerp_u8(cfg->far_scale, cfg->mid_scale, t);
    }

    num = (int16_t)(y - cfg->mid_y);
    den = (int16_t)(cfg->near_y - cfg->mid_y);
    if (den <= 0) return cfg->near_scale;
    t = (uint8_t)((num * 255) / den);
    return ng_depthfx_lerp_u8(cfg->mid_scale, cfg->near_scale, t);
}

void NEOGEO_USER ng_depthfx_apply_y(NGCharacter *c, const NGDepthFXConfig *cfg)
{
    uint8_t scale;
    NGDepthFXConfig local;

    if (!c || !c->visible) return;
    if (!cfg) {
        ng_depthfx_default(&local);
        cfg = &local;
    }

    scale = ng_depthfx_scale_for_y(cfg, c->y);

    /* Scale lives in SCB2; update_transform writes it every frame without a
     * full tile re-upload.  Only set dirty if tiles actually need re-uploading. */
    if (!cfg->preserve_x_scale)
        c->scale_x = scale;
    c->scale_y = scale;

    ng_char_set_priority(c, cfg->priority_band, c->y);
}

void NEOGEO_USER ng_depthfx_apply_virtual_z(NGCharacter *c, const NGDepthFXConfig *cfg, int16_t z)
{
    NGDepthFXConfig local;
    int16_t mapped_y;

    if (!c) return;
    if (!cfg) {
        ng_depthfx_default(&local);
        cfg = &local;
    }

    mapped_y = (int16_t)(cfg->near_y - z);
    ng_depthfx_apply_y(c, cfg);
    c->depth_offset = mapped_y;
}
