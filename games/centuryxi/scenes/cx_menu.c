/*
 * cx_menu.c -- Century XI: the front of the game.
 *
 * The title, the mode list, the nation picker, the Road's tables and bracket,
 * the options, and the trophy. Modes share one match: only the frame around it
 * changes. On an arcade board a credit buys one game; a second player buys his
 * own with START.
 */
#include "cx_render.h"
#include "cx_tournament.h"

static uint8_t s_team, s_foe;
static CxTournament s_tournament;
static CxCup s_cup;

static const char *const k_stadium[8] = { "BIG RAMEN BOWL", "MOCHI MOCHI DOME", "TAKOYAKI ARENA", "LUCKY CAT STADIUM",
                                          "KAWAII KICK DOME", "ONIGIRI OVAL", "BANZAI FOOTBALL PARK", "SUPER SUSHI STADIUM" };
static const char *const k_city[8] = { "TOKYO-YO", "YOKOHAMHAM", "OSAKA-KA", "KYOTOTO", "SAPPOROLL", "KOBEBE", "NAGOYAY", "FUKUOKAKA" };
static const char *const k_modes[7] = { "ROAD TO 2030", "QUICK CUP", "EXHIBITION", "TWO-PLAYER VERSUS", "PENALTY CHALLENGE",
                                        "TRAINING GROUND", "OPTIONS" };
enum { M_ROAD, M_CUP, M_EXHIBITION, M_VERSUS, M_PENS, M_TRAINING, M_OPTIONS, M_COUNT };

/* May a game begin? On MVS only once START has paid (the BIOS then sets game
 * mode); on AES at once. */
static uint8_t NEOGEO_USER may_play(void)
{
    return (uint8_t)(!ng_sys_is_mvs() || NEO_REGISTER8(BIOS_USER_MODE) == 2u);
}

void NEOGEO_USER cx_attract_init(void)
{
    cx_init_video();
    setBACKDROP(BLACK);
    cx_picture_show(CX_ART_BG_TITLE);
    cx_spr_show(&cx_logo, SPR_LOGO, CX_ART_LOGO, 0, 32, 4, 0);
    cx_puts_center(27, "(C) 2026 EAGLE SOFTWARE", CX_PAL_GOLD);
    cx_man_palette(BANK_MEN, 26, 0, 0, 0, 0);
    cx_man_palette(BANK_MEN + 1u, 0, 0, 3, 1, 0);
    cx_man_palette(BANK_MEN + 2u, 9, 0, 0, 4, 0);
    cx_music_title();
}

void NEOGEO_USER cx_attract_step(uint16_t tick)
{
    uint8_t run = (uint8_t)(2u * CX_FIG_FRAMES + 1u + ((tick >> 2) & 3u));
    int16_t x = (int16_t)(tick & 511u);
    cx_figure_show(&cx_men[0], SPR_MEN, CX_ART_FIG0, run, (int16_t)(x - 60), 204, 0, BANK_MEN);
    cx_figure_show(&cx_men[1], SPR_MEN + 4u, CX_ART_FIG1, run, (int16_t)(x - 110), 212, 0, (uint8_t)(BANK_MEN + 1u));
    cx_figure_show(&cx_men[2], SPR_MEN + 8u, CX_ART_FIG2, run, (int16_t)(x - 160), 204, 0, (uint8_t)(BANK_MEN + 2u));
    cx_spr_show(&cx_ball_s, SPR_BALL, CX_ART_BALL, (uint8_t)((tick >> 2) & 3u), (int16_t)(x - 30), 192, 0);
#ifdef NG_AES
    if ((tick >> 4) & 1u) cx_puts_center(24, ">> PUSH START <<", CX_PAL_GOLD);
    else cx_clear_row(24);
#else
    if (read_p1credit() > 0) {
        if ((tick >> 4) & 1u) cx_puts_center(24, ">> PUSH START <<", CX_PAL_GOLD);
        else cx_clear_row(24);
    } else if ((tick >> 4) & 1u) {
        cx_puts_center(24, "INSERT COIN", CX_PAL_RED);
    } else {
        cx_clear_row(24);
    }
#endif
}

static void NEOGEO_USER stat_bar(uint8_t x, uint8_t y, const char *label, uint8_t val, uint8_t pal)
{
    char bar[17];
    uint8_t i;
    for (i = 0; i < 16u; i++) bar[i] = (i < val) ? '=' : '.';
    bar[16] = 0;
    ng_fix_puts(x, y, label, CX_PAL_WHITE);
    ng_fix_puts((uint8_t)(x + 8u), y, bar, pal);
}

