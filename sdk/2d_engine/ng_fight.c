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
#define HOP_FRAMES      4u     /* up released this soon after take-off: a short hop */
#define MOTION_LATE     7u     /* frames a motion's last step may precede its button */
#define NGF_CANCEL_KARA 0x08u  /* internal: a second button joins the normal just started */

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
    in->press_age[0] = in->press_age[1] = in->press_age[2] = in->press_age[3] = 255u;
    in->release_age[0] = in->release_age[1] = in->release_age[2] = in->release_age[3] = 255u;
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

static void NEOGEO_USER input_push(NGFInput *in, uint8_t raw, uint8_t facing_left, uint8_t paused)
{
    uint8_t prev = in->raw[in->head];
    uint8_t dir, i;
    for (i = 0; i < 4u; i++) {
        uint8_t bit = (uint8_t)(NGF_BTN_A << i);
        if (!paused && in->press_age[i] < 255u) in->press_age[i]++;
        if (!paused && in->release_age[i] < 255u) in->release_age[i]++;
        if ((raw & bit) && !(prev & bit)) { in->pending |= bit; in->press_age[i] = 0; }
        if ((prev & bit) && !(raw & bit)) { in->release_pending |= bit; in->release_age[i] = 0; }
    }
    if (in->motion_frames < NGF_INPUT_FRAMES) in->motion_frames++;
    in->head = (uint8_t)((in->head + 1u) & (NGF_INPUT_FRAMES - 1u));
    in->raw[in->head] = raw;
    in->press[in->head] = (uint8_t)(raw & (uint8_t)~prev & NGF_BTN_ALL);
    in->released = (uint8_t)(prev & (uint8_t)~raw & NGF_BTN_ALL);
    dir = numpad(raw, facing_left);
    charge((uint8_t)((BACKS >> dir) & 1u), &in->charge_back, &in->back_release, &in->back_age);
    charge((uint8_t)((DOWNS >> dir) & 1u), &in->charge_down, &in->down_release, &in->down_age);
}

void NEOGEO_USER ngf_input_push(NGFInput *in, uint8_t raw, uint8_t facing_left)
{
    input_push(in, raw, facing_left, 0);
}

uint8_t NEOGEO_USER ngf_input_dir(const NGFInput *in, uint8_t ago, uint8_t facing_left)
{
    return numpad(in->raw[(uint8_t)(in->head - ago) & (NGF_INPUT_FRAMES - 1u)], facing_left);
}

static uint8_t NEOGEO_USER buffered_buttons(const NGFInput *in, uint8_t mask, uint8_t buffer, uint8_t release)
{
    uint8_t i, pending = release ? in->release_pending : (uint8_t)(in->pending | in->kara);
    const uint8_t *age = release ? in->release_age : in->press_age;
    if (!mask || !buffer || (pending & mask) != mask) return 0;
    if (buffer > NGF_INPUT_FRAMES) buffer = NGF_INPUT_FRAMES;
    for (i = 0; i < 4u; i++)
        if ((mask & (uint8_t)(NGF_BTN_A << i)) && age[i] >= buffer) return 0;
    return 1;
}

uint8_t NEOGEO_USER ngf_input_pressed(const NGFInput *in, uint8_t mask, uint8_t buffer)
{
    return buffered_buttons(in, mask, buffer, 0);
}

uint8_t NEOGEO_USER ngf_input_released(const NGFInput *in, uint8_t mask, uint8_t buffer)
{
    return buffered_buttons(in, mask, buffer, 1);
}

void NEOGEO_USER ngf_input_consume(NGFInput *in, uint8_t mask)
{
    uint8_t i;
    for (i = 0; i < NGF_INPUT_FRAMES; i++) in->press[i] &= (uint8_t)~mask;
    in->pending &= (uint8_t)~mask;
    in->release_pending &= (uint8_t)~mask;
    in->released &= (uint8_t)~mask;
    in->kara &= (uint8_t)~mask;
    in->back_release = in->down_release = 0;
}

/* Ordered steps per motion, each a set of numpad directions. */
static const uint16_t motion_steps[NGF_MOTION_COUNT][6] = {
    {0},
    {DIR(2), DIR(3), DIR(6)},
    {DIR(2), DIR(1), DIR(4)},
    {DIR(6), DIR(2) | DIR(1), DIR(3) | DIR(6)},
    {DIR(4), DIR(2) | DIR(3), DIR(1) | DIR(4)},
    {DIR(4) | DIR(1), DIR(2) | DIR(1) | DIR(3), DIR(6) | DIR(3)},
    {DIR(6) | DIR(3), DIR(2) | DIR(3) | DIR(1), DIR(4) | DIR(1)},
    {DIR(2), DIR(6) | DIR(3), DIR(2), DIR(3), DIR(6) | DIR(3)},
    {DIR(2), DIR(4) | DIR(1), DIR(2), DIR(1), DIR(4) | DIR(1)},
};
static const uint8_t motion_len[NGF_MOTION_COUNT] = {0, 3, 3, 3, 3, 3, 3, 5, 5};
static const uint8_t motion_window[NGF_MOTION_COUNT] = {0, 28, 28, 28, 28, 36, 36, 52, 52, 0, 0, 40, 60, 14, 14, 0, 0, 0};

/* Clockwise octants: R, DR, D, DL, L, UL, U, UR. Neutral is ignored.
 * Cardinal-only turns are lenient, but reversals/opposite jumps restart the
 * path. Four ordered cardinal sectors are a 270-degree execution; eight
 * require a second traversal. The last diagonal has one-octant leniency,
 * allowing a grounded command before an up-diagonal initiates a jump. A
 * button may follow within MOTION_LATE frames. */
static uint8_t NEOGEO_USER ordered_rotation(const NGFInput *in, uint8_t facing_left,
    uint8_t window, uint8_t backwards, uint8_t required)
{
    static const uint8_t octant[10] = {255u, 3u, 2u, 1u, 4u, 255u, 0u, 5u, 6u, 7u};
    uint8_t age = window, previous = 255u, count = 0u, travel = 0u, last_age = 255u;
    while (age) {
        uint8_t current, delta;
        age--;
        current = octant[ngf_input_dir(in, age, facing_left)];
        if (current == 255u) continue;
        if (previous == current) { last_age = age; continue; }
        delta = previous == 255u ? 0u : (uint8_t)((backwards ? previous - current : current - previous) & 7u);
        if (previous == 255u || delta > 2u || age + 6u < last_age) {
            count = (uint8_t)((current & 1u) == 0u);
            travel = 0u;
        } else {
            travel = (uint8_t)(travel + delta);
            if (!(current & 1u) && count < 8u) count++;
        }
        previous = current;
        last_age = age;
        if ((uint8_t)(count + (current & 1u)) >= required &&
            travel >= (required == 8u ? 13u : 5u) && age <= MOTION_LATE) return 1u;
    }
    return 0u;
}

