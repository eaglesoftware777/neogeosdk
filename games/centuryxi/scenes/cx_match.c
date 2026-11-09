/*
 * cx_match.c -- Century XI: the match's fixed step.
 *
 * Twenty-two men and a ball, 60 steps a second, no drawing. One call of cx_tick
 * advances the world from the two sticks; the same inputs and the same seed
 * give the same match. Modules: cx_ball (physics), cx_player (men, feints, the
 * player's man), cx_keeper, cx_ai, cx_rules (laws, restarts, shoot-out),
 * cx_nations (teams and squads).
 */
#include "cx_internal.h"

const int8_t cx_kdx[8] = {16, 11, 0, -11, -16, -11, 0, 11};
const int8_t cx_kdy[8] = {0, 11, 16, 11, 0, -11, -16, -11};

uint16_t CX_CODE cx_rnd(CxWorld *w)
{
    uint32_t x = w->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    w->rng = x;
    return (uint16_t)(x >> 8);
}

/* The 8-way direction nearest to (dx, dy). */
uint8_t CX_CODE cx_dir_of(int16_t dx, int16_t dy)
{
    int16_t ax = cx_abs(dx), ay = cx_abs(dy);
    if (ax > ay * 2) return dx > 0 ? 0 : 4;
    if (ay > ax * 2) return dy > 0 ? 2 : 6;
    if (dx > 0) return dy > 0 ? 1 : 7;
    return dy > 0 ? 3 : 5;
}

void CX_CODE cx_init(CxWorld *w, uint8_t team_l, uint8_t team_r, uint8_t humans, uint8_t stadium)
{
    uint8_t *raw = (uint8_t *)w;
    uint16_t k;
    for (k = 0; k < sizeof(*w); k++) raw[k] = 0;
    w->rng = 0x787C0DE5u ^ ((uint32_t)team_l << 8) ^ team_r;
    w->team[0] = (uint8_t)(team_l < CX_SQUADS ? team_l : 0);
    w->team[1] = (uint8_t)(team_r < CX_SQUADS ? team_r : 1);
    w->human[0] = (uint8_t)(humans & 1u);
    w->human[1] = (uint8_t)((humans >> 1) & 1u);
    w->ctrl[0] = w->ctrl[1] = NONE;
    w->stadium = stadium;
    cx_set_length(w, 5, CX_ET_NORMAL);
    w->pass_to = NONE;
    w->runner[0] = 9;
    w->runner[1] = 20;
    w->wall_man[0] = w->wall_man[1] = NONE;
    w->wrong_card = NONE;
    w->challenges[0] = w->challenges[1] = 2;
    w->off_man = NONE;
    w->injured = NONE;
    w->ref_style = (uint8_t)(w->rng % 3u);
    w->sub_role[0] = w->sub_role[1] = 0;
    w->tactic[0] = w->tactic[1] = 1;
    w->duel_a = w->duel_d = NONE;
    w->rules = CX_RULES_MODERN;
    w->level = 3;                 /* level 4 of 8 */
    w->mental[0] = w->mental[1] = 1;
    cx_field_side(w, 0);
    cx_field_side(w, 1);
    cx_kickoff(w, 0);
}

void CX_CODE cx_set_length(CxWorld *w, uint8_t minutes, uint8_t et_mode)
{
    uint8_t m = minutes < 2u ? 2u : minutes;
    w->half_frames = (uint16_t)(m * 1800u);             /* half the minutes, 3600 frames each */
    w->fpm = (uint16_t)(w->half_frames / 45u);
    w->limit = w->half_frames;
    w->et_mode = et_mode;
}

/* Extra time is a third of a half; the clock shows 15 minutes of it. */
static uint16_t CX_CODE half_length(const CxWorld *w)
{
    return (uint16_t)(w->half >= 2u ? w->half_frames / 3u : w->half_frames);
}

static void CX_CODE next_half(CxWorld *w, uint8_t half)
{
    w->half = half;
    w->clock = 0;
    w->added = 0;
    w->stopped = 0;
    w->stop_ev = 0;
    w->limit = half_length(w);
    /* a rest at the break */
    {
        uint8_t i;
        for (i = 0; i < CX_MEN; i++) w->m[i].sta = (uint8_t)(w->m[i].sta > 215u ? 255u : w->m[i].sta + 40u);
    }
    cx_kickoff(w, (uint8_t)(half & 1u));
}

