#include "ng_bg.h"
#include "macro.h"
#include "neogeo.h"

static NGBgLayer ng_bg[NG_BG_LAYERS];

static const uint16_t ng_bg_slot[NG_BG_LAYERS] = {
    NG_SPR_BG0_FIRST,
    NG_SPR_BG1_FIRST,
};

void NEOGEO_USER ng_bg_init(void)
{
    uint8_t i;
    for (i = 0; i < NG_BG_LAYERS; i++) {
        ng_bg[i].screen_id = 0;
        ng_bg[i].show_fn   = 0;
        ng_bg[i].dirty     = 0;
        ng_bg[i].scroll_x  = 0;
        ng_bg[i].scroll_y  = 0;
    }
}

void NEOGEO_USER ng_bg_set(uint8_t layer, uint16_t screen_id, NGShowScreenFn show_fn,
                            int16_t x0, int16_t y0,
                            uint8_t xr, uint8_t yr, uint8_t min_crt_sz, uint8_t strips,
                            uint16_t backdrop,
                            int16_t parallax_x_fp, int16_t parallax_y_fp)
{
    NGBgLayer *bg;
    if (layer >= NG_BG_LAYERS) return;
    bg = &ng_bg[layer];

    bg->screen_id     = screen_id;
    bg->show_fn       = show_fn;
    bg->x0            = x0;
    bg->y0            = y0;
    bg->xr            = xr;
    bg->yr            = yr;
    bg->min_crt_sz    = min_crt_sz;
    bg->strips        = strips ? strips : (uint8_t)NG_SPR_BG_STRIPS;
    bg->backdrop      = backdrop;
    bg->scroll_x      = 0;
    bg->scroll_y      = 0;
    bg->parallax_x_fp = parallax_x_fp;
    bg->parallax_y_fp = parallax_y_fp;
    bg->dirty         = 1;
}

void NEOGEO_USER ng_bg_set_by_id(uint8_t layer, uint16_t screen_id,
                                  int16_t parallax_x_fp, int16_t parallax_y_fp)
{
    NGShowScreenFn fn = 0;
    if (screen_id > 0 && screen_id <= ng_screen_count) {
        fn = ng_screen_table[screen_id];
    }
    ng_bg_set(layer, screen_id, fn,
              16, 24, 0x0F, 0xAF, 16, NG_SPR_BG_STRIPS,
              0x0000,
              parallax_x_fp, parallax_y_fp);
}

void NEOGEO_USER ng_bg_clear(uint8_t layer)
{
    NGBgLayer *bg;
    uint16_t slot;
    uint8_t i;

    if (layer >= NG_BG_LAYERS) return;
    bg   = &ng_bg[layer];
    slot = ng_bg_slot[layer];

    for (i = 0; i < bg->strips && i < NG_SPR_BG_STRIPS; i++) {
        vram_SCB234((uint16_t)(SCB3_ADDR + slot + i), 0);
    }
    bg->screen_id = 0;
    bg->show_fn   = 0;
    bg->dirty     = 0;
}

void NEOGEO_USER ng_bg_hide_all(void)
{
    uint8_t layer;
    for (layer = 0; layer < NG_BG_LAYERS; layer++) {
        ng_bg_clear(layer);
    }
}

void NEOGEO_USER ng_bg_mark_dirty(uint8_t layer)
{
    if (layer < NG_BG_LAYERS) ng_bg[layer].dirty = 1;
}

void NEOGEO_USER ng_bg_draw(int16_t camera_x, int16_t camera_y)
{
    uint8_t layer;

    for (layer = 0; layer < NG_BG_LAYERS; layer++) {
        NGBgLayer *bg = &ng_bg[layer];
        uint16_t slot;
        int16_t par_x;
        int16_t draw_x;

        if (!bg->screen_id || !bg->show_fn) continue;

        slot = ng_bg_slot[layer];

        /*
         * Parallax: multiply camera offset by the layer's parallax factor.
         * parallax_x_fp = 0x0100 → scrolls 1:1 with camera (far layer).
         * parallax_x_fp = 0x0080 → scrolls at half speed (mid-distance layer).
         * parallax_x_fp = 0      → fixed background, doesn't scroll.
         */
        par_x  = bg->parallax_x_fp ? (int16_t)NG_FP_MUL(camera_x, (int32_t)(uint16_t)bg->parallax_x_fp) : 0;
        draw_x = (int16_t)(bg->x0 - par_x);

        if (bg->dirty) {
            /*
             * Full tile + palette upload.  sprite_base = NG_SPR_VRAM_BASE(slot)
             * ensures the generated showScreenN uses sprite slots [slot..slot+strips-1]
             * for SCB2/3/4 as well (after the genscreens.py fix).
             *
             * Background layers live at the LOW slots NG_SPR_BG0_FIRST (1) and
             * NG_SPR_BG1_FIRST (17).  In this engine's render contract the LSPC
             * walks slots from 0 upward and later writes cover earlier ones, so
             * the low BG slots are drawn FIRST and characters at slots 96..223
             * (NG_SPR_CHAR_*) end up rendered on top — see ng_sprite_pool.h
             * for the canonical layout.
             */
            bg->show_fn((int)draw_x, (int)bg->y0,
                        (int)bg->xr, (int)bg->yr,
                        (int)bg->min_crt_sz,
                        bg->backdrop,
                        NG_SPR_VRAM_BASE(slot));
            bg->dirty    = 0;
            bg->scroll_x = camera_x;
            bg->scroll_y = camera_y;
        } else if (camera_x != bg->scroll_x || camera_y != bg->scroll_y) {
            /*
             * Scroll-only update: write SCB3 (Y position, driver strip only)
             * and SCB4 (X position, all strips) without re-uploading tile data.
             * This is fast enough to call every frame.
             */
            uint16_t scb3_drv;
            uint8_t  strip;

            scb3_drv = setSCB3((uint16_t)(496 - (int)bg->y0), 0, bg->min_crt_sz);
            vram_SCB234((uint16_t)(SCB3_ADDR + slot), scb3_drv);

            for (strip = 0; strip < bg->strips && strip < NG_SPR_BG_STRIPS; strip++) {
                uint16_t scb4 = setSCB4((uint16_t)(draw_x + (int16_t)(strip * 16)));
                vram_SCB234((uint16_t)(SCB4_ADDR + slot + strip), scb4);
            }

            bg->scroll_x = camera_x;
            bg->scroll_y = camera_y;
        }
    }
}