static void NEOGEO_USER nation_card(uint8_t team, const char *title)
{
    char name[14];
    const uint8_t star = 9;              /* the side's forward, on show */
    uint8_t look_fig, skin, hair;
    CxWorld *dummy = 0;
    (void)dummy;
    cx_screen_clear();
    cx_picture_show(CX_ART_BG_TITLE);
    cx_puts_center(3, title, CX_PAL_GOLD);
    cx_squad_name(team, name);
    cx_puts_center(6, name, CX_PAL_WHITE);
    cx_flag_show(0, team, 144, 56);
    stat_bar(9, 13, "SPEED", cx_squad_stat(team, 0), CX_PAL_CYAN);
    stat_bar(9, 14, "POWER", cx_squad_stat(team, 1), CX_PAL_RED);
    stat_bar(9, 15, "PRESS", cx_squad_stat(team, 2), CX_PAL_GOLD);
    stat_bar(9, 16, "SKILL", cx_squad_stat(team, 3), CX_PAL_GREEN);
    stat_bar(9, 17, "DEFEND", cx_squad_stat(team, 4), CX_PAL_WHITE);
    look_fig = cx_man_look(team, star, &skin, &hair);
    cx_man_palette(BANK_MEN, team, 0, skin, hair, 0);
    cx_figure_show(&cx_men[0], SPR_MEN, (uint16_t)(CX_ART_FIG0 + look_fig), (uint8_t)(2u * CX_FIG_FRAMES + CX_F_STAND_F), 44, 180, 0, BANK_MEN);
}

/* Picks a side: answers the nation, or 0xFF to go back. */
static uint8_t NEOGEO_USER pick_nation(uint8_t start, const char *title, uint8_t player2)
{
    uint8_t team = start, dirty = 1;
    uint16_t pressed, p2;
    for (;;) {
        (void)cx_input(&pressed);
        (void)cx_input2(&p2);
        if (player2) pressed = p2;
        if (pressed & CX_LEFT) { team = team ? (uint8_t)(team - 1u) : (uint8_t)(CX_SQUADS - 1u); cx_sfx_menu(); dirty = 1; }
        if (pressed & CX_RIGHT) { team = (uint8_t)(team + 1u); if (team >= CX_SQUADS) team = 0; cx_sfx_menu(); dirty = 1; }
        if (pressed & CX_UP) { team = (uint8_t)(team >= 8u ? team - 8u : team + CX_SQUADS - 8u); cx_sfx_menu(); dirty = 1; }
        if (pressed & CX_DOWN) { team = (uint8_t)(team + 8u); if (team >= CX_SQUADS) team = (uint8_t)(team - CX_SQUADS); cx_sfx_menu(); dirty = 1; }
        if (dirty) {
            dirty = 0;
            nation_card(team, title);
            cx_puts_center(25, "LEFT RIGHT CHOOSE  A CONFIRM  B BACK", CX_PAL_WHITE);
        }
        if (pressed & CX_A) { cx_sfx_menu(); return team; }
        if (pressed & CX_B) return 0xFFu;
    }
}

/* The ground for a friendly. */
static uint8_t NEOGEO_USER pick_ground(void)
{
    static uint8_t ground = 0;
    uint8_t dirty = 1;
    uint16_t pressed;
    cx_screen_clear();
    cx_picture_show(CX_ART_BG_TITLE);
    cx_puts_center(6, "CHOOSE THE GROUND", CX_PAL_GOLD);
    cx_puts_center(25, "LEFT RIGHT CHOOSE  A KICK OFF", CX_PAL_WHITE);
    for (;;) {
        (void)cx_input(&pressed);
        if (pressed & CX_LEFT) { ground = (uint8_t)((ground + 7u) & 7u); cx_sfx_menu(); dirty = 1; }
        if (pressed & CX_RIGHT) { ground = (uint8_t)((ground + 1u) & 7u); cx_sfx_menu(); dirty = 1; }
        if (dirty) {
            dirty = 0;
            cx_clear_row(12);
            cx_clear_row(14);
            cx_puts_center(12, k_stadium[ground], CX_PAL_WHITE);
            cx_puts_center(14, k_city[ground], CX_PAL_GREEN);
        }
        if (pressed & CX_A) { cx_sfx_menu(); return ground; }
    }
}

/* Picks a mode; answers the mode, or 0xFF if the game should end. */
static uint8_t NEOGEO_USER pick_mode(void)
{
    static uint8_t pick = 0;
    uint8_t dirty = 1, i;
    uint16_t pressed;
    cx_screen_clear();
    cx_picture_show(CX_ART_BG_TITLE);
    cx_spr_show(&cx_logo, SPR_LOGO, CX_ART_LOGO, 0, 32, 0, 0);
    for (;;) {
        (void)cx_input(&pressed);
        if (pressed & CX_UP) { pick = pick ? (uint8_t)(pick - 1u) : (uint8_t)(M_COUNT - 1u); cx_sfx_menu(); dirty = 1; }
        if (pressed & CX_DOWN) { pick = (uint8_t)(pick + 1u); if (pick >= M_COUNT) pick = 0; cx_sfx_menu(); dirty = 1; }
        if (dirty) {
            dirty = 0;
            for (i = 0; i < M_COUNT; i++) {
                cx_clear_row((uint8_t)(13u + i * 2u));
                cx_puts_center((uint8_t)(13u + i * 2u), k_modes[i], i == pick ? CX_PAL_GOLD : CX_PAL_WHITE);
            }
        }
        if ((pressed & (CX_A | CX_C)) && may_play()) { cx_sfx_menu(); return pick; }
    }
}

