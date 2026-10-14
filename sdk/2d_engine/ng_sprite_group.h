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
#define NG_SGF_QUEUED         0x80  /* NG_VRAM_DEFER: waiting for ng_vram_commit() */

/* Commit priority (NG_VRAM_DEFER): when a vertical blank hasn't room for
 * every change, the high ones go first and the low ones wait a frame. */
#define NG_SG_PRIO_NORMAL     0     /* the cast, shots, objects (the default) */
#define NG_SG_PRIO_HIGH       1     /* the player, the scrolling layers       */
#define NG_SG_PRIO_LOW        2     /* scenery and decoration                 */

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
    const uint8_t *tilePalettes;
    /* Footprint whose transparent padding is already resident in VRAM. */
    uint16_t mapFirst;
    uint8_t mapStrips;
    uint8_t mapHeight;
    uint8_t mapRows;
    uint8_t prio;       /* NG_SG_PRIO_*: set by ng_sprite_group_set_priority() */
    uint8_t readyRows;  /* NG_VRAM_DEFER: the rows its last full write left, when a
                         * move is then only the driving strip's two words (0: not so) */
} NGSpriteGroup;

void NEOGEO_USER ng_sprite_group_init(NGSpriteGroup *g, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette);
/* Mark specific attributes dirty so the next flush only uploads changed data. */
void NEOGEO_USER ng_sprite_group_mark_dirty(NGSpriteGroup *g, uint8_t dirty_flags);
/* Dirty-aware flush: only writes VRAM regions flagged in g->dirty. */
void NEOGEO_USER ng_sprite_group_flush(NGSpriteGroup *g);

/* A group's commit priority (NG_SG_PRIO_*); every group starts NORMAL. */
void NEOGEO_USER ng_sprite_group_set_priority(NGSpriteGroup *g, uint8_t prio);

#ifdef NG_VRAM_DEFER
/*
 * Video writes in the vertical blank (a game's GAME_ENGINE_DEFINES
 * -DNG_VRAM_DEFER=1, C engine). ng_sprite_group_flush() only puts the group
 * on its priority's list; ng_vram_commit(), called first thing after the
 * frame's wait for the vertical blank, writes the listed changes there --
 * the screen is never drawn from half-written sprite tables. A group is
 * written in the state it has at the commit, once however many times it
 * was flushed (the latest state: changes coalesce).
 *
 * The commit has a deadline: the blank's 40 lines end where the picture
 * starts (line 16), and no job begins at or after NG_VRAM_DEADLINE. What
 * doesn't fit stays listed, in order, for the next blank: hides first, then
 * the HIGH groups, a streaming layer's strip columns, the NORMAL groups, the
 * FIX layer's cells and last the LOW groups. Nothing listed is ever written
 * while the picture is drawn: a full list leaves the group dirty for its
 * next flush, a full FIX list falls back to whole text rows, and character
 * hides are a bitmap that can't overflow.
 *
 * ng_sprite_group_upload(), ng_sprite_group_hide(), ng_sprite_hide_all()
 * and ng_sprite_hide_range() still write at once: scene set-up, behind a
 * fade. During play, a group is drawn whole with
 * ng_sprite_group_mark_dirty(g, NG_SGF_DIRTY_ALL) and a flush. A group
 * listed but then thrown away must be dropped with ng_sprite_group_cancel(),
 * or the commit would bring it back (ng_sprite_hide_all() drops them all).
 */
#define NG_VRAM_QUEUE_GROUPS  256u   /* NORMAL */
#define NG_VRAM_QUEUE_HIGH    32u
#define NG_VRAM_QUEUE_LOW     128u
#define NG_VRAM_QUEUE_STRIPS  64u
#define NG_VRAM_DEADLINE      8u     /* the last line a job may begin before: the picture is at 16 */
void NEOGEO_USER ng_vram_commit(void);
/* Lines left before the deadline (0 while the picture is being drawn), and
 * whether a commit could start now. */
uint8_t NEOGEO_USER ng_vram_lines_left(void);
uint8_t NEOGEO_USER ng_vram_window_open(void);
/* Non-zero while the engine is writing video memory (the commit, a full
 * list's flush, a hide, an upload): raster bands (ng_raster.h) leave their
 * own video writes out then. */
extern volatile uint8_t ng_vram_busy;
void NEOGEO_USER ng_sprite_group_cancel(NGSpriteGroup *g);
void NEOGEO_USER ng_sprite_hide_range_queued(uint16_t firstSprite, uint16_t count);
#endif
void NEOGEO_USER ng_sprite_group_set_tile_base(NGSpriteGroup *g, uint16_t tileBase);
void NEOGEO_USER ng_sprite_group_set_tile_stride(NGSpriteGroup *g, uint16_t tileStride);
/* One strip shown from another column of the art (16-pixel columns from
 * tileBase): a layer wider than 512 pixels streams its columns this way,
 * each strip rewritten while off screen. With NG_VRAM_DEFER it is listed
 * for the blank (the same strip twice in a frame: the last column);
 * otherwise written at once. */
void NEOGEO_USER ng_sprite_group_set_strip_column(NGSpriteGroup *g, uint8_t strip, uint16_t column);
void NEOGEO_USER ng_sprite_group_set_palette(NGSpriteGroup *g, uint8_t palette);
/* Row-major bank map using tileStride; NULL restores the single bank. */
void NEOGEO_USER ng_sprite_group_set_palette_map(NGSpriteGroup *g, const uint8_t *banks);
void NEOGEO_USER ng_sprite_group_set_active_rows(NGSpriteGroup *g, uint8_t activeRows);
void NEOGEO_USER ng_sprite_group_set_pos(NGSpriteGroup *g, int16_t x, int16_t y);
void NEOGEO_USER ng_sprite_group_move(NGSpriteGroup *g, int16_t dx, int16_t dy);
void NEOGEO_USER ng_sprite_group_set_scale(NGSpriteGroup *g, uint8_t xScale, uint8_t yScale);
void NEOGEO_USER ng_sprite_group_set_flip(NGSpriteGroup *g, uint8_t hflip, uint8_t vflip);
void NEOGEO_USER ng_sprite_group_set_auto_anim(NGSpriteGroup *g, uint8_t autoAnim4, uint8_t autoAnim8);
void NEOGEO_USER ng_sprite_group_set_visible(NGSpriteGroup *g, uint8_t visible);
/* The common per-frame cases in one call each:
 * show_at = set_tile_base + set_palette + set_pos + set_visible(1) + flush;
 * hide_all = set_visible(0) + flush for each of `count` groups. */
void NEOGEO_USER ng_sprite_group_show_at(NGSpriteGroup *g, uint16_t tileBase, uint8_t palette,
                                         int16_t x, int16_t y);
void NEOGEO_USER ng_sprite_groups_hide_all(NGSpriteGroup *g, uint8_t count);
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
