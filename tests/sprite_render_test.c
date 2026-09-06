#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "neogeo.h"
#ifdef __cplusplus
#include "ng_sprite_group.hpp"
#include "ng_sprite_window.hpp"
#else
#include "ng_sprite_group.h"
#include "ng_sprite_window.h"
#endif

static uint16_t ram[0x8800];
static uint16_t address, increment;
static unsigned writes;

uint16_t setSCB2(uint16_t x, uint16_t y) { return (uint16_t)(((x & 15u) << 8) | (y & 255u)); }
uint16_t setSCB3(uint16_t y, uint16_t chain, uint16_t rows) { return (uint16_t)(((y & 511u) << 7) | (chain << 6) | rows); }
uint16_t setSCB4(uint16_t x) { return (uint16_t)((x & 511u) << 7); }
uint16_t setSCB1_2(uint16_t p, uint16_t t, uint16_t a8, uint16_t a4, uint16_t v, uint16_t h)
{ return (uint16_t)((p << 8) | (t << 4) | (a8 << 3) | (a4 << 2) | (v << 1) | h); }
void vram_init(uint16_t a, uint16_t i) { address = a; increment = i; }
void vram_sfix1(uint16_t value) { assert(address < 0x8800); ram[address] = value; address += increment; writes++; }
void vram_SCB234(uint16_t a, uint16_t value) { assert(a < 0x8800); ram[a] = value; writes++; }
void vram_SCB1(uint16_t *tiles, uint16_t *attrs, uint8_t count)
{ unsigned i; for (i = 0; i < count; i++) { vram_sfix1(tiles[i]); vram_sfix1(attrs[i]); } }
void vram_sprite(uint16_t a, uint16_t inc, uint16_t slot, uint16_t *tiles,
                 uint16_t *attrs, uint16_t count, uint16_t s2, uint16_t s3, uint16_t s4)
{
    vram_init(a, inc); vram_SCB1(tiles, attrs, (uint8_t)count);
    vram_SCB234(SCB2_ADDR + slot, s2); vram_SCB234(SCB3_ADDR + slot, s3); vram_SCB234(SCB4_ADDR + slot, s4);
}

int main(void)
{
    NGSpriteGroup g;
    NGSpriteWindow window;
    unsigned strip, row, before;
    memset(ram, 0x5a, sizeof(ram));
    ng_sprite_group_init(&g, 64u, 8u, 6u, 1000u, 19u);
    ng_sprite_group_set_tile_stride(&g, 16u);
    ng_sprite_group_set_scale(&g, 0x7fu, 0x7fu);
    ng_sprite_group_set_pos(&g, 80, 112);
    ng_sprite_group_upload(&g);
    assert((ram[0x8240] & 63u) == 3u);
    assert(ram[0x8440] == (80u << 7));
    for (strip = 0; strip < 8; strip++) {
        for (row = 0; row < 32; row++) {
            assert(ram[(64 + strip) * 64 + row * 2] == (row < 6 ? 1000 + row * 16 + strip : 0xffffu));
        }
        assert((ram[0x8240 + strip] & 0x40u) == (strip ? 0x40u : 0u));
    }
    before = writes;
    ng_sprite_group_flush(&g);
    assert(writes == before);
    ng_sprite_group_set_scale(&g, 0x3fu, 0x3fu);
    ng_sprite_group_flush(&g);
    assert((ram[0x8240] & 63u) == 2u);
    ng_sprite_group_set_visible(&g, 0u);
    ng_sprite_group_flush(&g);
    assert((ram[0x8240] & 127u) == 0u);
    before = writes;
    ng_sprite_group_set_pos(&g, 88, 100);
    ng_sprite_group_flush(&g);
    assert(writes == before);
    ng_sprite_group_set_visible(&g, 1u);
    ng_sprite_group_flush(&g);
    assert((ram[0x8240] & 63u) == 2u);
    assert(ram[0x8040] == 0x033fu);
    ng_sprite_group_set_flip(&g, 1u, 0u);
    ng_sprite_group_flush(&g);
    assert(ram[64 * 64] == 1007u);
    assert((ram[64 * 64 + 1] & 1u) == 1u);
    ng_sprite_window_init(&window, 1u, 64u, 16u);
    ng_sprite_window_set_shape(&window, 8u, 6u);
    before = writes;
    ng_sprite_window_clear_tail(&window);
    assert(writes == before);
    ng_sprite_window_set_shape(&window, 6u, 4u);
    ng_sprite_window_clear_tail(&window);
    assert(writes == before + 6u);
    assert((ram[0x8246] & 127u) == 0u);
    assert(ram[0x8446] == setSCB4(496u));
    assert(ram[0x8250] == 0x5a5au);
    assert(ram[0x8040] == 0x033fu);
    puts("sprite maps, shrinking, visibility, sticky tails and adjacent ownership: PASS");
    return 0;
}
