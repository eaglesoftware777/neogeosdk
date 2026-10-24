/*
 * sw_game.c — Seven Wonders screens and the flow of a game.
 *
 * Attract: the title, how to play, a demonstration board played by the
 * engine's own hints, the best scores. A credited game: how to play (an
 * operator setting), then chapter by chapter: the chapter card, each level's
 * card, the level, its result. A level lost costs a try; out of tries, a
 * continue (a credit on an arcade board, free on a console) or the end.
 * After the seventh wonder, the ending.
 *
 * Controls: the joystick moves the paw (match) or aims (bubbles); A picks
 * up a piece, then the joystick pushes it into its neighbour, or A fires;
 * B puts the piece down, or swaps the loaded and the next bubble; C is the
 * paw hammer, once a level: it breaks the piece under the paw, or the
 * bubble the aim points at; D shows a hint (match) or switches the aim
 * guide (bubbles). On a console START pauses.
 */
#include "sw.h"
#include "sdk/ng_system.h"

#define SAVE_VERSION  1u
#define SAVE_SIZE     (4u + SW_LEVELS * 5u)

static NGPuzzle s_p;
static uint32_t s_total;
static uint8_t s_cat_slot_mood, s_cat_timer, s_blink;
static uint8_t s_guide;

/* the game's own text for EagleBIOS's HOW TO PLAY screen */
static const NGHowToPlay k_howto = {
    {'H', 'T', 'P', '1'}, 10, 8, "SEVEN WONDERS PUZZLES",
    {
        "JOYSTICK  MOVE THE PAW OR AIM",
        "A  PICK UP A PIECE, THEN PUSH",
        "   IT INTO A NEIGHBOUR",
        "A  FIRE A BUBBLE",
        "B  PUT DOWN / SWAP BUBBLES",
        "C  PAW HAMMER, ONCE A LEVEL",
        "D  A HINT / THE AIM GUIDE",
        "",
        "LINE UP THREE OF A KIND",
        "POP THREE BUBBLES ALIKE",
    }
};

static const char *const k_colors[8] = { "RUBY", "AMBER", "SUNSTONE", "EMERALD", "AQUA", "SAPPHIRE", "AMETHYST", "ROSE" };

/* ======================================================================
 * the save block: best score and stars per level
 * ==================================================================== */
static uint8_t *NEOGEO_USER save(void)
{
    return (uint8_t *)ng_save_data();
}

static void NEOGEO_USER save_init(void)
{
    if (ng_save_capacity() < SAVE_SIZE) return;
    if (!ng_save_valid(SAVE_VERSION, SAVE_SIZE)) {
        uint8_t *d;
        uint16_t i;
        ng_save_format(SAVE_VERSION, SAVE_SIZE);
        d = save();
        for (i = 0; i < SAVE_SIZE; i++) d[i] = 0;
        d[0] = 'S'; d[1] = 'W'; d[2] = 1;
        ng_save_commit();
    }
}

static uint32_t NEOGEO_USER best_score(uint8_t level)
{
    const uint8_t *d = save() + 4u + 5u * level;
    if (ng_save_capacity() < SAVE_SIZE) return 0;
    return ((uint32_t)d[0] << 24) | ((uint32_t)d[1] << 16) | ((uint32_t)d[2] << 8) | d[3];
}

static uint8_t NEOGEO_USER best_stars(uint8_t level)
{
    if (ng_save_capacity() < SAVE_SIZE) return 0;
    return save()[4u + 5u * level + 4u];
}

static void NEOGEO_USER record(uint8_t level, uint32_t score, uint8_t stars)
{
    uint8_t *d;
    if (ng_save_capacity() < SAVE_SIZE) return;
    d = save() + 4u + 5u * level;
    if (score > best_score(level)) {
        d[0] = (uint8_t)(score >> 24); d[1] = (uint8_t)(score >> 16); d[2] = (uint8_t)(score >> 8); d[3] = (uint8_t)score;
    }
    if (stars > d[4]) d[4] = stars;
    ng_save_commit();
}

