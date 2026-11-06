/*
 * sw_game.c — Seven Wonders screens and the flow of a game.
 *
 * Attract: the title, how to play, a demonstration board played by the
 * engine's own hints (a match board and a picture in turn), the best
 * scores. A credited game: how to play (an operator setting), then chapter
 * by chapter: the chapter card, each level's card, the level, its result.
 * Each chapter is four levels of matching and bubbles, then a picture to
 * solve from its number clues. A level lost costs a try; out of tries, a
 * continue (a credit on an arcade board, free on a console) or the end.
 * After the seventh wonder, the ending.
 *
 * Controls: the joystick moves the paw (match, pictures) or aims (bubbles);
 * A picks up a piece, then the joystick pushes it into its neighbour, or A
 * fires, or A fills a square; B puts the piece down, swaps the loaded and
 * the next bubble, or crosses a square out (held, A and B go on along the
 * paw's way); C is the paw hammer, once a level: it breaks the piece under
 * the paw, or the bubble the aim points at, or shows the row and column
 * under the paw; D shows a hint (match) or switches the aim guide
 * (bubbles). On a console START pauses.
 */
#include "sw.h"
#include "sdk/ng_system.h"
#include "sdk/2d_engine/ng_fix.h"

#define SAVE_VERSION  2u
#define HISCORES      5u
#define HS_AT         (4u + SW_LEVELS * 5u)  /* the best five: name (3), wonder reached, score (4) */
#define SAVE_SIZE     (HS_AT + HISCORES * 8u)
#define SAVE_V1_SIZE  (4u + 28u * 5u)      /* the first layout: four levels a chapter */

static NGPuzzle s_p;
static uint32_t s_total;
static uint8_t s_cat_slot_mood, s_cat_timer, s_blink;
static uint8_t s_guide;
static uint8_t s_cat_x, s_cat_y;      /* where the cat of a level sits (a picture's is small, on the right) */

/* The screen's safe columns: a television may hide about twenty pixels at each
 * side, so text and the play area stay within x 24..296 (FIX columns 3..36). */
#define PANEL_COL 3u                  /* the side panel's text (8 columns) and a picture's corner box */
#define INFO_COL  31u                 /* a picture's column on the right (6 columns) */
static uint8_t s_drag;                /* pictures: 0, 1 filling, 2 crossing, 3 taking crosses away */
static uint8_t s_bonus_paw;           /* lucky paws won: one more hammer (or hint) next level each */

/* the game's own text for EagleBIOS's HOW TO PLAY screen */
static const NGHowToPlay k_howto = {
    {'H', 'T', 'P', '1'}, 12, 10, "SEVEN WONDERS PUZZLES",
    {
        "JOYSTICK  MOVE THE PAW OR AIM",
        "A  PICK UP A PIECE, THEN PUSH",
        "A  FIRE A BUBBLE",
        "B  PUT DOWN / SWAP BUBBLES",
        "C  PAW HAMMER, ONCE A LEVEL",
        "D  A HINT / THE AIM GUIDE",
        "PICTURES  A FILL  B CROSS OUT",
        "          C SHOW A ROW, ONCE",
        "",
        "LINE UP THREE OF A KIND",
        "POP THREE BUBBLES ALIKE",
        "FILL WHAT THE NUMBERS SAY",
    }
};
static const uint8_t k_howto_rows[12] = { 5, 7, 9, 11, 13, 15, 17, 18, 19, 21, 22, 23 };

static const char *const k_colors[8] = { "RUBY", "AMBER", "SUNSTONE", "EMERALD", "AQUA", "SAPPHIRE", "AMETHYST", "ROSE" };

/* ======================================================================
 * chapters, and the save block: best score and stars per level
 * ==================================================================== */
static uint8_t NEOGEO_USER chapter_of(uint8_t level)
{
    uint8_t c = 0;
    while (level >= SW_PER_CHAPTER) { level = (uint8_t)(level - SW_PER_CHAPTER); c++; }
    return c;
}

static uint8_t NEOGEO_USER stage_of(uint8_t level)
{
    while (level >= SW_PER_CHAPTER) level = (uint8_t)(level - SW_PER_CHAPTER);
    return level;
}

/* A level's five bytes: the first layout's four levels a chapter keep
 * their places, the seven pictures follow them. */
static uint16_t NEOGEO_USER save_at(uint8_t level)
{
    uint8_t c = chapter_of(level), st = stage_of(level);
    uint8_t slot = st < SW_PICTURE_STAGE ? (uint8_t)((c << 2) + st) : (uint8_t)(28u + c);
    return (uint16_t)(4u + 5u * slot);
}

static uint8_t *NEOGEO_USER save(void)
{
    return (uint8_t *)ng_save_data();
}

/* The high score table a fresh block starts with. */
static void NEOGEO_USER hs_defaults(uint8_t *d)
{
    static const char names[HISCORES][4] = { "MIM", "KIK", "NYA", "PAW", "SUN" };
    static const uint8_t wonders[HISCORES] = { 7, 6, 5, 3, 2 };
    uint8_t i;
    for (i = 0; i < HISCORES; i++) {
        uint8_t *e = d + HS_AT + 8u * i;
        uint32_t score = (uint32_t)(HISCORES - i) * 10000u;
        e[0] = (uint8_t)names[i][0]; e[1] = (uint8_t)names[i][1]; e[2] = (uint8_t)names[i][2];
        e[3] = wonders[i];
        e[4] = (uint8_t)(score >> 24); e[5] = (uint8_t)(score >> 16); e[6] = (uint8_t)(score >> 8); e[7] = (uint8_t)score;
    }
}

static void NEOGEO_USER save_init(void)
{
    if (ng_save_capacity() < SAVE_SIZE) return;
    if (!ng_save_valid(SAVE_VERSION, SAVE_SIZE)) {
        /* a first-layout block keeps its scores; anything else starts afresh */
        uint8_t keep[SAVE_V1_SIZE];
        uint8_t old = ng_save_valid(1u, SAVE_V1_SIZE);
        uint8_t *d;
        uint16_t i;
        if (old) ng_copy_bytes(keep, save(), SAVE_V1_SIZE);
        ng_save_format(SAVE_VERSION, SAVE_SIZE);
        d = save();
        for (i = 0; i < SAVE_SIZE; i++) d[i] = 0;
        if (old) ng_copy_bytes(d, keep, SAVE_V1_SIZE);
        hs_defaults(d);
        d[0] = 'S'; d[1] = 'W'; d[2] = SAVE_VERSION;
        ng_save_commit();
    }
}

static uint32_t NEOGEO_USER best_score(uint8_t level)
{
    const uint8_t *d = save() + save_at(level);
    if (ng_save_capacity() < SAVE_SIZE) return 0;
    return ((uint32_t)d[0] << 24) | ((uint32_t)d[1] << 16) | ((uint32_t)d[2] << 8) | d[3];
}

static uint8_t NEOGEO_USER best_stars(uint8_t level)
{
    if (ng_save_capacity() < SAVE_SIZE) return 0;
    return save()[save_at(level) + 4u];
}

static void NEOGEO_USER record(uint8_t level, uint32_t score, uint8_t stars)
{
    uint8_t *d;
    if (ng_save_capacity() < SAVE_SIZE) return;
    d = save() + save_at(level);
    if (score > best_score(level)) {
        d[0] = (uint8_t)(score >> 24); d[1] = (uint8_t)(score >> 16); d[2] = (uint8_t)(score >> 8); d[3] = (uint8_t)score;
    }
    if (stars > d[4]) d[4] = stars;
    ng_save_commit();
}

static uint32_t NEOGEO_USER hs_score(uint8_t i)
{
    const uint8_t *e = save() + HS_AT + 8u * i;
    if (ng_save_capacity() < SAVE_SIZE) return 0;
    return ((uint32_t)e[4] << 24) | ((uint32_t)e[5] << 16) | ((uint32_t)e[6] << 8) | e[7];
}

/* Where a score would go in the table; HISCORES when it doesn't make it. */
static uint8_t NEOGEO_USER hs_rank(uint32_t score)
{
    uint8_t i;
    if (ng_save_capacity() < SAVE_SIZE || !score) return HISCORES;
    for (i = 0; i < HISCORES; i++) if (score > hs_score(i)) return i;
    return HISCORES;
}

