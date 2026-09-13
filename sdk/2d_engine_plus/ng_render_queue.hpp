/*
 * ng_render_queue.h — VBlank-safe deferred write queue (Stage 3)
 *
 * Game logic MUST NOT write VRAM or palette RAM directly during gameplay.
 * Instead it posts commands here; ng_render_queue_flush() drains them
 * during or just after VBlank, when the screen is blanked and VRAM is
 * safe to write.
 *
 * All hardware writes are sequential VRAM_ADDR + VRAM_RW pairs, which
 * the 68000 can sustain at roughly 200 words / line of display time.
 * Keep the queue small and prioritised.
 */

#ifndef NG_RENDER_QUEUE_HPP
#define NG_RENDER_QUEUE_HPP

#include "ng_defs.hpp"

#ifdef __cplusplus
extern "C" {
#endif


/* Maximum commands buffered per frame.
 * Each command costs 6 bytes; 128 commands = 768 bytes in RAM.
 * Tune this if RAM is tight — 64 is safe minimum, 256 is comfortable max. */
#define NG_RQ_MAX_CMDS   128

/* Palette upload: 16 words (32 bytes) of palette data.
 * Up to 8 palette slots can be queued per frame. */
#define NG_RQ_MAX_PAL_UPLOADS   8

/* Command types */
#define NG_RQ_SPRITE_POS     0   /* write SCB3 + SCB4 for one sprite slot */
#define NG_RQ_SPRITE_TILE    1   /* write SCB1 tile+attr for one slot */
#define NG_RQ_SPRITE_PAL     2   /* change palette nibble in SCB1 attr of a slot */
#define NG_RQ_SPRITE_SHRINK  3   /* write SCB2 for one sprite slot */
#define NG_RQ_SPRITE_HIDE    4   /* zero SCB3 to hide a slot */
#define NG_RQ_SCB234         5   /* raw SCB234 write (addr + value) */

typedef struct {
    uint8_t  type;
    uint16_t slot;      /* hardware sprite slot (SCB234 address offset) */
    uint16_t a;         /* SCB3 / SCB1_tile / SCB2 / palette word / raw addr */
    uint16_t b;         /* SCB4 / SCB1_attr / raw value */
} NGRenderCmd;

typedef struct {
    uint16_t palette_addr;          /* word address in palette RAM */
    const uint16_t *data;           /* pointer to 16-entry palette */
} NGPaletteUpload;

void NEOGEO_USER ng_render_queue_init(void);

/* Post a position update for sprite slot (SCB3 + SCB4). */
void NEOGEO_USER ng_rq_sprite_pos(uint16_t slot, uint16_t scb3, uint16_t scb4);

/* Post a shrink update for sprite slot (SCB2). */
void NEOGEO_USER ng_rq_sprite_shrink(uint16_t slot, uint16_t scb2);

/* Post a hide command for sprite slot (zeroes SCB3). */
void NEOGEO_USER ng_rq_sprite_hide(uint16_t slot);

/* Post a raw SCB234 write (address is the VRAM word offset). */
void NEOGEO_USER ng_rq_scb234(uint16_t addr, uint16_t value);

/* Queue a 16-colour palette upload.
 * palette_addr: word offset into palette RAM (e.g. 0x400000 + index*32 shifted). */
void NEOGEO_USER ng_rq_palette_upload(uint16_t palette_slot, const uint16_t *data);

/* Drain the queue — call during VBlank (after waitVbl, before game logic). */
void NEOGEO_USER ng_render_queue_flush(void);

/* How many command slots are still free. */
uint8_t NEOGEO_USER ng_rq_free(void);

/* How many palette upload slots are still free. */
uint8_t NEOGEO_USER ng_rq_pal_free(void);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif