/*
 * cx_ball.c -- Century XI: the ball, in world space, and how men strike it.
 *
 * Ground and air are one model: position, velocity, height and vertical speed,
 * gravity in the air, the grass's friction, bounces that lose speed, a swerve for
 * curled shots, and posts, bar and net as things it can hit. It belongs to nobody's
 * animation: a man dribbling only knocks it on and catches it up.
 *
 * Passes are weighted: ngs_pass_speed is the speed that stops a ground ball a little
 * past a point d units away under this very friction, so a pass arrives firm but not
 * running away. A pass is aimed where the man will be when it gets there.
 */
#include "cx_internal.h"

/* The flight, the grass, the weighted passes, the meet point and the posts are the SDK's
 * football engine (ng_soccer); what men do with the ball is here. */
static const NGSPitch k_pitch = { CX_LEN, CX_WID, CX_POST, CX_BAR, GRAVITY };

/* a pass hit firmly enough to arrive moving (a man meets it), not to die at his feet */
static inline int16_t ground_speed(int16_t d) { return ngs_pass_speed(d); }

/* A pass's spot kept on the pitch: a man running for the line is met inside it (led
 * passes to a winger used to drop over the touchline and go out). */
static void CX_CODE on_pitch(int16_t *tx, int16_t *ty)
{
    *tx = cx_clamp(*tx, 24, CX_LEN - 24);
    *ty = cx_clamp(*ty, 20, CX_WID - 20);
}

void CX_CODE cx_ball_take(CxWorld *w, uint8_t i)
{
    CxBall *b = &w->ball;
    b->owner = i;
    b->last = i;
    b->curve = 0;
    b->fast = 0;
    if (w->pass_to == i || w->pass_to == NONE || cx_team_of(w->pass_to) != cx_team_of(i)) w->pass_to = NONE;
    w->first_go[0] = w->first_go[1] = 0;       /* a first-time call is for the pass it was made on */
}

void CX_CODE cx_kick_to(CxWorld *w, uint8_t from, int16_t tx, int16_t ty, int16_t speed, int16_t vh)
{
    CxBall *b = &w->ball;
    ngs_ball_strike(b, from, tx, ty, speed, vh);
    b->kind = CX_G_SHOT;
    w->pass_to = NONE;
    w->first_go[0] = w->first_go[1] = 0;
    w->m[from].state = CX_M_KICK;
    w->m[from].timer = 10;
    w->m[from].kick = CX_K_PASS;
    w->ev |= CX_EV_KICK;
}

/* Opponents standing in the way of a ball along this line. */
uint8_t CX_CODE cx_lane_blocks(const CxWorld *w, uint8_t team, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    /* one-instruction multiplies (cx_mul): written as plain 32-bit products they were library
     * calls, and a long ball looks down every man's lane */
    int16_t dx = (int16_t)(x1 - x0), dy = (int16_t)(y1 - y0);
    int16_t ax = cx_abs(dx), ay = cx_abs(dy);
    int16_t len = (int16_t)(ax > ay ? ax + ay * 3 / 8 : ay + ax * 3 / 8);
    int32_t len2, wide;
    uint8_t k, o = (uint8_t)((team ^ 1u) * 11u), n = 0;
    if (len < 1) return 0;
    len2 = cx_mul(dx, dx) + cx_mul(dy, dy);
    wide = cx_mul(20, len);
    for (k = o; k < o + 11u; k++) {
        int16_t ox, oy;
        int32_t dot, cross;
        if (!cx_active(w, k)) continue;
        ox = (int16_t)(U(w->m[k].x) - x0);
        oy = (int16_t)(U(w->m[k].y) - y0);
        dot = cx_mul(ox, dx) + cx_mul(oy, dy);
        if (dot <= 0 || dot >= len2) continue;
        cross = cx_mul(ox, dy) - cx_mul(oy, dx);
        if (cross < 0) cross = -cross;
        if (cross < wide) n++;
    }
    return n;
}

/* The best man to give it to. With the stick (use_dir) the player's direction decides,
 * and who is nearest that line; without it, the man most free and most forward. */
static uint8_t CX_CODE best_mate(CxWorld *w, uint8_t from, uint8_t use_dir, int16_t lo, int16_t hi)
{
    uint8_t t = cx_team_of(from), base = (uint8_t)(t * 11u), i, k, best = NONE;
    int16_t s = cx_attacks_right(w, t) ? 1 : -1, best_score = -32000;
    const CxMan *f = &w->m[from];
    int16_t fx0 = U(f->x), fy0 = U(f->y);
    for (i = (uint8_t)(base + 1u); i < base + 11u; i++) {
        const CxMan *m = &w->m[i];
        int16_t dx = (int16_t)(U(m->x) - fx0), dy = (int16_t)(U(m->y) - fy0), d, score, near = 200;
        int32_t along;
        if (i == from || !cx_active(w, i)) continue;
        d = cx_dist(m->x, m->y, f->x, f->y);
        if (d < lo || d > hi) continue;
        along = (int32_t)dx * cx_kdx[f->dir] + (int32_t)dy * cx_kdy[f->dir];
        /* the stick's half of the pitch: the man nearest its line first, but a man out to the
         * side still beats a ball into nobody (a third of a play-test half's sideways passes
         * found no-one inside the old 60 degrees and rolled into space) */
        if (use_dir && along < (int32_t)d * 2) continue;
        for (k = (uint8_t)((t ^ 1u) * 11u); k < (t ^ 1u) * 11u + 11u; k++) {
            int16_t e;
            if (!cx_active(w, k)) continue;
            e = cx_dist(w->m[k].x, w->m[k].y, m->x, m->y);
            if (e < near) near = e;
        }
        if (use_dir) {
            /* straight down the stick beats near it; a shorter ball beats a longer one */
            score = (int16_t)((int16_t)(along / (d ? d : 1)) * 30 - d / 3 + near / 2);
        } else {
            score = (int16_t)(dx * s + near - d / 6);
        }
        score = (int16_t)(score - cx_lane_blocks(w, t, fx0, fy0, U(m->x), U(m->y)) * 70);
        if (score > best_score) { best_score = score; best = i; }
    }
    return best;
}

/* No one to find: a touch into the space the stick points at. */
static void CX_CODE knock_on(CxWorld *w, uint8_t from, uint8_t lofted)
{
    const CxMan *f = &w->m[from];
    int16_t reach = lofted ? 300 : 120;
    int16_t tx = (int16_t)(U(f->x) + (cx_kdx[f->dir] * reach) / 16), ty = (int16_t)(U(f->y) + (cx_kdy[f->dir] * reach) / 16);
    on_pitch(&tx, &ty);
    cx_offside_mark(w, from);
    if (lofted) {
        int16_t air = (int16_t)(18 + reach / 18);
        cx_kick_to(w, from, tx, ty, (int16_t)(reach * Q / air), (int16_t)(air * 3 / 2));
        w->m[from].kick = CX_K_LONG;
    } else {
        cx_kick_to(w, from, tx, ty, ground_speed(reach), 0);
    }
    w->ev |= CX_EV_PASS;
}

void CX_CODE cx_pass_to(CxWorld *w, uint8_t from, uint8_t to, uint8_t lofted)
{
    const CxMan *f = &w->m[from], *m = &w->m[to];
    uint8_t t = cx_team_of(from), k;
    int16_t d = cx_dist(m->x, m->y, f->x, f->y), fly, tx, ty, miss;
    cx_offside_mark(w, from);
    /* how long it will take, then where he will be by then */
    fly = lofted ? (int16_t)(18 + d / 18) : (int16_t)(d * 24 / (ground_speed(d) + 1));
    tx = (int16_t)(U(m->x) + (int16_t)((m->vx * fly) >> 5));
    ty = (int16_t)(U(m->y) + (int16_t)((m->vy * fly) >> 5));
    /* a good player finds his man; pressure and distance are what spoil it, and the
     * side's level and ratings set how good is good (cx_pass_stray) */
    miss = (int16_t)((int16_t)cx_pass_stray(w, from) * d / 300);
    for (k = (uint8_t)((t ^ 1u) * 11u); k < (t ^ 1u) * 11u + 11u; k++)
        if (cx_dist(w->m[k].x, w->m[k].y, f->x, f->y) < 24) { miss = (int16_t)(w->human[t] ? miss + 2 : miss * 2 + 4); break; }
    if (f->type == CX_T_PLAYMAKER) miss = (int16_t)(miss / 2);
    if (miss > 0) {
        tx = (int16_t)(tx + (int16_t)(cx_rnd(w) % (uint16_t)(miss * 2 + 1)) - miss);
        ty = (int16_t)(ty + (int16_t)(cx_rnd(w) % (uint16_t)(miss * 2 + 1)) - miss);
    }
    on_pitch(&tx, &ty);
    d = cx_dist((int16_t)(tx * Q), (int16_t)(ty * Q), f->x, f->y);
    if (lofted) {
        /* a long ball is driven, not floated: it gets there before a defender does */
        int16_t air = (int16_t)(18 + d / 18);
        cx_kick_to(w, from, tx, ty, (int16_t)(d * Q / air + 2), (int16_t)(air * 3 / 2));
        w->m[from].kick = CX_K_LONG;
    } else {
        /* and the weight: a stray pass is a little under- or over-hit too */
        int16_t pace = ground_speed(d);
        if (miss > 1) pace = (int16_t)(pace + (int16_t)(cx_rnd(w) % (uint16_t)(miss + 1)) - miss / 2);
        cx_kick_to(w, from, tx, ty, pace, 0);
    }
    w->pass_to = to;
    if (w->human[t]) w->ctrl[t] = to;
    w->ev |= CX_EV_PASS;
}

/* A set piece played to a spot the player picked in the aim view: along the ground
 * weighted to stop there (A), lofted to drop there (C), or struck at it (B, a shot: a
 * low drive bending toward the middle of the goal). */
void CX_CODE cx_kick_at(CxWorld *w, uint8_t from, int16_t tx, int16_t ty, uint8_t how)
{
    const CxMan *f = &w->m[from];
    uint8_t t = cx_team_of(from);
    int16_t d = cx_dist((int16_t)(tx * Q), (int16_t)(ty * Q), f->x, f->y);
    if (how == 0u) {
        cx_offside_mark(w, from);
        cx_kick_to(w, from, tx, ty, ground_speed(d), 0);
        w->ev |= CX_EV_PASS;
    } else if (how == 2u) {
        int16_t air = (int16_t)(24 + d / 16);
        cx_offside_mark(w, from);
        cx_kick_to(w, from, tx, ty, (int16_t)(d * Q / air + 2), (int16_t)(air * 3 / 2));
        w->m[from].kick = CX_K_LONG;
        w->ev |= CX_EV_PASS;
    } else {
        int16_t gx = cx_attacks_right(w, t) ? CX_LEN : 0;
        cx_kick_to(w, from, tx, ty, (int16_t)(170 + f->shot * 2), 10);
        w->ball.curve = cx_bend_toward(&w->ball, (int16_t)(gx - U(f->x)), (int16_t)(CX_MID_Y - ty), 4);
        w->ball.kind = CX_G_FREEKICK;
        w->ball.fast = 1;
        w->m[from].kick = CX_K_SHOT;
        if (w->shots[t] < 255u) w->shots[t]++;
        w->ev |= CX_EV_SHOT;
    }
}

