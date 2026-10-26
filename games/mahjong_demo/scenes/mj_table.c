/*
 * mj_table.c -- Mahjong Demo: four-seat Mahjong, Hong Kong Old Style.
 *
 * Seat 0 is the player (East at the start), seats 1-3 are the engine's CPU
 * seats; in attract all four are. The engine deals, draws, decides for the
 * CPU seats and resolves claims; this file shows the table, asks the
 * player for decisions and paces the CPU turns so they can be followed.
 *
 * The screen: the three other seats' panels at the top (wind, score, tiles
 * held, flowers, melds and discards, as two-cell mini tiles), the
 * player's discards, a message line, the claim menu, and the player's hand
 * as sprite tiles along the bottom with exposed melds after it.
 *
 * Controls: LEFT / RIGHT choose a tile, A discards it, C declares a win or
 * a kong when one is possible, D asks the CPU for a hint. In a claim
 * window LEFT / RIGHT choose, A takes it and B passes. Holding B speeds
 * the CPU turns up.
 */
#include "mj.h"

static NGMahjong s_m;
static uint8_t s_demo, s_cursor, s_hinted;     /* s_hinted: the message line holds a hint */

/* ======================================================================
 * the table
 * ==================================================================== */
static const char *const k_wind[4] = { "EAST", "SOUTH", "WEST", "NORTH" };
static const char *const k_wind5[4] = { "EAST ", "SOUTH", "WEST ", "NORTH" };

/* Everything stays inside columns 3-36 (x 24-296): a CRT's frame hides the edges. */
#define COL_L          3u
#define COL_R          36u
#define HEAD_Y         1u
#define PANEL_Y(seat)  ((uint8_t)(2u + 4u * ((seat) - 1u)))   /* seats 1-3 */
#define OWN_Y          14u
#define MSG_Y          17u
#define MENU_Y         18u
#define NAME_Y         19u
#define HAND_Y         164
#define HELP_Y         26u

static void NEOGEO_USER header(void)
{
    mj_puts(3, HEAD_Y, k_wind5[s_m.round_wind - NGM_EAST], MJ_TXT_GOLD);
    mj_puts(9, HEAD_Y, "ROUND HAND", MJ_TXT_GOLD);
    mj_putn(19, HEAD_Y, (int32_t)s_m.hand_no + 1, 3, MJ_TXT_WHITE);
    mj_puts(23, HEAD_Y, "WALL", MJ_TXT_GOLD);
    mj_putn(27, HEAD_Y, s_m.wall_left, 3, MJ_TXT_WHITE);
    mj_puts(32, HEAD_Y, "HK OS", MJ_TXT_GREY);
}

/* A seat's name line: a turn marker, its wind, who plays it, its score, and
 * its flowers (room for six; past that five and a count). */
static void NEOGEO_USER seat_line(uint8_t seat, uint8_t y)
{
    const NGMahjongHand *h = &s_m.hand[seat];
    uint8_t b, n = 0, x = 25;
    mj_puts(3, y, s_m.turn == seat && s_m.phase == NGM_PH_TURN ? ">" : " ", MJ_TXT_GOLD);
    mj_puts(4, y, k_wind5[ng_mahjong_seat_wind(&s_m, seat) - NGM_EAST], seat == 0 ? MJ_TXT_SKY : MJ_TXT_WHITE);
    mj_puts(10, y, seat == 0 && !s_demo ? "YOU" : "CPU", seat == 0 ? MJ_TXT_SKY : MJ_TXT_GREY);
    mj_putn(13, y, s_m.score[seat], 6, MJ_TXT_WHITE);
    if (seat) {
        mj_putn(20, y, h->count, 2, MJ_TXT_GREY);
        mj_mini_back(22, y);
    }
    for (b = 0; b < 8u; b++) if (h->flowers & (1u << b)) n++;
    for (b = 0; b < 8u; b++) {
        if (!(h->flowers & (1u << b))) continue;
        if (n > 6u && x >= 35u) { mj_puts(x, y, "+", MJ_TXT_MINT); mj_putn((uint8_t)(x + 1u), y, n - 5u, 1, MJ_TXT_MINT); x = 37; break; }
        mj_mini(x, y, (uint8_t)(NGM_FLOWER + b));
        x = (uint8_t)(x + 2u);
    }
    mj_clear_from(x, y);
}

