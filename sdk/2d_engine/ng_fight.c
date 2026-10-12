#include "ng_fight.h"

/*
 * ng_fight core. Units: Q8 positions and speeds; frame data speeds (dx, dy,
 * launch_vx/vy) are in eighths of a pixel per frame, so `v * 32` is Q8.
 */

#define FIGHT_MARGIN   16
#define CHARGE_FRAMES  40u
#define CHARGE_WINDOW  10u
#define GUARD_REGEN     8u
#define DOWN_FRAMES    34u
#define DIZZY_FRAMES   56u
#define INTRO_FRAMES   96u
#define KO_FRAMES     150u
#define TIMEUP_FRAMES  90u
#define END_FRAMES    130u

/* ===================================================================== */
/*  Input                                                                 */
/* ===================================================================== */
#define DIR(n) ((uint16_t)(1u << (n)))
#define BACKS  (DIR(1) | DIR(4) | DIR(7))
#define FWDS   (DIR(3) | DIR(6) | DIR(9))
#define DOWNS  (DIR(1) | DIR(2) | DIR(3))
#define UPS    (DIR(7) | DIR(8) | DIR(9))

static uint8_t NEOGEO_USER numpad(uint8_t raw, uint8_t facing_left)
{
    uint8_t fwd = facing_left ? (raw & NGF_RAW_LEFT) : (raw & NGF_RAW_RIGHT);
    uint8_t back = facing_left ? (raw & NGF_RAW_RIGHT) : (raw & NGF_RAW_LEFT);
    uint8_t up = raw & NGF_RAW_UP, down = raw & NGF_RAW_DOWN;
    uint8_t col, row;
    if (fwd && back) fwd = back = 0;
    if (up && down) up = down = 0;
    col = back ? 0u : (fwd ? 2u : 1u);
    row = down ? 0u : (up ? 2u : 1u);
    return (uint8_t)(row * 3u + col + 1u);
}

uint8_t NEOGEO_USER ngf_raw_from_numpad(uint8_t n, uint8_t facing_left)
{
    uint8_t raw = 0, col, row;
    if (n < 1u || n > 9u) return 0;
    col = (uint8_t)((n - 1u) % 3u);
    row = (uint8_t)((n - 1u) / 3u);
    if (row == 0u) raw |= NGF_RAW_DOWN;
    if (row == 2u) raw |= NGF_RAW_UP;
    if (col == 0u) raw |= facing_left ? NGF_RAW_RIGHT : NGF_RAW_LEFT;
    if (col == 2u) raw |= facing_left ? NGF_RAW_LEFT : NGF_RAW_RIGHT;
    return raw;
}

void NEOGEO_USER ngf_input_reset(NGFInput *in)
{
    uint8_t *p = (uint8_t *)in;
    uint16_t i;
    for (i = 0; i < sizeof(*in); i++) p[i] = 0;
}

static void NEOGEO_USER charge(uint8_t held, uint8_t *count, uint8_t *release, uint8_t *age)
{
    if (held) {
        if (*count < 255u) (*count)++;
    } else if (*count) {
        *release = *count;
        *count = 0;
        *age = 0;
    } else if (*age < 255u) {
        (*age)++;
    }
}

void NEOGEO_USER ngf_input_push(NGFInput *in, uint8_t raw, uint8_t facing_left)
{
    uint8_t prev = in->raw[in->head];
    uint8_t dir;
    in->head = (uint8_t)((in->head + 1u) & (NGF_INPUT_FRAMES - 1u));
    in->raw[in->head] = raw;
    in->press[in->head] = (uint8_t)(raw & (uint8_t)~prev & NGF_BTN_ALL);
    in->released = (uint8_t)(prev & (uint8_t)~raw & NGF_BTN_ALL);
    dir = numpad(raw, facing_left);
    charge((uint8_t)((BACKS >> dir) & 1u), &in->charge_back, &in->back_release, &in->back_age);
    charge((uint8_t)((DOWNS >> dir) & 1u), &in->charge_down, &in->down_release, &in->down_age);
}

uint8_t NEOGEO_USER ngf_input_dir(const NGFInput *in, uint8_t ago, uint8_t facing_left)
{
    return numpad(in->raw[(uint8_t)(in->head - ago) & (NGF_INPUT_FRAMES - 1u)], facing_left);
}

uint8_t NEOGEO_USER ngf_input_pressed(const NGFInput *in, uint8_t mask, uint8_t buffer)
{
    uint8_t seen = 0, ago;
    if (!mask) return 0;
    for (ago = 0; ago < buffer && ago < NGF_INPUT_FRAMES; ago++)
        seen |= in->press[(uint8_t)(in->head - ago) & (NGF_INPUT_FRAMES - 1u)];
    return (uint8_t)((seen & mask) == mask);
}

void NEOGEO_USER ngf_input_consume(NGFInput *in, uint8_t mask)
{
    uint8_t i;
    for (i = 0; i < NGF_INPUT_FRAMES; i++) in->press[i] &= (uint8_t)~mask;
    in->back_release = in->down_release = 0;
}

/* Ordered steps per motion, each a set of numpad directions. */
static const uint16_t motion_steps[NGF_MOTION_COUNT][6] = {
    {0},
    {DIR(2), DIR(3), DIR(6)},
    {DIR(2), DIR(1), DIR(4)},
    {DIR(6), DIR(2), DIR(3)},
    {DIR(4), DIR(2), DIR(1)},
    {DIR(4), DIR(2), DIR(6)},
    {DIR(6), DIR(2), DIR(4)},
    {DIR(2), DIR(6), DIR(2), DIR(3), DIR(6)},
    {DIR(2), DIR(4), DIR(2), DIR(1), DIR(4)},
};
static const uint8_t motion_len[NGF_MOTION_COUNT] = {0, 3, 3, 3, 3, 3, 3, 5, 5};
static const uint8_t motion_window[NGF_MOTION_COUNT] = {0, 15, 15, 15, 15, 22, 22, 30, 30, 0, 0, 30, 12, 12};

uint8_t NEOGEO_USER ngf_input_motion(const NGFInput *in, uint8_t motion, uint8_t facing_left, uint8_t window)
{
    uint8_t now = ngf_input_dir(in, 0, facing_left);
    uint8_t ago, step;
    if (motion >= NGF_MOTION_COUNT) return 0;
    if (motion > NGF_MOTION_DQCB && motion < NGF_MOTION_CHARGE_BF) return 0;
    if (!window) window = motion_window[motion];
    if (window > NGF_INPUT_FRAMES) window = NGF_INPUT_FRAMES;
    switch (motion) {
    case NGF_MOTION_NONE: return 1;
    case NGF_MOTION_FWD: return (uint8_t)(now == 6u);
    case NGF_MOTION_BACK: return (uint8_t)(now == 4u);
    case NGF_MOTION_DOWN_FWD: return (uint8_t)(now == 3u);
    case NGF_MOTION_CHARGE_BF:
        return (uint8_t)(((FWDS >> now) & 1u) && in->back_release >= CHARGE_FRAMES && in->back_age <= CHARGE_WINDOW);
    case NGF_MOTION_CHARGE_DU:
        return (uint8_t)(((UPS >> now) & 1u) && in->down_release >= CHARGE_FRAMES && in->down_age <= CHARGE_WINDOW);
    case NGF_MOTION_360: {
        uint16_t seen = 0;
        for (ago = 0; ago < window; ago++) seen |= DIR(ngf_input_dir(in, ago, facing_left));
        return (uint8_t)((seen & BACKS) && (seen & FWDS) && (seen & DOWNS) && (seen & UPS));
    }
    case NGF_MOTION_DASH:
    case NGF_MOTION_BACKDASH: {
        uint8_t want = motion == NGF_MOTION_DASH ? 6u : 4u, gap = 0;
        if (now != want || ngf_input_dir(in, 1, facing_left) == want) return 0;
        for (ago = 1; ago < window; ago++) {
            uint8_t d = ngf_input_dir(in, ago, facing_left);
            if (d == 5u) gap = 1;
            else if (d == want && gap) return 1;
            else if (d != want) return 0;
        }
        return 0;
    }
    default:
        break;
    }
    /* The motion must end within the last five frames, steps in order. */
    step = motion_len[motion];
    for (ago = 0; ago < window && step; ago++) {
        uint8_t d = ngf_input_dir(in, ago, facing_left);
        if (step == motion_len[motion] && ago > 4u) return 0;
        if ((motion_steps[motion][step - 1u] >> d) & 1u) step--;
    }
    return (uint8_t)(step == 0u);
}

