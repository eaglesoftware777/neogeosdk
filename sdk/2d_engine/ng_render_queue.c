/*
 * ng_render_queue.c — VBlank-safe deferred VRAM / palette write queue.
 *
 * NeoGeo hardware notes:
 *   - VRAM writes can corrupt display if done mid-scanline outside blanking.
 *   - Safest window: during VBlank (16 lines) or during active display if the
 *     sprite is fully off-screen.  We treat VBlank as the only safe window.
 *   - Palette RAM writes are safe any time (palette is latched at H-blank), but
 *     visible flickering occurs if the write races with sprite rendering.
 *     Flushing palettes in VBlank avoids this.
 *   - The queue is a simple flat array; no malloc, no linked list.
 *   - Commands are applied in insertion order to preserve priorities.
 */

#include "ng_render_queue.h"
#include "macro.h"
#include "neogeo.h"

static NGRenderCmd  ng_rq_cmds[NG_RQ_MAX_CMDS];
static uint8_t      ng_rq_count;

static NGPaletteUpload ng_rq_palettes[NG_RQ_MAX_PAL_UPLOADS];
static uint8_t         ng_rq_pal_count;

void NEOGEO_USER ng_render_queue_init(void)
{
    ng_rq_count     = 0;
    ng_rq_pal_count = 0;
}

static NGRenderCmd * NEOGEO_USER ng_rq_alloc(void)
{
    if (ng_rq_count >= NG_RQ_MAX_CMDS) return 0;
    return &ng_rq_cmds[ng_rq_count++];
}

void NEOGEO_USER ng_rq_sprite_pos(uint16_t slot, uint16_t scb3, uint16_t scb4)
{
    NGRenderCmd *cmd = ng_rq_alloc();
    if (!cmd) return;
    cmd->type = NG_RQ_SPRITE_POS;
    cmd->slot = slot;
    cmd->a    = scb3;
    cmd->b    = scb4;
}

void NEOGEO_USER ng_rq_sprite_shrink(uint16_t slot, uint16_t scb2)
{
    NGRenderCmd *cmd = ng_rq_alloc();
    if (!cmd) return;
    cmd->type = NG_RQ_SPRITE_SHRINK;
    cmd->slot = slot;
    cmd->a    = scb2;
    cmd->b    = 0;
}

void NEOGEO_USER ng_rq_sprite_hide(uint16_t slot)
{
    NGRenderCmd *cmd = ng_rq_alloc();
    if (!cmd) return;
    cmd->type = NG_RQ_SPRITE_HIDE;
    cmd->slot = slot;
    cmd->a    = 0;
    cmd->b    = 0;
}

void NEOGEO_USER ng_rq_scb234(uint16_t addr, uint16_t value)
{
    NGRenderCmd *cmd = ng_rq_alloc();
    if (!cmd) return;
    cmd->type = NG_RQ_SCB234;
    cmd->slot = 0;
    cmd->a    = addr;
    cmd->b    = value;
}

void NEOGEO_USER ng_rq_palette_upload(uint16_t palette_slot, const uint16_t *data)
{
    NGPaletteUpload *p;
    if (ng_rq_pal_count >= NG_RQ_MAX_PAL_UPLOADS) return;
    if (!data) return;
    p = &ng_rq_palettes[ng_rq_pal_count++];
    /*
     * palette_slot 0..N maps to PALETTES + slot * 32 bytes.
     * palette_addr stored as word offset from PALETTES base.
     */
    p->palette_addr = (uint16_t)(palette_slot * 16u);   /* 16 words = 32 bytes per slot */
    p->data         = data;
}

void NEOGEO_USER ng_render_queue_flush(void)
{
    uint8_t i;

    /* --- Apply sprite commands --- */
    for (i = 0; i < ng_rq_count; i++) {
        const NGRenderCmd *cmd = &ng_rq_cmds[i];

        switch (cmd->type) {
        case NG_RQ_SPRITE_POS:
            /*
             * Position: SCB3 holds Y position + sticky bit + height.
             *           SCB4 holds X position.
             * Write SCB3 first — the hardware latches sticky chains on SCB3.
             */
            vram_SCB234((uint16_t)(SCB3_ADDR + cmd->slot), cmd->a);
            vram_SCB234((uint16_t)(SCB4_ADDR + cmd->slot), cmd->b);
            break;

        case NG_RQ_SPRITE_SHRINK:
            vram_SCB234((uint16_t)(SCB2_ADDR + cmd->slot), cmd->a);
            break;

        case NG_RQ_SPRITE_HIDE:
            /* Zero SCB3 height = sprite invisible.
             * SCB4 left untouched: avoids visible X glitch on unhide. */
            vram_SCB234((uint16_t)(SCB3_ADDR + cmd->slot), 0);
            break;

        case NG_RQ_SCB234:
            vram_SCB234(cmd->a, cmd->b);
            break;

        default:
            break;
        }
    }
    ng_rq_count = 0;

    /* --- Apply palette uploads --- */
    for (i = 0; i < ng_rq_pal_count; i++) {
        const NGPaletteUpload *p = &ng_rq_palettes[i];
        uint16_t base = (uint16_t)(PALETTES + (uint32_t)p->palette_addr * 2u);
        load_palettes((uint16_t *)p->data, (uintptr_t)base);
    }
    ng_rq_pal_count = 0;
}

uint8_t NEOGEO_USER ng_rq_free(void)
{
    return (uint8_t)(NG_RQ_MAX_CMDS - ng_rq_count);
}

uint8_t NEOGEO_USER ng_rq_pal_free(void)
{
    return (uint8_t)(NG_RQ_MAX_PAL_UPLOADS - ng_rq_pal_count);
}
