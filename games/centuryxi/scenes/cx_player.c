/*
 * cx_player.c -- Century XI: a man on the pitch.
 *
 * Movement, tackles, the one-on-one, and the man a player holds.
 *
 *   A       short pass along the ground, to the feet of the man the stick points at
 *   A again while it is on its way: he plays it back first time, a one-two
 *   A, stick behind him: a back-heel
 *   C       a long ball in the air; near goal a chip, from wide a cross
 *   A and C (classic: D too): a through ball into the space ahead of a runner
 *   B       shoot: wound up while held, struck when let go
 *   A, B as a pass comes to him: on first time, or a first-time shot (cx_human_step)
 *   A and B in a one-on-one: the feint
 *
 * Classic has three-button defending: A slide, B change man. Modern: A stand-tackle,
 * B slide, C change man, D sprint. The two
 * layouts drive the same football. Every man has weight: he walks away from a
 * standstill, builds to a run, and turns more slowly with the ball at his feet.
 */
#include "cx_internal.h"

/* Sixteen headings for a man running to a spot: the stick's eight and the eight
 * between them, so nobody zig-zags towards a point between two of the eight. The
 * sprite still faces the nearest of its eight. Components in 16ths. */
static const int8_t k_h16x[16] = {16, 15, 11, 6, 0, -6, -11, -15, -16, -15, -11, -6, 0, 6, 11, 15};
static const int8_t k_h16y[16] = {0, 6, 11, 15, 16, 15, 11, 6, 0, -6, -11, -15, -16, -15, -11, -6};

static uint8_t CX_CODE heading16(int16_t dx, int16_t dy)
{
    int16_t ax = cx_abs(dx), ay = cx_abs(dy);
    uint8_t q;
    /* the sector inside the quarter, from the slope: tan 11, 34, 56 and 79 degrees */
    if (ay * 5 <= ax) q = 0;
    else if (ay * 3 <= ax * 2) q = 1;
    else if (ay * 2 <= ax * 3) q = 2;
    else if (ay <= ax * 5) q = 3;
    else q = 4;
    if (dx >= 0) return (uint8_t)(dy >= 0 ? q : (16u - q) & 15u);
    return (uint8_t)(dy >= 0 ? 8u - q : 8u + q);
}

/* speed times a 16ths component, by shifts: 0, 6, 11, 15 or 16 */
static inline int16_t vel16(int8_t k, int16_t speed)
{
    int16_t a = (int16_t)(k < 0 ? -k : k), v;
    if (a == 16) v = speed;
    else if (a == 15) v = (int16_t)(speed - (speed >> 4));
    else if (a == 11) v = (int16_t)((int16_t)(speed * 11) >> 4);
    else if (a == 6) v = (int16_t)((speed >> 1) - (speed >> 3));
    else return 0;
    return (int16_t)(k < 0 ? -v : v);
}

void CX_CODE cx_move_to(CxMan *m, int16_t tx, int16_t ty, int16_t speed)
{
    int16_t dx = (int16_t)(tx - U(m->x)), dy = (int16_t)(ty - U(m->y));
    int16_t d = (int16_t)(cx_abs(dx) > cx_abs(dy) ? cx_abs(dx) + cx_abs(dy) / 2 : cx_abs(dy) + cx_abs(dx) / 2);
    uint8_t h;
    if (d < 4) { m->gx = m->gy = 0; return; }
    if (d < 24) speed = (int16_t)(((int16_t)((int16_t)(speed * d) * 11) >> 8) + 4);   /* ease in: about speed * d / 24 */
    m->dir = cx_dir_of(dx, dy);
    h = heading16(dx, dy);
    m->gx = vel16(k_h16x[h], speed);
    m->gy = vel16(k_h16y[h], speed);
}

/* A pass coming to him: he goes to where its path passes closest to him, a quarter of
 * the way back towards the ball (he meets it, he does not wait for it), instead of
 * chasing the spot the ball is at now, which a moving ball has already left. A slow
 * ball, or one already past him, he simply goes to; one in the air he meets where it
 * comes down. */
void CX_CODE cx_meet_pass(CxWorld *w, uint8_t i, int16_t speed)
{
    CxMan *m = &w->m[i];
    const CxBall *b = &w->ball;
    int16_t bx = U(b->x), by = U(b->y);
    int16_t dx = (int16_t)(U(m->x) - bx), dy = (int16_t)(U(m->y) - by), k;
    int32_t vv, dot;
    if (w->meet_t && (U(b->h) >= 6 || b->vh > 10)) { cx_move_to(m, w->meet_x, w->meet_y, speed); return; }
    vv = cx_mul(b->vx, b->vx) + cx_mul(b->vy, b->vy);
    dot = cx_mul(dx, b->vx) + cx_mul(dy, b->vy);
    if (vv < 24L * 24L || dot <= 0) {
        cx_move_to(m, (int16_t)(bx + U(b->vx) * 2), (int16_t)(by + U(b->vy) * 2), speed);
        return;
    }
    /* frames (x 64) until the ball is level with him, three quarters of the way: one
     * division, and the rest one-instruction multiplies (it runs every frame for the man
     * a pass is meant for) */
    dot = dot * 3 / 4;
    if (dot > 0x1FFFFFL) k = 32000;
    else {
        int32_t q = (dot << 6) / vv;
        k = (int16_t)(q > 32000 ? 32000 : q);
    }
    cx_move_to(m, (int16_t)(bx + (int16_t)(cx_mul(b->vx, k) >> 6)), (int16_t)(by + (int16_t)(cx_mul(b->vy, k) >> 6)), speed);
}

/* The same on the ball: he runs the way he faces, because that is where his touches
 * knock it, so a dribble keeps to the eight directions. */
