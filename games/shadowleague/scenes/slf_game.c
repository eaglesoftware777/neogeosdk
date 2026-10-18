/*
 * Shadow League Fighting: game flow — attract demo, fighter select, the
 * VS card, matches with their announcements, the arcade ladder and a
 * second player's challenge.
 */
#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include "sdk/ng_system.h"
#include "games/shadowleague/scenes/slf.h"
#include "games/shadowleague/artbox/generated/slf_assets.h"

enum { MODE_ARCADE, MODE_VERSUS, MODE_DEMO };
#define RESULT_CHALLENGER 0xFEu
#define RESULT_ABORT      0xFDu

static NGFMatch s_match;
static uint8_t s_def[2];
static uint8_t s_pick[2];
static uint8_t s_roster[2][4], s_team_mode, s_slot;
static uint8_t s_practice_pick;

static const uint8_t k_sound_map[10] = {
    0, SLF_SFX_HIT_LIGHT, SLF_SFX_HIT_HEAVY, SLF_SFX_BLOCK, SLF_SFX_SPECIAL,
    SLF_SFX_SUPER, SLF_SFX_WHOOSH, SLF_SFX_THROW, SLF_SFX_KO, SLF_SFX_LAND
};

uint8_t NEOGEO_USER slf_start_pending(void)
{
    if (NEO_REGISTER8(SLF_START_FLAG)) return 1u;
    if (ng_sys_is_mvs()) return (uint8_t)(read_p1credit() > 0);
    if (NEO_REGISTER8(BIOS_STATCHANGE) & 1u) {
        NEO_REGISTER8(SLF_START_FLAG) = 1u;
        NEO_REGISTER8(BIOS_USER_MODE) = 2u;
        return 1u;
    }
    return 0u;
}

static uint8_t NEOGEO_USER cpu_level(uint8_t step)
{
    (void)step;
    return ng_sys_is_mvs() ? slf_difficulty_from_dip(ng_dip_option(0)) : slf_home_difficulty;
}

static uint8_t NEOGEO_USER rounds_to_win(void)
{
    uint8_t r = ng_sys_is_mvs() ? (uint8_t)(ng_dip_option(1) + 1u) : slf_home_rounds;
    return r ? (r > 3u ? 3u : r) : 2u;
}

static void NEOGEO_USER wait_frames(uint16_t n)
{
    while (n--) slf_frame();
}

/* ------------------------------------------------------------ select */
static uint8_t NEOGEO_USER anim_sprite(uint8_t def, uint8_t anim, uint16_t tick)
{
    const NGFFighterDef *d = slf_fighter_defs[def];
    const NGFAnim *a = &d->anims[anim];
    uint16_t total = 0, t;
    uint8_t i;
    for (i = 0u; i < a->count; i++) {
        uint8_t time = d->frames[a->first + i].time;
        total = (uint16_t)(total + (time == 255u ? 30u : time));
    }
    t = (uint16_t)(tick % (total ? total : 1u));
    for (i = 0u; i < a->count; i++) {
        uint8_t time = d->frames[a->first + i].time;
        uint16_t len = time == 255u ? 30u : time;
        if (t < len) break;
        t = (uint16_t)(t - len);
    }
    if (i >= a->count) i = (uint8_t)(a->count - 1u);
    return d->frames[a->first + i].sprite;
}

static void NEOGEO_USER choose_format(void)
{
    uint16_t timer = 10u * 60u;
    slf_scene_begin(); slf_story_show(SLF_CARD_MODE);
    slf_puts_tall_center(6u, "CHOOSE YOUR BATTLE", SLF_TALL_GOLD);
    slf_puts_center(19u, "LEFT / RIGHT - CHANGE", SLF_PAL_TEXT);
    slf_puts_center(21u, "A OR C - CONFIRM", SLF_PAL_TEXT);
    for (;;) {
        slf_frame();
        if (slf_pressed(0u) & (NGF_RAW_LEFT | NGF_RAW_RIGHT | NGF_BTN_B)) {
            s_team_mode ^= 1u; slf_sfx(SLF_SFX_MENU);
        }
        slf_puts_tall_center(11u, s_team_mode ? "< 3 VS 3 + STRIKER >" : "<      1 VS 1      >", SLF_TALL_SILVER);
        slf_puts_center(15u, s_team_mode ? "ONE STRIKER FOR EACH TEAM" : "     SINGLE FIGHTER     ", SLF_PAL_GOLD);
        if ((slf_pressed(0u) & (NGF_BTN_A | NGF_BTN_C)) || slf_start_pressed(0u) || !--timer) break;
    }
}

static uint8_t NEOGEO_USER picked_before(uint8_t side, uint8_t def)
{
    uint8_t i;
    if (!s_team_mode) return 0;
    for (i = 0u; i < s_slot; i++) if (s_roster[side][i] == def) return 1u;
    return 0;
}

/* A random playable fighter (painted set in) other than `avoid`. */
static uint8_t NEOGEO_USER ready_fighter(uint8_t avoid)
{
    uint8_t def = (uint8_t)(slf_rand() % SLF_FIGHTER_COUNT), i;
    for (i = 0u; i < SLF_FIGHTER_COUNT; i++) {
        if (slf_fighter_ready[def] && def != avoid) return def;
        def = (uint8_t)((def + 1u) % SLF_FIGHTER_COUNT);
    }
    return avoid;
}

