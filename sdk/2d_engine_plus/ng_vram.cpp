#include "ng_hw.hpp"
#include "ng_vram.hpp"
#include "ng_sprite_group.hpp"

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
    /* Full hardware disable — ACT/chain/position/scale/tile/attr.
     * See ng_sprite_disable_hw() for the per-field rationale. */
    ng_sprite_disable_hw(sprite_slot);
}

void NEOGEO_USER ng_vram_clear_sprite_range(uint16_t first_sprite, uint16_t count)
{
    uint16_t end;

    if (first_sprite == 0xffffu) return;
    if (first_sprite >= NG_SPR_TOTAL) return;

    end = (uint16_t)(first_sprite + count);
    if (end > NG_SPR_TOTAL || end < first_sprite) end = NG_SPR_TOTAL;

    for (uint16_t sprite = first_sprite; sprite < end; sprite++) {
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
