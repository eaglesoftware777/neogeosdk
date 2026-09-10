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
    /* Tiles per row of this asset's canvas.  Row n of the
     * artwork starts tile_stride tiles after row n-1, so a
     * sprite group or character bound to this asset has to be
     * given this as its tile stride.  It is not always 16: it
     * is the canvas width in tiles, and an asset imported onto
     * a narrower canvas has a narrower stride.  Assuming 16
     * reads each row from 16 tiles on instead of this many,
     * which draws whatever is at that address - the right
     * palette over the wrong art. */
    uint8_t tile_stride;
} NGSpriteAssetMeta;

#define NG_ASSET_MODE_SCREEN 0
#define NG_ASSET_MODE_SPRITE 1

#define NG_ASSET_CATEGORY_BACKGROUND 0
#define NG_ASSET_CATEGORY_MAIN_CHARACTER 1
#define NG_ASSET_CATEGORY_OPPONENT 2
#define NG_ASSET_CATEGORY_NPC 3

#define NG_ASSET_META_COUNT 25

/* Hardware sprite strips each asset needs, by 1-based asset id. */
#define NG_ASSET_STRIPS_1 16  /* background_coast_city.png */
#define NG_ASSET_STRIPS_2 16  /* background_sky_mountains.png */
#define NG_ASSET_STRIPS_3 10  /* sprite_p1_pilot.png */
#define NG_ASSET_STRIPS_4 2  /* sprite_p1_plane.png */
#define NG_ASSET_STRIPS_5 10  /* sprite_p2_pilot.png */
#define NG_ASSET_STRIPS_6 2  /* sprite_p2_plane.png */
#define NG_ASSET_STRIPS_7 10  /* sprite_p3_pilot.png */
#define NG_ASSET_STRIPS_8 2  /* sprite_p3_plane.png */
#define NG_ASSET_STRIPS_9 2  /* sprite_shot_player.png */
#define NG_ASSET_STRIPS_10 2  /* opponent_bomber_olive.png */
#define NG_ASSET_STRIPS_11 5  /* opponent_boss_crimson_cathedral.png */
#define NG_ASSET_STRIPS_12 7  /* opponent_boss_gold_core.png */
#define NG_ASSET_STRIPS_13 7  /* opponent_boss_heli_carrier.png */
#define NG_ASSET_STRIPS_14 5  /* opponent_boss_navy_battleship.png */
#define NG_ASSET_STRIPS_15 7  /* opponent_boss_red_fortress.png */
#define NG_ASSET_STRIPS_16 7  /* opponent_boss_stealth_bomber.png */
#define NG_ASSET_STRIPS_17 5  /* opponent_boss_tank_fortress.png */
#define NG_ASSET_STRIPS_18 2  /* opponent_drone_red.png */
#define NG_ASSET_STRIPS_19 2  /* opponent_fighter_grayred.png */
#define NG_ASSET_STRIPS_20 2  /* opponent_heli_gunship.png */
#define NG_ASSET_STRIPS_21 2  /* opponent_interceptor_blackgold.png */
#define NG_ASSET_STRIPS_22 2  /* opponent_missile_boat.png */
#define NG_ASSET_STRIPS_23 2  /* opponent_shot_orb.png */
#define NG_ASSET_STRIPS_24 2  /* opponent_shot_ring.png */
#define NG_ASSET_STRIPS_25 2  /* opponent_tank_camo.png */

static const NGSpriteAssetMeta g_ng_asset_meta[NG_ASSET_META_COUNT] = {
    { 0, 16, 16, 16, 0, 0, 0, 0, 256, 256, 0, 0, 16 }, /* background_coast_city.png */
    { 256, 17, 16, 16, 0, 0, 0, 0, 256, 256, 0, 0, 16 }, /* background_sky_mountains.png */
    { 512, 18, 10, 15, 3, 1, 0, 0, 160, 240, 1, 0, 16 }, /* sprite_p1_pilot.png */
    { 768, 18, 2, 2, 0, 0, 5, 0, 22, 32, 1, 0, 2 }, /* sprite_p1_plane.png */
    { 1024, 18, 10, 15, 3, 1, 0, 0, 160, 240, 1, 0, 16 }, /* sprite_p2_pilot.png */
    { 1280, 18, 2, 2, 0, 0, 3, 0, 25, 32, 1, 0, 2 }, /* sprite_p2_plane.png */
    { 1536, 18, 10, 15, 3, 1, 0, 0, 160, 240, 1, 0, 16 }, /* sprite_p3_pilot.png */
    { 1792, 18, 2, 2, 0, 0, 3, 0, 25, 32, 1, 0, 2 }, /* sprite_p3_plane.png */
    { 2048, 18, 2, 3, 7, 13, 12, 8, 8, 40, 1, 0, 16 }, /* sprite_shot_player.png */
    { 2304, 19, 2, 2, 0, 0, 1, 0, 29, 32, 1, 3, 2 }, /* opponent_bomber_olive.png */
    { 2560, 20, 5, 7, 1, 0, 8, 0, 64, 112, 1, 3, 7 }, /* opponent_boss_crimson_cathedral.png */
    { 2816, 21, 7, 7, 0, 0, 0, 0, 112, 112, 1, 3, 7 }, /* opponent_boss_gold_core.png */
    { 3072, 22, 7, 7, 0, 0, 13, 0, 86, 112, 1, 3, 7 }, /* opponent_boss_heli_carrier.png */
    { 3328, 23, 5, 7, 1, 0, 13, 0, 54, 112, 1, 3, 7 }, /* opponent_boss_navy_battleship.png */
    { 3584, 24, 7, 7, 0, 0, 14, 0, 83, 112, 1, 3, 7 }, /* opponent_boss_red_fortress.png */
    { 3840, 25, 7, 7, 0, 0, 11, 0, 90, 112, 1, 3, 7 }, /* opponent_boss_stealth_bomber.png */
    { 4096, 26, 5, 7, 1, 0, 5, 0, 70, 112, 1, 3, 7 }, /* opponent_boss_tank_fortress.png */
    { 4352, 27, 2, 2, 0, 0, 0, 0, 32, 32, 1, 3, 2 }, /* opponent_drone_red.png */
    { 4608, 28, 2, 2, 0, 0, 2, 0, 28, 32, 1, 3, 2 }, /* opponent_fighter_grayred.png */
    { 4864, 29, 2, 2, 0, 0, 3, 0, 25, 32, 1, 3, 2 }, /* opponent_heli_gunship.png */
    { 5120, 30, 2, 2, 0, 0, 4, 0, 23, 32, 1, 3, 2 }, /* opponent_interceptor_blackgold.png */
    { 5376, 31, 2, 2, 0, 0, 9, 0, 14, 32, 1, 3, 2 }, /* opponent_missile_boat.png */
    { 5632, 32, 2, 2, 0, 0, 0, 0, 32, 32, 1, 3, 2 }, /* opponent_shot_orb.png */
    { 5888, 33, 2, 2, 0, 0, 0, 0, 32, 32, 1, 3, 2 }, /* opponent_shot_ring.png */
    { 6144, 34, 2, 2, 0, 0, 5, 0, 21, 32, 1, 3, 2 }, /* opponent_tank_camo.png */
};

#endif
