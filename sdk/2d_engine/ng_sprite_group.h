#ifndef NG_SPRITE_GROUP_H
#define NG_SPRITE_GROUP_H

#include "ng_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Hardware sprite group.
 *
 * Neo Geo sprites are 16px-wide vertical strips.  A large object is a group
 * of adjacent strips, normally using the SCB3 sticky/chain bit for strips
 * 1..N.  The first strip is the driver; chained strips inherit Y, height and
 * vertical shrink from the driver, while each strip still needs its own tile
 * map, palette attributes and horizontal shrink.
 */
/*
 * Dirty flag bits for NGSpriteGroup.
 * Only the bits that are set get written to VRAM on the next update.
 * This avoids re-uploading all SCB fields every frame for static objects.
 */
#define NG_SGF_DIRTY_POS      0x01  /* x/y changed → write SCB3/SCB4 */
#define NG_SGF_DIRTY_TILE     0x02  /* tileBase/stride changed → write SCB1 */
#define NG_SGF_DIRTY_PALETTE  0x04  /* palette changed → update SCB1 attrs */
#define NG_SGF_DIRTY_SHRINK   0x08  /* scale changed → write SCB2 */
#define NG_SGF_DIRTY_VIS      0x10  /* visibility changed */
#define NG_SGF_DIRTY_ALL      0x1F  /* force full upload */

typedef struct {
    uint16_t firstSprite;
    uint8_t strips;
    uint8_t heightTiles;
    uint8_t activeRows;
    uint16_t tileBase;
    uint16_t tileStride;
    uint8_t palette;
    int16_t x;
    int16_t y;
    uint8_t xScale;
    uint8_t yScale;
    uint8_t hflip;
    uint8_t vflip;
    uint8_t autoAnim4;
    uint8_t autoAnim8;
    uint8_t visible;
    uint8_t dirty;      /* bitmask of NG_SGF_DIRTY_* flags */
} NGSpriteGroup;

void NEOGEO_USER ng_sprite_group_init(NGSpriteGroup *g, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette);
/* Mark specific attributes dirty so the next flush only uploads changed data. */
void NEOGEO_USER ng_sprite_group_mark_dirty(NGSpriteGroup *g, uint8_t dirty_flags);
/* Dirty-aware flush: only writes VRAM regions flagged in g->dirty. */
void NEOGEO_USER ng_sprite_group_flush(NGSpriteGroup *g);
void NEOGEO_USER ng_sprite_group_set_tile_base(NGSpriteGroup *g, uint16_t tileBase);
void NEOGEO_USER ng_sprite_group_set_tile_stride(NGSpriteGroup *g, uint16_t tileStride);
void NEOGEO_USER ng_sprite_group_set_palette(NGSpriteGroup *g, uint8_t palette);
void NEOGEO_USER ng_sprite_group_set_active_rows(NGSpriteGroup *g, uint8_t activeRows);
void NEOGEO_USER ng_sprite_group_set_pos(NGSpriteGroup *g, int16_t x, int16_t y);
void NEOGEO_USER ng_sprite_group_move(NGSpriteGroup *g, int16_t dx, int16_t dy);
void NEOGEO_USER ng_sprite_group_set_scale(NGSpriteGroup *g, uint8_t xScale, uint8_t yScale);
void NEOGEO_USER ng_sprite_group_set_flip(NGSpriteGroup *g, uint8_t hflip, uint8_t vflip);
void NEOGEO_USER ng_sprite_group_set_auto_anim(NGSpriteGroup *g, uint8_t autoAnim4, uint8_t autoAnim8);
void NEOGEO_USER ng_sprite_group_set_visible(NGSpriteGroup *g, uint8_t visible);
void NEOGEO_USER ng_sprite_group_upload(NGSpriteGroup *g);
void NEOGEO_USER ng_sprite_group_update_transform(NGSpriteGroup *g);
void NEOGEO_USER ng_sprite_group_hide(NGSpriteGroup *g);
void NEOGEO_USER ng_engine_init_hardware(uint16_t transparentTile);

/*
 * Off-screen parking constants for hardware sprite teardown.
 *
 * The engine's SCB3 encoding is Y_field = 496 - screen_y, so
 * Y_field = 496 puts the sprite at screen_y = 0 (top of visible
 * area) — NOT off-screen.  If a disabled slot ever has its ACT/
 * chain bumped non-zero by a stray write, parking it at Y_field
 * = 496 makes it appear as a black/blank bar across the top of
 * the screen (this was visible in MAME after the first version
 * of ng_sprite_disable_hw shipped).
 *
 * Park at Y_field = 256 so screen_y = 240 (past the 224-line
 * visible window) and X = 496 so it sits well off the right
 * edge of the 320-px screen.  Even with the worst-case bit flip,
 * the resurrected sprite ends up where the user cannot see it.
 */
