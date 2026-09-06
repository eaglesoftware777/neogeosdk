#ifndef NG_BG_H
#define NG_BG_H

#include "ng_defs.h"
#include "ng_sprite_pool.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Background layers */
#define NG_BG_LAYERS     2
#define NG_BG_LAYER0     0   /* main background (behind everything) */
#define NG_BG_LAYER1     1   /* foreground overlay */

/* Max screens in dispatch table (must be >= total asset count) */
#define NG_SCREEN_TABLE_MAX  200

typedef void (*NGShowScreenFn)(int, int, int, int, int, uint16_t, uint16_t);

/* Dispatch table filled by ng_screen_table_init() generated in screens.c / main.c.
 * Index is the 1-based screen_id; index 0 is unused (NULL). */
extern const NGShowScreenFn ng_screen_table[];
extern const uint16_t       ng_screen_count;

typedef struct {
    uint16_t       screen_id;
    NGShowScreenFn show_fn;
    int16_t        x0;
    int16_t        y0;
    uint8_t        xr;
    uint8_t        yr;
    uint8_t        min_crt_sz;
    uint8_t        strips;
    uint16_t       backdrop;
    int16_t        scroll_x;
    int16_t        scroll_y;
    int16_t        parallax_x_fp;  /* 8.8 fixed-point; 0x0100 = 1:1, 0x0080 = half speed */
    int16_t        parallax_y_fp;
    uint8_t        dirty;
} NGBgLayer;

void NEOGEO_USER ng_bg_init(void);

/* Set a background layer.  show_fn is the generated showScreenN function.
 * x0/y0: top-left position on screen.
 * xr/yr: scale nibble / byte (0xF/0xFF = full size).
 * min_crt_sz: height in tiles passed to setSCB3.
 * strips: number of 16-px strips (typically 16 for a 256-px wide background).
 * parallax_x_fp / parallax_y_fp: 8.8 fixed-point scroll multipliers
 *   (0x0100 = 1:1 scroll with camera, 0x0080 = half-speed, 0 = fixed). */
void NEOGEO_USER ng_bg_set(uint8_t layer, uint16_t screen_id, NGShowScreenFn show_fn,
                            int16_t x0, int16_t y0,
                            uint8_t xr, uint8_t yr, uint8_t min_crt_sz, uint8_t strips,
                            uint16_t backdrop,
                            int16_t parallax_x_fp, int16_t parallax_y_fp);

/* Set background by screen_id using the auto-generated dispatch table.
 * Calls ng_bg_set with full-screen defaults. */
void NEOGEO_USER ng_bg_set_by_id(uint8_t layer, uint16_t screen_id,
                                  int16_t parallax_x_fp, int16_t parallax_y_fp);

void NEOGEO_USER ng_bg_clear(uint8_t layer);
void NEOGEO_USER ng_bg_hide_all(void);
void NEOGEO_USER ng_bg_mark_dirty(uint8_t layer);

/* Call each frame with current camera position.  Writes SCB3/4 for scroll-
 * only updates; re-uploads tile data when dirty. */
void NEOGEO_USER ng_bg_draw(int16_t camera_x, int16_t camera_y);


#ifdef __cplusplus
}
#endif
#endif
