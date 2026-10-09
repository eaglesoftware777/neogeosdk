#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ng_shooter.h"

static unsigned uploads, flushes, hides;
void ng_sprite_group_init(NGSpriteGroup *g, uint16_t slot, uint8_t strips,
                          uint8_t rows, uint16_t tile, uint8_t palette)
{
    memset(g, 0, sizeof(*g));
    g->firstSprite = slot; g->strips = strips; g->heightTiles = rows;
    g->tileBase = tile; g->palette = palette;
}
void ng_sprite_group_set_tile_stride(NGSpriteGroup *g, uint16_t stride) { g->tileStride = stride; }
void ng_sprite_group_set_palette_map(NGSpriteGroup *g, const uint8_t *map) { g->tilePalettes = map; }
void ng_sprite_group_set_priority(NGSpriteGroup *g, uint8_t priority) { g->prio = priority; }
void ng_sprite_group_set_pos(NGSpriteGroup *g, int16_t x, int16_t y) { g->x = x; g->y = y; }
void ng_sprite_group_upload(NGSpriteGroup *g) { (void)g; uploads++; }
void ng_sprite_group_flush(NGSpriteGroup *g) { (void)g; flushes++; }
void ng_sprite_group_hide(NGSpriteGroup *g) { (void)g; hides++; }

static int expected_y(int32_t travel, int ratio)
{
    int64_t q8 = (int64_t)travel * ratio / 256;
    int phase = (int)(q8 % (512*256));
    if (phase < 0) phase += 512*256;
    phase /= 256;
    return phase >= 256 ? phase-512 : phase;
}

int main(void)
{
    NGVerticalLayer layer;
    NGShooterCamera cam;
    uint8_t banks[704] = {0};
    int i, ratio;
    memset(&layer, 0, sizeof(layer));
    ng_shooter_camera_init(&cam);
    ng_shooter_camera_set_speed(&cam, 128);
    for (i=0; i<120; i++) ng_shooter_camera_step(&cam, 160, 112);
    assert(cam.travel_q8 == 60*256);
    ng_shooter_camera_set_follow(&cam, 8, 6, 16, 4);
    for (i=0; i<1000; i++) ng_shooter_camera_step(&cam, 300, 200);
    assert(cam.x_q8 == 8*256 && cam.y_q8 == 6*256);
    for (i=0; i<1000; i++) ng_shooter_camera_step(&cam, 20, 20);
    assert(cam.x_q8 == -8*256 && cam.y_q8 == -6*256);
    for (i=0; i<1000; i++) ng_shooter_camera_step(&cam, 160, 112);
    assert(cam.x_q8 == 0 && cam.y_q8 == 0);
    cam.travel_q8 = 0x3fffffff;
    ng_shooter_camera_step(&cam, 160, 112);
    assert(cam.travel_q8 == 0x3fffffff);
    ng_shooter_camera_set_speed(&cam, -128);
    cam.travel_q8 = -0x3fffffff;
    ng_shooter_camera_step(&cam, 160, 112);
    assert(cam.travel_q8 == -0x3fffffff);
    assert(!ng_vertical_layer_init(&layer, 1, 0, 16, 0, 16, banks, -16));
    assert(!ng_vertical_layer_init(&layer, 340, 22, 16, 0, 16, banks, -16));
    assert(!ng_vertical_layer_init(&layer, 1, 22, 13, 0, 16, banks, -16));
    assert(!ng_vertical_layer_init(&layer, 1, 22, 16, 65500, 16, banks, -16));
    assert(uploads == 0);
    assert(ng_vertical_layer_init(&layer, 1, 22, 16, 100, 16, banks, -16));
    assert(uploads == 2 && layer.pages[1].tileBase == 452);
    assert(layer.pages[1].tilePalettes == banks+352);
    assert(layer.pages[0].prio == NG_SG_PRIO_HIGH);
    for (ratio=0; ratio<=1024; ratio+=128) {
        for (i=-1800; i<1800; i++) {
            cam.travel_q8 = i*193;
            ng_vertical_layer_draw(&layer, &cam, ratio);
            assert(layer.pages[0].y == expected_y(cam.travel_q8, ratio));
            assert(layer.pages[0].y >= -256 && layer.pages[0].y < 256);
            assert(layer.pages[1].y >= -256 && layer.pages[1].y < 256);
            assert(layer.pages[0].y-layer.pages[1].y == 256 ||
                   layer.pages[1].y-layer.pages[0].y == 256);
        }
        cam.travel_q8 = 0x3fffffff;
        ng_vertical_layer_draw(&layer, &cam, ratio);
        assert(layer.pages[0].y == expected_y(cam.travel_q8, ratio));
    }
    assert(uploads == 2 && flushes > 60000);
    ng_vertical_layer_hide(&layer);
    assert(!layer.ready && hides == 2);
    ng_vertical_layer_draw(&layer, &cam, 256);
    ng_shooter_camera_step(NULL, 0, 0);
    ng_vertical_layer_hide(NULL);
    puts("Shooter: fractional/reverse scrolling, wraps, parallax, camera and validation passed");
    return 0;
}
