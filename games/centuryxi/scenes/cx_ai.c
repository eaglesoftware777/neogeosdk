/*
 * cx_ai.c -- Century XI: the computer's football, in three layers.
 *
 * A side's thinking (mentality, who makes the next run, its substitutions) runs a
 * few times a second. Each man's positional thought runs every eighth frame, in
 * turn, so at most three men think in any one frame. Only the movement toward
 * that place runs every frame. The computer's men have the speeds, reach and ball
 * of a player's: its difficulty is in its decisions (the brain below), with only a
 * step of extra pace for its man pressing the ball on the higher levels.
 *
 * The rule that keeps it football: one man presses the ball, one covers him, and
 * everyone else keeps his place in the shape and his distance from his team-mates.
 * Nobody chases the ball just because it is near. Sides play their own way: a
 * passing side keeps it on the ground, a direct one goes long, a flair side takes
 * men on, a counter side waits and breaks.
 */
#include "cx_internal.h"


/* ------------------------------------------------------------------ the brain
 * Levels 1..8 and each side's ratings, combined through ng_tune curves: for every
 * level the value at the weakest rating and at the strongest. A strong side on a
 * low level still passes better than a weak one, and every level is two bytes to
 * tune. A player's own side plays its team-mates at level 4 with no nudge. */
#define CX_LEVELS     8u
#define CX_LEVEL_MATE 3u

/* frames between the decisions of the man on the ball (skill) */
static const NGTuneSpan k_think_s[CX_LEVELS] = {{26, 20}, {20, 15}, {13, 10}, {11, 7}, {9, 6}, {7, 5}, {5, 4}, {4, 2}};
/* chance in 256 of going in when close (defending) */
static const NGTuneSpan k_bite_s[CX_LEVELS] = {{4, 8}, {8, 12}, {12, 18}, {14, 24}, {17, 28}, {20, 32}, {24, 36}, {28, 42}};
/* speed the presser adds (pressing) */
static const NGTuneSpan k_chase_s[CX_LEVELS] = {{0, 0}, {0, 1}, {0, 1}, {0, 2}, {1, 2}, {1, 3}, {2, 3}, {2, 4}};
/* the keeper's skill shift, 4 = none (keeping) */
static const NGTuneSpan k_keep_s[CX_LEVELS] = {{1, 3}, {2, 4}, {3, 4}, {3, 5}, {4, 5}, {4, 6}, {5, 7}, {6, 8}};
/* added to a shot's spread, 8 = none (power) */
static const NGTuneSpan k_aim_s[CX_LEVELS] = {{24, 18}, {20, 15}, {15, 11}, {11, 6}, {10, 5}, {9, 4}, {8, 4}, {7, 3}};
/* running speed shift, 4 = none (speed) */
static const NGTuneSpan k_pace_s[CX_LEVELS] = {{0, 2}, {1, 3}, {2, 4}, {3, 5}, {4, 5}, {4, 6}, {5, 6}, {5, 7}};
/* how far a pass strays over 300 units (the side's skill and the passer's touch) */
static const NGTuneSpan k_stray_s[CX_LEVELS] = {{40, 24}, {30, 16}, {16, 7}, {10, 3}, {9, 2}, {7, 2}, {5, 1}, {4, 0}};
static const NGTuneSpan k_stray_p[1] = {{6, 0}};        /* a player's own passes: true unless he is a poor passer */
/* chance in 256 of a clean first touch (the receiver's touch) */
static const NGTuneSpan k_touch_s[CX_LEVELS] = {{140, 190}, {150, 200}, {160, 210}, {170, 225}, {180, 232}, {190, 238}, {200, 244}, {210, 250}};
static const NGTuneSpan k_touch_p[1] = {{150, 245}};

static const NGTuneCurve k_think = {k_think_s, CX_LEVELS, NG_TUNE_STRAIGHT};
static const NGTuneCurve k_bite = {k_bite_s, CX_LEVELS, NG_TUNE_STRAIGHT};
static const NGTuneCurve k_chase = {k_chase_s, CX_LEVELS, NG_TUNE_STRAIGHT};
static const NGTuneCurve k_keep = {k_keep_s, CX_LEVELS, NG_TUNE_STRAIGHT};
static const NGTuneCurve k_aim = {k_aim_s, CX_LEVELS, NG_TUNE_EASE_OUT};
static const NGTuneCurve k_pace = {k_pace_s, CX_LEVELS, NG_TUNE_STRAIGHT};
static const NGTuneCurve k_stray = {k_stray_s, CX_LEVELS, NG_TUNE_EASE_OUT};
static const NGTuneCurve k_stray_player = {k_stray_p, 1, NG_TUNE_EASE_OUT};
static const NGTuneCurve k_touch = {k_touch_s, CX_LEVELS, NG_TUNE_STRAIGHT};
static const NGTuneCurve k_touch_player = {k_touch_p, 1, NG_TUNE_STRAIGHT};

/* The CPU and the score: a side behind plays sharper, one far ahead coasts, and a
 * one-goal game near the end is the tightest of all. Each cup round adds a little. */