static uint8_t NEOGEO_USER meld_tile(const NGMahjongMeld *md, uint8_t i)
{
    return (uint8_t)(md->type == NGM_MELD_CHOW ? md->kind + i : md->kind);
}

static uint8_t NEOGEO_USER meld_size(const NGMahjongMeld *md)
{
    return (uint8_t)(md->type >= NGM_MELD_KONG ? 4u : 3u);
}

/* Discards in rows of 17 from (COL_L, y); the one up for claims marked. */
static void NEOGEO_USER discards(uint8_t seat, uint8_t y, uint8_t rows)
{
    const NGMahjongHand *h = &s_m.hand[seat];
    uint8_t i, x = COL_L, row = 0;
    for (i = 0; i < h->ndiscards && row < rows; i++) {
        uint8_t taken = (uint8_t)(h->discard_taken[i >> 3] & (1u << (i & 7u)));
        if (taken) continue;                               /* claimed into a meld */
        mj_mini(x, (uint8_t)(y + row), h->discards[i]);
        x = (uint8_t)(x + 2u);
        if (x + 1u > COL_R) { x = COL_L; row++; }
    }
    if (s_m.phase == NGM_PH_CLAIM && s_m.discarder == seat && x > COL_L && row < rows) {
        mj_puts(x, (uint8_t)(y + row), "<", MJ_TXT_GOLD);
        x++;
    }
    for (; row < rows; row++, x = 0) mj_clear_from(x, (uint8_t)(y + row));
}

static void NEOGEO_USER other_seat(uint8_t seat)
{
    const NGMahjongHand *h = &s_m.hand[seat];
    uint8_t y = PANEL_Y(seat), m, i, x = COL_L, gap, w = 0;
    seat_line(seat, y);
    for (m = 0; m < h->nmelds; m++) w = (uint8_t)(w + 2u * meld_size(&h->melds[m]) + 1u);
    gap = (uint8_t)(w <= COL_R - COL_L + 2u ? 1u : 0u);    /* four kongs only fit shoulder to shoulder */
    for (m = 0; m < h->nmelds; m++) {
        const NGMahjongMeld *md = &h->melds[m];
        for (i = 0; i < meld_size(md); i++) {
            if (md->type == NGM_MELD_KONG_CONCEALED && (i == 0u || i == 3u)) mj_mini_back(x, (uint8_t)(y + 1u));
            else mj_mini(x, (uint8_t)(y + 1u), meld_tile(md, i));
            x = (uint8_t)(x + 2u);
        }
        x = (uint8_t)(x + gap);
    }
    mj_clear_from(x, (uint8_t)(y + 1u));
    discards(seat, (uint8_t)(y + 2u), 2);
}

/* The player's hand: concealed tiles sorted, the tile just drawn apart,
 * then the exposed melds. Returns how many concealed tiles there are. */
static uint8_t s_hand_kind[NGM_HAND_MAX], s_hand_n;

