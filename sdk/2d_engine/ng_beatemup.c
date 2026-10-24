/*
 * ng_beatemup.c -- the belt-scrolling brawler genre engine (see ng_beatemup.h).
 *
 * Sections: helpers, hits (volumes, damage, knockdowns, throws), the player
 * (states and controls, grabs, weapons, the vehicle), enemies (the crowd
 * director, slots, tokens, kinds, bosses), props and shots, waves and the
 * camera, the frame, and the draw list and hash.
 *
 * 68000 notes: no divide or modulo (percentages by shifts and adds), no
 * struct assignment, no recursion; every loop runs over a fixed array.
 */
#include "ng_beatemup.h"

#define GRAVITY        96          /* Q8 per frame per frame */
#define DOWN_FRAMES    36u
#define GETUP_FRAMES   16u
#define GETUP_INVULN   40u
#define RESPAWN_INVULN 120u
#define TAP_WINDOW     12u
#define GRAB_RANGE     (22 << 8)
#define GRAB_DEPTH     (8 << 8)
#define WALK_GRAB      6u
#define HOLD_MAX       120u
#define ESCAPE_MASH    8u
#define ENEMY_THROW    70u
#define COMBO_WINDOW   50u
#define HITS_WINDOW    90u
#define FORCE_DOWN     3u
#define THROW_DAMAGE   8u
#define THROWN_DAMAGE  6u
#define ATTACK_COOL    24u

static const NGBMove ram_move = { 0, 1, 0, 0, 0, NGB_MV_KNOCKDOWN | NGB_MV_UNBLOCKABLE, 0, 16, 0, 40, 8, 48, 40, 0, 0, NGB_NONE, 0, 0 };
static const NGBMove shove_move = { 0, 1, 0, 0, 8, 0, 0, 16, 0, 40, 8, 24, 0, 0, 0, NGB_NONE, 0, 0 };

/* ======================================================================
 * helpers
 * ==================================================================== */
static void NEOGEO_USER zero(uint8_t *d, uint16_t n)
{
    while (n--) *d++ = 0;
}

static uint16_t NEOGEO_USER rnd(NGBeatEmUp *b)
{
    uint32_t x = b->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    b->rng = x;
    return (uint16_t)(x >> 16);
}

static uint16_t NEOGEO_USER rnd_range(NGBeatEmUp *b, uint16_t n)
{
    return (uint16_t)(((uint32_t)rnd(b) * (uint32_t)n) >> 16);
}

static void NEOGEO_USER emit(NGBeatEmUp *b, uint8_t type, uint8_t a, uint8_t t, uint8_t c, uint16_t value)
{
    NGBEvent *e;
    if (b->ev_count >= NGB_EVENT_QUEUE) { b->ev_dropped++; return; }
    e = &b->ev[(b->ev_head + b->ev_count) & (NGB_EVENT_QUEUE - 1u)];
    e->type = type; e->a = a; e->b = t; e->c = c; e->value = value;
    b->ev_count++;
}

static int32_t NEOGEO_USER iabs(int32_t v) { return v < 0 ? -v : v; }
static int32_t NEOGEO_USER dirsign(uint8_t facing) { return facing ? -1 : 1; }
/* v turned to face the way given: a negate, never a multiply (32-bit
 * multiplies are library calls on the 68000) */
static int32_t NEOGEO_USER flip(uint8_t facing, int32_t v) { return facing ? -v : v; }
static int32_t NEOGEO_USER sgn(int32_t s, int32_t v) { return s < 0 ? -v : v; }

static void NEOGEO_USER add_score(NGBeatEmUp *b, uint32_t pts)
{
    uint32_t s = b->score + pts;
    b->score = s < b->score ? 0xFFFFFFFFu : s;
}

static uint8_t NEOGEO_USER airborne(const NGBFighter *f) { return (uint8_t)(f->z > 0 || f->vz > 0); }

static uint8_t NEOGEO_USER can_be_hit(const NGBFighter *f)
{
    if (!f->alive || f->invuln) return 0;
    if (!f->hp && f->state != NGB_ST_GRABBED) return 0;     /* beaten: let it fall and go out */
    switch (f->state) {
    case NGB_ST_NONE: case NGB_ST_DEAD: case NGB_ST_GETUP: return 0;
    default: return 1;
    }
}

/* a percentage of a number without dividing: hp * pct / 100 to within 1 */
static uint16_t NEOGEO_USER percent(uint8_t hp, uint8_t pct)
{
    uint32_t v = (uint32_t)((uint16_t)hp * (uint16_t)pct);   /* one mulu.w: at most 255 * 100 */
    /* v / 100 ~= v * 41 / 4096 (error < 1 for v < 25600), by shifts */
    return (uint16_t)(((v << 5) + (v << 3) + v + 40u) >> 12);
}

/* ======================================================================
 * hits
 * ==================================================================== */
static void NEOGEO_USER drop_weapon(NGBeatEmUp *b, uint8_t fi)
{
    NGBFighter *f = &b->f[fi];
    NGBProp *p;
    if (f->weapon == NGB_NONE) return;
    p = &b->prop[f->weapon];
    p->state = 0; p->owner = NGB_NONE;
    p->x = f->x; p->y = f->y; p->z = 0; p->vx = 0; p->vz = 0;
    f->weapon = NGB_NONE;
}

static void NEOGEO_USER break_vehicle(NGBeatEmUp *b, uint8_t pi)
{
    NGBProp *p = &b->prop[pi];
    emit(b, NGB_EV_VEHICLE_BREAK, NGB_NONE, pi, 0, 0);
    if (b->stage->nweapons) {
        /* the wreck: the stage's last weapon */
        p->kind = NGB_PROP_WEAPON;
        p->contents = (uint8_t)(b->stage->nweapons - 1u);
        p->hp = b->stage->weapons[p->contents].durability;
        p->state = 0; p->owner = NGB_NONE; p->speed = 0;
    } else {
        p->kind = NGB_PROP_NONE;
    }
}

static void NEOGEO_USER dismount(NGBeatEmUp *b, uint8_t fi, uint8_t damage)
{
    NGBFighter *f = &b->f[fi];
    NGBProp *p;
    if (f->vehicle == NGB_NONE) return;
    p = &b->prop[f->vehicle];
    p->state = 0; p->owner = NGB_NONE; p->speed = 0;
    p->x = f->x; p->y = f->y; p->z = 0;
    emit(b, NGB_EV_DISMOUNT, fi, f->vehicle, 0, 0);
    if (damage) {
        p->hp = (uint8_t)(p->hp > damage ? (uint8_t)(p->hp - damage) : 0u);
        if (!p->hp) break_vehicle(b, f->vehicle);
    }
    f->vehicle = NGB_NONE;
    if (f->state == NGB_ST_RIDE) f->state = NGB_ST_IDLE;
}

static void NEOGEO_USER release(NGBeatEmUp *b, uint8_t fi)
{
    NGBFighter *f = &b->f[fi];
    uint8_t o = f->hold;
    if (o == NGB_NONE) return;
    f->hold = NGB_NONE;
    if (o < NGB_FIGHTERS && b->f[o].hold == fi) {
        b->f[o].hold = NGB_NONE;
        if (b->f[o].state == NGB_ST_GRABBED || b->f[o].state == NGB_ST_GRAB) { b->f[o].state = NGB_ST_IDLE; b->f[o].timer = 0; }
    }
    if (f->state == NGB_ST_GRAB || f->state == NGB_ST_GRABBED) { f->state = NGB_ST_IDLE; f->timer = 0; }
}

static void NEOGEO_USER knock_down(NGBeatEmUp *b, uint8_t ti, int32_t vx, int32_t vz)
{
    NGBFighter *t = &b->f[ti];
    release(b, ti);
    dismount(b, ti, 2);
    drop_weapon(b, ti);
    t->state = NGB_ST_FALL;
    t->timer = 0;
    t->move = NGB_NONE;
    t->vx = vx;
    t->vz = vz < (2 << 8) ? (2 << 8) : vz;
    if (t->juggle < 255u) t->juggle++;
    t->hits_taken = 0;
    emit(b, NGB_EV_KNOCKDOWN, NGB_NONE, ti, 0, 0);
}

static uint8_t NEOGEO_USER flags_of(const NGBFighter *f)
{
    uint8_t fl = f->def ? f->def->flags : 0u;
    if (f->def && f->def->nphases && f->phase < f->def->nphases) fl = (uint8_t)(fl | f->def->phases[f->phase].flags);
    return fl;
}

static void NEOGEO_USER boss_phase(NGBeatEmUp *b, uint8_t ti)
{
    NGBFighter *t = &b->f[ti];
    const NGBFighterDef *d = t->def;
    while (d && t->phase + 1u < d->nphases) {
        const NGBBossPhase *ph = &d->phases[t->phase + 1u];
        if (t->hp >= percent(d->max_hp, ph->below_pct)) break;
        t->phase++;
        t->invuln = ph->invuln;
        emit(b, NGB_EV_BOSS_PHASE, ti, 0, t->phase, ph->event);
    }
}

