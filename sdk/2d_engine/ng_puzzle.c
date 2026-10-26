/*
 * ng_puzzle.c -- the board-puzzle genre engine (see ng_puzzle.h).
 *
 * One translation unit, sections in this order: small helpers (bits,
 * copies, the random generator, events), level parsing, the match board
 * (runs, specials, clearing, gravity, moves, shuffles), the bubble lattice
 * (topology, flight, attachment, groups, rows), picture logic (clues,
 * marks, finished lines), and the shared state machine, hashing,
 * save/load and replay.
 *
 * 68000 notes: no 32-bit divide or modulo in the frame's work (16-bit
 * divu.w through ngp_div only), straight copies through ngp_copy (GCC -O2
 * can turn a C copy loop into an addressing mode the 68000 evaluates after
 * the increment), and no struct assignment.
 */
#include "ng_puzzle.h"
#include "ng_trig.h"

/* internal phases (NGPuzzle.state) */
#define ST_IDLE    0u   /* waiting for input (match) or aiming (bubble) */
#define ST_MATCH   1u   /* find and clear runs */
#define ST_FALL    2u   /* gravity and refill */
#define ST_SETTLE  3u   /* no more runs: goals, moves, dead boards */
#define ST_FLIGHT  4u   /* a bubble in the air */
#define ST_POP     5u   /* a bubble stuck: pop, drop, rows */
#define ST_WON     6u
#define ST_LOST    7u

#define MAX_CASCADE   48u
#define MAX_RUNS      48u
#define SETTLE_LIMIT  8192u
#define FLIGHT_LIMIT  4096u
#define TIME_LOW      600u
#define LOGIC_FILL    10u           /* points: a cell filled */
#define LOGIC_LINE    50u           /* a row or column finished */
#define LOGIC_HEART   300u          /* the finished picture: per heart left */
#define LOGIC_SECOND  5u            /* and per second left on a timed board */
#define DEFAULT_SEED  0x2545F491u
#define SAVE_MAGIC    0x4E47505Au   /* 'NGPZ' */
#define SAVE_VERSION  2u

/* ======================================================================
 * helpers
 * ==================================================================== */
static uint8_t NEOGEO_USER bit_get(const uint8_t *b, uint8_t i)
{
    return (uint8_t)((b[i >> 3] >> (i & 7u)) & 1u);
}

static void NEOGEO_USER bit_set(uint8_t *b, uint8_t i)
{
    b[i >> 3] = (uint8_t)(b[i >> 3] | (1u << (i & 7u)));
}

static void NEOGEO_USER zero(uint8_t *d, uint16_t n)
{
    while (n--) *d++ = 0;
}

/* Byte copy, overlapping-safe in either direction. */
static void NEOGEO_USER ngp_copy(uint8_t *d, const uint8_t *s, uint16_t n)
{
    if (!n || d == s) return;
#if defined(__mc68000__) || defined(__m68k__)
    if (d < s) {
        __asm__ volatile("1:\n\tmove.b (%1)+,(%0)+\n\tsubq.w #1,%2\n\tbne.s 1b"
                         : "+a"(d), "+a"(s), "+d"(n) : : "memory", "cc");
    } else {
        d += n; s += n;
        __asm__ volatile("1:\n\tmove.b -(%1),-(%0)\n\tsubq.w #1,%2\n\tbne.s 1b"
                         : "+a"(d), "+a"(s), "+d"(n) : : "memory", "cc");
    }
#else
    if (d < s) { while (n--) *d++ = *s++; }
    else { d += n; s += n; while (n--) *--d = *--s; }
#endif
}

/* 16-bit unsigned divide: one divu.w on the 68000. */
static uint16_t NEOGEO_USER ngp_div(uint16_t a, uint16_t b)
{
    return b ? (uint16_t)((uint16_t)a / (uint16_t)b) : 0u;
}

static uint16_t NEOGEO_USER rng_next(NGPuzzle *p)
{
    uint32_t x = p->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    p->rng = x;
    return (uint16_t)(x >> 16);
}

/* 0..n-1 by one 16x16 multiply. */
static uint16_t NEOGEO_USER rng_range(NGPuzzle *p, uint16_t n)
{
    return (uint16_t)(((uint32_t)rng_next(p) * (uint32_t)n) >> 16);
}

static void NEOGEO_USER emit(NGPuzzle *p, uint8_t type, uint8_t cell, uint8_t a, uint8_t b, uint16_t value)
{
    NGPuzzleEvent *e;
    if (p->ev_count >= NGP_EVENT_QUEUE) { p->ev_dropped++; return; }
    e = &p->ev[(p->ev_head + p->ev_count) & (NGP_EVENT_QUEUE - 1u)];
    e->type = type; e->cell = cell; e->a = a; e->b = b; e->value = value;
    p->ev_count++;
}

static void NEOGEO_USER add_score(NGPuzzle *p, uint32_t pts)
{
    uint32_t s = p->score + pts;
    p->score = (s < p->score) ? 0xFFFFFFFFu : s;
}

static uint8_t NEOGEO_USER cell_x(const NGPuzzle *p, uint8_t c)
{
    return (uint8_t)(c - (uint8_t)(ngp_div(c, p->w) * p->w));
}

static uint8_t NEOGEO_USER cell_y(const NGPuzzle *p, uint8_t c)
{
    return (uint8_t)ngp_div(c, p->w);
}

static void NEOGEO_USER touch(NGPuzzle *p, uint8_t c)
{
    bit_set(p->dirty, c);
}

static void NEOGEO_USER set_piece(NGPuzzle *p, uint8_t c, uint8_t color, uint8_t kind)
{
    p->color[c] = color;
    p->kind[c] = kind;
    touch(p, c);
}

/* ======================================================================
 * level parsing and checking
 * ==================================================================== */
static uint8_t NEOGEO_USER is_sep(char ch)
{
    return (uint8_t)(ch == ' ' || ch == '/' || ch == '\n' || ch == '\r' || ch == '\t');
}

/* A logic layout character's picture colour: 0 for an empty cell. */
static uint8_t NEOGEO_USER logic_color(char ch)
{
    if (ch >= '1' && ch <= '9') return (uint8_t)(ch - '0');
    if (ch >= 'a' && ch <= 'f') return (uint8_t)(ch - 'a' + 10);
    return 0;
}

static uint8_t NEOGEO_USER char_ok(uint8_t mode, char ch)
{
    if (mode == NGP_MODE_LOGIC) return (uint8_t)(ch == '.' || logic_color(ch));
    if (ch >= '1' && ch <= '8') return 1;
    if (mode == NGP_MODE_MATCH) {
        switch (ch) {
        case '.': case '#': case 'c': case 'C': case 'K': case 'i': case 'I':
        case 'l': case 'h': case 'v': case 'b': case 'p': return 1;
        default: return 0;
        }
    }
    switch (ch) {
    case '.': case 'r': case 'A': case 'S': case 'R': case '-': return 1;
    default: return 0;
    }
}

static uint8_t NEOGEO_USER shifted_row(const NGPuzzle *p, uint8_t r)
{
    return (uint8_t)((r + p->parity) & 1u);
}

uint8_t NEOGEO_USER ng_puzzle_row_shifted(const NGPuzzle *p, uint8_t row)
{
    if (!p || p->mode != NGP_MODE_BUBBLE) return 0;
    return shifted_row(p, row);
}

uint8_t NEOGEO_USER ng_puzzle_row_cells(const NGPuzzle *p, uint8_t row)
{
    if (!p || row >= p->h) return 0;
    if (p->mode == NGP_MODE_BUBBLE && shifted_row(p, row)) return (uint8_t)(p->w - 1u);
    return p->w;
}

/* Three fixed pieces of one colour in a line (the board would start with a
 * run already made). grid holds the layout's board characters. */
static uint8_t NEOGEO_USER fixed_run(const char *grid, uint8_t w, uint8_t h)
{
    uint8_t x, y;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            char ch = grid[(uint16_t)y * w + x];
            if (ch < '1' || ch > '8') continue;
            if (x + 2u < w && grid[(uint16_t)y * w + x + 1] == ch && grid[(uint16_t)y * w + x + 2] == ch) return 1;
            if (y + 2u < h && grid[(uint16_t)(y + 1u) * w + x] == ch && grid[(uint16_t)(y + 2u) * w + x] == ch) return 1;
        }
    return 0;
}

int NEOGEO_USER ng_puzzle_level_check(const NGPuzzleLevel *lv, uint16_t *bad)
{
    uint16_t i, n = 0, cells, ice = 0, crates = 0, rescue = 0, filled = 0;
    char grid[NGP_MAX_CELLS];
    uint8_t g;
    if (bad) *bad = 0xFFFFu;
    if (!lv) return NGP_ERR_ARG;
    if (lv->mode > NGP_MODE_LOGIC) return NGP_ERR_LEVEL;
    if (lv->w < 3 || lv->w > NGP_MAX_W || lv->h < 3 || lv->h > NGP_MAX_H) return NGP_ERR_LEVEL;
    cells = (uint16_t)((uint16_t)lv->w * lv->h);
    if (cells > NGP_MAX_CELLS) return NGP_ERR_LEVEL;
    if (lv->mode == NGP_MODE_LOGIC) {
        if (lv->colors < 1 || lv->colors > 15 || !lv->layout) return NGP_ERR_LEVEL;
    } else if (lv->colors < 2 || lv->colors > NGP_MAX_COLORS) return NGP_ERR_LEVEL;
    if (lv->mode == NGP_MODE_BUBBLE) {
        if (lv->bubble_d < 8 || lv->bubble_d > 32) return NGP_ERR_LEVEL;
        if (lv->row_h < 4 || lv->row_h > lv->bubble_d) return NGP_ERR_LEVEL;
        if (lv->loss_row == 0 || lv->loss_row >= lv->h) return NGP_ERR_LEVEL;
        if (!lv->shot_speed_q8) return NGP_ERR_LEVEL;
        if (lv->launch_x < 0 || lv->launch_x >= (int16_t)((uint16_t)lv->w * lv->bubble_d)) return NGP_ERR_LEVEL;
        if (lv->launch_y <= (int16_t)((uint16_t)lv->loss_row * lv->row_h)) return NGP_ERR_LEVEL;
        if (lv->aim_min && (lv->aim_min <= 512u || lv->aim_max >= 1024u || lv->aim_min > lv->aim_max))
            return NGP_ERR_LEVEL;
    }
    if (lv->layout) {
        const char *s = lv->layout;
        for (i = 0; s[i]; i++) {
            if (is_sep(s[i])) continue;
            if (!char_ok(lv->mode, s[i])) { if (bad) *bad = i; return NGP_ERR_LEVEL; }
            if (n < cells) {
                uint8_t x = (uint8_t)(n - (uint16_t)(ngp_div(n, lv->w) * lv->w));
                uint8_t y = (uint8_t)ngp_div(n, lv->w);
                char ch = s[i];
                if (lv->mode == NGP_MODE_LOGIC) {
                    if (logic_color(ch) > lv->colors) { if (bad) *bad = i; return NGP_ERR_LEVEL; }
                    if (logic_color(ch)) filled++;
                } else if (ch >= '1' && ch <= '8' && (uint8_t)(ch - '0') > lv->colors) { if (bad) *bad = i; return NGP_ERR_LEVEL; }
                if (ch == 'i' || ch == 'I') ice++;
                if (ch == 'c' || ch == 'C' || ch == 'K') crates++;
                if (ch == 'R') rescue++;
                if (lv->mode == NGP_MODE_BUBBLE) {
                    uint8_t sh = (uint8_t)((y + (lv->first_shifted ? 1u : 0u)) & 1u);
                    if (sh && x == lv->w - 1u && ch != '-' && ch != '.') { if (bad) *bad = i; return NGP_ERR_LEVEL; }
                    if (!(sh && x == lv->w - 1u) && ch == '-') { if (bad) *bad = i; return NGP_ERR_LEVEL; }
                }
                grid[n] = ch;
            }
            n++;
        }
        if (n != cells) { if (bad) *bad = 0xFFFEu; return NGP_ERR_LEVEL; }
        if (lv->mode == NGP_MODE_MATCH && fixed_run(grid, lv->w, lv->h)) return NGP_ERR_LEVEL;
        if (lv->mode == NGP_MODE_LOGIC && !filled) return NGP_ERR_LEVEL;
    }
    for (g = 0; g < NGP_MAX_OBJ; g++) {
        const NGPuzzleGoal *o = &lv->goals[g];
        if (lv->mode == NGP_MODE_LOGIC) {
            /* a picture is won by finishing it, and nothing else may be asked: a
               finished picture that missed another goal would leave nothing to do */
            if (o->type != NGP_OBJ_NONE && o->type != NGP_OBJ_PICTURE) return NGP_ERR_LEVEL;
            continue;
        }
        switch (o->type) {
        case NGP_OBJ_NONE: case NGP_OBJ_SCORE: case NGP_OBJ_SPECIAL: break;
        case NGP_OBJ_COLOR: if (o->target < 1 || o->target > lv->colors) return NGP_ERR_LEVEL; break;
        case NGP_OBJ_ICE: if (lv->mode != NGP_MODE_MATCH || !ice) return NGP_ERR_LEVEL; break;
        case NGP_OBJ_CRATE: if (lv->mode != NGP_MODE_MATCH || !crates) return NGP_ERR_LEVEL; break;
        case NGP_OBJ_RESCUE: if (lv->mode != NGP_MODE_BUBBLE || !rescue) return NGP_ERR_LEVEL; break;
        case NGP_OBJ_CLEAR: if (lv->mode != NGP_MODE_BUBBLE) return NGP_ERR_LEVEL; break;
        default: return NGP_ERR_LEVEL;
        }
    }
    return NGP_OK;
}

