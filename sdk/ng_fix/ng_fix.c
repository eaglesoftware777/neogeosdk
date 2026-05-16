#include "ng_fix/ng_fix.h"
#include "neogeo.h"

extern void setsfix(void);
extern void clearFix(void);

static uint16_t ngfix_cache[NGFIX_HEIGHT][NGFIX_WIDTH];

static NGFixFont ngfix_font_state = {
    0,
    NGFIX_DEFAULT_BLANK_TILE,
    NGFIX_DEFAULT_FALLBACK_TILE,
    0
};

static NGFixBoxTiles ngfix_box_state = {
    '+', '+', '+', '+', '-', '|', NGFIX_DEFAULT_BLANK_TILE
};

static uint8_t NEOGEO_USER ngfix_in_bounds(uint8_t x, uint8_t y)
{
    return (uint8_t)(x < NGFIX_WIDTH && y < NGFIX_HEIGHT);
}

static uint8_t NEOGEO_USER ngfix_strlen_clamped(const char *text, uint8_t max_chars)
{
    uint8_t n = 0;

    if (!text) return 0;

    while (text[n] && n < max_chars) n++;
    return n;
}

void NEOGEO_USER ngfix_init(void)
{
    ngfix_font_state.ascii_base = 0;
    ngfix_font_state.blank_tile = NGFIX_DEFAULT_BLANK_TILE;
    ngfix_font_state.fallback_tile = NGFIX_DEFAULT_FALLBACK_TILE;
    ngfix_font_state.uppercase_only = 0;

    ngfix_box_state.top_left = '+';
    ngfix_box_state.top_right = '+';
    ngfix_box_state.bottom_left = '+';
    ngfix_box_state.bottom_right = '+';
    ngfix_box_state.horizontal = '-';
    ngfix_box_state.vertical = '|';
    ngfix_box_state.fill = NGFIX_DEFAULT_BLANK_TILE;

    ngfix_cache_invalidate();
    setsfix();  /* switch hardware to game S ROM; BIOS boot leaves BRDFIX=0 */
}

void NEOGEO_USER ngfix_set_font(const NGFixFont *font)
{
    if (!font) return;
    ngfix_font_state = *font;
    if (!ngfix_font_state.blank_tile)
        ngfix_font_state.blank_tile = NGFIX_DEFAULT_BLANK_TILE;
    if (!ngfix_font_state.fallback_tile)
        ngfix_font_state.fallback_tile = NGFIX_DEFAULT_FALLBACK_TILE;
    ngfix_cache_invalidate();
}

const NGFixFont* NEOGEO_USER ngfix_get_font(void)
{
    return &ngfix_font_state;
}

void NEOGEO_USER ngfix_set_box_tiles(const NGFixBoxTiles *tiles)
{
    if (!tiles) return;
    ngfix_box_state = *tiles;
}

const NGFixBoxTiles* NEOGEO_USER ngfix_get_box_tiles(void)
{
    return &ngfix_box_state;
}

uint16_t NEOGEO_USER ngfix_addr(uint8_t x, uint8_t y)
{
    return (uint16_t)(FIXMAP + (uint16_t)y + ((uint16_t)x * 32u));
}

uint16_t NEOGEO_USER ngfix_cell(uint16_t tile, uint8_t pal)
{
    return (uint16_t)(((uint16_t)(pal & NGFIX_PAL_MASK) << 12) | (tile & NGFIX_TILE_MASK));
}

uint16_t NEOGEO_USER ngfix_tile_for_char(char ch)
{
    uint8_t c = (uint8_t)ch;

    if (ch == ' ') return ngfix_font_state.blank_tile;

    if (ngfix_font_state.uppercase_only && c >= 'a' && c <= 'z') {
        c = (uint8_t)(c - ('a' - 'A'));
    }

    if (c < 0x20u) return ngfix_font_state.fallback_tile;

    return (uint16_t)(ngfix_font_state.ascii_base + (uint16_t)c);
}

void NEOGEO_USER ngfix_cache_invalidate(void)
{
    uint8_t y;
    uint8_t x;

    for (y = 0; y < NGFIX_HEIGHT; y++) {
        for (x = 0; x < NGFIX_WIDTH; x++) {
            ngfix_cache[y][x] = 0xFFFFu;
        }
    }
}

void NEOGEO_USER ngfix_write_cell(uint8_t x, uint8_t y, uint16_t cell)
{
    if (!ngfix_in_bounds(x, y)) return;

    vram_sfix(0x20, ngfix_addr(x, y), cell);
    ngfix_cache[y][x] = cell;
}

void NEOGEO_USER ngfix_cache_put_cell(uint8_t x, uint8_t y, uint16_t cell)
{
    if (!ngfix_in_bounds(x, y)) return;

    if (ngfix_cache[y][x] == cell) return;
    ngfix_write_cell(x, y, cell);
}

void NEOGEO_USER ngfix_write_tile(uint8_t x, uint8_t y, uint16_t tile, uint8_t pal)
{
    ngfix_write_cell(x, y, ngfix_cell(tile, pal));
}

