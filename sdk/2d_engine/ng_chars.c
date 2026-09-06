#include "ng_chars.h"
#include "ng_actions.h"
#include "ng_level.h"
#include "ng_sprite_pool.h"

static NGCharacter ng_chars[NG_MAX_CHARS];
static NGCharInterupt ng_char_interupts[NG_MAX_CHAR_KINDS];

static uint8_t ng_char_uploaded_strips[NG_MAX_CHARS];
static uint16_t ng_char_uploaded_first[NG_MAX_CHARS];
static uint8_t ng_palette_owner[64];
static uint8_t ng_default_arena_id;
static uint8_t ng_fixed_updates_per_frame = 1u;

/* Highest occupied slot index + 1. Limits scan loops to avoid iterating all
 * NG_MAX_CHARS entries when only a few characters are active. */
static uint8_t ng_chars_active_top;

uint8_t NEOGEO_USER ng_char_validate_asset_window(uint16_t tileBase,
                                                  uint8_t strips,
                                                  uint8_t rows,
                                                  uint16_t stride,
                                                  uint16_t tileStart,
                                                  uint16_t tileEnd)
{
    uint32_t last;
    uint16_t use_strips = strips ? strips : 1u;
    uint16_t use_rows = rows ? rows : 1u;
    uint16_t use_stride = stride ? stride : use_strips;

    if (tileEnd < tileStart) return 0u;
    if (tileBase < tileStart) return 0u;
    if (use_strips > NG_SPRITE_MAX_STRIPS) return 0u;
    if (use_rows > NG_SPRITE_MAX_HEIGHT_TILES) return 0u;
    if (use_stride < use_strips) return 0u;

    last = (uint32_t)tileBase +
           (uint32_t)(use_rows - 1u) * (uint32_t)use_stride +
           (uint32_t)(use_strips - 1u);

    if (last > 0xffffu) return 0u;
    return (uint8_t)(last <= (uint32_t)tileEnd);
}

static void NEOGEO_USER chars_hide_slot(uint16_t firstSprite, uint8_t strips)
{
    if (firstSprite == 0xffff) return;
    if (strips == 0) strips = NG_SPRITE_MAX_STRIPS;
    if (strips > NG_SPRITE_MAX_STRIPS) strips = NG_SPRITE_MAX_STRIPS;
    ng_sprite_hide_range(firstSprite, strips);
}

/* Hide a previously-uploaded char window using the actual strip count
 * we recorded for that slot.  Falling back to NG_SPRITE_MAX_STRIPS
 * (= 32) here was wiping 32 consecutive slots regardless of how wide
 * the char actually was, which stomped on adjacent char/NPC sprite
 * groups whose first_slot was within 32 of the hidden window. */
static void NEOGEO_USER chars_hide_uploaded(uint8_t idx)
{
    uint16_t first;
    uint8_t  strips;

    if (idx >= NG_MAX_CHARS) return;

    first = ng_char_uploaded_first[idx];
    strips = ng_char_uploaded_strips[idx];
    if (first == 0xffffu) return;
    if (strips == 0u) strips = NG_SPRITE_MAX_STRIPS;
    chars_hide_slot(first, strips);
}

static void NEOGEO_USER ng_chars_rebuild_top(void)
{
    uint8_t i = NG_MAX_CHARS;
    while (i > 0u && !ng_chars[i - 1u].active) --i;
    ng_chars_active_top = i;
}

void NEOGEO_USER ng_chars_init(void)
{
    uint8_t i;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        ng_chars[i].active = 0;
        ng_char_uploaded_strips[i] = 0;
        ng_char_uploaded_first[i] = 0xffff;
    }

    for (i = 0; i < NG_MAX_CHAR_KINDS; i++) {
        ng_char_interupts[i] = 0;
    }
    for (i = 0; i < 64u; i++) {
        ng_palette_owner[i] = 0xffu;
    }

    ng_chars_active_top = 0;
    ng_default_arena_id = 0u;
    ng_fixed_updates_per_frame = 1u;
}