void CX_CODE cx_move_to8(CxMan *m, int16_t tx, int16_t ty, int16_t speed)
{
    int16_t dx = (int16_t)(tx - U(m->x)), dy = (int16_t)(ty - U(m->y));
    int16_t d = (int16_t)(cx_abs(dx) > cx_abs(dy) ? cx_abs(dx) + cx_abs(dy) / 2 : cx_abs(dy) + cx_abs(dx) / 2);
    if (d < 4) { m->gx = m->gy = 0; return; }
    if (d < 24) speed = (int16_t)(((int16_t)((int16_t)(speed * d) * 11) >> 8) + 4);
    m->dir = cx_dir_of(dx, dy);
    m->gx = cx_vel(cx_kdx[m->dir], speed);
    m->gy = cx_vel(cx_kdy[m->dir], speed);
}

static int16_t CX_CODE pace_bonus(const CxMan *m) { return (int16_t)(m->pace >> 2); }

/* Facing the same way as the man he hits: he came from behind. */
uint8_t CX_CODE cx_from_behind(const CxMan *a, const CxMan *b)
{
    uint8_t gap = (uint8_t)((a->dir - b->dir) & 7u);
    return (uint8_t)(gap <= 1u || gap >= 7u);
}

void CX_CODE cx_tackle(CxWorld *w, uint8_t i, uint8_t slide)
{
    CxMan *m = &w->m[i];
    CxBall *b = &w->ball;
    m->state = slide ? CX_M_SLIDE : CX_M_TACKLE;
    m->timer = slide ? 22 : 10;
    if (slide) { m->vx = cx_vel(cx_kdx[m->dir], (int16_t)(42)); m->vy = cx_vel(cx_kdy[m->dir], (int16_t)(42)); }
    /* a keeper with the ball in his hands is not to be touched */
    if (!slide && b->owner != NONE && cx_team_of(b->owner) != cx_team_of(i) && w->m[b->owner].role == 0 &&
        cx_dist(m->x, m->y, w->m[b->owner].x, w->m[b->owner].y) < 22) {
        cx_foul(w, i, b->owner, 1);
        return;
    }
    if (!slide && b->owner != NONE && cx_team_of(b->owner) != cx_team_of(i) &&
        cx_dist(m->x, m->y, w->m[b->owner].x, w->m[b->owner].y) < 22 &&
        (cx_rnd(w) & 255u) < (uint16_t)(130 + m->tack * 8 + (w->m[b->owner].type == CX_T_DRIBBLE ? 0u : 20u) - w->m[b->owner].ctrl * 3u)) {
        uint8_t was = b->owner;
        b->owner = NONE;
        b->vx = cx_vel(cx_kdx[m->dir], (int16_t)(30));
        b->vy = cx_vel(cx_kdy[m->dir], (int16_t)(30));
        b->last = i;
        w->m[was].timer = 12;          /* a moment before he can take it back */
        w->ev |= CX_EV_TACKLE;
    } else if (!slide && b->owner != NONE && cx_team_of(b->owner) != cx_team_of(i) &&
               cx_dist(m->x, m->y, w->m[b->owner].x, w->m[b->owner].y) < 16 && (cx_rnd(w) & 15u) == 0) {
        /* he missed the ball and caught the man */
        uint8_t carrier = b->owner;
        CxMan *o = &w->m[carrier];
        b->owner = NONE;
        b->vx = o->vx;
        b->vy = o->vy;
        o->state = CX_M_DOWN;
        o->timer = 30;
        o->vx = o->vy = 0;
        cx_foul(w, i, carrier, 0);       /* a trip, not a lunge: no card by itself */
    }
}

/* A sliding man reaching the ball carrier: the ball first, or the man. */
static void CX_CODE slide_contact(CxWorld *w, uint8_t i)
{
    CxMan *m = &w->m[i];
    CxBall *b = &w->ball;
    uint8_t carrier = b->owner;
    CxMan *o = &w->m[carrier];
    uint8_t behind = cx_from_behind(m, o);
    /* a clean take needs the angle; from behind there is little of the ball to win */
    uint16_t clean = (uint16_t)((behind ? 60u : 170u) + m->tack * 5u);
    b->owner = NONE;
    b->vx = cx_vel(cx_kdx[m->dir], (int16_t)(34));
    b->vy = cx_vel(cx_kdy[m->dir], (int16_t)(34));
    b->last = i;
    o->state = CX_M_DOWN;
    o->timer = 40;
    o->vx = o->vy = 0;
    w->ev |= CX_EV_TACKLE;
    if ((cx_rnd(w) & 255u) >= clean) cx_foul(w, i, carrier, behind);
    /* a clean tackle in the box, and he goes down as if shot: he hopes nobody saw */
    else if (cx_abs((int16_t)(U(o->x) - (cx_attacks_right(w, cx_team_of(carrier)) ? CX_LEN : 0))) < CX_BOX_D &&
             (cx_rnd(w) & 7u) == 0) cx_dive(w, carrier);
}

/* The states other than running, each with its own pace and way out (rare: kept apart so
 * cx_man_step, called for 22 men a tick, saves few registers). */