static void NEOGEO_USER own_hand(uint8_t show_cursor)
{
    const NGMahjongHand *h = &s_m.hand[0];
    uint8_t i, n = 0, slot = 0, drawn_left = 0, m, k;
    int16_t x = 8 * COL_L;
    uint8_t drawn = (s_m.phase == NGM_PH_TURN && s_m.turn == 0 && s_m.drawn != NGM_NO_TILE) ? s_m.drawn : NGM_NO_TILE;
    for (i = 0; i < h->count; i++) {
        if (h->tiles[i] == drawn && !drawn_left) { drawn_left = 1; continue; }
        s_hand_kind[n++] = h->tiles[i];
    }
    if (drawn_left) s_hand_kind[n++] = drawn;
    s_hand_n = n;
    if (s_cursor >= n) s_cursor = (uint8_t)(n ? n - 1u : 0u);
    for (i = 0; i < n; i++) {
        uint8_t up = (uint8_t)(show_cursor && i == s_cursor);
        if (drawn_left && i == (uint8_t)(n - 1u)) x = (int16_t)(x + 6);
        mj_tile(slot++, s_hand_kind[i], x, (int16_t)(HAND_Y - (up ? 6 : 0)), up ? MJ_LOOK_SELECTED : MJ_LOOK_NORMAL);
        x = (int16_t)(x + 16);
    }
    x = (int16_t)(x + 8);
    for (m = 0; m < h->nmelds; m++) {
        const NGMahjongMeld *md = &h->melds[m];
        for (k = 0; k < meld_size(md); k++) {
            uint8_t kind = meld_tile(md, k);
            if (md->type == NGM_MELD_KONG_CONCEALED && (k == 0u || k == 3u)) kind = MJ_TILE_BACK;
            mj_tile(slot++, kind, x, (int16_t)(HAND_Y + 4), MJ_LOOK_NORMAL);
            x = (int16_t)(x + 12);
        }
        x = (int16_t)(x + 4);
    }
    mj_tiles_hide(slot, 24u);
    mj_line_c(NAME_Y, show_cursor && n ? mj_tile_name(s_hand_kind[s_cursor]) : 0, MJ_TXT_WHITE);
}

static void NEOGEO_USER message(const char *text, uint8_t pal)
{
    mj_line_c(MSG_Y, text, pal);
}

static void NEOGEO_USER table(uint8_t show_cursor)
{
    uint8_t s;
    header();
    for (s = 1; s < NGM_SEATS; s++) other_seat(s);
    seat_line(0, OWN_Y);
    discards(0, (uint8_t)(OWN_Y + 1u), 2);
    own_hand(show_cursor);
}

static void NEOGEO_USER help(const char *text)
{
    mj_line_c(HELP_Y, text, MJ_TXT_GREY);
}

/* ======================================================================
 * events: sounds and the message line
 * ==================================================================== */
static char s_line[40];

static const char *NEOGEO_USER say(uint8_t seat, const char *what)
{
    const char *w = k_wind[ng_mahjong_seat_wind(&s_m, seat) - NGM_EAST];
    uint8_t i = 0;
    while (*w && i < 20u) s_line[i++] = *w++;
    s_line[i++] = ':'; s_line[i++] = ' ';
    while (*what && i < 38u) s_line[i++] = *what++;
    s_line[i] = 0;
    return s_line;
}

static void NEOGEO_USER events(void)
{
    NGMahjongEvent e;
    while (ng_mahjong_event(&s_m, &e)) {
        switch (e.type) {
        case NGM_EV_DEAL: message("A NEW HAND IS DEALT", MJ_TXT_GOLD); mj_sfx(MJ_SFX_SHUFFLE); break;
        case NGM_EV_DRAW: if (e.seat == 0) mj_sfx(MJ_SFX_DRAW); break;
        case NGM_EV_FLOWER: message(say(e.seat, "A BONUS TILE"), MJ_TXT_MINT); mj_sfx(MJ_SFX_CLAIM); break;
        case NGM_EV_DISCARD: mj_sfx(MJ_SFX_DISCARD); break;
        case NGM_EV_CLAIM:
            message(say(e.seat, e.a == NGM_MELD_CHOW ? "CHOW!" : e.a == NGM_MELD_PUNG ? "PUNG!" : "KONG!"), MJ_TXT_GOLD);
            mj_sfx(e.a == NGM_MELD_KONG ? MJ_SFX_KONG : MJ_SFX_CLAIM);
            break;
        case NGM_EV_KONG_SELF: message(say(e.seat, "KONG!"), MJ_TXT_GOLD); mj_sfx(MJ_SFX_KONG); break;
        case NGM_EV_WIN: message(say(e.seat, e.a == e.seat ? "SELF-DRAWN WIN!" : "WIN ON THE DISCARD!"), MJ_TXT_RED); mj_sfx(MJ_SFX_WIN); break;
        case NGM_EV_EXHAUSTED: message("THE WALL IS EMPTY: A DRAW", MJ_TXT_GREY); mj_sfx(MJ_SFX_EXHAUSTED); break;
        default: break;
        }
    }
}