/* ===================================================================== */
/*  Helpers                                                               */
/* ===================================================================== */
static int16_t NEOGEO_USER sign_of(const NGFFighter *f) { return f->facing_left ? -1 : 1; }
int16_t NEOGEO_USER ngf_fighter_px(const NGFFighter *f) { return (int16_t)(f->x >> 8); }
int16_t NEOGEO_USER ngf_fighter_py(const NGFFighter *f) { return (int16_t)(f->h >> 8); }

uint16_t NEOGEO_USER ngf_distance(const NGFMatch *m)
{
    int32_t d = (m->f[0].x - m->f[1].x) >> 8;
    return (uint16_t)(d < 0 ? -d : d);
}

static void NEOGEO_USER event(NGFMatch *m, uint8_t type, uint8_t who, uint8_t data, int16_t x, int16_t y)
{
    NGFEvent *e;
    if (m->ev_count >= NGF_EVENTS) return;
    e = &m->ev[m->ev_count++];
    e->type = type; e->who = who; e->data = data; e->pad = 0; e->x = x; e->y = y;
}

uint8_t NEOGEO_USER ngf_pop_event(NGFMatch *m, NGFEvent *out)
{
    uint8_t i;
    if (!m->ev_count) return 0;
    *out = m->ev[0];
    for (i = 1; i < m->ev_count; i++) m->ev[i - 1u] = m->ev[i];
    m->ev_count--;
    return 1;
}

static uint16_t NEOGEO_USER rnd(NGFMatch *m)
{
    m->rng = (uint16_t)(m->rng * 25173u + 13849u);
    return (uint16_t)(m->rng >> 3);
}

const NGFFrame *NEOGEO_USER ngf_fighter_frame(const NGFFighter *f)
{
    const NGFAnim *a = &f->def->anims[f->anim];
    return &f->def->frames[a->first + f->frame];
}

const NGFFrame *NEOGEO_USER ngf_projectile_frame(const NGFMatch *m, const NGFProjectile *p)
{
    const NGFFighterDef *d = m->f[p->owner].def;
    return &d->frames[d->anims[p->def->anim].first + p->frame];
}

static void NEOGEO_USER set_anim(NGFFighter *f, uint8_t anim, uint8_t restart)
{
    if (!restart && f->anim == anim) return;
    f->anim = anim;
    f->frame = 0;
    f->frame_timer = 0;
}

static void NEOGEO_USER spawn_projectile(NGFMatch *m, uint8_t who);

static void NEOGEO_USER enter_frame(NGFMatch *m, uint8_t who)
{
    NGFFighter *f = &m->f[who];
    const NGFFrame *fr = ngf_fighter_frame(f);
    if (f->state != NGF_S_ATTACK) return;
    if (fr->flags & NGF_FF_REHIT) f->move_hit = 0;
    if (fr->flags & NGF_FF_ACTIVE) f->counter = 0;
    if ((fr->flags & NGF_FF_SPAWN) && f->move && f->move->projectile) spawn_projectile(m, who);
    if (fr->flags & NGF_FF_AIRBORNE) {
        f->grounded = 0;
        f->vh = (int16_t)(fr->dy * 32);
        f->vx = (int16_t)(fr->dx * 32 * sign_of(f));
    }
}

/* Advance the animation; 1 when a non-looping animation has ended. */
static uint8_t NEOGEO_USER anim_step(NGFMatch *m, uint8_t who)
{
    NGFFighter *f = &m->f[who];
    const NGFAnim *a = &f->def->anims[f->anim];
    const NGFFrame *fr = &f->def->frames[a->first + f->frame];
    if (++f->frame_timer < fr->time) return 0;
    f->frame_timer = 0;
    if ((uint8_t)(f->frame + 1u) < a->count) {
        f->frame++;
        enter_frame(m, who);
        return 0;
    }
    if (a->loop != 0xFFu) {
        f->frame = a->loop;
        enter_frame(m, who);
        return 0;
    }
    f->frame_timer = fr->time;   /* hold the last frame */
    return 1;
}

/* World-space box: left, right, bottom, top in pixels. */
typedef struct { int16_t l, r, b, t; } WBox;

static WBox NEOGEO_USER world_box(int32_t x, int32_t h, uint8_t facing_left, const NGFBox *b)
{
    WBox w;
    int16_t px = (int16_t)(x >> 8), ph = (int16_t)(h >> 8);
    w.l = facing_left ? (int16_t)(px - b->x - b->w) : (int16_t)(px + b->x);
    w.r = (int16_t)(w.l + b->w);
    w.b = (int16_t)(ph + b->y);
    w.t = (int16_t)(w.b + b->h);
    return w;
}

static uint8_t NEOGEO_USER overlap(WBox a, WBox b)
{
    return (uint8_t)(a.l < b.r && b.l < a.r && a.b < b.t && b.b < a.t);
}

static uint8_t NEOGEO_USER vulnerable(const NGFFighter *f)
{
    if (f->invuln || (ngf_fighter_frame(f)->flags & NGF_FF_INVULN)) return 0;
    switch (f->state) {
    case NGF_S_DOWN: case NGF_S_GETUP: case NGF_S_THROWN: case NGF_S_KO:
    case NGF_S_WIN: case NGF_S_LOSE: case NGF_S_INTRO:
        return 0;
    case NGF_S_FALL:
        return (uint8_t)(f->life != 0u);   /* juggles, but not after a KO */
    default:
        return 1;
    }
}

static uint8_t NEOGEO_USER hits_body(const NGFFighter *def, WBox hit)
{
    const NGFHurtSet *set = &def->def->hurt[ngf_fighter_frame(def)->hurt];
    uint8_t i;
    for (i = 0; i < set->count; i++)
        if (overlap(hit, world_box(def->x, def->h, def->facing_left, &set->box[i]))) return 1;
    return 0;
}

static uint8_t NEOGEO_USER can_block(const NGFFighter *f, uint8_t level)
{
    uint8_t dir;
    if (!f->grounded || level == NGF_LV_THROW || level == NGF_LV_UNBLOCKABLE) return 0;
    if (f->state != NGF_S_IDLE && f->state != NGF_S_CROUCH && f->state != NGF_S_BLOCK) return 0;
    dir = ngf_input_dir(&f->input, 0, f->facing_left);
    if (dir != 1u && dir != 4u) return 0;
    if (dir == 1u) return (uint8_t)(level != NGF_LV_OVERHEAD);
    return (uint8_t)(level != NGF_LV_LOW);
}

