/*
 * cx_rules.c -- Century XI: the laws, and the referee's record.
 *
 * Restarts, fouls, cards, advantage, offside and the shoot-out. Calls are made
 * from what happened in the simulation; a video review reads the record and
 * can only ever put the truth back, so it has no dice of its own.
 *
 * Offside: the man's distance past the second-last defender is measured when
 * his side plays the ball. Past FLAG_MARGIN the assistant flags it. Inside that
 * he is really off but the flag stays down, which is the case a review exists
 * for: with review switched off such a goal stands, as it would have in 1994.
 */
#include "cx_internal.h"

#define FLAG_MARGIN 10

uint8_t CX_CODE cx_nearest(const CxWorld *w, uint8_t team, int16_t x, int16_t y, uint8_t keeper_ok)
{
    uint8_t base = (uint8_t)(team * 11u), i, best = (uint8_t)(base + 1u);
    int16_t bd = 32000;
    for (i = (uint8_t)(base + (keeper_ok ? 0u : 1u)); i < base + 11u; i++) {
        int16_t d;
        if (!cx_active(w, i)) continue;
        d = cx_dist(w->m[i].x, w->m[i].y, (int16_t)(x * Q), (int16_t)(y * Q));
        if (w->m[i].state == CX_M_DOWN) d = (int16_t)(d + 200);
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

void CX_CODE cx_log(CxWorld *w, uint8_t type, uint8_t man, uint8_t other, int16_t margin)
{
    CxLog *l = &w->log[w->log_n & (CX_LOG_N - 1u)];
    l->clock = w->clock;
    l->type = type;
    l->man = man;
    l->other = other;
    l->half = w->half;
    l->x = U(w->ball.x);
    l->y = U(w->ball.y);
    l->margin = margin;
    w->log_n = (uint8_t)(w->log_n + 1u);
}

/* A free kick near goal: the defending side builds a wall, as on a real pitch. It lines
 * up on the near half of the goal (the keeper takes the far half), two to five men by
 * the danger, shoulder to shoulder across the line of the kick. It starts too close and
 * the referee walks it back to 9.15 m before the kick (cx_wall_step). */
static void CX_CODE make_wall(CxWorld *w, uint8_t team, int16_t x, int16_t y)
{
    uint8_t def = (uint8_t)(team ^ 1u), k, n;
    uint16_t used = 0;
    int16_t gx = cx_attacks_right(w, team) ? CX_LEN : 0;
    int16_t ty = (int16_t)(CX_MID_Y + (y > CX_MID_Y ? CX_POST / 2 : -CX_POST / 2));   /* the near half */
    int16_t dx = (int16_t)(gx - x), dy = (int16_t)(ty - y);
    int16_t len = (int16_t)(cx_abs(dx) > cx_abs(dy) ? cx_abs(dx) + cx_abs(dy) * 3 / 8 : cx_abs(dy) + cx_abs(dx) * 3 / 8);
    int16_t off = cx_abs((int16_t)(y - CX_MID_Y));
    w->fk_wall_n = 0;
    if (len > 420 || len < CX_WALL_D + 20) return;
    n = (uint8_t)(len < 300 ? (off < 120 ? 5u : (off < 220 ? 4u : 3u)) : (off < 160 ? 3u : 2u));
    /* it starts too close: 100 units, and steps back */
    w->fk_wx = (int16_t)(x + (int16_t)((int32_t)dx * 100 / len));
    w->fk_wy = (int16_t)(y + (int16_t)((int32_t)dy * 100 / len));
    /* across the line of the kick: (-dy, dx), thirteen units a man */
    w->fk_nx = (int16_t)((int32_t)-dy * 13 / len);
    w->fk_ny = (int16_t)((int32_t)dx * 13 / len);
    for (k = 0; k < n; k++) {
        int16_t s2 = (int16_t)(2 * k - (n - 1));      /* -(n-1) .. (n-1), in half places */
        int16_t mx = (int16_t)(w->fk_wx + w->fk_nx * s2 / 2), my = (int16_t)(w->fk_wy + w->fk_ny * s2 / 2), bd = 32000;
        uint8_t i, m = NONE;
        for (i = (uint8_t)(def * 11u + 1u); i < def * 11u + 11u; i++) {
            int16_t d;
            if (!cx_active(w, i) || (used & (1u << (i - def * 11u)))) continue;
            d = cx_dist(w->m[i].x, w->m[i].y, (int16_t)(mx * Q), (int16_t)(my * Q));
            if (d < bd) { bd = d; m = i; }
        }
        if (m == NONE) break;
        used = (uint16_t)(used | (1u << (m - def * 11u)));
        w->m[m].x = (int16_t)(mx * Q);
        w->m[m].y = (int16_t)(my * Q);
        w->m[m].vx = w->m[m].vy = w->m[m].gx = w->m[m].gy = 0;
        w->m[m].state = CX_M_FEINT;
        w->m[m].timer = 255;
        w->m[m].dir = (uint8_t)(cx_attacks_right(w, team) ? 4 : 0);
        w->fk_wall[w->fk_wall_n++] = m;
    }
    w->fk_t = 0;
}

/* Before the kick the wall shuffles back to its distance; at the kick it jumps, and for
 * as long as it is in the air a ball low enough to meet it is blocked. */
static void CX_CODE wall_place(CxWorld *w, int16_t step)
{
    uint8_t k, n = w->fk_wall_n;
    int16_t bx = U(w->ball.x), by = U(w->ball.y);
    int16_t dx = (int16_t)(w->fk_wx - bx), dy = (int16_t)(w->fk_wy - by);
    int16_t len = (int16_t)(cx_abs(dx) > cx_abs(dy) ? cx_abs(dx) + cx_abs(dy) * 3 / 8 : cx_abs(dy) + cx_abs(dx) * 3 / 8);
    if (len < 1 || len >= CX_WALL_D) return;
    w->fk_wx = (int16_t)(w->fk_wx + (int16_t)((int32_t)dx * step / len));
    w->fk_wy = (int16_t)(w->fk_wy + (int16_t)((int32_t)dy * step / len));
    for (k = 0; k < n; k++) {
        CxMan *m = &w->m[w->fk_wall[k]];
        int16_t s2 = (int16_t)(2 * k - (n - 1));
        if (m->state != CX_M_FEINT) continue;
        m->x = (int16_t)((w->fk_wx + w->fk_nx * s2 / 2) * Q);
        m->y = (int16_t)((w->fk_wy + w->fk_ny * s2 / 2) * Q);
        m->timer = 255;
    }
}

/* At a free kick or a corner the other side stands 9.15 m from the ball: anyone closer,
 * the wall and the keeper aside, is walked back out to the distance. */
void CX_CODE cx_keep_back(CxWorld *w)
{
    uint8_t o = (uint8_t)((w->restart_team ^ 1u) * 11u), i;
    int16_t bx = U(w->ball.x), by = U(w->ball.y);
    if (w->phase != CX_P_FREEKICK && w->phase != CX_P_CORNER) return;
    for (i = o; i < o + 11u; i++) {
        CxMan *m = &w->m[i];
        int16_t dx, dy, len;
        if (m->role == 0 || m->state == CX_M_OFF || m->state == CX_M_FEINT) continue;
        dx = (int16_t)(U(m->x) - bx);
        dy = (int16_t)(U(m->y) - by);
        len = (int16_t)(cx_abs(dx) > cx_abs(dy) ? cx_abs(dx) + cx_abs(dy) * 3 / 8 : cx_abs(dy) + cx_abs(dx) * 3 / 8);
        if (len >= CX_WALL_D) continue;
        if (len < 4) { dx = (int16_t)(w->ball.x > CX_LEN / 2 * Q ? -4 : 4); dy = 0; len = 4; }
        /* |dx|, |dy| < len < CX_WALL_D: the product fits 16 bits and the quotient is under
         * CX_WALL_D, so one divs does (the 32-bit division was a library call: a corner's
         * set-up spent 3,000 cycles a tick in it) */
        m->x = (int16_t)((bx + cx_div((int16_t)cx_mul(dx, CX_WALL_D), len)) * Q);
        m->y = (int16_t)((by + cx_div((int16_t)cx_mul(dy, CX_WALL_D), len)) * Q);
        m->vx = m->vy = 0;
    }
}

void CX_CODE cx_wall_step(CxWorld *w)
{
    CxBall *b = &w->ball;
    uint8_t k;
    if (!w->fk_wall_n) return;
    if (w->phase == CX_P_FREEKICK) {
        if (w->phase_t >= 8u && w->phase_t < 40u) wall_place(w, 1);
        return;
    }
    if (w->phase != CX_P_PLAY) { w->fk_wall_n = 0; return; }
    if (++w->fk_t > 36u) {
        for (k = 0; k < w->fk_wall_n; k++) if (w->m[w->fk_wall[k]].state == CX_M_FEINT) w->m[w->fk_wall[k]].state = CX_M_RUN;
        w->fk_wall_n = 0;
        return;
    }
    if (b->owner != NONE) return;
    for (k = 0; k < w->fk_wall_n; k++) {
        CxMan *m = &w->m[w->fk_wall[k]];
        /* jumping, the wall reaches a little higher than a man */
        if (U(b->h) < (m->state == CX_M_HEAD ? 34 : 26) && cx_dist(m->x, m->y, b->x, b->y) < 12 && b->last != w->fk_wall[k]) {
            b->vx = (int16_t)(-b->vx / 3 + (int16_t)(cx_rnd(w) & 15u) - 8);
            b->vy = (int16_t)(((cx_rnd(w) & 1u) ? 30 : -30) + b->vy / 4);
            b->vh = 9;
            b->curve = 0;
            b->last = w->fk_wall[k];
            w->ev |= CX_EV_TACKLE;
            w->fk_wall_n = 0;
            return;
        }
    }
}

/* A free kick at goal. Over the wall: at the near half, the corner the wall hides from the
 * keeper, lofted so it drops under the bar at the goal line; the lift is worked out from
 * the distance, with a taker's error. Round it: low at the keeper's half, which passes the
 * end of the wall, curling in. From very close the ball cannot get up and down in time. */
static void CX_CODE fk_strike(CxWorld *w, uint8_t i, uint8_t over, int8_t side)
{
    CxBall *b = &w->ball;
    CxMan *m = &w->m[i];
    uint8_t t = cx_team_of(i);
    int16_t gx = cx_attacks_right(w, t) ? CX_LEN + 6 : -6;
    int16_t bx = U(b->x), by = U(b->y);
    int16_t near = (int16_t)(by > CX_MID_Y ? 1 : -1);          /* the near post's side */
    int16_t err = (int16_t)(22 - m->shot);                      /* the better the boot, the truer */
    int16_t gy, speed, dist, frames, aim_h, vh;
    if (m->type == CX_T_SETPIECE) err = (int16_t)(err / 2);
    if (err < 2) err = 2;
    if (side == 0) side = (int8_t)(over ? near : -near);
    gy = (int16_t)(CX_MID_Y + side * (CX_POST - 18) + (int16_t)(cx_rnd(w) % (unsigned)(err * 4 + 1)) - err * 2);
    speed = (int16_t)(over ? 112 : 140);
    cx_kick_to(w, i, gx, gy, speed, 0);
    dist = cx_abs((int16_t)(gx - bx));
    frames = (int16_t)((int32_t)dist * Q / speed);
    if (frames < 4) frames = 4;
    /* the height it should cross the line at, in Q: under the bar, a little off for a poor taker */
    aim_h = (int16_t)((over ? 28 : 12) * Q + ((int16_t)(cx_rnd(w) % (unsigned)(err * 3 + 1)) - err * 3 / 2) * Q);
    vh = (int16_t)((aim_h + (int32_t)GRAVITY * frames * (frames - 1) / 2) / frames);
    b->vh = vh;
    b->h = 2 * Q;
    b->curve = cx_bend_y(b, (int16_t)((over ? 0 : -side * 8) * (m->type == CX_T_SETPIECE ? 2 : 1)));
    b->kind = CX_G_FREEKICK;
    b->fast = 1;
    m->kick = CX_K_SHOT;
    if (w->shots[t] < 255u) w->shots[t]++;
    w->ev |= CX_EV_SHOT;
}

void CX_CODE cx_restart(CxWorld *w, uint8_t phase, uint8_t team, int16_t x, int16_t y)
{
    CxBall *b = &w->ball;
    uint8_t taker, right = cx_attacks_right(w, team);
    w->phase = phase;
    w->phase_t = 0;
    w->restart_team = team;
    w->adv_t = 0;
    w->aim = 0;
    w->aim_on = 0;
    w->latch = 0;
    w->off_mask[0] = w->off_mask[1] = 0;
    w->off_man = NONE;
    w->off_live = 0;
    cx_subs_apply(w);
    b->x = (int16_t)(x * Q);
    b->y = (int16_t)(y * Q);
    b->h = b->vx = b->vy = b->vh = b->curve = 0;
    if (phase == CX_P_GOALKICK) taker = (uint8_t)(team * 11u);
    else taker = cx_nearest(w, team, x, y, 0);
    w->restart_man = taker;
    w->m[taker].x = (int16_t)(x * Q - (phase == CX_P_THROW ? 0 : cx_kdx[right ? 0 : 4] * 10));
    w->m[taker].y = (int16_t)(y * Q);
    w->m[taker].vx = w->m[taker].vy = 0;
    w->m[taker].gx = w->m[taker].gy = 0;
    w->m[taker].dir = phase == CX_P_THROW ? (uint8_t)(y < CX_MID_Y ? 2 : 6) : (uint8_t)(right ? 0 : 4);
    w->m[taker].state = phase == CX_P_THROW ? CX_M_THROW : CX_M_RUN;
    if (phase == CX_P_FREEKICK) make_wall(w, team, x, y);
    if (phase == CX_P_PENALTY) {
        CxMan *k = &w->m[(uint8_t)((team ^ 1u) * 11u)];
        k->x = (int16_t)((right ? CX_LEN - 6 : 6) * Q);
        k->y = (int16_t)(CX_MID_Y * Q);
        k->vx = k->vy = 0;
        k->state = CX_M_RUN;
        k->dir = (uint8_t)(right ? 4 : 0);
    }
    b->owner = taker;
    b->last = taker;
    w->ev |= CX_EV_WHISTLE;
}

void CX_CODE cx_kickoff(CxWorld *w, uint8_t team)
{
    uint8_t i;
    for (i = 0; i < CX_MEN; i++) {
        CxMan *m = &w->m[i];
        int16_t hx = m->hx;
        int16_t x = (int16_t)(hx / 2);                       /* in their own half */
        if (m->state == CX_M_OFF) continue;
        if (m->role == 0) x = hx;
        if (!cx_attacks_right(w, cx_team_of(i))) x = (int16_t)(CX_LEN - x);
        m->x = (int16_t)(x * Q);
        m->y = (int16_t)(m->hy * Q);
        m->vx = m->vy = 0;
        m->state = CX_M_RUN;
        m->timer = 0;
        m->dir = (uint8_t)(cx_attacks_right(w, cx_team_of(i)) ? 0 : 4);
    }
    /* the kicking side's two forwards stand on the spot: one on the ball, the other a stride
     * back and to the side to take the first pass, and a midfielder a little further back
     * on the other side (play tests: there was nobody near the man on the ball to play it to) */
    {
        uint8_t k2 = (uint8_t)(team * 11u + 10u), k3 = (uint8_t)(team * 11u + 6u);
        int16_t s = cx_attacks_right(w, team) ? 1 : -1;
        if (w->m[k2].state != CX_M_OFF) {
            w->m[k2].x = (int16_t)((CX_LEN / 2 - s * 26) * Q);
            w->m[k2].y = (int16_t)((CX_MID_Y + 46) * Q);
        }
        if (w->m[k3].state != CX_M_OFF) {
            w->m[k3].x = (int16_t)((CX_LEN / 2 - s * 110) * Q);
            w->m[k3].y = (int16_t)((CX_MID_Y - 64) * Q);
        }
    }
    i = (uint8_t)(team * 11u + 9u);
    if (w->m[i].state == CX_M_OFF) i = (uint8_t)(team * 11u + 8u);
    w->m[i].x = (int16_t)((CX_LEN / 2 + (cx_attacks_right(w, team) ? -10 : 10)) * Q);
    w->m[i].y = (int16_t)(CX_MID_Y * Q);
    w->ball.x = (int16_t)(CX_LEN / 2 * Q);
    w->ball.y = (int16_t)(CX_MID_Y * Q);
    w->ball.h = w->ball.vx = w->ball.vy = w->ball.vh = w->ball.curve = 0;
    w->ball.owner = i;
    w->ball.last = i;
    w->restart_team = team;
    w->restart_man = i;
    w->phase = CX_P_KICKOFF;
    w->phase_t = 0;
    w->latch = 0;
    w->off_mask[0] = w->off_mask[1] = 0;
    w->off_man = NONE;
    w->off_live = 0;
    w->adv_t = 0;
    w->ev |= CX_EV_WHISTLE;
}

void CX_CODE cx_goal_for(CxWorld *w, uint8_t team)
{
    w->scorer = w->ball.last;
    w->goal_kind = w->ball.kind;
    /* a deflection or a keeper's fingertips are not an own goal: the striker has scored */
    if (w->scorer != NONE && cx_team_of(w->scorer) != team &&
        w->ball.striker != NONE && cx_team_of(w->ball.striker) == team) w->scorer = w->ball.striker;
    if (w->scorer != NONE && cx_team_of(w->scorer) != team) w->goal_kind = CX_G_OWN;
    else if (w->save_t && w->goal_kind != CX_G_PENALTY && w->ball.last != w->ball.striker) w->goal_kind = CX_G_REBOUND;
    {
        /* his nation's own way a third of the time, otherwise any of the others (the prayer
         * only for the sides it belongs to), and never the same as the side's last one */
        static const uint8_t k_any[12] = { CX_C_RUN, CX_C_KNEE, CX_C_SHIRT, CX_C_BABY, CX_C_DANCE, CX_C_HIPS, CX_C_BELLY,
                                           CX_C_FLIP, CX_C_SAMBA, CX_C_SCREAM, CX_C_SKY, CX_C_SKATE };
        uint16_t r = cx_rnd(w);
        uint8_t c = cx_nation_celebration(w->team[team]), n;
        if ((r & 7u) >= 3u || c == w->celeb_last[team]) {
            for (n = 0; n < 4u; n++) {
                c = k_any[(r >> 3) % 12u];
                if (c != w->celeb_last[team]) break;
                r = cx_rnd(w);
            }
        }
        w->celeb = c;
        w->celeb_last[team] = c;
    }
    if (w->score[team] < 99u) w->score[team]++;
    w->phase = CX_P_GOAL;
    w->phase_t = 0;
    w->kickoff_team = (uint8_t)(team ^ 1u);
    w->ev |= CX_EV_GOAL;
    w->adv_t = 0;
    cx_log(w, CX_LOG_GOAL, w->scorer, NONE, 0);
    if (w->scorer != NONE && cx_team_of(w->scorer) == team) {
        w->m[w->scorer].state = CX_M_CELEB;
        w->m[w->scorer].timer = 250;
    }
    /* a marginal offside in the move: the record has it, and review will find it */
    if (w->var != CX_VAR_OFF && w->var != CX_VAR_CHALLENGE && w->rules == CX_RULES_MODERN && w->off_live && w->off_man != NONE &&
        cx_team_of(w->off_man) == team) {
        w->review = CX_LOG_OFFSIDE;
        w->review_keep = 0;
        w->review_man = w->off_man;
        w->review_team = (uint8_t)(team ^ 1u);
        w->review_x = U(w->m[w->off_man].x);
        w->review_y = U(w->m[w->off_man].y);
    }
}

/* The minute after a goal: the scorer's side run to him, the others hang their heads. */
void CX_CODE cx_goal_scene_step(CxWorld *w)
{
    uint8_t i, sc = w->scorer, piled = 0;
    uint8_t mine;
    if (sc == NONE) return;
    mine = cx_team_of(sc);
    if (w->goal_kind == CX_G_OWN) mine = (uint8_t)(mine ^ 1u);
    /* the scorer wheels away to the supporters: to the touchline on his side of the
     * pitch, short of the corner flag, and celebrates there in front of the stand, his
     * nation's own way, with the camera on him and the crowd behind */
    if (w->m[sc].state == CX_M_CELEB && w->goal_kind != CX_G_OWN) {
        CxMan *m = &w->m[sc];
        int16_t right = cx_attacks_right(w, mine);
        int16_t cx0 = right ? CX_LEN - 130 : 130, cy0 = U(m->y) < CX_MID_Y ? 14 : CX_WID - 14;
        int16_t dx = (int16_t)(cx0 - U(m->x)), dy = (int16_t)(cy0 - U(m->y));
        if (w->phase_t >= 10u && w->phase_t < 170u && cx_abs(dx) + cx_abs(dy) > 12) {
            uint8_t h = (uint8_t)(w->phase_t < 30u ? 2u : 0u);       /* a step to get going */
            uint8_t dir = cx_dir_of(dx, dy);
            m->dir = dir;
            m->vx = cx_vel(cx_kdx[dir], (int16_t)(SPRINT_V + 6 - h * 8));
            m->vy = cx_vel(cx_kdy[dir], (int16_t)(SPRINT_V + 6 - h * 8));
        } else {
            m->vx = m->vy = 0;
        }
    }
    if ((w->phase_t & 7u) != 1u) return;
    for (i = 0; i < CX_MEN; i++) {
        CxMan *m = &w->m[i];
        if (i == sc || !cx_active(w, i) || m->state == CX_M_CELEB) continue;
        if (cx_team_of(i) == mine) {
            /* the nearest three pile on. The hardware shows 96 sprites a line: a whole team
             * in one band of the screen, with the GOAL words over it, is more than that */
            if (m->role != 0 && piled < 3u && cx_dist(m->x, m->y, w->m[sc].x, w->m[sc].y) < 420) {
                piled++;
                m->state = CX_M_RUN;
                if (cx_dist(m->x, m->y, w->m[sc].x, w->m[sc].y) < 36) {
                    /* he has reached him: the side's celebration is done together */
                    m->state = CX_M_CELEB;
                    m->timer = 180;
                    m->vx = m->vy = m->gx = m->gy = 0;
                } else {
                    cx_move_to(m, U(w->m[sc].x), U(w->m[sc].y), SPRINT_V);
                    m->vx = m->gx;
                    m->vy = m->gy;
                }
            } else {
                m->vx = m->vy = 0;
            }
        } else if (m->state == CX_M_RUN && (m->role == 0 || cx_dist(m->x, m->y, w->m[sc].x, w->m[sc].y) < 200)) {
            m->state = CX_M_SAD;
            m->timer = 200;
            m->vx = m->vy = 0;
        }
    }
}

/* ---------------------------------------------------------------- offside */
void CX_CODE cx_offside_mark(CxWorld *w, uint8_t from)
{
    uint8_t t = cx_team_of(from), def = (uint8_t)(t ^ 1u), i, right = cx_attacks_right(w, t), close;
    int16_t pos[11], line, by;
    /* in the direction of attack: distances run toward the goal being attacked; the
     * line and the men past it are ng_soccer's (the passer, the keeper and anyone off
     * the pitch are left out) */
    int16_t sign = right ? 1 : -1;
    uint8_t n = 0;
    for (i = (uint8_t)(def * 11u); i < def * 11u + 11u; i++)
        if (cx_active(w, i)) pos[n++] = (int16_t)(U(w->m[i].x) * sign);
    line = ngs_offside_line(pos, n, (int16_t)(U(w->ball.x) * sign), (int16_t)((CX_LEN / 2) * sign));
    for (i = 0; i < 11u; i++) {
        uint8_t k = (uint8_t)(t * 11u + i);
        pos[i] = (int16_t)((i == 0u || k == from || !cx_active(w, k)) ? -32768 : U(w->m[k].x) * sign);
    }
    w->off_mask[t] = ngs_offside_mask(pos, 11, line, FLAG_MARGIN, &close, &by);
    w->off_mask[def] = 0;
    w->off_man = close == NGS_NOBODY ? NONE : (uint8_t)(t * 11u + close);
    w->off_margin = by;
    w->off_live = 0;
}

uint8_t CX_CODE cx_offside_check(CxWorld *w, uint8_t i)
{
    uint8_t t = cx_team_of(i);
    if (w->off_mask[t] & (1u << (i - t * 11u))) {
        w->ev |= CX_EV_OFFSIDE;
        cx_log(w, CX_LOG_OFFSIDE, i, NONE, 0);
        cx_stop_play(w, CX_P_FREEKICK, (uint8_t)(t ^ 1u), cx_clamp(U(w->m[i].x), 20, CX_LEN - 20), cx_clamp(U(w->m[i].y), 10, CX_WID - 10));
        return 1;
    }
    /* the phase is over once anyone else has played it */
    if (i == w->off_man) w->off_live = 1;
    else if (w->off_man != NONE && cx_team_of(w->off_man) != t) { w->off_man = NONE; w->off_live = 0; }
    w->off_mask[0] = w->off_mask[1] = 0;
    return 0;
}

/* ---------------------------------------------------------------- fouls */
static void CX_CODE send_off(CxWorld *w, uint8_t man)
{
    CxMan *m = &w->m[man];
    uint8_t t = cx_team_of(man);
    m->state = CX_M_OFF;
    m->vx = m->vy = 0;
    m->x = (int16_t)((CX_LEN / 2) * Q);
    m->y = (int16_t)(-40 * Q);
    if (w->reds[t] < 255u) w->reds[t]++;
    if (w->ball.owner == man) w->ball.owner = NONE;
    if (w->ctrl[t] == man) w->ctrl[t] = NONE;
    cx_log(w, CX_LOG_RED, man, NONE, 0);
}

static void CX_CODE show_card(CxWorld *w, uint8_t man, uint8_t red)
{
    CxMan *m = &w->m[man];
    uint8_t t = cx_team_of(man);
    /* a keeper is not sent off here: the arcade game has no one to replace him */
    if (m->role == 0) red = 0;
    m->card++;
    w->card_man = man;
    if (red || m->card >= 2u) {
        w->card_red = 1;
        send_off(w, man);
    } else {
        w->card_red = 0;
        if (w->yellows[t] < 255u) w->yellows[t]++;
        cx_log(w, CX_LOG_CARD, man, NONE, 0);
    }
    w->ev |= CX_EV_CARD;
}

static uint8_t CX_CODE in_box_of(const CxWorld *w, uint8_t team, int16_t x, int16_t y)
{
    /* team's own penalty area */
    int16_t line = cx_attacks_right(w, team) ? 0 : CX_LEN;
    return (uint8_t)(cx_abs((int16_t)(x - line)) < CX_BOX_D && cx_abs((int16_t)(y - CX_MID_Y)) < CX_BOX_W);
}

/* Nobody of the fouling side between the man and the goal but the keeper. */
static uint8_t CX_CODE last_man(const CxWorld *w, uint8_t tackler, uint8_t victim)
{
    uint8_t t = cx_team_of(tackler), i;
    int16_t s = cx_attacks_right(w, t) ? 1 : -1;
    int16_t vx = U(w->m[victim].x);
    for (i = (uint8_t)(t * 11u + 1u); i < t * 11u + 11u; i++) {
        if (i == tackler || !cx_active(w, i)) continue;
        if (U(w->m[i].x) * s < vx * s) return 0;
    }
    return 1;
}

/* The whistle has gone: everything stops while the referee sorts it out, and then the
 * restart he gives. */
void CX_CODE cx_stop_play(CxWorld *w, uint8_t phase, uint8_t team, int16_t x, int16_t y)
{
    CxBall *b = &w->ball;
    uint8_t i;
    w->pend_phase = phase;
    w->pend_team = team;
    w->pend_x = x;
    w->pend_y = y;
    w->phase = CX_P_STOP;
    w->phase_t = 0;
    w->stop_t = 90;
    w->adv_t = 0;
    b->owner = NONE;
    b->vx = b->vy = b->vh = b->curve = 0;
    b->h = 0;
    w->pass_to = NONE;
    for (i = 0; i < CX_MEN; i++) w->m[i].gx = w->m[i].gy = 0;
    w->ev |= CX_EV_WHISTLE;
}

/* The keeper of team t, or NONE with nobody in goal. */
uint8_t CX_CODE cx_keeper_of(const CxWorld *w, uint8_t t)
{
    uint8_t k;
    for (k = (uint8_t)(t * 11u); k < t * 11u + 11u; k++)
        if (w->m[k].role == 0 && w->m[k].state != CX_M_OFF) return k;
    return NONE;
}

/* Inside the six-yard box team t defends. */
uint8_t CX_CODE cx_in_six_of(const CxWorld *w, uint8_t t, int16_t x, int16_t y)
{
    int16_t line = cx_attacks_right(w, t) ? 0 : CX_LEN;
    return (uint8_t)(cx_abs((int16_t)(x - line)) < CX_SIX_D + 8 && cx_abs((int16_t)(y - CX_MID_Y)) < CX_SIX_W + 8);
}

/* A loose ball the keeper is on, or about to be: nobody else may take it there. In his
 * six-yard box he has it if he is near and no further from it than the man who wants it,
 * and nobody takes it from under a keeper who is down on it. */
uint8_t CX_CODE cx_keepers_ball(const CxWorld *w, uint8_t i)
{
    const CxBall *b = &w->ball;
    uint8_t k = cx_keeper_of(w, (uint8_t)(cx_team_of(i) ^ 1u));
    const CxMan *g;
    int16_t dk, di;
    if (k == NONE || b->owner != NONE || U(b->h) >= 30) return 0;
    g = &w->m[k];
    if (g->state == CX_M_ROLL || g->state == CX_M_ARGUE) return 0;
    dk = cx_dist(g->x, g->y, b->x, b->y);
    /* down on it: under his body. A parry that has run away from him is anybody's */
    if ((g->state == CX_M_DIVE || g->state == CX_M_DOWN) && dk < 18) return 1;
    if (!cx_in_six_of(w, cx_team_of(k), U(b->x), U(b->y))) return 0;
    /* a ball played to him, a cut-back across the six: his, unless the keeper is on it */
    if (w->pass_to == i) return (uint8_t)(dk < 12);
    di = cx_dist(w->m[i].x, w->m[i].y, b->x, b->y);
    return (uint8_t)(dk < 30 && dk <= di);
}

/* Hands off the keeper. A man who runs into a keeper holding the ball, or one down on a
 * loose ball, or who stands on him while he holds it, gives away a free kick and is
 * booked: no pinching it from his gloves in a crowd. */
void CX_CODE cx_keeper_guard(CxWorld *w)
{
    const CxBall *b = &w->ball;
    uint8_t t;
    if (w->phase != CX_P_PLAY) return;
    for (t = 0; t < 2u; t++) {
        uint8_t k = cx_keeper_of(w, t), o, hold, on;
        const CxMan *g;
        if (k == NONE) continue;
        g = &w->m[k];
        hold = (uint8_t)(b->owner == k);
        on = (uint8_t)(b->owner == NONE && (g->state == CX_M_DIVE || g->state == CX_M_DOWN) &&
                       U(b->h) < 30 && cx_dist(g->x, g->y, b->x, b->y) < 30);
        if (!hold && !on) continue;
        for (o = (uint8_t)((t ^ 1u) * 11u + 1u); o < (t ^ 1u) * 11u + 11u; o++) {
            const CxMan *a = &w->m[o];
            int32_t dx, dy;
            if (a->state != CX_M_RUN && a->state != CX_M_TACKLE && a->state != CX_M_SLIDE) continue;
            if (cx_dist(a->x, a->y, g->x, g->y) >= 12) continue;
            dx = (int32_t)(g->x - a->x);
            dy = (int32_t)(g->y - a->y);
            /* holding: any contact; on the ground: only a man who comes at him */
            if (hold || (int32_t)a->vx * dx + (int32_t)a->vy * dy > 0) {
                w->keeper_foul = 1;
                cx_foul(w, o, k, 0);
                return;
            }
        }
    }
}

void CX_CODE cx_foul(CxWorld *w, uint8_t tackler, uint8_t victim, uint8_t behind)
{
    uint8_t t = cx_team_of(tackler), vt = (uint8_t)(t ^ 1u);
    int16_t x = U(w->m[victim].x), y = U(w->m[victim].y);
    uint8_t box = in_box_of(w, t, x, y);
    uint8_t last = (uint8_t)(last_man(w, tackler, victim) &&
                             cx_abs((int16_t)(x - (cx_attacks_right(w, t) ? 0 : CX_LEN))) < 340);    /* a clear run at goal */
    uint8_t card = 0, modern = (uint8_t)(w->rules == CX_RULES_MODERN);
    uint8_t keeper = (uint8_t)(w->m[victim].role == 0);
    w->ev |= CX_EV_FOUL;
    if (w->fouls[t] < 255u) w->fouls[t]++;
    /* Laying a finger on a keeper is a booking at the least, and with the ball in his hands
     * it can be worse. Retro: a tackle from behind is a booking. Modern: a last-man foul is a red. */
    if (keeper) card = (uint8_t)(modern && w->ball.owner == victim && w->m[tackler].card ? 2u : 1u);
    else if (behind) card = (uint8_t)(modern && last ? 2u : 1u);
    else if (last && modern) card = 1;
    else if ((cx_rnd(w) & (w->ref_style == 2u ? 3u : (w->ref_style ? 7u : 15u))) == 0) card = 1;
    {
        /* the guilty side gathers round the referee to explain football to him */
        uint8_t k, n = 0;
        for (k = (uint8_t)(t * 11u + 1u); k < t * 11u + 11u && n < 3u; k++) {
            CxMan *a = &w->m[k];
            if (a->state == CX_M_RUN && k != tackler && cx_dist(a->x, a->y, w->m[victim].x, w->m[victim].y) < 150) {
                a->state = CX_M_ARGUE;
                a->timer = 70;
                a->vx = a->vy = 0;
                n++;
            }
        }
    }
    if (behind || keeper || (cx_rnd(w) & 3u) == 0) {
        CxMan *v = &w->m[victim];
        v->state = CX_M_ROLL;
        v->timer = 120;
        v->vx = v->vy = 0;
        w->injured = victim;
        w->ev |= CX_EV_INJURY;
    }
    /* referees are human: now and then the record will show he was wrong */
    w->wrong = (uint8_t)(!behind && !keeper && (cx_rnd(w) & 7u) == 0);
    w->wrong_card = (uint8_t)(card == 1u ? tackler : NONE);
    if (box) {
        cx_log(w, CX_LOG_PEN, tackler, victim, (int16_t)w->wrong);
        if (card) show_card(w, tackler, (uint8_t)(card == 2u));
        cx_stop_play(w, CX_P_PENALTY, vt, (int16_t)(cx_attacks_right(w, vt) ? CX_LEN - CX_SPOT : CX_SPOT), CX_MID_Y);
        /* an automatic review catches a wrong penalty at once */
        if (w->wrong && w->var != CX_VAR_OFF && w->var != CX_VAR_CHALLENGE && modern) {
            w->review = CX_LOG_FOUL;
            w->review_keep = 0;
            w->review_man = tackler;
            w->review_team = t;
            w->phase = CX_P_VAR;
            w->phase_t = 0;
        }
        return;
    }
    cx_log(w, CX_LOG_FOUL, tackler, victim, (int16_t)w->wrong);
    /* the fouled side has it at its feet and is through: he waves play on, and only then */
    if (!last && !card && !keeper) {
        uint8_t near = cx_nearest(w, vt, U(w->ball.x), U(w->ball.y), 0);
        int16_t s = cx_attacks_right(w, vt) ? 1 : -1;
        int16_t to_goal = (int16_t)(s > 0 ? CX_LEN - U(w->ball.x) : U(w->ball.x));
        if (near != victim && cx_dist(w->m[near].x, w->m[near].y, w->ball.x, w->ball.y) < 24 && to_goal < 420) {
            w->adv_t = 70;
            w->adv_team = vt;
            w->adv_man = tackler;
            w->adv_card = 0;
            w->foul_x = x;
            w->foul_y = y;
            w->ev |= CX_EV_ADV;
            return;
        }
    }
    if (card) show_card(w, tackler, (uint8_t)(card == 2u));
    cx_stop_play(w, CX_P_FREEKICK, vt, cx_clamp(x, 20, CX_LEN - 20), cx_clamp(y, 10, CX_WID - 10));
}

/* A side's right to ask: against a goal it has just conceded, or a call just given against it. */
uint8_t CX_CODE cx_var_open(const CxWorld *w, uint8_t team)
{
    if (w->var != CX_VAR_CHALLENGE || !w->challenges[team] || w->review) return 0;
    if (w->phase == CX_P_GOAL && w->phase_t < 150u) {
        uint8_t scorers = w->scorer == NONE ? team : cx_team_of(w->scorer);
        if (w->goal_kind == CX_G_OWN) scorers = (uint8_t)(scorers ^ 1u);
        return (uint8_t)(scorers != team);
    }
    if (w->phase == CX_P_STOP) return (uint8_t)(w->pend_team != team);
    return 0;
}

/* The record decides. A side that was right keeps its chance; a side that was wrong loses it. */
uint8_t CX_CODE cx_var_challenge(CxWorld *w, uint8_t team)
{
    if (!cx_var_open(w, team)) return 0;
    w->asked = team;
    w->ev |= CX_EV_CHALLENGE;
    if (w->phase == CX_P_GOAL) {
        if (w->off_live && w->off_man != NONE) {
            w->review = CX_LOG_OFFSIDE;
            w->review_keep = 0;
            w->review_man = w->off_man;
            w->review_team = team;
            w->review_x = U(w->m[w->off_man].x);
            w->review_y = U(w->m[w->off_man].y);
        } else {
            w->review = CX_LOG_GOAL;
            w->review_keep = 1;
            w->review_man = w->scorer;
            w->review_team = team;
        }
    } else {
        w->review = CX_LOG_FOUL;
        w->review_keep = (uint8_t)!w->wrong;
        w->review_man = w->wrong_card != NONE ? w->wrong_card : (w->log_n ? w->log[(w->log_n - 1u) & (CX_LOG_N - 1u)].man : (uint8_t)(team * 11u + 1u));
        w->review_team = team;
    }
    if (w->review_keep) w->challenges[team]--;
    w->phase = CX_P_VAR;
    w->phase_t = 0;
    return 1;
}

/* He went down with nobody near him. The referee has seen this film before. */
void CX_CODE cx_dive(CxWorld *w, uint8_t man)
{
    CxMan *m = &w->m[man];
    uint8_t vt = (uint8_t)(cx_team_of(man) ^ 1u);
    m->state = CX_M_ROLL;
    m->timer = 90;
    m->vx = m->vy = 0;
    w->ev |= CX_EV_DIVE;
    if (w->rules == CX_RULES_MODERN) {
        show_card(w, man, 0);
        cx_stop_play(w, CX_P_FREEKICK, vt, cx_clamp(U(m->x), 20, CX_LEN - 20), cx_clamp(U(m->y), 10, CX_WID - 10));
    }
}

/* The ball on a defender's arm in his own box. */
static void CX_CODE handball_check(CxWorld *w)
{
    const CxBall *b = &w->ball;
    uint8_t i;
    int16_t h = U(b->h);
    if (b->owner != NONE || h < 10 || h > 34 || (cx_abs(b->vx) + cx_abs(b->vy)) < 30) return;
    for (i = 0; i < CX_MEN; i++) {
        const CxMan *m = &w->m[i];
        uint8_t t = cx_team_of(i);
        if (m->role == 0 || !cx_active(w, i) || b->last == i || cx_team_of(b->last) == t) continue;
        if (cx_dist(m->x, m->y, b->x, b->y) < 12 && in_box_of(w, t, U(m->x), U(m->y)) && (cx_rnd(w) & 15u) == 0) {
            uint8_t vt = (uint8_t)(t ^ 1u);
            w->ev |= (uint32_t)(CX_EV_HANDBALL | CX_EV_FOUL);
            if (w->fouls[t] < 255u) w->fouls[t]++;
            show_card(w, i, 0);
            cx_stop_play(w, CX_P_PENALTY, vt, (int16_t)(cx_attacks_right(w, vt) ? CX_LEN - CX_SPOT : CX_SPOT), CX_MID_Y);
            return;
        }
    }
}

void CX_CODE cx_rules_step(CxWorld *w)
{
    if ((w->frame & 3u) == 0) handball_check(w);
    if (w->phase != CX_P_PLAY) return;
    if (!w->adv_t) return;
    w->adv_t--;
    if (w->ball.owner != NONE && cx_team_of(w->ball.owner) != w->adv_team) {
        /* nothing came of it: back to the foul */
        uint8_t card = w->adv_card;
        int16_t x = w->foul_x, y = w->foul_y;
        uint8_t team = w->adv_team, man = w->adv_man;
        w->adv_t = 0;
        if (card) show_card(w, man, (uint8_t)(card == 2u));
        cx_stop_play(w, CX_P_FREEKICK, team, cx_clamp(x, 20, CX_LEN - 20), cx_clamp(y, 10, CX_WID - 10));
    } else if (!w->adv_t && w->adv_card) {
        show_card(w, w->adv_man, (uint8_t)(w->adv_card == 2u));     /* play went on: the booking still comes */
    }
}

/* ---------------------------------------------------------------- restarts */
static void CX_CODE clear_box(CxWorld *w)
{
    uint8_t t = w->restart_team, i;
    int16_t edge = cx_attacks_right(w, t) ? CX_LEN - CX_BOX_D - 16 : CX_BOX_D + 16;
    int16_t spot = cx_attacks_right(w, t) ? CX_LEN - CX_SPOT : CX_SPOT;
    for (i = 0; i < CX_MEN; i++) {
        CxMan *m = &w->m[i];
        int16_t x = U(m->x);
        if (i == w->restart_man || (m->role == 0 && cx_team_of(i) != t) || !cx_active(w, i)) continue;
        if ((spot > CX_LEN / 2 && x > edge) || (spot < CX_LEN / 2 && x < edge)) cx_move_to(m, edge, U(m->y), RUN_V);
        else m->vx = m->vy = 0;
    }
}

static void CX_CODE take_penalty(CxWorld *w, uint8_t human, uint16_t held, uint16_t foe_held, uint8_t foe_human)
{
    uint8_t t = w->restart_team, i = w->restart_man, k = (uint8_t)((t ^ 1u) * 11u);
    CxMan *keeper = &w->m[k];
    uint8_t guess, aim = (uint8_t)(human ? ((held & CX_UP) ? 1u : ((held & CX_DOWN) ? 2u : 0u)) : (uint8_t)(cx_rnd(w) % 3u));
    int16_t right = cx_attacks_right(w, t);
    int16_t gx = right ? CX_LEN + 8 : -8;
    int16_t gy = (int16_t)(CX_MID_Y + (aim == 1u ? -66 : (aim == 2u ? 66 : 0)));
    int16_t speed = (int16_t)(96 + w->m[i].shot * 2);
    w->phase = CX_P_PLAY;
    w->phase_t = 0;
    w->m[i].state = CX_M_RUN;
    cx_kick_to(w, i, gx, gy, speed, 5);
    w->ball.kind = CX_G_PENALTY;
    w->ball.fast = 1;
    if (w->shots[t] < 255u) w->shots[t]++;
    if (foe_human) guess = (uint8_t)((foe_held & CX_UP) ? 1u : ((foe_held & CX_DOWN) ? 2u : 0u));
    else guess = (uint8_t)(cx_rnd(w) % 3u);
    if (guess) {
        keeper->state = CX_M_DIVE;
        keeper->timer = 26;
        keeper->vy = (int16_t)(guess == 2u ? 56 : -56);
        keeper->dir = (uint8_t)(guess == 2u ? 2 : 6);
    }
}

void CX_CODE cx_restart_step(CxWorld *w, uint16_t held0, uint16_t pressed0, uint16_t held1, uint16_t pressed1)
{
    uint8_t t = w->restart_team, i = w->restart_man;
    uint8_t human = w->human[t];
    uint16_t pressed = t ? pressed1 : pressed0, held = t ? held1 : held0;
    CxMan *m = &w->m[i];
    if (human) w->ctrl[t] = i;
    if (human) {
        int16_t dx = (int16_t)((held & CX_RIGHT) ? 1 : ((held & CX_LEFT) ? -1 : 0));
        int16_t dy = (int16_t)((held & CX_DOWN) ? 1 : ((held & CX_UP) ? -1 : 0));
        /* the aim view (D at a free kick or a corner): the camera goes to a target the
         * stick moves anywhere on the pitch, and the kick goes there */
        if ((w->phase == CX_P_FREEKICK || w->phase == CX_P_CORNER) && (pressed & CX_D)) {
            w->aim_on = (uint8_t)!w->aim_on;
            if (w->aim_on) {
                int16_t gx = cx_attacks_right(w, t) ? CX_LEN : 0, sg = cx_attacks_right(w, t) ? -1 : 1;
                w->aim_x = (int16_t)(gx + sg * (w->phase == CX_P_CORNER ? 90 : 20));
                w->aim_y = CX_MID_Y;
            }
        }
        if (w->aim_on) {
            w->aim_x = cx_clamp((int16_t)(w->aim_x + dx * 5), 10, CX_LEN - 10);
            w->aim_y = cx_clamp((int16_t)(w->aim_y + dy * 4), 10, CX_WID - 10);
        } else if ((dx || dy) && w->phase != CX_P_PENALTY) m->dir = cx_dir_of(dx, dy);
        /* the stick's way as he plays it: held there, the man it goes to runs to meet it */
        w->recv_dir[t] = (dx || dy) ? cx_dir_of(dx, dy) : 0xFFu;
    }
    if (human) w->latch = (uint8_t)(w->latch | (pressed & (CX_A | CX_B | CX_C)));   /* an early press is kept */
    if (w->phase == CX_P_PENALTY) clear_box(w);
    if (w->phase_t < 24) return;
    /* the CPU takes a corner once its men are in the box, or after four seconds */
    if (!human && w->phase == CX_P_CORNER && w->phase_t > 50 && w->phase_t < 240) {
        uint8_t k, ready = 1;
        for (k = (uint8_t)(t * 11u + 1u); k < t * 11u + 11u; k++) {
            const CxMan *a = &w->m[k];
            if (k == i || !cx_active(w, k) || a->line < 2u || (!a->tx && !a->ty)) continue;
            if (cx_abs((int16_t)(U(a->x) - a->tx)) + cx_abs((int16_t)(U(a->y) - a->ty)) > 40) { ready = 0; break; }
        }
        if (!ready) return;
    }
    if ((human && w->latch) || (!human && w->phase_t > 50) || w->phase_t > 240) {
        uint8_t latch = w->latch, phase = w->phase;
        w->latch = 0;
        if (phase == CX_P_PENALTY) {
            if (w->phase_t < 60) { w->latch = latch; return; }
            take_penalty(w, human, held, t ? held0 : held1, w->human[t ^ 1u]);
            return;
        }
        w->phase = CX_P_PLAY;
        w->phase_t = 0;
        if (w->ball.owner != i) { m->state = CX_M_RUN; return; }
        if (phase == CX_P_THROW) {
            /* by hand: A to a man close by, B down the line, C as far as his arms will
             * send it (near their box, a long throw into it) */
            uint8_t how;
            if (human) how = (uint8_t)((latch & CX_C) ? CX_TH_LONG : ((latch & CX_B) ? CX_TH_LINE : CX_TH_FEET));
            else {
                int16_t gx = cx_attacks_right(w, t) ? CX_LEN : 0;
                uint16_t r = (uint16_t)(cx_rnd(w) & 7u);
                if (cx_abs((int16_t)(gx - U(w->ball.x))) < 320 && r < 3u) how = CX_TH_LONG;
                else how = (uint8_t)(r < 5u ? CX_TH_LINE : CX_TH_FEET);
            }
            cx_throw(w, i, how);
            return;
        }
        m->state = CX_M_RUN;
        if (human && w->aim_on && (phase == CX_P_CORNER || phase == CX_P_FREEKICK)) {
            uint8_t k;
            for (k = 0; k < w->fk_wall_n; k++) {
                CxMan *v = &w->m[w->fk_wall[k]];
                if (v->state == CX_M_FEINT) { v->state = CX_M_HEAD; v->timer = 22; }
            }
            w->fk_t = 0;
            m->dir = cx_dir_of((int16_t)(w->aim_x - U(m->x)), (int16_t)(w->aim_y - U(m->y)));
            cx_kick_at(w, i, w->aim_x, w->aim_y, (uint8_t)((latch & CX_B) ? 1u : ((latch & CX_C) ? 2u : 0u)));
            w->aim_on = 0;
            return;
        }
        if (phase == CX_P_CORNER) {
            /* A short, B driven to the near post (with the stick into the pitch, swung to
             * the spot), C floated to the far post */
            uint8_t kind;
            if (human) {
                uint16_t inward = cx_attacks_right(w, t) ? CX_LEFT : CX_RIGHT;
                if (latch & CX_A) kind = CX_CK_SHORT;
                else if (latch & CX_C) kind = CX_CK_FAR;
                else kind = (uint8_t)((held & inward) ? CX_CK_SPOT : CX_CK_NEAR);
            } else {
                uint16_t r = (uint16_t)(cx_rnd(w) & 15u);
                kind = (uint8_t)(r < 2u ? CX_CK_SHORT : (r < 7u ? CX_CK_NEAR : (r < 12u ? CX_CK_FAR : CX_CK_SPOT)));
            }
            cx_corner(w, i, kind);
        } else if (phase == CX_P_FREEKICK) {
            int16_t gx = cx_attacks_right(w, t) ? CX_LEN : 0;
            int16_t range = cx_abs((int16_t)(gx - U(w->ball.x)));
            uint8_t k;
            /* the wall goes up as he strikes it */
            for (k = 0; k < w->fk_wall_n; k++) {
                CxMan *v = &w->m[w->fk_wall[k]];
                if (v->state == CX_M_FEINT) { v->state = CX_M_HEAD; v->timer = 22; }
            }
            w->fk_t = 0;
            if (human ? ((latch & CX_B) != 0) : (range < 360)) {
                /* B over the wall; B with up or down, round it to that side */
                uint8_t round = (uint8_t)(human ? (held & (CX_UP | CX_DOWN)) != 0 : ((cx_rnd(w) & 3u) == 0));
                int8_t side = (int8_t)(human ? ((held & CX_UP) ? -1 : ((held & CX_DOWN) ? 1 : 0)) : 0);
                fk_strike(w, i, (uint8_t)!round, side);
            } else {
                cx_pass(w, i, human, (uint8_t)(human ? ((latch & CX_C) != 0) : (cx_rnd(w) & 1u)));
            }
        } else if (phase == CX_P_GOALKICK) {
            cx_pass(w, i, human, (uint8_t)(human ? !(latch & CX_A) : (cx_rnd(w) & 1u)));
        } else if (phase == CX_P_KICKOFF && human && !(latch & CX_C) && w->recv_dir[t] == 0xFFu) {
            cx_pass_short(w, i);           /* to the man alongside: the move starts from there */
        } else {
            cx_pass(w, i, human, (uint8_t)(human && (latch & CX_C)));
        }
    }
}

void CX_CODE cx_penalty_taken(CxWorld *w, uint8_t scored)
{
    uint8_t t = w->restart_team, i = w->restart_man;
    CxBall *b = &w->ball;
    w->phase = CX_P_PLAY;
    w->phase_t = 0;
    w->m[i].state = CX_M_RUN;
    if (w->shots[t] < 255u) w->shots[t]++;
    b->last = i;
    b->kind = CX_G_PENALTY;
    if (scored) { cx_goal_for(w, t); return; }
    /* saved: the keeper has it */
    cx_ball_take(w, (uint8_t)((t ^ 1u) * 11u));
    w->m[(uint8_t)((t ^ 1u) * 11u)].state = CX_M_HOLD;
    w->m[(uint8_t)((t ^ 1u) * 11u)].timer = w->human[t ^ 1u] ? 210u : 50u;
    w->ev |= CX_EV_SAVE;
}

/* ---------------------------------------------------------------- shoot-out */
void CX_CODE cx_shootout_init(CxShootout *s, uint8_t first, uint32_t seed)
{
    s->score[0] = s->score[1] = 0;
    s->taken[0] = s->taken[1] = 0;
    s->turn = (uint8_t)(first & 1u);
    s->order = s->turn;
    s->over = 0;
    s->winner = NONE;
    s->rng = seed ? seed : 787u;
}

static uint16_t CX_CODE so_roll(CxShootout *s)
{
    uint32_t x = s->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    s->rng = x;
    return (uint16_t)((x >> 8) & 255u);
}

uint8_t CX_CODE cx_pen_roll(uint8_t aim, uint8_t dive, uint8_t taker_skill, uint8_t keeper_skill, uint32_t *dice)
{
    uint32_t x = *dice ? *dice : 787u;
    uint16_t r;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *dice = x;
    r = (uint16_t)((x >> 8) & 255u);
    if (aim == dive) return (uint8_t)(r < (uint16_t)(36u + taker_skill * 6u - (keeper_skill < 9u ? keeper_skill * 3u : 27u)));
    return (uint8_t)(r >= (uint16_t)(10u + (16u - (taker_skill > 16u ? 16u : taker_skill)) * 3u));
}

void CX_CODE cx_shootout_record(CxShootout *s, uint8_t scored)
{
    uint8_t t = s->turn;
    if (s->over) return;
    s->taken[t]++;
    if (scored) s->score[t]++;
    s->turn = (uint8_t)(t ^ 1u);
    /* five each: ends as soon as one side cannot be caught */
    if (s->taken[0] <= 5u && s->taken[1] <= 5u) {
        uint8_t left0 = (uint8_t)(5u - s->taken[0]), left1 = (uint8_t)(5u - s->taken[1]);
        if (s->score[0] > (uint8_t)(s->score[1] + left1)) { s->over = 1; s->winner = 0; }
        else if (s->score[1] > (uint8_t)(s->score[0] + left0)) { s->over = 1; s->winner = 1; }
    }
    /* then level turns decide */
    if (!s->over && s->taken[0] == s->taken[1] && s->taken[0] >= 5u && s->score[0] != s->score[1]) {
        s->over = 1;
        s->winner = (uint8_t)(s->score[0] > s->score[1] ? 0u : 1u);
    }
}

uint8_t CX_CODE cx_shootout_kick(CxShootout *s, uint8_t aim, uint8_t dive, uint8_t taker_skill, uint8_t keeper_skill)
{
    uint8_t scored;
    if (s->over) return 0;
    scored = cx_pen_roll(aim, dive, taker_skill, keeper_skill, &s->rng);
    cx_shootout_record(s, scored);
    return scored;
}

void CX_CODE cx_shootout_auto(CxShootout *s, uint8_t a, uint8_t b)
{
    while (!s->over) {
        uint8_t side = s->turn;
        uint8_t kicker = side ? b : a, keeper = side ? a : b;
        uint8_t aim = (uint8_t)(so_roll(s) % 3u), dive = (uint8_t)(so_roll(s) % 3u);
        (void)cx_shootout_kick(s, aim, dive, cx_squad_stat(kicker, 1), cx_squad_stat(keeper, 5));
    }
}
