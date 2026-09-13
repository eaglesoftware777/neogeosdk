#ifndef SDK_NG_SHOW_NG_SHOW_H
#define SDK_NG_SHOW_NG_SHOW_H

#include <stdint.h>
#include "macro.h"
#include "ng_video/ng_video.h"

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

typedef void (*NGShowScreenFn)(int, int, int, int, int, uint16_t, uint16_t);

typedef struct {
    NGShowScreenFn fn;
    int16_t x;
    int16_t y;
    uint8_t x_shrink;
    uint8_t y_shrink;
    uint8_t height_tiles;
    uint16_t backdrop;
    uint16_t sprite_base;
    uint16_t clear_first;
    uint16_t clear_count;
} NGShowScreenDraw;

void NEOGEO_USER ngshow_clear_range(uint16_t first, uint16_t count);
void NEOGEO_USER ngshow_draw(const NGShowScreenDraw *draw);
void NEOGEO_USER ngshow_draw_simple(NGShowScreenFn fn, int16_t x, int16_t y, uint8_t height_tiles, uint16_t sprite_base);
void NEOGEO_USER ngshow_preload_palette(NGShowScreenFn fn, uint16_t sprite_base);

#endif
