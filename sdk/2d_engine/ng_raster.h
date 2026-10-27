#ifndef NG_RASTER_H
#define NG_RASTER_H

#include "ng_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Raster bands (docs/raster.md): changes made part way down the screen, on
 * the LSPC timer interrupt (IRQ2), every 8 or 16 lines. Built only for a
 * game with -DNG_RASTER=1 in GAME_ENGINE_DEFINES; C engine only.
 *
 * Each frame the game fills a table -- for a band, up to
 * NG_RASTER_MAX_COLORS colour writes and NG_RASTER_MAX_WORDS sprite-table
 * words -- and ng_raster_vblank(), right after the frame's wait for the
 * blank, puts it in use for the frame being drawn. A band's writes land
 * where its interrupt comes, at the end of the line before the band's
 * first line (ng_raster_band_line()).
 *
 * The sprite-table words are skipped while the engine itself is writing
 * video memory (ng_vram_busy: the commit, under NG_VRAM_DEFER); a game that
 * writes VRAM directly during play must not use them.
 *
 * The game's IRQ2 handler calls ng_raster_irq(). With an empty table the
 * timer interrupt is off.
 */

#define NG_RASTER_MAX_COLORS  4
#define NG_RASTER_MAX_WORDS   2
#define NG_RASTER_BANDS       33   /* bands 1..32 of 8 lines (1..16 of 16) */

/* Bands of `lines` lines (8 or 16); 0 turns raster bands off. */
void NEOGEO_USER ng_raster_start(uint8_t lines);
void NEOGEO_USER ng_raster_stop(void);
/* The auto-animation speed the LSPC mode register keeps (0 by default). */
void NEOGEO_USER ng_raster_set_anim_speed(uint8_t speed);

/* The band whose writes are in place from screen line y (0..223) on, and
 * the screen line a band starts at (negative: in the blank above). */
uint8_t NEOGEO_USER ng_raster_band_at(int16_t y);
int16_t NEOGEO_USER ng_raster_band_line(uint8_t band);
/* The last band a frame has (on line 232): 32 for bands of 8 lines, 16 for
 * bands of 16. Bands start at 1. */
uint8_t NEOGEO_USER ng_raster_last_band(void);

/* The next frame's table: empty it, then add writes. A full band ignores
 * more. color_index is 0..4095 (palette * 16 + colour). */
void NEOGEO_USER ng_raster_clear(void);
void NEOGEO_USER ng_raster_color(uint8_t band, uint16_t color_index, uint16_t color);
void NEOGEO_USER ng_raster_vram(uint8_t band, uint16_t vram_addr, uint16_t value);

/* `count` bands from `first` on, one sprite-table word each: values[k]
 * into vram_addr for band first + k. */
void NEOGEO_USER ng_raster_vram_bands(uint8_t first, uint8_t count, uint16_t vram_addr, const uint16_t *values);

/* After the frame's wait for the blank: the table just built is the one
 * the frame uses. */
void NEOGEO_USER ng_raster_vblank(void);

/* From the game's IRQ2 handler; or point IRQ2 straight at
 * ng_raster_irq_handler (an interrupt routine itself). */
void NEOGEO_USER ng_raster_irq(void);
NEOGEO_INTERRUPT void NEOGEO_USER ng_raster_irq_handler(void);

/* Interrupts taken and bands that skipped their sprite-table words, since
 * ng_raster_start() (for measurement). */
uint16_t NEOGEO_USER ng_raster_irq_count(void);
uint16_t NEOGEO_USER ng_raster_vram_skips(void);

#ifdef __cplusplus
}
#endif

#endif
