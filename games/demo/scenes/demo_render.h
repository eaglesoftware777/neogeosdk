#ifndef DEMO_RENDER_H
#define DEMO_RENDER_H

#include "sdk/macro.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

void NEOGEO_USER demo_render_init(void);
void NEOGEO_USER demo_render_hbar(uint8_t x, uint8_t y, uint8_t len, uint8_t pal);
void NEOGEO_USER demo_render_vbar(uint8_t x, uint8_t y, uint8_t h, uint8_t pal);
void NEOGEO_USER demo_render_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pal);
void NEOGEO_USER demo_render_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pal);
void NEOGEO_USER demo_render_clear(void);
void NEOGEO_USER demo_render_run(void);


#ifdef __cplusplus
}
#endif
#endif /* DEMO_RENDER_H */
