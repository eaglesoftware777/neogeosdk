#ifndef NG_PUZZLE_H
#define NG_PUZZLE_H
/*
 * ng_puzzle: a deterministic board-puzzle core (v1.7.7 genre engine).
 * Selected with GAME_GENRE_ENGINE = puzzle in a game's game.mk, which
 * compiles sdk/2d_engine/ng_puzzle.c into the ROM; nothing else is needed.
 *
 * One context plays one level of one of three families:
 *
 *   NGP_MODE_MATCH   swap two neighbouring pieces to line up three or more
 *                    of a colour: runs clear, pieces fall, the board refills
 *                    and cascades chain. Runs of four make line pieces, runs
 *                    of five a prism, crossing runs a bomb. Crates, ice,
 *                    locks and walls are board furniture with objectives.
 *   NGP_MODE_BUBBLE  aim a launcher and fire a bubble into a staggered
 *                    (hexagonal) lattice. It rebounds off the side walls,
 *                    sticks to the nearest free lattice cell, pops a group of
 *                    three or more of its colour, and drops whatever no
 *                    longer hangs from the ceiling or an anchor.
 *   NGP_MODE_LOGIC   picture logic: number clues give, for every row and
 *                    column, the runs of filled cells in order; fill them
 *                    all to complete the picture. A cell filled wrongly is
 *                    a mistake (it is crossed out and costs a heart); a
 *                    finished row or column crosses out its other cells.
 *
 * The core owns the logic only. It never touches video RAM or the sound
 * driver: the game draws the board with the SDK's sprites and FIX layer and
 * plays sounds from the events it drains. Logic and presentation are
 * separate: an action is checked and committed at once, and the work that
 * follows (clearing, falling, cascading, a bubble's flight) advances one
 * phase per ng_puzzle_update(). A game that waits for its animation before
 * the next update gets the same board, score and random draws as one that
 * calls it every frame, or as ng_puzzle_settle() which runs it all at once.
 *
 * Everything lives in the caller's NGPuzzle: no heap, no floating point, no
 * hidden globals, no recursion. Random numbers come from the context's own
 * xorshift generator, so the same level, seed and action list replay the
 * same game (ng_puzzle_hash, ng_puzzle_replay). Coordinates in bubble mode
 * are Q8 pixels (pixels * 256) measured from the field's top-left corner;
 * angles are 1024 steps to the full turn (256 = straight down, 768 = up).
 *
 * Read the board straight from the context (the fields marked read-only);
 * change it only through the calls below.
 */
#include <stdint.h>
#include "ng_defs.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ---- capacities ------------------------------------------------------ */
#define NGP_MAX_W        12u
#define NGP_MAX_H        16u
#define NGP_MAX_CELLS    192u          /* NGP_MAX_W * NGP_MAX_H */
#define NGP_MAX_COLORS   8u
#define NGP_MAX_OBJ      4u
#define NGP_EVENT_QUEUE  32u           /* power of two */
#define NGP_NO_CELL      0xFFu
#define NGP_BITS_BYTES   ((NGP_MAX_CELLS + 7u) / 8u)

/* ---- results --------------------------------------------------------- */
enum {
    NGP_OK = 0,
    NGP_ERR_ARG = -1,        /* bad pointer, size or command */
    NGP_ERR_RANGE = -2,      /* a cell or angle outside the board */
    NGP_ERR_BUSY = -3,       /* still resolving the last action */
    NGP_ERR_ILLEGAL = -4,    /* a swap that makes no match (it is undone) */
    NGP_ERR_STATE = -5,      /* the level is over, or the wrong mode */
    NGP_ERR_LEVEL = -6,      /* level data rejected by ng_puzzle_level_check */
    NGP_ERR_SAVE = -7        /* save data from another level or version */
};

enum { NGP_MODE_MATCH = 0, NGP_MODE_BUBBLE = 1, NGP_MODE_LOGIC = 2 };

/* ng_puzzle_status() */
enum { NGP_STATUS_READY = 0, NGP_STATUS_BUSY = 1, NGP_STATUS_WON = 2, NGP_STATUS_LOST = 3 };