uint8_t NEOGEO_USER ngf_input_motion(const NGFInput *in, uint8_t motion, uint8_t facing_left, uint8_t window)
{
    uint8_t now = ngf_input_dir(in, 0, facing_left);
    uint8_t ago, step;
    if (motion >= NGF_MOTION_COUNT) return 0;
    if (motion > NGF_MOTION_DQCB && motion < NGF_MOTION_CHARGE_BF) return 0;
    if (!window) window = motion_window[motion];
    if (window > NGF_INPUT_FRAMES) window = NGF_INPUT_FRAMES;
    if (window > in->motion_frames) window = in->motion_frames;
    switch (motion) {
    case NGF_MOTION_NONE: return 1;
    case NGF_MOTION_FWD:
        return (uint8_t)(now == 6u || (now == 5u && (ngf_input_dir(in, 1, facing_left) == 6u ||
                                                     ngf_input_dir(in, 2, facing_left) == 6u)));
    case NGF_MOTION_BACK:
        return (uint8_t)(now == 4u || (now == 5u && (ngf_input_dir(in, 1, facing_left) == 4u ||
                                                     ngf_input_dir(in, 2, facing_left) == 4u)));
    case NGF_MOTION_DOWN_FWD: return (uint8_t)(now == 3u);
    case NGF_MOTION_CHARGE_BF:
        return (uint8_t)(((FWDS >> now) & 1u) && in->back_release >= CHARGE_FRAMES && in->back_age <= CHARGE_WINDOW);
    case NGF_MOTION_CHARGE_DU:
        return (uint8_t)(((UPS >> now) & 1u) && in->down_release >= CHARGE_FRAMES && in->down_age <= CHARGE_WINDOW);
    case NGF_MOTION_360:
    case NGF_MOTION_720: {
        uint8_t required = motion == NGF_MOTION_720 ? 8u : 4u;
        return (uint8_t)(ordered_rotation(in, facing_left, window, 0u, required) ||
                         ordered_rotation(in, facing_left, window, 1u, required));
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
    /* The motion must end within the last few frames, steps in order. */
    step = motion_len[motion];
    for (ago = 0; ago < window && step; ago++) {
        uint8_t d = ngf_input_dir(in, ago, facing_left);
        if (step == motion_len[motion] && ago > MOTION_LATE) return 0;
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

static uint8_t NEOGEO_USER event_priority(uint8_t type)
{
    switch (type) {
    case NGF_EV_KO: case NGF_EV_ROUND: case NGF_EV_FIGHT:
    case NGF_EV_TIME_UP: case NGF_EV_WIN: case NGF_EV_TAG:
        return 3u;
    case NGF_EV_MOVE: case NGF_EV_PROJECTILE: case NGF_EV_LAND:
        return 1u;
    default: return 2u;
    }
}

static void NEOGEO_USER event(NGFMatch *m, uint8_t type, uint8_t who, uint8_t data, int16_t x, int16_t y)
{
    NGFEvent *e;
    if (m->ev_count >= NGF_EVENTS) {
        uint8_t i, victim = NGF_EVENTS, priority = event_priority(type);
        for (i = 0u; i < NGF_EVENTS; i++) {
            uint8_t p = event_priority(m->ev[i].type);
            if (p < priority) { victim = i; priority = p; }
        }
        m->events_dropped++;
        if (victim == NGF_EVENTS) {
            if (event_priority(type) == 3u) m->events_critical_dropped++;
            return;
        }
        /* Preserve the order of every retained event. Cosmetic pressure
         * must not suppress a round transition or a decisive impact. */
        for (i = victim; i + 1u < m->ev_count; i++) m->ev[i] = m->ev[i + 1u];
        m->ev_count--;
    }
    e = &m->ev[m->ev_count++];
    e->type = type; e->who = who; e->data = data; e->pad = 0; e->x = x; e->y = y;
    if (m->ev_count > m->events_peak) m->events_peak = m->ev_count;
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
    if (!f->grounded && f->def->combat && f->def->combat->juggle_limit &&
        f->juggle_points >= f->def->combat->juggle_limit) return 0;
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

/* Resolve only the boxes of the currently displayed animation frame.
 * Save the actual contacting rectangle for sparks/counters and strike trades. */
static uint8_t NEOGEO_USER frame_contact(const NGFFighterDef *d, uint8_t anim, uint8_t frame,
    const NGFMove *move, int32_t x, int32_t h, uint8_t facing_left,
    const NGFFighter *target, WBox *contact)
{
    if (d->hit_sets && d->frame_hits) {
        uint16_t index = (uint16_t)(d->anims[anim].first + frame);
        const NGFHitSet *set = &d->hit_sets[d->frame_hits[index]];
        uint8_t k;
        for (k = 0u; k < set->count && k < 3u; k++) {
            WBox box = world_box(x, h, facing_left, &set->box[k]);
            if (set->box[k].w && set->box[k].h && hits_body(target, box)) {
                *contact = box;
                return 1u;
            }
        }
        return 0u;
    }
    if (!move->hit.w || !move->hit.h) return 0u;
    *contact = world_box(x, h, facing_left, &move->hit);
    return hits_body(target, *contact);
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
        {
            uint8_t stop = (uint8_t)(hp->hitstop / 2u + 2u);
            if (m->hitstop < stop) m->hitstop = stop;
        }
        return;
    }
    {
        uint16_t dmg = hp->damage;
        uint8_t scale = def->combo > 5u ? 5u : def->combo;
        dmg = (uint16_t)(dmg * (10u - scale) / 10u);
        if (att->max_super) dmg = (uint16_t)(dmg + dmg / 2u);
        else if (att->max_timer) dmg = (uint16_t)(dmg + dmg / 4u);
        if (def->state == NGF_S_ATTACK && def->counter) {
            dmg = (uint16_t)(dmg + dmg / 4u);
            event(m, NGF_EV_HIT, att_i, 0xFE, ex, ey);   /* counter */
        }
        if (!dmg) dmg = 1;
        def->life = def->life > dmg ? (uint16_t)(def->life - dmg) : (m->training ? 1u : 0u);
        if (def->combo < 255u) def->combo++;
        def->combo_damage = def->combo_damage > (uint16_t)(65535u - dmg) ? 65535u : (uint16_t)(def->combo_damage + dmg);
        if (att->power < NGF_POWER_MAX) att->power = (uint16_t)(att->power + hp->meter);
        if (def->power < NGF_POWER_MAX) def->power = (uint16_t)(def->power + hp->meter / 2u);
        if (att->power > NGF_POWER_MAX) att->power = NGF_POWER_MAX;
        if (def->power > NGF_POWER_MAX) def->power = NGF_POWER_MAX;
        if (!def->grounded || (hp->on_hit & (NGF_ON_KNOCKDOWN | NGF_ON_LAUNCH)) || !def->life) {
            int8_t lvx = hp->launch_vx ? hp->launch_vx : 12, lvy = hp->launch_vy ? hp->launch_vy : 28;
            int16_t away = def->x >= att->x ? 1 : -1;
            uint8_t airborne = (uint8_t)!def->grounded;
            int16_t previous_vh = def->vh;
            if (!def->life && lvy < 32) lvy = 32;
            def->state = NGF_S_FALL;
            def->grounded = 0;
            def->vh = (int16_t)(lvy * 32);
            if (def->def->combat) {
                if (def->juggle_points < 255u) def->juggle_points++;
                if (airborne) {
                    if (def->relaunches >= def->def->combat->relaunch_limit)
                        def->vh = previous_vh < 0 ? previous_vh : 0;
                    else def->relaunches++;
                }
            }
            def->vx = (int16_t)(away * lvx * 32);
            set_anim(def, NGF_A_FALL, 1);
        } else {
            uint8_t low = (uint8_t)(def->state == NGF_S_CROUCH ||
                                    (def->state == NGF_S_ATTACK && def->move && (def->move->stance & NGF_STANCE_CROUCH)));
            def->state = NGF_S_HIT;
            def->stun_frames = (uint8_t)(hp->hitstun + (def->counter ? 4u : 0u));
            if (def->def->combat && def->combo > 4u) {
                uint8_t loss = (uint8_t)((def->combo > 10u ? 10u : def->combo) - 4u);
                def->stun_frames = def->stun_frames > (uint8_t)(6u + loss) ?
                    (uint8_t)(def->stun_frames - loss) : 6u;
            }
            set_anim(def, low ? NGF_A_HIT_LOW : NGF_A_HIT, 1);
            push_away(m, att, def, hp->pushback);
        }
        def->move = 0;
        def->counter = 0;
        event(m, NGF_EV_HIT, att_i, hp->sound, ex, ey);
        if (!def->life) event(m, NGF_EV_KO, (uint8_t)(att_i ^ 1u), 0, ex, ey);
        if (m->hitstop < hp->hitstop) m->hitstop = hp->hitstop;
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

/* Proximity guard. Holding back shows a guard and holds position only while
 * a strike can actually arrive: the opponent's move still has active frames
 * ahead and its authored reach (plus a small margin) covers this fighter's
 * front edge, or an enemy projectile is flying at it and is close. Any other
 * attack or projectile on screen leaves walking backward free. Real blocking
 * is decided at contact by can_block(), independent of this appearance. */
#define GUARD_REACH_MARGIN  12
#define GUARD_SHOT_FRAMES   10
static uint8_t NEOGEO_USER guard_threat(const NGFMatch *m, uint8_t who)
{
    const NGFFighter *f = &m->f[who], *o = &m->f[who ^ 1u];
    int16_t x = ngf_fighter_px(f);
    uint8_t i;
    if (o->state == NGF_S_ATTACK && o->move && o->move->kind != NGF_KIND_THROW && o->move->hit.w) {
        const NGFAnim *a = &o->def->anims[o->anim];
        int16_t gap = (int16_t)((int16_t)ngf_distance(m) - (int16_t)f->def->push_w);
        int16_t reach = (int16_t)(o->move->hit.x + (int16_t)o->move->hit.w + GUARD_REACH_MARGIN);
        uint8_t strike = 0u;
        for (i = o->frame; i < a->count; i++)
            if (o->def->frames[a->first + i].flags & NGF_FF_ACTIVE) { strike = 1u; break; }
        if (strike && gap <= reach) return 1u;
    }
    for (i = 0; i < NGF_PROJECTILES; i++) {
        const NGFProjectile *p = &m->proj[i];
        int16_t d, speed;
        if (!p->active || p->owner == who) continue;
        d = (int16_t)(p->facing_left ? (p->x >> 8) - x : x - (p->x >> 8));
        speed = (int16_t)(p->def->speed < 0 ? -p->def->speed : p->def->speed);
        if (d >= 0 && d <= (int16_t)(speed * GUARD_SHOT_FRAMES + f->def->push_w)) return 1u;
    }
    return 0u;
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

static const NGFThrowAnchor * NEOGEO_USER find_throw_anchor(const NGFFighterDef *d, uint8_t move_index)
{
    uint8_t i;
    if (!d || !d->combat || !d->combat->throws) return 0;
    for (i = 0u; i < d->combat->throw_count; i++)
        if (d->combat->throws[i].move == move_index) return &d->combat->throws[i];
    return 0;
}

static void NEOGEO_USER place_thrown(const NGFMatch *m, NGFFighter *f, NGFFighter *o, int8_t direction, uint8_t base_x, uint8_t adjust_size)
{
    int16_t dist = (int16_t)base_x;
    int32_t lo = (int32_t)FIGHT_MARGIN << 8;
    int32_t hi = (int32_t)(m->stage_w - FIGHT_MARGIN) << 8;
    if (adjust_size && o->def->push_w > 20u) dist = (int16_t)(dist + (int16_t)(o->def->push_w - 20u));
    o->x = f->x + (int32_t)(direction * dist) * 256;
    if (o->x < lo) { o->x = lo; f->x = o->x - (int32_t)(direction * dist) * 256; }
    if (o->x > hi) { o->x = hi; f->x = o->x - (int32_t)(direction * dist) * 256; }
}

static void NEOGEO_USER start_move(NGFMatch *m, uint8_t who, uint8_t k)
{
    NGFFighter *f = &m->f[who], *o = &m->f[who ^ 1u];
    const NGFMove *mv = &f->def->moves[k];
    if (f->state != NGF_S_ATTACK) {
        uint8_t r;
        for (r = 0u; r < NGF_CANCEL_RULES; r++) f->route_uses[r] = 0u;
    }
    /* A+B out of blockstun: a guard-cancel roll for one stock. */
    if (f->state == NGF_S_BLOCK && mv->kind == NGF_KIND_EVADE) {
        f->power = (uint16_t)(f->power - NGF_POWER_STOCK);
        f->invuln = 10;
    }
    f->max_super = 0;
    f->move = mv;
    f->move_index = k;
    f->state_timer = 0;
    f->state = NGF_S_ATTACK;
    f->move_hit = 0;
    f->move_contact = 0u;
    f->counter = 1;
    set_anim(f, mv->anim, 1);
    if (f->grounded) f->vx = 0;
    else f->air_used = 1;
    if (mv->kind == NGF_KIND_SUPER) {
        if (f->max_timer) {               /* the MAX super: free, stronger, ends the mode */
            f->max_timer = 0;
            f->max_super = 1;
        } else {
            f->power = (uint16_t)(f->power - mv->cost * NGF_POWER_STOCK);
        }
        m->super_freeze = 30u;
        m->super_who = who;
        event(m, NGF_EV_SUPER_FREEZE, who, (uint8_t)(mv->cost | (f->max_super ? 0x80u : 0u)),
              ngf_fighter_px(f), ngf_fighter_py(f));
    }
    if (mv->kind == NGF_KIND_GUARD_ATTACK) f->spirit = (uint8_t)(f->spirit - mv->cost);
    ngf_input_consume(&f->input, mv->buttons);
    f->input.kara = mv->kind <= NGF_KIND_COMMAND ? mv->buttons : 0u;
    if (mv->motion && mv->motion < NGF_MOTION_DASH) f->input.motion_frames = 0;
    event(m, NGF_EV_MOVE, who, mv->kind, ngf_fighter_px(f), ngf_fighter_py(f));
    if (mv->kind == NGF_KIND_THROW) {
        const NGFThrowAnchor *ta = find_throw_anchor(f->def, k);
        uint8_t tech = (ta && ta->tech_ticks) ? ta->tech_ticks : 4u;
        uint8_t grip = (ta && ta->grip_x) ? ta->grip_x : 26u;
        if (o->cpu ? (uint8_t)(rnd(m) % 100u) < (uint8_t)(m->cpu[who ^ 1u].level * 5u) :
                     (ngf_input_pressed(&o->input, NGF_BTN_C, tech) || ngf_input_pressed(&o->input, NGF_BTN_D, tech))) {
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
        place_thrown(m, f, o, sign_of(f), grip, (uint8_t)(ta != 0));
        o->facing_left = (uint8_t)!f->facing_left;
        set_anim(o, NGF_A_THROWN, 1);
        event(m, NGF_EV_THROW, who, 0, ngf_fighter_px(o), 40);
    }
    enter_frame(m, who);
}

uint8_t NEOGEO_USER ngf_cancel_check(const NGFFighter *f, uint8_t to, uint8_t *rule_index)
{
    const NGFCombatRules *rules = f->def->combat;
    uint8_t i, reason = NGF_CANCEL_NO_ROUTE;
    if (!rules || !rules->routes || rules->route_count > NGF_CANCEL_RULES ||
        f->state != NGF_S_ATTACK || to >= f->def->move_count) return reason;
    for (i = 0u; i < rules->route_count; i++) {
        const NGFCancelRule *r = &rules->routes[i];
        const NGFMove *target = &f->def->moves[to];
        uint8_t contact = f->move_contact ? f->move_contact : NGF_CONTACT_WHIFF;
        if (r->from != f->move_index || r->to != to) continue;
        reason = NGF_CANCEL_WINDOW;
        if (f->state_timer < r->first_tick || f->state_timer > r->last_tick) continue;
        reason = NGF_CANCEL_CONTACT;
        if (!(r->conditions & contact)) continue;
        reason = NGF_CANCEL_REPEAT;
        if (!r->max_uses || f->route_uses[i] >= r->max_uses) continue;
        reason = NGF_CANCEL_RESOURCE;
        if ((r->conditions & NGF_CANCEL_NEEDS_MAX) && !f->max_timer) continue;
        if (target->kind == NGF_KIND_SUPER && !f->max_timer &&
            f->power < (uint16_t)(target->cost * NGF_POWER_STOCK)) continue;
        if (rule_index) *rule_index = i;
        return NGF_CANCEL_OK;
    }
    return reason;
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
        uint8_t btn_ok, route = 255u;
        if (!(mv->stance & stance)) continue;
        if ((mv->stance & NGF_STANCE_CLOSE) && dist > f->def->close_range) continue;
        if ((mv->stance & NGF_STANCE_FAR) && dist <= f->def->close_range) continue;
        if (f->state == NGF_S_BLOCK) {
            if (mv->kind == NGF_KIND_GUARD_ATTACK) { if (f->spirit < mv->cost) continue; }
            else if (mv->kind != NGF_KIND_EVADE || mv->motion || f->power < NGF_POWER_STOCK) continue;
        } else if (mv->kind == NGF_KIND_GUARD_ATTACK) {
            continue;
        }
        if (cancel && (!f->def->combat || (cancel & NGF_CANCEL_KARA))) {
            uint8_t ok = 0, kara = f->input.kara;
            if ((cancel & NGF_CANCEL_KARA) && mv->buttons == kara && mv->kind <= NGF_KIND_COMMAND) continue;
            if ((cancel & NGF_CANCEL_CHAIN) && mv->kind == NGF_KIND_NORMAL && k > f->move_index) ok = 1;
            if ((cancel & NGF_CANCEL_SPECIAL) && mv->kind == NGF_KIND_SPECIAL && k != f->move_index) ok = 1;
            if ((cancel & (NGF_CANCEL_SPECIAL | NGF_CANCEL_SUPER)) && mv->kind == NGF_KIND_SUPER) ok = 1;
            if ((cancel & NGF_CANCEL_KARA) && kara && (mv->buttons & kara) == kara && mv->buttons != kara) ok = 1;
            if (!ok) continue;
        }
        if (stance == NGF_STANCE_AIR && f->air_used) continue;
        if (mv->kind == NGF_KIND_SUPER && !f->max_timer && f->power < (uint16_t)(mv->cost * NGF_POWER_STOCK)) continue;
        if (mv->projectile && has_projectile(m, who)) continue;
        btn_ok = ngf_input_pressed(&f->input, mv->buttons,
                                   (uint8_t)(mv->kind <= NGF_KIND_COMMAND || mv->kind == NGF_KIND_THROW ? 4u : 5u));
        if (!btn_ok && (mv->kind == NGF_KIND_SPECIAL || mv->kind == NGF_KIND_SUPER)) {
            if (ngf_input_released(&f->input, mv->buttons, 3u)) btn_ok = 1;
        }
        if (!btn_ok) continue;
        if (mv->motion && !ngf_input_motion(&f->input, mv->motion, f->facing_left, mv->window)) continue;
        if (cancel && f->def->combat && !(cancel & NGF_CANCEL_KARA)) {
            f->cancel_reject = ngf_cancel_check(f, k, &route);
            if (f->cancel_reject != NGF_CANCEL_OK) continue;
        }
        if (mv->kind == NGF_KIND_THROW) {
            uint16_t reach = (uint16_t)(f->def->push_w + o->def->push_w + 20u);
            if (mv->range > reach) reach = mv->range;
            if (!throwable(o) || dist > reach) continue;
        }
        if (route != 255u) f->route_uses[route]++;
        f->cancel_reject = NGF_CANCEL_OK;
        start_move(m, who, k);
        return 1;
    }
    return 0;
}

/* B+C (A+B+C works too): spend a stock to enter MAX mode. */
static uint8_t NEOGEO_USER max_start(NGFMatch *m, uint8_t who)
{
    NGFFighter *f = &m->f[who];
    if (f->max_timer || !f->grounded || f->power < NGF_POWER_STOCK) return 0;
    if (!ngf_input_pressed(&f->input, NGF_BTN_B | NGF_BTN_C, 4u)) return 0;
    f->power = (uint16_t)(f->power - NGF_POWER_STOCK);
    f->max_timer = NGF_MAX_FRAMES;
    ngf_input_consume(&f->input, NGF_BTN_A | NGF_BTN_B | NGF_BTN_C);
    f->state = NGF_S_IDLE;
    f->move = 0;
    f->vx = 0;
    set_anim(f, NGF_A_IDLE, 1);
    m->super_freeze = 20u;
    m->super_who = who;
    event(m, NGF_EV_MAX, who, 0, ngf_fighter_px(f), ngf_fighter_py(f));
    return 1;
}

/* Up from the ground: a jump, a short hop if up is let go at once, and
 * longer out of a run. */
static void NEOGEO_USER jump_start(NGFFighter *f, uint8_t dir, uint8_t run)
{
    const NGFFighterDef *d = f->def;
    int16_t vx = dir == 9u ? d->jump_vx : dir == 7u ? -d->jump_vx : 0;
    if (run && vx > 0) vx = (int16_t)(vx + vx / 2);
    f->state = NGF_S_JUMP;
    f->state_timer = 0;
    f->grounded = 0;
    f->air_used = 0;
    f->hop = 1;
    f->vh = d->jump_vy;
    f->vx = (int16_t)(vx * sign_of(f));
    set_anim(f, NGF_A_JUMP, 1);
}

static void NEOGEO_USER free_move(NGFMatch *m, uint8_t who, uint8_t controls)
{
    NGFFighter *f = &m->f[who];
    const NGFFighterDef *d = f->def;
    uint8_t dir = controls ? ngf_input_dir(&f->input, 0, f->facing_left) : 5u;
    if (controls && (max_start(m, who) || try_moves(m, who, 0))) return;
    if (dir >= 7u) {
        jump_start(f, dir, 0);
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
        set_anim(f, dir == 1u && guard_threat(m, who) ? NGF_A_GUARD_LOW : NGF_A_CROUCH, 0);
        return;
    }
    f->state = NGF_S_IDLE;
    if (dir == 6u) {
        f->vx = (int16_t)(d->walk_fwd * sign_of(f));
        set_anim(f, NGF_A_WALK_F, 0);
        if (!(m->frame & 7u) && f->spirit < NGF_SPIRIT_MAX) f->spirit++;
    } else if (dir == 4u) {
        /* Holding back retreats unless a strike can arrive now (see
         * guard_threat); then it shows a standing guard in place. A distant
         * attack or projectile never pins walking backward. */
        uint8_t threat = guard_threat(m, who);
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
    f->juggle_points = f->relaunches = 0u;
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
    if (f->max_timer) f->max_timer--;
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
        if (f->state_timer < 255u) f->state_timer++;
        if (f->hop) {
            if (f->state_timer > HOP_FRAMES) f->hop = 0;
            else if (!controls || ngf_input_dir(&f->input, 0, f->facing_left) < 7u) {
                f->vh = (int16_t)(f->vh * 11 / 16);
                f->hop = 0;
            }
        }
        if (controls) try_moves(m, who, 0);
        anim_step(m, who);
        break;
    case NGF_S_DASH:
        f->state_timer++;
        f->vx = (int16_t)(f->def->dash * sign_of(f));
        if (!(m->frame & 3u) && f->spirit < NGF_SPIRIT_MAX) f->spirit++;
        anim_step(m, who);
        if (controls && (max_start(m, who) || try_moves(m, who, 0))) break;
        if (controls && ngf_input_dir(&f->input, 0, f->facing_left) >= 7u) {
            jump_start(f, ngf_input_dir(&f->input, 0, f->facing_left), 1);
            break;
        }
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
        uint8_t done, cancel = f->move ? f->move->cancel : 0u;
        if (f->max_timer && f->move && f->move->kind == NGF_KIND_SPECIAL) cancel |= NGF_CANCEL_SPECIAL | NGF_CANCEL_SUPER;
        if (f->def->combat) cancel |= 0x10u; /* authored route policy */
        if ((f->move_hit || f->def->combat) && cancel && controls && try_moves(m, who, cancel)) break;
        /* A connected normal may be cancelled into MAX mode. */
        if (f->move_hit && f->move && f->move->kind <= NGF_KIND_COMMAND && controls && max_start(m, who)) break;
        /* The first frames of a normal still take a second button (A+B, C+D,
         * B+C) or a special: the buttons need not land on the same frame. */
        if (f->move && f->move->kind <= NGF_KIND_COMMAND && f->state_timer <= 2u && controls) {
            uint8_t keep = f->input.pending;
            f->input.pending |= f->input.kara;
            if (max_start(m, who) ||
                try_moves(m, who, NGF_CANCEL_CHAIN | NGF_CANCEL_SPECIAL | NGF_CANCEL_SUPER | NGF_CANCEL_KARA)) break;
            f->input.pending = keep;
        } else {
            f->input.kara = 0;
        }
        f->state_timer++;
        if (f->move && f->move->kind == NGF_KIND_THROW && o->state == NGF_S_THROWN) {
            const NGFThrowAnchor *ta = find_throw_anchor(f->def, f->move_index);
            uint8_t carry = (ta && ta->carry_tick) ? ta->carry_tick : 8u;
            uint8_t grip = (ta && ta->grip_x) ? ta->grip_x : 26u;
            uint8_t lift = f->state_timer > carry ? carry : (uint8_t)f->state_timer;
            int8_t direction = sign_of(f);
            if (f->move->motion == NGF_MOTION_BACK && f->state_timer >= carry) direction = (int8_t)-direction;
            place_thrown(m, f, o, direction, grip, (uint8_t)(ta != 0));
            if (ta && ta->lift) o->h = ((int32_t)lift * ta->lift * 256) / (carry ? carry : 1u);
            else o->h = (int32_t)(lift * ((f->move->on_hit & NGF_ON_LIFT) ? 7u : 2u)) * 256;
            o->grounded = 0u;
            o->vx = o->vh = 0;
        }
        done = anim_step(m, who);
        if (f->move && f->move->kind == NGF_KIND_THROW && done) {
            NGFFighter *t = o;
            if (t->state == NGF_S_THROWN) {
                const NGFThrowAnchor *ta = find_throw_anchor(f->def, f->move_index);
                HitParams hp;
                int8_t throw_dir = (int8_t)((f->move->motion == NGF_MOTION_BACK) ? -sign_of(f) : sign_of(f));
                uint8_t rel = (ta && (ta->release_x || ta->grip_x)) ? (ta->release_x ? ta->release_x : ta->grip_x) : 28u;
                place_thrown(m, f, t, throw_dir, rel, 1u);
                params_from_move(f->move, &hp);
                hp.level = NGF_LV_UNBLOCKABLE;
                hp.on_hit |= NGF_ON_KNOCKDOWN;
                t->state = NGF_S_IDLE;
                t->grounded = 1u;
                t->juggle_points = 0u;
                t->relaunches = 0u;
                apply_hit(m, who, &hp, ngf_fighter_px(t), 40, 1);
                /* KOF throw trajectory: launch high and far in the throw direction */
                {
                    int16_t lvx = hp.launch_vx ? (int16_t)(hp.launch_vx > 0 ? hp.launch_vx : -hp.launch_vx) : 16;
                    int16_t lvy = hp.launch_vy ? hp.launch_vy : 26;
                    t->vx = (int16_t)(throw_dir * lvx * 96);
                    t->vh = (int16_t)(lvy * 76);
                    t->h = (int32_t)(ta && ta->lift ? ta->lift : 24u) * 256;
                    t->grounded = 0u;
                    t->state = NGF_S_FALL;
                    set_anim(t, NGF_A_FALL, 1);
                }
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
    const NGFMove *contact_move[2] = {0, 0};
    WBox box[2] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
    for (i = 0; i < 2u; i++) {
        NGFFighter *f = &m->f[i], *o = &m->f[i ^ 1u];
        const NGFFrame *fr = ngf_fighter_frame(f);
        if (f->state != NGF_S_ATTACK || !f->move || f->move->kind == NGF_KIND_THROW) continue;
        if (!(fr->flags & NGF_FF_ACTIVE) || f->move_hit) continue;
        if (!vulnerable(o)) continue;
        hit[i] = frame_contact(f->def, f->anim, f->frame, f->move, f->x, f->h, f->facing_left, o, &box[i]);
        contact_move[i] = f->move;
    }
    for (i = 0; i < 2u; i++) {
        if (!hit[i]) continue;
        {
            NGFFighter *f = &m->f[i];
            HitParams hp;
            const NGFMove *mv = contact_move[i];
            if (!mv) continue;
            params_from_move(mv, &hp);
            f->move_hit = 1;
            if (try_counter(m, (uint8_t)(i ^ 1u), (int16_t)(f->facing_left ? box[i].l : box[i].r),
                            (int16_t)((box[i].b + box[i].t) / 2))) continue;
            f->move_contact |= can_block(&m->f[i ^ 1u], hp.level) ? NGF_CONTACT_BLOCK :
                (uint8_t)(NGF_CONTACT_HIT | (m->f[i ^ 1u].counter ? NGF_CONTACT_COUNTER : 0u));
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
enum { PLAN_NONE, PLAN_GUARD, PLAN_JUMP_IN, PLAN_WAKE };
enum { PICK_WALK, PICK_RUN, PICK_JUMP, PICK_HOP, PICK_POKE, PICK_RUSH, PICK_BACK, PICK_WAIT,
       PICK_THROW, PICK_COMBO, PICK_LOW, PICK_BLOW, PICK_SWEEP, PICK_ROLL, PICK_BACKDASH };

static void NEOGEO_USER cpu_push(NGFCpu *c, uint8_t raw, uint8_t frames)
{
    while (frames-- && c->len < sizeof(c->seq)) c->seq[c->len++] = raw;
}

static uint8_t NEOGEO_USER chance(NGFMatch *m, uint16_t percent)
{
    return (uint8_t)(rnd(m) % 100u < percent);
}

/* How far in front a move's strike reaches, px. */
static uint8_t NEOGEO_USER move_reach(const NGFMove *mv)
{
    int16_t r = (int16_t)(mv->hit.x + mv->hit.w);
    return (uint8_t)(r < 0 ? 0 : r > 200 ? 200 : r);
}

static uint8_t NEOGEO_USER move_invuln(const NGFFighterDef *d, const NGFMove *mv)
{
    const NGFAnim *a = &d->anims[mv->anim];
    uint8_t i;
    for (i = 0; i < a->count && i < 3u; i++)
        if (d->frames[a->first + i].flags & NGF_FF_INVULN) return 1;
    return 0;
}

/* The other fighter's move has missed and has no strike left. */
static uint8_t NEOGEO_USER recovering(const NGFFighter *o)
{
    const NGFAnim *a;
    uint8_t i;
    if (o->state != NGF_S_ATTACK || o->move_hit || !o->move || !o->grounded) return 0;
    a = &o->def->anims[o->anim];
    for (i = o->frame; i < a->count; i++)
        if (o->def->frames[a->first + i].flags & NGF_FF_ACTIVE) return 0;
    return 1;
}

/* Distance to the nearest enemy projectile flying at us, 0xFFFF none. */
static uint16_t NEOGEO_USER incoming(const NGFMatch *m, uint8_t who)
{
    int16_t x = ngf_fighter_px(&m->f[who]);
    uint16_t best = 0xFFFFu;
    uint8_t i;
    for (i = 0; i < NGF_PROJECTILES; i++) {
        const NGFProjectile *p = &m->proj[i];
        int16_t d;
        if (!p->active || p->owner == who) continue;
        d = (int16_t)(p->facing_left ? (p->x >> 8) - x : x - (p->x >> 8));
        if (d >= 0 && (uint16_t)d < best) best = (uint16_t)d;
    }
    return best;
}

static uint8_t NEOGEO_USER find_normal(const NGFFighterDef *d, uint8_t stance, uint8_t button, uint8_t avoid)
{
    uint8_t k, best = 0xFFu;
    for (k = 0; k < d->move_count; k++) {
        const NGFMove *mv = &d->moves[k];
        if (mv->kind != NGF_KIND_NORMAL || mv->buttons != button || !(mv->stance & stance)) continue;
        if ((mv->stance & NGF_STANCE_AIR) != (stance & NGF_STANCE_AIR)) continue;
        if (!(mv->stance & avoid)) return k;
        if (best == 0xFFu) best = k;
    }
    return best;
}

static uint8_t NEOGEO_USER role_anim(const NGFCpu *c, const NGFFighterDef *d, uint8_t role)
{
    return c->role[role] == 0xFFu ? 0xFFu : d->moves[c->role[role]].anim;
}

/* Sort the fighter's moves into the roles the CPU plays them in. */
static void NEOGEO_USER cpu_learn(NGFCpu *c, const NGFFighterDef *d)
{
    const NGFCpuProfile *p = &d->cpu_profile;
    uint8_t k;
    for (k = 0; k < NGF_CPU_ROLES; k++) c->role[k] = 0xFFu;
    c->learned = d;
    c->role[NGF_CPU_POKE] = find_normal(d, NGF_STANCE_STAND, NGF_BTN_D, NGF_STANCE_CLOSE);
    c->role[NGF_CPU_LOW] = find_normal(d, NGF_STANCE_CROUCH, NGF_BTN_B, 0);
    c->role[NGF_CPU_JAB] = find_normal(d, NGF_STANCE_CROUCH, NGF_BTN_A, 0);
    c->role[NGF_CPU_HEAVY] = find_normal(d, NGF_STANCE_STAND, NGF_BTN_C, NGF_STANCE_FAR);
    c->role[NGF_CPU_SWEEP] = find_normal(d, NGF_STANCE_CROUCH, NGF_BTN_D, 0);
    c->role[NGF_CPU_AIR] = find_normal(d, NGF_STANCE_AIR, NGF_BTN_D, 0);
    c->role[NGF_CPU_AIR2] = find_normal(d, NGF_STANCE_AIR, NGF_BTN_C, 0);
    if (c->role[NGF_CPU_POKE] == 0xFFu && p->poke < d->move_count) c->role[NGF_CPU_POKE] = p->poke;
    if (c->role[NGF_CPU_SWEEP] == 0xFFu && p->sweep < d->move_count) c->role[NGF_CPU_SWEEP] = p->sweep;
    if (p->anti_air < d->move_count) c->role[NGF_CPU_ANTI_AIR] = p->anti_air;
    if (p->special < d->move_count && d->moves[p->special].kind == NGF_KIND_SPECIAL &&
        d->moves[p->special].damage) c->role[NGF_CPU_RUSH] = p->special;
    if (p->super_move < d->move_count && d->moves[p->super_move].motion != NGF_MOTION_720)
        c->role[NGF_CPU_SUPER] = p->super_move;
    if (p->throw_move < d->move_count) c->role[NGF_CPU_THROW] = p->throw_move;
    for (k = 0; k < d->move_count; k++) {
        const NGFMove *mv = &d->moves[k];
        switch (mv->kind) {
        case NGF_KIND_SPECIAL:
            if (mv->projectile) { c->role[NGF_CPU_PROJECTILE] = k; break; }
            if (!mv->motion || !mv->damage || mv->motion == NGF_MOTION_720) break;
            if (c->role[NGF_CPU_ANTI_AIR] == 0xFFu && (mv->motion == NGF_MOTION_DP || move_invuln(d, mv))) {
                c->role[NGF_CPU_ANTI_AIR] = k;
                break;
            }
            if (mv->anim == role_anim(c, d, NGF_CPU_ANTI_AIR) || mv->anim == role_anim(c, d, NGF_CPU_RUSH) ||
                mv->anim == role_anim(c, d, NGF_CPU_SPECIAL)) break;
            if (c->role[NGF_CPU_RUSH] == 0xFFu) c->role[NGF_CPU_RUSH] = k;
            else if (c->role[NGF_CPU_SPECIAL] == 0xFFu) c->role[NGF_CPU_SPECIAL] = k;
            else if (c->role[NGF_CPU_SPECIAL2] == 0xFFu) c->role[NGF_CPU_SPECIAL2] = k;
            break;
        case NGF_KIND_SUPER:
            if (c->role[NGF_CPU_SUPER] == 0xFFu && mv->motion != NGF_MOTION_720) c->role[NGF_CPU_SUPER] = k;
            break;
        case NGF_KIND_THROW:
            if (c->role[NGF_CPU_THROW] == 0xFFu) c->role[NGF_CPU_THROW] = k;
            break;
        case NGF_KIND_EVADE:
            if (!mv->motion && c->role[NGF_CPU_ROLL] == 0xFFu) c->role[NGF_CPU_ROLL] = k;
            break;
        case NGF_KIND_COMMAND:
            if (mv->buttons == (NGF_BTN_C | NGF_BTN_D) && c->role[NGF_CPU_BLOW] == 0xFFu) c->role[NGF_CPU_BLOW] = k;
            break;
        default:
            break;
        }
    }
}

/* Queue the stick motion and buttons of move k; fast: one frame per step. */
static void NEOGEO_USER cpu_queue_move(NGFMatch *m, uint8_t who, uint8_t k, uint8_t fast)
{
    static const uint8_t pads[][6] = {
        {0}, {2, 3, 6}, {2, 1, 4}, {6, 2, 3}, {4, 2, 1}, {4, 1, 2, 3, 6}, {6, 3, 2, 1, 4},
        {2, 3, 6, 2, 3, 6}, {2, 1, 4, 2, 1, 4},
    };
    static const uint8_t lens[] = {0, 3, 3, 3, 3, 5, 5, 6, 6};
    static const uint8_t spin[] = {4, 1, 2, 3, 6};
    NGFCpu *c = &m->cpu[who];
    const NGFFighter *f = &m->f[who];
    const NGFMove *mv;
    uint8_t fl = f->facing_left, i, last = 5u, step = fast ? 1u : 2u;
    if (k >= f->def->move_count) return;
    mv = &f->def->moves[k];
    c->len = c->pos = 0;
    c->wait = 0;
    if (mv->motion >= NGF_MOTION_QCF && mv->motion <= NGF_MOTION_DQCB) {
        for (i = 0; i + 1u < lens[mv->motion]; i++) cpu_push(c, ngf_raw_from_numpad(pads[mv->motion][i], fl), step);
        last = pads[mv->motion][lens[mv->motion] - 1u];
    } else if (mv->motion == NGF_MOTION_360) {
        for (i = 0; i < sizeof(spin); i++) cpu_push(c, ngf_raw_from_numpad(spin[i], fl), 1u);
        last = 9u;                      /* the button lands with the up-diagonal, before any jump */
    } else if (mv->motion == NGF_MOTION_CHARGE_BF) {
        cpu_push(c, ngf_raw_from_numpad(4, fl), (uint8_t)(CHARGE_FRAMES + 2u));
        last = 6u;
    } else if (mv->motion == NGF_MOTION_CHARGE_DU) {
        cpu_push(c, ngf_raw_from_numpad(1, fl), (uint8_t)(CHARGE_FRAMES + 2u));
        last = 9u;
    } else if (mv->motion == NGF_MOTION_FWD) last = 6u;
    else if (mv->motion == NGF_MOTION_BACK) last = 4u;
    else if (mv->motion == NGF_MOTION_DOWN_FWD) last = 3u;
    else if (mv->stance == NGF_STANCE_CROUCH) {
        last = 2u;
        cpu_push(c, ngf_raw_from_numpad(2, fl), 1u);
    }
    cpu_push(c, (uint8_t)(ngf_raw_from_numpad(last, fl) | mv->buttons), 1u);
    cpu_push(c, ngf_raw_from_numpad(last <= 3u ? 2u : 5u, fl), 1u);
}

static uint8_t NEOGEO_USER cpu_move(NGFMatch *m, uint8_t who, uint8_t k, uint8_t fast)
{
    if (k == 0xFFu) return 0;
    cpu_queue_move(m, who, k, fast);
    return 1;
}

static void NEOGEO_USER cpu_jump(NGFCpu *c, uint8_t fl, uint8_t dir, uint8_t hop)
{
    c->len = c->pos = 0;
    cpu_push(c, ngf_raw_from_numpad(dir, fl), hop ? 2u : (uint8_t)(HOP_FRAMES + 2u));
    c->plan = PLAN_JUMP_IN;
}

static void NEOGEO_USER cpu_hold(NGFCpu *c, uint8_t raw, uint8_t frames, uint8_t plan)
{
    c->hold = raw;
    c->wait = frames;
    c->plan = plan;
}

/* Guard high against jumps and overheads, low otherwise; weaker CPUs guess. */
static uint8_t NEOGEO_USER cpu_guard(NGFMatch *m, uint8_t who, uint8_t guess)
{
    NGFFighter *f = &m->f[who];
    const NGFFighter *o = &m->f[who ^ 1u];
    uint8_t lv = m->cpu[who].level, level = o->move ? o->move->level : NGF_LV_MID;
    uint8_t high = (uint8_t)(!o->grounded || level == NGF_LV_OVERHEAD);
    if (guess && lv < 9u && chance(m, (uint16_t)((9u - lv) * 4u))) high ^= 1u;
    f->block_low = (uint8_t)!high;
    return ngf_raw_from_numpad(high ? 4u : 1u, f->facing_left);
}

/* A cancellable normal, and the special or super it cancels into once it connects. */
static uint8_t NEOGEO_USER cpu_combo(NGFMatch *m, uint8_t who, uint8_t starter)
{
    NGFCpu *c = &m->cpu[who];
    const NGFFighter *f = &m->f[who];
    const NGFFighterDef *d = f->def;
    uint8_t k = c->role[starter], next = 0xFFu, cancel, pick;
    if (k == 0xFFu) return 0;
    cancel = d->moves[k].cancel;
    if ((cancel & (NGF_CANCEL_SPECIAL | NGF_CANCEL_SUPER)) && chance(m, (uint16_t)(25u + c->level * 8u))) {
        pick = (uint8_t)(rnd(m) & 3u);
        if ((f->power >= NGF_POWER_STOCK || f->max_timer) && chance(m, (uint16_t)(15u + c->level * 5u)))
            next = c->role[NGF_CPU_SUPER];
        if (next == 0xFFu && (cancel & NGF_CANCEL_SPECIAL)) {
            next = c->role[pick == 0u ? NGF_CPU_ANTI_AIR : pick == 1u ? NGF_CPU_SPECIAL :
                           pick == 2u ? NGF_CPU_SPECIAL2 : NGF_CPU_RUSH];
            if (next == 0xFFu) next = c->role[NGF_CPU_RUSH];
        }
    }
    cpu_queue_move(m, who, k, 0);
    c->next = next;
    return 1;
}

static uint8_t NEOGEO_USER cpu_pick(NGFMatch *m, NGFCpu *c, const uint8_t *weight, uint8_t count)
{
    uint16_t total = 0, r;
    uint8_t i, tries;
    for (i = 0; i < count; i++) total = (uint16_t)(total + weight[i]);
    if (!total) return PICK_WAIT;
    for (tries = 0; tries < 2u; tries++) {
        r = (uint16_t)(rnd(m) % total);
        for (i = 0; i < count; i++) {
            if (r < weight[i]) break;
            r = (uint16_t)(r - weight[i]);
        }
        if (i != c->last || weight[i] == total) break;   /* vary the choice */
    }
    c->last = i;
    return i;
}

uint8_t NEOGEO_USER ngf_cpu_input(NGFMatch *m, uint8_t who)
{
    NGFCpu *c = &m->cpu[who];
    NGFFighter *f = &m->f[who], *o = &m->f[who ^ 1u];
    const NGFFighterDef *d = f->def;
    uint8_t lv = c->level, fl = f->facing_left;
    uint8_t react = c->difficulty ? ngf_cpu_reaction_frames(c->difficulty) : (uint8_t)(lv >= 10u ? 4u : 14u - lv);
    uint8_t ready = (uint8_t)(f->state == NGF_S_IDLE || f->state == NGF_S_CROUCH || f->state == NGF_S_DASH);
    uint8_t weight[PICK_BACKDASH + 1], i;
    uint16_t dist, shot;
    if (c->learned != d) cpu_learn(c, d);
    /* Whether this CPU saw the other fighter's attack coming is decided once,
     * as it starts: a guard has to be up before a fast normal lands. */
    if (o->state == NGF_S_ATTACK && o->state_timer == (c->difficulty ? react : 1u))
        c->judge = chance(m, (uint16_t)(25u + lv * 7u));
    if (o->state != NGF_S_ATTACK) c->judge = 0;
    if (c->pos < c->len) return c->seq[c->pos++];
    c->len = c->pos = 0;
    if (m->state != NGF_M_FIGHT) { c->wait = 0; c->next = 0xFFu; return 0; }
    dist = ngf_distance(m);
    shot = incoming(m, who);
    /* A hold (walking, waiting) gives way the moment something new comes at us. */
    if (c->wait) {
        uint8_t alert = (uint8_t)(ready && c->plan != PLAN_GUARD &&
            ((o->state == NGF_S_ATTACK && c->judge && o->grounded) ||
             (o->state == NGF_S_JUMP && o->state_timer == react) || shot < 140u));
        if (!alert) { c->wait--; return c->hold; }
        c->wait = 0;
    }
    if (c->plan == PLAN_GUARD) c->plan = PLAN_NONE;
    c->hold = 0;
    switch (f->state) {
    case NGF_S_ATTACK:
        if (c->next != 0xFFu && f->move_hit) {
            uint8_t k = c->next;
            c->next = 0xFFu;
            /* Better CPUs only spend a super on a hit, not on a block. */
            if (d->moves[k].kind == NGF_KIND_SUPER && o->state != NGF_S_HIT && lv >= 5u) return 0;
            cpu_queue_move(m, who, k, 1u);
            return ngf_cpu_input(m, who);
        }
        return 0;
    case NGF_S_BLOCK:
        c->next = 0xFFu;
        if (lv >= 4u && f->stun_frames > 3u && !(m->frame & 7u) && chance(m, (uint16_t)(lv + lv / 2u))) {
            if (f->power >= NGF_POWER_STOCK && c->role[NGF_CPU_ROLL] != 0xFFu && chance(m, 50u))
                return NGF_BTN_A | NGF_BTN_B;                       /* guard-cancel roll */
            if (f->spirit >= NGF_SPIRIT_MAX / 2u)
                return (uint8_t)(cpu_guard(m, who, 0) | NGF_BTN_C | NGF_BTN_D);   /* guard attack */
        }
        return cpu_guard(m, who, 0);
    case NGF_S_HIT:
        c->next = 0xFFu;
        return cpu_guard(m, who, 0);
    case NGF_S_JUMP:
        if (!f->air_used) {
            int16_t h = ngf_fighter_py(f);
            uint8_t k = c->role[(m->frame & 1u) ? NGF_CPU_AIR : NGF_CPU_AIR2];
            if (k == 0xFFu) k = c->role[NGF_CPU_AIR];
            if (k != 0xFFu && ((f->vh < 0 && h < 70 && dist < (uint16_t)(move_reach(&d->moves[k]) + 36u)) ||
                               (!o->grounded && dist < 60u && h > 20))) {
                cpu_push(c, d->moves[k].buttons, 1u);
                return ngf_cpu_input(m, who);
            }
        }
        return 0;
    case NGF_S_DOWN:
    case NGF_S_GETUP:
        c->plan = PLAN_WAKE;
        c->next = 0xFFu;
        return cpu_guard(m, who, 0);
    default:
        if (!ready) return 0;
        break;
    }
    c->next = 0xFFu;

    /* Getting up next to the other fighter: a reversal, or guard. */
    if (c->plan == PLAN_WAKE) {
        c->plan = PLAN_NONE;
        if (dist < 90u && chance(m, (uint16_t)(lv * 5u)) && cpu_move(m, who, c->role[NGF_CPU_ANTI_AIR], 1u))
            return ngf_cpu_input(m, who);
        if (dist < 100u && o->state == NGF_S_ATTACK) { cpu_hold(c, cpu_guard(m, who, 1), 12u, PLAN_GUARD); return c->hold; }
    }
    /* Landed from a jump-in: keep going while the other fighter reels. */
    if (c->plan == PLAN_JUMP_IN) {
        c->plan = PLAN_NONE;
        if ((o->state == NGF_S_HIT || o->state == NGF_S_BLOCK) && dist < 85u &&
            cpu_combo(m, who, (rnd(m) & 1u) ? NGF_CPU_HEAVY : NGF_CPU_LOW))
            return ngf_cpu_input(m, who);
    }
    /* A projectile on the way: jump it, roll through it, or guard. */
    if (shot < 140u) {
        if (shot > 60u && chance(m, (uint16_t)(10u + lv * 5u))) { cpu_jump(c, fl, dist < 150u ? 9u : 8u, 0); return ngf_cpu_input(m, who); }
        if (shot < 90u && chance(m, (uint16_t)(lv * 4u)) && cpu_move(m, who, c->role[NGF_CPU_ROLL], 1u)) return ngf_cpu_input(m, who);
        cpu_hold(c, cpu_guard(m, who, 0), 10u, PLAN_GUARD);
        return c->hold;
    }
    if (dist > 130u && dist < 260u && c->role[NGF_CPU_PROJECTILE] != 0xFFu && !has_projectile(m, who) &&
        chance(m, (uint16_t)(8u + lv * 4u)) && cpu_move(m, who, c->role[NGF_CPU_PROJECTILE], 0u))
        return ngf_cpu_input(m, who);
    /* A jump coming in: anti-air it, meet it in the air, or guard high. */
    if (!o->grounded && (o->state == NGF_S_JUMP || o->state == NGF_S_ATTACK) && o->state_timer >= react && dist < 150u) {
        uint8_t toward = (uint8_t)(dist < 70u || (fl ? o->vx > 0 : o->vx < 0));
        if (toward && chance(m, (uint16_t)(25u + lv * 7u))) {
            uint8_t k = 0xFFu;
            if ((f->power >= NGF_POWER_STOCK || f->max_timer) && c->role[NGF_CPU_SUPER] != 0xFFu &&
                move_invuln(d, &d->moves[c->role[NGF_CPU_SUPER]]) && dist < 90u && chance(m, 30u))
                k = c->role[NGF_CPU_SUPER];
            else if (c->role[NGF_CPU_ANTI_AIR] != 0xFFu && dist < 100u && (o->vh < 0 || dist < 60u))
                k = c->role[NGF_CPU_ANTI_AIR];
            else if (dist < 75u && (o->vh < 0 || dist < 50u))
                k = c->role[NGF_CPU_HEAVY];
            if (cpu_move(m, who, k, 1u)) return ngf_cpu_input(m, who);
            if (dist < 110u && ngf_fighter_py(o) > 30) { cpu_jump(c, fl, 8u, 0); return ngf_cpu_input(m, who); }
        }
        if (toward && chance(m, (uint16_t)(40u + lv * 5u))) {
            cpu_hold(c, cpu_guard(m, who, 1), 14u, PLAN_GUARD);
            return c->hold;
        }
    }
    /* A ground attack: punish it if it missed, otherwise guard. */
    if (o->state == NGF_S_ATTACK && o->move && o->grounded) {
        if (recovering(o) && o->state_timer >= react && dist < 95u && chance(m, (uint16_t)(20u + lv * 8u))) {
            if ((f->power >= NGF_POWER_STOCK || f->max_timer) && chance(m, (uint16_t)(lv * 5u)) &&
                cpu_move(m, who, c->role[NGF_CPU_SUPER], 1u)) return ngf_cpu_input(m, who);
            if (cpu_combo(m, who, dist < 70u ? NGF_CPU_HEAVY : NGF_CPU_LOW)) return ngf_cpu_input(m, who);
        }
        if (!recovering(o) && c->judge && dist < (uint16_t)(move_reach(o->move) + 45u)) {
            cpu_hold(c, cpu_guard(m, who, 1), 10u, PLAN_GUARD);
            return c->hold;
        }
    }
    /* A dizzy opponent: walk in and combo. */
    if (o->state == NGF_S_DIZZY) {
        if (dist > 70u) { cpu_hold(c, ngf_raw_from_numpad(6, fl), 6u, PLAN_NONE); return c->hold; }
        if (cpu_combo(m, who, NGF_CPU_HEAVY)) return ngf_cpu_input(m, who);
    }
    /* Call the striker. */
    if (m->assist[who] && m->assist_uses[who] && !m->striker[who].cooldown && !m->striker[who].active &&
        dist < 90u && chance(m, lv)) {
        cpu_push(c, NGF_BTN_A | NGF_BTN_C, 1u);
        cpu_push(c, 0, 1u);
        return ngf_cpu_input(m, who);
    }
    /* MAX mode when there is power to burn. */
    if (!f->max_timer && f->power >= NGF_POWER_STOCK && lv >= 3u && dist < 140u &&
        chance(m, (uint16_t)(f->power >= 2u * NGF_POWER_STOCK ? lv : lv / 3u + 1u))) {
        cpu_push(c, NGF_BTN_B | NGF_BTN_C, 1u);
        cpu_push(c, 0, 1u);
        return ngf_cpu_input(m, who);
    }
    /* Neutral: weigh what to do from the distance. */
    for (i = 0; i <= PICK_BACKDASH; i++) weight[i] = 0;
    {
        const NGFCpuProfile *p = &d->cpu_profile;
        uint8_t aggro = (uint8_t)(p->aggression / 16u + lv * 2u + (f->max_timer ? 12u : 0u));
        uint8_t poke = c->role[NGF_CPU_POKE] == 0xFFu ? 0u : move_reach(&d->moves[c->role[NGF_CPU_POKE]]);
        weight[PICK_WAIT] = (uint8_t)(lv >= 9u ? 3u : 3u + (9u - lv) * 3u);
        if (dist > 170u) {
            weight[PICK_WALK] = 20;
            weight[PICK_RUN] = (uint8_t)(10u + aggro);
            weight[PICK_HOP] = 4;
            if (c->role[NGF_CPU_RUSH] != 0xFFu && d->moves[c->role[NGF_CPU_RUSH]].range > 100u) weight[PICK_RUSH] = 4;
        } else if (dist > 80u) {
            weight[PICK_WALK] = 18;
            weight[PICK_RUN] = (uint8_t)(6u + aggro / 2u);
            weight[PICK_JUMP] = (uint8_t)(6u + p->jump / 10u + lv);
            weight[PICK_HOP] = (uint8_t)(4u + lv / 2u);
            if (poke && dist < (uint16_t)(poke + 20u)) weight[PICK_POKE] = (uint8_t)(8u + lv);
            if (c->role[NGF_CPU_RUSH] != 0xFFu && dist < (uint16_t)(d->moves[c->role[NGF_CPU_RUSH]].range + 40u))
                weight[PICK_RUSH] = (uint8_t)(4u + lv);
            weight[PICK_BACK] = 5;
        } else {
            if (c->role[NGF_CPU_THROW] != 0xFFu && throwable(o) && dist <= (uint16_t)(d->moves[c->role[NGF_CPU_THROW]].range + 4u))
                weight[PICK_THROW] = (uint8_t)(8u + lv * 2u);
            weight[PICK_COMBO] = (uint8_t)(10u + aggro);
            weight[PICK_LOW] = (uint8_t)(6u + aggro / 2u);
            if (c->role[NGF_CPU_BLOW] != 0xFFu) weight[PICK_BLOW] = 3;
            if (c->role[NGF_CPU_SWEEP] != 0xFFu) weight[PICK_SWEEP] = 5;
            if (c->role[NGF_CPU_POKE] != 0xFFu) weight[PICK_POKE] = 4;
            if (c->role[NGF_CPU_ROLL] != 0xFFu) weight[PICK_ROLL] = 2;
            weight[PICK_HOP] = (uint8_t)(2u + lv / 3u);
            weight[PICK_BACKDASH] = 3;
            weight[PICK_BACK] = 3;
        }
    }
    switch (cpu_pick(m, c, weight, PICK_BACKDASH + 1u)) {
    case PICK_WALK:
        cpu_hold(c, ngf_raw_from_numpad(6, fl), (uint8_t)(6u + rnd(m) % 16u), PLAN_NONE);
        return c->hold;
    case PICK_RUN:
        cpu_push(c, ngf_raw_from_numpad(6, fl), 1u);
        cpu_push(c, 0, 1u);
        cpu_hold(c, ngf_raw_from_numpad(6, fl), (uint8_t)(10u + rnd(m) % 20u), PLAN_NONE);
        return ngf_cpu_input(m, who);
    case PICK_JUMP:
        cpu_jump(c, fl, 9u, 0);
        return ngf_cpu_input(m, who);
    case PICK_HOP:
        cpu_jump(c, fl, 9u, 1u);
        return ngf_cpu_input(m, who);
    case PICK_POKE:
        if (cpu_move(m, who, c->role[NGF_CPU_POKE], 0)) return ngf_cpu_input(m, who);
        break;
    case PICK_RUSH:
        if (cpu_move(m, who, c->role[NGF_CPU_RUSH], 0)) return ngf_cpu_input(m, who);
        break;
    case PICK_BACK:
        cpu_hold(c, ngf_raw_from_numpad(4, fl), (uint8_t)(6u + rnd(m) % 10u), PLAN_NONE);
        return c->hold;
    case PICK_THROW:
        if (cpu_move(m, who, c->role[NGF_CPU_THROW], 0)) return ngf_cpu_input(m, who);
        break;
    case PICK_COMBO: {
        uint8_t starter = (uint8_t)(rnd(m) % 3u);
        if (cpu_combo(m, who, starter == 0u ? NGF_CPU_HEAVY : starter == 1u ? NGF_CPU_LOW : NGF_CPU_JAB))
            return ngf_cpu_input(m, who);
        break;
    }
    case PICK_LOW:
        if (cpu_combo(m, who, NGF_CPU_LOW)) return ngf_cpu_input(m, who);
        break;
    case PICK_BLOW:
        if (cpu_move(m, who, c->role[NGF_CPU_BLOW], 0)) return ngf_cpu_input(m, who);
        break;
    case PICK_SWEEP:
        if (cpu_move(m, who, c->role[NGF_CPU_SWEEP], 0)) return ngf_cpu_input(m, who);
        break;
    case PICK_ROLL:
        if (cpu_move(m, who, c->role[NGF_CPU_ROLL], 0)) return ngf_cpu_input(m, who);
        break;
    case PICK_BACKDASH:
        cpu_push(c, ngf_raw_from_numpad(4, fl), 1u);
        cpu_push(c, 0, 1u);
        cpu_push(c, ngf_raw_from_numpad(4, fl), 1u);
        cpu_push(c, 0, 1u);
        return ngf_cpu_input(m, who);
    default:
        break;
    }
    cpu_hold(c, f->state == NGF_S_CROUCH ? ngf_raw_from_numpad(2, fl) : 0u,
             (uint8_t)(3u + rnd(m) % (uint16_t)(lv >= 9u ? 6u : 24u - 2u * lv)), PLAN_NONE);
    return c->hold;
}

void NEOGEO_USER ngf_set_cpu(NGFMatch *m, uint8_t who, uint8_t level)
{
    NGFCpu *c;
    if (who > 1u) return;
    c = &m->cpu[who];
    c->level = level > 9u ? 9u : level;
    c->difficulty = 0u;
    c->len = c->pos = c->wait = 0;
    c->hold = 0;
    c->plan = PLAN_NONE;
    c->next = 0xFFu;
    c->learned = 0;
    m->f[who].cpu = (uint8_t)(level != 0u);
}

uint8_t NEOGEO_USER ngf_cpu_reaction_frames(uint8_t difficulty)
{
    static const uint8_t frames[7] = {20u, 16u, 12u, 10u, 8u, 6u, 4u};
    if (difficulty < 1u || difficulty > NGF_DIFFICULTY_LEVELS) return 0u;
    return frames[difficulty - 1u];
}

uint8_t NEOGEO_USER ngf_set_cpu_difficulty(NGFMatch *m, uint8_t who, uint8_t difficulty)
{
    static const uint8_t levels[7] = {1u, 2u, 4u, 5u, 6u, 8u, 9u};
    if (who > 1u || difficulty < 1u || difficulty > NGF_DIFFICULTY_LEVELS) return 0u;
    ngf_set_cpu(m, who, levels[difficulty - 1u]);
    m->cpu[who].difficulty = difficulty;
    return 1u;
}

static void NEOGEO_USER cpu_restart(NGFMatch *m, uint8_t who)
{
    if (m->cpu[who].difficulty) ngf_set_cpu_difficulty(m, who, m->cpu[who].difficulty);
    else ngf_set_cpu(m, who, m->cpu[who].level);
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

static void NEOGEO_USER striker_step(NGFMatch *m)
{
    uint8_t i;
    for (i = 0; i < 2u; i++) {
        NGFStriker *st = &m->striker[i];
        const NGFAnim *anim;
        const NGFFrame *fr;
        uint8_t done = 0;
        if (st->cooldown && !st->active) st->cooldown--;
        if (!st->active) continue;
        st->state_timer++;
        anim = &st->def->anims[st->anim];
        fr = &st->def->frames[anim->first + st->frame];
        if (++st->frame_timer >= fr->time) {
            st->frame_timer = 0;
            if ((uint8_t)(st->frame + 1u) < anim->count) {
                st->frame++;
                fr = &st->def->frames[anim->first + st->frame];
                if (fr->flags & NGF_FF_REHIT) st->move_hit = 0;
            } else if (st->state == 2u) done = 1;
            else st->frame = 0;
        }
        if (st->state == 1u) {
            st->x += st->vx;
            if (st->state_timer >= 12u) {
                st->state = 2u; st->state_timer = 0;
                st->anim = st->def->moves[st->move_index].anim;
                st->frame = st->frame_timer = 0;
            }
        } else if (st->state == 2u) {
            const NGFMove *mv = &st->def->moves[st->move_index];
            NGFFighter *target = &m->f[i ^ 1u];
            st->x += (int32_t)(st->facing_left ? -fr->dx : fr->dx) * 32;
            if ((fr->flags & NGF_FF_ACTIVE) && !st->move_hit && vulnerable(target)) {
                WBox box;
                if (frame_contact(st->def, st->anim, st->frame, mv, st->x, st->h, st->facing_left, target, &box)) {
                    HitParams hp;
                    st->move_hit = 1;
                    params_from_move(mv, &hp);
                    hp.damage = (uint8_t)((uint16_t)hp.damage * 3u / 4u);
                    apply_hit(m, i, &hp, (int16_t)((box.l + box.r) / 2), (int16_t)((box.b + box.t) / 2), 1);
                }
            }
            if (done) {
                st->state = 3u; st->state_timer = 0;
                st->anim = NGF_A_WALK_B; st->frame = st->frame_timer = 0;
            }
        } else {
            st->x -= st->vx;
            if (st->state_timer >= 12u) st->active = 0;
        }
    }
}

void NEOGEO_USER ngf_match_set_mode(NGFMatch *m, uint8_t mode)
{
    m->match_mode = mode <= NGF_MODE_TAG ? mode : NGF_MODE_1V1;
    if (m->match_mode != NGF_MODE_1V1) m->rounds_to_win = 1;
}

void NEOGEO_USER ngf_team_set(NGFMatch *m, uint8_t who, const NGFFighterDef *f1,
                              const NGFFighterDef *f2, const NGFFighterDef *f3)
{
    uint8_t count = 0, i;
    if (who > 1u) return;
    for (i = 0; i < 3u; i++) { m->team[who][i] = 0; m->team_life[who][i] = 0; }
    if (f1) m->team[who][count++] = f1;
    if (f2) m->team[who][count++] = f2;
    if (f3) m->team[who][count++] = f3;
    if (!count) m->team[who][count++] = m->f[who].def;
    m->team_count[who] = count; m->team_active[who] = 0;
    for (i = 0; i < count; i++) m->team_life[who][i] = m->team[who][i]->life;
    m->f[who].def = m->team[who][0];
    fighter_reset(m, who);
    if (m->state == NGF_M_FIGHT) { m->f[who].state = NGF_S_IDLE; set_anim(&m->f[who], NGF_A_IDLE, 1); }
    m->tag_reserve[who] = m->f[who].life;
    cpu_restart(m, who);
}

void NEOGEO_USER ngf_striker_set(NGFMatch *m, uint8_t who, const NGFFighterDef *def)
{
    if (who > 1u) return;
    m->assist[who] = def;
    m->assist_uses[who] = def ? 3u : 0u;
    m->striker[who].active = m->striker[who].cooldown = 0;
}

uint8_t NEOGEO_USER ngf_call_striker(NGFMatch *m, uint8_t who)
{
    NGFStriker *st;
    NGFFighter *f;
    const NGFFighterDef *d;
    uint8_t k, best = 0xFFu;
    if (who > 1u || m->state != NGF_M_FIGHT || !m->assist[who] || !m->assist_uses[who]) return 0;
    st = &m->striker[who]; f = &m->f[who]; d = m->assist[who];
    if (st->active || st->cooldown || !f->grounded || !f->life ||
        (f->state != NGF_S_IDLE && f->state != NGF_S_CROUCH && f->state != NGF_S_ATTACK && f->state != NGF_S_DASH)) return 0;
    if (d->combat && d->combat->assist_move) {
        best = (uint8_t)(d->combat->assist_move - 1u);
        if (best >= d->move_count || d->moves[best].projectile ||
            !d->moves[best].hit.w || !d->moves[best].hit.h) return 0u;
    }
    for (k = 0; k < d->move_count && !(d->combat && d->combat->assist_move); k++) {
        const NGFMove *mv = &d->moves[k];
        if (mv->kind != NGF_KIND_NORMAL || !(mv->stance & NGF_STANCE_STAND) || mv->projectile || !mv->hit.w || !mv->hit.h) continue;
        if (best == 0xFFu || mv->damage > d->moves[best].damage) best = k;
    }
    if (best == 0xFFu) return 0;
    st->active = 1; st->state = 1; st->state_timer = 0; st->owner = who;
    st->facing_left = f->facing_left;
    st->x = f->x - (int32_t)(sign_of(f) * 60) * 256; st->h = 0;
    st->vx = (int16_t)(sign_of(f) * 4 * 256); st->def = d;
    st->anim = NGF_A_DASH; st->frame = st->frame_timer = st->move_hit = 0;
    st->move_index = best; st->cooldown = 180u; m->assist_uses[who]--;
    event(m, NGF_EV_STRIKER, who, best, (int16_t)(st->x >> 8), 0);
    return 1;
}

uint8_t NEOGEO_USER ngf_tag_switch(NGFMatch *m, uint8_t who)
{
    NGFFighter *f;
    uint8_t next, n;
    if (who > 1u || m->state != NGF_M_FIGHT || m->match_mode != NGF_MODE_TAG || m->team_count[who] < 2u) return 0;
    f = &m->f[who];
    if (!f->grounded || !f->life || (f->state != NGF_S_IDLE && f->state != NGF_S_CROUCH)) return 0;
    m->team_life[who][m->team_active[who]] = f->life;
    next = m->team_active[who];
    for (n = 1u; n < m->team_count[who]; n++) {
        next = (uint8_t)((next + 1u) % m->team_count[who]);
        if (m->team_life[who][next]) break;
    }
    if (n == m->team_count[who]) return 0;
    m->tag_reserve[who] = f->life;
    m->team_active[who] = next; f->def = m->team[who][next];
    f->life = m->team_life[who][next]; f->guard = f->def->guard_max;
    f->move = 0; f->move_hit = f->counter = f->combo = 0; f->combo_damage = 0;
    f->move_contact = f->cancel_reject = f->juggle_points = f->relaunches = 0u;
    { uint8_t r; for (r = 0u; r < NGF_CANCEL_RULES; r++) f->route_uses[r] = 0u; }
    f->state = NGF_S_DASH; f->state_timer = 0;
    set_anim(f, NGF_A_DASH, 1); cpu_restart(m, who);
    event(m, NGF_EV_TAG, who, next, ngf_fighter_px(f), ngf_fighter_py(f));
    return 1;
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
    m->round_seconds = NGF_ROUND_SECONDS;
    m->winner = 0xFFu;
    m->round = 0;
    m->zoom_scale = 255u;
    m->match_mode = NGF_MODE_1V1;
    m->team[0][0] = p1; m->team_count[0] = 1; m->team_active[0] = 0; m->tag_reserve[0] = p1 ? p1->life : 0;
    m->team[1][0] = p2; m->team_count[1] = 1; m->team_active[1] = 0; m->tag_reserve[1] = p2 ? p2->life : 0;
    m->team_life[0][0] = p1->life; m->team_life[1][0] = p2->life;
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
    m->clock = (uint16_t)(m->round_seconds * NGF_SECOND_FRAMES);
    m->seconds = m->round_seconds;
    m->hitstop = m->super_freeze = 0;
    for (i = 0; i < 2u; i++) {
        m->striker[i].active = m->striker[i].cooldown = 0;
        cpu_restart(m, i);
    }
    m->round_winner = 0xFFu;
    m->camera_x = (int16_t)((m->stage_w - m->screen_w) / 2);
    event(m, NGF_EV_ROUND, 0, m->round, 0, 0);
}

uint8_t NEOGEO_USER ngf_set_round_seconds(NGFMatch *m, uint8_t seconds)
{
    if (seconds < 10u || seconds > 99u) return 0u;
    m->round_seconds = seconds;
    if (m->state == NGF_M_INTRO) {
        m->clock = (uint16_t)(seconds * NGF_SECOND_FRAMES);
        m->seconds = seconds;
    }
    return 1u;
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
    if (m->match_mode != NGF_MODE_1V1) {
        uint8_t i, exhausted[2] = {0, 0}, changed[2] = {0, 0};
        uint16_t life[2];
        for (i = 0; i < 2u; i++) {
            NGFFighter *f = &m->f[i];
            m->team_life[i][m->team_active[i]] = f->life;
            if (w == 2u || i != w) {
                uint8_t n, next = m->team_active[i];
                m->team_life[i][next] = 0;
                for (n = 1u; n < m->team_count[i]; n++) {
                    next = (uint8_t)((next + 1u) % m->team_count[i]);
                    if (m->team_life[i][next]) break;
                }
                if (n == m->team_count[i]) { exhausted[i] = 1; f->life = 0; }
                else {
                    m->team_active[i] = next; f->def = m->team[i][next];
                    f->life = m->team_life[i][next]; changed[i] = 1;
                }
            } else {
                uint16_t recover = (uint16_t)(f->def->life / 8u);
                f->life = f->def->life - f->life < recover ? f->def->life : (uint16_t)(f->life + recover);
            }
            life[i] = f->life;
        }
        if (!exhausted[0] && !exhausted[1]) {
            ngf_round_start(m);
            for (i = 0; i < 2u; i++) {
                m->f[i].life = life[i]; m->team_life[i][m->team_active[i]] = life[i];
                if (changed[i]) event(m, NGF_EV_TAG, i, m->team_active[i], ngf_fighter_px(&m->f[i]), 0);
            }
            m->assist_uses[0] = m->assist[0] ? 3u : 0u;
            m->assist_uses[1] = m->assist[1] ? 3u : 0u;
            return;
        }
        w = (uint8_t)(exhausted[0] && exhausted[1] ? 2u : exhausted[0] ? 1u : 0u);
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
    uint8_t paused = (uint8_t)(m->hitstop || m->super_freeze);
    raw[0] = raw1;
    raw[1] = raw2;
    m->frame++;
    for (i = 0; i < 2u; i++) {
        if (m->cpu[i].level) raw[i] = paused ? m->f[i].input.raw[m->f[i].input.head] : ngf_cpu_input(m, i);
        input_push(&m->f[i].input, raw[i], m->f[i].facing_left, paused);
    }
    if (m->hitstop) { m->hitstop--; return; }
    if (m->super_freeze) {
        m->super_freeze--;
        return;
    }
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
    for (i = 0; i < 2u; i++) {
        NGFFighter *f = &m->f[i];
        if (controls && f->grounded) {
            uint8_t can_assist = (uint8_t)(f->state == NGF_S_IDLE || f->state == NGF_S_CROUCH || f->state == NGF_S_DASH ||
                                           (f->state == NGF_S_ATTACK && f->state_timer <= 3u && f->move && f->move->kind == NGF_KIND_NORMAL));
            if (can_assist && (ngf_input_pressed(&f->input, NGF_BTN_A | NGF_BTN_C, 4u) ||
                               (m->match_mode != NGF_MODE_1V1 && ngf_input_pressed(&f->input, NGF_BTN_B | NGF_BTN_C, 4u) && f->power < NGF_POWER_STOCK))) {
                if (ngf_call_striker(m, i)) {
                    if (f->state == NGF_S_ATTACK) { f->state = NGF_S_IDLE; f->move = 0; set_anim(f, NGF_A_IDLE, 1); }
                    ngf_input_consume(&f->input, NGF_BTN_A | NGF_BTN_C | NGF_BTN_B);
                }
            }
            if (can_assist && ngf_input_pressed(&f->input, NGF_BTN_B | NGF_BTN_D, 4u)) {
                if (ngf_tag_switch(m, i)) {
                    if (f->state == NGF_S_ATTACK) { f->state = NGF_S_IDLE; f->move = 0; }
                    ngf_input_consume(&f->input, NGF_BTN_B | NGF_BTN_D);
                }
            }
        }
        fighter_step(m, i, controls);
        if (m->super_freeze) return;
    }
    if (controls) { striker_step(m); projectiles_step(m); }
    resolve_space(m);
    if (controls) resolve_attacks(m);

    /* Zoom scale computation */
    {
        uint16_t dist = ngf_distance(m);
        if (dist <= 160u) {
            m->zoom_scale = 255u;
        } else {
            uint16_t sc = 255u - (uint16_t)((dist - 160u) * 63u / 200u);
            m->zoom_scale = (uint8_t)(sc < 192u ? 192u : sc);
        }
    }

    switch (m->state) {
    case NGF_M_FIGHT:
        if (!m->f[0].life || !m->f[1].life) {
            m->state = NGF_M_KO;
            m->state_timer = 0;
            for (i = 0; i < NGF_PROJECTILES; i++) m->proj[i].active = 0;
            m->striker[0].active = m->striker[1].active = 0;
        } else if (!m->clock) {
            m->state = NGF_M_TIME_UP;
            m->state_timer = 0;
            for (i = 0; i < NGF_PROJECTILES; i++) m->proj[i].active = 0;
            m->striker[0].active = m->striker[1].active = 0;
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
            if (m->match_mode != NGF_MODE_1V1) {
                uint8_t exhausted0 = (uint8_t)(m->team_active[0] >= m->team_count[0] - 1u && m->team_life[0][m->team_active[0]] == 0);
                uint8_t exhausted1 = (uint8_t)(m->team_active[1] >= m->team_count[1] - 1u && m->team_life[1][m->team_active[1]] == 0);
                if (exhausted0 || exhausted1) {
                    m->state = NGF_M_OVER;
                    m->state_timer = 0;
                    m->winner = (uint8_t)(exhausted0 && exhausted1 ? 2u : exhausted0 ? 1u : 0u);
                } else {
                    ngf_round_start(m);
                    m->f[0].life = m->team_life[0][m->team_active[0]];
                    m->f[1].life = m->team_life[1][m->team_active[1]];
                    m->assist_uses[0] = m->assist[0] ? 3u : 0u;
                    m->assist_uses[1] = m->assist[1] ? 3u : 0u;
                }
            } else {
                uint8_t a = (uint8_t)(m->f[0].wins >= m->rounds_to_win), b = (uint8_t)(m->f[1].wins >= m->rounds_to_win);
                if (a || b || m->round >= 9u) {
                    m->state = NGF_M_OVER;
                    m->state_timer = 0;
                    m->winner = (uint8_t)(a && b ? 2u : a ? 0u : b ? 1u : 2u);
                } else {
                    ngf_round_start(m);
                }
            }
        }
        break;
    default:
        break;
    }
}

void NEOGEO_USER ngf_training_reset(NGFTraining *t, NGFMatch *m)
{
    uint8_t i;
    ngf_round_start(m);
    m->training = 1u;
    m->clock = 99u * NGF_SECOND_FRAMES;
    m->seconds = 99u;
    m->state = NGF_M_FIGHT;
    m->round = 1u;
    m->winner = m->round_winner = 0xFFu;
    m->f[0].wins = m->f[1].wins = 0u;
    for (i = 0u; i < 2u; i++) {
        m->f[i].state = NGF_S_IDLE;
        set_anim(&m->f[i], NGF_A_IDLE, 1u);
    }
    m->ev_count = 0u;
    t->idle[0] = t->idle[1] = 0u;
    t->record_pos = 0u;
}

void NEOGEO_USER ngf_training_init(NGFTraining *t, NGFMatch *m)
{
    uint16_t i;
    uint8_t *p = (uint8_t *)t;
    for (i = 0u; i < sizeof(*t); i++) p[i] = 0u;
    t->infinite_power = t->recover_life = 1u;
    ngf_set_cpu(m, 0u, 0u);
    ngf_set_cpu(m, 1u, 0u);
    ngf_training_reset(t, m);
}

uint8_t NEOGEO_USER ngf_training_set_dummy(NGFTraining *t, NGFMatch *m, uint8_t dummy)
{
    if (dummy >= NGF_DUMMY_COUNT) return 0u;
    t->dummy = dummy;
    t->record_pos = 0u;
    if (dummy == NGF_DUMMY_RECORD) t->record_size = 0u;
    if (dummy == NGF_DUMMY_CPU) ngf_set_cpu_difficulty(m, 1u, NGF_DIFFICULTY_DEFAULT);
    else ngf_set_cpu(m, 1u, 0u);
    return 1u;
}

void NEOGEO_USER ngf_training_step(NGFTraining *t, NGFMatch *m, uint8_t raw1, uint8_t raw2)
{
    uint8_t dummy = 0u, i;
    switch (t->dummy) {
    case NGF_DUMMY_GUARD:
        dummy = ngf_raw_from_numpad((m->f[0].move && m->f[0].move->level == NGF_LV_LOW) ? 1u : 4u,
                                   m->f[1].facing_left);
        break;
    case NGF_DUMMY_CROUCH: dummy = NGF_RAW_DOWN; break;
    case NGF_DUMMY_JUMP: dummy = NGF_RAW_UP; break;
    case NGF_DUMMY_PLAYER2: dummy = raw2; break;
    case NGF_DUMMY_RECORD:
        dummy = raw1;
        raw1 = 0u;
        if (t->record_size < NGF_TRAIN_RECORD_FRAMES) t->record[t->record_size++] = dummy;
        break;
    case NGF_DUMMY_PLAYBACK:
        if (t->record_size) {
            dummy = t->record[t->record_pos++];
            if (t->record_pos >= t->record_size) t->record_pos = 0u;
        }
        break;
    default: break;
    }
    m->training = 1u;
    m->clock = 99u * NGF_SECOND_FRAMES;
    m->seconds = 99u;
    ngf_match_step(m, raw1, dummy);
    for (i = 0u; i < 2u; i++) {
        NGFFighter *f = &m->f[i];
        if (t->infinite_power) { f->power = NGF_POWER_MAX; f->spirit = NGF_SPIRIT_MAX; }
        if (f->state == NGF_S_IDLE || f->state == NGF_S_CROUCH) {
            if (t->idle[i] < 60u) t->idle[i]++;
            if (t->idle[i] == 60u && t->recover_life) {
                f->life = f->def->life;
                f->guard = f->def->guard_max;
            }
        } else t->idle[i] = 0u;
    }
}

uint32_t NEOGEO_USER ngf_match_hash(const NGFMatch *m)
{
    uint32_t h = 2166136261u;
    uint8_t i, k;
#define MIX(v) (h = (h ^ (uint32_t)(v)) * 16777619u)
    for (i = 0; i < 2u; i++) {
        const NGFFighter *f = &m->f[i];
        const NGFInput *in = &f->input;
        const NGFCpu *cpu = &m->cpu[i];
        const NGFStriker *st = &m->striker[i];
        MIX(f->x); MIX(f->h); MIX((uint16_t)f->vx); MIX((uint16_t)f->vh); MIX((uint16_t)f->push_vx);
        MIX(f->state); MIX(f->state_timer); MIX(f->grounded); MIX(f->facing_left);
        MIX(f->anim); MIX(f->frame); MIX(f->frame_timer); MIX(f->move_hit); MIX(f->move_index);
        MIX(f->move_contact); MIX(f->juggle_points); MIX(f->relaunches);
        for (k = 0u; k < NGF_CANCEL_RULES; k++) MIX(f->route_uses[k]);
        MIX(f->move != 0); MIX(f->air_used); MIX(f->block_low); MIX(f->counter);
        MIX(f->life); MIX(f->power); MIX(f->guard); MIX(f->spirit); MIX(f->combo); MIX(f->combo_damage);
        MIX(f->wins); MIX(f->invuln); MIX(f->stun_frames); MIX(f->cpu);
        MIX(f->max_timer); MIX(f->max_super); MIX(f->hop);
        for (k = 0; f->def->name[k] && k < 80u; k++) MIX((uint8_t)f->def->name[k]);
        MIX(in->head); MIX(in->released); MIX(in->pending); MIX(in->release_pending); MIX(in->motion_frames);
        MIX(in->charge_back); MIX(in->charge_down); MIX(in->back_release); MIX(in->down_release);
        MIX(in->back_age); MIX(in->down_age);
        MIX(in->kara);
        for (k = 0; k < NGF_INPUT_FRAMES; k++) { MIX(in->raw[k]); MIX(in->press[k]); }
        for (k = 0; k < 4u; k++) { MIX(in->press_age[k]); MIX(in->release_age[k]); }
        MIX(cpu->len); MIX(cpu->pos); MIX(cpu->wait); MIX(cpu->level); MIX(cpu->hold);
        MIX(cpu->difficulty); MIX(cpu->plan); MIX(cpu->next); MIX(cpu->last); MIX(cpu->judge);
        MIX(cpu->learned == f->def);
        for (k = 0u; k < NGF_CPU_ROLES; k++) MIX(cpu->role[k]);
        for (k = 0; k < cpu->len; k++) MIX(cpu->seq[k]);
        MIX(m->team_count[i]); MIX(m->team_active[i]); MIX(m->assist_uses[i]);
        for (k = 0; k < 3u; k++) MIX(m->team_life[i][k]);
        MIX(st->active); MIX(st->cooldown); MIX(st->state); MIX(st->state_timer);
        MIX(st->x); MIX(st->h); MIX((uint16_t)st->vx); MIX(st->facing_left);
        MIX(st->anim); MIX(st->frame); MIX(st->frame_timer); MIX(st->move_index); MIX(st->move_hit);
    }
    for (i = 0; i < NGF_PROJECTILES; i++) {
        const NGFProjectile *p = &m->proj[i];
        MIX(p->active);
        if (p->active) {
            MIX(p->x); MIX(p->h); MIX(p->owner); MIX(p->facing_left);
            MIX(p->life); MIX(p->frame); MIX(p->frame_timer);
        }
    }
    MIX(m->clock); MIX(m->seconds); MIX(m->rng); MIX(m->round); MIX(m->state); MIX(m->state_timer);
    MIX(m->winner); MIX(m->round_winner); MIX(m->rounds_to_win); MIX(m->frame); MIX(m->hitstop);
    MIX(m->super_freeze); MIX(m->super_who); MIX(m->zoom_scale); MIX(m->match_mode);
    MIX(m->training); MIX(m->round_seconds);
    MIX(m->stage_w); MIX(m->camera_x); MIX(m->screen_w); MIX(m->floor_y);
#undef MIX
    return h;
}
