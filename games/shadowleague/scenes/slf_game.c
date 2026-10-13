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
    uint8_t lv = ng_sys_is_mvs() ? (uint8_t)(2u + ng_dip_option(0) * 2u) : 4u;
    lv = (uint8_t)(lv + step);
    return lv > 9u ? 9u : lv;
}

static uint8_t NEOGEO_USER rounds_to_win(void)
{
    uint8_t r = ng_sys_is_mvs() ? (uint8_t)(ng_dip_option(1) + 1u) : 2u;
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

static void NEOGEO_USER select_screen(uint8_t humans)
{
    static const int16_t k_x[2] = {84, 236};
    static const uint8_t k_port_col[7] = { 5u, 9u, 14u, 18u, 23u, 27u, 32u };
    uint8_t cursor[2], last_cursor[2], done[2], p, i;
    uint8_t win_base = 0u, last_active = 0u;
    uint16_t tick = 0, timer = 25u * 60u;
    slf_scene_begin();
    slf_music(SLF_TRACK_TITLE);
    cursor[0] = 0u;
    cursor[1] = 1u;
    last_cursor[0] = last_cursor[1] = 0xFFu;
    done[0] = (uint8_t)!(humans & 1u);
    done[1] = (uint8_t)!(humans & 2u);
    slf_fighter_palettes(cursor[0], 0u, cursor[1], (uint8_t)(cursor[0] == cursor[1]));
    slf_stage_set(0u);
    slf_stage_show(96);
    slf_fighter_bank(0u, 0u);
    slf_fighter_bank(1u, 1u);
    slf_puts_center(1u, "SELECT YOUR FIGHTER", SLF_PAL_GOLD);
    for (;;) {
        uint8_t row = 20u;
        slf_frame();
        tick++;
        if (timer) timer--;
        for (p = 0u; p < 2u; p++) {
            uint8_t press = slf_pressed(p);
            if (done[p]) continue;
            if (press & NGF_RAW_LEFT) {
                cursor[p] = (uint8_t)((cursor[p] + SLF_FIGHTER_COUNT - 1u) % SLF_FIGHTER_COUNT);
                last_active = p;
                slf_sfx(SLF_SFX_MENU);
            } else if (press & NGF_RAW_RIGHT) {
                cursor[p] = (uint8_t)((cursor[p] + 1u) % SLF_FIGHTER_COUNT);
                last_active = p;
                slf_sfx(SLF_SFX_MENU);
            } else if (press & NGF_RAW_UP) {
                cursor[p] = (uint8_t)((cursor[p] + SLF_FIGHTER_COUNT - 11u) % SLF_FIGHTER_COUNT);
                last_active = p;
                slf_sfx(SLF_SFX_MENU);
            } else if (press & NGF_RAW_DOWN) {
                cursor[p] = (uint8_t)((cursor[p] + 11u) % SLF_FIGHTER_COUNT);
                last_active = p;
                slf_sfx(SLF_SFX_MENU);
            }
            if ((press & (NGF_BTN_A | NGF_BTN_C)) || slf_start_pressed(p) || !timer) {
                done[p] = 1u;
                s_pick[p] = cursor[p];
                slf_sfx(SLF_SFX_SELECT);
            }
        }
        {
            uint8_t act = cursor[last_active];
            if (act < 3u) win_base = 0u;
            else if (act > (uint8_t)(SLF_FIGHTER_COUNT - 4u)) win_base = (uint8_t)(SLF_FIGHTER_COUNT - 7u);
            else win_base = (uint8_t)(act - 3u);
        }
        for (i = 0u; i < 2u; i++) {
            uint8_t f_id = done[i] ? s_pick[i] : cursor[i];
            uint8_t chosen = (uint8_t)(done[i] && (humans & (1u << i)));
            slf_fighter_pose(i, f_id, anim_sprite(f_id, chosen ? NGF_A_WIN : NGF_A_IDLE, tick), k_x[i],
                             SLF_FLOOR_Y - 125, i);
        }
        slf_portraits_show(win_base, cursor[0], cursor[1], done[0], done[1], tick);
        if (tick == 1u || cursor[0] != last_cursor[0] || cursor[1] != last_cursor[1]) {
            slf_clear_rows(row, (uint8_t)(row + 3u));
            slf_clear_rows(3u, 3u);
            slf_clear_rows(9u, 9u);
            slf_puts(1u, row, slf_fighter_defs[cursor[0]]->name, SLF_PAL_TEXT);
            slf_puts(1u, (uint8_t)(row + 2u), slf_fighter_origin[cursor[0]], SLF_PAL_GOLD);
            slf_puts(1u, (uint8_t)(row + 3u), slf_fighter_style[cursor[0]], SLF_PAL_DIM);

            slf_puts_right(38u, row, slf_fighter_defs[cursor[1]]->name, SLF_PAL_TEXT);
            slf_puts_right(38u, (uint8_t)(row + 2u), slf_fighter_origin[cursor[1]], SLF_PAL_GOLD);
            slf_puts_right(38u, (uint8_t)(row + 3u), slf_fighter_style[cursor[1]], SLF_PAL_DIM);

            {
                char idx_str[10];
                uint8_t cur_num = (uint8_t)(cursor[last_active] + 1u);
                idx_str[0] = '<';
                idx_str[1] = ' ';
                idx_str[2] = (char)('0' + cur_num / 10u);
                idx_str[3] = (char)('0' + cur_num % 10u);
                idx_str[4] = '/';
                idx_str[5] = '4';
                idx_str[6] = '4';
                idx_str[7] = ' ';
                idx_str[8] = '>';
                idx_str[9] = '\0';
                slf_puts_center(9u, idx_str, SLF_PAL_GOLD);
            }

            last_cursor[0] = cursor[0];
            last_cursor[1] = cursor[1];
            slf_fighter_palettes(cursor[0], 0u, cursor[1], (uint8_t)(cursor[0] == cursor[1]));
        }
        for (p = 0u; p < 2u; p++) {
            uint8_t cur = cursor[p];
            if (!(humans & (1u << p))) continue;
            if (cur >= win_base && cur < (uint8_t)(win_base + 7u)) {
                uint8_t slot = (uint8_t)(cur - win_base);
                uint8_t c = k_port_col[slot];
                uint8_t pal = (uint8_t)(p ? SLF_PAL_BLUE : SLF_PAL_RED);
                if (done[p]) pal = SLF_PAL_GOLD;
                else if (tick & 8u) pal = SLF_PAL_WHITE;
                if (cursor[0] == cursor[1]) {
                    if (p == 0u) slf_puts(c, 3u, "1P", pal);
                    else slf_puts((uint8_t)(c + 2u), 3u, "2P", pal);
                } else {
                    slf_puts((uint8_t)(c + 1u), 3u, p ? "2P" : "1P", pal);
                }
            }
        }
        {
            char t[3];
            t[0] = (char)('0' + (timer / 60u) / 10u);
            t[1] = (char)('0' + (timer / 60u) % 10u);
            t[2] = '\0';
            slf_puts(19u, 26u, t, SLF_PAL_GOLD);
        }
        if (done[0] && done[1]) break;
    }
    slf_portraits_hide();
    wait_frames(50u);
}

static void NEOGEO_USER vs_screen(void)
{
    uint16_t tick;
    slf_scene_begin();
    slf_fighter_palettes(s_def[0], 0u, s_def[1], (uint8_t)(s_def[0] == s_def[1]));
    slf_stage_set(slf_fighter_stages[s_def[1]]);
    slf_stage_show(96);
    slf_fighter_bank(0u, 0u);
    slf_fighter_bank(1u, 1u);
    for (tick = 0u; tick < 150u; tick++) {
        slf_frame();
        slf_fighter_pose(0u, s_def[0], anim_sprite(s_def[0], NGF_A_INTRO, tick), 84, SLF_FLOOR_Y - 125, 0u);
        slf_fighter_pose(1u, s_def[1], anim_sprite(s_def[1], NGF_A_INTRO, tick), 236, SLF_FLOOR_Y - 125, 1u);
        if (tick == 1u) {
            slf_big_center(9u, SLF_BIG_VS, SLF_BIG_VS_W, SLF_BIG_VS_H, SLF_PAL_RED);
            slf_puts(2u, 24u, slf_fighter_defs[s_def[0]]->name, SLF_PAL_GOLD);
            slf_puts_right(37u, 24u, slf_fighter_defs[s_def[1]]->name, SLF_PAL_GOLD);
            slf_puts_center(26u, slf_stage_names[slf_fighter_stages[s_def[1]]], SLF_PAL_TEXT);
        }
    }
}

/* ------------------------------------------------------------ match */
static void NEOGEO_USER announce_clear(void)
{
    slf_clear_rows(8u, 15u);
}

static void NEOGEO_USER announce(NGFMatch *m, uint8_t state)
{
    announce_clear();
    switch (state) {
    case NGF_M_INTRO:
        if (m->rounds_to_win > 1u && m->f[0].wins == m->rounds_to_win - 1u && m->f[1].wins == m->rounds_to_win - 1u)
            slf_big_center(10u, SLF_BIG_FINAL, SLF_BIG_FINAL_W, SLF_BIG_FINAL_H, SLF_PAL_BIG);
        else if (m->round == 1u) slf_big_center(10u, SLF_BIG_ROUND1, SLF_BIG_ROUND1_W, SLF_BIG_ROUND1_H, SLF_PAL_BIG);
        else if (m->round == 2u) slf_big_center(10u, SLF_BIG_ROUND2, SLF_BIG_ROUND2_W, SLF_BIG_ROUND2_H, SLF_PAL_BIG);
        else slf_big_center(10u, SLF_BIG_ROUND3, SLF_BIG_ROUND3_W, SLF_BIG_ROUND3_H, SLF_PAL_BIG);
        break;
    case NGF_M_FIGHT:
        slf_big_center(10u, SLF_BIG_FIGHT, SLF_BIG_FIGHT_W, SLF_BIG_FIGHT_H, SLF_PAL_RED);
        break;
    case NGF_M_KO:
        slf_big_center(10u, SLF_BIG_KO, SLF_BIG_KO_W, SLF_BIG_KO_H, SLF_PAL_RED);
        break;
    case NGF_M_TIME_UP:
        slf_big_center(10u, SLF_BIG_TIMEUP, SLF_BIG_TIMEUP_W, SLF_BIG_TIMEUP_H, SLF_PAL_BIG);
        break;
    case NGF_M_ROUND_END:
        if (m->round_winner < 2u) {
            const NGFFighter *w = &m->f[m->round_winner];
            slf_puts_center(9u, w->def->name, SLF_PAL_GOLD);
            slf_big_center(11u, SLF_BIG_WIN, SLF_BIG_WIN_W, SLF_BIG_WIN_H, SLF_PAL_BIG);
            if (w->life == w->def->life)
                slf_big_center(14u, SLF_BIG_PERFECT, SLF_BIG_PERFECT_W, SLF_BIG_PERFECT_H, SLF_PAL_BLUE);
        } else {
            slf_big_center(10u, SLF_BIG_DRAW, SLF_BIG_DRAW_W, SLF_BIG_DRAW_H, SLF_PAL_BIG);
        }
        break;
    default:
        break;
    }
}

static void NEOGEO_USER play_events(NGFMatch *m)
{
    NGFEvent e;
    while (ngf_pop_event(m, &e)) {
        int16_t sx = (int16_t)(e.x - m->camera_x), sy = (int16_t)(SLF_FLOOR_Y - e.y);
        switch (e.type) {
        case NGF_EV_HIT:
            if (e.data == 0xFEu) { slf_sfx(SLF_SFX_COUNTER); break; }
            slf_sfx(e.data < 10u ? k_sound_map[e.data] : SLF_SFX_HIT_LIGHT);
            slf_fx_spawn(0u, sx, sy);
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
            break;
        case NGF_EV_COUNTER:
            slf_sfx(SLF_SFX_COUNTER);
            slf_big(e.who ? 26u : 4u, 8u, SLF_BIG_COUNTER, SLF_BIG_COUNTER_W, SLF_BIG_COUNTER_H, SLF_PAL_BLUE);
            break;
        case NGF_EV_KO: slf_sfx(SLF_SFX_KO); break;
        case NGF_EV_MOVE:
            if (e.data == NGF_KIND_SUPER) slf_sfx(SLF_SFX_SUPER);
            else if (e.data == NGF_KIND_SPECIAL) slf_sfx(SLF_SFX_SPECIAL);
            else if (e.data != NGF_KIND_THROW) slf_sfx(SLF_SFX_WHOOSH);
            break;
        case NGF_EV_LAND:
            if (e.data == NGF_S_FALL) { slf_sfx(SLF_SFX_LAND); slf_fx_spawn(2u, sx, SLF_FLOOR_Y - 6); }
            break;
        case NGF_EV_FIGHT: slf_sfx(SLF_SFX_BELL); break;
        default: break;
        }
    }
}

static uint8_t NEOGEO_USER run_match(uint8_t lv0, uint8_t lv1, uint8_t mode)
{
    NGFMatch *m = &s_match;
    uint8_t last_state = 0xFFu, last_round = 0u;
    uint16_t over = 0, counter_clear = 0;
    slf_scene_begin();
    slf_fx_reset();
    slf_stage_set(slf_fighter_stages[s_def[1]]);
    slf_stage_show(96);
    slf_fighter_palettes(s_def[0], 0u, s_def[1], (uint8_t)(s_def[0] == s_def[1]));
    if (mode != MODE_DEMO) slf_music(SLF_TRACK_DOJO);
    ngf_match_init(m, slf_fighter_defs[s_def[0]], slf_fighter_defs[s_def[1]], SLF_STAGE_W, slf_rand());
    m->rounds_to_win = mode == MODE_DEMO ? 1u : rounds_to_win();
    ngf_set_cpu(m, 0u, lv0);
    ngf_set_cpu(m, 1u, lv1);
    if (mode == MODE_DEMO) slf_puts_center(22u, "DEMO PLAY", SLF_PAL_GOLD);
    for (;;) {
        slf_frame();
        if (mode == MODE_DEMO && (slf_start_pending() || slf_start_pressed(0u))) return RESULT_ABORT;
        if (mode == MODE_ARCADE && NEO_REGISTER8(SLF_P2_FLAG)) return RESULT_CHALLENGER;
        ngf_match_step(m, slf_joy(0u), slf_joy(1u));
        play_events(m);
        if (m->state != last_state || m->round != last_round) {
            if (m->state == NGF_M_FIGHT && last_state == NGF_M_INTRO) counter_clear = 30u;
            if (m->state != NGF_M_OVER) announce(m, m->state == NGF_M_FIGHT ? NGF_M_FIGHT : m->state);
            last_state = m->state;
            last_round = m->round;
        }
        if (m->state == NGF_M_INTRO && m->state_timer == 60u) announce(m, NGF_M_FIGHT);
        if (counter_clear && !--counter_clear) announce_clear();
        slf_match_draw(m, s_def);
        slf_hud_draw(m);
        if (mode == MODE_DEMO && (m->frame & 31u) == 0u)
            slf_puts_center(24u, (m->frame & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PUSH START") : "           ",
                            SLF_PAL_TEXT);
        if (m->state == NGF_M_OVER && ++over > 60u) return m->winner;
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
    select_screen(3u);
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
    slf_music(SLF_TRACK_TITLE);
    slf_stage_set(0u);
    slf_stage_show(96);
    slf_puts_center(8u, "CONTINUE ?", SLF_PAL_GOLD);
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
    NEO_REGISTER8(SLF_P2_FLAG) = 0u;
    select_screen(1u);
    /* 8-stage tournament: 5 random contenders, 1 rival, 1 sub-boss, 1 final boss */
    for (stage = 0u; stage < 5u; stage++) {
        uint8_t opp;
        do {
            opp = (uint8_t)(slf_rand() % 40u);
        } while (opp == s_pick[0]);
        ladder[stage] = opp;
    }
    ladder[5] = (s_pick[0] == 1u) ? 0u : 1u;
    ladder[6] = (uint8_t)(41u + (slf_rand() & 1u));
    ladder[7] = (s_pick[0] == 43u) ? 42u : 43u;

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
        slf_stage_set(3u);
        slf_stage_show(96);
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

static uint8_t NEOGEO_USER attract_story(void)
{
    uint16_t t;
    slf_scene_begin();
    slf_music(SLF_TRACK_TITLE);
    slf_stage_set(3u);
    slf_stage_show(96);
    slf_puts_center(3u, "PROJECT CHIMERA : PROLOGUE", SLF_PAL_GOLD);
    slf_puts_center(6u, "YEAR 2099 -- NEO TOKYO", SLF_PAL_BLUE);
    slf_puts_center(9u, "THE SHADOW VEIL HAS SUMMONED 44 FIGHTERS", SLF_PAL_WHITE);
    slf_puts_center(11u, "FROM 32 NATIONS ACROSS THE GLOBE.", SLF_PAL_WHITE);
    slf_puts_center(14u, "HIDDEN BENEATH THE TOURNAMENT LIES", SLF_PAL_GOLD);
    slf_puts_center(16u, "A SECRET CYBERNETIC PROTOCOL.", SLF_PAL_RED);
    slf_puts_center(19u, "ONLY THE STRONGEST WILL SURVIVE.", SLF_PAL_TEXT);
    for (t = 0u; t < 360u; t++) {
        slf_frame();
        if (slf_start_pressed(1u)) { NEO_REGISTER8(SLF_P2_FLAG) = 1u; return 1u; }
        if (slf_start_pending() || slf_start_pressed(0u)) return 1u;
        if ((t & 31u) == 0u)
            slf_puts_center(24u, (t & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PUSH START") : "           ", SLF_PAL_TEXT);
    }
    return 0u;
}

static uint8_t NEOGEO_USER attract_tutorial(void)
{
    uint16_t t;
    slf_scene_begin();
    slf_music(SLF_TRACK_TITLE);
    slf_stage_set(0u);
    slf_stage_show(96);
    slf_fighter_palettes(0u, 0u, 3u, 0u);
    slf_fighter_bank(0u, 0u);
    slf_fighter_bank(1u, 1u);
    slf_puts_center(2u, "-- HOW TO PLAY --", SLF_PAL_GOLD);
    slf_puts(4u, 5u, "(A) LIGHT PUNCH    (B) LIGHT KICK", SLF_PAL_WHITE);
    slf_puts(4u, 7u, "(C) HEAVY PUNCH    (D) HEAVY KICK", SLF_PAL_WHITE);
    slf_puts(4u, 10u, "TACTICAL EVASION :  (A) + (B)", SLF_PAL_BLUE);
    slf_puts(4u, 12u, "BLOWBACK COUNTER :  (C) + (D)", SLF_PAL_RED);
    slf_puts(4u, 15u, "SPECIAL MOVES    :  D,DF,F + BTN", SLF_PAL_GOLD);
    slf_puts(4u, 17u, "SUPER CLIMAX     :  QCFx2 / HCF + BTN", SLF_PAL_GOLD);
    for (t = 0u; t < 360u; t++) {
        slf_frame();
        slf_fighter_pose(0u, 0u, anim_sprite(0u, NGF_A_IDLE, t), 72, SLF_FLOOR_Y - 125, 0u);
        slf_fighter_pose(1u, 3u, anim_sprite(3u, NGF_A_IDLE, t), 248, SLF_FLOOR_Y - 125, 1u);
        if (slf_start_pressed(1u)) { NEO_REGISTER8(SLF_P2_FLAG) = 1u; return 1u; }
        if (slf_start_pending() || slf_start_pressed(0u)) return 1u;
        if ((t & 31u) == 0u)
            slf_puts_center(24u, (t & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PUSH START") : "           ", SLF_PAL_TEXT);
    }
    return 0u;
}

static uint8_t NEOGEO_USER attract_showcase(void)
{
    static const uint8_t k_show_fighters[4] = { 0u, 1u, 7u, 43u };
    uint8_t k;
    for (k = 0u; k < 4u; k++) {
        uint8_t f = k_show_fighters[k];
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
    slf_puts_center(1u, "HI-SCORE 50000", SLF_PAL_GOLD);
    for (t = 0u; t < 6u * 60u; t++) {
        slf_frame();
        if (slf_start_pressed(1u)) { NEO_REGISTER8(SLF_P2_FLAG) = 1u; return 1u; }
        if (slf_start_pending() || slf_start_pressed(0u)) return 1u;
        if ((t & 31u) == 0u)
            slf_puts_center(24u, (t & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PUSH START") : "           ",
                            SLF_PAL_TEXT);
    }
    return 0u;
}

void NEOGEO_USER slf_run_attract(void)
{
    for (;;) {
        if (attract_title()) break;
        if (attract_story()) break;
        if (attract_tutorial()) break;
        if (attract_showcase()) break;
        s_def[0] = (uint8_t)(slf_rand() % SLF_FIGHTER_COUNT);
        s_def[1] = (uint8_t)(slf_rand() % SLF_FIGHTER_COUNT);
        if (s_def[0] == s_def[1]) s_def[1] = (uint8_t)((s_def[0] + 1u) % SLF_FIGHTER_COUNT);
        if (run_match(5u, 5u, MODE_DEMO) == RESULT_ABORT) break;
    }
    if (NEO_REGISTER8(SLF_P2_FLAG)) {
        versus();
    }
}
