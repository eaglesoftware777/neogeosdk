#ifndef SKY_DRAW_H
#define SKY_DRAW_H

/* Resident critical sprites occupy the upper character-pool slots.
 * Leave96..149 for scenery, pickups and optional effects. */
#define SKY_FIXED_ENEMY_FIRST   150u
#define SKY_FIXED_BOSS_FIRST    182u
#define SKY_FIXED_FACE_FIRST    190u
#define SKY_FIXED_PLAYER_FIRST  198u
#define SKY_FIXED_PSHOT_FIRST   206u
#define SKY_FIXED_ESHOT_FIRST   216u

#include <stdint.h>
#include "sky.h"
#include "sdk/2d_engine/ng_chars.h"

/* ------------------------------------------------------------------ */
/*  Assets                                                              */
/* ------------------------------------------------------------------ */
/*
 * Bind an artbox asset to a character and anchor it on its CENTRE.
 *
 * Two things make this more than a tile_base assignment.  A page is
 * always 16x16 tiles with the real artwork parked somewhere inside it,
 * so the group has to start at the artwork's first tile
 * (tile_base + tile_row_start*16 + tile_col_start) rather than at the
 * page origin.  And the leftover sub-tile padding lives INSIDE the
 * sprite, so it shrinks with the SCB2 scale while the tile-grid part
 * does not - the centre offset therefore has to be computed at the
 * scale the object will actually be drawn at.
 */
void NEOGEO_USER sky_bind(NGCharacter *c, uint16_t id, uint8_t scale, uint8_t band);
void NEOGEO_USER sky_damage_color(const NGCharacter *c, uint8_t critical);
void NEOGEO_USER sky_damage_flash(const NGCharacter *c, uint8_t flash);
void NEOGEO_USER sky_hud_pilot_palette(const NGCharacter *c, uint8_t pilot_idx, uint8_t situation);
void NEOGEO_USER sky_meter(uint8_t x, uint8_t y, uint8_t width,
                           uint8_t filled, uint8_t palette);

/* Painted size of an asset once drawn at `scale`, for hitboxes/spacing. */
int16_t NEOGEO_USER sky_width(uint16_t id, uint8_t scale);
int16_t NEOGEO_USER sky_height(uint16_t id, uint8_t scale);

/* Spawn a character already bound, centred and visible.  0 when full. */
NGCharacter * NEOGEO_USER sky_spawn(uint8_t kind, uint16_t id,
                                    int16_t cx, int16_t cy,
                                    uint8_t scale, uint8_t band);

/* ------------------------------------------------------------------ */
/*  Shooter terrain/cloud layers (slots 1..88, characters start at 96) */
/* ------------------------------------------------------------------ */
void NEOGEO_USER sky_bg_select(uint8_t id);
void NEOGEO_USER sky_bg_advance(uint8_t pixels);
uint16_t NEOGEO_USER sky_bg_travel(void);
uint16_t NEOGEO_USER sky_bg_travel_end(void);
uint8_t NEOGEO_USER sky_bg_water(int16_t x, int16_t y);
void NEOGEO_USER sky_bg_follow(int16_t x, int16_t y);
int16_t NEOGEO_USER sky_bg_camera_x(void);
void NEOGEO_USER sky_depth_draw(void);
void NEOGEO_USER sky_bg_draw(void);
void NEOGEO_USER sky_bg_hide(void);
/* Cockpit instrument k (0 attitude, 1 radar, 2 G meter) showing art id,
 * its top-left corner at (x, y); a repeat of the same id does nothing. */
void NEOGEO_USER sky_instrument_show(uint8_t k, uint16_t id, int16_t x, int16_t y);
void NEOGEO_USER sky_instruments_hide(void);
void NEOGEO_USER sky_title_draw(void);
void NEOGEO_USER sky_presentation_init(void);
void NEOGEO_USER sky_palette_vblank(void);
void NEOGEO_USER sky_fade_out(void);

/* ------------------------------------------------------------------ */
/*  Frame pump                                                          */
/* ------------------------------------------------------------------ */
void NEOGEO_USER sky_scene_begin(void);
/* waitVbl + input sample.  Returns the frame counter. */
uint16_t NEOGEO_USER sky_frame(void);
uint16_t NEOGEO_USER sky_joy(void);
uint16_t NEOGEO_USER sky_joy_pressed(void);
uint16_t NEOGEO_USER sky_rand(void);

/* ------------------------------------------------------------------ */
/*  FIX layer                                                           */
/* ------------------------------------------------------------------ */
void NEOGEO_USER sky_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal);
void NEOGEO_USER sky_text_box(uint8_t x, uint8_t y, uint8_t width,
                             uint8_t rows, const char *text, uint8_t pal);
/*
 * Blank `cells` FIX cells.
 *
 * NOT the same as printing spaces: the BIOS font draws its space glyph
 * as an OPAQUE colour-2 field, so " " punches a solid plate through the
 * artwork behind it.  Tile 0xFF is the genuinely transparent one.
 */
void NEOGEO_USER sky_fix_blank(uint8_t x, uint8_t y, uint8_t cells);
/*
 * Load the artbox infix palettes into FIX banks and record where each
 * image landed.  Must run once, from setup_fix_palettes(), before any
 * sky_infix() call.
 */
void NEOGEO_USER sky_fix_palettes_init(void);
void NEOGEO_USER sky_infix(uint8_t x, uint8_t y, uint8_t infix_index);
void NEOGEO_USER sky_infix_number(uint8_t x, uint8_t y, uint32_t value, uint8_t digits);

/* infix_palettes.h indices, in the order fixtiles.py queued the PNGs. */
#define SKY_INFIX_1P        0u
#define SKY_INFIX_2P        1u
#define SKY_INFIX_3P        2u
#define SKY_INFIX_HI        3u
#define SKY_INFIX_SCORE     4u
#define SKY_INFIX_DIGIT0    5u
#define SKY_INFIX_ENERGY    15u
#define SKY_INFIX_ENERGY_MT 16u
#define SKY_INFIX_LIFE      17u

#endif /* SKY_DRAW_H */
