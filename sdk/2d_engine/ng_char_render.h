#ifndef NG_CHAR_RENDER_H
#define NG_CHAR_RENDER_H

/* Shared by both character managers after slot ownership is resolved. */
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
    ng_sprite_group_set_tile_base(g, c->sprite_tile);
    ng_sprite_group_set_palette(g, c->palette);
    ng_sprite_group_set_tile_stride(g, c->sprite_stride ? c->sprite_stride : strips);
    ng_sprite_group_set_palette_map(g, c->sprite_palette_map);
    ng_sprite_group_set_active_rows(g, c->sprite_active_rows ? c->sprite_active_rows : height);
    ng_sprite_group_set_pos(g, (int16_t)(c->x + c->sprite_offset_x - camera_x),
                           (int16_t)(c->y + c->sprite_offset_y - camera_y));
    ng_sprite_group_set_scale(g, c->scale_x, c->scale_y);
    ng_sprite_group_set_flip(g, c->flip_x, c->flip_y);
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
