#ifndef SDK_NG_FIX_NG_FIX_H
#define SDK_NG_FIX_NG_FIX_H

#include <stdint.h>
#include "macro.h"

/*
 * Neo Geo FIX SDK
 *
 * The FIX layer is the 8x8 tile plane backed by the S1 ROM.  It is intended
 * for HUDs, captions, debug text, menus, score/life readouts and small UI
 * decorations.  This module keeps the direct hardware details in one place:
 * FIXMAP addressing, palette bits, explicit blank tile use and optional cached
 * writes to avoid rewriting unchanged text every frame.
 */

#define NGFIX_WIDTH          40u
#define NGFIX_HEIGHT         32u
#define NGFIX_SAFE_X         1u
#define NGFIX_SAFE_Y         2u
#define NGFIX_SAFE_WIDTH     38u
#define NGFIX_SAFE_HEIGHT    28u

#define NGFIX_DEFAULT_BLANK_TILE    0x00FFu
#define NGFIX_DEFAULT_FALLBACK_TILE '?'

/*
 * FIX cells use a 12-bit tile number plus a 4-bit palette number.
 * Palette numbers are limited to 0..15 on the FIX layer.
 */
#define NGFIX_TILE_MASK      0x0FFFu
#define NGFIX_PAL_MASK       0x0Fu

/*
 * Font mapping from characters to S1 ROM tile numbers.
 *
 * The default mapping uses the character byte itself as the tile number, which
 * matches the existing SDK text helpers.  For custom S1 fonts, set ascii_base
 * to the first tile of the font.  Then character C maps to ascii_base + C.
 */
typedef struct {
    uint16_t ascii_base;
    uint16_t blank_tile;
    uint16_t fallback_tile;
    uint8_t uppercase_only;
} NGFixFont;

/* Tiles used by box/window drawing.  Defaults are ASCII-style tiles. */
typedef struct {
    uint16_t top_left;
    uint16_t top_right;
    uint16_t bottom_left;
    uint16_t bottom_right;
    uint16_t horizontal;
    uint16_t vertical;
    uint16_t fill;
} NGFixBoxTiles;

void NEOGEO_USER ngfix_init(void);
void NEOGEO_USER ngfix_set_font(const NGFixFont *font);
const NGFixFont* NEOGEO_USER ngfix_get_font(void);
void NEOGEO_USER ngfix_set_box_tiles(const NGFixBoxTiles *tiles);
const NGFixBoxTiles* NEOGEO_USER ngfix_get_box_tiles(void);

uint16_t NEOGEO_USER ngfix_addr(uint8_t x, uint8_t y);
uint16_t NEOGEO_USER ngfix_cell(uint16_t tile, uint8_t pal);
uint16_t NEOGEO_USER ngfix_tile_for_char(char ch);

/* Direct hardware writes.  These always write to VRAM and update the cache. */
void NEOGEO_USER ngfix_write_cell(uint8_t x, uint8_t y, uint16_t cell);
void NEOGEO_USER ngfix_write_tile(uint8_t x, uint8_t y, uint16_t tile, uint8_t pal);
void NEOGEO_USER ngfix_write_char(uint8_t x, uint8_t y, char ch, uint8_t pal);
void NEOGEO_USER ngfix_write_string(uint8_t x, uint8_t y, const char *text, uint8_t pal);

/* Cached writes.  These skip VRAM if the requested cell is already present. */
void NEOGEO_USER ngfix_cache_invalidate(void);
void NEOGEO_USER ngfix_cache_put_cell(uint8_t x, uint8_t y, uint16_t cell);
void NEOGEO_USER ngfix_put_tile(uint8_t x, uint8_t y, uint16_t tile, uint8_t pal);
void NEOGEO_USER ngfix_putc(uint8_t x, uint8_t y, char ch, uint8_t pal);
void NEOGEO_USER ngfix_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal);
void NEOGEO_USER ngfix_puts_clipped(uint8_t x, uint8_t y, const char *text, uint8_t pal, uint8_t max_chars);

/* Clear helpers use the explicit blank tile by default, not ASCII space. */
void NEOGEO_USER ngfix_blank_cell(uint8_t x, uint8_t y);
void NEOGEO_USER ngfix_clear(void);
void NEOGEO_USER ngfix_clear_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void NEOGEO_USER ngfix_clear_line(uint8_t y);

/* Numeric output.  width=0 means natural width. */
void NEOGEO_USER ngfix_put_u16(uint8_t x, uint8_t y, uint16_t value, uint8_t pal, uint8_t width);
void NEOGEO_USER ngfix_put_u32(uint8_t x, uint8_t y, uint32_t value, uint8_t pal, uint8_t width);
void NEOGEO_USER ngfix_put_hex16(uint8_t x, uint8_t y, uint16_t value, uint8_t pal);
void NEOGEO_USER ngfix_put_hex32(uint8_t x, uint8_t y, uint32_t value, uint8_t pal);

/* UI helpers. */
void NEOGEO_USER ngfix_center(uint8_t y, const char *text, uint8_t pal);
void NEOGEO_USER ngfix_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pal);
void NEOGEO_USER ngfix_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t tile, uint8_t pal);
void NEOGEO_USER ngfix_bar_u8(uint8_t x, uint8_t y, uint8_t w, uint8_t value, uint8_t max, uint8_t pal, uint16_t fill_tile, uint16_t empty_tile);

#endif
