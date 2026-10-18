#ifndef NG_RPG_H
#define NG_RPG_H
/*
 * ng_rpg: a deterministic top-down adventure / RPG core (v1.7.7 genre
 * engine). Selected with GAME_GENRE_ENGINE = rpg in a game's game.mk.
 *
 * What it runs, all from ROM tables the game supplies (NGRpgWorld):
 *   - rooms on a 16x16 tile grid with exits (locked by conditions),
 *     tile collision (walls, water, pits, narrow gaps, climbable vines,
 *     hazards, doorways, one-way ledges) and per-character abilities;
 *   - a party of up to four: one leads, the others follow its trail, wait
 *     where told, or go to a tile to help (a short path search on the tiles
 *     around them), step out of doorways, and are put back beside the
 *     leader only after they have been stuck for a while (an event says so);
 *   - objects: people, signs, chests and pickups, rest spots, locks, hidden
 *     passages opened with a tool, pushable blocks (which fill pits),
 *     pressure plates, levers, gates and bridges driven by signals, gear
 *     sockets and trains, water channels and valves, mirrors and lenses
 *     carrying light to receivers, bells and lever sequences, balances,
 *     moving platforms, timed hazards and light creatures that can be
 *     calmed (never killed);
 *   - signals: each room has 32; plates, levers, receivers, wet outlets,
 *     turning gears, balances and finished sequences drive them, gates and
 *     bridges read them. Logic runs before the game draws, never waiting on
 *     an animation;
 *   - world state: 512 flags, 32 counters (quest stages), an inventory
 *     with stacks and key items that can't be dropped or lost, crafting
 *     recipes, 256 collectibles, visited rooms, restored regions and a
 *     checkpoint; dialogue graphs with conditions, choices and actions.
 *
 * Nothing here is magic: abilities are practical skills, items are tools
 * and supplies, progress comes from flags and equipment. Games define what
 * their flags, items and dialogue mean.
 *
 * The core never touches video RAM or sound: the game draws the room,
 * party and objects from the context and turns events into sounds and
 * effects. Everything lives in the caller's NGRpg (3256 bytes): no heap,
 * no floating point, no hidden globals, no recursion. Positions are Q8
 * pixels (pixels * 256) of a character's feet, from the room's top-left.
 * One ng_rpg_update() is one frame.
 */
#include <stdint.h>
#include "ng_defs.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ---- capacities ------------------------------------------------------ */
#define NGR_TILE_PX        16u
#define NGR_ROOM_MAX_W     40u
#define NGR_ROOM_MAX_H     32u
#define NGR_ROOM_TILES     1280u      /* NGR_ROOM_MAX_W * NGR_ROOM_MAX_H */
#define NGR_ROOM_BITS      160u       /* bytes per room-sized bit set */
#define NGR_MAX_OBJECTS    32u
#define NGR_MAX_PARTY      4u
#define NGR_INV_SLOTS      40u        /* the last NGR_INV_KEYS hold only key items, tools and records */
#define NGR_INV_KEYS       8u
#define NGR_FLAGS          512u
#define NGR_COUNTERS       32u
#define NGR_COLLECT        256u
#define NGR_MAX_ROOMS      64u
#define NGR_TRAIL          64u        /* power of two */
#define NGR_NAV_W          24u        /* path search window, tiles */
#define NGR_NAV_H          16u
#define NGR_PATH_MAX       24u
#define NGR_EVENT_QUEUE    32u        /* power of two */
#define NGR_NONE           0xFFu
#define NGR_END            0xFFFFu    /* dialogue: no next node */

/* ---- results --------------------------------------------------------- */
enum {
    NGR_OK = 0,
    NGR_ERR_ARG = -1,
    NGR_ERR_STATE = -2,      /* dialogue open, party down, nothing there */
    NGR_ERR_NOPE = -3,       /* not possible now: locked, missing item, full */
    NGR_ERR_KEY = -4,        /* a key item can't be dropped */
    NGR_ERR_SAVE = -5,
    NGR_ERR_WORLD = -6
};

/* ---- tiles (NGRpgRoomDef.tiles) --------------------------------------- */
#define NGR_T_SOLID   0x01u
#define NGR_T_WATER   0x02u   /* blocks unless NGR_AB_SWIM */
#define NGR_T_PIT     0x04u   /* blocks; a pushed block fills it */
#define NGR_T_NARROW  0x08u   /* only NGR_AB_NARROW passes */
#define NGR_T_CLIMB   0x10u   /* only NGR_AB_CLIMB passes (vines, rock faces) */
#define NGR_T_HAZARD  0x20u   /* hurts (thorns, steam): passable */
#define NGR_T_DOOR    0x40u   /* a doorway: followers never wait on it */
#define NGR_T_LEDGE   0x80u   /* one way: only entered moving down */

