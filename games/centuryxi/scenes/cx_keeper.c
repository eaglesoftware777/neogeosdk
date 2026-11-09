/*
 * cx_keeper.c -- Century XI: the goalkeepers.
 *
 * A keeper stands on the angle, reads where a shot will cross the line, dives at
 * what he can reach, claims loose balls in his box and may not pick up a pass
 * from his own side's foot. His ratings decide how much he can hold; he obeys the
 * same ball as everyone else.
 */
#include "cx_internal.h"

static uint8_t CX_CODE back_pass(const CxWorld *w, uint8_t i)
{
    const CxBall *b = &w->ball;
    return (uint8_t)(b->last != NONE && b->last != i && cx_team_of(b->last) == cx_team_of(i) &&
                     w->m[b->last].state == CX_M_KICK);
}

static void CX_CODE claim(CxWorld *w, uint8_t i, uint8_t hold_frames)
{
    CxMan *m = &w->m[i];
    CxBall *b = &w->ball;
    uint8_t t = cx_team_of(i);
    cx_ball_take(w, i);
    m->state = CX_M_HOLD;
    /* a CPU side defending a lead late on takes its time over it */
    if (w->brain[t].cpu && w->mental[t] == 0u && w->half >= 1u) hold_frames = (uint8_t)(hold_frames + 50u);
    if (w->human[t]) hold_frames = 210;
    m->timer = hold_frames;
    b->vx = b->vy = b->vh = 0;
}

/* Out of his hands: a quick throw to a free man nearby when there is one, otherwise a
 * punt up to the front men. A direct side punts more often. */
static void CX_CODE release(CxWorld *w, uint8_t i)
{
    uint8_t t = cx_team_of(i), o0 = (uint8_t)((t ^ 1u) * 11u), k, o, best = NONE;
    int16_t s = cx_attacks_right(w, t) ? 1 : -1, best_score = -32000;
    const CxMan *g = &w->m[i];
    for (k = (uint8_t)(t * 11u + 1u); k < t * 11u + 11u; k++) {
        const CxMan *m = &w->m[k];
        int16_t d, room = 200, score;
        if (!cx_active(w, k)) continue;
        d = cx_dist(m->x, m->y, g->x, g->y);
        if (d < 60 || d > 240) continue;
        for (o = (uint8_t)(o0 + 1u); o < o0 + 11u; o++) {
            int16_t e;
            if (!cx_active(w, o)) continue;
            e = cx_dist(w->m[o].x, w->m[o].y, m->x, m->y);
            if (e < room) room = e;
        }
        if (room < 50) continue;
        score = (int16_t)(room + (U(m->x) - U(g->x)) * s / 4);
        if (score > best_score) { best_score = score; best = k; }
    }
    if (best != NONE && !(cx_squad_style(w->team[t]) == STYLE_DIRECT && (cx_rnd(w) & 1u))) {
        cx_pass_to(w, i, best, 0);
        return;
    }
    cx_pass(w, i, 0, 1);
}

/* A ball reaching his hands, flying or on his feet: held if he can (an easy one still
 * squirms out now and then, more for a poor keeper), else tipped round the post or
 * pushed back out, and the rebound is live. */
static void CX_CODE keeper_stop(CxWorld *w, uint8_t i, uint8_t skill)
{
    CxBall *b = &w->ball;
    int16_t speed = (int16_t)(cx_abs(b->vx) + cx_abs(b->vy)), by = U(b->y);
    if (speed < 108 + skill * 3 || (cx_rnd(w) & 3) == 0) {
        if (speed > 50 && (cx_rnd(w) & 31u) < (uint16_t)(17u - skill) / 3u) {
            b->vx = (int16_t)(-b->vx / 5);
            b->vy = (int16_t)(b->vy / 3 + 12);
            b->vh = 10;
            b->last = i;
            w->ev |= CX_EV_FUMBLE;
        } else {
            claim(w, i, 70);
        }
    } else if (cx_rnd(w) & 1u) {
        b->vx = (int16_t)(b->vx / 3);
        b->vy = (int16_t)(by < CX_MID_Y ? -56 : 56);
        b->vh = 12;
        b->curve = 0;
        b->last = i;
    } else {
        b->vx = (int16_t)(-b->vx / 3);
        b->vy = (int16_t)((cx_rnd(w) & 1) ? 40 : -40);
        b->vh = 14;
        b->curve = 0;
        b->last = i;
    }
    w->save_t = 90;
    w->ev |= CX_EV_SAVE;
}

