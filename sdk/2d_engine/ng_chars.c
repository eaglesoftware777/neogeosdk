#include "ng_chars.h"
#include "ng_actions.h"
#include "ng_level.h"
#include "ng_sprite_pool.h"

static NGCharacter ng_chars[NG_MAX_CHARS];
static NGCharInterupt ng_char_interupts[NG_MAX_CHAR_KINDS];

static uint8_t ng_char_uploaded_strips[NG_MAX_CHARS];
static uint16_t ng_char_uploaded_first[NG_MAX_CHARS];

static void NEOGEO_USER chars_hide_slot(uint16_t firstSprite, uint8_t strips)
{
    if (firstSprite == 0xffff) return;
    if (strips == 0) strips = NG_SPRITE_MAX_STRIPS;
    if (strips > NG_SPRITE_MAX_STRIPS) strips = NG_SPRITE_MAX_STRIPS;
    ng_sprite_hide_range(firstSprite, strips);
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
    chars_hide_slot(c->sprite_first, NG_SPRITE_MAX_STRIPS);

    if (i != 0xff) {
        ng_char_uploaded_strips[i] = 0;
        ng_char_uploaded_first[i] = 0xffff;
    }

    c->active = 0;
}

void NEOGEO_USER ng_chars_clear_kind(uint8_t kind)
{
    uint8_t i;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        if (ng_chars[i].active && ng_chars[i].kind == kind) {
            chars_hide_slot(ng_chars[i].sprite_first, NG_SPRITE_MAX_STRIPS);
            ng_char_uploaded_strips[i] = 0;
            ng_char_uploaded_first[i] = 0xffff;
            ng_chars[i].active = 0;
        }
    }
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

    for (i = 0; i < NG_MAX_CHARS; i++) {
        if (ng_chars[i].active) count++;
    }

    return count;
}

uint8_t NEOGEO_USER ng_chars_index(NGCharacter *c)
{
    uint8_t i;

    if (!c) return 0xff;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        if (&ng_chars[i] == c) return i;
    }

    return 0xff;
}

void NEOGEO_USER ng_chars_set_game_interupt(uint8_t kind, NGCharInterupt fn)
{
    if (kind >= NG_MAX_CHAR_KINDS) return;
    ng_char_interupts[kind] = fn;
}

void NEOGEO_USER ng_chars_update(void)
{
    uint8_t i;

    for (i = 0; i < NG_MAX_CHARS; i++) {
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

/*
 * Y-depth sort: build an order[] of active visible character indices, sorted
 * by Y descending (highest Y = closest to viewer = lowest slot number = drawn
 * in front).  Returns the count of entries placed in order[].
 * Invisible chars are NOT included; they are hidden separately.
 */
static uint8_t NEOGEO_USER ng_chars_depth_sort(uint8_t *order)
{
    uint8_t i, j, count = 0;
    uint8_t tmp;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        NGCharacter *c = &ng_chars[i];
        if (c->active && c->visible && c->sprite_first != 0xffff)
            order[count++] = i;
    }

    /* Insertion sort by Y descending */
    for (i = 1; i < count; i++) {
        tmp = order[i];
        j = i;
        while (j > 0 && ng_chars[order[j-1]].y < ng_chars[tmp].y) {
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

    /* Hide invisible / inactive chars that still have a VRAM slot booked. */
    for (i = 0; i < NG_MAX_CHARS; i++) {
        NGCharacter *c = &ng_chars[i];
        if (!c->active || !c->visible || c->sprite_first == 0xffff) {
            if (ng_char_uploaded_first[i] != 0xffff) {
                ng_sprite_hide_range(ng_char_uploaded_first[i], ng_char_uploaded_strips[i]);
                ng_char_uploaded_strips[i] = 0;
                ng_char_uploaded_first[i] = 0xffff;
            }
            if (c->active && !c->visible)
                c->sprite_dirty = 1;
        }
    }

    /* Build Y-sorted draw order for active visible chars. */
    count = ng_chars_depth_sort(order);

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
        uint8_t strips = c->sprite_strips ? c->sprite_strips : 1;
        if (strips > NG_SPRITE_MAX_STRIPS) strips = NG_SPRITE_MAX_STRIPS;

        if (c->sprite_first != next_slot) {
            /* Slot changed due to depth-sort reorder: hide stale VRAM. */
            if (ng_char_uploaded_first[idx] != 0xffff) {
                ng_sprite_hide_range(ng_char_uploaded_first[idx],
                                     ng_char_uploaded_strips[idx]);
                ng_char_uploaded_strips[idx] = 0;
                ng_char_uploaded_first[idx] = 0xffff;
            }
            c->sprite_first = next_slot;
            c->sprite_dirty = 1;
        }
        next_slot += strips;
    }

    /* Phase 2 – draw each char in depth-sorted order (front-to-back). */
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

        if (c->sprite_dirty) {
            /*
             * Only hide excess strips (char narrowed this frame) – do NOT
             * clear the active strips first or we flicker.
             */
            if (ng_char_uploaded_strips[idx] > visibleStrips) {
                ng_sprite_hide_range(
                    c->sprite_first + visibleStrips,
                    (uint8_t)(ng_char_uploaded_strips[idx] - visibleStrips)
                );
            }
            ng_sprite_group_upload(&g);
            ng_char_uploaded_strips[idx] = visibleStrips;
            ng_char_uploaded_first[idx]  = c->sprite_first;
            c->sprite_dirty = 0;
        } else {
            ng_sprite_group_update_transform(&g);
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

    if (c->sprite_strips != new_strips) {
        /* Width changed: the old VRAM slot allocation is stale — hide it now
         * so the sorter can repack slots cleanly on the next draw. */
        uint8_t idx = ng_chars_index(c);
        if (idx != 0xff && ng_char_uploaded_first[idx] != 0xffff) {
            ng_sprite_hide_range(ng_char_uploaded_first[idx], ng_char_uploaded_strips[idx]);
            ng_char_uploaded_strips[idx] = 0;
            ng_char_uploaded_first[idx]  = 0xffff;
        }
    }

    c->sprite_strips      = new_strips;
    c->sprite_height      = new_height;
    c->sprite_active_rows = new_height;
    c->sprite_tile        = tileBase;
    c->sprite_stride      = new_strips;
    c->palette            = palette;
    c->sprite_dirty       = 1;
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