static void __attribute__((noinline)) CX_CODE man_state(CxWorld *w, uint8_t i, CxMan *m)
{
    CxBall *b = &w->ball;
    switch (m->state) {
    case CX_M_KICK:
    case CX_M_TACKLE:
    case CX_M_HEAD:
    case CX_M_VOLLEY:
        m->vx = (int16_t)(m->vx / 2);
        m->vy = (int16_t)(m->vy / 2);
        if (!m->timer) m->state = CX_M_RUN;
        break;
    case CX_M_BIKE:
        m->vx = m->vy = 0;
        if (!m->timer) { m->state = CX_M_DOWN; m->timer = 16; }
        break;
    case CX_M_FLY:
        m->vx = (int16_t)(m->vx - m->vx / 10);
        m->vy = (int16_t)(m->vy - m->vy / 10);
        if (!m->timer) { m->state = CX_M_DOWN; m->timer = 12; m->vx = m->vy = 0; }
        break;
    case CX_M_ROLL:
    case CX_M_ARGUE:
        m->vx = m->vy = 0;
        if (!m->timer) m->state = CX_M_RUN;
        break;
    case CX_M_WIND:
        m->vx = (int16_t)(m->vx / 2);
        m->vy = (int16_t)(m->vy / 2);
        if (!m->timer) {
            m->state = CX_M_RUN;
            if (b->owner == i) cx_shoot(w, i, 26, (int8_t)m->aimv);
        }
        break;
    case CX_M_THROWN:
        m->vx = m->vy = m->gx = m->gy = 0;
        if (!m->timer) m->state = CX_M_RUN;
        break;
    case CX_M_SLIDE:
        m->vx = (int16_t)(m->vx - m->vx / 10);
        m->vy = (int16_t)(m->vy - m->vy / 10);
        if (b->owner == NONE && U(b->h) < 8 && cx_dist(m->x, m->y, b->x, b->y) < 16) {
            b->vx = (int16_t)(cx_vel(cx_kdx[m->dir], (int16_t)(40)) + m->vx);
            b->vy = (int16_t)(cx_vel(cx_kdy[m->dir], (int16_t)(40)) + m->vy);
            b->last = i;
            w->ev |= CX_EV_TACKLE;
        } else if (b->owner != NONE && cx_team_of(b->owner) != cx_team_of(i) &&
                   cx_dist(m->x, m->y, w->m[b->owner].x, w->m[b->owner].y) < 18) {
            if (w->m[b->owner].role == 0) cx_foul(w, i, b->owner, 1);      /* straight through the keeper */
            else slide_contact(w, i);
        } else if ((w->frame & 1u) == 0) {
            /* the ball has gone and he takes the man anyway: a late one */
            uint8_t k, o = (uint8_t)((cx_team_of(i) ^ 1u) * 11u);
            for (k = o; k < o + 11u; k++) {
                CxMan *v = &w->m[k];
                if (v->state == CX_M_OFF || v->state == CX_M_DOWN || v->state == CX_M_ROLL) continue;
                if (cx_dist(m->x, m->y, v->x, v->y) < 13) {
                    /* he may ride it (half the time), and a booking needs it from behind: every
                     * late slide was a yellow card, four in one play-test half */
                    if (cx_rnd(w) & 1u) break;
                    v->state = CX_M_DOWN;
                    v->timer = 40;
                    v->vx = v->vy = v->gx = v->gy = 0;
                    cx_foul(w, i, k, cx_from_behind(m, v));
                    break;
                }
            }
        }
        if (w->phase != CX_P_PLAY) break;
        if (!m->timer) { m->state = CX_M_DOWN; m->timer = 14; m->vx = m->vy = 0; }
        break;
    case CX_M_DOWN:
    case CX_M_SAD:
        m->vx = m->vy = 0;
        if (!m->timer) m->state = CX_M_RUN;
        break;
    case CX_M_BEATEN:
        m->vx = (int16_t)(m->vx / 2);
        m->vy = (int16_t)(m->vy / 2);
        if (!m->timer) m->state = CX_M_RUN;
        break;
    case CX_M_FEINT:
        if (!m->timer) m->state = CX_M_RUN;
        break;
    case CX_M_CELEB:
        m->vx = (int16_t)(m->vx - m->vx / 16);
        m->vy = (int16_t)(m->vy - m->vy / 16);
        if (!m->timer) m->state = CX_M_RUN;
        break;
    case CX_M_DIVE:
        m->vx = (int16_t)(m->vx - m->vx / 12);
        m->vy = (int16_t)(m->vy - m->vy / 12);
        if (!m->timer) { m->state = CX_M_DOWN; m->timer = 20; }
        break;
    default:
        break;
    }
}

/* Picking the ball up: a loose ball at his feet (the test is made in cx_man_step). */
static void __attribute__((noinline)) CX_CODE man_pickup(CxWorld *w, uint8_t i, CxMan *m)
{
    CxBall *b = &w->ball;
    int16_t pace = (int16_t)(cx_abs(b->vx) + cx_abs(b->vy));
    if (U(b->h) >= REACH) pace = (int16_t)(pace / 2);   /* on the chest: the drop takes the sting out */
    if (cx_keepers_ball(w, i)) return;          /* the keeper's: it is his to gather */
    if (cx_offside_check(w, i)) return;
    /* the first touch: a good one kills the ball, a poor one lets it get away from
     * him. The harder it comes and the closer a man is on him, the harder it is;
     * the chance comes from his touch, his side's level and the score (the brain). */
    if (pace > 44) {
        int16_t chance = (int16_t)((int16_t)cx_touch_chance(w, i) - (pace - 44) / 2);
        uint8_t k, o = (uint8_t)((cx_team_of(i) ^ 1u) * 11u);
        for (k = (uint8_t)(o + 1u); k < o + 11u; k++)
            if (cx_active(w, k) && cx_dist(w->m[k].x, w->m[k].y, m->x, m->y) < 26) { chance = (int16_t)(chance - 20); break; }
        if ((int16_t)(cx_rnd(w) & 255u) >= chance) {
            /* off his shin: a third of its pace kept, knocked a step aside */
            b->vx = (int16_t)(b->vx / 3 + (int16_t)(cx_rnd(w) % 25u) - 12);
            b->vy = (int16_t)(b->vy / 3 + (int16_t)(cx_rnd(w) % 25u) - 12);
            b->last = i;
            m->timer = 8;
            w->ev |= CX_EV_BOUNCE;
            return;
        }
    }
    {
        uint8_t t = cx_team_of(i), go = (uint8_t)(w->pass_to == i ? w->first_go[t] : 0u);
        cx_ball_take(w, i);
        m->timer = 12;
        /* the player asked for it back as it travelled: first time, a one-two; or called it on
         * first time, a pass the stick's way or a shot, without a touch to set it */
        if (w->human[t] && go == 2u) { m->timer = 0; cx_shoot(w, i, 30, 0); w->ev |= CX_EV_SPECIAL; return; }
        if (w->human[t] && go == 1u) { m->timer = 0; m->dir = w->first_dir[t]; cx_pass(w, i, 1, 0); w->ev |= CX_EV_SPECIAL; return; }
        if (w->wall_go[t] && w->wall_man[t] != i && w->wall_man[t] != NONE) cx_wall_return(w, i);
    }
}