static void NEOGEO_USER hs_insert(uint8_t rank, const uint8_t *name, uint8_t wonder, uint32_t score)
{
    uint8_t *d = save() + HS_AT, i, *e;
    if (rank >= HISCORES || ng_save_capacity() < SAVE_SIZE) return;
    for (i = (uint8_t)(HISCORES - 1u); i > rank; i--)          /* entry by entry, last first */
        ng_copy_bytes(d + 8u * i, d + 8u * (i - 1u), 8u);
    e = d + 8u * rank;
    e[0] = name[0]; e[1] = name[1]; e[2] = name[2]; e[3] = wonder;
    e[4] = (uint8_t)(score >> 24); e[5] = (uint8_t)(score >> 16); e[6] = (uint8_t)(score >> 8); e[7] = (uint8_t)score;
    ng_save_commit();
}

/* ======================================================================
 * small screens
 * ==================================================================== */
static uint8_t NEOGEO_USER dip(uint8_t n) { return ng_dip_option(n); }

static void NEOGEO_USER copyright(void)
{
    sw_puts_c(26, "(C) 2026 EAGLE SOFTWARE", SW_TXT_WHITE);
}

/* The title's cats: they blink in turn and bob a little, out of step. */
static void NEOGEO_USER title_cats(uint16_t t)
{
    uint8_t bob = (uint8_t)((t >> 4) & 1u);
    if ((t & 15u) != 0) return;
    sw_title_cat(0, SW_CAT_MIMI, (uint8_t)((t & 0xF0u) == 0x80u ? 1u : 0u), 24, (int16_t)(104 - (bob << 1)));
    sw_title_cat(1, SW_CAT_KIKO, (uint8_t)((t & 0xF0u) == 0x00u ? 1u : 0u), 200, (int16_t)(102 + (bob << 1)));
}

static void NEOGEO_USER title_art(void)
{
    sw_logo(1, 6);
    title_cats(0);
    copyright();
}

void NEOGEO_USER sw_title_screen(void)
{
    uint16_t countdown = 15u * 60u, t = 0;
    save_init();
    sw_scene(SW_BG_TITLE);
    sw_music(SW_MUS_TITLE);
    title_art();
    for (;;) {
        if (NEO_REGISTER8(SW_START_FLAG)) break;
        t++;
        if (!ng_sys_is_mvs()) {
            sw_puts_c(22, (t & 32u) ? "PRESS START" : "           ", SW_TXT_WHITE);
            if (sw_start_pending()) break;
        } else if (read_p1credit() > 0) {
            sw_puts_c(22, "PRESS 1P START", SW_TXT_WHITE);
            sw_putn(18, 24, (uint32_t)(countdown / 60u), 2, SW_TXT_GOLD);
            if (countdown) countdown--;
            else sw_auto_start();
        } else {
            sw_puts_c(22, (t & 32u) ? "INSERT COIN" : "           ", SW_TXT_WHITE);
        }
        title_cats(t);
        sw_frame();
    }
    sw_sfx(SW_SFX_START);
}

/* The game's own how-to screen: the same on every system ROM. */
static void NEOGEO_USER how_to_play(uint16_t frames)
{
    uint8_t i;
    sw_scene(SW_BG_MAP);
    sw_card(1);
    sw_puts_c(3, "HOW TO PLAY", SW_TXT_GOLD);
    for (i = 0; i < k_howto.lines; i++) sw_puts(5, k_howto_rows[i], k_howto.line[i], (uint8_t)(i < 8u ? SW_TXT_WHITE : SW_TXT_MINT));
    while (frames--) {
        sw_frame();
        if (sw_start_pending() && !NEO_REGISTER8(SW_START_FLAG)) return;
        if (NEO_REGISTER8(SW_START_FLAG) && (sw_pressed() & (SW_BUTTONS | SW_START))) return;
    }
}

static void NEOGEO_USER best_scores(void)
{
    uint8_t c;
    sw_scene(SW_BG_MAP);
    sw_card(1);
    sw_puts_c(3, "THE SEVEN WONDERS", SW_TXT_GOLD);
    for (c = 0; c < SW_CHAPTERS; c++) {
        uint8_t l, stars = 0;
        uint32_t best = 0;
        for (l = 0; l < SW_PER_CHAPTER; l++) {
            stars = (uint8_t)(stars + best_stars((uint8_t)(c * SW_PER_CHAPTER + l)));
            best += best_score((uint8_t)(c * SW_PER_CHAPTER + l));
        }
        sw_puts(6, (uint8_t)(5u + 3u * c), sw_chapters[c].name, SW_TXT_WHITE);
        sw_putn(26, (uint8_t)(6u + 3u * c), best, 7, SW_TXT_GOLD);
        sw_putn(7, (uint8_t)(6u + 3u * c), stars, 2, SW_TXT_MINT);
        sw_puts(10, (uint8_t)(6u + 3u * c), "STARS", SW_TXT_MINT);
    }
    copyright();
    sw_wait(360);
}

/* The best five: rank, name, score and how far they got. */
static void NEOGEO_USER hiscores(uint8_t mark)
{
    static const char *const ranks[HISCORES] = { "1ST", "2ND", "3RD", "4TH", "5TH" };
    uint8_t i;
    uint16_t t;
    sw_scene(SW_BG_MAP);
    sw_card(1);
    sw_puts_c(3, "BEST CATS", SW_TXT_GOLD);
    sw_puts(6, 5, "     NAME   SCORE   WONDER", SW_TXT_SKY);
    for (i = 0; i < HISCORES; i++) {
        const uint8_t *e = save() + HS_AT + 8u * i;
        char name[4], w[4];
        uint8_t y = (uint8_t)(7u + (i << 1)), pal = i == mark ? SW_TXT_PINK : i ? SW_TXT_WHITE : SW_TXT_GOLD;
        if (ng_save_capacity() < SAVE_SIZE) break;
        name[0] = (char)e[0]; name[1] = (char)e[1]; name[2] = (char)e[2]; name[3] = 0;
        if (e[3] >= 8u) { w[0] = 'A'; w[1] = 'L'; w[2] = 'L'; }
        else { w[0] = ' '; w[1] = 'W'; w[2] = (char)('0' + e[3]); }
        w[3] = 0;
        sw_puts(6, y, ranks[i], pal);
        sw_puts(11, y, name, pal);
        sw_putn(16, y, hs_score(i), 7, pal);
        sw_puts(27, y, w, pal);
    }
    sw_cat(0, SW_CAT_MIMI, SW_MOOD_HAPPY, 72, 132);
    sw_cat(1, SW_CAT_KIKO, SW_MOOD_HAPPY, 184, 132);
    for (t = 0; t < 360u; t++) {
        if (mark < HISCORES) sw_puts(6, (uint8_t)(7u + (mark << 1)), (t & 16u) ? "   " : ranks[mark], SW_TXT_PINK);
        sw_frame();
        if (sw_start_pending() && !NEO_REGISTER8(SW_START_FLAG)) return;
        if (mark < HISCORES && t > 60u && (sw_pressed() & SW_BUTTONS)) return;
    }
}

