/*
 * ng_rpg.c -- the top-down adventure / RPG genre engine (see ng_rpg.h).
 *
 * Sections: helpers, world state (flags, counters, items, conditions,
 * actions), tiles and collision, rooms and objects, the puzzle networks
 * (plates, gates, water, light, gears, sequences, balances), movement and
 * the party (leader, followers, paths), creatures and hazards, dialogue,
 * player commands, save/load and hashing.
 *
 * 68000 notes: no divide or modulo (tiles are 16 pixels: shifts), no
 * struct assignment, no recursion; path searches are breadth-first over a
 * fixed window with a fixed queue.
 */
#include "ng_rpg.h"

#define BOX_HW        (5 << 8)       /* feet box half width, Q8 */
#define BOX_HH        (4 << 8)
#define PUSH_FRAMES   14u
#define STUCK_PATH    40u
#define STUCK_WARP    170u
#define INVULN        60u
#define SAVE_MAGIC    0x4E475250u    /* 'NGRP' */
#define SAVE_VERSION  1u
#define BLOCK_WEIGHT  3u

/* ======================================================================
 * helpers
 * ==================================================================== */
static void NEOGEO_USER zero(uint8_t *d, uint16_t n)
{
    while (n--) *d++ = 0;
}

static uint8_t NEOGEO_USER bget(const uint8_t *b, uint16_t i) { return (uint8_t)((b[i >> 3] >> (i & 7u)) & 1u); }
static void NEOGEO_USER bset(uint8_t *b, uint16_t i) { b[i >> 3] = (uint8_t)(b[i >> 3] | (1u << (i & 7u))); }
static void NEOGEO_USER bclr(uint8_t *b, uint16_t i) { b[i >> 3] = (uint8_t)(b[i >> 3] & ~(1u << (i & 7u))); }

static uint16_t NEOGEO_USER rnd(NGRpg *g)
{
    uint32_t x = g->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    g->rng = x;
    return (uint16_t)(x >> 16);
}

/* 0..n-1 by one 16x16 multiply. */
static uint16_t NEOGEO_USER rnd_range(NGRpg *g, uint16_t n)
{
    return (uint16_t)(((uint32_t)rnd(g) * (uint32_t)n) >> 16);
}

static void NEOGEO_USER emit(NGRpg *g, uint8_t type, uint8_t a, uint8_t b, uint16_t value)
{
    NGRpgEvent *e;
    if (g->ev_count >= NGR_EVENT_QUEUE) { g->ev_dropped++; return; }
    e = &g->ev[(g->ev_head + g->ev_count) & (NGR_EVENT_QUEUE - 1u)];
    e->type = type; e->a = a; e->b = b; e->pad = 0; e->value = value;
    g->ev_count++;
}

static int32_t NEOGEO_USER tile_centre(uint8_t t) { return ((int32_t)t << 12) + (8 << 8); }
static uint8_t NEOGEO_USER tile_of(int32_t q8) { return (uint8_t)(q8 < 0 ? 0 : (q8 >> 12) > 255 ? 255 : (q8 >> 12)); }

static uint16_t NEOGEO_USER tindex(const NGRpg *g, uint8_t tx, uint8_t ty)
{
    return (uint16_t)((uint16_t)ty * g->def->w + tx);
}

static void NEOGEO_USER dir_step(uint8_t dir, int8_t *dx, int8_t *dy)
{
    *dx = (int8_t)(dir == NGR_FACE_RIGHT ? 1 : dir == NGR_FACE_LEFT ? -1 : 0);
    *dy = (int8_t)(dir == NGR_FACE_DOWN ? 1 : dir == NGR_FACE_UP ? -1 : 0);
}

static NGRpgMember *NEOGEO_USER lead(NGRpg *g) { return &g->party[g->leader]; }

/* ======================================================================
 * world state
 * ==================================================================== */
uint8_t NEOGEO_USER ng_rpg_flag(const NGRpg *g, uint16_t flag)
{
    if (!g || !flag || flag >= NGR_FLAGS) return 0;
    return bget(g->flags, flag);
}

void NEOGEO_USER ng_rpg_set_flag(NGRpg *g, uint16_t flag, uint8_t on)
{
    if (!g || !flag || flag >= NGR_FLAGS) return;
    if (on) {
        if (!bget(g->flags, flag)) emit(g, NGR_EV_FLAG, 0, 1, flag);
        bset(g->flags, flag);
    } else {
        bclr(g->flags, flag);
    }
}

uint8_t NEOGEO_USER ng_rpg_collected(const NGRpg *g, uint8_t id)
{
    return g ? bget(g->collected, id) : 0;
}

static const NGRpgItemDef *NEOGEO_USER item_def(const NGRpg *g, uint8_t item)
{
    if (!g->world || item >= g->world->nitems || !g->world->items) return 0;
    return &g->world->items[item];
}

uint8_t NEOGEO_USER ng_rpg_count(const NGRpg *g, uint8_t item)
{
    uint8_t i;
    uint16_t n = 0;
    if (!g) return 0;
    for (i = 0; i < NGR_INV_SLOTS; i++) if (g->inv_count[i] && g->inv_item[i] == item) n = (uint16_t)(n + g->inv_count[i]);
    return (uint8_t)(n > 255u ? 255u : n);
}

/* One of an item into a bag laid out with `stride` bytes between slots
 * (the context's own arrays, or a save's interleaved pairs). 1 if it fit. */
static uint8_t NEOGEO_USER inv_add(const NGRpg *g, uint8_t *items, uint8_t *counts, uint8_t stride, uint8_t item)
{
    const NGRpgItemDef *d = item_def(g, item);
    uint8_t i, maxs, key, last;
    uint8_t *it, *ct;
    if (!d) return 0;
    maxs = d->max_stack ? d->max_stack : 1u;
    /* key items, tools and records may also use the last NGR_INV_KEYS slots,
     * which nothing else can fill: they always have room */
    key = (uint8_t)(d->flags & (NGR_I_KEY | NGR_I_TOOL | NGR_I_RECORD));
    last = (uint8_t)(key ? NGR_INV_SLOTS : NGR_INV_SLOTS - NGR_INV_KEYS);
    for (i = 0, it = items, ct = counts; i < NGR_INV_SLOTS; i++, it += stride, ct += stride)
        if (*ct && *it == item && *ct < maxs) { (*ct)++; return 1; }
    for (i = 0, it = items, ct = counts; i < last; i++, it += stride, ct += stride)
        if (!*ct) { *it = item; *ct = 1; return 1; }
    return 0;
}

uint8_t NEOGEO_USER ng_rpg_give(NGRpg *g, uint8_t item, uint8_t count)
{
    uint8_t given = 0;
    if (!g || !count || !item_def(g, item)) return 0;
    while (given < count) {
        if (!inv_add(g, g->inv_item, g->inv_count, 1, item)) { emit(g, NGR_EV_FULL, item, 0, 0); break; }
        given++;
    }
    if (given) emit(g, NGR_EV_ITEM, item, given, 0);
    return given;
}

uint8_t NEOGEO_USER ng_rpg_take(NGRpg *g, uint8_t item, uint8_t count)
{
    uint8_t i, taken = 0;
    if (!g) return 0;
    for (i = NGR_INV_SLOTS; i-- > 0 && taken < count;) {
        while (g->inv_count[i] && g->inv_item[i] == item && taken < count) { g->inv_count[i]--; taken++; }
    }
    return taken;
}

int NEOGEO_USER ng_rpg_drop(NGRpg *g, uint8_t item, uint8_t count)
{
    const NGRpgItemDef *d;
    if (!g) return NGR_ERR_ARG;
    d = item_def(g, item);
    if (!d) return NGR_ERR_ARG;
    if (d->flags & NGR_I_KEY) return NGR_ERR_KEY;
    if (ng_rpg_count(g, item) < count) return NGR_ERR_NOPE;
    ng_rpg_take(g, item, count);
    return NGR_OK;
}

int NEOGEO_USER ng_rpg_craft(NGRpg *g, uint8_t recipe)
{
    const NGRpgRecipe *r;
    uint8_t i, free_slot = 0;
    if (!g || !g->world || recipe >= g->world->nrecipes) return NGR_ERR_ARG;
    r = &g->world->recipes[recipe];
    for (i = 0; i < 3u; i++)
        if (r->in_count[i] && ng_rpg_count(g, r->in_item[i]) < r->in_count[i]) return NGR_ERR_NOPE;
    /* room for the result once the inputs are gone? */
    for (i = 0; i < NGR_INV_SLOTS; i++) if (!g->inv_count[i] || g->inv_item[i] == r->out_item) free_slot = 1;
    for (i = 0; i < 3u && !free_slot; i++) if (r->in_count[i] && ng_rpg_count(g, r->in_item[i]) == r->in_count[i]) free_slot = 1;
    if (!free_slot) return NGR_ERR_NOPE;
    for (i = 0; i < 3u; i++) if (r->in_count[i]) ng_rpg_take(g, r->in_item[i], r->in_count[i]);
    ng_rpg_give(g, r->out_item, r->out_count ? r->out_count : 1u);
    emit(g, NGR_EV_CRAFT, recipe, 0, 0);
    return NGR_OK;
}

uint8_t NEOGEO_USER ng_rpg_check(const NGRpg *g, const NGRpgCond *c)
{
    if (!g || !c) return 1;
    switch (c->op) {
    case NGR_C_ALWAYS: return 1;
    case NGR_C_FLAG: return ng_rpg_flag(g, c->a);
    case NGR_C_NOT_FLAG: return (uint8_t)!ng_rpg_flag(g, c->a);
    case NGR_C_ITEM: return (uint8_t)(ng_rpg_count(g, (uint8_t)c->a) >= (c->b ? c->b : 1u));
    case NGR_C_COUNTER_GE: return (uint8_t)(c->a < NGR_COUNTERS && g->counters[c->a] >= c->b);
    case NGR_C_COUNTER_EQ: return (uint8_t)(c->a < NGR_COUNTERS && g->counters[c->a] == c->b);
    case NGR_C_LEADER: return (uint8_t)(g->nparty && g->party[g->leader].id == c->a);
    case NGR_C_SIGNAL: return (uint8_t)(c->a < 32u && ((g->signals >> c->a) & 1u));
    case NGR_C_COLLECTED: return (uint8_t)(c->a < NGR_COLLECT && bget(g->collected, c->a));
    default: return 0;
    }
}

static void NEOGEO_USER open_object(NGRpg *g, uint8_t i);
static void NEOGEO_USER checkpoint_here(NGRpg *g);

void NEOGEO_USER ng_rpg_run(NGRpg *g, const NGRpgAction *a)
{
    uint8_t guard = 0;
    if (!g || !a) return;
    for (; a->op != NGR_A_END && guard < 32u; a++, guard++) {
        switch (a->op) {
        case NGR_A_SET_FLAG: ng_rpg_set_flag(g, a->a, 1); break;
        case NGR_A_CLEAR_FLAG: ng_rpg_set_flag(g, a->a, 0); break;
        case NGR_A_GIVE: ng_rpg_give(g, (uint8_t)a->a, (uint8_t)(a->b ? a->b : 1u)); break;
        case NGR_A_TAKE: ng_rpg_take(g, (uint8_t)a->a, (uint8_t)(a->b ? a->b : 1u)); break;
        case NGR_A_SET_COUNTER:
            if (a->a < NGR_COUNTERS) { g->counters[a->a] = (uint8_t)a->b; emit(g, NGR_EV_COUNTER, (uint8_t)a->a, (uint8_t)a->b, 0); }
            break;
        case NGR_A_ADD_COUNTER:
            if (a->a < NGR_COUNTERS) {
                uint16_t v = (uint16_t)(g->counters[a->a] + a->b);
                g->counters[a->a] = (uint8_t)(v > 255u ? 255u : v);
                emit(g, NGR_EV_COUNTER, (uint8_t)a->a, g->counters[a->a], 0);
            }
            break;
        case NGR_A_DIALOGUE: ng_rpg_dialogue_start(g, a->a); break;
        case NGR_A_CHECKPOINT: checkpoint_here(g); break;
        case NGR_A_RESTORE:
            if (a->a < 16u && !(g->restored & (1u << a->a))) {
                g->restored = (uint16_t)(g->restored | (1u << a->a));
                emit(g, NGR_EV_RESTORE, (uint8_t)a->a, 0, 0);
            }
            break;
        case NGR_A_HEAL: {
            uint8_t i;
            for (i = 0; i < g->nparty; i++) {
                NGRpgMember *m = &g->party[i];
                uint16_t hp = (uint16_t)(a->b ? m->hp + a->b : m->max_hp);
                m->hp = (uint8_t)(hp > m->max_hp ? m->max_hp : hp);
            }
            break;
        }
        case NGR_A_TELEPORT:
            ng_rpg_enter_room(g, (uint8_t)a->a, (uint8_t)(a->b & 0xFFu), (uint8_t)(a->b >> 8), lead(g)->facing);
            return;   /* the room changed: the rest of the list belonged to the old one */
        case NGR_A_COLLECT:
            if (a->a < NGR_COLLECT && !bget(g->collected, a->a)) { bset(g->collected, a->a); emit(g, NGR_EV_COLLECT, (uint8_t)a->a, 0, 0); }
            break;
        case NGR_A_EVENT: emit(g, NGR_EV_GAME, (uint8_t)a->a, 0, a->b); break;
        case NGR_A_OPEN_OBJECT: if (a->a < NGR_MAX_OBJECTS) open_object(g, (uint8_t)a->a); break;
        default: break;
        }
    }
}