/* A long lob: high and floated, dropping in front of the man as he runs on. */
void CX_CODE cx_lob_to(CxWorld *w, uint8_t from, uint8_t to)
{
    const CxMan *f = &w->m[from], *m = &w->m[to];
    uint8_t t = cx_team_of(from);
    /* lower and quicker than it was (a second's hang let every defender near get under
     * it, and half a player's long balls to a marked man were lost): the man it is for
     * takes it on his chest as it drops */
    int16_t d = cx_dist(m->x, m->y, f->x, f->y), air = (int16_t)(22 + d / 16), tx, ty, miss;
    cx_offside_mark(w, from);
    tx = (int16_t)(U(m->x) + (int16_t)((m->vx * air) >> 5));
    ty = (int16_t)(U(m->y) + (int16_t)((m->vy * air) >> 5));
    miss = (int16_t)((int16_t)cx_pass_stray(w, from) * d / 300);
    if (miss > 0) {
        tx = (int16_t)(tx + (int16_t)(cx_rnd(w) % (uint16_t)(miss * 2 + 1)) - miss);
        ty = (int16_t)(ty + (int16_t)(cx_rnd(w) % (uint16_t)(miss * 2 + 1)) - miss);
    }
    on_pitch(&tx, &ty);
    d = cx_dist((int16_t)(tx * Q), (int16_t)(ty * Q), f->x, f->y);
    cx_kick_to(w, from, tx, ty, (int16_t)(d * Q / air + 2), (int16_t)(air * 3 / 2 + 2));
    w->m[from].kick = CX_K_LONG;
    w->pass_to = to;
    if (w->human[t]) w->ctrl[t] = to;
    w->ev |= CX_EV_PASS;
}

/* The long ball's man: the one furthest up the pitch in the stick's direction (or
 * anywhere upfield with the stick at rest), free of a marker if possible. */
static uint8_t CX_CODE long_mate(CxWorld *w, uint8_t from, uint8_t use_dir)
{
    uint8_t t = cx_team_of(from), base = (uint8_t)(t * 11u), i, k, best = NONE;
    int16_t s = cx_attacks_right(w, t) ? 1 : -1, best_score = -32000;
    const CxMan *f = &w->m[from];
    for (i = (uint8_t)(base + 1u); i < base + 11u; i++) {
        const CxMan *m = &w->m[i];
        int16_t dx = (int16_t)(U(m->x) - U(f->x)), dy = (int16_t)(U(m->y) - U(f->y)), d, near = 200, score;
        if (i == from || !cx_active(w, i)) continue;
        d = cx_dist(m->x, m->y, f->x, f->y);
        if (d < 200 || d > 760) continue;
        if (use_dir && (int32_t)dx * cx_kdx[f->dir] + (int32_t)dy * cx_kdy[f->dir] < (int32_t)d * 8) continue;
        for (k = (uint8_t)((t ^ 1u) * 11u); k < (t ^ 1u) * 11u + 11u; k++) {
            int16_t e;
            if (!cx_active(w, k)) continue;
            e = cx_dist(w->m[k].x, w->m[k].y, m->x, m->y);
            if (e < near) near = e;
        }
        /* free beats far: up the pitch counts, but a man with a marker on him loses the
         * ball (half of a player's long balls in play tests), and the longest balls are the
         * least sure */
        if (near > 160) near = 160;
        score = (int16_t)(cx_sgn(dx, s) / 3 + near * 2 - cx_abs((int16_t)(d - 420)) / 4 -
                          cx_lane_blocks(w, t, U(f->x), U(f->y), U(m->x), U(m->y)) * 12);
        if (score > best_score) { best_score = score; best = i; }
    }
    return best;
}

void CX_CODE cx_pass(CxWorld *w, uint8_t from, uint8_t cone, uint8_t lob)
{
    uint8_t to = lob ? long_mate(w, from, cone) : NONE;
    if (lob && to != NONE) { cx_lob_to(w, from, to); return; }
    to = best_mate(w, from, cone != 0u, lob ? 150 : 30, lob ? 640 : 320);
    /* no man in that direction on a long ball: try anyone within reach of a short one */
    if (to == NONE && cone && !lob) to = best_mate(w, from, 1, 20, 480);
    /* nobody that side at all (a winger pointing at the touchline): the best man anywhere,
     * not a ball rolled out of play */
    if (to == NONE && cone && !lob) to = best_mate(w, from, 0, 20, 320);
    /* cone 2 (a player standing still): nobody the way he faces, the man most free close by,
     * then further */
    if (to == NONE && cone == 2u && !lob) to = best_mate(w, from, 0, 30, 200);
    if (to == NONE && cone == 2u) to = best_mate(w, from, 0, 30, lob ? 640 : 320);
    if (to != NONE) { cx_pass_to(w, from, to, lob); return; }
    /* nobody on: the player's stick says where, and the computer's man clears it upfield,
     * never the way he happens to be facing, which may be his own goal */
    if (!cone) w->m[from].dir = (uint8_t)(cx_attacks_right(w, cx_team_of(from)) ? 0 : 4);
    knock_on(w, from, lob);
}

/* A short one to the man most free close by (a kick-off with the stick at rest: the
 * forward alongside him). */
void CX_CODE cx_pass_short(CxWorld *w, uint8_t from)
{
    uint8_t to = best_mate(w, from, 0, 20, 160);
    if (to == NONE) { cx_pass(w, from, 2, 0); return; }
    cx_pass_to(w, from, to, 0);
}

/* The keeper's long kick: the man furthest up the pitch the stick's way, or with the
 * stick at rest the front man most free nearest the middle (a ball down the middle is
 * a ball every forward can attack). */
static uint8_t CX_CODE punt_mate(CxWorld *w, uint8_t from, uint8_t aimed)
{
    uint8_t t = cx_team_of(from), base = (uint8_t)(t * 11u), i, k, best = NONE;
    int16_t s = cx_attacks_right(w, t) ? 1 : -1, best_score = -32000;
    const CxMan *f = &w->m[from];
    for (i = (uint8_t)(base + 1u); i < base + 11u; i++) {
        const CxMan *m = &w->m[i];
        int16_t dx = (int16_t)(U(m->x) - U(f->x)), dy = (int16_t)(U(m->y) - U(f->y)), d, near = 200, score;
        if (!cx_active(w, i) || dx * s < 160) continue;
        d = cx_dist(m->x, m->y, f->x, f->y);
        if (d < 220 || d > 780) continue;
        if (aimed && (int32_t)dx * cx_kdx[f->dir] + (int32_t)dy * cx_kdy[f->dir] < (int32_t)d * 8) continue;
        for (k = (uint8_t)((t ^ 1u) * 11u); k < (t ^ 1u) * 11u + 11u; k++) {
            int16_t e;
            if (!cx_active(w, k)) continue;
            e = cx_dist(w->m[k].x, w->m[k].y, m->x, m->y);
            if (e < near) near = e;
        }
        /* free beats far: the man furthest up is the one they mark (most of a player's long
         * kicks to him were lost in play tests) */
        if (near > 140) near = 140;
        score = (int16_t)(dx * s / 4 - cx_abs((int16_t)(U(m->y) - CX_MID_Y)) / (aimed ? 4 : 3) + near * 2);
        if (score > best_score) { best_score = score; best = i; }
    }
    return best;
}