/* The album: each chapter's picture once it is done, and its stars. */
static void NEOGEO_USER album(uint8_t fresh)
{
    static const int16_t ax[7] = { 52, 108, 164, 220, 76, 132, 188 };
    static const int16_t ay[7] = { 36, 36, 36, 36, 108, 108, 108 };
    uint8_t i, k, done = 0;
    uint16_t t;
    sw_scene(SW_BG_MAP);
    sw_card(1);
    sw_puts_c(3, "PICTURE ALBUM", SW_TXT_GOLD);
    for (i = 0; i < 7u; i++) {
        uint8_t level = (uint8_t)(i * SW_PER_CHAPTER + SW_PICTURE_STAGE), st = best_stars(level);
        uint8_t col = (uint8_t)((ax[i] + 12) >> 3), row = (uint8_t)(i < 4u ? 11u : 20u);
        sw_album(i, (uint8_t)(st > 0u), ax[i], ay[i]);
        for (k = 0; k < 3u; k++)
            ng_fix_put_tile((uint8_t)(col + k), row, SW_STAR_TILE, k < st ? SW_TXT_STAR : SW_TXT_STAR_OFF);
        if (st) done++;
    }
    if (done == 7u) sw_puts_c(23, "THE ALBUM IS COMPLETE!", SW_TXT_GOLD);
    else {
        sw_putn(13, 23, done, 1, SW_TXT_MINT);
        sw_puts(15, 23, "OF 7 PICTURES", SW_TXT_MINT);
    }
    if (fresh < 7u) {
        sw_puts_c(24, "A NEW PICTURE!", SW_TXT_PINK);
        sw_word(SW_WORD_LOVELY, (int16_t)(ax[fresh] + 24), (int16_t)(ay[fresh] + 20));
        sw_sfx(SW_SFX_STAR);
    }
    for (t = 0; t < 300u; t++) {
        if (fresh < 7u && t < 90u) sw_album(fresh, (uint8_t)((t >> 3) & 1u), ax[fresh], ay[fresh]);
        sw_frame();
        if (sw_start_pending() && !NEO_REGISTER8(SW_START_FLAG)) return;
        if (fresh < 7u && t > 100u && (sw_pressed() & SW_BUTTONS)) return;
    }
}

/* A score for the table: three letters, UP and DOWN to choose, A for the next, B back. */
static void NEOGEO_USER name_entry(uint32_t score, uint8_t wonder)
{
    static const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.!? ";
    uint8_t rank = hs_rank(score), slot = 0, pick[3] = { 0, 0, 0 }, name[3], i;
    uint16_t t = 0, left = 30u * 60u;
    if (rank >= HISCORES) return;
    sw_scene(SW_BG_MAP);
    sw_card(1);
    sw_music(SW_MUS_RESULT);
    sw_puts_c(3, "A NEW RECORD!", SW_TXT_GOLD);
    sw_putn(16, 5, score, 8, SW_TXT_WHITE);
    sw_puts_c(8, "ENTER YOUR NAME", SW_TXT_SKY);
    sw_puts_c(24, "UP/DOWN CHOOSE  A NEXT", SW_TXT_PINK);
    sw_cat(0, SW_CAT_MIMI, SW_MOOD_CHEER, 128, 120);
    for (;;) {
        uint16_t in;
        sw_frame();
        t++;
        in = sw_repeat();
        if (in & (SW_UP | SW_RIGHT)) { if (++pick[slot] >= sizeof(letters) - 1u) pick[slot] = 0; sw_sfx(SW_SFX_MOVE); }
        if (in & (SW_DOWN | SW_LEFT)) { pick[slot] = (uint8_t)(pick[slot] ? pick[slot] - 1u : sizeof(letters) - 2u); sw_sfx(SW_SFX_MOVE); }
        if ((sw_pressed() & SW_B) && slot) slot--;
        if (sw_pressed() & (SW_A | SW_START)) { sw_sfx(SW_SFX_PICK); if (++slot >= 3u) break; }
        for (i = 0; i < 3u; i++) {
            char c[2];
            c[0] = (i == slot && (t & 16u)) ? '_' : letters[pick[i]];
            c[1] = 0;
            sw_puts((uint8_t)(17u + (i << 1)), 11, c, i == slot ? SW_TXT_PINK : SW_TXT_GOLD);
        }
        if (!(t & 31u)) sw_cat(0, SW_CAT_MIMI, (t & 32u) ? SW_MOOD_HAPPY : SW_MOOD_CHEER, 128, 120);
        sw_putn(18, 13, (uint32_t)(left / 60u), 2, SW_TXT_WHITE);
        if (!--left) break;
    }
    for (i = 0; i < 3u; i++) name[i] = (uint8_t)letters[pick[i]];
    hs_insert(rank, name, wonder, score);
    hiscores(rank);
}

/* After a level won, the lucky cat's roulette: stop it with a button. The
 * more stars, the better the odds of the big prizes. */
static void NEOGEO_USER lucky_paw(uint8_t stars)
{
    static const uint16_t points[4] = { 500u, 1000u, 3000u, 0u };
    static const uint8_t odds[3][4] = { { 60, 25, 5, 10 }, { 45, 30, 10, 15 }, { 30, 30, 20, 20 } };
    static const char *const labels[4] = { "500", "1000", "3000", "PAW" };
    uint8_t at = 0, pick = 0, wait = 4, tick = 0, steps = 0, stopping = 0, i;
    uint16_t frames = 0, roll;
    sw_scene(SW_BG_MAP);
    sw_card(1);
    sw_sfx(SW_SFX_MEOW);
    sw_puts_c(3, "LUCKY PAW", SW_TXT_GOLD);
    sw_puts_c(24, "PRESS A BUTTON TO STOP", SW_TXT_PINK);
    for (i = 0; i < 4u; i++) {
        uint8_t n = 0;
        while (labels[i][n]) n++;
        sw_prize(i, i, (int16_t)(72 + 48 * i), 136);
        sw_puts((uint8_t)(((88u + 48u * i) >> 3) - (n >> 1)), 22, labels[i], i < 3u ? SW_TXT_GOLD : SW_TXT_PINK);
    }
    sw_prize(4, SW_PRIZE_SELECT, 72, 136);
    sw_lucky_cat(0, 112, 32);
    /* the prize is drawn now; the wheel slows down onto it */
    if (stars < 1u) stars = 1u;
    if (stars > 3u) stars = 3u;
    roll = (uint16_t)(((sw_rand() >> 8) * 100u) >> 8);          /* 0..99 */
    for (pick = 0; pick < 3u; pick++) {
        if (roll < odds[stars - 1u][pick]) break;
        roll = (uint16_t)(roll - odds[stars - 1u][pick]);
    }
    for (;;) {
        sw_frame();
        frames++;
        if (!(frames & 15u)) sw_lucky_cat((uint8_t)((frames >> 4) & 1u), 112, 32);
        if (!stopping && ((frames > 20u && (sw_pressed() & SW_BUTTONS)) || frames > 150u)) {
            stopping = 1;
            steps = (uint8_t)(8u + ((pick - at) & 3u));
        }
        if (++tick >= wait) {
            tick = 0;
            at = (uint8_t)((at + 1u) & 3u);
            sw_prize(4, SW_PRIZE_SELECT, (int16_t)(72 + 48 * at), 136);
            if (stopping) {
                sw_sfx(SW_SFX_TICK);              /* a tick only as it slows down */
                wait = (uint8_t)(wait + 2u);
                if (!--steps) break;
            }
        }
    }
    sw_puts_c(24, "                      ", SW_TXT_PINK);
    if (pick < 3u) {
        sw_puts_c(24, pick == 2u ? "A LUCKY BAG!" : "COINS!", SW_TXT_GOLD);
        s_total += points[pick];
    } else {
        sw_puts_c(24, "ONE MORE PAW NEXT LEVEL!", SW_TXT_PINK);
        if (s_bonus_paw < 3u) s_bonus_paw++;
    }
    sw_word(pick >= 2u ? SW_WORD_LUCKY : SW_WORD_NICE, 160, 104);
    sw_sfx(pick == 2u ? SW_SFX_WIN : SW_SFX_STAR);
    for (frames = 0; frames < 150u; frames++) {
        sw_prize(4, (frames & 8u) ? SW_PRIZE_KINDS : SW_PRIZE_SELECT, (int16_t)(72 + 48 * at), 136);
        if (!(frames & 15u)) sw_lucky_cat((uint8_t)((frames >> 4) & 1u), 112, 32);
        sw_frame();
        if (frames > 40u && (sw_pressed() & SW_BUTTONS)) break;
    }
}

/* ======================================================================
 * the panel
 * ==================================================================== */
static uint32_t s_shown_score;
static uint16_t s_shown_moves, s_shown_goal[NGP_MAX_OBJ];
static uint8_t s_shown_hint;

