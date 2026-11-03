/*
 * Shadow League Fighting: game flow — attract demo, fighter select, the
 * VS card, matches with their announcements, the arcade ladder and a
 * second player's challenge.
 */
#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include "sdk/ng_system.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "games/shadowleague/scenes/slf.h"
#include "games/shadowleague/artbox/generated/slf_assets.h"

enum { MODE_ARCADE, MODE_VERSUS, MODE_DEMO };
#define RESULT_CHALLENGER 0xFEu
#define RESULT_ABORT      0xFDu

static NGFMatch s_match;
static uint8_t s_def[2];
static uint8_t s_end_def[2];        /* who was on the floor when the match ended */
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
    if (a->loop != 0xFFu && a->loop && a->loop < a->count && tick >= total) {
        /* played once: from then on only the part from its loop frame on */
        uint16_t intro = 0;
        for (i = 0u; i < a->loop; i++) {
            uint8_t time = d->frames[a->first + i].time;
            intro = (uint16_t)(intro + (time == 255u ? 30u : time));
        }
        t = (uint16_t)(intro + (tick - total) % (uint16_t)(total - intro ? total - intro : 1u));
    }
    for (i = 0u; i < a->count; i++) {
        uint8_t time = d->frames[a->first + i].time;
        uint16_t len = time == 255u ? 30u : time;
        if (t < len) break;
        t = (uint16_t)(t - len);
    }
    if (i >= a->count) i = (uint8_t)(a->count - 1u);
    return d->frames[a->first + i].sprite;
}

/* Like anim_sprite, but plays once and holds the last frame. */
static uint8_t NEOGEO_USER anim_hold(uint8_t def, uint8_t anim, uint16_t tick)
{
    const NGFFighterDef *d = slf_fighter_defs[def];
    const NGFAnim *a = &d->anims[anim];
    uint8_t i;
    for (i = 0u; i + 1u < a->count; i++) {
        uint8_t time = d->frames[a->first + i].time;
        uint16_t len = time == 255u ? 30u : time;
        if (tick < len) break;
        tick = (uint16_t)(tick - len);
    }
    return d->frames[a->first + i].sprite;
}

/* Both formats listed, the chosen one in gold between markers and the other
 * in silver, with its description and the seconds left below. */