typedef struct {
    uint8_t damage, guard_damage, chip, hitstun, blockstun, pushback;
    uint8_t level, on_hit, meter, hitstop, sound;
    int8_t launch_vx, launch_vy;
} HitParams;

static void NEOGEO_USER push_away(NGFMatch *m, NGFFighter *att, NGFFighter *def, uint8_t px)
{
    int16_t away = def->x >= att->x ? 1 : -1;
    int32_t dx = def->x >> 8;
    uint8_t at_wall = (uint8_t)((away > 0 && dx >= m->stage_w - FIGHT_MARGIN - 2) ||
                                (away < 0 && dx <= FIGHT_MARGIN + 2) ||
                                (away > 0 && dx >= m->camera_x + m->screen_w - FIGHT_MARGIN - 2) ||
                                (away < 0 && dx <= m->camera_x + FIGHT_MARGIN + 2));
    int16_t v = (int16_t)(px * 64);
    if (at_wall) att->push_vx = (int16_t)(-away * v);
    else def->push_vx = (int16_t)(away * v);
}

static void NEOGEO_USER apply_hit(NGFMatch *m, uint8_t att_i, const HitParams *hp, int16_t ex, int16_t ey, uint8_t melee)
{
    NGFFighter *att = &m->f[att_i], *def = &m->f[att_i ^ 1u];
    if (can_block(def, hp->level)) {
        uint8_t chip = hp->chip;
        if (def->life <= chip) chip = (uint8_t)(def->life > 1u ? def->life - 1u : 0u);
        def->life = (uint16_t)(def->life - chip);
        def->state = NGF_S_BLOCK;
        def->stun_frames = hp->blockstun;
        set_anim(def, ngf_input_dir(&def->input, 0, def->facing_left) == 1u || def->block_low ?
                 NGF_A_GUARD_LOW : NGF_A_GUARD, 1);
        if (def->spirit < NGF_SPIRIT_MAX - 4u) def->spirit = (uint8_t)(def->spirit + 4u);
        if (att->power < NGF_POWER_MAX) att->power = (uint16_t)(att->power + hp->meter / 2u);
        if (def->guard <= hp->guard_damage) {
            def->guard = def->def->guard_max;
            def->state = NGF_S_DIZZY;
            def->state_timer = DIZZY_FRAMES;
            set_anim(def, NGF_A_DIZZY, 1);
            event(m, NGF_EV_GUARD_BREAK, (uint8_t)(att_i ^ 1u), 0, ex, ey);
        } else {
            def->guard = (uint8_t)(def->guard - hp->guard_damage);
            event(m, NGF_EV_BLOCK, (uint8_t)(att_i ^ 1u), hp->sound, ex, ey);
        }
        if (melee || def->grounded) push_away(m, att, def, hp->pushback);
        m->hitstop = (uint8_t)(hp->hitstop / 2u + 2u);
        return;
    }
    {
        uint16_t dmg = hp->damage;
        uint8_t scale = def->combo > 5u ? 5u : def->combo;
        dmg = (uint16_t)(dmg * (10u - scale) / 10u);
        if (def->state == NGF_S_ATTACK && def->counter) {
            dmg = (uint16_t)(dmg + dmg / 4u);
            event(m, NGF_EV_HIT, att_i, 0xFE, ex, ey);   /* counter */
        }
        if (!dmg) dmg = 1;
        def->life = def->life > dmg ? (uint16_t)(def->life - dmg) : 0u;
        def->combo++;
        def->combo_damage = (uint16_t)(def->combo_damage + dmg);
        if (att->power < NGF_POWER_MAX) att->power = (uint16_t)(att->power + hp->meter);
        if (def->power < NGF_POWER_MAX) def->power = (uint16_t)(def->power + hp->meter / 2u);
        if (att->power > NGF_POWER_MAX) att->power = NGF_POWER_MAX;
        if (def->power > NGF_POWER_MAX) def->power = NGF_POWER_MAX;
        if (!def->grounded || (hp->on_hit & (NGF_ON_KNOCKDOWN | NGF_ON_LAUNCH)) || !def->life) {
            int8_t lvx = hp->launch_vx ? hp->launch_vx : 12, lvy = hp->launch_vy ? hp->launch_vy : 28;
            int16_t away = def->x >= att->x ? 1 : -1;
            if (!def->life && lvy < 32) lvy = 32;
            def->state = NGF_S_FALL;
            def->grounded = 0;
            def->vh = (int16_t)(lvy * 32);
            def->vx = (int16_t)(away * lvx * 32);
            set_anim(def, NGF_A_FALL, 1);
        } else {
            uint8_t low = (uint8_t)(def->state == NGF_S_CROUCH ||
                                    (def->state == NGF_S_ATTACK && def->move && (def->move->stance & NGF_STANCE_CROUCH)));
            def->state = NGF_S_HIT;
            def->stun_frames = (uint8_t)(hp->hitstun + (def->counter ? 4u : 0u));
            set_anim(def, low ? NGF_A_HIT_LOW : NGF_A_HIT, 1);
            push_away(m, att, def, hp->pushback);
        }
        def->move = 0;
        def->counter = 0;
        event(m, NGF_EV_HIT, att_i, hp->sound, ex, ey);
        if (!def->life) event(m, NGF_EV_KO, (uint8_t)(att_i ^ 1u), 0, ex, ey);
        m->hitstop = hp->hitstop;
    }
}

static void NEOGEO_USER start_move(NGFMatch *m, uint8_t who, uint8_t k);

/* A strike meeting a counter stance: the defender answers with its follow-up. */
static uint8_t NEOGEO_USER try_counter(NGFMatch *m, uint8_t def_i, int16_t x, int16_t y)
{
    NGFFighter *d = &m->f[def_i];
    if (d->state != NGF_S_ATTACK || !d->move || !d->move->follow) return 0;
    if (!(ngf_fighter_frame(d)->flags & NGF_FF_COUNTER)) return 0;
    if ((uint8_t)(d->move->follow - 1u) >= d->def->move_count) return 0;
    if (d->spirit < NGF_SPIRIT_MAX - 8u) d->spirit = (uint8_t)(d->spirit + 8u);
    start_move(m, def_i, (uint8_t)(d->move->follow - 1u));
    d->invuln = 8;
    event(m, NGF_EV_COUNTER, def_i, 0, x, y);
    m->hitstop = 10;
    return 1;
}

static void NEOGEO_USER params_from_move(const NGFMove *mv, HitParams *hp)
{
    hp->damage = mv->damage; hp->guard_damage = mv->guard_damage; hp->chip = mv->chip;
    hp->hitstun = mv->hitstun; hp->blockstun = mv->blockstun; hp->pushback = mv->pushback;
    hp->level = mv->level; hp->on_hit = mv->on_hit; hp->meter = mv->meter;
    hp->hitstop = mv->hitstop; hp->sound = mv->hit_sound;
    hp->launch_vx = mv->launch_vx; hp->launch_vy = mv->launch_vy;
}

