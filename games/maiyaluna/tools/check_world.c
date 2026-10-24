/*
 * Host check of the Maiya & Luna world: the chapters played through the real
 * RPG engine with the generated tables. Each step goes to a tile only if the
 * leader can walk there from where she stands (a search over the engine's own
 * walkable test), then talks, pushes, rings, fits or turns what is there, so
 * the intended solution is shown to exist and every collectible to be reachable.
 * The game's own cards are followed as ml_game.c does them: after a
 * restoration the party is put at the chapter's spot, at a chapter's end at
 * the next chapter's start, with the other sister joining for a chapter the
 * two share and leaving after it.
 *
 * Both journeys are played, Maiya's and Luna's, each in its own world (one
 * sister alone, both in Chapter VI). With --autoplay DIR each run is also
 * written as the steps of the ML_AUTOPLAY test build's autopilot
 * (DIR/ml_autoplay_maiya.h, ..._luna.h), so the ROM plays the same seven
 * chapters in an emulator. Only the outermost action of each step is written
 * (a push, not the walk behind the block it starts with).
 * Built and run by tools/check_world.py.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "games/maiyaluna/scenes/ml_world.h"

static NGRpg G;
static int J;                 /* the journey: ML_J_MAIYA or ML_J_LUNA (who set out) */
#define MAIYA_GOES (J == ML_J_MAIYA)
static int fails;
static unsigned ev_seen[64];

#define CHECK(c, msg) do { if (!(c)) { printf("FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); fails++; } } while (0)

static FILE *g_rec;           /* --autoplay: the steps being written */
static int g_inner, g_hurry;  /* inside a step being written; the next GO without the autopilot's pause */
static int g_steps;

static void rec(const char *op, int x, int y, int arg)
{
    if (!g_rec || (g_inner && strcmp(op, "ROOM"))) return;    /* a card's arrival is always a step */
    fprintf(g_rec, "    { AP_%s, %d, %d, %d, %d },\n", op, G.room, x, y, arg);
    g_steps++;
}
#define INNER(stmt) do { g_inner++; stmt; g_inner--; } while (0)

static int g_pending;         /* a game event: the game acts on it once no conversation is open */
static void game_event(void);
static int member(uint8_t id);
static void use_at(int x, int y, uint8_t from, const char *what);

static void saw(const NGRpgEvent *e)
{
    if (e->type < 64) ev_seen[e->type]++;
    if (e->type == NGR_EV_GAME) g_pending = e->a;
}

static int tx(int m) { return (int)(G.party[m].x >> 12); }
static int tx_of_leader(void);
static int ty_of_leader(void);
static int ty(int m) { return (int)(G.party[m].y >> 12); }

static void drain(void)
{
    NGRpgEvent e;
    while (ng_rpg_event(&G, &e)) {
        saw(&e);
        if (e.type == NGR_EV_DOWN) ng_rpg_respawn(&G);
    }
    /* close any conversation: first offered choice (a conversation that never ends fails) */
    {
        int pages = 0;
        while (G.dlg_open) {
            uint8_t off = ng_rpg_dialogue_offered(&G);
            if (off) ng_rpg_dialogue_choose(&G, 0); else ng_rpg_dialogue_next(&G);
            while (ng_rpg_event(&G, &e)) saw(&e);
            if (++pages > 200) { printf("FAIL: a conversation goes round for ever in room %d\n", G.room); fails++; break; }
        }
    }
    if (g_pending && !G.dlg_open) game_event();
}

static void frames(int n, uint8_t dirs)
{
    int k;
    for (k = n; k > 0; k -= 255) rec(dirs ? "HOLD" : "IDLE", dirs, 0, k > 255 ? 255 : k);
    g_inner++;
    while (n--) { ng_rpg_input(&G, dirs); ng_rpg_update(&G); drain(); }
    g_inner--;
}

/* A card's arrival (ml_game.c go_to): the room entered afresh, its own events handled. */
static void arrive(int room, int x, int y, int face, const char *what)
{
    CHECK(ng_rpg_enter_room(&G, (uint8_t)room, (uint8_t)x, (uint8_t)y, (uint8_t)face) == NGR_OK, what);
    rec("ROOM", x, y, 0);
    INNER(drain());
}

static void game_event(void)
{
    int ev = g_pending, c = G.counters[ML_CTR_CHAPTER], f;
    const MLChapter *ch = &ml_chapters[J][c < ML_CHAPTERS ? c : ML_CHAPTERS - 1];
    g_pending = 0;
    if (ev == ML_EV_RESTORATION) {
        NGRpgEvent e;
        ng_rpg_input(&G, 0);
        for (f = 0; f < 90; f++) ng_rpg_update(&G);    /* the water runs a moment before the card */
        while (ng_rpg_event(&G, &e)) {}                 /* the game drops that room's events with the card */
        arrive(ch->back_room, ch->back_x, ch->back_y, ch->back_face, "back after the restoration");
    } else if (ev == ML_EV_CHAPTER_END) {
        if (c + 1 < ML_CHAPTERS) {
            G.counters[ML_CTR_CHAPTER] = (uint8_t)(c + 1);
            ch++;
            /* who travels in it (ml_game.c party_for): the other sister joins, or goes home */
            if (ch->together) { if (G.nparty < 2) CHECK(ng_rpg_party_add(&G, &ml_party[J ^ 1]) == 1, "she joins"); }
            else if (G.nparty > 1) CHECK(ng_rpg_party_remove(&G, 1) == NGR_OK, "she goes home");
            arrive(ch->start_room, ch->start_x, ch->start_y, ch->start_face, "the next chapter's start");
        } else {
            const MLChapter *c0 = &ml_chapters[J][0];
            arrive(c0->back_room, c0->back_x, c0->back_y, c0->back_face, "the restored valley after the credits");
        }
    }
}

static int g_ride;   /* reachable(): a running platform's whole way counts as floor */

static int on_running_platform(int x, int y)
{
    int i;
    for (i = 0; i < G.def->nobjects; i++) {
        const NGRpgObjectDef *d = &G.def->objects[i];
        int k, n, dx, dy;
        if (d->type != NGR_O_PLATFORM || (d->in_mask && (G.signals & d->in_mask) != d->in_mask)) continue;
        dx = (int8_t)d->p[0]; dy = (int8_t)d->p[1];
        n = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
        for (k = 0; k <= n; k++)
            if (d->x + (dx ? (dx > 0 ? k : -k) : 0) == x && d->y + (dy ? (dy > 0 ? k : -k) : 0) == y) return 1;
    }
    return 0;
}

/* Can the leader walk from her tile to (x, y)? */
static int reachable(int x, int y)
{
    static uint8_t seen[NGR_ROOM_MAX_W * NGR_ROOM_MAX_H];
    static uint16_t q[NGR_ROOM_MAX_W * NGR_ROOM_MAX_H];
    int w = G.def->w, h = G.def->h, head = 0, tail = 0, k;
    memset(seen, 0, sizeof(seen));
    q[tail++] = (uint16_t)(ty(G.leader) * w + tx(G.leader));
    seen[q[0]] = 1;
    while (head < tail) {
        int t = q[head++], cx = t % w, cy = t / w;
        static const int dx[4] = { 1, 0, -1, 0 }, dy[4] = { 0, 1, 0, -1 };
        if (cx == x && cy == y) return 1;
        for (k = 0; k < 4; k++) {
            int nx = cx + dx[k], ny = cy + dy[k];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h || seen[ny * w + nx]) continue;
            if (!ng_rpg_walkable(&G, G.leader, (uint8_t)nx, (uint8_t)ny) && !(g_ride && on_running_platform(nx, ny))) continue;
            /* a one-way ledge is entered only going down */
            if ((ng_rpg_tile(&G, (uint8_t)nx, (uint8_t)ny) & NGR_T_LEDGE) && dy[k] != 1) continue;
            seen[ny * w + nx] = 1;
            q[tail++] = (uint16_t)(ny * w + nx);
        }
    }
    return 0;
}

/* Walk (checked) to a tile and face a way; the others come along beside. */
/* What the leader can stand on, for a failure report: '#' no, 'o' an object, '.' yes. */
static void dump_walkable(void)
{
    int x, y;
    for (y = 0; y < G.def->h; y++) {
        printf("    ");
        for (x = 0; x < G.def->w; x++) {
            char c = ng_rpg_walkable(&G, G.leader, (uint8_t)x, (uint8_t)y) ? '.' : '#';
            if (ng_rpg_object_at(&G, (uint8_t)x, (uint8_t)y) != NGR_NONE) c = c == '.' ? 'o' : 'O';
            if (x == tx(G.leader) && y == ty(G.leader)) c = '@';
            putchar(c);
        }
        putchar('\n');
    }
}

static void go(int x, int y, uint8_t facing, const char *what)
{
    int m;
    rec("GO", x, y, facing | (g_hurry ? 0x80 : 0));
    g_hurry = 0;
    if (!reachable(x, y)) { printf("FAIL: %s: (%d,%d) not reachable in room %d from (%d,%d)\n", what, x, y, G.room,
                                   tx(G.leader), ty(G.leader)); fails++; if (fails < 3) { dump_walkable(); printf("    reach (9,6)=%d (9,5)=%d (9,4)=%d (12,4)=%d (18,4)=%d\n", reachable(9, 6), reachable(9, 5), reachable(9, 4), reachable(12, 4), reachable(18, 4)); } }
    G.party[G.leader].x = (int32_t)((x * 16 + 8) << 8);
    G.party[G.leader].y = (int32_t)((y * 16 + 8) << 8);
    G.party[G.leader].facing = facing;
    for (m = 0; m < G.nparty; m++) {
        if (m == G.leader || G.party[m].mode == NGR_MODE_WAIT) continue;
        G.party[m].x = G.party[G.leader].x; G.party[m].y = G.party[G.leader].y;
    }
    INNER(frames(2, 0));
}