static const NGTuneNudge k_nudge = {
    {5, 3, 1, 0, 0, -2, -4},      /* early: three or more down .. three or more up */
    {3, 4, 6, 2, 3, 0, -3},       /* late */
    2, 6,                         /* each round played, up to three rounds' worth */
};

/* A 1..16 attribute as a 0..31 rating. */
uint8_t CX_CODE cx_rating16(uint8_t attribute) { return ng_tune_from16(attribute); }

void CX_CODE cx_brain_update(CxWorld *w, uint8_t t)
{
    CxBrain *br = &w->brain[t];
    uint8_t k, team = w->team[t];
    uint8_t late = (uint8_t)(w->half >= 2u || (w->half == 1u && w->clock > (uint16_t)(w->half_frames / 3u * 2u)));
    int16_t gap = (int16_t)((int16_t)w->score[t] - (int16_t)w->score[t ^ 1u]);
    int8_t n;
    /* the nations sit between 9 and 16: stretched over the whole scale, so the gap shows */
    for (k = 0; k < 6u; k++) {
        int16_t st = (int16_t)cx_squad_stat(team, k);
        br->rate[k] = ng_tune_clamp((int16_t)((st - 8) * 4 - 1));
    }
    br->cpu = (uint8_t)!w->human[t];
    if (br->cpu) {
        br->level = ng_tune_level(w->level, w->round, 2u, CX_LEVELS);
        br->nudge = ng_tune_nudge(&k_nudge, gap, late, w->round);
#ifdef CX_STATS_SIDE0          /* host statistics only: side 0 stands in for a player at level 4 */
        if (t == 0u) { br->level = CX_LEVEL_MATE; br->nudge = 0; }
#endif
    } else {
        br->level = CX_LEVEL_MATE;
        br->nudge = 0;
    }
    n = br->nudge;
    br->think = ng_tune_value(&k_think, br->level, (int16_t)(br->rate[3] + n));
    br->bite = ng_tune_value(&k_bite, br->level, (int16_t)(br->rate[4] + n));
    br->chase = ng_tune_value(&k_chase, br->level, (int16_t)(br->rate[2] + n));
    br->keep = br->cpu ? (int8_t)(ng_tune_value(&k_keep, br->level, (int16_t)(br->rate[5] + n)) - 4) : 0;
    br->aim = br->cpu ? (int8_t)(ng_tune_value(&k_aim, br->level, (int16_t)(br->rate[1] + n)) - 8) : 0;
    br->pace = (int8_t)(ng_tune_value(&k_pace, br->level, (int16_t)(br->rate[0] + n)) - 4);
    br->look = (uint8_t)(br->level < 2u ? 15u : 7u);
    if (br->think < 1u) br->think = 1;
}

/* How far a pass from this man may stray, in units over 300. */
uint8_t CX_CODE cx_pass_stray(const CxWorld *w, uint8_t from)
{
    const CxBrain *br = &w->brain[cx_team_of(from)];
    int16_t r = (int16_t)(((int16_t)br->rate[3] + (int16_t)ng_tune_from16(w->m[from].ctrl)) / 2 + br->nudge);
    return br->cpu ? ng_tune_value(&k_stray, br->level, r) : ng_tune_value(&k_stray_player, 0, r);
}

/* The chance in 256 that this man's first touch kills the ball. */
uint8_t CX_CODE cx_touch_chance(const CxWorld *w, uint8_t i)
{
    const CxBrain *br = &w->brain[cx_team_of(i)];
    int16_t r = (int16_t)((int16_t)ng_tune_from16(w->m[i].ctrl) + br->nudge);
    return br->cpu ? ng_tune_value(&k_touch, br->level, r) : ng_tune_value(&k_touch_player, 0, r);
}

void CX_CODE cx_team_think(CxWorld *w, uint8_t t)
{
    int16_t diff = (int16_t)((int16_t)w->score[t] - (int16_t)w->score[t ^ 1u]);
    uint8_t late = (uint8_t)(w->half >= 1u && w->clock > w->half_frames / 2u);
    /* a forward or a wide man makes the next run */
    if ((w->frame & 127u) == 0) {
        uint8_t k, pick = (uint8_t)(cx_rnd(w) % 10u), n;
        for (n = 0; n < 10u; n++) {
            k = (uint8_t)(t * 11u + 1u + (pick + n) % 10u);
            if (cx_active(w, k) && w->m[k].line >= 2u && k != w->ball.owner) { w->runner[t] = k; break; }
        }
    }
    cx_brain_update(w, t);
    if (w->human[t]) { w->mental[t] = w->tactic[t]; return; }      /* the player's own orders */
    /* tired legs in the second half: the CPU's bench comes on */
    if (w->half == 1u && w->subs[t] < 3u && !w->sub_role[t] && w->clock > (uint16_t)(w->half_frames / 3u * (w->subs[t] + 1u))) {
        uint8_t k, worst = 0, lowest = 255;
        for (k = 1; k < 11u; k++) {
            const CxMan *m = &w->m[t * 11u + k];
            if (m->state != CX_M_OFF && m->squad < 11u && m->sta < lowest) { lowest = m->sta; worst = k; }
        }
        if (worst) cx_sub_request(w, t, worst, w->subs[t]);
    }
    w->mental[t] = 1;
    if (late && diff < 0) w->mental[t] = 2;
    else if (late && diff > 0) w->mental[t] = 0;
    else if (diff <= -2) w->mental[t] = 2;
}

