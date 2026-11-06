/*
 * mj_sol.c -- Mahjong Demo: Mahjong solitaire on the turtle layout.
 *
 * The engine deals so the layout can always be cleared, says which tiles
 * are free (nothing on top, one long side open), removes pairs and keeps
 * the undo list, hints and shuffles. This file draws the 144 tiles as
 * sprites and moves the cursor.
 *
 * Tiles are drawn far to near: by layer, then row, then column, each a
 * sprite in that order, a layer shifted up and left so the sides of the
 * tiles beneath show. A tile's look tells its state: blocked tiles are
 * dimmer, the picked tile warm, a hinted pair cool; the cursor's tile is
 * lifted and blinks.
 *
 * Controls: the stick moves between free tiles, A picks (a second tile
 * that matches removes the pair), B undoes, C shows a pair, D shuffles
 * what is left, START pauses (continue, deal again, back to the menu).
 */
#include "mj.h"

static NGMahjongSol s_s;
static NGMahjongPos s_pos[NGMS_MAX_TILES];
static uint8_t s_order[NGMS_MAX_TILES];      /* draw order: s_order[k] is the tile in sprite slot k */
static uint8_t s_slot[NGMS_MAX_TILES];       /* and back */
static uint8_t s_free[NGMS_MAX_TILES];
static int16_t s_ox, s_oy;
static uint8_t s_cursor, s_hint_a = NGMS_NONE, s_hint_b = NGMS_NONE;
static uint16_t s_time;                      /* seconds */
static uint8_t s_tick;

#define TOP_Y   20
#define STEP_X  8       /* pixels per half tile across */
#define STEP_Y  11      /* and down: rows overlap two pixels */
#define LIFT    3       /* a layer's shift up and left */

static int16_t NEOGEO_USER tile_x(uint8_t i) { return (int16_t)(s_ox + STEP_X * s_pos[i].x - LIFT * s_pos[i].z); }
static int16_t NEOGEO_USER tile_y(uint8_t i) { return (int16_t)(s_oy + STEP_Y * s_pos[i].y - LIFT * s_pos[i].z); }

static uint8_t NEOGEO_USER gone(uint8_t i)
{
    return (uint8_t)(s_s.gone[i >> 3] & (1u << (i & 7u)));
}

/* Layer, row, column: the order the sprites are given out in. */
static void NEOGEO_USER sort_layout(void)
{
    uint8_t i, j, n = s_s.n, xmax = 0, ymax = 0;
    for (i = 0; i < n; i++) s_order[i] = i;
    for (i = 1; i < n; i++) {                          /* insertion sort: once a deal */
        uint8_t t = s_order[i];
        uint16_t kt = (uint16_t)(((uint16_t)s_pos[t].z << 12) | ((uint16_t)s_pos[t].y << 6) | s_pos[t].x);
        j = i;
        while (j > 0) {
            uint8_t u = s_order[j - 1u];
            uint16_t ku = (uint16_t)(((uint16_t)s_pos[u].z << 12) | ((uint16_t)s_pos[u].y << 6) | s_pos[u].x);
            if (ku <= kt) break;
            s_order[j] = u;
            j--;
        }
        s_order[j] = t;
    }
    for (i = 0; i < n; i++) {
        s_slot[s_order[i]] = i;
        if (s_pos[i].x > xmax) xmax = s_pos[i].x;
        if (s_pos[i].y > ymax) ymax = s_pos[i].y;
    }
    s_ox = (int16_t)((320 - STEP_X * (xmax + 2)) >> 1);
    s_oy = (int16_t)(TOP_Y + ((200 - TOP_Y - STEP_Y * (ymax + 2)) >> 1));
}

static void NEOGEO_USER find_free(void)
{
    uint8_t i;
    for (i = 0; i < s_s.n; i++) s_free[i] = (uint8_t)(!gone(i) && ng_mjsol_is_free(&s_s, i));
}

static void NEOGEO_USER draw_tile(uint8_t i)
{
    uint8_t look, lift = 0;
    if (gone(i)) { mj_tile_hide(s_slot[i]); return; }
    look = s_free[i] ? MJ_LOOK_NORMAL : MJ_LOOK_BLOCKED;
    if (i == s_s.selected) look = MJ_LOOK_SELECTED;
    if (i == s_hint_a || i == s_hint_b) look = MJ_LOOK_HINT;
    if (i == s_cursor) {
        lift = LIFT;
        if ((s_tick & 16u) && look == MJ_LOOK_NORMAL) look = MJ_LOOK_SELECTED;
    }
    mj_tile(s_slot[i], s_s.kind[i], tile_x(i), (int16_t)(tile_y(i) - lift), look);
}

static void NEOGEO_USER draw_all(void)
{
    uint8_t k;
    for (k = 0; k < s_s.n; k++) draw_tile(s_order[k]);
}

static void NEOGEO_USER status(void)
{
    char t[6];
    uint16_t m = 0, sec = s_time;
    uint8_t mt = 0, st = 0;
    while (sec >= 60u) { sec = (uint16_t)(sec - 60u); m++; }
    while (m >= 10u) { m = (uint16_t)(m - 10u); if (++mt >= 10u) mt = 0; }
    while (sec >= 10u) { sec = (uint16_t)(sec - 10u); st++; }
    mj_puts(3, 0, "TILES", MJ_TXT_GOLD);
    mj_putn(9, 0, s_s.left, 3, MJ_TXT_WHITE);
    mj_puts(14, 0, "PAIRS", MJ_TXT_GOLD);
    mj_putn(20, 0, ng_mjsol_free_pairs(&s_s), 2, MJ_TXT_WHITE);
    mj_puts(24, 0, "TIME", MJ_TXT_GOLD);
    t[0] = (char)('0' + mt); t[1] = (char)('0' + m); t[2] = ':';
    t[3] = (char)('0' + st); t[4] = (char)('0' + sec); t[5] = 0;
    mj_puts(29, 0, t, MJ_TXT_WHITE);
}

/* The free tile nearest in a direction (dx, dy = -1, 0, 1), or the cursor. */
static uint8_t NEOGEO_USER step(int8_t dx, int8_t dy)
{
    int16_t cx = tile_x(s_cursor), cy = tile_y(s_cursor), best = 0x7FFF;
    uint8_t i, pick = s_cursor;
    for (i = 0; i < s_s.n; i++) {
        int16_t ex, ey, along, across, score;
        if (!s_free[i] || i == s_cursor) continue;
        ex = (int16_t)(tile_x(i) - cx);
        ey = (int16_t)(tile_y(i) - cy);
        along = dx ? (dx > 0 ? ex : (int16_t)-ex) : (dy > 0 ? ey : (int16_t)-ey);
        across = (int16_t)(dx ? (ey < 0 ? -ey : ey) : (ex < 0 ? -ex : ex));
        if (along <= 4) continue;
        score = (int16_t)(along + 2 * across);
        if (score < best) { best = score; pick = i; }
    }
    return pick;
}

static uint8_t NEOGEO_USER any_free(void)
{
    uint8_t k;
    for (k = 0; k < s_s.n; k++) if (s_free[s_order[s_s.n - 1u - k]]) return s_order[s_s.n - 1u - k];
    return 0;
}

static void NEOGEO_USER deal(uint32_t seed)
{
    uint8_t n = ng_mjsol_layout_turtle(s_pos);
    ng_mjsol_init(&s_s, s_pos, n, seed);
    sort_layout();
    find_free();
    s_hint_a = s_hint_b = NGMS_NONE;
    s_cursor = any_free();
    s_time = 0;
}

static void NEOGEO_USER message(const char *text, uint8_t pal)
{
    mj_line_c(27, text, pal);
}

/* 0 play on, 1 deal again, 2 back to the menu */
static uint8_t NEOGEO_USER pause_menu(void)
{
    static const char *const item[3] = { "CONTINUE", "DEAL AGAIN", "BACK TO THE MENU" };
    uint8_t sel = 0, i;
    for (;;) {
        for (i = 0; i < 3u; i++) {
            mj_line_c((uint8_t)(11u + 2u * i), item[i], i == sel ? MJ_TXT_GOLD : MJ_TXT_WHITE);
        }
        mj_frame();
        if (mj_quit_requested()) return 2;
        if (mj_repeat() & MJ_UP && sel) { sel--; mj_sfx(MJ_SFX_MOVE); }
        if (mj_repeat() & MJ_DOWN && sel < 2u) { sel++; mj_sfx(MJ_SFX_MOVE); }
        if (mj_pressed() & (MJ_A | MJ_START)) break;
        if (mj_pressed() & MJ_B) { sel = 0; break; }
    }
    for (i = 0; i < 3u; i++) mj_line_c((uint8_t)(11u + 2u * i), 0, MJ_TXT_WHITE);
    return sel;
}