/* Why a level ended (NGPuzzle.end_reason). */
enum {
    NGP_END_NONE = 0, NGP_END_OBJECTIVES, NGP_END_MOVES, NGP_END_TIME,
    NGP_END_LOSS_LINE, NGP_END_NO_MOVES, NGP_END_BOARD_CLEAR, NGP_END_SHOTS,
    NGP_END_MISTAKES          /* logic: the last heart went on a wrong cell */
};

/* ---- the board ------------------------------------------------------- */
/* color[]: 0 empty, 1..8 a colour, NGP_COLOR_PRISM (matches nothing, takes
 * the colour it is swapped with), NGP_COLOR_STONE (bubble mode: never pops).
 * Logic boards keep the solution here: 0 an empty cell, 1..15 a filled one
 * in that picture colour (the game shows it once the picture is done). */
#define NGP_COLOR_EMPTY  0u
#define NGP_COLOR_PRISM  9u
#define NGP_COLOR_STONE  10u

/* kind[]: what a piece does besides its colour. */
enum {
    NGP_KIND_PLAIN = 0,
    NGP_KIND_LINE_H,         /* clears its row when it goes */
    NGP_KIND_LINE_V,         /* clears its column */
    NGP_KIND_BOMB,           /* clears the 3x3 around it */
    NGP_KIND_PRISM,          /* swapped with a piece: clears that colour */
    NGP_KIND_ANCHOR,         /* bubble: holds what touches it, never pops */
    NGP_KIND_RESCUE          /* bubble: a caged friend, freed when popped or dropped */
};

/* kind[] on a logic board: what the player has marked. */
enum {
    NGP_MARK_NONE = 0,
    NGP_MARK_FILL,           /* filled (always right: a wrong fill is a mistake) */
    NGP_MARK_CROSS,          /* crossed out by the player or a finished line */
    NGP_MARK_MISS            /* a mistake: crossed out, never fillable */
};

/* cover[]: board furniture under or instead of a piece. */
#define NGP_COVER_WALL    0x80u  /* no piece ever; blocks falling */
#define NGP_COVER_LOCK    0x40u  /* piece can't be swapped; a match unlocks it */
#define NGP_COVER_CRATE   0x30u  /* crate hit points (1..3) << 4; no piece */
#define NGP_COVER_ICE     0x03u  /* ice layers (1..3) under the piece */
#define NGP_CRATE_HP(c)   (((c) >> 4) & 3u)
#define NGP_ICE(c)        ((c) & 3u)

/* Neighbour directions. Match boards use 0..3; bubble lattices 0..5. */
enum { NGP_DIR_RIGHT = 0, NGP_DIR_DOWN = 1, NGP_DIR_LEFT = 2, NGP_DIR_UP = 3 };
enum { NGP_HEX_E = 0, NGP_HEX_SE, NGP_HEX_SW, NGP_HEX_W, NGP_HEX_NW, NGP_HEX_NE };

/* ---- objectives ------------------------------------------------------ */
enum {
    NGP_OBJ_NONE = 0,
    NGP_OBJ_SCORE,           /* reach count points (count is in hundreds) */
    NGP_OBJ_COLOR,           /* clear count pieces of colour target */
    NGP_OBJ_ICE,             /* break count layers of ice (0: all of it) */
    NGP_OBJ_CRATE,           /* break count crates (0: all of them) */
    NGP_OBJ_SPECIAL,         /* make count special pieces */
    NGP_OBJ_RESCUE,          /* bubble: free count caged friends (0: all) */
    NGP_OBJ_CLEAR,           /* bubble: clear every bubble (count unused) */
    NGP_OBJ_PICTURE          /* logic: fill every cell of the picture (count unused) */
};

typedef struct {
    uint8_t type;            /* NGP_OBJ_* */
    uint8_t target;          /* colour for NGP_OBJ_COLOR */
    uint16_t count;
} NGPuzzleGoal;

/* ---- level data (ROM) ------------------------------------------------ */
/*
 * layout: h rows of w characters, row after row; spaces, '/' and newlines
 * between them are skipped. Match boards:
 *   .  a random piece        1-8  a piece of that colour
 *   #  wall                  c C K  crate with 1, 2, 3 hit points
 *   i I  random piece on 1 or 2 layers of ice     l  locked random piece
 *   h v b  random-coloured line (row / column) or bomb piece   p  prism
 * Bubble lattices (rows whose start is shifted half a cell to the right
 * hold w-1 bubbles; write their last column as '-'):
 *   .  empty      1-8  a bubble of that colour   r  a random colour
 *   A  anchor     S  stone   R  caged friend in a random colour   -  none
 * Logic boards (a layout is required):
 *   .  an empty cell          1-9 a-f  a filled cell in picture colour 1..15
 * A NULL layout fills a match board at random and leaves a bubble field
 * empty.
 */