/* ---- abilities (NGRpgMemberDef.abilities) ------------------------------- */
#define NGR_AB_NARROW  0x01u
#define NGR_AB_CLIMB   0x02u
#define NGR_AB_SWIM    0x04u
#define NGR_AB_PUSH    0x08u  /* moves heavy blocks */
#define NGR_AB_FINE    0x10u  /* works fine mechanisms (NGRpgObjectDef p[6] & 1) */

/* ---- conditions and actions -------------------------------------------- */
enum {
    NGR_C_ALWAYS = 0,
    NGR_C_FLAG,              /* flag a is set */
    NGR_C_NOT_FLAG,          /* flag a is clear */
    NGR_C_ITEM,              /* at least b of item a */
    NGR_C_COUNTER_GE,        /* counter a >= b */
    NGR_C_COUNTER_EQ,        /* counter a == b */
    NGR_C_LEADER,            /* the leader is party member id a */
    NGR_C_SIGNAL,            /* room signal bit a is on */
    NGR_C_COLLECTED          /* collectible a was found */
};
typedef struct { uint8_t op, pad; uint16_t a, b; } NGRpgCond;

enum {
    NGR_A_END = 0,
    NGR_A_SET_FLAG,          /* a */
    NGR_A_CLEAR_FLAG,        /* a */
    NGR_A_GIVE,              /* item a, count b */
    NGR_A_TAKE,              /* item a, count b */
    NGR_A_SET_COUNTER,       /* counter a = b (quest stage) */
    NGR_A_ADD_COUNTER,       /* counter a += b */
    NGR_A_DIALOGUE,          /* start dialogue node a */
    NGR_A_CHECKPOINT,        /* checkpoint at the leader */
    NGR_A_RESTORE,           /* region a restored (bit a of 16) */
    NGR_A_HEAL,              /* the party, b hit points (0: full) */
    NGR_A_TELEPORT,          /* room a, tile b = y * 256 + x */
    NGR_A_COLLECT,           /* collectible a */
    NGR_A_EVENT,             /* NGR_EV_GAME with a, b for the game */
    NGR_A_OPEN_OBJECT        /* object a in this room: open / reveal / switch on */
};
typedef struct { uint8_t op, pad; uint16_t a, b; } NGRpgAction;   /* lists end with NGR_A_END */

/* ---- world data (ROM) ----------------------------------------------- */
#define NGR_I_KEY        0x01u   /* never dropped, never lost */
#define NGR_I_CONSUME    0x02u   /* used up: param = hit points restored */
#define NGR_I_TOOL       0x04u   /* used on objects (locks, hidden passages, sockets) */
#define NGR_I_COLLECT    0x08u
#define NGR_I_MATERIAL   0x10u
#define NGR_I_RECORD     0x20u   /* maps, diagrams, notes */
typedef struct {
    uint8_t flags, max_stack, param, weight;
} NGRpgItemDef;

typedef struct {
    uint8_t in_item[3], in_count[3];   /* in_count 0: unused */
    uint8_t out_item, out_count;
} NGRpgRecipe;

typedef struct {
    uint8_t speaker, nchoices;
    uint16_t text;                     /* the game's string id */
    uint16_t next;                     /* NGR_END closes the conversation */
    uint16_t alt;                      /* shown instead when cond fails */
    NGRpgCond cond;
    const NGRpgAction *actions;        /* run when the node is shown (NULL: none) */
    uint16_t choice_text[3], choice_next[3];
    NGRpgCond choice_cond[3];          /* a choice is offered only when its condition holds */
} NGRpgDialogueNode;

typedef struct {
    uint8_t x, y, w, h;                /* tiles */
    uint8_t room, to_x, to_y, facing;  /* where it leads */
    NGRpgCond cond;                    /* to pass */
    uint16_t locked_text;              /* the game's string id */
} NGRpgExit;

enum { NGR_TRIG_ENTER = 0, NGR_TRIG_STEP, NGR_TRIG_INTERACT, NGR_TRIG_ROOM, NGR_TRIG_WATCH };
typedef struct {
    uint8_t x, y, w, h;                /* tiles (NGR_TRIG_ROOM and _WATCH: unused) */
    uint8_t when, pad;
    uint16_t once_flag;                /* 0: every time; else fires once, then sets it */
    NGRpgCond cond;
    const NGRpgAction *actions;
} NGRpgTrigger;