/* How far up the pitch the opposing back line stands, in the direction of attack. */
static int16_t CX_CODE their_line(const CxWorld *w, uint8_t t, int16_t s)
{
    uint8_t k, o = (uint8_t)((t ^ 1u) * 11u);
    int16_t a = -30000, b = -30000;
    for (k = o; k < o + 11u; k++) {
        int16_t x;
        if (!cx_active(w, k)) continue;
        x = (int16_t)(U(w->m[k].x) * s);
        if (x > a) { b = a; a = x; } else if (x > b) b = x;
    }
    return (int16_t)(b * s);
}

/* How many of the side's outfielders (not the man on the ball) are nearer him than i. */
static uint8_t CX_CODE support_rank(const CxWorld *w, uint8_t i, uint8_t carrier)
{
    uint8_t t = cx_team_of(i), k, r = 0;
    int16_t d = cx_dist(w->m[i].x, w->m[i].y, w->m[carrier].x, w->m[carrier].y);
    for (k = (uint8_t)(t * 11u + 1u); k < t * 11u + 11u; k++) {
        if (k == i || k == carrier || !cx_active(w, k)) continue;
        if (cx_dist(w->m[k].x, w->m[k].y, w->m[carrier].x, w->m[carrier].y) < d && ++r >= 2u) break;
    }
    return r;
}