/* Frames to wait, B held (or attract with START pending) making it quick. */
static uint8_t NEOGEO_USER pause(uint8_t frames)
{
    while (frames--) {
        mj_frame();
        if (mj_quit_requested()) return 1;
        if (mj_held() & MJ_B) frames = (uint8_t)(frames > 2u ? 2u : frames);
    }
    return 0;
}

/* ======================================================================
 * the player's decisions
 * ==================================================================== */
static void NEOGEO_USER hint(void)
{
    NGMahjongView v;
    uint8_t act, arg, i;
    ng_mahjong_view(&s_m, 0, &v);
    ng_mahjong_cpu_choose(&v, 2u, &act, &arg);
    mj_sfx(MJ_SFX_HINT);
    s_hinted = 1;
    if (act == NGM_ACT_TSUMO) { message("HINT: YOU CAN WIN - PRESS C", MJ_TXT_SKY); return; }
    if (act == NGM_ACT_KONG_SELF) { message("HINT: DECLARE A KONG - C", MJ_TXT_SKY); }
    else message("HINT: DISCARD THE RAISED TILE", MJ_TXT_SKY);
    for (i = 0; i < s_hand_n; i++) if (s_hand_kind[i] == arg) { s_cursor = i; break; }
}

static void NEOGEO_USER own_turn(void)
{
    uint16_t w = ng_mahjong_wants(&s_m, 0), in;
    uint8_t k;
    if (s_hinted) { message(0, MJ_TXT_WHITE); s_hinted = 0; }   /* last turn's hint */
    table(1);
    help((w & NGM_CAN_TSUMO) ? "A DISCARD  C WIN!  D HINT" : (w & NGM_CAN_KONG_SELF) ? "A DISCARD  C KONG  D HINT"
                                                                                     : "A DISCARD  D HINT");
    for (;;) {
        mj_frame();
        if (mj_quit_requested()) return;
        in = mj_repeat();
        if (in & MJ_LEFT && s_cursor) { s_cursor--; mj_sfx(MJ_SFX_MOVE); own_hand(1); }
        if (in & MJ_RIGHT && s_cursor + 1u < s_hand_n) { s_cursor++; mj_sfx(MJ_SFX_MOVE); own_hand(1); }
        if (in & MJ_D) { hint(); own_hand(1); }
        k = s_hand_kind[s_cursor];
        if (in & MJ_A) {
            if (ng_mahjong_command(&s_m, 0, NGM_ACT_DISCARD, k) == NGM_OK) break;
            mj_sfx(MJ_SFX_NOPE);
        }
        if (in & MJ_C) {
            if (w & NGM_CAN_TSUMO) { ng_mahjong_command(&s_m, 0, NGM_ACT_TSUMO, 0); break; }
            if (w & NGM_CAN_KONG_SELF) {
                uint8_t i;
                if (ng_mahjong_command(&s_m, 0, NGM_ACT_KONG_SELF, k) == NGM_OK) break;
                for (i = 0; i < NGM_KINDS; i++)          /* not the chosen tile: the first kong there is */
                    if (ng_mahjong_command(&s_m, 0, NGM_ACT_KONG_SELF, i) == NGM_OK) break;
                if (i < NGM_KINDS) break;
            }
            mj_sfx(MJ_SFX_NOPE);
        }
    }
    help(0);
    mj_line_c(NAME_Y, 0, MJ_TXT_WHITE);
}

typedef struct { uint16_t can; uint8_t act, arg; const char *label; } ClaimOption;