/* The next playable, not yet picked fighter from `def` in a direction
 * (0 left, 1 right, 2 up, 3 down; 0xFF: the nearest from `def`). Locked
 * cells are passed over, and rows with nothing playable are left. */
static uint8_t NEOGEO_USER wall_move(uint8_t side, uint8_t def, uint8_t dir)
{
    uint8_t tries, k, along = (uint8_t)(dir == 0u ? 0u : 1u), across = (uint8_t)(dir == 2u ? 2u : 3u);
    if (dir <= 1u) {
        for (tries = 0u; tries < 8u; tries++) {
            def = slf_wall_step(def, dir);
            if (slf_fighter_ready[def] && !picked_before(side, def)) return def;
        }
    } else if (dir != 0xFFu) {
        def = slf_wall_step(def, dir);
    }
    for (tries = 0u; tries < SLF_WALL_ROW_COUNT; tries++) {
        for (k = 0u; k < 8u; k++) {
            if (slf_fighter_ready[def] && !picked_before(side, def)) return def;
            def = slf_wall_step(def, along);
        }
        def = slf_wall_step(def, across);
    }
    return def;
}

/* Up to n characters of text (the two players' lines share the bottom rows). */
static const char *NEOGEO_USER clip(char *buf, const char *text, uint8_t n)
{
    uint8_t i;
    for (i = 0u; i < n && text[i]; i++) buf[i] = text[i];
    buf[i] = '\0';
    return buf;
}

static void NEOGEO_USER select_info(uint8_t side, uint8_t def)
{
    char buf[20];
    if (side == 0u) {
        slf_puts(1u, 24u, clip(buf, slf_fighter_origin[def], 13u), SLF_PAL_DIM);
        slf_puts(1u, 25u, clip(buf, slf_fighter_style[def], 18u), SLF_PAL_TEXT);
        slf_puts_tall(1u, 26u, slf_fighter_defs[def]->name, SLF_TALL_SILVER);
    } else {
        slf_puts_right(38u, 24u, clip(buf, slf_fighter_origin[def], 13u), SLF_PAL_DIM);
        slf_puts_right(38u, 25u, clip(buf, slf_fighter_style[def], 18u), SLF_PAL_TEXT);
        slf_puts_tall_right(38u, 26u, slf_fighter_defs[def]->name, SLF_TALL_SILVER);
    }
}

/*
 * The whole roster at once on the octagonal wall; each player's cursor
 * moves cell to cell, their face fills the top corner and their fighter
 * stands below it.
 */
static void NEOGEO_USER select_screen(uint8_t humans)
{
    static const int16_t k_x[2] = {30, 290};
    uint8_t cursor[2], last_cursor[2], done[2], p, i;
    uint16_t tick = 0, timer = 25u * 60u, last_secs = 0xFFFFu;
    slf_scene_begin();
    if (!s_slot) slf_music(SLF_TRACK_SELECT);
    cursor[0] = wall_move(0u, s_slot, 0xFFu);
    cursor[1] = wall_move(1u, (uint8_t)((cursor[0] + 1u) % SLF_FIGHTER_COUNT), 0xFFu);
    s_pick[0] = cursor[0]; s_pick[1] = cursor[1];
    last_cursor[0] = last_cursor[1] = 0xFFu;
    done[0] = (uint8_t)!(humans & 1u);
    done[1] = (uint8_t)!(humans & 2u);
    slf_fighter_palettes(cursor[0], 0u, cursor[1], (uint8_t)(cursor[0] == cursor[1]));
    slf_story_show(SLF_CARD_SELECT);
    slf_wall_show();
    slf_fighter_bank(0u, 0u);
    slf_fighter_bank(1u, 1u);
    {
        static const char *const headings[4] = {"SELECT YOUR CAPTAIN", "SELECT SECOND FIGHTER", "SELECT THIRD FIGHTER", "SELECT YOUR ASSIST"};
        slf_puts_tall_center(0u, s_practice_pick == 1u ? "TRAINING FIGHTER" : s_practice_pick == 2u ?
                            "TRAINING PARTNER" : s_team_mode ? headings[s_slot] : "SELECT YOUR FIGHTER", SLF_TALL_GOLD);
    }
    for (;;) {
        slf_frame();
        tick++;
        if (timer) timer--;
        for (p = 0u; p < 2u; p++) {
            uint8_t press = slf_pressed(p), dir = 0xFFu;
            if (done[p]) continue;
            if (press & NGF_RAW_LEFT) dir = 0u;
            else if (press & NGF_RAW_RIGHT) dir = 1u;
            else if (press & NGF_RAW_UP) dir = 2u;
            else if (press & NGF_RAW_DOWN) dir = 3u;
            if (dir != 0xFFu) {
                cursor[p] = wall_move(p, cursor[p], dir);
                slf_sfx(SLF_SFX_MENU);
            }
            if ((press & (NGF_BTN_A | NGF_BTN_C)) || slf_start_pressed(p) || !timer) {
                done[p] = 1u;
                s_pick[p] = cursor[p];
                slf_sfx(SLF_SFX_SELECT);
            }
        }
        for (i = 0u; i < 2u; i++) {
            uint8_t f_id = done[i] ? s_pick[i] : cursor[i];
            uint8_t chosen = (uint8_t)(done[i] && (humans & (1u << i)));
            slf_wall_face(i, f_id);
            slf_fighter_pose(i, f_id, anim_sprite(f_id, chosen ? NGF_A_WIN : NGF_A_IDLE, tick), k_x[i],
                             SLF_FLOOR_Y - 125, i);
            if (humans & (1u << i)) slf_wall_cursor(i, f_id, chosen, tick);
        }
        if (cursor[0] != last_cursor[0] || cursor[1] != last_cursor[1]) {
            slf_clear_rows(24u, 27u);
            last_secs = 0xFFFFu;
            select_info(0u, cursor[0]);
            select_info(1u, cursor[1]);
            last_cursor[0] = cursor[0];
            last_cursor[1] = cursor[1];
            slf_fighter_palettes(cursor[0], 0u, cursor[1], (uint8_t)(cursor[0] == cursor[1]));
        }
        if (timer / 60u != last_secs) {
            char t[3];
            last_secs = timer / 60u;
            t[0] = (char)('0' + last_secs / 10u);
            t[1] = (char)('0' + last_secs % 10u);
            t[2] = '\0';
            slf_puts_tall(19u, 26u, t, SLF_TALL_GOLD);
        }
        if (done[0] && done[1]) break;
    }
    wait_frames(50u);
}