/* ======================================================================
 * match board: runs and moves
 * ==================================================================== */
static uint8_t NEOGEO_USER matchable(const NGPuzzle *p, uint8_t c)
{
    uint8_t col = p->color[c];
    return (uint8_t)(col >= 1 && col <= NGP_MAX_COLORS && !(p->cover[c] & (NGP_COVER_WALL | NGP_COVER_CRATE)));
}

static uint8_t NEOGEO_USER movable(const NGPuzzle *p, uint8_t c)
{
    uint8_t col = p->color[c];
    if (!col || col > NGP_COLOR_PRISM) return 0;
    return (uint8_t)!(p->cover[c] & (NGP_COVER_WALL | NGP_COVER_CRATE | NGP_COVER_LOCK));
}

/* Colour at c with cells a and b exchanged (b = NGP_NO_CELL: as is). */
static uint8_t NEOGEO_USER color_sw(const NGPuzzle *p, uint8_t c, uint8_t a, uint8_t b)
{
    if (b != NGP_NO_CELL) {
        if (c == a) c = b; else if (c == b) c = a;
    }
    return matchable(p, c) ? p->color[c] : 0u;
}

/* Longest straight run through cell c (colour col) with a, b exchanged. */
static uint8_t NEOGEO_USER run_through(const NGPuzzle *p, uint8_t c, uint8_t col, uint8_t a, uint8_t b)
{
    uint8_t x = cell_x(p, c), y = cell_y(p, c), best, n;
    int16_t i;
    if (!col || col > NGP_MAX_COLORS) return 0;
    n = 1;
    for (i = (int16_t)x - 1; i >= 0 && color_sw(p, (uint8_t)(c - (x - i)), a, b) == col; i--) n++;
    for (i = (int16_t)x + 1; i < (int16_t)p->w && color_sw(p, (uint8_t)(c + (i - x)), a, b) == col; i++) n++;
    best = n;
    n = 1;
    for (i = (int16_t)y - 1; i >= 0 && color_sw(p, (uint8_t)(c - (uint8_t)((y - i) * p->w)), a, b) == col; i--) n++;
    for (i = (int16_t)y + 1; i < (int16_t)p->h && color_sw(p, (uint8_t)(c + (uint8_t)((i - y) * p->w)), a, b) == col; i++) n++;
    return n > best ? n : best;
}

static uint8_t NEOGEO_USER is_special(uint8_t kind)
{
    return (uint8_t)(kind == NGP_KIND_LINE_H || kind == NGP_KIND_LINE_V || kind == NGP_KIND_BOMB);
}

static uint8_t NEOGEO_USER neighbors4(const NGPuzzle *p, uint8_t a, uint8_t b)
{
    uint8_t ax = cell_x(p, a), ay = cell_y(p, a), bx = cell_x(p, b), by = cell_y(p, b);
    if (ay == by) return (uint8_t)(ax + 1u == bx || bx + 1u == ax);
    if (ax == bx) return (uint8_t)(ay + 1u == by || by + 1u == ay);
    return 0;
}

/* How good swapping a and b would be: 0 = illegal. */
static uint8_t NEOGEO_USER swap_value(const NGPuzzle *p, uint8_t a, uint8_t b)
{
    uint8_t ra, rb, ca, cb;
    if (!movable(p, a) || !movable(p, b)) return 0;
    ca = p->color[a]; cb = p->color[b];
    if (ca == NGP_COLOR_PRISM || cb == NGP_COLOR_PRISM) return 7;
    if (is_special(p->kind[a]) && is_special(p->kind[b])) return 6;
    if (ca == cb) return 0;
    ra = run_through(p, a, cb, a, b);
    rb = run_through(p, b, ca, a, b);
    if (ra < 3) ra = 0;
    if (rb < 3) rb = 0;
    return ra > rb ? ra : rb;
}

uint8_t NEOGEO_USER ng_puzzle_hint(const NGPuzzle *p, uint8_t *cell, uint8_t *dir)
{
    uint8_t c, best = 0, bc = NGP_NO_CELL, bd = 0, n;
    if (!p || p->mode != NGP_MODE_MATCH) return 0;
    n = (uint8_t)(p->w * p->h);
    for (c = 0; c < n; c++) {
        uint8_t x = cell_x(p, c), v;
        if (x + 1u < p->w) {
            v = swap_value(p, c, (uint8_t)(c + 1u));
            if (v > best) { best = v; bc = c; bd = NGP_DIR_RIGHT; }
        }
        if (c + p->w < n) {
            v = swap_value(p, c, (uint8_t)(c + p->w));
            if (v > best) { best = v; bc = c; bd = NGP_DIR_DOWN; }
        }
    }
    if (!best) return 0;
    if (cell) *cell = bc;
    if (dir) *dir = bd;
    return 1;
}

uint8_t NEOGEO_USER ng_puzzle_has_moves(const NGPuzzle *p)
{
    return ng_puzzle_hint(p, 0, 0);
}

static uint8_t NEOGEO_USER board_has_run(const NGPuzzle *p)
{
    uint8_t c, n = (uint8_t)(p->w * p->h);
    for (c = 0; c < n; c++)
        if (matchable(p, c) && run_through(p, c, p->color[c], NGP_NO_CELL, NGP_NO_CELL) >= 3) return 1;
    return 0;
}

/* A colour for cell c that doesn't complete a run with the two cells to
 * its left or above (cells are filled in board order). */
static uint8_t NEOGEO_USER calm_color(NGPuzzle *p, uint8_t c)
{
    uint8_t x = cell_x(p, c), y = cell_y(p, c), tries, col = 1;
    for (tries = 0; tries < 16; tries++) {
        col = (uint8_t)(1u + rng_range(p, p->colors));
        if (x >= 2 && matchable(p, (uint8_t)(c - 1)) && p->color[c - 1] == col && p->color[c - 2] == col
            && matchable(p, (uint8_t)(c - 2))) continue;
        if (y >= 2 && matchable(p, (uint8_t)(c - p->w)) && p->color[c - p->w] == col
            && matchable(p, (uint8_t)(c - 2u * p->w)) && p->color[c - 2u * p->w] == col) continue;
        if (x + 2u < p->w && matchable(p, (uint8_t)(c + 1)) && matchable(p, (uint8_t)(c + 2))
            && p->color[c + 1] == col && p->color[c + 2] == col) continue;
        if (y + 2u < p->h && matchable(p, (uint8_t)(c + p->w)) && matchable(p, (uint8_t)(c + 2u * p->w))
            && p->color[c + p->w] == col && p->color[c + 2u * p->w] == col) continue;
        break;
    }
    return col;
}

/* Reshuffle the movable pieces until there are no runs and a move exists.
 * Returns 0 if the board can't be given a move. */
static uint8_t NEOGEO_USER shuffle_board(NGPuzzle *p)
{
    uint8_t c, n = (uint8_t)(p->w * p->h), m = 0, attempt;
    for (c = 0; c < n; c++)
        if (movable(p, c) && p->color[c] != NGP_COLOR_PRISM) p->queue[m++] = c;
    if (m < 2) return 0;
    for (attempt = 0; attempt < 24; attempt++) {
        uint8_t i;
        for (i = (uint8_t)(m - 1u); i > 0; i--) {
            uint8_t j = (uint8_t)rng_range(p, (uint16_t)(i + 1u));
            uint8_t ci = p->queue[i], cj = p->queue[j];
            uint8_t tc = p->color[ci], tk = p->kind[ci];
            p->color[ci] = p->color[cj]; p->kind[ci] = p->kind[cj];
            p->color[cj] = tc; p->kind[cj] = tk;
        }
        if (attempt >= 12) {
            /* past halfway: recolour plain pieces instead of reordering */
            for (i = 0; i < m; i++) {
                uint8_t ci = p->queue[i];
                if (p->kind[ci] == NGP_KIND_PLAIN) p->color[ci] = calm_color(p, ci);
            }
        }
        if (!board_has_run(p) && ng_puzzle_has_moves(p)) {
            for (i = 0; i < m; i++) touch(p, p->queue[i]);
            return 1;
        }
    }
    for (c = 0; c < m; c++) touch(p, p->queue[c]);
    return 0;
}

/* ======================================================================
 * match board: clearing
 * ==================================================================== */
typedef struct { uint8_t start, len, dir, color; } Run;

static void NEOGEO_USER mark_area(const NGPuzzle *p, uint8_t *m, uint8_t c, uint8_t kind)
{
    uint8_t x = cell_x(p, c), y = cell_y(p, c), i, j;
    if (kind == NGP_KIND_LINE_H) {
        uint8_t row = (uint8_t)(c - x);
        for (i = 0; i < p->w; i++) bit_set(m, (uint8_t)(row + i));
    } else if (kind == NGP_KIND_LINE_V) {
        for (i = 0; i < p->h; i++) bit_set(m, (uint8_t)(x + (uint8_t)(i * p->w)));
    } else if (kind == NGP_KIND_BOMB) {
        for (j = 0; j < 3; j++) {
            int16_t yy = (int16_t)y + j - 1;
            if (yy < 0 || yy >= (int16_t)p->h) continue;
            for (i = 0; i < 3; i++) {
                int16_t xx = (int16_t)x + i - 1;
                if (xx < 0 || xx >= (int16_t)p->w) continue;
                bit_set(m, (uint8_t)((uint8_t)yy * p->w + (uint8_t)xx));
            }
        }
    }
}

static void NEOGEO_USER mark_color(const NGPuzzle *p, uint8_t *m, uint8_t col)
{
    uint8_t c, n = (uint8_t)(p->w * p->h);
    for (c = 0; c < n; c++)
        if (matchable(p, c) && p->color[c] == col) bit_set(m, c);
}

static uint8_t NEOGEO_USER common_color(const NGPuzzle *p)
{
    uint8_t cnt[NGP_MAX_COLORS + 1u], c, n = (uint8_t)(p->w * p->h), best = 1;
    zero(cnt, sizeof(cnt));
    for (c = 0; c < n; c++)
        if (matchable(p, c)) cnt[p->color[c]]++;
    for (c = 2; c <= NGP_MAX_COLORS; c++) if (cnt[c] > cnt[best]) best = c;
    return best;
}

static void NEOGEO_USER goal_progress(NGPuzzle *p, uint8_t type, uint8_t target, uint16_t amount)
{
    uint8_t g;
    for (g = 0; g < NGP_MAX_OBJ; g++) {
        const NGPuzzleGoal *o = &p->level->goals[g];
        if (o->type != type || !p->goal_left[g]) continue;
        if (type == NGP_OBJ_COLOR && o->target != target) continue;
        p->goal_left[g] = (uint16_t)(p->goal_left[g] > amount ? (uint16_t)(p->goal_left[g] - amount) : 0u);
        emit(p, NGP_EV_GOAL, NGP_NO_CELL, g, 0, p->goal_left[g]);
    }
}

static void NEOGEO_USER hit_crate(NGPuzzle *p, uint8_t c)
{
    uint8_t hp = NGP_CRATE_HP(p->cover[c]);
    if (!hp) return;
    hp--;
    p->cover[c] = (uint8_t)((p->cover[c] & (uint8_t)~NGP_COVER_CRATE) | (uint8_t)(hp << 4));
    touch(p, c);
    emit(p, NGP_EV_CRATE_HIT, c, hp, 0, 0);
    if (!hp) goal_progress(p, NGP_OBJ_CRATE, 0, 1);
}

/* One match phase. Returns the number of cells cleared or changed. */
static uint8_t NEOGEO_USER match_phase(NGPuzzle *p)
{
    Run runs[MAX_RUNS];
    uint8_t hmark[NGP_BITS_BYTES], vmark[NGP_BITS_BYTES], fired[NGP_BITS_BYTES], hitc[NGP_BITS_BYTES];
    uint8_t made_cell[MAX_RUNS], made_kind[MAX_RUNS], made_color[MAX_RUNS], used[MAX_RUNS];
    uint8_t nruns = 0, nmade = 0, n = (uint8_t)(p->w * p->h), c, x, y, i, any;
    uint16_t cleared = 0, fired_n = 0;
    uint8_t *m = p->mark;
    uint8_t sa = p->cascade == 1u ? p->swap_a : NGP_NO_CELL;
    uint8_t sb = p->cascade == 1u ? p->swap_b : NGP_NO_CELL;

    zero(m, NGP_BITS_BYTES); zero(hmark, NGP_BITS_BYTES); zero(vmark, NGP_BITS_BYTES);
    zero(fired, NGP_BITS_BYTES); zero(hitc, NGP_BITS_BYTES); zero(p->cleared, NGP_BITS_BYTES);

    /* runs across, then down */
    for (y = 0; y < p->h; y++) {
        x = 0;
        while (x < p->w) {
            uint8_t s = (uint8_t)(y * p->w + x), len = 1, col;
            if (!matchable(p, s)) { x++; continue; }
            col = p->color[s];
            while (x + len < p->w && matchable(p, (uint8_t)(s + len)) && p->color[s + len] == col) len++;
            if (len >= 3) {
                for (i = 0; i < len; i++) { bit_set(m, (uint8_t)(s + i)); bit_set(hmark, (uint8_t)(s + i)); }
                if (nruns < MAX_RUNS) { runs[nruns].start = s; runs[nruns].len = len; runs[nruns].dir = 0; runs[nruns].color = col; nruns++; }
            }
            x = (uint8_t)(x + len);
        }
    }
    for (x = 0; x < p->w; x++) {
        y = 0;
        while (y < p->h) {
            uint8_t s = (uint8_t)(y * p->w + x), len = 1, col;
            if (!matchable(p, s)) { y++; continue; }
            col = p->color[s];
            while (y + len < p->h && matchable(p, (uint8_t)(s + len * p->w)) && p->color[s + len * p->w] == col) len++;
            if (len >= 3) {
                for (i = 0; i < len; i++) { bit_set(m, (uint8_t)(s + i * p->w)); bit_set(vmark, (uint8_t)(s + i * p->w)); }
                if (nruns < MAX_RUNS) { runs[nruns].start = s; runs[nruns].len = len; runs[nruns].dir = 1; runs[nruns].color = col; nruns++; }
            }
            y = (uint8_t)(y + len);
        }
    }

    any = nruns;
    /* the swap itself: prisms and two specials go off without a run */
    if (sa != NGP_NO_CELL) {
        uint8_t a = sa, b = sb;
        uint8_t ca = p->color[a], cb = p->color[b];
        if (ca == NGP_COLOR_PRISM && cb == NGP_COLOR_PRISM) {
            for (c = 0; c < n; c++) if (p->color[c]) bit_set(m, c);
            bit_set(fired, a); bit_set(fired, b);
            emit(p, NGP_EV_SPECIAL_FIRED, a, NGP_KIND_PRISM, 0, 0);
            fired_n += 2; any = 1;
        } else if (ca == NGP_COLOR_PRISM || cb == NGP_COLOR_PRISM) {
            uint8_t pr = (ca == NGP_COLOR_PRISM) ? a : b, other = (pr == a) ? b : a;
            mark_color(p, m, p->color[other]);
            bit_set(m, pr); bit_set(m, other); bit_set(fired, pr);
            emit(p, NGP_EV_SPECIAL_FIRED, pr, NGP_KIND_PRISM, 0, 0);
            fired_n++; any = 1;
        } else if (is_special(p->kind[a]) && is_special(p->kind[b])) {
            bit_set(m, a); bit_set(m, b); any = 1;
        }
    }
    p->swap_a = p->swap_b = NGP_NO_CELL;
    if (!any) return 0;

    if (p->cascade >= 2u) emit(p, NGP_EV_CASCADE, NGP_NO_CELL, p->cascade, 0, 0);
    for (i = 0; i < nruns; i++)
        emit(p, NGP_EV_MATCH, runs[i].start, runs[i].len, runs[i].dir, runs[i].color);

    /* specials to make: crossings make bombs, then runs of 5 prisms, of 4 lines */
    zero(used, sizeof(used));
    if (!(p->level->flags & NGP_LF_NO_SPECIALS)) {
        for (i = 0; i < nruns && nmade < MAX_RUNS; i++) {
            uint8_t k;
            if (runs[i].dir != 0 || used[i]) continue;
            for (k = 0; k < runs[i].len; k++) {
                uint8_t cc = (uint8_t)(runs[i].start + k), j;
                if (!bit_get(vmark, cc)) continue;
                for (j = 0; j < nruns; j++) {
                    uint8_t t, hit = 0;
                    if (runs[j].dir != 1 || used[j]) continue;
                    for (t = 0; t < runs[j].len; t++) if ((uint8_t)(runs[j].start + t * p->w) == cc) hit = 1;
                    if (!hit) continue;
                    used[i] = used[j] = 1;
                    if (runs[i].len >= 5 || runs[j].len >= 5) break;  /* leave it to the prism rule */
                    made_cell[nmade] = cc; made_kind[nmade] = NGP_KIND_BOMB; made_color[nmade] = runs[i].color;
                    nmade++;
                    break;
                }
                if (used[i]) break;
            }
        }
        for (i = 0; i < nruns && nmade < MAX_RUNS; i++) {
            uint8_t k, at, step;
            if (runs[i].len < 4) continue;
            if (used[i] && runs[i].len < 5) continue;
            step = runs[i].dir ? p->w : 1u;
            at = (uint8_t)(runs[i].start + step);
            for (k = 0; k < runs[i].len; k++) {
                uint8_t cc = (uint8_t)(runs[i].start + k * step);
                if (cc == sa || cc == sb) { at = cc; break; }
            }
            made_cell[nmade] = at;
            if (runs[i].len >= 5) { made_kind[nmade] = NGP_KIND_PRISM; made_color[nmade] = NGP_COLOR_PRISM; }
            else { made_kind[nmade] = runs[i].dir ? NGP_KIND_LINE_H : NGP_KIND_LINE_V; made_color[nmade] = runs[i].color; }
            nmade++;
        }
    }

    /* specials caught in the clear go off, and what they hit may go off too */
    {
        uint8_t again = 1, guard = 0;
        while (again && guard++ < 64u) {
            again = 0;
            for (c = 0; c < n; c++) {
                uint8_t k;
                if (!bit_get(m, c) || bit_get(fired, c) || !p->color[c]) continue;
                k = p->kind[c];
                if (p->color[c] == NGP_COLOR_PRISM) {
                    bit_set(fired, c);
                    mark_color(p, m, common_color(p));
                    emit(p, NGP_EV_SPECIAL_FIRED, c, NGP_KIND_PRISM, 0, 0);
                    fired_n++; again = 1;
                } else if (is_special(k)) {
                    bit_set(fired, c);
                    mark_area(p, m, c, k);
                    emit(p, NGP_EV_SPECIAL_FIRED, c, k, 0, 0);
                    fired_n++; again = 1;
                }
            }
        }
    }

    /* cells that become new specials keep their piece */
    for (i = 0; i < nmade; i++) m[made_cell[i] >> 3] = (uint8_t)(m[made_cell[i] >> 3] & ~(1u << (made_cell[i] & 7u)));

    for (c = 0; c < n; c++) {
        if (!bit_get(m, c)) continue;
        if (p->cover[c] & NGP_COVER_WALL) continue;
        if (p->cover[c] & NGP_COVER_CRATE) { if (!bit_get(hitc, c)) { bit_set(hitc, c); hit_crate(p, c); } continue; }
        if (!p->color[c]) continue;
        if (p->cover[c] & NGP_COVER_LOCK) {
            p->cover[c] = (uint8_t)(p->cover[c] & (uint8_t)~NGP_COVER_LOCK);
            touch(p, c);
            emit(p, NGP_EV_UNLOCK, c, 0, 0, 0);
            continue;
        }
        if (p->color[c] <= NGP_MAX_COLORS) goal_progress(p, NGP_OBJ_COLOR, p->color[c], 1);
        if (NGP_ICE(p->cover[c])) {
            uint8_t ice = (uint8_t)(NGP_ICE(p->cover[c]) - 1u);
            p->cover[c] = (uint8_t)((p->cover[c] & (uint8_t)~NGP_COVER_ICE) | ice);
            emit(p, NGP_EV_ICE_HIT, c, ice, 0, 0);
            goal_progress(p, NGP_OBJ_ICE, 0, 1);
        }
        set_piece(p, c, 0, NGP_KIND_PLAIN);
        bit_set(p->cleared, c);
        cleared++;
    }
    /* crates next to a cleared cell take a hit (once per phase) */
    for (c = 0; c < n; c++) {
        uint8_t d;
        if (!bit_get(p->cleared, c)) continue;
        for (d = 0; d < 4; d++) {
            uint8_t nb = ng_puzzle_neighbor(p, c, d);
            if (nb == NGP_NO_CELL || !(p->cover[nb] & NGP_COVER_CRATE) || bit_get(hitc, nb)) continue;
            bit_set(hitc, nb);
            hit_crate(p, nb);
        }
    }
    for (i = 0; i < nmade; i++) {
        set_piece(p, made_cell[i], made_color[i], made_kind[i]);
        p->specials_made = (uint8_t)(p->specials_made < 255u ? p->specials_made + 1u : 255u);
        emit(p, NGP_EV_SPECIAL_MADE, made_cell[i], made_kind[i], 0, 0);
        goal_progress(p, NGP_OBJ_SPECIAL, 0, 1);
    }
    {
        uint32_t pts = (uint32_t)cleared * 10u * p->cascade + (uint32_t)fired_n * 30u + (uint32_t)nmade * 50u;
        add_score(p, pts);
        emit(p, NGP_EV_CLEARED, NGP_NO_CELL, (uint8_t)(cleared > 255u ? 255u : cleared), 0,
             (uint16_t)(pts > 0xFFFFu ? 0xFFFFu : pts));
    }
    return (uint8_t)(cleared + nmade + 1u);
}

static uint8_t NEOGEO_USER blocks_fall(const NGPuzzle *p, uint8_t c)
{
    return (uint8_t)((p->cover[c] & (NGP_COVER_WALL | NGP_COVER_CRATE | NGP_COVER_LOCK)) != 0);
}

/* Gravity and refill, straight down: walls, crates and locked pieces split a
 * column, and only a part open to the top refills. */
static void NEOGEO_USER fall_straight(NGPuzzle *p, uint8_t *fell, uint8_t *added)
{
    uint8_t x;
    for (x = 0; x < p->w; x++) {
        int16_t bottom = (int16_t)p->h - 1;
        while (bottom >= 0) {
            int16_t top = bottom, r, wr;
            uint8_t open;
            while (bottom >= 0 && blocks_fall(p, (uint8_t)((uint8_t)bottom * p->w + x))) bottom--;
            if (bottom < 0) break;
            top = bottom;
            while (top - 1 >= 0 && !blocks_fall(p, (uint8_t)((uint8_t)(top - 1) * p->w + x))) top--;
            open = (uint8_t)(top == 0);
            wr = bottom;
            for (r = bottom; r >= top; r--) {
                uint8_t src = (uint8_t)((uint8_t)r * p->w + x);
                if (!p->color[src]) continue;
                if (r != wr) {
                    uint8_t dst = (uint8_t)((uint8_t)wr * p->w + x);
                    set_piece(p, dst, p->color[src], p->kind[src]);
                    set_piece(p, src, 0, NGP_KIND_PLAIN);
                    p->fall[dst] = (uint8_t)(p->fall[src] + (wr - r));
                    p->fall[src] = 0;
                    (*fell)++;
                }
                wr--;
            }
            if (open && !(p->level->flags & NGP_LF_NO_REFILL)) {
                uint8_t empties = (uint8_t)(wr - top + 1);
                for (r = wr; r >= top; r--) {
                    uint8_t dst = (uint8_t)((uint8_t)r * p->w + x);
                    set_piece(p, dst, (uint8_t)(1u + rng_range(p, p->colors)), NGP_KIND_PLAIN);
                    p->fall[dst] = empties;
                    (*added)++;
                }
            }
            bottom = top - 1;
        }
    }
}

/* A hole that nothing above can reach (under a crate, a wall or a lock)
 * takes a piece sliding in from the row above, left first then right.
 * One slide per call, lowest hole first; 1 if a piece moved. */
static uint8_t NEOGEO_USER fall_slide(NGPuzzle *p)
{
    int16_t r;
    uint8_t x, k;
    for (r = (int16_t)p->h - 1; r >= 1; r--) {
        for (x = 0; x < p->w; x++) {
            uint8_t dst = (uint8_t)((uint8_t)r * p->w + x), up = (uint8_t)(dst - p->w);
            if (p->color[dst] || blocks_fall(p, dst)) continue;
            if (p->color[up] && !blocks_fall(p, up)) continue;     /* it will fall straight */
            for (k = 0; k < 2u; k++) {
                uint8_t sx = (uint8_t)(k ? x + 1u : x - 1u), src;
                if (k ? x + 1u >= p->w : !x) continue;
                src = (uint8_t)(up - x + sx);
                if (!p->color[src] || blocks_fall(p, src)) continue;
                set_piece(p, dst, p->color[src], p->kind[src]);
                set_piece(p, src, 0, NGP_KIND_PLAIN);
                p->fall[dst] = (uint8_t)(p->fall[src] + 1u);
                p->fall[src] = 0;
                return 1;
            }
        }
    }
    return 0;
}

static void NEOGEO_USER fall_phase(NGPuzzle *p)
{
    uint8_t fell = 0, added = 0, guard = NGP_MAX_CELLS;
    zero(p->fall, NGP_MAX_CELLS);
    fall_straight(p, &fell, &added);
    if (!(p->level->flags & NGP_LF_NO_REFILL))
        while (guard-- && fall_slide(p)) { fell++; fall_straight(p, &fell, &added); }
    emit(p, NGP_EV_FALL, NGP_NO_CELL, fell, added, 0);
}

/* ======================================================================
 * bubble lattice
 * ==================================================================== */
static uint8_t NEOGEO_USER valid_cell(const NGPuzzle *p, int16_t r, int16_t c)
{
    if (r < 0 || r >= (int16_t)p->h || c < 0) return 0;
    return (uint8_t)(c < (int16_t)ng_puzzle_row_cells(p, (uint8_t)r));
}

uint8_t NEOGEO_USER ng_puzzle_cell(const NGPuzzle *p, uint8_t x, uint8_t y)
{
    if (!p || x >= p->w || y >= p->h) return NGP_NO_CELL;
    if (p->mode == NGP_MODE_BUBBLE && !valid_cell(p, y, x)) return NGP_NO_CELL;
    return (uint8_t)(y * p->w + x);
}