/* ======================================================================
 * tiles and collision
 * ==================================================================== */
uint8_t NEOGEO_USER ng_rpg_tile(const NGRpg *g, uint8_t tx, uint8_t ty)
{
    uint16_t i;
    uint8_t f;
    if (!g || !g->def || tx >= g->def->w || ty >= g->def->h) return NGR_T_SOLID;
    i = tindex(g, tx, ty);
    f = g->def->tiles ? g->def->tiles[i] : 0u;
    if (bget(g->floor, i)) f = (uint8_t)(f & (uint8_t)~(NGR_T_WATER | NGR_T_PIT | NGR_T_HAZARD));
    if (bget(g->solid, i)) f = (uint8_t)(f | NGR_T_SOLID);
    return f;
}

static uint8_t NEOGEO_USER pass(const NGRpg *g, uint8_t ab, uint8_t is_block, uint8_t tx, uint8_t ty, uint8_t dir)
{
    uint8_t f = ng_rpg_tile(g, tx, ty);
    if (f & (NGR_T_SOLID | NGR_T_PIT)) return 0;
    if ((f & NGR_T_WATER) && (is_block || !(ab & NGR_AB_SWIM))) return 0;
    if ((f & NGR_T_NARROW) && (is_block || !(ab & NGR_AB_NARROW))) return 0;
    if ((f & NGR_T_CLIMB) && (is_block || !(ab & NGR_AB_CLIMB))) return 0;
    if ((f & NGR_T_LEDGE) && dir != NGR_FACE_DOWN) return 0;
    return 1;
}

uint8_t NEOGEO_USER ng_rpg_walkable(const NGRpg *g, uint8_t member, uint8_t tx, uint8_t ty)
{
    if (!g || !g->def) return 0;
    if (member == NGR_NONE) return (uint8_t)(pass(g, 0, 1, tx, ty, NGR_FACE_DOWN) && ng_rpg_object_at(g, tx, ty) == NGR_NONE);
    if (member >= g->nparty) return 0;
    return pass(g, g->party[member].abilities, 0, tx, ty, NGR_FACE_DOWN);
}

static uint8_t NEOGEO_USER def_w(const NGRpgObjectDef *d) { return d->w ? d->w : 1u; }
static uint8_t NEOGEO_USER def_h(const NGRpgObjectDef *d) { return d->h ? d->h : 1u; }

static uint8_t NEOGEO_USER covers(const NGRpg *g, uint8_t i, uint8_t tx, uint8_t ty)
{
    const NGRpgObjectDef *d = &g->def->objects[i];
    const NGRpgObject *o = &g->obj[i];
    return (uint8_t)(tx >= o->x && tx < (uint8_t)(o->x + def_w(d)) && ty >= o->y && ty < (uint8_t)(o->y + def_h(d)));
}

uint8_t NEOGEO_USER ng_rpg_object_at(const NGRpg *g, uint8_t tx, uint8_t ty)
{
    uint8_t i;
    uint32_t only = 0xFFFFFFFFu;
    if (!g || !g->def) return NGR_NONE;
    /* Nothing that stays put covers the tile (rebuild_overlays keeps the
     * map): only people, creatures and platforms can be there. */
    if (tx < g->def->w && ty < g->def->h && !bget(g->occupied, tindex(g, tx, ty))) {
        if (!g->movers) return NGR_NONE;
        only = g->movers;
    }
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        uint8_t t = g->def->objects[i].type;
        if (!((only >> i) & 1u) || !g->obj[i].alive) continue;
        if (t == NGR_O_SEQUENCE || t == NGR_O_BALANCE || t == NGR_O_SIGNAL_FLAG) continue;
        if (t == NGR_O_NPC || t == NGR_O_CREATURE || t == NGR_O_PLATFORM) {
            if (tile_of(g->obj[i].px) == tx && tile_of(g->obj[i].py) == ty) return i;
            continue;
        }
        if (covers(g, i, tx, ty)) return i;
    }
    return NGR_NONE;
}

/* Solid while alive / closed. */
static uint8_t NEOGEO_USER object_solid(const NGRpg *g, uint8_t i)
{
    const NGRpgObject *o = &g->obj[i];
    const NGRpgObjectDef *d = &g->def->objects[i];
    if (!o->alive) return 0;
    switch (d->type) {
    case NGR_O_SIGN: case NGR_O_CHEST: case NGR_O_REST: case NGR_O_BLOCK: case NGR_O_LEVER:
    case NGR_O_GEAR: case NGR_O_VALVE: case NGR_O_EMITTER: case NGR_O_MIRROR: case NGR_O_LENS:
    case NGR_O_RECEIVER: case NGR_O_BELL:
        return 1;
    case NGR_O_LOCK: case NGR_O_GATE: return (uint8_t)!o->state;
    case NGR_O_SECRET: return (uint8_t)(!o->state && d->p[1]);
    default: return 0;
    }
}

/* What an object adds to the overlays, the object map and so to water and
 * light: alive, the state that opens, turns or fills it, and its tile (a
 * person's, a creature's or a platform's where it is now). */
static void NEOGEO_USER object_key(const NGRpg *g, uint8_t i, uint8_t *k)
{
    const NGRpgObject *o = &g->obj[i];
    k[0] = o->alive; k[1] = 0; k[2] = o->x; k[3] = o->y;
    switch (g->def->objects[i].type) {
    case NGR_O_NPC: case NGR_O_CREATURE: case NGR_O_PLATFORM: k[2] = tile_of(o->px); k[3] = tile_of(o->py); break;
    case NGR_O_SEQUENCE: case NGR_O_BALANCE: case NGR_O_SIGNAL_FLAG: k[0] = 0; k[2] = 0; k[3] = 0; break;
    case NGR_O_BLOCK: k[1] = o->aux; break;
    case NGR_O_LOCK: case NGR_O_GATE: case NGR_O_SECRET: case NGR_O_BRIDGE: case NGR_O_VALVE: case NGR_O_MIRROR:
        k[1] = o->state; break;
    default: break;
    }
}

static void NEOGEO_USER rebuild_overlays(NGRpg *g)
{
    uint8_t i;
    zero(g->solid, NGR_ROOM_BITS);
    zero(g->floor, NGR_ROOM_BITS);
    zero(g->occupied, NGR_ROOM_BITS);
    g->movers = 0;
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        const NGRpgObjectDef *d = &g->def->objects[i];
        const NGRpgObject *o = &g->obj[i];
        uint8_t x, y, solid = object_solid(g, i), floor = 0;
        /* ng_rpg_object_at's map: the tiles of what is there and stays put;
         * the ones that move about are looked at every time */
        if (d->type == NGR_O_NPC || d->type == NGR_O_CREATURE || d->type == NGR_O_PLATFORM) g->movers |= (uint32_t)1u << i;
        else if (o->alive && d->type != NGR_O_SEQUENCE && d->type != NGR_O_BALANCE && d->type != NGR_O_SIGNAL_FLAG)
            for (y = 0; y < def_h(d); y++)
                for (x = 0; x < def_w(d); x++) {
                    uint8_t tx = (uint8_t)(o->x + x), ty = (uint8_t)(o->y + y);
                    if (tx < g->def->w && ty < g->def->h) bset(g->occupied, tindex(g, tx, ty));
                }
        if (d->type == NGR_O_BRIDGE && o->state) floor = 1;
        if (d->type == NGR_O_BLOCK && !o->alive && o->aux) floor = 1;   /* sank into a pit */
        if (d->type == NGR_O_PLATFORM) {
            uint8_t px = tile_of(o->px), py = tile_of(o->py);
            for (y = 0; y < def_h(d); y++) for (x = 0; x < def_w(d); x++)
                if ((uint8_t)(px + x) < g->def->w && (uint8_t)(py + y) < g->def->h) bset(g->floor, tindex(g, (uint8_t)(px + x), (uint8_t)(py + y)));
            continue;
        }
        if (!solid && !floor) continue;
        for (y = 0; y < def_h(d); y++)
            for (x = 0; x < def_w(d); x++) {
                uint8_t tx = (uint8_t)(o->x + x), ty = (uint8_t)(o->y + y);
                if (tx >= g->def->w || ty >= g->def->h) continue;
                if (solid) bset(g->solid, tindex(g, tx, ty));
                if (floor) bset(g->floor, tindex(g, tx, ty));
            }
    }
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) object_key(g, i, &g->keys[4u * i]);
    g->ov_epoch++;
}

/* The overlays again only if an object's key changed since they were made. */
static void NEOGEO_USER refresh_overlays(NGRpg *g)
{
    uint8_t i, k[4];
    const uint8_t *s = g->keys;
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++, s += 4) {
        object_key(g, i, k);
        if (k[0] != s[0] || k[1] != s[1] || k[2] != s[2] || k[3] != s[3]) { rebuild_overlays(g); return; }
    }
}

/* Can a member's feet box sit at (x, y)? Tiles under its four corners. */
static uint8_t NEOGEO_USER box_ok(const NGRpg *g, const NGRpgMember *m, int32_t x, int32_t y, uint8_t dir)
{
    int32_t xs[2], ys[2];
    uint8_t i, j;
    xs[0] = x - BOX_HW; xs[1] = x + BOX_HW - 1;
    ys[0] = y - BOX_HH; ys[1] = y + BOX_HH - 1;
    if (xs[0] < 0 || ys[0] < 0) return 0;
    for (i = 0; i < 2u; i++)
        for (j = 0; j < 2u; j++) {
            uint8_t tx = tile_of(xs[i]), ty = tile_of(ys[j]);
            if (!pass(g, m->abilities, 0, tx, ty, dir)) {
                /* a ledge only stops you entering it from the side or below */
                if ((ng_rpg_tile(g, tx, ty) & NGR_T_LEDGE) && tx == tile_of(m->x) && ty == tile_of(m->y)) continue;
                return 0;
            }
        }
    return 1;
}

/* People block the leader (not followers, not each other). */
static uint8_t NEOGEO_USER npc_blocks(const NGRpg *g, int32_t x, int32_t y)
{
    uint8_t i;
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        const NGRpgObject *o = &g->obj[i];
        int32_t dx, dy;
        if (g->def->objects[i].type != NGR_O_NPC || !o->alive) continue;
        dx = o->px - x; dy = o->py - y;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        if (dx < (10 << 8) && dy < (8 << 8)) return 1;
    }
    return 0;
}

/* ======================================================================
 * rooms and objects
 * ==================================================================== */
static void NEOGEO_USER init_objects(NGRpg *g)
{
    uint8_t i;
    zero((uint8_t *)g->obj, (uint16_t)sizeof(g->obj));
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        const NGRpgObjectDef *d = &g->def->objects[i];
        NGRpgObject *o = &g->obj[i];
        uint8_t done = ng_rpg_flag(g, d->flag);
        o->alive = 1;
        o->x = d->x; o->y = d->y;
        o->px = tile_centre(d->x); o->py = tile_centre(d->y);
        o->facing = NGR_FACE_DOWN;
        switch (d->type) {
        case NGR_O_PICKUP: if (done) o->alive = 0; break;
        case NGR_O_NPC: if ((d->p[4] == 1u && !done) || (d->p[4] == 2u && done)) o->alive = 0; break;
        case NGR_O_CHEST: case NGR_O_LOCK: case NGR_O_SECRET: case NGR_O_GATE: case NGR_O_BRIDGE:
            o->state = done; break;
        case NGR_O_SEQUENCE: if (done) { o->state = 0xFF; o->aux = 1; } break;
        case NGR_O_CREATURE: if (done) o->alive = 0; o->hits = d->p[1] ? d->p[1] : 1u; break;
        case NGR_O_VALVE: o->state = d->p[0] ? 1u : 0u; break;
        case NGR_O_LEVER: o->state = done; break;                    /* with a flag: where it was left */
        case NGR_O_MIRROR: o->state = (uint8_t)(d->p[0] & 1u); break;
        case NGR_O_GEAR: o->state = (d->p[0] & 3u) ? 1u : (done ? 1u : 0u); break;   /* driver and target carry fixed gears */
        default: break;
        }
    }
    g->signals = 0;
    g->derived = 0;
    zero(g->wet, NGR_ROOM_BITS);
    zero(g->lit, NGR_ROOM_BITS);
    g->gear_turn = 0; g->gear_jam = 0;
    rebuild_overlays(g);
}

/* A gear set in a socket that has no flag still belongs to the bag: it
 * comes back when the room's puzzles are put back or the room is left (a
 * socket with a flag keeps its gear for good). */