static inline __attribute__((always_inline)) void man_step(CxWorld *w, uint8_t i)
{
    CxMan *m = &w->m[i];
    const CxBall *b = &w->ball;
    if (m->state == CX_M_OFF) return;
    if (m->timer) m->timer--;
    if (m->state == CX_M_RUN) {
        /* weight: he reaches the speed he wants a little at a time, more slowly with the ball */
        int16_t acc = (int16_t)(b->owner == i ? 7 + (m->ctrl >> 2) : 9);   /* a good dribbler turns quicker with it */
        m->vx = (int16_t)(m->vx + cx_clamp((int16_t)(m->gx - m->vx), (int16_t)-acc, acc));
        m->vy = (int16_t)(m->vy + cx_clamp((int16_t)(m->gy - m->vy), (int16_t)-acc, acc));
    } else man_state(w, i, m);
    m->x = (int16_t)(m->x + m->vx);
    m->y = (int16_t)(m->y + m->vy);
    m->x = cx_clamp(m->x, (int16_t)(-40 * Q), (int16_t)((CX_LEN + 40) * Q));
    m->y = cx_clamp(m->y, (int16_t)(-24 * Q), (int16_t)((CX_WID + 24) * Q));
    /* a keeper stays in play: holding the ball behind his own line would be a goal */
    if (m->role == 0) m->x = cx_clamp(m->x, (int16_t)(6 * Q), (int16_t)((CX_LEN - 6) * Q));
    /* his legs keep time with the ground he covers: no skating, walking or running */
    {
        uint16_t pace = (uint16_t)(cx_abs(m->vx) + cx_abs(m->vy));
        uint16_t acc2 = (uint16_t)(m->stride + pace);
        if (acc2 >= 96u) { acc2 = (uint16_t)(acc2 - 96u); m->anim++; }
        m->stride = (uint8_t)acc2;
    }
    /* picking the ball up */
    /* the man a ball is meant for takes a dropping one on his chest or thigh; anyone else
     * only once it is down at his feet */
    if (b->owner == NONE && (m->state == CX_M_RUN || m->state == CX_M_CELEB) && !m->timer && m->role != 0 &&
        (U(b->h) < REACH || (w->pass_to == i && b->vh <= 0 && U(b->h) < 30)) &&
        cx_dist(m->x, m->y, b->x, b->y) < (w->pass_to == i ? 15 : 12) && w->phase == CX_P_PLAY)
        man_pickup(w, i, m);
}

void CX_CODE cx_man_step(CxWorld *w, uint8_t i) { man_step(w, i); }

/* Every man's step in turn, stopping at a whistle: one call a tick, not 22. */
void CX_CODE cx_men_step(CxWorld *w)
{
    uint8_t i;
    for (i = 0; i < CX_MEN; i++) {
        man_step(w, i);
        if (w->phase != CX_P_PLAY) return;
    }
}

/* ---------------------------------------------------------------- feints */
/* The one-on-one: the man on the ball and an opponent squaring up in front
 * of him. Nothing here is decided by an animation: the distance, how each man
 * is moving and their ratings give a number, and the reason is kept so the
 * player can be told why it went the way it did. */
void CX_CODE cx_duel_scan(CxWorld *w)
{
    uint8_t c = w->ball.owner, k, best = NONE;
    int16_t bd = 50;
    w->duel_a = w->duel_d = NONE;
    if (c == NONE || w->m[c].role == 0 || w->m[c].state != CX_M_RUN) return;
    for (k = 0; k < CX_MEN; k++) {
        const CxMan *d = &w->m[k];
        int16_t e, dx, dy;
        if (cx_team_of(k) == cx_team_of(c) || d->role == 0 || !cx_active(w, k) || d->state == CX_M_DOWN ||
            d->state == CX_M_BEATEN) continue;
        e = cx_dist(d->x, d->y, w->m[c].x, w->m[c].y);
        if (e >= bd) continue;
        dx = (int16_t)(U(d->x) - U(w->m[c].x));
        dy = (int16_t)(U(d->y) - U(w->m[c].y));
        if ((int32_t)dx * cx_kdx[w->m[c].dir] + (int32_t)dy * cx_kdy[w->m[c].dir] <= 0) continue;     /* behind him */
        bd = e;
        best = k;
    }
    if (best != NONE) { w->duel_a = c; w->duel_d = best; }
}