uint8_t NEOGEO_USER ng_puzzle_neighbor(const NGPuzzle *p, uint8_t cell, uint8_t dir)
{
    int16_t r, c;
    if (!p || cell >= (uint8_t)(p->w * p->h)) return NGP_NO_CELL;
    r = cell_y(p, cell); c = cell_x(p, cell);
    if (p->mode != NGP_MODE_BUBBLE) {
        switch (dir) {
        case NGP_DIR_RIGHT: c++; break;
        case NGP_DIR_DOWN: r++; break;
        case NGP_DIR_LEFT: c--; break;
        case NGP_DIR_UP: r--; break;
        default: return NGP_NO_CELL;
        }
        if (r < 0 || c < 0 || r >= (int16_t)p->h || c >= (int16_t)p->w) return NGP_NO_CELL;
        return (uint8_t)((uint8_t)r * p->w + (uint8_t)c);
    }
    if (!valid_cell(p, r, c)) return NGP_NO_CELL;   /* a shifted row's unused last column */
    {
        uint8_t sh = shifted_row(p, (uint8_t)r);
        switch (dir) {
        case NGP_HEX_E: c++; break;
        case NGP_HEX_W: c--; break;
        case NGP_HEX_NE: r--; if (sh) c++; break;
        case NGP_HEX_NW: r--; if (!sh) c--; break;
        case NGP_HEX_SE: r++; if (sh) c++; break;
        case NGP_HEX_SW: r++; if (!sh) c--; break;
        default: return NGP_NO_CELL;
        }
        if (!valid_cell(p, r, c)) return NGP_NO_CELL;
        return (uint8_t)((uint8_t)r * p->w + (uint8_t)c);
    }
}

static void NEOGEO_USER bubble_center(const NGPuzzle *p, uint8_t cell, int16_t *x, int16_t *y)
{
    uint8_t r = cell_y(p, cell), c = cell_x(p, cell);
    uint8_t d = p->level->bubble_d;
    *x = (int16_t)((uint16_t)c * d + (d >> 1) + (shifted_row(p, r) ? (d >> 1) : 0u));
    *y = (int16_t)((uint16_t)r * p->level->row_h + (d >> 1));
}

void NEOGEO_USER ng_puzzle_cell_px(const NGPuzzle *p, uint8_t cell, uint8_t cell_px, int16_t *x, int16_t *y)
{
    int16_t cx = 0, cy = 0;
    if (p && cell < (uint8_t)(p->w * p->h)) {
        if (p->mode == NGP_MODE_BUBBLE) {
            bubble_center(p, cell, &cx, &cy);
            cx = (int16_t)(cx - (p->level->bubble_d >> 1));
            cy = (int16_t)(cy - (p->level->bubble_d >> 1));
        } else {
            cx = (int16_t)((uint16_t)cell_x(p, cell) * cell_px);
            cy = (int16_t)((uint16_t)cell_y(p, cell) * cell_px);
        }
    }
    if (x) *x = cx;
    if (y) *y = cy;
}

static uint8_t NEOGEO_USER occupied(const NGPuzzle *p, uint8_t c)
{
    return (uint8_t)(p->color[c] != 0);
}

/* sin and cos of a 1024-step angle, 1.14, interpolated from ng_trig. */
static int16_t NEOGEO_USER sin1024(uint16_t a)
{
    uint8_t i = (uint8_t)(a >> 2), f = (uint8_t)(a & 3u);
    int16_t s0 = ng_sin(i), s1 = ng_sin((uint8_t)(i + 1u));
    return (int16_t)(s0 + (((int16_t)(s1 - s0) * (int16_t)f) >> 2));
}

static int16_t NEOGEO_USER cos1024(uint16_t a)
{
    return sin1024((uint16_t)((a + 256u) & 1023u));
}

typedef struct {
    int32_t x, y, vx, vy;      /* Q8, per substep */
    uint16_t steps;            /* substeps taken */
} Flight;

static uint16_t NEOGEO_USER aim_lo(const NGPuzzle *p) { return p->level->aim_min ? p->level->aim_min : 544u; }
static uint16_t NEOGEO_USER aim_hi(const NGPuzzle *p) { return p->level->aim_min ? p->level->aim_max : 992u; }

/* Substeps per update: each moves at most a quarter of a bubble. */
static uint8_t NEOGEO_USER substeps(const NGPuzzle *p)
{
    uint16_t quarter = (uint16_t)((uint16_t)p->level->bubble_d << 6);  /* d/4 in Q8 */
    uint16_t n = ngp_div((uint16_t)(p->level->shot_speed_q8 + quarter - 1u), quarter);
    return (uint8_t)(n ? (n > 16u ? 16u : n) : 1u);
}

static void NEOGEO_USER flight_start(const NGPuzzle *p, uint16_t angle, Flight *f)
{
    uint8_t n = substeps(p);
    int16_t sp = (int16_t)ngp_div(p->level->shot_speed_q8, n);   /* < 32768: one muls.w each below */
    f->x = (int32_t)p->level->launch_x << 8;
    f->y = (int32_t)p->level->launch_y << 8;
    f->vx = ((int32_t)sp * (int32_t)cos1024(angle)) >> NG_TRIG_SHIFT;
    f->vy = ((int32_t)sp * (int32_t)sin1024(angle)) >> NG_TRIG_SHIFT;
    if (f->vy > -16) f->vy = -16;   /* always climbing */
    f->steps = 0;
}

/* Squared distance from (x, y) to a cell's centre, pixels. */
static int32_t NEOGEO_USER dist2(const NGPuzzle *p, uint8_t cell, int16_t x, int16_t y)
{
    int16_t cx, cy, dx, dy;
    bubble_center(p, cell, &cx, &cy);
    dx = (int16_t)(x - cx); dy = (int16_t)(y - cy);
    return (int32_t)dx * dx + (int32_t)dy * dy;
}

static int16_t NEOGEO_USER row_guess(const NGPuzzle *p, int16_t y)
{
    int16_t v = (int16_t)(y - (int16_t)(p->level->bubble_d >> 1));
    if (v < 0) return 0;
    return (int16_t)ngp_div((uint16_t)(v + (p->level->row_h >> 1)), p->level->row_h);
}

static int16_t NEOGEO_USER col_guess(const NGPuzzle *p, int16_t r, int16_t x)
{
    int16_t v = (int16_t)(x - (shifted_row(p, (uint8_t)r) ? (int16_t)(p->level->bubble_d >> 1) : 0));
    if (v < 0) return 0;
    return (int16_t)ngp_div((uint16_t)v, p->level->bubble_d);
}

static uint8_t NEOGEO_USER attachable(const NGPuzzle *p, uint8_t cell)
{
    uint8_t d;
    if (occupied(p, cell)) return 0;
    if (cell_y(p, cell) == 0) return 1;
    for (d = 0; d < 6; d++) {
        uint8_t nb = ng_puzzle_neighbor(p, cell, d);
        if (nb != NGP_NO_CELL && occupied(p, nb)) return 1;
    }
    return 0;
}

/* The free cell a bubble at (x, y) settles in: the nearest attachable one
 * around it, searched wider if need be. NGP_NO_CELL if there is none. */
static uint8_t NEOGEO_USER attach_cell(const NGPuzzle *p, int16_t x, int16_t y)
{
    int16_t r0 = row_guess(p, y), r, c, span;
    uint8_t best = NGP_NO_CELL;
    int32_t bd = 0x7FFFFFFF;
    for (span = 1; span <= 3 && best == NGP_NO_CELL; span++) {
        for (r = (int16_t)(r0 - span); r <= r0 + span; r++) {
            int16_t c0;
            if (r < 0 || r >= (int16_t)p->h) continue;
            c0 = col_guess(p, r, x);
            for (c = (int16_t)(c0 - span); c <= c0 + span; c++) {
                uint8_t cell;
                int32_t d;
                if (!valid_cell(p, r, c)) continue;
                cell = (uint8_t)((uint8_t)r * p->w + (uint8_t)c);
                if (!attachable(p, cell)) continue;
                d = dist2(p, cell, x, y);
                if (d < bd) { bd = d; best = cell; }
            }
        }
    }
    return best;
}

/* One substep. Returns 0 still flying, 1 stuck (cell set), 2 lost.
 * *bounce gets 1 or 2 for a rebound off the left or right wall. */
static uint8_t NEOGEO_USER flight_step(const NGPuzzle *p, Flight *f, uint8_t *cell, uint8_t *bounce, uint8_t *hit)
{
    int32_t rad = (int32_t)(p->level->bubble_d >> 1) << 8;
    int32_t wide = (int32_t)((uint16_t)p->w * (uint16_t)p->level->bubble_d) << 8;   /* mulu.w */
    int16_t x, y, r0, r, c;
    int32_t hit2 = (int32_t)((uint16_t)(p->level->bubble_d - 2u) * (uint16_t)(p->level->bubble_d - 2u));
    *bounce = 0;
    if (hit) *hit = NGP_NO_CELL;
    f->x += f->vx; f->y += f->vy; f->steps++;
    if (f->x < rad) { f->x = 2 * rad - f->x; f->vx = -f->vx; *bounce = 1; }
    else if (f->x > wide - rad) { f->x = 2 * (wide - rad) - f->x; f->vx = -f->vx; *bounce = 2; }
    x = (int16_t)(f->x >> 8); y = (int16_t)(f->y >> 8);
    if (f->y <= rad) {
        *cell = attach_cell(p, x, (int16_t)(p->level->bubble_d >> 1));
        return (uint8_t)(*cell == NGP_NO_CELL ? 2u : 1u);
    }
    r0 = row_guess(p, y);
    for (r = (int16_t)(r0 - 1); r <= r0 + 1; r++) {
        int16_t c0;
        if (r < 0 || r >= (int16_t)p->h) continue;
        c0 = col_guess(p, r, x);
        for (c = (int16_t)(c0 - 1); c <= c0 + 1; c++) {
            uint8_t cc;
            if (!valid_cell(p, r, c)) continue;
            cc = (uint8_t)((uint8_t)r * p->w + (uint8_t)c);
            if (!occupied(p, cc)) continue;
            if (dist2(p, cc, x, y) < hit2) {
                if (hit) *hit = cc;
                *cell = attach_cell(p, x, y);
                return (uint8_t)(*cell == NGP_NO_CELL ? 2u : 1u);
            }
        }
    }
    if (f->steps > FLIGHT_LIMIT) return 2;
    return 0;
}

static uint8_t NEOGEO_USER bubble_trace(const NGPuzzle *p, uint16_t angle, NGPuzzlePoint *pts,
                                       uint8_t max, uint8_t *cell, uint8_t *hit)
{
    Flight f;
    uint8_t n = 0, res = 0, bounce, at = NGP_NO_CELL;
    if (cell) *cell = NGP_NO_CELL;
    if (hit) *hit = NGP_NO_CELL;
    if (!p || !p->level || p->mode != NGP_MODE_BUBBLE) return 0;
    if (angle < aim_lo(p)) angle = aim_lo(p);
    if (angle > aim_hi(p)) angle = aim_hi(p);
    flight_start(p, angle, &f);
    if (pts && n < max) { pts[n].x = (int16_t)(f.x >> 8); pts[n].y = (int16_t)(f.y >> 8); n++; }
    while (!res) {
        res = flight_step(p, &f, &at, &bounce, hit);
        if (bounce && pts && n < max) { pts[n].x = (int16_t)(f.x >> 8); pts[n].y = (int16_t)(f.y >> 8); n++; }
    }
    if (res == 1) {
        int16_t cx, cy;
        bubble_center(p, at, &cx, &cy);
        if (pts && n < max) { pts[n].x = cx; pts[n].y = cy; n++; }
        if (cell) *cell = at;
    }
    return n;
}

uint8_t NEOGEO_USER ng_puzzle_bubble_preview(const NGPuzzle *p, uint16_t angle, NGPuzzlePoint *pts,
                                             uint8_t max, uint8_t *cell)
{
    return bubble_trace(p, angle, pts, max, cell, 0);
}

uint8_t NEOGEO_USER ng_puzzle_bubble_target(const NGPuzzle *p, uint16_t angle)
{
    uint8_t hit;
    bubble_trace(p, angle, 0, 0, 0, &hit);
    return hit;
}

uint8_t NEOGEO_USER ng_puzzle_preview_begin(NGPuzzlePreview *q, const NGPuzzle *p,
                                            uint16_t angle, uint32_t generation)
{
    Flight f;
    if (!q) return NGP_PREVIEW_INVALID;
    q->source = 0; q->level = 0;
    q->state = NGP_PREVIEW_INVALID; q->count = 0;
    q->cell = q->hit = NGP_NO_CELL;
    if (!p || !p->level || p->mode != NGP_MODE_BUBBLE) return q->state;
    q->requested_angle = angle;
    if (angle < aim_lo(p)) angle = aim_lo(p);
    if (angle > aim_hi(p)) angle = aim_hi(p);
    flight_start(p, angle, &f);
    q->source = p; q->level = p->level; q->generation = generation;
    q->parity = p->parity; q->angle = angle; q->steps = 0;
    q->x_q8 = f.x; q->y_q8 = f.y; q->vx_q8 = f.vx; q->vy_q8 = f.vy;
    q->points[0].x = (int16_t)(f.x >> 8); q->points[0].y = (int16_t)(f.y >> 8);
    q->count = 1; q->state = NGP_PREVIEW_BUSY;
    return q->state;
}

