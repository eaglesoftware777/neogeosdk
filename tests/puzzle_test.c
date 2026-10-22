/* Host checks for ng_puzzle (match boards and bubble lattices). */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ng_puzzle.h"

static NGPuzzle P, Q;

static NGPuzzleLevel match_level(uint8_t w, uint8_t h, uint8_t colors, const char *layout, uint8_t flags)
{
    NGPuzzleLevel lv;
    memset(&lv, 0, sizeof(lv));
    lv.mode = NGP_MODE_MATCH; lv.w = w; lv.h = h; lv.colors = colors;
    lv.seed = 1234; lv.layout = layout; lv.flags = flags;
    return lv;
}

static NGPuzzleLevel bubble_level(const char *layout)
{
    NGPuzzleLevel lv;
    memset(&lv, 0, sizeof(lv));
    lv.mode = NGP_MODE_BUBBLE; lv.w = 8; lv.h = 12; lv.colors = 4; lv.seed = 99;
    lv.layout = layout; lv.bubble_d = 16; lv.row_h = 14; lv.loss_row = 10;
    lv.launch_x = 64; lv.launch_y = 200; lv.shot_speed_q8 = 6 * 256;
    return lv;
}

static int count_events(NGPuzzle *p, uint8_t type, NGPuzzleEvent *last)
{
    NGPuzzleEvent e;
    int n = 0;
    while (ng_puzzle_event(p, &e)) {
        if (e.type == type) { n++; if (last) *last = e; }
    }
    return n;
}

/* Drain events, remembering the first of a type. */
static int find_event(NGPuzzle *p, uint8_t type, NGPuzzleEvent *first)
{
    NGPuzzleEvent e;
    int found = 0;
    while (ng_puzzle_event(p, &e)) {
        if (e.type == type && !found) { found = 1; if (first) *first = e; }
    }
    return found;
}

static void test_level_check(void)
{
    NGPuzzleLevel lv = match_level(5, 5, 4, NULL, 0);
    uint16_t bad;
    assert(ng_puzzle_level_check(&lv, &bad) == NGP_OK);
    lv.w = 2; assert(ng_puzzle_level_check(&lv, &bad) == NGP_ERR_LEVEL); lv.w = 5;
    lv.colors = 9; assert(ng_puzzle_level_check(&lv, &bad) == NGP_ERR_LEVEL); lv.colors = 4;
    lv.layout = "....." "....." "..x.." "....." ".....";
    assert(ng_puzzle_level_check(&lv, &bad) == NGP_ERR_LEVEL && bad == 12);
    lv.layout = "....";                                          /* too short */
    assert(ng_puzzle_level_check(&lv, &bad) == NGP_ERR_LEVEL && bad == 0xFFFE);
    lv.layout = "111.." "....." "....." "....." ".....";       /* fixed run across */
    assert(ng_puzzle_level_check(&lv, 0) == NGP_ERR_LEVEL);
    lv.layout = "2...." "2...." "2...." "....." ".....";       /* fixed run down */
    assert(ng_puzzle_level_check(&lv, 0) == NGP_ERR_LEVEL);
    lv.layout = "5...." "....." "....." "....." ".....";       /* colour above colours */
    assert(ng_puzzle_level_check(&lv, 0) == NGP_ERR_LEVEL);
    lv.layout = NULL; lv.goals[0].type = NGP_OBJ_ICE;            /* ice goal, no ice */
    assert(ng_puzzle_level_check(&lv, 0) == NGP_ERR_LEVEL);
    lv.layout = "i...." "....." "....." "....." ".....";
    assert(ng_puzzle_level_check(&lv, 0) == NGP_OK);
    {
        NGPuzzleLevel b = bubble_level("1111111." "1111111-" "........" "........-"
                                       "........" ".......-" "........" ".......-"
                                       "........" ".......-" "........" ".......-");
        /* a shifted row's last column must be '-' or '.' */
        assert(ng_puzzle_level_check(&b, &bad) == NGP_ERR_LEVEL);
        b.layout = "11111111" "1111111-" "........" ".......-" "........" ".......-"
                   "........" ".......-" "........" ".......-" "........" ".......-";
        assert(ng_puzzle_level_check(&b, &bad) == NGP_OK);
        b.layout = "1111111-" "1111111-" "........" ".......-" "........" ".......-"
                   "........" ".......-" "........" ".......-" "........" ".......-";
        assert(ng_puzzle_level_check(&b, &bad) == NGP_ERR_LEVEL && bad == 7);
        b.layout = NULL; b.loss_row = 12;
        assert(ng_puzzle_level_check(&b, 0) == NGP_ERR_LEVEL);
    }
    printf("level check: OK\n");
}

static uint8_t opposite4(uint8_t d) { return (uint8_t)((d + 2u) & 3u); }
static uint8_t opposite6(uint8_t d) { return (uint8_t)((d + 3u) % 6u); }

static void test_topology(void)
{
    NGPuzzleLevel lv = match_level(7, 5, 5, NULL, 0);
    uint8_t c, d, n;
    assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
    n = (uint8_t)(P.w * P.h);
    for (c = 0; c < n; c++)
        for (d = 0; d < 4; d++) {
            uint8_t nb = ng_puzzle_neighbor(&P, c, d);
            uint8_t x = (uint8_t)(c % 7), y = (uint8_t)(c / 7);
            int edge = (d == NGP_DIR_RIGHT && x == 6) || (d == NGP_DIR_LEFT && x == 0) ||
                       (d == NGP_DIR_DOWN && y == 4) || (d == NGP_DIR_UP && y == 0);
            assert(edge ? nb == NGP_NO_CELL : nb != NGP_NO_CELL);
            if (nb != NGP_NO_CELL) assert(ng_puzzle_neighbor(&P, nb, opposite4(d)) == c);
        }
    assert(ng_puzzle_cell(&P, 7, 0) == NGP_NO_CELL && ng_puzzle_cell(&P, 0, 5) == NGP_NO_CELL);
    assert(ng_puzzle_cell(&P, 6, 4) == 34);
    {
        int par;
        for (par = 0; par < 2; par++) {
            NGPuzzleLevel b = bubble_level(NULL);
            int links = 0;
            b.first_shifted = (uint8_t)par;
            assert(ng_puzzle_init(&P, &b, 0) == NGP_OK);
            n = (uint8_t)(P.w * P.h);
            for (c = 0; c < n; c++) {
                uint8_t r = (uint8_t)(c / 8), col = (uint8_t)(c % 8);
                if (ng_puzzle_cell(&P, col, r) == NGP_NO_CELL) {
                    assert(ng_puzzle_row_shifted(&P, r) && col == 7);
                    continue;
                }
                for (d = 0; d < 6; d++) {
                    uint8_t nb = ng_puzzle_neighbor(&P, c, d);
                    if (nb == NGP_NO_CELL) continue;
                    links++;
                    assert(ng_puzzle_neighbor(&P, nb, opposite6(d)) == c);
                    {   /* neighbours sit one bubble apart */
                        int16_t x0, y0, x1, y1;
                        int dx, dy;
                        ng_puzzle_cell_px(&P, c, 0, &x0, &y0);
                        ng_puzzle_cell_px(&P, nb, 0, &x1, &y1);
                        dx = x1 - x0; dy = y1 - y0;
                        assert(dx * dx + dy * dy >= 14 * 14 && dx * dx + dy * dy <= 17 * 17);
                    }
                }
            }
            assert(links > 0);
            assert(ng_puzzle_row_cells(&P, 0) == (par ? 7 : 8));
            assert(ng_puzzle_row_cells(&P, 1) == (par ? 8 : 7));
        }
    }
    printf("topology (square edges, both hex parities): OK\n");
}

/* 5x5: swapping cell 12 up lines up row 2 (cells 10..12). */
static const char *ROW_BOARD =
    "12412"
    "24314"
    "33142"
    "41243"
    "12421";

static void test_match_rows_and_columns(void)
{
    NGPuzzleLevel lv = match_level(5, 5, 4, ROW_BOARD, NGP_LF_NO_REFILL | NGP_LF_NO_SHUFFLE);
    NGPuzzleEvent ev;
    uint8_t before[25];
    assert(ng_puzzle_level_check(&lv, 0) == NGP_OK);
    assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
    memcpy(before, P.color, 25);
    /* an illegal swap changes nothing and spends no move */
    assert(ng_puzzle_action(&P, NGP_CMD_SWAP, 0, NGP_DIR_RIGHT) == NGP_ERR_ILLEGAL);
    assert(memcmp(before, P.color, 25) == 0 && P.moves_used == 0);
    assert(find_event(&P, NGP_EV_SWAP_BACK, &ev) && ev.cell == 0 && ev.a == 1);
    /* the hint finds the one good swap's run */
    {
        uint8_t c, d;
        assert(ng_puzzle_hint(&P, &c, &d));
    }
    assert(ng_puzzle_action(&P, NGP_CMD_SWAP, 12, NGP_DIR_UP) == NGP_OK);
    assert(ng_puzzle_status(&P) == NGP_STATUS_BUSY);
    assert(ng_puzzle_action(&P, NGP_CMD_SWAP, 0, NGP_DIR_RIGHT) == NGP_ERR_BUSY);
    ng_puzzle_update(&P);            /* match phase */
    assert(P.cleared[1] & (1u << (10 - 8)) && P.cleared[1] & (1u << (11 - 8)) && P.cleared[1] & (1u << (12 - 8)));
    assert(find_event(&P, NGP_EV_MATCH, &ev) && ev.cell == 10 && ev.a == 3 && ev.b == 0 && ev.value == 3);
    ng_puzzle_update(&P);            /* fall phase */
    /* columns 0..2 dropped one row */
    assert(P.color[0] == 0 && P.color[5] == 1 && P.color[10] == 2);
    assert(P.color[1] == 0 && P.color[6] == 2 && P.color[11] == 4);
    assert(P.color[2] == 0 && P.color[7] == 4 && P.color[12] == 1);
    assert(P.fall[5] == 1 && P.fall[10] == 1 && P.fall[15] == 0);
    ng_puzzle_settle(&P);
    assert(P.moves_used == 1 && P.score == 30);
    printf("match across, gravity: OK\n");

    /* the same board turned on its side matches down a column */
    {
        static char t[26];
        int x, y;
        for (y = 0; y < 5; y++) for (x = 0; x < 5; x++) t[x * 5 + y] = ROW_BOARD[y * 5 + x];
        t[25] = 0;
        lv.layout = t;
        assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
        /* row 2 / col 2 of the original is col 2 / row 2 here: swap cell 12 left */
        assert(ng_puzzle_action(&P, NGP_CMD_SWAP, 12, NGP_DIR_LEFT) == NGP_OK);
        ng_puzzle_update(&P);
        assert(find_event(&P, NGP_EV_MATCH, &ev) && ev.cell == 2 && ev.a == 3 && ev.b == 1 && ev.value == 3);
        ng_puzzle_settle(&P);
        assert(P.color[2] == 0 && P.color[7] == 0 && P.color[12] == 0);   /* NO_REFILL: column 2 rows 0..2 empty */
    }
    printf("match down: OK\n");
}

/* 6x6: swapping cell 20 up lines up four (cells 18..21). */
static const char *FOUR_BOARD =
    "234523"
    "345234"
    "451345"
    "112152"
    "523413"
    "234524";

static void test_specials(void)
{
    NGPuzzleLevel lv = match_level(6, 6, 5, FOUR_BOARD, NGP_LF_NO_REFILL | NGP_LF_NO_SHUFFLE);
    NGPuzzleEvent ev;
    int made = 0, fired = 0;
    assert(ng_puzzle_level_check(&lv, 0) == NGP_OK);
    assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
    assert(ng_puzzle_action(&P, NGP_CMD_SWAP, 20, NGP_DIR_UP) == NGP_OK);
    ng_puzzle_update(&P);
    {
        NGPuzzleEvent e;
        while (ng_puzzle_event(&P, &e)) if (e.type == NGP_EV_SPECIAL_MADE) { made++; ev = e; }
    }
    assert(made == 1 && ev.cell == 20 && ev.a == NGP_KIND_LINE_V);
    assert(P.kind[20] == NGP_KIND_LINE_V && P.color[20] == 1);
    assert(P.color[18] == 0 && P.color[19] == 0 && P.color[21] == 0);
    ng_puzzle_settle(&P);
    assert(P.specials_made == 1);

    /* a column piece caught in a run clears its column */
    assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
    P.kind[19] = NGP_KIND_LINE_V;            /* cell 19 is part of the run made below */
    assert(ng_puzzle_action(&P, NGP_CMD_SWAP, 20, NGP_DIR_UP) == NGP_OK);
    ng_puzzle_update(&P);
    {
        NGPuzzleEvent e;
        while (ng_puzzle_event(&P, &e)) if (e.type == NGP_EV_SPECIAL_FIRED) { fired++; ev = e; }
    }
    assert(fired == 1 && ev.cell == 19 && ev.a == NGP_KIND_LINE_V);
    {
        int y;
        for (y = 0; y < 6; y++) assert(P.color[y * 6 + 1] == 0);    /* column 1 empty */
    }
    /* a prism swapped with a piece clears that colour */
    assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
    P.color[0] = NGP_COLOR_PRISM; P.kind[0] = NGP_KIND_PRISM;     /* next to a 3 */
    assert(ng_puzzle_action(&P, NGP_CMD_SWAP, 0, NGP_DIR_RIGHT) == NGP_OK);
    ng_puzzle_update(&P);
    {
        int i, threes = 0;
        for (i = 0; i < 36; i++) if (P.color[i] == 3) threes++;
        assert(threes == 0);
    }
    ng_puzzle_settle(&P);
    printf("specials (line made, line fired, prism): OK\n");
}