void CX_CODE cx_feint(CxWorld *w, uint8_t a, uint8_t d, uint8_t way)
{
    CxMan *m = &w->m[a], *o = &w->m[d];
    CxBall *b = &w->ball;
    int16_t gap = cx_dist(m->x, m->y, o->x, o->y);
    int16_t speed = (int16_t)(cx_abs(m->vx) + cx_abs(m->vy));
    int16_t att = (int16_t)(m->ctrl * 2 + speed / 16), def = (int16_t)(o->tack * 2);
    int16_t margin;
    uint8_t why = CX_WHY_SKILL, facing = (uint8_t)(((m->dir - o->dir) & 7u) == 4u);
    uint8_t dir = m->dir;
    if (gap >= 18 && gap <= 36) { att = (int16_t)(att + 3); why = CX_WHY_TIMING; }
    else if (gap < 18) { att = (int16_t)(att - 5); why = CX_WHY_CLOSE; }
    else { att = (int16_t)(att - 2); why = CX_WHY_FAR; }
    if (facing) def = (int16_t)(def + 3);
    if (o->state == CX_M_SLIDE || o->state == CX_M_TACKLE) { att = (int16_t)(att + 10); why = CX_WHY_COMMITTED; }
    if (way == CX_WAY_BACK && (cx_abs(o->vx) + cx_abs(o->vy)) > 24) att = (int16_t)(att + 3);
    margin = (int16_t)(att - def + (int16_t)(cx_rnd(w) & 7u) - (int16_t)(cx_rnd(w) & 7u));
    /* the way he goes: across the man, or back on himself */
    if (way == CX_WAY_UP) dir = (uint8_t)(cx_kdx[m->dir] >= 0 ? 7u : 5u);
    else if (way == CX_WAY_DOWN) dir = (uint8_t)(cx_kdx[m->dir] >= 0 ? 1u : 3u);
    else if (way == CX_WAY_BACK) dir = (uint8_t)((m->dir + 4u) & 7u);
    w->ev |= CX_EV_FEINT;
    if (margin >= 8 && way == CX_WAY_SWERVE) w->ev |= CX_EV_NUTMEG;     /* straight through his legs */
    if (margin >= 3) {
        w->duel_res = CX_DUEL_BEAT;
        o->state = CX_M_BEATEN;
        o->timer = 28;
        m->dir = dir;
        m->state = CX_M_FEINT;
        m->timer = 8;
        m->vx = cx_vel(cx_kdx[dir], (int16_t)((SPRINT_V + 6)));
        m->vy = cx_vel(cx_kdy[dir], (int16_t)((SPRINT_V + 6)));
    } else if (margin >= 0) {
        w->duel_res = CX_DUEL_STEP;
        o->state = CX_M_BEATEN;
        o->timer = 10;
        m->dir = dir;
        m->state = CX_M_FEINT;
        m->timer = 6;
        m->vx = cx_vel(cx_kdx[dir], (int16_t)(RUN_V));
        m->vy = cx_vel(cx_kdy[dir], (int16_t)(RUN_V));
    } else if (margin >= -4) {
        w->duel_res = CX_DUEL_FAIL;
        if (why == CX_WHY_TIMING || why == CX_WHY_SKILL) why = CX_WHY_READ;
        m->state = CX_M_FEINT;
        m->timer = 16;
        m->vx = m->vy = 0;
    } else {
        w->duel_res = CX_DUEL_LOST;
        if (why == CX_WHY_TIMING || why == CX_WHY_SKILL) why = CX_WHY_READ;
        b->owner = NONE;
        b->last = d;
        b->vx = cx_vel(cx_kdx[o->dir], (int16_t)(28));
        b->vy = cx_vel(cx_kdy[o->dir], (int16_t)(28));
        m->state = CX_M_FEINT;
        m->timer = 18;
        m->vx = m->vy = 0;
        w->ev |= CX_EV_TACKLE;
    }
    w->duel_why = why;
}

/* ---------------------------------------------------------------- the player */
static uint8_t CX_CODE stick_dir(uint16_t held, uint8_t *dir)
{
    int16_t dx = (int16_t)((held & CX_RIGHT) ? 1 : ((held & CX_LEFT) ? -1 : 0));
    int16_t dy = (int16_t)((held & CX_DOWN) ? 1 : ((held & CX_UP) ? -1 : 0));
    if (!dx && !dy) return 0;
    *dir = cx_dir_of(dx, dy);
    return 1;
}

static uint8_t CX_CODE feint_way(const CxMan *m, uint16_t held)
{
    uint8_t back = (uint8_t)((cx_kdx[m->dir] > 0 && (held & CX_LEFT)) || (cx_kdx[m->dir] < 0 && (held & CX_RIGHT)));
    if (back) return CX_WAY_BACK;
    if (held & CX_UP) return CX_WAY_UP;
    if (held & CX_DOWN) return CX_WAY_DOWN;
    return CX_WAY_SWERVE;
}

/* Two buttons at once are a different pass: a first press of A or C waits two frames for
 * its partner. B never waits, unless A is down with it. */
/* A trick with nobody square in front (A and B together): back on the stick, the drag
 * back, turning with the ball under his sole; up or down, the step-over, a burst to that
 * side with the ball; otherwise the rainbow flick, the ball lifted over his own head
 * for him to run on to. With a man in front it is a feint (cx_feint) instead. */
void CX_CODE cx_trick(CxWorld *w, uint8_t i, uint16_t held)
{
    CxMan *m = &w->m[i];
    CxBall *b = &w->ball;
    int8_t fx = cx_kdx[m->dir], fy = cx_kdy[m->dir];
    uint8_t back = (uint8_t)((fx > 0 && (held & CX_LEFT)) || (fx < 0 && (held & CX_RIGHT)) ||
                             (fx == 0 && fy > 0 && (held & CX_UP)) || (fx == 0 && fy < 0 && (held & CX_DOWN)));
    w->ev |= CX_EV_FEINT;
    if (back) {
        m->dir = (uint8_t)((m->dir + 4u) & 7u);
        m->vx = m->vy = 0;
        m->state = CX_M_FEINT;
        m->timer = 10;
        return;
    }
    if (fx != 0 && (held & (CX_UP | CX_DOWN))) {
        int16_t side = (int16_t)((held & CX_UP) ? -1 : 1);
        m->state = CX_M_FEINT;
        m->timer = 9;
        m->vx = cx_vel(fx, 18);
        m->vy = (int16_t)(side * (SPRINT_V + 4));
        return;
    }
    /* the rainbow: over his head, dropping a couple of strides ahead */
    b->owner = NONE;
    b->last = i;
    b->h = 4 * Q;
    b->vx = cx_vel(fx, 40);
    b->vy = cx_vel(fy, 40);
    b->vh = 32;
    b->curve = 0;
    b->fast = 0;
    m->timer = 10;                              /* he must let it come over first */
    w->pass_to = i;
    w->ev |= CX_EV_SPECIAL;
}