/* Objects. p[] meaning by type is listed with each one. */
enum {
    NGR_O_NONE = 0,
    NGR_O_NPC,        /* text = dialogue; p0 0 stand / 1 wander / 2 patrol; p1 speed (px*16); p2,p3 patrol dx, dy (int8 tiles);
                         p4 1: there only while `flag` is set, 2: only while it is clear (checked on entering the room) */
    NGR_O_SIGN,       /* text = dialogue */
    NGR_O_PICKUP,     /* walked over: p0 item (NGR_NONE: none), p1 count, p2 collectible (NGR_NONE); flag = taken */
    NGR_O_CHEST,      /* as PICKUP, opened with A */
    NGR_O_REST,       /* heal and checkpoint */
    NGR_O_LOCK,       /* solid until opened with item p0 (p1 1: the key is used up); flag = opened; on a channel it holds the water back until opened */
    NGR_O_SECRET,     /* hidden passage: p0 tool item (0: just A), p1 1 solid until found; flag = found */
    NGR_O_BLOCK,      /* pushable; p0 1 heavy (needs NGR_AB_PUSH); fills a pit */
    NGR_O_PLATE,      /* pressed by weight >= p0 (0: anything); p1 1 stays down; drives out_mask */
    NGR_O_LEVER,      /* A toggles; drives out_mask (p6 1: needs NGR_AB_FINE, as for any object used with A); flag = its position, kept */
    NGR_O_GATE,       /* solid while closed; opens when in_mask signals are all on (p0 1: when off); flag = open for good */
    NGR_O_BRIDGE,     /* floor over water / pit while in_mask is on (p0 1: while off); flag = for good */
    NGR_O_GEAR,       /* socket: p0 1 driver, 2 target (both fixed gears); p1 target turn 1 / 2; p2-p4 meshing sockets; p5 gear item (A places it) */
    NGR_O_WATER_SRC,  /* water starts here (on a channel); in_mask: only while those signals are on */
    NGR_O_VALVE,      /* on a channel: A opens / closes; open at start when p0 1 */
    NGR_O_OUTLET,     /* drives out_mask while its tile is wet */
    NGR_O_EMITTER,    /* light: p0 direction (0 right, 1 down, 2 left, 3 up); in_mask: only while on */
    NGR_O_MIRROR,     /* A turns it: state 0 '/', 1 '\' (p0: the state it starts in) */
    NGR_O_LENS,       /* focuses light passing through */
    NGR_O_RECEIVER,   /* drives out_mask while lit (p0 1: focused light only) */
    NGR_O_BELL,       /* A rings tone p0; step of any sequence listing it */
    NGR_O_SEQUENCE,   /* p0-p5 object indices in order (NGR_NONE ends); drives out_mask when done; flag = done for good */
    NGR_O_PAN,        /* a balance pan: weight on its tiles */
    NGR_O_BALANCE,    /* p0 left pan, p1 right pan, p2 threshold: drives out_mask while left - right >= p2 */
    NGR_O_PLATFORM,   /* moves by p0, p1 (int8 tiles) over p2 * 8 frames and back; floor where it is; in_mask: only while on */
    NGR_O_HAZARD,     /* hurts p2 every period: on for p1 * 4 of every p0 * 4 frames */
    NGR_O_CREATURE,   /* p0 0 guard / 1 wander / 2 patrol / 3 chase; p1 hits to calm; p2 damage; p3 speed (px*16); p4 chase radius (tiles); flag = calmed */
    NGR_O_SIGNAL_FLAG /* drives out_mask while world flag `flag` is set */
};
typedef struct {
    uint8_t type, x, y, w, h;          /* tiles; w, h 0 count as 1 */
    uint8_t p[7];
    uint16_t flag;                     /* world flag, meaning by type (0: none) */
    uint16_t text;                     /* dialogue node */
    uint32_t in_mask, out_mask;        /* signals read / driven */
    const NGRpgAction *actions;        /* run when it is used, opened, found or done */
} NGRpgObjectDef;

typedef struct {
    uint8_t w, h, region, flags;       /* flags: the game's own */
    const uint8_t *tiles;              /* w * h NGR_T_* */
    const uint8_t *channels;           /* NULL, or w * h: non-zero where water can run */
    const NGRpgExit *exits;
    const NGRpgObjectDef *objects;
    const NGRpgTrigger *triggers;
    uint8_t nexits, nobjects, ntriggers, pad;
    uint16_t music, art;               /* the game's ids, passed through */
} NGRpgRoomDef;