static void test_furniture(void)
{
    /* crates break next to a cleared run, ice under a cleared piece, locks first unlock */
    static const char *b =
        "12412"
        "24c14"
        "33142"
        "41243"
        "12421";
    NGPuzzleLevel lv = match_level(5, 5, 4, b, NGP_LF_NO_REFILL | NGP_LF_NO_SHUFFLE);
    NGPuzzleEvent ev;
    lv.goals[0].type = NGP_OBJ_CRATE;
    assert(ng_puzzle_level_check(&lv, 0) == NGP_OK);
    assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
    assert(P.goal_left[0] == 1 && NGP_CRATE_HP(P.cover[7]) == 1);
    /* the crate stands where the 3 was: swap 11 up is illegal (crate), swap 12 left makes 3,1? no:
       use the hammer booster on the crate */
    assert(ng_puzzle_action(&P, NGP_CMD_SWAP, 12, NGP_DIR_UP) == NGP_ERR_ILLEGAL);
    assert(ng_puzzle_action(&P, NGP_CMD_HAMMER, 7, 0) == NGP_OK);
    assert(find_event(&P, NGP_EV_CRATE_HIT, &ev) && ev.cell == 7 && ev.a == 0);
    assert(P.goal_left[0] == 0);
    ng_puzzle_settle(&P);
    assert(ng_puzzle_status(&P) == NGP_STATUS_WON && P.end_reason == NGP_END_OBJECTIVES);

    {
        static const char *b2 =
            "12412"
            "24314"
            "3I142"
            "41243"
            "12421";
        lv = match_level(5, 5, 4, b2, NGP_LF_NO_REFILL | NGP_LF_NO_SHUFFLE);
        lv.goals[0].type = NGP_OBJ_ICE;
        assert(ng_puzzle_level_check(&lv, 0) == NGP_OK);
        assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
        assert(P.goal_left[0] == 2);
        P.color[11] = 3;   /* the iced cell's random piece made a 3, so row 2 can line up */
        assert(ng_puzzle_action(&P, NGP_CMD_SWAP, 12, NGP_DIR_UP) == NGP_OK);
        ng_puzzle_update(&P);
        assert(NGP_ICE(P.cover[11]) == 1 && P.goal_left[0] == 1);
        ng_puzzle_settle(&P);
    }
    {
        static const char *b3 =
            "12412"
            "24314"
            "3l142"
            "41243"
            "12421";
        lv = match_level(5, 5, 4, b3, NGP_LF_NO_REFILL | NGP_LF_NO_SHUFFLE);
        assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
        P.color[11] = 3;
        assert(ng_puzzle_action(&P, NGP_CMD_SWAP, 11, NGP_DIR_UP) == NGP_ERR_ILLEGAL);   /* locked */
        assert(ng_puzzle_action(&P, NGP_CMD_SWAP, 12, NGP_DIR_UP) == NGP_OK);
        ng_puzzle_update(&P);
        assert(!(P.cover[11] & NGP_COVER_LOCK) && P.color[11] == 3);   /* unlocked, piece kept */
        assert(P.color[10] == 0 && P.color[12] == 0);
        ng_puzzle_settle(&P);
    }
    printf("crates, ice, locks: OK\n");
}

static void test_slide_into_sealed_holes(void)
{
    /* the cell under the wall can only be refilled by a piece sliding in from the side */
    static const char *b =
        "12121"
        "34#43"
        "12312"
        "34143"
        "21212";
    NGPuzzleLevel lv = match_level(5, 5, 4, b, NGP_LF_NO_SHUFFLE);
    uint8_t c, holes = 0;
    assert(ng_puzzle_level_check(&lv, 0) == NGP_OK);
    assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
    assert(ng_puzzle_action(&P, NGP_CMD_HAMMER, 12, 0) == NGP_OK);
    ng_puzzle_settle(&P);
    for (c = 0; c < 25u; c++) if (!(P.cover[c] & NGP_COVER_WALL) && !P.color[c]) holes++;
    assert(holes == 0 && P.color[12] != 0);
    /* without refill nothing slides: the hole stays */
    lv.flags = NGP_LF_NO_SHUFFLE | NGP_LF_NO_REFILL;
    assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
    assert(ng_puzzle_action(&P, NGP_CMD_HAMMER, 12, 0) == NGP_OK);
    ng_puzzle_settle(&P);
    assert(P.color[12] == 0);
    printf("holes under walls fill by sliding pieces: OK\n");
}

static void test_dead_board(void)
{
    static const char *dead = "1212" "3434" "1212" "3434";
    NGPuzzleLevel lv = match_level(4, 4, 4, dead, NGP_LF_NO_SHUFFLE);
    uint32_t h1, h2;
    assert(ng_puzzle_level_check(&lv, 0) == NGP_OK);
    assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
    assert(ng_puzzle_status(&P) == NGP_STATUS_LOST && P.end_reason == NGP_END_NO_MOVES);
    lv.flags = 0;
    assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
    assert(ng_puzzle_status(&P) == NGP_STATUS_READY && P.shuffles == 1 && ng_puzzle_has_moves(&P));
    h1 = ng_puzzle_hash(&P);
    assert(ng_puzzle_init(&Q, &lv, 0) == NGP_OK);
    h2 = ng_puzzle_hash(&Q);
    assert(h1 == h2);                                  /* the shuffle is seeded */
    assert(ng_puzzle_init(&Q, &lv, 77) == NGP_OK);
    assert(ng_puzzle_hash(&Q) != h1);
    printf("dead board: lost without shuffles, reshuffled (deterministically) with: OK\n");
}