static void NEOGEO_USER events(void)
{
    NGMahjongSolEvent e;
    while (ng_mjsol_event(&s_s, &e)) {
        switch (e.type) {
        case NGMS_EV_SELECT: mj_sfx(MJ_SFX_SELECT); break;
        case NGMS_EV_DESELECT: mj_sfx(MJ_SFX_MOVE); break;
        case NGMS_EV_PAIR: mj_sfx(MJ_SFX_PAIR); message(0, MJ_TXT_WHITE); break;
        case NGMS_EV_MISMATCH: mj_sfx(MJ_SFX_NOPE); message("NOT A PAIR", MJ_TXT_RED); break;
        case NGMS_EV_BLOCKED: mj_sfx(MJ_SFX_NOPE); message("THAT TILE IS NOT FREE", MJ_TXT_RED); break;
        case NGMS_EV_UNDO: mj_sfx(MJ_SFX_UNDO); break;
        case NGMS_EV_SHUFFLE: mj_sfx(MJ_SFX_SHUFFLE); message("SHUFFLED", MJ_TXT_SKY); break;
        case NGMS_EV_STUCK: mj_sfx(MJ_SFX_EXHAUSTED); message("NO PAIRS LEFT: D SHUFFLES", MJ_TXT_GOLD); break;
        default: break;
        }
    }
}

void NEOGEO_USER mj_sol_run(void)
{
    uint8_t frames = 0;
    deal(((uint32_t)mj_frame() << 16) ^ 0x51A7E5u ^ mj_held());
    mj_screen(0);
    draw_all();
    status();
    message("A PICK  B UNDO  C HINT  D SHUFFLE", MJ_TXT_GREY);
    mj_reveal();
    mj_music(MJ_MUS_SOLITAIRE);
    for (;;) {
        uint16_t in;
        uint8_t was = s_cursor, changed = 0;
        mj_frame();
        if (mj_quit_requested()) return;
        s_tick++;
        if (++frames >= 60u) { frames = 0; if (s_s.status == NGMS_PLAYING) s_time++; status(); }
        in = mj_repeat();
        if (in & MJ_LEFT) s_cursor = step(-1, 0);
        if (in & MJ_RIGHT) s_cursor = step(1, 0);
        if (in & MJ_UP) s_cursor = step(0, -1);
        if (in & MJ_DOWN) s_cursor = step(0, 1);
        if (s_cursor != was) { mj_sfx(MJ_SFX_MOVE); draw_tile(was); }
        if (in & MJ_A) {
            uint8_t a = s_s.selected;
            ng_mjsol_select(&s_s, s_cursor);
            s_hint_a = s_hint_b = NGMS_NONE;
            changed = 1;
            if (a != NGMS_NONE && a != s_cursor && gone(a)) {
                find_free();
                if (!s_free[s_cursor] || gone(s_cursor)) s_cursor = step(1, 0) != s_cursor ? step(1, 0) : any_free();
            }
        }
        if (in & MJ_B) { if (ng_mjsol_undo(&s_s) == NGM_OK) changed = 1; else mj_sfx(MJ_SFX_NOPE); }
        if (in & MJ_C) {
            if (ng_mjsol_hint(&s_s, &s_hint_a, &s_hint_b)) {        /* to the pair's other tile once one is picked */
                mj_sfx(MJ_SFX_HINT);
                s_cursor = s_s.selected == s_hint_a ? s_hint_b : s_hint_a;
                changed = 1;
            }
            else mj_sfx(MJ_SFX_NOPE);
        }
        if (in & MJ_D) { if (ng_mjsol_shuffle(&s_s) == NGM_OK) changed = 1; }
        if (mj_pressed() & MJ_START) {
            uint8_t r = pause_menu();
            if (r == 2u) return;
            if (r == 1u) { deal(((uint32_t)mj_frame() << 16) ^ s_s.rng); changed = 1; mj_sfx(MJ_SFX_SHUFFLE); }
        }
        events();
        if (changed) {
            find_free();
            if (gone(s_cursor) || !s_free[s_cursor]) s_cursor = any_free();
            draw_all();
            status();
        } else draw_tile(s_cursor);
        if (s_s.status == NGMS_CLEARED) {
            mj_sfx(MJ_SFX_CLEARED);
            mj_music(MJ_MUS_RESULT);
            mj_line_c(11, "THE TURTLE IS CLEARED!", MJ_TXT_GOLD);
            mj_line_c(13, "WELL PLAYED", MJ_TXT_WHITE);
            status();
            mj_wait_button(1800);
            return;
        }
    }
}