static void NEOGEO_USER choose_format(uint8_t player)
{
    uint16_t timer = 10u * 60u, last_secs = 0xFFFFu;
    uint8_t other = player ^ 1u, shown = 0xFFu;
    slf_scene_begin(); slf_story_show(SLF_CARD_MODE);
    slf_puts_tall_center(5u, "CHOOSE YOUR BATTLE", SLF_TALL_GOLD);
    slf_puts_center(21u, "UP / DOWN - CHANGE", SLF_PAL_TEXT);
    slf_puts_center(23u, "A OR C - CONFIRM", SLF_PAL_TEXT);
    for (;;) {
        slf_frame();
        if ((slf_pressed(player) | slf_pressed(other)) &
            (NGF_RAW_UP | NGF_RAW_DOWN | NGF_RAW_LEFT | NGF_RAW_RIGHT | NGF_BTN_B)) {
            s_team_mode ^= 1u; slf_sfx(SLF_SFX_MENU);
        }
        if (s_team_mode != shown) {
            shown = s_team_mode;
            slf_puts_tall_center(9u, s_team_mode ? "      1 VS 1      " : ">     1 VS 1     <",
                                 s_team_mode ? SLF_TALL_SILVER : SLF_TALL_GOLD);
            slf_puts_tall_center(12u, s_team_mode ? "> 3 VS 3 + STRIKER <" : "  3 VS 3 + STRIKER  ",
                                 s_team_mode ? SLF_TALL_GOLD : SLF_TALL_SILVER);
            slf_puts_center(16u, s_team_mode ? "THREE FIGHTERS AND A STRIKER" : "    ONE FIGHTER EACH SIDE   ",
                            SLF_PAL_GOLD);
        }
        if (timer / 60u != last_secs) {
            char t[3];
            last_secs = timer / 60u;
            t[0] = (char)('0' + last_secs / 10u);
            t[1] = (char)('0' + last_secs % 10u);
            t[2] = '\0';
            slf_puts_tall(19u, 25u, t, SLF_TALL_SILVER);
        }
        if ((slf_pressed(player) & (NGF_BTN_A | NGF_BTN_C)) || slf_start_pressed(player) ||
            (slf_pressed(other) & (NGF_BTN_A | NGF_BTN_C)) || slf_start_pressed(other) || !--timer) break;
    }
    slf_sfx(SLF_SFX_SELECT);
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

/* Up to n characters of text (the two players' lines share the bottom rows,
 * columns 3-36: 16 each and a gap). */
static const char *NEOGEO_USER clip(char *buf, const char *text, uint8_t n)
{
    uint8_t i;
    for (i = 0u; i < n && text[i]; i++) buf[i] = text[i];
    buf[i] = '\0';
    return buf;
}

/* The fighting style in gold over the name, a clear row between them (the
 * country is on the VS screen). */
static void NEOGEO_USER select_info(uint8_t side, uint8_t def)
{
    char buf[20];
    if (side == 0u) {
        slf_puts(3u, 24u, clip(buf, slf_fighter_style[def], 16u), SLF_PAL_GOLD);
        slf_puts_tall(3u, 26u, slf_fighter_defs[def]->name, SLF_TALL_SILVER);
    } else {
        slf_puts_right(36u, 24u, clip(buf, slf_fighter_style[def], 16u), SLF_PAL_GOLD);
        slf_puts_tall_right(36u, 26u, slf_fighter_defs[def]->name, SLF_TALL_SILVER);
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

static void NEOGEO_USER vs_screen(uint8_t stage_fighter)
{
    uint16_t tick;
    uint8_t stage_id = stage_fighter < SLF_FIGHTER_COUNT ? slf_fighter_stages[stage_fighter] : 0u;
    soundStopAll();
    slf_scene_begin();
    slf_sfx(SLF_SFX_WHOOSH);
    slf_fighter_palettes(s_def[0], 0u, s_def[1], (uint8_t)(s_def[0] == s_def[1]));
    slf_story_show(SLF_CARD_VS);
    slf_fighter_bank(0u, 0u);
    slf_fighter_bank(1u, 1u);
    for (tick = 0u; tick < 150u; tick++) {
        slf_frame();
        slf_fighter_pose(0u, s_def[0], anim_sprite(s_def[0], NGF_A_INTRO, tick), 84, SLF_FLOOR_Y - 125, 0u);
        slf_fighter_pose(1u, s_def[1], anim_sprite(s_def[1], NGF_A_INTRO, tick), 236, SLF_FLOOR_Y - 125, 1u);
        if (tick == 1u) {
            char buf[20];
            slf_big_center(9u, SLF_BIG_VS, SLF_BIG_VS_W, SLF_BIG_VS_H, SLF_PAL_RED);
            slf_puts_tall(3u, 21u, slf_fighter_defs[s_def[0]]->name, SLF_TALL_RED);
            slf_puts_tall_right(36u, 21u, slf_fighter_defs[s_def[1]]->name, SLF_TALL_BLUE);
            slf_puts(3u, 23u, clip(buf, slf_fighter_origin[s_def[0]], 16u), SLF_PAL_TEXT);
            slf_puts_right(36u, 23u, clip(buf, slf_fighter_origin[s_def[1]], 16u), SLF_PAL_TEXT);
            slf_puts_center(26u, slf_stage_names[stage_id], SLF_PAL_TEXT);
        }
    }
}

/* ------------------------------------------------------------ score */
/*
 * Points as the blows land -- ten a point of damage, a hundred a hit, five
 * hundred a counter hit -- and after each round or bout a player wins, the
 * bonus count: what life is left, the seconds left on the clock, and the
 * untouched bonus when no blow got through, each line counted up with a tick
 * and settled with a chime, then the total counted into the score.
 */
uint32_t slf_score[2];
static uint8_t s_scoring;                 /* a match that counts (not the attract's) */
static uint16_t s_seen_life[2], s_bout_life[2];
static const NGFFighterDef *s_seen_def[2];
static uint8_t s_untouched[2];            /* untouched wins this match */
static uint8_t s_round_untouched;         /* the round just ended was won untouched */
static uint8_t s_end_team;                /* the match just ended was a team battle */

#define SCORE_MAX 99999999UL
#define BONUS_UNTOUCHED 20000UL
#define BONUS_SWEEP     50000UL
#define BONUS_LEGEND   100000UL

static void NEOGEO_USER score_add(uint8_t side, uint32_t points)
{
    if (!s_scoring || side > 1u) return;
    slf_score[side] = slf_score[side] + points > SCORE_MAX ? SCORE_MAX : slf_score[side] + points;
}

static void NEOGEO_USER score_step(const NGFMatch *m)
{
    uint8_t i;
    for (i = 0u; i < 2u; i++) {
        const NGFFighter *f = &m->f[i];
        if (f->def == s_seen_def[i] && f->life < s_seen_life[i] &&
            (m->state == NGF_M_FIGHT || m->state == NGF_M_KO))
            score_add((uint8_t)(i ^ 1u), (uint32_t)(s_seen_life[i] - f->life) * 10u);
        s_seen_life[i] = f->life;
        s_seen_def[i] = f->def;
    }
}

/* value as decimal, right-aligned to end at column x_end (no leading zeros);
 * `tall` in the two-row face */
static void NEOGEO_USER put_number(uint8_t x_end, uint8_t y, uint32_t value, uint8_t pal, uint8_t tall)
{
    static const uint32_t pow10[8] = {10000000UL, 1000000UL, 100000UL, 10000UL, 1000UL, 100UL, 10UL, 1UL};
    char text[9];
    uint8_t i, n = 0u;
    for (i = 0u; i < 8u; i++) {
        char d = '0';
        while (value >= pow10[i]) { value -= pow10[i]; d++; }
        if (n || d != '0' || i == 7u) text[n++] = d;
    }
    text[n] = '\0';
    if (tall) slf_puts_tall_right(x_end, y, text, pal);
    else slf_puts_right(x_end, y, text, pal);
}

enum { TALLY_OFF, TALLY_BANNER, TALLY_LINE, TALLY_TOTAL, TALLY_HOLD };
#define TALLY_X     9u              /* labels from here; numbers end at TALLY_END */
#define TALLY_END   30u
#define TALLY_COUNT 20u             /* frames a line counts for */
static uint8_t s_tally, s_tally_side, s_tally_line, s_tally_lines;
static uint16_t s_tally_t;
static uint32_t s_tally_value[3], s_tally_shown, s_tally_total, s_tally_step;
static const uint8_t k_tally_row[3] = {9u, 11u, 13u};
static uint16_t s_win_voice_hold;

static void NEOGEO_USER tally_begin(NGFMatch *m, uint8_t side)
{
    const NGFFighter *f = &m->f[side];
    uint32_t pct = f->def->life ? ((uint32_t)f->life * 100u) / f->def->life : 0u;
    s_tally_value[0] = pct * 100u;
    s_tally_value[1] = (uint32_t)m->seconds * 100u;
    s_tally_value[2] = s_round_untouched ? BONUS_UNTOUCHED : 0u;
    s_tally_lines = s_round_untouched ? 3u : 2u;
    s_tally_total = s_tally_value[0] + s_tally_value[1] + s_tally_value[2];
    s_tally_side = side;
    s_tally = TALLY_BANNER;
    s_tally_t = 0u;
    m->end_hold = 1u;
}

static void NEOGEO_USER tally_line_start(void)
{
    s_tally_shown = 0u;
    s_tally_step = s_tally_value[s_tally_line] / TALLY_COUNT + 1u;
    s_tally_t = 0u;
}

/* Everything counted at once (a button), the rest of the total into the score. */
static void NEOGEO_USER tally_finish(void)
{
    uint8_t k;
    for (k = 0u; k < s_tally_lines; k++) put_number(TALLY_END, k_tally_row[k], s_tally_value[k], SLF_PAL_TEXT, 0u);
    score_add(s_tally_side, s_tally_total - (s_tally == TALLY_TOTAL ? s_tally_shown : 0u));
    slf_puts_tall(TALLY_X, 16u, "TOTAL", SLF_TALL_GOLD);
    put_number(TALLY_END, 16u, s_tally_total, SLF_TALL_GOLD, 1u);
    slf_sfx(SLF_SFX_TALLY_END);
    s_tally = TALLY_HOLD;
    s_tally_t = 40u;
}

static void NEOGEO_USER tally_step(NGFMatch *m)
{
    uint8_t skip = (uint8_t)(slf_pressed(s_tally_side) & (NGF_BTN_A | NGF_BTN_B | NGF_BTN_C | NGF_BTN_D));
    s_tally_t++;
    switch (s_tally) {
    case TALLY_BANNER:                       /* the winner's name and the calls first */
        if (s_tally_t < 56u) return;
        slf_clear_rows(7u, 17u);
        slf_puts(TALLY_X, k_tally_row[0], "LIFE BONUS", SLF_PAL_TEXT);
        slf_puts(TALLY_X, k_tally_row[1], "TIME BONUS", SLF_PAL_TEXT);
        if (s_tally_lines > 2u) slf_puts(TALLY_X, k_tally_row[2], "UNTOUCHED BONUS", SLF_PAL_GOLD);
        s_tally = TALLY_LINE;
        s_tally_line = 0u;
        tally_line_start();
        return;
    case TALLY_LINE:
        if (skip) { tally_finish(); return; }
        if (s_tally_t <= 6u) return;         /* a beat between the lines */
        s_tally_shown += s_tally_step;
        if (s_tally_shown >= s_tally_value[s_tally_line]) {
            put_number(TALLY_END, k_tally_row[s_tally_line], s_tally_value[s_tally_line], SLF_PAL_TEXT, 0u);
            slf_sfx(SLF_SFX_TALLY_END);
            if (++s_tally_line < s_tally_lines) { tally_line_start(); return; }
            s_tally = TALLY_TOTAL;
            slf_puts_tall(TALLY_X, 16u, "TOTAL", SLF_TALL_GOLD);
            s_tally_shown = 0u;
            s_tally_step = s_tally_total / (TALLY_COUNT + 10u) + 1u;
            s_tally_t = 0u;
            return;
        }
        put_number(TALLY_END, k_tally_row[s_tally_line], s_tally_shown, SLF_PAL_TEXT, 0u);
        if (!(s_tally_t % 3u)) slf_sfx(SLF_SFX_TALLY);
        return;
    case TALLY_TOTAL:                        /* counted into the score, the HUD with it */
        if (skip) { tally_finish(); return; }
        if (s_tally_t <= 10u) return;
        {
            uint32_t step = s_tally_step;
            if (s_tally_shown + step > s_tally_total) step = s_tally_total - s_tally_shown;
            s_tally_shown += step;
            score_add(s_tally_side, step);
        }
        put_number(TALLY_END, 16u, s_tally_shown, SLF_TALL_GOLD, 1u);
        if (s_tally_shown >= s_tally_total) {
            slf_sfx(SLF_SFX_TALLY_END);
            s_tally = TALLY_HOLD;
            s_tally_t = 0u;
        } else if (!(s_tally_t % 3u)) slf_sfx(SLF_SFX_TALLY);
        return;
    case TALLY_HOLD:
        if (s_tally_t < 70u && !(skip && s_tally_t > 10u)) return;
        slf_clear_rows(7u, 17u);
        s_tally = TALLY_OFF;
        m->end_hold = (uint8_t)(s_win_voice_hold != 0u);
        return;
    default:
        return;
    }
}

/* ------------------------------------------------------------ match */
static uint8_t s_feedback_timer;
static uint8_t s_ref_phase = 0xFFu;

/* The referee's count, 3, 2, 1, above his head once he has walked in: the
 * round title makes way for it, and the fight call clears it. The figure
 * itself is drawn with the match sprites. */
static void NEOGEO_USER referee_call(const NGFMatch *m)
{
    uint16_t t = m->state_timer;
    uint16_t intro = m->intro_frames ? m->intro_frames : 96u;
    uint16_t slash = (uint16_t)(intro > 36u ? intro - 36u : intro);
    uint8_t phase = 0u;
    if (t >= slash) phase = 4u;
    else if (t >= (uint16_t)(intro * 58u / 100u)) phase = 3u;
    else if (t >= (uint16_t)(intro * 40u / 100u)) phase = 2u;
    else if (t >= (uint16_t)(intro * 22u / 100u)) phase = 1u;
    if (phase == s_ref_phase) return;
    s_ref_phase = phase;
    if (phase == 0u) return;
    if (phase == 4u) {                      /* the fight call is already on rows 10-13 */
        slf_clear_rows(6u, 9u);
        return;
    }
    slf_clear_rows(6u, 13u);
    if (phase == 1u) slf_big_center(6u, SLF_BIG_COUNT3, SLF_BIG_COUNT3_W, SLF_BIG_COUNT3_H, SLF_PAL_BIG);
    else if (phase == 2u) slf_big_center(6u, SLF_BIG_COUNT2, SLF_BIG_COUNT2_W, SLF_BIG_COUNT2_H, SLF_PAL_BIG);
    else slf_big_center(6u, SLF_BIG_COUNT1, SLF_BIG_COUNT1_W, SLF_BIG_COUNT1_H, SLF_PAL_RED);
    slf_sfx(phase == 3u ? SLF_SFX_WHOOSH : SLF_SFX_SELECT);
}

static void NEOGEO_USER announce_clear(void)
{
    slf_clear_rows(7u, 17u);
    s_feedback_timer = 0u;
}

static void NEOGEO_USER announcer(uint8_t line)
{
    slf_sfx((uint8_t)(SLF_SND_ANNOUNCER + line));
}

/* A fighter's own lines (slf_fighter_voice): their win, then their shouts. */
enum { SLF_VOICE_WIN, SLF_VOICE_ATTACK, SLF_VOICE_HEAVY, SLF_VOICE_THROW, SLF_VOICE_HURT, SLF_VOICE_KO };

static void NEOGEO_USER fighter_says(uint8_t def, uint8_t line)
{
    slf_voice(slf_fighter_voice[(uint16_t)def * SLF_VOICE_KINDS + line]);
}

/* The shared kiai of the fighter's voice type ("Hyah!", "Ugh!", the K.O. cry,
 * "Haaaaaa!") and their own line take turns, so both stay in the fight. */
static uint8_t s_shout_turn;

static void NEOGEO_USER shout(uint8_t def, uint8_t kind, uint8_t line)
{
    if (++s_shout_turn & 1u) slf_sfx((uint8_t)(SLF_SND_TYPES + slf_voice_type[def] * 4u + kind));
    else fighter_says(def, line);
}

/* A fighter's grunt (SLF_GRUNT_*): the power-up is always the kiai, the K.O.
 * always their own last word. */
static void NEOGEO_USER grunt(uint8_t def, uint8_t kind)
{
    if (kind == SLF_GRUNT_POWER) slf_sfx((uint8_t)(SLF_SND_TYPES + slf_voice_type[def] * 4u + kind));
    else if (kind == SLF_GRUNT_KO) fighter_says(def, SLF_VOICE_KO);
    else shout(def, kind, kind == SLF_GRUNT_HURT ? SLF_VOICE_HURT : SLF_VOICE_ATTACK);
}

/* The round's winner speaks once the win pose is under way. */
static uint8_t s_win_voice_t, s_win_voice_def;

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
        slf_big_center(10u, SLF_BIG_CLASH, SLF_BIG_CLASH_W, SLF_BIG_CLASH_H, SLF_PAL_RED);
        break;
    case NGF_M_KO:
        announcer(SLF_ANN_KO);
        slf_big_center(10u, SLF_BIG_BLACKOUT, SLF_BIG_BLACKOUT_W, SLF_BIG_BLACKOUT_H, SLF_PAL_RED);
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
            if (s_round_untouched) {         /* no blow got through: a sting of its own */
                announcer(SLF_ANN_PERFECT);
                slf_sfx(SLF_SFX_PERFECT);
                slf_big_center(14u, SLF_BIG_UNTOUCHED, SLF_BIG_UNTOUCHED_W, SLF_BIG_UNTOUCHED_H, SLF_PAL_BLUE);
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

/* A player's match win: the winner's name, then THE NIGHT IS YOURS swept in
 * a column a frame, flashing white before it settles to gold. */
static void NEOGEO_USER night_call(const NGFMatch *m, uint16_t over)
{
    if (over == 1u) {
        announce_clear();
        slf_puts_tall_center(7u, m->f[m->winner].def->name, SLF_TALL_GOLD);
        announcer(SLF_ANN_YOUWIN);
    }
    if (over <= SLF_BIG_NIGHT_W)
        slf_big_reveal(10u, SLF_BIG_NIGHT, SLF_BIG_NIGHT_W, SLF_BIG_NIGHT_H, SLF_PAL_BIG, (uint8_t)over);
    else if (over <= SLF_BIG_NIGHT_W + 24u && !((over - SLF_BIG_NIGHT_W) & 3u))
        /* six steps, white on the odd ones, so it ends gold */
        slf_big_center(10u, SLF_BIG_NIGHT, SLF_BIG_NIGHT_W, SLF_BIG_NIGHT_H,
                       (((over - SLF_BIG_NIGHT_W) >> 2) & 1u) ? SLF_PAL_WHITE : SLF_PAL_BIG);
}

/*
 * Before THE NIGHT IS YOURS, a match won the hard way gets its own call:
 * three wins without a blow taken (TRIPLE UNTOUCHED, the legend's fanfare),
 * or a whole team beaten by the first fighter sent in (CLEAN SWEEP). Its
 * bonus is counted into the score under it.
 */
enum { FINALE_NONE, FINALE_SWEEP, FINALE_LEGEND };
#define FINALE_FRAMES 170u
static uint8_t s_finale;
static uint32_t s_finale_shown;

static void NEOGEO_USER finale_step(const NGFMatch *m, uint16_t over)
{
    uint8_t legend = (uint8_t)(s_finale == FINALE_LEGEND), w = m->winner;
    uint32_t bonus = legend ? BONUS_LEGEND : BONUS_SWEEP;
    uint16_t tile = legend ? SLF_BIG_LEGEND : SLF_BIG_SWEEP;
    uint8_t tw = legend ? SLF_BIG_LEGEND_W : SLF_BIG_SWEEP_W, th = legend ? SLF_BIG_LEGEND_H : SLF_BIG_SWEEP_H;
    if (over == 1u) {
        announce_clear();
        slf_puts_tall_center(6u, m->f[w].def->name, SLF_TALL_GOLD);
        slf_sfx(legend ? SLF_SFX_LEGEND : SLF_SFX_SWEEP);
        s_finale_shown = 0u;
    }
    if (over <= tw) slf_big_reveal(9u, tile, tw, th, SLF_PAL_BIG, (uint8_t)over);
    else if (over <= (uint16_t)(tw + 32u) && !((over - tw) & 3u))
        slf_big_center(9u, tile, tw, th, (((over - tw) >> 2) & 1u) ? SLF_PAL_WHITE : SLF_PAL_BIG);
    if (over == 30u) announcer(legend ? SLF_ANN_LEGEND : SLF_ANN_SWEEP);
    if (over == 50u) slf_puts(TALLY_X, 14u, legend ? "LEGEND BONUS" : "SWEEP BONUS", SLF_PAL_GOLD);
    if (over > 56u && s_finale_shown < bonus) {
        uint32_t step = bonus / 40u;
        if (s_finale_shown + step > bonus) step = bonus - s_finale_shown;
        s_finale_shown += step;
        score_add(w, step);
        put_number(TALLY_END, 14u, s_finale_shown, SLF_PAL_TEXT, 0u);
        if (s_finale_shown >= bonus) slf_sfx(SLF_SFX_TALLY_END);
        else if (!(over % 3u)) slf_sfx(SLF_SFX_TALLY);
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
        fighter_says(def, SLF_VOICE_THROW);
        break;
    case NGF_KIND_COMMAND:
    case NGF_KIND_GUARD_ATTACK:
        fighter_says(def, SLF_VOICE_HEAVY);
        break;
    case NGF_KIND_NORMAL:
        /* heavy normals every other one, light ones now and then */
        if (f->move->buttons & (NGF_BTN_C | NGF_BTN_D)) { if (m->frame & 1u) shout(def, SLF_GRUNT_ATTACK, SLF_VOICE_HEAVY); }
        else if (!(m->frame % 3u)) shout(def, SLF_GRUNT_ATTACK, SLF_VOICE_ATTACK);
        break;
    default:
        break;
    }
}

static uint8_t s_move_state[2];

void NEOGEO_USER slf_combat_events(NGFMatch *m, const uint8_t *defs)
{
    NGFEvent e;
    uint8_t i;
    for (i = 0u; i < 2u; i++) if (s_hurt_quiet[i]) s_hurt_quiet[i]--;
    /* Dust kicked up from the pushing foot as a dash or back-dash starts:
     * behind a forward dash, in front of a back-dash. */
    for (i = 0u; i < 2u; i++) {
        const NGFFighter *f = &m->f[i];
        uint8_t st = f->state;
        if (st != s_move_state[i] && (st == NGF_S_DASH || st == NGF_S_BACKDASH)) {
            int16_t fx = (int16_t)(ngf_fighter_px(f) - m->camera_x);
            slf_fx_spawn(2u, (int16_t)(fx + ((f->facing_left ^ (st == NGF_S_BACKDASH)) ? 18 : -18)),
                         SLF_FLOOR_Y - 6);
        }
        s_move_state[i] = st;
    }
    while (ngf_pop_event(m, &e)) {
        int16_t sx = (int16_t)(e.x - m->camera_x), sy = (int16_t)(SLF_FLOOR_Y - e.y);
        switch (e.type) {
        case NGF_EV_HIT:
            score_add(e.who, e.data == 0xFEu ? 500u : 100u);
            if (e.data == 0xFEu) { slf_sfx(SLF_SFX_COUNTER); break; }
            slf_sfx(e.data < 10u ? k_sound_map[e.data] : SLF_SFX_HIT_LIGHT);
            slf_fx_spawn(0u, sx, sy);
            if (e.data == 2u) slf_shake(6u);           /* a heavy blow lands */
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
            slf_shake(10u);
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
            slf_shake(18u);
            if (e.who < 2u) grunt(defs[e.who], SLF_GRUNT_KO);
            break;
        case NGF_EV_MOVE:
            if (e.data == NGF_KIND_SUPER) {
                slf_sfx(SLF_SFX_SUPER); slf_fx_spawn(4u, sx, (int16_t)(SLF_FLOOR_Y - 66));
                slf_trail(e.who, 48u);
            }
            else if (e.data == NGF_KIND_SPECIAL) {
                slf_sfx(SLF_SFX_SPECIAL);
                slf_trail(e.who, 24u);
                if (e.who < 2u && slf_fighter_fx[defs[e.who]]) slf_fx_spawn(3u, (int16_t)(sx + (m->f[e.who].facing_left ? -28 : 28)), (int16_t)(SLF_FLOOR_Y - 64));
            }
            else if (e.data != NGF_KIND_THROW) slf_sfx(SLF_SFX_WHOOSH);
            if (e.who < 2u) move_voice(m, e.who, defs[e.who], e.data);
            break;
        case NGF_EV_LAND:
            if (e.data == NGF_S_FALL) { slf_sfx(SLF_SFX_LAND); slf_shake(8u); }
            slf_fx_spawn(2u, sx, SLF_FLOOR_Y - 6);  /* any landing, from a jump or a back-dash too */
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
    uint8_t cpu_side = lv0 ? 0u : 1u;
    uint8_t initial_stage = slf_fighter_stages[s_def[cpu_side]];
    uint16_t over = 0, counter_clear = 0;
    slf_scene_begin();
    slf_fx_reset();
    slf_stage_set(initial_stage);
    slf_stage_show(96);
    slf_fighter_palettes(s_def[0], 0u, s_def[1], (uint8_t)(s_def[0] == s_def[1]));
    if (mode != MODE_DEMO) slf_music((uint8_t)(SLF_TRACK_STAGE + initial_stage));
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
    /* Room for the referee to appear and count before the fight call, with
     * the fighters held well apart while he does. */
    ngf_set_intro_frames(m, 200u);
    ngf_set_start_distance(m, 104u);
    if (mode == MODE_DEMO) slf_puts_center(22u, "DEMO PLAY", SLF_PAL_GOLD);
    s_scoring = (uint8_t)(mode != MODE_DEMO);
    slf_hud_scores = s_scoring;
    s_untouched[0] = s_untouched[1] = 0u;
    s_seen_def[0] = s_seen_def[1] = 0;
    s_bout_life[0] = m->f[0].life; s_bout_life[1] = m->f[1].life;
    s_tally = TALLY_OFF;
    s_finale = FINALE_NONE;
    s_end_team = team;
    for (;;) {
        slf_frame();
        if (mode == MODE_DEMO && (slf_start_pending() || slf_start_pressed(0u) || slf_start_pressed(1u))) return RESULT_ABORT;
        if (mode == MODE_ARCADE) {
            if (lv0 == 0u) {
                /* P1 playing: P2 can challenge */
                if (NEO_REGISTER8(SLF_P2_FLAG) || slf_start_pressed(1u)) {
                    if (ng_sys_is_mvs()) {
                        uint16_t saved_sr;
                        __asm__ volatile ("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d" (saved_sr) : : "memory");
                        NEO_REGISTER8(0x10FDB0) = 0;
                        NEO_REGISTER8(0x10FDB1) = 1;
                        CALLNEOGEOF(SYS_CREDIT_CHECK);
                        if (NEO_REGISTER8(0x10FDB1)) {
                            CALLNEOGEOF(SYS_CREDIT_DOWN);
                            __asm__ volatile ("move.w %0,%%sr" : : "d" (saved_sr) : "memory");
                            NEO_REGISTER8(BIOS_PLAYER2_MODE) = 1;
                            return RESULT_CHALLENGER;
                        }
                        __asm__ volatile ("move.w %0,%%sr" : : "d" (saved_sr) : "memory");
                    } else {
                        NEO_REGISTER8(BIOS_PLAYER2_MODE) = 1;
                        return RESULT_CHALLENGER;
                    }
                }
            } else {
                /* P2 playing: P1 can challenge */
                if (NEO_REGISTER8(SLF_START_FLAG) || slf_start_pressed(0u)) {
                    if (ng_sys_is_mvs()) {
                        uint16_t saved_sr;
                        __asm__ volatile ("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d" (saved_sr) : : "memory");
                        NEO_REGISTER8(0x10FDB0) = 1;
                        NEO_REGISTER8(0x10FDB1) = 0;
                        CALLNEOGEOF(SYS_CREDIT_CHECK);
                        if (NEO_REGISTER8(0x10FDB0)) {
                            CALLNEOGEOF(SYS_CREDIT_DOWN);
                            __asm__ volatile ("move.w %0,%%sr" : : "d" (saved_sr) : "memory");
                            NEO_REGISTER8(BIOS_PLAYER1_MODE) = 1;
                            return RESULT_CHALLENGER;
                        }
                        __asm__ volatile ("move.w %0,%%sr" : : "d" (saved_sr) : "memory");
                    } else {
                        NEO_REGISTER8(BIOS_PLAYER1_MODE) = 1;
                        return RESULT_CHALLENGER;
                    }
                }
            }
        }
        ngf_match_step(m, slf_joy(0u), slf_joy(1u));
        if (s_feedback_timer && !--s_feedback_timer) slf_clear_rows(7u, 9u);
        slf_combat_events(m, active_def);
        score_step(m);
        if (m->state != last_state || m->round != last_round) {
            if (m->state == NGF_M_FIGHT && last_state == NGF_M_INTRO) {
                counter_clear = 30u;
                s_bout_life[0] = m->f[0].life;     /* untouched: none of this lost */
                s_bout_life[1] = m->f[1].life;
            }
            if (m->state == NGF_M_ROUND_END) {
                uint8_t w = m->round_winner;
                if (w < 2u) {
                    s_win_voice_def = active_def[w];
                    s_win_voice_t = 30u;
                    s_win_voice_hold = (uint16_t)(s_win_voice_t + slf_fighter_voice_frames[s_win_voice_def] + 20u);
                    m->end_hold = 1u;
                }
                s_round_untouched = (uint8_t)(w < 2u && m->f[w].life && m->f[w].life >= s_bout_life[w]);
                if (s_round_untouched) s_untouched[w]++;
            }
            if (m->state != NGF_M_OVER) announce(m, m->state == NGF_M_FIGHT ? NGF_M_FIGHT : m->state);
            /* a player's round or bout: the bonus count, the end held for it */
            if (m->state == NGF_M_ROUND_END && m->round_winner < 2u && s_scoring && !m->f[m->round_winner].cpu)
                tally_begin(m, m->round_winner);
            last_state = m->state;
            last_round = m->round;
        }
        if (m->state == NGF_M_INTRO && m->state_timer == (uint16_t)(m->intro_frames - 36u)) announce(m, NGF_M_FIGHT);
        if (m->state == NGF_M_INTRO) referee_call(m);
        else s_ref_phase = 0xFFu;
        if (counter_clear && !--counter_clear) announce_clear();
        if (s_win_voice_t && !--s_win_voice_t) fighter_says(s_win_voice_def, SLF_VOICE_WIN);
        if (s_win_voice_hold) {
            if (!--s_win_voice_hold && !s_tally) m->end_hold = 0u;
        }
        if (s_tally) tally_step(m);
        if (team) {
            uint8_t side, changed = 0u;
            for (side = 0u; side < 2u; side++) {
                uint8_t def = s_roster[side][m->team_active[side]];
                if (def != active_def[side]) { active_def[side] = def; changed = 1u; }
            }
            if (changed) {
                uint8_t new_stage = slf_fighter_stages[active_def[cpu_side]];
                /* The next fighter comes in through a short fade: the new colours and
                 * a new arena are loaded while the screen is black, so no frame ever
                 * shows new tiles under old palettes (or the other way round). */
                uint8_t wait;
                ng_palfx_screen_fade_out(16u, 6u);
                for (wait = 0u; wait < 12u && ng_palfx_screen_fading(); wait++) slf_frame();
                slf_fighter_palettes(active_def[0], 0u, active_def[1], (uint8_t)(active_def[0] == active_def[1]));
                if (new_stage != slf_stage_current()) {
                    /* the old arena goes and the new colours load; its tiles only
                     * once those colours have reached palette RAM */
                    slf_stage_set(new_stage);
                    for (wait = 0u; wait < 30u && !ng_palfx_screen_ready(SLF_PAL_STAGE, SLF_STAGE_BANKS); wait++)
                        slf_frame();
                    slf_stage_show(m->camera_x);
                    if (mode != MODE_DEMO) slf_music((uint8_t)(SLF_TRACK_STAGE + new_stage));
                }
                slf_frame();
                ng_palfx_screen_fade_in(0u, 8u);
            }
        }
        slf_match_draw(m, active_def);
        slf_hud_draw(m);
        if (mode == MODE_DEMO && (m->frame & 31u) == 0u)
            slf_puts_center(24u, (m->frame & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PUSH START") : "           ",
                            SLF_PAL_TEXT);
        if (m->state == NGF_M_OVER) {
            uint8_t player_won = (uint8_t)(mode != MODE_DEMO && m->winner < 2u && !m->f[m->winner].cpu);
            uint16_t lead;
            if (++over == 1u && player_won) {
                uint8_t w = m->winner;
                if (s_untouched[w] >= 3u) s_finale = FINALE_LEGEND;
                else if (team && m->team_active[w] == 0u && m->team_count[w ^ 1u] >= 3u) s_finale = FINALE_SWEEP;
            }
            lead = s_finale ? FINALE_FRAMES : 0u;
            if (player_won && over <= lead) finale_step(m, over);
            else if (player_won) night_call(m, (uint16_t)(over - lead));
            if (over > (player_won ? 120u + lead : 60u)) {
                s_end_def[0] = active_def[0];
                s_end_def[1] = active_def[1];
                return m->winner;
            }
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

/* After a match: the winner's victory poses, and the side that lost -- the
 * whole team after a team battle, one fighter otherwise -- down on one knee
 * facing it, darkened, in the arena with its lights down; the winner's last
 * word above. Any button after the first second moves on. */
static void NEOGEO_USER win_scene(uint8_t side)
{
    static const int16_t k_lose_x[3] = {150, 210, 270};   /* a kneel is some 60 pixels wide */
    uint8_t win_def = s_end_def[side], lose[3], n = 0u, k, win_alt = 0u;
    uint8_t stage = slf_stage_current();
    uint16_t tick, voice_len = (uint16_t)slf_fighter_voice_frames[win_def];
    uint16_t total_ticks = (uint16_t)(voice_len + 150u);
    const char *q0 = slf_fighter_quote[win_def][0];
    const char *q1 = slf_fighter_quote[win_def][1];
    uint8_t len0 = slf_slen(q0), len1 = slf_slen(q1);
    uint8_t x0 = (uint8_t)(len0 < 40u ? (40u - len0) / 2u : 0u);
    uint8_t x1 = (uint8_t)(len1 < 40u ? (40u - len1) / 2u : 0u);
    if (total_ticks < 320u) total_ticks = 320u;
    if (s_end_team) for (k = 0u; k < 3u; k++) lose[n++] = s_roster[side ^ 1u][k];
    else lose[n++] = s_end_def[side ^ 1u];
    for (k = 0u; k < n; k++) if (lose[k] == win_def && side) win_alt = 1u;   /* (the side-2 copy wore the other colours) */
    slf_scene_begin();
    slf_music(SLF_TRACK_VICTORY);
    slf_actor_palettes(0u, win_def, win_alt, 0u);
    for (k = 0u; k < n; k++) slf_actor_palettes((uint8_t)(1u + k), lose[k], (uint8_t)(lose[k] == win_def && !side), 1u);
    slf_stage_set(stage);
    slf_stage_show(96);
    slf_stage_darken(1u);
    slf_puts_tall_center(3u, slf_fighter_defs[win_def]->name, SLF_TALL_GOLD);
    for (tick = 0u; tick < total_ticks; tick++) {
        slf_frame();
        if (tick == 20u) fighter_says(win_def, SLF_VOICE_WIN);
        if (tick >= 20u) {
            uint16_t sp = (uint16_t)(tick - 20u);
            uint8_t c0 = (uint8_t)(sp / 2u);
            if (c0 > len0) c0 = len0;
            slf_puts_partial(x0, 6u, q0, c0, SLF_PAL_TEXT);
            if (sp >= (uint16_t)(len0 * 2u + 8u)) {
                uint8_t c1 = (uint8_t)((sp - (len0 * 2u + 8u)) / 2u);
                if (c1 > len1) c1 = len1;
                slf_puts_partial(x1, 7u, q1, c1, SLF_PAL_TEXT);
            }
        }
        slf_actor_pose(0u, win_def, anim_sprite(win_def, NGF_A_WIN, tick), n > 1u ? 64 : 104, SLF_FLOOR_Y - 125, 0u);
        for (k = 0u; k < n; k++) {
            uint8_t kneel = anim_hold(lose[k], NGF_A_LOSE, tick);
            int16_t x = n > 1u ? k_lose_x[k] : 216;
            /* facing left, toward the winner: the drawn body centred on x */
            slf_actor_pose((uint8_t)(1u + k), lose[k], kneel, (int16_t)(x + slf_fighter_body_offset(lose[k], kneel)),
                           SLF_FLOOR_Y - 125, 1u);
        }
        if (tick > (uint16_t)(voice_len + 40u) && ((slf_pressed(0u) | slf_pressed(1u)) & (NGF_BTN_A | NGF_BTN_B | NGF_BTN_C | NGF_BTN_D)))
            break;
    }
    soundFadeOutSpeed(3u);
    ng_palfx_screen_fade_out(0u, 14u);
    for (tick = 0u; tick < 14u; tick++) slf_frame();
}

static void NEOGEO_USER versus(void)
{
    uint8_t result;
    NEO_REGISTER8(SLF_P2_FLAG) = 0u;
    NEO_REGISTER8(SLF_START_FLAG) = 0u;
    slf_score[0] = slf_score[1] = 0u;
    slf_scene_begin();
    slf_big_center(10u, SLF_BIG_CHALLENGER, SLF_BIG_CHALLENGER_W, SLF_BIG_CHALLENGER_H, SLF_PAL_RED);
    slf_sfx(SLF_SFX_BELL);
    wait_frames(120u);
    choose_format(0u);
    select_roster(3u);
    s_def[0] = s_pick[0];
    s_def[1] = s_pick[1];
    vs_screen(s_def[1]);
    result = run_match(0u, 0u, MODE_VERSUS);
    if (result < 2u) win_scene(result);
    else {
        slf_music(SLF_TRACK_VICTORY);
        big_card(SLF_BIG_DRAW, SLF_BIG_DRAW_W, SLF_BIG_DRAW_H, SLF_PAL_BIG, 0xFFu, 180u);
    }
    NEO_REGISTER8(BIOS_PLAYER2_MODE) = 3u;
}

/* The player's fighter, beaten and swaying, while the clock runs. A
 * continue and they shake it off; time out and they go down. */
static uint8_t NEOGEO_USER continue_screen(uint8_t player)
{
    uint8_t def = s_end_def[player];
    uint16_t timer = 10u * 60u, tick = 0u;
    uint8_t last_sec = 0xFFu, again = 0u;
    slf_scene_begin();
    slf_music(SLF_TRACK_CONTINUE);
    slf_fighter_palettes(def, 0u, def, 0u);
    slf_fighter_bank(0u, 0u);
    slf_fighter_bruise(0u, def, 0u);
    slf_stage_set(slf_stage_current());
    slf_stage_show(96);
    slf_stage_darken(1u);
    slf_puts_tall_center(3u, "CONTINUE ?", SLF_TALL_GOLD);
    while (!again && timer--) {
        uint8_t sec = (uint8_t)(timer / 60u);
        slf_frame();
        slf_fighter_pose(0u, def, anim_sprite(def, NGF_A_DIZZY, tick++), 160, SLF_FLOOR_Y - 125, 0u);
        if (sec != last_sec) {
            /* The digits are one strip: a digit's lower half is a strip's width on. */
            slf_big(19u, 6u, (uint16_t)(SLF_BIG_DIGITS + sec * 2u), 2u, 1u, sec < 4u ? SLF_PAL_RED : SLF_PAL_BIG);
            slf_big(19u, 7u, (uint16_t)(SLF_BIG_DIGITS + SLF_BIG_DIGITS_W + sec * 2u), 2u, 1u,
                    sec < 4u ? SLF_PAL_RED : SLF_PAL_BIG);
            last_sec = sec;
            if (sec < 9u) slf_sfx(SLF_SFX_MENU);
        }
        if (!ng_sys_is_mvs()) {
            slf_puts_center(9u, "PRESS START TO CONTINUE", SLF_PAL_TEXT);
            if (slf_start_pressed(player) || (slf_pressed(player) & (NGF_BTN_A | NGF_BTN_C))) {
                slf_sfx(SLF_SFX_SELECT);
                again = 1u;
            }
        } else {
            int cred = player ? *(volatile uint8_t *)P2_CREDITS : read_p1credit();
            if (cred > 0) {
                slf_puts_center(9u, player ? "PRESS 2P START TO CONTINUE" : "PRESS 1P START TO CONTINUE", SLF_PAL_GOLD);
                if (slf_start_pressed(player) || (slf_pressed(player) & (NGF_BTN_A | NGF_BTN_C))) {
                    uint16_t saved_sr;
                    __asm__ volatile ("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d" (saved_sr) : : "memory");
                    NEO_REGISTER8(0x10FDB0) = (uint8_t)(player == 0 ? 1 : 0);
                    NEO_REGISTER8(0x10FDB1) = (uint8_t)(player == 1 ? 1 : 0);
                    CALLNEOGEOF(SYS_CREDIT_CHECK);
                    if (NEO_REGISTER8(player ? 0x10FDB1 : 0x10FDB0)) {
                        CALLNEOGEOF(SYS_CREDIT_DOWN);
                    }
                    __asm__ volatile ("move.w %0,%%sr" : : "d" (saved_sr) : "memory");
                    slf_sfx(SLF_SFX_SELECT);
                    again = 1u;
                }
            } else {
                slf_puts_center(9u, "   INSERT COIN TO CONTINUE   ", SLF_PAL_TEXT);
            }
        }
    }
    slf_clear_rows(3u, 9u);
    if (again) {
        slf_fighter_palettes(def, 0u, def, 0u);
        slf_stage_darken(0u);
        slf_big_center(4u, SLF_BIG_RISE, SLF_BIG_RISE_W, SLF_BIG_RISE_H, SLF_PAL_BIG);
        slf_sfx(SLF_SFX_POWER);
        grunt(def, SLF_GRUNT_POWER);
    } else {
        grunt(def, SLF_GRUNT_KO);
    }
    for (tick = 0u; tick < 90u; tick++) {
        slf_frame();
        slf_fighter_pose(0u, def, again ? anim_sprite(def, NGF_A_INTRO, tick) : anim_hold(def, NGF_A_DOWN, tick),
                         160, SLF_FLOOR_Y - 125, 0u);
    }
    return again;
}

void NEOGEO_USER slf_run(void)
{
    uint8_t ladder[8];
    uint8_t stage;
    uint8_t human = (NEO_REGISTER8(SLF_START_FLAG) == 2u || NEO_REGISTER8(SLF_P2_FLAG) == 1u) ? 1u : 0u;
    uint8_t cpu = (uint8_t)(human ^ 1u);
    slf_audio_demo(0u);
    slf_score[0] = slf_score[1] = 0u;        /* a new game: a new score */
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
    NEO_REGISTER8(SLF_START_FLAG) = 0u;
    choose_format(human);
    select_roster((uint8_t)(1u << human));
    /* 8-stage tournament: 5 random contenders, 1 rival, 1 sub-boss, 1 final
     * boss, all playable fighters (the bosses are stood in for until theirs are). */
    ladder[5] = (s_pick[human] == 1u) ? 0u : 1u;
    ladder[6] = (uint8_t)(41u + (slf_rand() & 1u));
    ladder[7] = (s_pick[human] == 43u) ? 42u : 43u;
    for (stage = 0u; stage < 8u; stage++) {
        uint8_t opp = ladder[stage], tries, k, seen = 1u;
        if (stage >= 5u && slf_fighter_ready[opp] && opp != s_pick[human]) continue;
        for (tries = 0u; tries < 12u && seen; tries++) {
            opp = ready_fighter(s_pick[human]);
            seen = 0u;
            for (k = 0u; k < stage; k++) if (ladder[k] == opp) seen = 1u;
        }
        ladder[stage] = opp;
    }

    for (stage = 0u; stage < 8u; stage++) {
        uint8_t result;
        s_def[human] = s_pick[human];
        s_def[cpu] = ladder[stage];
        vs_screen(s_def[cpu]);
        result = run_match(human == 0u ? 0u : cpu_level(stage), human == 1u ? 0u : cpu_level(stage), MODE_ARCADE);
        if (result == RESULT_CHALLENGER) {
            versus();
            return;
        }
        if (result < 2u) win_scene(result);
        if (result != human) {
            if (!continue_screen(human)) {
                slf_music(SLF_TRACK_VICTORY);
                big_card(SLF_BIG_GAMEOVER, SLF_BIG_GAMEOVER_W, SLF_BIG_GAMEOVER_H, SLF_PAL_RED, s_def[cpu], 240u);
                return;
            }
            stage--;
            continue;
        }
    }
    slf_music(SLF_TRACK_VICTORY);
    big_card(SLF_BIG_CONGRATS, SLF_BIG_CONGRATS_W, SLF_BIG_CONGRATS_H, SLF_PAL_BIG, s_pick[human], 240u);
    {
        uint16_t end_t;
        slf_scene_begin();
        slf_story_show(SLF_CARD_ENDING);
        slf_fighter_palettes(s_pick[human], 0u, s_pick[human], 0u);
        slf_fighter_bank(0u, 0u);
        slf_puts_center(4u, "PROJECT CHIMERA TERMINATED", SLF_PAL_GOLD);
        slf_puts_center(7u, "THE VEIL CORE HAS CRUMBLED.", SLF_PAL_WHITE);
        slf_puts_center(10u, "NEW SHADOW LEAGUE CHAMPION:", SLF_PAL_BLUE);
        slf_puts_center(13u, slf_fighter_defs[s_pick[human]]->name, SLF_PAL_BIG);
        slf_puts_center(16u, slf_fighter_origin[s_pick[human]], SLF_PAL_GOLD);
        slf_puts_center(19u, "THANK YOU FOR PLAYING!", SLF_PAL_WHITE);
        for (end_t = 0u; end_t < 360u; end_t++) {
            slf_frame();
            slf_fighter_pose(0u, s_pick[human], anim_sprite(s_pick[human], NGF_A_WIN, end_t), 160, SLF_FLOOR_Y - 125, 0u);
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
                slf_puts(3u, 0u, (t & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PUSH START ") : "           ", SLF_PAL_TEXT);
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
    slf_puts(4u, 10u, "EVADE ROLL     : (A) + (B)", SLF_PAL_BLUE);
    slf_puts(4u, 12u, "BLOWBACK       : (C) + (D)", SLF_PAL_RED);
    slf_puts(4u, 14u, "MAX POWER MODE : (B) + (C)", SLF_PAL_RED);
    slf_puts(4u, 16u, "SPECIAL MOVES  : D,DF,F + BTN", SLF_PAL_GOLD);
    slf_puts(4u, 18u, "SUPER MOVES    : D,DF,F X2 + BTN", SLF_PAL_GOLD);
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
    slf_audio_demo(0u);
}