static uint8_t NEOGEO_USER gear_loose(const NGRpg *g, uint8_t i)
{
    const NGRpgObjectDef *d = &g->def->objects[i];
    return (uint8_t)(d->type == NGR_O_GEAR && !(d->p[0] & 3u) && d->p[5] && g->obj[i].state && !d->flag);
}

static void NEOGEO_USER refund_gears(NGRpg *g)
{
    uint8_t i;
    if (!g->def) return;
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++)
        if (gear_loose(g, i)) { inv_add(g, g->inv_item, g->inv_count, 1, g->def->objects[i].p[5]); g->obj[i].state = 0; }
}

static void NEOGEO_USER trail_reset(NGRpg *g)
{
    uint8_t i;
    NGRpgMember *m = lead(g);
    for (i = 0; i < NGR_TRAIL; i++) { g->trail_x[i] = (int16_t)(m->x >> 8); g->trail_y[i] = (int16_t)(m->y >> 8); }
    g->trail_head = 0; g->trail_n = 1;
}

/* A walkable tile next to (tx, ty) for member m, nearest first. */
static uint8_t NEOGEO_USER near_tile(const NGRpg *g, uint8_t m, uint8_t tx, uint8_t ty, uint8_t *ox, uint8_t *oy, uint8_t avoid_door)
{
    static const int8_t ring[24][2] = {
        {0, 1}, {-1, 0}, {1, 0}, {0, -1}, {-1, 1}, {1, 1}, {-1, -1}, {1, -1},
        {0, 2}, {-2, 0}, {2, 0}, {0, -2}, {-1, 2}, {1, 2}, {-2, 1}, {2, 1},
        {-2, -1}, {2, -1}, {-1, -2}, {1, -2}, {-2, 2}, {2, 2}, {-2, -2}, {2, -2} };
    uint8_t i;
    for (i = 0; i < 24u; i++) {
        int16_t x = (int16_t)(tx + ring[i][0]), y = (int16_t)(ty + ring[i][1]);
        if (x < 0 || y < 0 || x >= (int16_t)g->def->w || y >= (int16_t)g->def->h) continue;
        if (!pass(g, g->party[m].abilities, 0, (uint8_t)x, (uint8_t)y, NGR_FACE_DOWN)) continue;
        if (avoid_door && (ng_rpg_tile(g, (uint8_t)x, (uint8_t)y) & (NGR_T_DOOR | NGR_T_NARROW))) continue;
        if (ng_rpg_tile(g, (uint8_t)x, (uint8_t)y) & (NGR_T_HAZARD | NGR_T_LEDGE)) continue;
        *ox = (uint8_t)x; *oy = (uint8_t)y;
        return 1;
    }
    return 0;
}

static void NEOGEO_USER run_triggers(NGRpg *g, uint8_t when);

int NEOGEO_USER ng_rpg_enter_room(NGRpg *g, uint8_t room, uint8_t tx, uint8_t ty, uint8_t facing)
{
    uint8_t i;
    if (!g || !g->world || room >= g->world->nrooms || !g->nparty) return NGR_ERR_ARG;
    if (tx >= g->world->rooms[room].w || ty >= g->world->rooms[room].h) return NGR_ERR_ARG;
    refund_gears(g);
    g->room = room;
    g->def = &g->world->rooms[room];
    if (tx >= g->def->w || ty >= g->def->h) return NGR_ERR_ARG;
    init_objects(g);
    for (i = 0; i < g->nparty; i++) {
        NGRpgMember *m = &g->party[i];
        uint8_t x = tx, y = ty;
        if (i != g->leader) {
            if (m->mode != NGR_MODE_LEAD) m->mode = NGR_MODE_FOLLOW;
            near_tile(g, i, tx, ty, &x, &y, 1);
        }
        m->x = tile_centre(x); m->y = tile_centre(y);
        m->facing = facing;
        m->moving = 0; m->push = 0; m->stuck = 0; m->path_len = 0; m->path_pos = 0;
    }
    trail_reset(g);
    g->pending_exit = 0;
    g->in_exit = 0xFFu;            /* standing in the arrival doorway doesn't count */
    for (i = 0; i < 4u; i++) g->in_trig[i] = 0xFFu;
    if (room < NGR_MAX_ROOMS) bset(g->visited, room);
    emit(g, NGR_EV_ROOM, room, 0, 0);
    run_triggers(g, NGR_TRIG_ROOM);
    return NGR_OK;
}

int NEOGEO_USER ng_rpg_take_exit(NGRpg *g)
{
    const NGRpgExit *e;
    if (!g || !g->pending_exit) return NGR_ERR_STATE;
    e = &g->def->exits[g->pending_exit - 1u];
    g->pending_exit = 0;
    return ng_rpg_enter_room(g, e->room, e->to_x, e->to_y, e->facing);
}

void NEOGEO_USER ng_rpg_reset_room(NGRpg *g)
{
    uint8_t i;
    if (!g || !g->def) return;
    refund_gears(g);
    init_objects(g);
    /* a block or gate put back where someone stands: they step aside */
    for (i = 0; i < g->nparty; i++) {
        NGRpgMember *m = &g->party[i];
        uint8_t tx = tile_of(m->x), ty = tile_of(m->y), nx, ny;
        if (box_ok(g, m, m->x, m->y, NGR_FACE_DOWN)) continue;
        if (near_tile(g, i, tx, ty, &nx, &ny, 0)) {
            m->x = tile_centre(nx); m->y = tile_centre(ny);
            m->path_len = m->path_pos = 0;
        }
    }
}

static void NEOGEO_USER checkpoint_here(NGRpg *g)
{
    NGRpgMember *m = lead(g);
    g->ck_room = g->room; g->ck_x = tile_of(m->x); g->ck_y = tile_of(m->y);
    emit(g, NGR_EV_CHECKPOINT, g->room, 0, 0);
}

int NEOGEO_USER ng_rpg_world_check(const NGRpgWorld *w, uint8_t *room, uint16_t *index)
{
    uint8_t r;
    uint16_t i;
    if (room) *room = NGR_NONE;
    if (index) *index = 0xFFFFu;
    if (!w || !w->rooms || !w->nrooms || w->nrooms > NGR_MAX_ROOMS) return NGR_ERR_WORLD;
    for (r = 0; r < w->nrooms; r++) {
        const NGRpgRoomDef *d = &w->rooms[r];
        if (room) *room = r;
        if (!d->w || !d->h || d->w > NGR_ROOM_MAX_W || d->h > NGR_ROOM_MAX_H || !d->tiles) return NGR_ERR_WORLD;
        if (d->nobjects > NGR_MAX_OBJECTS || d->ntriggers > 32u) return NGR_ERR_WORLD;
        for (i = 0; i < d->nexits; i++) {
            const NGRpgExit *e = &d->exits[i];
            if (index) *index = i;
            if (e->room >= w->nrooms) return NGR_ERR_WORLD;
            if (e->to_x >= w->rooms[e->room].w || e->to_y >= w->rooms[e->room].h) return NGR_ERR_WORLD;
            if (w->rooms[e->room].tiles[(uint16_t)e->to_y * w->rooms[e->room].w + e->to_x] & (NGR_T_SOLID | NGR_T_PIT | NGR_T_WATER))
                return NGR_ERR_WORLD;
        }
        for (i = 0; i < d->nobjects; i++) {
            const NGRpgObjectDef *o = &d->objects[i];
            if (index) *index = (uint16_t)(0x100u + i);
            if (o->type == NGR_O_BALANCE && (o->p[0] >= d->nobjects || o->p[1] >= d->nobjects)) return NGR_ERR_WORLD;
            if (o->type == NGR_O_SEQUENCE || o->type == NGR_O_BALANCE || o->type == NGR_O_SIGNAL_FLAG) continue;
            if (o->x >= d->w || o->y >= d->h) return NGR_ERR_WORLD;
            if ((o->type == NGR_O_PICKUP || o->type == NGR_O_CHEST) && o->p[0] != NGR_NONE && o->p[0] >= w->nitems) return NGR_ERR_WORLD;
            if ((o->type == NGR_O_NPC || o->type == NGR_O_SIGN) && o->text >= w->ndialogue) return NGR_ERR_WORLD;
        }
    }
    for (i = 0; i < w->ndialogue; i++) {
        const NGRpgDialogueNode *n = &w->dialogue[i];
        uint8_t c;
        if (room) *room = NGR_NONE;
        if (index) *index = i;
        if (n->next != NGR_END && n->next >= w->ndialogue) return NGR_ERR_WORLD;
        if (n->cond.op != NGR_C_ALWAYS && n->alt != NGR_END && n->alt >= w->ndialogue) return NGR_ERR_WORLD;
        if (n->nchoices > 3u) return NGR_ERR_WORLD;
        for (c = 0; c < n->nchoices; c++)
            if (n->choice_next[c] != NGR_END && n->choice_next[c] >= w->ndialogue) return NGR_ERR_WORLD;
    }
    for (i = 0; i < w->nrecipes; i++) {
        const NGRpgRecipe *rc = &w->recipes[i];
        if (rc->out_item >= w->nitems) return NGR_ERR_WORLD;
    }
    return NGR_OK;
}

int NEOGEO_USER ng_rpg_init(NGRpg *g, const NGRpgWorld *w, uint32_t seed)
{
    if (!g || !w) return NGR_ERR_ARG;
    if (ng_rpg_world_check(w, 0, 0) != NGR_OK) return NGR_ERR_WORLD;
    zero((uint8_t *)g, (uint16_t)sizeof(NGRpg));
    g->world = w;
    g->rng = seed ? seed : 0x13579BDFu;
    g->leader = 0;
    return NGR_OK;
}

int NEOGEO_USER ng_rpg_party_add(NGRpg *g, const NGRpgMemberDef *d)
{
    NGRpgMember *m;
    if (!g || !d || g->nparty >= NGR_MAX_PARTY) return NGR_ERR_ARG;
    m = &g->party[g->nparty];
    zero((uint8_t *)m, (uint16_t)sizeof(NGRpgMember));
    m->id = d->id; m->abilities = d->abilities; m->speed = d->speed ? d->speed : 24u;
    m->max_hp = d->max_hp ? d->max_hp : 6u; m->hp = m->max_hp; m->weight = d->weight ? d->weight : 2u;
    m->mode = g->nparty ? NGR_MODE_FOLLOW : NGR_MODE_LEAD;
    m->trail_lag = (uint16_t)(g->nparty * 3u);
    if (g->nparty && g->def) {          /* joining in a room: at the leader's side */
        const NGRpgMember *l = lead(g);
        m->x = l->x; m->y = l->y; m->facing = l->facing;
    }
    return g->nparty++;
}

int NEOGEO_USER ng_rpg_party_remove(NGRpg *g, uint8_t member)
{
    uint8_t i;
    uint16_t k;
    if (!g || member >= g->nparty || g->nparty < 2u) return NGR_ERR_ARG;
    if (g->dlg_open) return NGR_ERR_STATE;
    for (i = member; i + 1u < g->nparty; i++) {
        uint8_t *d = (uint8_t *)&g->party[i];
        const uint8_t *s = (const uint8_t *)&g->party[i + 1u];
        for (k = 0; k < (uint16_t)sizeof(NGRpgMember); k++) d[k] = s[k];
    }
    g->nparty--;
    zero((uint8_t *)&g->party[g->nparty], (uint16_t)sizeof(NGRpgMember));
    if (g->leader == member) g->leader = 0;
    else if (g->leader > member) g->leader--;
    for (i = 0; i < g->nparty; i++) {
        g->party[i].trail_lag = (uint16_t)(i * 3u);
        if (i == g->leader) g->party[i].mode = NGR_MODE_LEAD;
        else if (g->party[i].mode == NGR_MODE_LEAD) g->party[i].mode = NGR_MODE_FOLLOW;
        g->party[i].path_len = g->party[i].path_pos = 0;
    }
    trail_reset(g);
    return NGR_OK;
}

/* ======================================================================
 * puzzle networks
 * ==================================================================== */
static uint8_t NEOGEO_USER weight_on(const NGRpg *g, uint8_t i)
{
    uint8_t m, j, w = 0;
    for (m = 0; m < g->nparty; m++)
        if (covers(g, i, tile_of(g->party[m].x), tile_of(g->party[m].y))) w = (uint8_t)(w + g->party[m].weight);
    for (j = 0; j < g->def->nobjects && j < NGR_MAX_OBJECTS; j++)
        if (g->def->objects[j].type == NGR_O_BLOCK && g->obj[j].alive && covers(g, i, g->obj[j].x, g->obj[j].y))
            w = (uint8_t)(w + BLOCK_WEIGHT);
    return w;
}