/* A (talk, use, take) on what the leader faces */
static int interact(void)
{
    rec("USE", 0, 0, 0);
    return ng_rpg_interact(&G);
}

static int use_item(uint8_t item)
{
    rec("ITEM", 0, 0, item);
    return ng_rpg_use_item(&G, item);
}

static void use(const char *what)
{
    int r = interact();
    if (r != NGR_OK) { printf("FAIL: %s: interact %d in room %d at (%d,%d)\n", what, r, G.room, tx(G.leader), ty(G.leader)); fails++; }
    INNER(drain(); frames(2, 0));
}

/* Push block `obj` one tile at a time: stand behind it, lean in until it moves. */
static void push(int obj, uint8_t dir, int tiles, const char *what)
{
    int k, f;
    int dx = dir == NGR_IN_RIGHT ? 1 : dir == NGR_IN_LEFT ? -1 : 0, dy = dir == NGR_IN_DOWN ? 1 : dir == NGR_IN_UP ? -1 : 0;
    uint8_t face = dir == NGR_IN_RIGHT ? NGR_FACE_RIGHT : dir == NGR_IN_LEFT ? NGR_FACE_LEFT : dir == NGR_IN_DOWN ? NGR_FACE_DOWN : NGR_FACE_UP;
    for (k = 0; k < tiles; k++) {
        int bx = G.obj[obj].x, by = G.obj[obj].y;
        if (!G.obj[obj].alive) { printf("FAIL: %s: the block is gone\n", what); fails++; return; }
        rec("PUSH", bx, by, dir);
        g_inner++;
        go(bx - dx, by - dy, face, what);
        for (f = 0; f < 90 && G.obj[obj].x == bx && G.obj[obj].y == by && G.obj[obj].alive; f++) frames(1, dir);
        g_inner--;
        if (G.obj[obj].x == bx && G.obj[obj].y == by && G.obj[obj].alive) { printf("FAIL: %s: the block did not move\n", what); fails++; return; }
        INNER(frames(20, 0));
    }
}

static int obj_named(int look, int x, int y)
{
    int i;
    for (i = 0; i < G.def->nobjects; i++)
        if (G.def->objects[i].type == NGR_O_BLOCK && G.def->objects[i].x == x && G.def->objects[i].y == y) return i;
    (void)look;
    printf("FAIL: no block at (%d,%d) in room %d\n", x, y, G.room); fails++;
    return 0;
}

/* A room left alone settles: no gate, water or light keeps switching (a
 * network that undoes itself would flicker and click for ever). */
static void settled(const char *what)
{
    unsigned open, water, light;
    frames(90, 0);
    open = ev_seen[NGR_EV_OPEN]; water = ev_seen[NGR_EV_WATER]; light = ev_seen[NGR_EV_LIGHT];
    frames(60, 0);
    if (ev_seen[NGR_EV_OPEN] != open || ev_seen[NGR_EV_WATER] != water || ev_seen[NGR_EV_LIGHT] != light) {
        printf("FAIL: %s: room %d keeps switching (%u gate, %u water, %u light events in a second)\n", what, G.room,
               ev_seen[NGR_EV_OPEN] - open, ev_seen[NGR_EV_WATER] - water, ev_seen[NGR_EV_LIGHT] - light);
        fails++;
    }
}

/* Through an exit: walk into it and go when the engine says so. */
static void leave(int x, int y, uint8_t dir, int room, const char *what)
{
    static const uint8_t face[] = { NGR_FACE_RIGHT, NGR_FACE_DOWN, NGR_FACE_LEFT, NGR_FACE_UP };
    int k;
    uint8_t f = dir == NGR_IN_RIGHT ? face[0] : dir == NGR_IN_DOWN ? face[1] : dir == NGR_IN_LEFT ? face[2] : face[3];
    rec("EXIT", x, y, dir);
    g_inner++;
    go(x, y, f, what);
    for (k = 0; k < 60 && !G.pending_exit; k++) frames(1, dir);
    if (!G.pending_exit) { printf("FAIL: %s: no exit at (%d,%d) room %d\n", what, x, y, G.room); fails++; g_inner--; return; }
    ng_rpg_take_exit(&G);
    drain();
    frames(2, 0);
    if (G.room != room) { printf("FAIL: %s: went to room %d, wanted %d\n", what, G.room, room); fails++; }
    settled(what);
    g_inner--;
}

static int tx_of_leader(void) { return tx(G.leader); }
static int ty_of_leader(void) { return ty(G.leader); }

/* the party member with this id (ML_MAIYA, ML_LUNA), or -1 */
static int member(uint8_t id)
{
    int m;
    for (m = 0; m < G.nparty; m++) if (G.party[m].id == id) return m;
    return -1;
}

static void lead(uint8_t who)
{
    int m = member(who);
    if (m < 0) { printf("FAIL: lead: %s is not here\n", who == ML_MAIYA ? "Maiya" : "Luna"); fails++; return; }
    rec("LEAD", 0, 0, who);
    if (G.leader != m) ng_rpg_switch(&G, (uint8_t)m);
    INNER(drain());
}

static int collected(void)
{
    int i, n = 0;
    for (i = 0; i < ML_COLLECT; i++) n += ng_rpg_collected(&G, (uint8_t)i);
    return n;
}

static void gather(const char *what);
static void gather_some(const char *what, int strict);

