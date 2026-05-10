#include "ng_physics.h"
#include "ng_properties.h"

static NGPhysicsBody ng_physics_bodies[NG_MAX_CHARS];
static NGSolidRect ng_solids[NG_MAX_SOLIDS];
static uint8_t ng_solid_count;

static void ng_physics_sync_position(NGCharacter *c, int16_t x, int16_t y)
{
    c->x = x;
    c->y = y;
    c->x_fp = NG_TO_FP(x);
    c->y_fp = NG_TO_FP(y);
}

static int16_t ng_physics_min_i16(int16_t a, int16_t b)
{
    return (a < b) ? a : b;
}

static void ng_physics_apply_drag(int32_t *v, int32_t drag)
{
    if (*v > 0) {
        *v -= drag;
        if (*v < 0) *v = 0;
    } else if (*v < 0) {
        *v += drag;
        if (*v > 0) *v = 0;
    }
}

static void ng_physics_resolve_world(NGCharacter *c, NGPhysicsBody *body)
{
    int16_t left = (int16_t)prop_get(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_LEFT);
    int16_t top = (int16_t)prop_get(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_TOP);
    int16_t right = (int16_t)prop_get(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_RIGHT);
    int16_t bottom = (int16_t)prop_get(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_BOTTOM);
    int16_t body_x = c->body_x;
    int16_t body_y = c->body_y;
    int16_t body_w = c->body_w ? c->body_w : 16;
    int16_t body_h = c->body_h ? c->body_h : 16;
    int16_t min_x = (int16_t)(left - body_x);
    int16_t max_x = (int16_t)(right - body_x - body_w);
    int16_t min_y = (int16_t)(top - body_y);
    int16_t max_y = (int16_t)(bottom - body_y - body_h);

    if (right <= left || bottom <= top) return;

    if (c->x < min_x) {
        ng_physics_sync_position(c, min_x, c->y);
        c->vx_fp = 0;
    } else if (c->x > max_x) {
        ng_physics_sync_position(c, max_x, c->y);
        c->vx_fp = 0;
    }

    if (c->y < min_y) {
        ng_physics_sync_position(c, c->x, min_y);
        c->vy_fp = 0;
    } else if (c->y > max_y) {
        ng_physics_sync_position(c, c->x, max_y);
        c->vy_fp = 0;
        body->grounded = 1;
    }
}

static void ng_physics_resolve_solid(NGCharacter *c, NGPhysicsBody *body, const NGSolidRect *solid)
{
    NGRect old_rect;
    NGRect new_rect;
    NGRect solid_rect;
    int16_t overlap_x;
    int16_t overlap_y;

    old_rect.x = (int16_t)(body->prev_x + c->body_x);
    old_rect.y = (int16_t)(body->prev_y + c->body_y);
    old_rect.w = c->body_w ? c->body_w : 16;
    old_rect.h = c->body_h ? c->body_h : 16;

    new_rect = char_body_rect(c);
    solid_rect.x = solid->x;
    solid_rect.y = solid->y;
    solid_rect.w = solid->w;
    solid_rect.h = solid->h;

    if (!ng_rect_hit(new_rect, solid_rect)) return;

    if ((int16_t)(old_rect.y + old_rect.h) <= solid_rect.y && c->vy_fp >= 0) {
        ng_physics_sync_position(c, c->x, (int16_t)(solid_rect.y - c->body_y - new_rect.h));
        c->vy_fp = 0;
        body->grounded = 1;
        return;
    }
    if (old_rect.y >= (int16_t)(solid_rect.y + solid_rect.h) && c->vy_fp <= 0) {
        ng_physics_sync_position(c, c->x, (int16_t)(solid_rect.y + solid_rect.h - c->body_y));
        c->vy_fp = 0;
        return;
    }
    if ((int16_t)(old_rect.x + old_rect.w) <= solid_rect.x && c->vx_fp >= 0) {
        ng_physics_sync_position(c, (int16_t)(solid_rect.x - c->body_x - new_rect.w), c->y);
        c->vx_fp = 0;
        return;
    }
    if (old_rect.x >= (int16_t)(solid_rect.x + solid_rect.w) && c->vx_fp <= 0) {
        ng_physics_sync_position(c, (int16_t)(solid_rect.x + solid_rect.w - c->body_x), c->y);
        c->vx_fp = 0;
        return;
    }

    overlap_x = ng_physics_min_i16(
        (int16_t)(new_rect.x + new_rect.w - solid_rect.x),
        (int16_t)(solid_rect.x + solid_rect.w - new_rect.x)
    );
    overlap_y = ng_physics_min_i16(
        (int16_t)(new_rect.y + new_rect.h - solid_rect.y),
        (int16_t)(solid_rect.y + solid_rect.h - new_rect.y)
    );

    if (overlap_x < overlap_y) {
        if (new_rect.x < solid_rect.x) {
            ng_physics_sync_position(c, (int16_t)(solid_rect.x - c->body_x - new_rect.w), c->y);
        } else {
            ng_physics_sync_position(c, (int16_t)(solid_rect.x + solid_rect.w - c->body_x), c->y);
        }
        c->vx_fp = 0;
    } else {
        if (new_rect.y < solid_rect.y) {
            ng_physics_sync_position(c, c->x, (int16_t)(solid_rect.y - c->body_y - new_rect.h));
            body->grounded = 1;
        } else {
            ng_physics_sync_position(c, c->x, (int16_t)(solid_rect.y + solid_rect.h - c->body_y));
        }
        c->vy_fp = 0;
    }
}

