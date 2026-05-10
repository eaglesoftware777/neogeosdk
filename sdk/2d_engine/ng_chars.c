#include "ng_chars.h"
#include "ng_actions.h"
#include "ng_level.h"

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

void NEOGEO_USER ng_chars_draw(void)
{
    uint8_t i;
    const NGLevelState *level = level_state();
    int16_t camera_x = level ? level->scroll_x : 0;
    int16_t camera_y = level ? level->scroll_y : 0;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        NGCharacter *c = &ng_chars[i];
        NGSpriteGroup g;
        uint8_t visibleStrips;

        if (!c->active) continue;
        if (c->sprite_first == 0xffff) continue;

        visibleStrips = c->sprite_strips ? c->sprite_strips : 1;
        if (visibleStrips > NG_SPRITE_MAX_STRIPS) visibleStrips = NG_SPRITE_MAX_STRIPS;

        if (!c->visible) {
            if (ng_char_uploaded_strips[i] != 0) {
                chars_hide_slot(ng_char_uploaded_first[i], ng_char_uploaded_strips[i]);
                ng_char_uploaded_strips[i] = 0;
                ng_char_uploaded_first[i] = 0xffff;
            }
            c->sprite_dirty = 1;
            continue;
        }

        if (ng_char_uploaded_first[i] != 0xffff &&
            ng_char_uploaded_first[i] != c->sprite_first) {
            chars_hide_slot(ng_char_uploaded_first[i], ng_char_uploaded_strips[i]);
            ng_char_uploaded_strips[i] = 0;
            ng_char_uploaded_first[i] = 0xffff;
            c->sprite_dirty = 1;
        }

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
        ng_sprite_group_set_pos(&g, (int16_t)(c->x + c->sprite_offset_x - camera_x), (int16_t)(c->y + c->sprite_offset_y - camera_y));
        ng_sprite_group_set_scale(&g, c->scale_x, c->scale_y);
        ng_sprite_group_set_flip(&g, c->flip_x, c->flip_y);

        if (c->sprite_dirty) {
            /*
             * Hardware quirk: do not clear active strips before upload; doing
             * so can flicker.  Only turn off strips that were visible in the
             * previous frame but are no longer used by this narrower frame.
             */
            if (ng_char_uploaded_strips[i] > visibleStrips) {
                ng_sprite_hide_range(
                    c->sprite_first + visibleStrips,
                    (uint8_t)(ng_char_uploaded_strips[i] - visibleStrips)
                );
            }

            ng_sprite_group_upload(&g);
            ng_char_uploaded_strips[i] = visibleStrips;
            ng_char_uploaded_first[i] = c->sprite_first;
            c->sprite_dirty = 0;
        } else {
            ng_sprite_group_update_transform(&g);
        }
    }
}

void NEOGEO_USER ng_char_set_sprite(NGCharacter *c, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette)
{
    if (!c) return;

    if (c->sprite_first != 0xffff && c->sprite_first != firstSprite) {
        uint8_t i = ng_chars_index(c);
        chars_hide_slot(c->sprite_first, NG_SPRITE_MAX_STRIPS);
        if (i != 0xff) {
            ng_char_uploaded_strips[i] = 0;
            ng_char_uploaded_first[i] = 0xffff;
        }
    }

    c->sprite_first = firstSprite;
    c->sprite_strips = strips ? strips : 1;
    if (c->sprite_strips > NG_SPRITE_MAX_STRIPS) c->sprite_strips = NG_SPRITE_MAX_STRIPS;

    c->sprite_height = heightTiles ? heightTiles : 1;
    if (c->sprite_height > NG_SPRITE_MAX_HEIGHT_TILES) c->sprite_height = NG_SPRITE_MAX_HEIGHT_TILES;

    c->sprite_active_rows = c->sprite_height;
    c->sprite_tile = tileBase;
    c->sprite_stride = c->sprite_strips;
    c->palette = palette;
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