/* Versus needs the second player: on an arcade board he pays with his own START,
 * on a console he only has to press it. Answers 0 if nobody came. */
static uint8_t NEOGEO_USER second_player(void)
{
    uint16_t f, pressed, p2;
    cx_screen_clear();
    cx_picture_show(CX_ART_BG_TITLE);
    cx_puts_center(10, "PLAYER 2", CX_PAL_CYAN);
    for (f = 0; f < 900u; f++) {
        uint16_t joy = cx_input(&pressed);
        (void)cx_input2(&p2);
        if ((f >> 4) & 1u) cx_puts_center(13, ng_sys_is_mvs() && !cx_player2_can_pay() ? "INSERT COIN" : "PRESS START", CX_PAL_GOLD);
        else cx_clear_row(13);
        if (pressed & CX_B) return 0;
        if ((joy & START2) || (p2 & CX_A)) {
            if (ng_sys_is_mvs() && !cx_pay_player2()) continue;   /* his own credit, however the system counts it */
            cx_sfx_menu();
            return 1;
        }
    }
    return 0;
}

/* The options as a new machine has them. */
static void NEOGEO_USER options_default(void)
{
    cx_options.classic = 1;
    cx_options.rules = CX_RULES_MODERN;
    cx_options.var = CX_VAR_CHALLENGE;
    cx_options.level = 3;
    cx_options.levels8 = CX_LEVELS8;
    cx_options.length = 1;                      /* five minutes */
    cx_options.extra = CX_ET_NORMAL;
}

static void NEOGEO_USER options_screen(void)
{
    static const char *const k_rules[2] = { "CLASSIC 1994", "MODERN" };
    static const char *const k_var[4] = { "OFF", "AUTOMATIC", "AUTOMATIC FULL", "ASK FOR IT" };
    static const char *const k_level[8] = { "1 SUNDAY LEAGUE", "2 AMATEUR", "3 SEMI-PRO", "4 PROFESSIONAL",
                                            "5 INTERNATIONAL", "6 WORLD CLASS", "7 LEGEND", "8 IMPOSSIBLE" };
    static const char *const k_len[5] = { "3 MINUTES", "5 MINUTES", "7 MINUTES", "10 MINUTES", "14 MINUTES" };
    static const char *const k_extra[3] = { "EXTRA TIME", "GOLDEN GOAL", "PENALTIES ONLY" };
    uint8_t row = 0, dirty = 1, erase = 0;
    uint16_t pressed;
    cx_screen_clear();
    cx_picture_show(CX_ART_BG_TITLE);
    cx_puts_center(4, "OPTIONS", CX_PAL_GOLD);
    for (;;) {
        (void)cx_input(&pressed);
        if (pressed & CX_UP) { row = row ? (uint8_t)(row - 1u) : 6u; erase = 0; cx_sfx_menu(); dirty = 1; }
        if (pressed & CX_DOWN) { row = (uint8_t)(row + 1u); if (row > 6u) row = 0; erase = 0; cx_sfx_menu(); dirty = 1; }
        if (row == 6u && (pressed & CX_A)) {
            /* the last row: A, then A again, wipes everything kept and starts afresh */
            if (!erase) { erase = 1; }
            else {
                cx_save_erase();
                options_default();
                if (ng_sys_is_mvs()) {
                    cx_options.level = (uint8_t)(ng_dip_option(0) & 7u);
                    if (ng_dip_option(1) < 5u) cx_options.length = ng_dip_option(1);
                }
                erase = 2;
            }
            cx_sfx_menu();
            dirty = 1;
            pressed = (uint16_t)(pressed & ~CX_A);
        }
        if (pressed & (CX_LEFT | CX_RIGHT)) {
            if (row == 0u) cx_options.classic ^= 1u;
            else if (row == 1u) cx_options.rules ^= 1u;
            else if (row == 2u) cx_options.var = (uint8_t)((cx_options.var + 1u) & 3u);
            else if (row == 3u) cx_options.level = (uint8_t)((cx_options.level + ((pressed & CX_LEFT) ? 7u : 1u)) & 7u);
            else if (row == 4u) cx_options.length = (uint8_t)((cx_options.length + 1u) % 5u);
            else if (row == 5u) cx_options.extra = (uint8_t)((cx_options.extra + 1u) % 3u);
            cx_sfx_menu();
            dirty = 1;
        }
        if (dirty) {
            dirty = 0;
            cx_clear_row(9); cx_clear_row(11); cx_clear_row(13); cx_clear_row(15); cx_clear_row(17); cx_clear_row(19);
            ng_fix_puts(5, 9, "CONTROLS", row == 0u ? CX_PAL_GOLD : CX_PAL_WHITE);
            ng_fix_puts(20, 9, cx_options.classic ? "CLASSIC" : "MODERN", CX_PAL_CYAN);
            ng_fix_puts(5, 11, "REFEREE", row == 1u ? CX_PAL_GOLD : CX_PAL_WHITE);
            ng_fix_puts(20, 11, k_rules[cx_options.rules & 1u], CX_PAL_CYAN);
            ng_fix_puts(5, 13, "VIDEO REVIEW", row == 2u ? CX_PAL_GOLD : CX_PAL_WHITE);
            ng_fix_puts(20, 13, k_var[cx_options.var], CX_PAL_CYAN);
            ng_fix_puts(5, 15, "CPU LEVEL", row == 3u ? CX_PAL_GOLD : CX_PAL_WHITE);
            ng_fix_puts(20, 15, k_level[cx_options.level & 7u], CX_PAL_CYAN);
            ng_fix_puts(5, 17, "MATCH LENGTH", row == 4u ? CX_PAL_GOLD : CX_PAL_WHITE);
            ng_fix_puts(20, 17, k_len[cx_options.length], CX_PAL_CYAN);
            ng_fix_puts(5, 19, "A LEVEL CUP TIE", row == 5u ? CX_PAL_GOLD : CX_PAL_WHITE);
            ng_fix_puts(21, 19, k_extra[cx_options.extra], CX_PAL_CYAN);
            cx_clear_row(21);
            ng_fix_puts(5, 21, erase == 1u ? "PRESS A AGAIN TO ERASE ALL" : (erase == 2u ? "SAVED DATA ERASED" : "ERASE SAVED DATA"),
                        row == 6u ? (erase == 1u ? CX_PAL_RED : CX_PAL_GOLD) : CX_PAL_WHITE);
            cx_puts_center(24, "LEFT RIGHT CHANGE  A DONE", CX_PAL_WHITE);
        }
        if (pressed & (CX_A | CX_B | START1)) { cx_save_write(); return; }
    }
}