static void NEOGEO_USER select_roster(uint8_t humans)
{
    uint8_t p, slot, count = s_team_mode ? 4u : 1u;
    for (slot = 0u; slot < count; slot++) {
        s_slot = slot; select_screen(humans);
        for (p = 0u; p < 2u; p++) s_roster[p][slot] = s_pick[p];
    }
    s_slot = 0u;
    for (p = 0u; p < 2u; p++) s_pick[p] = s_roster[p][0];
}

static void NEOGEO_USER cpu_roster(uint8_t side, uint8_t captain)
{
    uint8_t slot;
    s_roster[side][0] = captain;
    for (slot = 1u; slot < 4u; slot++) {
        uint8_t def = (uint8_t)((captain + slot * 7u) % SLF_FIGHTER_COUNT), i, k, taken = 1u;
        for (i = 0u; i < SLF_FIGHTER_COUNT && taken; i++) {
            taken = (uint8_t)!slf_fighter_ready[def];
            for (k = 0u; k < slot; k++) if (s_roster[side][k] == def) taken = 1u;
            if (taken) def = (uint8_t)((def + 1u) % SLF_FIGHTER_COUNT);
        }
        s_roster[side][slot] = def;
    }
}

static void NEOGEO_USER vs_screen(void)
{
    uint16_t tick;
    slf_scene_begin();
    slf_fighter_palettes(s_def[0], 0u, s_def[1], (uint8_t)(s_def[0] == s_def[1]));
    slf_story_show(SLF_CARD_VS);
    slf_fighter_bank(0u, 0u);
    slf_fighter_bank(1u, 1u);
    for (tick = 0u; tick < 150u; tick++) {
        slf_frame();
        slf_fighter_pose(0u, s_def[0], anim_sprite(s_def[0], NGF_A_INTRO, tick), 84, SLF_FLOOR_Y - 125, 0u);
        slf_fighter_pose(1u, s_def[1], anim_sprite(s_def[1], NGF_A_INTRO, tick), 236, SLF_FLOOR_Y - 125, 1u);
        if (tick == 1u) {
            slf_big_center(9u, SLF_BIG_VS, SLF_BIG_VS_W, SLF_BIG_VS_H, SLF_PAL_RED);
            slf_puts_tall(2u, 23u, slf_fighter_defs[s_def[0]]->name, SLF_TALL_RED);
            slf_puts_tall_right(37u, 23u, slf_fighter_defs[s_def[1]]->name, SLF_TALL_BLUE);
            slf_puts_center(26u, slf_stage_names[slf_fighter_stages[s_def[1]]], SLF_PAL_TEXT);
        }
    }
}

/* ------------------------------------------------------------ match */
static uint8_t s_feedback_timer;

static void NEOGEO_USER announce_clear(void)
{
    slf_clear_rows(7u, 17u);
    s_feedback_timer = 0u;
}

static void NEOGEO_USER announcer(uint8_t line)
{
    slf_sfx((uint8_t)(SLF_SND_ANNOUNCER + line));
}

/* A fighter's grunt (SLF_GRUNT_*), in the voice type cast for them. */
static void NEOGEO_USER grunt(uint8_t def, uint8_t kind)
{
    slf_sfx((uint8_t)(SLF_SND_TYPES + slf_voice_type[def] * 4u + kind));
}

