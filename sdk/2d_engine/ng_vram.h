#ifndef NG_VRAM_H
#define NG_VRAM_H

#include "ng_defs.h"
#include "ng_sprite_pool.h"

uint16_t NEOGEO_USER ng_vram_sprite_slot_to_scb1(uint16_t sprite_slot);
uint16_t NEOGEO_USER ng_vram_scb1_to_sprite_slot(uint16_t scb1_base);
void NEOGEO_USER ng_vram_clear_sprite_slot(uint16_t sprite_slot);
void NEOGEO_USER ng_vram_clear_sprite_range(uint16_t first_sprite, uint16_t count);
void NEOGEO_USER ng_vram_clear_sprite_vram_base(uint16_t sprite_base, uint16_t count);
void NEOGEO_USER ng_vram_clear_all_sprites(void);

#endif