/* ===================================================================== */
/*  Projectiles                                                           */
/* ===================================================================== */
static void NEOGEO_USER spawn_projectile(NGFMatch *m, uint8_t who)
{
    NGFFighter *f = &m->f[who];
    const NGFProjectileDef *pd;
    uint8_t i;
    if (!f->move || !f->move->projectile || f->move->projectile > f->def->projectile_count) return;
    pd = &f->def->projectiles[f->move->projectile - 1u];
    for (i = 0; i < NGF_PROJECTILES; i++) {
        NGFProjectile *p = &m->proj[i];
        if (p->active) continue;
        p->def = pd;
        p->owner = who;
        p->facing_left = f->facing_left;
        p->x = f->x + (int32_t)(pd->spawn_x * sign_of(f)) * 256;
        p->h = f->h + (int32_t)pd->spawn_y * 256;
        p->life = pd->life;
        p->frame = 0;
        p->frame_timer = 0;
        p->active = 1;
        event(m, NGF_EV_PROJECTILE, who, i, (int16_t)(p->x >> 8), (int16_t)(p->h >> 8));
        return;
    }
}

static uint8_t NEOGEO_USER has_projectile(const NGFMatch *m, uint8_t who)
{
    uint8_t i;
    for (i = 0; i < NGF_PROJECTILES; i++)
        if (m->proj[i].active && m->proj[i].owner == who) return 1;
    return 0;
}

static void NEOGEO_USER projectiles_step(NGFMatch *m)
{
    uint8_t i, j;
    for (i = 0; i < NGF_PROJECTILES; i++) {
        NGFProjectile *p = &m->proj[i];
        const NGFFighterDef *od;
        const NGFAnim *a;
        int16_t px;
        if (!p->active) continue;
        od = m->f[p->owner].def;
        a = &od->anims[p->def->anim];
        if (++p->frame_timer >= od->frames[a->first + p->frame].time) {
            p->frame_timer = 0;
            p->frame = (uint8_t)((p->frame + 1u) % a->count);
        }
        p->x += (int32_t)(p->facing_left ? -p->def->speed : p->def->speed) * 256;
        px = (int16_t)(p->x >> 8);
        if (!--p->life || px < m->camera_x - 64 || px > m->camera_x + m->screen_w + 64) p->active = 0;
    }
    /* Opposing projectiles cancel each other. */
    for (i = 0; i < NGF_PROJECTILES; i++) for (j = (uint8_t)(i + 1u); j < NGF_PROJECTILES; j++) {
        NGFProjectile *a = &m->proj[i], *b = &m->proj[j];
        if (!a->active || !b->active || a->owner == b->owner) continue;
        if (overlap(world_box(a->x, a->h, a->facing_left, &a->def->box),
                    world_box(b->x, b->h, b->facing_left, &b->def->box))) {
            a->active = b->active = 0;
            event(m, NGF_EV_BLOCK, 2, 0, (int16_t)((a->x + b->x) >> 9), (int16_t)(a->h >> 8));
        }
    }
    for (i = 0; i < NGF_PROJECTILES; i++) {
        NGFProjectile *p = &m->proj[i];
        NGFFighter *t;
        WBox box;
        if (!p->active) continue;
        t = &m->f[p->owner ^ 1u];
        if (!vulnerable(t)) continue;
        box = world_box(p->x, p->h, p->facing_left, &p->def->box);
        if (hits_body(t, box)) {
            HitParams hp;
            hp.damage = p->def->damage; hp.guard_damage = (uint8_t)(p->def->damage / 2u); hp.chip = p->def->chip;
            hp.hitstun = p->def->hitstun; hp.blockstun = p->def->blockstun; hp.pushback = 10;
            hp.level = p->def->level; hp.on_hit = p->def->on_hit; hp.meter = 12; hp.hitstop = 6;
            hp.sound = p->def->hit_sound; hp.launch_vx = 0; hp.launch_vy = 0;
            p->active = 0;
            if (try_counter(m, (uint8_t)(p->owner ^ 1u), (int16_t)((box.l + box.r) / 2), (int16_t)((box.b + box.t) / 2))) continue;
            apply_hit(m, p->owner, &hp, (int16_t)((box.l + box.r) / 2), (int16_t)((box.b + box.t) / 2), 0);
        }
    }
}

/* ===================================================================== */
/*  Fighter control                                                       */
/* ===================================================================== */
static uint8_t NEOGEO_USER throwable(const NGFFighter *o)
{
    if (!o->grounded || o->invuln) return 0;
    return (uint8_t)(o->state == NGF_S_IDLE || o->state == NGF_S_CROUCH || o->state == NGF_S_DASH);
}

static void NEOGEO_USER start_move(NGFMatch *m, uint8_t who, uint8_t k)
{
    NGFFighter *f = &m->f[who], *o = &m->f[who ^ 1u];
    const NGFMove *mv = &f->def->moves[k];
    f->move = mv;
    f->move_index = k;
    f->state = NGF_S_ATTACK;
    f->move_hit = 0;
    f->counter = 1;
    set_anim(f, mv->anim, 1);
    if (f->grounded) f->vx = 0;
    else f->air_used = 1;
    if (mv->kind == NGF_KIND_SUPER) f->power = (uint16_t)(f->power - mv->cost * NGF_POWER_STOCK);
    if (mv->kind == NGF_KIND_GUARD_ATTACK) f->spirit = (uint8_t)(f->spirit - mv->cost);
    ngf_input_consume(&f->input, mv->buttons);
    event(m, NGF_EV_MOVE, who, mv->kind, ngf_fighter_px(f), ngf_fighter_py(f));
    if (mv->kind == NGF_KIND_THROW) {
        if (!o->cpu && (ngf_input_pressed(&o->input, NGF_BTN_C, 4) || ngf_input_pressed(&o->input, NGF_BTN_D, 4))) {
            /* Broken: both pushed apart. */
            f->state = NGF_S_IDLE;
            f->move = 0;
            set_anim(f, NGF_A_IDLE, 1);
            o->push_vx = (int16_t)(o->x >= f->x ? 24 * 64 : -24 * 64);
            f->push_vx = (int16_t)-o->push_vx;
            if (o->spirit < NGF_SPIRIT_MAX - 16u) o->spirit = (uint8_t)(o->spirit + 16u);
            event(m, NGF_EV_TECH, (uint8_t)(who ^ 1u), 0, ngf_fighter_px(o), 40);
            return;
        }
        o->state = NGF_S_THROWN;
        o->move = 0;
        o->vx = 0;
        o->x = f->x + (int32_t)(sign_of(f) * 26) * 256;
        o->facing_left = (uint8_t)!f->facing_left;
        set_anim(o, NGF_A_THROWN, 1);
        event(m, NGF_EV_THROW, who, 0, ngf_fighter_px(o), 40);
    }
    enter_frame(m, who);
}