static uint8_t CX_CODE keeper_skill(const CxWorld *w, uint8_t t)
{
    int16_t sk = (int16_t)((int16_t)cx_squad_stat(w->team[t], 5) + w->brain[t].keep);
    return (uint8_t)cx_clamp(sk, 1, 16);
}

/* A shot coming at side t's goal, on target or near it: frames to the line (0: none). */
uint8_t CX_CODE cx_shot_at(const CxWorld *w, uint8_t t)
{
    const CxBall *b = &w->ball;
    uint8_t right_goal = (uint8_t)!cx_attacks_right(w, t);
    int16_t line = right_goal ? CX_LEN : 0, frames, cross;
    if (w->phase != CX_P_PLAY || b->owner != NONE || !b->fast || b->striker == NONE || cx_team_of(b->striker) == t) return 0;
    if (right_goal ? b->vx <= 20 : b->vx >= -20) return 0;
    frames = cx_div((int16_t)((line - U(b->x)) * Q), b->vx);
    if (frames <= 0 || frames > 60) return 0;
    cross = (int16_t)(U(b->y) + (int16_t)(cx_mul(b->vy, frames) >> 4));
    if (cx_abs((int16_t)(cross - CX_MID_Y)) >= CX_POST + 40) return 0;
    return (uint8_t)frames;
}

/* A player's keeper with a shot coming: the stick moves him about his box, A, B or C
 * dives the stick's way (with it at rest, to where the shot will cross). Until the player
 * touches the stick or a button he is the CPU's, so leaving him alone costs nothing. */
