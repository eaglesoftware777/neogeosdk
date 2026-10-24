/*
 * mj_game.c -- Mahjong Demo: title, attract, menu, rules and how to play.
 *
 * Attract: the title, then a hand of four CPU seats playing each other
 * (the engine on its own), then how to play. A credited game opens a
 * menu: four-seat Mahjong, solitaire, the rules this demo implements, or
 * the end of the game.
 */
#include "mj.h"

static uint8_t s_attract;

static const NGHowToPlay k_howto = {
    {'H', 'T', 'P', '1'}, 12, 12, "MAHJONG DEMO",
    {
        "FOUR SEATS: YOU ARE EAST.",
        "LEFT/RIGHT CHOOSE A TILE,",
        "A DISCARDS IT. C DECLARES A",
        "WIN OR A KONG. D: A HINT.",
        "CLAIMS: CHOOSE, A TAKES,",
        "B PASSES. HOLD B: FASTER.",
        "",
        "SOLITAIRE: PICK TWO FREE",
        "TILES THAT MATCH. A PICKS,",
        "B UNDOES, C HINTS, D SHUFFLES.",
        "",
        "THE ART HERE IS TEST ART.",
    }
};

uint8_t NEOGEO_USER mj_dip(uint8_t n) { return ng_dip_option(n); }

/* In attract, a credit or a START ends what is showing. */
uint8_t NEOGEO_USER mj_quit_requested(void)
{
    return (uint8_t)(s_attract && mj_start_pending());
}

/* ======================================================================
 * title and attract
 * ==================================================================== */
static void NEOGEO_USER title_art(void)
{
    static const uint8_t show[6] = { NGM_EAST, NGM_RED, 4u, 13u, 22u, NGM_FLOWER };
    uint8_t i;
    mj_screen(1);
    for (i = 0; i < 6u; i++) mj_tile(i, show[i], (int16_t)(112 + 16 * i), 92, MJ_LOOK_NORMAL);
    mj_puts_c(17, "FOUR-SEAT MAHJONG, HONG KONG OLD STYLE", MJ_TXT_WHITE);
    mj_puts_c(18, "AND MAHJONG SOLITAIRE", MJ_TXT_WHITE);
    mj_puts_c(25, "NEOGEOSDK GENRE ENGINE DEMO - TEST ART", MJ_TXT_GREY);
    mj_puts_c(27, "(C) 2026 EAGLE SOFTWARE", MJ_TXT_WHITE);
    mj_reveal();
}

void NEOGEO_USER mj_title_screen(void)
{
    uint16_t t = 0;
    uint8_t secs = 15, f = 0;
    s_attract = 0;
    title_art();
    mj_music(MJ_MUS_MENU);
    for (;;) {
        if (NEO_REGISTER8(MJ_START_FLAG)) break;
        t++;
        if (!ng_sys_is_mvs()) {
            mj_line_c(21, (t & 32u) ? "PRESS START" : 0, MJ_TXT_GOLD);
            if (mj_start_pending()) break;
        } else if (read_p1credit() > 0) {
            mj_line_c(21, "PRESS 1P START", MJ_TXT_GOLD);
            mj_putn(19, 22, secs, 2, MJ_TXT_WHITE);
            if (++f >= 60u) { f = 0; if (secs) secs--; }
            if (!secs) mj_auto_start();
        } else {
            mj_line_c(21, (t & 32u) ? "INSERT COIN" : 0, MJ_TXT_GOLD);
        }
        mj_frame();
    }
    mj_sfx(MJ_SFX_START);
}

static void NEOGEO_USER own_how_to_play(uint16_t frames)
{
    uint8_t i;
    mj_screen(0);
    mj_puts_c(2, k_howto.title, MJ_TXT_GOLD);
    for (i = 0; i < k_howto.lines; i++) mj_puts(5, (uint8_t)(5u + i), k_howto.line[i], i < 7u ? MJ_TXT_WHITE : MJ_TXT_SKY);
    mj_reveal();
    while (frames--) {
        mj_frame();
        if (mj_quit_requested()) return;
        if (!s_attract && (mj_pressed() & (MJ_BUTTONS | MJ_START))) return;
    }
}