/* A goal in the panel: its name on one row, what is left of it right-aligned below. */
static void NEOGEO_USER goal_text(uint8_t i, uint8_t y)
{
    const NGPuzzleGoal *g = &s_p.level->goals[i];
    char line[9];
    const char *name = "";
    uint8_t k;
    switch (g->type) {
    case NGP_OBJ_COLOR: name = k_colors[(g->target - 1u) & 7u]; break;
    case NGP_OBJ_ICE: name = "ICE"; break;
    case NGP_OBJ_CRATE: name = "CRATES"; break;
    case NGP_OBJ_SPECIAL: name = "SPECIALS"; break;
    case NGP_OBJ_RESCUE: name = "KITTENS"; break;
    case NGP_OBJ_CLEAR: name = "CLEAR"; break;
    case NGP_OBJ_SCORE: name = "SCORE"; break;
    default: return;
    }
    for (k = 0; k < 8u; k++) line[k] = ' ';
    line[8] = 0;
    for (k = 0; name[k] && k < 8u; k++) line[k] = name[k];
    sw_puts(PANEL_COL, y, line, SW_TXT_WHITE);
    if (g->type == NGP_OBJ_CLEAR) {
        sw_puts(PANEL_COL + 6u, (uint8_t)(y + 1u), ng_puzzle_status(&s_p) == NGP_STATUS_WON ? "OK" : "  ", SW_TXT_MINT);
    } else if (g->type == NGP_OBJ_SCORE) {
        sw_putn(PANEL_COL + 2u, (uint8_t)(y + 1u), (uint32_t)g->count * 100u, 6, SW_TXT_GOLD);
    } else {
        sw_putn(PANEL_COL + 5u, (uint8_t)(y + 1u), s_p.goal_left[i], 3, s_p.goal_left[i] ? SW_TXT_GOLD : SW_TXT_MINT);
    }
}

/* A picture's numbers: hearts, time, score and squares left in the corner
 * box (FIX columns 3-9, rows 2-5), the picture and the buttons in the column
 * on the right (columns 31-36) under the small cat. */
static void NEOGEO_USER logic_panel(uint8_t full, uint8_t level, uint8_t hint)
{
    if (full) {
        char lv[7];
        lv[0] = (char)('1' + chapter_of(level)); lv[1] = ' '; lv[2] = 'O'; lv[3] = 'F'; lv[4] = ' ';
        lv[5] = (char)('0' + SW_CHAPTERS); lv[6] = 0;
        sw_puts(INFO_COL, 9, lv, SW_TXT_GOLD);
        sw_puts(INFO_COL, 11, "A FILL", SW_TXT_PINK);
        sw_puts(INFO_COL, 12, "B MARK", SW_TXT_PINK);
        sw_puts(PANEL_COL, 5, "LEFT", SW_TXT_CLUE);
        s_shown_score = 0xFFFFFFFFu; s_shown_moves = 0xFFFEu; s_shown_goal[0] = 0xFFFEu; s_shown_hint = 0xFFu;
    }
    if (hint != s_shown_hint) {
        sw_puts(INFO_COL, 13, hint ? "C HINT" : "      ", SW_TXT_PINK);
        sw_puts(INFO_COL, 14, "  ", SW_TXT_GOLD);
        if (hint > 1u) { sw_puts(INFO_COL, 14, "X", SW_TXT_GOLD); sw_putn(INFO_COL + 1u, 14, hint, 1, SW_TXT_GOLD); }
        s_shown_hint = hint;
    }
    if (s_p.score != s_shown_score) { sw_putn(PANEL_COL, 4, s_p.score, 7, SW_TXT_CLUE_HI); s_shown_score = s_p.score; }
    if (s_p.time_left != 0xFFFFu) {
        uint16_t secs = 0, t = s_p.time_left;
        while (t >= 60u) { t = (uint16_t)(t - 60u); secs++; }
        if (secs != s_shown_moves) {
            uint8_t m = 0;
            uint16_t r = secs;
            char clock[6];
            while (r >= 60u) { r = (uint16_t)(r - 60u); m++; }
            clock[0] = (char)(m >= 10u ? '9' : '0' + m); clock[1] = ':';
            clock[2] = '0'; while (r >= 10u) { r = (uint16_t)(r - 10u); clock[2]++; }
            clock[3] = (char)('0' + r); clock[4] = 0;
            sw_puts(PANEL_COL, 3, clock, secs <= 10u ? SW_TXT_CLUE_HI : SW_TXT_CLUE);
            s_shown_moves = secs;
        }
    }
    if (s_p.goal_left[0] != s_shown_goal[0]) { sw_putn(PANEL_COL + 4u, 5, s_p.goal_left[0], 3, SW_TXT_CLUE); s_shown_goal[0] = s_p.goal_left[0]; }
}

static void NEOGEO_USER panel(uint8_t full, uint8_t level)
{
    uint8_t i;
    uint16_t moves = s_p.mode == NGP_MODE_MATCH ? s_p.moves_left : s_p.shots_left;
    if (full) {
        char lv[9];
        lv[0] = 'W'; lv[1] = 'O'; lv[2] = 'N'; lv[3] = 'D'; lv[4] = 'E'; lv[5] = 'R'; lv[6] = ' ';
        lv[7] = (char)('1' + chapter_of(level)); lv[8] = 0;
        sw_puts(PANEL_COL, 2, lv, SW_TXT_GOLD);
        lv[0] = 'L'; lv[1] = 'E'; lv[2] = 'V'; lv[3] = 'E'; lv[4] = 'L'; lv[5] = ' ';
        lv[6] = (char)('1' + stage_of(level)); lv[7] = 0;
        sw_puts(PANEL_COL, 3, lv, SW_TXT_WHITE);
        sw_puts(PANEL_COL, 13, "SCORE", SW_TXT_SKY);
        sw_puts(PANEL_COL, 16, s_p.time_left != 0xFFFFu ? "TIME" : s_p.mode == NGP_MODE_MATCH ? "MOVES" : "SHOTS", SW_TXT_SKY);
        sw_puts(PANEL_COL, 19, "GOALS", SW_TXT_SKY);
        sw_puts(PANEL_COL, 26, s_p.mode == NGP_MODE_MATCH ? "D HINT" : "D GUIDE", SW_TXT_PINK);
        s_shown_score = 0xFFFFFFFFu; s_shown_moves = 0xFFFEu;
        for (i = 0; i < NGP_MAX_OBJ; i++) s_shown_goal[i] = 0xFFFEu;
    }
    if (s_p.score != s_shown_score) { sw_putn(PANEL_COL, 14, s_p.score, 8, SW_TXT_GOLD); s_shown_score = s_p.score; }
    if (s_p.time_left != 0xFFFFu) {
        uint16_t secs = 0, t = s_p.time_left;
        while (t >= 60u) { t = (uint16_t)(t - 60u); secs++; }
        if (secs != s_shown_moves) { sw_putn(PANEL_COL, 17, secs, 3, secs <= 10u ? SW_TXT_PINK : SW_TXT_WHITE); s_shown_moves = secs; }
    } else if (moves != s_shown_moves) {
        if (moves == 0xFFFFu) sw_puts(PANEL_COL, 17, "FREE", SW_TXT_WHITE);
        else sw_putn(PANEL_COL, 17, moves, 3, moves <= 3u ? SW_TXT_PINK : SW_TXT_WHITE);
        s_shown_moves = moves;
    }
    for (i = 0; i < 3u; i++) {
        if (s_p.level->goals[i].type == NGP_OBJ_NONE) continue;
        if (s_p.goal_left[i] != s_shown_goal[i] || full) { goal_text(i, (uint8_t)(20u + (i << 1))); s_shown_goal[i] = s_p.goal_left[i]; }
    }
}

/* ======================================================================
 * cats
 * ==================================================================== */
static uint8_t s_cat_id;

static void NEOGEO_USER cat_mood(uint8_t mood, uint8_t frames)
{
    s_cat_slot_mood = mood;
    s_cat_timer = frames;
    sw_cat(0, s_cat_id, mood, s_cat_x, s_cat_y);
}

static void NEOGEO_USER cat_tick(void)
{
    if (s_cat_timer) {
        if (!--s_cat_timer) sw_cat(0, s_cat_id, SW_MOOD_IDLE, s_cat_x, s_cat_y);
        return;
    }
    if (++s_blink >= 150u) {
        s_blink = 0;
        sw_cat(0, s_cat_id, SW_MOOD_BLINK, s_cat_x, s_cat_y);
        s_cat_timer = 8;
    }
}