typedef struct {
    uint8_t mode;            /* NGP_MODE_* */
    uint8_t w, h;            /* board size, cells (bubble: w is the long row) */
    uint8_t colors;          /* colours in play, 2..8 (logic: picture colours, 1..15) */
    uint32_t seed;           /* 0: a fixed default seed */
    uint16_t moves;          /* match: swaps allowed (0 = unlimited);
                                bubble: shots allowed (0 = unlimited);
                                logic: hearts, mistakes allowed (0 = unlimited) */
    uint16_t time_frames;    /* 0 = untimed; counts only while waiting for input */
    const char *layout;
    NGPuzzleGoal goals[NGP_MAX_OBJ];
    uint32_t stars[3];       /* scores for one, two and three stars */
    uint8_t flags;           /* NGP_LF_* */
    /* bubble geometry and rules (ignored by match boards) */
    uint8_t bubble_d;        /* bubble diameter, pixels (8..32) */
    uint8_t row_h;           /* row spacing, pixels (usually d * 7 / 8) */
    uint8_t loss_row;        /* a bubble resting on this row loses (< h) */
    uint8_t push_every;      /* shots between new rows from the top (0 = never) */
    uint8_t miss_limit;      /* missed shots before a new row (0 = never) */
    uint8_t first_shifted;   /* 1: row 0 starts half a cell to the right */
    int16_t launch_x, launch_y;   /* launcher, pixels from the field origin */
    uint16_t shot_speed_q8;  /* pixels per update, Q8 */
    uint16_t aim_min, aim_max;    /* allowed aim angles (1024 steps per turn) */
} NGPuzzleLevel;

#define NGP_LF_NO_SPECIALS  0x01u   /* runs of 4+ make no special pieces */
#define NGP_LF_NO_SHUFFLE   0x02u   /* no moves left loses instead of shuffling */
#define NGP_LF_NO_REFILL    0x04u   /* cleared cells stay empty (clear-the-board puzzles) */

/* ---- events ---------------------------------------------------------- */
enum {
    NGP_EV_NONE = 0,
    NGP_EV_SWAP,             /* cell, a = other cell */
    NGP_EV_SWAP_BACK,        /* cell, a = other cell: the swap made nothing */
    NGP_EV_MATCH,            /* cell = run start, a = length, b = 0 across / 1 down, value = colour */
    NGP_EV_SPECIAL_MADE,     /* cell, a = kind */
    NGP_EV_SPECIAL_FIRED,    /* cell, a = kind */
    NGP_EV_CLEARED,          /* a = pieces cleared this step, value = points */
    NGP_EV_CASCADE,          /* a = cascade level (2 = first chain) */
    NGP_EV_FALL,             /* a = pieces that fell, b = pieces added */
    NGP_EV_CRATE_HIT,        /* cell, a = hit points left */
    NGP_EV_ICE_HIT,          /* cell, a = layers left */
    NGP_EV_UNLOCK,           /* cell */
    NGP_EV_SHUFFLE,          /* no moves were left: the board was reshuffled */
    NGP_EV_GOAL,             /* a = goal index, value = remaining */
    NGP_EV_FIRE,             /* a = colour, value = angle */
    NGP_EV_BOUNCE,           /* a = 0 left wall / 1 right wall */
    NGP_EV_ATTACH,           /* cell, a = colour */
    NGP_EV_POP,              /* a = bubbles popped, value = points */
    NGP_EV_DROP,             /* a = bubbles dropped, value = points */
    NGP_EV_RESCUE,           /* cell: a caged friend is free */
    NGP_EV_MISS,             /* a = misses in a row */
    NGP_EV_NEW_ROW,          /* a new row pushed in from the top */
    NGP_EV_NO_ATTACH,        /* the shot found no free cell and was lost */
    NGP_EV_TIME_LOW,         /* ten seconds left */
    NGP_EV_WON,              /* a = NGP_END_*, b = stars */
    NGP_EV_LOST,             /* a = NGP_END_* */
    NGP_EV_MARK,             /* logic: cell, a = new mark, b = old mark, value = points */
    NGP_EV_MISTAKE,          /* logic: cell, a = hearts left (255: unlimited) */
    NGP_EV_LINE,             /* logic: cell = row or column, a = 0 row / 1 column, value = points */
    NGP_EV_REVEAL            /* logic: the hint showed the row and column through cell */
};