static void NEOGEO_USER claim_window(void)
{
    static const ClaimOption all[7] = {
        { NGM_CAN_RON, NGM_ACT_RON, 0, "WIN!" },
        { NGM_CAN_KONG, NGM_ACT_KONG, 0, "KONG" },
        { NGM_CAN_PUNG, NGM_ACT_PUNG, 0, "PUNG" },
        { NGM_CAN_CHOW_LOW, NGM_ACT_CHOW, 0, "CHOW" },
        { NGM_CAN_CHOW_MID, NGM_ACT_CHOW, 1, "CHOW" },
        { NGM_CAN_CHOW_HIGH, NGM_ACT_CHOW, 2, "CHOW" },
        { NGM_CAN_PASS, NGM_ACT_PASS, 0, "PASS" },
    };
    const ClaimOption *opt[7];
    uint16_t w = ng_mahjong_wants(&s_m, 0);
    uint8_t n = 0, i, sel = 0, k = s_m.last_discard;
    for (i = 0; i < 7u; i++) if (w & all[i].can) opt[n++] = &all[i];
    if (w == NGM_CAN_PASS) { ng_mahjong_command(&s_m, 0, NGM_ACT_PASS, 0); return; }
    table(0);
    message(say(s_m.discarder, mj_tile_name(k)), MJ_TXT_WHITE);
    help("LEFT/RIGHT CHOOSE  A TAKE  B PASS");
    for (;;) {
        uint8_t x = COL_L, y = MENU_Y;
        for (i = 0; i < n; i++) {
            uint8_t chow = (uint8_t)(opt[i]->act == NGM_ACT_CHOW);
            if (x + (chow ? 12u : 5u) > COL_R + 1u) { mj_clear_from(x, y); x = COL_L; y++; }   /* on to the next row */
            mj_puts(x, y, i == sel ? ">" : " ", MJ_TXT_GOLD);
            mj_puts((uint8_t)(x + 1u), y, opt[i]->label, i == sel ? MJ_TXT_GOLD : MJ_TXT_WHITE);
            x = (uint8_t)(x + 6u);
            if (chow) {                                        /* the run it makes */
                uint8_t lo = (uint8_t)(k - opt[i]->arg), t;
                for (t = 0; t < 3u; t++) { mj_mini(x, y, (uint8_t)(lo + t)); x = (uint8_t)(x + 2u); }
                x++;
            }
        }
        mj_clear_from(x, y);
        if (y == MENU_Y) mj_clear_from(0, (uint8_t)(MENU_Y + 1u));
        for (;;) {
            uint16_t in;
            mj_frame();
            if (mj_quit_requested()) return;
            in = mj_repeat();
            if (in & MJ_LEFT && sel) { sel--; mj_sfx(MJ_SFX_MOVE); break; }
            if (in & MJ_RIGHT && sel + 1u < n) { sel++; mj_sfx(MJ_SFX_MOVE); break; }
            if (in & MJ_B) { ng_mahjong_command(&s_m, 0, NGM_ACT_PASS, 0); goto done; }
            if (in & MJ_A) {
                if (ng_mahjong_command(&s_m, 0, opt[sel]->act, opt[sel]->arg) == NGM_OK) goto done;
                mj_sfx(MJ_SFX_NOPE);
            }
        }
    }
done:
    mj_clear_text(MENU_Y, MENU_Y + 1u);
    help(0);
}

/* ======================================================================
 * the end of a hand, the end of the game
 * ==================================================================== */