/* A cheering word over the middle of the board. */
static void NEOGEO_USER cheer(uint8_t word)
{
    if (s_p.mode == NGP_MODE_LOGIC) sw_word(word, 220, 132);
    else sw_word(word, 212, 100);
}

static void NEOGEO_USER react(const NGPuzzleEvent *e)
{
    switch (e->type) {
    case NGP_EV_SWAP_BACK: cat_mood(SW_MOOD_SAD, 40); break;
    case NGP_EV_CASCADE:
        /* chains: nice, great, lovely, then nyan-derful */
        if (e->a >= 2u) cheer(e->a >= 5u ? SW_WORD_AMAZING : e->a == 4u ? SW_WORD_LOVELY : e->a == 3u ? SW_WORD_GREAT : SW_WORD_NICE);
        if (e->a >= 3u) { cat_mood(SW_MOOD_HAPPY, 50); sw_sfx(SW_SFX_MEOW); }
        break;
    case NGP_EV_SPECIAL_MADE: cat_mood(SW_MOOD_WOW, 50); cheer(SW_WORD_GREAT); break;
    case NGP_EV_POP: if (e->a >= 5u) cheer(SW_WORD_NICE); break;
    case NGP_EV_DROP:
        if (e->a >= 3u) cheer(e->a >= 6u ? SW_WORD_AMAZING : SW_WORD_GREAT);
        if (e->a >= 4u) { cat_mood(SW_MOOD_WOW, 50); sw_sfx(SW_SFX_MEOW); }
        break;
    case NGP_EV_MISS: if (e->a >= 3u) cat_mood(SW_MOOD_SAD, 40); break;
    case NGP_EV_RESCUE: cat_mood(SW_MOOD_CHEER, 60); cheer(SW_WORD_LOVELY); break;
    case NGP_EV_SHUFFLE: cat_mood(SW_MOOD_WOW, 40); break;
    case NGP_EV_MISTAKE: cat_mood(SW_MOOD_SAD, 40); break;
    case NGP_EV_LINE: if (!s_cat_timer) cat_mood(SW_MOOD_HAPPY, 30); cheer(SW_WORD_NICE); break;
    case NGP_EV_REVEAL: cat_mood(SW_MOOD_WOW, 40); break;
    default: break;
    }
}

/* ======================================================================
 * a level
 * ==================================================================== */
enum { PLAY_WON = 0, PLAY_LOST, PLAY_DEMO_DONE, PLAY_DATA_ERROR };

/* A cell's column and row, without a divide. */
static void NEOGEO_USER cell_xy(uint8_t c, uint8_t *x, uint8_t *y)
{
    uint8_t r = 0;
    while (c >= s_p.w) { c = (uint8_t)(c - s_p.w); r++; }
    *x = c; *y = r;
}

/* Pictures, the player's turn: A fills and B crosses out (or takes a cross
 * away); held, they go on along the paw's way. A mistake stops the filling
 * until A is let go, so one slip never costs a row of hearts. */
static void NEOGEO_USER logic_apply(uint8_t cell)
{
    uint8_t k = s_p.kind[cell];
    if (s_drag == 1u && k == NGP_MARK_NONE) {
        ng_puzzle_action(&s_p, NGP_CMD_MARK, cell, NGP_MARK_FILL);
        if (s_p.kind[cell] == NGP_MARK_MISS) s_drag = 0;
    } else if (s_drag == 2u && k == NGP_MARK_NONE) {
        ng_puzzle_action(&s_p, NGP_CMD_MARK, cell, NGP_MARK_CROSS);
    } else if (s_drag == 3u && k == NGP_MARK_CROSS) {
        ng_puzzle_action(&s_p, NGP_CMD_MARK, cell, NGP_MARK_NONE);
    }
}

/* The finished picture: shown in its colours corner to corner, with its name. */
static void NEOGEO_USER logic_reveal(const SWPicture *pic)
{
    uint8_t step, t, n;
    char name[23];
    sw_logic_cursor(0, NGP_NO_CELL);
    sw_sfx(SW_SFX_STAR);
    for (step = 1; step <= (uint8_t)(s_p.w + s_p.h); step++) {
        sw_logic_reveal(&s_p, pic->bg, step);
        for (t = 0; t < 3u; t++) { sw_frame(); sw_board_draw(&s_p); }
    }
    /* the name over the column clues */
    for (n = 0; n < 22u && pic->name[n]; n++) name[n] = pic->name[n];
    name[n] = 0;
    ng_fix_clear_rect(10, 2, 21, 4, 0);             /* the column clues give way to its name */
    sw_puts((uint8_t)(21u - ((n + 1u) >> 1)), 3, name, SW_TXT_CLUE_HI);
    sw_word(SW_WORD_DONE, 220, 132);
    sw_sfx(SW_SFX_SPECIAL);                 /* the fanfare waits for the result screen */
    cat_mood(SW_MOOD_CHEER, 200);
    for (t = 0; t < 150u; t++) {
        sw_frame();
        sw_board_draw(&s_p);
        if (t > 40u && (sw_pressed() & SW_BUTTONS)) break;
    }
}