#define NGR_WF_AUTO_EXIT   0x01u  /* take exits at once (else NGR_EV_EXIT and ng_rpg_take_exit) */
#define NGR_WF_NO_TELEPORT 0x02u  /* never put a stuck follower back by the leader */
typedef struct {
    const NGRpgRoomDef *rooms;
    const NGRpgItemDef *items;
    const NGRpgRecipe *recipes;
    const NGRpgDialogueNode *dialogue;
    uint8_t nrooms, nitems, nrecipes, flags;
    uint16_t ndialogue;
} NGRpgWorld;

typedef struct {
    uint8_t id;                        /* the game's character id */
    uint8_t abilities;                 /* NGR_AB_* */
    uint8_t speed;                     /* pixels * 16 per frame (24 = 1.5 px) */
    uint8_t max_hp, weight;
} NGRpgMemberDef;

/* ---- runtime ------------------------------------------------------------ */
enum { NGR_MODE_LEAD = 0, NGR_MODE_FOLLOW, NGR_MODE_WAIT, NGR_MODE_ASSIST };
enum { NGR_FACE_RIGHT = 0, NGR_FACE_DOWN, NGR_FACE_LEFT, NGR_FACE_UP };

typedef struct {
    int32_t x, y;                      /* Q8 pixels: feet */
    uint8_t id, abilities, speed, max_hp, hp, weight;
    uint8_t facing, mode, moving, invuln;
    uint8_t push;                      /* frames spent pushing into a block */
    uint8_t stuck;                     /* frames without getting closer */
    uint8_t tx, ty;                    /* assist / path target tile */
    uint8_t path_len, path_pos;
    uint8_t wx, wy;                    /* the tile the current path step leads to */
    uint8_t blocked;                   /* frames a path step made no progress */
    uint8_t path[NGR_PATH_MAX];        /* directions */
    uint16_t trail_lag;                /* which trail point it walks to */
} NGRpgMember;

typedef struct {
    uint8_t alive;                     /* 0: gone (taken, opened, calmed) */
    uint8_t state;                     /* on / open / turned / gear in socket / progress */
    uint8_t x, y;                      /* tile (blocks move) */
    uint8_t aux, timer, facing, hits;
    int32_t px, py;                    /* Q8, for things that move */
} NGRpgObject;

enum {
    NGR_EV_NONE = 0,
    NGR_EV_ROOM,          /* a = room */
    NGR_EV_EXIT,          /* a = room, b = exit index: waiting for ng_rpg_take_exit */
    NGR_EV_LOCKED,        /* a = exit index, value = locked text */
    NGR_EV_ITEM,          /* a = item, b = count */
    NGR_EV_FULL,          /* a = item: no room for it */
    NGR_EV_COLLECT,       /* a = collectible */
    NGR_EV_DIALOGUE,      /* value = node shown */
    NGR_EV_DIALOGUE_END,
    NGR_EV_SIGNAL,        /* a = bit, b = on */
    NGR_EV_OPEN,          /* a = object (gate, lock, bridge), b = open */
    NGR_EV_FOUND,         /* a = object: hidden passage found */
    NGR_EV_PUSH,          /* a = block object */
    NGR_EV_FILL,          /* a = block object: it filled a pit */
    NGR_EV_PLATE,         /* a = object, b = down */
    NGR_EV_LEVER,         /* a = object, b = state */
    NGR_EV_TONE,          /* a = object, b = tone */
    NGR_EV_SEQ_STEP,      /* a = sequence object, b = steps done */
    NGR_EV_SEQ_WRONG,     /* a = sequence object */
    NGR_EV_SEQ_DONE,      /* a = sequence object */
    NGR_EV_GEARS,         /* a = turning sockets, b = 1 jammed */
    NGR_EV_WATER,         /* a = outlet object, b = wet */
    NGR_EV_LIGHT,         /* a = receiver object, b = lit */
    NGR_EV_HURT,          /* a = member, b = hit points left */
    NGR_EV_DOWN,          /* a = member: call ng_rpg_respawn */
    NGR_EV_HIT,           /* a = creature object, b = hits left */
    NGR_EV_CALMED,        /* a = creature object */
    NGR_EV_SWITCH,        /* a = new leader member index */
    NGR_EV_STUCK,         /* a = member: lost its way */
    NGR_EV_TELEPORT,      /* a = member: put back beside the leader */
    NGR_EV_RESTORE,       /* a = region */
    NGR_EV_CHECKPOINT,
    NGR_EV_REST,
    NGR_EV_FLAG,          /* value = flag set */
    NGR_EV_COUNTER,       /* a = counter, b = value */
    NGR_EV_CRAFT,         /* a = recipe */
    NGR_EV_USE,           /* a = item, b = object (NGR_NONE: none) */
    NGR_EV_GAME           /* a, value: NGR_A_EVENT from the data */
};
typedef struct { uint8_t type, a, b, pad; uint16_t value; } NGRpgEvent;

