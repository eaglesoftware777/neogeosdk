/*
 * ng_palette_fx.h — Palette effects system (Stage 7)
 *
 * NeoGeo palette RAM layout:
 *   0x400000 + slot * 32:  16 colours × 2 bytes = 32 bytes per slot.
 *   256 slots per bank, 2 banks. Sprites select palettes 0..255.
 *
 * NeoGeo colour word format:
 *   bit 15 = common dark bit; bits 14/13/12 = R/G/B low bits.
 *   bits 11..8, 7..4, 3..0 = R/G/B upper four bits.
 *   Only pixel index 0 is transparent, regardless of its color word.
 *
 * Safety: No direct palette RAM writes during game logic.
 * All writes are posted to ng_render_queue palettes → flushed at VBlank.
 * During active fades/flashes the modified palette is rebuilt in RAM and
 * uploaded as a batch via ng_rq_palette_upload().
 */

#ifndef NG_PALETTE_FX_H
#define NG_PALETTE_FX_H

#include "ng_defs.h"
#include "ng_render_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of palette slots that can have active FX simultaneously.
 * Each active slot costs 32 bytes (one 16-colour palette) + 4 bytes state. */
#define NG_PALFX_MAX_SLOTS   8

/* FX types */
#define NG_PALFX_NONE        0
#define NG_PALFX_FADE_IN     1   /* fade from black to base palette */
#define NG_PALFX_FADE_OUT    2   /* fade from base palette to black */
#define NG_PALFX_FLASH_WHITE 3   /* white flash, decays to normal */
#define NG_PALFX_FLASH_RED   4   /* red tint flash (damage indicator) */
#define NG_PALFX_FLASH_BLUE  5   /* blue tint flash (magic/shield) */
#define NG_PALFX_PULSE       6   /* brightness pulse (looping) */
#define NG_PALFX_CYCLE       7   /* rotate palette entries 1..N */

typedef struct {
    uint8_t  active;
    uint8_t  palette_slot;   /* hardware palette slot 0..63 */
    uint8_t  fx_type;
    uint8_t  timer;          /* frames remaining / phase counter */
    uint8_t  duration;       /* total frames for the effect */
    uint8_t  cycle_start;    /* first colour index for CYCLE effect */
    uint8_t  cycle_end;      /* last colour index for CYCLE effect */
    uint8_t  cycle_pos;      /* current rotation offset */
    const uint16_t *base_pal; /* pointer to source 16-colour palette in ROM/RAM */
    uint16_t work_pal[16];    /* modified palette built each frame */
} NGPalFxSlot;

typedef struct {
    uint16_t asset_id;
    uint8_t palette_slot;
    uint16_t colors[16];
} NGPaletteAsset;

void NEOGEO_USER ng_palette_fx_init(void);
void NEOGEO_USER ng_palette_fx_update(void);
void NEOGEO_USER ng_palette_load_bank(uint8_t palette_slot, const uint16_t *pal);
uint8_t NEOGEO_USER ng_palette_load_asset(const NGPaletteAsset *assets,
                                          uint16_t count,
                                          uint16_t asset_id);

/* Upload base palette to hardware immediately (safe to call at scene init). */
void NEOGEO_USER ng_palfx_upload_base(uint8_t palette_slot, const uint16_t *pal);

/* Start a fade in from black.  duration in frames. */
void NEOGEO_USER ng_palfx_fade_in(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);

/* Start a fade out to black.  duration in frames. */
void NEOGEO_USER ng_palfx_fade_out(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);

/* White flash.  duration in frames (should be short: 4..16). */
void NEOGEO_USER ng_palfx_flash_white(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);

/* Red damage flash. */
void NEOGEO_USER ng_palfx_flash_red(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);

/* Blue/magic flash. */
void NEOGEO_USER ng_palfx_flash_blue(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);

/* Brightness pulse (looping).  duration = period in frames. */
void NEOGEO_USER ng_palfx_pulse(uint8_t palette_slot, const uint16_t *base_pal, uint8_t period);

/* Colour cycle: rotate entries [start..end] each frame. */
void NEOGEO_USER ng_palfx_cycle(uint8_t palette_slot, const uint16_t *base_pal,
                                  uint8_t start, uint8_t end);

/* Stop any active effect on this slot and restore base palette. */
void NEOGEO_USER ng_palfx_stop(uint8_t palette_slot);

/* Query: is any effect active on this slot? */
uint8_t NEOGEO_USER ng_palfx_active(uint8_t palette_slot);

#if NG_PALFX_SCREEN
/*
 * The screen: every palette bank a game uses, faded as one.
 *
 * Built in only for a game whose game.mk sets
 *   GAME_ENGINE_DEFINES = -DNG_PALFX_SCREEN=1
 * (C engine). The game lends two buffers of count * 16 words, for banks
 * 0 .. count-1: the colours as loaded and the colours to show. From then
 * on it loads those banks with ng_palfx_screen_load() instead of writing
 * palette RAM, the effects above on those banks land in the same place,
 * and the lot reaches palette RAM in one piece from ng_palfx_vblank(),
 * called first thing after each vertical blank begins.
 *
 * A fade takes every bank toward white or black together: level 0 is the
 * true colours, 16 all white (or black). A bank loaded while the screen is
 * faded comes up at the fade's level, so a scene set up behind a white-out
 * rises out of it in one piece. While faded, the dark bit is dropped.
 * The fade moves once per ng_palette_fx_update(), which the engine frame
 * runs; a game looping on its own calls it once a frame itself.
 */
#define NG_PALFX_BLACK   0u
#define NG_PALFX_WHITE   1u

/* Take over banks 0 .. count-1, starting from the colours now on screen. */
void NEOGEO_USER ng_palfx_screen_init(uint16_t *base, uint16_t *out, uint8_t count);
/* Load a bank (0 .. count-1); other banks are ignored. */
void NEOGEO_USER ng_palfx_screen_load(uint8_t bank, const uint16_t *colors);
/* A bank's colours as last loaded: a base for the effects above. */
const uint16_t * NEOGEO_USER ng_palfx_screen_colors(uint8_t bank);
/* The backdrop colour, written at the next ng_palfx_vblank(); not faded. */
void NEOGEO_USER ng_palfx_screen_backdrop(uint16_t color);
/* From the true colours to all `target` over `duration` frames; it stays there. */
void NEOGEO_USER ng_palfx_screen_fade_out(uint8_t target, uint8_t duration);
/* From all `target` (at once) back to the true colours over `duration` frames. */
void NEOGEO_USER ng_palfx_screen_fade_in(uint8_t target, uint8_t duration);
/* True colours back now, any fade dropped. */
void NEOGEO_USER ng_palfx_screen_stop(void);
/* 1 while a fade is still moving. */
uint8_t NEOGEO_USER ng_palfx_screen_fading(void);
/* In the vertical blank: put what changed on screen. */
void NEOGEO_USER ng_palfx_vblank(void);
#endif


#ifdef __cplusplus
}
#endif
#endif