/* A keeper's ball out of his hands, as a player plays it. CX_KD_SHORT rolled out to a
 * man close by (the stick's way if it points); CX_KD_PUNT kicked long (punt_mate), to
 * the centre circle if nobody is up; CX_KD_THROW thrown long and flat over his head to
 * a man out wide or in midfield. */
void CX_CODE cx_keeper_out(CxWorld *w, uint8_t from, uint8_t how, uint8_t aimed)
{
    CxMan *f = &w->m[from];
    CxBall *b = &w->ball;
    uint8_t t = cx_team_of(from), to;
    int16_t s = cx_attacks_right(w, t) ? 1 : -1;
    f->state = CX_M_RUN;
    if (!aimed) f->dir = (uint8_t)(s > 0 ? 0 : 4);
    if (how == CX_KD_PUNT) {
        to = punt_mate(w, from, aimed);
        /* nobody up the stick's way: the best man anywhere, not a kick into empty grass
         * (a player's punts often went "to nobody" and were the other side's) */
        if (to == NONE && aimed) to = punt_mate(w, from, 0);
        if (to != NONE) { cx_lob_to(w, from, to); return; }
        {
            int16_t d = cx_dist((int16_t)(CX_LEN / 2 * Q), (int16_t)(CX_MID_Y * Q), f->x, f->y), air = (int16_t)(28 + d / 14);
            cx_kick_to(w, from, CX_LEN / 2, CX_MID_Y, (int16_t)(d * Q / air + 2), (int16_t)(air * 3 / 2 + 4));
            f->kick = CX_K_LONG;
            w->ev |= CX_EV_PASS;
        }
        return;
    }
    if (how == CX_KD_THROW) {
        int16_t d, air, tx, ty;
        to = best_mate(w, from, aimed, 110, 420);
        if (to == NONE) to = best_mate(w, from, 0, 110, 420);
        if (to == NONE) { cx_pass(w, from, aimed, 0); return; }
        /* where he will be when it gets there */
        d = cx_dist(w->m[to].x, w->m[to].y, f->x, f->y);
        air = (int16_t)(14 + d / 12);
        tx = (int16_t)(U(w->m[to].x) + (int16_t)((w->m[to].vx * air) >> 5));
        ty = (int16_t)(U(w->m[to].y) + (int16_t)((w->m[to].vy * air) >> 5));
        on_pitch(&tx, &ty);
        d = cx_dist((int16_t)(tx * Q), (int16_t)(ty * Q), f->x, f->y);
        cx_offside_mark(w, from);
        cx_kick_to(w, from, tx, ty, (int16_t)(d * Q / air + 2), (int16_t)(air * 3 / 2 - 6));
        b->h = 26 * Q;                     /* from over his head */
        f->kick = CX_K_PASS;
        w->pass_to = to;
        if (w->human[t]) w->ctrl[t] = to;
        w->ev |= CX_EV_PASS;
        return;
    }
    to = best_mate(w, from, aimed, 40, 280);
    if (to == NONE) to = best_mate(w, from, 0, 40, 300);
    if (to == NONE) to = best_mate(w, from, 0, 40, 460);     /* nobody close: the nearest who is free */
    if (to != NONE) cx_pass_to(w, from, to, 0);
    else knock_on(w, from, 0);
}

/* Into the space ahead of a man already running: he gets there, a defender does not. */
void CX_CODE cx_through(CxWorld *w, uint8_t from, uint8_t cone)
{
    const CxMan *f = &w->m[from];
    uint8_t t = cx_team_of(from), i, to = NONE;
    int16_t s = cx_attacks_right(w, t) ? 1 : -1, best = -32000;
    for (i = (uint8_t)(t * 11u + 1u); i < t * 11u + 11u; i++) {
        const CxMan *m = &w->m[i];
        int16_t dx = (int16_t)(U(m->x) - U(f->x)), dy = (int16_t)(U(m->y) - U(f->y)), d, score;
        if (i == from || !cx_active(w, i)) continue;
        d = cx_dist(m->x, m->y, f->x, f->y);
        if (dx * s < 20 || d > 460) continue;
        if (cone && (int32_t)dx * cx_kdx[f->dir] + (int32_t)dy * cx_kdy[f->dir] < (int32_t)d * 5) continue;
        score = (int16_t)(dx * s + (m->vx * s) * 2 - cx_abs(dy) / 2 + (m->type == CX_T_SPEED ? 40 : 0));
        if (score > best) { best = score; to = i; }
    }
    if (to == NONE) { knock_on(w, from, 0); return; }
    {
        const CxMan *m = &w->m[to];
        int16_t tx = cx_clamp((int16_t)(U(m->x) + s * 110), 20, CX_LEN - 20);
        int16_t ty = (int16_t)(U(m->y) + (CX_MID_Y - U(m->y)) / 6);
        int16_t d = cx_dist((int16_t)(tx * Q), (int16_t)(ty * Q), f->x, f->y);
        int16_t miss = (int16_t)((int16_t)cx_pass_stray(w, from) * d / 300);
        if (miss > 0) {
            tx = (int16_t)(tx + (int16_t)(cx_rnd(w) % (uint16_t)(miss * 2 + 1)) - miss);
            ty = (int16_t)(ty + (int16_t)(cx_rnd(w) % (uint16_t)(miss * 2 + 1)) - miss);
        }
        cx_offside_mark(w, from);
        cx_kick_to(w, from, tx, ty, (int16_t)(ground_speed(d) + 8), 0);
        w->pass_to = to;
        if (w->human[t]) w->ctrl[t] = to;
        w->runner[t] = to;
        w->ev |= (uint32_t)(CX_EV_PASS | CX_EV_THROUGH);
    }
}