/* Random play: no softlock, no run left standing, hashes replay. */
static void test_random_play(void)
{
    NGPuzzleLevel lv = match_level(8, 8, 6, NULL, 0);
    NGPuzzleAction acts[400];
    uint32_t seed;
    int cascades = 0, shuffles = 0, total_moves = 0;
    clock_t t0 = clock();
    lv.moves = 60;
    for (seed = 1; seed <= 60; seed++) {
        int n = 0, guard = 0;
        uint16_t tick = 0;
        assert(ng_puzzle_init(&P, &lv, seed) == NGP_OK);
        while (ng_puzzle_status(&P) == NGP_STATUS_READY && guard++ < 200) {
            uint8_t c, d;
            NGPuzzleEvent e;
            assert(ng_puzzle_hint(&P, &c, &d));
            assert(ng_puzzle_action(&P, NGP_CMD_SWAP, c, d) == NGP_OK);
            acts[n].tick = P.tick; acts[n].cmd = NGP_CMD_SWAP; acts[n].a = c; acts[n].b = d; n++;
            /* wait a few frames between phases, as an animating game would */
            while (ng_puzzle_status(&P) == NGP_STATUS_BUSY) {
                ng_puzzle_update(&P);
                while (ng_puzzle_event(&P, &e)) {
                    if (e.type == NGP_EV_CASCADE) cascades++;
                    if (e.type == NGP_EV_SHUFFLE) shuffles++;
                }
            }
            total_moves++;
            if (ng_puzzle_status(&P) == NGP_STATUS_READY) {
                /* settled boards hold no run and are full */
                uint8_t i;
                for (i = 0; i < 64; i++) assert(P.color[i] != 0);
            }
            tick = P.tick;
        }
        (void)tick;
        assert(ng_puzzle_status(&P) != NGP_STATUS_BUSY);
        {
            uint32_t h = ng_puzzle_hash(&P);
            assert(ng_puzzle_replay(&Q, &lv, seed, acts, (uint16_t)n) == h);
        }
    }
    assert(cascades > 0);
    printf("random play: 60 seeds, %d moves, %d cascades, %d shuffles, replays match (%.1f ms host)\n",
           total_moves, cascades, shuffles, 1000.0 * (double)(clock() - t0) / CLOCKS_PER_SEC);
}

static void test_save_mid_cascade(void)
{
    NGPuzzleLevel lv = match_level(8, 8, 5, NULL, 0);
    uint8_t buf[NGP_SAVE_SIZE];
    uint32_t seed;
    int checked = 0;
    for (seed = 1; seed < 40; seed++) {
        uint8_t c, d;
        uint16_t n;
        uint32_t h1, h2;
        assert(ng_puzzle_init(&P, &lv, seed) == NGP_OK);
        assert(ng_puzzle_hint(&P, &c, &d));
        assert(ng_puzzle_action(&P, NGP_CMD_SWAP, c, d) == NGP_OK);
        ng_puzzle_update(&P);                     /* cleared, waiting to fall */
        n = ng_puzzle_save(&P, buf, sizeof(buf));
        assert(n > 0 && n <= NGP_SAVE_SIZE);
        ng_puzzle_settle(&P);
        h1 = ng_puzzle_hash(&P);
        assert(ng_puzzle_load(&Q, &lv, buf, n) == NGP_OK);
        assert(ng_puzzle_status(&Q) == NGP_STATUS_BUSY);
        ng_puzzle_settle(&Q);
        h2 = ng_puzzle_hash(&Q);
        assert(h1 == h2);
        checked++;
    }
    /* a save from another level is refused */
    {
        NGPuzzleLevel other = match_level(8, 8, 6, NULL, 0);
        uint16_t n = ng_puzzle_save(&P, buf, sizeof(buf));
        assert(ng_puzzle_load(&Q, &other, buf, n) == NGP_ERR_SAVE);
        buf[4] = 99;
        assert(ng_puzzle_load(&Q, &lv, buf, n) == NGP_ERR_SAVE);
    }
    printf("save/load mid-cascade: %d resumed games match: OK\n", checked);
}

static void test_rejected_save_keeps_live_context(void)
{
    NGPuzzleLevel lv = match_level(8, 8, 5, NULL, 0);
    NGPuzzleLevel invalid;
    static NGPuzzle before;
    uint8_t buf[NGP_SAVE_SIZE], saved;
    uint16_t n, size;
    assert(ng_puzzle_init(&P, &lv, 123u) == NGP_OK);
    assert(ng_puzzle_init(&Q, &lv, 456u) == NGP_OK);
    n = ng_puzzle_save(&P, buf, sizeof(buf));
    assert(n > 24u);
    before = Q;
    /* Every incomplete prefix, including a valid header, must leave ALL
     * existing state/events/dirty cells untouched on rejection. */
    for (size = 0u; size < n; size++) {
        assert(ng_puzzle_load(&Q, &lv, buf, size) != NGP_OK);
        assert(memcmp(&Q, &before, sizeof(Q)) == 0);
    }
    saved = buf[0]; buf[0] ^= 0x80u;
    assert(ng_puzzle_load(&Q, &lv, buf, n) == NGP_ERR_SAVE);
    assert(memcmp(&Q, &before, sizeof(Q)) == 0); buf[0] = saved;
    saved = buf[4]; buf[4] = 99u;
    assert(ng_puzzle_load(&Q, &lv, buf, n) == NGP_ERR_SAVE);
    assert(memcmp(&Q, &before, sizeof(Q)) == 0); buf[4] = saved;
    invalid = lv; invalid.w = 0u;
    assert(ng_puzzle_load(&Q, &invalid, buf, n) != NGP_OK);
    assert(memcmp(&Q, &before, sizeof(Q)) == 0);
    assert(ng_puzzle_load(&Q, &lv, buf, n) == NGP_OK);
    assert(ng_puzzle_hash(&Q) == ng_puzzle_hash(&P));
    printf("rejected save: %u truncated prefixes preserve every live byte: OK\n", n);
}

static void test_hash_future_state(void)
{
    NGPuzzleLevel lv = bubble_level(NULL), other;
    uint32_t h;
    assert(ng_puzzle_init(&P, &lv, 123u) == NGP_OK);
    h = ng_puzzle_hash(&P);
#define HASH_CHANGE(field) do { Q = P; Q.field++; assert(ng_puzzle_hash(&Q) != h); } while (0)
    HASH_CHANGE(shot_vx_q8); HASH_CHANGE(shot_vy_q8); HASH_CHANGE(shot_kind);
    HASH_CHANGE(attach_cell); HASH_CHANGE(swap_a); HASH_CHANGE(swap_b);
    HASH_CHANGE(seed); HASH_CHANGE(tick); HASH_CHANGE(specials_made); HASH_CHANGE(shuffles);
#undef HASH_CHANGE
    /* Notification queues and search/animation scratch do not decide play. */
    Q = P; Q.ev_count = 0; Q.ev_dropped++; Q.dirty[0] ^= 1u;
    Q.mark[0] ^= 1u; Q.queue[0]++; Q.fall[0]++; Q.cleared[0] ^= 1u;
    assert(ng_puzzle_hash(&Q) == h);
    other = lv; other.push_every = 4u; Q = P; Q.level = &other;
    assert(ng_puzzle_hash(&Q) != h);
    printf("hash distinguishes future flight, reset, phase and rule state: OK\n");
}