/* demo: 0 player, 1 the hint bot for a while */
static uint8_t NEOGEO_USER play(const NGPuzzleLevel *lv, uint8_t level, uint32_t seed, uint8_t demo,
                                const SWPicture *pic)
{
    uint8_t cursor, cx, cy, picked = 0, hammer, bubble_rainbow, finish = 0, pause = 0;
    uint8_t shown_cursor = NGP_NO_CELL, clues_dirty = 1;
    uint16_t demo_t = 0, demo_wait = 0, hint_t = 0, idle_t = 0, shown_hearts = 0xFFFEu;
    uint8_t chapter = chapter_of(level), logic;
    /* A test build (SW_AUTOPLAY, see README) lets the hint bot play every level, so a
       script can run the whole game in an emulator. */
#ifdef SW_AUTOPLAY
    uint8_t bot = 1;
#else
    uint8_t bot = demo;
#endif
    if (sw_level_init(&s_p, lv, seed) != NGP_OK) return PLAY_DATA_ERROR;
    logic = (uint8_t)(s_p.mode == NGP_MODE_LOGIC);
    /* the paw (hammer, or a picture's hint): one a level, and one more for each lucky paw won */
    hammer = bubble_rainbow = (uint8_t)(1u + (demo ? 0u : s_bonus_paw));
    if (!demo) s_bonus_paw = 0;
    sw_scene(demo ? (logic ? 4u : 1u) : chapter);
    s_cat_id = demo ? (logic ? SW_CAT_KIKO : SW_CAT_MIMI) : sw_chapters[chapter < SW_CHAPTERS ? chapter : 0].cat;
    s_cat_x = logic ? 248u : 24u;
    s_cat_y = logic ? 18u : 32u;
    s_cat_timer = 0;
    s_drag = 0;
    sw_cats_small(logic);
    cat_mood(SW_MOOD_IDLE, 0);
    if (!logic) sw_panel(1);
    if (!sw_board_bind(&s_p)) return PLAY_DATA_ERROR;
    sw_music(logic ? SW_MUS_MAP : s_p.mode == NGP_MODE_MATCH ? SW_MUS_MATCH : SW_MUS_BUBBLE);
    if (logic) logic_panel(1, level, hammer);
    else panel(1, level);
    if (demo) sw_puts(logic ? INFO_COL : PANEL_COL, logic ? 24u : 26u, logic ? "DEMO" : "DEMO    ", SW_TXT_PINK);
    cx = (uint8_t)(s_p.w >> 1); cy = (uint8_t)(s_p.h >> 1);
    cursor = (uint8_t)(cy * s_p.w + cx);
    s_guide = (uint8_t)(dip(3) == 0);
    for (;;) {
        NGPuzzleEvent e;
        uint16_t in;
        sw_frame();
        in = sw_repeat();
        if (demo && sw_start_pending()) return PLAY_DEMO_DONE;
        if (!demo && !ng_sys_is_mvs() && (sw_pressed() & SW_START)) { pause ^= 1u; sw_puts_c(13, pause ? "PAUSE" : "     ", SW_TXT_GOLD); }
        if (pause) continue;
        /* Steering prepares the NEXT shot; the in-flight velocity is fixed.
         * Fire, swap and boosters still require a ready board. */
        if (!bot && s_p.mode == NGP_MODE_BUBBLE) {
            uint16_t aim = (uint16_t)(sw_held() & (SW_LEFT | SW_RIGHT));
            if (aim == SW_LEFT) ng_puzzle_action(&s_p, NGP_CMD_AIM_BY, -6, 0);
            if (aim == SW_RIGHT) ng_puzzle_action(&s_p, NGP_CMD_AIM_BY, 6, 0);
            if (sw_pressed() & SW_UP) ng_puzzle_action(&s_p, NGP_CMD_AIM, 768, 0);
        }
        if (ng_puzzle_status(&s_p) == NGP_STATUS_READY && sw_board_idle()) {
            if (bot && logic) {
                /* the bot knows the picture: the next square to fill, in board order */
                if (++demo_wait >= 8u) {
                    uint8_t c, n = (uint8_t)(s_p.w * s_p.h);
                    demo_wait = 0;
                    for (c = 0; c < n; c++) if (s_p.color[c] && s_p.kind[c] != NGP_MARK_FILL) break;
                    if (c < n) { cursor = c; cell_xy(c, &cx, &cy); ng_puzzle_action(&s_p, NGP_CMD_MARK, c, NGP_MARK_FILL); }
                }
                if (demo && ++demo_t >= 60u * 25u) return PLAY_DEMO_DONE;
            } else if (bot) {
                /* the demonstration plays the engine's own hints, at a human pace */
                if (++demo_wait >= 40u) {
                    demo_wait = 0;
                    if (s_p.mode == NGP_MODE_MATCH) {
                        uint8_t c, d;
                        if (ng_puzzle_hint(&s_p, &c, &d)) { cursor = c; cell_xy(c, &cx, &cy); ng_puzzle_action(&s_p, NGP_CMD_SWAP, c, d); }
                    } else {
                        /* the hint for this bubble or, if it does better, the next one */
                        uint16_t a = 768, a2 = 768;
                        uint8_t h = ng_puzzle_bubble_hint(&s_p, &a);
                        if (h < 2u) {
                            ng_puzzle_action(&s_p, NGP_CMD_SWAP_NEXT, 0, 0);
                            if (ng_puzzle_bubble_hint(&s_p, &a2) > h) { a = a2; h = 1; }
                            else ng_puzzle_action(&s_p, NGP_CMD_SWAP_NEXT, 0, 0);
                        }
                        if (!h) a = (uint16_t)(576u + (((sw_rand() & 0x1FFu) * 3u) >> 2));
                        ng_puzzle_action(&s_p, NGP_CMD_AIM, (int16_t)a, 0);
                        ng_puzzle_action(&s_p, NGP_CMD_FIRE, 0, 0);
                    }
                }
                if (demo && ++demo_t >= 60u * 25u) return PLAY_DEMO_DONE;
            } else if (logic) {
                uint16_t held = sw_held();
                uint8_t moved = 0;
                if ((in & SW_LEFT) && cx) { cx--; moved = 1; }
                else if ((in & SW_RIGHT) && cx + 1u < s_p.w) { cx++; moved = 1; }
                if ((in & SW_UP) && cy) { cy--; moved = 1; }
                else if ((in & SW_DOWN) && cy + 1u < s_p.h) { cy++; moved = 1; }
                if (moved) {
                    cursor = (uint8_t)(cy * s_p.w + cx);
                    if (!s_drag) sw_sfx(SW_SFX_MOVE);
                }
                if (sw_pressed() & SW_A) {
                    /* filling, from wherever it starts: only empty squares take it */
                    s_drag = 1;
                    logic_apply(cursor);
                } else if (sw_pressed() & SW_B) {
                    /* crossing out, or taking crosses away when it starts on one */
                    s_drag = (uint8_t)(s_p.kind[cursor] == NGP_MARK_CROSS ? 3u : 2u);
                    logic_apply(cursor);
                } else if (moved && s_drag && (held & (s_drag == 1u ? SW_A : SW_B))) {
                    logic_apply(cursor);
                }
                if (!(held & (SW_A | SW_B))) s_drag = 0;
                if ((sw_pressed() & SW_C) && hammer &&
                    ng_puzzle_action(&s_p, NGP_CMD_HAMMER, cursor, 0) == NGP_OK) hammer--;
            } else if (s_p.mode == NGP_MODE_MATCH) {
                uint8_t x = cx, y = cy, dir = 0xFFu;
                if (in & SW_LEFT) dir = NGP_DIR_LEFT; else if (in & SW_RIGHT) dir = NGP_DIR_RIGHT;
                else if (in & SW_UP) dir = NGP_DIR_UP; else if (in & SW_DOWN) dir = NGP_DIR_DOWN;
                if (dir != 0xFFu) {
                    if (picked) {
                        picked = 0;
                        ng_puzzle_action(&s_p, NGP_CMD_SWAP, cursor, dir);
                    } else {
                        if (dir == NGP_DIR_LEFT && x) x--;
                        if (dir == NGP_DIR_RIGHT && x + 1u < s_p.w) x++;
                        if (dir == NGP_DIR_UP && y) y--;
                        if (dir == NGP_DIR_DOWN && y + 1u < s_p.h) y++;
                        cx = x; cy = y;
                        cursor = (uint8_t)(y * s_p.w + x);
                        sw_sfx(SW_SFX_MOVE);
                    }
                    idle_t = 0;
                }
                if (sw_pressed() & SW_A) { picked ^= 1u; sw_sfx(SW_SFX_PICK); idle_t = 0; }
                if (sw_pressed() & SW_B) picked = 0;
                if ((sw_pressed() & SW_C) && hammer) {
                    if (ng_puzzle_action(&s_p, NGP_CMD_HAMMER, cursor, 0) == NGP_OK) { hammer--; sw_sfx(SW_SFX_BLAST); cat_mood(SW_MOOD_CHEER, 40); }
                }
                if ((sw_pressed() & SW_D) || ++idle_t > 600u) {
                    uint8_t c, d;
                    if (ng_puzzle_hint(&s_p, &c, &d)) { cursor = c; cell_xy(c, &cx, &cy); hint_t = 60; }
                    idle_t = 0;
                }
            } else {
                if (sw_pressed() & SW_A) ng_puzzle_action(&s_p, NGP_CMD_FIRE, 0, 0);
                if (sw_pressed() & SW_B) { ng_puzzle_action(&s_p, NGP_CMD_SWAP_NEXT, 0, 0); sw_sfx(SW_SFX_MOVE); }
                if ((sw_pressed() & SW_C) && bubble_rainbow) {
                    uint8_t hit = ng_puzzle_bubble_target(&s_p, s_p.angle);
                    if (hit != NGP_NO_CELL && ng_puzzle_action(&s_p, NGP_CMD_HAMMER, hit, 0) == NGP_OK) {
                        bubble_rainbow--; sw_sfx(SW_SFX_BLAST); cat_mood(SW_MOOD_CHEER, 40);
                    } else {
                        sw_sfx(SW_SFX_NOPE);
                    }
                }
                if (sw_pressed() & SW_D) s_guide ^= 1u;
            }
        }
        if (sw_board_idle() || ng_puzzle_flying(&s_p)) ng_puzzle_update(&s_p);
        while (ng_puzzle_event(&s_p, &e)) {
            sw_board_event(&s_p, &e);
            react(&e);
            if (e.type == NGP_EV_LINE || e.type == NGP_EV_REVEAL) clues_dirty = 1;
            if (e.type == NGP_EV_WON || e.type == NGP_EV_LOST) finish = 1;
        }
        /* Notifications can overflow during a large cascade. The engine's
         * terminal state remains authoritative even if its event was lost. */
        if (ng_puzzle_status(&s_p) == NGP_STATUS_WON ||
            ng_puzzle_status(&s_p) == NGP_STATUS_LOST) finish = 1;
        sw_board_draw(&s_p);
        if (logic) {
            if (cursor != shown_cursor || clues_dirty) { sw_logic_clues(&s_p, cursor); shown_cursor = cursor; clues_dirty = 0; }
            if (s_p.moves_left != shown_hearts) { sw_logic_hearts(s_p.moves_left, lv->moves ? lv->moves : 0xFFFFu); shown_hearts = s_p.moves_left; }
            sw_logic_cursor((uint8_t)(ng_puzzle_status(&s_p) == NGP_STATUS_READY), cursor);
            logic_panel(0, level, hammer);
        } else if (s_p.mode == NGP_MODE_MATCH) {
            sw_cursor((uint8_t)(ng_puzzle_status(&s_p) == NGP_STATUS_READY && (!hint_t || (hint_t & 8u))), cursor, picked);
            if (hint_t) hint_t--;
        } else {
            sw_aim(&s_p, (uint8_t)(ng_puzzle_status(&s_p) != NGP_STATUS_WON && ng_puzzle_status(&s_p) != NGP_STATUS_LOST),
                   (uint8_t)(s_guide && ng_puzzle_status(&s_p) == NGP_STATUS_READY));
        }
        if (!logic) panel(0, level);
        cat_tick();
        if (finish && sw_board_idle()) {
            uint8_t t;
            if (logic && ng_puzzle_status(&s_p) == NGP_STATUS_WON) { logic_panel(0, level, hammer); logic_reveal(pic); }
            for (t = 0; t < 30u; t++) { sw_frame(); sw_board_draw(&s_p); }
            break;
        }
    }
    return ng_puzzle_status(&s_p) == NGP_STATUS_WON ? PLAY_WON : PLAY_LOST;
}