static void NEOGEO_USER result_screen(uint8_t team, uint8_t foe, uint8_t won)
{
    char a[14], b[14];
    cx_screen_clear();
    cx_picture_show(CX_ART_BG_TITLE);
    cx_squad_name(team, a);
    cx_squad_name(foe, b);
    cx_spr_show(&cx_banner, SPR_BANNER, CX_ART_BANNER, BANNER_FULL, 80, 24, 0);
    cx_puts_center(11, a, CX_PAL_GOLD);
    cx_put_2d(17, 13, cx_result[0], CX_PAL_WHITE);
    ng_fix_puts(19, 13, "-", CX_PAL_WHITE);
    cx_put_2d(21, 13, cx_result[1], CX_PAL_WHITE);
    cx_puts_center(15, b, CX_PAL_CYAN);
    cx_puts_center(19, won == 3u ? "A DRAW" : (won ? "VICTORY!" : "DEFEAT"), won == 3u ? CX_PAL_WHITE : (won ? CX_PAL_GREEN : CX_PAL_RED));
    (void)cx_any_press(420);
}

static void NEOGEO_USER scorers(uint8_t y, uint8_t n);

static void NEOGEO_USER group_table(void)
{
    uint8_t g, row, order[4];
    char name[14], title[] = "GROUP A";
    for (g = 0; g < 32u; g++) if (s_tournament.groups[g] == s_tournament.human) break;
    g = (uint8_t)(g / 4u);
    title[6] = (char)('A' + g);
    cx_tournament_ranking(&s_tournament, g, order);
    cx_screen_clear();
    cx_picture_show(CX_ART_BG_TITLE);
    cx_puts_center(4, title, CX_PAL_GOLD);
    ng_fix_puts(5, 7, "NATION        P  W  D  L  GD PTS", CX_PAL_WHITE);
    for (row = 0; row < 4u; row++) {
        const CxStanding *st = &s_tournament.table[order[row]];
        uint8_t y = (uint8_t)(10u + row * 2u);
        int16_t gd = (int16_t)(st->scored - st->conceded);
        cx_squad_name(order[row], name);
        ng_fix_puts(5, y, name, order[row] == s_tournament.human ? CX_PAL_GOLD : CX_PAL_WHITE);
        cx_put_2d(18, y, st->played, CX_PAL_WHITE);
        cx_put_2d(21, y, st->wins, CX_PAL_WHITE);
        cx_put_2d(24, y, st->draws, CX_PAL_WHITE);
        cx_put_2d(27, y, st->losses, CX_PAL_WHITE);
        if (gd < 0) { ng_fix_puts(29, y, "-", CX_PAL_RED); gd = (int16_t)-gd; }
        cx_put_2d(30, y, (uint16_t)gd, CX_PAL_WHITE);
        cx_put_2d(33, y, st->points, CX_PAL_CYAN);
    }
    scorers(18, 3);
    cx_puts_center(26, "A CONTINUE", CX_PAL_WHITE);
    (void)cx_any_press(900);
}