void CX_CODE cx_keeper_player(CxWorld *w, uint8_t t, uint8_t i, uint16_t held, uint16_t pressed)
{
    CxMan *m = &w->m[i];
    const CxBall *b = &w->ball;
    uint8_t skill = keeper_skill(w, t), right_goal = (uint8_t)!cx_attacks_right(w, t);
    int16_t line = right_goal ? CX_LEN : 0, s = right_goal ? -1 : 1;
    int16_t sx = (int16_t)((held & CX_RIGHT) ? 1 : ((held & CX_LEFT) ? -1 : 0));
    int16_t sy = (int16_t)((held & CX_DOWN) ? 1 : ((held & CX_UP) ? -1 : 0));
    /* the CPU went first for him (it dives the moment a shot is struck, before anyone could
     * react): a button with the stick in the first ten frames of the dive turns it the
     * player's way, as hard as his own */
    if (m->state == CX_M_DIVE && m->timer > 16u && w->keep_manual[t] != 2u && (pressed & (CX_A | CX_B | CX_C)) && (sx || sy)) {
        int16_t fly = (int16_t)(60 + skill);
        w->keep_manual[t] = 2;
        m->timer = 26;
        m->dir = cx_dir_of(sx, sy);
        m->vx = (int16_t)(sx ? cx_vel(cx_kdx[m->dir], fly) : s * 4);
        m->vy = cx_vel(cx_kdy[m->dir], fly);
        return;
    }
    if ((sx || sy || (pressed & (CX_A | CX_B | CX_C))) && !w->keep_manual[t]) w->keep_manual[t] = 1;
    if (!w->keep_manual[t] || m->state != CX_M_RUN) { cx_keeper_step(w, i); return; }
    if (pressed & (CX_A | CX_B | CX_C)) {
        /* a dive the player reads and times goes further than the CPU's (40 + skill): a placed
         * shot by the post is his to reach, if he goes the right way in time */
        int16_t fly = (int16_t)(60 + skill);
        w->keep_manual[t] = 2;
        if (!sx && !sy) {
            /* at rest: across to where it will cross, or a catch where he stands */
            int16_t frames = (int16_t)cx_shot_at(w, t), cross = U(b->y);
            if (frames) cross = (int16_t)(U(b->y) + (int16_t)(cx_mul(b->vy, frames) >> 4));
            if (cx_abs((int16_t)(cross - U(m->y))) <= 12) {
                if (b->owner == NONE && U(b->h) < 40 && cx_dist(m->x, m->y, b->x, b->y) < 18) keeper_stop(w, i, skill);
                return;
            }
            sy = (int16_t)(cross > U(m->y) ? 1 : -1);
        }
        m->state = CX_M_DIVE;
        m->timer = 26;
        m->dir = cx_dir_of(sx, sy);
        m->vx = (int16_t)(sx ? cx_vel(cx_kdx[m->dir], fly) : s * 4);
        m->vy = cx_vel(cx_kdy[m->dir], fly);
        return;
    }
    if (sx || sy) {
        /* a keeper's quick steps, kept inside his box and off the back of his line */
        int16_t sp = (int16_t)(26 + skill / 2), nx;
        m->dir = cx_dir_of(sx, sy);
        m->gx = cx_vel(cx_kdx[m->dir], sp);
        m->gy = cx_vel(cx_kdy[m->dir], sp);
        nx = (int16_t)((U(m->x) - line) * s);
        if ((nx < 6 && m->gx * s < 0) || (nx > CX_BOX_D - 20 && m->gx * s > 0)) m->gx = 0;
        if (cx_abs((int16_t)(U(m->y) - CX_MID_Y)) > CX_BOX_W - 20 && m->gy * (U(m->y) - CX_MID_Y) > 0) m->gy = 0;
    } else {
        m->gx = m->gy = 0;
    }
    /* one at him: he gathers it */
    if (b->owner == NONE && U(b->h) < 40 && cx_dist(m->x, m->y, b->x, b->y) < 16) keeper_stop(w, i, skill);
}

/* An opponent through on goal with nobody of ours between him and it. */
static uint8_t CX_CODE through_on_goal(const CxWorld *w, uint8_t t, int16_t line, int16_t *cx, int16_t *cy)
{
    const CxBall *b = &w->ball;
    uint8_t k;
    int16_t x, y, dl;
    if (b->owner == NONE || cx_team_of(b->owner) == t || w->phase != CX_P_PLAY) return 0;
    x = U(w->m[b->owner].x);
    y = U(w->m[b->owner].y);
    dl = cx_abs((int16_t)(x - line));
    if (dl > 280 || cx_abs((int16_t)(y - CX_MID_Y)) > CX_BOX_W) return 0;
    for (k = (uint8_t)(t * 11u + 1u); k < t * 11u + 11u; k++) {
        const CxMan *d = &w->m[k];
        if (!cx_active(w, k) || d->state != CX_M_RUN) continue;
        if (cx_abs((int16_t)(U(d->x) - line)) < dl && cx_abs((int16_t)(U(d->y) - y)) < 80) return 0;
    }
    *cx = x;
    *cy = y;
    return 1;
}