/* Where a man wants to be now. Runs one man in eight each frame. */
static void CX_CODE think(CxWorld *w, uint8_t i)
{
    CxMan *m = &w->m[i];
    CxBall *b = &w->ball;
    uint8_t t = cx_team_of(i), k;
    int16_t s = cx_attacks_right(w, t) ? 1 : -1;
    int16_t bx = U(b->x), by = U(b->y);
    int16_t hx = cx_attacks_right(w, t) ? m->hx : (int16_t)(CX_LEN - m->hx);
    uint8_t ours = (uint8_t)((b->owner != NONE && cx_team_of(b->owner) == t) || (w->pass_to != NONE && cx_team_of(w->pass_to) == t));
    int16_t push = (int16_t)(w->mental[t] == 2u ? 70 : (w->mental[t] == 0u ? -70 : 0));
    int16_t own = s > 0 ? 0 : CX_LEN, gx = s > 0 ? CX_LEN : 0;
    int16_t tx, ty;
    /* the shape moves as a block with the ball: it never folds onto it */
    tx = (int16_t)(hx + (ours ? (bx - CX_LEN / 2) + s * 110 : (bx - CX_LEN / 2) * 5 / 8 - s * 50) + s * push);
    ty = (int16_t)(m->hy + cx_div((int16_t)(by - CX_MID_Y), 5));
    /* our keeper has it in his hands: the back men split wide either side of the box to take
     * it from him, a midfielder drops into the space in front of it, the rest go up for the
     * long one (with everyone holding the shape, his roll-outs found nobody) */
    if (b->owner != NONE && b->owner == (uint8_t)(t * 11u) && w->m[b->owner].state == CX_M_HOLD && w->phase == CX_P_PLAY) {
        if (m->line == 1u) {
            tx = (int16_t)(own + s * (m->hy < CX_MID_Y - 120 || m->hy > CX_MID_Y + 120 ? 190 : 120));
            ty = (int16_t)(CX_MID_Y + (m->hy - CX_MID_Y) * 3 / 2);
        } else if (m->line == 2u && (m->role & 1u)) {
            tx = (int16_t)(own + s * 300);
        }
    }
    /* a corner: the attackers take their spots, the defenders hold the posts and stand
     * goal-side of each spot, and one of theirs stays up for the break */
    if (w->phase == CX_P_CORNER && m->role != 0) {
        static const int16_t k_ck_x[6] = {40, 60, 64, CX_SPOT - 10, CX_BOX_D + 20, 70};
        static const int16_t k_ck_y[6] = {56, 0, -72, 16, -40, 0};
        int16_t in = cx_attacks_right(w, w->restart_team) ? CX_LEN : 0, sg = cx_attacks_right(w, w->restart_team) ? -1 : 1;
        int16_t n = (int16_t)(by < CX_MID_Y ? -1 : 1);
        uint8_t slot = (uint8_t)(m->role % 6u);
        if (t == w->restart_team && m->line >= 2u) {
            tx = (int16_t)(in + sg * k_ck_x[slot]);
            ty = slot == 5u ? (int16_t)(by - n * 60) : (int16_t)(CX_MID_Y + n * k_ck_y[slot]);
            m->tx = cx_clamp(tx, 20, CX_LEN - 20);
            m->ty = cx_clamp(ty, 20, CX_WID - 20);
            return;
        }
        if (t != w->restart_team) {
            if (m->role == 2u || m->role == 3u) {
                tx = (int16_t)(in + sg * 8);
                ty = (int16_t)(CX_MID_Y + (m->role == 2u ? n : -n) * (CX_POST - 12));
            } else if (m->role == 10u) {
                tx = (int16_t)(in + sg * (CX_LEN / 2 - 40));
                ty = (int16_t)(CX_MID_Y + n * 60);
            } else {
                tx = (int16_t)(in + sg * (k_ck_x[slot] - 12));
                ty = slot == 5u ? (int16_t)(by - n * 80) : (int16_t)(CX_MID_Y + n * k_ck_y[slot] + 6);
            }
            m->tx = cx_clamp(tx, 20, CX_LEN - 20);
            m->ty = cx_clamp(ty, 20, CX_WID - 20);
            return;
        }
    }
    /* a throw-in: the nearest team-mate comes short for it, the next runs down the line */
    if (w->phase == CX_P_THROW && t == w->restart_team && w->restart_man != NONE) {
        const CxMan *th = &w->m[w->restart_man];
        int16_t d = cx_dist(m->x, m->y, th->x, th->y), inward = (int16_t)(U(th->y) < CX_MID_Y ? 1 : -1);
        uint8_t k, rank = 0;
        for (k = (uint8_t)(t * 11u + 1u); k < t * 11u + 11u; k++)
            if (k != i && k != w->restart_man && cx_active(w, k) && cx_dist(w->m[k].x, w->m[k].y, th->x, th->y) < d) rank++;
        if (rank < 2u) {
            tx = rank == 0u ? (int16_t)(U(th->x) - s * 40) : (int16_t)(U(th->x) + s * 110);
            ty = (int16_t)(U(th->y) + inward * (rank == 0u ? 70 : 34));
            m->tx = cx_clamp(tx, 30, CX_LEN - 30);
            m->ty = cx_clamp(ty, 26, CX_WID - 26);
            return;
        }
    }
    /* set pieces into the box: a free kick close enough to be crossed */
    if (w->phase == CX_P_FREEKICK && cx_abs((int16_t)((cx_attacks_right(w, w->restart_team) ? CX_LEN : 0) - bx)) < 460) {
        int16_t in = cx_attacks_right(w, w->restart_team) ? CX_LEN : 0, sg = cx_attacks_right(w, w->restart_team) ? -1 : 1;
        if (m->line >= 2u || t != w->restart_team) {
            tx = (int16_t)(in + sg * (70 + (m->role & 3u) * 44));
            ty = (int16_t)(CX_MID_Y + ((int16_t)(m->role % 5u) - 2) * 74 + (t == w->restart_team ? 16 : 0));
            m->tx = tx;
            m->ty = ty;
            return;
        }
    }
    if (!ours && m->line == 1u) {
        /* the back line holds one line behind the ball, and it is the line that makes a forward offside */
        int16_t deep = (int16_t)(own + s * 170);
        tx = (int16_t)(bx - s * 200);
        if (cx_sgn((int16_t)(tx - deep), s) < 0) tx = deep;
    } else if (!ours && m->line == 2u) {
        /* the middle sits between the ball and its goal */
        int16_t screen = (int16_t)(bx - s * 90);
        if (cx_sgn((int16_t)(tx - screen), s) > 0) tx = screen;
    } else if (ours && i == w->runner[t] && b->owner != i) {
        /* the run: up to the last defender and a step beyond, into a channel */
        int16_t line = their_line(w, t, s);
        tx = (int16_t)(line + s * (int16_t)((w->frame >> 3) & 31u) - s * 10);
        ty = (int16_t)(m->hy < CX_MID_Y ? CX_MID_Y - 130 : CX_MID_Y + 130);
    } else if (ours && w->human[t] && b->owner != NONE && b->owner != i && m->line >= 1u && support_rank(w, i, b->owner) < 2u) {
        /* beside a player's man on the ball, the two nearest come to help: one alongside him
         * on the side with room, one a few strides behind, so he always has a short pass,
         * sideways or back (the CPU's sides keep their runners going forward) */
        const CxMan *c = &w->m[b->owner];
        int16_t cy = U(c->y), side = (int16_t)(cy < CX_MID_Y ? 1 : -1);
        if (support_rank(w, i, b->owner) == 0u) { tx = (int16_t)(U(c->x) + s * 10); ty = (int16_t)(cy + side * 110); }
        else { tx = (int16_t)(U(c->x) - s * 100); ty = (int16_t)(cy - side * 40); }
    } else if (ours && m->line >= 2u && b->owner != NONE) {
        /* an angle to receive it: ahead and to the side of the man on the ball, not on top of him,
         * and on the right side of the last defender */
        int16_t off = (int16_t)(m->hy < CX_MID_Y ? -130 : 130);
        int16_t cy = U(w->m[b->owner].y);
        int16_t line = their_line(w, t, s);
        if (cx_abs((int16_t)(ty - cy)) < 110) ty = (int16_t)(cy + off);
        if (cx_sgn((int16_t)(tx - bx), s) < 70) tx = (int16_t)(bx + cx_sgn(100, s));
        if (cx_sgn((int16_t)(tx - line), s) > -6) tx = (int16_t)(line - s * 6);
    }
    /* defending, a marker goes with his man: goal-side of him, a stride off, tight near our
     * box and looser further out, where the zone still counts for half */
    if (!ours && m->mark != NONE && w->phase == CX_P_PLAY) {
        const CxMan *o = &w->m[m->mark];
        int16_t ox = U(o->x), oy = U(o->y), near_goal = cx_abs((int16_t)(ox - own));
        int16_t mx = (int16_t)(ox - s * 20), my = (int16_t)(oy + cx_div((int16_t)(CX_MID_Y - oy), 6));
        if (near_goal < 300 && m->line == 1u) { tx = mx; ty = my; }
        else { tx = (int16_t)((tx + mx) / 2); ty = (int16_t)((ty + my) / 2); }
    }
    if (b->h > 8 * Q && b->owner == NONE && ours && m->line == 3u && cx_abs((int16_t)(gx - bx)) < 380) {
        /* a cross in the air: the front men attack the space in front of goal */
        tx = (int16_t)(gx - s * 120);
        ty = (int16_t)(CX_MID_Y + (by - CX_MID_Y) / 2);
    }
    /* keep your distance: from your own men, and from the ball's man most of all */
    for (k = (uint8_t)(t * 11u + 1u); k < t * 11u + 11u; k++) {
        const CxMan *o = &w->m[k];
        int16_t ox, oy, reach;
        if (k == i || !cx_active(w, k)) continue;
        ox = (int16_t)(U(o->x) - tx);
        oy = (int16_t)(U(o->y) - ty);
        reach = (int16_t)(k == b->owner ? 90 : 62);
        if (cx_abs(ox) < reach && cx_abs(oy) < reach) ty = (int16_t)(ty + (oy >= 0 ? -reach / 2 : reach / 2));
    }
    m->tx = cx_clamp(tx, 30, CX_LEN - 30);
    m->ty = cx_clamp(ty, 26, CX_WID - 26);
}