/* ======================================================================
 * small screens
 * ==================================================================== */
static uint8_t NEOGEO_USER dip(uint8_t n) { return ng_dip_option(n); }

static void NEOGEO_USER copyright(void)
{
    sw_puts_c(27, "(C) 2026 EAGLE SOFTWARE", SW_TXT_WHITE);
}

void NEOGEO_USER sw_title_screen(void)
{
    uint16_t countdown = 15u * 60u, t = 0;
    save_init();
    sw_scene(SW_BG_TITLE);
    sw_music(SW_MUS_TITLE);
    sw_logo(1, 28);
    sw_cat(0, SW_CAT_MIMI, SW_MOOD_HAPPY, 48, 120);
    sw_cat(1, SW_CAT_KIKO, SW_MOOD_HAPPY, 208, 120);
    sw_puts_c(13, "P U Z Z L E S", SW_TXT_GOLD);
    copyright();
    for (;;) {
        if (NEO_REGISTER8(SW_START_FLAG)) break;
        t++;
        if (!ng_sys_is_mvs()) {
            sw_puts_c(23, (t & 32u) ? "PRESS START" : "           ", SW_TXT_WHITE);
            if (sw_start_pending()) break;
        } else if (read_p1credit() > 0) {
            sw_puts_c(23, "PRESS 1P START", SW_TXT_WHITE);
            sw_putn(18, 25, (uint32_t)(countdown / 60u), 2, SW_TXT_GOLD);
            if (countdown) countdown--;
            else sw_auto_start();
        } else {
            sw_puts_c(23, (t & 32u) ? "INSERT COIN" : "           ", SW_TXT_WHITE);
        }
        if ((t & 127u) == 0) {
            sw_cat(0, SW_CAT_MIMI, (uint8_t)((t >> 7) & 1u ? SW_MOOD_BLINK : SW_MOOD_HAPPY), 48, 120);
            sw_cat(1, SW_CAT_KIKO, (uint8_t)((t >> 7) & 1u ? SW_MOOD_HAPPY : SW_MOOD_BLINK), 208, 120);
        }
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
    for (i = 0; i < k_howto.lines; i++) sw_puts(5, (uint8_t)(5u + 2u * i), k_howto.line[i], (uint8_t)(i < 8u ? SW_TXT_WHITE : SW_TXT_MINT));
    sw_cat(0, SW_CAT_MIMI, SW_MOOD_HAPPY, 252, 156);
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

/* ======================================================================
 * the panel
 * ==================================================================== */
static uint32_t s_shown_score;
static uint16_t s_shown_moves, s_shown_goal[NGP_MAX_OBJ];

static void NEOGEO_USER goal_text(uint8_t i, uint8_t y)
{
    const NGPuzzleGoal *g = &s_p.level->goals[i];
    char line[13];
    const char *name = "";
    uint8_t k;
    switch (g->type) {
    case NGP_OBJ_COLOR: name = k_colors[(g->target - 1u) & 7u]; break;
    case NGP_OBJ_ICE: name = "ICE"; break;
    case NGP_OBJ_CRATE: name = "CRATES"; break;
    case NGP_OBJ_SPECIAL: name = "SPECIALS"; break;
    case NGP_OBJ_RESCUE: name = "KITTENS"; break;
    case NGP_OBJ_CLEAR: name = "CLEAR ALL"; break;
    case NGP_OBJ_SCORE: name = "SCORE"; break;
    default: return;
    }
    for (k = 0; k < 12u; k++) line[k] = ' ';
    line[12] = 0;
    for (k = 0; name[k] && k < 9u; k++) line[k] = name[k];
    sw_puts(1, y, line, SW_TXT_WHITE);
    if (g->type == NGP_OBJ_CLEAR) {
        sw_puts(10, y, ng_puzzle_status(&s_p) == NGP_STATUS_WON ? "OK" : "  ", SW_TXT_MINT);
    } else if (g->type == NGP_OBJ_SCORE) {
        sw_putn(7, (uint8_t)(y + 1u), (uint32_t)g->count * 100u, 5, SW_TXT_GOLD);
    } else {
        sw_putn(10, y, s_p.goal_left[i], 2, s_p.goal_left[i] ? SW_TXT_GOLD : SW_TXT_MINT);
    }
}

static void NEOGEO_USER panel(uint8_t full, uint8_t level)
{
    uint8_t i;
    uint16_t moves = s_p.mode == NGP_MODE_MATCH ? s_p.moves_left : s_p.shots_left;
    if (full) {
        char lv[10];
        lv[0] = 'W'; lv[1] = 'O'; lv[2] = 'N'; lv[3] = 'D'; lv[4] = 'E'; lv[5] = 'R'; lv[6] = ' ';
        lv[7] = (char)('1' + (level >> 2)); lv[8] = 0;
        sw_puts(2, 1, lv, SW_TXT_GOLD);
        lv[0] = 'L'; lv[1] = 'E'; lv[2] = 'V'; lv[3] = 'E'; lv[4] = 'L'; lv[5] = ' ';
        lv[6] = (char)('1' + (level >> 2)); lv[7] = '-'; lv[8] = (char)('1' + (level & 3u)); lv[9] = 0;
        sw_puts(2, 2, lv, SW_TXT_WHITE);
        sw_puts(1, 13, "SCORE", SW_TXT_SKY);
        sw_puts(1, 16, s_p.time_left != 0xFFFFu ? "TIME" : s_p.mode == NGP_MODE_MATCH ? "MOVES" : "SHOTS", SW_TXT_SKY);
        sw_puts(1, 19, "GOALS", SW_TXT_SKY);
        sw_puts(1, 26, s_p.mode == NGP_MODE_MATCH ? "D HINT" : "D GUIDE", SW_TXT_PINK);
        s_shown_score = 0xFFFFFFFFu; s_shown_moves = 0xFFFEu;
        for (i = 0; i < NGP_MAX_OBJ; i++) s_shown_goal[i] = 0xFFFEu;
    }
    if (s_p.score != s_shown_score) { sw_putn(2, 14, s_p.score, 8, SW_TXT_GOLD); s_shown_score = s_p.score; }
    if (s_p.time_left != 0xFFFFu) {
        uint16_t secs = 0, t = s_p.time_left;
        while (t >= 60u) { t = (uint16_t)(t - 60u); secs++; }
        if (secs != s_shown_moves) { sw_putn(2, 17, secs, 3, secs <= 10u ? SW_TXT_PINK : SW_TXT_WHITE); s_shown_moves = secs; }
    } else if (moves != s_shown_moves) {
        if (moves == 0xFFFFu) sw_puts(2, 17, "FREE", SW_TXT_WHITE);
        else sw_putn(2, 17, moves, 3, moves <= 3u ? SW_TXT_PINK : SW_TXT_WHITE);
        s_shown_moves = moves;
    }
    for (i = 0; i < NGP_MAX_OBJ; i++) {
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
    sw_cat(0, s_cat_id, mood, 18, 32);
}

static void NEOGEO_USER cat_tick(void)
{
    if (s_cat_timer) {
        if (!--s_cat_timer) sw_cat(0, s_cat_id, SW_MOOD_IDLE, 18, 32);
        return;
    }
    if (++s_blink >= 150u) {
        s_blink = 0;
        sw_cat(0, s_cat_id, SW_MOOD_BLINK, 18, 32);
        s_cat_timer = 8;
    }
}

static void NEOGEO_USER react(const NGPuzzleEvent *e)
{
    switch (e->type) {
    case NGP_EV_SWAP_BACK: cat_mood(SW_MOOD_SAD, 40); break;
    case NGP_EV_CASCADE: if (e->a >= 3u) { cat_mood(SW_MOOD_HAPPY, 50); sw_sfx(SW_SFX_MEOW); } break;
    case NGP_EV_SPECIAL_MADE: cat_mood(SW_MOOD_WOW, 50); break;
    case NGP_EV_DROP: if (e->a >= 4u) { cat_mood(SW_MOOD_WOW, 50); sw_sfx(SW_SFX_MEOW); } break;
    case NGP_EV_MISS: if (e->a >= 3u) cat_mood(SW_MOOD_SAD, 40); break;
    case NGP_EV_RESCUE: cat_mood(SW_MOOD_CHEER, 60); break;
    case NGP_EV_SHUFFLE: cat_mood(SW_MOOD_WOW, 40); break;
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

/* demo: 0 player, 1 the hint bot for a while */
static uint8_t NEOGEO_USER play(const NGPuzzleLevel *lv, uint8_t level, uint32_t seed, uint8_t demo)
{
    uint8_t cursor, cx, cy, picked = 0, hammer = 1, bubble_rainbow = 1, finish = 0, pause = 0;
    uint16_t demo_t = 0, demo_wait = 0, hint_t = 0, idle_t = 0;
    uint8_t chapter = (uint8_t)(level >> 2);
    /* A test build (SW_AUTOPLAY, see README) lets the hint bot play every level, so a
       script can run the whole game in an emulator. */
#ifdef SW_AUTOPLAY
    uint8_t bot = 1;
#else
    uint8_t bot = demo;
#endif
    if (sw_level_init(&s_p, lv, seed) != NGP_OK) return PLAY_DATA_ERROR;
    sw_scene(demo ? 1u : chapter);
    s_cat_id = demo ? SW_CAT_MIMI : sw_chapters[chapter < SW_CHAPTERS ? chapter : 0].cat;
    s_cat_timer = 0;
    cat_mood(SW_MOOD_IDLE, 0);
    sw_panel(1);
    if (!sw_board_bind(&s_p)) return PLAY_DATA_ERROR;
    sw_music(s_p.mode == NGP_MODE_MATCH ? SW_MUS_MATCH : SW_MUS_BUBBLE);
    panel(1, level);
    if (demo) sw_puts(1, 26, "DEMO PLAY", SW_TXT_PINK);
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
            if (bot) {
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
                    if (ng_puzzle_action(&s_p, NGP_CMD_HAMMER, cursor, 0) == NGP_OK) { hammer = 0; sw_sfx(SW_SFX_BLAST); cat_mood(SW_MOOD_CHEER, 40); }
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
                        bubble_rainbow = 0; sw_sfx(SW_SFX_BLAST); cat_mood(SW_MOOD_CHEER, 40);
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
            if (e.type == NGP_EV_WON || e.type == NGP_EV_LOST) finish = 1;
        }
        /* Notifications can overflow during a large cascade. The engine's
         * terminal state remains authoritative even if its event was lost. */
        if (ng_puzzle_status(&s_p) == NGP_STATUS_WON ||
            ng_puzzle_status(&s_p) == NGP_STATUS_LOST) finish = 1;
        sw_board_draw(&s_p);
        if (s_p.mode == NGP_MODE_MATCH) {
            sw_cursor((uint8_t)(ng_puzzle_status(&s_p) == NGP_STATUS_READY && (!hint_t || (hint_t & 8u))), cursor, picked);
            if (hint_t) hint_t--;
        } else {
            sw_aim(&s_p, (uint8_t)(ng_puzzle_status(&s_p) != NGP_STATUS_WON && ng_puzzle_status(&s_p) != NGP_STATUS_LOST),
                   (uint8_t)(s_guide && ng_puzzle_status(&s_p) == NGP_STATUS_READY));
        }
        panel(0, level);
        cat_tick();
        if (finish && sw_board_idle()) {
            uint8_t t;
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
        sw_puts_c(6, line, SW_TXT_GOLD);
    }
    sw_puts_c(7, ch->name, SW_TXT_WHITE);
    sw_puts_c(21, ch->line1, SW_TXT_WHITE);
    sw_puts_c(23, ch->line2, SW_TXT_MINT);
    sw_cat(0, ch->cat, SW_MOOD_HAPPY, 128, 82);
    sw_wait(200);
}

static void NEOGEO_USER level_card(uint8_t level)
{
    uint8_t i, y = 13;
    const NGPuzzleLevel *lv = &sw_levels[level];
    sw_scene(SW_BG_MAP);
    sw_card(1);
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
    sw_puts_c(25, "PRESS A BUTTON", SW_TXT_PINK);
    {
        uint16_t t;
        for (t = 0; t < 600u; t++) { sw_frame(); if (t > 20u && (sw_pressed() & SW_BUTTONS)) break; }
    }
}

static void NEOGEO_USER result(uint8_t won, uint8_t level)
{
    sw_scene(SW_BG_MAP);
    sw_card(1);
    sw_music(won ? SW_MUS_RESULT : SW_MUS_OVER);
    sw_sfx(won ? SW_SFX_WIN : SW_SFX_LOSE);
    sw_cat(0, s_cat_id, won ? SW_MOOD_CHEER : SW_MOOD_SAD, 128, 40);
    sw_puts_c(14, won ? "WONDERFUL!" : "NOT THIS TIME", won ? SW_TXT_GOLD : SW_TXT_PINK);
    sw_puts(12, 17, "SCORE", SW_TXT_SKY);
    sw_putn(20, 17, s_p.score, 8, SW_TXT_WHITE);
    if (won) {
        uint8_t i;
        sw_stars(0, 130, 160);
        for (i = 1; i <= s_p.stars; i++) { sw_wait(20); sw_stars(i, 130, 160); sw_sfx(SW_SFX_STAR); }
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
    sw_scene(SW_BG_TITLE);
    sw_music(SW_MUS_FINALE);
    sw_logo(1, 12);
    sw_cat(0, SW_CAT_MIMI, SW_MOOD_CHEER, 40, 140);
    sw_cat(1, SW_CAT_KIKO, SW_MOOD_CHEER, 216, 140);
    for (i = 0; i < (uint8_t)(sizeof(credits) / sizeof(credits[0])); i++) {
        sw_puts_c((uint8_t)(10u + i), credits[i], i == 5u ? SW_TXT_GOLD : SW_TXT_WHITE);
        sw_wait(40);
    }
    sw_puts(12, 24, "TOTAL", SW_TXT_SKY);
    sw_putn(20, 24, s_total, 8, SW_TXT_GOLD);
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
        sw_logo(1, 28);
        sw_cat(0, SW_CAT_MIMI, SW_MOOD_HAPPY, 48, 120);
        sw_cat(1, SW_CAT_KIKO, SW_MOOD_HAPPY, 208, 120);
        sw_puts_c(13, "P U Z Z L E S", SW_TXT_GOLD);
        copyright();
        for (t = 0; t < 420u; t++) {
            sw_puts_c(23, (t & 32u) ? (ng_sys_is_mvs() ? "INSERT COIN" : "PRESS START") : "           ", SW_TXT_WHITE);
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
        play(&sw_demo_level, 0, (uint32_t)(sw_rand() | 1u), 1);
        if (sw_start_pending()) return;
        best_scores();
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
        if (!(level & 3u) && !attempt) chapter_card((uint8_t)(level >> 2));
        level_card(level);
        outcome = play(&sw_levels[level], level, sw_levels[level].seed + attempt, 0);
        if (outcome == PLAY_DATA_ERROR) {
            sw_scene(SW_BG_MAP);
            sw_puts_c(13, "LEVEL DATA ERROR", SW_TXT_PINK);
            sw_wait(180);
            return;
        }
        won = (uint8_t)(outcome == PLAY_WON);
        result(won, level);
        if (won) {
            s_total += s_p.score;
            level++;
            attempt = 0;
            continue;
        }
        attempt++;
#ifdef SW_AUTOPLAY
        continue;                     /* the test run retries until it reaches the ending */
#endif
        if (--tries) continue;
        if (!continue_screen()) { game_over(); return; }
        tries = max_tries;
    }
    ending();
}