NGCharacter* NEOGEO_USER chars_add(uint8_t kind, int16_t x, int16_t y)
{
    uint8_t i;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        NGCharacter *c = &ng_chars[i];

        if (!c->active) {
            c->active = 1;
            c->kind = kind;
            c->state = 0;
            c->facing = 1;
            c->visible = 1;

            c->x = x;
            c->y = y;
            c->x_fp = NG_TO_FP(x);
            c->y_fp = NG_TO_FP(y);
            c->vx_fp = 0;
            c->vy_fp = 0;

            c->action = 0;
            c->action_pos = 0;
            c->action_timer = 0;

            c->sprite_first = 0xffff;
            c->sprite_tile = 0;
            c->sprite_stride = 1;
            c->sprite_strips = 1;
            c->sprite_height = 1;
            c->sprite_active_rows = 1;
            c->palette = 0;
            c->scale_x = NG_SPRITE_FULL_XSCALE;
            c->scale_y = NG_SPRITE_FULL_YSCALE;
            c->flip_x = 0;
            c->flip_y = 0;
            c->sprite_dirty = 1;
            c->priority_band = NG_RENDER_BAND_PLAYER;
            c->depth_offset = 0;
            c->sprite_offset_x = 0;
            c->sprite_offset_y = 0;
            ng_char_uploaded_strips[i] = 0;
            ng_char_uploaded_first[i] = 0xffff;

            c->body_x = 0;
            c->body_y = 0;
            c->body_w = 16;
            c->body_h = 16;

            c->hit_x = 0;
            c->hit_y = 0;
            c->hit_w = 0;
            c->hit_h = 0;

            c->hp = 1;
            c->max_hp = 1;
            c->flags = 0;
            c->data0 = 0;
            c->data1 = 0;
            c->data2 = 0;
            c->asset_tile_start = 0u;
            c->asset_tile_end = 0u;
            c->asset_bounds_enabled = 0u;
            c->life_state = NG_CHAR_LIFE_VISIBLE;
            c->arena_id = ng_default_arena_id;
            c->cull_margin_left = 0;
            c->cull_margin_right = 0;
            c->cull_margin_top = 0;
            c->cull_margin_bottom = 0;
            c->anim_clip = 0;
            c->anim_frame = 0;
            c->anim_timer = 0;

            if ((uint8_t)(i + 1u) > ng_chars_active_top)
                ng_chars_active_top = (uint8_t)(i + 1u);
            return c;
        }
    }

    return 0;
}

void NEOGEO_USER ng_chars_remove(NGCharacter *c)
{
    uint8_t i;

    if (!c) return;

    i = ng_chars_index(c);
    if (i != 0xff) {
        chars_hide_uploaded(i);
        ng_char_uploaded_strips[i] = 0;
        ng_char_uploaded_first[i] = 0xffff;
    } else {
        chars_hide_slot(c->sprite_first,
                        c->sprite_strips ? c->sprite_strips
                                         : (uint8_t)NG_SPRITE_MAX_STRIPS);
    }

    c->active = 0;
    c->life_state = NG_CHAR_LIFE_FREE;
    ng_chars_rebuild_top();
}

void NEOGEO_USER ng_chars_clear_kind(uint8_t kind)
{
    uint8_t i;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        if (ng_chars[i].active && ng_chars[i].kind == kind) {
            chars_hide_uploaded(i);
            ng_char_uploaded_strips[i] = 0;
            ng_char_uploaded_first[i] = 0xffff;
            ng_chars[i].active = 0;
            ng_chars[i].life_state = NG_CHAR_LIFE_FREE;
        }
    }
    ng_chars_rebuild_top();
}

NGCharacter* NEOGEO_USER chars_find(uint8_t kind)
{
    uint8_t i;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        if (ng_chars[i].active && ng_chars[i].kind == kind) return &ng_chars[i];
    }

    return 0;
}

NGCharacter* NEOGEO_USER chars_at(uint8_t index)
{
    if (index >= NG_MAX_CHARS) return 0;
    return &ng_chars[index];
}

uint8_t NEOGEO_USER ng_chars_count(void)
{
    uint8_t i;
    uint8_t count = 0;

    for (i = 0; i < ng_chars_active_top; i++) {
        if (ng_chars[i].active) count++;
    }

    return count;
}