static uint8_t NEOGEO_USER try_moves(NGFMatch *m, uint8_t who, uint8_t cancel)
{
    NGFFighter *f = &m->f[who], *o = &m->f[who ^ 1u];
    uint8_t dir = ngf_input_dir(&f->input, 0, f->facing_left);
    uint8_t stance = !f->grounded ? NGF_STANCE_AIR : (dir <= 3u ? NGF_STANCE_CROUCH : NGF_STANCE_STAND);
    uint16_t dist = ngf_distance(m);
    uint8_t k;
    for (k = 0; k < f->def->move_count; k++) {
        const NGFMove *mv = &f->def->moves[k];
        if (!(mv->stance & stance)) continue;
        if ((mv->stance & NGF_STANCE_CLOSE) && dist > f->def->close_range) continue;
        if ((mv->stance & NGF_STANCE_FAR) && dist <= f->def->close_range) continue;
        if (f->state == NGF_S_BLOCK) {
            if (mv->kind != NGF_KIND_GUARD_ATTACK || f->spirit < mv->cost) continue;
        } else if (mv->kind == NGF_KIND_GUARD_ATTACK) {
            continue;
        }
        if (cancel) {
            uint8_t ok = 0;
            if ((cancel & NGF_CANCEL_CHAIN) && mv->kind == NGF_KIND_NORMAL && k > f->move_index) ok = 1;
            if ((cancel & NGF_CANCEL_SPECIAL) && mv->kind == NGF_KIND_SPECIAL) ok = 1;
            if ((cancel & (NGF_CANCEL_SPECIAL | NGF_CANCEL_SUPER)) && mv->kind == NGF_KIND_SUPER) ok = 1;
            if (!ok) continue;
        }
        if (stance == NGF_STANCE_AIR && f->air_used) continue;
        if (mv->kind == NGF_KIND_SUPER && f->power < (uint16_t)(mv->cost * NGF_POWER_STOCK)) continue;
        if (mv->projectile && has_projectile(m, who)) continue;
        if (!ngf_input_pressed(&f->input, mv->buttons,
                               (uint8_t)(mv->kind <= NGF_KIND_COMMAND || mv->kind == NGF_KIND_THROW ? 2u : 5u))) continue;
        if (mv->motion && !ngf_input_motion(&f->input, mv->motion, f->facing_left, mv->window)) continue;
        if (mv->kind == NGF_KIND_THROW && (!throwable(o) || dist > mv->range)) continue;
        start_move(m, who, k);
        return 1;
    }
    return 0;
}

static void NEOGEO_USER free_move(NGFMatch *m, uint8_t who, uint8_t controls)
{
    NGFFighter *f = &m->f[who], *o = &m->f[who ^ 1u];
    const NGFFighterDef *d = f->def;
    uint8_t dir = controls ? ngf_input_dir(&f->input, 0, f->facing_left) : 5u;
    if (controls && try_moves(m, who, 0)) return;
    if (dir >= 7u) {
        f->state = NGF_S_JUMP;
        f->grounded = 0;
        f->air_used = 0;
        f->vh = d->jump_vy;
        f->vx = (int16_t)((dir == 9u ? d->jump_vx : dir == 7u ? -d->jump_vx : 0) * sign_of(f));
        set_anim(f, NGF_A_JUMP, 1);
        return;
    }
    if (controls && ngf_input_motion(&f->input, NGF_MOTION_DASH, f->facing_left, 0)) {
        f->state = NGF_S_DASH;
        f->state_timer = 0;
        set_anim(f, NGF_A_DASH, 1);
        return;
    }
    if (controls && ngf_input_motion(&f->input, NGF_MOTION_BACKDASH, f->facing_left, 0)) {
        f->state = NGF_S_BACKDASH;
        f->grounded = 0;
        f->vh = d->backdash_vy;
        f->vx = (int16_t)(-d->backdash_vx * sign_of(f));
        f->invuln = 6;
        set_anim(f, NGF_A_BACKDASH, 1);
        return;
    }
    f->block_low = (uint8_t)(dir == 1u);
    if (dir <= 3u) {
        f->state = NGF_S_CROUCH;
        f->vx = 0;
        set_anim(f, dir == 1u && o->state == NGF_S_ATTACK ? NGF_A_GUARD_LOW : NGF_A_CROUCH, 0);
        return;
    }
    f->state = NGF_S_IDLE;
    if (dir == 6u) {
        f->vx = (int16_t)(d->walk_fwd * sign_of(f));
        set_anim(f, NGF_A_WALK_F, 0);
        if (!(m->frame & 7u) && f->spirit < NGF_SPIRIT_MAX) f->spirit++;
    } else if (dir == 4u) {
        uint8_t threat = (uint8_t)((o->state == NGF_S_ATTACK && ngf_distance(m) < 150u) || has_projectile(m, (uint8_t)(who ^ 1u)));
        f->vx = threat ? 0 : (int16_t)(-d->walk_back * sign_of(f));
        set_anim(f, threat ? NGF_A_GUARD : NGF_A_WALK_B, 0);
    } else {
        f->vx = 0;
        set_anim(f, NGF_A_IDLE, 0);
    }
}

static void NEOGEO_USER land(NGFMatch *m, uint8_t who)
{
    NGFFighter *f = &m->f[who];
    f->grounded = 1;
    f->h = 0;
    f->vh = 0;
    f->vx = 0;
    event(m, NGF_EV_LAND, who, f->state, ngf_fighter_px(f), 0);
    if (f->state == NGF_S_FALL) {
        f->state = f->life ? NGF_S_DOWN : NGF_S_KO;
        f->state_timer = DOWN_FRAMES;
        set_anim(f, NGF_A_DOWN, 1);
    } else if (f->state == NGF_S_ATTACK && f->move &&
               (uint8_t)(f->frame + 1u) < f->def->anims[f->anim].count) {
        f->frame++;            /* land into the move's recovery */
        f->frame_timer = 0;
    } else if (f->state != NGF_S_THROWN) {
        f->state = NGF_S_IDLE;
        f->move = 0;
        set_anim(f, NGF_A_IDLE, 1);
    }
}

