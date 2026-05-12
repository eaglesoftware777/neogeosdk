#include "ng_show/ng_show.h"
#include "neogeo.h"

void NEOGEO_USER ngshow_clear_range(uint16_t first, uint16_t count)
{
    ngvideo_clear_sprites(first, count);
}

void NEOGEO_USER ngshow_draw(const NGShowScreenDraw *draw)
{
    uint16_t first;
    uint16_t count;

    if (!draw || !draw->fn) return;

    first = draw->clear_first;
    count = draw->clear_count ? draw->clear_count : NGVIDEO_RANGE_SHOW_COUNT;
    ngshow_clear_range(first, count);

    draw->fn((int)draw->x,
             (int)draw->y,
             (int)draw->x_shrink,
             (int)draw->y_shrink,
             (int)draw->height_tiles,
             draw->backdrop,
             draw->sprite_base);
}

void NEOGEO_USER ngshow_draw_simple(NGShowScreenFn fn, int16_t x, int16_t y, uint8_t height_tiles, uint16_t sprite_base)
{
    NGShowScreenDraw draw;

    draw.fn = fn;
    draw.x = x;
    draw.y = y;
    draw.x_shrink = 0x0F;
    draw.y_shrink = 0xAF;
    draw.height_tiles = height_tiles;
    draw.backdrop = 0x0000;
    draw.sprite_base = sprite_base;
    draw.clear_first = (uint16_t)(sprite_base / 64u);
    draw.clear_count = NGVIDEO_RANGE_SHOW_COUNT;
    ngshow_draw(&draw);
}

void NEOGEO_USER ngshow_preload_palette(NGShowScreenFn fn, uint16_t sprite_base)
{
    if (!fn) return;
    fn(-320, 1024, 0, 0, 0, 0x0000, sprite_base);
    waitVbl();
    ngvideo_clear_sprites((uint16_t)(sprite_base / 64u), NGVIDEO_RANGE_SHOW_COUNT);
}