uint8_t NEOGEO_USER ng_chars_index(NGCharacter *c)
{
    uint8_t i;
    if (!c) return 0xff;
    i = (uint8_t)(c - ng_chars);
    if (i >= NG_MAX_CHARS) return 0xff;
    return i;
}

void NEOGEO_USER ng_chars_set_game_interupt(uint8_t kind, NGCharInterupt fn)
{
    if (kind >= NG_MAX_CHAR_KINDS) return;
    ng_char_interupts[kind] = fn;
}

void NEOGEO_USER ng_chars_reset_slot(uint8_t index)
{
    NGCharacter *c;
    if (index >= NG_MAX_CHARS) return;

    c = &ng_chars[index];
    chars_hide_uploaded(index);

    ng_char_uploaded_strips[index] = 0;
    ng_char_uploaded_first[index] = 0xffff;

    c->active = 0;
    c->visible = 0;
    c->sprite_first = 0xffff;
    c->sprite_tile = 0u;
    c->sprite_stride = 1u;
    c->sprite_strips = 1u;
    c->sprite_height = 1u;
    c->sprite_active_rows = 1u;
    c->palette = 0u;
    c->scale_x = NG_SPRITE_FULL_XSCALE;
    c->scale_y = NG_SPRITE_FULL_YSCALE;
    c->flip_x = 0u;
    c->flip_y = 0u;
    c->sprite_dirty = 0u;
    c->sprite_offset_x = 0;
    c->sprite_offset_y = 0;
    c->asset_tile_start = 0u;
    c->asset_tile_end = 0u;
    c->asset_bounds_enabled = 0u;
    c->life_state = NG_CHAR_LIFE_FREE;
    c->arena_id = 0u;
    c->cull_margin_left = 0;
    c->cull_margin_right = 0;
    c->cull_margin_top = 0;
    c->cull_margin_bottom = 0;
    c->anim_clip = 0;
    c->anim_frame = 0;
    c->anim_timer = 0;

    ng_chars_rebuild_top();
}

void NEOGEO_USER ng_chars_begin_scene_arena(uint8_t arena_id)
{
    ng_default_arena_id = arena_id;
}

void NEOGEO_USER ng_chars_clear_arena(uint8_t arena_id)
{
    uint8_t i;
    for (i = 0; i < NG_MAX_CHARS; i++) {
        if (ng_chars[i].active && ng_chars[i].arena_id == arena_id) {
            ng_chars_reset_slot(i);
        }
    }
}

void NEOGEO_USER ng_chars_set_default_arena(uint8_t arena_id)
{
    ng_default_arena_id = arena_id;
}

void NEOGEO_USER ng_chars_set_fixed_step(uint8_t updates_per_frame)
{
    ng_fixed_updates_per_frame = updates_per_frame ? updates_per_frame : 1u;
}

void NEOGEO_USER ng_chars_update_fixed(void)
{
    uint8_t i;
    for (i = 0; i < ng_fixed_updates_per_frame; i++) {
        ng_chars_update();
    }
}

void NEOGEO_USER ng_chars_defrag_slots(void)
{
    uint8_t i;
    for (i = 0; i < ng_chars_active_top; i++) {
        if (!ng_chars[i].active) continue;
        ng_char_uploaded_strips[i] = 0u;
        ng_char_uploaded_first[i] = 0xffff;
        ng_chars[i].sprite_first = 0xffff;
        ng_chars[i].sprite_dirty = 1u;
    }
}

void NEOGEO_USER ng_chars_update(void)
{
    uint8_t i;

    for (i = 0; i < ng_chars_active_top; i++) {
        NGCharacter *c = &ng_chars[i];

        if (!c->active) continue;

        if (c->kind < NG_MAX_CHAR_KINDS && ng_char_interupts[c->kind]) {
            ng_char_interupts[c->kind](c);
        }

        ng_actions_update(c);

        c->x_fp += c->vx_fp;
        c->y_fp += c->vy_fp;
        c->x = NG_FROM_FP(c->x_fp);
        c->y = NG_FROM_FP(c->y_fp);
    }
}