static void NEOGEO_USER fighter_step(NGFMatch *m, uint8_t who, uint8_t controls)
{
    NGFFighter *f = &m->f[who], *o = &m->f[who ^ 1u];
    const NGFFrame *fr;
    if (f->invuln) f->invuln--;
    if (f->state != NGF_S_BLOCK && !(m->frame & (GUARD_REGEN - 1u)) && f->guard < f->def->guard_max) f->guard++;
    switch (f->state) {
    case NGF_S_IDLE:
    case NGF_S_CROUCH:
        f->combo = 0;
        f->combo_damage = 0;
        free_move(m, who, controls);
        if (f->state == NGF_S_IDLE || f->state == NGF_S_CROUCH) anim_step(m, who);
        break;
    case NGF_S_JUMP:
        if (controls) try_moves(m, who, 0);
        anim_step(m, who);
        break;
    case NGF_S_DASH:
        f->state_timer++;
        f->vx = (int16_t)(f->def->dash * sign_of(f));
        if (!(m->frame & 3u) && f->spirit < NGF_SPIRIT_MAX) f->spirit++;
        anim_step(m, who);
        if (controls && try_moves(m, who, 0)) break;
        if (f->state_timer >= 10u && (!controls || ngf_input_dir(&f->input, 0, f->facing_left) != 6u)) {
            f->state = NGF_S_IDLE;
            f->vx = 0;
            set_anim(f, NGF_A_IDLE, 1);
        }
        break;
    case NGF_S_BACKDASH:
        anim_step(m, who);
        break;
    case NGF_S_ATTACK: {
        uint8_t done;
        if (f->move_hit && f->move && f->move->cancel && controls && try_moves(m, who, f->move->cancel)) break;
        done = anim_step(m, who);
        if (f->move && f->move->kind == NGF_KIND_THROW && done) {
            NGFFighter *t = o;
            if (t->state == NGF_S_THROWN) {
                HitParams hp;
                params_from_move(f->move, &hp);
                hp.level = NGF_LV_UNBLOCKABLE;
                hp.on_hit |= NGF_ON_KNOCKDOWN;
                t->state = NGF_S_IDLE;
                apply_hit(m, who, &hp, ngf_fighter_px(t), 40, 1);
            }
        }
        if (done) {
            f->move = 0;
            if (f->grounded) {
                f->state = NGF_S_IDLE;
                set_anim(f, NGF_A_IDLE, 1);
            } else {
                f->state = NGF_S_JUMP;
                set_anim(f, NGF_A_JUMP, 0);
                f->frame = (uint8_t)(f->def->anims[NGF_A_JUMP].count - 1u);
            }
        }
        break;
    }
    case NGF_S_BLOCK:
        if (controls && try_moves(m, who, 0)) break;
        if (!f->stun_frames || !--f->stun_frames) {
            f->state = NGF_S_IDLE;
            set_anim(f, NGF_A_IDLE, 1);
        }
        break;
    case NGF_S_HIT:
        anim_step(m, who);
        if (!f->stun_frames || !--f->stun_frames) {
            f->state = NGF_S_IDLE;
            set_anim(f, NGF_A_IDLE, 1);
        }
        break;
    case NGF_S_FALL:
    case NGF_S_THROWN:
    case NGF_S_KO:
        anim_step(m, who);
        break;
    case NGF_S_DOWN:
        anim_step(m, who);
        if (!--f->state_timer) {
            f->state = NGF_S_GETUP;
            set_anim(f, NGF_A_GETUP, 1);
        }
        break;
    case NGF_S_GETUP:
        if (anim_step(m, who)) {
            f->state = NGF_S_IDLE;
            f->invuln = 4;
            set_anim(f, NGF_A_IDLE, 1);
        }
        break;
    case NGF_S_DIZZY:
        anim_step(m, who);
        if (!--f->state_timer) {
            f->state = NGF_S_IDLE;
            set_anim(f, NGF_A_IDLE, 1);
        }
        break;
    default:
        anim_step(m, who);
        break;
    }
    /* Motion. */
    fr = ngf_fighter_frame(f);
    if (f->grounded) {
        int32_t dx = f->vx;
        if (f->state == NGF_S_ATTACK || f->state == NGF_S_GETUP) dx = (int32_t)fr->dx * 32 * sign_of(f);
        if (f->state == NGF_S_HIT || f->state == NGF_S_BLOCK || f->state == NGF_S_DOWN) dx = 0;
        f->x += dx + f->push_vx;
    } else {
        f->vh = (int16_t)(f->vh - f->def->gravity);
        f->h += f->vh;
        f->x += f->vx + f->push_vx;
        if (f->h <= 0 && f->vh < 0 && f->state != NGF_S_THROWN) land(m, who);
    }
    f->push_vx = (int16_t)(f->push_vx - f->push_vx / 4);
    if (f->push_vx > -16 && f->push_vx < 16) f->push_vx = 0;
}

/* Push boxes, stage and screen bounds, camera. */
static void NEOGEO_USER resolve_space(NGFMatch *m)
{
    NGFFighter *a = &m->f[0], *b = &m->f[1];
    int32_t lo, hi, mid;
    uint8_t i;
    if (a->state != NGF_S_THROWN && b->state != NGF_S_THROWN) {
        int32_t wa = (int32_t)a->def->push_w << 8, wb = (int32_t)b->def->push_w << 8;
        int32_t ha = (int32_t)(a->state == NGF_S_CROUCH ? a->def->crouch_h : a->def->push_h) << 8;
        int32_t hb = (int32_t)(b->state == NGF_S_CROUCH ? b->def->crouch_h : b->def->push_h) << 8;
        int32_t gap = (a->x > b->x ? a->x - b->x : b->x - a->x);
        if (gap < wa + wb && a->h < b->h + hb && b->h < a->h + ha) {
            int32_t push = (wa + wb - gap) / 2 + 1;
            if (a->x < b->x || (a->x == b->x && a->facing_left == 0)) { a->x -= push; b->x += push; }
            else { a->x += push; b->x -= push; }
        }
    }
    mid = (a->x + b->x) / 2 / 256;
    m->camera_x = (int16_t)(mid - m->screen_w / 2);
    if (m->camera_x < 0) m->camera_x = 0;
    if (m->camera_x > m->stage_w - m->screen_w) m->camera_x = (int16_t)(m->stage_w - m->screen_w);
    lo = (int32_t)(m->camera_x + FIGHT_MARGIN) << 8;
    hi = (int32_t)(m->camera_x + m->screen_w - FIGHT_MARGIN) << 8;
    for (i = 0; i < 2u; i++) {
        NGFFighter *f = &m->f[i];
        if (f->x < lo) f->x = lo;
        if (f->x > hi) f->x = hi;
    }
    /* Keep the push boxes apart against a wall too. */
    if (a->state != NGF_S_THROWN && b->state != NGF_S_THROWN && a->grounded && b->grounded) {
        int32_t need = (int32_t)(a->def->push_w + b->def->push_w) << 8;
        if (a->x <= b->x && b->x - a->x < need) {
            if (a->x <= lo) b->x = a->x + need; else a->x = b->x - need;
        } else if (b->x < a->x && a->x - b->x < need) {
            if (b->x <= lo) a->x = b->x + need; else b->x = a->x - need;
        }
    }
}

static void NEOGEO_USER resolve_attacks(NGFMatch *m)
{
    uint8_t hit[2] = {0, 0}, i;
    WBox box[2] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
    for (i = 0; i < 2u; i++) {
        NGFFighter *f = &m->f[i], *o = &m->f[i ^ 1u];
        const NGFFrame *fr = ngf_fighter_frame(f);
        if (f->state != NGF_S_ATTACK || !f->move || f->move->kind == NGF_KIND_THROW) continue;
        if (!(fr->flags & NGF_FF_ACTIVE) || f->move_hit || !f->move->hit.w) continue;
        if (!vulnerable(o)) continue;
        box[i] = world_box(f->x, f->h, f->facing_left, &f->move->hit);
        hit[i] = hits_body(o, box[i]);
    }
    for (i = 0; i < 2u; i++) {
        if (!hit[i]) continue;
        {
            NGFFighter *f = &m->f[i];
            HitParams hp;
            const NGFMove *mv = f->move;
            if (!mv) continue;
            params_from_move(mv, &hp);
            f->move_hit = 1;
            if (try_counter(m, (uint8_t)(i ^ 1u), (int16_t)(f->facing_left ? box[i].l : box[i].r),
                            (int16_t)((box[i].b + box[i].t) / 2))) continue;
            apply_hit(m, i, &hp, (int16_t)(m->f[i].facing_left ? box[i].l : box[i].r),
                      (int16_t)((box[i].b + box[i].t) / 2), 1);
        }
    }
}

static void NEOGEO_USER face_each_other(NGFMatch *m)
{
    uint8_t i;
    for (i = 0; i < 2u; i++) {
        NGFFighter *f = &m->f[i], *o = &m->f[i ^ 1u];
        if (!f->grounded) continue;
        if (f->state != NGF_S_IDLE && f->state != NGF_S_CROUCH && f->state != NGF_S_INTRO) continue;
        if (o->x != f->x) f->facing_left = (uint8_t)(o->x < f->x);
    }
}

/* ===================================================================== */
/*  CPU                                                                   */
/* ===================================================================== */
static void NEOGEO_USER cpu_push(NGFCpu *c, uint8_t raw)
{
    if (c->len < sizeof(c->seq)) c->seq[c->len++] = raw;
}