/* Who marks whom while side t defends: their men nearest our goal first, each taken by the
 * nearest of our backs and midfielders still free (not the man pressing the ball, not the
 * player's own man). A man left to his zone only, the shape of old, let the other side's
 * forwards stand free at the edge of the box and shoot. Every eight frames. */
void CX_CODE cx_marks(CxWorld *w, uint8_t t)
{
    uint8_t o0 = (uint8_t)((t ^ 1u) * 11u), base = (uint8_t)(t * 11u), k, j, n;
    int16_t own = cx_attacks_right(w, t) ? 0 : CX_LEN;
    uint8_t pick[4], np = 0;
    int16_t pd[4];
    CxMan *m;
    for (j = (uint8_t)(base + 1u), m = &w->m[j]; j < base + 11u; j++, m++) m->mark = NONE;
    /* their four men nearest our goal line (not the man on the ball: he is pressed), one
     * pass over them keeping the nearest four in order */
    for (k = (uint8_t)(o0 + 1u), m = &w->m[k]; k < o0 + 11u; k++, m++) {
        int16_t d;
        uint8_t at;
        if (m->state == CX_M_OFF || k == w->ball.owner) continue;
        d = cx_abs((int16_t)(U(m->x) - own));
        if (d >= 560) continue;
        at = np < 4u ? np++ : 4u;
        while (at > 0u && pd[at - 1u] > d) { if (at < 4u) { pd[at] = pd[at - 1u]; pick[at] = pick[at - 1u]; } at--; }
        if (at < 4u) { pd[at] = d; pick[at] = k; }
    }
    /* each, most dangerous first, to the nearest of our backs and midfielders still free */
    for (n = 0; n < np; n++) {
        const CxMan *a = &w->m[pick[n]];
        uint8_t best = NONE;
        int16_t bd = 260;
        for (j = (uint8_t)(base + 1u), m = &w->m[j]; j < base + 11u; j++, m++) {
            int16_t d;
            if (m->state == CX_M_OFF || m->line == 3u || m->mark != NONE || j == w->chase[t][0] ||
                (w->human[t] && w->ctrl[t] == j)) continue;
            d = cx_dist(m->x, m->y, a->x, a->y);
            if (d < bd) { bd = d; best = j; }
        }
        if (best != NONE) w->m[best].mark = pick[n];
    }
}

/* Men do not stand inside one another: anyone too close is nudged apart, a few a frame. */
void CX_CODE cx_spacing(CxWorld *w)
{
    uint8_t i = (uint8_t)(w->frame % CX_MEN), k;
    CxMan *a = &w->m[i];
    if (a->state == CX_M_OFF) return;
    for (k = 0; k < CX_MEN; k++) {
        CxMan *o = &w->m[k];
        int16_t dx, dy;
        if (k == i || o->state == CX_M_OFF) continue;
        dx = (int16_t)(a->x - o->x);
        dy = (int16_t)(a->y - o->y);
        if (cx_abs(dx) < 18 * Q && cx_abs(dy) < 18 * Q) {
            a->x = (int16_t)(a->x + (dx >= 0 ? 3 * Q : -3 * Q));
            a->y = (int16_t)(a->y + (dy >= 0 ? 2 * Q : -2 * Q));
        }
    }
}

