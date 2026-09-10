#include "ng_chars.hpp"
#include "ng_actions.hpp"
#include "ng_level.hpp"
#include "ng_sprite_pool.hpp"

static void NEOGEO_USER ng_char_reset_fields(NGCharacter *c);
static uint8_t ng_palette_owner[64];
static uint8_t ng_default_arena_id;
static uint8_t ng_fixed_updates_per_frame = 1u;

static uint8_t NEOGEO_USER ng_char_validate_asset_window_impl(uint16_t tileBase,
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

void CharManager::hideUploaded(uint8_t idx)
{
    uint16_t first;
    uint8_t  strips;

    if (idx >= NG_MAX_CHARS) return;
    first = uploaded_first[idx];
    strips = uploaded_strips[idx];
    if (first == 0xffffu) return;
    if (strips == 0u) strips = NG_SPRITE_MAX_STRIPS;
    hideSlot(first, strips);
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

    if (sx < (int16_t)(NG_SPRITE_CULL_LEFT - w - c->cull_margin_left)) return 0;
    if (sx > (int16_t)(NG_SPRITE_CULL_RIGHT + c->cull_margin_right)) return 0;
    if (sy < (int16_t)(NG_SPRITE_CULL_TOP - h - c->cull_margin_top)) return 0;
    if (sy > (int16_t)(NG_SPRITE_CULL_BOTTOM + c->cull_margin_bottom)) return 0;

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
        pool[i].sprite_palette_map = 0;
        uploaded_strips[i]    = 0;
        uploaded_first[i]     = 0xffff;
    }
    for (i = 0; i < NG_MAX_CHAR_KINDS; i++) {
        interrupts[i] = 0;
    }
    for (i = 0; i < 64u; i++) {
        ng_palette_owner[i] = 0xffu;
    }
    active_top = 0;
    ng_default_arena_id = 0u;
    ng_fixed_updates_per_frame = 1u;
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
            c->sprite_palette_map = 0;

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
    if (i != 0xff) {
        hideUploaded(i);
        uploaded_strips[i] = 0;
        uploaded_first[i]  = 0xffff;
    } else {
        hideSlot(c->sprite_first,
                 c->sprite_strips ? c->sprite_strips
                                  : (uint8_t)NG_SPRITE_MAX_STRIPS);
    }

    c->active = 0;
    c->life_state = NG_CHAR_LIFE_FREE;
    rebuildTop();
}

void CharManager::clearKind(uint8_t kind)
{
    uint8_t i;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        if (pool[i].active && pool[i].kind == kind) {
            hideUploaded(i);
            uploaded_strips[i] = 0;
            uploaded_first[i]  = 0xffff;
            pool[i].active     = 0;
            pool[i].life_state = NG_CHAR_LIFE_FREE;
        }
    }
    rebuildTop();
}

void CharManager::resetSlot(uint8_t index)
{
    NGCharacter *c;
    if (index >= NG_MAX_CHARS) return;
    c = &pool[index];

    hideUploaded(index);
    clearUploadSlot(index);
    ng_char_reset_fields(c);
    rebuildTop();
}

void CharManager::clearArena(uint8_t arena_id)
{
    uint8_t i;
    for (i = 0; i < NG_MAX_CHARS; i++) {
        if (pool[i].active && pool[i].arena_id == arena_id) {
            resetSlot(i);
        }
    }
}

void CharManager::setDefaultArena(uint8_t arena_id)
{
    ng_default_arena_id = arena_id;
}

void CharManager::setFixedStep(uint8_t updates_per_frame)
{
    ng_fixed_updates_per_frame = updates_per_frame ? updates_per_frame : 1u;
}

void CharManager::updateFixed()
{
    uint8_t i;
    for (i = 0; i < ng_fixed_updates_per_frame; i++) {
        update();
    }
}

