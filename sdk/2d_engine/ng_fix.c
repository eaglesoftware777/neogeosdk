#include "ng_fix.h"
#include "ng_fix/ng_fix.h"

void NEOGEO_USER ng_fix_blank_cell(uint8_t x, uint8_t y)
{
    ngfix_blank_cell(x, y);
}

void NEOGEO_USER ng_fix_init(void)
{
    ngfix_init();
}

void NEOGEO_USER ng_fix_invalidate_all(void)
{
    ngfix_cache_invalidate();
}

void NEOGEO_USER ng_fix_clear(void)
{
    ngfix_clear();
}

void NEOGEO_USER ng_fix_clear_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pal)
{
    NG_UNUSED(pal);
    ngfix_clear_rect(x, y, w, h);
}

void NEOGEO_USER ng_fix_putc(uint8_t x, uint8_t y, char ch, uint8_t pal)
{
    ngfix_putc(x, y, ch, pal);
}

void NEOGEO_USER ng_fix_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    ngfix_puts(x, y, text, pal);
}

void NEOGEO_USER ng_fix_put_u16(uint8_t x, uint8_t y, uint16_t value, uint8_t pal, uint16_t tile_offset)
{
    NG_UNUSED(tile_offset);
    ngfix_put_u16(x, y, value, pal, 0);
}

void NEOGEO_USER ng_fix_put_u32(uint8_t x, uint8_t y, uint32_t value, uint8_t pal, uint16_t tile_offset)
{
    NG_UNUSED(tile_offset);
    ngfix_put_u32(x, y, value, pal, 0);
}