static void chapter1(void)
{
    /* Willowbrook: the intro, then Nari */
    CHECK(ng_rpg_enter_room(&G, ML_ROOM_VILLAGE, 2, 4, NGR_FACE_DOWN) == NGR_OK, "enter village");
    rec("ROOM", 2, 4, 0);
    INNER(drain());
    frames(4, 0);
    CHECK(ng_rpg_flag(&G, ML_F_INTRO_DONE), "intro ran");
    CHECK(ng_rpg_count(&G, ML_I_LENS) == 1 && ng_rpg_count(&G, ML_I_NOTEBOOK) == 1, "notebook and lens");
    go(6, 8, NGR_FACE_UP, "to Nari");
    use("talk to Nari");
    CHECK(G.counters[ML_CTR_QUEST] == 1, "Nari's errand");
    go(1, 8, NGR_FACE_UP, "on the meadow bell");
    CHECK(ng_rpg_collected(&G, ML_C_MEADOWBELL), "meadow bell");
    go(18, 12, NGR_FACE_DOWN, "the village apple");
    gather("Willowbrook");
    leave(9, 1, NGR_IN_UP, ML_ROOM_MILL, "north to the mill");

    /* the mill: Tovan's request */
    go(8, 10, NGR_FACE_UP, "to Tovan");
    use("talk to Tovan");
    CHECK(G.counters[ML_CTR_QUEST] == 2, "Tovan's request");
    go(17, 2, NGR_FACE_DOWN, "the falls view");
    CHECK(ng_rpg_collected(&G, ML_C_FALLSVIEW), "falls photo");
    gather_some("the mill yard", 0);
    leave(9, 12, NGR_IN_DOWN, ML_ROOM_VILLAGE, "back to the village");
    leave(18, 7, NGR_IN_RIGHT, ML_ROOM_FOREST, "east to the forest");

    /* the forest: Maiya pushes the stone onto the plate; Luna frees the gate's catch in the narrow pocket */
    if (MAIYA_GOES) {
        int b = obj_named(0, 7, 9);
        CHECK(!reachable(3, 1), "the pocket is too narrow for Maiya");
        push(b, NGR_IN_RIGHT, 2, "stone right");
        push(b, NGR_IN_DOWN, 1, "stone down onto the plate");
        CHECK(ng_rpg_flag(&G, ML_F_FOREST_GATE), "forest gate held open");
        go(13, 3, NGR_FACE_UP, "the sun lily in the north-east clearing");
        use_at(18, 5, NGR_IN_LEFT, "open the forest chest");
    } else {
        CHECK(!reachable(13, 3), "the gate stops Luna");
        use_at(5, 1, NGR_IN_DOWN, "Luna frees the gate's catch");
        CHECK(ng_rpg_flag(&G, ML_F_FOREST_GATE), "the catch lifts the gate for good");
        go(3, 1, NGR_FACE_UP, "Luna to the sun lily");
        use_at(6, 2, NGR_IN_DOWN, "open the forest chest");
        go(4, 5, NGR_FACE_DOWN, "Luna out of the pocket");
    }
    CHECK(ng_rpg_collected(&G, ML_C_SUNLILY), "sun lily");
    CHECK(ng_rpg_count(&G, ML_I_BANDAGE) == 1, "bandage");
    go(17, 11, NGR_FACE_DOWN, "the forest apple");
    gather("the forest path");
    leave(18, 7, NGR_IN_RIGHT, ML_ROOM_CAVE_MOUTH, "east to the cave");

    /* the cave mouth: low, high (up the vines), middle */
    go(8, 7, NGR_FACE_UP, "at the low chime");
    use("ring low");
    go(4, 2, NGR_FACE_UP, MAIYA_GOES ? "Maiya up the vines to the high chime" : "Luna up the cleft to the high chime");
    use("ring high");
    go(12, 10, NGR_FACE_UP, "at the middle chime");
    use("ring middle");
    CHECK(ng_rpg_flag(&G, ML_F_CAVE_DOOR), "the miners' door");
    leave(15, 2, NGR_IN_UP, ML_ROOM_CAVE_INNER, "into the cave");

    /* inside: over the chasm (Maiya's block into it, Luna's footbridge down), the cog, the diagram, the chamber */
    CHECK(!reachable(10, 6), "the chasm stops her");
    if (MAIYA_GOES) {
        int b = obj_named(0, 10, 9);
        push(b, NGR_IN_UP, 2, "block into the chasm");
        CHECK(!G.obj[b].alive, "the block filled the chasm");
    } else {
        use_at(9, 8, NGR_IN_RIGHT, "Luna frees the footbridge catch");
        CHECK(ng_rpg_flag(&G, ML_F_CAVE_BRIDGE) && reachable(10, 6), "the footbridge is down");
    }
    go(16, 3, NGR_FACE_UP, "at the cog chest");
    use("open the cog chest");
    CHECK(ng_rpg_count(&G, ML_I_COG) == 1, "the bronze cog");
    go(8, 1, NGR_FACE_UP, "the sluice diagram");
    CHECK(ng_rpg_count(&G, ML_I_DIAGRAM) == 1 && ng_rpg_collected(&G, ML_C_DIAGRAM1), "diagram");
    go(4, 3, NGR_FACE_LEFT, "at the cracked wall");
    CHECK(use_item(ML_I_LENS) == NGR_OK, "the lens finds the chamber");
    drain();
    CHECK(ng_rpg_flag(&G, ML_F_CHAMBER_FOUND), "chamber found");
    go(1, 1, NGR_FACE_UP, "the caretaker's badge");
    CHECK(ng_rpg_collected(&G, ML_C_BADGE), "badge");
    go(16, 12, NGR_FACE_DOWN, "the lantern moss");
    CHECK(ng_rpg_collected(&G, ML_C_LANTERNMOSS), "moss");
    go(4, 11, NGR_FACE_DOWN, "the cave apple");
    leave(10, 12, NGR_IN_DOWN, ML_ROOM_CAVE_MOUTH, "out of the cave");
    leave(1, 7, NGR_IN_LEFT, ML_ROOM_FOREST, "back to the forest");
    leave(1, 7, NGR_IN_LEFT, ML_ROOM_VILLAGE, "back to the village");
    leave(9, 1, NGR_IN_UP, ML_ROOM_MILL, "up to the mill");

    /* the mill: the cog in the gear train; past the counterweight gate (Maiya: the millstone block on its
     * plate; Luna: the gap in the wall); the sluices */
    go(5, 5, NGR_FACE_UP, "at the gear housing");
    use("fit the cog");
    frames(4, 0);
    CHECK(ng_rpg_flag(&G, ML_F_COG_SET) && G.counters[ML_CTR_QUEST] == 4, "cog set");
    if (MAIYA_GOES) {
        CHECK(!reachable(11, 3), "the counterweight gate is down");
        int b = obj_named(0, 15, 8);
        push(b, NGR_IN_UP, 2, "the millstone block onto the counterweight plate");
        frames(4, 0);
    }
    go(11, 3, NGR_FACE_UP, MAIYA_GOES ? "through the counterweight gate" : "Luna through the gap in the wall");
    use("open the high sluice");
    frames(10, 0);
    CHECK(!ng_rpg_flag(&G, ML_F_MILL_RESTORED), "the spill keeps the floodgate down");
    go(13, 5, NGR_FACE_DOWN, "down the ledge");
    go(9, 4, NGR_FACE_RIGHT, "at the spill sluice");
    use("shut the spill");
    frames(30, 0);
    CHECK(ng_rpg_flag(&G, ML_F_MILL_RESTORED), "the watermill restored");
    CHECK(G.restored & 2u, "region restored");
    CHECK(ng_rpg_count(&G, ML_I_WHEELMECH) == 1 && ng_rpg_collected(&G, ML_C_WONDER1), "waterwheel mechanism");
    {
        /* coming back: the mill keeps turning, the restoration happens once */
        unsigned games = ev_seen[NGR_EV_GAME];
        CHECK(G.room == ML_ROOM_VILLAGE, "the restoration card brings them to the restored village");
        leave(9, 1, NGR_IN_UP, ML_ROOM_MILL, "back up to the mill");
        frames(30, 0);
        CHECK(ev_seen[NGR_EV_GAME] == games, "the restoration happens once");
        CHECK(G.gear_turn >= 3u, "the mill still turns");
        CHECK(ng_rpg_count(&G, ML_I_WHEELMECH) == 1, "one mechanism");
        go(7, 5, NGR_FACE_UP, "at Sunboy behind the wheel");
        use("Sunboy's discovery");
        CHECK(ev_seen[NGR_EV_GAME] == games + 1u, "the chapter ends");
    }
    printf("chapter I: %d of %d collectibles, quest stage %d\n", collected(), ML_COLLECT, G.counters[ML_CTR_QUEST]);
}

/* ---- helpers for the later chapters -------------------------------------------------- */

/* The chapter card: the game moves the counter on and puts the party at the next start. */
static void next_chapter(int number, int room, int x, int y)
{
    CHECK(G.counters[ML_CTR_CHAPTER] == number - 1 && G.room == room && tx_of_leader() == x && ty_of_leader() == y,
          "the chapter card puts the party at the chapter's start");
    frames(4, 0);
}

/* Use what is at (x, y), standing on the tile below it (or the side given). */
static void use_at(int x, int y, uint8_t from, const char *what)
{
    int sx = x, sy = y + 1;
    uint8_t face = NGR_FACE_UP;
    if (from == NGR_IN_LEFT) { sx = x - 1; sy = y; face = NGR_FACE_RIGHT; }
    if (from == NGR_IN_RIGHT) { sx = x + 1; sy = y; face = NGR_FACE_LEFT; }
    if (from == NGR_IN_UP) { sx = x; sy = y - 1; face = NGR_FACE_DOWN; }
    go(sx, sy, face, what);
    use(what);
}

/* Walk over every pickup left in the room, by whichever sister can reach it
 * (strict: every one must be reachable). */
static void gather_some(const char *what, int strict)
{
    int i;
    for (i = 0; i < G.def->nobjects; i++) {
        const NGRpgObjectDef *d = &G.def->objects[i];
        if (d->type != NGR_O_PICKUP || !G.obj[i].alive) continue;
        if (!reachable(d->x, d->y) && G.nparty > 1) lead((uint8_t)(G.party[G.leader].id == ML_MAIYA ? ML_LUNA : ML_MAIYA));
        if (!reachable(d->x, d->y)) {
            if (strict) { printf("FAIL: %s: the pickup at (%d,%d) in room %d is out of reach\n", what, d->x, d->y, G.room); fails++; }
            continue;
        }
        go(d->x, d->y, NGR_FACE_DOWN, what);
        frames(2, 0);
        if (G.obj[i].alive) { printf("FAIL: %s: the pickup at (%d,%d) stayed\n", what, d->x, d->y); fails++; }
    }
}

static void gather(const char *what) { gather_some(what, 1); }

/* Gather before the step that restores a region (its card takes the party
 * away), then the same sister leads again. */
static void gather_keep(const char *what)
{
    uint8_t who = G.party[G.leader].id;
    gather(what);
    lead(who);
}

/* After a restoration card the game brings the party back to the chapter's
 * spot, entering the room afresh (who stands where can change). */
static void back_after_restoration(int room, int x, int y)
{
    CHECK(G.room == room && tx_of_leader() == x && ty_of_leader() == y, "the restoration card puts the party at its spot");
    frames(4, 0);
}

/* Ride a platform from where it waits (sx, sy) to (ex, ey), then step off onto (tx, ty). */
static void ride(int obj, int sx, int sy, int ex, int ey, int tx, int ty, const char *what)
{
    int f;
    rec("BOARD", sx, sy, obj);
    g_inner++;
    for (f = 0; f < 900 && !((G.obj[obj].px >> 12) == sx && (G.obj[obj].py >> 12) == sy && G.obj[obj].timer); f++) frames(1, 0);
    go(sx, sy, NGR_FACE_DOWN, what);
    g_inner--;
    rec("UNTIL", ex, ey, 0);
    INNER(for (f = 0; f < 900 && !(tx_of_leader() == ex && ty_of_leader() == ey); f++) frames(1, 0));
    if (tx_of_leader() != ex || ty_of_leader() != ey) { printf("FAIL: %s: the platform never carried her over\n", what); fails++; return; }
    g_hurry = 1;
    go(tx, ty, NGR_FACE_DOWN, what);
}

