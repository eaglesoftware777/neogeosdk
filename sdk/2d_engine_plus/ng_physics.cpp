#include "ng_physics.hpp"
#include "ng_properties.hpp"

/* --- PhysicsWorld singleton --- */

PhysicsWorld& PhysicsWorld::instance()
{
    static PhysicsWorld pw;
    return pw;
}

/* --- PhysicsWorld private helpers --- */

void PhysicsWorld::applyDrag(int32_t *v, int32_t drag)
{
    if (*v > 0) { *v -= drag; if (*v < 0) *v = 0; }
    else if (*v < 0) { *v += drag; if (*v > 0) *v = 0; }
}

void PhysicsWorld::syncPos(NGCharacter *c, int16_t x, int16_t y)
{
    c->x = x; c->y = y;
    c->x_fp = NG_TO_FP(x);
    c->y_fp = NG_TO_FP(y);
}

int16_t PhysicsWorld::minI16(int16_t a, int16_t b)
{
    return (a < b) ? a : b;
}

void PhysicsWorld::resolveWorld(NGCharacter *c, NGPhysicsBody *body)
{
    int16_t left   = (int16_t)ng_prop_get(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_LEFT);
    int16_t top    = (int16_t)ng_prop_get(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_TOP);
    int16_t right  = (int16_t)ng_prop_get(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_RIGHT);
    int16_t bottom = (int16_t)ng_prop_get(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_BOTTOM);
    int16_t bx = c->body_x, by = c->body_y;
    int16_t bw = c->body_w ? c->body_w : 16;
    int16_t bh = c->body_h ? c->body_h : 16;
    int16_t min_x = (int16_t)(left  - bx);
    int16_t max_x = (int16_t)(right - bx - bw);
    int16_t min_y = (int16_t)(top   - by);
    int16_t max_y = (int16_t)(bottom - by - bh);

    if (right <= left || bottom <= top) return;

    if (c->x < min_x)      { syncPos(c, min_x, c->y); c->vx_fp = 0; }
    else if (c->x > max_x) { syncPos(c, max_x, c->y); c->vx_fp = 0; }

    if (c->y < min_y)      { syncPos(c, c->x, min_y); c->vy_fp = 0; }
    else if (c->y > max_y) { syncPos(c, c->x, max_y); c->vy_fp = 0; body->grounded = 1; }
}

void PhysicsWorld::resolveSolid(NGCharacter *c, NGPhysicsBody *body, const NGSolidRect *solid)
{
    NGRect old_rect, new_rect, sr;
    int16_t ox, oy;

    old_rect.x = (int16_t)(body->prev_x + c->body_x);
    old_rect.y = (int16_t)(body->prev_y + c->body_y);
    old_rect.w = c->body_w ? c->body_w : 16;
    old_rect.h = c->body_h ? c->body_h : 16;

    new_rect = ng_char_body_rect(c);
    sr.x = solid->x; sr.y = solid->y;
    sr.w = solid->w; sr.h = solid->h;

    if (!ng_rect_hit(new_rect, sr)) return;

    if ((int16_t)(old_rect.y + old_rect.h) <= sr.y && c->vy_fp >= 0) {
        syncPos(c, c->x, (int16_t)(sr.y - c->body_y - new_rect.h));
        c->vy_fp = 0; body->grounded = 1; return;
    }
    if (old_rect.y >= (int16_t)(sr.y + sr.h) && c->vy_fp <= 0) {
        syncPos(c, c->x, (int16_t)(sr.y + sr.h - c->body_y));
        c->vy_fp = 0; return;
    }
    if ((int16_t)(old_rect.x + old_rect.w) <= sr.x && c->vx_fp >= 0) {
        syncPos(c, (int16_t)(sr.x - c->body_x - new_rect.w), c->y);
        c->vx_fp = 0; return;
    }
    if (old_rect.x >= (int16_t)(sr.x + sr.w) && c->vx_fp <= 0) {
        syncPos(c, (int16_t)(sr.x + sr.w - c->body_x), c->y);
        c->vx_fp = 0; return;
    }

    ox = minI16((int16_t)(new_rect.x + new_rect.w - sr.x),
                (int16_t)(sr.x + sr.w - new_rect.x));
    oy = minI16((int16_t)(new_rect.y + new_rect.h - sr.y),
                (int16_t)(sr.y + sr.h - new_rect.y));

    if (ox < oy) {
        if (new_rect.x < sr.x) syncPos(c, (int16_t)(sr.x - c->body_x - new_rect.w), c->y);
        else                   syncPos(c, (int16_t)(sr.x + sr.w - c->body_x), c->y);
        c->vx_fp = 0;
    } else {
        if (new_rect.y < sr.y) { syncPos(c, c->x, (int16_t)(sr.y - c->body_y - new_rect.h)); body->grounded = 1; }
        else                    syncPos(c, c->x, (int16_t)(sr.y + sr.h - c->body_y));
        c->vy_fp = 0;
    }
}

/* --- PhysicsWorld public methods --- */

