/* Host checks for ng_rpg: rooms, party, puzzles, dialogue, items, saves. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ng_rpg.h"

enum { ROPE, GEAR, KEY, SHOVEL, BERRY, WOOD, FIBRE, PLANK, NITEMS };
enum { MAIYA = 1, LUNA = 2 };

static NGRpg G, H;
static int ev_count[64];

/* '#' wall, '.' floor, '~' water, 'o' pit, 'n' narrow, 'v' climb, '^' hazard, 'D' door, 'L' ledge */
static void build(uint8_t *out, const char *const *rows, int w, int h)
{
    int x, y;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            char c = rows[y][x];
            uint8_t f = 0;
            switch (c) {
            case '#': f = NGR_T_SOLID; break;
            case '~': f = NGR_T_WATER; break;
            case 'o': f = NGR_T_PIT; break;
            case 'n': f = NGR_T_NARROW; break;
            case 'v': f = NGR_T_CLIMB; break;
            case '^': f = NGR_T_HAZARD; break;
            case 'D': f = NGR_T_DOOR; break;
            case 'L': f = NGR_T_LEDGE; break;
            default: break;
            }
            out[y * w + x] = f;
        }
}

static const char *const VILLAGE[12] = {
    "################",
    "#..............#",
    "#..............#",
    "#..............#",
    "#......o.......#",
    "#..............#",
    "#..............#",
    "#....#n#.......#",
    "#....#.#.......#",
    "#....#.#.......#",
    "#....###.......#",
    "##########DD####" };
static const char *const CAVE[10] = {
    "##############",
    "#............#",
    "#............#",
    "#............#",
    "#............#",
    "#............#",
    "#............#",
    "#............#",
    "#............#",
    "##############" };
static const char *const FOREST[8] = {
    "############",
    "#..........#",
    "#..~~~~....#",
    "#..~~~~....#",
    "#..........#",
    "#.^........#",
    "#..........#",
    "############" };
static const char *const JAM[5] = { "#####", "#...#", "#...#", "#...#", "#####" };

static uint8_t t_village[16 * 12], t_cave[14 * 10], t_forest[12 * 8], t_jam[25];
static uint8_t ch_cave[14 * 10];

static const NGRpgAction A_LEARN[] = { {NGR_A_SET_FLAG, 0, 20, 0}, {NGR_A_END, 0, 0, 0} };
static const NGRpgAction A_KEY[] = { {NGR_A_GIVE, 0, KEY, 1}, {NGR_A_END, 0, 0, 0} };
static const NGRpgAction A_T0[] = { {NGR_A_SET_FLAG, 0, 31, 0}, {NGR_A_END, 0, 0, 0} };
static const NGRpgAction A_T1[] = { {NGR_A_ADD_COUNTER, 0, 0, 1}, {NGR_A_END, 0, 0, 0} };
static const NGRpgAction A_T2[] = { {NGR_A_SET_FLAG, 0, 32, 0}, {NGR_A_RESTORE, 0, 3, 0}, {NGR_A_END, 0, 0, 0} };

#define NODE(spk, txt, nxt) { spk, 0, txt, nxt, NGR_END, {0, 0, 0, 0}, NULL, {0, 0, 0}, {0, 0, 0}, {{0,0,0,0},{0,0,0,0},{0,0,0,0}} }
static const NGRpgDialogueNode DLG[] = {
    { 1, 3, 100, NGR_END, NGR_END, {0, 0, 0, 0}, NULL, {110, 111, 112}, {1, 2, NGR_END},
      {{NGR_C_ALWAYS, 0, 0, 0}, {NGR_C_FLAG, 0, 20, 0}, {NGR_C_ALWAYS, 0, 0, 0}} },
    { 1, 0, 101, NGR_END, NGR_END, {0, 0, 0, 0}, A_LEARN, {0, 0, 0}, {0, 0, 0}, {{0,0,0,0},{0,0,0,0},{0,0,0,0}} },
    { 1, 0, 102, NGR_END, NGR_END, {0, 0, 0, 0}, A_KEY, {0, 0, 0}, {0, 0, 0}, {{0,0,0,0},{0,0,0,0},{0,0,0,0}} },
    { 0, 0, 103, NGR_END, 4, {NGR_C_NOT_FLAG, 0, 21, 0}, NULL, {0, 0, 0}, {0, 0, 0}, {{0,0,0,0},{0,0,0,0},{0,0,0,0}} },
    NODE(0, 104, NGR_END),
};

#define O(type, x, y, p0, p1, p2, p3, p4, p5, p6, flag, text, in, out) \
    { type, x, y, 1, 1, {p0, p1, p2, p3, p4, p5, p6}, flag, text, in, out, NULL }