/* The draw: the thirty-two who qualified from the forty-eight, into eight groups, pot by
 * pot as a draw is made, a nation at a time. */
static void NEOGEO_USER draw_screen(void)
{
    uint8_t g, k;
    char code[4], title[] = "GROUP A";
    cx_screen_clear();
    cx_picture_show(CX_ART_BG_TITLE);
    cx_puts_center(1, "THE DRAW", CX_PAL_GOLD);
    cx_puts_center(3, "32 OF 48 NATIONS QUALIFIED", CX_PAL_WHITE);
    for (g = 0; g < 8u; g++) {
        title[6] = (char)('A' + g);
        ng_fix_puts((uint8_t)(g < 4u ? 3u : 22u), (uint8_t)(6u + (g & 3u) * 5u), title, CX_PAL_GOLD);
    }
    cx_wait(30);
    for (k = 0; k < 4u; k++) {
        for (g = 0; g < 8u; g++) {
            uint8_t team = s_tournament.groups[g * 4u + k];
            cx_squad_code(team, code);
            ng_fix_puts((uint8_t)((g < 4u ? 3u : 22u) + k * 4u), (uint8_t)(8u + (g & 3u) * 5u), code,
                        team == s_tournament.human ? CX_PAL_GOLD : CX_PAL_WHITE);
            cx_sfx_menu();
            cx_wait(team == s_tournament.human ? 40u : 7u);
        }
    }
    cx_puts_center(26, "A CONTINUE", CX_PAL_WHITE);
    (void)cx_any_press(1200);
}

/* One knock-out tie: "BAN 2-1 MTG" when played, "BAN v MTG" before, dots before it is set. */
static void NEOGEO_USER tie_line(uint8_t x, uint8_t y, uint8_t n)
{
    char a[4] = "...", b[4] = "...", sc[4] = " v ";
    const CxFixture *f = &s_tournament.fixture[n];
    uint8_t pal = CX_PAL_WHITE;
    if (n < s_tournament.generated) {
        cx_squad_code(f->home, a);
        cx_squad_code(f->away, b);
        if (f->home == s_tournament.human || f->away == s_tournament.human) pal = CX_PAL_GOLD;
        if (f->played) {
            sc[0] = (char)('0' + (f->goals[0] > 9u ? 9u : f->goals[0]));
            sc[1] = '-';
            sc[2] = (char)('0' + (f->goals[1] > 9u ? 9u : f->goals[1]));
        }
    }
    ng_fix_puts(x, y, a, pal);
    ng_fix_puts((uint8_t)(x + 4u), y, sc, pal);
    ng_fix_puts((uint8_t)(x + 8u), y, b, pal);
}

/* The knock-out as it stands: the last sixteen, the quarters, the semis, the final. */
static void NEOGEO_USER bracket(void)
{
    uint8_t i;
    cx_screen_clear();
    cx_picture_show(CX_ART_BG_TITLE);
    cx_puts_center(2, "THE KNOCK-OUT", CX_PAL_GOLD);
    ng_fix_puts(2, 4, "LAST 16", CX_PAL_CYAN);
    ng_fix_puts(15, 4, "QUARTERS", CX_PAL_CYAN);
    ng_fix_puts(28, 4, "SEMIS", CX_PAL_CYAN);
    for (i = 0; i < 8u; i++) tie_line(1, (uint8_t)(6u + i * 2u), (uint8_t)(48u + i));
    for (i = 0; i < 4u; i++) tie_line(14, (uint8_t)(7u + i * 4u), (uint8_t)(56u + i));
    for (i = 0; i < 2u; i++) tie_line(27, (uint8_t)(9u + i * 8u), (uint8_t)(60u + i));
    ng_fix_puts(2, 23, "FINAL", CX_PAL_GOLD);
    tie_line(8, 23, 63);
    ng_fix_puts(22, 23, "3RD", CX_PAL_WHITE);
    tie_line(26, 23, 62);
    cx_puts_center(26, "A CONTINUE", CX_PAL_WHITE);
    (void)cx_any_press(900);
}

