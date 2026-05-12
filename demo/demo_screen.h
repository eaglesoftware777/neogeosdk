#ifndef DEMO_SCREEN_H
#define DEMO_SCREEN_H

#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/2d_engine/ng_sprite_pool.h"

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

typedef void (*DemoShowScreenFn)(int, int, int, int, int, uint16_t, uint16_t);

#define DEMO_SCREEN_TILE(screen_id) ((uint16_t)(((screen_id) - 1u) * 256u))
#define DEMO_SCREEN_PALETTE(screen_id) ((uint8_t)(0x10u + ((screen_id) - 1u)))
#define DEMO_SPRITE_SCREEN(sprite_id) ((uint8_t)((sprite_id) + 10u))
#define DEMO_SPRITE_TILE(sprite_id) DEMO_SCREEN_TILE(DEMO_SPRITE_SCREEN(sprite_id))
#define DEMO_SPRITE_PALETTE(sprite_id) DEMO_SCREEN_PALETTE(DEMO_SPRITE_SCREEN(sprite_id))

/*
 * Never use sprite_base 0 for visible generated screens.
 * VRAM 0000H-003FH is reserved for transparency/init data.
 * Slot 1 => SCB1 base 0040H.
 */
#define DEMO_SHOWSCREEN_SLOT 1u
#define DEMO_SHOWSCREEN_BASE NG_SPR_VRAM_BASE(DEMO_SHOWSCREEN_SLOT)
#define DEMO_SHOWSCREEN_BASE2 NG_SPR_VRAM_BASE(24u)
#define DEMO_SHOWSCREEN_BASE3 NG_SPR_VRAM_BASE(48u)
#define DEMO_PRELOAD_BASE NG_SPR_VRAM_BASE(320u)

void NEOGEO_USER demo_clear_scene(void);
void NEOGEO_USER demo_screen_showcase(void);
void NEOGEO_USER showWalkDemo(int loops, int delay_ms);
void NEOGEO_USER showTitleScreen(void);

uint8_t NEOGEO_USER demo_advance_requested(void);
uint8_t NEOGEO_USER demo_wait_frames_or_a(uint16_t frames);
void NEOGEO_USER demo_scene_caption(const char *title, const char *line1, const char *line2);
void NEOGEO_USER demo_clear_all_sprites(void);
void NEOGEO_USER demo_safe_show(DemoShowScreenFn fn, int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);

void NEOGEO_USER demo_fix_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal);
void NEOGEO_USER demo_load_screen_palette(uint8_t screen_id);
void NEOGEO_USER demo_draw_sprite_screen(uint8_t screen_id, uint16_t first_sprite, int16_t x, int16_t y, uint8_t strips, uint8_t rows, uint8_t scale_x, uint8_t scale_y);

void NEOGEO_USER showScreen1(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen2(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen3(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen4(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen5(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen6(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen7(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen8(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen9(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen10(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen11(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen12(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen13(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen14(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen15(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen16(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen17(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen18(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen19(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen20(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen21(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen22(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen23(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen24(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen25(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen26(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen27(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen28(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen29(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen30(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen31(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen32(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen33(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen34(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen35(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen36(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen37(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen38(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen39(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen40(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen41(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen42(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen43(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen44(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen45(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen46(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen47(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen48(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen49(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen50(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen51(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen52(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen53(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen54(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen55(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen56(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen57(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen58(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen59(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen60(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen61(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen62(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen63(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen64(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen65(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen66(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen67(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen68(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen69(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen70(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen71(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen72(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen73(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen74(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen75(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen76(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen77(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen78(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen79(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen80(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen81(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen82(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen83(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen84(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen85(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen86(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen87(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen88(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen89(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen90(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen91(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen92(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen93(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen94(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen95(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen96(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen97(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen98(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen99(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen100(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen101(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen102(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen103(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen104(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen105(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen106(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen107(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen108(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen109(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);

#endif