/* Returns 1 if the hit landed (not blocked or missed). */
static uint8_t NEOGEO_USER apply_hit(NGBeatEmUp *b, uint8_t ai, uint8_t ti, const NGBMove *mv, uint8_t damage)
{
    NGBFighter *a = &b->f[ai], *t = &b->f[ti];
    uint8_t fl = flags_of(t), stop;
    int32_t away = a->x <= t->x ? 1 : -1;
    uint8_t knock = (uint8_t)(mv->flags & (NGB_MV_KNOCKDOWN | NGB_MV_LAUNCH));
    if (!can_be_hit(t)) return 0;
    if (t->state == NGB_ST_DOWN && !(mv->flags & NGB_MV_LOW)) return 0;
    /* shields hold from the front while not attacking */
    if ((fl & NGB_FD_SHIELD) && t->state != NGB_ST_ATTACK && !(mv->flags & NGB_MV_UNBLOCKABLE) && !knock
        && dirsign(t->facing) == -away) {
        t->state = NGB_ST_BLOCK; t->timer = 0; t->hitstun = 12;
        t->vx = sgn(away, 1 << 8);
        a->hitstop = 3; t->hitstop = 3;
        emit(b, NGB_EV_BLOCKED, ai, ti, 0, 0);
        return 0;
    }
    t->hp = (uint8_t)(t->hp > damage ? (uint8_t)(t->hp - damage) : 0u);
    /* a short hitstop: a jab 3 frames, the heaviest blow 6 (longer reads as the game freezing) */
    stop = (uint8_t)(2u + (damage >> 2));
    if (stop > 6u) stop = 6u;
    a->hitstop = stop; t->hitstop = stop;
    emit(b, NGB_EV_HIT, ai, ti, damage, 0);
    if (ai != ti && (ai == NGB_PLAYER || a->state == NGB_ST_THROWN)) {
        b->combo = (uint16_t)(b->combo_timer ? b->combo + 1u : 1u);
        b->combo_timer = COMBO_WINDOW;
        if (b->combo >= 2u) emit(b, NGB_EV_COMBO, 0, 0, 0, b->combo);
        add_score(b, (uint32_t)damage * 10u);
    }
    if (ti == NGB_PLAYER) {
        t->hits_taken++;
        if (t->hits_taken >= FORCE_DOWN) knock = 1;     /* no endless chains on the player */
    } else {
        t->token = 0;                                   /* a hit attacker gives up its turn */
        if (t->ai != NGB_AI_ENTER) t->ai = NGB_AI_WAIT;
    }
    if (t->def && t->def->nphases) boss_phase(b, ti);
    if (airborne(t) && t->state == NGB_ST_FALL) knock = 1;   /* juggles stay knockdowns */
    if (!t->hp) knock = 1;
    if ((fl & NGB_FD_ARMOR) && !knock && t->hp) return 1;    /* armour: no stagger */
    if (!knock && t->state == NGB_ST_GRABBED && t->hold == ai) return 1;   /* a pummel: still held */
    if (knock) {
        int32_t vz = (int32_t)mv->push_z << 4;
        if (t->juggle >= 3u) vz = 1 << 8;                    /* juggles lose height */
        knock_down(b, ti, sgn(away, (int32_t)(mv->push_x ? mv->push_x : 32) << 4), vz);
    } else {
        release(b, ti);
        if (t->state == NGB_ST_RIDE) dismount(b, ti, 1);
        t->state = NGB_ST_HITSTUN;
        t->timer = 0;
        t->hitstun = mv->hitstun ? mv->hitstun : 12u;
        t->move = NGB_NONE;
        t->vx = sgn(away, (int32_t)mv->push_x << 3);
    }
    return 1;
}

/* Does a's active move volume touch t? */
static uint8_t NEOGEO_USER touches(const NGBFighter *a, const NGBFighter *t, const NGBMove *mv)
{
    int32_t x0 = a->x + flip(a->facing, (int32_t)mv->box_x << 8), x1 = x0 + flip(a->facing, (int32_t)mv->box_w << 8);
    int32_t lo = x0 < x1 ? x0 : x1, hi = x0 < x1 ? x1 : x0;
    int32_t hw = (int32_t)(t->def ? t->def->half_w : 12u) << 8;
    int32_t z0 = a->z + ((int32_t)mv->box_z << 8), z1 = z0 + ((int32_t)mv->box_h << 8);
    int32_t th = (int32_t)(t->def ? t->def->height : 60u) << 8;
    int32_t dz = (int32_t)(mv->box_depth ? mv->box_depth : 8u) << 8;
    if (t->state == NGB_ST_DOWN) th = 16 << 8;               /* lying flat */
    if (iabs(a->y - t->y) > dz + ((int32_t)(t->def ? t->def->half_depth : 4u) << 8)) return 0;
    if (t->x + hw < lo || t->x - hw > hi) return 0;
    if (t->z + th < z0 || t->z > z1) return 0;
    return 1;
}

static const NGBMove *NEOGEO_USER move_of(const NGBeatEmUp *b, const NGBFighter *f)
{
    if (f->move == NGB_NONE) return 0;
    if (f->move == 0xFEu) {                     /* a weapon swing */
        if (f->weapon == NGB_NONE) return 0;
        return b->stage->weapons[b->prop[f->weapon].contents].move;
    }
    if (!f->def || f->move >= f->def->nmoves) return 0;
    return &f->def->moves[f->move];
}

static void NEOGEO_USER start_move(NGBeatEmUp *b, uint8_t fi, uint8_t mv)
{
    NGBFighter *f = &b->f[fi];
    const NGBMove *m;
    f->move = mv;
    m = move_of(b, f);
    if (!m) { f->move = NGB_NONE; return; }
    f->state = NGB_ST_ATTACK;
    f->move_frame = 0;
    f->chain_buf = 0;
    f->serial++;
    f->timer = 0;
    if (!airborne(f)) { f->vx = 0; f->vy = 0; }
}

static void NEOGEO_USER spawn_shot(NGBeatEmUp *b, uint8_t fi, const NGBMove *m)
{
    uint8_t i;
    NGBFighter *f = &b->f[fi];
    for (i = 0; i < NGB_MAX_SHOTS; i++) {
        NGBShot *s = &b->shot[i];
        if (s->alive) continue;
        s->alive = 1; s->owner = fi; s->damage = m->damage; s->timer = 120;
        s->x = f->x + flip(f->facing, (int32_t)m->box_x << 8);
        s->y = f->y; s->z = f->z + ((int32_t)m->box_z << 8);
        s->vx = flip(f->facing, (int32_t)(m->lunge ? m->lunge : 48) << 4);
        emit(b, NGB_EV_SHOT, fi, i, 0, 0);
        return;
    }
}

static uint8_t NEOGEO_USER enemy_of(uint8_t a, uint8_t t)
{
    return (uint8_t)((a == NGB_PLAYER) != (t == NGB_PLAYER));
}

static void NEOGEO_USER grab(NGBeatEmUp *b, uint8_t hi, uint8_t ti)
{
    NGBFighter *h = &b->f[hi], *t = &b->f[ti];
    release(b, ti);
    dismount(b, ti, 0);
    h->state = NGB_ST_GRAB; h->hold = ti; h->hold_frames = 0; h->pummels = 0; h->timer = 0; h->vx = h->vy = 0;
    t->state = NGB_ST_GRABBED; t->hold = hi; t->hold_frames = 0; t->pummels = 0; t->timer = 0; t->vx = t->vy = 0;
    t->move = NGB_NONE;
    t->x = h->x + flip(h->facing, 18 << 8);
    t->y = h->y; t->z = 0;
    t->facing = (uint8_t)!h->facing;
    emit(b, NGB_EV_GRAB, hi, ti, 0, 0);
}

static void NEOGEO_USER throw_held(NGBeatEmUp *b, uint8_t hi, uint8_t backwards)
{
    NGBFighter *h = &b->f[hi];
    uint8_t ti = h->hold;
    NGBFighter *t;
    int32_t dir;
    uint8_t w;
    if (ti == NGB_NONE) return;
    t = &b->f[ti];
    h->hold = NGB_NONE; t->hold = NGB_NONE;
    dir = ((h->facing != 0) != (backwards != 0)) ? -1 : 1;
    if (backwards) h->facing = (uint8_t)!h->facing;
    w = (uint8_t)(t->def && t->def->weight ? t->def->weight : 1u);
    t->state = NGB_ST_THROWN; t->timer = 0;
    t->vx = sgn(dir, (int32_t)(w > 2u ? 3 : 4) << 8);
    t->vz = 3 << 8;
    t->x = h->x + sgn(dir, 12 << 8);
    t->serial++;
    h->state = NGB_ST_ATTACK;
    h->move = NGB_NONE; h->move_frame = 0; h->timer = 0;
    h->hitstun = 18;                         /* recovery after a throw */
    emit(b, NGB_EV_THROW, hi, ti, backwards, 0);
}

/* ======================================================================
 * the player
 * ==================================================================== */
