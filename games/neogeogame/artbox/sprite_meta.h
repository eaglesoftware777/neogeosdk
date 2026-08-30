#ifndef ARTBOX_SPRITE_META_H
#define ARTBOX_SPRITE_META_H

#include <stdint.h>

typedef struct {
    uint16_t tile_base;
    uint8_t palette_bank;
    uint8_t strips;
    uint8_t active_rows;
    uint8_t tile_col_start;
    uint8_t tile_row_start;
    uint8_t x_pad;
    uint8_t y_pad;
    uint16_t content_width;
    uint16_t content_height;
    uint8_t mode;
    uint8_t category;
} NGSpriteAssetMeta;

#define NG_ASSET_MODE_SCREEN 0
#define NG_ASSET_MODE_SPRITE 1

#define NG_ASSET_CATEGORY_BACKGROUND 0
#define NG_ASSET_CATEGORY_MAIN_CHARACTER 1
#define NG_ASSET_CATEGORY_OPPONENT 2
#define NG_ASSET_CATEGORY_NPC 3

#define NG_ASSET_META_COUNT 10

static const NGSpriteAssetMeta g_ng_asset_meta[NG_ASSET_META_COUNT] = {
    { 0, 16, 16, 16, 0, 0, 0, 0, 256, 256, 0, 0 }, /* 001_starfield.png */
    { 256, 17, 16, 16, 0, 0, 0, 0, 256, 256, 0, 0 }, /* 002_space_bg.png */
    { 512, 18, 4, 2, 6, 14, 12, 6, 40, 26, 1, 0 }, /* 010_ship.png */
    { 768, 19, 4, 2, 6, 14, 10, 6, 43, 26, 1, 0 }, /* 011_ship_alt.png */
    { 1024, 20, 4, 4, 6, 12, 0, 0, 64, 64, 1, 0 }, /* i_are_spaceship.png */
    { 1280, 21, 2, 2, 7, 7, 13, 10, 5, 11, 1, 0 }, /* 030_player_bullet.png */
    { 1536, 22, 2, 2, 7, 7, 13, 10, 5, 11, 1, 0 }, /* 031_enemy_bullet.png */
    { 1792, 23, 4, 4, 6, 6, 13, 13, 37, 37, 1, 0 }, /* 040_explosion.png */
    { 2048, 24, 16, 11, 0, 5, 0, 11, 256, 165, 1, 3 }, /* 020_enemy.png */
    { 2304, 25, 2, 1, 7, 15, 5, 3, 22, 13, 1, 3 }, /* 032_life.png */
};

#endif
