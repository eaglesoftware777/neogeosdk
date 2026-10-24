// A C++14 game reaching the four genre engines through their C headers:
// the engines are compiled as C, this file as C++ (as a USE_2D_PLUS game's
// sources are), and the link only works if every call kept C linkage and
// both sides agree on the contexts' layout.
#include <cstdio>
#include <cstring>
#include "ng_puzzle.h"
#include "ng_mahjong.h"
#include "ng_rpg.h"
#include "ng_beatemup.h"

static NGPuzzle puzzle;
static NGMahjong table;
static NGMahjongSol sol;
static NGRpg rpg;
static NGBeatEmUp brawl;

int main()
{
    // puzzle
    NGPuzzleLevel lv;
    std::memset(&lv, 0, sizeof(lv));
    lv.mode = NGP_MODE_MATCH; lv.w = 8; lv.h = 8; lv.colors = 6; lv.seed = 3;
    if (ng_puzzle_init(&puzzle, &lv, 0) != NGP_OK) return 1;
    uint8_t c = 0, d = 0;
    if (!ng_puzzle_hint(&puzzle, &c, &d)) return 2;
    if (ng_puzzle_action(&puzzle, NGP_CMD_SWAP, c, d) != NGP_OK) return 3;
    ng_puzzle_settle(&puzzle);
    if (ng_puzzle_status(&puzzle) == NGP_STATUS_BUSY || puzzle.score == 0) return 4;
    lv.mode = NGP_MODE_BUBBLE; lv.bubble_d = 16; lv.row_h = 14; lv.loss_row = 7;
    lv.launch_x = 64; lv.launch_y = 128; lv.shot_speed_q8 = 1536;
    if (ng_puzzle_init(&puzzle, &lv, 0) != NGP_OK) return 14;
    NGPuzzlePreview preview{};
    if (ng_puzzle_preview_begin(&preview, &puzzle, 768, 1) != NGP_PREVIEW_BUSY) return 15;
    for (int i = 0; i < 600 && preview.state == NGP_PREVIEW_BUSY; i++)
        ng_puzzle_preview_step(&preview, &puzzle, 1, 8);
    if (preview.state != NGP_PREVIEW_READY || preview.cell == NGP_NO_CELL ||
        ng_puzzle_bubble_target(&puzzle, 768) != NGP_NO_CELL) return 16;

    // mahjong
    if (ng_mahjong_init(&table, nullptr, 9) != NGM_OK) return 5;
    for (int i = 0; i < 4000 && table.phase != NGM_PH_HAND_OVER; i++) ng_mahjong_update(&table);
    if (table.phase != NGM_PH_HAND_OVER || !ng_mahjong_tiles_ok(&table)) return 6;
    static NGMahjongPos turtle[144];
    if (ng_mjsol_init(&sol, turtle, ng_mjsol_layout_turtle(turtle), 4) != NGM_OK) return 7;

    // rpg: one room, one member
    static const uint8_t tiles[4 * 4] = { 1,1,1,1, 1,0,0,1, 1,0,0,1, 1,1,1,1 };
    NGRpgRoomDef room;
    std::memset(&room, 0, sizeof(room));
    room.w = 4; room.h = 4; room.tiles = tiles;
    NGRpgWorld world;
    std::memset(&world, 0, sizeof(world));
    world.rooms = &room; world.nrooms = 1;
    if (ng_rpg_init(&rpg, &world, 1) != NGR_OK) return 8;
    NGRpgMemberDef m = { 1, 0, 24, 6, 2 };
    if (ng_rpg_party_add(&rpg, &m) != 0) return 9;
    if (ng_rpg_enter_room(&rpg, 0, 1, 1, NGR_FACE_DOWN) != NGR_OK) return 10;
    for (int i = 0; i < 30; i++) { ng_rpg_input(&rpg, NGR_IN_RIGHT); ng_rpg_update(&rpg); }
    if ((rpg.party[0].x >> 12) != 2) return 11;

    // beat-'em-up: a stage and a player
    static const NGBMove jab[1] = { { 3, 2, 6, 3, 12, 0, 4, 30, 20, 30, 8, 16, 0, 0, 0, NGB_NONE, 0, 0 } };
    NGBFighterDef hero;
    std::memset(&hero, 0, sizeof(hero));
    hero.max_hp = 40; hero.speed = 24; hero.depth_speed = 16; hero.jump = 64; hero.half_w = 12; hero.height = 56;
    hero.half_depth = 4; hero.moves = jab; hero.nmoves = 1; hero.chain = 0;
    hero.air = hero.low = hero.dash = hero.special = hero.pummel = hero.throw_fwd = hero.throw_back = NGB_NONE;
    NGBStage stage;
    std::memset(&stage, 0, sizeof(stage));
    stage.length = 640; stage.y_min = 140; stage.y_max = 200; stage.view_w = 320;
    if (ng_beatemup_init(&brawl, &stage, &hero, 3, 2) != NGB_OK) return 12;
    for (int i = 0; i < 20; i++) { ng_beatemup_input(&brawl, NGB_IN_RIGHT, 0); ng_beatemup_update(&brawl); }
    if (brawl.f[NGB_PLAYER].x <= (64 << 8)) return 13;

    std::printf("genre engines from C++14: puzzle %lu, mahjong %u, rpg %u, beat-'em-up %u bytes: OK\n",
                (unsigned long)sizeof(NGPuzzle), (unsigned)sizeof(NGMahjong), (unsigned)sizeof(NGRpg),
                (unsigned)sizeof(NGBeatEmUp));
    return 0;
}