uint8_t NEOGEO_USER ng_puzzle_preview_step(NGPuzzlePreview *q, const NGPuzzle *p,
                                           uint32_t generation, uint8_t budget)
{
    Flight f;
    uint8_t i, res = 0, cell = NGP_NO_CELL, hit = NGP_NO_CELL, bounce;
    if (!q) return NGP_PREVIEW_INVALID;
    if (q->state != NGP_PREVIEW_BUSY && q->state != NGP_PREVIEW_READY) return q->state;
    if (!p || p != q->source || generation != q->generation || p->level != q->level ||
        p->mode != NGP_MODE_BUBBLE || p->parity != q->parity) {
        q->state = NGP_PREVIEW_CANCELLED; q->count = 0;
        q->cell = q->hit = NGP_NO_CELL;
        return q->state;
    }
    if (q->state == NGP_PREVIEW_READY || !budget) return q->state;
    if (budget > NGP_PREVIEW_MAX_STEPS) budget = NGP_PREVIEW_MAX_STEPS;
    f.x = q->x_q8; f.y = q->y_q8; f.vx = q->vx_q8; f.vy = q->vy_q8; f.steps = q->steps;
    for (i = 0; i < budget && !res; i++) {
        res = flight_step(p, &f, &cell, &bounce, &hit);
        if (bounce && q->count < NGP_PREVIEW_POINTS) {
            q->points[q->count].x = (int16_t)(f.x >> 8);
            q->points[q->count].y = (int16_t)(f.y >> 8); q->count++;
        }
    }
    q->x_q8 = f.x; q->y_q8 = f.y; q->vx_q8 = f.vx; q->vy_q8 = f.vy; q->steps = f.steps;
    if (res) {
        q->state = NGP_PREVIEW_READY; q->hit = hit;
        if (res == 1) {
            q->cell = cell;
            if (q->count < NGP_PREVIEW_POINTS) {
                bubble_center(p, cell, &q->points[q->count].x, &q->points[q->count].y);
                q->count++;
            }
        }
    }
    return q->state;
}

/* Same-colour group through cell (its colour col), into p->queue-like
 * storage q with visit marks v. Returns the group size. */
static uint8_t NEOGEO_USER group_of(const NGPuzzle *p, uint8_t start, uint8_t col, uint8_t *q, uint8_t *v)
{
    uint8_t head = 0, tail = 0, d;
    zero(v, NGP_BITS_BYTES);
    q[tail++] = start; bit_set(v, start);
    while (head < tail) {
        uint8_t c = q[head++];
        for (d = 0; d < 6; d++) {
            uint8_t nb = ng_puzzle_neighbor(p, c, d);
            if (nb == NGP_NO_CELL || bit_get(v, nb)) continue;
            if (p->color[nb] != col || p->kind[nb] == NGP_KIND_ANCHOR) continue;
            bit_set(v, nb);
            q[tail++] = nb;
        }
    }
    return tail;
}

uint8_t NEOGEO_USER ng_puzzle_bubble_hint(const NGPuzzle *p, uint16_t *angle)
{
    uint8_t q[NGP_MAX_CELLS], v[NGP_BITS_BYTES], best = 0;
    uint16_t a, lo, hi, ba = 768u, bdist = 0xFFFFu;
    if (!p || p->mode != NGP_MODE_BUBBLE || !p->shot_color) return 0;
    lo = aim_lo(p); hi = aim_hi(p);
    for (a = lo; a <= hi; a = (uint16_t)(a + 12u)) {
        uint8_t cell, size, d, same = 0;
        ng_puzzle_bubble_preview(p, a, 0, 0, &cell);
        if (cell == NGP_NO_CELL) continue;
        /* the group the shot would join: its same-coloured neighbours' groups */
        size = 1;
        zero(v, NGP_BITS_BYTES);
        for (d = 0; d < 6; d++) {
            uint8_t nb = ng_puzzle_neighbor(p, cell, d);
            if (nb == NGP_NO_CELL || p->color[nb] != p->shot_color || p->kind[nb] == NGP_KIND_ANCHOR) continue;
            if (bit_get(v, nb)) continue;
            {
                uint8_t vv[NGP_BITS_BYTES], k, g = group_of(p, nb, p->shot_color, q, vv);
                for (k = 0; k < NGP_BITS_BYTES; k++) v[k] = (uint8_t)(v[k] | vv[k]);
                size = (uint8_t)(size + g);
                same = 1;
            }
        }
        if (!same) continue;
        /* a pop beats any pair; among equals, the straighter shot */
        if (size > best || (size == best && (uint16_t)(a > 768u ? a - 768u : 768u - a) < bdist)) {
            best = size; ba = a; bdist = (uint16_t)(a > 768u ? a - 768u : 768u - a);
        }
    }
    if (!best) return 0;
    if (angle) *angle = ba;
    return (uint8_t)(best >= 3u ? 2u : 1u);
}

static uint8_t NEOGEO_USER present_colors(const NGPuzzle *p)
{
    uint8_t c, n = (uint8_t)(p->w * p->h), mask = 0;
    for (c = 0; c < n; c++) {
        uint8_t col = p->color[c];
        if (col >= 1 && col <= NGP_MAX_COLORS && p->kind[c] != NGP_KIND_ANCHOR) mask = (uint8_t)(mask | (1u << (col - 1u)));
    }
    return mask;
}

static uint8_t NEOGEO_USER pick_bubble(NGPuzzle *p)
{
    uint8_t mask = present_colors(p), cnt = 0, i, k;
    if (!mask) return (uint8_t)(1u + rng_range(p, p->colors));
    for (i = 0; i < 8; i++) if (mask & (1u << i)) cnt++;
    k = (uint8_t)rng_range(p, cnt);
    for (i = 0; i < 8; i++) {
        if (!(mask & (1u << i))) continue;
        if (!k) return (uint8_t)(i + 1u);
        k--;
    }
    return 1;
}

/* Push a new row in at the top; everything moves down one row. */
static void NEOGEO_USER push_row(NGPuzzle *p)
{
    uint8_t n = (uint8_t)(p->w * p->h), c, mask = present_colors(p);
    ngp_copy(&p->color[p->w], &p->color[0], (uint16_t)(n - p->w));
    ngp_copy(&p->kind[p->w], &p->kind[0], (uint16_t)(n - p->w));
    p->parity ^= 1u;
    for (c = 0; c < p->w; c++) {
        uint8_t col = 0;
        if (valid_cell(p, 0, c)) {
            if (mask) {
                do { col = (uint8_t)(1u + rng_range(p, p->colors)); } while (!(mask & (1u << (col - 1u))));
            } else {
                col = (uint8_t)(1u + rng_range(p, p->colors));
            }
        }
        p->color[c] = col;
        p->kind[c] = NGP_KIND_PLAIN;
    }
    /* a cell that left the lattice when the rows' shift changed holds nothing */
    for (c = 0; c < n; c++) {
        if (!valid_cell(p, cell_y(p, c), cell_x(p, c))) { p->color[c] = 0; p->kind[c] = NGP_KIND_PLAIN; }
        touch(p, c);
    }
    emit(p, NGP_EV_NEW_ROW, NGP_NO_CELL, 0, 0, 0);
}

static uint16_t NEOGEO_USER rescued_count(NGPuzzle *p, uint8_t cell)
{
    if (p->kind[cell] != NGP_KIND_RESCUE) return 0;
    emit(p, NGP_EV_RESCUE, cell, 0, 0, 0);
    goal_progress(p, NGP_OBJ_RESCUE, 0, 1);
    return 1;
}

static void NEOGEO_USER pop_phase(NGPuzzle *p)
{
    uint8_t n = (uint8_t)(p->w * p->h), c, size = 0, head = 0, tail = 0, d, dropped = 0;
    uint8_t cell = p->attach_cell;
    if (cell != NGP_NO_CELL && p->color[cell] >= 1 && p->color[cell] <= NGP_MAX_COLORS) {
        size = group_of(p, cell, p->color[cell], p->queue, p->mark);
    }
    if (size >= 3) {
        uint8_t i;
        for (i = 0; i < size; i++) {
            uint8_t cc = p->queue[i];
            rescued_count(p, cc);
            goal_progress(p, NGP_OBJ_COLOR, p->color[cc], 1);
            set_piece(p, cc, 0, NGP_KIND_PLAIN);
        }
        add_score(p, (uint32_t)size * 10u);
        emit(p, NGP_EV_POP, NGP_NO_CELL, size, 0, (uint16_t)(size * 10u));
        p->misses = 0;
    } else {
        p->misses++;
        emit(p, NGP_EV_MISS, NGP_NO_CELL, p->misses, 0, 0);
    }
    /* what still hangs from the ceiling or an anchor */
    zero(p->mark, NGP_BITS_BYTES);
    for (c = 0; c < n; c++) {
        if (!occupied(p, c)) continue;
        if (cell_y(p, c) == 0 || p->kind[c] == NGP_KIND_ANCHOR) { bit_set(p->mark, c); p->queue[tail++] = c; }
    }
    while (head < tail) {
        uint8_t cc = p->queue[head++];
        for (d = 0; d < 6; d++) {
            uint8_t nb = ng_puzzle_neighbor(p, cc, d);
            if (nb == NGP_NO_CELL || bit_get(p->mark, nb) || !occupied(p, nb)) continue;
            bit_set(p->mark, nb);
            p->queue[tail++] = nb;
        }
    }
    for (c = 0; c < n; c++) {
        if (!occupied(p, c) || bit_get(p->mark, c)) continue;
        rescued_count(p, c);
        if (p->color[c] <= NGP_MAX_COLORS) goal_progress(p, NGP_OBJ_COLOR, p->color[c], 1);
        set_piece(p, c, 0, NGP_KIND_PLAIN);
        dropped++;
    }
    if (dropped) {
        uint32_t pts = (uint32_t)dropped * 20u * (dropped >= 10u ? 2u : 1u);
        add_score(p, pts);
        emit(p, NGP_EV_DROP, NGP_NO_CELL, dropped, 0, (uint16_t)pts);
    }
    p->shots_since_row++;
    if (p->level->push_every && p->shots_since_row >= p->level->push_every) {
        p->shots_since_row = 0;
        push_row(p);
    } else if (p->level->miss_limit && p->misses >= p->level->miss_limit) {
        p->misses = 0;
        push_row(p);
    }
}

/* ======================================================================
 * picture logic
 * ==================================================================== */
/* The k-th cell of row (column 0) or column (column 1) `index`. */
static uint8_t NEOGEO_USER line_cell(const NGPuzzle *p, uint8_t column, uint8_t index, uint8_t k)
{
    return column ? (uint8_t)(k * p->w + index) : (uint8_t)(index * p->w + k);
}

uint8_t NEOGEO_USER ng_puzzle_logic_clue(const NGPuzzle *p, uint8_t column, uint8_t index,
                                         uint8_t *runs, uint8_t max)
{
    uint8_t k, len, count = 0, run = 0;
    if (!p || p->mode != NGP_MODE_LOGIC || index >= (column ? p->w : p->h)) return 0xFFu;
    len = column ? p->h : p->w;
    for (k = 0; k <= len; k++) {
        if (k < len && p->color[line_cell(p, column, index, k)]) { run++; continue; }
        if (!run) continue;
        if (runs && count < max) runs[count] = run;
        count++;
        run = 0;
    }
    return count;
}

uint8_t NEOGEO_USER ng_puzzle_logic_line_done(const NGPuzzle *p, uint8_t column, uint8_t index)
{
    uint8_t k, len;
    if (!p || p->mode != NGP_MODE_LOGIC || index >= (column ? p->w : p->h)) return 0;
    len = column ? p->h : p->w;
    for (k = 0; k < len; k++) {
        uint8_t c = line_cell(p, column, index, k);
        if (p->color[c] && p->kind[c] != NGP_MARK_FILL) return 0;
    }
    return 1;
}

/* Cells of the picture still to fill. */
static uint16_t NEOGEO_USER logic_left(const NGPuzzle *p)
{
    uint8_t c, n = (uint8_t)(p->w * p->h);
    uint16_t left = 0;
    for (c = 0; c < n; c++) if (p->color[c] && p->kind[c] != NGP_MARK_FILL) left++;
    return left;
}

static void NEOGEO_USER logic_goal(NGPuzzle *p)
{
    uint8_t g;
    uint16_t left = logic_left(p);
    for (g = 0; g < NGP_MAX_OBJ; g++) {
        if (p->level->goals[g].type != NGP_OBJ_PICTURE || p->goal_left[g] == left) continue;
        p->goal_left[g] = left;
        emit(p, NGP_EV_GOAL, NGP_NO_CELL, g, 0, left);
    }
}

/* A line just finished: cross out its other cells and score it. */
static void NEOGEO_USER logic_line(NGPuzzle *p, uint8_t column, uint8_t index)
{
    uint8_t k, len = column ? p->h : p->w;
    for (k = 0; k < len; k++) {
        uint8_t c = line_cell(p, column, index, k);
        if (p->kind[c] == NGP_MARK_NONE) { p->kind[c] = NGP_MARK_CROSS; touch(p, c); }
    }
    add_score(p, LOGIC_LINE);
    emit(p, NGP_EV_LINE, index, column, 0, LOGIC_LINE);
}

/* Cell c was just filled (by the player or the hint): finish its lines. */
static void NEOGEO_USER logic_filled(NGPuzzle *p, uint8_t c)
{
    uint8_t x = cell_x(p, c), y = cell_y(p, c);
    if (ng_puzzle_logic_line_done(p, 0, y)) logic_line(p, 0, y);
    if (ng_puzzle_logic_line_done(p, 1, x)) logic_line(p, 1, x);
}

static uint8_t NEOGEO_USER goals_met(const NGPuzzle *p);
static void NEOGEO_USER finish(NGPuzzle *p, uint8_t won, uint8_t reason);

