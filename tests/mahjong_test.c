/* Host checks for ng_mahjong (HK Old Style table and solitaire). */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ng_mahjong.h"

static NGMahjong M, N;
static NGMahjongSol S, T;

/* "123m456p789s1234567z": m characters, p dots, s bamboo, z honours
 * (1-4 winds east..north, 5-7 dragons red, green, white). */
static void counts_of(const char *txt, uint8_t *c)
{
    char digits[32];
    int nd = 0;
    memset(c, 0, NGM_KINDS);
    for (; *txt; txt++) {
        if (*txt >= '1' && *txt <= '9') { digits[nd++] = *txt; continue; }
        if (*txt == ' ') continue;
        {
            int base = *txt == 'm' ? 0 : *txt == 'p' ? 9 : *txt == 's' ? 18 : 27, i;
            for (i = 0; i < nd; i++) c[base + digits[i] - '1']++;
            nd = 0;
        }
    }
}

static uint8_t kind_of(const char *t)   /* "5m" */
{
    uint8_t c[NGM_KINDS], i;
    counts_of(t, c);
    for (i = 0; i < NGM_KINDS; i++) if (c[i]) return i;
    return NGM_NO_TILE;
}

static void set_hand(NGMahjong *m, uint8_t seat, const char *txt)
{
    uint8_t c[NGM_KINDS], k, i;
    NGMahjongHand *h = &m->hand[seat];
    counts_of(txt, c);
    h->count = 0;
    for (k = 0; k < NGM_KINDS; k++) for (i = 0; i < c[k]; i++) h->tiles[h->count++] = k;
}

static void test_shapes(void)
{
    uint8_t c[NGM_KINDS];
    counts_of("123m456m789m123p55p", c); assert(ng_mahjong_is_win(c, 0));
    counts_of("111m222p333s111z55m", c); assert(ng_mahjong_is_win(c, 0));
    counts_of("19m19p19s1234567z1m", c); assert(ng_mahjong_is_win(c, 0));          /* thirteen orphans */
    counts_of("123m456m789m12p55p9s", c); assert(!ng_mahjong_is_win(c, 0));
    counts_of("11123456789995m", c); assert(ng_mahjong_is_win(c, 0));                /* nine gates */
    counts_of("456m789m123p55p", c); assert(ng_mahjong_is_win(c, 1));                /* with a meld down */
    counts_of("456m789m123p55p", c); assert(!ng_mahjong_is_win(c, 0));
    counts_of("11223344556677p", c); assert(ng_mahjong_is_win(c, 0));                /* chows, no seven pairs needed */
    counts_of("1133557799m1133p", c); assert(!ng_mahjong_is_win(c, 0));              /* seven pairs: not a hand here */
    counts_of("123m456m789m123p55p", c); assert(ng_mahjong_shanten(c, 0) == -1);
    counts_of("123m456m789m123p5p", c); assert(ng_mahjong_shanten(c, 0) == 0);
    counts_of("19m19p19s1234567z", c); assert(ng_mahjong_shanten(c, 0) == 0);
    counts_of("123m456m789m12p5p", c); assert(ng_mahjong_shanten(c, 0) == 1);
    counts_of("13579m2468p1357s", c); assert(ng_mahjong_shanten(c, 0) >= 3);
    counts_of("456m789m12p55p", c); assert(ng_mahjong_shanten(c, 1) == 0);
    printf("winning shapes and shanten: OK\n");
}

static uint8_t faan_of(const char *concealed, const char *win_tile, uint8_t self_drawn, uint32_t *pat)
{
    NGMahjongResult r;
    uint8_t ok;
    char full[64];
    /* a self-drawn tile is already in the hand */
    snprintf(full, sizeof(full), "%s%s", concealed, self_drawn ? win_tile : "");
    set_hand(&M, 0, full);
    ok = ng_mahjong_score(&M, 0, kind_of(win_tile), self_drawn, &r);
    assert(ok);
    if (pat) *pat = r.patterns;
    return r.faan;
}