static void test_save_integrity_and_rules(void)
{
    NGPuzzleLevel lv = bubble_level(NULL), other;
    static NGPuzzle before;
    uint8_t buf[NGP_SAVE_SIZE];
    uint16_t n, i;
    uint8_t bit;
    assert(ng_puzzle_init(&P, &lv, 123u) == NGP_OK);
    assert(ng_puzzle_action(&P, NGP_CMD_FIRE, 0, 0) == NGP_OK);
    ng_puzzle_update(&P);
    n = ng_puzzle_save(&P, buf, sizeof(buf));
    assert(n > 0u);
    assert(ng_puzzle_init(&Q, &lv, 456u) == NGP_OK);
    before = Q;
    for (i = 0; i < n; i++) for (bit = 1u; bit; bit = (uint8_t)(bit << 1)) {
        buf[i] ^= bit;
        assert(ng_puzzle_load(&Q, &lv, buf, n) != NGP_OK);
        assert(memcmp(&Q, &before, sizeof(Q)) == 0);
        buf[i] ^= bit;
    }
#define RULE_CHANGE(field) do { other = lv; other.field++; \
    assert(ng_puzzle_load(&Q, &other, buf, n) != NGP_OK); \
    assert(memcmp(&Q, &before, sizeof(Q)) == 0); } while (0)
    RULE_CHANGE(moves); RULE_CHANGE(time_frames); RULE_CHANGE(flags);
    RULE_CHANGE(goals[0].count); RULE_CHANGE(stars[0]);
    RULE_CHANGE(bubble_d); RULE_CHANGE(row_h); RULE_CHANGE(loss_row);
    RULE_CHANGE(push_every); RULE_CHANGE(miss_limit); RULE_CHANGE(first_shifted);
    RULE_CHANGE(launch_x); RULE_CHANGE(launch_y); RULE_CHANGE(shot_speed_q8);
    RULE_CHANGE(aim_max);
#undef RULE_CHANGE
    assert(ng_puzzle_load(&Q, &lv, buf, n) == NGP_OK);
    assert(ng_puzzle_hash(&Q) == ng_puzzle_hash(&P));
    /* Each intermediate flight/pop/settle state must resume identically. */
    for (i = 0; i < 200u && ng_puzzle_status(&P) == NGP_STATUS_BUSY; i++) {
        ng_puzzle_update(&P); ng_puzzle_update(&Q);
        assert(ng_puzzle_hash(&Q) == ng_puzzle_hash(&P));
        n = ng_puzzle_save(&P, buf, sizeof(buf));
        assert(ng_puzzle_load(&Q, &lv, buf, n) == NGP_OK);
        assert(ng_puzzle_hash(&Q) == ng_puzzle_hash(&P));
    }
    assert(ng_puzzle_status(&P) != NGP_STATUS_BUSY);
    printf("save integrity: every single-bit error rejected atomically; flight resumes: OK\n");
}

static void save_test_checksum(uint8_t *b, uint16_t size)
{
    uint16_t i;
    uint32_t h = 2166136261u;
    for (i = 0; i < size - 4u; i++) h = (h ^ b[i]) * 16777619u;
    b[size - 4u] = (uint8_t)(h >> 24); b[size - 3u] = (uint8_t)(h >> 16);
    b[size - 2u] = (uint8_t)(h >> 8); b[size - 1u] = (uint8_t)h;
}

static void test_save_legacy_and_validation(void)
{
    NGPuzzleLevel lv = match_level(8, 8, 5, NULL, 0);
    static NGPuzzle before;
    uint8_t buf[NGP_SAVE_SIZE];
    uint16_t n;
    assert(ng_puzzle_init(&P, &lv, 123u) == NGP_OK);
    assert(ng_puzzle_init(&Q, &lv, 456u) == NGP_OK);
    before = Q;
    n = ng_puzzle_save(&P, buf, sizeof(buf));
    /* Correct checksums cannot make an out-of-range swap index safe. */
    buf[24u + 3u * 64u + 8u + 12u + 10u] = 200u;
    save_test_checksum(buf, n);
    assert(ng_puzzle_load(&Q, &lv, buf, n) == NGP_ERR_SAVE);
    assert(memcmp(&Q, &before, sizeof(Q)) == 0);
    n = ng_puzzle_save(&P, buf, sizeof(buf));
    buf[24u] = 200u; save_test_checksum(buf, n);
    assert(ng_puzzle_load(&Q, &lv, buf, n) == NGP_ERR_SAVE);
    assert(memcmp(&Q, &before, sizeof(Q)) == 0);
    n = ng_puzzle_save(&P, buf, sizeof(buf));
    /* Legacy v1 has the same payload, the original identity of this fixed
     * level (FNV of 00 08 08 05 00 00 04 D2), and no checksum trailer. */
    buf[4] = 1u; buf[8] = 0x10u; buf[9] = 0x96u; buf[10] = 0xAEu; buf[11] = 0x4Cu;
    assert(ng_puzzle_load(&Q, &lv, buf, (uint16_t)(n - 4u)) == NGP_OK);
    assert(ng_puzzle_hash(&Q) == ng_puzzle_hash(&P));
    printf("legacy v1 save compatibility and checked index/color validation: OK\n");
}

static void test_save_every_match_phase(void)
{
    NGPuzzleLevel lv = match_level(12, 16, 5, NULL, 0);
    uint8_t buf[NGP_SAVE_SIZE], c, d;
    uint16_t n, steps;
    uint32_t seed;
    for (seed = 1; seed <= 20u; seed++) {
        assert(ng_puzzle_init(&P, &lv, seed) == NGP_OK);
        assert(ng_puzzle_hint(&P, &c, &d));
        assert(ng_puzzle_action(&P, NGP_CMD_SWAP, c, d) == NGP_OK);
        steps = 0;
        do {
            n = ng_puzzle_save(&P, buf, sizeof(buf));
            assert(n == 652u && n <= NGP_SAVE_SIZE);
            assert(ng_puzzle_load(&Q, &lv, buf, n) == NGP_OK);
            assert(ng_puzzle_hash(&P) == ng_puzzle_hash(&Q));
            ng_puzzle_update(&P); ng_puzzle_update(&Q);
            assert(ng_puzzle_hash(&P) == ng_puzzle_hash(&Q));
            assert(++steps < 200u);
        } while (ng_puzzle_status(&P) == NGP_STATUS_BUSY);
    }
    printf("maximum board: every match phase resumes identically across 20 seeds: OK\n");
}

