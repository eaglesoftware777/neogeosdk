/*
 * The minimal programs quoted in docs/GENRE_ENGINES_V1.7.7.md, one per
 * genre engine, kept compiling and running here (tests/test_genre_engines.py
 * builds this with all four engines and -Werror). On the console each
 * example's loop would be the game's frame loop: draw from the context,
 * turn the events into sounds, wait for the vertical blank.
 */
#include <stdio.h>
#include "ng_puzzle.h"
#include "ng_mahjong.h"
#include "ng_rpg.h"
#include "ng_beatemup.h"

/* ---- puzzle: one match-3 level ----------------------------------------- */
static const NGPuzzleLevel k_level = {
    .mode = NGP_MODE_MATCH, .w = 8, .h = 8, .colors = 5, .seed = 1234u,
    .moves = 20u,                                   /* twenty swaps */
    .goals = { { NGP_OBJ_SCORE, 0, 30u } },         /* 3000 points */
    .stars = { 3000u, 5000u, 8000u },
};
static NGPuzzle s_board;

static int puzzle_example(void)
{
    NGPuzzleEvent e;
    uint8_t cell, dir, swaps = 0;
    if (ng_puzzle_level_check(&k_level, 0) != NGP_OK) return 0;
    ng_puzzle_init(&s_board, &k_level, 0);
    while (ng_puzzle_status(&s_board) != NGP_STATUS_WON && ng_puzzle_status(&s_board) != NGP_STATUS_LOST
           && swaps < 5u) {
        if (ng_puzzle_status(&s_board) == NGP_STATUS_READY && ng_puzzle_hint(&s_board, &cell, &dir)) {
            ng_puzzle_action(&s_board, NGP_CMD_SWAP, cell, dir);      /* the player's swap */
            swaps++;
        }
        ng_puzzle_update(&s_board);                                    /* one phase a frame */
        while (ng_puzzle_event(&s_board, &e)) { /* NGP_EV_MATCH: a sound; NGP_EV_FALL: animate */ }
    }
    return s_board.score > 0u;
}

/* ---- puzzle: a picture-logic level, filled from its clues -------------- */
static const NGPuzzleLevel k_picture = {
    .mode = NGP_MODE_LOGIC, .w = 5, .h = 5, .colors = 7, .moves = 5,   /* five hearts */
    .layout = "4.4.4" "44444" ".444." "..7.." ".777.",                 /* a tulip */
    .goals = { { NGP_OBJ_PICTURE, 0, 0 } },
};

static int picture_example(void)
{
    NGPuzzleEvent e;
    uint8_t row, runs[4], k, cell;
    if (ng_puzzle_level_check(&k_picture, 0) != NGP_OK) return 0;
    ng_puzzle_init(&s_board, &k_picture, 0);
    /* a full row's clue is a single run of five: fill it, as a player would first */
    for (row = 0; row < 5u; row++) {
        uint8_t n = ng_puzzle_logic_clue(&s_board, 0, row, runs, 4);  /* a row's clue */
        if (n == 1u && runs[0] == 5u)
            for (k = 0; k < 5u; k++)
                ng_puzzle_action(&s_board, NGP_CMD_MARK, (int16_t)(row * 5u + k), NGP_MARK_FILL);
    }
    /* then the rest of the picture */
    for (cell = 0; cell < 25u; cell++)
        if (s_board.color[cell] && s_board.kind[cell] != NGP_MARK_FILL)
            ng_puzzle_action(&s_board, NGP_CMD_MARK, cell, NGP_MARK_FILL);  /* or NGP_MARK_CROSS */
    while (ng_puzzle_event(&s_board, &e)) { /* NGP_EV_MARK, NGP_EV_MISTAKE, NGP_EV_LINE: sounds */ }
    return ng_puzzle_status(&s_board) == NGP_STATUS_WON && s_board.moves_left == 5u;
}

/* ---- mahjong: one hand of four-seat HK Old Style, all seats CPU --------- */
static NGMahjong s_table;

static int mahjong_example(void)
{
    NGMahjongRules rules;
    NGMahjongEvent e;
    uint16_t steps = 0;
    ng_mahjong_rules_default(&rules);
    ng_mahjong_init(&s_table, &rules, 785u);
    /* ng_mahjong_set_cpu(&s_table, 0, NGM_HUMAN) would make seat 0 a player:
     * then, while ng_mahjong_wants(&s_table, 0) is non-zero, the game asks
     * and answers with ng_mahjong_command(&s_table, 0, NGM_ACT_..., arg). */
    while (s_table.phase != NGM_PH_HAND_OVER && steps++ < 2000u) {
        ng_mahjong_update(&s_table);
        while (ng_mahjong_event(&s_table, &e)) { /* NGM_EV_DISCARD: a clack */ }
    }
    return s_table.phase == NGM_PH_HAND_OVER && ng_mahjong_tiles_ok(&s_table);
}

/* ---- mahjong solitaire: the turtle, cleared by its own solution --------- */
static NGMahjongPos s_turtle[NGMS_MAX_TILES];
static NGMahjongSol s_sol;

static int solitaire_example(void)
{
    uint8_t n = ng_mjsol_layout_turtle(s_turtle), a, b;
    if (ng_mjsol_init(&s_sol, s_turtle, n, 7u) != NGM_OK) return 0;
    while (s_sol.status == NGMS_PLAYING && ng_mjsol_hint(&s_sol, &a, &b)) {
        ng_mjsol_select(&s_sol, a);                 /* the player picks a free tile ... */
        ng_mjsol_select(&s_sol, b);                 /* ... and its match: the pair goes */
    }
    if (s_sol.status == NGMS_STUCK) ng_mjsol_shuffle(&s_sol);   /* hints can walk into a dead end */
    return s_sol.left < n;
}

