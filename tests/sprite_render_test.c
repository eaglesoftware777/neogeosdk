/*
 * Sprite renderer regression tests.
 *
 * The engine sources are compiled against a stand-in VRAM array so the
 * hardware-facing rules can be checked on the host: how many active
 * characters end up in SCB3 once a sprite is shrunk, how much of the SCB1
 * map an upload has to blank, and which slots a partial flush is allowed to
 * touch.  Both renderers are built from this one file - the C engine as C,
 * the C++ engine as C++ - so the two cannot drift apart silently.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "neogeo.h"
#include "../sdk/2d_engine/ng_sprite_hw.h"
#include "../sdk/2d_engine/ng_palette_math.h"
#ifdef __cplusplus
#include "ng_art_asset.hpp"
#include "ng_palette_fx.hpp"
#include "ng_sprite_group.hpp"
#include "ng_sprite_window.hpp"
#else
#include "ng_art_asset.h"
#include "ng_palette_fx.h"
#include "ng_sprite_group.h"
#include "ng_sprite_window.h"
#endif

static uint16_t ram[0x8800];
static uint16_t address, increment;
static unsigned writes;
static uint16_t uploaded_palette[16];

void ng_rq_palette_upload(uint16_t slot, const uint16_t *data)
{
    (void)slot;
    memcpy(uploaded_palette, data, sizeof(uploaded_palette));
}

void load_palettes(uint16_t *palette, uintptr_t destination)
{
    (void)destination;
    memcpy(uploaded_palette, palette, sizeof(uploaded_palette));
}

#define UNTOUCHED 0x5a5au

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

static uint16_t map_tile(unsigned slot, unsigned row) { return ram[slot * 64 + row * 2]; }
static uint16_t act_of(unsigned slot) { return (uint16_t)(ram[SCB3_ADDR + slot] & 63u); }

/*
 * A six-tile sprite at half height occupies three characters, not six: the
 * active-character count is the sprite's height on screen, and the shrink
 * only chooses which source row each scanline reads.  Leave it at six and
 * the picture repeats in the lower half of its own window.
 */
static void test_shrink_sets_active_characters(NGSpriteGroup *g)
{
    unsigned strip, row;

    ng_sprite_group_init(g, 64u, 8u, 6u, 1000u, 19u);
    ng_sprite_group_set_tile_stride(g, 16u);
    ng_sprite_group_set_scale(g, 0x7fu, 0x7fu);
    ng_sprite_group_set_pos(g, 80, 112);
    ng_sprite_group_upload(g);

    assert(act_of(64) == 3u);
    assert(ram[SCB4_ADDR + 64] == (80u << 7));

    for (strip = 0; strip < 8; strip++) {
        /* Rows the lookup can reach hold art, then the transparent tile. */
        for (row = 0; row < 16; row++)
            assert(map_tile(64 + strip, row) == (row < 6 ? 1000 + row * 16 + strip : 0xffffu));
        /* Rows 16..31 are only reachable past 256 scanlines, which three
         * active characters cannot span, so an upload must not pay for them. */
        for (row = 16; row < 32; row++)
            assert(map_tile(64 + strip, row) == UNTOUCHED);
        assert((ram[SCB3_ADDR + 64 + strip] & 0x40u) == (strip ? 0x40u : 0u));
    }
}

/* Past sixteen active characters the sprite spans more than 256 scanlines,
 * the lookup mirrors into the upper half of the map, and all 32 rows have to
 * be laid down. */
static void test_tall_sprite_blanks_whole_map(NGSpriteGroup *g)
{
    unsigned row;

    ng_sprite_group_init(g, 200u, 1u, 20u, 2000u, 3u);
    ng_sprite_group_set_tile_stride(g, 16u);
    ng_sprite_group_set_scale(g, 0xffu, 0xffu);
    ng_sprite_group_set_pos(g, 32, 200);
    ng_sprite_group_upload(g);

    assert(act_of(200) == 20u);
    for (row = 0; row < 32; row++)
        assert(map_tile(200, row) == (row < 20 ? 2000 + row * 16 : 0xffffu));
}

/* A scale change moves the active-character count, so it has to reach SCB3
 * and not only the shrink register. */