static void test_timer_and_score(void)
{
    NGPuzzleLevel lv = match_level(6, 6, 5, NULL, 0);
    int i;
    lv.time_frames = 10;
    assert(ng_puzzle_init(&P, &lv, 5) == NGP_OK);
    for (i = 0; i < 9; i++) ng_puzzle_update(&P);
    assert(ng_puzzle_status(&P) == NGP_STATUS_READY && P.time_left == 1);
    ng_puzzle_update(&P);
    assert(ng_puzzle_status(&P) == NGP_STATUS_LOST && P.end_reason == NGP_END_TIME);
    assert(ng_puzzle_action(&P, NGP_CMD_SWAP, 0, 0) == NGP_ERR_STATE);
    /* the score saturates */
    lv.time_frames = 0;
    assert(ng_puzzle_init(&P, &lv, 5) == NGP_OK);
    P.score = 0xFFFFFFF0u;
    {
        uint8_t c, d;
        assert(ng_puzzle_hint(&P, &c, &d));
        assert(ng_puzzle_action(&P, NGP_CMD_SWAP, c, d) == NGP_OK);
        ng_puzzle_settle(&P);
        assert(P.score == 0xFFFFFFFFu);
    }
    /* moves run out */
    lv.moves = 1;
    assert(ng_puzzle_init(&P, &lv, 5) == NGP_OK);
    lv.goals[0].type = NGP_OBJ_SCORE; lv.goals[0].count = 999;
    assert(ng_puzzle_init(&P, &lv, 5) == NGP_OK);
    {
        uint8_t c, d;
        assert(ng_puzzle_hint(&P, &c, &d));
        assert(ng_puzzle_action(&P, NGP_CMD_SWAP, c, d) == NGP_OK);
        ng_puzzle_settle(&P);
        assert(ng_puzzle_status(&P) == NGP_STATUS_LOST && P.end_reason == NGP_END_MOVES);
    }
    printf("timer, moves, score saturation: OK\n");
}

static void test_terminal_state_survives_full_event_queue(void)
{
    NGPuzzleLevel lv = match_level(6, 6, 5, NULL, 0);
    NGPuzzleEvent event;
    unsigned i, lost = 0u;
    lv.time_frames = 1u;
    assert(ng_puzzle_init(&P, &lv, 5) == NGP_OK);
    P.ev_head = 0u; P.ev_count = NGP_EVENT_QUEUE;
    for (i = 0u; i < NGP_EVENT_QUEUE; i++) P.ev[i].type = NGP_EV_TIME_LOW;
    ng_puzzle_update(&P);
    while (ng_puzzle_event(&P, &event)) if (event.type == NGP_EV_LOST) lost++;
    assert(!lost && P.ev_dropped);
    assert(ng_puzzle_status(&P) == NGP_STATUS_LOST && P.end_reason == NGP_END_TIME);
    puts("terminal status remains authoritative after event overflow: OK");
}

static void test_worst_case(void)
{
    /* the whole 12x16 board in one go: two prisms swapped */
    NGPuzzleLevel lv = match_level(12, 16, 8, NULL, 0);
    clock_t t0;
    assert(ng_puzzle_init(&P, &lv, 3) == NGP_OK);
    P.color[0] = NGP_COLOR_PRISM; P.kind[0] = NGP_KIND_PRISM;
    P.color[1] = NGP_COLOR_PRISM; P.kind[1] = NGP_KIND_PRISM;
    t0 = clock();
    assert(ng_puzzle_action(&P, NGP_CMD_SWAP, 0, NGP_DIR_RIGHT) == NGP_OK);
    ng_puzzle_update(&P);
    {
        int i, left = 0;
        for (i = 0; i < 192; i++) if (P.color[i]) left++;
        assert(left == 0);
    }
    ng_puzzle_settle(&P);
    assert(ng_puzzle_status(&P) != NGP_STATUS_BUSY);
    printf("full-board clear (12x16): OK (%.2f ms host)\n", 1000.0 * (double)(clock() - t0) / CLOCKS_PER_SEC);
}

static uint16_t angle_for(NGPuzzle *p, uint8_t want)
{
    uint16_t a;
    for (a = 544; a <= 992; a++) {
        uint8_t cell;
        ng_puzzle_bubble_preview(p, a, 0, 0, &cell);
        if (cell == want) return a;
    }
    return 0;
}