static uint8_t NEOGEO_USER ng_char_render_visible(NGCharacter *c, int16_t camera_x, int16_t camera_y)
{
    int16_t sx;
    int16_t sy;
    int16_t w;
    int16_t h;
    uint8_t strips;
    uint8_t rows;

    if (!c) return 0;
    if (!c->active || !c->visible || c->sprite_first == 0xffff) return 0;

    strips = c->sprite_strips ? c->sprite_strips : 1;
    rows = c->sprite_active_rows ? c->sprite_active_rows : c->sprite_height;
    if (!rows) rows = 1;

    sx = (int16_t)(c->x + c->sprite_offset_x - camera_x);
    sy = (int16_t)(c->y + c->sprite_offset_y - camera_y);
    w = (int16_t)(strips * 16);
    h = (int16_t)(rows * 16);

    if (sx < (int16_t)(NG_SPRITE_CULL_LEFT - w - c->cull_margin_left)) return 0;
    if (sx > (int16_t)(NG_SPRITE_CULL_RIGHT + c->cull_margin_right)) return 0;
    if (sy < (int16_t)(NG_SPRITE_CULL_TOP - h - c->cull_margin_top)) return 0;
    if (sy > (int16_t)(NG_SPRITE_CULL_BOTTOM + c->cull_margin_bottom)) return 0;

    return 1;
}

static int16_t NEOGEO_USER ng_char_sort_y(NGCharacter *c)
{
    return (int16_t)(c->y + c->depth_offset);
}

static uint8_t NEOGEO_USER ng_char_draws_before(NGCharacter *a, NGCharacter *b)
{
    int16_t ay;
    int16_t by;

    if (a->priority_band != b->priority_band) {
        return (uint8_t)(a->priority_band < b->priority_band);
    }

    ay = ng_char_sort_y(a);
    by = ng_char_sort_y(b);

    /* Greater Y = lower on screen = nearer, so it must be assigned later. */
    return (uint8_t)(ay < by);
}

/*
 * Priority/depth sort: build an order[] of active visible character indices.
 * Lower/back priority bands are placed first and receive lower hardware sprite
 * slots. Higher/front bands are assigned later slots, which draw in front.
 * Inside one band, characters are sorted by Y ascending, so lower-on-screen
 * characters draw later.
 * Invisible/offscreen chars are hidden separately.
 */
static uint8_t NEOGEO_USER ng_chars_depth_sort(uint8_t *order, int16_t camera_x, int16_t camera_y)
{
    uint8_t i, j, count = 0;
    uint8_t tmp;

    for (i = 0; i < ng_chars_active_top; i++) {
        NGCharacter *c = &ng_chars[i];
        if (ng_char_render_visible(c, camera_x, camera_y))
            order[count++] = i;
    }

    /* Insertion sort by render band, then Y ascending. */
    for (i = 1; i < count; i++) {
        tmp = order[i];
        j = i;
        while (j > 0 && !ng_char_draws_before(&ng_chars[order[j-1]], &ng_chars[tmp])) {
            order[j] = order[j-1];
            j--;
        }
        order[j] = tmp;
    }

    return count;
}

