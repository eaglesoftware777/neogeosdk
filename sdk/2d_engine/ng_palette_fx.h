/*
 * ng_palette_fx.h — Palette effects system (Stage 7)
 *
 * NeoGeo palette RAM layout:
 *   0x400000 + slot * 32:  16 colours × 2 bytes = 32 bytes per slot.
 *   16 slots per bank, 2 banks.  Sprites use palettes 0..15 in bank 0.
 *   Each colour word: 0xRGB in 1-5-5-5 format with bit 15 as dark flag.
 *
 * NeoGeo colour word format:
 *   bit 15 = dark (halves all channels)
 *   bits 14-10 = red (5 bits)
 *   bits  9- 5 = green (5 bits)
 *   bits  4- 0 = blue (5 bits)
 *   Special: value 0x8000 = transparent (colour index 0 is transparent).
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

#endif