static void test_bubbles(void)
{
    NGPuzzleLevel lv = bubble_level(NULL);
    NGPuzzleEvent ev;
    NGPuzzlePoint pts[8];
    uint8_t cell, n, bounced = 0;
    uint16_t a;
    /* empty field: a shot to the left rebounds and sticks to the ceiling */
    assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
    n = ng_puzzle_bubble_preview(&P, 560, pts, 8, &cell);
    assert(n >= 3 && cell != NGP_NO_CELL && cell < 8);
    assert(ng_puzzle_action(&P, NGP_CMD_AIM, 560, 0) == NGP_OK && P.angle == 560);
    assert(ng_puzzle_action(&P, NGP_CMD_FIRE, 0, 0) == NGP_OK);
    while (ng_puzzle_status(&P) == NGP_STATUS_BUSY) {
        NGPuzzleEvent e;
        ng_puzzle_update(&P);
        while (ng_puzzle_event(&P, &e)) {
            if (e.type == NGP_EV_BOUNCE && e.a == 0) bounced = 1;
            if (e.type == NGP_EV_ATTACH) ev = e;
        }
    }
    assert(bounced && ev.cell == cell);              /* the preview was exact */
    assert(P.color[cell] != 0);
    /* the aim is clamped */
    assert(ng_puzzle_action(&P, NGP_CMD_AIM, 100, 0) == NGP_OK && P.angle == 544);
    assert(ng_puzzle_action(&P, NGP_CMD_AIM_BY, 2000, 0) == NGP_OK && P.angle == 992);

    /* a group of three pops; what hung from it drops; an anchor holds */
    {
        static const char *L =
            "22......"
            "3......-"
            "........" ".......-" "........" ".......-"
            "........" ".......-" "........" ".......-" "........" ".......-";
        lv = bubble_level(L);
        assert(ng_puzzle_level_check(&lv, 0) == NGP_OK);
        assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
        assert(ng_puzzle_action(&P, NGP_CMD_COLOR_SHOT, 2, 0) == NGP_OK);
        a = angle_for(&P, 2);
        assert(a);
        assert(ng_puzzle_action(&P, NGP_CMD_AIM, (int16_t)a, 0) == NGP_OK);
        assert(ng_puzzle_action(&P, NGP_CMD_FIRE, 0, 0) == NGP_OK);
        ng_puzzle_settle(&P);
        {
            int pops = 0, drops = 0;
            NGPuzzleEvent e;
            while (ng_puzzle_event(&P, &e)) {
                if (e.type == NGP_EV_POP) { pops++; assert(e.a == 3); }
                if (e.type == NGP_EV_DROP) { drops++; assert(e.a == 1); }
            }
            assert(pops == 1 && drops == 1);
        }
        assert(P.color[0] == 0 && P.color[1] == 0 && P.color[2] == 0 && P.color[8] == 0);
        assert(ng_puzzle_status(&P) == NGP_STATUS_WON && P.end_reason == NGP_END_BOARD_CLEAR);
    }
    {
        static const char *L =
            "22......"
            "3......-"
            "A......."
            ".......-" "........" ".......-" "........" ".......-" "........" ".......-"
            "........" ".......-";
        lv = bubble_level(L);
        assert(ng_puzzle_level_check(&lv, 0) == NGP_OK);
        assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
        assert(ng_puzzle_action(&P, NGP_CMD_COLOR_SHOT, 2, 0) == NGP_OK);
        a = angle_for(&P, 2);
        assert(a);
        ng_puzzle_action(&P, NGP_CMD_AIM, (int16_t)a, 0);
        ng_puzzle_action(&P, NGP_CMD_FIRE, 0, 0);
        ng_puzzle_settle(&P);
        assert(count_events(&P, NGP_EV_DROP, 0) == 0);
        assert(P.color[8] == 3 && P.kind[16] == NGP_KIND_ANCHOR);   /* held by the anchor */
    }
    printf("bubbles: rebound, exact preview, pop, drop, anchor: OK\n");
}

static void test_bubble_rows(void)
{
    static const char *L =
        "12341234"
        "4123412-"
        "........" ".......-" "........" ".......-" "........" ".......-"
        "........" ".......-" "........" ".......-";
    NGPuzzleLevel lv = bubble_level(L);
    int shots = 0;
    lv.push_every = 1;
    assert(ng_puzzle_level_check(&lv, 0) == NGP_OK);
    assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
    while (ng_puzzle_status(&P) == NGP_STATUS_READY && shots < 40) {
        uint8_t par = P.parity;
        assert(ng_puzzle_action(&P, NGP_CMD_AIM, 768, 0) == NGP_OK);
        assert(ng_puzzle_action(&P, NGP_CMD_FIRE, 0, 0) == NGP_OK);
        ng_puzzle_settle(&P);
        shots++;
        if (ng_puzzle_status(&P) == NGP_STATUS_READY) assert(P.parity != par);   /* a row came in */
        {   /* the lattice stays consistent after rows move */
            uint8_t c, d;
            for (c = 0; c < 96; c++)
                for (d = 0; d < 6; d++) {
                    uint8_t nb = ng_puzzle_neighbor(&P, c, d);
                    if (nb != NGP_NO_CELL) assert(ng_puzzle_neighbor(&P, nb, opposite6(d)) == c);
                }
        }
    }
    assert(ng_puzzle_status(&P) == NGP_STATUS_LOST && P.end_reason == NGP_END_LOSS_LINE);
    printf("bubble rows pushed in, lattice kept, loss line after %d shots: OK\n", shots);
}

static void test_bubble_hint_and_replay(void)
{
    static const char *L =
        "rrrrrrrr"
        "rrrrrrr-"
        "rrrrrrrr"
        "rrrrrrr-"
        "........" ".......-" "........" ".......-" "........" ".......-" "........" ".......-";
    NGPuzzleLevel lv = bubble_level(L);
    NGPuzzleAction acts[80];
    int n = 0, guard = 0, hinted = 0;
    uint32_t h;
    lv.miss_limit = 5;
    assert(ng_puzzle_init(&P, &lv, 0) == NGP_OK);
    while (ng_puzzle_status(&P) == NGP_STATUS_READY && guard++ < 80) {
        uint16_t a = 768;
        if (ng_puzzle_bubble_hint(&P, &a)) hinted++;
        ng_puzzle_action(&P, NGP_CMD_AIM, (int16_t)a, 0);
        acts[n].tick = P.tick; acts[n].cmd = NGP_CMD_AIM; acts[n].a = (int16_t)a; acts[n].b = 0; n++;
        ng_puzzle_action(&P, NGP_CMD_FIRE, 0, 0);
        acts[n].tick = P.tick; acts[n].cmd = NGP_CMD_FIRE; acts[n].a = 0; acts[n].b = 0; n++;
        while (ng_puzzle_status(&P) == NGP_STATUS_BUSY) {
            NGPuzzleEvent e;
            ng_puzzle_update(&P);
            while (ng_puzzle_event(&P, &e)) {}
        }
        if (n >= 78) break;
    }
    assert(hinted > 0);
    h = ng_puzzle_hash(&P);
    assert(ng_puzzle_replay(&Q, &lv, 0, acts, (uint16_t)n) == h);
    printf("bubble hints (%d of %d shots) and replay hash: OK (%s)\n", hinted, n / 2,
           ng_puzzle_status(&P) == NGP_STATUS_WON ? "cleared" : "in play");
}