void NEOGEO_USER ngfix_put_tile(uint8_t x, uint8_t y, uint16_t tile, uint8_t pal)
{
    ngfix_cache_put_cell(x, y, ngfix_cell(tile, pal));
}

void NEOGEO_USER ngfix_write_char(uint8_t x, uint8_t y, char ch, uint8_t pal)
{
    ngfix_write_tile(x, y, ngfix_tile_for_char(ch), pal);
}

void NEOGEO_USER ngfix_putc(uint8_t x, uint8_t y, char ch, uint8_t pal)
{
    ngfix_put_tile(x, y, ngfix_tile_for_char(ch), pal);
}

void NEOGEO_USER ngfix_write_string(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    uint8_t cursor = x;

    if (!text || y >= NGFIX_HEIGHT || x >= NGFIX_WIDTH) return;

    while (*text && cursor < NGFIX_WIDTH) {
        ngfix_write_char(cursor, y, *text, pal);
        cursor++;
        text++;
    }
}

void NEOGEO_USER ngfix_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    uint8_t cursor = x;

    if (!text || y >= NGFIX_HEIGHT || x >= NGFIX_WIDTH) return;

    while (*text && cursor < NGFIX_WIDTH) {
        ngfix_putc(cursor, y, *text, pal);
        cursor++;
        text++;
    }
}

void NEOGEO_USER ngfix_puts_clipped(uint8_t x, uint8_t y, const char *text, uint8_t pal, uint8_t max_chars)
{
    uint8_t cursor = x;
    uint8_t count = 0;

    if (!text || y >= NGFIX_HEIGHT || x >= NGFIX_WIDTH) return;

    while (*text && cursor < NGFIX_WIDTH && count < max_chars) {
        ngfix_putc(cursor, y, *text, pal);
        cursor++;
        text++;
        count++;
    }
}

void NEOGEO_USER ngfix_blank_cell(uint8_t x, uint8_t y)
{
    ngfix_put_tile(x, y, ngfix_font_state.blank_tile, 0);
}

void NEOGEO_USER ngfix_clear_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t iy;
    uint8_t ix;

    if (x >= NGFIX_WIDTH || y >= NGFIX_HEIGHT) return;

    for (iy = 0; iy < h; iy++) {
        uint8_t py = (uint8_t)(y + iy);
        if (py >= NGFIX_HEIGHT) break;

        for (ix = 0; ix < w; ix++) {
            uint8_t px = (uint8_t)(x + ix);
            if (px >= NGFIX_WIDTH) break;
            ngfix_blank_cell(px, py);
        }
    }
}

void NEOGEO_USER ngfix_clear_line(uint8_t y)
{
    if (y >= NGFIX_HEIGHT) return;
    ngfix_clear_rect(0, y, NGFIX_WIDTH, 1);
}

void NEOGEO_USER ngfix_clear(void)
{
    clearFix();     /* BIOS call resets BRDFIX to 0; restore game S ROM after */
    setsfix();
    ngfix_cache_invalidate();
    ngfix_clear_rect(0, 0, NGFIX_WIDTH, NGFIX_HEIGHT);
}

static uint8_t NEOGEO_USER ngfix_emit_dec(uint8_t x, uint8_t y, uint32_t value, uint8_t pal, uint8_t width)
{
    static const uint32_t pow10[10] = {
        1000000000UL, 100000000UL, 10000000UL, 1000000UL, 100000UL,
        10000UL, 1000UL, 100UL, 10UL, 1UL
    };
    char digits[10];
    uint8_t digit_count = 0;
    uint8_t out_count;
    uint8_t i;
    uint8_t started = 0;

    for (i = 0; i < 10; i++) {
        uint8_t digit = 0;
        while (value >= pow10[i]) {
            value -= pow10[i];
            digit++;
        }
        if (digit || started || i == 9) {
            digits[digit_count++] = (char)('0' + digit);
            started = 1;
        }
    }

    out_count = digit_count;
    if (width > out_count) out_count = width;

    for (i = 0; i < out_count; i++) {
        uint8_t digit_pos = (uint8_t)(out_count - i);
        if ((uint8_t)(x + i) >= NGFIX_WIDTH) break;
        if (digit_pos > digit_count) ngfix_putc((uint8_t)(x + i), y, ' ', pal);
        else ngfix_putc((uint8_t)(x + i), y, digits[(uint8_t)(digit_count - digit_pos)], pal);
    }

    return out_count;
}

void NEOGEO_USER ngfix_put_u16(uint8_t x, uint8_t y, uint16_t value, uint8_t pal, uint8_t width)
{
    if (x >= NGFIX_WIDTH || y >= NGFIX_HEIGHT) return;
    ngfix_emit_dec(x, y, (uint32_t)value, pal, width);
}

void NEOGEO_USER ngfix_put_u32(uint8_t x, uint8_t y, uint32_t value, uint8_t pal, uint8_t width)
{
    if (x >= NGFIX_WIDTH || y >= NGFIX_HEIGHT) return;
    ngfix_emit_dec(x, y, value, pal, width);
}