static const NGRpgObjectDef OBJ_VILLAGE[] = {
    O(NGR_O_NPC, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
    O(NGR_O_CHEST, 12, 2, ROPE, 1, NGR_NONE, 0, 0, 0, 0, 10, 0, 0, 0),
    O(NGR_O_BLOCK, 7, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
    O(NGR_O_PLATE, 11, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1u << 0),
    O(NGR_O_BLOCK, 11, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
    O(NGR_O_GATE, 13, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1u << 0, 0),
    O(NGR_O_LEVER, 2, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1u << 1),
    O(NGR_O_GATE, 13, 9, 0, 0, 0, 0, 0, 0, 0, 11, 0, 1u << 1, 0),
    O(NGR_O_PICKUP, 14, 1, NGR_NONE, 0, 5, 0, 0, 0, 0, 12, 0, 0, 0),
    O(NGR_O_SIGN, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0),
};
static const NGRpgExit EXIT_VILLAGE[] = { { 10, 11, 2, 1, 1, 2, 2, NGR_FACE_DOWN, {NGR_C_FLAG, 0, 20, 0}, 200 } };
static const NGRpgTrigger TRIG_VILLAGE[] = {
    { 4, 5, 3, 1, NGR_TRIG_ENTER, 0, 30, {NGR_C_ALWAYS, 0, 0, 0}, A_T0 },
    { 5, 5, 3, 1, NGR_TRIG_ENTER, 0, 0, {NGR_C_ALWAYS, 0, 0, 0}, A_T1 },
    { 0, 0, 0, 0, NGR_TRIG_WATCH, 0, 33, {NGR_C_COUNTER_GE, 0, 0, 2}, A_T2 },
};
static const NGRpgObjectDef OBJ_CAVE[] = {
    O(NGR_O_WATER_SRC, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                     /* 0 */
    O(NGR_O_VALVE, 5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                         /* 1 */
    O(NGR_O_OUTLET, 12, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1u << 2),                 /* 2 */
    O(NGR_O_EMITTER, 2, 8, NGR_FACE_UP, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),             /* 3 */
    O(NGR_O_MIRROR, 2, 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                        /* 4 */
    O(NGR_O_LENS, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                          /* 5 */
    O(NGR_O_RECEIVER, 8, 5, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1u << 3),                /* 6 */
    O(NGR_O_GEAR, 10, 7, 1, 0, 8, NGR_NONE, NGR_NONE, 0, 0, 0, 0, 0, 0),           /* 7 driver */
    O(NGR_O_GEAR, 11, 7, 0, 0, 7, 9, NGR_NONE, GEAR, 0, 0, 0, 0, 0),               /* 8 socket */
    O(NGR_O_GEAR, 12, 7, 2, 1, 8, NGR_NONE, NGR_NONE, 0, 0, 0, 0, 0, 1u << 4),     /* 9 target */
    O(NGR_O_BELL, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                          /* 10 */
    O(NGR_O_BELL, 3, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                          /* 11 */
    O(NGR_O_BELL, 5, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                          /* 12 */
    O(NGR_O_SEQUENCE, 0, 0, 11, 10, 12, NGR_NONE, NGR_NONE, NGR_NONE, 0, 40, 0, 0, 1u << 5), /* 13 */
    O(NGR_O_PAN, 8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                           /* 14 */
    O(NGR_O_PAN, 10, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                          /* 15 */
    O(NGR_O_BALANCE, 0, 0, 14, 15, 2, 0, 0, 0, 0, 0, 0, 0, 1u << 6),               /* 16 */
    O(NGR_O_GATE, 7, 8, 0, 0, 0, 0, 0, 0, 0, 41, 0, (1u << 2) | (1u << 3) | (1u << 4), 0), /* 17 */
    O(NGR_O_LOCK, 9, 3, PLANK, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0),                      /* 18 a missing pipe section */
};
static const NGRpgExit EXIT_CAVE[] = { { 12, 8, 1, 1, 2, 1, 4, NGR_FACE_RIGHT, {0, 0, 0, 0}, 0 } };
static const NGRpgObjectDef OBJ_FOREST[] = {
    O(NGR_O_CREATURE, 8, 5, 3, 2, 1, 8, 4, 0, 0, 50, 0, 0, 0),                     /* 0 */
    O(NGR_O_REST, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),                          /* 1 */
    O(NGR_O_SECRET, 10, 1, SHOVEL, 1, 0, 0, 0, 0, 0, 51, 0, 0, 0),                 /* 2 */
    O(NGR_O_LOCK, 10, 6, KEY, 1, 0, 0, 0, 0, 0, 52, 0, 0, 0),                      /* 3 */
    O(NGR_O_PLATFORM, 3, 2, 3, 0, 8, 10, 0, 0, 0, 0, 0, 0, 0),                     /* 4 */
    O(NGR_O_PLATFORM, 3, 3, 3, 0, 8, 10, 0, 0, 0, 0, 0, 1u << 0, 0),               /* 5 runs on signal 0 */
    O(NGR_O_LEVER, 8, 4, 0, 0, 0, 0, 0, 0, 0, 53, 0, 0, 1u << 0),                  /* 6 remembered (flag 53) */
};
static const NGRpgObjectDef OBJ_JAM[] = {
    O(NGR_O_GEAR, 1, 1, 1, 0, 1, 2, NGR_NONE, 0, 0, 0, 0, 0, 0),       /* driver, meshes 1 and 2 */
    O(NGR_O_GEAR, 2, 1, 0, 0, 0, 2, NGR_NONE, GEAR, 0, 0, 0, 0, 0),    /* meshes the driver and 2 */
    O(NGR_O_GEAR, 1, 2, 0, 0, 0, 1, NGR_NONE, GEAR, 0, 0, 0, 0, 0),    /* meshes the driver and 1 */
};

static NGRpgRoomDef ROOMS[4];
static const NGRpgItemDef ITEMS[NITEMS] = {
    { NGR_I_TOOL, 1, 0, 0 }, { NGR_I_TOOL, 3, 0, 1 }, { NGR_I_KEY, 3, 0, 0 }, { NGR_I_TOOL, 1, 0, 2 },
    { NGR_I_CONSUME, 9, 2, 0 }, { NGR_I_MATERIAL, 9, 0, 1 }, { NGR_I_MATERIAL, 9, 0, 0 }, { NGR_I_MATERIAL, 9, 0, 2 },
};
static const NGRpgRecipe RECIPES[] = { { {WOOD, FIBRE, 0}, {2, 3, 0}, PLANK, 1 } };
static NGRpgWorld WORLD;
static const NGRpgMemberDef MAIYA_DEF = { MAIYA, NGR_AB_CLIMB | NGR_AB_PUSH, 24, 6, 2 };
static const NGRpgMemberDef LUNA_DEF = { LUNA, NGR_AB_NARROW | NGR_AB_FINE, 20, 6, 2 };

static void setup_world(void)
{
    int i;
    build(t_village, VILLAGE, 16, 12);
    build(t_cave, CAVE, 14, 10);
    build(t_forest, FOREST, 12, 8);
    build(t_jam, JAM, 5, 5);
    memset(ch_cave, 0, sizeof(ch_cave));
    for (i = 1; i <= 12; i++) ch_cave[3 * 14 + i] = 1;
    memset(ROOMS, 0, sizeof(ROOMS));
    ROOMS[0].w = 16; ROOMS[0].h = 12; ROOMS[0].tiles = t_village;
    ROOMS[0].objects = OBJ_VILLAGE; ROOMS[0].nobjects = sizeof(OBJ_VILLAGE) / sizeof(OBJ_VILLAGE[0]);
    ROOMS[0].exits = EXIT_VILLAGE; ROOMS[0].nexits = 1;
    ROOMS[0].triggers = TRIG_VILLAGE; ROOMS[0].ntriggers = 3;
    ROOMS[1].w = 14; ROOMS[1].h = 10; ROOMS[1].tiles = t_cave; ROOMS[1].channels = ch_cave;
    ROOMS[1].objects = OBJ_CAVE; ROOMS[1].nobjects = sizeof(OBJ_CAVE) / sizeof(OBJ_CAVE[0]);
    ROOMS[1].exits = EXIT_CAVE; ROOMS[1].nexits = 1;
    ROOMS[2].w = 12; ROOMS[2].h = 8; ROOMS[2].tiles = t_forest;
    ROOMS[2].objects = OBJ_FOREST; ROOMS[2].nobjects = sizeof(OBJ_FOREST) / sizeof(OBJ_FOREST[0]);
    ROOMS[3].w = 5; ROOMS[3].h = 5; ROOMS[3].tiles = t_jam;
    ROOMS[3].objects = OBJ_JAM; ROOMS[3].nobjects = 3;
    memset(&WORLD, 0, sizeof(WORLD));
    WORLD.rooms = ROOMS; WORLD.nrooms = 4;
    WORLD.items = ITEMS; WORLD.nitems = NITEMS;
    WORLD.recipes = RECIPES; WORLD.nrecipes = 1;
    WORLD.dialogue = DLG; WORLD.ndialogue = sizeof(DLG) / sizeof(DLG[0]);
}

static void new_game(NGRpg *g, uint8_t room, uint8_t x, uint8_t y)
{
    assert(ng_rpg_init(g, &WORLD, 7) == NGR_OK);
    assert(ng_rpg_party_add(g, &MAIYA_DEF) == 0);
    assert(ng_rpg_party_add(g, &LUNA_DEF) == 1);
    assert(ng_rpg_enter_room(g, room, x, y, NGR_FACE_DOWN) == NGR_OK);
}

static void drain(NGRpg *g)
{
    NGRpgEvent e;
    while (ng_rpg_event(g, &e)) ev_count[e.type]++;
}

static void run(NGRpg *g, uint8_t dirs, int frames)
{
    int i;
    for (i = 0; i < frames; i++) {
        uint8_t m;
        ng_rpg_input(g, dirs);
        ng_rpg_update(g);
        drain(g);
        for (m = 0; m < g->nparty; m++) {   /* nobody ever stands in a wall */
            uint8_t tx = (uint8_t)(g->party[m].x >> 12), ty = (uint8_t)(g->party[m].y >> 12);
            assert(!(ng_rpg_tile(g, tx, ty) & NGR_T_SOLID) || g->def->objects == OBJ_FOREST);
        }
    }
}

static void put(NGRpg *g, uint8_t m, uint8_t tx, uint8_t ty, uint8_t facing)
{
    g->party[m].x = ((int32_t)tx << 12) + (8 << 8);
    g->party[m].y = ((int32_t)ty << 12) + (8 << 8);
    g->party[m].facing = facing;
}

static uint8_t tile_x(const NGRpg *g, uint8_t m) { return (uint8_t)(g->party[m].x >> 12); }
static uint8_t tile_y(const NGRpg *g, uint8_t m) { return (uint8_t)(g->party[m].y >> 12); }

static void test_world_check(void)
{
    NGRpgExit bad_exit[1];
    NGRpgRoomDef rooms[4];
    NGRpgWorld w;
    NGRpgDialogueNode dlg[5];
    uint8_t room;
    uint16_t idx;
    assert(ng_rpg_world_check(&WORLD, &room, &idx) == NGR_OK);
    memcpy(rooms, ROOMS, sizeof(rooms));
    memcpy(&w, &WORLD, sizeof(w));
    w.rooms = rooms;
    memcpy(bad_exit, EXIT_VILLAGE, sizeof(bad_exit));
    bad_exit[0].room = 9;
    rooms[0].exits = bad_exit;
    assert(ng_rpg_world_check(&w, &room, &idx) == NGR_ERR_WORLD && room == 0 && idx == 0);
    bad_exit[0].room = 1; bad_exit[0].to_x = 0;     /* lands in a wall */
    assert(ng_rpg_world_check(&w, &room, &idx) == NGR_ERR_WORLD);
    memcpy(&w, &WORLD, sizeof(w));
    memcpy(dlg, DLG, sizeof(dlg));
    dlg[1].next = 77;
    w.dialogue = dlg;
    assert(ng_rpg_world_check(&w, &room, &idx) == NGR_ERR_WORLD && idx == 1);
    assert(ng_rpg_init(&G, &w, 1) == NGR_ERR_WORLD);
    printf("world check: OK\n");
}

static void test_movement_and_party(void)
{
    int32_t x0;
    new_game(&G, 0, 3, 5);
    assert(G.leader == 0 && tile_x(&G, 0) == 3 && tile_y(&G, 0) == 5);
    assert(!(ng_rpg_tile(&G, tile_x(&G, 1), tile_y(&G, 1)) & NGR_T_SOLID));
    x0 = G.party[0].x;
    run(&G, NGR_IN_RIGHT, 30);
    assert(G.party[0].x - x0 == 30 * 24 * 16);
    run(&G, NGR_IN_UP, 120);                     /* up to the wall */
    assert(tile_y(&G, 0) == 1);
    /* the follower keeps up */
    run(&G, NGR_IN_DOWN, 40);
    run(&G, 0, 60);
    {
        int32_t dx = G.party[1].x - G.party[0].x, dy = G.party[1].y - G.party[0].y;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        assert(dx + dy < (40 << 8));
    }
    /* Maiya can't go through the narrow gap; Luna can */
    put(&G, 0, 6, 6, NGR_FACE_DOWN);
    put(&G, 1, 3, 3, NGR_FACE_DOWN);
    run(&G, NGR_IN_DOWN, 30);
    assert(tile_y(&G, 0) == 6);
    assert(ng_rpg_switch(&G, NGR_NONE) == NGR_OK && G.leader == 1);
    assert(ev_count[NGR_EV_SWITCH] == 0);        /* not drained yet */
    drain(&G);
    assert(ev_count[NGR_EV_SWITCH] == 1);
    put(&G, 1, 6, 6, NGR_FACE_DOWN);
    put(&G, 0, 6, 5, NGR_FACE_DOWN);
    run(&G, NGR_IN_DOWN, 60);
    assert(tile_y(&G, 1) == 9);                   /* in the pocket */
    /* Maiya can't follow: stuck, then put beside Luna */
    memset(ev_count, 0, sizeof(ev_count));
    run(&G, 0, 400);
    assert(ev_count[NGR_EV_STUCK] >= 1 && ev_count[NGR_EV_TELEPORT] == 1);
    assert(tile_x(&G, 0) == 6 && (tile_y(&G, 0) == 8 || tile_y(&G, 0) == 9));
    printf("movement, walls, narrow gap, follower, stuck and recovered: OK\n");

    /* without teleports the stuck follower stays put and says so */
    {
        NGRpgWorld w;
        memcpy(&w, &WORLD, sizeof(w));
        w.flags = NGR_WF_NO_TELEPORT;
        assert(ng_rpg_init(&H, &w, 7) == NGR_OK);
        ng_rpg_party_add(&H, &MAIYA_DEF); ng_rpg_party_add(&H, &LUNA_DEF);
        ng_rpg_enter_room(&H, 0, 6, 6, NGR_FACE_DOWN);
        ng_rpg_switch(&H, 1);
        put(&H, 1, 6, 6, NGR_FACE_DOWN); put(&H, 0, 6, 5, NGR_FACE_DOWN);
        memset(ev_count, 0, sizeof(ev_count));
        run(&H, NGR_IN_DOWN, 60);
        run(&H, 0, 400);
        assert(ev_count[NGR_EV_STUCK] >= 1 && ev_count[NGR_EV_TELEPORT] == 0);
        assert(tile_y(&H, 0) <= 6);
    }
    /* around the long way: the path search finds a route the trail can't */
    new_game(&G, 0, 3, 8);
    ng_rpg_switch(&G, 1);
    put(&G, 1, 3, 8, NGR_FACE_RIGHT); put(&G, 0, 3, 9, NGR_FACE_RIGHT);
    run(&G, NGR_IN_UP, 40);
    run(&G, NGR_IN_RIGHT, 70);
    run(&G, NGR_IN_DOWN, 60);
    run(&G, 0, 300);
    {
        int32_t dx = G.party[1].x - G.party[0].x, dy = G.party[1].y - G.party[0].y;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        assert(dx + dy < (48 << 8));
    }
    /* wait and assist */
    assert(ng_rpg_order(&G, 0, NGR_MODE_WAIT, 0, 0) == NGR_OK);
    {
        int32_t wx = G.party[0].x, wy = G.party[0].y;
        run(&G, NGR_IN_LEFT, 60);
        assert(G.party[0].x == wx && G.party[0].y == wy);
    }
    assert(ng_rpg_order(&G, 0, NGR_MODE_ASSIST, 12, 3) == NGR_OK);
    run(&G, 0, 400);
    assert(tile_x(&G, 0) == 12 && tile_y(&G, 0) == 3);
    assert(ng_rpg_order(&G, 0, NGR_MODE_ASSIST, 0, 0) == NGR_ERR_NOPE);   /* a wall */
    printf("path search around walls, wait, assist: OK\n");
}

static void test_dialogue_and_items(void)
{
    new_game(&G, 0, 3, 3);
    put(&G, 0, 3, 3, NGR_FACE_UP);
    assert(ng_rpg_interact(&G) == NGR_OK && G.dlg_open && G.dlg_node == 0);
    assert(ng_rpg_dialogue_offered(&G) == 0x5);           /* choice 1 hidden */
    assert(ng_rpg_dialogue_next(&G) == NGR_ERR_STATE);    /* a choice is wanted */
    assert(ng_rpg_dialogue_choose(&G, 1) == NGR_ERR_ARG);
    assert(ng_rpg_interact(&G) == NGR_ERR_STATE);         /* talking already */
    run(&G, NGR_IN_LEFT, 10);
    assert(tile_x(&G, 0) == 3);                            /* nobody walks off mid-sentence */
    assert(ng_rpg_dialogue_choose(&G, 0) == NGR_OK && G.dlg_node == 1 && ng_rpg_flag(&G, 20));
    assert(ng_rpg_dialogue_next(&G) == NGR_OK && !G.dlg_open);
    assert(ng_rpg_interact(&G) == NGR_OK && ng_rpg_dialogue_offered(&G) == 0x7);
    assert(ng_rpg_dialogue_choose(&G, 1) == NGR_OK && ng_rpg_count(&G, KEY) == 1);
    ng_rpg_dialogue_next(&G);
    /* a sign whose node depends on a flag */
    put(&G, 0, 1, 2, NGR_FACE_UP);
    assert(ng_rpg_interact(&G) == NGR_OK && G.dlg_node == 3);
    ng_rpg_dialogue_next(&G);
    ng_rpg_set_flag(&G, 21, 1);
    assert(ng_rpg_interact(&G) == NGR_OK && G.dlg_node == 4);
    ng_rpg_dialogue_next(&G);
    /* the chest opens once and stays open */
    put(&G, 0, 12, 3, NGR_FACE_UP);
    assert(ng_rpg_interact(&G) == NGR_OK && ng_rpg_count(&G, ROPE) == 1 && ng_rpg_flag(&G, 10));
    assert(ng_rpg_interact(&G) == NGR_ERR_STATE);
    ng_rpg_enter_room(&G, 0, 3, 5, NGR_FACE_DOWN);
    assert(G.obj[1].state == 1);
    /* the pickup: a collectible */
    put(&G, 0, 14, 2, NGR_FACE_UP);
    run(&G, NGR_IN_UP, 10);
    assert(ng_rpg_collected(&G, 5) && ng_rpg_flag(&G, 12) && !G.obj[8].alive);
    printf("dialogue choices and conditions, chest, pickup: OK\n");

    /* inventory: full, keys always fit and never drop, crafting is all or nothing */
    {
        uint8_t got = ng_rpg_give(&G, WOOD, 200), more;
        assert(got == 200);
        more = ng_rpg_give(&G, FIBRE, 255);
        assert(more < 255 && ng_rpg_count(&G, FIBRE) == more);
        assert(ng_rpg_give(&G, BERRY, 1) == 0);
        assert(ng_rpg_give(&G, KEY, 1) == 1 && ng_rpg_count(&G, KEY) == 2);
        assert(ng_rpg_drop(&G, KEY, 1) == NGR_ERR_KEY);
        assert(ng_rpg_drop(&G, WOOD, 10) == NGR_OK && ng_rpg_count(&G, WOOD) == 190);
        ng_rpg_take(&G, FIBRE, 255);
        assert(ng_rpg_count(&G, FIBRE) == 0);
        assert(ng_rpg_craft(&G, 0) == NGR_ERR_NOPE && ng_rpg_count(&G, WOOD) == 190);
        ng_rpg_give(&G, FIBRE, 3);
        assert(ng_rpg_craft(&G, 0) == NGR_OK && ng_rpg_count(&G, PLANK) == 1);
        assert(ng_rpg_count(&G, WOOD) == 188 && ng_rpg_count(&G, FIBRE) == 0);
        ng_rpg_take(&G, WOOD, 255); ng_rpg_take(&G, PLANK, 9);
    }
    printf("inventory limits, key items, crafting: OK\n");
}

static void test_village_puzzles(void)
{
    new_game(&G, 0, 7, 2);
    /* a block pushed into the pit fills it */
    put(&G, 0, 7, 2, NGR_FACE_DOWN);
    memset(ev_count, 0, sizeof(ev_count));
    run(&G, NGR_IN_DOWN, 40);
    assert(ev_count[NGR_EV_FILL] == 1 && ng_rpg_walkable(&G, 0, 7, 4));
    run(&G, NGR_IN_DOWN, 40);
    assert(tile_y(&G, 0) >= 4);
    /* a block onto the plate opens the gate; a reset puts it back */
    put(&G, 0, 11, 3, NGR_FACE_DOWN);
    assert(!ng_rpg_walkable(&G, 0, 13, 8));
    run(&G, NGR_IN_DOWN, 80);
    assert(G.obj[4].y == 6 && G.obj[3].state == 1 && (G.signals & 1u));
    assert(ng_rpg_walkable(&G, 0, 13, 8));
    ng_rpg_reset_room(&G);
    assert(G.obj[4].y == 4 && !(G.signals & 1u) && !ng_rpg_walkable(&G, 0, 13, 8));
    assert(ng_rpg_walkable(&G, 0, 7, 3) == 0);             /* the first block is back too */
    /* the lever's gate stays open for good */
    put(&G, 0, 2, 8, NGR_FACE_DOWN);
    assert(ng_rpg_interact(&G) == NGR_OK);
    run(&G, 0, 2);
    assert(G.obj[7].state == 1 && ng_rpg_flag(&G, 11));
    assert(ng_rpg_interact(&G) == NGR_OK);
    run(&G, 0, 2);
    assert(!(G.signals & 2u) && G.obj[7].state == 1);
    ng_rpg_reset_room(&G);
    run(&G, 0, 1);
    assert(G.obj[7].state == 1);
    printf("blocks fill pits, plates, gates, lever, permanent results, room reset: OK\n");

    /* triggers: once, every time, a watched condition */
    new_game(&G, 0, 3, 5);
    memset(ev_count, 0, sizeof(ev_count));
    run(&G, NGR_IN_RIGHT, 40);                  /* through both rects */
    assert(ng_rpg_flag(&G, 31) && G.counters[0] == 1);
    run(&G, NGR_IN_LEFT, 50);
    run(&G, NGR_IN_RIGHT, 40);
    assert(G.counters[0] == 2 && ng_rpg_flag(&G, 32) && (G.restored & (1u << 3)));
    assert(ev_count[NGR_EV_RESTORE] == 1);
    run(&G, NGR_IN_LEFT, 50);
    run(&G, NGR_IN_RIGHT, 40);
    assert(G.counters[0] == 3);
    printf("triggers (once, repeating, watched): OK\n");

    /* the exit is locked until the elder has explained */
    new_game(&G, 0, 10, 9);
    memset(ev_count, 0, sizeof(ev_count));
    run(&G, NGR_IN_DOWN, 60);
    assert(ev_count[NGR_EV_LOCKED] == 1 && G.room == 0 && !G.pending_exit);
    run(&G, NGR_IN_UP, 30);
    ng_rpg_set_flag(&G, 20, 1);
    run(&G, NGR_IN_DOWN, 60);
    assert(ev_count[NGR_EV_EXIT] == 1 && G.pending_exit);
    assert(ng_rpg_take_exit(&G) == NGR_OK && G.room == 1 && tile_x(&G, 0) == 2 && tile_y(&G, 0) == 2);
    printf("locked exit, then through: OK\n");
}

static void test_cave_networks(void)
{
    new_game(&G, 1, 6, 6);
    run(&G, 0, 2);
    assert(!(G.signals & (1u << 2)) && !(G.signals & (1u << 3)) && !(G.signals & (1u << 4)));
    /* water: open the valve */
    put(&G, 0, 5, 4, NGR_FACE_UP);
    memset(ev_count, 0, sizeof(ev_count));
    assert(ng_rpg_interact(&G) == NGR_OK);
    run(&G, 0, 2);
    assert(!ev_count[NGR_EV_WATER] && !(G.signals & (1u << 2)));   /* a lock on the channel holds the water */
    ng_rpg_give(&G, PLANK, 1);
    put(&G, 0, 9, 4, NGR_FACE_UP);
    assert(ng_rpg_interact(&G) == NGR_OK && G.obj[18].state == 1);
    run(&G, 0, 2);
    assert(ev_count[NGR_EV_WATER] == 1 && (G.signals & (1u << 2)));
    assert(G.wet[(3 * 14 + 12) >> 3] & (1u << ((3 * 14 + 12) & 7)));
    /* light: turn the mirror; the receiver wants the lens' focus */
    put(&G, 0, 3, 6, NGR_FACE_UP);
    put(&G, 0, 3, 5, NGR_FACE_LEFT);
    assert(ng_rpg_interact(&G) == NGR_OK);
    run(&G, 0, 2);
    assert(G.obj[6].state == 1 && (G.signals & (1u << 3)));
    /* gears: a gear in the empty socket turns the target */
    ng_rpg_give(&G, GEAR, 1);
    put(&G, 0, 11, 6, NGR_FACE_DOWN);
    assert(ng_rpg_interact(&G) == NGR_OK && G.obj[8].state == 1 && ng_rpg_count(&G, GEAR) == 0);
    run(&G, 0, 2);
    assert(G.gear_turn == 3 && !G.gear_jam && (G.signals & (1u << 4)));
    assert(G.obj[17].state == 1 && ng_rpg_flag(&G, 41));      /* water + light + gears */
    /* take the gear back out: the target stops, the gate stays open for good */
    assert(ng_rpg_interact(&G) == NGR_OK && ng_rpg_count(&G, GEAR) == 1);
    run(&G, 0, 2);
    assert(!(G.signals & (1u << 4)) && G.obj[17].state == 1);
    printf("water (held by a lock until it opens), light and lens, gear train, gate on all three: OK\n");

    /* a gear left in a socket without a flag comes back: on a reset, in a save, on leaving */
    assert(ng_rpg_interact(&G) == NGR_OK && G.obj[8].state == 1 && ng_rpg_count(&G, GEAR) == 0);
    ng_rpg_reset_room(&G);
    assert(G.obj[8].state == 0 && ng_rpg_count(&G, GEAR) == 1);
    assert(ng_rpg_interact(&G) == NGR_OK && ng_rpg_count(&G, GEAR) == 0);
    {
        static uint8_t buf[NGR_SAVE_SIZE];
        static NGRpg L;
        assert(ng_rpg_save(&G, buf, sizeof(buf)) == NGR_SAVE_SIZE);
        L = G;
        assert(ng_rpg_load(&L, buf, sizeof(buf)) == NGR_OK);
        assert(ng_rpg_count(&L, GEAR) == 1 && L.obj[8].state == 0);
    }
    assert(ng_rpg_enter_room(&G, 0, 2, 2, NGR_FACE_DOWN) == NGR_OK && ng_rpg_count(&G, GEAR) == 1);
    assert(ng_rpg_enter_room(&G, 1, 6, 6, NGR_FACE_DOWN) == NGR_OK);
    printf("a loose gear is never lost (reset, save, leaving): OK\n");

    /* bells in order */
    memset(ev_count, 0, sizeof(ev_count));
    put(&G, 0, 1, 2, NGR_FACE_UP);
    ng_rpg_interact(&G);                          /* wrong first bell */
    assert(ev_count[NGR_EV_SEQ_WRONG] == 0);
    drain(&G);
    assert(ev_count[NGR_EV_SEQ_WRONG] == 1 && ev_count[NGR_EV_TONE] == 1);
    put(&G, 0, 3, 2, NGR_FACE_UP); ng_rpg_interact(&G);
    put(&G, 0, 1, 2, NGR_FACE_UP); ng_rpg_interact(&G);
    put(&G, 0, 5, 2, NGR_FACE_UP); ng_rpg_interact(&G);
    run(&G, 0, 2);
    assert(ev_count[NGR_EV_SEQ_DONE] == 1 && (G.signals & (1u << 5)) && ng_rpg_flag(&G, 40));
    /* the balance: Maiya alone on the left pan tips it; Luna on the right evens it */
    put(&G, 0, 8, 1, NGR_FACE_DOWN); put(&G, 1, 4, 4, NGR_FACE_DOWN);
    run(&G, 0, 1);
    assert(G.signals & (1u << 6));
    put(&G, 1, 10, 1, NGR_FACE_DOWN);
    G.party[1].mode = NGR_MODE_WAIT;
    run(&G, 0, 1);
    assert(!(G.signals & (1u << 6)));
    printf("bell sequence, balance: OK\n");

    /* gears that mesh in a triangle jam */
    new_game(&G, 3, 3, 3);
    ng_rpg_give(&G, GEAR, 2);
    put(&G, 0, 2, 2, NGR_FACE_UP);
    assert(ng_rpg_interact(&G) == NGR_OK);
    run(&G, 0, 1);
    assert(G.gear_turn == 2 && !G.gear_jam);
    put(&G, 0, 2, 2, NGR_FACE_LEFT);
    assert(ng_rpg_interact(&G) == NGR_OK);
    run(&G, 0, 1);
    assert(G.gear_jam && G.gear_turn == 0);
    printf("jammed gear train: OK\n");
}

static void test_forest(void)
{
    int i;
    new_game(&G, 2, 2, 4);
    /* rest: heal and checkpoint */
    put(&G, 0, 1, 2, NGR_FACE_UP);
    G.party[0].hp = 2;
    assert(ng_rpg_interact(&G) == NGR_OK && G.party[0].hp == 6 && G.ck_room == 2 && G.ck_x == 1 && G.ck_y == 2);
    /* the creature chases and hurts once, then the leader is safe for a while */
    memset(ev_count, 0, sizeof(ev_count));
    put(&G, 0, 6, 5, NGR_FACE_RIGHT);
    run(&G, 0, 90);                               /* half a pixel a frame, 22 pixels to go */
    assert(ev_count[NGR_EV_HURT] >= 1 && G.party[0].hp < 6);
    /* calm it with two strikes (the second only after its stun) */
    for (i = 0; i < 400 && G.obj[0].alive; i++) {
        G.obj[0].px = G.party[0].x + (12 << 8) * (G.party[0].facing == NGR_FACE_RIGHT ? 1 : -1);
        G.obj[0].py = G.party[0].y;
        G.party[0].invuln = 60;
        ng_rpg_strike(&G);
        run(&G, 0, 1);
    }
    assert(!G.obj[0].alive && ng_rpg_flag(&G, 50) && ev_count[NGR_EV_CALMED] == 1 && ev_count[NGR_EV_HIT] == 2);
    ng_rpg_enter_room(&G, 2, 2, 4, NGR_FACE_DOWN);
    assert(!G.obj[0].alive);
    /* a hidden passage opened with the shovel */
    put(&G, 0, 10, 2, NGR_FACE_UP);
    assert(ng_rpg_use_item(&G, SHOVEL) == NGR_ERR_NOPE);
    ng_rpg_give(&G, SHOVEL, 1);
    assert(!ng_rpg_walkable(&G, 0, 10, 1));
    assert(ng_rpg_use_item(&G, SHOVEL) == NGR_OK && ng_rpg_flag(&G, 51) && ng_rpg_walkable(&G, 0, 10, 1));
    /* a lock: no key, then the key (used up) */
    put(&G, 0, 10, 5, NGR_FACE_DOWN);
    assert(ng_rpg_interact(&G) == NGR_ERR_NOPE);
    ng_rpg_give(&G, KEY, 1);
    assert(ng_rpg_interact(&G) == NGR_OK && ng_rpg_count(&G, KEY) == 0 && ng_rpg_flag(&G, 52));
    /* supplies heal */
    G.party[0].hp = 3;
    ng_rpg_give(&G, BERRY, 2);
    assert(ng_rpg_use_item(&G, BERRY) == NGR_OK && G.party[0].hp == 5 && ng_rpg_count(&G, BERRY) == 1);
    /* the platform carries whoever stands on it over the water */
    put(&G, 0, 3, 2, NGR_FACE_RIGHT);
    {
        int32_t x0 = G.party[0].x;
        run(&G, 0, 120);
        assert(G.party[0].x > x0 + (16 << 8));
    }
    /* a platform with inputs waits until they are on (a winch with its rope) */
    put(&G, 0, 3, 3, NGR_FACE_RIGHT);
    {
        int32_t x0 = G.party[0].x;
        run(&G, 0, 120);
        assert(G.party[0].x == x0 && G.obj[5].x == 3);
        put(&G, 0, 8, 5, NGR_FACE_UP);
        assert(ng_rpg_interact(&G) == NGR_OK && G.obj[6].state == 1);
        put(&G, 0, 3, 3, NGR_FACE_RIGHT);
        run(&G, 0, 120);
        assert(G.party[0].x > x0 + (16 << 8));
    }
    /* a lever with a flag stays where it was left when the room is entered again */
    assert(ng_rpg_flag(&G, 53));
    assert(ng_rpg_enter_room(&G, 2, 1, 1, NGR_FACE_DOWN) == NGR_OK);
    run(&G, 0, 2);
    assert(G.obj[6].state == 1 && (G.signals & (1u << 0)));
    put(&G, 0, 8, 5, NGR_FACE_UP);
    assert(ng_rpg_interact(&G) == NGR_OK && G.obj[6].state == 0 && !ng_rpg_flag(&G, 53));
    assert(ng_rpg_enter_room(&G, 2, 1, 1, NGR_FACE_DOWN) == NGR_OK);
    run(&G, 0, 2);
    assert(G.obj[6].state == 0);
    /* hazard tiles hurt; down at zero, back at the checkpoint healed */
    memset(ev_count, 0, sizeof(ev_count));
    put(&G, 0, 2, 5, NGR_FACE_DOWN);
    G.party[0].hp = 1; G.party[0].invuln = 0;
    run(&G, 0, 2);
    assert(G.down && ev_count[NGR_EV_DOWN] == 1);
    {
        int32_t x = G.party[0].x;
        run(&G, NGR_IN_RIGHT, 10);
        assert(G.party[0].x == x);                 /* frozen while down */
    }
    ng_rpg_respawn(&G);
    assert(!G.down && G.party[0].hp == 6 && G.room == 2 && tile_x(&G, 0) == 1 && tile_y(&G, 0) == 2);
    printf("rest, creature (hurt, calmed for good), hidden passage, lock, supplies, platforms, remembered lever, down and back: OK\n");
}

static void test_saves_and_limits(void)
{
    uint8_t buf[NGR_SAVE_SIZE];
    uint16_t n;
    new_game(&G, 0, 3, 5);
    ng_rpg_set_flag(&G, 20, 1); ng_rpg_set_flag(&G, 77, 1);
    G.counters[5] = 9;
    ng_rpg_give(&G, WOOD, 7); ng_rpg_give(&G, KEY, 1);
    put(&G, 0, 9, 9, NGR_FACE_LEFT);
    G.ck_room = 0; G.ck_x = 3; G.ck_y = 5; G.restored = 0x21;
    n = ng_rpg_save(&G, buf, sizeof(buf));
    assert(n == NGR_SAVE_SIZE);
    assert(ng_rpg_init(&H, &WORLD, 99) == NGR_OK);
    ng_rpg_party_add(&H, &MAIYA_DEF); ng_rpg_party_add(&H, &LUNA_DEF);
    assert(ng_rpg_load(&H, buf, n) == NGR_OK);
    assert(memcmp(G.flags, H.flags, sizeof(G.flags)) == 0 && H.counters[5] == 9);
    assert(ng_rpg_count(&H, WOOD) == 7 && ng_rpg_count(&H, KEY) == 1 && H.restored == 0x21);
    assert(H.room == 0 && tile_x(&H, 0) == 9 && tile_y(&H, 0) == 9 && H.rng == G.rng);
    buf[40] ^= 1;
    assert(ng_rpg_load(&H, buf, n) == NGR_ERR_SAVE);
    buf[40] ^= 1; buf[4] = 2;
    assert(ng_rpg_load(&H, buf, n) == NGR_ERR_SAVE);
    buf[4] = 1;
    {   /* a different party is refused */
        NGRpg *k = &H;
        assert(ng_rpg_init(k, &WORLD, 1) == NGR_OK);
        ng_rpg_party_add(k, &LUNA_DEF); ng_rpg_party_add(k, &MAIYA_DEF);
        assert(ng_rpg_load(k, buf, n) == NGR_ERR_SAVE);
    }
    printf("save, load, corrupt and foreign saves refused: OK\n");

    /* the event queue is bounded */
    {
        int i;
        new_game(&G, 0, 3, 5);
        for (i = 0; i < 80; i++) ng_rpg_set_flag(&G, (uint16_t)(100 + i), 1);
        assert(G.ev_count == NGR_EVENT_QUEUE && G.ev_dropped > 0);
        drain(&G);
    }
    /* same inputs, same game */
    {
        static const uint8_t script[] = { NGR_IN_RIGHT, NGR_IN_DOWN, NGR_IN_RIGHT | NGR_IN_DOWN, 0, NGR_IN_LEFT, NGR_IN_UP };
        uint32_t h1, h2;
        int r, i;
        for (r = 0; r < 2; r++) {
            NGRpg *g = r ? &H : &G;
            new_game(g, 2, 2, 4);
            for (i = 0; i < 600; i++) { ng_rpg_input(g, script[(i >> 5) % 6]); ng_rpg_update(g); drain(g); }
        }
        h1 = ng_rpg_hash(&G); h2 = ng_rpg_hash(&H);
        assert(h1 == h2);
    }
    printf("bounded events, determinism: OK\n");
}

/* A sister joins for a while and leaves again (Maiya & Luna's Chapter VI). */
static void test_party_remove(void)
{
    static uint8_t buf[NGR_SAVE_SIZE];
    assert(ng_rpg_init(&G, &WORLD, 7) == NGR_OK);
    assert(ng_rpg_party_add(&G, &LUNA_DEF) == 0);
    assert(ng_rpg_party_remove(&G, 0) == NGR_ERR_ARG);           /* never the last one */
    assert(ng_rpg_enter_room(&G, 0, 3, 5, NGR_FACE_DOWN) == NGR_OK);
    assert(ng_rpg_switch(&G, NGR_NONE) == NGR_ERR_STATE);        /* alone: nobody to switch to */
    assert(ng_rpg_party_add(&G, &MAIYA_DEF) == 1);
    assert(ng_rpg_switch(&G, 1) == NGR_OK && G.leader == 1);
    drain(&G);
    run(&G, NGR_IN_RIGHT, 20);
    assert(ng_rpg_party_remove(&G, 4) == NGR_ERR_ARG);
    assert(ng_rpg_party_remove(&G, 1) == NGR_OK);                /* the leader goes: Luna leads */
    assert(G.nparty == 1 && G.leader == 0 && G.party[0].id == LUNA_DEF.id && G.party[0].mode == NGR_MODE_LEAD);
    run(&G, NGR_IN_DOWN, 20);
    assert(tile_y(&G, 0) > 5);
    /* a solo save loads into a solo party, not into a pair */
    assert(ng_rpg_save(&G, buf, sizeof(buf)) == NGR_SAVE_SIZE);
    assert(ng_rpg_init(&H, &WORLD, 7) == NGR_OK);
    ng_rpg_party_add(&H, &LUNA_DEF);
    assert(ng_rpg_load(&H, buf, sizeof(buf)) == NGR_OK && H.nparty == 1 && H.party[0].id == LUNA_DEF.id);
    ng_rpg_party_add(&H, &MAIYA_DEF);
    assert(ng_rpg_load(&H, buf, sizeof(buf)) == NGR_ERR_SAVE);
    printf("party: a member joins, leads and leaves; solo saves: OK\n");
}

int main(void)
{
    setup_world();
    test_world_check();
    test_movement_and_party();
    test_party_remove();
    test_dialogue_and_items();
    test_village_puzzles();
    test_cave_networks();
    test_forest();
    test_saves_and_limits();
    printf("ng_rpg: all tests passed (context %u bytes, save %u bytes)\n", (unsigned)sizeof(NGRpg), (unsigned)NGR_SAVE_SIZE);
    return 0;
}