static void NEOGEO_USER announce(NGFMatch *m, uint8_t state)
{
    announce_clear();
    switch (state) {
    case NGF_M_INTRO:
        if (m->rounds_to_win > 1u && m->f[0].wins == m->rounds_to_win - 1u && m->f[1].wins == m->rounds_to_win - 1u)
            announcer(SLF_ANN_FINAL);
        else
            announcer((uint8_t)(m->round <= 1u ? SLF_ANN_ROUND1 : m->round == 2u ? SLF_ANN_ROUND2 : SLF_ANN_ROUND3));
        if (m->match_mode != NGF_MODE_1V1) {
            char bout[7] = {'B', 'O', 'U', 'T', ' ', '1', '\0'};
            bout[5] = (char)('0' + m->round);
            slf_puts_center(10u, "TEAM BATTLE", SLF_PAL_GOLD);
            slf_puts_center(12u, bout, SLF_PAL_BIG);
        } else if (m->rounds_to_win > 1u && m->f[0].wins == m->rounds_to_win - 1u && m->f[1].wins == m->rounds_to_win - 1u)
            slf_big_center(10u, SLF_BIG_FINAL, SLF_BIG_FINAL_W, SLF_BIG_FINAL_H, SLF_PAL_BIG);
        else if (m->round == 1u) slf_big_center(10u, SLF_BIG_ROUND1, SLF_BIG_ROUND1_W, SLF_BIG_ROUND1_H, SLF_PAL_BIG);
        else if (m->round == 2u) slf_big_center(10u, SLF_BIG_ROUND2, SLF_BIG_ROUND2_W, SLF_BIG_ROUND2_H, SLF_PAL_BIG);
        else slf_big_center(10u, SLF_BIG_ROUND3, SLF_BIG_ROUND3_W, SLF_BIG_ROUND3_H, SLF_PAL_BIG);
        break;
    case NGF_M_FIGHT:
        announcer(SLF_ANN_FIGHT);
        slf_big_center(10u, SLF_BIG_FIGHT, SLF_BIG_FIGHT_W, SLF_BIG_FIGHT_H, SLF_PAL_RED);
        break;
    case NGF_M_KO:
        announcer(SLF_ANN_KO);
        slf_big_center(10u, SLF_BIG_KO, SLF_BIG_KO_W, SLF_BIG_KO_H, SLF_PAL_RED);
        break;
    case NGF_M_TIME_UP:
        announcer(SLF_ANN_TIMEUP);
        slf_big_center(10u, SLF_BIG_TIMEUP, SLF_BIG_TIMEUP_W, SLF_BIG_TIMEUP_H, SLF_PAL_BIG);
        break;
    case NGF_M_ROUND_END:
        if (m->round_winner < 2u) {
            const NGFFighter *w = &m->f[m->round_winner];
            slf_puts_center(9u, w->def->name, SLF_PAL_GOLD);
            slf_big_center(11u, SLF_BIG_WIN, SLF_BIG_WIN_W, SLF_BIG_WIN_H, SLF_PAL_BIG);
            if (w->life == w->def->life) {
                announcer(SLF_ANN_PERFECT);
                slf_big_center(14u, SLF_BIG_PERFECT, SLF_BIG_PERFECT_W, SLF_BIG_PERFECT_H, SLF_PAL_BLUE);
            }
        } else {
            announcer(SLF_ANN_DRAW);
            slf_big_center(10u, SLF_BIG_DRAW, SLF_BIG_DRAW_W, SLF_BIG_DRAW_H, SLF_PAL_BIG);
        }
        break;
    default:
        break;
    }
}

/* Frames before a fighter may grunt again from being hit (a combo is not a choir). */
static uint8_t s_hurt_quiet[2];

static void NEOGEO_USER move_voice(const NGFMatch *m, uint8_t who, uint8_t def, uint8_t kind)
{
    const NGFFighter *f = &m->f[who];
    uint8_t line;
    if (who > 1u || !f->move) return;
    switch (kind) {
    case NGF_KIND_SPECIAL:
    case NGF_KIND_SUPER:
        line = slf_move_voice[slf_move_voice_ofs[def] + f->move_index];
        if (line) slf_sfx(line);
        break;
    case NGF_KIND_THROW:
    case NGF_KIND_COMMAND:
    case NGF_KIND_GUARD_ATTACK:
        grunt(def, SLF_GRUNT_ATTACK);
        break;
    case NGF_KIND_NORMAL:
        /* heavy normals, every other one */
        if ((f->move->buttons & (NGF_BTN_C | NGF_BTN_D)) && (m->frame & 1u)) grunt(def, SLF_GRUNT_ATTACK);
        break;
    default:
        break;
    }
}