void PhysicsWorld::init()
{
    uint8_t i;
    for (i = 0; i < NG_MAX_CHARS; i++) {
        bodies[i].enabled    = 0;
        bodies[i].grounded   = 0;
        bodies[i].flags      = 0;
        bodies[i].prev_x     = 0;
        bodies[i].prev_y     = 0;
        bodies[i].gravity_fp  = NG_FP_FROM_FRAC(1, 4);
        bodies[i].max_fall_fp = NG_TO_FP(4);
        bodies[i].drag_x_fp   = NG_FP_FROM_FRAC(1, 8);
        bodies[i].drag_y_fp   = 0;
    }
    solid_count = 0;
}

void PhysicsWorld::attach(NGCharacter *c, uint16_t flags)
{
    uint8_t idx = ng_chars_index(c);
    if (idx == 0xff) return;
    bodies[idx].enabled  = 1;
    bodies[idx].grounded = 0;
    bodies[idx].flags    = flags;
}

void PhysicsWorld::detach(NGCharacter *c)
{
    uint8_t idx = ng_chars_index(c);
    if (idx == 0xff) return;
    bodies[idx].enabled = 0;
}

NGPhysicsBody* PhysicsWorld::body(NGCharacter *c)
{
    uint8_t idx = ng_chars_index(c);
    if (idx == 0xff) return 0;
    return &bodies[idx];
}

void PhysicsWorld::setGravity(NGCharacter *c, int32_t gravity_fp, int32_t max_fall_fp)
{
    NGPhysicsBody *b = body(c);
    if (!b) return;
    b->gravity_fp  = gravity_fp;
    b->max_fall_fp = max_fall_fp;
}

void PhysicsWorld::setDrag(NGCharacter *c, int32_t drag_x_fp, int32_t drag_y_fp)
{
    NGPhysicsBody *b = body(c);
    if (!b) return;
    b->drag_x_fp = drag_x_fp;
    b->drag_y_fp = drag_y_fp;
}

void PhysicsWorld::clearSolids()
{
    solid_count = 0;
}

uint8_t PhysicsWorld::addSolid(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t flags)
{
    if (solid_count >= NG_MAX_SOLIDS) return 0xff;
    solids[solid_count] = { x, y, w, h, flags };
    return (uint8_t)(solid_count++);
}

void PhysicsWorld::updatePre()
{
    uint8_t i;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        NGCharacter *c   = chars_at(i);
        NGPhysicsBody *b = &bodies[i];

        if (!b->enabled || !c || !c->active) continue;

        b->prev_x  = c->x;
        b->prev_y  = c->y;
        b->grounded = 0;

        if (b->flags & NG_PHYSICS_GRAVITY) {
            c->vy_fp += b->gravity_fp;
            if (c->vy_fp > b->max_fall_fp) c->vy_fp = b->max_fall_fp;
        }
        if (b->flags & NG_PHYSICS_DRAG_X) applyDrag(&c->vx_fp, b->drag_x_fp);
        if (b->flags & NG_PHYSICS_DRAG_Y) applyDrag(&c->vy_fp, b->drag_y_fp);
    }
}

void PhysicsWorld::resolve()
{
    uint8_t i, s;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        NGCharacter *c   = chars_at(i);
        NGPhysicsBody *b = &bodies[i];

        if (!b->enabled || !c || !c->active) continue;

        if (b->flags & NG_PHYSICS_WORLD)  resolveWorld(c, b);
        if (b->flags & NG_PHYSICS_SOLIDS) {
            for (s = 0; s < solid_count; s++) resolveSolid(c, b, &solids[s]);
        }
    }
}

uint8_t PhysicsWorld::isGrounded(NGCharacter *c)
{
    NGPhysicsBody *b = body(c);
    return b ? b->grounded : 0;
}

/* --- extern "C" wrappers --- */

extern "C" {

void NEOGEO_USER ng_physics_init(void)
{
    PhysicsWorld::instance().init();
}

void NEOGEO_USER ng_physics_attach(NGCharacter *c, uint16_t flags)
{
    PhysicsWorld::instance().attach(c, flags);
}

void NEOGEO_USER ng_physics_detach(NGCharacter *c)
{
    PhysicsWorld::instance().detach(c);
}

NGPhysicsBody* physics_body(NGCharacter *c)
{
    return PhysicsWorld::instance().body(c);
}

void NEOGEO_USER ng_physics_set_gravity(NGCharacter *c, int32_t gravity_fp, int32_t max_fall_fp)
{
    PhysicsWorld::instance().setGravity(c, gravity_fp, max_fall_fp);
}

void NEOGEO_USER ng_physics_set_drag(NGCharacter *c, int32_t drag_x_fp, int32_t drag_y_fp)
{
    PhysicsWorld::instance().setDrag(c, drag_x_fp, drag_y_fp);
}

void NEOGEO_USER ng_physics_clear_solids(void)
{
    PhysicsWorld::instance().clearSolids();
}

uint8_t NEOGEO_USER ng_physics_add_solid(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t flags)
{
    return PhysicsWorld::instance().addSolid(x, y, w, h, flags);
}

void NEOGEO_USER ng_physics_update_pre(void)
{
    PhysicsWorld::instance().updatePre();
}

void NEOGEO_USER ng_physics_resolve(void)
{
    PhysicsWorld::instance().resolve();
}

uint8_t NEOGEO_USER ng_physics_is_grounded(NGCharacter *c)
{
    return PhysicsWorld::instance().isGrounded(c);
}

} /* extern "C" */