/* Behind him without looking. */
void CX_CODE cx_backheel(CxWorld *w, uint8_t from)
{
    CxMan *f = &w->m[from];
    uint8_t keep = f->dir, to;
    f->dir = (uint8_t)((f->dir + 4u) & 7u);
    to = best_mate(w, from, 1, 20, 170);
    if (to != NONE) cx_pass_to(w, from, to, 0);
    else knock_on(w, from, 0);
    f->dir = keep;
    w->ev |= CX_EV_BACKHEEL;
}

/* The second half of a one-two: first time, into the path of the man who passed it. */
void CX_CODE cx_wall_return(CxWorld *w, uint8_t from)
{
    uint8_t t = cx_team_of(from), to = w->wall_man[t];
    const CxMan *m = &w->m[to];
    int16_t s = cx_attacks_right(w, t) ? 1 : -1;
    int16_t tx = cx_clamp((int16_t)(U(m->x) + s * 90), 20, CX_LEN - 20), ty = U(m->y);
    int16_t d = cx_dist((int16_t)(tx * Q), (int16_t)(ty * Q), w->m[from].x, w->m[from].y);
    cx_offside_mark(w, from);
    cx_kick_to(w, from, tx, ty, ground_speed(d), 0);
    w->pass_to = to;
    w->wall_go[t] = 0;
    w->wall_t[t] = 40;
    if (w->human[t]) w->ctrl[t] = to;
    w->ev |= (uint32_t)(CX_EV_PASS | CX_EV_ONETWO);
}

/* A throw-in: from over his head, with both hands, and no further than an arm sends it.
 * CX_TH_FEET to a man close by, CX_TH_LINE down the line ahead of a man running up it,
 * CX_TH_LONG as far as his arms go: near the box, a long throw into it. */
void CX_CODE cx_throw(CxWorld *w, uint8_t from, uint8_t how)
{
    CxMan *f = &w->m[from];
    CxBall *b = &w->ball;
    uint8_t t = cx_team_of(from), far = (uint8_t)(how == CX_TH_LONG);
    int16_t s = cx_attacks_right(w, t) ? 1 : -1, gx = s > 0 ? CX_LEN : 0;
    uint8_t to = NONE;
    int16_t tx, ty, d, air;
    if (how == CX_TH_LONG && cx_abs((int16_t)(gx - U(f->x))) < 320) {
        /* into the box: towards the near post, for a head to flick on */
        tx = (int16_t)(gx - s * 70);
        ty = (int16_t)(CX_MID_Y + (U(f->y) < CX_MID_Y ? -40 : 40));
    } else {
        if (how == CX_TH_LINE) {
            /* the man furthest up the line within reach: thrown into the space ahead of him */
            uint8_t k;
            int16_t best = 0;
            for (k = (uint8_t)(t * 11u + 1u); k < t * 11u + 11u; k++) {
                const CxMan *m = &w->m[k];
                int16_t up = (int16_t)((U(m->x) - U(f->x)) * s);
                if (k == from || !cx_active(w, k) || up < 30 || up > 220 || cx_abs((int16_t)(U(m->y) - U(f->y))) > 120) continue;
                if (up > best) { best = up; to = k; }
            }
        }
        if (to == NONE) to = best_mate(w, from, 1, 30, far ? 300 : 170);
        if (to == NONE) to = best_mate(w, from, 0, 30, far ? 300 : 170);
        if (to != NONE) {
            tx = U(w->m[to].x);
            ty = U(w->m[to].y);
            if (how == CX_TH_LINE) { tx = (int16_t)(tx + s * 60); ty = (int16_t)(ty + (U(f->y) < CX_MID_Y ? -10 : 10)); }
        } else {
            int16_t reach = far ? 190 : 90;
            tx = (int16_t)(U(f->x) + (cx_kdx[f->dir] * reach) / 16);
            ty = (int16_t)(U(f->y) + (cx_kdy[f->dir] * reach) / 16);
        }
    }
    d = cx_dist((int16_t)(tx * Q), (int16_t)(ty * Q), f->x, f->y);
    air = (int16_t)(16 + d / 10);
    cx_kick_to(w, from, tx, ty, (int16_t)(d * Q / air + 2), (int16_t)(air * 3 / 2 - 8));
    b->h = 28 * Q;                         /* it leaves from above his head */
    w->pass_to = to;
    if (to != NONE && w->human[cx_team_of(from)]) w->ctrl[cx_team_of(from)] = to;
    f->state = CX_M_THROWN;
    f->timer = 16;
    w->ev |= CX_EV_THROW;
}

/* A lofted ball that swerves towards (bx, by): aimed off by as far as the swerve
 * will bring it back, so it still drops where it was meant to. air: frames aloft;
 * amount: the swerve in quarters of a Q a frame. */
static void CX_CODE loft_bent(CxWorld *w, uint8_t from, int16_t tx, int16_t ty, int16_t air, int16_t rise,
                              int16_t bx, int16_t by, int16_t amount)
{
    const CxMan *f = &w->m[from];
    int16_t fx = U(f->x), fy = U(f->y), dx = (int16_t)(tx - fx), dy = (int16_t)(ty - fy);
    int16_t ax = cx_abs(dx), ay = cx_abs(dy), len = (int16_t)(ax > ay ? ax + ay * 3 / 8 : ay + ax * 3 / 8);
    /* clockwise (positive) when the swerve's side is to the right of the flight */
    int16_t c = (int16_t)(((int32_t)(-dy) * bx + (int32_t)dx * by) >= 0 ? amount : -amount);
    /* sideways drift over the flight: (c/4) t^2 / 2 in Q, so c t^2 / 128 units */
    int16_t drift = (int16_t)((int32_t)amount * air * air / 128), d;
    if (len < 1) len = 1;
    tx = (int16_t)(tx - (int16_t)((int32_t)(c > 0 ? -dy : dy) * drift / len));
    ty = (int16_t)(ty - (int16_t)((int32_t)(c > 0 ? dx : -dx) * drift / len));
    d = cx_dist((int16_t)(tx * Q), (int16_t)(ty * Q), f->x, f->y);
    cx_kick_to(w, from, tx, ty, (int16_t)(d * Q / air + 2), rise);
    w->ball.curve = c;
    w->m[from].kick = CX_K_LONG;
    w->ev |= CX_EV_PASS;
}