void NEOGEO_USER slf_combat_events(NGFMatch *m, const uint8_t *defs)
{
    NGFEvent e;
    uint8_t i;
    for (i = 0u; i < 2u; i++) if (s_hurt_quiet[i]) s_hurt_quiet[i]--;
    while (ngf_pop_event(m, &e)) {
        int16_t sx = (int16_t)(e.x - m->camera_x), sy = (int16_t)(SLF_FLOOR_Y - e.y);
        switch (e.type) {
        case NGF_EV_HIT:
            if (e.data == 0xFEu) { slf_sfx(SLF_SFX_COUNTER); break; }
            slf_sfx(e.data < 10u ? k_sound_map[e.data] : SLF_SFX_HIT_LIGHT);
            slf_fx_spawn(0u, sx, sy);
            if (e.who < 2u) {
                uint8_t hurt = (uint8_t)(e.who ^ 1u);
                const NGFFighter *h = &m->f[hurt];
                if (h->life && !s_hurt_quiet[hurt] && (e.data == 2u || h->state == NGF_S_FALL)) {
                    grunt(defs[hurt], SLF_GRUNT_HURT);
                    s_hurt_quiet[hurt] = 40u;
                }
            }
            break;
        case NGF_EV_BLOCK:
            slf_sfx(SLF_SFX_BLOCK);
            slf_fx_spawn(1u, sx, sy);
            break;
        case NGF_EV_THROW: slf_sfx(SLF_SFX_THROW); break;
        case NGF_EV_TECH: slf_sfx(SLF_SFX_BLOCK); break;
        case NGF_EV_GUARD_BREAK:
            slf_sfx(SLF_SFX_HIT_HEAVY);
            slf_fx_spawn(1u, sx, sy);
            slf_clear_rows(7u, 9u);
            slf_big_center(7u, SLF_BIG_GUARDBREAK, SLF_BIG_GUARDBREAK_W, SLF_BIG_GUARDBREAK_H, SLF_PAL_RED);
            s_feedback_timer = 45u;
            break;
        case NGF_EV_COUNTER:
            slf_sfx(SLF_SFX_COUNTER);
            slf_clear_rows(7u, 9u);
            slf_big_center(7u, SLF_BIG_COUNTER, SLF_BIG_COUNTER_W, SLF_BIG_COUNTER_H, SLF_PAL_BLUE);
            s_feedback_timer = 30u;
            break;
        case NGF_EV_KO:
            slf_sfx(SLF_SFX_KO);
            if (e.who < 2u) grunt(defs[e.who], SLF_GRUNT_KO);
            break;
        case NGF_EV_MOVE:
            if (e.data == NGF_KIND_SUPER) { slf_sfx(SLF_SFX_SUPER); slf_fx_spawn(4u, sx, (int16_t)(SLF_FLOOR_Y - 66)); }
            else if (e.data == NGF_KIND_SPECIAL) {
                slf_sfx(SLF_SFX_SPECIAL);
                if (e.who < 2u && slf_fighter_fx[defs[e.who]]) slf_fx_spawn(3u, (int16_t)(sx + (m->f[e.who].facing_left ? -28 : 28)), (int16_t)(SLF_FLOOR_Y - 64));
            }
            else if (e.data != NGF_KIND_THROW) slf_sfx(SLF_SFX_WHOOSH);
            if (e.who < 2u) move_voice(m, e.who, defs[e.who], e.data);
            break;
        case NGF_EV_LAND:
            if (e.data == NGF_S_FALL) { slf_sfx(SLF_SFX_LAND); slf_fx_spawn(2u, sx, SLF_FLOOR_Y - 6); }
            break;
        case NGF_EV_FIGHT: slf_sfx(SLF_SFX_BELL); break;
        case NGF_EV_MAX:
            slf_sfx(SLF_SFX_POWER);
            if (e.who < 2u) grunt(defs[e.who], SLF_GRUNT_POWER);
            slf_clear_rows(7u, 9u);
            slf_puts_tall_center(7u, "MAX POWER", SLF_TALL_RED);
            s_feedback_timer = 45u;
            break;
        default: break;
        }
    }
}

