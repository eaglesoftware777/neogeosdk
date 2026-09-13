#ifndef NG_FIX_H
#define NG_FIX_H

#include "ng_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NG_FIX_SAFE_X       1
#define NG_FIX_SAFE_Y       2
#define NG_FIX_SAFE_WIDTH   38
#define NG_FIX_SAFE_HEIGHT  28

void NEOGEO_USER ng_fix_init(void);
void NEOGEO_USER ng_fix_invalidate_all(void);
void NEOGEO_USER ng_fix_clear(void);
void NEOGEO_USER ng_fix_clear_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pal);
void NEOGEO_USER ng_fix_putc(uint8_t x, uint8_t y, char ch, uint8_t pal);
void NEOGEO_USER ng_fix_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal);
void NEOGEO_USER ng_fix_put_u16(uint8_t x, uint8_t y, uint16_t value, uint8_t pal, uint16_t tile_offset);
void NEOGEO_USER ng_fix_put_u32(uint8_t x, uint8_t y, uint32_t value, uint8_t pal, uint16_t tile_offset);

/* Direct hardware blank using tile $00FF, matching the BIOS clear pattern. */
void NEOGEO_USER ng_fix_blank_cell(uint8_t x, uint8_t y);


#ifdef __cplusplus
}
#endif
#endif