static void NEOGEO_USER cpu_queue_move(NGFMatch *m, uint8_t who, uint8_t k)
{
    static const uint8_t pads[][6] = {
        {0}, {2, 3, 6}, {2, 1, 4}, {6, 2, 3}, {4, 2, 1}, {4, 1, 2, 3, 6}, {6, 3, 2, 1, 4},
        {2, 3, 6, 2, 3, 6}, {2, 1, 4, 2, 1, 4},
    };
    static const uint8_t lens[] = {0, 3, 3, 3, 3, 5, 5, 6, 6};
    NGFFighter *f = &m->f[who];
    NGFCpu *c = &m->cpu[who];
    const NGFMove *mv;
    uint8_t fl = f->facing_left, i, last = 5u;
    if (k >= f->def->move_count) return;
    mv = &f->def->moves[k];
    c->len = c->pos = 0;
    if (mv->motion >= NGF_MOTION_QCF && mv->motion <= NGF_MOTION_DQCB) {
        for (i = 0; i < lens[mv->motion]; i++) {
            last = pads[mv->motion][i];
            cpu_push(c, ngf_raw_from_numpad(last, fl));
            cpu_push(c, ngf_raw_from_numpad(last, fl));
        }
    } else if (mv->motion == NGF_MOTION_CHARGE_BF) {
        c->hold = ngf_raw_from_numpad(4, fl);
        c->wait = (uint8_t)(CHARGE_FRAMES + 2u);
        last = 6u;
    } else if (mv->motion == NGF_MOTION_CHARGE_DU) {
        c->hold = ngf_raw_from_numpad(1, fl);
        c->wait = (uint8_t)(CHARGE_FRAMES + 2u);
        last = 9u;
    } else if (mv->motion == NGF_MOTION_FWD) last = 6u;
    else if (mv->motion == NGF_MOTION_BACK) last = 4u;
    else if (mv->motion == NGF_MOTION_DOWN_FWD) last = 3u;
    else if (mv->stance == NGF_STANCE_CROUCH) {
        last = 2u;
        cpu_push(c, ngf_raw_from_numpad(2, fl));
    }
    cpu_push(c, (uint8_t)(ngf_raw_from_numpad(last, fl) | mv->buttons));
    cpu_push(c, ngf_raw_from_numpad(last == 2u ? 2u : 5u, fl));
}

uint8_t NEOGEO_USER ngf_cpu_input(NGFMatch *m, uint8_t who)
{
    NGFCpu *c = &m->cpu[who];
    NGFFighter *f = &m->f[who], *o = &m->f[who ^ 1u];
    const NGFCpuProfile *cpu = &f->def->cpu_profile;
    uint16_t dist, r;
    uint8_t lv = c->level, fl = f->facing_left, threat;
    if (c->wait) { c->wait--; return c->hold; }
    if (c->pos < c->len) return c->seq[c->pos++];
    c->len = c->pos = 0;
    c->hold = 0;
    f->block_low = 0;
    if (m->state != NGF_M_FIGHT) return 0;
    dist = ngf_distance(m);
    r = (uint16_t)(rnd(m) % 100u);
    threat = (uint8_t)((o->state == NGF_S_ATTACK && dist < (uint16_t)(o->move ? o->move->range + 70u : 120u)) ||
                       has_projectile(m, (uint8_t)(who ^ 1u)));
    if (f->state == NGF_S_BLOCK || f->state == NGF_S_HIT) { c->hold = ngf_raw_from_numpad(f->block_low ? 1u : 4u, fl); return c->hold; }
    if (f->state != NGF_S_IDLE && f->state != NGF_S_CROUCH && f->state != NGF_S_JUMP && f->state != NGF_S_DASH) return 0;
    if (f->state == NGF_S_JUMP) {
        if (!f->air_used && f->vh < 0 && dist < 70u && cpu->rush != 0xFFu && r < 60u) {
            const NGFMove *mv;
            uint8_t k;
            for (k = 0; k < f->def->move_count; k++) {
                mv = &f->def->moves[k];
                if ((mv->stance & NGF_STANCE_AIR) && mv->kind == NGF_KIND_NORMAL) {
                    cpu_push(c, mv->buttons);
                    cpu_push(c, 0);
                    break;
                }
            }
        }
        return 0;
    }
    if (threat && r < (uint16_t)(30u + lv * 7u)) {
        uint8_t low = (uint8_t)(o->move && o->move->level == NGF_LV_LOW);
        f->block_low = low;
        c->hold = ngf_raw_from_numpad(low ? 1u : 4u, fl);
        c->wait = (uint8_t)(8u + (rnd(m) & 7u));
        return c->hold;
    }
    if (!o->grounded && o->state == NGF_S_JUMP && dist < 110u && cpu->anti_air != 0xFFu && r < (uint16_t)(20u + lv * 8u)) {
        cpu_queue_move(m, who, cpu->anti_air);
        return ngf_cpu_input(m, who);
    }
    if (o->state == NGF_S_ATTACK && o->move && dist < (uint16_t)(cpu->range + 30u) && cpu->rush != 0xFFu &&
        o->frame + 1u >= o->def->anims[o->anim].count && r < (uint16_t)(lv * 9u)) {
        cpu_queue_move(m, who, cpu->rush);
        return ngf_cpu_input(m, who);
    }
    if (f->power >= NGF_POWER_STOCK && cpu->super_move != 0xFFu && dist < 90u && r < (uint16_t)(4u + lv)) {
        cpu_queue_move(m, who, cpu->super_move);
        return ngf_cpu_input(m, who);
    }
    if (dist > (uint16_t)(cpu->range + 30u)) {
        if (cpu->special != 0xFFu && dist > 140u && r < (uint16_t)(6u + lv * 2u)) {
            cpu_queue_move(m, who, cpu->special);
            return ngf_cpu_input(m, who);
        }
        if (r < (uint16_t)(cpu->jump / 16u) && dist < 170u) {
            cpu_push(c, ngf_raw_from_numpad(9, fl));
            cpu_push(c, ngf_raw_from_numpad(9, fl));
            return ngf_cpu_input(m, who);
        }
        c->hold = ngf_raw_from_numpad(6, fl);
        c->wait = (uint8_t)(6u + (rnd(m) % 18u));
        return c->hold;
    }
    if (dist < 44u && cpu->throw_move != 0xFFu && throwable(o) && r < 18u) {
        cpu_queue_move(m, who, cpu->throw_move);
        return ngf_cpu_input(m, who);
    }
    if (r < (uint16_t)(cpu->aggression / 8u + lv * 3u)) {
        uint8_t pick = (uint8_t)(rnd(m) % 4u);
        uint8_t k = pick == 0u ? cpu->sweep : pick == 1u ? cpu->special : pick == 2u ? cpu->rush : cpu->poke;
        if (k == 0xFFu) k = cpu->poke;
        cpu_queue_move(m, who, k);
        c->wait = 0;
        return ngf_cpu_input(m, who);
    }
    if (r > 85u) {
        c->hold = ngf_raw_from_numpad(4, fl);
        c->wait = (uint8_t)(4u + (rnd(m) & 7u));
        return c->hold;
    }
    c->hold = 0;
    c->wait = (uint8_t)(4u + (rnd(m) % (uint16_t)(26u - 2u * (lv > 9u ? 9u : lv))));
    return 0;
}

void NEOGEO_USER ngf_set_cpu(NGFMatch *m, uint8_t who, uint8_t level)
{
    NGFCpu *c = &m->cpu[who];
    c->level = level;
    c->len = c->pos = c->wait = 0;
    c->hold = 0;
    m->f[who].cpu = (uint8_t)(level != 0u);
}