static void NEOGEO_USER flow_water(NGRpg *g)
{
    uint16_t head = 0, tail = 0, n = (uint16_t)((uint16_t)g->def->w * g->def->h);
    uint8_t i;
    uint16_t *q = g->nav_q;
    uint16_t cap = (uint16_t)(NGR_NAV_W * NGR_NAV_H);
    /* the same objects where they were and the same signals: the same water */
    if (g->water_ok && g->water_epoch == g->ov_epoch && g->water_sig == g->signals) return;
    g->water_ok = 1; g->water_epoch = g->ov_epoch; g->water_sig = g->signals;
    zero(g->wet, NGR_ROOM_BITS);
    if (!g->def->channels) return;
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        const NGRpgObjectDef *d = &g->def->objects[i];
        uint16_t t;
        if (d->type != NGR_O_WATER_SRC) continue;
        if (d->in_mask && (g->signals & d->in_mask) != d->in_mask) continue;
        t = tindex(g, g->obj[i].x, g->obj[i].y);
        if (!bget(g->wet, t) && tail < cap) { bset(g->wet, t); q[tail++] = t; }
    }
    while (head < tail) {
        uint16_t t = q[head++];
        uint8_t x, y = 0, d;
        uint16_t r = t;
        /* t = y * w + x without a divide: walk the rows */
        while (r >= g->def->w) { r = (uint16_t)(r - g->def->w); y++; }
        x = (uint8_t)r;
        for (d = 0; d < 4u; d++) {
            uint8_t nx = x, ny = y, o;
            uint16_t nt;
            /* the neighbour the way NGR_FACE_<d> faces, by steps of the index */
            if (d == NGR_FACE_RIGHT) { if ((uint8_t)(x + 1u) >= g->def->w) continue; nx++; nt = (uint16_t)(t + 1u); }
            else if (d == NGR_FACE_DOWN) { if ((uint8_t)(y + 1u) >= g->def->h) continue; ny++; nt = (uint16_t)(t + g->def->w); }
            else if (d == NGR_FACE_LEFT) { if (!x) continue; nx--; nt = (uint16_t)(t - 1u); }
            else { if (!y) continue; ny--; nt = (uint16_t)(t - g->def->w); }
            if (nt >= n || bget(g->wet, nt) || !g->def->channels[nt]) continue;
            o = ng_rpg_object_at(g, nx, ny);
            if (o != NGR_NONE) {
                uint8_t ty = g->def->objects[o].type;
                if (ty == NGR_O_VALVE && !g->obj[o].state) continue;
                if (ty == NGR_O_GATE && !g->obj[o].state) continue;
                if (ty == NGR_O_LOCK && !g->obj[o].state) continue;    /* a missing section of channel */
                if (ty == NGR_O_BLOCK) continue;
            }
            bset(g->wet, nt);
            if (tail < cap) q[tail++] = nt;
        }
    }
}

static void NEOGEO_USER trace_light(NGRpg *g)
{
    uint8_t i;
    if (g->light_ok && g->light_epoch == g->ov_epoch && g->light_sig == g->signals) {
        for (i = 0; i < NGR_MAX_OBJECTS; i++) if ((g->light_rx >> i) & 1u) g->obj[i].timer = 1;
        return;
    }
    g->light_ok = 1; g->light_epoch = g->ov_epoch; g->light_sig = g->signals; g->light_rx = 0;
    zero(g->lit, NGR_ROOM_BITS);
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        const NGRpgObjectDef *d = &g->def->objects[i];
        int16_t x, y;
        uint8_t dir, steps, focused = 0;
        if (d->type != NGR_O_EMITTER || !g->obj[i].alive) continue;
        if (d->in_mask && (g->signals & d->in_mask) != d->in_mask) continue;
        x = g->obj[i].x; y = g->obj[i].y; dir = (uint8_t)(d->p[0] & 3u);
        for (steps = 0; steps < 96u; steps++) {
            int8_t dx, dy;
            uint8_t o, f;
            dir_step(dir, &dx, &dy);
            x = (int16_t)(x + dx); y = (int16_t)(y + dy);
            if (x < 0 || y < 0 || x >= (int16_t)g->def->w || y >= (int16_t)g->def->h) break;
            o = ng_rpg_object_at(g, (uint8_t)x, (uint8_t)y);
            if (o != NGR_NONE) {
                uint8_t t = g->def->objects[o].type;
                if (t == NGR_O_MIRROR) {
                    /* '/': right<->up, left<->down; '\': right<->down, left<->up */
                    static const uint8_t slash[4] = { NGR_FACE_UP, NGR_FACE_LEFT, NGR_FACE_DOWN, NGR_FACE_RIGHT };
                    static const uint8_t back[4] = { NGR_FACE_DOWN, NGR_FACE_RIGHT, NGR_FACE_UP, NGR_FACE_LEFT };
                    bset(g->lit, tindex(g, (uint8_t)x, (uint8_t)y));
                    dir = g->obj[o].state ? back[dir] : slash[dir];
                    continue;
                }
                if (t == NGR_O_LENS) { focused = 1; bset(g->lit, tindex(g, (uint8_t)x, (uint8_t)y)); continue; }
                if (t == NGR_O_RECEIVER) {
                    if (!g->def->objects[o].p[0] || focused) {
                        bset(g->lit, tindex(g, (uint8_t)x, (uint8_t)y));
                        g->obj[o].timer = 1;
                        g->light_rx |= (uint32_t)1u << o;
                    }
                    break;
                }
                if (object_solid(g, o)) break;
            }
            f = ng_rpg_tile(g, (uint8_t)x, (uint8_t)y);
            if (f & NGR_T_SOLID) break;
            bset(g->lit, tindex(g, (uint8_t)x, (uint8_t)y));
        }
    }
}

/* Gear trains: from each driver, meshed sockets with a gear turn the other
 * way; a socket asked to turn both ways jams the whole train. */
static void NEOGEO_USER turn_gears(NGRpg *g)
{
    uint8_t q[NGR_MAX_OBJECTS], head = 0, tail = 0, i, jam = 0, turning = 0;
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) g->obj[i].aux = (g->def->objects[i].type == NGR_O_GEAR) ? 0u : g->obj[i].aux;
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        const NGRpgObjectDef *d = &g->def->objects[i];
        if (d->type != NGR_O_GEAR || !(d->p[0] & 1u)) continue;
        if (d->in_mask && (g->signals & d->in_mask) != d->in_mask) continue;
        g->obj[i].aux = 1;
        q[tail++] = i;
    }
    while (head < tail) {
        uint8_t c = q[head++], k;
        const NGRpgObjectDef *d = &g->def->objects[c];
        for (k = 2; k <= 4u; k++) {
            uint8_t nb = d->p[k], want;
            if (nb == NGR_NONE || nb >= g->def->nobjects || g->def->objects[nb].type != NGR_O_GEAR) continue;
            if (!g->obj[nb].state) continue;
            want = (uint8_t)(g->obj[c].aux == 1u ? 2u : 1u);
            if (!g->obj[nb].aux) { g->obj[nb].aux = want; if (tail < NGR_MAX_OBJECTS) q[tail++] = nb; }
            else if (g->obj[nb].aux != want) jam = 1;
        }
    }
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        if (g->def->objects[i].type != NGR_O_GEAR) continue;
        if (jam) g->obj[i].aux = 0;
        if (g->obj[i].aux) turning++;
    }
    if (turning != g->gear_turn || jam != g->gear_jam) emit(g, NGR_EV_GEARS, turning, jam, 0);
    g->gear_turn = turning; g->gear_jam = jam;
}

static uint8_t NEOGEO_USER gate_wants_open(const NGRpg *g, uint8_t i, uint32_t sig)
{
    const NGRpgObjectDef *d = &g->def->objects[i];
    uint8_t on;
    if (!d->in_mask) return g->obj[i].state;
    on = (uint8_t)((sig & d->in_mask) == d->in_mask);
    return (uint8_t)(d->p[0] & 1u ? !on : on);
}

/* Gates and bridges to match the signals. The solid and floor overlays are
 * rebuilt when one of them moves; with check set, also when anything else
 * they are made of changed (blocks, platforms, locks): once a frame. */
static void NEOGEO_USER set_gates(NGRpg *g, uint32_t sig, uint8_t check)
{
    uint8_t moved = 0;
    uint8_t i;
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        const NGRpgObjectDef *d = &g->def->objects[i];
        NGRpgObject *o = &g->obj[i];
        uint8_t want;
        if (d->type != NGR_O_GATE && d->type != NGR_O_BRIDGE) continue;
        if (d->flag && ng_rpg_flag(g, d->flag)) {                          /* open for good */
            if (!o->state) { o->state = 1; moved = 1; }
            continue;
        }
        want = gate_wants_open(g, i, sig);
        if (want == o->state) continue;
        /* a gate never closes on someone standing in it */
        if (!want && d->type == NGR_O_GATE) {
            uint8_t m, blocked = 0;
            for (m = 0; m < g->nparty; m++) if (covers(g, i, tile_of(g->party[m].x), tile_of(g->party[m].y))) blocked = 1;
            if (blocked) continue;
        }
        o->state = want;
        moved = 1;
        emit(g, NGR_EV_OPEN, i, want, 0);
        if (want && d->flag) { ng_rpg_set_flag(g, d->flag, 1); ng_rpg_run(g, d->actions); }
        else if (want) ng_rpg_run(g, d->actions);
    }
    if (moved) rebuild_overlays(g);
    else if (check) refresh_overlays(g);
}

/*
 * Signals, gates, water, light and gears, once a frame. Plates, pans,
 * levers, sequences, flags and balances give the base signals. Wet outlets,
 * lit receivers and turning target gears give derived ones, which can open
 * gates, start water sources and drive gears in turn: the passes repeat
 * (four at most) until the derived signals stop changing, starting from the
 * last frame's, so a longer chain finishes over the next frames. A network
 * that switches itself off flickers rather than hangs.
 */
static uint32_t NEOGEO_USER derived_signals(NGRpg *g)
{
    uint32_t sig = 0;
    uint8_t i;
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        const NGRpgObjectDef *d = &g->def->objects[i];
        const NGRpgObject *o = &g->obj[i];
        uint8_t on = 0;
        switch (d->type) {
        case NGR_O_OUTLET: on = bget(g->wet, tindex(g, o->x, o->y)); break;
        case NGR_O_RECEIVER: on = (uint8_t)(o->timer != 0); break;
        case NGR_O_GEAR: on = (uint8_t)((d->p[0] & 2u) && o->aux && (!d->p[1] || o->aux == d->p[1])); break;
        default: break;
        }
        if (on) sig |= d->out_mask;
    }
    return sig;
}

static void NEOGEO_USER networks(NGRpg *g)
{
    uint32_t base = 0, derived = g->derived, prev, old = g->signals;
    uint8_t i, pass_no;
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        const NGRpgObjectDef *d = &g->def->objects[i];
        NGRpgObject *o = &g->obj[i];
        uint8_t on = 0;
        switch (d->type) {
        case NGR_O_PLATE: {
            uint8_t w = weight_on(g, i), down = (uint8_t)(w >= (d->p[0] ? d->p[0] : 1u));
            if (d->p[1] && o->state) down = 1;
            if (down != o->state) { o->state = down; emit(g, NGR_EV_PLATE, i, down, 0); if (down) ng_rpg_run(g, d->actions); }
            on = o->state;
            break;
        }
        case NGR_O_PAN: o->aux = weight_on(g, i); break;
        case NGR_O_LEVER: on = o->state; break;
        case NGR_O_SEQUENCE: on = o->aux; break;
        case NGR_O_SIGNAL_FLAG: on = ng_rpg_flag(g, d->flag); break;
        case NGR_O_BALANCE: {
            uint8_t l = d->p[0], r = d->p[1];
            int16_t diff = 0;
            if (l < NGR_MAX_OBJECTS && r < NGR_MAX_OBJECTS) diff = (int16_t)((int16_t)weight_on(g, l) - (int16_t)weight_on(g, r));
            on = (uint8_t)(diff >= (int16_t)d->p[2]);
            o->state = on;
            break;
        }
        default: break;
        }
        if (on) base |= d->out_mask;
    }
    for (pass_no = 0; pass_no < 4u; pass_no++) {
        prev = derived;
        g->signals = base | derived;
        set_gates(g, g->signals, (uint8_t)(pass_no == 0));
        for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) if (g->def->objects[i].type == NGR_O_RECEIVER) g->obj[i].timer = 0;
        flow_water(g);
        trace_light(g);
        turn_gears(g);
        derived = derived_signals(g);
        if (derived == prev) break;
    }
    g->derived = derived;
    g->signals = base | derived;
    set_gates(g, g->signals, 0);
    /* events for outlets and receivers that changed */
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        const NGRpgObjectDef *d = &g->def->objects[i];
        NGRpgObject *o = &g->obj[i];
        if (d->type == NGR_O_OUTLET) {
            uint8_t wet = bget(g->wet, tindex(g, o->x, o->y));
            if (wet != o->state) { o->state = wet; emit(g, NGR_EV_WATER, i, wet, 0); if (wet) ng_rpg_run(g, d->actions); }
        } else if (d->type == NGR_O_RECEIVER) {
            uint8_t lit = o->timer;
            if (lit != o->state) { o->state = lit; emit(g, NGR_EV_LIGHT, i, lit, 0); if (lit) ng_rpg_run(g, d->actions); }
        }
    }
    if (g->signals != old) {
        uint8_t b;
        for (b = 0; b < 32u; b++) if (((g->signals ^ old) >> b) & 1u) emit(g, NGR_EV_SIGNAL, b, (uint8_t)((g->signals >> b) & 1u), 0);
    }
}