typedef struct {
    uint8_t type, cell, a, b;
    uint16_t value;
} NGPuzzleEvent;

/* Commands for ng_puzzle_action(). */
enum {
    NGP_CMD_SWAP = 1,        /* a = cell, b = NGP_DIR_* */
    NGP_CMD_SWAP_CELLS,      /* a, b = two neighbouring cells */
    NGP_CMD_AIM,             /* a = angle (clamped to the level's range) */
    NGP_CMD_AIM_BY,          /* a = signed change of angle */
    NGP_CMD_FIRE,            /* launch the loaded bubble */
    NGP_CMD_SWAP_NEXT,       /* exchange the loaded and the next bubble */
    NGP_CMD_HAMMER,          /* booster: clear cell a (match) or bubble a; logic:
                                show the row and column through cell a */
    NGP_CMD_SHUFFLE,         /* booster: reshuffle the board now (match) */
    NGP_CMD_COLOR_SHOT,      /* booster: load a bubble of colour a */
    NGP_CMD_MARK             /* logic: mark cell a as b (NGP_MARK_FILL, _CROSS or _NONE
                                to take a cross away); filling a wrong cell is a mistake */
};

/* One recorded action, for ng_puzzle_replay(): applied when the context
 * has run tick updates since the level began. */
typedef struct {
    uint16_t tick;
    uint8_t cmd;
    uint8_t pad;
    int16_t a, b;
} NGPuzzleAction;

typedef struct { int16_t x, y; } NGPuzzlePoint;

/* ---- the context ------------------------------------------------------ */
typedef struct {
    /* read-only for the game */
    uint8_t mode, w, h, colors;
    uint8_t color[NGP_MAX_CELLS];
    uint8_t kind[NGP_MAX_CELLS];
    uint8_t cover[NGP_MAX_CELLS];
    uint8_t fall[NGP_MAX_CELLS];      /* rows this piece fell in the last fall phase */
    uint8_t dirty[NGP_BITS_BYTES];    /* cells changed since ng_puzzle_dirty_clear */
    uint8_t cleared[NGP_BITS_BYTES];  /* cells emptied by the last clear phase */
    uint32_t score;
    uint16_t moves_used, moves_left;  /* moves_left 0xFFFF: unlimited */
    uint16_t time_left;               /* frames; 0xFFFF: untimed */
    uint16_t goal_left[NGP_MAX_OBJ];
    uint8_t cascade;                  /* chain level of the current resolution */
    uint8_t state;                    /* internal phase */
    uint8_t end_reason;               /* NGP_END_* */
    uint8_t stars;
    uint8_t specials_made;
    uint8_t shuffles;
    uint16_t tick;                    /* updates since the level began */
    /* bubble mode */
    uint16_t angle;                   /* current aim */
    uint8_t shot_color, next_color;   /* loaded and next bubble */
    uint8_t shot_kind;
    uint8_t misses, shots_since_row;
    uint8_t parity;                   /* row r starts shifted when ((r + parity) & 1) */
    uint8_t attach_cell;              /* last cell a bubble stuck to */
    int32_t shot_x_q8, shot_y_q8;     /* the bubble in flight */
    int32_t shot_vx_q8, shot_vy_q8;
    uint16_t shots_left;              /* 0xFFFF: unlimited */
    /* internal */
    const NGPuzzleLevel *level;
    uint32_t seed, rng;
    uint8_t swap_a, swap_b;           /* cells of the swap being resolved */
    uint8_t ev_head, ev_count;
    uint16_t ev_dropped;              /* events lost to a full queue */
    NGPuzzleEvent ev[NGP_EVENT_QUEUE];
    uint8_t mark[NGP_BITS_BYTES];     /* scratch */
    uint8_t queue[NGP_MAX_CELLS];     /* scratch (searches) */
} NGPuzzle;

/* Optional bounded, read-only path prediction. Storage belongs to the caller;
 * the source context and its immutable ROM level must remain alive. */
