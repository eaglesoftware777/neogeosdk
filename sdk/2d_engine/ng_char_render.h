#ifndef NG_CHAR_RENDER_H
#define NG_CHAR_RENDER_H

/* Shared by both character managers after slot ownership is resolved.
 * (Kept out of line in C: inlined into the draw loop, GCC lost the two
 * pointers and worked each field's address out again from the index.) */
#ifndef __cplusplus
__attribute__((noinline))
#endif
static void NEOGEO_USER ng_char_sync_group(NGSpriteGroup *g, NGCharacter *c,
                                           uint8_t strips, uint8_t fresh,
                                           int16_t camera_x, int16_t camera_y)
{
    uint8_t height = c->sprite_height ? c->sprite_height : 1u;
    if (fresh || g->firstSprite != c->sprite_first || g->strips != strips ||
        g->heightTiles != height) {
        ng_sprite_group_init(g, c->sprite_first, strips, height,
                             c->sprite_tile, c->palette);
    }
#ifndef __cplusplus
    g->prio = c->vram_prio;
#endif
#ifdef __cplusplus
    ng_sprite_group_set_tile_base(g, c->sprite_tile);
    ng_sprite_group_set_palette(g, c->palette);
    ng_sprite_group_set_tile_stride(g, c->sprite_stride ? c->sprite_stride : strips);
    ng_sprite_group_set_palette_map(g, c->sprite_palette_map);
    ng_sprite_group_set_active_rows(g, c->sprite_active_rows ? c->sprite_active_rows : height);
    ng_sprite_group_set_pos(g, (int16_t)(c->x + c->sprite_offset_x - camera_x),
                           (int16_t)(c->y + c->sprite_offset_y - camera_y));
    ng_sprite_group_set_scale(g, c->scale_x, c->scale_y);
    ng_sprite_group_set_flip(g, c->flip_x, c->flip_y);
#else
    /* The eight ng_sprite_group_set_*() calls, field by field: each field
     * that differs is taken and marks what it marks there. Every character
     * passes here every frame, and the calls were most of its cost. */
    {
        uint8_t d = 0;
        uint16_t stride = c->sprite_stride ? c->sprite_stride : strips;
        uint8_t rows = c->sprite_active_rows ? c->sprite_active_rows : height;
        int16_t x = (int16_t)(c->x + c->sprite_offset_x - camera_x);
        int16_t y = (int16_t)(c->y + c->sprite_offset_y - camera_y);
        uint8_t hflip = c->flip_x ? 1u : 0u;
        uint8_t vflip = c->flip_y ? 1u : 0u;

        if (g->tileBase != c->sprite_tile) { g->tileBase = c->sprite_tile; d |= NG_SGF_DIRTY_TILE; }
        if (g->palette != c->palette) { g->palette = c->palette; d |= NG_SGF_DIRTY_PALETTE; }
        if (g->tileStride != stride) { g->tileStride = stride; d |= NG_SGF_DIRTY_TILE; }
        if (g->tilePalettes != c->sprite_palette_map) {
            g->tilePalettes = c->sprite_palette_map;
            d |= NG_SGF_DIRTY_PALETTE;
        }
        if (rows > g->heightTiles) rows = g->heightTiles;
        if (rows > NG_SPRITE_MAX_HEIGHT_TILES) rows = NG_SPRITE_MAX_HEIGHT_TILES;
        if (g->activeRows != rows) { g->activeRows = rows; d |= NG_SGF_DIRTY_POS | NG_SGF_DIRTY_SHRINK; }
        if (g->x != x || g->y != y) { g->x = x; g->y = y; d |= NG_SGF_DIRTY_POS; }
        if (g->xScale != c->scale_x || g->yScale != c->scale_y) {
            g->xScale = c->scale_x;
            g->yScale = c->scale_y;
            d |= NG_SGF_DIRTY_SHRINK;
        }
        if (g->hflip != hflip || g->vflip != vflip) {
            g->hflip = hflip;
            g->vflip = vflip;
            d |= NG_SGF_DIRTY_TILE | NG_SGF_DIRTY_PALETTE;
        }
        g->dirty |= d;
    }
#endif
    /* An explicit invalidation without a field change may follow raw VRAM
     * writes. Rebuild the whole group in that case. */
#ifdef NG_VRAM_DEFER
    if (c->sprite_dirty && !(g->dirty & NG_SGF_DIRTY_ALL))   /* (a listing for the commit isn't a change) */
#else
    if (c->sprite_dirty && !g->dirty)
#endif
        ng_sprite_group_mark_dirty(g, NG_SGF_DIRTY_ALL);
    ng_sprite_group_flush(g);
    c->sprite_dirty = 0u;
}

#endif