static void NEOGEO_USER sequence_input(NGRpg *g, uint8_t obj)
{
    uint8_t i;
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        const NGRpgObjectDef *d = &g->def->objects[i];
        NGRpgObject *o = &g->obj[i];
        uint8_t len = 0, k, listed = 0;
        if (d->type != NGR_O_SEQUENCE || o->aux) continue;
        for (k = 0; k < 6u && d->p[k] != NGR_NONE; k++) { len++; if (d->p[k] == obj) listed = 1; }
        if (!listed || !len) continue;
        if (d->p[o->state] == obj) {
            o->state++;
            emit(g, NGR_EV_SEQ_STEP, i, o->state, 0);
            if (o->state >= len) {
                o->aux = 1;
                emit(g, NGR_EV_SEQ_DONE, i, 0, 0);
                if (d->flag) ng_rpg_set_flag(g, d->flag, 1);
                ng_rpg_run(g, d->actions);
            }
        } else {
            o->state = (uint8_t)(d->p[0] == obj ? 1u : 0u);
            emit(g, NGR_EV_SEQ_WRONG, i, o->state, 0);
        }
    }
}

static void NEOGEO_USER open_object(NGRpg *g, uint8_t i)
{
    const NGRpgObjectDef *d;
    NGRpgObject *o;
    if (!g->def || i >= g->def->nobjects) return;
    d = &g->def->objects[i]; o = &g->obj[i];
    switch (d->type) {
    case NGR_O_LOCK: case NGR_O_GATE: case NGR_O_BRIDGE:
        if (!o->state) { o->state = 1; emit(g, NGR_EV_OPEN, i, 1, 0); if (d->flag) ng_rpg_set_flag(g, d->flag, 1); }
        break;
    case NGR_O_SECRET:
        if (!o->state) { o->state = 1; emit(g, NGR_EV_FOUND, i, 0, 0); if (d->flag) ng_rpg_set_flag(g, d->flag, 1); }
        break;
    case NGR_O_LEVER: case NGR_O_VALVE:
        o->state = 1; break;
    default: break;
    }
    rebuild_overlays(g);
}

/* ======================================================================
 * movement and the party
 * ==================================================================== */
static uint8_t NEOGEO_USER try_move(NGRpg *g, uint8_t mi, int32_t dx, int32_t dy, uint8_t is_leader)
{
    NGRpgMember *m = &g->party[mi];
    uint8_t moved = 0, dir;
    if (dx) {
        int32_t nx = m->x + dx;
        dir = dx > 0 ? NGR_FACE_RIGHT : NGR_FACE_LEFT;
        if (box_ok(g, m, nx, m->y, dir) && !(is_leader && npc_blocks(g, nx, m->y))) { m->x = nx; moved = 1; }
    }
    if (dy) {
        int32_t ny = m->y + dy;
        dir = dy > 0 ? NGR_FACE_DOWN : NGR_FACE_UP;
        if (box_ok(g, m, m->x, ny, dir) && !(is_leader && npc_blocks(g, m->x, ny))) { m->y = ny; moved = 1; }
    }
    return moved;
}

static void NEOGEO_USER push_block(NGRpg *g)
{
    NGRpgMember *m = lead(g);
    int8_t dx, dy;
    uint8_t tx, ty, bx, by, o;
    int16_t nx, ny;
    dir_step(m->facing, &dx, &dy);
    tx = tile_of(m->x); ty = tile_of(m->y);
    bx = (uint8_t)(tx + dx); by = (uint8_t)(ty + dy);
    o = ng_rpg_object_at(g, bx, by);
    if (o == NGR_NONE || g->def->objects[o].type != NGR_O_BLOCK) { m->push = 0; return; }
    if (g->def->objects[o].p[0] && !(m->abilities & NGR_AB_PUSH)) { m->push = 0; return; }
    if (++m->push < PUSH_FRAMES) return;
    m->push = 0;
    nx = (int16_t)(bx + dx); ny = (int16_t)(by + dy);
    if (nx < 0 || ny < 0 || nx >= (int16_t)g->def->w || ny >= (int16_t)g->def->h) return;
    {
        uint8_t f = ng_rpg_tile(g, (uint8_t)nx, (uint8_t)ny), k;
        for (k = 0; k < g->nparty; k++) if (tile_of(g->party[k].x) == (uint8_t)nx && tile_of(g->party[k].y) == (uint8_t)ny) return;
        if (ng_rpg_object_at(g, (uint8_t)nx, (uint8_t)ny) != NGR_NONE) {
            uint8_t t = g->def->objects[ng_rpg_object_at(g, (uint8_t)nx, (uint8_t)ny)].type;
            if (t != NGR_O_PLATE && t != NGR_O_PAN) return;
        }
        if (f & NGR_T_PIT) {
            g->obj[o].x = (uint8_t)nx; g->obj[o].y = (uint8_t)ny;
            g->obj[o].alive = 0; g->obj[o].aux = 1;
            emit(g, NGR_EV_FILL, o, 0, 0);
            rebuild_overlays(g);
            return;
        }
        if (f & (NGR_T_SOLID | NGR_T_WATER | NGR_T_NARROW | NGR_T_CLIMB | NGR_T_LEDGE)) return;
    }
    g->obj[o].x = (uint8_t)nx; g->obj[o].y = (uint8_t)ny;
    emit(g, NGR_EV_PUSH, o, m->facing, 0);
    rebuild_overlays(g);
}

static void NEOGEO_USER record_trail(NGRpg *g)
{
    NGRpgMember *m = lead(g);
    int16_t x = (int16_t)(m->x >> 8), y = (int16_t)(m->y >> 8);
    int16_t dx = (int16_t)(x - g->trail_x[g->trail_head]), dy = (int16_t)(y - g->trail_y[g->trail_head]);
    if (dx < 0) dx = (int16_t)-dx;
    if (dy < 0) dy = (int16_t)-dy;
    if (dx + dy < 6) return;
    g->trail_head = (uint8_t)((g->trail_head + 1u) & (NGR_TRAIL - 1u));
    g->trail_x[g->trail_head] = x; g->trail_y[g->trail_head] = y;
    if (g->trail_n < NGR_TRAIL) g->trail_n++;
}

static void NEOGEO_USER move_leader(NGRpg *g)
{
    NGRpgMember *m = lead(g);
    int32_t sp = (int32_t)m->speed << 4, dx = 0, dy = 0;
    uint8_t d = g->in_dirs;
    if (d & NGR_IN_LEFT) dx = -sp; else if (d & NGR_IN_RIGHT) dx = sp;
    if (d & NGR_IN_UP) dy = -sp; else if (d & NGR_IN_DOWN) dy = sp;
    if (dx && dy) { dx = (dx * 181) >> 8; dy = (dy * 181) >> 8; }
    m->moving = (uint8_t)(dx || dy);
    if (dx > 0) m->facing = NGR_FACE_RIGHT; else if (dx < 0) m->facing = NGR_FACE_LEFT;
    if (dy > 0 && !dx) m->facing = NGR_FACE_DOWN; else if (dy < 0 && !dx) m->facing = NGR_FACE_UP;
    if (!m->moving) { m->push = 0; return; }
    if (!try_move(g, g->leader, dx, dy, 1) && !(dx && dy)) push_block(g);
    else m->push = 0;
    record_trail(g);
}

/* Breadth-first path from member mi to tile (gx, gy) inside a window around
 * both. Fills its path; returns 1 if found. */
static uint8_t NEOGEO_USER find_path(NGRpg *g, uint8_t mi, uint8_t gx, uint8_t gy)
{
    NGRpgMember *m = &g->party[mi];
    uint8_t sx = tile_of(m->x), sy = tile_of(m->y), ox, oy, found = 0;
    uint16_t head = 0, tail = 0, cells = (uint16_t)(NGR_NAV_W * NGR_NAV_H), start, goal;
    int16_t cx = (int16_t)((sx + gx) >> 1), cy = (int16_t)((sy + gy) >> 1);
    int16_t x0 = (int16_t)(cx - (int16_t)(NGR_NAV_W / 2u)), y0 = (int16_t)(cy - (int16_t)(NGR_NAV_H / 2u));
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    ox = (uint8_t)x0; oy = (uint8_t)y0;
    if (gx < ox || gy < oy || sx < ox || sy < oy || gx >= ox + NGR_NAV_W || gy >= oy + NGR_NAV_H
        || sx >= ox + NGR_NAV_W || sy >= oy + NGR_NAV_H) return 0;
    zero(g->nav_dir, cells);
    start = (uint16_t)((uint16_t)(sy - oy) * NGR_NAV_W + (sx - ox));
    goal = (uint16_t)((uint16_t)(gy - oy) * NGR_NAV_W + (gx - ox));
    g->nav_dir[start] = 5;
    g->nav_q[tail++] = start;
    while (head < tail) {
        uint16_t c = g->nav_q[head++];
        uint8_t lx, ly = 0, d;
        uint16_t r = c;
        if (c == goal) { found = 1; break; }
        while (r >= NGR_NAV_W) { r = (uint16_t)(r - NGR_NAV_W); ly++; }
        lx = (uint8_t)r;
        for (d = 0; d < 4u; d++) {
            int8_t dx, dy;
            int16_t nx, ny;
            uint16_t nc;
            dir_step(d, &dx, &dy);
            nx = (int16_t)(lx + dx); ny = (int16_t)(ly + dy);
            if (nx < 0 || ny < 0 || nx >= (int16_t)NGR_NAV_W || ny >= (int16_t)NGR_NAV_H) continue;
            nc = (uint16_t)((uint16_t)ny * NGR_NAV_W + (uint16_t)nx);
            if (g->nav_dir[nc]) continue;
            if (!pass(g, m->abilities, 0, (uint8_t)(ox + nx), (uint8_t)(oy + ny), d)) continue;
            if (ng_rpg_tile(g, (uint8_t)(ox + nx), (uint8_t)(oy + ny)) & NGR_T_HAZARD) continue;
            g->nav_dir[nc] = (uint8_t)(d + 1u);
            g->nav_q[tail++] = nc;
        }
    }
    if (!found) return 0;
    {   /* walk back from the goal, then keep the first steps */
        uint8_t back[NGR_NAV_W * NGR_NAV_H / 4u];
        uint16_t c = goal, n = 0, limit = (uint16_t)sizeof(back);
        while (c != start && n < limit) {
            uint8_t d = (uint8_t)(g->nav_dir[c] - 1u);
            int8_t dx, dy;
            back[n++] = d;
            dir_step(d, &dx, &dy);
            c = (uint16_t)((int16_t)c - (int16_t)dy * (int16_t)NGR_NAV_W - dx);
        }
        if (c != start) return 0;
        m->path_len = 0;
        while (n && m->path_len < NGR_PATH_MAX) m->path[m->path_len++] = back[--n];
        m->path_pos = 0;
        m->blocked = 0;
        m->wx = sx; m->wy = sy;
        if (m->path_len) {
            int8_t dx, dy;
            dir_step(m->path[0], &dx, &dy);
            m->wx = (uint8_t)(sx + dx); m->wy = (uint8_t)(sy + dy);
        }
    }
    return 1;
}

static int32_t NEOGEO_USER manhattan(int32_t ax, int32_t ay, int32_t bx, int32_t by)
{
    int32_t dx = ax - bx, dy = ay - by;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx + dy;
}

/* One step along a member's path; returns 1 while walking. Each step
 * walks to the centre of the next tile; a step that makes no headway for a
 * while drops the path (the caller looks again). */
static uint8_t NEOGEO_USER walk_path(NGRpg *g, uint8_t mi)
{
    NGRpgMember *m = &g->party[mi];
    int32_t cx, cy, dx, dy, mx, my, sp = (int32_t)m->speed << 4;
    if (m->path_pos >= m->path_len) return 0;
    cx = tile_centre(m->wx); cy = tile_centre(m->wy);
    dx = cx - m->x; dy = cy - m->y;
    mx = dx > sp ? sp : dx < -sp ? -sp : dx;
    my = dy > sp ? sp : dy < -sp ? -sp : dy;
    m->facing = m->path[m->path_pos];
    m->moving = 1;
    if (!try_move(g, mi, mx, my, 0)) {
        if (++m->blocked > 20u) { m->path_len = m->path_pos = 0; m->blocked = 0; return 0; }
        return 1;
    }
    m->blocked = 0;
    if (manhattan(m->x, m->y, cx, cy) <= (1 << 8)) {
        m->path_pos++;
        if (m->path_pos < m->path_len) {
            int8_t sx, sy;
            dir_step(m->path[m->path_pos], &sx, &sy);
            m->wx = (uint8_t)(m->wx + sx); m->wy = (uint8_t)(m->wy + sy);
        }
    }
    return 1;
}