/* A corner: CX_CK_NEAR driven to the near post and bending in, CX_CK_FAR floated to the
 * far post and bending away, CX_CK_SPOT swung in towards the penalty spot, CX_CK_SHORT
 * rolled to a man close by. No offside from a corner. */
void CX_CODE cx_corner(CxWorld *w, uint8_t from, uint8_t kind)
{
    uint8_t t = cx_team_of(from);
    int16_t right = cx_attacks_right(w, t);
    int16_t gx = right ? CX_LEN : 0, in = right ? -1 : 1;
    int16_t by = U(w->ball.y), n = (int16_t)(by < CX_MID_Y ? -1 : 1);       /* the near side */
    int16_t tx, ty, d, air, miss;
    uint8_t k, o = (uint8_t)((t ^ 1u) * 11u), at = NONE;
    if (kind == CX_CK_SHORT) {
        int16_t best = 140;
        for (k = (uint8_t)(t * 11u + 1u); k < t * 11u + 11u; k++) {
            int16_t e;
            if (k == from || !cx_active(w, k)) continue;
            e = cx_dist(w->m[k].x, w->m[k].y, w->m[from].x, w->m[from].y);
            if (e < best) { best = e; at = k; }
        }
        if (at != NONE) { cx_pass_to(w, from, at, 0); return; }
        kind = CX_CK_SPOT;
    }
    if (kind == CX_CK_NEAR) { tx = (int16_t)(gx + in * 40); ty = (int16_t)(CX_MID_Y + n * 56); }
    else if (kind == CX_CK_FAR) { tx = (int16_t)(gx + in * 64); ty = (int16_t)(CX_MID_Y - n * 72); }
    else { tx = (int16_t)(gx + in * (CX_SPOT - 24)); ty = (int16_t)(CX_MID_Y + n * 16); }
    /* a poor delivery strays (the taker's touch, the side's level) */
    d = cx_dist((int16_t)(tx * Q), (int16_t)(ty * Q), w->m[from].x, w->m[from].y);
    miss = (int16_t)((int16_t)cx_pass_stray(w, from) * d / 300);
    if (w->m[from].type == CX_T_SETPIECE) miss = (int16_t)(miss / 2);
    if (miss > 0) {
        tx = (int16_t)(tx + (int16_t)(cx_rnd(w) % (uint16_t)(miss * 2 + 1)) - miss);
        ty = (int16_t)(ty + (int16_t)(cx_rnd(w) % (uint16_t)(miss * 2 + 1)) - miss);
    }
    (void)o;
    if (kind == CX_CK_NEAR) {
        air = (int16_t)(20 + d / 18);
        loft_bent(w, from, tx, ty, air, (int16_t)(air * 3 / 2 - 4), (int16_t)(gx - tx), (int16_t)(CX_MID_Y - ty), 3);
    } else if (kind == CX_CK_FAR) {
        /* away from goal: into the pitch */
        air = (int16_t)(30 + d / 12);
        loft_bent(w, from, tx, ty, air, (int16_t)(air * 3 / 2), in, 0, 2);
    } else {
        air = (int16_t)(26 + d / 14);
        loft_bent(w, from, tx, ty, air, (int16_t)(air * 3 / 2), (int16_t)(gx - tx), (int16_t)(CX_MID_Y - ty), 2);
    }
    w->ball.kind = CX_G_SHOT;
}

/* From wide: over the defenders to the far side of the six-yard box, dropping to head height. */
void CX_CODE cx_cross(CxWorld *w, uint8_t from)
{
    const CxMan *f = &w->m[from];
    uint8_t t = cx_team_of(from);
    int16_t right = cx_attacks_right(w, t);
    int16_t tx = (int16_t)(right ? CX_LEN - 130 : 130);
    int16_t ty = (int16_t)(U(f->y) < CX_MID_Y ? CX_MID_Y + 50 : CX_MID_Y - 50);
    int16_t d = cx_dist((int16_t)(tx * Q), (int16_t)(ty * Q), f->x, f->y);
    int16_t miss = (int16_t)((int16_t)cx_pass_stray(w, from) * d / 300), air;
    if (miss > 0) {
        tx = (int16_t)(tx + (int16_t)(cx_rnd(w) % (uint16_t)(miss * 2 + 1)) - miss);
        ty = (int16_t)(ty + (int16_t)(cx_rnd(w) % (uint16_t)(miss * 2 + 1)) - miss);
        d = cx_dist((int16_t)(tx * Q), (int16_t)(ty * Q), f->x, f->y);
    }
    air = (int16_t)(22 + d / 14);
    cx_offside_mark(w, from);
    cx_kick_to(w, from, tx, ty, (int16_t)(d * Q / air + 2), (int16_t)(air * 3 / 2));
    w->m[from].kick = CX_K_LONG;
    w->ev |= CX_EV_PASS;
}