/* The referee's whistle at the end of a period. */
static void CX_CODE period_over(CxWorld *w)
{
    uint8_t level = (uint8_t)(w->score[0] == w->score[1]);
    w->ev |= CX_EV_WHISTLE;
    w->phase_t = 0;
    switch (w->half) {
    case 0: w->phase = CX_P_HALF; break;
    case 1:
        if (w->et && level) w->phase = w->et_mode == CX_ET_NONE ? CX_P_PENS : CX_P_ETBREAK;
        else w->phase = CX_P_FULL;
        break;
    case 2: w->phase = CX_P_HALF; break;
    default: w->phase = level ? CX_P_PENS : CX_P_FULL; break;
    }
}

/* A review ends: the record's answer stands. */
static void CX_CODE review_done(CxWorld *w)
{
    uint8_t was = w->review, keep = w->review_keep;
    w->review = 0;
    if (was == CX_LOG_OFFSIDE) {
        /* the man was off: no goal, and a free kick where he was */
        uint8_t scored = (uint8_t)(w->review_team ^ 1u);
        if (w->score[scored]) w->score[scored]--;
        cx_restart(w, CX_P_FREEKICK, w->review_team, cx_clamp(w->review_x, 20, CX_LEN - 20), cx_clamp(w->review_y, 10, CX_WID - 10));
    } else if (was == CX_LOG_FOUL) {
        if (keep) { cx_restart(w, w->pend_phase, w->pend_team, w->pend_x, w->pend_y); return; }
        /* he got the ball: no foul, the booking is taken back, and it is theirs */
        if (w->wrong_card != NONE) {
            CxMan *m = &w->m[w->wrong_card];
            uint8_t t = cx_team_of(w->wrong_card);
            if (m->card) m->card--;
            if (w->yellows[t]) w->yellows[t]--;
            w->wrong_card = NONE;
        }
        w->wrong = 0;
        cx_restart(w, CX_P_FREEKICK, w->review_team, cx_clamp(U(w->ball.x), 20, CX_LEN - 20), cx_clamp(U(w->ball.y), 10, CX_WID - 10));
    } else if (w->half >= 2u && w->et_mode == CX_ET_GOLDEN) {
        w->phase = CX_P_FULL;
        w->phase_t = 0;
    } else {
        cx_kickoff(w, w->kickoff_team);
    }
}

/* Who is on top: the side with the ball near the other's goal. The terraces follow this. */
static void CX_CODE momentum_step(CxWorld *w)
{
    uint8_t t, o = w->ball.owner;
    for (t = 0; t < 2u; t++) if (w->momentum[t]) w->momentum[t]--;
    if (o != NONE) {
        uint8_t mine = cx_team_of(o);
        int16_t s = cx_attacks_right(w, mine) ? 1 : -1;
        if ((U(w->ball.x) - CX_LEN / 2) * s > 0 && w->momentum[mine] < 240u) w->momentum[mine] = (uint8_t)(w->momentum[mine] + 6u);
    }
}

