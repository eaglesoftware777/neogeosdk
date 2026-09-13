#include "ng_fix.hpp"
#include "ng_hw.hpp"

extern "C" {


static char ng_fix_chars[NG_FIX_HEIGHT][NG_FIX_WIDTH];
static uint8_t ng_fix_pals[NG_FIX_HEIGHT][NG_FIX_WIDTH];

void NEOGEO_USER ng_fix_blank_cell(uint8_t x, uint8_t y)
{
    uint16_t addrfix;

    if (x >= NG_FIX_WIDTH || y >= NG_FIX_HEIGHT) return;

    /*
     * The FIX layer is transparent at color index 0, but ASCII ' ' is still
     * a real tile number.  Some S-ROMs have visible pixels in that tile.
     * Use tile $00FF for an explicit blank cell, as BIOS clear routines do.
     */
    addrfix = (uint16_t)(FIXMAP + y + ((uint16_t)x * 32u));
    vram_sfix(0x20, addrfix, 0x00FF);

    ng_fix_chars[y][x] = ' ';
    ng_fix_pals[y][x] = 0;
}

static void NEOGEO_USER ng_fix_mark_blank(uint8_t x, uint8_t y)
{
    if (x >= NG_FIX_WIDTH || y >= NG_FIX_HEIGHT) return;

    ng_fix_chars[y][x] = ' ';
    ng_fix_pals[y][x] = 0;
}

void NEOGEO_USER ng_fix_init(void)
{
    ng_fix_invalidate_all();
}

void NEOGEO_USER ng_fix_invalidate_all(void)
{
    uint8_t y;
    uint8_t x;

    for (y = 0; y < NG_FIX_HEIGHT; y++) {
        for (x = 0; x < NG_FIX_WIDTH; x++) {
            ng_fix_chars[y][x] = '\0';
            ng_fix_pals[y][x] = 0xff;
        }
    }
}

void NEOGEO_USER ng_fix_clear(void)
{
    clearFix();

    {
        uint8_t y;
        uint8_t x;

        for (y = 0; y < NG_FIX_HEIGHT; y++) {
            for (x = 0; x < NG_FIX_WIDTH; x++) {
                ng_fix_mark_blank(x, y);
            }
        }
    }
}

void NEOGEO_USER ng_fix_clear_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pal)
{
    uint8_t iy;
    uint8_t ix;

    NG_UNUSED(pal);

    if (x >= NG_FIX_WIDTH || y >= NG_FIX_HEIGHT) return;

    for (iy = 0; iy < h; iy++) {
        uint8_t py = (uint8_t)(y + iy);
        if (py >= NG_FIX_HEIGHT) break;

        for (ix = 0; ix < w; ix++) {
            uint8_t px = (uint8_t)(x + ix);
            if (px >= NG_FIX_WIDTH) break;
            ng_fix_blank_cell(px, py);
        }
    }
}

void NEOGEO_USER ng_fix_putc(uint8_t x, uint8_t y, char ch, uint8_t pal)
{
    char buffer[2];

    if (x >= NG_FIX_WIDTH || y >= NG_FIX_HEIGHT) return;

    /*
     * FIX can only address the first 16 palettes.
     */
    pal &= 0x0f;

    if (ch == ' ') {
        if (ng_fix_chars[y][x] != ' ' || ng_fix_pals[y][x] != 0) {
            ng_fix_blank_cell(x, y);
        }
        return;
    }

    if (ng_fix_chars[y][x] == ch && ng_fix_pals[y][x] == pal) return;

    buffer[0] = ch;
    buffer[1] = '\0';

    fixtext_out(x, y, buffer, pal);

    ng_fix_chars[y][x] = ch;
    ng_fix_pals[y][x] = pal;
}

void NEOGEO_USER ng_fix_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    uint8_t cursor = x;

    if (!text || y >= NG_FIX_HEIGHT || x >= NG_FIX_WIDTH) return;

    while (*text && cursor < NG_FIX_WIDTH) {
        ng_fix_putc(cursor, y, *text, pal);
        text++;
        cursor++;
    }
}

static void NEOGEO_USER ng_fix_put_digit(uint8_t x, uint8_t y, uint8_t digit, uint8_t pal)
{
    ng_fix_putc(x, y, (char)('0' + digit), pal);
}

static uint8_t NEOGEO_USER ng_fix_emit_dec32(uint8_t x, uint8_t y, uint32_t value, uint8_t pal)
{
    static const uint32_t pow10[10] = {
        1000000000UL, 100000000UL, 10000000UL, 1000000UL, 100000UL,
        10000UL, 1000UL, 100UL, 10UL, 1UL
    };

    uint8_t i;
    uint8_t started = 0;
    uint8_t cursor = x;

    for (i = 0; i < 10; i++) {
        uint8_t digit = 0;

        while (value >= pow10[i]) {
            value -= pow10[i];
            digit++;
        }

        if (digit || started || i == 9) {
            if (cursor >= NG_FIX_WIDTH) break;
            ng_fix_put_digit(cursor, y, digit, pal);
            cursor++;
            started = 1;
        }
    }

    return (uint8_t)(cursor - x);
}

void NEOGEO_USER ng_fix_put_u16(uint8_t x, uint8_t y, uint16_t value, uint8_t pal, uint16_t tile_offset)
{
    uint8_t used;
    uint8_t i;

    NG_UNUSED(tile_offset);

    if (x >= NG_FIX_WIDTH || y >= NG_FIX_HEIGHT) return;

    used = ng_fix_emit_dec32(x, y, (uint32_t)value, pal);

    for (i = used; i < 5; i++) {
        if ((uint8_t)(x + i) >= NG_FIX_WIDTH) break;
        ng_fix_blank_cell((uint8_t)(x + i), y);
    }
}

void NEOGEO_USER ng_fix_put_u32(uint8_t x, uint8_t y, uint32_t value, uint8_t pal, uint16_t tile_offset)
{
    uint8_t used;
    uint8_t i;

    NG_UNUSED(tile_offset);

    if (x >= NG_FIX_WIDTH || y >= NG_FIX_HEIGHT) return;

    used = ng_fix_emit_dec32(x, y, value, pal);

    for (i = used; i < 10; i++) {
        if ((uint8_t)(x + i) >= NG_FIX_WIDTH) break;
        ng_fix_blank_cell((uint8_t)(x + i), y);
    }
}


} /* extern "C" */
