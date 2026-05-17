#include "ng_chars.hpp"
#include "ng_actions.hpp"
#include "ng_level.hpp"
#include "ng_sprite_pool.hpp"

/* --- CharManager singleton --- */

CharManager& CharManager::instance()
{
    static CharManager mgr;
    return mgr;
}

/* --- CharManager private helpers --- */

void CharManager::rebuildTop()
{
    uint8_t i = NG_MAX_CHARS;
    while (i > 0u && !pool[i - 1u].active) --i;
    active_top = i;
}

void CharManager::hideSlot(uint16_t firstSprite, uint8_t strips)
{
    if (firstSprite == 0xffff) return;
    if (strips == 0) strips = NG_SPRITE_MAX_STRIPS;
    if (strips > NG_SPRITE_MAX_STRIPS) strips = NG_SPRITE_MAX_STRIPS;
    NGSpriteGroup::hideRange(firstSprite, strips);
}

uint8_t CharManager::renderVisible(const NGCharacter *c, int16_t cam_x, int16_t cam_y) const
{
    int16_t sx, sy, w, h;
    uint8_t strips, rows;

    if (!c) return 0;
    if (!c->active || !c->visible || c->sprite_first == 0xffff) return 0;

    strips = c->sprite_strips ? c->sprite_strips : 1;
    rows   = c->sprite_active_rows ? c->sprite_active_rows : c->sprite_height;
    if (!rows) rows = 1;

    sx = (int16_t)(c->x + c->sprite_offset_x - cam_x);
    sy = (int16_t)(c->y + c->sprite_offset_y - cam_y);
    w  = (int16_t)(strips * 16);
    h  = (int16_t)(rows * 16);

    if (sx < (int16_t)(NG_SPRITE_CULL_LEFT - w)) return 0;
    if (sx > NG_SPRITE_CULL_RIGHT)                return 0;
    if (sy < (int16_t)(NG_SPRITE_CULL_TOP - h))  return 0;
    if (sy > NG_SPRITE_CULL_BOTTOM)               return 0;

    return 1;
}

int16_t CharManager::sortY(const NGCharacter *c) const
{
    return (int16_t)(c->y + c->depth_offset);
}

uint8_t CharManager::drawsBefore(const NGCharacter *a, const NGCharacter *b) const
{
    if (a->priority_band != b->priority_band)
        return (uint8_t)(a->priority_band > b->priority_band);
    return (uint8_t)(sortY(a) > sortY(b));
}

uint8_t CharManager::depthSort(uint8_t *order, int16_t cam_x, int16_t cam_y) const
{
    uint8_t i, j, tmp, count = 0;

    for (i = 0; i < active_top; i++) {
        if (renderVisible(&pool[i], cam_x, cam_y))
            order[count++] = i;
    }

    for (i = 1; i < count; i++) {
        tmp = order[i];
        j   = i;
        while (j > 0 && !drawsBefore(&pool[order[j-1]], &pool[tmp])) {
            order[j] = order[j-1];
            j--;
        }
        order[j] = tmp;
    }

    return count;
}

/* --- CharManager public methods --- */

void CharManager::init()
{
    uint8_t i;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        pool[i].active        = 0;
        uploaded_strips[i]    = 0;
        uploaded_first[i]     = 0xffff;
    }
    for (i = 0; i < NG_MAX_CHAR_KINDS; i++) {
        interrupts[i] = 0;
    }
    active_top = 0;
}

NGCharacter* CharManager::add(uint8_t kind, int16_t px, int16_t py)
{
    uint8_t i;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        NGCharacter *c = &pool[i];

        if (!c->active) {
            c->active   = 1;
            c->kind     = kind;
            c->state    = 0;
            c->facing   = 1;
            c->visible  = 1;

            c->x    = px;
            c->y    = py;
            c->x_fp = NG_TO_FP(px);
            c->y_fp = NG_TO_FP(py);
            c->vx_fp = 0;
            c->vy_fp = 0;

            c->action       = 0;
            c->action_pos   = 0;
            c->action_timer = 0;

            c->sprite_first       = 0xffff;
            c->sprite_tile        = 0;
            c->sprite_stride      = 1;
            c->sprite_strips      = 1;
            c->sprite_height      = 1;
            c->sprite_active_rows = 1;
            c->palette            = 0;
            c->scale_x            = NG_SPRITE_FULL_XSCALE;
            c->scale_y            = NG_SPRITE_FULL_YSCALE;
            c->flip_x             = 0;
            c->flip_y             = 0;
            c->sprite_dirty       = 1;
            c->priority_band      = NG_RENDER_BAND_PLAYER;
            c->depth_offset       = 0;
            c->sprite_offset_x    = 0;
            c->sprite_offset_y    = 0;

            uploaded_strips[i] = 0;
            uploaded_first[i]  = 0xffff;

            c->body_x = 0; c->body_y = 0;
            c->body_w = 16; c->body_h = 16;
            c->hit_x  = 0; c->hit_y  = 0;
            c->hit_w  = 0; c->hit_h  = 0;

            c->hp     = 1;
            c->max_hp = 1;
            c->flags  = 0;
            c->data0  = 0; c->data1 = 0; c->data2 = 0;

            if ((uint8_t)(i + 1u) > active_top)
                active_top = (uint8_t)(i + 1u);

            return c;
        }
    }
    return 0;
}