/* The championship's leading scorers, from row y: rank, name, nation, goals. */
static void NEOGEO_USER scorers(uint8_t y, uint8_t n)
{
    CxScorer top[5];
    char name[12], code[4];
    uint8_t k, got = cx_tournament_top(&s_tournament, top, (uint8_t)(n > 5u ? 5u : n));
    if (!got) return;
    ng_fix_puts(5, y, "TOP SCORERS", CX_PAL_GOLD);
    for (k = 0; k < got; k++) {
        uint8_t row = (uint8_t)(y + 2u + k * 2u), pal = top[k].team == s_tournament.human ? CX_PAL_GOLD : CX_PAL_WHITE;
        cx_man_name(top[k].team, top[k].squad, name);
        cx_squad_code(top[k].team, code);
        ng_fix_puts(5, row, name, pal);
        ng_fix_puts(17, row, code, pal);
        cx_put_2d(22, row, top[k].goals, CX_PAL_CYAN);
    }
}

/* Before kick-off: both sides' eleven, with numbers, and who the player's side is without. */
static void NEOGEO_USER lineups(uint8_t us, uint8_t them, uint16_t absent)
{
    uint8_t k, ours[11], theirs[11];
    char name[12], code[4];
    cx_lineup(absent, ours);
    cx_lineup(0, theirs);
    cx_screen_clear();
    cx_picture_show(CX_ART_BG_TITLE);
    cx_puts_center(2, "THE TEAMS", CX_PAL_GOLD);
    cx_squad_code(us, code);
    ng_fix_puts(6, 4, code, CX_PAL_GOLD);
    cx_squad_code(them, code);
    ng_fix_puts(26, 4, code, CX_PAL_CYAN);
    for (k = 0; k < 11u; k++) {
        uint8_t y = (uint8_t)(6u + k * 2u);
        cx_put_2d(2, y, (uint16_t)(ours[k] + 1u), CX_PAL_WHITE);
        cx_man_name(us, ours[k], name);
        ng_fix_puts(5, y, name, ours[k] != k ? CX_PAL_GREEN : CX_PAL_WHITE);
        cx_put_2d(22, y, (uint16_t)(theirs[k] + 1u), CX_PAL_WHITE);
        cx_man_name(them, theirs[k], name);
        ng_fix_puts(25, y, name, CX_PAL_WHITE);
    }
    for (k = 0; k < 16u; k++) {
        if (absent & (1u << k)) {
            char line[40] = "SUSPENDED: ";
            cx_man_name(us, k, line + 11);
            cx_puts_center(3, line, CX_PAL_RED);
            break;
        }
    }
    cx_puts_center(27, "A KICK OFF", CX_PAL_WHITE);
    (void)cx_any_press(900);
}

/* After a championship match: bookings carried, suspensions served and earned. */
static void NEOGEO_USER discipline(void)
{
    uint8_t sq, cards[NGS_CUP_SQUAD];
    for (sq = 0; sq < NGS_CUP_SQUAD; sq++) cards[sq] = cx_match_card(sq);
    ngs_cup_discipline(&s_tournament, cards);
}

static void NEOGEO_USER fixture_card(uint8_t next, uint8_t us, uint8_t them, uint8_t stadium)
{
    static const char *const k_round[6] = { "GROUP MATCH", "ROUND OF 16", "QUARTER-FINAL", "SEMI-FINAL", "THIRD PLACE", "THE FINAL" };
    char a[14], b[14];
    uint8_t stage = ngs_cup_round(next);
    cx_screen_clear();
    cx_picture_show(CX_ART_BG_TITLE);
    cx_puts_center(4, k_round[stage], CX_PAL_GOLD);
    cx_squad_name(us, a);
    cx_squad_name(them, b);
    cx_flag_show(0, us, 64, 80);
    cx_flag_show(1, them, 224, 80);
    ng_fix_puts(4, 16, a, CX_PAL_GOLD);
    ng_fix_puts(24, 16, b, CX_PAL_CYAN);
    cx_puts_center(14, "VS", CX_PAL_WHITE);
    cx_puts_center(20, k_stadium[stadium & 7u], CX_PAL_WHITE);
    cx_puts_center(21, k_city[stadium & 7u], CX_PAL_GREEN);
    cx_puts_center(25, "A KICK OFF", CX_PAL_WHITE);
    (void)cx_any_press(600);
}