static void CX_CODE on_ball(CxWorld *w, uint8_t t, uint8_t i, uint16_t held, uint16_t pressed, uint8_t moving, uint8_t turned)
{
    CxMan *m = &w->m[i];
    int8_t aim = (int8_t)((held & CX_UP) ? -1 : ((held & CX_DOWN) ? 1 : 0));
    uint16_t cur = (uint16_t)((pressed & (CX_A | CX_B | CX_C)) | w->chord_btn[t]);
    uint16_t both = (uint16_t)(CX_A | CX_B), thru = (uint16_t)(CX_A | CX_C);
    w->recv_dir[t] = moving ? m->dir : 0xFFu;
    if (w->classic_controls && (pressed & CX_D)) { w->chord_btn[t] = 0; cx_through(w, i, moving); return; }
    if (!cur) return;
    if ((cur & both) == both) {
        w->chord_btn[t] = 0;
        if (w->duel_a == i) cx_feint(w, i, w->duel_d, feint_way(m, held));
        else cx_trick(w, i, held);
        return;
    }
    if ((cur & thru) == thru) { w->chord_btn[t] = 0; cx_through(w, i, moving); return; }
    if (cur & CX_B) {
        /* B winds the shot up while it is held and strikes it when it is let go: a tap
         * places it, the full wind-up (the gauge) is a thunderbolt */
        w->chord_btn[t] = 0;
        w->charge[t] = 1;
        return;
    }
    if (!w->chord_btn[t]) { w->chord_btn[t] = (uint8_t)cur; w->chord[t] = 2; return; }
    if (--w->chord[t]) return;
    w->chord_btn[t] = 0;
    if (cur & CX_A) {
        if (turned) { cx_backheel(w, i); return; }
        /* the passer may run on for the return */
        w->wall_man[t] = i;
        w->wall_t[t] = 50;
        w->wall_go[t] = 0;
        /* the stick's way; standing still, the way he faces, else the man most free */
        cx_pass(w, i, moving ? 1u : 2u, 0);
    } else {
        /* C: from the edge of the box, central, it lifts it over the keeper; wide of the box a
         * cross, anywhere else a long ball to the man up the pitch the stick points at */
        int16_t gx = cx_attacks_right(w, t) ? CX_LEN : 0;
        int16_t wide = cx_abs((int16_t)(U(m->y) - CX_MID_Y)), range = cx_abs((int16_t)(gx - U(m->x)));
        if (range < 360 && wide < 160) cx_chip(w, i, aim);
        else if (range < 460 && wide >= 170) cx_cross(w, i);       /* wide of the box: a cross */
        else cx_pass(w, i, moving, 1);
    }
}

