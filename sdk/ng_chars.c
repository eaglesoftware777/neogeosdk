#include "ng_chars.h"
#include "ng_actions.h"
#include "ng_properties.h"

static NGCharacter ng_chars[NG_MAX_CHARS];
static NGCharInterupt ng_char_interupts[NG_MAX_CHAR_KINDS];

void NEOGEO_USER chars_init(void)
{
    uint8_t i;
    for (i = 0; i < NG_MAX_CHARS; i++) {
        ng_chars[i].active = 0;
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

void NEOGEO_USER chars_remove(NGCharacter *c)
{
    if (!c) return;
    c->active = 0;
}

void NEOGEO_USER chars_clear_kind(uint8_t kind)
{
    uint8_t i;
    for (i = 0; i < NG_MAX_CHARS; i++) {
        if (ng_chars[i].active && ng_chars[i].kind == kind) {
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

uint8_t NEOGEO_USER chars_count(void)
{
    uint8_t i, count = 0;
    for (i = 0; i < NG_MAX_CHARS; i++) if (ng_chars[i].active) count++;
    return count;
}

uint8_t NEOGEO_USER chars_index(NGCharacter *c)
{
    uint8_t i;

    if (!c) return 0xff;
    for (i = 0; i < NG_MAX_CHARS; i++) {
        if (&ng_chars[i] == c) return i;
    }
    return 0xff;
}

void NEOGEO_USER chars_set_game_interupt(uint8_t kind, NGCharInterupt fn)
{
    if (kind >= NG_MAX_CHAR_KINDS) return;
    ng_char_interupts[kind] = fn;
}

void NEOGEO_USER chars_update(void)
{
    uint8_t i;
    for (i = 0; i < NG_MAX_CHARS; i++) {
        NGCharacter *c = &ng_chars[i];
        if (!c->active) continue;

        if (c->kind < NG_MAX_CHAR_KINDS && ng_char_interupts[c->kind]) {
            ng_char_interupts[c->kind](c);
        }

        actions_update(c);

        c->x_fp += c->vx_fp;
        c->y_fp += c->vy_fp;
        c->x = NG_FROM_FP(c->x_fp);
        c->y = NG_FROM_FP(c->y_fp);
    }
}

void NEOGEO_USER chars_draw(void)
{
    uint8_t i;
    for (i = 0; i < NG_MAX_CHARS; i++) {
        NGCharacter *c = &ng_chars[i];
        NGSpriteGroup g;
        if (!c->active || !c->visible) continue;
        if (c->sprite_first == 0xffff) continue;

        ngSpriteGroupInit(&g, c->sprite_first, c->sprite_strips, c->sprite_height, c->sprite_tile, c->palette);
        ngSpriteGroupSetTileStride(&g, c->sprite_stride ? c->sprite_stride : c->sprite_strips);
        ngSpriteGroupSetActiveRows(&g, c->sprite_active_rows ? c->sprite_active_rows : c->sprite_height);
        ngSpriteGroupSetPos(&g, c->x, c->y);
        ngSpriteGroupSetScale(&g, c->scale_x, c->scale_y);
        ngSpriteGroupSetFlip(&g, c->flip_x, c->flip_y);

        if (c->sprite_dirty) {
            ngSpriteGroupUpload(&g);
            c->sprite_dirty = 0;
        } else {
            ngSpriteGroupUpdateTransform(&g);
        }
    }
}

void NEOGEO_USER char_set_sprite(NGCharacter *c, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette)
{
    if (!c) return;
    c->sprite_first = firstSprite;
    c->sprite_strips = strips;
    c->sprite_height = heightTiles;
    c->sprite_active_rows = heightTiles;
    c->sprite_tile = tileBase;
    c->sprite_stride = strips;
    c->palette = palette;
    c->sprite_dirty = 1;
}

void NEOGEO_USER char_set_body(NGCharacter *c, int16_t x, int16_t y, int16_t w, int16_t h)
{
    if (!c) return;
    c->body_x = x;
    c->body_y = y;
    c->body_w = w;
    c->body_h = h;
}

void NEOGEO_USER char_set_pos(NGCharacter *c, int16_t x, int16_t y)
{
    if (!c) return;
    c->x = x;
    c->y = y;
    c->x_fp = NG_TO_FP(x);
    c->y_fp = NG_TO_FP(y);
}

void NEOGEO_USER char_set_speed(NGCharacter *c, int16_t vx_px, int16_t vy_px)
{
    if (!c) return;
    c->vx_fp = NG_TO_FP(vx_px);
    c->vy_fp = NG_TO_FP(vy_px);
}

void NEOGEO_USER char_damage(NGCharacter *c, uint8_t amount)
{
    if (!c) return;
    if (amount >= c->hp) c->hp = 0;
    else c->hp -= amount;
}

void NEOGEO_USER char_heal(NGCharacter *c, uint8_t amount)
{
    if (!c) return;
    c->hp += amount;
    if (c->hp > c->max_hp) c->hp = c->max_hp;
}

NGRect NEOGEO_USER char_body_rect(NGCharacter *c)
{
    NGRect r = {0, 0, 0, 0};
    if (!c) return r;
    r.x = (int16_t)(c->x + c->body_x);
    r.y = (int16_t)(c->y + c->body_y);
    r.w = c->body_w;
    r.h = c->body_h;
    return r;
}

NGRect NEOGEO_USER char_hit_rect(NGCharacter *c)
{
    NGRect r = {0, 0, 0, 0};
    if (!c) return r;
    r.x = (int16_t)(c->x + c->hit_x);
    r.y = (int16_t)(c->y + c->hit_y);
    r.w = c->hit_w;
    r.h = c->hit_h;
    return r;
}