void CX_CODE cx_keeper_step(CxWorld *w, uint8_t i)
{
    CxMan *m = &w->m[i];
    CxBall *b = &w->ball;
    uint8_t t = cx_team_of(i);
    uint8_t right_goal = (uint8_t)!cx_attacks_right(w, t);
    int16_t line = right_goal ? CX_LEN : 0, s = right_goal ? -1 : 1;
    int16_t bx = U(b->x), by = U(b->y);
    int16_t sk = (int16_t)((int16_t)cx_squad_stat(w->team[t], 5) + w->brain[t].keep);
    uint8_t skill = (uint8_t)cx_clamp(sk, 1, 16);
    if (m->state == CX_M_OFF) return;
    if (m->state == CX_M_KICK) return;              /* his own throw or kick is on its way */
    if (m->state == CX_M_HOLD) {
        m->vx = m->vy = 0;
        if (!m->timer) {
            m->state = CX_M_RUN;
            release(w, i);
        }
        return;
    }
    if (m->state == CX_M_DIVE || m->state == CX_M_DOWN) {
        /* at the feet of a man through on goal: the ball, if he times it */
        if (m->state == CX_M_DIVE && b->owner != NONE && cx_team_of(b->owner) != t &&
            cx_dist(m->x, m->y, b->x, b->y) < 18 && m->timer > 8u) {
            CxMan *c = &w->m[b->owner];
            m->timer = 8;
            if ((cx_rnd(w) & 255u) < (uint16_t)(110u + skill * 7u - c->ctrl * 4u)) {
                c->timer = 16;
                claim(w, i, 60);
                w->save_t = 90;
                w->ev |= CX_EV_SAVE;
            }
            return;
        }
        if (b->owner == NONE && U(b->h) < 44 && cx_dist(m->x, m->y, b->x, b->y) < (m->state == CX_M_DIVE ? 22 : 12))
            keeper_stop(w, i, skill);
        return;
    }
    /* a free kick over his own wall: he cannot see it until it has cleared the men, unless
     * he read the taker (a good keeper more often): decided once, at the kick */
    if (w->fk_wall_n && w->phase == CX_P_PLAY && b->owner == NONE) {
        if (w->fk_t == 1u) w->fk_read = (uint8_t)((cx_rnd(w) & 15u) < 4u + skill / 2u);
        if (w->fk_t < 8u && !w->fk_read) return;
    }
    /* a high ball dropping into his six-yard area: his to come for. He catches it if he
     * has room and the hands, and punches it clear when there are bodies round him. */
    if (b->owner == NONE && w->phase == CX_P_PLAY && w->meet_t && (b->vh > 30 || U(b->h) > 18 || b->vh < 0) && !back_pass(w, i)) {
        /* where it drops to head height (worked out once a frame in cx_ball_step) */
        int16_t f = w->meet_t, lx = w->meet_x, ly = w->meet_y;
        if (cx_abs((int16_t)(lx - line)) < CX_SIX_D + 24 && cx_abs((int16_t)(ly - CX_MID_Y)) < CX_SIX_W &&
            cx_dist(m->x, m->y, (int16_t)(lx * Q), (int16_t)(ly * Q)) < f * 3 / 2 + 6) {
            cx_move_to(m, lx, ly, SPRINT_V);
            if (U(b->h) < 46 && cx_dist(m->x, m->y, b->x, b->y) < 18) {
                uint8_t k, o0 = (uint8_t)((t ^ 1u) * 11u), crowd = 0;
                for (k = (uint8_t)(o0 + 1u); k < o0 + 11u; k++)
                    if (cx_active(w, k) && cx_dist(w->m[k].x, w->m[k].y, m->x, m->y) < 18) { crowd = 1; break; }
                if (!crowd && (cx_rnd(w) & 255u) < (uint16_t)(120u + skill * 6u)) {
                    claim(w, i, 60);
                    w->ev |= CX_EV_CLAIM;
                } else {
                    b->vx = (int16_t)(s * (56 + (int16_t)(cx_rnd(w) & 31u)));
                    b->vy = (int16_t)((by < CX_MID_Y ? -1 : 1) * (20 + (int16_t)(cx_rnd(w) & 15u)));
                    b->vh = 16;
                    b->curve = 0;
                    b->last = i;
                    w->ev |= CX_EV_PUNCH;
                }
                w->save_t = 60;
            }
            return;
        }
    }
    /* a shot on its way: where it crosses the line. One that reaches him where he stands he
     * stops (it used to pass through a keeper who had no need to dive: a shot struck
     * straight at him from 20 yards went in every time) */
    if (b->owner == NONE && ((s > 0 && b->vx < -20) || (s < 0 && b->vx > 20))) {
        if (U(b->h) < 40 && cx_dist(m->x, m->y, b->x, b->y) < 16) { keeper_stop(w, i, skill); return; }
        int16_t frames = cx_div((int16_t)((line - bx) * Q), b->vx);
        if (frames > 0 && frames < 40) {
            int16_t cross = (int16_t)(by + (int16_t)(cx_mul(b->vy, frames) >> 4));
            if (cx_abs((int16_t)(cross - CX_MID_Y)) < CX_POST + 24) {
                int16_t dy = (int16_t)(cross - U(m->y));
                if (cx_abs(dy) > 14 && frames < 22) {
                    /* how far he flies is his rating: a placed shot near a post beats an
                     * ordinary keeper about half the time */
                    int16_t fly = (int16_t)(40 + skill);
                    m->state = CX_M_DIVE;
                    m->timer = 26;
                    m->vy = (int16_t)(dy > 0 ? fly : -fly);
                    m->vx = (int16_t)(s * 4);
                    m->dir = (uint8_t)(dy > 0 ? 2 : 6);
                    return;
                }
                /* across his line at a keeper's shuffle, not an outfielder's sprint */
                cx_move_to(m, (int16_t)(line + s * 14), cross, (int16_t)(22 + skill / 2));
                return;
            }
        }
    }
    /* a loose ball in his box: go for it */
    if (b->owner == NONE && cx_abs((int16_t)(bx - line)) < CX_BOX_D - 60 && cx_abs((int16_t)(by - CX_MID_Y)) < CX_BOX_W - 80) {
        cx_move_to(m, bx, by, SPRINT_V);
        if (U(b->h) < 30 && cx_dist(m->x, m->y, b->x, b->y) < 14) {
            if (back_pass(w, i)) {
                /* his own side's pass: he may only clear it */
                cx_kick_to(w, i, (int16_t)(bx + s * 400), (int16_t)(by < CX_MID_Y ? 100 : CX_WID - 100), 70, 34);
                return;
            }
            claim(w, i, 60);
        }
        return;
    }
    /* a wall in front: it has the near half, he takes the far one */
    if (w->phase == CX_P_FREEKICK && w->fk_wall_n && w->restart_team != t) {
        cx_move_to(m, (int16_t)(line + s * 10), (int16_t)(CX_MID_Y + (by > CX_MID_Y ? -CX_POST / 5 : CX_POST / 5)), RUN_V);
        return;
    }
    /* a man through on him: off his line to narrow the angle, and at his feet when close */
    {
        int16_t cx, cy;
        if (through_on_goal(w, t, line, &cx, &cy)) {
            int16_t dl = cx_abs((int16_t)(cx - line)), out = cx_div((int16_t)(dl * 2), 5);
            if (out > 100) out = 100;
            if (out < 14) out = 14;
            cx_move_to(m, (int16_t)(line + cx_sgn(out, s)), (int16_t)(CX_MID_Y + cx_div((int16_t)cx_mul((int16_t)(cy - CX_MID_Y), out), (int16_t)(dl ? dl : 1))), SPRINT_V);
            if (cx_dist(m->x, m->y, b->x, b->y) < 30 && (cx_rnd(w) & 3u) == 0) {
                int16_t dx = (int16_t)(bx - U(m->x)), dy = (int16_t)(by - U(m->y));
                m->state = CX_M_DIVE;
                m->timer = 22;
                m->dir = cx_dir_of(dx, dy);
                m->vx = cx_vel(cx_kdx[m->dir], 40);
                m->vy = cx_vel(cx_kdy[m->dir], 40);
            }
            return;
        }
    }
    cx_move_to(m, (int16_t)(line + s * (18 + cx_abs((int16_t)(bx - line)) / 40)),
               cx_clamp((int16_t)(CX_MID_Y + cx_div((int16_t)(by - CX_MID_Y), 3)), CX_MID_Y - CX_POST + 6, CX_MID_Y + CX_POST - 6), RUN_V);
}