static void test_scoring(void)
{
    uint32_t pat;
    NGMahjongRules r;
    ng_mahjong_rules_default(&r);
    assert(ng_mahjong_init(&M, &r, 1) == NGM_OK);
    M.phase = NGM_PH_TURN; M.first_turn = 0; M.dealer = 0; M.round_wind = NGM_EAST;
    M.wall_left = 50;
    assert(faan_of("123m456m789m123p5p", "5p", 0, &pat) == 2);
    assert(pat == (NGM_PAT_ALL_CHOWS | NGM_PAT_CONCEALED));
    assert(faan_of("123m123m456m789m5m", "5m", 0, &pat) == 9);
    assert(pat & NGM_PAT_FULL_FLUSH && pat & NGM_PAT_ALL_CHOWS);
    assert(faan_of("111m999m111z555z2m", "2m", 0, &pat) == 10);          /* capped at the limit */
    assert(pat & NGM_PAT_ALL_PUNGS && pat & NGM_PAT_HALF_FLUSH && pat & NGM_PAT_SEAT_WIND && pat & NGM_PAT_ROUND_WIND);
    assert(faan_of("555z666z777z123m9p", "9p", 0, &pat) == 9);
    assert(pat & NGM_PAT_BIG_DRAGONS && !(pat & NGM_PAT_DRAGON_PUNG));
    assert(faan_of("555z666z7z123m999p", "7z", 0, &pat) == 6);         /* small dragons 5 + concealed */
    assert(pat & NGM_PAT_SMALL_DRAGONS);
    assert(faan_of("123m456m789m123p5p", "5p", 1, &pat) == 3);          /* self-drawn adds one */
    assert(faan_of("19m19p19s1234567z", "1m", 0, &pat) == 10 && pat == NGM_PAT_THIRTEEN);
    assert(faan_of("1112345678999m", "5m", 0, &pat) == 10 && pat == NGM_PAT_NINE_GATES);
    /* a chicken hand: a pung down, no pattern */
    M.hand[0].nmelds = 1;
    M.hand[0].melds[0].type = NGM_MELD_PUNG; M.hand[0].melds[0].kind = kind_of("2p"); M.hand[0].melds[0].from = 1;
    assert(faan_of("345m678s999p4s", "4s", 0, &pat) == 0 && pat == 0);
    M.hand[0].nmelds = 0;
    /* the last tile from the wall */
    M.wall_left = 0;
    assert(faan_of("123m456m789m123p5p", "5p", 1, &pat) == 4 && (pat & NGM_PAT_LAST_TILE));
    M.wall_left = 50;
    printf("faan scoring fixtures: OK\n");
}

static void claim_setup(void)
{
    NGMahjongRules r;
    uint8_t s;
    ng_mahjong_rules_default(&r);
    assert(ng_mahjong_init(&M, &r, 2) == NGM_OK);
    for (s = 0; s < 4; s++) ng_mahjong_set_cpu(&M, s, NGM_HUMAN);
    M.dealer = 0; M.round_wind = NGM_EAST; M.first_turn = 0; M.wall_left = 60;
    M.wall_head = 0; M.wall_tail = 59;
    for (s = 0; s < 60; s++) M.wall[s] = kind_of("9s");
    set_hand(&M, 0, "1234m");                         /* only for the discard */
    set_hand(&M, 1, "34m111p222p333p99s");            /* chows 5m (345) */
    set_hand(&M, 2, "55m111s222s333s9p");             /* pungs 5m */
    set_hand(&M, 3, "123p456p789p11s46m");            /* wins on 5m */
    M.phase = NGM_PH_TURN; M.turn = 0; M.drawn = NGM_NO_TILE;
    assert(ng_mahjong_command(&M, 1, NGM_ACT_DISCARD, kind_of("4m")) == NGM_ERR_TURN);
    set_hand(&M, 0, "5m1234m");
    assert(ng_mahjong_command(&M, 0, NGM_ACT_DISCARD, kind_of("5m")) == NGM_OK);
    assert(M.phase == NGM_PH_CLAIM);
}