void NEOGEO_USER ng_chars_draw(void)
{
    uint8_t i;
    uint8_t order[NG_MAX_CHARS];
    uint8_t count;
    uint16_t next_slot;
    const NGLevelState *level = level_state();
    int16_t camera_x = level ? level->scroll_x : 0;
    int16_t camera_y = level ? level->scroll_y : 0;

    /* Hide inactive, invisible or offscreen chars that still have a VRAM slot booked. */
    for (i = 0; i < ng_chars_active_top; i++) {
        NGCharacter *c = &ng_chars[i];
        uint8_t should_draw = ng_char_render_visible(c, camera_x, camera_y);

        if (!should_draw) {
            if (ng_char_uploaded_first[i] != 0xffff) {
                chars_hide_uploaded(i);
                ng_char_uploaded_strips[i] = 0;
                ng_char_uploaded_first[i] = 0xffff;
            }
            if (c->active && c->visible)
                c->sprite_dirty = 1;
        }
    }

    /* Build sorted draw order for active visible on-screen chars. */
    count = ng_chars_depth_sort(order, camera_x, camera_y);

    /*
     * Phase 1 – recompute hardware slot assignments based on sort order.
     * Chars that change slot: hide their old VRAM data now so Phase 2
     * uploads don't conflict with stale slot contents.
     * All hides happen before any uploads to avoid one char's upload
     * overwriting another's stale data mid-frame.
     */
    next_slot = NG_SPR_CHAR_FIRST;
    for (i = 0; i < count; i++) {
        uint8_t idx = order[i];
        NGCharacter *c = &ng_chars[idx];
        uint8_t visibleStrips = c->sprite_strips ? c->sprite_strips : 1;
        if (visibleStrips > NG_SPRITE_MAX_STRIPS) visibleStrips = NG_SPRITE_MAX_STRIPS;

        if ((uint16_t)(next_slot + visibleStrips - 1u) > NG_SPR_CHAR_LAST) {
            break;
        }

        if (c->sprite_first != next_slot) {
            if (ng_char_uploaded_first[idx] != 0xffff) {
                chars_hide_uploaded(idx);
                ng_char_uploaded_strips[idx] = 0;
                ng_char_uploaded_first[idx] = 0xffff;
            }
            c->sprite_first = next_slot;
            c->sprite_dirty = 1;
        }
        next_slot = (uint16_t)(next_slot + visibleStrips);
    }
    {
        uint8_t drawn = i;
        /* Evicted objects must release their previous slots before any
         * newly allocated object is uploaded into those slots. */
        for (; i < count; i++) {
            uint8_t idx = order[i];
            if (ng_char_uploaded_first[idx] != 0xffffu) chars_hide_uploaded(idx);
            ng_char_uploaded_first[idx] = 0xffffu;
            ng_char_uploaded_strips[idx] = 0u;
            ng_chars[idx].sprite_dirty = 1u;
        }
        count = drawn;
    }

    /* Phase 2 – draw each char in depth-sorted order (back-to-front). */
    for (i = 0; i < count; i++) {
        uint8_t idx = order[i];
        NGCharacter *c = &ng_chars[idx];
        NGSpriteGroup g;
        uint8_t visibleStrips;

        visibleStrips = c->sprite_strips ? c->sprite_strips : 1;
        if (visibleStrips > NG_SPRITE_MAX_STRIPS) visibleStrips = NG_SPRITE_MAX_STRIPS;

        ng_sprite_group_init(
            &g,
            c->sprite_first,
            visibleStrips,
            c->sprite_height ? c->sprite_height : 1,
            c->sprite_tile,
            c->palette
        );
        ng_sprite_group_set_tile_stride(&g, c->sprite_stride ? c->sprite_stride : visibleStrips);
        ng_sprite_group_set_active_rows(&g, c->sprite_active_rows ? c->sprite_active_rows : g.heightTiles);
        ng_sprite_group_set_pos(&g,
            (int16_t)(c->x + c->sprite_offset_x - camera_x),
            (int16_t)(c->y + c->sprite_offset_y - camera_y));
        ng_sprite_group_set_scale(&g, c->scale_x, c->scale_y);
        ng_sprite_group_set_flip(&g, c->flip_x, c->flip_y);

        /* Tail clear: only the slots that this char ACTUALLY used
         * last frame and is no longer using.  Previously we wiped
         * the full NG_SPRITE_MAX_STRIPS (=32) window every frame,
         * which clobbered up to 26 unrelated slots and pushed the
         * vblank past its budget — the horizontal-strip / black-
         * box artefacts came from those overruns spilling into
         * active video. */
        if (c->sprite_dirty) {
            uint8_t prev = ng_char_uploaded_strips[idx];
            if (prev > visibleStrips) {
                ng_sprite_hide_range((uint16_t)(c->sprite_first + visibleStrips),
                                     (uint16_t)(prev - visibleStrips));
            }
            ng_sprite_group_upload(&g);
            ng_char_uploaded_strips[idx] = visibleStrips;
            ng_char_uploaded_first[idx]  = c->sprite_first;
            c->sprite_dirty = 0;
        } else {
            uint8_t prev = ng_char_uploaded_strips[idx];
            ng_sprite_group_update_transform(&g);
            if (prev > visibleStrips) {
                ng_sprite_hide_range((uint16_t)(c->sprite_first + visibleStrips),
                                     (uint16_t)(prev - visibleStrips));
            }
        }
    }
}