void CharManager::defragSlots()
{
    uint8_t i;
    for (i = 0; i < active_top; i++) {
        if (!pool[i].active) continue;
        uploaded_strips[i] = 0u;
        uploaded_first[i] = 0xffff;
        pool[i].sprite_first = 0xffff;
        pool[i].sprite_dirty = 1u;
    }
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

static void NEOGEO_USER ng_char_reset_fields(NGCharacter *c)
{
    c->active = 0u;
    c->visible = 0u;
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
    c->sprite_palette_map = 0;
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
                hideUploaded(i);
                uploaded_strips[i] = 0;
                uploaded_first[i]  = 0xffff;
                /* Releasing the slots means the next draw must upload
                 * again, whatever the reason for hiding - a char that
                 * blinked off otherwise returns through the
                 * transform-only path with no map and no chain bits. */
                c->sprite_dirty = 1;
            }
        }
    }

    count = depthSort(order, cam_x, cam_y);

    /* Phase 1 — reassign hardware slots by depth order; hide stale
     * data first.  Pack slots tightly by each char's actual visible
     * strip count instead of striding by NG_SPRITE_MAX_STRIPS (=32)
     * which capped the engine at 4 chars and wasted 28 slots each. */
    next_slot = NG_SPR_CHAR_FIRST;
    for (i = 0; i < count; i++) {
        uint8_t idx = order[i];
        NGCharacter *c = &pool[idx];
        uint8_t vis_strips = c->sprite_strips ? c->sprite_strips : 1u;
        if (vis_strips > NG_SPRITE_MAX_STRIPS) vis_strips = NG_SPRITE_MAX_STRIPS;

        if ((uint16_t)(next_slot + vis_strips - 1u) > NG_SPR_CHAR_LAST) {
            break;
        }

        if (c->sprite_first != next_slot) {
            if (uploaded_first[idx] != 0xffff) {
                hideUploaded(idx);
                uploaded_strips[idx] = 0;
                uploaded_first[idx]  = 0xffff;
            }
            c->sprite_first = next_slot;
            c->sprite_dirty = 1;
        }
        next_slot = (uint16_t)(next_slot + vis_strips);
    }
    {
        uint8_t drawn = i;
        for (; i < count; i++) {
            uint8_t idx = order[i];
            if (uploaded_first[idx] != 0xffffu) hideUploaded(idx);
            uploaded_first[idx] = 0xffffu;
            uploaded_strips[idx] = 0u;
            pool[idx].sprite_dirty = 1u;
        }
        count = drawn;
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
        g.setPaletteMap(c->sprite_palette_map);
        g.setActiveRows(c->sprite_active_rows ? c->sprite_active_rows : g.heightTiles);
        g.setPos((int16_t)(c->x + c->sprite_offset_x - cam_x),
                 (int16_t)(c->y + c->sprite_offset_y - cam_y));
        g.setScale(c->scale_x, c->scale_y);
        g.setFlip(c->flip_x, c->flip_y);

        /* Tail clear bounded by the slots this char actually used
         * last frame.  See the C engine companion for the budget
         * rationale — wiping 32 slots every frame overruns vblank. */
        if (c->sprite_dirty) {
            uint8_t prev = uploaded_strips[idx];
            if (prev > vis_strips) {
                NGSpriteGroup::hideRange((uint16_t)(c->sprite_first + vis_strips),
                                        (uint16_t)(prev - vis_strips));
            }
            g.upload();
            uploaded_strips[idx] = vis_strips;
            uploaded_first[idx]  = c->sprite_first;
            c->sprite_dirty      = 0;
        } else {
            uint8_t prev = uploaded_strips[idx];
            g.updateTransform();
            if (prev > vis_strips) {
                NGSpriteGroup::hideRange((uint16_t)(c->sprite_first + vis_strips),
                                        (uint16_t)(prev - vis_strips));
            }
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

    if (ns > NG_SPRITE_MAX_STRIPS)      ns = NG_SPRITE_MAX_STRIPS;
    if (nh > NG_SPRITE_MAX_HEIGHT_TILES) nh = NG_SPRITE_MAX_HEIGHT_TILES;
    if (asset_bounds_enabled) {
        /* Validate against the stride this char will be drawn with, not
         * its strip count.  The two match only for an asset that fills
         * its canvas; every other one has its rows tile_stride apart, so
         * checking with the strip count under-measures the window and
         * lets through exactly the bind this is here to catch. */
        uint16_t use_stride = sprite_stride ? sprite_stride : (uint16_t)ns;
        if (use_stride < ns) use_stride = ns;
        if (!ng_char_validate_asset_window_impl(tb,
                                                ns,
                                                nh,
                                                use_stride,
                                                asset_tile_start,
                                                asset_tile_end)) {
            return;
        }
    }

    if (sprite_first == 0xffff)
        sprite_first = first;

    /* CharManager::draw owns all hardware changes during the frame flush. */

    sprite_strips      = ns;
    sprite_height      = nh;
    sprite_active_rows = nh;
    sprite_tile        = tb;
    sprite_stride      = ns;
    palette            = pal;
    sprite_palette_map = 0;
    sprite_dirty       = 1;
    life_state = visible ? NG_CHAR_LIFE_VISIBLE : NG_CHAR_LIFE_HIDDEN;
}

void NGCharacter::setPaletteMap(const uint8_t *banks)
{
    if (sprite_palette_map != banks) {
        sprite_palette_map = banks;
        sprite_dirty = 1u;
    }
}

void NGCharacter::setAssetBounds(uint16_t tileStart, uint16_t tileEnd)
{
    if (tileEnd < tileStart) {
        asset_bounds_enabled = 0u;
        asset_tile_start = 0u;
        asset_tile_end = 0u;
        return;
    }

    asset_bounds_enabled = 1u;
    asset_tile_start = tileStart;
    asset_tile_end = tileEnd;
}

uint8_t NGCharacter::bindAsset(const NGSpriteAssetView *asset)
{
    if (!asset) return 0u;
    uint16_t stride = asset->tile_stride ? asset->tile_stride : asset->strips;
    if (!ng_char_validate_asset_window_impl(asset->tile_base, asset->strips,
            asset->rows, stride, asset->tile_start, asset->tile_end)) return 0u;
    setAssetBounds(asset->tile_start, asset->tile_end);
    sprite_stride = stride;
    setSprite(sprite_first, asset->strips, asset->rows, asset->tile_base, asset->palette);
    setTileStride(asset->tile_stride ? asset->tile_stride : asset->strips);
    setPaletteMap(asset->tile_palettes);
    sprite_offset_x = asset->offset_x;
    sprite_offset_y = asset->offset_y;
    return 1u;
}

void NGCharacter::setCullMargin(int16_t l, int16_t r, int16_t t, int16_t b)
{
    cull_margin_left = l;
    cull_margin_right = r;
    cull_margin_top = t;
    cull_margin_bottom = b;
}

void NGCharacter::setAnimClip(const NGAnimClip *clip)
{
    anim_clip = clip;
    anim_frame = 0u;
    anim_timer = 0u;
}

void NGCharacter::animUpdate()
{
    if (!anim_clip || anim_clip->frame_count == 0u) return;
    anim_timer++;
    if (anim_timer < (anim_clip->frame_period ? anim_clip->frame_period : 1u)) return;
    anim_timer = 0u;
    if ((uint8_t)(anim_frame + 1u) >= anim_clip->frame_count) {
        if (anim_clip->loop) anim_frame = 0u;
    } else {
        anim_frame++;
    }
}

/*
 * How many tiles apart the artwork's rows are - the asset's canvas width
 * in tiles, which is not always 16.  An asset imported onto a narrower
 * canvas has a narrower stride, and binding it with 16 reads every row
 * after the first from further along the C ROM than the artwork is.
 * Take it from the asset metadata rather than assuming.
 *
 * Callers normally set it after setSprite(), so this re-checks the
 * window the pair now describes and refuses a stride that would walk
 * the sprite past its asset.
 */
void NGCharacter::setTileStride(uint16_t stride)
{
    uint16_t use_stride = stride ? stride : (uint16_t)sprite_strips;
    if (use_stride < sprite_strips) use_stride = sprite_strips;

    if (asset_bounds_enabled &&
        !ng_char_validate_asset_window_impl(sprite_tile,
                                            sprite_strips,
                                            sprite_height,
                                            use_stride,
                                            asset_tile_start,
                                            asset_tile_end)) {
        return;
    }

    sprite_stride = use_stride;
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

uint8_t NEOGEO_USER ng_char_validate_asset_window(uint16_t tileBase,
                                                  uint8_t strips,
                                                  uint8_t rows,
                                                  uint16_t stride,
                                                  uint16_t tileStart,
                                                  uint16_t tileEnd)
{
    return ng_char_validate_asset_window_impl(tileBase, strips, rows, stride, tileStart, tileEnd);
}

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

void NEOGEO_USER ng_chars_reset_slot(uint8_t index)
{
    CharManager::instance().resetSlot(index);
}

void NEOGEO_USER ng_chars_begin_scene_arena(uint8_t arena_id)
{
    CharManager::instance().setDefaultArena(arena_id);
}

void NEOGEO_USER ng_chars_clear_arena(uint8_t arena_id)
{
    CharManager::instance().clearArena(arena_id);
}

void NEOGEO_USER ng_chars_set_default_arena(uint8_t arena_id)
{
    CharManager::instance().setDefaultArena(arena_id);
}

void NEOGEO_USER ng_chars_set_fixed_step(uint8_t updates_per_frame)
{
    CharManager::instance().setFixedStep(updates_per_frame);
}

void NEOGEO_USER ng_chars_update_fixed(void)
{
    CharManager::instance().updateFixed();
}

void NEOGEO_USER ng_chars_defrag_slots(void)
{
    CharManager::instance().defragSlots();
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

void NEOGEO_USER ng_char_set_palette_map(NGCharacter *c, const uint8_t *banks)
{
    if (c) c->setPaletteMap(banks);
}

void NEOGEO_USER ng_char_set_asset_bounds(NGCharacter *c, uint16_t tileStart, uint16_t tileEnd)
{
    if (c) c->setAssetBounds(tileStart, tileEnd);
}

uint8_t NEOGEO_USER ng_char_bind_asset(NGCharacter *c, const NGSpriteAssetView *asset)
{
    if (!c) return 0u;
    return c->bindAsset(asset);
}

void NEOGEO_USER ng_char_set_cull_margin(NGCharacter *c, int16_t l, int16_t r, int16_t t, int16_t b)
{
    if (c) c->setCullMargin(l, r, t, b);
}

void NEOGEO_USER ng_char_set_anim_clip(NGCharacter *c, const NGAnimClip *clip)
{
    if (c) c->setAnimClip(clip);
}

void NEOGEO_USER ng_char_anim_update(NGCharacter *c)
{
    if (c) c->animUpdate();
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

} /* extern "C" */