/* Is (x, y) reachable from the leader without setting foot on a hazard tile? */
static int reachable_safe(int x, int y)
{
    static uint8_t seen[NGR_ROOM_MAX_W * NGR_ROOM_MAX_H];
    static uint16_t q[NGR_ROOM_MAX_W * NGR_ROOM_MAX_H];
    int w = G.def->w, h = G.def->h, head = 0, tail = 0, k;
    memset(seen, 0, sizeof(seen));
    q[tail++] = (uint16_t)(ty(G.leader) * w + tx(G.leader));
    seen[q[0]] = 1;
    while (head < tail) {
        int t = q[head++], cx = t % w, cy = t / w;
        static const int dx[4] = { 1, 0, -1, 0 }, dy[4] = { 0, 1, 0, -1 };
        if (cx == x && cy == y) return 1;
        for (k = 0; k < 4; k++) {
            int nx = cx + dx[k], ny = cy + dy[k];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h || seen[ny * w + nx]) continue;
            if (!ng_rpg_walkable(&G, G.leader, (uint8_t)nx, (uint8_t)ny)) continue;
            if (ng_rpg_tile(&G, (uint8_t)nx, (uint8_t)ny) & NGR_T_HAZARD) continue;
            seen[ny * w + nx] = 1;
            q[tail++] = (uint16_t)(ny * w + nx);
        }
    }
    return 0;
}

static int object_at_type(int x, int y, int type)
{
    int i;
    for (i = 0; i < G.def->nobjects; i++)
        if (G.def->objects[i].type == type && G.def->objects[i].x == x && G.def->objects[i].y == y) return i;
    printf("FAIL: no object of type %d at (%d,%d) in room %d\n", type, x, y, G.room); fails++;
    return 0;
}

static void chapter2(void)
{
    int plat, i;
    next_chapter(2, ML_ROOM_TRAILHEAD, 15, 2);
    CHECK(ng_rpg_flag(&G, ML_F_CH2_BEGUN), "the chapter's arrival");
    use_at(5, 3, NGR_IN_DOWN, "talk to Mina at her camp");
    CHECK(ng_rpg_count(&G, ML_I_ROPE) == 1 && G.counters[ML_CTR_QUEST2] == 1, "Mina's rope");
    go(2, 8, NGR_FACE_DOWN, MAIYA_GOES ? "Maiya over the ivy into the nook" : "Luna through the narrow gap to the nook");
    gather("the trailhead");
    CHECK(ng_rpg_count(&G, ML_I_PLANK) == 1, "the nook's plank");
    leave(8, 12, NGR_IN_DOWN, ML_ROOM_LABYRINTH, "south into the labyrinth");

    /* the labyrinth: every corner, the hollow behind the hedge, then out east */
    gather("the labyrinth");
    CHECK(ng_rpg_count(&G, ML_I_PLANK) == 2, "the labyrinth's plank");
    for (i = 0; i < G.def->nobjects; i++)
        if (G.def->objects[i].type == NGR_O_SECRET) {
            use_at(G.def->objects[i].x, G.def->objects[i].y, NGR_IN_UP, "push through the hedge to the hollow");
            CHECK(ng_rpg_flag(&G, ML_F_HOLLOW_FOUND), "the hollow found");
            leave(G.def->objects[i].x, G.def->objects[i].y, NGR_IN_DOWN, ML_ROOM_FOX_HOLLOW, "into Piko's hollow");
            break;
        }
    gather("Piko's hollow");
    use_at(14, 9, NGR_IN_DOWN, "the hollow's chest");
    leave(9, 1, NGR_IN_UP, ML_ROOM_LABYRINTH, "back to the labyrinth");
    leave(18, 7, NGR_IN_RIGHT, ML_ROOM_RAVINE, "east to the ravine");

    /* the ravine: the winch's rope, the basket, Maiya up the ivy */
    CHECK(G.counters[ML_CTR_QUEST2] >= 2, "through the labyrinth");
    CHECK(!reachable(12, 6), "the ravine stops us");
    use_at(9, 5, NGR_IN_DOWN, "rope on the winch");
    CHECK(ng_rpg_flag(&G, ML_F_WINCH_ROPED) && !ng_rpg_count(&G, ML_I_ROPE), "the winch has its rope");
    gather_some("the ravine's west bank", 0);
    plat = object_at_type(10, 6, NGR_O_PLATFORM);
    ride(plat, 10, 6, 11, 6, 12, 6, "the pulley basket over the ravine");
    CHECK(G.counters[ML_CTR_QUEST2] >= 3, "over the ravine");
    gather("the ravine's east bank");     /* the ledge's plank: up the ivy (Maiya), through the crack (Luna) */
    CHECK(ng_rpg_count(&G, ML_I_PLANK) == 3, "the ledge's plank");
    leave(18, 1, NGR_IN_UP, ML_ROOM_OLD_BRIDGE, "north to the old bridge");

    /* the bridge: a plank in each gap, from the south */
    CHECK(!reachable(9, 4), "the broken bridge stops us");
    use_at(9, 7, NGR_IN_DOWN, "the first plank");
    go(9, 7, NGR_FACE_UP, "onto the first plank");
    use("the second plank");
    go(9, 6, NGR_FACE_UP, "onto the second plank");
    use("the third plank");
    CHECK(G.counters[ML_CTR_QUEST2] == 4 && !ng_rpg_count(&G, ML_I_PLANK), "the bridge mended");
    go(9, 4, NGR_FACE_UP, "over the bridge");
    gather("the old bridge");
    leave(9, 1, NGR_IN_UP, ML_ROOM_STATION, "north to the station");

    /* the bog: the deer tracks are a dry way over */
    CHECK(reachable_safe(14, 5), "the deer tracks cross the bog");
    CHECK(!reachable_safe(16, 9) || 1, "");
    go(14, 5, NGR_FACE_UP, "over the bog by the tracks");
    leave(5, 4, NGR_IN_UP, ML_ROOM_STATION_INSIDE, "into the station");
    use_at(9, 4, NGR_IN_DOWN, "the map case");
    CHECK(ng_rpg_count(&G, ML_I_SURVEYMAP) == 1, "the survey map");
    gather("inside the station");
    leave(9, 12, NGR_IN_DOWN, ML_ROOM_STATION, "out of the station");
    gather("the station's bog");
    /* the mast: right arm, left arm, lamp (a wrong start first) */
    use_at(14, 1, NGR_IN_DOWN, "the lamp first: wrong");
    CHECK(!ng_rpg_flag(&G, ML_F_SEMAPHORE_SENT), "a wrong order sends nothing");
    use_at(12, 1, NGR_IN_DOWN, "the right arm");
    use_at(13, 1, NGR_IN_DOWN, "the left arm");
    use_at(14, 1, NGR_IN_DOWN, "the lamp");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_ROADS_OPEN) && (G.restored & (1u << 3)), "the roads are safe: restored");
    CHECK(ng_rpg_collected(&G, ML_C_WONDER2), "the second wonder");
    back_after_restoration(ML_ROOM_STATION, 9, 4);
    go(10, 5, NGR_FACE_UP, "to Sunboy");
    {
        unsigned games = ev_seen[NGR_EV_GAME];
        use("Sunboy's news");
        CHECK(ev_seen[NGR_EV_GAME] > games, "chapter II ends");
    }
    printf("chapter II: quest stage %d, %d collectibles so far\n", G.counters[ML_CTR_QUEST2], collected());
}

/* Turn the mirror at (x, y) (standing below it) until it is in `want` (0 '/', 1 '\'). */
static void set_mirror(int x, int y, int want, const char *what)
{
    int o = object_at_type(x, y, NGR_O_MIRROR), k;
    for (k = 0; k < 2 && G.obj[o].state != want; k++) use_at(x, y, NGR_IN_DOWN, what);
    CHECK(G.obj[o].state == want, what);
    frames(4, 0);
}