#define NG_SPRITE_DISABLED_YREG   256u
#define NG_SPRITE_DISABLED_X      496u
#define NG_SPRITE_DISABLED_SCB3   ((uint16_t)(NG_SPRITE_DISABLED_YREG << 7))

/*
 * Blank-tile convention for hardware sprite teardown.
 *
 * ng_sprite_disable_hw() fills every SCB1 row of a slot with a
 * (tile, attr) pair.  If we wrote (0, 0) the LSPC would still
 * happily render C-ROM tile 0 through palette bank 0 — and on
 * NeoGeo, palette[0][0] is the monitor-sync reference colour
 * (pure black).  Combined with any non-transparent pixel in
 * tile 0, that paints a black rectangle wherever the slot's Y
 * happens to fall on-screen.
 *
 * Convention:
 *   - Tile 0xFFFF is reserved as the always-transparent C-ROM
 *     tile id (matches NGFIX_DEFAULT_BLANK_TILE = 0x00FF for the
 *     FIX layer's separate space).  This tile lives near the top
 *     of the SCB1-addressable 16-bit tile range (0..0xFFFF), which
 *     for this project's 16 MB C-ROM is well past the last asset
 *     (0x78FA) and so reads as all-zero pixel data — fully
 *     transparent.
 *   - Attribute 0x0000 keeps palette/flip bits zero.  With a fully
 *     transparent tile the chosen palette is irrelevant, but using
 *     0 keeps the disabled slot identifiable in VRAM dumps.
 *
 * If a project ships its own C-ROM, ensure tile 0xFFFF (or whatever
 * the project picks) is empty (every pixel index = 0) or override
 * these constants before including this header.  Verify with:
 *
 *     xxd -s $((0xFFFF * 64)) -l 64 games/<game>/artbox/<id>-c1.c1
 *     xxd -s $((0xFFFF * 64)) -l 64 games/<game>/artbox/<id>-c2.c2
 *
 * both should print 64 zero bytes.
 */
#ifndef NG_SPRITE_BLANK_TILE
#define NG_SPRITE_BLANK_TILE  0xFFFFu
#endif
#ifndef NG_SPRITE_BLANK_ATTR
#define NG_SPRITE_BLANK_ATTR  0x0000u
#endif

/* Hide a raw hardware-sprite range and clear stale chain/position state. */
void NEOGEO_USER ng_sprite_hide_range(uint16_t firstSprite, uint16_t count);
void NEOGEO_USER ng_sprite_hide_vram_base(uint16_t spriteBase, uint16_t count);
void NEOGEO_USER ng_sprite_hide_all(void);

/*
 * Hardware sprite teardown.
 *
 * Both ng_sprite_disable_hw() and ng_sprite_park_off() do the
 * SAME full teardown — a partial "quick park" that only zeros
 * SCB3 (and maybe SCB1 row 0) is not enough.  If any later write
 * resurrects ACT/chain or the LSPC wraps the height field, rows
 * 1..31 of SCB1 still hold last-frame artwork and reappear as
 * horizontal strips / black boxes around the active chars.
 *
 * The pair is a real distinction now: disable_hw is the full ~67-write
 * teardown for scene boundaries, park_off is the three-word per-frame
 * hide.  park_off can skip the SCB1 wipe because upload and flush
 * guarantee the map's padding, and because a parked slot's X sits
 * off-screen right, where the hardware skips it outright.  Either way
 * callers bound the count to what they actually own (ng_chars_draw
 * Phase 2 uses prev_strips - vis_strips; a sprite window uses its
 * max_used_strips high-water mark).
 *
 * Per slot the teardown writes:
 *   SCB3       = NG_SPRITE_DISABLED_SCB3 (ACT=0, chain=0, Y off-screen)
 *   SCB2       = 0x0FFF (full scale)
 *   SCB4       = NG_SPRITE_DISABLED_X (off-screen right)
 *   SCB1[0..63]= 32 × (NG_SPRITE_BLANK_TILE, NG_SPRITE_BLANK_ATTR)
 */
void NEOGEO_USER ng_sprite_disable_hw(uint16_t spr);
void NEOGEO_USER ng_sprite_disable_hw_range(uint16_t first, uint16_t count);
void NEOGEO_USER ng_sprite_park_off(uint16_t spr);
void NEOGEO_USER ng_sprite_park_off_range(uint16_t first, uint16_t count);


#ifdef __cplusplus
}
#endif
#endif