typedef struct {
    const NGRpgWorld *world;
    const NGRpgRoomDef *def;
    uint8_t room;
    uint8_t nparty, leader;
    NGRpgMember party[NGR_MAX_PARTY];
    NGRpgObject obj[NGR_MAX_OBJECTS];
    uint8_t solid[NGR_ROOM_BITS];      /* tiles made solid by objects */
    uint8_t floor[NGR_ROOM_BITS];      /* tiles made floor (bridges, filled pits, platforms) */
    uint8_t wet[NGR_ROOM_BITS];        /* water reached these channel tiles */
    uint8_t lit[NGR_ROOM_BITS];        /* light crossed these tiles */
    uint32_t signals;
    uint32_t derived;                  /* last frame's signals from water, light and gears */
    uint8_t flags[NGR_FLAGS / 8u];
    uint8_t counters[NGR_COUNTERS];
    uint8_t inv_item[NGR_INV_SLOTS], inv_count[NGR_INV_SLOTS];
    uint8_t collected[NGR_COLLECT / 8u];
    uint8_t visited[NGR_MAX_ROOMS / 8u];
    uint16_t restored;                 /* regions */
    uint8_t ck_room, ck_x, ck_y;       /* checkpoint tile */
    uint8_t in_dirs;                   /* NGR_IN_* held this frame */
    uint8_t dlg_open, down;
    uint16_t dlg_node;
    uint8_t pending_exit;              /* exit index + 1 */
    uint8_t in_exit, in_trig[4];       /* edge detection */
    uint8_t gear_turn;                 /* sockets turning */
    uint8_t gear_jam;
    int16_t trail_x[NGR_TRAIL], trail_y[NGR_TRAIL];
    uint8_t trail_head, trail_n;
    uint8_t nav_dir[NGR_NAV_W * NGR_NAV_H];
    uint16_t nav_q[NGR_NAV_W * NGR_NAV_H];
    uint32_t tick, rng;
    uint8_t ev_head, ev_count;
    uint16_t ev_dropped;
    NGRpgEvent ev[NGR_EVENT_QUEUE];
} NGRpg;

/* Input bits for ng_rpg_input (the BIOS joystick byte's direction bits). */
#define NGR_IN_UP     0x01u
#define NGR_IN_DOWN   0x02u
#define NGR_IN_LEFT   0x04u
#define NGR_IN_RIGHT  0x08u

#define NGR_SAVE_SIZE  320u

/* ---- calls --------------------------------------------------------------- */
/* Check the world tables (rooms, exits, objects, items, dialogue links).
 * Returns NGR_OK or NGR_ERR_WORLD with *room / *index of the first fault. */
int      NEOGEO_USER ng_rpg_world_check(const NGRpgWorld *w, uint8_t *room, uint16_t *index);
int      NEOGEO_USER ng_rpg_init(NGRpg *g, const NGRpgWorld *w, uint32_t seed);
/* Add a party member (the first leads; one added while the party is in a
 * room starts at the leader's side). Returns its index or an error. */
int      NEOGEO_USER ng_rpg_party_add(NGRpg *g, const NGRpgMemberDef *def);
/* Take party member `member` (an index) out; the others move up. When the
 * leader goes, the first one left leads. Not while a conversation is open,
 * never the last one. NGR_OK or an error. */