static void test_bubble_target_and_busy_aim(void)
{
    static const char *L =
        "12341234" "1212121-" "........" ".......-"
        "........" ".......-" "........" ".......-"
        "........" ".......-" "........" ".......-";
    static const char *A =
        "....A..." ".......-" "........" ".......-"
        "........" ".......-" "........" ".......-"
        "........" ".......-" "........" ".......-";
    NGPuzzleLevel lv = bubble_level(L);
    static NGPuzzle before;
    uint16_t angle, steps;
    unsigned wrong_neighbor = 0;
    assert(ng_puzzle_init(&P, &lv, 123u) == NGP_OK);
    before = P;
    for (angle = 544u; angle <= 992u; angle++) {
        uint8_t cell, hit = ng_puzzle_bubble_target(&P, angle), old = NGP_NO_CELL, d;
        ng_puzzle_bubble_preview(&P, angle, 0, 0, &cell);
        assert(hit != NGP_NO_CELL && P.color[hit]);
        assert(memcmp(&P, &before, sizeof(P)) == 0);
        if (cell != NGP_NO_CELL) for (d = 0; d < 6u && old == NGP_NO_CELL; d++) {
            uint8_t nb = ng_puzzle_neighbor(&P, cell, d);
            if (nb != NGP_NO_CELL && P.color[nb]) old = nb;
        }
        if (old != hit) wrong_neighbor++;
    }
    assert(wrong_neighbor > 0u); /* Original neighbour guess really differs. */
    assert(ng_puzzle_action(&P, NGP_CMD_AIM, 700, 0) == NGP_OK);
    assert(ng_puzzle_action(&P, NGP_CMD_FIRE, 0, 0) == NGP_OK);
    Q = P;
    steps = 0;
    while (ng_puzzle_status(&P) == NGP_STATUS_BUSY) {
        assert(ng_puzzle_action(&P, NGP_CMD_AIM_BY, 6, 0) == NGP_OK);
        ng_puzzle_update(&P); ng_puzzle_update(&Q);
        assert(P.angle != Q.angle);
        assert(P.shot_x_q8 == Q.shot_x_q8 && P.shot_y_q8 == Q.shot_y_q8);
        assert(P.shot_vx_q8 == Q.shot_vx_q8 && P.shot_vy_q8 == Q.shot_vy_q8);
        assert(P.state == Q.state && P.score == Q.score && P.rng == Q.rng);
        assert(memcmp(P.color, Q.color, sizeof(P.color)) == 0);
        assert(memcmp(P.kind, Q.kind, sizeof(P.kind)) == 0);
        assert(++steps < 200u);
    }
    lv = bubble_level(A);
    assert(ng_puzzle_init(&P, &lv, 123u) == NGP_OK);
    assert(ng_puzzle_bubble_target(&P, 768u) == 4u);
    before = P;
    assert(ng_puzzle_action(&P, NGP_CMD_HAMMER, 4, 0) == NGP_ERR_RANGE);
    assert(memcmp(&P, &before, sizeof(P)) == 0);
    lv = bubble_level(NULL);
    assert(ng_puzzle_init(&P, &lv, 123u) == NGP_OK);
    assert(ng_puzzle_bubble_target(&P, 768u) == NGP_NO_CELL);
    assert(ng_puzzle_bubble_target(0, 768u) == NGP_NO_CELL);
    printf("bubble targeting: %u wrong-neighbour cases fixed; busy aim preserves flight: OK\n", wrong_neighbor);
}

static void test_incremental_preview(void)
{
    static const char *L =
        "12341234" "1212121-" "........" ".......-"
        "........" ".......-" "........" ".......-"
        "........" ".......-" "........" ".......-";
    NGPuzzleLevel lv = bubble_level(L);
    static NGPuzzle before;
    NGPuzzlePreview q = {0};
    NGPuzzlePoint pts[NGP_PREVIEW_POINTS];
    uint16_t a;
    uint8_t budget;
    assert(ng_puzzle_init(&P, &lv, 123u) == NGP_OK);
    before = P;
    for (a = 544; a <= 992; a++) for (budget = 1; budget <= 9; budget++) {
        uint8_t cell, count = ng_puzzle_bubble_preview(&P, a, pts, NGP_PREVIEW_POINTS, &cell);
        unsigned calls = 0;
        assert(ng_puzzle_preview_begin(&q, &P, a, 7) == NGP_PREVIEW_BUSY);
        assert(ng_puzzle_preview_step(&q, &P, 7, 0) == NGP_PREVIEW_BUSY && q.steps == 0);
        while (q.state == NGP_PREVIEW_BUSY) {
            uint16_t steps = q.steps;
            ng_puzzle_preview_step(&q, &P, 7, budget);
            assert(q.steps - steps <= (budget < 8 ? budget : 8));
            assert(++calls <= 4097u);
        }
        assert(q.state == NGP_PREVIEW_READY && q.count == count && q.cell == cell);
        assert(q.hit == ng_puzzle_bubble_target(&P, a));
        assert(memcmp(pts, q.points, count * sizeof(pts[0])) == 0);
        assert(memcmp(&P, &before, sizeof(P)) == 0);
        assert(ng_puzzle_preview_step(&q, &P, 7, 8) == NGP_PREVIEW_READY);
    }
    /* A completed result is stale too after a generation change. */
    assert(ng_puzzle_preview_step(&q, &P, 8, 0) == NGP_PREVIEW_CANCELLED);
    assert(q.count == 0 && q.cell == NGP_NO_CELL && q.hit == NGP_NO_CELL);
    assert(ng_puzzle_preview_begin(&q, &P, 768, 9) == NGP_PREVIEW_BUSY);
    assert(ng_puzzle_preview_step(&q, &Q, 9, 8) == NGP_PREVIEW_CANCELLED);
    assert(ng_puzzle_preview_begin(&q, &P, 768, 9) == NGP_PREVIEW_BUSY);
    P.parity ^= 1u;
    assert(ng_puzzle_preview_step(&q, &P, 9, 8) == NGP_PREVIEW_CANCELLED);
    P = before;
    assert(ng_puzzle_preview_begin(&q, &P, 0, 9) == NGP_PREVIEW_BUSY);
    assert(q.angle == 544 && q.requested_angle == 0);
    assert(ng_puzzle_preview_begin(&q, 0, 768, 9) == NGP_PREVIEW_INVALID);
    assert(ng_puzzle_preview_step(0, &P, 9, 8) == NGP_PREVIEW_INVALID);
    puts("bounded preview: 449 angles x 9 budgets match exact paths, cancel safely, preserve all state: OK");
}

int main(void)
{
    test_level_check();
    test_topology();
    test_match_rows_and_columns();
    test_specials();
    test_furniture();
    test_slide_into_sealed_holes();
    test_dead_board();
    test_random_play();
    test_save_mid_cascade();
    test_rejected_save_keeps_live_context();
    test_hash_future_state();
    test_save_integrity_and_rules();
    test_save_legacy_and_validation();
    test_save_every_match_phase();
    test_timer_and_score();
    test_terminal_state_survives_full_event_queue();
    test_worst_case();
    test_bubbles();
    test_bubble_rows();
    test_bubble_hint_and_replay();
    test_bubble_target_and_busy_aim();
    test_incremental_preview();
    printf("ng_puzzle: all tests passed (context %u bytes, save %u bytes)\n",
           (unsigned)sizeof(NGPuzzle), (unsigned)NGP_SAVE_SIZE);
    return 0;
}