void CharManager::remove(NGCharacter *c)
{
    uint8_t i;

    if (!c) return;
    i = indexOf(c);
    hideSlot(c->sprite_first, NG_SPRITE_MAX_STRIPS);

    if (i != 0xff) {
        uploaded_strips[i] = 0;
        uploaded_first[i]  = 0xffff;
    }

    c->active = 0;
    rebuildTop();
}

void CharManager::clearKind(uint8_t kind)
{
    uint8_t i;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        if (pool[i].active && pool[i].kind == kind) {
            hideSlot(pool[i].sprite_first, NG_SPRITE_MAX_STRIPS);
            uploaded_strips[i] = 0;
            uploaded_first[i]  = 0xffff;
            pool[i].active     = 0;
        }
    }
    rebuildTop();
}

NGCharacter* CharManager::find(uint8_t kind) const
{
    uint8_t i;
    for (i = 0; i < NG_MAX_CHARS; i++) {
        if (pool[i].active && pool[i].kind == kind)
            return const_cast<NGCharacter*>(&pool[i]);
    }
    return 0;
}

NGCharacter* CharManager::at(uint8_t index) const
{
    if (index >= NG_MAX_CHARS) return 0;
    return const_cast<NGCharacter*>(&pool[index]);
}

uint8_t CharManager::count() const
{
    uint8_t i, n = 0;
    for (i = 0; i < active_top; i++) {
        if (pool[i].active) n++;
    }
    return n;
}

uint8_t CharManager::indexOf(const NGCharacter *c) const
{
    uint8_t i;
    if (!c) return 0xff;
    i = (uint8_t)(c - pool);
    if (i >= NG_MAX_CHARS) return 0xff;
    return i;
}

void CharManager::setInterrupt(uint8_t kind, NGCharInterupt fn)
{
    if (kind >= NG_MAX_CHAR_KINDS) return;
    interrupts[kind] = fn;
}

void CharManager::clearUploadSlot(uint8_t index)
{
    if (index >= NG_MAX_CHARS) return;
    uploaded_strips[index] = 0;
    uploaded_first[index]  = 0xffff;
}

void CharManager::update()
{
    uint8_t i;

    for (i = 0; i < active_top; i++) {
        NGCharacter *c = &pool[i];

        if (!c->active) continue;

        if (c->kind < NG_MAX_CHAR_KINDS && interrupts[c->kind])
            interrupts[c->kind](c);

        ng_actions_update(c);

        c->x_fp += c->vx_fp;
        c->y_fp += c->vy_fp;
        c->x = NG_FROM_FP(c->x_fp);
        c->y = NG_FROM_FP(c->y_fp);
    }
}