/* ===================================================================== */
/*  Match                                                                 */
/* ===================================================================== */
static void NEOGEO_USER fighter_reset(NGFMatch *m, uint8_t who)
{
    NGFFighter *f = &m->f[who];
    const NGFFighterDef *d = f->def;
    uint16_t power = f->power;
    uint8_t wins = f->wins, cpu = f->cpu, pal = f->palette;
    uint8_t *p = (uint8_t *)f;
    uint16_t i;
    for (i = 0; i < sizeof(*f); i++) p[i] = 0;
    f->def = d;
    f->power = power;
    f->wins = wins;
    f->cpu = cpu;
    f->palette = pal;
    f->life = d->life;
    f->guard = d->guard_max;
    f->grounded = 1;
    f->x = (int32_t)(m->stage_w / 2 + (who ? 72 : -72)) << 8;
    f->facing_left = who;
    f->state = NGF_S_INTRO;
    set_anim(f, NGF_A_INTRO, 1);
}

void NEOGEO_USER ngf_match_init(NGFMatch *m, const NGFFighterDef *p1, const NGFFighterDef *p2,
                                int16_t stage_w, uint16_t seed)
{
    uint8_t *p = (uint8_t *)m;
    uint16_t i;
    for (i = 0; i < sizeof(*m); i++) p[i] = 0;
    m->f[0].def = p1;
    m->f[1].def = p2;
    m->f[1].palette = (uint8_t)(p1 == p2);
    m->stage_w = stage_w;
    m->screen_w = 320;
    m->floor_y = 200;
    m->rng = seed ? seed : 0x2F6Bu;
    m->rounds_to_win = 2;
    m->winner = 0xFFu;
    m->round = 0;
    ngf_round_start(m);
}

void NEOGEO_USER ngf_round_start(NGFMatch *m)
{
    uint8_t i;
    m->round++;
    fighter_reset(m, 0);
    fighter_reset(m, 1);
    for (i = 0; i < NGF_PROJECTILES; i++) m->proj[i].active = 0;
    m->state = NGF_M_INTRO;
    m->state_timer = 0;
    m->clock = (uint16_t)(NGF_ROUND_SECONDS * NGF_SECOND_FRAMES);
    m->seconds = NGF_ROUND_SECONDS;
    m->hitstop = 0;
    m->round_winner = 0xFFu;
    m->camera_x = (int16_t)((m->stage_w - m->screen_w) / 2);
    event(m, NGF_EV_ROUND, 0, m->round, 0, 0);
}

static void NEOGEO_USER end_round(NGFMatch *m)
{
    NGFFighter *a = &m->f[0], *b = &m->f[1];
    uint8_t w = 2;
    if (m->state == NGF_M_KO) {
        if (a->life && !b->life) w = 0;
        else if (b->life && !a->life) w = 1;
    } else {
        uint32_t ra = (uint32_t)a->life * b->def->life, rb = (uint32_t)b->life * a->def->life;
        if (ra > rb) w = 0;
        else if (rb > ra) w = 1;
    }
    m->round_winner = w;
    if (w != 1u) a->wins++;
    if (w != 0u) b->wins++;
    if (w < 2u) {
        NGFFighter *win = &m->f[w];
        if (win->state != NGF_S_KO && win->life) { win->state = NGF_S_WIN; win->move = 0; set_anim(win, NGF_A_WIN, 1); }
    }
    {
        uint8_t i;
        for (i = 0; i < 2u; i++) {
            NGFFighter *f = &m->f[i];
            if (i != w && f->life && f->grounded && f->state != NGF_S_WIN) {
                f->state = NGF_S_LOSE; f->move = 0; set_anim(f, NGF_A_LOSE, 1);
            }
        }
    }
    event(m, NGF_EV_WIN, w, m->round, 0, 0);
    m->state = NGF_M_ROUND_END;
    m->state_timer = 0;
}

void NEOGEO_USER ngf_match_step(NGFMatch *m, uint8_t raw1, uint8_t raw2)
{
    uint8_t raw[2], i, controls;
    raw[0] = raw1;
    raw[1] = raw2;
    m->frame++;
    for (i = 0; i < 2u; i++) {
        if (m->cpu[i].level) raw[i] = ngf_cpu_input(m, i);
        ngf_input_push(&m->f[i].input, raw[i], m->f[i].facing_left);
    }
    if (m->hitstop) { m->hitstop--; return; }
    m->state_timer++;
    switch (m->state) {
    case NGF_M_INTRO:
        if (m->state_timer == 60u) event(m, NGF_EV_FIGHT, 0, 0, 0, 0);
        if (m->state_timer >= INTRO_FRAMES) {
            m->state = NGF_M_FIGHT;
            m->state_timer = 0;
            for (i = 0; i < 2u; i++) {
                m->f[i].state = NGF_S_IDLE;
                set_anim(&m->f[i], NGF_A_IDLE, 1);
            }
        }
        break;
    case NGF_M_FIGHT:
        if (m->clock) {
            m->clock--;
            m->seconds = (uint8_t)((m->clock + NGF_SECOND_FRAMES - 1u) / NGF_SECOND_FRAMES);
        }
        break;
    default:
        break;
    }
    controls = (uint8_t)(m->state == NGF_M_FIGHT);
    face_each_other(m);
    for (i = 0; i < 2u; i++) fighter_step(m, i, controls);
    projectiles_step(m);
    resolve_space(m);
    if (m->state == NGF_M_FIGHT || m->state == NGF_M_KO) resolve_attacks(m);
    switch (m->state) {
    case NGF_M_FIGHT:
        if (!m->f[0].life || !m->f[1].life) {
            m->state = NGF_M_KO;
            m->state_timer = 0;
        } else if (!m->clock) {
            m->state = NGF_M_TIME_UP;
            m->state_timer = 0;
            event(m, NGF_EV_TIME_UP, 0, 0, 0, 0);
        }
        break;
    case NGF_M_KO:
        if (m->state_timer >= KO_FRAMES) end_round(m);
        break;
    case NGF_M_TIME_UP:
        if (m->state_timer >= TIMEUP_FRAMES) end_round(m);
        break;
    case NGF_M_ROUND_END:
        if (m->state_timer >= END_FRAMES) {
            uint8_t a = (uint8_t)(m->f[0].wins >= m->rounds_to_win), b = (uint8_t)(m->f[1].wins >= m->rounds_to_win);
            if (a || b || m->round >= 9u) {
                m->state = NGF_M_OVER;
                m->state_timer = 0;
                m->winner = (uint8_t)(a && b ? 2u : a ? 0u : b ? 1u : 2u);
            } else {
                ngf_round_start(m);
            }
        }
        break;
    default:
        break;
    }
}

uint32_t NEOGEO_USER ngf_match_hash(const NGFMatch *m)
{
    uint32_t h = 2166136261u;
    uint8_t i;
#define MIX(v) (h = (h ^ (uint32_t)(v)) * 16777619u)
    for (i = 0; i < 2u; i++) {
        const NGFFighter *f = &m->f[i];
        MIX(f->x); MIX(f->h); MIX((uint16_t)f->vx); MIX((uint16_t)f->vh); MIX(f->state);
        MIX(f->anim); MIX(f->frame); MIX(f->life); MIX(f->power); MIX(f->guard); MIX(f->spirit);
        MIX(f->facing_left); MIX(f->wins);
    }
    for (i = 0; i < NGF_PROJECTILES; i++) if (m->proj[i].active) { MIX(m->proj[i].x); MIX(m->proj[i].owner); }
    MIX(m->clock); MIX(m->rng); MIX(m->round); MIX(m->state);
#undef MIX
    return h;
}