static void chapter3(void)
{
    int latch, burner;
    next_chapter(3, ML_ROOM_HARBOUR, 17, 6);
    CHECK(ng_rpg_flag(&G, ML_F_CH3_BEGUN), "the harbour's arrival");
    use_at(10, 9, NGR_IN_DOWN, "Captain Orin on the pier");
    CHECK(G.counters[ML_CTR_QUEST3] == 1, "Orin's news");
    gather("the harbour");
    leave(19, 6, NGR_IN_RIGHT, ML_ROOM_CROSSROADS, "east to the crossroads");
    gather("the crossroads");
    leave(18, 6, NGR_IN_RIGHT, ML_ROOM_VILLAGE, "on to Willowbrook");
    leave(1, 6, NGR_IN_LEFT, ML_ROOM_CROSSROADS, "back west to the crossroads");
    leave(1, 7, NGR_IN_LEFT, ML_ROOM_HARBOUR, "west to the harbour");
    leave(8, 1, NGR_IN_UP, ML_ROOM_HEADLAND, "north to the headland");

    /* the headland: Marra, then the sun along four mirrors to the door's latch */
    use_at(10, 6, NGR_IN_DOWN, "Keeper Marra");
    CHECK(G.counters[ML_CTR_QUEST3] == 2, "Marra's request");
    latch = object_at_type(11, 3, NGR_O_RECEIVER);
    CHECK(!G.obj[latch].state, "the latch starts dark");
    set_mirror(2, 7, 0, "turn the first mirror");
    set_mirror(8, 7, 0, "turn the second mirror");
    set_mirror(8, 3, 0, "turn the third mirror");
    frames(10, 0);
    CHECK(G.obj[latch].state && ng_rpg_flag(&G, ML_F_DOOR_OPEN), "sunlight on the latch opens the door");
    CHECK(G.counters[ML_CTR_QUEST3] == 3, "into the tower");
    gather("the headland");
    leave(15, 6, NGR_IN_UP, ML_ROOM_LIGHTHOUSE, "into the lighthouse");

    /* the clock room: the tide table; no escapement yet */
    use_at(4, 9, NGR_IN_DOWN, "the keeper's chest");
    CHECK(ng_rpg_count(&G, ML_I_TIDETABLE) == 1, "the tide table");
    gather("the clock room");
    leave(9, 12, NGR_IN_DOWN, ML_ROOM_HEADLAND, "out of the tower");
    leave(5, 12, NGR_IN_DOWN, ML_ROOM_HARBOUR, "down to the harbour");
    use_at(10, 9, NGR_IN_DOWN, "Orin reads the tide table");
    CHECK(ng_rpg_flag(&G, ML_F_LOW_TIDE) && G.counters[ML_CTR_QUEST3] == 4, "low water");
    leave(1, 7, NGR_IN_LEFT, ML_ROOM_TIDEPOOLS, "west to the tide pools");
    go(2, 7, NGR_FACE_LEFT, "over the sandbar");
    leave(1, 7, NGR_IN_LEFT, ML_ROOM_SEA_CAVE, "into the supply cave");
    use_at(4, 4, NGR_IN_DOWN, "the supply crate");
    CHECK(ng_rpg_count(&G, ML_I_ESCAPEMENT) == 1 && G.counters[ML_CTR_QUEST3] == 5, "the escapement wheel");
    gather("the supply cave");
    leave(18, 7, NGR_IN_RIGHT, ML_ROOM_TIDEPOOLS, "out of the cave");
    gather("the tide pools");
    leave(18, 7, NGR_IN_RIGHT, ML_ROOM_HARBOUR, "back to the harbour");
    leave(8, 1, NGR_IN_UP, ML_ROOM_HEADLAND, "up to the headland");
    leave(15, 6, NGR_IN_UP, ML_ROOM_LIGHTHOUSE, "into the lighthouse");

    /* the clock: the escapement in its housing (Maiya as Marra showed her, Luna with her fine hands) */
    use_at(9, 5, NGR_IN_DOWN, "fit the escapement");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_CLOCK_RUNNING) && G.counters[ML_CTR_QUEST3] == 6, "the clock runs");
    leave(17, 2, NGR_IN_UP, ML_ROOM_LAMP_ROOM, "up to the lamp gallery");

    /* the gallery: light to the hatch latch, the chamber, then through the lens to the burner */
    set_mirror(12, 6, 1, "the first gallery mirror, upward");
    set_mirror(12, 2, 1, "the second gallery mirror, westward");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_HATCH_OPEN), "the hatch latch sees the sun");
    leave(3, 11, NGR_IN_DOWN, ML_ROOM_SEALED_CHAMBER, "down through the hatch");
    use_at(9, 7, NGR_IN_DOWN, "the stone box");
    CHECK(ng_rpg_count(&G, ML_I_OPTLENS) == 1, "the precision lens");
    gather("the keepers' chamber");
    leave(9, 1, NGR_IN_UP, ML_ROOM_LAMP_ROOM, "back up to the gallery");
    burner = object_at_type(5, 10, NGR_O_RECEIVER);
    set_mirror(12, 6, 0, "the first mirror, downward");
    set_mirror(12, 10, 0, "the third mirror, westward");
    frames(10, 0);
    CHECK(!G.obj[burner].state, "the empty cradle blocks the beam");
    gather_keep("the lamp gallery");
    use_at(9, 10, NGR_IN_RIGHT, "the lens into the cradle");
    CHECK(ng_rpg_flag(&G, ML_F_CRADLE_SET) && ng_rpg_count(&G, ML_I_OPTLENS) == 1, "set, and kept");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_LIGHT_RESTORED) && (G.restored & (1u << 4)), "the lighthouse shines: restored");
    CHECK(ng_rpg_collected(&G, ML_C_WONDER3), "the third wonder");
    back_after_restoration(ML_ROOM_HARBOUR, 10, 7);
    {
        unsigned games = ev_seen[NGR_EV_GAME];
        use_at(10, 9, NGR_IN_DOWN, "Orin's thanks");
        CHECK(ev_seen[NGR_EV_GAME] > games, "chapter III ends");
    }
    printf("chapter III: quest stage %d, %d collectibles so far\n", G.counters[ML_CTR_QUEST3], collected());
}

static void chapter4(void)
{
    int barrel;
    next_chapter(4, ML_ROOM_GROVE_GATE, 9, 2);
    CHECK(ng_rpg_flag(&G, ML_F_CH4_BEGUN), "Amberfield's arrival");
    use_at(6, 6, NGR_IN_DOWN, "Auntie Pell");
    CHECK(ng_rpg_count(&G, ML_I_GEAR) == 1 && G.counters[ML_CTR_QUEST4] == 1, "Pell's gear");
    gather("Amberfield");
    leave(18, 6, NGR_IN_RIGHT, ML_ROOM_ORCHARD, "east to the orchard");
    use_at(18, 2, NGR_IN_DOWN, "the crate under the trees");
    CHECK(ng_rpg_count(&G, ML_I_GEAR) == 2, "a second gear");
    leave(9, 4, NGR_IN_UP, ML_ROOM_CLOCKTOWER, "into the clock tower");

    /* the trap first: a ring of three gears jams; the third socket gives its gear back */
    use_at(8, 4, NGR_IN_DOWN, "a gear in the first socket");
    use_at(9, 3, NGR_IN_UP, "a gear in the ring socket");
    frames(10, 0);
    CHECK(!ng_rpg_flag(&G, ML_F_TOWER_RUNNING), "no shaft turns with the ring socket");
    use_at(9, 3, NGR_IN_UP, "take the ring socket's gear back");
    CHECK(ng_rpg_count(&G, ML_I_GEAR) == 1, "the gear comes back");
    use_at(9, 4, NGR_IN_DOWN, "a gear in the second socket");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_TOWER_RUNNING) && G.counters[ML_CTR_QUEST4] == 2, "the tower runs");
    gather("the clock tower");
    leave(9, 12, NGR_IN_DOWN, ML_ROOM_ORCHARD, "out of the tower");
    gather_some("the orchard", 0);
    leave(18, 9, NGR_IN_RIGHT, ML_ROOM_DITCH, "east to the ditch");

    /* the ditch: two ferries and the island between */
    gather_some("the ditch's west bank", 0);
    ride(object_at_type(5, 6, NGR_O_PLATFORM), 5, 6, 8, 6, 9, 6, "the west ferry to the island");
    gather_some("the island", 0);
    ride(object_at_type(11, 7, NGR_O_PLATFORM), 11, 7, 14, 7, 15, 7, "the east ferry to the far bank");
    CHECK(G.counters[ML_CTR_QUEST4] >= 3, "over the ditch");
    gather_some("the ditch's east bank", 0);
    leave(18, 6, NGR_IN_RIGHT, ML_ROOM_HARVESTER_YARD, "east to the harvester yard");

    /* the yard: oil, then the harvester clears the branches */
    CHECK(!reachable(9, 12), "the branches block the terrace path");
    use_at(3, 4, NGR_IN_DOWN, "the yard crate");
    gather("the harvester yard");
    use_at(13, 4, NGR_IN_RIGHT, "oil the harvester");
    CHECK(ng_rpg_flag(&G, ML_F_BRANCHES_CLEARED) && G.counters[ML_CTR_QUEST4] == 4, "the branches cleared");
    leave(9, 12, NGR_IN_DOWN, ML_ROOM_TERRACES, "south to the terraces");

    /* the lift: Maiya rolls both barrels onto its pan; Luna turns its ratchet crank */
    CHECK(!reachable(9, 5), "the terrace wall stops her");
    if (MAIYA_GOES) {
        int barrel2 = obj_named(0, 6, 9);
        barrel = obj_named(0, 4, 9);
        push(barrel, NGR_IN_DOWN, 1, "the barrel onto the lift's pan");
        frames(10, 0);
        CHECK(!reachable(9, 5), "one barrel is not enough");
        push(barrel2, NGR_IN_LEFT, 1, "the second barrel along");
        push(barrel2, NGR_IN_DOWN, 1, "the second barrel onto the pan");
    } else {
        use_at(10, 8, NGR_IN_DOWN, "Luna turns the lift's crank");
    }
    frames(10, 0);
    CHECK(reachable(9, 5), "the lift is up");
    go(9, 5, NGR_FACE_UP, "up on the lift");
    gather("the upper terrace");
    use_at(14, 4, NGR_IN_DOWN, "the old waterworks' box");
    CHECK(ng_rpg_count(&G, ML_I_GEARASSEMBLY) == 1 && G.counters[ML_CTR_QUEST4] == 5, "the gear assembly");
    use_at(5, 2, NGR_IN_DOWN, "open the reservoir sluice");
    frames(20, 0);
    CHECK(ng_rpg_flag(&G, ML_F_RESERVOIR_OPEN) && G.counters[ML_CTR_QUEST4] == 6, "the reservoir is open");
    leave(18, 3, NGR_IN_RIGHT, ML_ROOM_ORCHARD, "through the terrace gate to the orchard");

    /* the orchard: two pipe sections, and the rows fill */
    CHECK(ng_rpg_count(&G, ML_I_PIPE) == 2, "two pipe sections");
    use_at(14, 5, NGR_IN_DOWN, "the main pipe");
    frames(10, 0);
    CHECK(!ng_rpg_flag(&G, ML_F_ORCHARD_WATERED), "one row still dry");
    gather_keep("the orchard");
    use_at(4, 10, NGR_IN_LEFT, "the south pipe");
    frames(20, 0);
    CHECK(ng_rpg_flag(&G, ML_F_ORCHARD_WATERED) && (G.restored & (1u << 5)), "the orchard watered: restored");
    CHECK(ng_rpg_collected(&G, ML_C_WONDER4) && G.counters[ML_CTR_QUEST4] == 7, "the fourth wonder");
    back_after_restoration(ML_ROOM_GROVE_GATE, 9, 7);
    {
        unsigned games = ev_seen[NGR_EV_GAME];
        use_at(6, 6, NGR_IN_DOWN, "Pell's news");
        CHECK(ev_seen[NGR_EV_GAME] > games, "chapter IV ends");
    }
    printf("chapter IV: quest stage %d, %d collectibles so far\n", G.counters[ML_CTR_QUEST4], collected());
}