static void NEOGEO_USER step_off_doorway(NGRpg *g, uint8_t mi)
{
    NGRpgMember *m = &g->party[mi];
    uint8_t tx = tile_of(m->x), ty = tile_of(m->y), nx, ny;
    if (!(ng_rpg_tile(g, tx, ty) & (NGR_T_DOOR | NGR_T_NARROW))) return;
    if (m->path_pos < m->path_len) return;
    if (near_tile(g, mi, tx, ty, &nx, &ny, 1)) find_path(g, mi, nx, ny);
}

static void NEOGEO_USER warp_beside_leader(NGRpg *g, uint8_t mi)
{
    NGRpgMember *m = &g->party[mi], *l = lead(g);
    uint8_t x, y;
    if (!near_tile(g, mi, tile_of(l->x), tile_of(l->y), &x, &y, 1)) return;
    m->x = tile_centre(x); m->y = tile_centre(y);
    m->stuck = 0; m->path_len = 0; m->path_pos = 0;
    emit(g, NGR_EV_TELEPORT, mi, 0, 0);
}

static void NEOGEO_USER move_follower(NGRpg *g, uint8_t mi, uint8_t rank)
{
    NGRpgMember *m = &g->party[mi], *l = lead(g);
    int32_t tx, ty, d_before, d_after, sp = (int32_t)m->speed << 4;
    uint8_t lag;
    m->moving = 0;
    if (m->mode == NGR_MODE_WAIT) { step_off_doorway(g, mi); walk_path(g, mi); return; }
    if (m->mode == NGR_MODE_ASSIST) {
        if (m->path_pos < m->path_len) { walk_path(g, mi); return; }
        if (tile_of(m->x) != m->tx || tile_of(m->y) != m->ty) {
            if (!find_path(g, mi, m->tx, m->ty)) {
                if (m->stuck < 255u) m->stuck++;
                if (m->stuck == STUCK_PATH) emit(g, NGR_EV_STUCK, mi, 0, 0);
            }
        }
        return;
    }
    /* following: walk to a point of the leader's trail */
    if (m->path_pos < m->path_len) {
        walk_path(g, mi);
        if (m->path_pos >= m->path_len) m->stuck = 0;
        return;
    }
    lag = (uint8_t)(rank * 3u);
    if (g->trail_n > lag) {
        uint8_t k = (uint8_t)((g->trail_head - lag) & (NGR_TRAIL - 1u));
        tx = (int32_t)g->trail_x[k] << 8; ty = (int32_t)g->trail_y[k] << 8;
    } else {
        tx = l->x; ty = l->y;
    }
    d_before = manhattan(m->x, m->y, l->x, l->y);
    if (manhattan(m->x, m->y, tx, ty) <= (3 << 8) || d_before <= ((int32_t)(10u + 8u * rank) << 8)) {
        m->stuck = 0;
        if (!l->moving) step_off_doorway(g, mi);
        return;
    }
    {
        int32_t dx = tx - m->x, dy = ty - m->y;
        int32_t mx = dx > sp ? sp : dx < -sp ? -sp : dx;
        int32_t my = dy > sp ? sp : dy < -sp ? -sp : dy;
        if (mx > 0) m->facing = NGR_FACE_RIGHT; else if (mx < 0) m->facing = NGR_FACE_LEFT;
        if (!mx) m->facing = my > 0 ? NGR_FACE_DOWN : NGR_FACE_UP;
        m->moving = (uint8_t)(try_move(g, mi, mx, my, 0));
    }
    d_after = manhattan(m->x, m->y, l->x, l->y);
    if (d_after >= d_before) {
        if (m->stuck < 255u) m->stuck++;
    } else if (m->stuck) {
        m->stuck--;
    }
    if (m->stuck == STUCK_PATH) {
        emit(g, NGR_EV_STUCK, mi, 0, 0);
        if (find_path(g, mi, tile_of(l->x), tile_of(l->y))) m->stuck = (uint8_t)(STUCK_PATH + 1u);
    }
    if (m->stuck >= STUCK_WARP && !(g->world->flags & NGR_WF_NO_TELEPORT)) warp_beside_leader(g, mi);
}

/* ======================================================================
 * creatures, hazards, platforms, pickups
 * ==================================================================== */
static void NEOGEO_USER hurt(NGRpg *g, uint8_t mi, uint8_t dmg, int32_t from_x, int32_t from_y)
{
    NGRpgMember *m = &g->party[mi];
    if (m->invuln || g->down || !dmg) return;
    m->hp = (uint8_t)(m->hp > dmg ? (uint8_t)(m->hp - dmg) : 0u);
    m->invuln = INVULN;
    emit(g, NGR_EV_HURT, mi, m->hp, 0);
    {   /* a small push away */
        int32_t kx = m->x > from_x ? (4 << 8) : m->x < from_x ? -(4 << 8) : 0;
        int32_t ky = m->y > from_y ? (4 << 8) : m->y < from_y ? -(4 << 8) : 0;
        try_move(g, mi, kx, ky, 0);
    }
    if (!m->hp && mi == g->leader) { g->down = 1; emit(g, NGR_EV_DOWN, mi, 0, 0); }
}

static void NEOGEO_USER move_thing(NGRpg *g, NGRpgObject *o, int32_t dx, int32_t dy)
{
    int32_t nx = o->px + dx, ny = o->py + dy;
    if (nx < (8 << 8) || ny < (8 << 8)) return;
    if (pass(g, 0, 1, tile_of(nx), tile_of(ny), NGR_FACE_DOWN) && !bget(g->solid, tindex(g, tile_of(nx), tile_of(ny)))) {
        o->px = nx; o->py = ny;
    } else {
        o->facing = (uint8_t)(rnd(g) & 3u);
        o->timer = 0;
    }
}

static void NEOGEO_USER update_movers(NGRpg *g)
{
    uint8_t i, mi;
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        const NGRpgObjectDef *d = &g->def->objects[i];
        NGRpgObject *o = &g->obj[i];
        if (!o->alive) continue;
        if (d->type == NGR_O_NPC || d->type == NGR_O_CREATURE) {
            uint8_t beh = d->p[0];
            int32_t sp = (int32_t)(d->type == NGR_O_NPC ? d->p[1] : d->p[3]) << 4;
            if (!sp) sp = 12 << 4;
            if (o->timer) o->timer--;
            if (d->type == NGR_O_CREATURE && o->state) { o->state--; continue; }   /* stunned */
            if (d->type == NGR_O_CREATURE && beh == 3u) {
                NGRpgMember *l = lead(g);
                int32_t dist = manhattan(o->px, o->py, l->x, l->y);
                if (dist < ((int32_t)(d->p[4] ? d->p[4] : 4u) << 12)) {
                    int32_t dx = l->x > o->px ? sp : l->x < o->px ? -sp : 0;
                    int32_t dy = l->y > o->py ? sp : l->y < o->py ? -sp : 0;
                    move_thing(g, o, dx, dy);
                    goto contact;
                }
                beh = 1;
            }
            if (beh == 1u) {
                int8_t dx, dy;
                if (!o->timer) { o->timer = (uint8_t)(48u + (rnd(g) & 63u)); o->facing = (uint8_t)rnd_range(g, 5); }
                if (o->facing < 4u && o->timer > 24u) {
                    dir_step(o->facing, &dx, &dy);
                    move_thing(g, o, dx * sp, dy * sp);
                }
            } else if (beh == 2u) {
                int32_t ox = tile_centre(d->x), oy = tile_centre(d->y);
                int32_t ex = tile_centre((uint8_t)(d->x + (int8_t)d->p[d->type == NGR_O_NPC ? 2 : 5]));
                int32_t ey = tile_centre((uint8_t)(d->y + (int8_t)d->p[d->type == NGR_O_NPC ? 3 : 6]));
                int32_t gx = o->aux ? ox : ex, gy = o->aux ? oy : ey;
                int32_t dx = gx - o->px, dy = gy - o->py;
                if (manhattan(o->px, o->py, gx, gy) <= sp) o->aux ^= 1u;
                else move_thing(g, o, dx > 0 ? sp : dx < 0 ? -sp : 0, dy > 0 ? sp : dy < 0 ? -sp : 0);
            }
        contact:
            if (d->type == NGR_O_CREATURE && !g->down)
                for (mi = 0; mi < g->nparty; mi++) {
                    NGRpgMember *m = &g->party[mi];
                    int32_t dx = m->x - o->px, dy = m->y - o->py;
                    if (dx < 0) dx = -dx;
                    if (dy < 0) dy = -dy;
                    if (dx < (10 << 8) && dy < (10 << 8)) hurt(g, mi, d->p[2] ? d->p[2] : 1u, o->px, o->py);
                }
        } else if (d->type == NGR_O_PLATFORM) {
            int32_t ox = tile_centre(d->x), oy = tile_centre(d->y);
            int32_t ex = tile_centre((uint8_t)(d->x + (int8_t)d->p[0])), ey = tile_centre((uint8_t)(d->y + (int8_t)d->p[1]));
            int32_t sp = (int32_t)(d->p[2] ? d->p[2] : 8u) << 4;
            int32_t gx = o->aux ? ox : ex, gy = o->aux ? oy : ey, dx, dy, mx, my;
            if (d->in_mask && (g->signals & d->in_mask) != d->in_mask) continue;   /* stopped where it is */
            if (o->timer) { o->timer--; continue; }
            dx = gx - o->px; dy = gy - o->py;
            mx = dx > sp ? sp : dx < -sp ? -sp : dx;
            my = dy > sp ? sp : dy < -sp ? -sp : dy;
            /* carry whoever stands on it */
            for (mi = 0; mi < g->nparty; mi++) {
                NGRpgMember *m = &g->party[mi];
                if (tile_of(m->x) == tile_of(o->px) && tile_of(m->y) == tile_of(o->py)) { m->x += mx; m->y += my; }
            }
            o->px += mx; o->py += my;
            if (!mx && !my) { o->aux ^= 1u; o->timer = d->p[3]; }
        } else if (d->type == NGR_O_HAZARD) {
            uint8_t period = (uint8_t)(d->p[0] ? d->p[0] : 16u), on = d->p[1];
            o->timer++;
            if (o->timer >= (uint8_t)(period << 2)) o->timer = 0;
            o->state = (uint8_t)(o->timer < (uint8_t)(on << 2));
            if (o->state)
                for (mi = 0; mi < g->nparty; mi++)
                    if (covers(g, i, tile_of(g->party[mi].x), tile_of(g->party[mi].y)))
                        hurt(g, mi, d->p[2] ? d->p[2] : 1u, tile_centre(o->x), tile_centre(o->y));
        } else if (d->type == NGR_O_PICKUP) {
            for (mi = 0; mi < g->nparty; mi++) {
                if (!covers(g, i, tile_of(g->party[mi].x), tile_of(g->party[mi].y))) continue;
                if (d->p[0] != NGR_NONE) ng_rpg_give(g, d->p[0], d->p[1] ? d->p[1] : 1u);
                if (d->p[2] != NGR_NONE && !bget(g->collected, d->p[2])) { bset(g->collected, d->p[2]); emit(g, NGR_EV_COLLECT, d->p[2], 0, 0); }
                if (d->flag) ng_rpg_set_flag(g, d->flag, 1);
                o->alive = 0;
                ng_rpg_run(g, d->actions);
                break;
            }
        }
    }
    /* thorns and steam in the tiles */
    for (mi = 0; mi < g->nparty; mi++) {
        NGRpgMember *m = &g->party[mi];
        if (ng_rpg_tile(g, tile_of(m->x), tile_of(m->y)) & NGR_T_HAZARD) hurt(g, mi, 1, m->x, m->y - (8 << 8));
    }
}

/* ======================================================================
 * triggers and exits
 * ==================================================================== */
static uint8_t NEOGEO_USER in_rect(uint8_t x, uint8_t y, uint8_t rx, uint8_t ry, uint8_t rw, uint8_t rh)
{
    return (uint8_t)(x >= rx && x < (uint8_t)(rx + (rw ? rw : 1u)) && y >= ry && y < (uint8_t)(ry + (rh ? rh : 1u)));
}

static void NEOGEO_USER fire(NGRpg *g, const NGRpgTrigger *t)
{
    if (t->once_flag && ng_rpg_flag(g, t->once_flag)) return;
    if (!ng_rpg_check(g, &t->cond)) return;
    if (t->once_flag) ng_rpg_set_flag(g, t->once_flag, 1);
    ng_rpg_run(g, t->actions);
}

static void NEOGEO_USER run_triggers(NGRpg *g, uint8_t when)
{
    uint8_t i, lx = tile_of(lead(g)->x), ly = tile_of(lead(g)->y), room = g->room;
    for (i = 0; i < g->def->ntriggers && i < 32u; i++) {
        const NGRpgTrigger *t = &g->def->triggers[i];
        uint8_t inside, was = (uint8_t)((g->in_trig[i >> 3] >> (i & 7u)) & 1u);
        if (t->when != when) continue;
        if (when == NGR_TRIG_ROOM) { fire(g, t); if (g->room != room) return; continue; }
        if (when == NGR_TRIG_WATCH) inside = ng_rpg_check(g, &t->cond);
        else inside = in_rect(lx, ly, t->x, t->y, t->w, t->h);
        if (inside) g->in_trig[i >> 3] = (uint8_t)(g->in_trig[i >> 3] | (1u << (i & 7u)));
        else g->in_trig[i >> 3] = (uint8_t)(g->in_trig[i >> 3] & ~(1u << (i & 7u)));
        if (!inside) continue;
        if ((when == NGR_TRIG_ENTER || when == NGR_TRIG_WATCH) && was) continue;
        fire(g, t);
        if (g->room != room) return;
    }
}