static void test_claims(void)
{
    uint16_t w;
    claim_setup();
    assert(ng_mahjong_wants(&M, 0) == 0);
    w = ng_mahjong_wants(&M, 1); assert(w & NGM_CAN_CHOW_HIGH && !(w & NGM_CAN_PUNG));
    w = ng_mahjong_wants(&M, 2); assert(w & NGM_CAN_PUNG && !(w & NGM_CAN_CHOW_HIGH));
    w = ng_mahjong_wants(&M, 3); assert(w & NGM_CAN_RON);
    assert(ng_mahjong_command(&M, 2, NGM_ACT_CHOW, 2) == NGM_ERR_ILLEGAL);   /* not the next seat */
    assert(ng_mahjong_command(&M, 0, NGM_ACT_PASS, 0) == NGM_ERR_TURN);      /* the discarder */
    assert(ng_mahjong_command(&M, 1, NGM_ACT_CHOW, 2) == NGM_OK);
    assert(ng_mahjong_command(&M, 2, NGM_ACT_PUNG, 0) == NGM_OK);
    ng_mahjong_update(&M);                      /* seat 3 has not answered */
    assert(M.phase == NGM_PH_CLAIM);
    assert(ng_mahjong_command(&M, 3, NGM_ACT_RON, 0) == NGM_OK);
    ng_mahjong_update(&M);
    assert(M.phase == NGM_PH_HAND_OVER && M.result.winner == 3 && M.result.from == 0);
    assert(M.result.delta[0] < 0 && M.result.delta[3] > 0);
    assert(M.result.delta[0] + M.result.delta[1] + M.result.delta[2] + M.result.delta[3] == 0);

    claim_setup();      /* ron passed: the pung beats the chow */
    ng_mahjong_command(&M, 1, NGM_ACT_CHOW, 2);
    ng_mahjong_command(&M, 2, NGM_ACT_PUNG, 0);
    ng_mahjong_command(&M, 3, NGM_ACT_PASS, 0);
    ng_mahjong_update(&M);
    assert(M.phase == NGM_PH_TURN && M.turn == 2 && M.hand[2].nmelds == 1 && M.hand[2].melds[0].type == NGM_MELD_PUNG);
    assert(ng_mahjong_wants(&M, 2) & NGM_CAN_DISCARD);
    assert(!(ng_mahjong_wants(&M, 2) & NGM_CAN_TSUMO));

    claim_setup();      /* only the chow */
    ng_mahjong_command(&M, 1, NGM_ACT_CHOW, 2);
    ng_mahjong_command(&M, 2, NGM_ACT_PASS, 0);
    ng_mahjong_command(&M, 3, NGM_ACT_PASS, 0);
    ng_mahjong_update(&M);
    assert(M.turn == 1 && M.hand[1].melds[0].type == NGM_MELD_CHOW && M.hand[1].melds[0].kind == kind_of("3m"));

    claim_setup();      /* nobody: seat 1 draws */
    ng_mahjong_command(&M, 1, NGM_ACT_PASS, 0);
    ng_mahjong_command(&M, 2, NGM_ACT_PASS, 0);
    ng_mahjong_command(&M, 3, NGM_ACT_PASS, 0);
    ng_mahjong_update(&M);
    assert(M.turn == 1 && M.drawn == kind_of("9s") && M.hand[1].count == 14);
    printf("claims: priorities (win, pung, chow, draw) and wrong seats refused: OK\n");
}

static void play_hand(NGMahjong *m, int *wins, int *draws, int *turns)
{
    int guard = 0;
    while (m->phase != NGM_PH_HAND_OVER && guard++ < 4000) {
        NGMahjongEvent e;
        ng_mahjong_update(m);
        assert(ng_mahjong_tiles_ok(m));
        while (ng_mahjong_event(m, &e)) {}
    }
    assert(m->phase == NGM_PH_HAND_OVER);
    if (m->result.winner == NGM_NO_TILE) (*draws)++; else (*wins)++;
    *turns += m->turns;
}

static void test_cpu_games(void)
{
    NGMahjongRules r;
    uint32_t seed;
    int wins = 0, draws = 0, turns = 0, hands = 0, games = 0;
    clock_t t0 = clock();
    ng_mahjong_rules_default(&r);
    for (seed = 1; seed <= 60; seed++) {
        int s, guard = 0;
        r.flowers = (uint8_t)(seed & 1u);
        assert(ng_mahjong_init(&M, &r, seed) == NGM_OK);
        for (s = 0; s < 4; s++) ng_mahjong_set_cpu(&M, (uint8_t)s, (uint8_t)((seed + s) % 3u));
        for (;;) {
            int32_t sum;
            play_hand(&M, &wins, &draws, &turns);
            hands++;
            sum = M.score[0] + M.score[1] + M.score[2] + M.score[3];
            assert(sum == 4 * r.start_score);
            if (ng_mahjong_next_hand(&M) != NGM_OK) break;
            assert(guard++ < 200);
        }
        assert(M.phase == NGM_PH_GAME_OVER);
        games++;
    }
    printf("CPU games: %d games, %d hands (%d won, %d drawn), %d discards, tiles conserved (%.0f ms host)\n",
           games, hands, wins, draws, turns, 1000.0 * (double)(clock() - t0) / CLOCKS_PER_SEC);
    assert(wins > 0 && draws >= 0);
}