static void NEOGEO_USER trophy(uint8_t team, uint8_t won, uint8_t road)
{
    char name[14];
    uint8_t skin, hair, fig = cx_man_look(team, 9, &skin, &hair);
    cx_screen_clear();
    cx_picture_show(CX_ART_BG_TITLE);
    cx_squad_name(team, name);
    cx_man_palette(BANK_MEN, team, 0, skin, hair, 0);
    if (won) {
        /* five of them round the cup, each his own way, and the golden boot named */
        static const uint8_t k_who[5] = { 9, 10, 8, 2, 0 };
        static const int16_t k_x[5] = { 56, 104, 216, 264, 160 };
        static const uint8_t k_moves[5][2] = { { CX_F_CELEB_JUMP, CX_F_CELEB_RUN }, { CX_F_SAMBA1, CX_F_SAMBA2 },
                                               { CX_F_SCREAM, CX_F_CELEB_JUMP }, { CX_F_HIP1, CX_F_HIP2 }, { CX_F_SKY, CX_F_CELEB_JUMP } };
        uint8_t k, figs[5];
        uint16_t f, pressed;
        cx_music_champion();
        cx_spr_show(&cx_banner, SPR_BANNER, CX_ART_BANNER, BANNER_CHAMP, 80, 24, 0);
        cx_spr_show(&cx_trophy, SPR_TROPHY, CX_ART_TROPHY, 0, 136, 70, 0);
        for (k = 0; k < 5u; k++) {
            figs[k] = cx_man_look(team, k_who[k], &skin, &hair);
            cx_man_palette((uint8_t)(BANK_MEN + k), team, 0, skin, hair, (uint8_t)(k_who[k] == 0u));
        }
        cx_puts_center(20, "CHAMPIONS OF THE ROAD TO 2030", CX_PAL_GOLD);
        cx_puts_center(22, name, CX_PAL_CYAN);
        if (road) {
            CxScorer top[1];
            if (cx_tournament_top(&s_tournament, top, 1)) {
                char line[40] = "GOLDEN BOOT: ", who[12], code[4];
                uint8_t n = 13, j;
                cx_man_name(top[0].team, top[0].squad, who);
                cx_squad_code(top[0].team, code);
                for (j = 0; who[j]; j++) line[n++] = who[j];
                line[n++] = ' '; line[n++] = code[0]; line[n++] = code[1]; line[n++] = code[2];
                line[n] = 0;
                cx_puts_center(24, line, CX_PAL_WHITE);
            }
        }
        cx_puts_center(26, "EAGLE SOFTWARE", CX_PAL_GOLD);
        for (f = 0; f < 900u; f++) {
            (void)cx_input(&pressed);
            for (k = 0; k < 5u; k++) {
                uint8_t beat = (uint8_t)(((f + k * 7u) >> 3) & 1u), keeper = (uint8_t)(k_who[k] == 0u);
                uint16_t art = keeper ? CX_ART_KEEPER : (uint16_t)(CX_ART_FIG0 + figs[k]);
                uint8_t frame = keeper ? (uint8_t)(2u * CX_KEEP_FRAMES + (beat ? CX_K_GK_HOP : CX_K_GK_GRIN))
                                       : (uint8_t)(2u * CX_FIG_FRAMES + k_moves[k][beat]);
                cx_figure_show(&cx_men[k], (uint16_t)(SPR_MEN + k * 5u), art, frame, k_x[k],
                               (int16_t)(k == 4u ? 196 : 180 - (beat && !keeper ? 4 : 0)), (uint8_t)(k_x[k] > 160), (uint8_t)(BANK_MEN + k));
            }
            if (f > 120u && (pressed & (CX_A | CX_B | CX_C | START1))) break;
        }
        for (k = 0; k < 5u; k++) cx_spr_hide(&cx_men[k]);
    } else {
        cx_music_final();
        cx_figure_show(&cx_men[0], SPR_MEN, (uint16_t)(CX_ART_FIG0 + fig), (uint8_t)(2u * CX_FIG_FRAMES + CX_F_KNEEL), 160, 160, 0, BANK_MEN);
        cx_puts_center(20, "THE ROAD ENDS HERE", CX_PAL_RED);
        cx_puts_center(22, name, CX_PAL_WHITE);
        cx_puts_center(26, "EAGLE SOFTWARE", CX_PAL_GOLD);
        (void)cx_any_press(600);
    }
}

/* A Road in progress: go back to it, or start again? 1 continue, 0 new. */
static uint8_t NEOGEO_USER resume_road(void)
{
    uint16_t pressed;
    cx_screen_clear();
    cx_picture_show(CX_ART_BG_TITLE);
    cx_puts_center(10, "A ROAD TO 2030 IS IN PROGRESS", CX_PAL_GOLD);
    cx_puts_center(14, "A CONTINUE", CX_PAL_WHITE);
    cx_puts_center(16, "B START AGAIN", CX_PAL_WHITE);
    for (;;) {
        (void)cx_input(&pressed);
        if (pressed & CX_A) return 1;
        if (pressed & CX_B) return 0;
    }
}

/* Matches the player's side has already played on this road: the CPU sharpens as it goes. */
static uint8_t NEOGEO_USER road_played(uint8_t team)
{
    uint8_t k, n = 0;
    for (k = 0; k < 64u; k++) {
        const CxFixture *f = &s_tournament.fixture[k];
        if (f->played && (f->home == team || f->away == team)) n++;
    }
    return n;
}