/* ======================================================================
 * cards and results
 * ==================================================================== */
static const char *const k_roman[7] = { "I", "II", "III", "IV", "V", "VI", "VII" };

static void NEOGEO_USER chapter_card(uint8_t c)
{
    const SWChapter *ch = &sw_chapters[c];
    sw_scene(c);
    sw_music(SW_MUS_MAP);
    sw_banner(0, 24);
    sw_banner(1, 160);
    sw_puts_c(4, "THE SEVEN WONDERS", SW_TXT_SKY);
    {
        char line[16];
        uint8_t i = 0;
        const char *w = "WONDER ";
        while (w[i]) { line[i] = w[i]; i++; }
        { const char *r = k_roman[c]; uint8_t k = 0; while (r[k]) line[i++] = r[k++]; }
        line[i] = 0;
        sw_puts_c(5, line, SW_TXT_GOLD);
    }
    sw_puts_c(6, ch->name, SW_TXT_WHITE);
    sw_puts_c(21, ch->line1, SW_TXT_WHITE);
    sw_puts_c(23, ch->line2, SW_TXT_MINT);
    {
        /* the stars this wonder has given so far, of fifteen */
        uint8_t l, stars = 0;
        for (l = 0; l < SW_PER_CHAPTER; l++) stars = (uint8_t)(stars + best_stars((uint8_t)(c * SW_PER_CHAPTER + l)));
        ng_fix_put_tile(14, 7, SW_STAR_TILE, SW_TXT_STAR);
        sw_putn(16, 7, stars, 2, SW_TXT_GOLD);
        sw_puts(19, 7, "OF 15", SW_TXT_WHITE);
    }
    sw_cat(0, ch->cat, SW_MOOD_HAPPY, 128, 82);
    sw_wait(200);
}

static void NEOGEO_USER wait_button(void)
{
    uint16_t t;
    sw_puts_c(24, "PRESS A BUTTON", SW_TXT_PINK);
    for (t = 0; t < 600u; t++) { sw_frame(); if (t > 20u && (sw_pressed() & SW_BUTTONS)) break; }
}

/* A picture's card: how the clues work, every time, in three short lines. */
static void NEOGEO_USER picture_card(uint8_t level)
{
    const NGPuzzleLevel *lv = &sw_levels[level];
    sw_puts_c(4, "PICTURE LOGIC", SW_TXT_GOLD);
    sw_puts_c(7, sw_level_tips[level], SW_TXT_WHITE);
    sw_puts_c(10, "NUMBERS ARE RUNS OF SQUARES", SW_TXT_MINT);
    sw_puts_c(11, "IN ITS ROW OR COLUMN, IN ORDER", SW_TXT_MINT);
    sw_puts_c(13, "A FILL   B CROSS OUT   C HINT", SW_TXT_SKY);
    sw_puts_c(16, "FINISH THE PICTURE", SW_TXT_WHITE);
    if (lv->moves) {
        sw_puts(10, 19, "HEARTS", SW_TXT_SKY);
        sw_putn(28, 19, lv->moves, 3, SW_TXT_WHITE);
    }
    if (lv->time_frames) {
        sw_puts(10, 21, "SECONDS", SW_TXT_SKY);
        sw_putn(28, 21, lv->time_frames / 60u, 3, SW_TXT_WHITE);
    }
    wait_button();
}

static void NEOGEO_USER level_card(uint8_t level)
{
    uint8_t i, y = 13;
    const NGPuzzleLevel *lv = &sw_levels[level];
    sw_scene(SW_BG_MAP);
    sw_card(1);
    if (lv->mode == NGP_MODE_LOGIC) { picture_card(level); return; }
    sw_puts_c(4, lv->mode == NGP_MODE_MATCH ? "MATCH AND COLLECT" : "BUBBLE LAUNCH", SW_TXT_GOLD);
    sw_puts_c(7, sw_level_tips[level], SW_TXT_WHITE);
    sw_puts_c(11, "GOALS", SW_TXT_SKY);
    for (i = 0; i < NGP_MAX_OBJ; i++) {
        const NGPuzzleGoal *g = &lv->goals[i];
        const char *t = 0;
        switch (g->type) {
        case NGP_OBJ_COLOR: t = k_colors[(g->target - 1u) & 7u]; break;
        case NGP_OBJ_ICE: t = "BREAK ALL THE ICE"; break;
        case NGP_OBJ_CRATE: t = "BREAK ALL THE CRATES"; break;
        case NGP_OBJ_SPECIAL: t = "MAKE SPECIAL PIECES"; break;
        case NGP_OBJ_RESCUE: t = "FREE THE KITTENS"; break;
        case NGP_OBJ_CLEAR: t = "CLEAR EVERY BUBBLE"; break;
        case NGP_OBJ_SCORE: t = "REACH THE SCORE"; break;
        default: break;
        }
        if (!t) continue;
        sw_puts(10, y, t, SW_TXT_WHITE);
        if (g->type == NGP_OBJ_COLOR || g->type == NGP_OBJ_SPECIAL) sw_putn(28, y, g->count, 3, SW_TXT_GOLD);
        if (g->type == NGP_OBJ_SCORE) sw_putn(26, y, (uint32_t)g->count * 100u, 5, SW_TXT_GOLD);
        y = (uint8_t)(y + 2u);
    }
    if (lv->moves) {
        sw_puts(10, (uint8_t)(y + 1u), lv->mode == NGP_MODE_MATCH ? "MOVES" : "SHOTS", SW_TXT_SKY);
        sw_putn(28, (uint8_t)(y + 1u), lv->moves, 3, SW_TXT_WHITE);
    }
    if (lv->time_frames) {
        sw_puts(10, (uint8_t)(y + 1u), "SECONDS", SW_TXT_SKY);
        sw_putn(28, (uint8_t)(y + 1u), lv->time_frames / 60u, 3, SW_TXT_WHITE);
    }
    wait_button();
}