/* ---- rpg: one walled room, one person to talk to ----------------------- */
#define W 8
#define H 6
static const uint8_t k_tiles[W * H] = {
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1 };
static const NGRpgAction k_met[] = { { NGR_A_SET_FLAG, 0, 1, 0 }, { NGR_A_END, 0, 0, 0 } };
static const NGRpgDialogueNode k_lines[1] = {
    { 2, 0, 100u, NGR_END, NGR_END, { NGR_C_ALWAYS, 0, 0, 0 }, k_met, { 0, 0, 0 }, { 0, 0, 0 },
      { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } } } };
static const NGRpgObjectDef k_objects[1] = {
    { NGR_O_NPC, 5, 2, 1, 1, { 0, 0, 0, 0, 0, 0, 0 }, 0, 0, 0, 0, 0 } };
static const NGRpgRoomDef k_rooms[1] = {
    { W, H, 0, 0, k_tiles, 0, 0, k_objects, 0, 0, 1, 0, 0, 0, 0 } };
static const NGRpgWorld k_world = { k_rooms, 0, 0, k_lines, 1, 0, 0, 0, 1 };
static const NGRpgMemberDef k_hero = { 1, NGR_AB_PUSH, 24, 10, 2 };
static NGRpg s_world;

static int rpg_example(void)
{
    NGRpgEvent e;
    uint8_t room;
    uint16_t index, f;
    if (ng_rpg_world_check(&k_world, &room, &index) != NGR_OK) return 0;
    ng_rpg_init(&s_world, &k_world, 1u);
    ng_rpg_party_add(&s_world, &k_hero);
    ng_rpg_enter_room(&s_world, 0, 2, 2, NGR_FACE_RIGHT);
    for (f = 0; f < 60u; f++) {                     /* walk right for a second */
        ng_rpg_input(&s_world, NGR_IN_RIGHT);
        ng_rpg_update(&s_world);
        while (ng_rpg_event(&s_world, &e)) { }
    }
    ng_rpg_input(&s_world, 0);
    ng_rpg_interact(&s_world);                      /* A: talk to whoever is in front */
    while (ng_rpg_event(&s_world, &e)) { /* NGR_EV_DIALOGUE: show line e.value */ }
    if (s_world.dlg_open) ng_rpg_dialogue_next(&s_world);
    return ng_rpg_flag(&s_world, 1);
}

/* ---- beat-'em-up: a short street with one wave ------------------------- */
/*                        su ac re dmg hs flags            bx bw bz bh bd px pz lng cnl next      snd anim */
static const NGBMove k_moves[2] = {
    { 3, 2, 7, 3, 14, 0,                 6, 34, 46, 30, 10, 14, 0, 0, 6, 1,        0, 0 },   /* jab, chains */
    { 5, 3, 14, 7, 0, NGB_MV_KNOCKDOWN,  4, 46, 30, 34, 12, 44, 30, 8, 0, NGB_NONE, 0, 1 },   /* kick */
};
/* hp spd dep jmp hw ht hd wt moves n chain air low dash special pummel thr_f thr_b ai reach aggr attacks n flags score phases n think */
static const NGBFighterDef k_hero_def = { 60, 34, 22, 72, 12, 84, 6, 3, k_moves, 2, 0, 1, 1, NGB_NONE, NGB_NONE, NGB_NONE,
                                          NGB_NONE, NGB_NONE, 0, 0, 0, { 0 }, 0, 0, 0, 0, 0, 0 };
static const NGBFighterDef k_thug = { 20, 24, 16, 64, 12, 80, 6, 2, k_moves, 2, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                      NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_AI_BRAWLER, 28, 8, { 0 }, 1, 0, 100,
                                      0, 0, 0 };
static const NGBFighterDef *const k_enemies[1] = { &k_thug };
static const NGBWave k_waves[1] = { { 40, NGB_WAVE_LOCK | NGB_WAVE_BOSS, 2, { 0, 0 }, 0x00, 30 } };
static const NGBStage k_stage = { 640, 140, 206, 320, k_waves, 0, k_enemies, 0, 1, 0, 1, 0,
                                  2, 6, 0, 6, 0, 0, 0, 0 };
static NGBeatEmUp s_street;

static int beatemup_example(void)
{
    NGBEvent e;
    uint16_t f, kos = 0;
    ng_beatemup_init(&s_street, &k_stage, &k_hero_def, 3, 99u);
    for (f = 0; f < 3600u && ng_beatemup_status(&s_street) == NGB_PLAYING; f++) {
        ng_beatemup_input(&s_street, NGB_IN_RIGHT, (f & 8u) ? NGB_BTN_A : 0);   /* the pad, held */
        ng_beatemup_update(&s_street);
        while (ng_beatemup_event(&s_street, &e)) if (e.type == NGB_EV_KO) kos++;
        /* draw: ng_beatemup_draw_list() gives fighters, props and shots far to near */
    }
    return kos > 0u;
}

int main(void)
{
    int ok = puzzle_example() && picture_example() && mahjong_example() && solitaire_example() && rpg_example() && beatemup_example();
    printf("genre examples: %s\n", ok ? "all tests passed" : "FAILED");
    return ok ? 0 : 1;
}
