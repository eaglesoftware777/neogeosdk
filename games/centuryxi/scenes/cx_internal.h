/* Shared by the match modules; nothing outside the simulation includes this. */
#ifndef CX_INTERNAL_H
#define CX_INTERNAL_H

#include "cx_sim.h"
#include "ng_tune.h"

#define Q          CX_Q
#define U(v)       ((int16_t)((v) >> 4))
#define RUN_V      34
#define SPRINT_V   46
#define DRIBBLE_V  31
#define REACH      12
#define HEAD_MIN   12
#define HEAD_MAX   36
#define GRAVITY    3
#define NONE       CX_NOBODY

/* how a side likes to play (cx_squad_style) */
enum { STYLE_PASSING, STYLE_DIRECT, STYLE_FLAIR, STYLE_COUNTER };

extern const int8_t cx_kdx[8];
extern const int8_t cx_kdy[8];

uint16_t CX_CODE cx_rnd(CxWorld *w);
uint8_t CX_CODE cx_dir_of(int16_t dx, int16_t dy);

/* Small and called constantly: a call would cost more than the work. */
static inline int16_t cx_abs(int16_t v) { return (int16_t)(v < 0 ? -v : v); }
static inline int16_t cx_clamp(int16_t v, int16_t lo, int16_t hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline uint8_t cx_team_of(uint8_t i) { return (uint8_t)(i >= 11u); }
static inline uint8_t cx_active(const CxWorld *w, uint8_t i) { return (uint8_t)(w->m[i].state != CX_M_OFF); }

/* Distance in units, the larger axis plus half the smaller (close enough). */
static inline int16_t cx_dist(int16_t ax, int16_t ay, int16_t bx, int16_t by)
{
    int16_t dx = cx_abs((int16_t)(U(ax) - U(bx))), dy = cx_abs((int16_t)(U(ay) - U(by)));
    return (int16_t)(dx > dy ? dx + dy / 2 : dy + dx / 2);
}

/* speed along one axis of direction component k (0, 11 or 16 of 16): word-sized
 * maths, where k * speed as C writes it is a library call on the 68000. */
static inline int16_t cx_vel(int8_t k, int16_t speed)
{
    int16_t v;
    if (k == 0) return 0;
    v = (k == 16 || k == -16) ? speed : (int16_t)((int16_t)(speed * 11) >> 4);
    return (int16_t)(k < 0 ? -v : v);
}

/* Swerves (ng_soccer): one that pushes a ball towards +y on screen when it flies right
 * and towards -y when it flies left (the way a shot or free kick is aimed), and one of
 * `amount` that turns its flight towards (dx, dy). b->curve turns the flight clockwise
 * when positive, whichever way it flies. */
static inline int16_t cx_bend_y(const CxBall *b, int16_t push_y) { return ngs_bend_y(b, push_y); }
static inline int16_t cx_bend_toward(const CxBall *b, int16_t dx, int16_t dy, int16_t amount)
{
    return ngs_bend_toward(b, dx, dy, amount);
}

/* cx_ai.c: the brain (ng_tune curves) */
void CX_CODE cx_brain_update(CxWorld *w, uint8_t t);
uint8_t CX_CODE cx_pass_stray(const CxWorld *w, uint8_t from);
uint8_t CX_CODE cx_touch_chance(const CxWorld *w, uint8_t i);
uint8_t CX_CODE cx_rating16(uint8_t attribute);

/* cx_ball.c */
void CX_CODE cx_kick_to(CxWorld *w, uint8_t from, int16_t tx, int16_t ty, int16_t speed, int16_t vh);
void CX_CODE cx_pass(CxWorld *w, uint8_t from, uint8_t cone, uint8_t lob);
void CX_CODE cx_pass_to(CxWorld *w, uint8_t from, uint8_t to, uint8_t lofted);
void CX_CODE cx_pass_short(CxWorld *w, uint8_t from);
void CX_CODE cx_lob_to(CxWorld *w, uint8_t from, uint8_t to);
void CX_CODE cx_kick_at(CxWorld *w, uint8_t from, int16_t tx, int16_t ty, uint8_t how);
void CX_CODE cx_through(CxWorld *w, uint8_t from, uint8_t cone);
void CX_CODE cx_backheel(CxWorld *w, uint8_t from);
void CX_CODE cx_wall_return(CxWorld *w, uint8_t from);
void CX_CODE cx_throw(CxWorld *w, uint8_t from, uint8_t how);
void CX_CODE cx_corner(CxWorld *w, uint8_t from, uint8_t kind);
uint8_t CX_CODE cx_lane_blocks(const CxWorld *w, uint8_t team, int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void CX_CODE cx_shoot(CxWorld *w, uint8_t from, uint8_t power, int8_t aim);
void CX_CODE cx_chip(CxWorld *w, uint8_t from, int8_t aim);
void CX_CODE cx_cross(CxWorld *w, uint8_t from);
void CX_CODE cx_aerial(CxWorld *w, uint8_t i, uint8_t to_goal);
void CX_CODE cx_ball_step(CxWorld *w);
void CX_CODE cx_ball_take(CxWorld *w, uint8_t i);

/* cx_rules.c */
void CX_CODE cx_restart(CxWorld *w, uint8_t phase, uint8_t team, int16_t x, int16_t y);
void CX_CODE cx_kickoff(CxWorld *w, uint8_t team);
void CX_CODE cx_restart_step(CxWorld *w, uint16_t held0, uint16_t pressed0, uint16_t held1, uint16_t pressed1);
void CX_CODE cx_goal_for(CxWorld *w, uint8_t team);
void CX_CODE cx_foul(CxWorld *w, uint8_t tackler, uint8_t victim, uint8_t behind);
uint8_t CX_CODE cx_keeper_of(const CxWorld *w, uint8_t t);
uint8_t CX_CODE cx_in_six_of(const CxWorld *w, uint8_t t, int16_t x, int16_t y);
uint8_t CX_CODE cx_keepers_ball(const CxWorld *w, uint8_t i);
void CX_CODE cx_keeper_guard(CxWorld *w);
void CX_CODE cx_wall_step(CxWorld *w);
void CX_CODE cx_keep_back(CxWorld *w);
void CX_CODE cx_stop_play(CxWorld *w, uint8_t phase, uint8_t team, int16_t x, int16_t y);
void CX_CODE cx_dive(CxWorld *w, uint8_t man);
void CX_CODE cx_rules_step(CxWorld *w);
void CX_CODE cx_goal_scene_step(CxWorld *w);
void CX_CODE cx_offside_mark(CxWorld *w, uint8_t from);
uint8_t CX_CODE cx_offside_check(CxWorld *w, uint8_t i);
uint8_t CX_CODE cx_nearest(const CxWorld *w, uint8_t team, int16_t x, int16_t y, uint8_t keeper_ok);
void CX_CODE cx_log(CxWorld *w, uint8_t type, uint8_t man, uint8_t other, int16_t margin);

/* cx_player.c */
void CX_CODE cx_move_to(CxMan *m, int16_t tx, int16_t ty, int16_t speed);
void CX_CODE cx_move_to8(CxMan *m, int16_t tx, int16_t ty, int16_t speed);
void CX_CODE cx_meet_pass(CxWorld *w, uint8_t i, int16_t speed);
void CX_CODE cx_keeper_human(CxWorld *w, uint8_t t, uint8_t i, uint16_t held, uint16_t pressed);
/* a keeper's ball out of his hands (cx_keeper_out) */
enum { CX_KD_SHORT, CX_KD_PUNT, CX_KD_THROW };
void CX_CODE cx_keeper_out(CxWorld *w, uint8_t from, uint8_t how, uint8_t aimed);
void CX_CODE cx_trick(CxWorld *w, uint8_t i, uint16_t held);
void CX_CODE cx_man_step(CxWorld *w, uint8_t i);
void CX_CODE cx_men_step(CxWorld *w);
void CX_CODE cx_human_step(CxWorld *w, uint8_t t, uint16_t held, uint16_t pressed);
void CX_CODE cx_choose_ctrl(CxWorld *w, uint8_t t, uint16_t pressed);
void CX_CODE cx_tackle(CxWorld *w, uint8_t i, uint8_t slide);
uint8_t CX_CODE cx_from_behind(const CxMan *a, const CxMan *b);
void CX_CODE cx_duel_scan(CxWorld *w);
/* Tired legs: what stamina takes off a man's top speed. */
static inline int16_t cx_tired(const CxMan *m) { return (int16_t)(m->sta > 128u ? 0 : (m->sta > 64u ? 2 : 4)); }
void CX_CODE cx_feint(CxWorld *w, uint8_t a, uint8_t d, uint8_t way);

/* cx_keeper.c */
void CX_CODE cx_keeper_step(CxWorld *w, uint8_t i);
uint8_t CX_CODE cx_shot_at(const CxWorld *w, uint8_t t);
void CX_CODE cx_keeper_player(CxWorld *w, uint8_t t, uint8_t i, uint16_t held, uint16_t pressed);

/* cx_ai.c */
void CX_CODE cx_team_think(CxWorld *w, uint8_t t);
void CX_CODE cx_ai_step(CxWorld *w, uint8_t i, uint8_t job);
void CX_CODE cx_spacing(CxWorld *w);
void CX_CODE cx_marks(CxWorld *w, uint8_t t);
void CX_CODE cx_ai_headers(CxWorld *w);

/* cx_nations.c */
void CX_CODE cx_field_side(CxWorld *w, uint8_t t);

#endif