static void NEOGEO_USER check_exits(NGRpg *g)
{
    uint8_t i, lx = tile_of(lead(g)->x), ly = tile_of(lead(g)->y), now = 0;
    for (i = 0; i < g->def->nexits; i++) {
        const NGRpgExit *e = &g->def->exits[i];
        if (!in_rect(lx, ly, e->x, e->y, e->w, e->h)) continue;
        now = (uint8_t)(i + 1u);
        break;
    }
    if (now == g->in_exit) return;
    if (g->in_exit == 0xFFu && now) return;     /* still in the doorway we came through */
    g->in_exit = now;
    if (!now) return;
    {
        const NGRpgExit *e = &g->def->exits[now - 1u];
        if (!ng_rpg_check(g, &e->cond)) { emit(g, NGR_EV_LOCKED, (uint8_t)(now - 1u), 0, e->locked_text); return; }
        g->pending_exit = now;
        emit(g, NGR_EV_EXIT, e->room, (uint8_t)(now - 1u), 0);
        if (g->world->flags & NGR_WF_AUTO_EXIT) ng_rpg_take_exit(g);
    }
}

/* ======================================================================
 * the frame
 * ==================================================================== */
void NEOGEO_USER ng_rpg_input(NGRpg *g, uint8_t dirs)
{
    if (g) g->in_dirs = dirs;
}

void NEOGEO_USER ng_rpg_update(NGRpg *g)
{
    uint8_t i, rank = 0, room;
    if (!g || !g->def || !g->nparty) return;
    g->tick++;
    for (i = 0; i < g->nparty; i++) if (g->party[i].invuln) g->party[i].invuln--;
    if (g->dlg_open || g->down) { networks(g); return; }
    room = g->room;
    move_leader(g);
    for (i = 0; i < g->nparty; i++) {
        if (i == g->leader) continue;
        rank++;
        move_follower(g, i, rank);
    }
    update_movers(g);
    if (g->room != room || g->down) return;
    networks(g);
    run_triggers(g, NGR_TRIG_ENTER);
    if (g->room != room) return;
    run_triggers(g, NGR_TRIG_STEP);
    if (g->room != room) return;
    run_triggers(g, NGR_TRIG_WATCH);
    if (g->room != room) return;
    check_exits(g);
}

/* ======================================================================
 * dialogue
 * ==================================================================== */
static void NEOGEO_USER show_node(NGRpg *g, uint16_t node)
{
    uint8_t hops = 0;
    const NGRpgWorld *w = g->world;
    while (node != NGR_END && node < w->ndialogue && hops++ < 16u) {
        const NGRpgDialogueNode *n = &w->dialogue[node];
        if (n->cond.op == NGR_C_ALWAYS || ng_rpg_check(g, &n->cond)) break;
        node = n->alt;
    }
    if (node == NGR_END || node >= w->ndialogue) {
        g->dlg_open = 0;
        emit(g, NGR_EV_DIALOGUE_END, 0, 0, 0);
        return;
    }
    g->dlg_open = 1;
    g->dlg_node = node;
    emit(g, NGR_EV_DIALOGUE, w->dialogue[node].speaker, w->dialogue[node].nchoices, node);
    ng_rpg_run(g, w->dialogue[node].actions);
}

int NEOGEO_USER ng_rpg_dialogue_start(NGRpg *g, uint16_t node)
{
    if (!g || !g->world || node >= g->world->ndialogue) return NGR_ERR_ARG;
    show_node(g, node);
    return NGR_OK;
}

uint8_t NEOGEO_USER ng_rpg_dialogue_offered(const NGRpg *g)
{
    const NGRpgDialogueNode *n;
    uint8_t i, bits = 0;
    if (!g || !g->dlg_open) return 0;
    n = &g->world->dialogue[g->dlg_node];
    for (i = 0; i < n->nchoices && i < 3u; i++)
        if (ng_rpg_check(g, &n->choice_cond[i])) bits = (uint8_t)(bits | (1u << i));
    return bits;
}

int NEOGEO_USER ng_rpg_dialogue_next(NGRpg *g)
{
    const NGRpgDialogueNode *n;
    if (!g || !g->dlg_open) return NGR_ERR_STATE;
    n = &g->world->dialogue[g->dlg_node];
    if (n->nchoices && ng_rpg_dialogue_offered(g)) return NGR_ERR_STATE;   /* a choice is wanted */
    show_node(g, n->next);
    return NGR_OK;
}

int NEOGEO_USER ng_rpg_dialogue_choose(NGRpg *g, uint8_t i)
{
    const NGRpgDialogueNode *n;
    if (!g || !g->dlg_open) return NGR_ERR_STATE;
    n = &g->world->dialogue[g->dlg_node];
    if (i >= n->nchoices || !((ng_rpg_dialogue_offered(g) >> i) & 1u)) return NGR_ERR_ARG;
    show_node(g, n->choice_next[i]);
    return NGR_OK;
}

/* ======================================================================
 * commands
 * ==================================================================== */
static void NEOGEO_USER facing_tile(NGRpg *g, uint8_t *tx, uint8_t *ty)
{
    NGRpgMember *m = lead(g);
    int8_t dx, dy;
    dir_step(m->facing, &dx, &dy);
    *tx = (uint8_t)(tile_of(m->x) + dx);
    *ty = (uint8_t)(tile_of(m->y) + dy);
}

static uint8_t NEOGEO_USER near_npc(NGRpg *g)
{
    NGRpgMember *m = lead(g);
    uint8_t i;
    int8_t dx, dy;
    int32_t fx, fy;
    dir_step(m->facing, &dx, &dy);
    fx = m->x + (int32_t)dx * (12 << 8); fy = m->y + (int32_t)dy * (12 << 8);
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        const NGRpgObject *o = &g->obj[i];
        if (g->def->objects[i].type != NGR_O_NPC || !o->alive) continue;
        if (manhattan(o->px, o->py, fx, fy) < (14 << 8)) return i;
    }
    return NGR_NONE;
}

int NEOGEO_USER ng_rpg_interact(NGRpg *g)
{
    uint8_t tx, ty, o, i;
    const NGRpgObjectDef *d;
    NGRpgObject *ob;
    NGRpgMember *m;
    if (!g || !g->def || !g->nparty) return NGR_ERR_ARG;
    if (g->dlg_open || g->down) return NGR_ERR_STATE;
    m = lead(g);
    o = near_npc(g);
    if (o == NGR_NONE) {
        facing_tile(g, &tx, &ty);
        o = ng_rpg_object_at(g, tx, ty);
    } else {
        tx = tile_of(g->obj[o].px); ty = tile_of(g->obj[o].py);
    }
    if (o == NGR_NONE) {
        for (i = 0; i < g->def->ntriggers; i++) {
            const NGRpgTrigger *t = &g->def->triggers[i];
            if (t->when == NGR_TRIG_INTERACT && in_rect(tx, ty, t->x, t->y, t->w, t->h)) { fire(g, t); return NGR_OK; }
        }
        return NGR_ERR_STATE;
    }
    d = &g->def->objects[o]; ob = &g->obj[o];
    if ((d->p[6] & 1u) && !(m->abilities & NGR_AB_FINE)) return NGR_ERR_NOPE;
    switch (d->type) {
    case NGR_O_NPC:
        ob->facing = (uint8_t)((m->facing + 2u) & 3u);
        return ng_rpg_dialogue_start(g, d->text);
    case NGR_O_SIGN:
        return ng_rpg_dialogue_start(g, d->text);
    case NGR_O_CHEST:
        if (ob->state) return NGR_ERR_STATE;
        ob->state = 1;
        if (d->p[0] != NGR_NONE) ng_rpg_give(g, d->p[0], d->p[1] ? d->p[1] : 1u);
        if (d->p[2] != NGR_NONE && !bget(g->collected, d->p[2])) { bset(g->collected, d->p[2]); emit(g, NGR_EV_COLLECT, d->p[2], 0, 0); }
        if (d->flag) ng_rpg_set_flag(g, d->flag, 1);
        emit(g, NGR_EV_OPEN, o, 1, 0);
        ng_rpg_run(g, d->actions);
        return NGR_OK;
    case NGR_O_REST:
        for (i = 0; i < g->nparty; i++) g->party[i].hp = g->party[i].max_hp;
        checkpoint_here(g);
        emit(g, NGR_EV_REST, o, 0, 0);
        ng_rpg_run(g, d->actions);
        return NGR_OK;
    case NGR_O_LOCK:
        if (ob->state) return NGR_ERR_STATE;
        if (!ng_rpg_count(g, d->p[0])) { emit(g, NGR_EV_LOCKED, NGR_NONE, o, d->text); return NGR_ERR_NOPE; }
        if (d->p[1]) ng_rpg_take(g, d->p[0], 1);
        open_object(g, o);
        ng_rpg_run(g, d->actions);
        return NGR_OK;
    case NGR_O_SECRET:
        if (ob->state || d->p[0]) return NGR_ERR_NOPE;
        open_object(g, o);
        ng_rpg_run(g, d->actions);
        return NGR_OK;
    case NGR_O_LEVER:
        ob->state ^= 1u;
        if (d->flag) ng_rpg_set_flag(g, d->flag, ob->state);
        emit(g, NGR_EV_LEVER, o, ob->state, 0);
        sequence_input(g, o);
        if (ob->state) ng_rpg_run(g, d->actions);
        return NGR_OK;
    case NGR_O_VALVE:
        ob->state ^= 1u;
        emit(g, NGR_EV_LEVER, o, ob->state, 0);
        sequence_input(g, o);
        return NGR_OK;
    case NGR_O_MIRROR:
        ob->state ^= 1u;
        emit(g, NGR_EV_LEVER, o, ob->state, 0);
        return NGR_OK;
    case NGR_O_BELL:
        emit(g, NGR_EV_TONE, o, d->p[0], 0);
        sequence_input(g, o);
        return NGR_OK;
    case NGR_O_GEAR:
        if (d->p[0] & 3u) return NGR_ERR_STATE;       /* the driver and the target are fixed */
        if (!ob->state) {
            if (!d->p[5] || !ng_rpg_count(g, d->p[5])) return NGR_ERR_NOPE;
            ng_rpg_take(g, d->p[5], 1);
            ob->state = 1;
            if (d->flag) ng_rpg_set_flag(g, d->flag, 1);    /* a socket with a flag keeps it for good */
        } else {
            if (d->flag && ng_rpg_flag(g, d->flag)) return NGR_ERR_STATE;   /* fixed for good */
            if (d->p[5] && !ng_rpg_give(g, d->p[5], 1)) return NGR_ERR_NOPE;
            ob->state = 0;
        }
        emit(g, NGR_EV_LEVER, o, ob->state, 0);
        return NGR_OK;
    default:
        return NGR_ERR_STATE;
    }
}

int NEOGEO_USER ng_rpg_use_item(NGRpg *g, uint8_t item)
{
    const NGRpgItemDef *d;
    uint8_t tx, ty, o;
    if (!g || !g->def || !g->nparty) return NGR_ERR_ARG;
    if (g->dlg_open || g->down) return NGR_ERR_STATE;
    d = item_def(g, item);
    if (!d || !ng_rpg_count(g, item)) return NGR_ERR_NOPE;
    if (d->flags & NGR_I_CONSUME) {
        NGRpgMember *m = lead(g);
        uint16_t hp = (uint16_t)(m->hp + (d->param ? d->param : 1u));
        if (m->hp >= m->max_hp) return NGR_ERR_NOPE;
        m->hp = (uint8_t)(hp > m->max_hp ? m->max_hp : hp);
        ng_rpg_take(g, item, 1);
        emit(g, NGR_EV_USE, item, NGR_NONE, 0);
        return NGR_OK;
    }
    facing_tile(g, &tx, &ty);
    o = ng_rpg_object_at(g, tx, ty);
    emit(g, NGR_EV_USE, item, o, 0);
    if (o != NGR_NONE) {
        const NGRpgObjectDef *od = &g->def->objects[o];
        NGRpgObject *ob = &g->obj[o];
        if (od->type == NGR_O_SECRET && !ob->state && od->p[0] == item) { open_object(g, o); ng_rpg_run(g, od->actions); return NGR_OK; }
        if (od->type == NGR_O_LOCK && !ob->state && od->p[0] == item) { if (od->p[1]) ng_rpg_take(g, item, 1); open_object(g, o); ng_rpg_run(g, od->actions); return NGR_OK; }
        if (od->type == NGR_O_GEAR && !ob->state && od->p[5] == item) { ng_rpg_take(g, item, 1); ob->state = 1; emit(g, NGR_EV_LEVER, o, 1, 0); return NGR_OK; }
    }
    {   /* tool triggers: an interact trigger whose condition asks for this item */
        uint8_t i;
        for (i = 0; i < g->def->ntriggers; i++) {
            const NGRpgTrigger *t = &g->def->triggers[i];
            if (t->when == NGR_TRIG_INTERACT && t->cond.op == NGR_C_ITEM && t->cond.a == item && in_rect(tx, ty, t->x, t->y, t->w, t->h)) {
                fire(g, t);
                return NGR_OK;
            }
        }
    }
    return NGR_ERR_NOPE;
}