#define NGP_PREVIEW_POINTS 6u
#define NGP_PREVIEW_MAX_STEPS 8u
enum {
    NGP_PREVIEW_EMPTY = 0, NGP_PREVIEW_BUSY, NGP_PREVIEW_READY,
    NGP_PREVIEW_CANCELLED, NGP_PREVIEW_INVALID
};
typedef struct {
    const NGPuzzle *source;
    const NGPuzzleLevel *level;
    uint32_t generation;
    int32_t x_q8, y_q8, vx_q8, vy_q8;
    uint16_t steps, angle, requested_angle;
    uint8_t state, count, cell, hit, parity;
    NGPuzzlePoint points[NGP_PREVIEW_POINTS];
} NGPuzzlePreview;

/* Buffer capacity for ng_puzzle_save(); its return value is the used length. */
#define NGP_SAVE_SIZE   (24u + 4u * NGP_MAX_CELLS + 4u * NGP_MAX_OBJ + 32u)

/* ---- calls ------------------------------------------------------------ */
/* Check level data: NGP_OK, or NGP_ERR_LEVEL with *bad_index set to the
 * offending layout character (or 0xFFFF for a header field) when non-NULL. */
int      NEOGEO_USER ng_puzzle_level_check(const NGPuzzleLevel *level, uint16_t *bad_index);
/* Start a level. seed 0 takes the level's seed. The level must stay valid
 * (ROM) while the context is in use. Returns NGP_OK or an error. */
int      NEOGEO_USER ng_puzzle_init(NGPuzzle *p, const NGPuzzleLevel *level, uint32_t seed);
/* Start the same level again with the same seed. */
void     NEOGEO_USER ng_puzzle_reset(NGPuzzle *p);
/* Give a command (NGP_CMD_*). Returns NGP_OK, NGP_ERR_BUSY while the last
 * one is resolving, NGP_ERR_ILLEGAL for a swap that makes nothing (the board
 * swaps it back and no move is spent), or another error. */
int      NEOGEO_USER ng_puzzle_action(NGPuzzle *p, uint8_t cmd, int16_t a, int16_t b);
/* One logical tick: the next resolution phase, a step of a bubble's flight,
 * or a frame of the level timer while waiting for input. Call it once a
 * frame, or only when the game's animation of the last phase is done. */
void     NEOGEO_USER ng_puzzle_update(NGPuzzle *p);
/* Run updates until input is wanted or the level ends (bounded). */
void     NEOGEO_USER ng_puzzle_settle(NGPuzzle *p);
uint8_t  NEOGEO_USER ng_puzzle_status(const NGPuzzle *p);
/* Bubble mode: 1 while a shot is in the air (draw it at shot_x_q8, shot_y_q8). */
uint8_t  NEOGEO_USER ng_puzzle_flying(const NGPuzzle *p);
/* Take the oldest event; 0 when there is none. */
uint8_t  NEOGEO_USER ng_puzzle_event(NGPuzzle *p, NGPuzzleEvent *out);

/* Board helpers. */
uint8_t  NEOGEO_USER ng_puzzle_cell(const NGPuzzle *p, uint8_t x, uint8_t y);   /* NGP_NO_CELL if outside */
uint8_t  NEOGEO_USER ng_puzzle_neighbor(const NGPuzzle *p, uint8_t cell, uint8_t dir);
uint8_t  NEOGEO_USER ng_puzzle_row_shifted(const NGPuzzle *p, uint8_t row);     /* bubble rows */
uint8_t  NEOGEO_USER ng_puzzle_row_cells(const NGPuzzle *p, uint8_t row);       /* cells in a row */
/* Top-left pixel of a cell (match: cell_px a side; bubble: the lattice). */
void     NEOGEO_USER ng_puzzle_cell_px(const NGPuzzle *p, uint8_t cell, uint8_t cell_px,
                                       int16_t *x, int16_t *y);
/* Dirty cells: the next one at or after from, or NGP_NO_CELL; then clear. */
uint8_t  NEOGEO_USER ng_puzzle_next_dirty(const NGPuzzle *p, uint8_t from);
void     NEOGEO_USER ng_puzzle_dirty_clear(NGPuzzle *p);

/* Logic mode: the clue of row (column 0) or column (column 1) `index`: its runs
 * of filled cells, first to last, into runs[] (up to max). Returns how many
 * runs the line has (0: an empty line); 0xFF for a bad context or index. */