void CX_CODE cx_human_step(CxWorld *w, uint8_t t, uint16_t held, uint16_t pressed)
{
    uint8_t i = w->ctrl[t];
    CxMan *m = &w->m[i];
    CxBall *b = &w->ball;
    uint8_t dir = m->dir, moving, turned;
    int16_t speed;
    if (m->state == CX_M_HOLD) {
        if (pressed & (CX_A | CX_B | CX_C)) { m->state = CX_M_RUN; cx_pass(w, i, 1, (uint8_t)!(pressed & CX_A)); }
        return;
    }
    if (m->state != CX_M_RUN) { w->charge[t] = 0; return; }
    if (w->charge[t] && w->charge[t] < 4u && (pressed & CX_A) && b->owner == i) {
        /* B and A together, B a hair first: the trick, not a shot */
        w->charge[t] = 0;
        if (w->duel_a == i) cx_feint(w, i, w->duel_d, feint_way(m, held));
        else cx_trick(w, i, held);
        return;
    }
    if (w->charge[t]) {
        if (b->owner != i) w->charge[t] = 0;                       /* he lost it first */
        else if ((held & CX_B) && w->charge[t] < CX_CHARGE_MAX) w->charge[t]++;
        else {
            /* a tap places it; held all the way it is a drive that rises, and harder still
             * from a man at full tilt */
            uint8_t power = (uint8_t)(16u + w->charge[t] + ((cx_abs(m->vx) + cx_abs(m->vy)) > 40 ? 4u : 0u));
            int8_t aim = (int8_t)((held & CX_UP) ? -1 : ((held & CX_DOWN) ? 1 : 0));
            w->charge[t] = 0;
            cx_shoot(w, i, power, aim);
            return;
        }
    }
    moving = stick_dir(held, &dir);
    {
        /* From a standstill a man walks, and the longer the stick is held the more he runs.
         * With the ball a walk keeps it under his foot; at a run he has to chase it. */
        uint8_t sprint = (uint8_t)((held & CX_D) && !w->classic_controls && m->sta > 24u);
        uint8_t has = (uint8_t)(b->owner == i);
        int16_t walk = (int16_t)(has ? 17 : 22);
        int16_t top = (int16_t)(sprint ? SPRINT_V + pace_bonus(m) : (has ? DRIBBLE_V + 2 + (m->pace >> 3) : RUN_V + 2 + (m->pace >> 3)));
        uint8_t back = (uint8_t)(((dir - m->dir) & 7u) == 4u);
        if (m->type == CX_T_DRIBBLE && has) top = (int16_t)(top + 2);
        if (moving) { if (m->run_t < 16u) m->run_t++; if (back) m->run_t = (uint8_t)(m->run_t >> 1); }
        else m->run_t = 0;
        speed = (int16_t)(walk + (((top - walk) * m->run_t) >> 4));
        if (sprint && (w->frame & 3u) == 0 && m->sta) m->sta--;
        speed = (int16_t)(speed - cx_tired(m));
        turned = (uint8_t)(back && has);
    }
    if (moving) {
        if (!turned) m->dir = dir;
        m->gx = cx_vel(cx_kdx[dir], speed);
        m->gy = cx_vel(cx_kdy[dir], speed);
    } else {
        m->gx = m->gy = 0;
    }
    /* a press held back for the A+B chord dies with the ball: kept, it fired a pass the moment
     * the side next won the ball */
    if (b->owner != i) w->chord_btn[t] = 0;
    if (b->owner == i) {
        on_ball(w, t, i, held, pressed, moving, turned);
        if (turned && w->ball.owner == i) m->dir = dir;       /* no back-heel: he simply turns */
        return;
    }
    /* his pass is on its way: A asks for it back, and the man it is meant for moves to meet it */
    if (w->pass_to == i && b->owner == NONE) {
        uint8_t lofted = (uint8_t)(U(b->h) >= 6 || b->vh > 10);
        /* as it comes along the ground: A with the stick pointing on plays it on first time
         * that way, A with the stick at rest gives it back to the passer (the one-two), B
         * hits it first time at goal */
        if (!lofted && (pressed & CX_B)) { w->first_go[t] = 2; w->wall_go[t] = 0; }
        else if (pressed & CX_A) {
            uint8_t sd = m->dir;
            if (!lofted && stick_dir(held, &sd)) { w->first_go[t] = 1; w->first_dir[t] = sd; w->wall_go[t] = 0; }
            else { w->wall_go[t] = 1; w->first_go[t] = 0; }
        }
        /* he goes to meet it whatever the stick says, along the ground or where it drops:
         * steered away, he let the defender in (half a player's passes were lost that way in
         * play tests, and most of his keeper's long kicks). D sprints to it. */
        if (!w->aerial_t[t]) {
            uint8_t sprint = (uint8_t)((held & CX_D) && !w->classic_controls && m->sta > 24u);
            cx_meet_pass(w, i, (int16_t)((sprint ? SPRINT_V : RUN_V + 2) + (m->pace >> 3)));
        }
        if (!lofted) return;            /* A here asked for it back: no tackle on nobody */
    }
    /* without the ball: a ball in the air. A or B pressed as it comes says he goes for
     * it: for the next second or so he runs to where it drops to head height and meets
     * it there, B at goal (header, volley or the overhead scissor kick by its height),
     * A on to a team-mate. No frame-perfect press is needed. A ball of our own (a cross,
     * a long ball) he goes for from anywhere, and with the stick left alone he goes for it
     * by himself: near their goal at goal, elsewhere on to a man (play tests: crosses fell
     * to nobody, and B pressed too far from the drop slid the man along the grass). */
    {
        uint8_t air = (uint8_t)(b->owner == NONE && w->meet_t && (U(b->h) >= 6 || b->vh > 10));
        uint8_t ours = (uint8_t)(air && b->last != NONE && cx_team_of(b->last) == t && b->last != i);
        int16_t to_meet = air ? cx_dist(m->x, m->y, (int16_t)(w->meet_x * Q), (int16_t)(w->meet_y * Q)) : 32000;
        if (air && (pressed & (CX_A | CX_B)) && (ours || to_meet < 120)) {
            w->aerial_t[t] = 50;
            w->aerial_shot[t] = (uint8_t)((pressed & CX_B) != 0);
        } else if (ours && !w->aerial_t[t] && !moving && to_meet < 220 && w->pass_to != i) {
            int16_t gx = cx_attacks_right(w, t) ? CX_LEN : 0;
            w->aerial_t[t] = 50;
            w->aerial_shot[t] = (uint8_t)(cx_abs((int16_t)(gx - w->meet_x)) < 300 && cx_abs((int16_t)(w->meet_y - CX_MID_Y)) < CX_BOX_W);
        }
        if (ours && (pressed & (CX_A | CX_B))) pressed = (uint16_t)(pressed & (uint16_t)~(CX_A | CX_B));   /* never a tackle */
    }
    if (w->aerial_t[t]) {
        w->aerial_t[t]--;
        if (b->owner != NONE) w->aerial_t[t] = 0;
        else {
            int16_t d = cx_dist(m->x, m->y, b->x, b->y);
            /* close, he meets it square on; a stretch away, only as it is about to get away
             * from him (a diving header, a lunge) */
            if (U(b->h) >= 6 && U(b->h) <= 46 && (d < 16 || (d < 30 && (U(b->h) < 14 || w->meet_t <= 2u)))) {
                w->aerial_t[t] = 0;
                if (cx_offside_check(w, i)) return;
                cx_aerial(w, i, w->aerial_shot[t]);
                return;
            }
            if (w->meet_t && !moving) {
                cx_move_to(m, w->meet_x, w->meet_y, (int16_t)(SPRINT_V + 4));
                /* there, he waits for it facing their goal */
                if (!m->gx && !m->gy) m->dir = (uint8_t)(cx_attacks_right(w, t) ? 0 : 4);
            }
            return;
        }
    }
    /* A tackles. Close to the man on the ball it is a standing tackle on both layouts: a
     * Classic player's A used to slide in from a stride away, and his slides were most of
     * his fouls and bookings */
    if (pressed & CX_A) {
        uint8_t near_him = (uint8_t)(b->owner != NONE && cx_team_of(b->owner) != t &&
                                     cx_dist(m->x, m->y, w->m[b->owner].x, w->m[b->owner].y) < 24);
        cx_tackle(w, i, (uint8_t)(w->classic_controls && !near_him));
    }
    else if (!w->classic_controls && (pressed & CX_B)) cx_tackle(w, i, 1);
    else if (!moving && b->owner != NONE && cx_team_of(b->owner) != t && w->m[b->owner].role != 0) {
        /* the stick at rest near their man on the ball: he jockeys him, goal-side, a stride
         * off and facing him, instead of standing where he was left */
        const CxMan *c = &w->m[b->owner];
        int16_t d = cx_dist(m->x, m->y, c->x, c->y);
        if (d < 70) {
            int16_t s = cx_attacks_right(w, t) ? 1 : -1;
            cx_move_to(m, (int16_t)(U(c->x) - s * 20), (int16_t)(U(c->y) + cx_div((int16_t)(CX_MID_Y - U(c->y)), 8)), (int16_t)(RUN_V + (m->pace >> 3)));
            m->dir = cx_dir_of((int16_t)(U(c->x) - U(m->x)), (int16_t)(U(c->y) - U(m->y)));
        }
    }
}