typedef struct { uint32_t bit; const char *name; const char *faan; } Pattern;
static const Pattern k_patterns[] = {
    { NGM_PAT_SELF_DRAWN, "SELF-DRAWN", "1" }, { NGM_PAT_CONCEALED, "CONCEALED HAND", "1" },
    { NGM_PAT_ALL_CHOWS, "ALL CHOWS", "1" }, { NGM_PAT_ALL_PUNGS, "ALL PUNGS", "3" },
    { NGM_PAT_HALF_FLUSH, "HALF FLUSH", "3" }, { NGM_PAT_FULL_FLUSH, "FULL FLUSH", "7" },
    { NGM_PAT_DRAGON_PUNG, "DRAGON PUNG", "1 EACH" }, { NGM_PAT_SEAT_WIND, "SEAT WIND", "1" },
    { NGM_PAT_ROUND_WIND, "ROUND WIND", "1" }, { NGM_PAT_SMALL_DRAGONS, "SMALL THREE DRAGONS", "5" },
    { NGM_PAT_BIG_DRAGONS, "BIG THREE DRAGONS", "8" }, { NGM_PAT_SMALL_WINDS, "SMALL FOUR WINDS", "10" },
    { NGM_PAT_BIG_WINDS, "BIG FOUR WINDS", "LIMIT" }, { NGM_PAT_ALL_HONORS, "ALL HONOURS", "10" },
    { NGM_PAT_THIRTEEN, "THIRTEEN ORPHANS", "LIMIT" }, { NGM_PAT_NINE_GATES, "NINE GATES", "LIMIT" },
    { NGM_PAT_ALL_TERMINALS, "ALL TERMINALS", "10" }, { NGM_PAT_LAST_TILE, "LAST TILE", "1" },
    { NGM_PAT_KONG_DRAW, "WIN ON A KONG", "1" }, { NGM_PAT_NO_FLOWERS, "NO FLOWERS", "1" },
    { NGM_PAT_OWN_FLOWER, "OWN FLOWER", "1 EACH" }, { NGM_PAT_HEAVENLY, "HEAVENLY HAND", "LIMIT" },
};

static uint8_t NEOGEO_USER hand_over(void)
{
    const NGMahjongResult *r = &s_m.result;
    uint8_t y = 4, i, s, x = COL_L;
    mj_tiles_hide(0, 24u);
    mj_clear_text(1, 27);
    mj_music(MJ_MUS_RESULT);
    if (r->winner == NGM_NO_TILE) {
        mj_puts_c(3, "A DRAW: THE WALL IS EMPTY", MJ_TXT_GOLD);
    } else {
        const NGMahjongHand *h = &s_m.hand[r->winner];
        mj_puts_c(2, say(r->winner, r->from == r->winner ? "WINS, SELF-DRAWN" : "WINS ON A DISCARD"), MJ_TXT_GOLD);
        for (i = 0; i < h->count; i++) { mj_mini(x, y, h->tiles[i]); x = (uint8_t)(x + 2u); }
        x++;
        for (i = 0; i < h->nmelds; i++) {
            uint8_t t;
            if (x + 2u * meld_size(&h->melds[i]) > COL_R + 1u) { x = COL_L; y++; }   /* kongs: a second row */
            for (t = 0; t < meld_size(&h->melds[i]); t++) { mj_mini(x, y, meld_tile(&h->melds[i], t)); x = (uint8_t)(x + 2u); }
            x++;
        }
        y = 6;
        for (i = 0; i < sizeof(k_patterns) / sizeof(k_patterns[0]) && y < 16u; i++) {
            if (!(r->patterns & k_patterns[i].bit)) continue;
            mj_puts(6, y, k_patterns[i].name, MJ_TXT_WHITE);
            mj_puts(28, y, k_patterns[i].faan, MJ_TXT_SKY);
            y++;
        }
        if (y == 6u) mj_puts(6, y++, "CHICKEN HAND", MJ_TXT_WHITE);
        mj_puts(6, (uint8_t)(y + 1u), "FAAN", MJ_TXT_GOLD);
        mj_putn(26, (uint8_t)(y + 1u), r->faan, 3, MJ_TXT_GOLD);
    }
    for (s = 0; s < NGM_SEATS; s++) {
        uint8_t row = (uint8_t)(19u + s);
        mj_puts(6, row, k_wind[ng_mahjong_seat_wind(&s_m, s) - NGM_EAST], s == 0 && !s_demo ? MJ_TXT_SKY : MJ_TXT_WHITE);
        mj_putn(13, row, r->delta[s], 6, r->delta[s] < 0 ? MJ_TXT_RED : MJ_TXT_MINT);
        mj_putn(22, row, s_m.score[s], 7, MJ_TXT_WHITE);
    }
    mj_puts_c(25, "PRESS A", MJ_TXT_GREY);
    if (s_demo) { if (pause(240)) return 1; }
    else if (!mj_wait_button(1800) && mj_quit_requested()) return 1;
    mj_clear_text(1, 27);
    return 0;
}