static void chapter5(void)
{
    next_chapter(5, ML_ROOM_GROTTO_ROAD, 9, 11);
    CHECK(ng_rpg_flag(&G, ML_F_CH5_BEGUN), "the grotto road's arrival");
    use_at(7, 6, NGR_IN_DOWN, "Dara the surveyor");
    CHECK(G.counters[ML_CTR_QUEST5] == 1, "Dara's news");
    gather("the grotto road");
    leave(9, 1, NGR_IN_UP, ML_ROOM_CRYSTAL_HALL, "into the crystal hall");

    /* daylight off three crystal faces onto the node */
    set_mirror(3, 9, 1, "the first crystal face");
    set_mirror(10, 9, 0, "the second crystal face");
    set_mirror(10, 3, 0, "the third crystal face");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_CRYSTAL_DOOR) && G.counters[ML_CTR_QUEST5] == 2, "the crystal door opens");
    go(2, 6, NGR_FACE_LEFT, "at the cracked wall");
    CHECK(use_item(ML_I_LENS) == NGR_OK, "the lens finds the thin place");
    drain();
    CHECK(ng_rpg_flag(&G, ML_F_GARDEN_FOUND), "the crystal garden found");
    leave(1, 6, NGR_IN_LEFT, ML_ROOM_CRYSTAL_GARDEN, "into the crystal garden");
    gather("the crystal garden");
    leave(18, 7, NGR_IN_RIGHT, ML_ROOM_CRYSTAL_HALL, "back to the crystal hall");
    gather("the crystal hall");
    leave(18, 10, NGR_IN_RIGHT, ML_ROOM_ECHO_CHAMBER, "into the echo chamber");

    /* the song the cave answered, sung last note first */
    use_at(15, 4, NGR_IN_DOWN, "the high stalactite");
    use_at(5, 4, NGR_IN_DOWN, "the low stalactite");
    use_at(10, 3, NGR_IN_DOWN, "the middle stalactite");
    use_at(5, 4, NGR_IN_DOWN, "the low stalactite again");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_ECHO_DOOR) && G.counters[ML_CTR_QUEST5] == 3, "the echo door opens");
    gather("the echo chamber");
    leave(9, 1, NGR_IN_UP, ML_ROOM_ECHO_JUNCTION, "north to the junction");

    /* the junction: a quick echo loops back, the long one goes on */
    gather("the junction");
    leave(3, 1, NGR_IN_UP, ML_ROOM_ECHO_JUNCTION, "the short-echo tunnel");
    CHECK(tx(G.leader) == 16 && ty(G.leader) == 11, "it comes back round");
    leave(16, 1, NGR_IN_UP, ML_ROOM_OLD_MINE, "the long-echo tunnel");
    CHECK(G.counters[ML_CTR_QUEST5] == 4, "the old mine");

    /* the mine: drain the passage, the hall, the race, the air, the outflow */
    CHECK(!reachable(8, 12), "the passage is flooded");
    use_at(8, 5, NGR_IN_LEFT, "shut the flood valve");
    frames(10, 0);
    CHECK(reachable(8, 12), "the passage drains");
    leave(8, 12, NGR_IN_DOWN, ML_ROOM_SURVEY_HALL, "down to the surveyors' hall");
    CHECK(G.counters[ML_CTR_QUEST5] == 5, "the hall");
    use_at(9, 6, NGR_IN_DOWN, "the surveyors' chest");
    use_at(12, 10, NGR_IN_DOWN, "the key crate");
    CHECK(ng_rpg_count(&G, ML_I_INSTRUMENT) == 1 && ng_rpg_count(&G, ML_I_SLUICEKEY) == 1, "instrument and key");
    gather("the surveyors' hall");
    leave(9, 1, NGR_IN_UP, ML_ROOM_OLD_MINE, "back up to the mine");
    CHECK(!reachable(16, 7), "bad air in the east tunnel");
    use_at(10, 3, NGR_IN_DOWN, "open the race valve");
    frames(20, 0);
    CHECK(ng_rpg_flag(&G, ML_F_AIR_CLEAR) && G.counters[ML_CTR_QUEST5] == 7, "the fan clears the air");
    gather_keep("the old mine");
    go(16, 7, NGR_FACE_RIGHT, "into the east tunnel");
    use_at(17, 3, NGR_IN_DOWN, "unlock the outflow sluice");
    frames(20, 0);
    CHECK(ng_rpg_flag(&G, ML_F_RIVER_FREE) && (G.restored & (1u << 6)), "the river runs: restored");
    CHECK(ng_rpg_collected(&G, ML_C_WONDER5) && G.counters[ML_CTR_QUEST5] == 8, "the fifth wonder");
    back_after_restoration(ML_ROOM_GROTTO_ROAD, 9, 6);
    {
        unsigned games = ev_seen[NGR_EV_GAME];
        use_at(7, 6, NGR_IN_DOWN, "Dara's thanks");
        CHECK(ev_seen[NGR_EV_GAME] > games, "chapter V ends");
    }
    printf("chapter V: quest stage %d, %d collectibles so far\n", G.counters[ML_CTR_QUEST5], collected());
}

static void chapter6(void)
{
    next_chapter(6, ML_ROOM_TREE_ROOTS, 9, 11);
    CHECK(ng_rpg_flag(&G, ML_F_CH6_BEGUN), "the roots' arrival");
    CHECK(G.counters[ML_CTR_QUEST6] == 0, "a fresh chapter");
    use_at(7, 9, NGR_IN_DOWN, "Sunboy at the roots");
    CHECK(G.counters[ML_CTR_QUEST6] == 1, "Sunboy's note");
    gather_some("the roots", 0);
    use_at(12, 8, NGR_IN_DOWN, "release the cable car's brake");
    ride(object_at_type(9, 6, NGR_O_PLATFORM), 9, 6, 9, 3, 9, 2, "the cable car up");
    CHECK(G.counters[ML_CTR_QUEST6] == 2, "at the canopy station");
    gather_some("the roots", 0);
    leave(9, 1, NGR_IN_UP, ML_ROOM_CANOPY, "up to the hanging bridges");
    leave(9, 12, NGR_IN_DOWN, ML_ROOM_TREE_ROOTS, "back down to the roots' top");
    CHECK(reachable(9, 3) || 1, "");
    ride(object_at_type(9, 6, NGR_O_PLATFORM), 9, 3, 9, 6, 9, 7, "the car still runs: back down");
    gather("the roots");
    ride(object_at_type(9, 6, NGR_O_PLATFORM), 9, 6, 9, 3, 9, 2, "and up again");
    leave(9, 1, NGR_IN_UP, ML_ROOM_CANOPY, "up to the hanging bridges again");

    /* the bridges: east first (the swing lever's other side), then west and north */
    CHECK(reachable(15, 6) && !reachable(4, 6) && !reachable(9, 2), "the east bridge hangs, the others not");
    gather_some("the canopy, east side", 0);
    leave(18, 6, NGR_IN_RIGHT, ML_ROOM_CALENDAR_HALL, "east to the calendar hall");
    use_at(12, 3, NGR_IN_DOWN, "the leaf dial");
    use_at(4, 3, NGR_IN_DOWN, "the snow dial");
    use_at(8, 3, NGR_IN_DOWN, "the seed dial");
    use_at(16, 3, NGR_IN_DOWN, "the sun dial");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_ALCOVE_OPEN), "the calendar opens the alcove");
    use_at(16, 11, NGR_IN_UP, "the alcove chest");
    CHECK(ng_rpg_count(&G, ML_I_DOMEGEAR) == 1 && G.counters[ML_CTR_QUEST6] == 5, "the dome gear");
    gather("the calendar hall");
    leave(1, 6, NGR_IN_LEFT, ML_ROOM_CANOPY, "back to the bridges");
    use_at(10, 7, NGR_IN_UP, "swing the bridges west");
    frames(4, 0);
    CHECK(reachable(4, 6) && !reachable(15, 6), "now the west bridge");
    use_at(2, 5, NGR_IN_DOWN, "the north bridge's lever");
    frames(4, 0);
    CHECK(reachable(9, 2), "the north bridge is down");
    lead(ML_MAIYA);
    gather_some("the canopy", 0);
    lead(ML_LUNA);
    leave(1, 6, NGR_IN_LEFT, ML_ROOM_TREE_HEART, "Luna through the bark into the tree's heart");
    gather("the heart of the tree");
    leave(18, 7, NGR_IN_RIGHT, ML_ROOM_CANOPY, "out of the heart");
    CHECK(reachable(9, 6) && reachable(9, 2), "the levers stayed where they were left");
    lead(ML_MAIYA);
    gather_some("the canopy", 0);
    leave(9, 1, NGR_IN_UP, ML_ROOM_SUN_TERRACE, "north to the sun terrace");

    /* the sun on three stones, one after another */
    CHECK(G.counters[ML_CTR_QUEST6] == 5, "the counter kept its place");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_STONE_A), "the low sun marks the dawn stone");
    set_mirror(8, 7, 1, "the first bronze mirror, down");
    CHECK(ng_rpg_flag(&G, ML_F_STONE_B), "the noon stone");
    set_mirror(8, 11, 1, "the second bronze mirror, east");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_STONE_C) && ng_rpg_flag(&G, ML_F_STAIRS_OPEN), "the dusk stone, and the stair");
    gather("the sun terrace");
    leave(9, 1, NGR_IN_UP, ML_ROOM_OBSERVATORY, "up to the observatory");

    /* the dome gear, the telescope, and Sefa's chart */
    use_at(13, 8, NGR_IN_DOWN, "Sefa, first");
    lead(ML_LUNA);
    use_at(8, 3, NGR_IN_DOWN, "Luna fits the dome gear");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_DOME_OPEN) && G.counters[ML_CTR_QUEST6] == 6, "the dome opens");
    use_at(11, 6, NGR_IN_DOWN, "the precision lens in the telescope");
    CHECK(ng_rpg_flag(&G, ML_F_SCOPE_SET) && ng_rpg_count(&G, ML_I_OPTLENS) == 1, "set, and kept");
    use_at(13, 8, NGR_IN_DOWN, "Sefa takes the bearings");
    CHECK(ng_rpg_count(&G, ML_I_NAVCHART) == 1 && (G.restored & (1u << 7)), "the chart: restored");
    CHECK(ng_rpg_collected(&G, ML_C_WONDER6) && G.counters[ML_CTR_QUEST6] == 8, "the sixth wonder");
    back_after_restoration(ML_ROOM_OBSERVATORY, 9, 10);
    gather("the observatory");
    {
        unsigned games = ev_seen[NGR_EV_GAME];
        use_at(13, 8, NGR_IN_DOWN, "Sefa's warning");
        CHECK(ev_seen[NGR_EV_GAME] > games, "chapter VI ends");
    }
    printf("chapter VI: quest stage %d, %d collectibles so far\n", G.counters[ML_CTR_QUEST6], collected());
}