void CX_CODE cx_shoot(CxWorld *w, uint8_t from, uint8_t power, int8_t aim)
{
    uint8_t t = cx_team_of(from);
    const CxMan *m = &w->m[from];
    int16_t gx = cx_attacks_right(w, t) ? CX_LEN + 8 : -8;
    int16_t spread = (int16_t)(26 - m->shot);           /* the better the shot, the truer */
    int16_t gy;
    uint8_t k;
    /* with a man in his face it flies wider */
    for (k = (uint8_t)((t ^ 1u) * 11u + 1u); k < (t ^ 1u) * 11u + 11u; k++)
        if (cx_dist(w->m[k].x, w->m[k].y, m->x, m->y) < 30) { spread = (int16_t)(spread + spread / 2 + 6); break; }
    if (m->type == CX_T_SHOOTER) spread = (int16_t)(spread * 2 / 3);
    spread = (int16_t)(spread + w->brain[t].aim);                /* the CPU's level */
    if (power > 22u) spread = (int16_t)(spread + (power - 22u) / 2u);   /* a full-blooded one is wilder */
    if (spread < 1) spread = 1;
    gy = (int16_t)(CX_MID_Y + aim * 62 + (int16_t)((cx_rnd(w) % (unsigned)(spread * 2 + 1)) - spread));
    {
        /* a tap is a firm shot, a full wind-up a thunderbolt */
        int16_t speed = (int16_t)(132 + (power * 5) / 2 + m->shot + (m->type == CX_T_SHOOTER ? 10 : 0));
        int16_t vh = (int16_t)(power > 18 ? 6 + (power - 18) * 2 : 4);
        int16_t range = cx_abs((int16_t)(gx - U(m->x)));
        cx_kick_to(w, from, gx, gy, speed, vh);
        if (w->shots[t] < 255u) w->shots[t]++;
        /* an aimed corner bends in a touch */
        w->ball.curve = cx_bend_y(&w->ball, (int16_t)(aim * -4));
        w->ball.kind = range > 320 ? CX_G_LONG : (range < 90 ? CX_G_TAPIN : CX_G_SHOT);
        w->ball.fast = 1;
        w->m[from].kick = CX_K_SHOT;
        w->ev |= CX_EV_SHOT;
    }
}

/* Over the keeper: slow and high, dropping under the bar. */
void CX_CODE cx_chip(CxWorld *w, uint8_t from, int8_t aim)
{
    uint8_t t = cx_team_of(from);
    const CxMan *m = &w->m[from];
    int16_t gx = cx_attacks_right(w, t) ? CX_LEN + 8 : -8;
    int16_t range = cx_abs((int16_t)(gx - U(m->x)));
    int16_t air = (int16_t)(26 + range / 14);
    cx_kick_to(w, from, gx, (int16_t)(CX_MID_Y + aim * 50), (int16_t)(range * Q / air), (int16_t)(air * 3 / 2));
    if (w->shots[t] < 255u) w->shots[t]++;
    w->ball.kind = CX_G_CHIP;
    w->m[from].kick = CX_K_LONG;
    w->ev |= CX_EV_SHOT;
}

/* A man meeting a ball in the air: how he meets it depends on its height, how
 * far he must stretch and which way he is facing. to_goal: he goes for goal. */
void CX_CODE cx_aerial(CxWorld *w, uint8_t i, uint8_t to_goal)
{
    CxMan *m = &w->m[i];
    CxBall *b = &w->ball;
    uint8_t t = cx_team_of(i);
    int16_t right = cx_attacks_right(w, t);
    int16_t gx = right ? CX_LEN + 8 : -8;
    int16_t h = U(b->h), d = cx_dist(m->x, m->y, b->x, b->y);
    int16_t gy = (int16_t)(CX_MID_Y + (int16_t)(cx_rnd(w) % 140u) - 70);
    int16_t facing = (int16_t)(cx_kdx[m->dir] * (right ? 1 : -1));
    uint8_t kind, state, ev_special = 1;
    int16_t speed, range = cx_abs((int16_t)(gx - U(m->x)));
    /* at goal only from where a goal can come: B in his own half volleyed it at a goal a
     * thousand units off. Out there it is headed or volleyed on, up the pitch from deep. */
    uint8_t clear = (uint8_t)(to_goal && range > 520);
    if (clear) to_goal = 0;
    if (!to_goal) {
        /* on to a man, or cleared: met at the height it comes, a volley at the waist, now and
         * then the overhead kick when it drops on him (most of all with his back to where it
         * goes), a header otherwise */
        cx_pass(w, i, 1, (uint8_t)(clear && range > CX_LEN / 2));
        if (h < 14) { m->state = CX_M_VOLLEY; m->timer = 16; w->ev |= CX_EV_SPECIAL; }
        else if (h >= 22 && (facing < 0 || (cx_rnd(w) & 3u) == 0)) { m->state = CX_M_BIKE; m->timer = 24; w->ev |= CX_EV_SPECIAL; }
        else { m->state = CX_M_HEAD; m->timer = 14; }
        return;
    }
    /* waist high: a volley; chest high with his back to goal, or now and then when it
     * drops on him, the overhead scissor kick; a stretch away, the diving header; else
     * a header. All struck hard: a ball met in the air has its own pace in it. */
    if (h < 16) { kind = CX_G_VOLLEY; state = CX_M_VOLLEY; speed = (int16_t)(150 + m->shot); }
    else if (h >= 20 && (facing < 0 || (h >= 26 && (cx_rnd(w) & 3u) == 0))) { kind = CX_G_BIKE; state = CX_M_BIKE; speed = (int16_t)(140 + m->shot); }
    else if (d > 24) { kind = CX_G_DIVING; state = CX_M_FLY; speed = (int16_t)(126 + m->shot); }
    else { kind = CX_G_HEADER; state = CX_M_HEAD; speed = (int16_t)(116 + m->shot + (m->type == CX_T_TARGET ? 14 : 0)); ev_special = 0; }
    cx_kick_to(w, i, gx, gy, speed, kind == CX_G_HEADER ? 2 : 6);
    b->kind = kind;
    b->fast = 1;
    if (w->shots[t] < 255u) w->shots[t]++;
    m->state = state;
    m->timer = (uint8_t)(state == CX_M_FLY ? 22 : (state == CX_M_BIKE ? 26 : 16));
    if (state == CX_M_FLY) {
        m->vx = cx_vel(cx_kdx[m->dir], 44);
        m->vy = cx_vel(cx_kdy[m->dir], 44);
    }
    w->ev |= (uint32_t)(CX_EV_SHOT | (ev_special ? CX_EV_SPECIAL : 0u));
}

/* Posts and bar: a ball reaching the line near one of them comes back. */
static uint8_t CX_CODE frame_hit(CxWorld *w, int16_t off, uint8_t right)
{
    if (!ngs_frame_test(&w->ball, &k_pitch, off)) return 0;
    ngs_frame_rebound(&w->ball, &k_pitch, right, cx_rnd(w));
    w->ev |= CX_EV_POST;
    return 1;
}