void NEOGEO_USER ng_char_set_sprite(NGCharacter *c, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette)
{
    uint8_t new_strips;
    uint8_t new_height;

    if (!c) return;

    new_strips = strips ? strips : 1;
    if (new_strips > NG_SPRITE_MAX_STRIPS) new_strips = NG_SPRITE_MAX_STRIPS;
    new_height = heightTiles ? heightTiles : 1;
    if (new_height > NG_SPRITE_MAX_HEIGHT_TILES) new_height = NG_SPRITE_MAX_HEIGHT_TILES;

    if (c->asset_bounds_enabled) {
        uint16_t use_stride = new_strips;
        if (!ng_char_validate_asset_window(tileBase,
                                           new_strips,
                                           new_height,
                                           use_stride,
                                           c->asset_tile_start,
                                           c->asset_tile_end)) {
            return;
        }
    }

    /*
     * Y-depth sorting (ng_chars_draw) owns sprite_first — it reassigns the
     * hardware slot each frame based on Y order.  We only seed sprite_first
     * the very first time so the char enters the sorted list; after that the
     * sorter keeps it up to date.
     *
     * If the strip count changes, the sorter will reallocate slots for the
     * new width on the next draw call (it detects the slot-boundary shift).
     */
    if (c->sprite_first == 0xffff)
        c->sprite_first = firstSprite;   /* initial seed — sorter overrides next frame */

    /* This is a logic-side setter. Reallocation and tail hides belong to
     * ng_chars_draw(), at the caller's controlled VRAM flush point. */

    c->sprite_strips      = new_strips;
    c->sprite_height      = new_height;
    c->sprite_active_rows = new_height;
    c->sprite_tile        = tileBase;
    c->sprite_stride      = new_strips;
    c->palette            = palette;
    c->sprite_dirty       = 1;
    c->life_state = c->visible ? NG_CHAR_LIFE_VISIBLE : NG_CHAR_LIFE_HIDDEN;
}

void NEOGEO_USER ng_char_set_asset_bounds(NGCharacter *c, uint16_t tileStart, uint16_t tileEnd)
{
    if (!c) return;
    if (tileEnd < tileStart) {
        c->asset_bounds_enabled = 0u;
        c->asset_tile_start = 0u;
        c->asset_tile_end = 0u;
        return;
    }

    c->asset_bounds_enabled = 1u;
    c->asset_tile_start = tileStart;
    c->asset_tile_end = tileEnd;
}

uint8_t NEOGEO_USER ng_char_bind_asset(NGCharacter *c, const NGSpriteAssetView *asset)
{
    if (!c || !asset) return 0u;
    ng_char_set_asset_bounds(c, asset->tile_start, asset->tile_end);
    ng_char_set_sprite(c, c->sprite_first,
                       asset->strips,
                       asset->rows,
                       asset->tile_base,
                       asset->palette);
    ng_char_set_tile_stride(c, asset->tile_stride ? asset->tile_stride : asset->strips);
    c->sprite_offset_x = asset->offset_x;
    c->sprite_offset_y = asset->offset_y;
    return 1u;
}

void NEOGEO_USER ng_char_set_cull_margin(NGCharacter *c, int16_t l, int16_t r, int16_t t, int16_t b)
{
    if (!c) return;
    c->cull_margin_left = l;
    c->cull_margin_right = r;
    c->cull_margin_top = t;
    c->cull_margin_bottom = b;
}

void NEOGEO_USER ng_char_set_anim_clip(NGCharacter *c, const NGAnimClip *clip)
{
    if (!c) return;
    c->anim_clip = clip;
    c->anim_frame = 0u;
    c->anim_timer = 0u;
}