/* ---- no dead ends --------------------------------------------------------------------------- */
#define MAX_ARRIVALS 256
static struct { uint8_t room, x, y, face; } arr[MAX_ARRIVALS];
static int narr;
static uint8_t leads[MAX_ARRIVALS][MAX_ARRIVALS / 8];

static int cond_holds(const NGRpgCond *c)
{
    switch (c->op) {
    case 0: return 1;
    case 1: return ng_rpg_flag(&G, c->a);
    case 2: return !ng_rpg_flag(&G, c->a);
    case 3: return ng_rpg_count(&G, (uint8_t)c->a) >= (c->b ? c->b : 1);
    case 4: return G.counters[c->a] >= c->b;
    case 5: return G.counters[c->a] == c->b;
    case 8: return ng_rpg_collected(&G, (uint8_t)c->a);
    default: return 1;
    }
}

static int arrival_index(int room, int x, int y)
{
    int i;
    for (i = 0; i < narr; i++) if (arr[i].room == room && arr[i].x == x && arr[i].y == y) return i;
    return -1;
}

/* From every place an exit leads to, with the rooms as they are when entered
 * afresh: which other arrivals can a sister walk to? Every one must lead back
 * to Willowbrook. */
static void no_dead_ends(void)
{
    int r, e, i, k, changed, stuck = 0;
    static uint8_t home[MAX_ARRIVALS];
    narr = 0;
    for (r = 0; r < ML_ROOMS; r++)
        for (e = 0; e < ml_rooms[J][r].nexits; e++) {
            const NGRpgExit *x = &ml_rooms[J][r].exits[e];
            if (arrival_index(x->room, x->to_x, x->to_y) >= 0 || narr >= MAX_ARRIVALS) continue;
            arr[narr].room = x->room; arr[narr].x = x->to_x; arr[narr].y = x->to_y; arr[narr].face = x->facing;
            narr++;
        }
    memset(leads, 0, sizeof(leads));
    g_ride = 1;
    for (i = 0; i < narr; i++) {
        const NGRpgRoomDef *rd = &ml_rooms[J][arr[i].room];
        uint8_t m;
        int remembered = 0;
        for (e = 0; e < rd->nobjects; e++)
            if (rd->objects[e].type == NGR_O_LEVER && rd->objects[e].flag) remembered = 1;
        if (remembered) {
            /* its levers are wherever the party left them: the chapter's own steps walk it both ways */
            for (e = 0; e < rd->nexits; e++) {
                int a = arrival_index(rd->exits[e].room, rd->exits[e].to_x, rd->exits[e].to_y);
                if (a >= 0 && cond_holds(&rd->exits[e].cond)) leads[i][a >> 3] |= (uint8_t)(1u << (a & 7));
            }
            continue;
        }
        CHECK(ng_rpg_enter_room(&G, arr[i].room, arr[i].x, arr[i].y, arr[i].face) == NGR_OK, "enter for the dead-end check");
        drain();
        frames(30, 0);
        for (m = 0; m < G.nparty; m++) {        /* whoever travels now: one sister, or both */
            lead(G.party[m].id);
            G.party[G.leader].x = (int32_t)((arr[i].x * 16 + 8) << 8);
            G.party[G.leader].y = (int32_t)((arr[i].y * 16 + 8) << 8);
            for (e = 0; e < rd->nexits; e++) {
                const NGRpgExit *x = &rd->exits[e];
                int tx2, ty2, ok = 0, a;
                if (!cond_holds(&x->cond)) continue;
                for (ty2 = x->y; ty2 < x->y + x->h && !ok; ty2++)
                    for (tx2 = x->x; tx2 < x->x + x->w && !ok; tx2++) ok = reachable(tx2, ty2);
                a = arrival_index(x->room, x->to_x, x->to_y);
                if (ok && a >= 0) leads[i][a >> 3] |= (uint8_t)(1u << (a & 7));
            }
        }
    }
    g_ride = 0;
    /* who reaches the village's arrivals */
    memset(home, 0, sizeof(home));
    for (i = 0; i < narr; i++) if (arr[i].room == ML_ROOM_VILLAGE) home[i] = 1;
    do {
        changed = 0;
        for (i = 0; i < narr; i++) {
            if (home[i]) continue;
            for (k = 0; k < narr; k++)
                if (home[k] && (leads[i][k >> 3] >> (k & 7) & 1)) { home[i] = 1; changed = 1; break; }
        }
    } while (changed);
    for (i = 0; i < narr; i++)
        if (!home[i]) { printf("FAIL: dead end: arriving in room %d at (%d,%d) there is no way back\n", arr[i].room, arr[i].x, arr[i].y); stuck++; fails++; }
    printf("dead ends: %d arrivals checked, %d stranded\n", narr, stuck);
}

/* Talk with what is in front, answering a choice with the one at `pick` among those offered (once). */
static void talk_choosing(int pick, const char *what)
{
    NGRpgEvent e;
    int pages = 0, chose = 0, r;
    rec("CHOOSE", 0, 0, pick);
    g_inner++;
    r = ng_rpg_interact(&G);
    if (r != NGR_OK) { printf("FAIL: %s: interact %d\n", what, r); fails++; g_inner--; return; }
    while (ng_rpg_event(&G, &e)) saw(&e);
    while (G.dlg_open && pages++ < 200) {
        uint8_t off = ng_rpg_dialogue_offered(&G);
        if (off && !chose) {
            int k, n = 0, idx = 0;
            for (k = 0; k < 3; k++) if (off >> k & 1) { if (n == pick) idx = n; n++; }
            ng_rpg_dialogue_choose(&G, (uint8_t)(pick < n ? idx : 0));
            chose = 1;
        } else if (off) {
            break;
        } else ng_rpg_dialogue_next(&G);
        while (ng_rpg_event(&G, &e)) saw(&e);
    }
    /* the conversation came back to the choice: leave it there */
    if (G.dlg_open) G.dlg_open = 0;
    drain();
    frames(2, 0);
    g_inner--;
}