static void test_scale_change_refreshes_scb3(NGSpriteGroup *g)
{
    unsigned before = writes;

    ng_sprite_group_flush(g);
    assert(writes == before);                 /* nothing dirty, nothing written */

    ng_sprite_group_set_scale(g, 0x3fu, 0x3fu);
    ng_sprite_group_flush(g);
    assert(ram[SCB2_ADDR + 64] == 0x033fu);
    assert(act_of(64) == 2u);
    assert((ram[SCB3_ADDR + 65] & 63u) == 2u); /* chained copies follow */
}

/* Cropping a page to fewer rows moves the active-character count, which
 * every chained strip carries a copy of. */
static void test_active_rows_reach_chained_strips(NGSpriteGroup *g)
{
    ng_sprite_group_set_scale(g, 0xffu, 0xffu);
    ng_sprite_group_set_active_rows(g, 4u);
    ng_sprite_group_flush(g);
    assert(act_of(64) == 4u);
    assert((ram[SCB3_ADDR + 71] & 127u) == (0x40u | 4u));

    ng_sprite_group_set_active_rows(g, 6u);
    ng_sprite_group_set_scale(g, 0x3fu, 0x3fu);
    ng_sprite_group_flush(g);
    assert(act_of(64) == 2u);
    assert((ram[SCB3_ADDR + 71] & 127u) == (0x40u | 2u));
}

/* A move is the hot path.  The hardware reads no position from a chained
 * slot, so a scroll must cost two words, not two per strip. */
static void test_move_touches_driver_only(NGSpriteGroup *g)
{
    unsigned before;

    ram[SCB3_ADDR + 65] = UNTOUCHED;
    before = writes;
    ng_sprite_group_set_pos(g, 88, 100);
    ng_sprite_group_flush(g);
    assert(writes == before + 2u);
    assert(ram[SCB3_ADDR + 65] == UNTOUCHED);
    assert(ram[SCB4_ADDR + 64] == (88u << 7));
}

/* Hiding parks every strip; showing again has to put the chain bits back,
 * because parking a slot clears them. */
static void test_hide_and_show_restore_the_chain(NGSpriteGroup *g)
{
    unsigned before;

    ng_sprite_group_set_visible(g, 0u);
    ng_sprite_group_flush(g);
    assert((ram[SCB3_ADDR + 64] & 127u) == 0u);
    assert(ram[SCB4_ADDR + 64] == setSCB4(496u));

    before = writes;
    ng_sprite_group_set_pos(g, 40, 40);
    ng_sprite_group_flush(g);
    assert(writes == before);                 /* invisible groups stay quiet */

    ng_sprite_group_set_visible(g, 1u);
    ng_sprite_group_flush(g);
    assert(act_of(64) == 2u);
    assert((ram[SCB3_ADDR + 65] & 0x40u) == 0x40u);
    assert(map_tile(64, 0) == 1000u);
}

static void test_flip_rewrites_the_map(NGSpriteGroup *g)
{
    ng_sprite_group_set_flip(g, 1u, 0u);
    ng_sprite_group_flush(g);
    assert(map_tile(64, 0) == 1007u);         /* strip order reverses */
    assert((ram[64 * 64 + 1] & 1u) == 1u);
}

/*
 * A window only owns the strips it is using.  Shrinking one must park the
 * strips it gave up and nothing beyond them, or it takes the neighbouring
 * sprite's slots with it.
 */
static void test_window_parks_only_its_own_tail(void)
{
    NGSpriteWindow window;
    unsigned before;

    ng_sprite_window_init(&window, 1u, 64u, 16u);
    ng_sprite_window_set_shape(&window, 8u, 6u);
    before = writes;
    ng_sprite_window_clear_tail(&window);
    assert(writes == before);                 /* nothing given up yet */

    ng_sprite_window_set_shape(&window, 6u, 4u);
    ng_sprite_window_clear_tail(&window);
    assert(writes == before + 6u);            /* two strips parked, 3 words each */
    assert((ram[SCB3_ADDR + 70] & 127u) == 0u);
    assert(ram[SCB4_ADDR + 70] == setSCB4(496u));
    assert(ram[SCB2_ADDR + 80] == UNTOUCHED); /* the neighbour is untouched */

    /* A full teardown is bounded the same way.  This window reserved 16
     * strips but has never drawn into more than 8, so parking all 16
     * would take the neighbour allocated at slot 72 with it. */
    before = writes;
    ng_sprite_window_hide(&window);
    assert(writes == before + 8u * 3u);
    assert(ram[SCB2_ADDR + 72] == UNTOUCHED);
}