static void NEOGEO_USER road(uint8_t team, uint16_t seed, uint8_t resume)
{
    int16_t next;
    if (resume) {
        cx_road_fetch(&s_tournament);
        team = s_tournament.human;
    } else {
        cx_tournament_init(&s_tournament, team, 2030u + seed);
        cx_road_store(&s_tournament);
        draw_screen();
    }
    while ((next = cx_tournament_next(&s_tournament)) >= 0) {
        CxFixture *f = &s_tournament.fixture[next];
        uint8_t away = (uint8_t)(f->away == team);
        uint8_t them = away ? f->home : f->away;
        uint8_t stadium = (uint8_t)(next == 63 ? 0u : (uint8_t)(next & 7));
        uint8_t stage = (uint8_t)(next < 48 ? 1u : next == 63 ? 3u : 2u);
        uint8_t result, winner;
        if (next < 48) group_table();
        else bracket();
        fixture_card((uint8_t)next, team, them, stadium);
        lineups(team, them, s_tournament.suspended);
        cx_match_round = road_played(team);
        cx_absent[0] = s_tournament.suspended;          /* the player's side is side 0 */
        result = cx_match(team, them, stadium, stage, 1);
        cx_absent[0] = 0;
        if (result == 2u) return;
        result_screen(team, them, result);
        {
            uint8_t k, tm, sq;
            for (k = 0; cx_match_goal(k, &tm, &sq); k++) cx_tournament_scorer(&s_tournament, tm, sq);
        }
        discipline();
        winner = result == 3u ? CX_NO_TEAM : (result ? team : them);
        (void)cx_tournament_result(&s_tournament, (uint8_t)next, cx_result[away ? 1 : 0], cx_result[away ? 0 : 1], winner);
        cx_road_store(&s_tournament);
    }
    cx_road_finish(team, (uint8_t)(s_tournament.champion == team));
    bracket();
    trophy(team, (uint8_t)(s_tournament.champion == team), 1);
}

/* Three knock-out rounds, one credit: the quick cup. */
static void NEOGEO_USER quick_cup(uint8_t team)
{
    cx_cup_init(&s_cup, team);
    while (s_cup.round < 3u) {
        uint8_t them = s_cup.foe, stadium = (uint8_t)(s_cup.round * 3u), r;
        fixture_card((uint8_t)(56u + s_cup.round * 3u), team, them, stadium);
        cx_match_round = s_cup.round;
        r = cx_match(team, them, stadium, (uint8_t)(s_cup.round == 2u ? 3u : 2u), 1);
        if (r == 2u) return;
        result_screen(team, them, r);
        cx_cup_apply(&s_cup, (uint8_t)(r == 1u));
    }
    trophy(team, s_cup.won, 0);
}

void NEOGEO_USER cx_run(void)
{
    uint16_t seed = 0;
    uint8_t mode, picked;
    cx_init_video();
    setBACKDROP(BLACK);
    s_team = 26;                                /* Japan hosts: the first side offered */
    options_default();
    cx_save_load();
    /* on an arcade board the operator's settings start each game */
    if (ng_sys_is_mvs()) {
        cx_options.level = (uint8_t)(ng_dip_option(0) & 7u);
        if (ng_dip_option(1) < 5u) cx_options.length = ng_dip_option(1);
    }
    cx_music_title();
    for (;;) {
        mode = pick_mode();
        if (mode != M_OPTIONS) cx_music_menu();
        seed = (uint16_t)(seed + NEO_REGISTER8(BIOS_P1CURRENT) + 1u);
        if (mode == M_OPTIONS) { options_screen(); continue; }
        if (mode == M_ROAD && cx_road_saved() && resume_road()) { road(s_team, seed, 1); break; }
        if (mode == M_VERSUS && !second_player()) continue;
        picked = pick_nation(s_team, mode == M_VERSUS ? "PLAYER 1  CHOOSE YOUR NATION" : "CHOOSE YOUR NATION", 0);
        if (picked == 0xFFu) continue;
        s_team = picked;
        if (mode == M_ROAD) { road(s_team, seed, 0); break; }
        if (mode == M_CUP) { quick_cup(s_team); break; }
        if (mode == M_TRAINING) { cx_training(s_team); break; }
        s_foe = (uint8_t)(s_team + 1u);
        if (s_foe >= CX_SQUADS) s_foe = 0;
        picked = pick_nation(s_foe, mode == M_VERSUS ? "PLAYER 2  CHOOSE YOUR NATION" : "CHOOSE THE OPPONENT", (uint8_t)(mode == M_VERSUS));
        if (picked == 0xFFu) continue;
        s_foe = picked;
        if (mode == M_PENS) {
            (void)cx_penalties(s_team, s_foe, 1);
        } else {
            uint8_t r;
            cx_match_round = 0;
            r = cx_match(s_team, s_foe, pick_ground(), 0, (uint8_t)(mode == M_VERSUS ? 3u : 1u));
            if (r != 2u) result_screen(s_team, s_foe, r);
        }
        break;
    }
    clearSprs();
    ng_fix_clear();
    soundStopAll();
}
