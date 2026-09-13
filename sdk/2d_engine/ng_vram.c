#include "ng_vram.h"
#include "ng_sprite_group.h"
#include "macro.h"
#include "neogeo.h"

uint16_t NEOGEO_USER ng_vram_sprite_slot_to_scb1(uint16_t sprite_slot)
{
    return (uint16_t)(sprite_slot << 6);
}

uint16_t NEOGEO_USER ng_vram_scb1_to_sprite_slot(uint16_t scb1_base)
{
    return (uint16_t)(scb1_base >> 6);
}

void NEOGEO_USER ng_vram_clear_sprite_slot(uint16_t sprite_slot)
{
    /* Delegate to the hardware-level disable.  Older callers used
     * this helper expecting a "fully off" sprite slot; in practice
     * it was only zeroing SCB2/3/4 + the first SCB1 word.  That left
     * the slot's chain bit, scale, X position and per-row SCB1
     * tile/attr state intact, and a single bit flip in SCB3 from
     * an adjacent driver strip would resurrect the slot as a
     * ghost.  ng_sprite_disable_hw does the complete teardown:
     * ACT=0, chain=0, off-screen Y=496, full scale, tile=attr=0. */
    ng_sprite_disable_hw(sprite_slot);
}

void NEOGEO_USER ng_vram_clear_sprite_range(uint16_t first_sprite, uint16_t count)
{
    uint16_t end;
    uint16_t sprite;

    if (first_sprite == 0xffffu) return;
    if (first_sprite >= NG_SPR_TOTAL) return;

    end = (uint16_t)(first_sprite + count);
    if (end > NG_SPR_TOTAL || end < first_sprite) end = NG_SPR_TOTAL;

    for (sprite = first_sprite; sprite < end; sprite++) {
        ng_vram_clear_sprite_slot(sprite);
    }
}

void NEOGEO_USER ng_vram_clear_sprite_vram_base(uint16_t sprite_base, uint16_t count)
{
    ng_vram_clear_sprite_range(ng_vram_scb1_to_sprite_slot(sprite_base), count);
}

void NEOGEO_USER ng_vram_clear_all_sprites(void)
{
    ng_vram_clear_sprite_range(0u, NG_SPR_TOTAL);
}