static void NEOGEO_USER result(uint8_t won, uint8_t level)
{
    uint8_t waited = 0;
    sw_scene(SW_BG_MAP);
    sw_card(1);
    /* the fanfare (or the sad fall) alone, then the result's tune */
    sw_hush();
    sw_sfx(won ? SW_SFX_WIN : SW_SFX_LOSE);
    sw_cat(0, s_cat_id, won ? SW_MOOD_CHEER : SW_MOOD_SAD, 128, 40);
    sw_puts_c(14, won ? "WONDERFUL!" : "NOT THIS TIME", won ? SW_TXT_GOLD : SW_TXT_PINK);
    sw_puts(12, 17, "SCORE", SW_TXT_SKY);
    sw_putn(20, 17, s_p.score, 8, SW_TXT_WHITE);
    if (won) {
        uint8_t i;
        sw_stars(0, 130, 160);
        for (i = 1; i <= s_p.stars; i++) { sw_wait(20); waited = (uint8_t)(waited + 20u); sw_stars(i, 130, 160); sw_sfx(SW_SFX_STAR); }
        if (s_p.score >= best_score(level)) sw_puts_c(23, "A NEW BEST!", SW_TXT_MINT);
        record(level, s_p.score, s_p.stars);
        sw_puts(12, 19, "TOTAL", SW_TXT_SKY);
        sw_putn(20, 19, s_total + s_p.score, 8, SW_TXT_GOLD);
    } else {
        const char *why = "";
        switch (s_p.end_reason) {
        case NGP_END_MOVES: why = "OUT OF MOVES"; break;
        case NGP_END_TIME: why = "OUT OF TIME"; break;
        case NGP_END_LOSS_LINE: why = "THE BUBBLES REACHED THE LINE"; break;
        case NGP_END_SHOTS: why = "OUT OF SHOTS"; break;
        case NGP_END_NO_MOVES: why = "NO MOVES LEFT"; break;
        case NGP_END_MISTAKES: why = "OUT OF HEARTS"; break;
        default: break;
        }
        sw_puts_c(20, why, SW_TXT_WHITE);
    }
    sw_wait(240);
    sw_stars(0, 0, 240);
}

/* 1 to play on */
static uint8_t NEOGEO_USER continue_screen(void)
{
    uint8_t secs = 10;
    uint16_t t = 0;
    sw_scene(SW_BG_MAP);
    sw_card(1);
    sw_music(SW_MUS_OVER);
    sw_cat(0, SW_CAT_MIMI, SW_MOOD_SAD, 128, 48);
    sw_puts_c(15, "CONTINUE?", SW_TXT_GOLD);
    for (;;) {
        sw_frame();
        sw_putn(19, 18, secs, 2, SW_TXT_WHITE);
        if (ng_sys_is_mvs()) sw_puts_c(21, read_p1credit() > 0 ? "PRESS START" : "INSERT COIN", SW_TXT_WHITE);
        else sw_puts_c(21, "PRESS START", SW_TXT_WHITE);
        if (sw_pressed() & (SW_START | SW_A)) {
            if (sw_spend_credit()) { sw_sfx(SW_SFX_START); return 1; }
        }
        if (sw_pressed() & SW_BUTTONS) t = (uint16_t)(t + 30u);   /* buttons hurry the count */
        if (++t >= 60u) { t = 0; if (!secs) return 0; secs--; sw_sfx(SW_SFX_TICK); }
    }
}

static void NEOGEO_USER game_over(void)
{
    sw_scene(SW_BG_MAP);
    sw_card(1);
    sw_music(SW_MUS_OVER);
    sw_puts_c(8, "GAME OVER", SW_TXT_PINK);
    sw_puts(12, 11, "TOTAL", SW_TXT_SKY);
    sw_putn(20, 11, s_total, 8, SW_TXT_GOLD);
    sw_cat(0, SW_CAT_KIKO, SW_MOOD_SAD, 128, 120);
    sw_wait(300);
}

static void NEOGEO_USER ending(void)
{
    static const char *const credits[] = {
        "THE SEVEN WONDERS SHINE AGAIN", "", "MIMI AND KIKO THANK YOU", "", "",
        "SEVEN WONDERS PUZZLES", "EAGLE SOFTWARE", "", "MADE WITH THE NEOGEOSDK",
        "AND ITS PUZZLE ENGINE", "", "THANK YOU FOR PLAYING",
    };
    uint8_t i;
    uint16_t t;
    /* the title, both cats cheering; then the credits on a card */
    sw_scene(SW_BG_TITLE);
    sw_music(SW_MUS_FINALE);
    sw_logo(1, 6);
    for (t = 0; t < 240u; t++) {
        if (!(t & 15u)) {
            uint8_t bob = (uint8_t)((t >> 4) & 1u);
            sw_title_cat(0, SW_CAT_MIMI, 2, 24, (int16_t)(104 - (bob << 1)));
            sw_title_cat(1, SW_CAT_KIKO, 2, 200, (int16_t)(102 + (bob << 1)));
        }
        sw_frame();
        if (sw_start_pending() && !NEO_REGISTER8(SW_START_FLAG)) break;
    }
    sw_logo(0, 0);
    sw_cat_hide(0);
    sw_cat_hide(1);
    sw_card(1);
    sw_cat(0, SW_CAT_MIMI, SW_MOOD_CHEER, 40, 136);
    sw_cat(1, SW_CAT_KIKO, SW_MOOD_CHEER, 216, 136);
    for (i = 0; i < (uint8_t)(sizeof(credits) / sizeof(credits[0])); i++) {
        sw_puts_c((uint8_t)(4u + i), credits[i], i == 5u ? SW_TXT_GOLD : SW_TXT_WHITE);
        sw_wait(40);
    }
    sw_puts(13, 23, "TOTAL", SW_TXT_SKY);
    sw_putn(19, 23, s_total, 8, SW_TXT_GOLD);
    sw_wait(600);
}

/* ======================================================================
 * attract and a game
 * ==================================================================== */
void NEOGEO_USER sw_run_attract(void)
{
    save_init();
    for (;;) {
        uint16_t t;
        sw_scene(SW_BG_TITLE);
        sw_music(SW_MUS_TITLE);
        title_art();
        for (t = 0; t < 420u; t++) {
            sw_puts_c(22, (t & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PRESS START") : "           ", SW_TXT_WHITE);
            title_cats(t);
            sw_frame();
            if (sw_start_pending()) return;
        }
        if (ng_sys_is_eaglebios()) {
            /* EagleBIOS shows the game's own controls on its screen */
            sw_fade_out();
            ng_sys_howtoplay(&k_howto);
            if (sw_start_pending()) return;
        } else {
            how_to_play(420);
            if (sw_start_pending()) return;
        }
        /* the demonstrations take turns: a match board, then a picture */
        NEO_REGISTER8(SW_DEMO_TURN) = (uint8_t)(NEO_REGISTER8(SW_DEMO_TURN) + 1u);
        if (NEO_REGISTER8(SW_DEMO_TURN) & 1u) play(&sw_demo_logic, 0, 0, 1, &sw_demo_picture);
        else play(&sw_demo_level, 0, (uint32_t)(sw_rand() | 1u), 1, &sw_demo_picture);
        if (sw_start_pending()) return;
        {
            /* the boards take turns: the best five, the album, the stars of each wonder */
            uint8_t turn = NEO_REGISTER8(SW_DEMO_TURN);
            while (turn >= 3u) turn = (uint8_t)(turn - 3u);
            if (turn == 0u) hiscores(HISCORES);
            else if (turn == 1u) album(7);
            else best_scores();
        }
        if (sw_start_pending()) return;
    }
}

void NEOGEO_USER sw_run(void)
{
    uint8_t level = 0, tries, max_tries = (uint8_t)(dip(0) + 1u), attempt = 0;
    save_init();
    s_total = 0;
    tries = max_tries;
    if (dip(1) == 0) how_to_play(900);
    while (level < SW_LEVELS) {
        uint8_t won, outcome;
        if (!stage_of(level) && !attempt) chapter_card(chapter_of(level));
        level_card(level);
        outcome = play(&sw_levels[level], level, sw_levels[level].seed + attempt, 0, &sw_pictures[level]);
        if (outcome == PLAY_DATA_ERROR) {
            sw_scene(SW_BG_MAP);
            sw_puts_c(13, "LEVEL DATA ERROR", SW_TXT_PINK);
            sw_wait(180);
            return;
        }
        won = (uint8_t)(outcome == PLAY_WON);
        {
            /* a picture finished for the first time goes into the album */
            uint8_t fresh = (uint8_t)(won && stage_of(level) == SW_PICTURE_STAGE && !best_stars(level));
            result(won, level);
            if (won) {
                s_total += s_p.score;
                if (fresh) album(chapter_of(level));
                lucky_paw(s_p.stars);
                level++;
                attempt = 0;
                continue;
            }
        }
        attempt++;
#ifdef SW_AUTOPLAY
        continue;                     /* the test run retries until it reaches the ending */
#endif
        if (--tries) continue;
        if (!continue_screen()) { game_over(); name_entry(s_total, (uint8_t)(chapter_of(level) + 1u)); return; }
        tries = max_tries;
    }
    ending();
    name_entry(s_total, 8);
}
