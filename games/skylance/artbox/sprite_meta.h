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

#define NG_ASSET_META_COUNT 25

static const NGSpriteAssetMeta g_ng_asset_meta[NG_ASSET_META_COUNT] = {
    { 0, 16, 16, 16, 0, 0, 0, 0, 256, 256, 0, 0 }, /* background_coast_city.png */
    { 256, 17, 16, 16, 0, 0, 0, 0, 256, 256, 0, 0 }, /* background_sky_mountains.png */
    { 512, 18, 10, 15, 3, 1, 0, 0, 160, 240, 1, 0 }, /* sprite_p1_pilot.png */
    { 768, 19, 8, 10, 4, 6, 8, 0, 112, 160, 1, 0 }, /* sprite_p1_plane.png */
    { 1024, 20, 10, 15, 3, 1, 0, 0, 160, 240, 1, 0 }, /* sprite_p2_pilot.png */
    { 1280, 21, 8, 9, 4, 7, 8, 0, 112, 144, 1, 0 }, /* sprite_p2_plane.png */
    { 1536, 22, 10, 15, 3, 1, 0, 0, 160, 240, 1, 0 }, /* sprite_p3_pilot.png */
    { 1792, 23, 8, 9, 4, 7, 8, 0, 112, 144, 1, 0 }, /* sprite_p3_plane.png */
    { 2048, 24, 2, 3, 7, 13, 12, 8, 8, 40, 1, 0 }, /* sprite_shot_player.png */
    { 2304, 25, 10, 10, 3, 6, 8, 0, 144, 160, 1, 3 }, /* opponent_bomber_olive.png */
    { 2560, 26, 10, 16, 3, 0, 7, 0, 146, 256, 1, 3 }, /* opponent_boss_crimson_cathedral.png */
    { 2816, 27, 16, 15, 0, 1, 8, 0, 240, 240, 1, 3 }, /* opponent_boss_gold_core.png */
    { 3072, 28, 14, 16, 1, 0, 13, 0, 197, 256, 1, 3 }, /* opponent_boss_heli_carrier.png */
    { 3328, 29, 8, 16, 4, 0, 2, 0, 124, 256, 1, 3 }, /* opponent_boss_navy_battleship.png */
    { 3584, 30, 12, 16, 2, 0, 1, 0, 190, 256, 1, 3 }, /* opponent_boss_red_fortress.png */
    { 3840, 31, 14, 16, 1, 0, 9, 0, 205, 256, 1, 3 }, /* opponent_boss_stealth_bomber.png */
    { 4096, 32, 11, 16, 2, 0, 15, 0, 161, 256, 1, 3 }, /* opponent_boss_tank_fortress.png */
    { 4352, 33, 4, 4, 6, 12, 4, 8, 56, 56, 1, 3 }, /* opponent_drone_red.png */
    { 4608, 34, 8, 9, 4, 7, 0, 0, 128, 144, 1, 3 }, /* opponent_fighter_grayred.png */
    { 4864, 35, 8, 9, 4, 7, 8, 0, 112, 144, 1, 3 }, /* opponent_heli_gunship.png */
    { 5120, 36, 8, 9, 4, 7, 12, 0, 104, 144, 1, 3 }, /* opponent_interceptor_blackgold.png */
    { 5376, 37, 6, 10, 5, 6, 12, 0, 72, 160, 1, 3 }, /* opponent_missile_boat.png */
    { 5632, 38, 2, 2, 7, 14, 0, 0, 32, 32, 1, 3 }, /* opponent_shot_orb.png */
    { 5888, 39, 4, 3, 6, 13, 12, 8, 40, 40, 1, 3 }, /* opponent_shot_ring.png */
    { 6144, 40, 6, 8, 5, 8, 8, 8, 80, 120, 1, 3 }, /* opponent_tank_camo.png */
};

#endif