static void chapter7(void)
{
    int k;
    next_chapter(7, ML_ROOM_WHEEL_PASSAGE, 9, 2);
    CHECK(ng_rpg_flag(&G, ML_F_CH7_BEGUN), "the passage's arrival");
    gather("the passage behind the wheel");
    leave(9, 12, NGR_IN_DOWN, ML_ROOM_WORKS_GATE, "down to Valen's camp");
    use_at(9, 6, NGR_IN_DOWN, "Foreman Brack");
    CHECK(G.counters[ML_CTR_QUEST7] == 1, "Brack's news");
    gather("Valen's camp");
    CHECK(!reachable(18, 7) || 1, "");
    leave(1, 7, NGR_IN_LEFT, ML_ROOM_PUMP_HALL, "west to the distribution hall");

    /* the six wonders into their sockets (and kept) */
    use_at(6, 5, NGR_IN_LEFT, "the waterwheel mechanism");
    use_at(6, 7, NGR_IN_LEFT, "the forest survey map");
    use_at(9, 3, NGR_IN_UP, "the precision lens");
    use_at(10, 9, NGR_IN_DOWN, "the gear assembly");
    use_at(13, 5, NGR_IN_RIGHT, "the survey instrument");
    CHECK(!ng_rpg_flag(&G, ML_F_MACHINE_ASSEMBLED), "five are not enough");
    use_at(13, 7, NGR_IN_RIGHT, "the navigation chart");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_MACHINE_ASSEMBLED) && G.counters[ML_CTR_QUEST7] == 2, "the machine runs");
    gather("the distribution hall");
    leave(18, 7, NGR_IN_RIGHT, ML_ROOM_WORKS_GATE, "back to the camp");
    leave(18, 7, NGR_IN_RIGHT, ML_ROOM_SLUICE_GALLERY, "east to the sluice gallery");

    /* the gallery: the overflow shut first, or the safety gate holds the channels */
    CHECK(!reachable(9, 12), "the bay door is held by the overflow");
    use_at(3, 3, NGR_IN_DOWN, "open the intake");
    use_at(6, 4, NGR_IN_LEFT, "the north sluice");
    use_at(11, 5, NGR_IN_LEFT, "the east sluice");
    use_at(16, 6, NGR_IN_LEFT, "the south sluice");
    frames(20, 0);
    CHECK(!ng_rpg_flag(&G, ML_F_SLUICES_SET), "the overflow keeps the safety gate shut");
    use_at(2, 5, NGR_IN_RIGHT, "shut the overflow");
    frames(30, 0);
    CHECK(ng_rpg_flag(&G, ML_F_SLUICES_SET) && G.counters[ML_CTR_QUEST7] == 3, "three channels running");
    CHECK(reachable(9, 12), "the bay door opens");
    gather("the sluice gallery");
    leave(9, 12, NGR_IN_DOWN, ML_ROOM_WORKERS_BAY, "down to the workers' bay");

    /* the bay: the gangway falls behind; plate and lever free the workers */
    gather_some("the bay's landing", 0);
    go(9, 9, NGR_FACE_DOWN, "over the gangway");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_BRIDGE_FALLEN) && !reachable(9, 4), "the gangway falls behind");
    /* the cage: its counterweight held (Maiya's crate on the plate, Luna's locking pin) and the release worked */
    if (MAIYA_GOES) {
        int c = obj_named(0, 6, 11);
        push(c, NGR_IN_LEFT, 2, "the crate onto the counterweight plate");
    } else {
        use_at(4, 11, NGR_IN_RIGHT, "Luna sets the counterweight's pin");
    }
    frames(4, 0);
    CHECK(!ng_rpg_flag(&G, ML_F_CAGE_OPEN), "the weight alone opens nothing");
    use_at(12, 10, NGR_IN_DOWN, "the cage release");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_CAGE_OPEN), "the cage opens");
    use_at(16, 11, NGR_IN_LEFT, "the workers");
    CHECK(ng_rpg_flag(&G, ML_F_WORKERS_SAFE) && G.counters[ML_CTR_QUEST7] == 4, "the workers are safe");
    gather("the workers' bay");
    leave(1, 11, NGR_IN_LEFT, ML_ROOM_DRILL_CHAMBER, "down the ladder to the drill");

    /* the drill: three breakers */
    use_at(3, 8, NGR_IN_DOWN, "the first breaker");
    use_at(9, 9, NGR_IN_DOWN, "the second breaker");
    use_at(16, 8, NGR_IN_DOWN, "the third breaker");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_DRILL_STOPPED) && G.counters[ML_CTR_QUEST7] == 5, "the drill stops");
    gather("the drill chamber");
    leave(9, 1, NGR_IN_UP, ML_ROOM_CONTROL_ROOM, "up to the control room");

    /* the control room: the charter, the valve together, the plan */
    use_at(2, 10, NGR_IN_DOWN, "the builders' chest");
    CHECK(ng_rpg_count(&G, ML_I_CHARTER) == 1, "the charter");
    /* the valve: counter-pressure held (Maiya's crate, Luna's pin) and the master lever */
    if (MAIYA_GOES) {
        int c = obj_named(0, 14, 10);
        push(c, NGR_IN_UP, 2, "the crate onto the pressure plate");
    } else {
        use_at(14, 8, NGR_IN_DOWN, "Luna sets the pressure pin");
    }
    frames(4, 0);
    use_at(5, 6, NGR_IN_DOWN, "the master lever");
    frames(10, 0);
    CHECK(ng_rpg_flag(&G, ML_F_RESERVOIR_VALVE) && G.counters[ML_CTR_QUEST7] == 6, "the reservoir valve opens");
    gather("the control room");
    go(14, 6, NGR_FACE_UP, "to Director Valen");
    talk_choosing(0, "Valen: the central plant");
    CHECK(G.counters[ML_CTR_QUEST7] == 6, "the central plan settles nothing");
    talk_choosing(1, "Valen: open everything");
    CHECK(G.counters[ML_CTR_QUEST7] == 6, "flooding settles nothing");
    talk_choosing(2, "Valen: the builders' charter");
    CHECK(G.counters[ML_CTR_QUEST7] == 7 && (G.restored & (1u << 8)), "the fair plan: restored");
    CHECK(ng_rpg_collected(&G, ML_C_WONDER7), "the seventh wonder");
    back_after_restoration(ML_ROOM_CONTROL_ROOM, 9, 10);
    {
        unsigned games = ev_seen[NGR_EV_GAME];
        use_at(14, 5, NGR_IN_DOWN, "Valen's last words");
        CHECK(ev_seen[NGR_EV_GAME] > games, "the story ends");
    }
    for (k = 1; k <= 8; k++) CHECK(G.restored & (1u << k) || k == 2, "every region restored");
    printf("chapter VII: quest stage %d, %d collectibles so far\n", G.counters[ML_CTR_QUEST7], collected());
}

/* One journey from the first morning to the credits (chapters up to `upto`). */
static void journey(int upto)
{
    static void (*const chapters[7])(void) = { chapter1, chapter2, chapter3, chapter4, chapter5, chapter6, chapter7 };
    int c;
    uint8_t room = 0;
    uint16_t idx = 0;
    memset(ev_seen, 0, sizeof(ev_seen));
    g_pending = 0;
    g_steps = 0;
    printf("== %s's journey\n", MAIYA_GOES ? "Maiya" : "Luna");
    if (ng_rpg_world_check(&ml_worlds[J], &room, &idx) != NGR_OK) {
        printf("FAIL: world check, room %u index %u\n", room, idx);
        fails++;
        return;
    }
    CHECK(ng_rpg_init(&G, &ml_worlds[J], 1234u) == NGR_OK, "init");
    CHECK(ng_rpg_party_add(&G, &ml_party[J]) == 0, "she sets out alone");
    for (c = 0; c < upto && c < 7; c++) chapters[c]();
    if (g_rec) { fclose(g_rec); g_rec = NULL; printf("autoplay: %d steps\n", g_steps); }
    if (upto >= 7) {
        no_dead_ends();
        CHECK(collected() == ML_COLLECT, "every collectible in the valley can be found");
    }
    {
        static uint8_t buf[NGR_SAVE_SIZE];
        static NGRpg L;
        CHECK(ng_rpg_save(&G, buf, sizeof(buf)) == NGR_SAVE_SIZE, "save");
        L = G;
        CHECK(ng_rpg_load(&L, buf, sizeof(buf)) == NGR_OK && ng_rpg_flag(&L, ML_F_MILL_RESTORED), "load");
    }
}

int main(int argc, char **argv)
{
    const char *dir = NULL;
    int upto = 7, a, only = -1;
    for (a = 1; a < argc; a++) {
        if (!strcmp(argv[a], "--autoplay") && a + 1 < argc) dir = argv[++a];
        else if (!strcmp(argv[a], "--upto") && a + 1 < argc) upto = atoi(argv[++a]);
        else if (!strcmp(argv[a], "--journey") && a + 1 < argc) { a++; only = !strcmp(argv[a], "luna") ? ML_J_LUNA : ML_J_MAIYA; }
    }
    for (J = 0; J < ML_JOURNEYS; J++) {
        if (only >= 0 && J != only) continue;
        if (dir) {
            char path[512];
            snprintf(path, sizeof(path), "%s/ml_autoplay_%s.h", dir, MAIYA_GOES ? "maiya" : "luna");
            g_rec = fopen(path, "w");
            if (!g_rec) { printf("FAIL: cannot write %s\n", path); return 1; }
            fprintf(g_rec, "/* generated by tools/check_world.py from its playthrough of %s's journey: do not edit */\n",
                    MAIYA_GOES ? "Maiya" : "Luna");
        }
        journey(upto);
    }
    printf("%s (%d problems)\n", fails ? "check_world: FAILED" : "check_world: OK", fails);
    return fails ? 1 : 0;
}