/* On the ball. Decided a few times a second; between decisions he carries on. */
static void CX_CODE carry(CxWorld *w, uint8_t i, int16_t s, int16_t pace)
{
    CxMan *m = &w->m[i];
    uint8_t t = cx_team_of(i), k, style = cx_squad_style(w->team[t]);
    int16_t gx = s > 0 ? CX_LEN : 0;
    int16_t x = U(m->x), y = U(m->y);
    int16_t to_goal = cx_abs((int16_t)(gx - x));
    int16_t off_mid = cx_abs((int16_t)(y - CX_MID_Y));
    int16_t own_goal = cx_abs((int16_t)((s > 0 ? 0 : CX_LEN) - x));
    int16_t near = 300, ny = 0;
    uint16_t r;
    /* a one-two: the man who gave it to him is running on for it back, first time */
    if (w->wall_t[t] && w->wall_man[t] != NONE && w->wall_man[t] != i && cx_active(w, w->wall_man[t]) &&
        w->hold_t < 8u && cx_sgn((int16_t)(U(w->m[w->wall_man[t]].x) - x), s) > 20) {
        cx_wall_return(w, i);
        return;
    }
    if (w->think_t[t]) { w->think_t[t]--; return; }
    w->think_t[t] = w->brain[t].think;
    for (k = (uint8_t)((t ^ 1u) * 11u); k < (t ^ 1u) * 11u + 11u; k++) {
        int16_t d;
        if (!cx_active(w, k)) continue;
        d = cx_dist(w->m[k].x, w->m[k].y, m->x, m->y);
        if (d < near) { near = d; ny = U(w->m[k].y); }
    }
    r = (uint16_t)(cx_rnd(w) & 255u);
    /* in front of his own goal with a man on him: get rid of it */
    if (near < 40 && own_goal < 260) { cx_pass(w, i, 0, 1); return; }
    /* a shot, if there is a sight of goal: from the edge of the box, now and then from
     * range, and once in a while through the crowd for whatever it hits */
    if (to_goal < 420 && off_mid < 230) {
        uint8_t blocked = cx_lane_blocks(w, t, x, y, gx, CX_MID_Y);
        /* from range only now and then: every CPU goal in the play tests was the same shot
         * from the edge of the box, and a side works it closer, wide or through first */
        uint16_t want = (uint16_t)(to_goal < 130 ? 236u : (blocked ? (to_goal < 240 ? 96u : 24u) : (to_goal < 240 ? 200u : (to_goal < 340 ? 70u : 16u))));
        if (m->type == CX_T_SHOOTER) want = (uint16_t)(want + (to_goal >= 340 ? 40u : 30u));
        if (to_goal >= 340 && off_mid >= 140) want = 0;      /* not from range out wide */
        if (r < want) {
            /* the keeper off his line: lift it over him */
            const CxMan *kp = &w->m[(uint8_t)((t ^ 1u) * 11u)];
            if (to_goal < 300 && m->ctrl >= 12u && cx_abs((int16_t)(gx - U(kp->x))) > 90) cx_chip(w, i, (int8_t)((int16_t)(cx_rnd(w) % 3u) - 1));
            else {
                /* at the side the keeper has left open, mostly; now and then anywhere */
                int8_t a = (int8_t)(U(kp->y) > CX_MID_Y + 8 ? -1 : (U(kp->y) < CX_MID_Y - 8 ? 1 : ((cx_rnd(w) & 1u) ? 1 : -1)));
                if ((cx_rnd(w) & 3u) == 0) a = (int8_t)((int16_t)(cx_rnd(w) % 3u) - 1);
                cx_shoot(w, i, (uint8_t)(10u + (cx_rnd(w) & 15u)), a);
            }
            return;
        }
    }
    /* at the byline, wide of the goal: pulled back along the ground to a man arriving at the
     * edge of the box, the cut-back, for a first-time finish */
    if (to_goal < 160 && off_mid >= 100 && r < 150u) {
        uint8_t k2, best = NONE;
        int16_t bs = -32000;
        for (k2 = (uint8_t)(t * 11u + 1u); k2 < t * 11u + 11u; k2++) {
            const CxMan *c = &w->m[k2];
            int16_t cg, free_ = 200, sc;
            uint8_t o;
            if (k2 == i || !cx_active(w, k2)) continue;
            cg = cx_abs((int16_t)(gx - U(c->x)));
            if (cg < 80 || cg > 280 || cx_abs((int16_t)(U(c->y) - CX_MID_Y)) > 150) continue;
            for (o = (uint8_t)((t ^ 1u) * 11u + 1u); o < (t ^ 1u) * 11u + 11u; o++) {
                int16_t e;
                if (!cx_active(w, o)) continue;
                e = cx_dist(w->m[o].x, w->m[o].y, c->x, c->y);
                if (e < free_) free_ = e;
            }
            sc = (int16_t)(free_ * 2 - cx_abs((int16_t)(cg - 170)));
            if (sc > bs) { bs = sc; best = k2; }
        }
        if (best != NONE && bs > 40) { cx_pass_to(w, i, best, 0); return; }
    }
    /* wide near the byline: cross it, for a header, a volley or the overhead kick */
    if (to_goal < 380 && off_mid >= 160 && r < 140u) { cx_cross(w, i); return; }
    /* in the last third with a man close: give it and go, the one-two */
    if (to_goal < 620 && to_goal > 200 && near < 70 && r >= 200u && r < 230u) {
        w->wall_man[t] = i;
        w->wall_t[t] = 50;
        cx_pass(w, i, 0, 0);
        return;
    }
    /* a run on behind their line: the through ball, if there is a lane */
    {
        uint8_t run = w->runner[t];
        if (to_goal < 820 && run != NONE && run != i && cx_active(w, run) && r >= 160u && r < 200u &&
            cx_sgn((int16_t)(U(w->m[run].x) - x), s) > 80 &&
            !cx_lane_blocks(w, t, x, y, U(w->m[run].x), U(w->m[run].y))) {
            cx_through(w, i, 0);
            return;
        }
    }
    /* a man in front and the feet to beat him */
    if (w->duel_a == i && (m->type == CX_T_DRIBBLE || style == STYLE_FLAIR) && r < 60u) {
        cx_feint(w, i, w->duel_d, (uint8_t)(cx_rnd(w) % 3u));
        return;
    }
    /* a side that plays with flair shows it with room in front: the step-over, the drag
     * back, now and then the rainbow over his own head */
    if ((style == STYLE_FLAIR || m->type == CX_T_DRIBBLE) && near > 30 && near < 80 && r >= 240u) {
        uint16_t k = (uint16_t)(cx_rnd(w) % 3u);
        cx_trick(w, i, (uint16_t)(k == 0u ? CX_UP : (k == 1u ? CX_DOWN : 0u)));
        return;
    }
    /* pass when pressed, or when he has had it long enough for his side's taste */
    {
        uint8_t hold = (uint8_t)(style == STYLE_PASSING ? 95u : (style == STYLE_FLAIR ? 170u : 125u));
        if (m->type == CX_T_DRIBBLE) hold = (uint8_t)(hold + 40u);
        if ((near < 24 && w->hold_t > 18u) || w->hold_t > hold) {
            uint8_t run = w->runner[t];
            /* a run on: the through ball, if there is a lane */
            if (run != NONE && run != i && cx_active(w, run) && (m->type == CX_T_PLAYMAKER || (r & 3u) == 0) &&
                cx_sgn((int16_t)(U(w->m[run].x) - x), s) > 60 &&
                !cx_lane_blocks(w, t, x, y, U(w->m[run].x), U(w->m[run].y))) {
                cx_through(w, i, 0);
                return;
            }
            cx_pass(w, i, 0, (uint8_t)(style == STYLE_DIRECT ? (r & 1u) : ((style == STYLE_COUNTER && own_goal < 500) ? (r & 1u) : ((r & 15u) == 0u))));
            return;
        }
    }
    /* on he goes, round the man in front of him */
    {
        int16_t aim = (int16_t)(off_mid >= 170 && to_goal > 300 ? y : CX_MID_Y + (y - CX_MID_Y) / 2);
        if (near < 70) aim = (int16_t)(y + (ny > y ? -60 : 60));
        cx_move_to8(m, gx, cx_clamp(aim, 30, CX_WID - 30), (int16_t)(DRIBBLE_V + pace / 2 + (m->type == CX_T_DRIBBLE ? 2 : 0)));
    }
}