static void test_save_and_replay(void)
{
    NGMahjongRules r;
    uint8_t buf[NGM_SAVE_SIZE];
    uint16_t n;
    int i, w = 0, d = 0, t = 0;
    uint32_t h1, h2, h3;
    ng_mahjong_rules_default(&r);
    r.flowers = 1;
    assert(ng_mahjong_init(&M, &r, 777) == NGM_OK);
    for (i = 0; i < 25; i++) ng_mahjong_update(&M);
    n = ng_mahjong_save(&M, buf, sizeof(buf));
    assert(n > 0 && n <= NGM_SAVE_SIZE);
    play_hand(&M, &w, &d, &t);
    h1 = ng_mahjong_hash(&M);
    assert(ng_mahjong_load(&N, buf, n) == NGM_OK);
    play_hand(&N, &w, &d, &t);
    h2 = ng_mahjong_hash(&N);
    assert(h1 == h2);
    /* the same seed plays the same hand */
    assert(ng_mahjong_init(&N, &r, 777) == NGM_OK);
    play_hand(&N, &w, &d, &t);
    h3 = ng_mahjong_hash(&N);
    assert(h3 == h1);
    buf[0] ^= 0xFF;
    assert(ng_mahjong_load(&N, buf, n) == NGM_ERR_SAVE);
    printf("save/load mid-hand and seeded replay: OK (%u bytes)\n", (unsigned)n);
}

static void test_shanten_speed(void)
{
    uint32_t x = 12345;
    int i;
    long total = 0;
    clock_t t0 = clock();
    for (i = 0; i < 20000; i++) {
        uint8_t c[NGM_KINDS];
        int k = 0;
        memset(c, 0, sizeof(c));
        while (k < 14) {
            uint8_t t;
            x ^= x << 13; x ^= x >> 17; x ^= x << 5;
            t = (uint8_t)((x >> 8) % 34u);
            if (c[t] < 4) { c[t]++; k++; }
        }
        total += ng_mahjong_shanten(c, 0);
    }
    printf("shanten: 20000 random hands, mean %.2f (%.1f us each, host)\n", (double)total / 20000.0,
           1e6 * (double)(clock() - t0) / CLOCKS_PER_SEC / 20000.0);
}

/* ---- solitaire ---------------------------------------------------------- */
static void test_sol_rules(void)
{
    static const NGMahjongPos row[3] = { {0, 0, 0}, {2, 0, 0}, {4, 0, 0} };
    static const NGMahjongPos stack[4] = { {0, 0, 0}, {0, 0, 1}, {4, 0, 0}, {5, 1, 1} };
    static const NGMahjongPos tower[4] = { {0, 0, 0}, {0, 0, 1}, {0, 0, 2}, {0, 0, 3} };
    S.pos = row; S.n = 3; memset(S.gone, 0, sizeof(S.gone)); ng_mjsol_recount(&S);
    assert(ng_mjsol_is_free(&S, 0) && !ng_mjsol_is_free(&S, 1) && ng_mjsol_is_free(&S, 2));
    S.gone[0] = 1; ng_mjsol_recount(&S);               /* the left one gone: the middle is free */
    assert(ng_mjsol_is_free(&S, 1));
    S.pos = stack; S.n = 4; memset(S.gone, 0, sizeof(S.gone)); ng_mjsol_recount(&S);
    assert(!ng_mjsol_is_free(&S, 0) && ng_mjsol_is_free(&S, 1));
    assert(!ng_mjsol_is_free(&S, 2) && ng_mjsol_is_free(&S, 3));   /* half-overlap covers too */
    assert(ng_mjsol_match(NGM_FLOWER, NGM_FLOWER + 3) && ng_mjsol_match(NGM_SEASON, NGM_SEASON + 2));
    assert(!ng_mjsol_match(NGM_FLOWER, NGM_SEASON) && !ng_mjsol_match(1, 2));
    assert(ng_mjsol_init(&S, tower, 4, 1) == NGM_ERR_LAYOUT);       /* only one free tile at a time */
    printf("solitaire free-tile rules: OK\n");
}