void NEOGEO_USER physics_init(void)
{
    uint8_t i;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        ng_physics_bodies[i].enabled = 0;
        ng_physics_bodies[i].grounded = 0;
        ng_physics_bodies[i].flags = 0;
        ng_physics_bodies[i].prev_x = 0;
        ng_physics_bodies[i].prev_y = 0;
        ng_physics_bodies[i].gravity_fp = NG_FP_FROM_FRAC(1, 4);
        ng_physics_bodies[i].max_fall_fp = NG_TO_FP(4);
        ng_physics_bodies[i].drag_x_fp = NG_FP_FROM_FRAC(1, 8);
        ng_physics_bodies[i].drag_y_fp = 0;
    }
    ng_solid_count = 0;
}

void NEOGEO_USER physics_attach(NGCharacter *c, uint16_t flags)
{
    NGPhysicsBody *body;
    uint8_t index = chars_index(c);

    if (index == 0xff) return;
    body = &ng_physics_bodies[index];
    body->enabled = 1;
    body->grounded = 0;
    body->flags = flags;
}

void NEOGEO_USER physics_detach(NGCharacter *c)
{
    uint8_t index = chars_index(c);

    if (index == 0xff) return;
    ng_physics_bodies[index].enabled = 0;
}

NGPhysicsBody *NEOGEO_USER physics_body(NGCharacter *c)
{
    uint8_t index = chars_index(c);

    if (index == 0xff) return 0;
    return &ng_physics_bodies[index];
}

void NEOGEO_USER physics_set_gravity(NGCharacter *c, int32_t gravity_fp, int32_t max_fall_fp)
{
    NGPhysicsBody *body = physics_body(c);

    if (!body) return;
    body->gravity_fp = gravity_fp;
    body->max_fall_fp = max_fall_fp;
}

void NEOGEO_USER physics_set_drag(NGCharacter *c, int32_t drag_x_fp, int32_t drag_y_fp)
{
    NGPhysicsBody *body = physics_body(c);

    if (!body) return;
    body->drag_x_fp = drag_x_fp;
    body->drag_y_fp = drag_y_fp;
}

void NEOGEO_USER physics_clear_solids(void)
{
    ng_solid_count = 0;
}

uint8_t NEOGEO_USER physics_add_solid(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t flags)
{
    if (ng_solid_count >= NG_MAX_SOLIDS) return 0xff;
    ng_solids[ng_solid_count].x = x;
    ng_solids[ng_solid_count].y = y;
    ng_solids[ng_solid_count].w = w;
    ng_solids[ng_solid_count].h = h;
    ng_solids[ng_solid_count].flags = flags;
    ng_solid_count++;
    return (uint8_t)(ng_solid_count - 1u);
}

void NEOGEO_USER physics_update_pre(void)
{
    uint8_t i;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        NGCharacter *c = chars_at(i);
        NGPhysicsBody *body = &ng_physics_bodies[i];

        if (!body->enabled || !c || !c->active) continue;

        body->prev_x = c->x;
        body->prev_y = c->y;
        body->grounded = 0;

        if (body->flags & NG_PHYSICS_GRAVITY) {
            c->vy_fp += body->gravity_fp;
            if (c->vy_fp > body->max_fall_fp) c->vy_fp = body->max_fall_fp;
        }
        if (body->flags & NG_PHYSICS_DRAG_X) {
            ng_physics_apply_drag(&c->vx_fp, body->drag_x_fp);
        }
        if (body->flags & NG_PHYSICS_DRAG_Y) {
            ng_physics_apply_drag(&c->vy_fp, body->drag_y_fp);
        }
    }
}

void NEOGEO_USER physics_resolve(void)
{
    uint8_t i;
    uint8_t s;

    for (i = 0; i < NG_MAX_CHARS; i++) {
        NGCharacter *c = chars_at(i);
        NGPhysicsBody *body = &ng_physics_bodies[i];

        if (!body->enabled || !c || !c->active) continue;

        if (body->flags & NG_PHYSICS_WORLD) {
            ng_physics_resolve_world(c, body);
        }
        if (body->flags & NG_PHYSICS_SOLIDS) {
            for (s = 0; s < ng_solid_count; s++) {
                ng_physics_resolve_solid(c, body, &ng_solids[s]);
            }
        }
    }
}

uint8_t NEOGEO_USER physics_is_grounded(NGCharacter *c)
{
    NGPhysicsBody *body = physics_body(c);

    if (!body) return 0;
    return body->grounded;
}