/* Closing the man on the ball: goal-side of him, and only then a tackle. */
static void CX_CODE press(CxWorld *w, uint8_t i, int16_t s, int16_t pace)
{
    CxMan *m = &w->m[i];
    CxBall *b = &w->ball;
    int16_t bx = U(b->x), by = U(b->y);
    uint8_t t = cx_team_of(i);
    if (b->owner == NONE) {
        /* to where the ball will be: a rolling ball a little ahead, a high one where it drops */
        int16_t lead = 3;
        if (b->h > 0) lead = cx_clamp((int16_t)((b->vh > 0 ? cx_div((int16_t)(b->vh * 2), 3) : 0) + (b->h >> 6) + 3), 3, 34);
        cx_move_to(m, (int16_t)(bx + U(b->vx) * lead / 2), (int16_t)(by + U(b->vy) * lead / 2), (int16_t)(RUN_V + pace));
        return;
    }
    cx_move_to(m, (int16_t)(bx - s * 10), by, (int16_t)(RUN_V + pace + w->brain[t].chase));
    if (cx_team_of(b->owner) != t && !m->timer && w->m[b->owner].role != 0) {
        int16_t d = cx_dist(m->x, m->y, b->x, b->y);
        if (d < 22) {
            uint16_t r = cx_rnd(w) & 255u;
            uint16_t bite = (uint16_t)(w->brain[t].bite + (m->type == CX_T_STOPPER ? 8u : 0u));
            const CxMan *c = &w->m[b->owner];
            uint8_t behind = cx_from_behind(m, c);
            /* goal-side: between his man and his own goal, the only place a slide is safe */
            uint8_t goal_side = (uint8_t)((int16_t)(U(c->x) - U(m->x)) * s >= 0);
            /* a good defender goes to ground only from goal-side and in front, never on a
             * yellow card; from behind he mostly keeps running with his man */
            if (behind) bite = (uint16_t)(bite / 3u);
            if (r < (uint16_t)(bite / 3u) && !behind && goal_side && !m->card) cx_tackle(w, i, 1);
            else if (r < bite) cx_tackle(w, i, 0);
        }
    }
}