/* A player's keeper with the ball in his hands: the stick walks him about his box and
 * points the ball; A rolls it out to a man close by, B kicks it long (up the middle
 * with the stick at rest), C throws it long and flat over his head. Kept too long, he
 * kicks it long himself. */
void CX_CODE cx_keeper_human(CxWorld *w, uint8_t t, uint8_t i, uint16_t held, uint16_t pressed)
{
    CxMan *m = &w->m[i];
    uint8_t dir = m->dir, moving = stick_dir(held, &dir);
    int16_t s = cx_attacks_right(w, t) ? 1 : -1, line = s > 0 ? 0 : CX_LEN;
    m->vx = m->vy = m->gx = m->gy = 0;
    if (moving) m->dir = dir;
    if (pressed & (CX_A | CX_B | CX_C)) {
        cx_keeper_out(w, i, (pressed & CX_B) ? CX_KD_PUNT : ((pressed & CX_C) ? CX_KD_THROW : CX_KD_SHORT), moving);
        return;
    }
    if (!m->timer) { cx_keeper_out(w, i, CX_KD_PUNT, 0); return; }
    if (moving) {
        /* a walk, and never out of his box: outside it he may not handle */
        int16_t vx = cx_vel(cx_kdx[dir], 14), vy = cx_vel(cx_kdy[dir], 14);
        int16_t nx = (int16_t)(U(m->x + vx) - line), ny = U(m->y + vy);
        if (nx * s > 10 && nx * s < CX_BOX_D - 14) m->vx = vx;
        if (cx_abs((int16_t)(ny - CX_MID_Y)) < CX_BOX_W - 14) m->vy = vy;
    }
}

/* Each human side's man: the one on the ball, or the nearest to it. */
void CX_CODE cx_choose_ctrl(CxWorld *w, uint8_t t, uint16_t pressed)
{
    CxBall *b = &w->ball;
    uint8_t cur = w->ctrl[t];
    /* a ball in the air: the man nearest where it comes down, not where it is now */
    uint8_t near = (b->owner == NONE && w->meet_t && (U(b->h) >= 6 || b->vh > 10)) ? cx_nearest(w, t, w->meet_x, w->meet_y, 0)
                                                                                   : cx_nearest(w, t, U(b->x), U(b->y), 0);
    /* on the ball: him, the keeper too while it is in his hands (the player throws or kicks it) */
    if (b->owner != NONE && cx_team_of(b->owner) == t && (w->m[b->owner].role != 0 || w->m[b->owner].state == CX_M_HOLD)) {
        w->ctrl[t] = b->owner;
        return;
    }
    /* a shot coming at his goal: the player has his keeper for it, and the CPU keeps him
     * until the stick or a button says otherwise. Diving, he stays the player's until he
     * lands. */
    {
        uint8_t k = (uint8_t)(t * 11u);
        if (cx_active(w, k) && w->m[k].role == 0 &&
            (cx_shot_at(w, t) || (cur == k && w->m[k].state == CX_M_DIVE))) {
            if (cur != k) w->keep_manual[t] = 0;
            w->ctrl[t] = k;
            return;
        }
    }
    if (cur != NONE && cur < CX_MEN && w->m[cur].role == 0) { w->ctrl[t] = near; return; }    /* it has left his hands */
    if (w->aerial_t[t] && cur != NONE && cx_active(w, cur)) return;     /* he is going for it: no swap now */
    /* our own cross or long ball coming down for nobody in particular: the man under it is
     * the player's, a second and a half before it drops (it used to stay on the crosser) */
    if (b->owner == NONE && w->meet_t && w->meet_t < 45u && (U(b->h) >= 6 || b->vh > 10) && w->pass_to == NONE &&
        b->last != NONE && cx_team_of(b->last) == t && near != NONE && near != b->last &&
        cx_dist(w->m[near].x, w->m[near].y, (int16_t)(w->meet_x * Q), (int16_t)(w->meet_y * Q)) < 220) {
        w->ctrl[t] = near;
        return;
    }
    /* A or B as a ball flies in: the man under where it drops is the one who goes for it */
    if (b->owner == NONE && w->meet_t && (U(b->h) >= 6 || b->vh > 10) && (pressed & (CX_A | CX_B)) && cur != NONE && near != cur &&
        cx_dist(w->m[cur].x, w->m[cur].y, (int16_t)(w->meet_x * Q), (int16_t)(w->meet_y * Q)) >
        cx_dist(w->m[near].x, w->m[near].y, (int16_t)(w->meet_x * Q), (int16_t)(w->meet_y * Q))) {
        w->ctrl[t] = near;
        return;
    }
    if (cur == NONE || cx_team_of(cur) != t || !cx_active(w, cur)) { w->ctrl[t] = near; return; }
    if (pressed & (w->classic_controls ? CX_B : CX_C)) { w->ctrl[t] = near; return; }
    if (w->pass_to != NONE && cx_team_of(w->pass_to) == t) return;      /* it is coming to the man the pass was for */
    if (near != cur && w->m[cur].state == CX_M_RUN &&
        cx_dist(w->m[cur].x, w->m[cur].y, b->x, b->y) > cx_dist(w->m[near].x, w->m[near].y, b->x, b->y) + 70)
        w->ctrl[t] = near;
}