static uint8_t NEOGEO_USER near_prop(NGBeatEmUp *b, uint8_t fi, uint8_t want_vehicle)
{
    NGBFighter *f = &b->f[fi];
    uint8_t i, best = NGB_NONE;
    int32_t bd = 0x7FFFFFFF;
    for (i = 0; i < NGB_MAX_PROPS; i++) {
        NGBProp *p = &b->prop[i];
        int32_t d;
        if (p->state != 0) continue;
        if (want_vehicle ? p->kind != NGB_PROP_VEHICLE
                         : !(p->kind == NGB_PROP_WEAPON || p->kind == NGB_PROP_FOOD || p->kind == NGB_PROP_SCORE)) continue;
        if (iabs(p->y - f->y) > (10 << 8)) continue;
        d = iabs(p->x - f->x);
        if (d > (22 << 8)) continue;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

static uint8_t NEOGEO_USER grab_target(NGBeatEmUp *b, uint8_t fi)
{
    NGBFighter *f = &b->f[fi];
    uint8_t i;
    for (i = 1; i < NGB_FIGHTERS; i++) {
        NGBFighter *t = &b->f[i];
        int32_t dx;
        if (!t->alive || !can_be_hit(t) || airborne(t)) continue;
        if (t->state == NGB_ST_DOWN || t->state == NGB_ST_FALL || t->state == NGB_ST_THROWN) continue;
        if (flags_of(t) & (NGB_FD_HEAVY | NGB_FD_FLYER)) continue;
        if (iabs(t->y - f->y) > GRAB_DEPTH) continue;
        dx = flip(f->facing, t->x - f->x);
        if (dx < 0 || dx > GRAB_RANGE) continue;
        return i;
    }
    return NGB_NONE;
}

static void NEOGEO_USER pick_up(NGBeatEmUp *b, uint8_t pi)
{
    NGBFighter *f = &b->f[NGB_PLAYER];
    NGBProp *p = &b->prop[pi];
    if (p->kind == NGB_PROP_FOOD) {
        uint16_t hp = (uint16_t)(f->hp + (p->contents ? p->contents : 8u));
        f->hp = (uint8_t)(hp > f->def->max_hp ? f->def->max_hp : hp);
        emit(b, NGB_EV_FOOD, NGB_PLAYER, pi, p->contents, 0);
        p->kind = NGB_PROP_NONE;
        return;
    }
    if (p->kind == NGB_PROP_SCORE) {
        add_score(b, (uint32_t)(p->contents ? p->contents : 1u) * 100u);
        emit(b, NGB_EV_SCORE_ITEM, NGB_PLAYER, pi, 0, (uint16_t)((p->contents ? p->contents : 1u) * 100u));
        p->kind = NGB_PROP_NONE;
        return;
    }
    drop_weapon(b, NGB_PLAYER);
    p->state = 1; p->owner = NGB_PLAYER;
    f->weapon = pi;
    emit(b, NGB_EV_PICKUP, NGB_PLAYER, pi, 0, 0);
}

static void NEOGEO_USER mount(NGBeatEmUp *b, uint8_t v)
{
    NGBFighter *f = &b->f[NGB_PLAYER];
    NGBProp *p = &b->prop[v];
    f->state = NGB_ST_RIDE; f->vehicle = v; p->state = 3; p->owner = NGB_PLAYER; p->speed = 0;
    f->x = p->x; f->y = p->y;
    emit(b, NGB_EV_MOUNT, NGB_PLAYER, v, 0, 0);
}

/* NGB_SF_WALK_ON: off the vehicle with a jump that carries the player clear
 * of it (a running jump's reach, the way held or faced), so that walking on
 * doesn't put them straight back on. */
static void NEOGEO_USER jump_off(NGBeatEmUp *b)
{
    NGBFighter *f = &b->f[NGB_PLAYER];
    const NGBFighterDef *d = f->def;
    if (b->in_dirs & NGB_IN_LEFT) f->facing = 1;
    else if (b->in_dirs & NGB_IN_RIGHT) f->facing = 0;
    dismount(b, NGB_PLAYER, 0);
    f->state = NGB_ST_JUMP; f->air_done = 0; f->timer = 0;
    f->vz = (int32_t)(d->jump ? d->jump : 64u) << 4;
    f->vx = flip(f->facing, (int32_t)d->speed << 5);
    f->vy = 0;
    emit(b, NGB_EV_JUMP, NGB_PLAYER, 0, 0, 0);
}

/* NGB_SF_AUTO_PICKUP: what lies under the player's feet is taken without a
 * button. Food and points on any grounded step (a chain of punches included);
 * a weapon only when walking or standing with empty hands, so one that was
 * just dropped or thrown isn't swapped for another by accident. */
#define WALK_PICK_X  (18 << 8)
#define WALK_PICK_Y  (10 << 8)
static void NEOGEO_USER walk_over(NGBeatEmUp *b)
{
    NGBFighter *f = &b->f[NGB_PLAYER];
    uint8_t i, hands;
    if (!f->alive || f->z > 0) return;
    /* NGB_SF_WALK_ON: walking (or running) onto a vehicle that stands still
     * rides it. Only a step towards it counts, so stepping off one that was
     * just left (or landed on) doesn't mount it again. */
    if ((b->stage->flags & NGB_SF_WALK_ON) && (f->state == NGB_ST_WALK || f->state == NGB_ST_RUN) && f->vehicle == NGB_NONE)
        for (i = 0; i < NGB_MAX_PROPS; i++) {
            NGBProp *p = &b->prop[i];
            int32_t dx = p->x - f->x, dy = p->y - f->y;
            if (p->kind != NGB_PROP_VEHICLE || p->state != 0) continue;
            if (iabs(dx) > WALK_PICK_X || iabs(dy) > WALK_PICK_Y) continue;
            if (!((f->vx > 0 && dx > 0) || (f->vx < 0 && dx < 0) || (f->vy > 0 && dy > 0) || (f->vy < 0 && dy < 0))) continue;
            drop_weapon(b, NGB_PLAYER);
            mount(b, i);
            return;
        }
    if (!(b->stage->flags & NGB_SF_AUTO_PICKUP)) return;
    if (f->state != NGB_ST_IDLE && f->state != NGB_ST_WALK && f->state != NGB_ST_RUN && f->state != NGB_ST_ATTACK) return;
    hands = (uint8_t)(f->state != NGB_ST_ATTACK && f->weapon == NGB_NONE && f->vehicle == NGB_NONE);
    for (i = 0; i < NGB_MAX_PROPS; i++) {
        NGBProp *p = &b->prop[i];
        if (p->state != 0) continue;
        if (!(p->kind == NGB_PROP_FOOD || p->kind == NGB_PROP_SCORE || (hands && p->kind == NGB_PROP_WEAPON))) continue;
        if (iabs(p->x - f->x) > WALK_PICK_X || iabs(p->y - f->y) > WALK_PICK_Y) continue;
        pick_up(b, i);
        if (f->weapon != NGB_NONE) hands = 0;
    }
}

static void NEOGEO_USER throw_weapon(NGBeatEmUp *b)
{
    NGBFighter *f = &b->f[NGB_PLAYER];
    NGBProp *p;
    if (f->weapon == NGB_NONE) return;
    p = &b->prop[f->weapon];
    p->state = 2; p->owner = NGB_PLAYER; p->timer = 60;
    p->x = f->x; p->y = f->y; p->z = 24 << 8;
    p->vx = flip(f->facing, 6 << 8); p->vz = 1 << 8;
    f->weapon = NGB_NONE;
    emit(b, NGB_EV_THROW, NGB_PLAYER, NGB_NONE, 0, 0);
}

static void NEOGEO_USER player_walk(NGBeatEmUp *b, NGBFighter *f, uint8_t run)
{
    int32_t sp = (int32_t)f->def->speed << 4, dsp = (int32_t)f->def->depth_speed << 4;
    uint8_t d = b->in_dirs;
    f->vx = 0; f->vy = 0;
    if (run) {
        f->vx = flip(f->facing, sp << 1);
        if (d & NGB_IN_UP) f->vy = -dsp; else if (d & NGB_IN_DOWN) f->vy = dsp;
        return;
    }
    if (d & NGB_IN_LEFT) { f->vx = -sp; f->facing = 1; }
    else if (d & NGB_IN_RIGHT) { f->vx = sp; f->facing = 0; }
    if (d & NGB_IN_UP) f->vy = -dsp; else if (d & NGB_IN_DOWN) f->vy = dsp;
    f->state = (f->vx || f->vy) ? NGB_ST_WALK : NGB_ST_IDLE;
}

static void NEOGEO_USER player_ground(NGBeatEmUp *b)
{
    NGBFighter *f = &b->f[NGB_PLAYER];
    const NGBFighterDef *d = f->def;
    uint8_t pr = b->in_pressed, dirs = b->in_dirs;
    uint8_t lr = (uint8_t)(pr & (NGB_IN_LEFT | NGB_IN_RIGHT));
    /* double tap: run */
    if (lr) {
        if (b->tap_timer && b->tap_dir == lr && f->state != NGB_ST_RUN) {
            f->state = NGB_ST_RUN; f->facing = (uint8_t)(lr == NGB_IN_LEFT);
            emit(b, NGB_EV_DASH, NGB_PLAYER, 0, 0, 0);
            b->tap_timer = 0;
        } else {
            b->tap_dir = lr; b->tap_timer = TAP_WINDOW;
        }
    }
    if (f->state == NGB_ST_RUN && !(dirs & (f->facing ? NGB_IN_LEFT : NGB_IN_RIGHT))) f->state = NGB_ST_IDLE;
    if (pr & NGB_BTN_D && !b->special_cool && d->special != NGB_NONE && f->hp > 1u) {
        uint8_t cost = (uint8_t)(f->hp > 2u ? 2u : 1u);
        f->hp = (uint8_t)(f->hp - cost);
        b->special_cool = 30;
        start_move(b, NGB_PLAYER, d->special);
        f->invuln = (uint8_t)(d->moves[d->special].startup + d->moves[d->special].active);
        emit(b, NGB_EV_SPECIAL, NGB_PLAYER, 0, 0, 0);
        return;
    }
    if (pr & NGB_BTN_B) {
        uint8_t was_run = (uint8_t)(f->state == NGB_ST_RUN);
        f->state = NGB_ST_JUMP; f->vz = (int32_t)(d->jump ? d->jump : 64u) << 4; f->air_done = 0;
        if (was_run) f->vx = flip(f->facing, (int32_t)d->speed << 5);   /* a running jump goes further */
        emit(b, NGB_EV_JUMP, NGB_PLAYER, 0, 0, 0);
        return;
    }
    if (pr & NGB_BTN_C) {
        uint8_t v = near_prop(b, NGB_PLAYER, 1), p, t;
        if (v != NGB_NONE && f->weapon == NGB_NONE) { mount(b, v); return; }
        p = near_prop(b, NGB_PLAYER, 0);
        if (p != NGB_NONE) { pick_up(b, p); return; }
        if (f->weapon != NGB_NONE) { throw_weapon(b); return; }
        t = grab_target(b, NGB_PLAYER);
        if (t != NGB_NONE) { grab(b, NGB_PLAYER, t); return; }
    }
    if (pr & NGB_BTN_A) {
        if (f->state == NGB_ST_RUN && d->dash != NGB_NONE) { start_move(b, NGB_PLAYER, d->dash); f->vx = flip(f->facing, (int32_t)d->speed << 5); return; }
        if (f->weapon != NGB_NONE && b->stage->weapons[b->prop[f->weapon].contents].move) { start_move(b, NGB_PLAYER, 0xFEu); return; }
        if ((dirs & NGB_IN_DOWN) && d->low != NGB_NONE) { start_move(b, NGB_PLAYER, d->low); return; }
        if (d->chain != NGB_NONE) { start_move(b, NGB_PLAYER, d->chain); return; }
    }
    player_walk(b, f, (uint8_t)(f->state == NGB_ST_RUN));
    /* walking into someone takes hold of them */
    if (f->state == NGB_ST_WALK && (dirs & (f->facing ? NGB_IN_LEFT : NGB_IN_RIGHT))) {
        uint8_t t = grab_target(b, NGB_PLAYER);
        if (t != NGB_NONE && iabs(b->f[t].x - f->x) < (16 << 8)) {
            if (++f->hold_frames >= WALK_GRAB) { f->hold_frames = 0; grab(b, NGB_PLAYER, t); }
        } else {
            f->hold_frames = 0;
        }
    }
}

static void NEOGEO_USER player_ride(NGBeatEmUp *b)
{
    NGBFighter *f = &b->f[NGB_PLAYER];
    NGBProp *p = &b->prop[f->vehicle];
    const NGBStage *s = b->stage;
    int16_t max = (int16_t)(s->vehicle_max ? s->vehicle_max : 96u), acc = (int16_t)(s->vehicle_accel ? s->vehicle_accel : 3u);
    uint8_t d = b->in_dirs, i;
    if ((s->flags & NGB_SF_WALK_ON) && (b->in_pressed & (NGB_BTN_B | NGB_BTN_C)) && !airborne(f)) { jump_off(b); return; }
    if (b->in_pressed & NGB_BTN_C) { dismount(b, NGB_PLAYER, 0); return; }
    if (d & NGB_IN_RIGHT) { p->speed = (int16_t)(p->speed + acc > max ? max : p->speed + acc); f->facing = 0; }
    else if (d & NGB_IN_LEFT) { p->speed = (int16_t)(p->speed - acc < -max ? -max : p->speed - acc); f->facing = 1; }
    else if (p->speed > 0) p->speed = (int16_t)(p->speed > (acc >> 1) ? p->speed - (acc >> 1) : 0);
    else if (p->speed < 0) p->speed = (int16_t)(p->speed < -(acc >> 1) ? p->speed + (acc >> 1) : 0);
    f->vx = (int32_t)p->speed << 4;
    f->vy = 0;
    if (d & NGB_IN_UP) f->vy = -((int32_t)f->def->depth_speed << 4);
    else if (d & NGB_IN_DOWN) f->vy = (int32_t)f->def->depth_speed << 4;
    if ((b->in_pressed & NGB_BTN_B) && !airborne(f)) { f->vz = 5 << 7; emit(b, NGB_EV_JUMP, NGB_PLAYER, 1, 0, 0); }
    /* ramps */
    for (i = 0; i < NGB_MAX_PROPS; i++) {
        NGBProp *r = &b->prop[i];
        if (r->kind != NGB_PROP_RAMP || airborne(f)) continue;
        if (iabs(r->x - f->x) < (10 << 8) && iabs(r->y - f->y) < (10 << 8) && (p->speed >= (max >> 1) || p->speed <= -(max >> 1))) {
            f->vz = 3 << 8;
            emit(b, NGB_EV_JUMP, NGB_PLAYER, 2, 0, 0);
        }
    }
    /* ramming at speed */
    if (p->speed >= (max >> 1) || p->speed <= -(max >> 1)) {
        for (i = 1; i < NGB_FIGHTERS; i++) {
            NGBFighter *t = &b->f[i];
            NGBMove ram;
            if (!t->alive || !can_be_hit(t) || t->state == NGB_ST_DOWN || t->state == NGB_ST_FALL) continue;
            if (iabs(t->x - f->x) > (16 << 8) || iabs(t->y - f->y) > (12 << 8) || t->z > (24 << 8)) continue;
            ram.startup = ram_move.startup; ram.active = ram_move.active; ram.recovery = ram_move.recovery;
            ram.damage = ram_move.damage; ram.hitstun = ram_move.hitstun; ram.flags = ram_move.flags;
            ram.box_x = ram_move.box_x; ram.box_w = ram_move.box_w; ram.box_z = ram_move.box_z;
            ram.box_h = ram_move.box_h; ram.box_depth = ram_move.box_depth;
            ram.push_x = ram_move.push_x; ram.push_z = ram_move.push_z; ram.lunge = 0;
            ram.cancel = 0; ram.next = NGB_NONE; ram.sound = 0; ram.anim = 0;
            if (apply_hit(b, NGB_PLAYER, i, &ram, s->vehicle_ram ? s->vehicle_ram : 6u)) {
                emit(b, NGB_EV_RAM, NGB_PLAYER, i, 0, 0);
                if (p->hp) p->hp--;
                if (!p->hp) { uint8_t v = f->vehicle; f->vehicle = NGB_NONE; f->state = NGB_ST_IDLE; break_vehicle(b, v); return; }
            }
        }
    }
}

static void NEOGEO_USER player_control(NGBeatEmUp *b)
{
    NGBFighter *f = &b->f[NGB_PLAYER];
    const NGBFighterDef *d = f->def;
    if (b->tap_timer) b->tap_timer--;
    switch (f->state) {
    case NGB_ST_IDLE: case NGB_ST_WALK: case NGB_ST_RUN:
        player_ground(b);
        break;
    case NGB_ST_JUMP:
        if ((b->in_pressed & NGB_BTN_A) && !f->air_done && d->air != NGB_NONE) {
            int32_t vx = f->vx;
            f->air_done = 1;
            start_move(b, NGB_PLAYER, d->air);
            f->vx = vx;
        } else {
            int32_t sp = (int32_t)d->speed << 4;
            if (b->in_dirs & NGB_IN_LEFT) { f->vx = -sp; f->facing = 1; }
            else if (b->in_dirs & NGB_IN_RIGHT) { f->vx = sp; f->facing = 0; }
        }
        break;
    case NGB_ST_ATTACK:
        if (b->in_pressed & NGB_BTN_A) f->chain_buf = 1;
        break;
    case NGB_ST_GRAB: {
        uint8_t fwd = (uint8_t)(b->in_dirs & (f->facing ? NGB_IN_LEFT : NGB_IN_RIGHT));
        uint8_t back = (uint8_t)(b->in_dirs & (f->facing ? NGB_IN_RIGHT : NGB_IN_LEFT));
        if ((b->in_pressed & NGB_BTN_C) || ((b->in_pressed & NGB_BTN_A) && fwd)) { throw_held(b, NGB_PLAYER, (uint8_t)(back != 0)); break; }
        if ((b->in_pressed & NGB_BTN_A) && d->pummel != NGB_NONE) {
            uint8_t t = f->hold;
            if (t != NGB_NONE) {
                apply_hit(b, NGB_PLAYER, t, &d->moves[d->pummel], d->moves[d->pummel].damage);
                if (++f->pummels >= 3u && f->hold != NGB_NONE) throw_held(b, NGB_PLAYER, 0);
            }
        }
        break;
    }
    case NGB_ST_GRABBED:
        if (b->in_pressed) {
            f->pummels++;
            if (f->pummels >= ESCAPE_MASH) {
                uint8_t h = f->hold;
                release(b, NGB_PLAYER);
                if (h != NGB_NONE) { b->f[h].vx = flip(b->f[h].facing, -(2 << 8)); emit(b, NGB_EV_ESCAPE, h, NGB_PLAYER, 0, 0); }
            }
        }
        break;
    case NGB_ST_RIDE:
        player_ride(b);
        break;
    default:
        break;
    }
}

/* ======================================================================
 * enemies
 * ==================================================================== */
static uint8_t NEOGEO_USER enemy_ready(const NGBFighter *e)
{
    return (uint8_t)(e->alive && (e->state == NGB_ST_IDLE || e->state == NGB_ST_WALK));
}

/* The crowd: tokens for the nearest, spaced slots for the rest. */
static void NEOGEO_USER director(NGBeatEmUp *b)
{
    NGBFighter *p = &b->f[NGB_PLAYER];
    uint8_t i, holders = 0, max = b->stage->max_attackers ? b->stage->max_attackers : 2u;
    uint8_t left = 0, right = 0;
    for (i = 1; i < NGB_FIGHTERS; i++) if (b->f[i].alive && b->f[i].token) holders++;
    while (holders < max && !b->attack_cool) {
        uint8_t best = NGB_NONE;
        int32_t bd = 0x7FFFFFFF;
        for (i = 1; i < NGB_FIGHTERS; i++) {
            NGBFighter *e = &b->f[i];
            int32_t d;
            if (!enemy_ready(e) || e->token || e->ai == NGB_AI_RETREAT || e->ai == NGB_AI_ENTER) continue;
            d = iabs(e->x - p->x) + iabs(e->y - p->y);
            if (d < bd) { bd = d; best = i; }
        }
        if (best == NGB_NONE) break;
        b->f[best].token = 1;
        b->f[best].ai = NGB_AI_APPROACH;
        holders++;
    }
    /* slots: rank waiters on each side by index (stable), spread them out */
    for (i = 1; i < NGB_FIGHTERS; i++) {
        NGBFighter *e = &b->f[i];
        if (!e->alive || e->token || e->state == NGB_ST_DEAD) continue;
        if (e->x < p->x) e->slot = (uint8_t)(left++); else e->slot = (uint8_t)(0x80u | right++);
    }
    /* keep the sides balanced: move a waiter from a crowded side */
    if (left > right + 2u || right > left + 2u) {
        uint8_t from_left = (uint8_t)(left > right);
        for (i = NGB_FIGHTERS; i-- > 1;) {
            NGBFighter *e = &b->f[i];
            if (!e->alive || e->token || e->ai == NGB_AI_ENTER) continue;
            if (from_left != !(e->slot & 0x80u)) continue;
            e->slot = (uint8_t)(from_left ? (0x80u | right) : left);
            break;
        }
    }
}

static void NEOGEO_USER walk_to(NGBFighter *e, int32_t tx, int32_t ty, int32_t sp, int32_t dsp)
{
    int32_t dx = tx - e->x, dy = ty - e->y;
    e->vx = dx > sp ? sp : dx < -sp ? -sp : dx;
    e->vy = dy > dsp ? dsp : dy < -dsp ? -dsp : dy;
    e->state = (e->vx || e->vy) ? NGB_ST_WALK : NGB_ST_IDLE;
}

static void NEOGEO_USER pick_attack(NGBeatEmUp *b, uint8_t ei)
{
    NGBFighter *e = &b->f[ei];
    const NGBFighterDef *d = e->def;
    const uint8_t *list = d->attacks;
    uint8_t n = d->nattacks, mv;
    if (d->nphases && e->phase < d->nphases && d->phases[e->phase].nattacks) {
        list = d->phases[e->phase].attacks; n = d->phases[e->phase].nattacks;
    }
    if (!n) return;
    mv = list[rnd_range(b, n)];
    if (mv >= d->nmoves) return;
    start_move(b, ei, mv);
    emit(b, NGB_EV_TELEGRAPH, ei, 0, mv, 0);
    b->attack_cool = ATTACK_COOL;
}

static void NEOGEO_USER enemy_ai(NGBeatEmUp *b, uint8_t ei)
{
    NGBFighter *e = &b->f[ei], *p = &b->f[NGB_PLAYER];
    const NGBFighterDef *d = e->def;
    int32_t sp = (int32_t)(d->speed + (d->nphases && e->phase < d->nphases ? d->phases[e->phase].speed_add : 0u)) << 4;
    int32_t dsp = (int32_t)(d->depth_speed ? d->depth_speed : d->speed) << 4;
    int32_t side = e->x < p->x ? -1 : 1;
    int32_t reach = (int32_t)(d->reach ? d->reach : 30u) << 8;
    if (e->state == NGB_ST_GRAB) {          /* a grappler holding the player */
        if (++e->hold_frames >= ENEMY_THROW) {
            throw_held(b, ei, 0);                     /* the landing does the damage */
        }
        return;
    }
    if (!enemy_ready(e)) return;
    e->facing = (uint8_t)(p->x < e->x);
    if (p->state == NGB_ST_DEAD || p->state == NGB_ST_NONE) { e->vx = e->vy = 0; e->state = NGB_ST_IDLE; return; }
    switch (e->ai) {
    case NGB_AI_ENTER: {
        int32_t cx = b->camera_x + ((int32_t)b->stage->view_w << 7);
        int32_t edge = ((int32_t)b->stage->view_w << 7) - (24 << 8);
        if (iabs(e->x - cx) < edge) { e->ai = NGB_AI_WAIT; break; }
        walk_to(e, cx, e->y, sp, dsp);
        break;
    }
    case NGB_AI_WAIT: {
        uint8_t rank = (uint8_t)(e->slot & 0x7Fu);
        int32_t s = (e->slot & 0x80u) ? 1 : -1;
        int32_t off = (int32_t)(10u + 8u * (rank >> 1)) << 8;
        int32_t tx = p->x + sgn(s, (int32_t)(64u + 28u * rank) << 8);
        int32_t ty = p->y + ((rank & 1u) ? off : -off);
        int32_t y0 = (int32_t)b->stage->y_min << 8, y1 = (int32_t)b->stage->y_max << 8;
        {
            /* a spot off the screen (the hero near its edge): wait on the hero's other side, so the
             * crowd doesn't pile up against the edge it is kept inside */
            int32_t lo = b->camera_x + (24 << 8);
            int32_t hi = b->camera_x + ((int32_t)(b->stage->view_w ? b->stage->view_w : 320u) << 8) - (24 << 8);
            if (tx < lo || tx > hi) {
                int32_t other = p->x + p->x - tx;
                if (other >= lo && other <= hi) tx = other;
            }
        }
        if (ty < y0) ty = y0;
        if (ty > y1) ty = y1;
        if (iabs(tx - e->x) < (4 << 8) && iabs(ty - e->y) < (3 << 8)) { e->vx = e->vy = 0; e->state = NGB_ST_IDLE; }
        else walk_to(e, tx, ty, sp - (sp >> 2), dsp);
        break;
    }
    case NGB_AI_APPROACH: {
        int32_t tx = p->x + sgn(side, reach), ty = p->y;
        if (d->ai == NGB_AI_RANGED) tx = p->x + sgn(side, 110 << 8);
        if (d->ai == NGB_AI_CHARGER) tx = p->x + sgn(side, 90 << 8);
        if (d->ai == NGB_AI_JUMPER) tx = p->x + sgn(side, 56 << 8);
        {
            /* a spot outside the view (past the stage's end, say) is never
             * reached: line up on the player's other side instead */
            int32_t lo = b->camera_x + (8 << 8);
            int32_t hi = b->camera_x + ((int32_t)(b->stage->view_w ? b->stage->view_w : 320u) << 8) - (8 << 8);
            if (tx < lo || tx > hi) {
                int32_t other = p->x + p->x - tx;
                tx = (other >= lo && other <= hi) ? other : tx < lo ? lo : hi;
            }
        }
        if (iabs(tx - e->x) < (5 << 8) && iabs(ty - e->y) < (4 << 8) && !b->attack_cool) {
            e->vx = e->vy = 0;
            if (d->ai == NGB_AI_JUMPER) { e->vz = (int32_t)(d->jump ? d->jump : 56u) << 4; e->vx = sgn(-side, sp << 1); }
            pick_attack(b, ei);
            if (e->state == NGB_ST_ATTACK) e->ai = NGB_AI_ENGAGE;
        } else {
            walk_to(e, tx, ty, sp, dsp);
        }
        break;
    }
    case NGB_AI_ENGAGE:     /* the attack ended */
        e->ai = NGB_AI_RETREAT;
        e->token = 0;
        e->ai_timer = (uint8_t)(d->ai == NGB_AI_BOSS ? 20u : 36u + rnd_range(b, 40));
        break;
    case NGB_AI_RETREAT:
        if (!e->ai_timer || !--e->ai_timer) { e->ai = NGB_AI_WAIT; break; }
        walk_to(e, p->x + sgn(side, 96 << 8), e->y, sp >> 1, dsp);
        e->facing = (uint8_t)(p->x < e->x);
        break;
    default:
        e->ai = NGB_AI_WAIT;
        break;
    }
}

/* ======================================================================
 * moves, physics and states for everyone
 * ==================================================================== */
static void NEOGEO_USER run_move(NGBeatEmUp *b, uint8_t fi)
{
    NGBFighter *f = &b->f[fi];
    const NGBMove *m = move_of(b, f);
    uint8_t total, i;
    if (!m) {
        /* a throw's recovery */
        if (f->hitstun) { f->hitstun--; return; }
        f->state = airborne(f) ? NGB_ST_JUMP : NGB_ST_IDLE;
        return;
    }
    total = (uint8_t)(m->startup + m->active + m->recovery);
    if (f->move_frame >= m->startup && f->move_frame < (uint8_t)(m->startup + m->active)) {
        if (m->lunge) f->vx = flip(f->facing, (int32_t)m->lunge << 4);
        if (f->move_frame == m->startup && (m->flags & NGB_MV_PROJECTILE)) spawn_shot(b, fi, m);
        if (!(m->flags & NGB_MV_PROJECTILE)) {
            for (i = 0; i < NGB_FIGHTERS; i++) {
                NGBFighter *t = &b->f[i];
                if (i == fi || !t->alive || !enemy_of(fi, i)) continue;
                if (t->last_hit[fi] == f->serial) continue;
                if (!touches(f, t, m)) continue;
                t->last_hit[fi] = f->serial;
                if (m->flags & NGB_MV_GRAB) {
                    if (can_be_hit(t) && !airborne(t) && t->state != NGB_ST_DOWN && t->state != NGB_ST_FALL) { grab(b, fi, i); return; }
                    continue;
                }
                if (apply_hit(b, fi, i, m, m->damage) && fi == NGB_PLAYER && f->move == 0xFEu && f->weapon != NGB_NONE) {
                    NGBProp *w = &b->prop[f->weapon];
                    if (w->hp) w->hp--;
                    if (!w->hp) { emit(b, NGB_EV_WEAPON_BREAK, fi, f->weapon, 0, 0); w->kind = NGB_PROP_NONE; f->weapon = NGB_NONE; }
                }
                if (f->state != NGB_ST_ATTACK) return;      /* the hit stopped us (it was blocked into a throw) */
            }
            /* crates and barrels */
            if (fi == NGB_PLAYER)
                for (i = 0; i < NGB_MAX_PROPS; i++) {
                    NGBProp *p = &b->prop[i];
                    NGBFighter tmp;
                    if ((p->kind != NGB_PROP_CRATE && p->kind != NGB_PROP_BARREL) || p->state != 0 || p->timer) continue;
                    tmp.x = p->x; tmp.y = p->y; tmp.z = 0; tmp.def = 0; tmp.state = NGB_ST_IDLE;
                    if (!touches(f, &tmp, m)) continue;
                    p->timer = 12;
                    if (p->hp) p->hp--;
                    if (!p->hp) {
                        emit(b, NGB_EV_BREAK, fi, i, 0, 0);
                        if (p->contents != NGB_PROP_NONE && p->contents <= NGB_PROP_SCORE) {
                            p->kind = p->contents;
                            p->contents = (uint8_t)(p->kind == NGB_PROP_FOOD ? 12u : p->kind == NGB_PROP_SCORE ? 5u : 0u);
                            if (p->kind == NGB_PROP_WEAPON) p->hp = b->stage->nweapons ? b->stage->weapons[0].durability : 0u;
                        } else {
                            p->kind = NGB_PROP_NONE;
                        }
                    }
                }
        }
    }
    f->move_frame++;
    if (f->chain_buf && m->next != NGB_NONE && m->cancel && f->move_frame >= m->cancel && f->move != 0xFEu) {
        start_move(b, fi, m->next);
        return;
    }
    if (f->move_frame >= total) {
        f->move = NGB_NONE;
        f->vx = airborne(f) ? f->vx : 0;
        f->state = airborne(f) ? NGB_ST_JUMP : NGB_ST_IDLE;
        f->chain_buf = 0;
    }
}

static void NEOGEO_USER thrown_hits(NGBeatEmUp *b, uint8_t ti)
{
    NGBFighter *t = &b->f[ti];
    uint8_t i;
    for (i = 1; i < NGB_FIGHTERS; i++) {
        NGBFighter *o = &b->f[i];
        if (i == ti || !o->alive || !can_be_hit(o) || o->last_hit[ti] == t->serial) continue;
        if (o->state == NGB_ST_DOWN || o->state == NGB_ST_FALL) continue;
        if (iabs(o->x - t->x) > (18 << 8) || iabs(o->y - t->y) > (10 << 8)) continue;
        o->last_hit[ti] = t->serial;
        {
            NGBMove hit;
            hit.startup = 0; hit.active = 1; hit.recovery = 0; hit.damage = THROWN_DAMAGE; hit.hitstun = 0;
            hit.flags = NGB_MV_KNOCKDOWN | NGB_MV_UNBLOCKABLE; hit.box_x = 0; hit.box_w = 0; hit.box_z = 0;
            hit.box_h = 0; hit.box_depth = 0; hit.push_x = (int8_t)(t->vx > 0 ? 40 : -40); hit.push_z = 40;
            hit.lunge = 0; hit.cancel = 0; hit.next = NGB_NONE; hit.sound = 0; hit.anim = 0;
            apply_hit(b, NGB_PLAYER, i, &hit, THROWN_DAMAGE);
        }
    }
}

static void NEOGEO_USER physics(NGBeatEmUp *b, uint8_t fi)
{
    NGBFighter *f = &b->f[fi];
    const NGBStage *s = b->stage;
    int32_t y0 = (int32_t)s->y_min << 8, y1 = (int32_t)s->y_max << 8;
    uint8_t was_air = airborne(f);
    if (f->state == NGB_ST_GRABBED) return;
    f->x += f->vx;
    f->y += f->vy;
    /* a tall fighter keeps its head on the screen: its feet no higher up than its height */
    if (f->def && ((int32_t)f->def->height << 8) > y0) y0 = (int32_t)f->def->height << 8;
    if (y0 > y1) y0 = y1;
    if (f->y < y0) f->y = y0;
    if (f->y > y1) f->y = y1;
    if (f->x < 0) f->x = 0;
    if (f->x > ((int32_t)s->length << 8)) f->x = (int32_t)s->length << 8;
    if (fi == NGB_PLAYER) {
        int32_t lo = b->camera_x + (8 << 8), hi = b->camera_x + ((int32_t)(s->view_w ? s->view_w : 320u) << 8) - (8 << 8);
        if (f->x < lo) f->x = lo;
        if (f->x > hi) f->x = hi;
    } else if (f->ai != NGB_AI_ENTER && f->state != NGB_ST_DEAD) {
        /* an enemy that has come into view stays all in it: a throw or a
         * knock-back stops at the screen's edge (twice its half width in) */
        int32_t m = (int32_t)(f->def && f->def->half_w > 8u ? (uint16_t)f->def->half_w * 2u : 16u) << 8;
        int32_t lo = b->camera_x + m, hi = b->camera_x + ((int32_t)(s->view_w ? s->view_w : 320u) << 8) - m;
        if (lo < hi) {
            if (f->x < lo) { f->x = lo; if (f->vx < 0) f->vx = 0; }
            if (f->x > hi) { f->x = hi; if (f->vx > 0) f->vx = 0; }
        }
    }
    if (was_air) {
        f->z += f->vz;
        f->vz -= GRAVITY;
        if (f->z <= 0) {
            f->z = 0; f->vz = 0;
            if (f->state == NGB_ST_FALL || f->state == NGB_ST_THROWN) {
                if (f->state == NGB_ST_THROWN) {
                    f->hp = (uint8_t)(f->hp > THROW_DAMAGE ? (uint8_t)(f->hp - THROW_DAMAGE) : 0u);
                    emit(b, NGB_EV_HIT, NGB_PLAYER, fi, THROW_DAMAGE, 0);
                }
                f->state = NGB_ST_DOWN; f->timer = 0; f->vx = 0; f->vy = 0;
            } else if (f->state == NGB_ST_JUMP) {
                f->state = NGB_ST_IDLE; f->vx = 0;
                emit(b, NGB_EV_LAND, fi, 0, 0, 0);
            } else if (f->state == NGB_ST_ATTACK) {
                f->vx = 0;    /* an air attack lands and recovers on the ground */
            }
        }
    }
    if (f->state == NGB_ST_THROWN && fi != NGB_PLAYER) thrown_hits(b, fi);
}

static void NEOGEO_USER fighter_states(NGBeatEmUp *b, uint8_t fi)
{
    NGBFighter *f = &b->f[fi];
    if (f->timer < 0xFFFFu) f->timer++;
    switch (f->state) {
    case NGB_ST_HITSTUN: case NGB_ST_BLOCK:
        f->vx = f->vx - (f->vx >> 2);
        if (!f->hitstun || !--f->hitstun) { f->state = NGB_ST_IDLE; f->vx = 0; }
        break;
    case NGB_ST_DOWN:
        if (!f->hp) {
            if (f->timer >= DOWN_FRAMES) {
                if (fi == NGB_PLAYER) {
                    f->state = NGB_ST_DEAD; f->timer = 0;
                    if (b->lives) b->lives--;
                    emit(b, NGB_EV_PLAYER_DOWN, 0, 0, b->lives, 0);
                    if (b->lives) b->respawn_timer = 90;
                    else { b->status = NGB_GAME_OVER; emit(b, NGB_EV_GAME_OVER, 0, 0, 0, 0); }
                } else {
                    emit(b, NGB_EV_KO, NGB_NONE, fi, 0, f->def ? f->def->score : 0u);
                    add_score(b, f->def ? f->def->score : 0u);
                    f->state = NGB_ST_DEAD; f->alive = 0; f->token = 0;
                }
            }
            break;
        }
        if (f->timer >= DOWN_FRAMES) { f->state = NGB_ST_GETUP; f->timer = 0; f->invuln = GETUP_FRAMES + GETUP_INVULN; }
        break;
    case NGB_ST_GETUP:
        if (f->timer >= GETUP_FRAMES) {
            f->state = NGB_ST_IDLE; f->juggle = 0;
            emit(b, NGB_EV_GETUP, NGB_NONE, fi, 0, 0);
        }
        break;
    case NGB_ST_GRABBED: {
        NGBFighter *h = f->hold < NGB_FIGHTERS ? &b->f[f->hold] : 0;
        if (!h || !h->alive || h->hold != fi) { f->hold = NGB_NONE; f->state = NGB_ST_IDLE; break; }
        f->x = h->x + flip(h->facing, 18 << 8); f->y = h->y;
        /* the held one wriggles free in time */
        if (fi != NGB_PLAYER && ++f->hold_frames > (uint8_t)(HOLD_MAX - 10u * (f->def && f->def->weight < 8u ? f->def->weight : 8u))) {
            release(b, fi);
            emit(b, NGB_EV_ESCAPE, f->hold, fi, 0, 0);
        }
        break;
    }
    case NGB_ST_GRAB:
        if (f->hold == NGB_NONE || !b->f[f->hold].alive) { f->hold = NGB_NONE; f->state = NGB_ST_IDLE; }
        else if (fi == NGB_PLAYER && ++f->hold_frames > HOLD_MAX) release(b, fi);
        break;
    default:
        break;
    }
}

/* ======================================================================
 * props and shots
 * ==================================================================== */
static void NEOGEO_USER update_props(NGBeatEmUp *b)
{
    uint8_t i, j;
    for (i = 0; i < NGB_MAX_PROPS; i++) {
        NGBProp *p = &b->prop[i];
        if (p->kind == NGB_PROP_NONE) continue;
        if (p->timer) p->timer--;
        if (p->state == 1 && p->owner < NGB_FIGHTERS) {           /* held: with its owner */
            p->x = b->f[p->owner].x; p->y = b->f[p->owner].y; p->z = 20 << 8;
        } else if (p->state == 3 && p->owner < NGB_FIGHTERS) {    /* ridden */
            p->x = b->f[p->owner].x; p->y = b->f[p->owner].y; p->z = b->f[p->owner].z;
        } else if (p->state == 2) {                                /* flying */
            p->x += p->vx; p->z += p->vz; p->vz -= GRAVITY >> 1;
            for (j = 1; j < NGB_FIGHTERS; j++) {
                NGBFighter *t = &b->f[j];
                NGBMove hit;
                uint8_t dmg;
                if (!t->alive || !can_be_hit(t) || t->state == NGB_ST_DOWN) continue;
                if (iabs(t->x - p->x) > (14 << 8) || iabs(t->y - p->y) > (10 << 8)) continue;
                dmg = (uint8_t)(p->kind == NGB_PROP_WEAPON && b->stage->nweapons ? b->stage->weapons[p->contents].throw_damage : 8u);
                hit.startup = 0; hit.active = 1; hit.recovery = 0; hit.damage = dmg; hit.hitstun = 0;
                hit.flags = NGB_MV_KNOCKDOWN; hit.box_x = 0; hit.box_w = 0; hit.box_z = 0; hit.box_h = 0;
                hit.box_depth = 0; hit.push_x = (int8_t)(p->vx > 0 ? 32 : -32); hit.push_z = 32; hit.lunge = 0;
                hit.cancel = 0; hit.next = NGB_NONE; hit.sound = 0; hit.anim = 0;
                apply_hit(b, NGB_PLAYER, j, &hit, dmg);
                p->vx = -(p->vx >> 2);
                if (p->hp) p->hp--;
                break;
            }
            if (p->z <= 0 || !p->timer) {
                p->z = 0; p->vx = 0; p->vz = 0; p->state = 0; p->owner = NGB_NONE;
                if (p->kind == NGB_PROP_WEAPON && !p->hp) p->kind = NGB_PROP_NONE;
            }
        } else if (p->kind == NGB_PROP_HAZARD && !p->timer) {
            p->timer = 32;
            for (j = 0; j < NGB_FIGHTERS; j++) {
                NGBFighter *t = &b->f[j];
                if (!t->alive || !can_be_hit(t) || t->z > (8 << 8)) continue;
                if (iabs(t->x - p->x) > (12 << 8) || iabs(t->y - p->y) > (8 << 8)) continue;
                {
                    NGBMove hit;
                    hit.startup = 0; hit.active = 1; hit.recovery = 0; hit.damage = 2; hit.hitstun = 0;
                    hit.flags = NGB_MV_KNOCKDOWN | NGB_MV_UNBLOCKABLE; hit.box_x = 0; hit.box_w = 0; hit.box_z = 0;
                    hit.box_h = 0; hit.box_depth = 0; hit.push_x = 24; hit.push_z = 40; hit.lunge = 0;
                    hit.cancel = 0; hit.next = NGB_NONE; hit.sound = 0; hit.anim = 0;
                    apply_hit(b, j, j, &hit, 2);
                }
            }
        }
    }
    for (i = 0; i < NGB_MAX_SHOTS; i++) {
        NGBShot *s = &b->shot[i];
        uint8_t t;
        if (!s->alive) continue;
        s->x += s->vx;
        if (!--s->timer || s->x < b->camera_x - (32 << 8) || s->x > b->camera_x + ((int32_t)b->stage->view_w << 8) + (32 << 8)) { s->alive = 0; continue; }
        for (t = 0; t < NGB_FIGHTERS; t++) {
            NGBFighter *f = &b->f[t];
            if (!f->alive || !enemy_of(s->owner, t) || !can_be_hit(f) || f->state == NGB_ST_DOWN) continue;
            if (iabs(f->x - s->x) > (10 << 8) || iabs(f->y - s->y) > (8 << 8)) continue;
            if (s->z < f->z || s->z > f->z + ((int32_t)(f->def ? f->def->height : 60u) << 8)) continue;
            apply_hit(b, s->owner < NGB_FIGHTERS ? s->owner : t, t, &shove_move, s->damage);
            s->alive = 0;
            break;
        }
    }
}

/* ======================================================================
 * waves and the camera
 * ==================================================================== */
static void NEOGEO_USER waves(NGBeatEmUp *b)
{
    const NGBStage *s = b->stage;
    if (!b->spawn_pending && b->wave_next < s->nwaves && b->camera_x >= ((int32_t)s->waves[b->wave_next].at_x << 8)) {
        const NGBWave *w = &s->waves[b->wave_next];
        b->spawn_wave = b->wave_next;
        b->spawn_pending = w->count;
        b->spawn_timer = 0;
        b->wave_live = (uint8_t)(b->wave_next + 1u);
        if (w->flags & NGB_WAVE_LOCK) { b->locked = 1; b->lock_left = b->lock_right = b->camera_x; }
        emit(b, NGB_EV_WAVE, 0, 0, b->wave_next, 0);
        b->wave_next++;
    }
    if (b->spawn_pending) {
        const NGBWave *w = &s->waves[b->spawn_wave];
        uint8_t k = (uint8_t)(w->count - b->spawn_pending);
        if (b->spawn_timer) b->spawn_timer--;
        else if (ng_beatemup_enemies_alive(b) < (s->max_active ? s->max_active : NGB_MAX_ENEMIES) && w->enemy[k] < s->nenemies) {
            uint8_t y = (uint8_t)(s->y_min + rnd_range(b, (uint16_t)(s->y_max - s->y_min + 1u)));
            int r = ng_beatemup_spawn(b, s->enemies[w->enemy[k]], 0, y, (uint8_t)((w->sides >> k) & 1u));
            if (r >= 0) {
                b->f[r].wave = (uint8_t)(b->spawn_wave + 1u);
                b->spawn_pending--;
                b->spawn_timer = w->gap;
            }
        }
    }
    if (b->wave_live && !b->spawn_pending) {
        uint8_t i, left = 0;
        for (i = 1; i < NGB_FIGHTERS; i++) if (b->f[i].alive && b->f[i].wave == b->wave_live) left++;
        if (!left) {
            uint8_t idx = (uint8_t)(b->wave_live - 1u);
            emit(b, NGB_EV_WAVE_CLEAR, 0, 0, idx, 0);
            b->wave_live = 0;
            if (b->locked) { b->locked = 0; emit(b, NGB_EV_GO, 0, 0, 0, 0); }
            if (s->waves[idx].flags & NGB_WAVE_BOSS) { b->status = NGB_CLEAR; emit(b, NGB_EV_STAGE_CLEAR, 0, 0, 0, 0); }
        }
    }
}

static void NEOGEO_USER camera(NGBeatEmUp *b)
{
    const NGBStage *s = b->stage;
    NGBFighter *p = &b->f[NGB_PLAYER];
    int32_t view = (int32_t)(s->view_w ? s->view_w : 320u) << 8;
    int32_t target = p->x - (view >> 1) + (p->facing ? -(32 << 8) : (32 << 8));
    int32_t max = ((int32_t)s->length << 8) - view, step = (int32_t)(s->camera_max ? s->camera_max : 6u) << 8;
    int32_t dx;
    if (target < 0) target = 0;
    if (target > max) target = max;
    if (b->locked) { if (target < b->lock_left) target = b->lock_left; if (target > b->lock_right) target = b->lock_right; }
    if ((s->flags & NGB_SF_NO_BACKTRACK) && target < b->camera_x) target = b->camera_x;
    dx = target - b->camera_x;
    if (dx > step) dx = step;
    if (dx < -step) dx = -step;
    b->camera_x += dx;
}

/* ======================================================================
 * the frame
 * ==================================================================== */
void NEOGEO_USER ng_beatemup_input(NGBeatEmUp *b, uint8_t dirs, uint8_t buttons)
{
    uint8_t held;
    if (!b) return;
    held = (uint8_t)(dirs | buttons);
    b->in_pressed = (uint8_t)(held & ~(b->in_dirs | b->in_buttons));
    b->in_dirs = (uint8_t)(dirs & 0x0Fu);
    b->in_buttons = (uint8_t)(buttons & 0xF0u);
}

static void NEOGEO_USER respawn(NGBeatEmUp *b)
{
    NGBFighter *p = &b->f[NGB_PLAYER];
    uint8_t i;
    p->hp = p->def->max_hp;
    p->state = NGB_ST_IDLE; p->timer = 0; p->invuln = RESPAWN_INVULN;
    p->x = b->camera_x + ((int32_t)b->stage->view_w << 7);
    p->z = 0; p->vx = p->vy = p->vz = 0; p->hits_taken = 0; p->juggle = 0; p->hold = NGB_NONE;
    /* clear some room */
    for (i = 1; i < NGB_FIGHTERS; i++) {
        NGBFighter *e = &b->f[i];
        if (!e->alive || !can_be_hit(e)) continue;
        if (iabs(e->x - p->x) < (80 << 8)) knock_down(b, i, (e->x < p->x ? -1 : 1) * (3 << 8), 3 << 8);
    }
    emit(b, NGB_EV_RESPAWN, 0, 0, 0, 0);
}

void NEOGEO_USER ng_beatemup_update(NGBeatEmUp *b)
{
    uint8_t i;
    NGBFighter *p;
    if (!b || !b->stage) return;
    b->tick++;
    p = &b->f[NGB_PLAYER];
    if (b->status == NGB_GAME_OVER || b->status == NGB_CLEAR) { b->in_pressed = 0; return; }
    if (b->attack_cool) b->attack_cool--;
    if (b->special_cool) b->special_cool--;
    if (b->combo_timer && !--b->combo_timer) b->combo = 0;
    if (p->state == NGB_ST_DEAD && b->respawn_timer && !--b->respawn_timer) respawn(b);
    if (p->hits_taken && (b->tick & 0x3Fu) == 0) p->hits_taken--;      /* forgiven over time */
    for (i = 0; i < NGB_FIGHTERS; i++) {
        NGBFighter *f = &b->f[i];
        if (!f->alive) continue;
        if (f->invuln) f->invuln--;
        if (f->hitstop) { f->hitstop--; continue; }
        if (i == NGB_PLAYER) player_control(b);
        else enemy_ai(b, i);
        if (f->state == NGB_ST_ATTACK) run_move(b, i);
        physics(b, i);
        fighter_states(b, i);
        if (i == NGB_PLAYER) walk_over(b);
        else if (f->def && f->def->think && f->alive) f->def->think(b, i);
    }
    update_props(b);
    if (!(b->tick & 7u)) director(b);
    camera(b);
    waves(b);
    b->in_pressed = 0;
}

/* ======================================================================
 * set-up and queries
 * ==================================================================== */
int NEOGEO_USER ng_beatemup_spawn(NGBeatEmUp *b, const NGBFighterDef *def, int16_t x, uint8_t y, uint8_t from)
{
    uint8_t i;
    if (!b || !def || !def->max_hp) return NGB_ERR_ARG;
    for (i = 1; i < NGB_FIGHTERS; i++) {
        NGBFighter *e = &b->f[i];
        if (e->alive) continue;
        zero((uint8_t *)e, (uint16_t)sizeof(NGBFighter));
        e->alive = 1; e->def = def; e->hp = def->max_hp; e->state = NGB_ST_IDLE;
        e->move = NGB_NONE; e->hold = NGB_NONE; e->weapon = NGB_NONE; e->vehicle = NGB_NONE;
        if (from == 1) e->x = b->camera_x - (24 << 8);
        else if (from == 0) e->x = b->camera_x + ((int32_t)(b->stage->view_w ? b->stage->view_w : 320u) << 8) + (24 << 8);
        else e->x = (int32_t)x << 8;
        if (e->x < 0) e->x = 0;
        e->y = (int32_t)y << 8;
        e->ai = from <= 1u ? NGB_AI_ENTER : NGB_AI_WAIT;
        e->facing = (uint8_t)(b->f[NGB_PLAYER].x < e->x);
        {
            uint8_t k;
            for (k = 0; k < NGB_FIGHTERS; k++) e->last_hit[k] = 0xFFu;
        }
        return i;
    }
    return NGB_ERR_FULL;
}

int NEOGEO_USER ng_beatemup_add_prop(NGBeatEmUp *b, uint8_t kind, int16_t x, uint8_t y, uint8_t contents, uint8_t hp)
{
    uint8_t i;
    if (!b || kind == NGB_PROP_NONE || kind > NGB_PROP_RAMP) return NGB_ERR_ARG;
    for (i = 0; i < NGB_MAX_PROPS; i++) {
        NGBProp *p = &b->prop[i];
        if (p->kind != NGB_PROP_NONE) continue;
        zero((uint8_t *)p, (uint16_t)sizeof(NGBProp));
        p->kind = kind; p->x = (int32_t)x << 8; p->y = (int32_t)y << 8; p->contents = contents;
        p->owner = NGB_NONE;
        p->hp = hp;
        if (!hp) {
            if (kind == NGB_PROP_VEHICLE) p->hp = b->stage->vehicle_durability ? b->stage->vehicle_durability : 8u;
            else if (kind == NGB_PROP_WEAPON && contents < b->stage->nweapons) p->hp = b->stage->weapons[contents].durability;
            else p->hp = 1;
        }
        return i;
    }
    return NGB_ERR_FULL;
}

int NEOGEO_USER ng_beatemup_init(NGBeatEmUp *b, const NGBStage *stage, const NGBFighterDef *player, uint8_t lives, uint32_t seed)
{
    NGBFighter *p;
    uint8_t i;
    if (!b || !stage || !player || !player->max_hp || !stage->length || stage->y_max < stage->y_min) return NGB_ERR_ARG;
    if (stage->max_active > NGB_MAX_ENEMIES) return NGB_ERR_ARG;
    zero((uint8_t *)b, (uint16_t)sizeof(NGBeatEmUp));
    b->stage = stage;
    b->rng = seed ? seed : 0x2468ACE1u;
    b->lives = lives ? lives : 3u;
    b->status = NGB_PLAYING;
    p = &b->f[NGB_PLAYER];
    p->alive = 1; p->def = player; p->hp = player->max_hp; p->state = NGB_ST_IDLE;
    p->move = NGB_NONE; p->hold = NGB_NONE; p->weapon = NGB_NONE; p->vehicle = NGB_NONE;
    p->x = 64 << 8; p->y = (int32_t)((stage->y_min + stage->y_max) >> 1) << 8;
    for (i = 0; i < NGB_FIGHTERS; i++) p->last_hit[i] = 0xFFu;
    for (i = 0; i < NGB_MAX_SHOTS; i++) b->shot[i].alive = 0;
    for (i = 0; i < stage->nprops; i++) {
        const NGBPropSpawn *ps = &stage->props[i];
        ng_beatemup_add_prop(b, ps->kind, (int16_t)ps->x, ps->y, ps->contents, ps->hp);
    }
    return NGB_OK;
}

void NEOGEO_USER ng_beatemup_continue(NGBeatEmUp *b, uint8_t lives)
{
    if (!b || b->status != NGB_GAME_OVER) return;
    b->lives = lives ? lives : 3u;
    b->status = NGB_PLAYING;
    b->score = 0;
    respawn(b);
}

uint8_t NEOGEO_USER ng_beatemup_status(const NGBeatEmUp *b)
{
    if (!b) return NGB_GAME_OVER;
    if (b->status == NGB_PLAYING && b->f[NGB_PLAYER].state == NGB_ST_DEAD) return NGB_PLAYER_OUT;
    return b->status;
}

uint8_t NEOGEO_USER ng_beatemup_enemies_alive(const NGBeatEmUp *b)
{
    uint8_t i, n = 0;
    if (!b) return 0;
    for (i = 1; i < NGB_FIGHTERS; i++) if (b->f[i].alive) n++;
    return n;
}

uint8_t NEOGEO_USER ng_beatemup_event(NGBeatEmUp *b, NGBEvent *out)
{
    NGBEvent *e;
    if (!b || !b->ev_count) return 0;
    e = &b->ev[b->ev_head];
    if (out) { out->type = e->type; out->a = e->a; out->b = e->b; out->c = e->c; out->value = e->value; }
    b->ev_head = (uint8_t)((b->ev_head + 1u) & (NGB_EVENT_QUEUE - 1u));
    b->ev_count--;
    return 1;
}

uint8_t NEOGEO_USER ng_beatemup_draw_list(const NGBeatEmUp *b, uint8_t *ids, uint8_t max)
{
    int32_t key[NGB_FIGHTERS + NGB_MAX_PROPS + NGB_MAX_SHOTS];
    uint8_t n = 0, i, j;
    if (!b || !ids) return 0;
    for (i = 0; i < NGB_FIGHTERS && n < max; i++)
        if (b->f[i].alive || (i == NGB_PLAYER && b->f[i].state != NGB_ST_NONE)) { ids[n] = i; key[n] = b->f[i].y; n++; }
    for (i = 0; i < NGB_MAX_PROPS && n < max; i++)
        if (b->prop[i].kind != NGB_PROP_NONE && b->prop[i].state != 1) { ids[n] = (uint8_t)(0x40u + i); key[n] = b->prop[i].y - 1; n++; }
    for (i = 0; i < NGB_MAX_SHOTS && n < max; i++)
        if (b->shot[i].alive) { ids[n] = (uint8_t)(0x80u + i); key[n] = b->shot[i].y; n++; }
    for (i = 1; i < n; i++) {           /* stable insertion sort, far first */
        uint8_t id = ids[i];
        int32_t k = key[i];
        j = i;
        while (j > 0 && key[j - 1u] > k) { ids[j] = ids[j - 1u]; key[j] = key[j - 1u]; j--; }
        ids[j] = id; key[j] = k;
    }
    return n;
}

static uint32_t NEOGEO_USER fnv32(uint32_t h, uint32_t v)
{
    uint8_t k;
    for (k = 0; k < 4u; k++) {
        h ^= (uint8_t)(v >> (24u - 8u * k));
        h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
    }
    return h;
}

uint32_t NEOGEO_USER ng_beatemup_hash(const NGBeatEmUp *b)
{
    uint32_t h = 2166136261u;
    uint8_t i;
    if (!b) return 0;
    for (i = 0; i < NGB_FIGHTERS; i++) {
        const NGBFighter *f = &b->f[i];
        h = fnv32(h, (uint32_t)f->x); h = fnv32(h, (uint32_t)f->y); h = fnv32(h, (uint32_t)f->z);
        h = fnv32(h, ((uint32_t)f->alive << 24) | ((uint32_t)f->state << 16) | ((uint32_t)f->hp << 8) | f->ai);
    }
    for (i = 0; i < NGB_MAX_PROPS; i++) {
        const NGBProp *p = &b->prop[i];
        h = fnv32(h, ((uint32_t)p->kind << 24) | ((uint32_t)p->state << 16) | ((uint32_t)p->hp << 8));
        h = fnv32(h, (uint32_t)p->x);
    }
    h = fnv32(h, (uint32_t)b->camera_x);
    h = fnv32(h, b->score);
    h = fnv32(h, b->rng);
    h = fnv32(h, ((uint32_t)b->wave_next << 16) | ((uint32_t)b->spawn_pending << 8) | b->lives);
    return h;
}