static void test_palette_effects(void)
{
    uint16_t base[16] = {0x1234u, 0x4f00u, 0x20f0u, 0x100fu, 0x8000u};
    uint16_t result[16];
    unsigned color, i;

    for (color = 0; color <= 0xffffu; color++) {
        base[15] = (uint16_t)color;
        ng_palette_scale_colors(result, base, 255u);
        assert(result[15] == color);
        ng_palette_tint_colors(result, base, 255u, 0u, 0u, 0u);
        assert(result[15] == color);
    }
    assert(ng_color_pack(31u, 0u, 0u) == 0x4f00u);
    assert(ng_color_pack(0u, 31u, 0u) == 0x20f0u);
    assert(ng_color_pack(0u, 0u, 31u) == 0x100fu);

    ng_palette_fx_init();
    ng_palfx_flash_white(12u, base, 4u);
    ng_palette_fx_update();
    assert(uploaded_palette[0] == base[0]);
    for (i = 1; i < 16; i++) assert(uploaded_palette[i] == 0x7fffu);
    for (i = 0; i < 4; i++) ng_palette_fx_update();
    assert(memcmp(base, uploaded_palette, sizeof(base)) == 0);

    ng_palfx_fade_in(12u, base, 2u);
    ng_palette_fx_update();
    assert(ng_color_g(uploaded_palette[1]) == 0u);
    assert(ng_color_b(uploaded_palette[1]) == 0u);
    ng_palette_fx_update();
    assert(memcmp(base, uploaded_palette, sizeof(base)) == 0);

    ng_palfx_cycle(12u, base, 15u, 15u);
    for (i = 0; i < 20; i++) ng_palette_fx_update();
    assert(memcmp(base, uploaded_palette, sizeof(base)) == 0);
    ng_palfx_cycle(12u, base, 0u, 255u);
    for (i = 0; i < 20; i++) {
        ng_palette_fx_update();
        assert(uploaded_palette[0] == base[0]);
    }
    for (i = 1; i < 8; i++) {
        ng_palfx_pulse(12u, base, (uint8_t)i);
        ng_palette_fx_update();
    }
    ng_palfx_stop(12u);
    ng_palfx_fade_in(12u, 0, 1u);
    assert(!ng_palfx_active(12u));
}

int main(void)
{
    NGSpriteGroup g;
    NGSpriteGroup tall;
    NGArtAsset assets[3];
    memset(assets, 0, sizeof(assets));
    assets[0].asset_id = 1u;
    assets[1].asset_id = 2u;
    assets[2].asset_id = 90u;
    assert(ng_art_asset_find(assets, 3u, 2u) == &assets[1]);
    assert(ng_art_asset_find(assets, 3u, 90u) == &assets[2]);
    assert(!ng_art_asset_find(assets, 3u, 4u));
    assert(!ng_art_asset_find(0, 3u, 1u));

    memset(ram, 0x5a, sizeof(ram));

    test_shrink_sets_active_characters(&g);
    test_tall_sprite_blanks_whole_map(&tall);
    test_scale_change_refreshes_scb3(&g);
    test_active_rows_reach_chained_strips(&g);
    test_move_touches_driver_only(&g);
    test_hide_and_show_restore_the_chain(&g);
    test_flip_rewrites_the_map(&g);
    test_window_parks_only_its_own_tail();
    test_palette_effects();
    assert(ng_sprite_scaled_x(256u, 0x7fu) == 128u);
    assert(ng_sprite_scaled_y(256u, 0x7fu) == 128u);
    assert(ng_sprite_scaled_y(256u, 0xffu) == 256u);
    assert(ng_sprite_row_tile(100u, 3u, 2u) == 106u);

    puts("sprite geometry, map padding, window tails and palette effects: PASS");
    return 0;
}