static uint8_t NEOGEO_USER run_match(uint8_t lv0, uint8_t lv1, uint8_t mode)
{
    NGFMatch *m = &s_match;
    uint8_t last_state = 0xFFu, last_round = 0u;
    uint8_t active_def[2], team = (uint8_t)(s_team_mode && mode != MODE_DEMO);
    uint16_t over = 0, counter_clear = 0;
    slf_scene_begin();
    slf_fx_reset();
    slf_stage_set(slf_fighter_stages[s_def[1]]);
    slf_stage_show(96);
    slf_fighter_palettes(s_def[0], 0u, s_def[1], (uint8_t)(s_def[0] == s_def[1]));
    if (mode != MODE_DEMO) slf_music((uint8_t)(SLF_TRACK_STAGE + slf_fighter_stages[s_def[1]]));
    ngf_match_init(m, slf_fighter_defs[s_def[0]], slf_fighter_defs[s_def[1]], SLF_STAGE_W, slf_rand());
    m->rounds_to_win = mode == MODE_DEMO ? 1u : rounds_to_win();
    active_def[0] = s_def[0]; active_def[1] = s_def[1];
    if (team) {
        uint8_t side;
        if (lv0) cpu_roster(0u, s_def[0]);
        if (lv1) cpu_roster(1u, s_def[1]);
        ngf_match_set_mode(m, NGF_MODE_3V3);
        for (side = 0u; side < 2u; side++) {
            ngf_team_set(m, side, slf_fighter_defs[s_roster[side][0]],
                slf_fighter_defs[s_roster[side][1]], slf_fighter_defs[s_roster[side][2]]);
            ngf_striker_set(m, side, slf_fighter_defs[s_roster[side][3]]);
        }
    }
    if (lv0) ngf_set_cpu_difficulty(m, 0u, lv0); else ngf_set_cpu(m, 0u, 0u);
    if (lv1) ngf_set_cpu_difficulty(m, 1u, lv1); else ngf_set_cpu(m, 1u, 0u);
    ngf_set_round_seconds(m, ng_sys_is_mvs() ? slf_seconds_from_dip(ng_dip_option(2)) : slf_home_seconds);
    if (mode == MODE_DEMO) slf_puts_center(22u, "DEMO PLAY", SLF_PAL_GOLD);
    for (;;) {
        slf_frame();
        if (mode == MODE_DEMO && (slf_start_pending() || slf_start_pressed(0u))) return RESULT_ABORT;
        if (mode == MODE_ARCADE && NEO_REGISTER8(SLF_P2_FLAG)) return RESULT_CHALLENGER;
        ngf_match_step(m, slf_joy(0u), slf_joy(1u));
        if (s_feedback_timer && !--s_feedback_timer) slf_clear_rows(7u, 9u);
        slf_combat_events(m, active_def);
        if (m->state != last_state || m->round != last_round) {
            if (m->state == NGF_M_FIGHT && last_state == NGF_M_INTRO) counter_clear = 30u;
            if (m->state != NGF_M_OVER) announce(m, m->state == NGF_M_FIGHT ? NGF_M_FIGHT : m->state);
            last_state = m->state;
            last_round = m->round;
        }
        if (m->state == NGF_M_INTRO && m->state_timer == 60u) announce(m, NGF_M_FIGHT);
        if (counter_clear && !--counter_clear) announce_clear();
        if (team) {
            uint8_t side, changed = 0u;
            for (side = 0u; side < 2u; side++) {
                uint8_t def = s_roster[side][m->team_active[side]];
                if (def != active_def[side]) { active_def[side] = def; changed = 1u; }
            }
            if (changed) slf_fighter_palettes(active_def[0], 0u, active_def[1], (uint8_t)(active_def[0] == active_def[1]));
        }
        slf_match_draw(m, active_def);
        slf_hud_draw(m);
        if (mode == MODE_DEMO && (m->frame & 31u) == 0u)
            slf_puts_center(24u, (m->frame & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PUSH START") : "           ",
                            SLF_PAL_TEXT);
        if (m->state == NGF_M_OVER) {
            if (++over == 1u && m->winner < 2u && !m->f[m->winner].cpu) announcer(SLF_ANN_YOUWIN);
            if (over > 60u) return m->winner;
        }
    }
}

/* ------------------------------------------------------------ flows */
static void NEOGEO_USER big_card(uint16_t tile, uint8_t w, uint8_t h, uint8_t pal, uint8_t winner_def, uint16_t frames)
{
    uint16_t tick;
    slf_scene_begin();
    slf_fighter_palettes(winner_def < SLF_FIGHTER_COUNT ? winner_def : 0u, 0u,
                         winner_def < SLF_FIGHTER_COUNT ? winner_def : 0u, 0u);
    slf_stage_set(winner_def < SLF_FIGHTER_COUNT ? slf_fighter_stages[winner_def] : 0u);
    slf_stage_show(96);
    slf_fighter_bank(0u, 0u);
    slf_big_center(6u, tile, w, h, pal);
    for (tick = 0; tick < frames; tick++) {
        slf_frame();
        if (winner_def != 0xFFu && winner_def < SLF_FIGHTER_COUNT)
            slf_fighter_pose(0u, winner_def, anim_sprite(winner_def, NGF_A_WIN, tick), 160, SLF_FLOOR_Y - 125, 0u);
    }
}

static void NEOGEO_USER versus(void)
{
    uint8_t result;
    NEO_REGISTER8(SLF_P2_FLAG) = 0u;
    slf_scene_begin();
    slf_big_center(10u, SLF_BIG_CHALLENGER, SLF_BIG_CHALLENGER_W, SLF_BIG_CHALLENGER_H, SLF_PAL_RED);
    slf_sfx(SLF_SFX_BELL);
    wait_frames(120u);
    choose_format();
    select_roster(3u);
    s_def[0] = s_pick[0];
    s_def[1] = s_pick[1];
    vs_screen();
    result = run_match(0u, 0u, MODE_VERSUS);
    slf_music(SLF_TRACK_VICTORY);
    if (result < 2u) big_card(SLF_BIG_WIN, SLF_BIG_WIN_W, SLF_BIG_WIN_H, SLF_PAL_BIG, s_def[result], 240u);
    else big_card(SLF_BIG_DRAW, SLF_BIG_DRAW_W, SLF_BIG_DRAW_H, SLF_PAL_BIG, 0xFFu, 180u);
    NEO_REGISTER8(BIOS_PLAYER2_MODE) = 3u;
}

static uint8_t NEOGEO_USER continue_screen(void)
{
    uint16_t timer = 10u * 60u;
    uint8_t last_sec = 0xFFu;
    slf_scene_begin();
    slf_music(SLF_TRACK_CONTINUE);
    slf_stage_set(0u);
    slf_stage_show(96);
    slf_puts_tall_center(8u, "CONTINUE ?", SLF_TALL_GOLD);
    while (timer--) {
        uint8_t sec = (uint8_t)(timer / 60u);
        slf_frame();
        if (sec != last_sec) {
            slf_big_center(12u, (uint16_t)(SLF_BIG_DIGITS + sec * 2u), 2u, 2u, sec < 4u ? SLF_PAL_RED : SLF_PAL_BIG);
            last_sec = sec;
            if (sec < 9u) slf_sfx(SLF_SFX_MENU);
        }
        if (!ng_sys_is_mvs()) {
            slf_puts_center(24u, "PRESS START TO CONTINUE", SLF_PAL_TEXT);
            if (slf_start_pressed(0u) || (slf_pressed(0u) & (NGF_BTN_A | NGF_BTN_C))) {
                slf_sfx(SLF_SFX_SELECT);
                return 1u;
            }
        } else {
            if (read_p1credit() > 0) {
                slf_puts_center(24u, "PRESS 1P START TO CONTINUE", SLF_PAL_GOLD);
                if (slf_start_pressed(0u) || (slf_pressed(0u) & (NGF_BTN_A | NGF_BTN_C))) {
                    uint16_t saved_sr;
                    __asm__ volatile ("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d" (saved_sr) : : "memory");
                    NEO_REGISTER8(0x10FDB0) = 1;
                    NEO_REGISTER8(0x10FDB1) = 0;
                    CALLNEOGEOF(SYS_CREDIT_CHECK);
                    if (NEO_REGISTER8(0x10FDB0)) {
                        CALLNEOGEOF(SYS_CREDIT_DOWN);
                    }
                    __asm__ volatile ("move.w %0,%%sr" : : "d" (saved_sr) : "memory");
                    slf_sfx(SLF_SFX_SELECT);
                    return 1u;
                }
            } else {
                slf_puts_center(24u, "   INSERT COIN TO CONTINUE   ", SLF_PAL_TEXT);
            }
        }
    }
    return 0u;
}

void NEOGEO_USER slf_run(void)
{
    uint8_t ladder[8];
    uint8_t stage;
    slf_audio_demo(0u);
    if (!ng_sys_is_mvs()) {
        uint8_t choice = slf_home_menu();
        if (choice == 1u) { versus(); return; }
        if (choice == 2u) {
            uint8_t first;
            s_team_mode = 0u; s_practice_pick = 1u;
            select_roster(1u); first = s_pick[0];
            s_practice_pick = 2u;
            select_roster(1u);
            s_practice_pick = 0u;
            slf_training_run(first, s_pick[0]);
            return;
        }
    }
    NEO_REGISTER8(SLF_P2_FLAG) = 0u;
    choose_format();
    select_roster(1u);
    /* 8-stage tournament: 5 random contenders, 1 rival, 1 sub-boss, 1 final
     * boss, all playable fighters (the bosses are stood in for until theirs are). */
    ladder[5] = (s_pick[0] == 1u) ? 0u : 1u;
    ladder[6] = (uint8_t)(41u + (slf_rand() & 1u));
    ladder[7] = (s_pick[0] == 43u) ? 42u : 43u;
    for (stage = 0u; stage < 8u; stage++) {
        uint8_t opp = ladder[stage], tries, k, seen = 1u;
        if (stage >= 5u && slf_fighter_ready[opp] && opp != s_pick[0]) continue;
        for (tries = 0u; tries < 12u && seen; tries++) {
            opp = ready_fighter(s_pick[0]);
            seen = 0u;
            for (k = 0u; k < stage; k++) if (ladder[k] == opp) seen = 1u;
        }
        ladder[stage] = opp;
    }

    for (stage = 0u; stage < 8u; stage++) {
        uint8_t result;
        s_def[0] = s_pick[0];
        s_def[1] = ladder[stage];
        vs_screen();
        result = run_match(0u, cpu_level(stage), MODE_ARCADE);
        if (result == RESULT_CHALLENGER) {
            versus();
            return;
        }
        if (result != 0u) {
            if (!continue_screen()) {
                slf_music(SLF_TRACK_VICTORY);
                big_card(SLF_BIG_GAMEOVER, SLF_BIG_GAMEOVER_W, SLF_BIG_GAMEOVER_H, SLF_PAL_RED, s_def[1], 240u);
                return;
            }
            stage--;
            continue;
        }
    }
    slf_music(SLF_TRACK_VICTORY);
    big_card(SLF_BIG_CONGRATS, SLF_BIG_CONGRATS_W, SLF_BIG_CONGRATS_H, SLF_PAL_BIG, s_pick[0], 240u);
    {
        uint16_t end_t;
        slf_scene_begin();
        slf_story_show(SLF_CARD_ENDING);
        slf_fighter_palettes(s_pick[0], 0u, s_pick[0], 0u);
        slf_fighter_bank(0u, 0u);
        slf_puts_center(4u, "PROJECT CHIMERA TERMINATED", SLF_PAL_GOLD);
        slf_puts_center(7u, "THE VEIL CORE HAS CRUMBLED.", SLF_PAL_WHITE);
        slf_puts_center(10u, "NEW SHADOW LEAGUE CHAMPION:", SLF_PAL_BLUE);
        slf_puts_center(13u, slf_fighter_defs[s_pick[0]]->name, SLF_PAL_BIG);
        slf_puts_center(16u, slf_fighter_origin[s_pick[0]], SLF_PAL_GOLD);
        slf_puts_center(19u, "THANK YOU FOR PLAYING!", SLF_PAL_WHITE);
        for (end_t = 0u; end_t < 360u; end_t++) {
            slf_frame();
            slf_fighter_pose(0u, s_pick[0], anim_sprite(s_pick[0], NGF_A_WIN, end_t), 160, SLF_FLOOR_Y - 125, 0u);
            if (slf_start_pressed(0u) || slf_start_pressed(1u)) break;
        }
    }
}

static const uint8_t featured_cast[4] = {0u, 12u, 3u, 4u};
static uint8_t NEOGEO_USER featured_fighter(uint8_t avoid)
{
    uint8_t k = (uint8_t)(slf_rand() & 3u);
    if (featured_cast[k] == avoid) k = (uint8_t)((k + 1u) & 3u);
    return featured_cast[k];
}

static uint8_t NEOGEO_USER attract_story(void)
{
    uint8_t card;
    slf_music(SLF_TRACK_TITLE);
    for (card = 1u; card < SLF_STORY_CARDS; card++) {
        uint16_t t, duration;
        slf_scene_begin();
        duration = slf_story_show(card);
        for (t = 0u; t < duration; t++) {
            slf_frame();
            slf_story_pan(t, (uint8_t)(card & 1u));
            if (slf_start_pressed(1u)) { NEO_REGISTER8(SLF_P2_FLAG) = 1u; return 1u; }
            if (slf_start_pending() || slf_start_pressed(0u)) return 1u;
            if ((t & 31u) == 0u)
                slf_puts(1u, 0u, (t & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PUSH START ") : "           ", SLF_PAL_TEXT);
        }
    }
    return 0u;
}

static uint8_t NEOGEO_USER attract_tutorial(void)
{
    uint16_t t;
    uint8_t a = featured_fighter(0xFFu), b = featured_fighter(a);
    slf_scene_begin();
    slf_music(SLF_TRACK_TITLE);
    slf_story_show(SLF_CARD_MODE);
    slf_fighter_palettes(a, 0u, b, 0u);
    slf_fighter_bank(0u, 0u);
    slf_fighter_bank(1u, 1u);
    slf_puts_tall_center(1u, "HOW TO PLAY", SLF_TALL_GOLD);
    slf_puts(4u, 5u, "(A) LIGHT PUNCH    (B) LIGHT KICK", SLF_PAL_WHITE);
    slf_puts(4u, 7u, "(C) HEAVY PUNCH    (D) HEAVY KICK", SLF_PAL_WHITE);
    slf_puts(4u, 10u, "EVADE ROLL       :  (A) + (B)", SLF_PAL_BLUE);
    slf_puts(4u, 12u, "BLOWBACK         :  (C) + (D)", SLF_PAL_RED);
    slf_puts(4u, 14u, "MAX POWER MODE   :  (B) + (C)", SLF_PAL_RED);
    slf_puts(4u, 16u, "SPECIAL MOVES    :  D,DF,F + BTN", SLF_PAL_GOLD);
    slf_puts(4u, 18u, "SUPER MOVES      :  D,DF,F X2 + BTN", SLF_PAL_GOLD);
    for (t = 0u; t < 360u; t++) {
        slf_frame();
        slf_fighter_pose(0u, a, anim_sprite(a, NGF_A_IDLE, t), 72, SLF_FLOOR_Y - 125, 0u);
        slf_fighter_pose(1u, b, anim_sprite(b, NGF_A_IDLE, t), 248, SLF_FLOOR_Y - 125, 1u);
        if (slf_start_pressed(1u)) { NEO_REGISTER8(SLF_P2_FLAG) = 1u; return 1u; }
        if (slf_start_pending() || slf_start_pressed(0u)) return 1u;
        if ((t & 31u) == 0u)
            slf_puts_center(24u, (t & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PUSH START") : "           ", SLF_PAL_TEXT);
    }
    return 0u;
}

static uint8_t NEOGEO_USER attract_showcase(void)
{
    uint8_t k, f = 0xFFu;
    for (k = 0u; k < 4u; k++) {
        f = featured_cast[k];
        uint16_t t;
        slf_scene_begin();
        slf_fighter_palettes(f, 0u, f, 0u);
        slf_fighter_bank(0u, 0u);
        slf_stage_set(slf_fighter_stages[f]);
        slf_stage_show(96);
        slf_puts_center(2u, "ROSTER SHOWCASE", SLF_PAL_GOLD);
        slf_puts_center(5u, slf_fighter_defs[f]->name, SLF_PAL_BIG);
        slf_puts_center(7u, slf_fighter_origin[f], SLF_PAL_WHITE);
        slf_puts_center(9u, slf_fighter_style[f], SLF_PAL_BLUE);
        for (t = 0u; t < 150u; t++) {
            slf_frame();
            slf_fighter_pose(0u, f, anim_sprite(f, NGF_A_WIN, t), 160, SLF_FLOOR_Y - 125, 0u);
            if (slf_start_pressed(1u)) { NEO_REGISTER8(SLF_P2_FLAG) = 1u; return 1u; }
            if (slf_start_pending() || slf_start_pressed(0u)) return 1u;
            if ((t & 31u) == 0u)
                slf_puts_center(24u, (t & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PUSH START") : "           ", SLF_PAL_TEXT);
        }
    }
    return 0u;
}

static uint8_t NEOGEO_USER attract_title(void)
{
    uint16_t t;
    slf_scene_begin();
    slf_music(SLF_TRACK_TITLE);
    slf_stage_set(0u);
    slf_title_draw();
    for (t = 0u; t < 6u * 60u; t++) {
        slf_frame();
        slf_title_tick(t);
        if (slf_start_pressed(1u)) { NEO_REGISTER8(SLF_P2_FLAG) = 1u; return 1u; }
        if (slf_start_pending() || slf_start_pressed(0u)) return 1u;
        if ((t & 31u) == 0u)
            slf_puts_center(26u, (t & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PUSH START") : "           ",
                            SLF_PAL_TEXT);
    }
    return 0u;
}

void NEOGEO_USER slf_run_attract(void)
{
    slf_audio_demo(1u);
    for (;;) {
        if (attract_title()) break;
        if (attract_story()) break;
        if (attract_tutorial()) break;
        if (attract_showcase()) break;
        s_def[0] = featured_fighter(0xFFu);
        s_def[1] = featured_fighter(s_def[0]);
        if (run_match(5u, 5u, MODE_DEMO) == RESULT_ABORT) break;
    }
    if (NEO_REGISTER8(SLF_P2_FLAG)) {
        slf_audio_demo(0u);
        versus();
    }
}