void CharManager::draw()
{
    uint8_t i, order[NG_MAX_CHARS], count;
    uint16_t next_slot;
    const NGLevelState *level = level_state();
    int16_t cam_x = level ? level->scroll_x : 0;
    int16_t cam_y = level ? level->scroll_y : 0;

    /* Hide chars that are inactive, invisible, or off-screen */
    for (i = 0; i < active_top; i++) {
        NGCharacter *c = &pool[i];
        uint8_t should_draw = renderVisible(c, cam_x, cam_y);

        if (!should_draw) {
            if (uploaded_first[i] != 0xffff) {
                NGSpriteGroup::hideRange(uploaded_first[i], uploaded_strips[i]);
                uploaded_strips[i] = 0;
                uploaded_first[i]  = 0xffff;
            }
            if (c->active && c->visible)
                c->sprite_dirty = 1;
        }
    }

    count = depthSort(order, cam_x, cam_y);

    /* Phase 1 — reassign hardware slots by depth order; hide stale data first */
    next_slot = NG_SPR_CHAR_FIRST;
    for (i = 0; i < count; i++) {
        uint8_t idx = order[i];
        NGCharacter *c = &pool[idx];
        uint8_t strips = c->sprite_strips ? c->sprite_strips : 1;
        if (strips > NG_SPRITE_MAX_STRIPS) strips = NG_SPRITE_MAX_STRIPS;

        if (c->sprite_first != next_slot) {
            if (uploaded_first[idx] != 0xffff) {
                NGSpriteGroup::hideRange(uploaded_first[idx], uploaded_strips[idx]);
                uploaded_strips[idx] = 0;
                uploaded_first[idx]  = 0xffff;
            }
            c->sprite_first = next_slot;
            c->sprite_dirty = 1;
        }
        next_slot += strips;
    }

    /* Phase 2 — upload or transform-only update each visible char */
    for (i = 0; i < count; i++) {
        uint8_t idx = order[i];
        NGCharacter *c = &pool[idx];
        NGSpriteGroup g;
        uint8_t vis_strips;

        vis_strips = c->sprite_strips ? c->sprite_strips : 1;
        if (vis_strips > NG_SPRITE_MAX_STRIPS) vis_strips = NG_SPRITE_MAX_STRIPS;

        g.init(c->sprite_first,
               vis_strips,
               c->sprite_height ? c->sprite_height : 1,
               c->sprite_tile,
               c->palette);
        g.setTileStride(c->sprite_stride ? c->sprite_stride : vis_strips);
        g.setActiveRows(c->sprite_active_rows ? c->sprite_active_rows : g.heightTiles);
        g.setPos((int16_t)(c->x + c->sprite_offset_x - cam_x),
                 (int16_t)(c->y + c->sprite_offset_y - cam_y));
        g.setScale(c->scale_x, c->scale_y);
        g.setFlip(c->flip_x, c->flip_y);

        if (c->sprite_dirty) {
            if (uploaded_strips[idx] > vis_strips) {
                NGSpriteGroup::hideRange(c->sprite_first + vis_strips,
                                        (uint8_t)(uploaded_strips[idx] - vis_strips));
            }
            g.upload();
            uploaded_strips[idx] = vis_strips;
            uploaded_first[idx]  = c->sprite_first;
            c->sprite_dirty      = 0;
        } else {
            g.updateTransform();
        }
    }
}

/* --- NGCharacter member methods --- */

void NGCharacter::setPos(int16_t px, int16_t py)
{
    x    = px;    y    = py;
    x_fp = NG_TO_FP(px);
    y_fp = NG_TO_FP(py);
}

void NGCharacter::setSpeed(int16_t vx_px, int16_t vy_px)
{
    vx_fp = NG_TO_FP(vx_px);
    vy_fp = NG_TO_FP(vy_px);
}

void NGCharacter::setSpeedFp(int32_t vx, int32_t vy)
{
    vx_fp = vx;
    vy_fp = vy;
}

void NGCharacter::addSpeedFp(int32_t ax, int32_t ay)
{
    vx_fp += ax;
    vy_fp += ay;
}

void NGCharacter::setSprite(uint16_t first, uint8_t strips_arg, uint8_t h, uint16_t tb, uint8_t pal)
{
    uint8_t ns = strips_arg ? strips_arg : 1;
    uint8_t nh = h          ? h          : 1;
    CharManager& mgr = CharManager::instance();

    if (ns > NG_SPRITE_MAX_STRIPS)      ns = NG_SPRITE_MAX_STRIPS;
    if (nh > NG_SPRITE_MAX_HEIGHT_TILES) nh = NG_SPRITE_MAX_HEIGHT_TILES;

    if (sprite_first == 0xffff)
        sprite_first = first;

    if (sprite_strips != ns) {
        uint8_t idx = mgr.indexOf(this);
        if (idx != 0xff) {
            uint16_t uf = mgr.uploaded_first[idx];
            if (uf != 0xffff) {
                NGSpriteGroup::hideRange(uf, mgr.uploaded_strips[idx]);
                mgr.clearUploadSlot(idx);
            }
        }
    }

    sprite_strips      = ns;
    sprite_height      = nh;
    sprite_active_rows = nh;
    sprite_tile        = tb;
    sprite_stride      = ns;
    palette            = pal;
    sprite_dirty       = 1;
}

void NGCharacter::setTileStride(uint16_t stride)
{
    sprite_stride = stride ? stride : (uint16_t)sprite_strips;
    sprite_dirty  = 1;
}

void NGCharacter::setBody(int16_t bx, int16_t by, int16_t bw, int16_t bh)
{
    body_x = bx; body_y = by;
    body_w = bw; body_h = bh;
}