uint8_t  NEOGEO_USER ng_puzzle_logic_clue(const NGPuzzle *p, uint8_t column, uint8_t index,
                                          uint8_t *runs, uint8_t max);
/* Logic mode: 1 when every filled cell of that row or column is filled. */
uint8_t  NEOGEO_USER ng_puzzle_logic_line_done(const NGPuzzle *p, uint8_t column, uint8_t index);

/* Match mode: a swap worth making (the longest run, first in board order). */
uint8_t  NEOGEO_USER ng_puzzle_hint(const NGPuzzle *p, uint8_t *cell, uint8_t *dir);
uint8_t  NEOGEO_USER ng_puzzle_has_moves(const NGPuzzle *p);
/* Bubble mode: the path a shot at angle would take, without firing it.
 * Fills up to max points (launcher, each rebound, the resting place) and
 * returns how many; *cell gets the lattice cell it would stick to. */
uint8_t  NEOGEO_USER ng_puzzle_bubble_preview(const NGPuzzle *p, uint16_t angle,
                                              NGPuzzlePoint *pts, uint8_t max, uint8_t *cell);
/* First occupied bubble actually struck along the preview's collision path.
 * NGP_NO_CELL for a clear route to the ceiling or invalid context. Unlike
 * preview's attachment cell, this is an occupied target, suitable for an
 * aimed booster. Anchors/stones are returned too: the game decides whether
 * its booster can affect them. Read-only, using the same flight geometry. */
uint8_t  NEOGEO_USER ng_puzzle_bubble_target(const NGPuzzle *p, uint16_t angle);
/* Begin/restart a prediction, then spend at most min(budget,8) flight substeps
 * per step(). Returns NGP_PREVIEW_*. Budget 0 validates without advancing.
 * Increment the caller's generation whenever occupancy, geometry or level
 * changes; a stale BUSY or READY result becomes CANCELLED without reading an
 * old context. Parity and level-pointer changes also cancel automatically.
 * Never mutates p, its RNG, events, dirty flags or gameplay clock. READY has
 * the same points/attachment/hit as the synchronous queries (up to 6 points).
 * BUSY exposes only the computed prefix, ending at x_q8/y_q8; it must not be
 * displayed as a completed collision prediction. Restart on an aim change. */
uint8_t  NEOGEO_USER ng_puzzle_preview_begin(NGPuzzlePreview *preview, const NGPuzzle *p,
                                             uint16_t angle, uint32_t generation);
uint8_t  NEOGEO_USER ng_puzzle_preview_step(NGPuzzlePreview *preview, const NGPuzzle *p,
                                            uint32_t generation, uint8_t budget);
/* Bubble mode: an angle worth firing at (bounded search, about 40 trial
 * paths: call it on a request, not every frame). Returns 2 if the shot
 * would pop a group, 1 if it only joins one of its colour, 0 if none helps. */
uint8_t  NEOGEO_USER ng_puzzle_bubble_hint(const NGPuzzle *p, uint16_t *angle);

/* State hash (FNV-1a over rules and future-affecting state; excludes event,
 * dirty, animation and search scratch). Hash values changed with save v2.
 * Saves are big-endian v2, at most NGP_SAVE_SIZE bytes, with a full rule
 * identity and an accidental-corruption checksum (not authentication).
 * Load validates before changing p. On success events are discarded and all
 * cells marked dirty; games must rebuild presentation from the loaded board.
 * Legacy v1 saves remain readable, with their original weaker level identity
 * and no checksum. Saving a loaded v1 context upgrades it to v2. */
uint32_t NEOGEO_USER ng_puzzle_hash(const NGPuzzle *p);
uint16_t NEOGEO_USER ng_puzzle_save(const NGPuzzle *p, uint8_t *buf, uint16_t size);
int      NEOGEO_USER ng_puzzle_load(NGPuzzle *p, const NGPuzzleLevel *level, const uint8_t *buf, uint16_t size);
/* Play a level from its start through a recorded action list, settling at
 * the end; returns the final state hash. */
uint32_t NEOGEO_USER ng_puzzle_replay(NGPuzzle *p, const NGPuzzleLevel *level, uint32_t seed,
                                      const NGPuzzleAction *actions, uint16_t count);

#ifdef __cplusplus
}
#endif
#endif
