#ifndef DEMO_H
#define DEMO_H

#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/2d_engine/ng_sprite_pool.h"

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

/* ------------------------------------------------------------------ */
/*  Sprite VRAM layout for demo scenes                                  */
/* ------------------------------------------------------------------ */
/*
 * Never use sprite_base 0 for visible generated screens.
 * VRAM 0000H-003FH is reserved for transparency/init data.
 * Slot 1 => SCB1 base 0040H.
 */
#define DEMO_SHOWSCREEN_SLOT   1u
#define DEMO_SHOWSCREEN_BASE   NG_SPR_VRAM_BASE(DEMO_SHOWSCREEN_SLOT)
#define DEMO_SHOWSCREEN_BASE2  NG_SPR_VRAM_BASE(24u)
#define DEMO_SHOWSCREEN_BASE3  NG_SPR_VRAM_BASE(48u)
#define DEMO_PRELOAD_BASE      NG_SPR_VRAM_BASE(320u)

/* Sprite tile / palette offset formulas matching screens.c palette banks */
#define DEMO_SCREEN_TILE(screen_id)    ((uint16_t)(((screen_id) - 1u) * 256u))
#define DEMO_SCREEN_PALETTE(screen_id) ((uint8_t)(0x10u + ((screen_id) - 1u)))

/* ------------------------------------------------------------------ */
/*  Shared show-screen callback type                                    */
/* ------------------------------------------------------------------ */
typedef void (*DemoShowScreenFn)(int, int, int, int, int, uint16_t, uint16_t);

/* ------------------------------------------------------------------ */
/*  Core scene utilities — implemented in demo.c                        */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_clear_scene(void);
void NEOGEO_USER demo_clear_all_sprites(void);

uint8_t NEOGEO_USER demo_advance_requested(void);
uint8_t NEOGEO_USER demo_wait(uint16_t frames);

void NEOGEO_USER demo_fix_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal);
void NEOGEO_USER demo_caption(const char *line1, const char *line2, const char *line3);

void NEOGEO_USER demo_safe_show(DemoShowScreenFn fn,
                                int x0, int y0,
                                int xr, int yr,
                                int min_crt_sz,
                                uint16_t backdrop,
                                uint16_t sprite_base);

/* Per-frame pump: waitVbl + flush queue + update subsystems + poll input.
 * Returns 1 if A-button pressed (scene should exit early). */
uint8_t NEOGEO_USER demo_frame(void);

/* Load palette for a given screen_id into hardware */
void NEOGEO_USER demo_load_screen_palette(uint8_t screen_id);

/* Draw a sprite-group screen at arbitrary position */
void NEOGEO_USER demo_draw_sprite_screen(uint8_t screen_id,
                                         uint16_t first_sprite,
                                         int16_t x, int16_t y,
                                         uint8_t strips, uint8_t rows,
                                         uint8_t scale_x, uint8_t scale_y);

/* ------------------------------------------------------------------ */
/*  Top-level demo flow — called from user.c                            */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_run_attract(void);
void NEOGEO_USER demo_run_full_flow(void);

/* Legacy entry points kept for user.c compat */
void NEOGEO_USER showEagleIntro(void);
void NEOGEO_USER showGameOver(void);
void NEOGEO_USER showTitleScreen(void);
void NEOGEO_USER showCharacterParade(void);
void NEOGEO_USER showWalkDemo(int loops, int delay_frames);

#endif /* DEMO_H */