void NGCharacter::setPriority(uint8_t band, int16_t offset)
{
    priority_band = band;
    depth_offset  = offset;
}

void NGCharacter::damage(uint8_t amount)
{
    if (amount >= hp) hp = 0;
    else hp -= amount;
}

void NGCharacter::heal(uint8_t amount)
{
    hp += amount;
    if (hp > max_hp) hp = max_hp;
}

NGRect NGCharacter::bodyRect() const
{
    NGRect r;
    r.x = (int16_t)(x + body_x);
    r.y = (int16_t)(y + body_y);
    r.w = body_w;
    r.h = body_h;
    return r;
}

NGRect NGCharacter::hitRect() const
{
    NGRect r;
    r.x = (int16_t)(x + hit_x);
    r.y = (int16_t)(y + hit_y);
    r.w = hit_w;
    r.h = hit_h;
    return r;
}

/* --- extern "C" wrappers --- */

extern "C" {

void NEOGEO_USER ng_chars_init(void)
{
    CharManager::instance().init();
}

NGCharacter* NEOGEO_USER chars_add(uint8_t kind, int16_t x, int16_t y)
{
    return CharManager::instance().add(kind, x, y);
}

void NEOGEO_USER ng_chars_remove(NGCharacter *c)
{
    CharManager::instance().remove(c);
}

void NEOGEO_USER ng_chars_clear_kind(uint8_t kind)
{
    CharManager::instance().clearKind(kind);
}

NGCharacter* NEOGEO_USER chars_find(uint8_t kind)
{
    return CharManager::instance().find(kind);
}

NGCharacter* NEOGEO_USER chars_at(uint8_t index)
{
    return CharManager::instance().at(index);
}

uint8_t NEOGEO_USER ng_chars_count(void)
{
    return CharManager::instance().count();
}

uint8_t NEOGEO_USER ng_chars_index(NGCharacter *c)
{
    return CharManager::instance().indexOf(c);
}

void NEOGEO_USER ng_chars_set_game_interupt(uint8_t kind, NGCharInterupt fn)
{
    CharManager::instance().setInterrupt(kind, fn);
}

void NEOGEO_USER ng_chars_update(void)
{
    CharManager::instance().update();
}

void NEOGEO_USER ng_chars_draw(void)
{
    CharManager::instance().draw();
}

void NEOGEO_USER ng_char_set_sprite(NGCharacter *c, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette)
{
    if (c) c->setSprite(firstSprite, strips, heightTiles, tileBase, palette);
}

void NEOGEO_USER ng_char_set_tile_stride(NGCharacter *c, uint16_t stride)
{
    if (c) c->setTileStride(stride);
}

void NEOGEO_USER ng_char_set_body(NGCharacter *c, int16_t x, int16_t y, int16_t w, int16_t h)
{
    if (c) c->setBody(x, y, w, h);
}

void NEOGEO_USER ng_char_set_pos(NGCharacter *c, int16_t x, int16_t y)
{
    if (c) c->setPos(x, y);
}

void NEOGEO_USER ng_char_set_speed(NGCharacter *c, int16_t vx_px, int16_t vy_px)
{
    if (c) c->setSpeed(vx_px, vy_px);
}

void NEOGEO_USER ng_char_set_speed_fp(NGCharacter *c, int32_t vx_fp, int32_t vy_fp)
{
    if (c) c->setSpeedFp(vx_fp, vy_fp);
}

void NEOGEO_USER ng_char_add_speed_fp(NGCharacter *c, int32_t ax_fp, int32_t ay_fp)
{
    if (c) c->addSpeedFp(ax_fp, ay_fp);
}

void NEOGEO_USER ng_char_set_priority(NGCharacter *c, uint8_t priority_band, int16_t depth_offset)
{
    if (c) c->setPriority(priority_band, depth_offset);
}

void NEOGEO_USER ng_char_damage(NGCharacter *c, uint8_t amount)
{
    if (c) c->damage(amount);
}

void NEOGEO_USER ng_char_heal(NGCharacter *c, uint8_t amount)
{
    if (c) c->heal(amount);
}

NGRect NEOGEO_USER ng_char_body_rect(NGCharacter *c)
{
    NGRect r = {0, 0, 0, 0};
    if (c) r = c->bodyRect();
    return r;
}

NGRect NEOGEO_USER ng_char_hit_rect(NGCharacter *c)
{
    NGRect r = {0, 0, 0, 0};
    if (c) r = c->hitRect();
    return r;
}

} /* extern "C" */