/* After any change: the picture done wins, with a bonus for hearts and time left. */
static void NEOGEO_USER logic_settle(NGPuzzle *p)
{
    logic_goal(p);
    if (logic_left(p)) return;
    if (p->moves_left != 0xFFFFu) add_score(p, (uint32_t)p->moves_left * LOGIC_HEART);
    if (p->time_left != 0xFFFFu) add_score(p, (uint32_t)ngp_div(p->time_left, 60u) * LOGIC_SECOND);
    if (goals_met(p)) finish(p, 1, NGP_END_OBJECTIVES);
}

static int NEOGEO_USER logic_mark(NGPuzzle *p, uint8_t c, uint8_t mark)
{
    uint8_t old = p->kind[c];
    if (mark == NGP_MARK_FILL) {
        if (old != NGP_MARK_NONE) return NGP_ERR_ILLEGAL;
        if (!p->color[c]) {
            /* a mistake: the cell is crossed out and a heart goes */
            p->kind[c] = NGP_MARK_MISS;
            touch(p, c);
            p->moves_used++;
            if (p->moves_left != 0xFFFFu && p->moves_left) p->moves_left--;
            emit(p, NGP_EV_MARK, c, NGP_MARK_MISS, old, 0);
            emit(p, NGP_EV_MISTAKE, c, p->moves_left == 0xFFFFu ? 0xFFu : (uint8_t)(p->moves_left > 254u ? 254u : p->moves_left), 0, 0);
            if (p->moves_left == 0) finish(p, 0, NGP_END_MISTAKES);
            return NGP_OK;
        }
        p->kind[c] = NGP_MARK_FILL;
        touch(p, c);
        add_score(p, LOGIC_FILL);
        emit(p, NGP_EV_MARK, c, NGP_MARK_FILL, old, LOGIC_FILL);
        logic_filled(p, c);
        logic_settle(p);
        return NGP_OK;
    }
    if (mark == NGP_MARK_CROSS) {
        if (old != NGP_MARK_NONE) return NGP_ERR_ILLEGAL;
    } else if (mark == NGP_MARK_NONE) {
        if (old != NGP_MARK_CROSS) return NGP_ERR_ILLEGAL;
    } else {
        return NGP_ERR_ARG;
    }
    p->kind[c] = mark;
    touch(p, c);
    emit(p, NGP_EV_MARK, c, mark, old, 0);
    return NGP_OK;
}

/* Rows (bits 0..15) and columns (bits 16..27) finished now. */
static uint32_t NEOGEO_USER logic_done_lines(const NGPuzzle *p)
{
    uint32_t done = 0;
    uint8_t k;
    for (k = 0; k < p->h; k++) if (ng_puzzle_logic_line_done(p, 0, k)) done |= (uint32_t)1u << k;
    for (k = 0; k < p->w; k++) if (ng_puzzle_logic_line_done(p, 1, k)) done |= (uint32_t)1u << (16u + k);
    return done;
}

/* The hint: the row and the column through c, filled and crossed as the
 * picture has them (a player's cross on a picture cell is corrected). */
static void NEOGEO_USER logic_reveal(NGPuzzle *p, uint8_t c)
{
    uint8_t x = cell_x(p, c), y = cell_y(p, c), k, pass;
    uint32_t before = logic_done_lines(p), after;
    emit(p, NGP_EV_REVEAL, c, 0, 0, 0);
    for (pass = 0; pass < 2; pass++) {
        uint8_t len = pass ? p->h : p->w;
        for (k = 0; k < len; k++) {
            uint8_t t = line_cell(p, pass, pass ? x : y, k);
            if (p->color[t]) {
                if (p->kind[t] == NGP_MARK_FILL) continue;
                p->kind[t] = NGP_MARK_FILL;
            } else {
                if (p->kind[t] != NGP_MARK_NONE) continue;
                p->kind[t] = NGP_MARK_CROSS;
            }
            touch(p, t);
        }
    }
    after = logic_done_lines(p) & ~before;
    for (k = 0; k < p->h; k++) if ((after >> k) & 1u) logic_line(p, 0, k);
    for (k = 0; k < p->w; k++) if ((after >> (16u + k)) & 1u) logic_line(p, 1, k);
    logic_settle(p);
}

/* ======================================================================
 * state machine
 * ==================================================================== */
static uint8_t NEOGEO_USER goals_met(const NGPuzzle *p)
{
    uint8_t g, any = 0;
    if (p->mode == NGP_MODE_LOGIC && logic_left(p)) return 0;
    for (g = 0; g < NGP_MAX_OBJ; g++) {
        const NGPuzzleGoal *o = &p->level->goals[g];
        if (o->type == NGP_OBJ_NONE) continue;
        any = 1;
        if (o->type == NGP_OBJ_SCORE) { if (p->score < (uint32_t)o->count * 100u) return 0; continue; }
        if (o->type == NGP_OBJ_CLEAR) { if (present_colors(p)) return 0; continue; }
        if (p->goal_left[g]) return 0;
    }
    if (!any && p->mode == NGP_MODE_BUBBLE) return (uint8_t)!present_colors(p);
    if (p->mode == NGP_MODE_LOGIC) return 1;    /* the picture is done */
    return any;
}

static void NEOGEO_USER finish(NGPuzzle *p, uint8_t won, uint8_t reason)
{
    uint8_t s = 0, i;
    p->end_reason = reason;
    if (won) {
        for (i = 0; i < 3; i++) if (p->level->stars[i] && p->score >= p->level->stars[i]) s = (uint8_t)(i + 1u);
        if (!s) s = 1;
        p->stars = s;
        p->state = ST_WON;
        emit(p, NGP_EV_WON, NGP_NO_CELL, reason, s, 0);
    } else {
        p->state = ST_LOST;
        emit(p, NGP_EV_LOST, NGP_NO_CELL, reason, 0, 0);
    }
}

static void NEOGEO_USER settle_match(NGPuzzle *p)
{
    if (goals_met(p)) { finish(p, 1, NGP_END_OBJECTIVES); return; }
    if (p->moves_left == 0) { finish(p, 0, NGP_END_MOVES); return; }
    if (!ng_puzzle_has_moves(p)) {
        if ((p->level->flags & NGP_LF_NO_SHUFFLE) || !shuffle_board(p)) { finish(p, 0, NGP_END_NO_MOVES); return; }
        p->shuffles++;
        emit(p, NGP_EV_SHUFFLE, NGP_NO_CELL, 0, 0, 0);
    }
    p->state = ST_IDLE;
}

static void NEOGEO_USER settle_bubble(NGPuzzle *p)
{
    uint8_t c, n = (uint8_t)(p->w * p->h);
    if (goals_met(p)) { finish(p, 1, present_colors(p) ? NGP_END_OBJECTIVES : NGP_END_BOARD_CLEAR); return; }
    for (c = 0; c < n; c++)
        if (occupied(p, c) && p->kind[c] != NGP_KIND_ANCHOR && cell_y(p, c) >= p->level->loss_row) {
            finish(p, 0, NGP_END_LOSS_LINE);
            return;
        }
    if (p->shots_left == 0) { finish(p, 0, NGP_END_SHOTS); return; }
    p->shot_color = p->next_color;
    p->shot_kind = NGP_KIND_PLAIN;
    if (p->shot_color && !(present_colors(p) & (1u << (p->shot_color - 1u))) && present_colors(p))
        p->shot_color = pick_bubble(p);   /* never load a colour that is gone */
    p->next_color = pick_bubble(p);
    p->state = ST_IDLE;
}

void NEOGEO_USER ng_puzzle_update(NGPuzzle *p)
{
    if (!p || !p->level) return;
    p->tick++;
    switch (p->state) {
    case ST_IDLE:
        if (p->time_left != 0xFFFFu && p->time_left) {
            p->time_left--;
            if (p->time_left == TIME_LOW) emit(p, NGP_EV_TIME_LOW, NGP_NO_CELL, 0, 0, 0);
            if (!p->time_left) {
                if (goals_met(p)) finish(p, 1, NGP_END_OBJECTIVES);
                else finish(p, 0, NGP_END_TIME);
            }
        }
        break;
    case ST_MATCH:
        if (!match_phase(p) || p->cascade > MAX_CASCADE) {
            p->state = ST_SETTLE;
            settle_match(p);
        } else {
            p->state = ST_FALL;
        }
        break;
    case ST_FALL:
        fall_phase(p);
        p->cascade++;
        p->state = ST_MATCH;
        break;
    case ST_SETTLE:
        settle_match(p);
        break;
    case ST_FLIGHT: {
        Flight f;
        uint8_t i, n = substeps(p), res = 0, cell = NGP_NO_CELL, bounce;
        f.x = p->shot_x_q8; f.y = p->shot_y_q8; f.vx = p->shot_vx_q8; f.vy = p->shot_vy_q8; f.steps = 0;
        for (i = 0; i < n && !res; i++) {
            res = flight_step(p, &f, &cell, &bounce, 0);
            if (bounce) emit(p, NGP_EV_BOUNCE, NGP_NO_CELL, (uint8_t)(bounce - 1u), 0, 0);
        }
        p->shot_x_q8 = f.x; p->shot_y_q8 = f.y; p->shot_vx_q8 = f.vx; p->shot_vy_q8 = f.vy;
        if (res == 1) {
            set_piece(p, cell, p->shot_color, p->shot_kind == NGP_KIND_RESCUE ? NGP_KIND_PLAIN : p->shot_kind);
            p->attach_cell = cell;
            emit(p, NGP_EV_ATTACH, cell, p->shot_color, 0, 0);
            p->state = ST_POP;
        } else if (res == 2) {
            p->attach_cell = NGP_NO_CELL;
            emit(p, NGP_EV_NO_ATTACH, NGP_NO_CELL, 0, 0, 0);
            p->misses++;
            p->state = ST_POP;
        }
        break;
    }
    case ST_POP:
        pop_phase(p);
        settle_bubble(p);
        break;
    default:
        break;
    }
}

void NEOGEO_USER ng_puzzle_settle(NGPuzzle *p)
{
    uint16_t guard = 0;
    if (!p) return;
    while (p->state != ST_IDLE && p->state != ST_WON && p->state != ST_LOST && guard++ < SETTLE_LIMIT)
        ng_puzzle_update(p);
}

uint8_t NEOGEO_USER ng_puzzle_status(const NGPuzzle *p)
{
    if (!p || !p->level) return NGP_STATUS_LOST;
    if (p->state == ST_WON) return NGP_STATUS_WON;
    if (p->state == ST_LOST) return NGP_STATUS_LOST;
    return p->state == ST_IDLE ? NGP_STATUS_READY : NGP_STATUS_BUSY;
}

uint8_t NEOGEO_USER ng_puzzle_flying(const NGPuzzle *p)
{
    return (uint8_t)(p && p->state == ST_FLIGHT);
}

uint8_t NEOGEO_USER ng_puzzle_event(NGPuzzle *p, NGPuzzleEvent *out)
{
    NGPuzzleEvent *e;
    if (!p || !p->ev_count) return 0;
    e = &p->ev[p->ev_head];
    if (out) { out->type = e->type; out->cell = e->cell; out->a = e->a; out->b = e->b; out->value = e->value; }
    p->ev_head = (uint8_t)((p->ev_head + 1u) & (NGP_EVENT_QUEUE - 1u));
    p->ev_count--;
    return 1;
}

uint8_t NEOGEO_USER ng_puzzle_next_dirty(const NGPuzzle *p, uint8_t from)
{
    uint16_t c, n;
    if (!p) return NGP_NO_CELL;
    n = (uint16_t)((uint16_t)p->w * p->h);
    for (c = from; c < n; c++) {
        if (!(c & 7u) && !p->dirty[c >> 3]) { c = (uint16_t)(c + 7u); continue; }
        if (bit_get(p->dirty, (uint8_t)c)) return (uint8_t)c;
    }
    return NGP_NO_CELL;
}

void NEOGEO_USER ng_puzzle_dirty_clear(NGPuzzle *p)
{
    if (p) zero(p->dirty, NGP_BITS_BYTES);
}

/* ======================================================================
 * set-up
 * ==================================================================== */
static void NEOGEO_USER parse_layout(NGPuzzle *p)
{
    const char *s = p->level->layout;
    uint8_t n = (uint8_t)(p->w * p->h), c = 0;
    uint16_t i = 0;
    zero(p->color, NGP_MAX_CELLS); zero(p->kind, NGP_MAX_CELLS); zero(p->cover, NGP_MAX_CELLS);
    if (!s) {
        if (p->mode == NGP_MODE_MATCH) for (c = 0; c < n; c++) p->color[c] = 0xFFu;   /* random */
        return;
    }
    while (s[i] && c < n) {
        char ch = s[i++];
        if (is_sep(ch)) continue;
        if (p->mode == NGP_MODE_LOGIC) { p->color[c] = logic_color(ch); }
        else if (ch >= '1' && ch <= '8') { p->color[c] = (uint8_t)(ch - '0'); }
        else if (p->mode == NGP_MODE_MATCH) {
            switch (ch) {
            case '.': p->color[c] = 0xFFu; break;
            case '#': p->cover[c] = NGP_COVER_WALL; break;
            case 'c': p->cover[c] = 0x10u; break;
            case 'C': p->cover[c] = 0x20u; break;
            case 'K': p->cover[c] = 0x30u; break;
            case 'i': p->color[c] = 0xFFu; p->cover[c] = 1u; break;
            case 'I': p->color[c] = 0xFFu; p->cover[c] = 2u; break;
            case 'l': p->color[c] = 0xFFu; p->cover[c] = NGP_COVER_LOCK; break;
            case 'h': p->color[c] = 0xFFu; p->kind[c] = NGP_KIND_LINE_H; break;
            case 'v': p->color[c] = 0xFFu; p->kind[c] = NGP_KIND_LINE_V; break;
            case 'b': p->color[c] = 0xFFu; p->kind[c] = NGP_KIND_BOMB; break;
            case 'p': p->color[c] = NGP_COLOR_PRISM; p->kind[c] = NGP_KIND_PRISM; break;
            default: break;
            }
        } else {
            switch (ch) {
            case 'r': p->color[c] = 0xFFu; break;
            case 'A': p->color[c] = NGP_COLOR_STONE; p->kind[c] = NGP_KIND_ANCHOR; break;
            case 'S': p->color[c] = NGP_COLOR_STONE; break;
            case 'R': p->color[c] = 0xFFu; p->kind[c] = NGP_KIND_RESCUE; break;
            default: break;
            }
        }
        c++;
    }
}

