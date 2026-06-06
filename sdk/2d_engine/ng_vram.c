#include "ng_vram.h"
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
    uint16_t scb1_base;

    if (sprite_slot >= NG_SPR_TOTAL) return;

    vram_SCB234((uint16_t)(SCB2_ADDR + sprite_slot), 0);
    vram_SCB234((uint16_t)(SCB3_ADDR + sprite_slot), 0);
    vram_SCB234((uint16_t)(SCB4_ADDR + sprite_slot), 0);

    /* Zero the first tile/attribute pair in SCB1 for this slot.  SCB3=0
     * is meant to mean "height 0 = invisible", but if a downstream write
     * later flips a single bit in SCB3 (e.g. sticky-chain inheritance
     * from a neighbouring driver strip), the leftover SCB1 tile would
     * render as a ghost.  Writing tile=0/attr=0 ensures the worst case
     * is a transparent strip, not stale art from a previous chapter. */
    scb1_base = (uint16_t)(64u * sprite_slot);
    vram_init(scb1_base, 1);
    vram_sfix1(0);   /* tile id row 0 */
    vram_sfix1(0);   /* attr   row 0 */
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