static void test_sol_deals(void)
{
    static NGMahjongPos turtle[144];
    uint8_t n = ng_mjsol_layout_turtle(turtle), i, j;
    uint32_t seed;
    clock_t t0;
    assert(n == 144);
    for (i = 0; i < n; i++)
        for (j = (uint8_t)(i + 1); j < n; j++)
            assert(!(turtle[i].x == turtle[j].x && turtle[i].y == turtle[j].y && turtle[i].z == turtle[j].z));
    t0 = clock();
    for (seed = 1; seed <= 30; seed++) {
        uint8_t p;
        NGMahjongSolEvent e;
        assert(ng_mjsol_init(&S, turtle, n, seed) == NGM_OK);
        assert(ng_mjsol_free_pairs(&S) > 0);
        {   /* the tile set: four of each kind, one of each bonus tile */
            uint8_t c[NGM_KINDS_ALL];
            memset(c, 0, sizeof(c));
            for (i = 0; i < n; i++) c[S.kind[i]]++;
            for (i = 0; i < NGM_KINDS; i++) assert(c[i] == 4);
            for (i = NGM_FLOWER; i < NGM_KINDS_ALL; i++) assert(c[i] == 1);
        }
        memcpy(&T, &S, sizeof(S));
        for (p = 0; p < 72; p++) {
            assert(ng_mjsol_select(&S, S.solution[2 * p]) == NGM_OK);
            assert(ng_mjsol_select(&S, S.solution[2 * p + 1]) == NGM_OK);
        }
        assert(S.status == NGMS_CLEARED && S.left == 0);
        while (ng_mjsol_event(&S, &e)) {}
        /* undo all the way back */
        for (p = 0; p < 72; p++) assert(ng_mjsol_undo(&S) == NGM_OK);
        assert(S.left == 144 && ng_mjsol_undo(&S) == NGM_ERR_STATE);
    }
    printf("solitaire: 30 turtle deals cleared by their solution order, undone (%.0f ms host)\n",
           1000.0 * (double)(clock() - t0) / CLOCKS_PER_SEC);

    /* selection: blocked, mismatch, pair; stuck boards reshuffle */
    assert(ng_mjsol_init(&S, turtle, n, 5) == NGM_OK);
    {
        uint8_t a, b, blocked = NGMS_NONE, x = NGMS_NONE, y = NGMS_NONE;
        for (i = 0; i < n; i++) if (!ng_mjsol_is_free(&S, i)) { blocked = i; break; }
        assert(ng_mjsol_select(&S, blocked) == NGM_ERR_ILLEGAL);
        for (i = 0; i < n && y == NGMS_NONE; i++)
            for (j = (uint8_t)(i + 1); j < n; j++)
                if (ng_mjsol_is_free(&S, i) && ng_mjsol_is_free(&S, j) && !ng_mjsol_match(S.kind[i], S.kind[j])) { x = i; y = j; break; }
        assert(ng_mjsol_select(&S, x) == NGM_OK && S.selected == x);
        assert(ng_mjsol_select(&S, y) == NGM_ERR_ILLEGAL && S.selected == y);
        assert(ng_mjsol_hint(&S, &a, &b));
        S.selected = NGMS_NONE;
        assert(ng_mjsol_select(&S, a) == NGM_OK && ng_mjsol_select(&S, b) == NGM_OK && S.left == 142);
        /* make the board stuck by hand: every free tile a different kind */
        {
            uint8_t k = 0;
            for (i = 0; i < n; i++)
                if (ng_mjsol_is_free(&S, i)) S.kind[i] = k++;
            assert(!ng_mjsol_hint(&S, 0, 0));
        }
    }
    printf("solitaire selection, mismatch, blocked tiles, hint: OK\n");
    {
        static const NGMahjongPos small[8] = { {0,0,0}, {2,0,0}, {4,0,0}, {6,0,0}, {0,2,0}, {2,2,0}, {4,2,0}, {6,2,0} };
        assert(ng_mjsol_init(&S, small, 8, 9) == NGM_OK);
        S.kind[0] = 1; S.kind[3] = 2; S.kind[4] = 3; S.kind[7] = 4;   /* four free ends, no pair: stuck */
        S.kind[1] = 1; S.kind[2] = 2; S.kind[5] = 3; S.kind[6] = 4;
        assert(!ng_mjsol_hint(&S, 0, 0));
        assert(ng_mjsol_shuffle(&S) == NGM_OK && ng_mjsol_hint(&S, 0, 0));
        {
            uint8_t c[NGM_KINDS_ALL];
            memset(c, 0, sizeof(c));
            for (i = 0; i < 8; i++) c[S.kind[i]]++;
            assert(c[1] == 2 && c[2] == 2 && c[3] == 2 && c[4] == 2);
        }
        {
            uint8_t p;
            for (p = 0; p < 4; p++) {
                assert(ng_mjsol_select(&S, S.solution[2 * p]) == NGM_OK);
                assert(ng_mjsol_select(&S, S.solution[2 * p + 1]) == NGM_OK);
            }
            assert(S.status == NGMS_CLEARED);
        }
    }
    printf("solitaire shuffle of a stuck board: solvable again: OK\n");
}

int main(void)
{
    test_shapes();
    test_scoring();
    test_claims();
    test_cpu_games();
    test_save_and_replay();
    test_shanten_speed();
    test_sol_rules();
    test_sol_deals();
    printf("ng_mahjong: all tests passed (table %u bytes, solitaire %u bytes)\n",
           (unsigned)sizeof(NGMahjong), (unsigned)sizeof(NGMahjongSol));
    return 0;
}