void NEOGEO_USER mj_run_attract(void)
{
    s_attract = 1;
    for (;;) {
        uint16_t t;
        title_art();
        mj_music(MJ_MUS_MENU);
        for (t = 0; t < 420u; t++) {
            mj_line_c(21, (t & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PRESS START") : 0, MJ_TXT_GOLD);
            mj_frame();
            if (mj_start_pending()) return;
        }
        mj_table_run(1);
        if (mj_start_pending()) return;
        if (ng_sys_is_eaglebios()) {
            mj_fade_out();
            ng_sys_howtoplay(&k_howto);
        } else {
            own_how_to_play(600);
        }
        if (mj_start_pending()) return;
    }
}

/* ======================================================================
 * the rules page
 * ==================================================================== */
static const char *const k_rules[] = {
    "FOUR SEATS: HONG KONG OLD STYLE",
    "",
    "136 TILES, OR 144 WITH FLOWERS",
    "WIN: FOUR SETS AND A PAIR,",
    "  OR THIRTEEN ORPHANS",
    "CLAIMS: A WIN FIRST, THEN PUNG",
    "  OR KONG, THEN THE NEXT SEAT'S",
    "  CHOW",
    "SCORE: BASE X 2 TO THE FAAN;",
    "  A DISCARDER PAYS DOUBLE,",
    "  A SELF-DRAWN WIN DOUBLE ALL",
    "NOT HERE: SEVEN PAIRS, ROBBING",
    "  A KONG, RIICHI RULES",
    "",
    "SOLITAIRE: A TILE IS FREE WITH",
    "  NOTHING ON TOP AND ITS LEFT",
    "  OR RIGHT SIDE OPEN",
};

static void NEOGEO_USER rules_page(void)
{
    uint8_t i;
    mj_screen(0);
    mj_puts_c(1, "THE RULES IN THIS DEMO", MJ_TXT_GOLD);
    for (i = 0; i < sizeof(k_rules) / sizeof(k_rules[0]); i++)
        mj_puts(3, (uint8_t)(4u + i), k_rules[i], k_rules[i][0] == ' ' ? MJ_TXT_WHITE : MJ_TXT_SKY);
    mj_puts_c(26, "PRESS A", MJ_TXT_GREY);
    mj_reveal();
    mj_wait_button(3600);
}

/* ======================================================================
 * a credited game: the menu
 * ==================================================================== */
enum { MENU_TABLE = 0, MENU_SOLITAIRE, MENU_RULES, MENU_END, MENU_ITEMS };

static uint8_t NEOGEO_USER menu(uint8_t sel)
{
    static const char *const item[MENU_ITEMS] = { "FOUR-SEAT MAHJONG", "MAHJONG SOLITAIRE", "THE RULES", "END THE GAME" };
    uint16_t idle = 0;
    uint8_t i;
    mj_screen(1);
    mj_puts_c(25, "UP/DOWN CHOOSE  A START", MJ_TXT_GREY);
    for (i = 0; i < MENU_ITEMS; i++) mj_line_c((uint8_t)(12u + 2u * i), item[i], i == sel ? MJ_TXT_GOLD : MJ_TXT_WHITE);
    mj_reveal();
    mj_music(MJ_MUS_MENU);
    for (;;) {
        uint16_t in;
        mj_frame();
        in = mj_repeat();
        if (in) idle = 0;
        else if (++idle >= 60u * 60u) return MENU_END;      /* a minute untouched: the game ends */
        if (in & (MJ_UP | MJ_DOWN)) {
            if (in & MJ_UP) sel = (uint8_t)(sel ? sel - 1u : MENU_ITEMS - 1u);
            else sel = (uint8_t)(sel + 1u < MENU_ITEMS ? sel + 1u : 0u);
            mj_sfx(MJ_SFX_MOVE);
            for (i = 0; i < MENU_ITEMS; i++) mj_line_c((uint8_t)(12u + 2u * i), item[i], i == sel ? MJ_TXT_GOLD : MJ_TXT_WHITE);
        }
        if (mj_pressed() & MJ_A) { mj_sfx(MJ_SFX_START); return sel; }
    }
}

void NEOGEO_USER mj_run(void)
{
    uint8_t sel = 0;
    s_attract = 0;
    if (mj_dip(3) == 0) own_how_to_play(600);
    for (;;) {
        sel = menu(sel);
        if (sel == MENU_TABLE) mj_table_run(0);
        else if (sel == MENU_SOLITAIRE) mj_sol_run();
        else if (sel == MENU_RULES) rules_page();
        else break;
    }
    mj_fade_out();
}