static char NEOGEO_USER ngfix_hex_nibble(uint8_t v)
{
    v &= 0x0Fu;
    if (v < 10u) return (char)('0' + (char)v);
    return (char)('A' + (char)(v - 10u));
}

void NEOGEO_USER ngfix_put_hex16(uint8_t x, uint8_t y, uint16_t value, uint8_t pal)
{
    uint8_t i;

    if (x >= NGFIX_WIDTH || y >= NGFIX_HEIGHT) return;

    for (i = 0; i < 4; i++) {
        uint8_t shift = (uint8_t)((3u - i) * 4u);
        if ((uint8_t)(x + i) >= NGFIX_WIDTH) break;
        ngfix_putc((uint8_t)(x + i), y, ngfix_hex_nibble((uint8_t)(value >> shift)), pal);
    }
}

void NEOGEO_USER ngfix_put_hex32(uint8_t x, uint8_t y, uint32_t value, uint8_t pal)
{
    uint8_t i;

    if (x >= NGFIX_WIDTH || y >= NGFIX_HEIGHT) return;

    for (i = 0; i < 8; i++) {
        uint8_t shift = (uint8_t)((7u - i) * 4u);
        if ((uint8_t)(x + i) >= NGFIX_WIDTH) break;
        ngfix_putc((uint8_t)(x + i), y, ngfix_hex_nibble((uint8_t)(value >> shift)), pal);
    }
}

void NEOGEO_USER ngfix_center(uint8_t y, const char *text, uint8_t pal)
{
    uint8_t len;
    uint8_t x;

    if (!text || y >= NGFIX_HEIGHT) return;

    len = ngfix_strlen_clamped(text, NGFIX_WIDTH);
    x = (uint8_t)((NGFIX_WIDTH - len) / 2u);
    ngfix_puts_clipped(x, y, text, pal, len);
}

void NEOGEO_USER ngfix_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t tile, uint8_t pal)
{
    uint8_t iy;
    uint8_t ix;

    if (x >= NGFIX_WIDTH || y >= NGFIX_HEIGHT) return;

    for (iy = 0; iy < h; iy++) {
        uint8_t py = (uint8_t)(y + iy);
        if (py >= NGFIX_HEIGHT) break;

        for (ix = 0; ix < w; ix++) {
            uint8_t px = (uint8_t)(x + ix);
            if (px >= NGFIX_WIDTH) break;
            ngfix_put_tile(px, py, tile, pal);
        }
    }
}

void NEOGEO_USER ngfix_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pal)
{
    uint8_t ix;
    uint8_t iy;

    if (w < 2u || h < 2u) return;
    if (x >= NGFIX_WIDTH || y >= NGFIX_HEIGHT) return;

    ngfix_put_tile(x, y, ngfix_box_state.top_left, pal);
    ngfix_put_tile((uint8_t)(x + w - 1u), y, ngfix_box_state.top_right, pal);
    ngfix_put_tile(x, (uint8_t)(y + h - 1u), ngfix_box_state.bottom_left, pal);
    ngfix_put_tile((uint8_t)(x + w - 1u), (uint8_t)(y + h - 1u), ngfix_box_state.bottom_right, pal);

    for (ix = 1; ix < (uint8_t)(w - 1u); ix++) {
        ngfix_put_tile((uint8_t)(x + ix), y, ngfix_box_state.horizontal, pal);
        ngfix_put_tile((uint8_t)(x + ix), (uint8_t)(y + h - 1u), ngfix_box_state.horizontal, pal);
    }

    for (iy = 1; iy < (uint8_t)(h - 1u); iy++) {
        ngfix_put_tile(x, (uint8_t)(y + iy), ngfix_box_state.vertical, pal);
        ngfix_put_tile((uint8_t)(x + w - 1u), (uint8_t)(y + iy), ngfix_box_state.vertical, pal);
        if (w > 2u) {
            ngfix_fill_rect((uint8_t)(x + 1u), (uint8_t)(y + iy), (uint8_t)(w - 2u), 1, ngfix_box_state.fill, pal);
        }
    }
}

void NEOGEO_USER ngfix_bar_u8(uint8_t x, uint8_t y, uint8_t w, uint8_t value, uint8_t max, uint8_t pal, uint16_t fill_tile, uint16_t empty_tile)
{
    uint8_t filled;
    uint8_t i;

    if (x >= NGFIX_WIDTH || y >= NGFIX_HEIGHT || w == 0) return;
    if (max == 0) max = 1;
    if (value > max) value = max;

    filled = (uint8_t)(((uint16_t)value * (uint16_t)w) / (uint16_t)max);

    for (i = 0; i < w; i++) {
        if ((uint8_t)(x + i) >= NGFIX_WIDTH) break;
        ngfix_put_tile((uint8_t)(x + i), y, (i < filled) ? fill_tile : empty_tile, pal);
    }
}