static void NEOGEO_USER deal(NGPuzzle *p)
{
    uint8_t n = (uint8_t)(p->w * p->h), c, attempt;
    uint8_t pending[NGP_BITS_BYTES];
    zero(pending, NGP_BITS_BYTES);
    for (c = 0; c < n; c++) if (p->color[c] == 0xFFu) { bit_set(pending, c); p->color[c] = 0; }
    for (attempt = 0; attempt < 32; attempt++) {
        for (c = 0; c < n; c++) {
            if (!bit_get(pending, c)) continue;
            if (p->mode == NGP_MODE_MATCH) p->color[c] = calm_color(p, c);
            else p->color[c] = (uint8_t)(1u + rng_range(p, p->colors));
        }
        if (p->mode != NGP_MODE_MATCH) break;
        if (!board_has_run(p) && ng_puzzle_has_moves(p)) break;
        for (c = 0; c < n; c++) if (bit_get(pending, c)) p->color[c] = 0;
    }
}

int NEOGEO_USER ng_puzzle_init(NGPuzzle *p, const NGPuzzleLevel *lv, uint32_t seed)
{
    uint8_t g;
    int rc;
    if (!p || !lv) return NGP_ERR_ARG;
    rc = ng_puzzle_level_check(lv, 0);
    if (rc != NGP_OK) return rc;
    zero((uint8_t *)p, (uint16_t)sizeof(NGPuzzle));
    p->level = lv;
    p->mode = lv->mode; p->w = lv->w; p->h = lv->h; p->colors = lv->colors;
    p->seed = seed ? seed : (lv->seed ? lv->seed : DEFAULT_SEED);
    p->rng = p->seed;
    p->parity = (uint8_t)(lv->first_shifted ? 1u : 0u);
    p->swap_a = p->swap_b = NGP_NO_CELL;
    p->attach_cell = NGP_NO_CELL;
    p->moves_left = (p->mode != NGP_MODE_BUBBLE && lv->moves) ? lv->moves : 0xFFFFu;
    p->shots_left = (p->mode == NGP_MODE_BUBBLE && lv->moves) ? lv->moves : 0xFFFFu;
    p->time_left = lv->time_frames ? lv->time_frames : 0xFFFFu;
    parse_layout(p);
    deal(p);
    for (g = 0; g < NGP_MAX_OBJ; g++) {
        const NGPuzzleGoal *o = &lv->goals[g];
        uint16_t cnt = o->count;
        if (!cnt && (o->type == NGP_OBJ_ICE || o->type == NGP_OBJ_CRATE || o->type == NGP_OBJ_RESCUE)) {
            uint8_t c, n = (uint8_t)(p->w * p->h);
            for (c = 0; c < n; c++) {
                if (o->type == NGP_OBJ_ICE) cnt = (uint16_t)(cnt + NGP_ICE(p->cover[c]));
                else if (o->type == NGP_OBJ_CRATE && NGP_CRATE_HP(p->cover[c])) cnt++;
                else if (o->type == NGP_OBJ_RESCUE && p->kind[c] == NGP_KIND_RESCUE) cnt++;
            }
        }
        if (o->type == NGP_OBJ_PICTURE) cnt = logic_left(p);
        p->goal_left[g] = (o->type == NGP_OBJ_SCORE || o->type == NGP_OBJ_CLEAR || o->type == NGP_OBJ_NONE) ? 0u : cnt;
    }
    if (p->mode == NGP_MODE_BUBBLE) {
        p->angle = 768u;
        p->shot_color = pick_bubble(p);
        p->next_color = pick_bubble(p);
    }
    zero(p->dirty, NGP_BITS_BYTES);
    {
        uint8_t c, n = (uint8_t)(p->w * p->h);
        for (c = 0; c < n; c++) touch(p, c);
    }
    p->state = ST_IDLE;
    if (p->mode == NGP_MODE_MATCH && !ng_puzzle_has_moves(p)) {
        /* an authored board without a move: reshuffle it, or end it */
        if (!(lv->flags & NGP_LF_NO_SHUFFLE) && shuffle_board(p)) {
            p->shuffles++;
            emit(p, NGP_EV_SHUFFLE, NGP_NO_CELL, 0, 0, 0);
        } else {
            finish(p, 0, NGP_END_NO_MOVES);
        }
    }
    return NGP_OK;
}

void NEOGEO_USER ng_puzzle_reset(NGPuzzle *p)
{
    const NGPuzzleLevel *lv;
    uint32_t seed;
    if (!p || !p->level) return;
    lv = p->level; seed = p->seed;
    ng_puzzle_init(p, lv, seed);
}

/* ======================================================================
 * commands
 * ==================================================================== */
static int NEOGEO_USER do_swap(NGPuzzle *p, uint8_t a, uint8_t b)
{
    uint8_t tc, tk;
    if (p->mode != NGP_MODE_MATCH) return NGP_ERR_STATE;
    if (a >= (uint8_t)(p->w * p->h) || b >= (uint8_t)(p->w * p->h) || !neighbors4(p, a, b)) return NGP_ERR_RANGE;
    if (!swap_value(p, a, b)) {
        emit(p, NGP_EV_SWAP_BACK, a, b, 0, 0);
        return NGP_ERR_ILLEGAL;
    }
    tc = p->color[a]; tk = p->kind[a];
    set_piece(p, a, p->color[b], p->kind[b]);
    set_piece(p, b, tc, tk);
    emit(p, NGP_EV_SWAP, a, b, 0, 0);
    p->swap_a = a; p->swap_b = b;
    p->moves_used++;
    if (p->moves_left != 0xFFFFu && p->moves_left) p->moves_left--;
    p->cascade = 1;
    p->state = ST_MATCH;
    return NGP_OK;
}

static uint16_t NEOGEO_USER clamp_aim(const NGPuzzle *p, int32_t a)
{
    if (a < (int32_t)aim_lo(p)) a = aim_lo(p);
    if (a > (int32_t)aim_hi(p)) a = aim_hi(p);
    return (uint16_t)a;
}

int NEOGEO_USER ng_puzzle_action(NGPuzzle *p, uint8_t cmd, int16_t a, int16_t b)
{
    if (!p || !p->level) return NGP_ERR_ARG;
    if (p->state == ST_WON || p->state == ST_LOST) return NGP_ERR_STATE;
    if (cmd == NGP_CMD_AIM || cmd == NGP_CMD_AIM_BY) {
        if (p->mode != NGP_MODE_BUBBLE) return NGP_ERR_STATE;
        p->angle = clamp_aim(p, cmd == NGP_CMD_AIM ? (int32_t)a : (int32_t)p->angle + a);
        return NGP_OK;
    }
    if (p->state != ST_IDLE) return NGP_ERR_BUSY;
    switch (cmd) {
    case NGP_CMD_SWAP: {
        uint8_t other;
        if (a < 0 || a >= (int16_t)(p->w * p->h) || b < 0 || b > 3) return NGP_ERR_RANGE;
        if (p->mode != NGP_MODE_MATCH) return NGP_ERR_STATE;
        other = ng_puzzle_neighbor(p, (uint8_t)a, (uint8_t)b);
        if (other == NGP_NO_CELL) return NGP_ERR_RANGE;
        return do_swap(p, (uint8_t)a, other);
    }
    case NGP_CMD_SWAP_CELLS:
        if (a < 0 || b < 0 || a > 255 || b > 255) return NGP_ERR_RANGE;
        return do_swap(p, (uint8_t)a, (uint8_t)b);
    case NGP_CMD_FIRE: {
        Flight f;
        if (p->mode != NGP_MODE_BUBBLE) return NGP_ERR_STATE;
        flight_start(p, p->angle, &f);
        p->shot_x_q8 = f.x; p->shot_y_q8 = f.y; p->shot_vx_q8 = f.vx; p->shot_vy_q8 = f.vy;
        if (p->shots_left != 0xFFFFu && p->shots_left) p->shots_left--;
        p->moves_used++;
        emit(p, NGP_EV_FIRE, NGP_NO_CELL, p->shot_color, 0, p->angle);
        p->state = ST_FLIGHT;
        return NGP_OK;
    }
    case NGP_CMD_SWAP_NEXT: {
        uint8_t t;
        if (p->mode != NGP_MODE_BUBBLE) return NGP_ERR_STATE;
        t = p->shot_color; p->shot_color = p->next_color; p->next_color = t;
        return NGP_OK;
    }
    case NGP_CMD_COLOR_SHOT:
        if (p->mode != NGP_MODE_BUBBLE) return NGP_ERR_STATE;
        if (a < 1 || a > (int16_t)p->colors) return NGP_ERR_RANGE;
        p->shot_color = (uint8_t)a;
        return NGP_OK;
    case NGP_CMD_MARK:
        if (p->mode != NGP_MODE_LOGIC) return NGP_ERR_STATE;
        if (a < 0 || a >= (int16_t)(p->w * p->h) || b < 0 || b > NGP_MARK_CROSS) return NGP_ERR_RANGE;
        return logic_mark(p, (uint8_t)a, (uint8_t)b);
    case NGP_CMD_HAMMER: {
        uint8_t c;
        if (a < 0 || a >= (int16_t)(p->w * p->h)) return NGP_ERR_RANGE;
        c = (uint8_t)a;
        if (p->mode == NGP_MODE_LOGIC) { logic_reveal(p, c); return NGP_OK; }
        if (p->mode == NGP_MODE_MATCH) {
            if (p->cover[c] & NGP_COVER_CRATE) { hit_crate(p, c); p->state = ST_FALL; p->cascade = 1; return NGP_OK; }
            if (!p->color[c] || (p->cover[c] & NGP_COVER_WALL)) return NGP_ERR_RANGE;
            zero(p->mark, NGP_BITS_BYTES);
            p->cover[c] = (uint8_t)(p->cover[c] & (uint8_t)~NGP_COVER_LOCK);
            set_piece(p, c, 0, NGP_KIND_PLAIN);
            zero(p->cleared, NGP_BITS_BYTES);
            bit_set(p->cleared, c);
            p->cascade = 1;
            p->state = ST_FALL;
            return NGP_OK;
        }
        if (!occupied(p, c) || p->kind[c] == NGP_KIND_ANCHOR) return NGP_ERR_RANGE;
        set_piece(p, c, 0, NGP_KIND_PLAIN);
        p->attach_cell = NGP_NO_CELL;
        p->state = ST_POP;
        return NGP_OK;
    }
    case NGP_CMD_SHUFFLE:
        if (p->mode != NGP_MODE_MATCH) return NGP_ERR_STATE;
        if (!shuffle_board(p)) return NGP_ERR_ILLEGAL;
        p->shuffles++;
        emit(p, NGP_EV_SHUFFLE, NGP_NO_CELL, 1, 0, 0);
        return NGP_OK;
    default:
        return NGP_ERR_ARG;
    }
}

/* ======================================================================
 * hash, save, load, replay
 * ==================================================================== */
static uint32_t NEOGEO_USER fnv(uint32_t h, const uint8_t *d, uint16_t n)
{
    while (n--) {
        h ^= *d++;
        /* h *= 16777619 (FNV prime 2^24 + 2^8 + 0x93), by shifts and adds */
        h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
    }
    return h;
}

static uint32_t NEOGEO_USER fnv32(uint32_t h, uint32_t v)
{
    uint8_t b[4];
    b[0] = (uint8_t)(v >> 24); b[1] = (uint8_t)(v >> 16); b[2] = (uint8_t)(v >> 8); b[3] = (uint8_t)v;
    return fnv(h, b, 4);
}

/* Keep the v1 identity solely for reading already-written saves. */
static uint32_t NEOGEO_USER level_id_v1(const NGPuzzleLevel *lv)
{
    uint32_t h = fnv32(2166136261u, ((uint32_t)lv->mode << 24) | ((uint32_t)lv->w << 16) | ((uint32_t)lv->h << 8) | lv->colors);
    h = fnv32(h, lv->seed);
    if (lv->layout) {
        const char *s = lv->layout;
        while (*s) { uint8_t ch = (uint8_t)*s++; h = fnv(h, &ch, 1); }
    }
    return h;
}