int      NEOGEO_USER ng_rpg_party_remove(NGRpg *g, uint8_t member);
/* Put the party in a room at a tile (a new game, a teleport, a load). */
int      NEOGEO_USER ng_rpg_enter_room(NGRpg *g, uint8_t room, uint8_t tx, uint8_t ty, uint8_t facing);
/* After NGR_EV_EXIT (and the game's fade): go through. */
int      NEOGEO_USER ng_rpg_take_exit(NGRpg *g);
/* Put the room's puzzles back as they were on entry; permanent results stay. */
void     NEOGEO_USER ng_rpg_reset_room(NGRpg *g);
void     NEOGEO_USER ng_rpg_input(NGRpg *g, uint8_t dirs);
void     NEOGEO_USER ng_rpg_update(NGRpg *g);
/* A: talk, read, open, take, pull, turn, ring, place a gear. */
int      NEOGEO_USER ng_rpg_interact(NGRpg *g);
/* C: use an item -- a tool on what the leader faces, or a supply on the leader. */
int      NEOGEO_USER ng_rpg_use_item(NGRpg *g, uint8_t item);
/* A short strike in front of the leader: calms creatures. */
int      NEOGEO_USER ng_rpg_strike(NGRpg *g);
/* Change the leader (NGR_NONE: the next one). The old leader follows. */
int      NEOGEO_USER ng_rpg_switch(NGRpg *g, uint8_t member);
/* NGR_MODE_FOLLOW, _WAIT (stays), _ASSIST (walks to tile tx, ty and stays). */
int      NEOGEO_USER ng_rpg_order(NGRpg *g, uint8_t member, uint8_t mode, uint8_t tx, uint8_t ty);
/* After NGR_EV_DOWN: the party back at the checkpoint, healed. */
void     NEOGEO_USER ng_rpg_respawn(NGRpg *g);

/* Dialogue. */
int      NEOGEO_USER ng_rpg_dialogue_start(NGRpg *g, uint16_t node);
/* Go on from a node without choices. */
int      NEOGEO_USER ng_rpg_dialogue_next(NGRpg *g);
/* Pick choice i of the current node (only offered choices count). */
int      NEOGEO_USER ng_rpg_dialogue_choose(NGRpg *g, uint8_t i);
/* The choices on offer now: bit i set if choice i may be picked. */
uint8_t  NEOGEO_USER ng_rpg_dialogue_offered(const NGRpg *g);

/* World state. */
uint8_t  NEOGEO_USER ng_rpg_flag(const NGRpg *g, uint16_t flag);
void     NEOGEO_USER ng_rpg_set_flag(NGRpg *g, uint16_t flag, uint8_t on);
uint8_t  NEOGEO_USER ng_rpg_check(const NGRpg *g, const NGRpgCond *c);
void     NEOGEO_USER ng_rpg_run(NGRpg *g, const NGRpgAction *a);
uint8_t  NEOGEO_USER ng_rpg_count(const NGRpg *g, uint8_t item);
/* Give items: returns how many were taken in (key items always fit). */
uint8_t  NEOGEO_USER ng_rpg_give(NGRpg *g, uint8_t item, uint8_t count);
uint8_t  NEOGEO_USER ng_rpg_take(NGRpg *g, uint8_t item, uint8_t count);
int      NEOGEO_USER ng_rpg_drop(NGRpg *g, uint8_t item, uint8_t count);
/* All inputs present: take them and give the output, all or nothing. */
int      NEOGEO_USER ng_rpg_craft(NGRpg *g, uint8_t recipe);
uint8_t  NEOGEO_USER ng_rpg_collected(const NGRpg *g, uint8_t id);

/* Tiles: the flags at a tile with objects applied, and whether a member
 * (NGR_NONE: a pushed block) may stand there. */
uint8_t  NEOGEO_USER ng_rpg_tile(const NGRpg *g, uint8_t tx, uint8_t ty);
uint8_t  NEOGEO_USER ng_rpg_walkable(const NGRpg *g, uint8_t member, uint8_t tx, uint8_t ty);
/* The object index at a tile (NGR_NONE). */
uint8_t  NEOGEO_USER ng_rpg_object_at(const NGRpg *g, uint8_t tx, uint8_t ty);

uint8_t  NEOGEO_USER ng_rpg_event(NGRpg *g, NGRpgEvent *out);
uint32_t NEOGEO_USER ng_rpg_hash(const NGRpg *g);
/* Save the world state, party and checkpoint (NGR_SAVE_SIZE bytes, big-
 * endian, versioned, checksummed). A loaded game starts at the saved room
 * with its puzzles at their entry state; permanent results are flags. */
uint16_t NEOGEO_USER ng_rpg_save(const NGRpg *g, uint8_t *buf, uint16_t size);
int      NEOGEO_USER ng_rpg_load(NGRpg *g, const uint8_t *buf, uint16_t size);

#ifdef __cplusplus
}
#endif
#endif
