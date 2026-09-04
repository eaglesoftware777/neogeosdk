#ifndef DEMO_H
#define DEMO_H

#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/2d_engine/ng_sprite_pool.h"

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

/* ------------------------------------------------------------------ */
/*  Screen background                                                   */
/* ------------------------------------------------------------------ */
/*
 * The backdrop register is the last word of palette RAM: it is what the
 * CRT shows wherever no sprite and no opaque FIX pixel is drawn, so it
 * is the demo's page colour.
 *
 * Transparent FIX and sprite pixels expose the backdrop.  Keep the
 * transition backdrop identical to the scene backdrop so a teardown
 * cannot flash a black frame between two white-backed chapters.
 */
#define DEMO_BG_CLEAR    WHITE
#define DEMO_BG          WHITE

/*
 * FIX text inks, one colour per FIX palette bank.  They have to be dark
 * because they are read against DEMO_BG - the light inks this demo used
 * on a black page (WHITE / CYAN / GREEN) are unreadable on a white one.
 * Bank layout matches setup_fix_palettes() in games/demo/user.c:
 *   0 = body text, 1 = subtitle/secondary, 2 = accent, 3 = prompts.
 */
#define DEMO_INK_BODY    BLACK
#define DEMO_INK_SUB     BLUE
#define DEMO_INK_ACCENT  MIDGREEN
#define DEMO_INK_PROMPT  RED

/*
 * A whole 16-entry FIX text palette from one ink colour.
 *
 * Entry 0 is never drawn (pixel value 0 is hardwired transparent on the
 * FIX layer), 1 is the glyph body and 3 a handful of accent pixels, so
 * both get the ink.  Everything else - entry 2 above all - is painted in
 * the page colour: the BIOS font draws each glyph, and the whole of its
 * space character, as an OPAQUE entry-2 field, so an entry 2 that does
 * not match the backdrop puts a visible box behind every character.
 */
#define DEMO_FIX_PAL(ink)  0x8000u, (ink), DEMO_BG, (ink), \
                           DEMO_BG, DEMO_BG, DEMO_BG, DEMO_BG, \
                           DEMO_BG, DEMO_BG, DEMO_BG, DEMO_BG, \
                           DEMO_BG, DEMO_BG, DEMO_BG, DEMO_BG

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

/* Metadata-backed tile/palette helpers for generated artbox assets. */
uint16_t NEOGEO_USER demo_screen_tile(uint8_t screen_id);
uint8_t  NEOGEO_USER demo_screen_palette(uint8_t screen_id);
uint8_t  NEOGEO_USER demo_screen_strips(uint8_t screen_id);
uint8_t  NEOGEO_USER demo_screen_rows(uint8_t screen_id);
int16_t  NEOGEO_USER demo_screen_x_offset(uint8_t screen_id);
int16_t  NEOGEO_USER demo_screen_y_offset(uint8_t screen_id);
uint8_t  NEOGEO_USER demo_screen_x_pad(uint8_t screen_id);
uint8_t  NEOGEO_USER demo_screen_y_pad(uint8_t screen_id);
uint16_t NEOGEO_USER demo_screen_content_width(uint8_t screen_id);
uint16_t NEOGEO_USER demo_screen_content_height(uint8_t screen_id);

/*
 * Stable bottom-center anchor for an animated sprite asset.
 *
 * The per-frame artbox meta varies wildly across an animation cycle
 * (strips 7..9, rows 7..10, tile_col_start 3..4, tile_row_start 6..9,
 *  x_pad 5..15, y_pad 2..15, content_width/height by 30+px).
 *
 * Old centering used grid_w/2 = strips*16/2, which shifted the visible
 * content by up to 8 pixels each animation frame.  That jitter was the
 * "split / old sprite still showing" the demo chapters were exhibiting.
 *
 * This helper computes a draw_x / draw_y such that, after the engine
 * adds back demo_screen_x_offset/demo_screen_y_offset and the hardware
 * applies SCB2 shrink, the actual artwork's bottom-center sits at the
 * requested (cx, cy).  Works across frames because it uses the per-
 * frame content metrics (x_pad, content_width, y_pad, content_height)
 * which DO describe each frame's real artwork position.
 */
void NEOGEO_USER demo_anchor_bottom_center(uint8_t screen_id,
                                           uint8_t scale_x,
                                           uint8_t scale_y,
                                           int16_t cx,
                                           int16_t cy,
                                           int16_t *out_x,
                                           int16_t *out_y);

#define DEMO_SCREEN_TILE(screen_id)    demo_screen_tile((uint8_t)(screen_id))
#define DEMO_SCREEN_PALETTE(screen_id) demo_screen_palette((uint8_t)(screen_id))

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

/* Draw a sprite-group screen at arbitrary position.  Queues the
 * upload; the actual SCB writes happen when demo_flush_sprite_queue
 * is called (from uframe inside vblank). */
void NEOGEO_USER demo_draw_sprite_screen(uint8_t screen_id,
                                         uint16_t first_sprite,
                                         int16_t x, int16_t y,
                                         uint8_t strips, uint8_t rows,
                                         uint8_t scale_x, uint8_t scale_y);
void NEOGEO_USER demo_draw_sprite_screen_flip(uint8_t screen_id,
                                              uint16_t first_sprite,
                                              int16_t x, int16_t y,
                                              uint8_t strips, uint8_t rows,
                                              uint8_t scale_x, uint8_t scale_y,
                                              uint8_t hflip);

/* Drain the deferred sprite-draw queue.  Call inside the vblank
 * window (right after waitVbl, before ng_render_queue_flush).
 * Without this, queued draws never reach hardware. */
void NEOGEO_USER demo_flush_sprite_queue(void);

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