/* A ball in the air: when and where it comes down to head height, or to the grass
 * if it never climbs that high. Once a frame, for the keepers and the men under it. */
#define MEET_H (24 * Q)
static void CX_CODE meet_point(CxWorld *w)
{
    w->meet_t = ngs_ball_meet(&w->ball, MEET_H, GRAVITY, &w->meet_x, &w->meet_y);
}

void CX_CODE cx_ball_step(CxWorld *w)
{
    CxBall *b = &w->ball;
    int16_t x, y;
    if (b->owner != NONE) {
        /* Dribbling is a run of touches, not a ball stuck to a boot. At a walk it stays
         * under his foot; at a run he knocks it on and catches it up, further the faster
         * he goes; and on a turn it swings round late. A keeper has it in his hands, and
         * a man taking a throw-in holds it over his head. */
        const CxMan *m = &w->m[b->owner];
        int16_t pace = (int16_t)(cx_abs(m->vx) + cx_abs(m->vy));
        int16_t reach = 9;
        if (m->state == CX_M_THROW) {
            b->x = m->x;
            b->y = m->y;
            b->h = 28 * Q;
            b->vx = b->vy = b->vh = 0;
            return;
        }
        if (m->role != 0 && pace > 14) {
            uint8_t beat = (uint8_t)(m->anim & 3u);
            uint8_t touch = (uint8_t)(beat < 2u ? beat : 4u - beat);
            /* closer touches than before at a run: at full tilt he still has to chase it */
            reach = (int16_t)(10 + touch * (pace > 40 ? 8 : (pace > 28 ? 6 : (pace > 20 ? 4 : 2))));
        }
        b->x = (int16_t)(b->x + cx_div((int16_t)((int16_t)(m->x + cx_kdx[m->dir] * reach) - b->x), 3));
        b->y = (int16_t)(b->y + cx_div((int16_t)((int16_t)(m->y + cx_kdy[m->dir] * reach) - b->y), 3));
        b->h = 0;
        b->vx = m->vx;
        b->vy = m->vy;
        b->vh = 0;
        if (m->vx || m->vy) b->spin++;
        w->meet_t = 0;
        return;
    }
    /* flight, swerve, gravity and drag, bounces and the grass (ng_soccer) */
    if (ngs_ball_fly(b, GRAVITY, w->frame) & NGS_FLY_BOUNCE) w->ev |= CX_EV_BOUNCE;
    meet_point(w);
    /* a shot through a crowd: a defender in its path blocks it, back into play or off
     * him and wide (and a deflection over his own line is a corner) */
    if (b->fast && b->kind != CX_G_PENALTY && U(b->h) < 20 && b->striker != NONE && w->phase == CX_P_PLAY) {
        uint8_t o = (uint8_t)((cx_team_of(b->striker) ^ 1u) * 11u), k;
        for (k = (uint8_t)(o + 1u); k < o + 11u; k++) {
            CxMan *d = &w->m[k];
            if (k == b->last || d->state != CX_M_RUN) continue;
            if (cx_abs((int16_t)(d->x - b->x)) < 7 * Q && cx_abs((int16_t)(d->y - b->y)) < 5 * Q) {
                if (cx_rnd(w) & 1u) break;                        /* past him or through his legs */
                if (cx_rnd(w) & 1u) {
                    b->vx = (int16_t)(-b->vx / 4);
                    b->vy = (int16_t)(b->vy / 2 + (int16_t)(cx_rnd(w) % 41u) - 20);
                } else {
                    b->vx = (int16_t)(b->vx / 2);
                    b->vy = (int16_t)(b->vy + ((cx_rnd(w) & 1u) ? 48 : -48));
                }
                b->vh = (int16_t)(6 + (int16_t)(cx_rnd(w) & 7u));
                b->curve = 0;
                b->fast = 0;
                b->last = k;
                w->ev |= CX_EV_BOUNCE;
                break;
            }
        }
    }
    x = U(b->x);
    y = U(b->y);
    if (w->phase == CX_P_GOAL) {
        /* in the net: it settles there, and no line counts it again */
        if (x < -20 || x > CX_LEN + 20) {
            b->x = (int16_t)((x < 0 ? -20 : CX_LEN + 20) * Q);
            b->vx = (int16_t)(-b->vx / 4);
            b->vy = (int16_t)(b->vy / 2);
        }
        return;
    }
    /* the goal lines */
    if (x < 0 || x > CX_LEN) {
        uint8_t right = (uint8_t)(x > CX_LEN);
        uint8_t attacker = cx_attacks_right(w, 0) == right ? 0 : 1;    /* the side shooting at this goal */
        int16_t off = cx_abs((int16_t)(y - CX_MID_Y));
        if (off < CX_POST && U(b->h) < CX_BAR) { cx_goal_for(w, attacker); return; }
        if (frame_hit(w, off, right)) return;
        w->ev |= CX_EV_OUT;
        if (b->kind != CX_G_SHOT || U(b->h) > 4 || cx_abs(b->vx) > 50) w->ev |= CX_EV_MISS;
        if (b->last != NONE && cx_team_of(b->last) != attacker)
            cx_restart(w, CX_P_CORNER, attacker, (int16_t)(right ? CX_LEN - 4 : 4), (int16_t)(y < CX_MID_Y ? 4 : CX_WID - 4));
        else
            cx_restart(w, CX_P_GOALKICK, (uint8_t)(attacker ^ 1u), (int16_t)(right ? CX_LEN - 80 : 80),
                       (int16_t)(y < CX_MID_Y ? CX_MID_Y - 120 : CX_MID_Y + 120));
        return;
    }
    if (y < 0 || y > CX_WID) {
        uint8_t team = b->last != NONE ? (uint8_t)(cx_team_of(b->last) ^ 1u) : 0;
        w->ev |= CX_EV_OUT;
        cx_restart(w, CX_P_THROW, team, cx_clamp(x, 20, CX_LEN - 20), (int16_t)(y < 0 ? 0 : CX_WID));
    }
}