static void NEOGEO_USER final_scores(void)
{
    uint8_t order[4] = { 0, 1, 2, 3 }, i, j;
    for (i = 0; i < 4u; i++)
        for (j = (uint8_t)(i + 1u); j < 4u; j++)
            if (s_m.score[order[j]] > s_m.score[order[i]]) { uint8_t t = order[i]; order[i] = order[j]; order[j] = t; }
    mj_tiles_hide(0, 24u);
    mj_clear_text(0, 27);
    mj_music(MJ_MUS_RESULT);
    mj_puts_c(4, "THE GAME IS OVER", MJ_TXT_GOLD);
    for (i = 0; i < 4u; i++) {
        uint8_t s = order[i], row = (uint8_t)(8u + 2u * i);
        mj_putn(8, row, (int32_t)i + 1, 1, MJ_TXT_GOLD);
        mj_puts(11, row, s == 0 && !s_demo ? "YOU" : "CPU", s == 0 && !s_demo ? MJ_TXT_SKY : MJ_TXT_WHITE);
        mj_puts(15, row, k_wind[s], MJ_TXT_GREY);
        mj_putn(22, row, s_m.score[s], 7, MJ_TXT_WHITE);
    }
    if (!s_demo) mj_puts_c(18, order[0] == 0 ? "FIRST PLACE. WELL PLAYED!" : "THANK YOU FOR PLAYING", MJ_TXT_MINT);
    if (s_demo) pause(240);
    else mj_wait_button(1800);
}

/* ======================================================================
 * a game
 * ==================================================================== */
uint8_t NEOGEO_USER mj_table_run(uint8_t demo)
{
    NGMahjongRules rules;
    static const uint8_t rounds[3] = { 1, 2, 4 };
    uint8_t s, level = mj_dip(0);
    s_demo = demo;
    s_cursor = 0;
    ng_mahjong_rules_default(&rules);
    rules.flowers = (uint8_t)(mj_dip(1) ? 1u : 0u);
    rules.rounds = rounds[mj_dip(2) < 3u ? mj_dip(2) : 0u];
    if (demo) rules.rounds = 1;
    ng_mahjong_init(&s_m, &rules, ((uint32_t)mj_frame() << 16) ^ 0x9E3779B9u ^ mj_held());
    for (s = 0; s < NGM_SEATS; s++) ng_mahjong_set_cpu(&s_m, s, s == 0 && !demo ? NGM_HUMAN : level);
    mj_screen(0);
    table(0);
    mj_reveal();
    mj_music(MJ_MUS_TABLE);
    for (;;) {
        if (mj_quit_requested()) return 0;
        if (s_m.phase == NGM_PH_GAME_OVER) break;
        if (s_m.phase == NGM_PH_HAND_OVER) {
            table(0);
            if (pause(demo ? 90u : 60u)) return 0;
            if (hand_over()) return 0;
            if (ng_mahjong_next_hand(&s_m) != NGM_OK) break;
            mj_music(MJ_MUS_TABLE);
            table(0);
            continue;
        }
        if (!demo && ng_mahjong_wants(&s_m, 0)) {
            if (s_m.phase == NGM_PH_TURN) own_turn();
            else claim_window();
            events();
            table(0);
            continue;
        }
        ng_mahjong_update(&s_m);
        events();
        table(0);
        if (s_m.phase == NGM_PH_CLAIM || (s_m.phase == NGM_PH_TURN && s_m.turn != 0) || demo)
            if (pause(demo ? 14u : 22u)) return 0;
    }
    final_scores();
    return 1;
}