int NEOGEO_USER ng_rpg_strike(NGRpg *g)
{
    NGRpgMember *m;
    int8_t dx, dy;
    int32_t fx, fy;
    uint8_t i, hit = 0;
    if (!g || !g->def || !g->nparty) return NGR_ERR_ARG;
    if (g->dlg_open || g->down) return NGR_ERR_STATE;
    m = lead(g);
    dir_step(m->facing, &dx, &dy);
    fx = m->x + (int32_t)dx * (12 << 8); fy = m->y + (int32_t)dy * (12 << 8);
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) {
        const NGRpgObjectDef *d = &g->def->objects[i];
        NGRpgObject *o = &g->obj[i];
        if (d->type != NGR_O_CREATURE || !o->alive || o->state) continue;
        if (manhattan(o->px, o->py, fx, fy) >= (16 << 8)) continue;
        hit = 1;
        if (o->hits) o->hits--;
        emit(g, NGR_EV_HIT, i, o->hits, 0);
        move_thing(g, o, (int32_t)dx * (10 << 8), (int32_t)dy * (10 << 8));
        o->state = 30;    /* stunned for half a second */
        if (!o->hits) {
            o->alive = 0;
            if (d->flag) ng_rpg_set_flag(g, d->flag, 1);
            emit(g, NGR_EV_CALMED, i, 0, 0);
            ng_rpg_run(g, d->actions);
        }
    }
    return hit ? NGR_OK : NGR_ERR_STATE;
}

int NEOGEO_USER ng_rpg_switch(NGRpg *g, uint8_t member)
{
    NGRpgMember *old;
    if (!g || g->nparty < 2u || g->dlg_open || g->down) return NGR_ERR_STATE;
    if (member == NGR_NONE) member = (uint8_t)(g->leader + 1u < g->nparty ? g->leader + 1u : 0u);
    if (member >= g->nparty || member == g->leader || !g->party[member].hp) return NGR_ERR_ARG;
    old = lead(g);
    old->mode = NGR_MODE_FOLLOW;
    old->path_len = old->path_pos = 0; old->stuck = 0;
    g->leader = member;
    lead(g)->mode = NGR_MODE_LEAD;
    lead(g)->path_len = lead(g)->path_pos = 0;
    trail_reset(g);
    g->in_exit = 0xFFu;
    emit(g, NGR_EV_SWITCH, member, 0, 0);
    return NGR_OK;
}

int NEOGEO_USER ng_rpg_order(NGRpg *g, uint8_t member, uint8_t mode, uint8_t tx, uint8_t ty)
{
    NGRpgMember *m;
    if (!g || member >= g->nparty || member == g->leader) return NGR_ERR_ARG;
    if (mode != NGR_MODE_FOLLOW && mode != NGR_MODE_WAIT && mode != NGR_MODE_ASSIST) return NGR_ERR_ARG;
    m = &g->party[member];
    m->mode = mode; m->stuck = 0; m->path_len = m->path_pos = 0;
    if (mode == NGR_MODE_ASSIST) {
        if (!pass(g, m->abilities, 0, tx, ty, NGR_FACE_DOWN)) return NGR_ERR_NOPE;
        m->tx = tx; m->ty = ty;
        if (!find_path(g, member, tx, ty)) return NGR_ERR_NOPE;
    }
    return NGR_OK;
}

void NEOGEO_USER ng_rpg_respawn(NGRpg *g)
{
    uint8_t i;
    if (!g || !g->nparty) return;
    for (i = 0; i < g->nparty; i++) { g->party[i].hp = g->party[i].max_hp; g->party[i].invuln = INVULN; }
    g->down = 0;
    g->dlg_open = 0;
    ng_rpg_enter_room(g, g->ck_room, g->ck_x, g->ck_y, NGR_FACE_DOWN);
}

uint8_t NEOGEO_USER ng_rpg_event(NGRpg *g, NGRpgEvent *out)
{
    NGRpgEvent *e;
    if (!g || !g->ev_count) return 0;
    e = &g->ev[g->ev_head];
    if (out) { out->type = e->type; out->a = e->a; out->b = e->b; out->pad = 0; out->value = e->value; }
    g->ev_head = (uint8_t)((g->ev_head + 1u) & (NGR_EVENT_QUEUE - 1u));
    g->ev_count--;
    return 1;
}

/* ======================================================================
 * hash, save, load
 * ==================================================================== */
static uint32_t NEOGEO_USER fnv(uint32_t h, const uint8_t *d, uint16_t n)
{
    while (n--) {
        h ^= *d++;
        h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
    }
    return h;
}

static uint32_t NEOGEO_USER fnv32(uint32_t h, uint32_t v)
{
    uint8_t b[4];
    b[0] = (uint8_t)(v >> 24); b[1] = (uint8_t)(v >> 16); b[2] = (uint8_t)(v >> 8); b[3] = (uint8_t)v;
    return fnv(h, b, 4);
}

uint32_t NEOGEO_USER ng_rpg_hash(const NGRpg *g)
{
    uint32_t h = 2166136261u;
    uint8_t i;
    if (!g) return 0;
    h = fnv(h, g->flags, sizeof(g->flags));
    h = fnv(h, g->counters, sizeof(g->counters));
    h = fnv(h, g->inv_item, sizeof(g->inv_item));
    h = fnv(h, g->inv_count, sizeof(g->inv_count));
    h = fnv(h, g->collected, sizeof(g->collected));
    h = fnv32(h, ((uint32_t)g->room << 24) | ((uint32_t)g->leader << 16) | g->restored);
    h = fnv32(h, g->signals);
    h = fnv32(h, g->rng);
    for (i = 0; i < g->nparty; i++) {
        const NGRpgMember *m = &g->party[i];
        h = fnv32(h, (uint32_t)m->x); h = fnv32(h, (uint32_t)m->y);
        h = fnv32(h, ((uint32_t)m->hp << 24) | ((uint32_t)m->mode << 16) | ((uint32_t)m->facing << 8));
    }
    for (i = 0; i < NGR_MAX_OBJECTS; i++) {
        const NGRpgObject *o = &g->obj[i];
        h = fnv32(h, ((uint32_t)o->alive << 24) | ((uint32_t)o->state << 16) | ((uint32_t)o->x << 8) | o->y);
        h = fnv32(h, (uint32_t)o->px ^ ((uint32_t)o->py << 7));
    }
    return h;
}

static uint16_t NEOGEO_USER fletcher(const uint8_t *d, uint16_t n)
{
    uint16_t a = 0, b = 0;
    while (n--) {
        a = (uint16_t)(a + *d++); if (a >= 255u) a = (uint16_t)(a - 255u);
        b = (uint16_t)(b + a); if (b >= 255u) b = (uint16_t)(b - 255u);
    }
    return (uint16_t)((b << 8) | a);
}

uint16_t NEOGEO_USER ng_rpg_save(const NGRpg *g, uint8_t *b, uint16_t size)
{
    uint16_t o = 0, i;
    uint16_t sum;
    if (!g || !b || size < NGR_SAVE_SIZE || !g->nparty) return 0;
    b[o++] = (uint8_t)(SAVE_MAGIC >> 24); b[o++] = (uint8_t)(SAVE_MAGIC >> 16);
    b[o++] = (uint8_t)(SAVE_MAGIC >> 8); b[o++] = (uint8_t)SAVE_MAGIC;
    b[o++] = SAVE_VERSION; b[o++] = g->room; b[o++] = g->leader; b[o++] = g->nparty;
    for (i = 0; i < NGR_MAX_PARTY; i++) {
        const NGRpgMember *m = &g->party[i];
        b[o++] = m->id; b[o++] = tile_of(m->x); b[o++] = tile_of(m->y);
        b[o++] = m->hp; b[o++] = m->mode; b[o++] = m->facing; b[o++] = m->tx; b[o++] = m->ty;
    }
    for (i = 0; i < sizeof(g->flags); i++) b[o++] = g->flags[i];
    for (i = 0; i < NGR_COUNTERS; i++) b[o++] = g->counters[i];
    for (i = 0; i < NGR_INV_SLOTS; i++) { b[o++] = g->inv_item[i]; b[o++] = g->inv_count[i]; }
    /* a loose gear in a socket goes into the saved bag: a loaded room starts with its sockets empty */
    if (g->def)
        for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++)
            if (gear_loose(g, (uint8_t)i))
                inv_add(g, b + (o - 2u * NGR_INV_SLOTS), b + (o - 2u * NGR_INV_SLOTS) + 1, 2u, g->def->objects[i].p[5]);
    for (i = 0; i < sizeof(g->collected); i++) b[o++] = g->collected[i];
    for (i = 0; i < sizeof(g->visited); i++) b[o++] = g->visited[i];
    b[o++] = (uint8_t)(g->restored >> 8); b[o++] = (uint8_t)g->restored;
    b[o++] = g->ck_room; b[o++] = g->ck_x; b[o++] = g->ck_y;
    b[o++] = (uint8_t)(g->rng >> 24); b[o++] = (uint8_t)(g->rng >> 16); b[o++] = (uint8_t)(g->rng >> 8); b[o++] = (uint8_t)g->rng;
    sum = fletcher(b, o);
    b[o++] = (uint8_t)(sum >> 8); b[o++] = (uint8_t)sum;
    while (o < NGR_SAVE_SIZE) b[o++] = 0;
    return o;
}

int NEOGEO_USER ng_rpg_load(NGRpg *g, const uint8_t *b, uint16_t size)
{
    uint16_t o = 8, i, end;
    uint8_t room, leader, n;
    if (!g || !b || size < NGR_SAVE_SIZE || !g->world) return NGR_ERR_ARG;
    if (b[0] != (uint8_t)(SAVE_MAGIC >> 24) || b[1] != (uint8_t)(SAVE_MAGIC >> 16) ||
        b[2] != (uint8_t)(SAVE_MAGIC >> 8) || b[3] != (uint8_t)SAVE_MAGIC || b[4] != SAVE_VERSION) return NGR_ERR_SAVE;
    end = (uint16_t)(8u + 8u * NGR_MAX_PARTY + sizeof(g->flags) + NGR_COUNTERS + 2u * NGR_INV_SLOTS +
                     sizeof(g->collected) + sizeof(g->visited) + 2u + 3u + 4u);
    if ((uint16_t)(((uint16_t)b[end] << 8) | b[end + 1u]) != fletcher(b, end)) return NGR_ERR_SAVE;
    room = b[5]; leader = b[6]; n = b[7];
    if (room >= g->world->nrooms || n != g->nparty || leader >= n) return NGR_ERR_SAVE;
    for (i = 0; i < n; i++) if (b[8u + 8u * i] != g->party[i].id) return NGR_ERR_SAVE;
    for (i = 0; i < NGR_MAX_PARTY; i++) {
        NGRpgMember *m = &g->party[i];
        uint8_t tx = b[o + 1u], ty = b[o + 2u];
        if (i < n) {
            m->hp = b[o + 3u]; m->mode = b[o + 4u]; m->facing = b[o + 5u]; m->tx = b[o + 6u]; m->ty = b[o + 7u];
            m->x = tile_centre(tx); m->y = tile_centre(ty);
        }
        o = (uint16_t)(o + 8u);
    }
    for (i = 0; i < sizeof(g->flags); i++) g->flags[i] = b[o++];
    for (i = 0; i < NGR_COUNTERS; i++) g->counters[i] = b[o++];
    for (i = 0; i < NGR_INV_SLOTS; i++) { g->inv_item[i] = b[o++]; g->inv_count[i] = b[o++]; }
    for (i = 0; i < sizeof(g->collected); i++) g->collected[i] = b[o++];
    for (i = 0; i < sizeof(g->visited); i++) g->visited[i] = b[o++];
    g->restored = (uint16_t)(((uint16_t)b[o] << 8) | b[o + 1u]); o = (uint16_t)(o + 2u);
    g->ck_room = b[o++]; g->ck_x = b[o++]; g->ck_y = b[o++];
    g->rng = ((uint32_t)b[o] << 24) | ((uint32_t)b[o + 1u] << 16) | ((uint32_t)b[o + 2u] << 8) | b[o + 3u];
    g->leader = leader;
    g->dlg_open = 0; g->down = 0; g->ev_count = 0; g->ev_head = 0;
    g->def = 0;                    /* the saved bag already holds the old room's loose gears */
    {
        NGRpgMember *l = &g->party[leader];
        uint8_t lx = tile_of(l->x), ly = tile_of(l->y), facing = l->facing;
        ng_rpg_enter_room(g, room, lx, ly, facing);
    }
    return NGR_OK;
}