void CX_CODE cx_tick(CxWorld *w, uint16_t held0, uint16_t pressed0, uint16_t held1, uint16_t pressed1)
{
    uint8_t i, t;
    /* last turn's hold-ups go on the fourth official's board */
    if (w->ev & (CX_EV_FOUL | CX_EV_CARD | CX_EV_SUB | CX_EV_INJURY | CX_EV_GOAL | CX_EV_OFFSIDE | CX_EV_CHALLENGE)) w->stop_ev = (uint8_t)(w->stop_ev < 240u ? w->stop_ev + 4u : 244u);
    else if ((w->ev & (CX_EV_THROW | CX_EV_OUT)) && w->stop_ev < 244u) w->stop_ev++;
    w->ev = 0;
    w->keeper_foul = 0;
    w->frame++;
    w->phase_t++;
    switch (w->phase) {
    case CX_P_OVER:
        return;
    case CX_P_VAR:
        if (w->phase_t > CX_VAR_FRAMES) review_done(w);
        return;
    case CX_P_STOP:
        for (i = 0; i < CX_MEN; i++) if (w->m[i].state != CX_M_OFF) cx_man_step(w, i);
        /* the CPU asks for a review when its captain thinks it has a case */
        for (t = 0; t < 2u; t++)
            if (!w->human[t] && w->phase_t == 30u && w->wrong && cx_var_open(w, t)) cx_var_challenge(w, t);
        if (w->phase == CX_P_STOP && w->phase_t > w->stop_t) cx_restart(w, w->pend_phase, w->pend_team, w->pend_x, w->pend_y);
        return;
    case CX_P_GOAL:
        cx_goal_scene_step(w);
        for (i = 0; i < CX_MEN; i++) cx_man_step(w, i);
        cx_ball_step(w);
        for (t = 0; t < 2u; t++)
            if (!w->human[t] && w->phase_t == 40u && w->off_live && cx_var_open(w, t)) cx_var_challenge(w, t);
        if (w->phase != CX_P_GOAL) return;
        if (w->phase_t > 250) {
            if (w->review) { w->phase = CX_P_VAR; w->phase_t = 0; w->ev |= CX_EV_WHISTLE; }
            else if (w->half >= 2u && w->et_mode == CX_ET_GOLDEN) { w->phase = CX_P_FULL; w->phase_t = 0; w->ev |= CX_EV_WHISTLE; }
            else cx_kickoff(w, w->kickoff_team);
        }
        return;
    case CX_P_HALF:
        if (w->phase_t > 180) next_half(w, (uint8_t)(w->half + 1u));
        return;
    case CX_P_ETBREAK:
        if (w->phase_t > 200) next_half(w, 2);
        return;
    case CX_P_FULL:
        if (w->phase_t > 200) { w->phase = CX_P_OVER; w->phase_t = 0; }
        return;
    case CX_P_PENS:
        if (w->phase_t > 120) { w->phase = CX_P_OVER; w->phase_t = 0; }
        return;
    default:
        break;
    }
    /* the sides think on different frames: both at once was the dearest tick of the match */
    if ((w->frame & 31u) == 0) { cx_team_think(w, 0); momentum_step(w); }
    else if ((w->frame & 31u) == 16u) cx_team_think(w, 1);
    if ((w->frame & 15u) == 2u) cx_marks(w, 0);
    else if ((w->frame & 15u) == 10u) cx_marks(w, 1);
    cx_spacing(w);
    if (w->wall_t[0]) w->wall_t[0]--;
    if (w->wall_t[1]) w->wall_t[1]--;
    if (w->ball.owner != NONE) { if (w->ball.owner != w->last_owner) { w->last_owner = w->ball.owner; w->hold_t = 0; } else if (w->hold_t < 255u) w->hold_t++; }
    /* The clock never stops, as in the real game, so the match lasts what the option
     * says it lasts; the fourth official adds on for what was lost. */
    {
        uint16_t base = half_length(w);
        if (++w->clock == base && !w->added) {
            /* the time the ball was dead, and what held the game up: three fouls a minute */
            uint16_t extra = (uint16_t)(1u + w->stopped / (uint16_t)(w->fpm * 4u) + w->stop_ev / 12u);
            w->added = (uint8_t)(extra > 5u ? 5u : extra);
            w->limit = (uint16_t)(base + w->added * w->fpm);
            w->ev |= CX_EV_ADDED;
        }
        if (w->phase != CX_P_PLAY && w->stopped < 60000u) w->stopped++;
        if (w->clock >= w->limit && (w->phase == CX_P_PLAY || w->phase == CX_P_THROW || w->phase == CX_P_GOALKICK)) { period_over(w); return; }
        /* legs tire with the minutes, whatever the length of the match */
        if ((w->clock % w->fpm) == 0) {
            for (i = 0; i < CX_MEN; i++) if (w->m[i].sta > 2u) w->m[i].sta = (uint8_t)(w->m[i].sta - (w->m[i].line == 0u ? 0u : 2u));
        }
    }
    if (w->save_t) w->save_t--;
    cx_wall_step(w);
    if (w->phase != CX_P_PLAY) {
        cx_restart_step(w, held0, pressed0, held1, pressed1);
        if (w->phase != CX_P_KICKOFF && w->phase != CX_P_PLAY) {
            /* the others take up their shape while the ball is placed */
            for (i = 0; i < CX_MEN; i++) {
                if (i == w->restart_man || !cx_active(w, i)) continue;
                if (w->phase == CX_P_PENALTY) { cx_man_step(w, i); continue; }
                if (w->m[i].role == 0) cx_keeper_step(w, i);
                /* as in play, a man in the shape has something new to do one tick in four
                 * (all twenty, every tick, made a corner's set-up the dearest second of a
                 * match); a keeper holding it still sends the men near him away at once */
                else if (((w->frame + i) & 3u) == 0u || w->pass_to == i || w->ball.owner == i ||
                         (w->ball.owner != NONE && w->m[w->ball.owner].role == 0) ||
                         (w->wall_t[cx_team_of(i)] && w->wall_man[cx_team_of(i)] == i) || (!w->m[i].tx && !w->m[i].ty))
                    cx_ai_step(w, i, 0);
                cx_man_step(w, i);
            }
            cx_keep_back(w);
        }
        cx_ball_step(w);
        return;
    }
    /* the two nearest outfielders of each side chase the ball: a fresh look
     * four times in a second's fifteen frames is plenty */
    if ((w->frame & 3u) == 0) for (t = 0; t < 2u; t++) {
        uint8_t base = (uint8_t)(t * 11u);
        int16_t d0 = 32000, d1 = 32000;
        w->chase[t][0] = w->chase[t][1] = NONE;
        for (i = (uint8_t)(base + 1u); i < base + 11u; i++) {
            int16_t d;
            if (!cx_active(w, i)) continue;
            d = cx_dist(w->m[i].x, w->m[i].y, w->ball.x, w->ball.y);
            if (d < d0) { d1 = d0; w->chase[t][1] = w->chase[t][0]; d0 = d; w->chase[t][0] = i; }
            else if (d < d1) { d1 = d; w->chase[t][1] = i; }
        }
    }
    for (t = 0; t < 2u; t++) if (w->human[t] && (pressed0 | pressed1 | (uint16_t)(w->frame & 1u))) cx_choose_ctrl(w, t, t ? pressed1 : pressed0);
    if (w->frame & 1u) cx_duel_scan(w);
    for (i = 0; i < CX_MEN; i++) {
        CxMan *m = &w->m[i];
        uint8_t owner;
        if (m->state == CX_M_OFF) continue;
        t = (uint8_t)(i >= 11u);
        owner = w->ball.owner;
        if (m->role == 0) {
            /* a player's keeper with it in his hands: the player throws or kicks it */
            if (w->human[t] && m->state == CX_M_HOLD && owner == i) cx_keeper_human(w, t, i, t ? held1 : held0, t ? pressed1 : pressed0);
            /* a shot coming in: the player has him (cx_choose_ctrl) */
            else if (w->human[t] && w->ctrl[t] == i) cx_keeper_player(w, t, i, t ? held1 : held0, t ? pressed1 : pressed0);
            else cx_keeper_step(w, i);
        } else if (w->human[t] && w->ctrl[t] == i) cx_human_step(w, t, t ? held1 : held0, t ? pressed1 : pressed0);
        else {
            /* one presses, one covers, the rest keep the shape. Beside a player's man nobody
             * presses for him: his nearest team-mate only covers, and takes a ball at his feet.
             * A keeper with the ball is let be: nobody presses him. */
            uint8_t loose = (uint8_t)(owner == NONE), job = 0;
            uint8_t theirs = (uint8_t)(!loose && (owner >= 11u) != t && w->m[owner].role != 0);
            if (loose || theirs) {
                uint8_t first = w->chase[t][0], second = w->chase[t][1];
                if (w->human[t]) {
                    if (first == w->ctrl[t]) first = second;
                    if (i == first) {
                        if (loose) { if (cx_dist(m->x, m->y, w->ball.x, w->ball.y) < 50) job = 1; }
                        else job = 2;
                    }
                } else if (i == first) job = 1;
                else if (i == second) job = theirs ? 2u : (uint8_t)(cx_dist(m->x, m->y, w->ball.x, w->ball.y) < 90 ? 1u : 0u);
            }
            /* A man keeping the shape decides where to run on one tick in four (and thinks on
             * fewer): on the other three there is nothing new for him, and the call alone,
             * for a dozen men a tick, was a fifth of the match's time. Anything that needs
             * him now (the ball, a pass to him, a ball in the air, a one-two) still does.
             * A ball in the air needs only the men it can drop on (cx_ai_step goes to meet it
             * from under 80 units) and a one-two only its runner: with every man called each
             * tick while a cross hung, a corner took the tick from 85 to 130 lines. The cover
             * (job 2) moves on one tick in four like the shape, so it needs no call between. */
            if (((w->frame + i) & 3u) == 0u || job == 1u || w->pass_to == i || owner == i ||
                (w->meet_t && cx_abs((int16_t)(U(m->x) - w->meet_x)) < 80 && cx_abs((int16_t)(U(m->y) - w->meet_y)) < 80) ||
                (w->wall_t[t] && w->wall_man[t] == i) || (!m->tx && !m->ty))
                cx_ai_step(w, i, job);
        }
        if (w->phase != CX_P_PLAY) return;       /* a whistle in the middle of the turn */
    }
    cx_ai_headers(w);
    if (w->phase != CX_P_PLAY) return;
    cx_men_step(w);
    if (w->phase != CX_P_PLAY) return;
    cx_ball_step(w);
    if (w->phase == CX_P_PLAY) cx_keeper_guard(w);
    if (w->phase == CX_P_PLAY) cx_rules_step(w);
}