/* An outfielder the CPU plays. job: 0 his place in the shape, 1 press, 2 cover the presser. */
void CX_CODE cx_ai_step(CxWorld *w, uint8_t i, uint8_t job)
{
    CxMan *m = &w->m[i];
    CxBall *b = &w->ball;
    uint8_t t = cx_team_of(i);
    int16_t s = cx_attacks_right(w, t) ? 1 : -1;
    int16_t pace = (int16_t)((m->pace >> 2) - cx_tired(m) + w->brain[t].pace);
    if (m->state != CX_M_RUN) return;
    if (b->owner == i) { carry(w, i, s, pace); return; }
    if (w->pass_to == i && b->owner == NONE) {
        /* it is coming to him: he goes to meet it */
        cx_meet_pass(w, i, (int16_t)(RUN_V + pace));
        return;
    }
    if (w->wall_t[t] && w->wall_man[t] == i) {
        cx_move_to(m, (int16_t)(U(m->x) + s * 120), U(m->y), (int16_t)(RUN_V + 3 + pace));      /* the one-two: he goes */
        return;
    }
    if (b->owner != NONE && b->owner != i && cx_team_of(b->owner) != t && w->m[b->owner].role == 0 &&
        cx_dist(m->x, m->y, w->m[b->owner].x, w->m[b->owner].y) < 70) {
        /* their keeper has it: out of his way, back up the pitch */
        cx_move_to(m, (int16_t)(U(m->x) - s * 80), U(m->y), (int16_t)(RUN_V + pace));
        return;
    }
    /* a ball dropping near him: he attacks it, to head it on or away */
    if (b->owner == NONE && w->meet_t && w->phase == CX_P_PLAY && U(b->h) + (b->vh >> 4) > 10) {
        int16_t d = cx_dist(m->x, m->y, (int16_t)(w->meet_x * Q), (int16_t)(w->meet_y * Q));
        if (d < 80 && d < w->meet_t * 2 + 24) {
            cx_move_to(m, w->meet_x, w->meet_y, (int16_t)(RUN_V + 4 + pace));
            return;
        }
    }
    if (job == 1u) { press(w, i, s, pace); return; }
    if (job == 2u) {
        int16_t own = s > 0 ? 0 : CX_LEN;
        int16_t bx = U(b->x), by = U(b->y);
        if (((w->frame + i) & 3u) == 0)
            cx_move_to(m, (int16_t)(bx + cx_div((int16_t)(own - bx), 6)), (int16_t)(by + cx_div((int16_t)(CX_MID_Y - by), 5)), (int16_t)(RUN_V - 2 + pace));
        return;
    }
    if (((w->frame + i) & w->brain[t].look) == 0 || (!m->tx && !m->ty)) think(w, i);
    if (((w->frame + i) & 3u) == 0) cx_move_to(m, m->tx, m->ty, (int16_t)((i == w->runner[t] ? RUN_V : RUN_V - 5) + pace));
}

/* A high ball dropping on a man: a header, or whatever the height allows. */
void CX_CODE cx_ai_headers(CxWorld *w)
{
    uint8_t i, t;
    if (w->ball.owner != NONE || U(w->ball.h) < 8 || U(w->ball.h) > 44) return;
    for (i = 0; i < CX_MEN; i++) {
        CxMan *m = &w->m[i];
        t = cx_team_of(i);
        if (m->role == 0 || m->state != CX_M_RUN || (w->human[t] && w->ctrl[t] == i)) continue;
        /* a pass in the air is its man's: his team-mates leave it, and a defender beats him to
         * it less often when he is there (long balls were headed away before they arrived) */
        if (w->pass_to != NONE && w->pass_to != i && cx_team_of(w->pass_to) == t) continue;
        if (cx_dist(m->x, m->y, w->ball.x, w->ball.y) < 22 &&
            (cx_rnd(w) & (m->type == CX_T_TARGET ? 1u : (w->pass_to != NONE && cx_dist(w->m[w->pass_to].x, w->m[w->pass_to].y, w->ball.x, w->ball.y) < 24 ? 7u : 3u))) == 0) {
            int16_t gx = cx_attacks_right(w, t) ? CX_LEN : 0;
            if (cx_offside_check(w, i)) return;
            cx_aerial(w, i, (uint8_t)(cx_abs((int16_t)(gx - U(m->x))) < 300));
            break;
        }
    }
}