static uint32_t NEOGEO_USER level_id(const NGPuzzleLevel *lv)
{
    uint8_t i;
    uint32_t h = level_id_v1(lv);
    h = fnv32(h, ((uint32_t)lv->moves << 16) | lv->time_frames);
    h = fnv32(h, lv->flags);
    for (i = 0; i < NGP_MAX_OBJ; i++)
        h = fnv32(h, ((uint32_t)lv->goals[i].type << 24) | ((uint32_t)lv->goals[i].target << 16) | lv->goals[i].count);
    for (i = 0; i < 3u; i++) h = fnv32(h, lv->stars[i]);
    if (lv->mode == NGP_MODE_BUBBLE) {
        h = fnv32(h, ((uint32_t)lv->bubble_d << 24) | ((uint32_t)lv->row_h << 16) | ((uint32_t)lv->loss_row << 8) | lv->push_every);
        h = fnv32(h, ((uint32_t)lv->miss_limit << 8) | lv->first_shifted);
        h = fnv32(h, ((uint32_t)(uint16_t)lv->launch_x << 16) | (uint16_t)lv->launch_y);
        h = fnv32(h, lv->shot_speed_q8);
        h = fnv32(h, ((uint32_t)lv->aim_min << 16) | lv->aim_max);
    }
    return h;
}

uint32_t NEOGEO_USER ng_puzzle_hash(const NGPuzzle *p)
{
    uint32_t h = 2166136261u;
    uint16_t n;
    uint8_t g;
    if (!p || !p->level || p->w > NGP_MAX_W || p->h > NGP_MAX_H) return 0;
    n = (uint16_t)((uint16_t)p->w * p->h);
    h = fnv32(h, level_id(p->level));
    h = fnv32(h, ((uint32_t)p->mode << 24) | ((uint32_t)p->w << 16) | ((uint32_t)p->h << 8) | p->colors);
    h = fnv(h, p->color, n); h = fnv(h, p->kind, n); h = fnv(h, p->cover, n);
    h = fnv32(h, p->score);
    h = fnv32(h, ((uint32_t)p->moves_used << 16) | p->moves_left);
    h = fnv32(h, ((uint32_t)p->time_left << 16) | p->shots_left);
    for (g = 0; g < NGP_MAX_OBJ; g++) h = fnv32(h, p->goal_left[g]);
    h = fnv32(h, ((uint32_t)p->state << 24) | ((uint32_t)p->end_reason << 16) | ((uint32_t)p->cascade << 8) | p->stars);
    h = fnv32(h, p->rng);
    h = fnv32(h, p->seed);
    h = fnv32(h, ((uint32_t)p->tick << 16) | ((uint32_t)p->specials_made << 8) | p->shuffles);
    h = fnv32(h, ((uint32_t)p->shot_kind << 24) | ((uint32_t)p->attach_cell << 16) | ((uint32_t)p->swap_a << 8) | p->swap_b);
    h = fnv32(h, ((uint32_t)p->angle << 16) | ((uint32_t)p->shot_color << 8) | p->next_color);
    h = fnv32(h, ((uint32_t)p->misses << 16) | ((uint32_t)p->shots_since_row << 8) | p->parity);
    h = fnv32(h, (uint32_t)p->shot_x_q8); h = fnv32(h, (uint32_t)p->shot_y_q8);
    h = fnv32(h, (uint32_t)p->shot_vx_q8); h = fnv32(h, (uint32_t)p->shot_vy_q8);
    return h;
}

static uint16_t NEOGEO_USER put32(uint8_t *b, uint16_t o, uint32_t v)
{
    b[o] = (uint8_t)(v >> 24); b[o + 1] = (uint8_t)(v >> 16); b[o + 2] = (uint8_t)(v >> 8); b[o + 3] = (uint8_t)v;
    return (uint16_t)(o + 4u);
}

static uint16_t NEOGEO_USER put16(uint8_t *b, uint16_t o, uint16_t v)
{
    b[o] = (uint8_t)(v >> 8); b[o + 1] = (uint8_t)v;
    return (uint16_t)(o + 2u);
}

static uint32_t NEOGEO_USER get32(const uint8_t *b, uint16_t o)
{
    return ((uint32_t)b[o] << 24) | ((uint32_t)b[o + 1] << 16) | ((uint32_t)b[o + 2] << 8) | b[o + 3];
}

static uint16_t NEOGEO_USER get16(const uint8_t *b, uint16_t o)
{
    return (uint16_t)(((uint16_t)b[o] << 8) | b[o + 1]);
}

uint16_t NEOGEO_USER ng_puzzle_save(const NGPuzzle *p, uint8_t *b, uint16_t size)
{
    uint16_t o = 0, n;
    uint8_t g;
    if (!p || !p->level || !b || size < NGP_SAVE_SIZE ||
        p->w != p->level->w || p->h != p->level->h ||
        p->w > NGP_MAX_W || p->h > NGP_MAX_H) return 0;
    n = (uint16_t)((uint16_t)p->w * p->h);
    o = put32(b, o, SAVE_MAGIC);
    b[o++] = SAVE_VERSION; b[o++] = p->state; b[o++] = p->cascade; b[o++] = p->end_reason;
    o = put32(b, o, level_id(p->level));
    o = put32(b, o, p->seed);
    o = put32(b, o, p->rng);
    o = put32(b, o, p->score);
    ngp_copy(&b[o], p->color, n); o = (uint16_t)(o + n);
    ngp_copy(&b[o], p->kind, n); o = (uint16_t)(o + n);
    ngp_copy(&b[o], p->cover, n); o = (uint16_t)(o + n);
    for (g = 0; g < NGP_MAX_OBJ; g++) o = put16(b, o, p->goal_left[g]);
    o = put16(b, o, p->moves_used); o = put16(b, o, p->moves_left);
    o = put16(b, o, p->time_left); o = put16(b, o, p->shots_left);
    o = put16(b, o, p->tick); o = put16(b, o, p->angle);
    b[o++] = p->stars; b[o++] = p->specials_made; b[o++] = p->shuffles; b[o++] = p->shot_color;
    b[o++] = p->next_color; b[o++] = p->shot_kind; b[o++] = p->misses; b[o++] = p->shots_since_row;
    b[o++] = p->parity; b[o++] = p->attach_cell; b[o++] = p->swap_a; b[o++] = p->swap_b;
    o = put32(b, o, (uint32_t)p->shot_x_q8); o = put32(b, o, (uint32_t)p->shot_y_q8);
    o = put32(b, o, (uint32_t)p->shot_vx_q8); o = put32(b, o, (uint32_t)p->shot_vy_q8);
    /* Accidental-corruption check, not an authentication signature. */
    o = put32(b, o, fnv(2166136261u, b, o));
    return o;
}

int NEOGEO_USER ng_puzzle_load(NGPuzzle *p, const NGPuzzleLevel *lv, const uint8_t *b, uint16_t size)
{
    uint16_t o = 0, n, payload, c;
    uint8_t g, ver, st, cas, end;
    int rc;
    if (!p || !lv || !b || size < 24u) return NGP_ERR_ARG;
    if (get32(b, 0) != SAVE_MAGIC) return NGP_ERR_SAVE;
    ver = b[4]; st = b[5]; cas = b[6]; end = b[7];
    if ((ver != 1u && ver != SAVE_VERSION) || st > ST_LOST || end > NGP_END_MISTAKES) return NGP_ERR_SAVE;
    rc = ng_puzzle_level_check(lv, 0);
    if (rc != NGP_OK) return rc;
    if (get32(b, 8) != (ver == 1u ? level_id_v1(lv) : level_id(lv))) return NGP_ERR_SAVE;
    /* Reject every truncated payload BEFORE init clears the caller's live
     * board, event queue and progress. Dimensions are validated above. */
    n = (uint16_t)((uint16_t)lv->w * lv->h);
    payload = (uint16_t)(24u + 3u * n + 2u * NGP_MAX_OBJ + 12u + 12u + 16u);
    if (size < (uint16_t)(payload + (ver == SAVE_VERSION ? 4u : 0u))) return NGP_ERR_SAVE;
    if (ver == SAVE_VERSION && get32(b, payload) != fnv(2166136261u, b, payload)) return NGP_ERR_SAVE;
    /* Validate indices/enums before any mutation, also for legacy payloads
     * which have no checksum. No temporary full context on the 68000 stack. */
    if ((lv->mode == NGP_MODE_MATCH && (st == ST_FLIGHT || st == ST_POP)) ||
        (lv->mode == NGP_MODE_BUBBLE && st >= ST_MATCH && st <= ST_SETTLE) ||
        (lv->mode == NGP_MODE_LOGIC && st != ST_IDLE && st < ST_WON)) return NGP_ERR_SAVE;
    if ((st >= ST_WON) != (end != NGP_END_NONE) || !get32(b, 12) || !get32(b, 16)) return NGP_ERR_SAVE;
    for (c = 0; c < n; c++) {
        uint8_t col = b[24u + c], kind = b[24u + n + c], cover = b[24u + 2u * n + c];
        if (lv->mode == NGP_MODE_LOGIC) {
            if (col > lv->colors || kind > NGP_MARK_MISS || cover || (kind == NGP_MARK_FILL && !col) ||
                (kind == NGP_MARK_MISS && col)) return NGP_ERR_SAVE;
            continue;
        }
        if (col > lv->colors && col != NGP_COLOR_PRISM && col != NGP_COLOR_STONE) return NGP_ERR_SAVE;
        if (kind > NGP_KIND_RESCUE || (cover & 0x0Cu)) return NGP_ERR_SAVE;
        if (lv->mode == NGP_MODE_MATCH && (col == NGP_COLOR_STONE || kind > NGP_KIND_PRISM)) return NGP_ERR_SAVE;
        if (lv->mode == NGP_MODE_BUBBLE && (col == NGP_COLOR_PRISM || cover)) return NGP_ERR_SAVE;
        if (!col && kind != NGP_KIND_PLAIN) return NGP_ERR_SAVE;
        if ((cover & (NGP_COVER_WALL | NGP_COVER_CRATE)) && col) return NGP_ERR_SAVE;
        if (lv->mode == NGP_MODE_MATCH && ((col == NGP_COLOR_PRISM) != (kind == NGP_KIND_PRISM))) return NGP_ERR_SAVE;
    }
    o = (uint16_t)(24u + 3u * n + 2u * NGP_MAX_OBJ + 12u);
    if (b[o] > 3u || b[o + 3u] > lv->colors || b[o + 4u] > lv->colors ||
        b[o + 5u] > NGP_KIND_RESCUE || b[o + 8u] > 1u || get16(b, (uint16_t)(o - 2u)) >= 1024u) return NGP_ERR_SAVE;
    for (g = 9u; g < 12u; g++)
        if (b[o + g] != NGP_NO_CELL && b[o + g] >= n) return NGP_ERR_SAVE;
    if ((b[o + 10u] == NGP_NO_CELL) != (b[o + 11u] == NGP_NO_CELL)) return NGP_ERR_SAVE;
    rc = ng_puzzle_init(p, lv, get32(b, 12));
    if (rc != NGP_OK) return rc;
    o = 16;
    p->rng = get32(b, o); o = (uint16_t)(o + 4u);
    p->score = get32(b, o); o = (uint16_t)(o + 4u);
    ngp_copy(p->color, &b[o], n); o = (uint16_t)(o + n);
    ngp_copy(p->kind, &b[o], n); o = (uint16_t)(o + n);
    ngp_copy(p->cover, &b[o], n); o = (uint16_t)(o + n);
    for (g = 0; g < NGP_MAX_OBJ; g++) { p->goal_left[g] = get16(b, o); o = (uint16_t)(o + 2u); }
    p->moves_used = get16(b, o); o = (uint16_t)(o + 2u);
    p->moves_left = get16(b, o); o = (uint16_t)(o + 2u);
    p->time_left = get16(b, o); o = (uint16_t)(o + 2u);
    p->shots_left = get16(b, o); o = (uint16_t)(o + 2u);
    p->tick = get16(b, o); o = (uint16_t)(o + 2u);
    p->angle = get16(b, o); o = (uint16_t)(o + 2u);
    p->stars = b[o++]; p->specials_made = b[o++]; p->shuffles = b[o++]; p->shot_color = b[o++];
    p->next_color = b[o++]; p->shot_kind = b[o++]; p->misses = b[o++]; p->shots_since_row = b[o++];
    p->parity = b[o++]; p->attach_cell = b[o++]; p->swap_a = b[o++]; p->swap_b = b[o++];
    p->shot_x_q8 = (int32_t)get32(b, o); o = (uint16_t)(o + 4u);
    p->shot_y_q8 = (int32_t)get32(b, o); o = (uint16_t)(o + 4u);
    p->shot_vx_q8 = (int32_t)get32(b, o); o = (uint16_t)(o + 4u);
    p->shot_vy_q8 = (int32_t)get32(b, o);
    p->state = st; p->cascade = cas; p->end_reason = end;
    p->ev_count = 0; p->ev_head = 0;
    {
        uint8_t c;
        for (c = 0; c < (uint8_t)n; c++) touch(p, c);
    }
    return NGP_OK;
}

uint32_t NEOGEO_USER ng_puzzle_replay(NGPuzzle *p, const NGPuzzleLevel *lv, uint32_t seed,
                                      const NGPuzzleAction *act, uint16_t count)
{
    uint16_t i, guard;
    if (!p || ng_puzzle_init(p, lv, seed) != NGP_OK) return 0;
    for (i = 0; i < count; i++) {
        guard = 0;
        while (p->tick < act[i].tick && guard++ < SETTLE_LIMIT) {
            NGPuzzleEvent e;
            ng_puzzle_update(p);
            while (ng_puzzle_event(p, &e)) {}
        }
        ng_puzzle_action(p, act[i].cmd, act[i].a, act[i].b);
    }
    ng_puzzle_settle(p);
    return ng_puzzle_hash(p);
}
