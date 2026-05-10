#include "ng_fix.h"
#include "neogeo.h"

static char ng_fix_chars[NG_FIX_HEIGHT][NG_FIX_WIDTH];
static uint8_t ng_fix_pals[NG_FIX_HEIGHT][NG_FIX_WIDTH];

static void ng_fix_mark_blank(uint8_t x, uint8_t y)
{
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

    for (iy = 0; iy < h; iy++) {
        for (ix = 0; ix < w; ix++) {
            ng_fix_putc((uint8_t)(x + ix), (uint8_t)(y + iy), ' ', pal);
        }
    }
}

void NEOGEO_USER ng_fix_putc(uint8_t x, uint8_t y, char ch, uint8_t pal)
{
    char buffer[2];

    if (x >= NG_FIX_WIDTH || y >= NG_FIX_HEIGHT) return;
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

    if (!text || y >= NG_FIX_HEIGHT) return;
    while (*text && cursor < NG_FIX_WIDTH) {
        ng_fix_putc(cursor, y, *text, pal);
        text++;
        cursor++;
    }
}

void NEOGEO_USER ng_fix_put_u16(uint8_t x, uint8_t y, uint16_t value, uint8_t pal, uint16_t tile_offset)
{
    display_digit(x, y, value, pal, tile_offset);
}

void NEOGEO_USER ng_fix_put_u32(uint8_t x, uint8_t y, uint32_t value, uint8_t pal, uint16_t tile_offset)
{
    display_digit(x, y, value, pal, tile_offset);
}