void NEOGEO_USER ng_char_anim_update(NGCharacter *c)
{
    if (!c || !c->anim_clip || c->anim_clip->frame_count == 0u) return;
    c->anim_timer++;
    if (c->anim_timer < (c->anim_clip->frame_period ? c->anim_clip->frame_period : 1u)) return;
    c->anim_timer = 0u;
    if ((uint8_t)(c->anim_frame + 1u) >= c->anim_clip->frame_count) {
        if (c->anim_clip->loop) c->anim_frame = 0u;
    } else {
        c->anim_frame++;
    }
}

uint8_t NEOGEO_USER ng_palette_claim(uint8_t palette_slot, uint8_t owner_kind)
{
    if (palette_slot >= 64u) return 0u;
    if (ng_palette_owner[palette_slot] != 0xffu &&
        ng_palette_owner[palette_slot] != owner_kind) return 0u;
    ng_palette_owner[palette_slot] = owner_kind;
    return 1u;
}

void NEOGEO_USER ng_palette_release(uint8_t palette_slot, uint8_t owner_kind)
{
    if (palette_slot >= 64u) return;
    if (ng_palette_owner[palette_slot] == owner_kind) {
        ng_palette_owner[palette_slot] = 0xffu;
    }
}

void NEOGEO_USER ng_char_set_tile_stride(NGCharacter *c, uint16_t stride)
{
    if (!c) return;
    c->sprite_stride = stride ? stride : (uint16_t)c->sprite_strips;
    c->sprite_dirty = 1;
}

void NEOGEO_USER ng_char_set_body(NGCharacter *c, int16_t x, int16_t y, int16_t w, int16_t h)
{
    if (!c) return;
    c->body_x = x;
    c->body_y = y;
    c->body_w = w;
    c->body_h = h;
}

void NEOGEO_USER ng_char_set_pos(NGCharacter *c, int16_t x, int16_t y)
{
    if (!c) return;
    c->x = x;
    c->y = y;
    c->x_fp = NG_TO_FP(x);
    c->y_fp = NG_TO_FP(y);
}

void NEOGEO_USER ng_char_set_speed(NGCharacter *c, int16_t vx_px, int16_t vy_px)
{
    if (!c) return;
    c->vx_fp = NG_TO_FP(vx_px);
    c->vy_fp = NG_TO_FP(vy_px);
}

void NEOGEO_USER ng_char_set_speed_fp(NGCharacter *c, int32_t vx_fp, int32_t vy_fp)
{
    if (!c) return;
    c->vx_fp = vx_fp;
    c->vy_fp = vy_fp;
}

void NEOGEO_USER ng_char_add_speed_fp(NGCharacter *c, int32_t ax_fp, int32_t ay_fp)
{
    if (!c) return;
    c->vx_fp += ax_fp;
    c->vy_fp += ay_fp;
}

void NEOGEO_USER ng_char_set_priority(NGCharacter *c, uint8_t priority_band, int16_t depth_offset)
{
    if (!c) return;
    c->priority_band = priority_band;
    c->depth_offset = depth_offset;
}

void NEOGEO_USER ng_char_damage(NGCharacter *c, uint8_t amount)
{
    if (!c) return;
    if (amount >= c->hp) c->hp = 0;
    else c->hp -= amount;
}

void NEOGEO_USER ng_char_heal(NGCharacter *c, uint8_t amount)
{
    if (!c) return;
    c->hp += amount;
    if (c->hp > c->max_hp) c->hp = c->max_hp;
}

NGRect NEOGEO_USER ng_char_body_rect(NGCharacter *c)
{
    NGRect r = {0, 0, 0, 0};

    if (!c) return r;

    r.x = (int16_t)(c->x + c->body_x);
    r.y = (int16_t)(c->y + c->body_y);
    r.w = c->body_w;
    r.h = c->body_h;

    return r;
}

NGRect NEOGEO_USER ng_char_hit_rect(NGCharacter *c)
{
    NGRect r = {0, 0, 0, 0};

    if (!c) return r;

    r.x = (int16_t)(c->x + c->hit_x);
    r.y = (int16_t)(c->y + c->hit_y);
    r.w = c->hit_w;
    r.h = c->hit_h;

    return r;
}
