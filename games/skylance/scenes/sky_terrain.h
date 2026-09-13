/*
 * A scrolling terrain page for a vertical shooter, shared by Sky Lance
 * and the demo's Sky Lance chapter.
 *
 * A page is 16 x 16 tiles - 256 px square - and the sprite chip can only
 * shrink, never stretch, so a scrolling backdrop is exactly one page
 * tall.  Two copies are stacked and slid downward; the second is flipped
 * vertically, so the join always meets the same source row from either
 * side and the picture never has to be authored to tile.  The tiles are
 * uploaded once; each frame moves the two groups and nothing else.
 */
#ifndef SKY_TERRAIN_H
#define SKY_TERRAIN_H

#include "sdk/2d_engine/ng_art_asset.h"
#include "sdk/2d_engine/ng_sprite_group.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t NEOGEO_USER ng_load_screen_palette(uint16_t id);
const NGArtAsset * NEOGEO_USER ng_screen_art_asset(uint16_t id);

typedef struct {
    NGSpriteGroup page[2];
    uint16_t scroll;
    uint8_t ready;
} SkyTerrain;

static void NEOGEO_USER sky_terrain_init(SkyTerrain *terrain, uint16_t id,
                                        uint16_t first_slot, int16_t x)
{
    const NGArtAsset *asset = ng_screen_art_asset(id);
    uint8_t i;
    terrain->ready = 0u;
    terrain->scroll = 0u;
    if (!asset || asset->strips != 16u || asset->active_rows != 16u) return;
    ng_load_screen_palette(id);
    for (i = 0u; i < 2u; i++) {
        NGSpriteGroup *page = &terrain->page[i];
        ng_sprite_group_init(page, (uint16_t)(first_slot + i * 16u),
                             16u, 16u, asset->tile_base, asset->palette_bank);
        ng_sprite_group_set_tile_stride(page, asset->tile_stride);
        ng_sprite_group_set_palette_map(page, asset->tile_palettes);
        ng_sprite_group_set_scale(page, 0xFFu, 0xFFu);
        /* Reflected repeat joins the same source row at either boundary.
         * No assumptions about hand-painted edge matching or blank padding. */
        ng_sprite_group_set_flip(page, 0u, i);
        ng_sprite_group_set_pos(page, x, i ? -256 : 0);
        ng_sprite_group_upload(page);
    }
    terrain->ready = 1u;
}

static void NEOGEO_USER sky_terrain_draw(SkyTerrain *terrain, uint8_t advance,
                                        int16_t x)
{
    uint8_t i;
    if (!terrain->ready) return;
    terrain->scroll = (uint16_t)((terrain->scroll + advance) & 511u);
    for (i = 0u; i < 2u; i++) {
        int16_t y = (int16_t)((terrain->scroll + i * 256u) & 511u);
        if (y >= 256) y -= 512;
        ng_sprite_group_set_pos(&terrain->page[i], x, y);
        /* Tile IDs and palette maps stay resident; only position is dirty. */
        ng_sprite_group_flush(&terrain->page[i]);
    }
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* SKY_TERRAIN_H */
