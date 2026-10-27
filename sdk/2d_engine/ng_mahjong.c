/*
 * ng_mahjong.c -- the Mahjong genre engine (see ng_mahjong.h).
 *
 * Sections: helpers (generator, events, tile arithmetic), hand analysis
 * (winning shapes, shanten, scoring), the four-seat table (wall, deal,
 * draws, claims, turns, payments, rounds), the CPU, save/load and hashing,
 * and solitaire (layouts, free tiles, solvable deals, selection, undo).
 *
 * Hand analysis works on counts by kind (34 entries) and recurses at most
 * four sets deep for winning shapes; the shanten search is a depth-first
 * walk with a node budget, so a CPU decision has a fixed worst case.
 * No 32-bit divide or modulo; no struct assignment; copies by index.
 */
#include "ng_mahjong.h"

#define DEFAULT_SEED   0x9E3779B9u
#define SAVE_MAGIC     0x4E474D4Au   /* 'NGMJ' */
#define SAVE_VERSION   1u
#define SHANTEN_BUDGET 6000u

/* ======================================================================
 * helpers
 * ==================================================================== */
static void NEOGEO_USER zero(uint8_t *d, uint16_t n)
{
    while (n--) *d++ = 0;
}

static uint32_t NEOGEO_USER xs32(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *state = x;
    return x;
}

static uint16_t NEOGEO_USER range16(uint32_t *state, uint16_t n)
{
    return (uint16_t)(((uint32_t)(uint16_t)(xs32(state) >> 16) * (uint32_t)n) >> 16);
}

static uint8_t NEOGEO_USER rank_of(uint8_t k)
{
    if (k >= 27u) return 0;
    if (k >= 18u) return (uint8_t)(k - 17u);
    if (k >= 9u) return (uint8_t)(k - 8u);
    return (uint8_t)(k + 1u);
}

static uint8_t NEOGEO_USER suit_of(uint8_t k)
{
    return (uint8_t)(k < 9u ? 0u : k < 18u ? 1u : k < 27u ? 2u : 3u);
}

static uint8_t NEOGEO_USER terminal_or_honor(uint8_t k)
{
    uint8_t r = rank_of(k);
    return (uint8_t)(k >= 27u || r == 1u || r == 9u);
}

static uint8_t NEOGEO_USER next_seat(uint8_t s) { return (uint8_t)((s + 1u) & 3u); }

static void NEOGEO_USER emit(NGMahjong *m, uint8_t type, uint8_t seat, uint8_t tile, uint8_t a, uint8_t b)
{
    NGMahjongEvent *e;
    if (m->ev_count >= NGM_EVENT_QUEUE) { m->ev_dropped++; return; }
    e = &m->ev[(m->ev_head + m->ev_count) & (NGM_EVENT_QUEUE - 1u)];
    e->type = type; e->seat = seat; e->tile = tile; e->a = a; e->b = b; e->pad = 0;
    m->ev_count++;
}

/* ======================================================================
 * hand analysis
 * ==================================================================== */
typedef struct {
    uint8_t type[4], kind[4], open[4];   /* sets: NGM_MELD_CHOW / PUNG / KONG; open = exposed */
    uint8_t n;
    uint8_t pair;
} Shape;

static uint8_t NEOGEO_USER total(const uint8_t *c)
{
    uint8_t i, t = 0;
    for (i = 0; i < NGM_KINDS; i++) t = (uint8_t)(t + c[i]);
    return t;
}

static uint8_t NEOGEO_USER thirteen_orphans(const uint8_t *c)
{
    static const uint8_t want[13] = { 0, 8, 9, 17, 18, 26, 27, 28, 29, 30, 31, 32, 33 };
    uint8_t i, pair = 0;
    if (total(c) != 14u) return 0;
    for (i = 0; i < 13; i++) {
        if (!c[want[i]]) return 0;
        if (c[want[i]] == 2u) pair++;
    }
    return (uint8_t)(pair == 1u);
}

/* Split counts into need sets (bounded recursion: need <= 4). */
static uint8_t NEOGEO_USER sets_of(uint8_t *c, uint8_t need)
{
    uint8_t i = 0, ok = 0;
    while (i < NGM_KINDS && !c[i]) i++;
    if (i >= NGM_KINDS) return (uint8_t)(need == 0);
    if (!need) return 0;
    if (c[i] >= 3u) {
        c[i] = (uint8_t)(c[i] - 3u);
        ok = sets_of(c, (uint8_t)(need - 1u));
        c[i] = (uint8_t)(c[i] + 3u);
        if (ok) return 1;
    }
    if (i < 27u && rank_of(i) <= 7u && c[i + 1] && c[i + 2]) {
        c[i]--; c[i + 1]--; c[i + 2]--;
        ok = sets_of(c, (uint8_t)(need - 1u));
        c[i]++; c[i + 1]++; c[i + 2]++;
    }
    return ok;
}

uint8_t NEOGEO_USER ng_mahjong_is_win(const uint8_t counts[NGM_KINDS], uint8_t melds)
{
    uint8_t c[NGM_KINDS], i, need;
    if (!counts || melds > 4u) return 0;
    for (i = 0; i < NGM_KINDS; i++) c[i] = counts[i];
    need = (uint8_t)(4u - melds);
    if (total(c) != (uint8_t)(need * 3u + 2u)) return 0;
    if (!melds && thirteen_orphans(c)) return 1;
    for (i = 0; i < NGM_KINDS; i++) {
        uint8_t ok;
        if (c[i] < 2u) continue;
        c[i] = (uint8_t)(c[i] - 2u);
        ok = sets_of(c, need);
        c[i] = (uint8_t)(c[i] + 2u);
        if (ok) return 1;
    }
    return 0;
}

/* --- shanten ---------------------------------------------------------------
 * Each suit (and the honours) is searched on its own for what it can make:
 * for m sets and p pairs (0/1), the most partial sets t beside them. The four
 * tables are then combined. A suit holds at most 14 tiles over 9 kinds, so
 * each search is small; a node budget caps it all the same. */
typedef struct {
    uint8_t c[9];
    uint8_t len, honors;
    int8_t best[5][2];      /* most partials for (sets, pair) */
    uint16_t nodes;
} Block;

static void NEOGEO_USER block_walk(Block *b, uint8_t i, uint8_t m, uint8_t t, uint8_t p)
{
    if (++b->nodes > SHANTEN_BUDGET) return;
    while (i < b->len && !b->c[i]) i++;
    if (m <= 4u && (int8_t)t > b->best[m][p]) b->best[m][p] = (int8_t)t;
    if (i >= b->len || m >= 4u) return;
    if (b->c[i] >= 3u) {
        b->c[i] = (uint8_t)(b->c[i] - 3u);
        block_walk(b, i, (uint8_t)(m + 1u), t, p);
        b->c[i] = (uint8_t)(b->c[i] + 3u);
    }
    if (!b->honors && i + 2u < b->len && b->c[i + 1] && b->c[i + 2]) {
        b->c[i]--; b->c[i + 1]--; b->c[i + 2]--;
        block_walk(b, i, (uint8_t)(m + 1u), t, p);
        b->c[i]++; b->c[i + 1]++; b->c[i + 2]++;
    }
    if (b->c[i] >= 2u) {
        b->c[i] = (uint8_t)(b->c[i] - 2u);
        if (!p) block_walk(b, i, m, t, 1);
        if (t < 4u) block_walk(b, i, m, (uint8_t)(t + 1u), p);
        b->c[i] = (uint8_t)(b->c[i] + 2u);
    }
    if (!b->honors && t < 4u) {
        if (i + 1u < b->len && b->c[i + 1]) {
            b->c[i]--; b->c[i + 1]--;
            block_walk(b, i, m, (uint8_t)(t + 1u), p);
            b->c[i]++; b->c[i + 1]++;
        }
        if (i + 2u < b->len && b->c[i + 2]) {
            b->c[i]--; b->c[i + 2]--;
            block_walk(b, i, m, (uint8_t)(t + 1u), p);
            b->c[i]++; b->c[i + 2]++;
        }
    }
    {
        uint8_t keep = b->c[i];
        b->c[i] = 0;
        block_walk(b, (uint8_t)(i + 1u), m, t, p);
        b->c[i] = keep;
    }
}

int8_t NEOGEO_USER ng_mahjong_shanten(const uint8_t counts[NGM_KINDS], uint8_t melds)
{
    int8_t acc[5][2], nxt[5][2], s = 8;
    uint8_t blk, i, m, p, m2, p2;
    if (!counts || melds > 4u) return 8;
    for (m = 0; m < 5u; m++) { acc[m][0] = acc[m][1] = -1; }
    acc[0][0] = 0;
    for (blk = 0; blk < 4u; blk++) {
        Block b;
        b.len = (uint8_t)(blk < 3u ? 9u : 7u);
        b.honors = (uint8_t)(blk == 3u);
        for (i = 0; i < b.len; i++) b.c[i] = counts[blk * 9u + i];
        for (m = 0; m < 5u; m++) { b.best[m][0] = b.best[m][1] = -1; }
        b.nodes = 0;
        block_walk(&b, 0, 0, 0, 0);
        for (m = 0; m < 5u; m++) { nxt[m][0] = nxt[m][1] = -1; }
        for (m = 0; m < 5u; m++) for (p = 0; p < 2u; p++) {
            if (acc[m][p] < 0) continue;
            for (m2 = 0; m + m2 < 5u; m2++) for (p2 = 0; p + p2 < 2u; p2++) {
                int8_t t;
                if (b.best[m2][p2] < 0) continue;
                t = (int8_t)(acc[m][p] + b.best[m2][p2]);
                if (t > nxt[m + m2][p + p2]) nxt[m + m2][p + p2] = t;
            }
        }
        for (m = 0; m < 5u; m++) { acc[m][0] = nxt[m][0]; acc[m][1] = nxt[m][1]; }
    }
    for (m = 0; m < 5u; m++) for (p = 0; p < 2u; p++) {
        int8_t t, v;
        uint8_t mt = (uint8_t)(m + melds);
        if (acc[m][p] < 0 || mt > 4u) continue;
        t = acc[m][p];
        if (t > (int8_t)(4u - mt)) t = (int8_t)(4u - mt);
        v = (int8_t)(8 - 2 * (int8_t)mt - t - (int8_t)p);
        if (v < s) s = v;
    }
    if (!melds) {
        static const uint8_t ortho[13] = { 0, 8, 9, 17, 18, 26, 27, 28, 29, 30, 31, 32, 33 };
        uint8_t kinds = 0, pair = 0;
        int8_t t;
        for (i = 0; i < 13; i++) { if (counts[ortho[i]]) kinds++; if (counts[ortho[i]] >= 2u) pair = 1; }
        t = (int8_t)(13 - (int8_t)kinds - (int8_t)pair);
        if (t < s) s = t;
    }
    /* a complete hand reads -1 */
    if (s <= 0 && ng_mahjong_is_win(counts, melds)) s = -1;
    else if (s < 0) s = 0;
    return s;
}

/* --- scoring ------------------------------------------------------------ */
typedef struct {
    const NGMahjong *m;
    uint8_t seat, self_drawn, tile;
    uint8_t c[NGM_KINDS];
    Shape s;
    uint8_t best;
    uint32_t best_pat;
} ScoreWalk;

static uint8_t NEOGEO_USER limit_of(const NGMahjong *m)
{
    return m->rules.limit_faan ? m->rules.limit_faan : 10u;
}

static void NEOGEO_USER rate_shape(ScoreWalk *w)
{
    const NGMahjong *m = w->m;
    const NGMahjongHand *h = &m->hand[w->seat];
    uint8_t lim = limit_of(m), i, chows = 0, pungs = 0, dp = 0, wp = 0, exposed = 0;
    uint8_t suits = 0, honors = 0, terms_only = 1, seat_wind = ng_mahjong_seat_wind(m, w->seat);
    uint16_t f = 0;
    uint32_t pat = 0;
    uint8_t limit_hand = 0;
    for (i = 0; i < w->s.n; i++) {
        uint8_t k = w->s.kind[i];
        if (w->s.type[i] == NGM_MELD_CHOW) {
            chows++;
            suits = (uint8_t)(suits | (1u << suit_of(k)));
            terms_only = 0;
        } else {
            pungs++;
            if (k >= 27u) honors = 1; else suits = (uint8_t)(suits | (1u << suit_of(k)));
            if (!(k < 27u && terminal_or_honor(k))) terms_only = 0;
            if (k >= NGM_RED) dp++;
            if (k >= NGM_EAST && k <= NGM_NORTH) {
                wp++;
                if (k == seat_wind) { f++; pat |= NGM_PAT_SEAT_WIND; }
                if (k == m->round_wind) { f++; pat |= NGM_PAT_ROUND_WIND; }
            }
        }
        if (w->s.open[i]) exposed = 1;
    }
    {
        uint8_t k = w->s.pair;
        if (k >= 27u) honors = 1; else suits = (uint8_t)(suits | (1u << suit_of(k)));
        if (!(k < 27u && terminal_or_honor(k))) terms_only = 0;
    }
    if (chows == 4u) { f++; pat |= NGM_PAT_ALL_CHOWS; }
    if (pungs == 4u) { f = (uint16_t)(f + 3u); pat |= NGM_PAT_ALL_PUNGS; }
    if (!suits && honors) { f = (uint16_t)(f + 10u); pat |= NGM_PAT_ALL_HONORS; }
    else if ((suits == 1u || suits == 2u || suits == 4u) && !honors) { f = (uint16_t)(f + 7u); pat |= NGM_PAT_FULL_FLUSH; }
    else if ((suits == 1u || suits == 2u || suits == 4u) && honors) { f = (uint16_t)(f + 3u); pat |= NGM_PAT_HALF_FLUSH; }
    if (terms_only && pungs == 4u) { f = (uint16_t)(f + 10u); pat |= NGM_PAT_ALL_TERMINALS; }
    if (dp == 3u) { f = (uint16_t)(f + 8u); pat |= NGM_PAT_BIG_DRAGONS; }
    else if (dp == 2u && w->s.pair >= NGM_RED) { f = (uint16_t)(f + 5u); pat |= NGM_PAT_SMALL_DRAGONS; }
    else if (dp) { f = (uint16_t)(f + dp); pat |= NGM_PAT_DRAGON_PUNG; }
    if (wp == 4u) { limit_hand = 1; pat |= NGM_PAT_BIG_WINDS; }
    else if (wp == 3u && w->s.pair >= NGM_EAST && w->s.pair <= NGM_NORTH) { f = (uint16_t)(f + 10u); pat |= NGM_PAT_SMALL_WINDS; }
    if (!exposed) { f++; pat |= NGM_PAT_CONCEALED; }
    if (w->self_drawn) { f++; pat |= NGM_PAT_SELF_DRAWN; }
    if (w->self_drawn && m->wall_left == 0u) { f++; pat |= NGM_PAT_LAST_TILE; }
    if (w->self_drawn && m->kong_draw) { f++; pat |= NGM_PAT_KONG_DRAW; }
    if (m->rules.flowers) {
        uint8_t own = (uint8_t)(seat_wind - NGM_EAST);
        if (!h->flowers) { f++; pat |= NGM_PAT_NO_FLOWERS; }
        if (h->flowers & (1u << own)) { f++; pat |= NGM_PAT_OWN_FLOWER; }
        if (h->flowers & (1u << (own + 4u))) { f++; pat |= NGM_PAT_OWN_FLOWER; }
    }
    if (limit_hand || f > lim) f = lim;
    if ((uint8_t)f > w->best || (f == w->best && w->best_pat == 0)) { w->best = (uint8_t)f; w->best_pat = pat; }
}

/* Every way to split the concealed counts into sets (bounded: 4 deep). */
static void NEOGEO_USER split(ScoreWalk *w)
{
    uint8_t i = 0, n = w->s.n;
    while (i < NGM_KINDS && !w->c[i]) i++;
    if (i >= NGM_KINDS) { if (n == 4u) rate_shape(w); return; }
    if (n >= 4u) return;
    if (w->c[i] >= 3u) {
        w->c[i] = (uint8_t)(w->c[i] - 3u);
        w->s.type[n] = NGM_MELD_PUNG; w->s.kind[n] = i; w->s.open[n] = 0; w->s.n = (uint8_t)(n + 1u);
        split(w);
        w->s.n = n;
        w->c[i] = (uint8_t)(w->c[i] + 3u);
    }
    if (i < 27u && rank_of(i) <= 7u && w->c[i + 1] && w->c[i + 2]) {
        w->c[i]--; w->c[i + 1]--; w->c[i + 2]--;
        w->s.type[n] = NGM_MELD_CHOW; w->s.kind[n] = i; w->s.open[n] = 0; w->s.n = (uint8_t)(n + 1u);
        split(w);
        w->s.n = n;
        w->c[i]++; w->c[i + 1]++; w->c[i + 2]++;
    }
}

static uint8_t NEOGEO_USER nine_gates(const uint8_t *c)
{
    uint8_t s, r;
    for (s = 0; s < 3u; s++) {
        uint8_t base = (uint8_t)(s * 9u), ok = 1, sum = 0;
        for (r = 0; r < 9u; r++) sum = (uint8_t)(sum + c[base + r]);
        if (sum != 14u) continue;
        if (c[base] < 3u || c[base + 8u] < 3u) ok = 0;
        for (r = 1; r < 8u && ok; r++) if (!c[base + r]) ok = 0;
        if (ok) return 1;
    }
    return 0;
}

uint8_t NEOGEO_USER ng_mahjong_score(const NGMahjong *m, uint8_t seat, uint8_t tile, uint8_t self_drawn,
                                     NGMahjongResult *r)
{
    ScoreWalk w;
    const NGMahjongHand *h;
    uint8_t i, lim;
    if (!m || seat >= NGM_SEATS || !r) return 0;
    h = &m->hand[seat];
    lim = limit_of(m);
    zero(w.c, NGM_KINDS);
    for (i = 0; i < h->count; i++) if (h->tiles[i] < NGM_KINDS) w.c[h->tiles[i]]++;
    if (!self_drawn && tile < NGM_KINDS) w.c[tile]++;
    r->faan = 0; r->patterns = 0;
    if (!ng_mahjong_is_win(w.c, h->nmelds)) return 0;
    if (m->first_turn && self_drawn && seat == m->dealer) {
        r->faan = lim; r->patterns = NGM_PAT_HEAVENLY;
        return 1;
    }
    if (!h->nmelds && thirteen_orphans(w.c)) { r->faan = lim; r->patterns = NGM_PAT_THIRTEEN; return 1; }
    if (!h->nmelds && nine_gates(w.c)) { r->faan = lim; r->patterns = NGM_PAT_NINE_GATES; return 1; }
    w.m = m; w.seat = seat; w.self_drawn = self_drawn; w.tile = tile;
    w.best = 0; w.best_pat = 0;
    for (i = 0; i < NGM_KINDS; i++) {
        uint8_t j;
        if (w.c[i] < 2u) continue;
        w.c[i] = (uint8_t)(w.c[i] - 2u);
        w.s.pair = i;
        w.s.n = 0;
        for (j = 0; j < h->nmelds; j++) {
            uint8_t t = h->melds[j].type;
            w.s.type[j] = (uint8_t)(t == NGM_MELD_CHOW ? NGM_MELD_CHOW : t == NGM_MELD_PUNG ? NGM_MELD_PUNG : NGM_MELD_KONG);
            w.s.kind[j] = h->melds[j].kind;
            w.s.open[j] = (uint8_t)(t != NGM_MELD_KONG_CONCEALED);
        }
        w.s.n = h->nmelds;
        split(&w);
        w.c[i] = (uint8_t)(w.c[i] + 2u);
    }
    r->faan = w.best; r->patterns = w.best_pat;
    return 1;
}

/* ======================================================================
 * the table
 * ==================================================================== */
void NEOGEO_USER ng_mahjong_rules_default(NGMahjongRules *r)
{
    if (!r) return;
    r->ruleset = NGM_RULES_HK_OLD_STYLE;
    r->flowers = 0;
    r->min_faan = 0;
    r->limit_faan = 10;
    r->rounds = 1;
    r->pad = 0;
    r->base_points = 1;
    r->start_score = 1000;
}

uint8_t NEOGEO_USER ng_mahjong_seat_wind(const NGMahjong *m, uint8_t seat)
{
    return (uint8_t)(NGM_EAST + ((seat - m->dealer) & 3u));
}

static void NEOGEO_USER hand_add(NGMahjongHand *h, uint8_t k)
{
    uint8_t i;
    if (h->count >= NGM_HAND_MAX) return;
    i = h->count;
    while (i > 0 && h->tiles[i - 1] > k) { h->tiles[i] = h->tiles[i - 1]; i--; }
    h->tiles[i] = k;
    h->count++;
}

static uint8_t NEOGEO_USER hand_remove(NGMahjongHand *h, uint8_t k)
{
    uint8_t i, j;
    for (i = 0; i < h->count; i++) {
        if (h->tiles[i] != k) continue;
        for (j = i; j + 1u < h->count; j++) h->tiles[j] = h->tiles[j + 1u];
        h->count--;
        return 1;
    }
    return 0;
}

static uint8_t NEOGEO_USER hand_has(const NGMahjongHand *h, uint8_t k)
{
    uint8_t i, n = 0;
    for (i = 0; i < h->count; i++) if (h->tiles[i] == k) n++;
    return n;
}

static uint8_t NEOGEO_USER take_tile(NGMahjong *m, uint8_t from_tail)
{
    uint8_t t;
    if (!m->wall_left) return NGM_NO_TILE;
    if (from_tail) { t = m->wall[m->wall_tail]; m->wall_tail--; }
    else { t = m->wall[m->wall_head]; m->wall_head++; }
    m->wall_left--;
    return t;
}

/* Draw for seat into its hand; bonus tiles go aside and are replaced from
 * the tail. Returns the tile kept, or NGM_NO_TILE if the wall is out. */
static uint8_t NEOGEO_USER draw(NGMahjong *m, uint8_t seat, uint8_t from_tail)
{
    uint8_t t;
    for (;;) {
        t = take_tile(m, from_tail);
        if (t == NGM_NO_TILE) return t;
        if (NGM_IS_BONUS(t)) {
            m->hand[seat].flowers = (uint8_t)(m->hand[seat].flowers | (1u << (t - NGM_FLOWER)));
            emit(m, NGM_EV_FLOWER, seat, t, 0, 0);
            from_tail = 1;
            continue;
        }
        hand_add(&m->hand[seat], t);
        emit(m, NGM_EV_DRAW, seat, t, 0, 0);
        return t;
    }
}

static void NEOGEO_USER shuffle_wall(NGMahjong *m)
{
    uint8_t i, n = 0, k, c;
    for (k = 0; k < NGM_KINDS; k++) for (c = 0; c < 4u; c++) m->wall[n++] = k;
    if (m->rules.flowers) for (k = NGM_FLOWER; k < NGM_KINDS_ALL; k++) m->wall[n++] = k;
    m->wall_size = n;
    for (i = (uint8_t)(n - 1u); i > 0; i--) {
        uint8_t j = (uint8_t)range16(&m->rng, (uint16_t)(i + 1u)), t = m->wall[i];
        m->wall[i] = m->wall[j]; m->wall[j] = t;
    }
    m->wall_head = 0;
    m->wall_tail = (uint8_t)(n - 1u);
    m->wall_left = n;
}

static void NEOGEO_USER deal(NGMahjong *m)
{
    uint8_t s, i, round;
    for (s = 0; s < NGM_SEATS; s++) {
        NGMahjongHand *h = &m->hand[s];
        zero((uint8_t *)h, (uint16_t)sizeof(NGMahjongHand));
    }
    shuffle_wall(m);
    /* four tiles each, three times, then one each */
    for (round = 0; round < 4u; round++)
        for (s = 0; s < NGM_SEATS; s++) {
            uint8_t seat = (uint8_t)((m->dealer + s) & 3u);
            uint8_t n = (uint8_t)(round < 3u ? 4u : 1u);
            for (i = 0; i < n; i++) {
                uint8_t t = take_tile(m, 0);
                hand_add(&m->hand[seat], t);
            }
        }
    /* bonus tiles in the deal are set aside and replaced, dealer first */
    for (s = 0; s < NGM_SEATS; s++) {
        uint8_t seat = (uint8_t)((m->dealer + s) & 3u);
        NGMahjongHand *h = &m->hand[seat];
        uint8_t again = 1;
        while (again) {
            again = 0;
            for (i = 0; i < h->count; i++) {
                uint8_t t = h->tiles[i];
                if (!NGM_IS_BONUS(t)) continue;
                hand_remove(h, t);
                h->flowers = (uint8_t)(h->flowers | (1u << (t - NGM_FLOWER)));
                emit(m, NGM_EV_FLOWER, seat, t, 0, 0);
                draw(m, seat, 1);
                again = 1;
                break;
            }
        }
    }
    zero(m->claim_want, NGM_SEATS); zero(m->claim_arg, NGM_SEATS);
    zero((uint8_t *)&m->result, (uint16_t)sizeof(m->result));
    m->result.winner = NGM_NO_TILE;
    m->turn = m->dealer;
    m->last_discard = NGM_NO_TILE; m->discarder = NGM_NO_TILE;
    m->kong_draw = 0;
    m->first_turn = 1;
    m->turns = 0;
    emit(m, NGM_EV_DEAL, m->dealer, NGM_NO_TILE, m->dealer, 0);
    m->drawn = draw(m, m->dealer, 0);
    m->phase = NGM_PH_TURN;
}

int NEOGEO_USER ng_mahjong_init(NGMahjong *m, const NGMahjongRules *rules, uint32_t seed)
{
    uint8_t s;
    if (!m) return NGM_ERR_ARG;
    zero((uint8_t *)m, (uint16_t)sizeof(NGMahjong));
    if (rules) {
        if (rules->ruleset != NGM_RULES_HK_OLD_STYLE) return NGM_ERR_ARG;
        m->rules.ruleset = rules->ruleset; m->rules.flowers = rules->flowers ? 1u : 0u;
        m->rules.min_faan = rules->min_faan; m->rules.limit_faan = rules->limit_faan;
        m->rules.rounds = rules->rounds; m->rules.base_points = rules->base_points;
        m->rules.start_score = rules->start_score;
    } else {
        ng_mahjong_rules_default(&m->rules);
    }
    if (!m->rules.limit_faan || m->rules.limit_faan > 13u) m->rules.limit_faan = 10;
    if (!m->rules.rounds || m->rules.rounds > 4u) m->rules.rounds = 1;
    if (!m->rules.base_points) m->rules.base_points = 1;
    m->seed = seed ? seed : DEFAULT_SEED;
    m->rng = m->seed;
    for (s = 0; s < NGM_SEATS; s++) { m->score[s] = m->rules.start_score; m->cpu[s] = 1; }
    m->dealer = 0;
    m->round_wind = NGM_EAST;
    m->phase = NGM_PH_START;
    return NGM_OK;
}

void NEOGEO_USER ng_mahjong_set_cpu(NGMahjong *m, uint8_t seat, uint8_t level)
{
    if (!m || seat >= NGM_SEATS) return;
    m->cpu[seat] = (level == NGM_HUMAN) ? NGM_HUMAN : (uint8_t)(level > 2u ? 2u : level);
}

/* --- what a seat may do ------------------------------------------------- */
static uint8_t NEOGEO_USER win_ok(const NGMahjong *m, uint8_t seat, uint8_t tile, uint8_t self_drawn)
{
    NGMahjongResult r;
    if (!ng_mahjong_score(m, seat, tile, self_drawn, &r)) return 0;
    return (uint8_t)(r.faan >= m->rules.min_faan);
}

static uint8_t NEOGEO_USER can_self_kong(const NGMahjong *m, uint8_t seat, uint8_t k)
{
    const NGMahjongHand *h = &m->hand[seat];
    uint8_t j;
    if (!m->wall_left || k >= NGM_KINDS) return 0;
    if (hand_has(h, k) == 4u && h->nmelds < NGM_MELDS_MAX) return 1;
    if (hand_has(h, k) >= 1u)
        for (j = 0; j < h->nmelds; j++)
            if (h->melds[j].type == NGM_MELD_PUNG && h->melds[j].kind == k) return 1;
    return 0;
}

uint16_t NEOGEO_USER ng_mahjong_wants(const NGMahjong *m, uint8_t seat)
{
    const NGMahjongHand *h;
    uint16_t w = 0;
    uint8_t k;
    if (!m || seat >= NGM_SEATS) return 0;
    h = &m->hand[seat];
    if (m->phase == NGM_PH_TURN && seat == m->turn) {
        w |= NGM_CAN_DISCARD;
        if (m->drawn != NGM_NO_TILE && win_ok(m, seat, m->drawn, 1)) w |= NGM_CAN_TSUMO;
        for (k = 0; k < NGM_KINDS; k++) if (can_self_kong(m, seat, k)) { w |= NGM_CAN_KONG_SELF; break; }
        return w;
    }
    if (m->phase != NGM_PH_CLAIM || seat == m->discarder || m->claim_want[seat]) return 0;
    k = m->last_discard;
    if (k >= NGM_KINDS) return 0;
    if (win_ok(m, seat, k, 0)) w |= NGM_CAN_RON;
    if (h->nmelds < NGM_MELDS_MAX) {
        uint8_t have = hand_has(h, k);
        if (have >= 2u) w |= NGM_CAN_PUNG;
        if (have >= 3u && m->wall_left) w |= NGM_CAN_KONG;
        if (seat == next_seat(m->discarder) && k < 27u) {
            uint8_t r = rank_of(k);
            if (r <= 7u && hand_has(h, (uint8_t)(k + 1u)) && hand_has(h, (uint8_t)(k + 2u))) w |= NGM_CAN_CHOW_LOW;
            if (r >= 2u && r <= 8u && hand_has(h, (uint8_t)(k - 1u)) && hand_has(h, (uint8_t)(k + 1u))) w |= NGM_CAN_CHOW_MID;
            if (r >= 3u && hand_has(h, (uint8_t)(k - 2u)) && hand_has(h, (uint8_t)(k - 1u))) w |= NGM_CAN_CHOW_HIGH;
        }
    }
    if (w) w |= NGM_CAN_PASS;
    return w;
}

/* --- payments and the end of a hand -------------------------------------- */
static void NEOGEO_USER settle_win(NGMahjong *m, uint8_t winner, uint8_t from, uint8_t tile, uint8_t self_drawn)
{
    NGMahjongResult *r = &m->result;
    uint8_t s, faan;
    int32_t pts;
    ng_mahjong_score(m, winner, tile, self_drawn, r);
    r->winner = winner; r->from = from; r->tile = tile;
    faan = r->faan;
    pts = (int32_t)m->rules.base_points << faan;
    for (s = 0; s < NGM_SEATS; s++) r->delta[s] = 0;
    for (s = 0; s < NGM_SEATS; s++) {
        int32_t pay;
        if (s == winner) continue;
        pay = self_drawn ? 2 * pts : (s == from ? 2 * pts : pts);
        r->delta[s] -= pay;
        r->delta[winner] += pay;
    }
    for (s = 0; s < NGM_SEATS; s++) m->score[s] += r->delta[s];
    if (!self_drawn) hand_add(&m->hand[winner], tile);
    emit(m, NGM_EV_WIN, winner, tile, from, faan);
    m->phase = NGM_PH_HAND_OVER;
}

static void NEOGEO_USER exhausted(NGMahjong *m)
{
    uint8_t s;
    m->result.winner = NGM_NO_TILE; m->result.from = NGM_NO_TILE; m->result.tile = NGM_NO_TILE;
    m->result.faan = 0; m->result.patterns = 0;
    for (s = 0; s < NGM_SEATS; s++) m->result.delta[s] = 0;
    emit(m, NGM_EV_EXHAUSTED, NGM_NO_TILE, NGM_NO_TILE, 0, 0);
    m->phase = NGM_PH_HAND_OVER;
}

static void NEOGEO_USER mark_taken(NGMahjong *m)
{
    NGMahjongHand *d = &m->hand[m->discarder];
    uint8_t i;
    if (!d->ndiscards) return;
    i = (uint8_t)(d->ndiscards - 1u);
    d->discard_taken[i >> 3] = (uint8_t)(d->discard_taken[i >> 3] | (1u << (i & 7u)));
}

static void NEOGEO_USER add_meld(NGMahjongHand *h, uint8_t type, uint8_t kind, uint8_t from, uint8_t claimed)
{
    NGMahjongMeld *md;
    if (h->nmelds >= NGM_MELDS_MAX) return;
    md = &h->melds[h->nmelds++];
    md->type = type; md->kind = kind; md->from = from; md->claimed = claimed;
}

/* --- turn actions --------------------------------------------------------- */
static int NEOGEO_USER turn_action(NGMahjong *m, uint8_t seat, uint8_t act, uint8_t arg)
{
    NGMahjongHand *h = &m->hand[seat];
    uint16_t w = ng_mahjong_wants(m, seat);
    if (act == NGM_ACT_TSUMO) {
        if (!(w & NGM_CAN_TSUMO)) return NGM_ERR_ILLEGAL;
        settle_win(m, seat, seat, m->drawn, 1);
        return NGM_OK;
    }
    if (act == NGM_ACT_KONG_SELF) {
        uint8_t j;
        if (!(w & NGM_CAN_KONG_SELF) || !can_self_kong(m, seat, arg)) return NGM_ERR_ILLEGAL;
        if (hand_has(h, arg) == 4u) {
            for (j = 0; j < 4u; j++) hand_remove(h, arg);
            add_meld(h, NGM_MELD_KONG_CONCEALED, arg, seat, NGM_NO_TILE);
            emit(m, NGM_EV_KONG_SELF, seat, arg, NGM_MELD_KONG_CONCEALED, 0);
        } else {
            for (j = 0; j < h->nmelds; j++)
                if (h->melds[j].type == NGM_MELD_PUNG && h->melds[j].kind == arg) { h->melds[j].type = NGM_MELD_KONG; break; }
            hand_remove(h, arg);
            emit(m, NGM_EV_KONG_SELF, seat, arg, NGM_MELD_KONG, 0);
        }
        m->first_turn = 0;
        m->drawn = draw(m, seat, 1);
        if (m->drawn == NGM_NO_TILE) { exhausted(m); return NGM_OK; }
        m->kong_draw = 1;
        return NGM_OK;
    }
    if (act == NGM_ACT_DISCARD) {
        uint8_t s;
        if (!(w & NGM_CAN_DISCARD) || arg >= NGM_KINDS || !hand_remove(h, arg)) return NGM_ERR_ILLEGAL;
        if (h->ndiscards < NGM_DISCARDS_MAX) h->discards[h->ndiscards++] = arg;
        m->last_discard = arg;
        m->discarder = seat;
        m->drawn = NGM_NO_TILE;
        m->kong_draw = 0;
        m->turns++;
        m->first_turn = 0;
        emit(m, NGM_EV_DISCARD, seat, arg, 0, 0);
        for (s = 0; s < NGM_SEATS; s++) { m->claim_want[s] = 0; m->claim_arg[s] = 0; }
        m->phase = NGM_PH_CLAIM;
        emit(m, NGM_EV_CLAIM_WINDOW, seat, arg, 0, 0);
        return NGM_OK;
    }
    return NGM_ERR_ILLEGAL;
}

/* All claims in: the winner, else a pung or kong, else a chow, else the
 * next seat draws. */
static void NEOGEO_USER resolve_claims(NGMahjong *m)
{
    uint8_t i, d = m->discarder, k = m->last_discard;
    for (i = 1; i < NGM_SEATS; i++) {
        uint8_t s = (uint8_t)((d + i) & 3u);
        if (m->claim_want[s] == NGM_ACT_RON) { mark_taken(m); settle_win(m, s, d, k, 0); return; }
    }
    for (i = 1; i < NGM_SEATS; i++) {
        uint8_t s = (uint8_t)((d + i) & 3u);
        NGMahjongHand *h = &m->hand[s];
        if (m->claim_want[s] == NGM_ACT_PUNG) {
            hand_remove(h, k); hand_remove(h, k);
            add_meld(h, NGM_MELD_PUNG, k, d, k);
            mark_taken(m);
            emit(m, NGM_EV_CLAIM, s, k, NGM_MELD_PUNG, d);
            m->turn = s; m->drawn = NGM_NO_TILE; m->kong_draw = 0; m->phase = NGM_PH_TURN;
            return;
        }
        if (m->claim_want[s] == NGM_ACT_KONG) {
            hand_remove(h, k); hand_remove(h, k); hand_remove(h, k);
            add_meld(h, NGM_MELD_KONG, k, d, k);
            mark_taken(m);
            emit(m, NGM_EV_CLAIM, s, k, NGM_MELD_KONG, d);
            m->turn = s;
            m->drawn = draw(m, s, 1);
            if (m->drawn == NGM_NO_TILE) { exhausted(m); return; }
            m->kong_draw = 1;
            m->phase = NGM_PH_TURN;
            return;
        }
    }
    {
        uint8_t s = next_seat(d);
        NGMahjongHand *h = &m->hand[s];
        if (m->claim_want[s] == NGM_ACT_CHOW) {
            uint8_t lo = (uint8_t)(k - m->claim_arg[s]);
            uint8_t j;
            for (j = 0; j < 3u; j++) if ((uint8_t)(lo + j) != k) hand_remove(h, (uint8_t)(lo + j));
            add_meld(h, NGM_MELD_CHOW, lo, d, k);
            mark_taken(m);
            emit(m, NGM_EV_CLAIM, s, k, NGM_MELD_CHOW, d);
            m->turn = s; m->drawn = NGM_NO_TILE; m->kong_draw = 0; m->phase = NGM_PH_TURN;
            return;
        }
        /* nobody: the next seat draws */
        m->turn = s;
        m->kong_draw = 0;
        m->drawn = draw(m, s, 0);
        if (m->drawn == NGM_NO_TILE) { exhausted(m); return; }
        m->phase = NGM_PH_TURN;
    }
}

static int NEOGEO_USER claim_action(NGMahjong *m, uint8_t seat, uint8_t act, uint8_t arg)
{
    uint16_t w = ng_mahjong_wants(m, seat);
    uint16_t need = 0;
    if (!w) return NGM_ERR_TURN;
    switch (act) {
    case NGM_ACT_PASS: need = NGM_CAN_PASS; break;
    case NGM_ACT_RON: need = NGM_CAN_RON; break;
    case NGM_ACT_PUNG: need = NGM_CAN_PUNG; break;
    case NGM_ACT_KONG: need = NGM_CAN_KONG; break;
    case NGM_ACT_CHOW:
        need = (uint16_t)(arg == 0u ? NGM_CAN_CHOW_LOW : arg == 1u ? NGM_CAN_CHOW_MID : arg == 2u ? NGM_CAN_CHOW_HIGH : 0u);
        break;
    default: return NGM_ERR_ILLEGAL;
    }
    if (!need || !(w & need)) return NGM_ERR_ILLEGAL;
    m->claim_want[seat] = act;
    m->claim_arg[seat] = arg;
    return NGM_OK;
}

int NEOGEO_USER ng_mahjong_command(NGMahjong *m, uint8_t seat, uint8_t action, uint8_t arg)
{
    if (!m || seat >= NGM_SEATS) return NGM_ERR_ARG;
    if (m->phase == NGM_PH_HAND_OVER || m->phase == NGM_PH_GAME_OVER) return NGM_ERR_STATE;
    if (m->phase == NGM_PH_TURN) {
        if (seat != m->turn) return NGM_ERR_TURN;
        return turn_action(m, seat, action, arg);
    }
    if (m->phase == NGM_PH_CLAIM) return claim_action(m, seat, action, arg);
    return NGM_ERR_TURN;
}

/* --- the view and the CPU ------------------------------------------------- */
void NEOGEO_USER ng_mahjong_view(const NGMahjong *m, uint8_t seat, NGMahjongView *v)
{
    uint8_t s, i;
    const NGMahjongHand *h;
    if (!m || !v || seat >= NGM_SEATS) return;
    zero((uint8_t *)v, (uint16_t)sizeof(NGMahjongView));
    h = &m->hand[seat];
    v->seat = seat; v->turn = m->turn; v->phase = m->phase; v->dealer = m->dealer;
    v->seat_wind = ng_mahjong_seat_wind(m, seat); v->round_wind = m->round_wind;
    v->last_discard = m->last_discard; v->discarder = m->discarder;
    v->drawn = (m->turn == seat) ? m->drawn : NGM_NO_TILE;
    v->wall_left = m->wall_left;
    v->wants = ng_mahjong_wants(m, seat);
    v->min_faan = m->rules.min_faan;
    for (i = 0; i < h->count; i++) v->counts[h->tiles[i]]++;
    v->nmelds = h->nmelds;
    for (i = 0; i < h->nmelds; i++) {
        v->melds[i].type = h->melds[i].type; v->melds[i].kind = h->melds[i].kind;
        v->melds[i].from = h->melds[i].from; v->melds[i].claimed = h->melds[i].claimed;
    }
    for (i = 0; i < NGM_KINDS; i++) v->seen[i] = v->counts[i];
    for (s = 0; s < NGM_SEATS; s++) {
        const NGMahjongHand *o = &m->hand[s];
        for (i = 0; i < o->ndiscards; i++) {
            uint8_t k = o->discards[i];
            if (k < NGM_KINDS && !(o->discard_taken[i >> 3] & (1u << (i & 7u)))) v->seen[k]++;
        }
        for (i = 0; i < o->nmelds; i++) {
            const NGMahjongMeld *md = &o->melds[i];
            if (md->type == NGM_MELD_KONG_CONCEALED && s != seat) continue;   /* face down */
            if (md->type == NGM_MELD_CHOW) {
                v->seen[md->kind]++; v->seen[md->kind + 1u]++; v->seen[md->kind + 2u]++;
            } else {
                v->seen[md->kind] = (uint8_t)(v->seen[md->kind] + (md->type == NGM_MELD_PUNG ? 3u : 4u));
            }
        }
    }
}

/* How much a tile is worth keeping (lower: discard it first). */
static int16_t NEOGEO_USER keep_value(const NGMahjongView *v, uint8_t k)
{
    int16_t val = 0;
    uint8_t c = v->counts[k];
    if (k >= 27u) {
        uint8_t valued = (uint8_t)(k >= NGM_RED || k == v->seat_wind || k == v->round_wind);
        val = (int16_t)(c >= 2u ? 40 : valued ? 8 : 0);
        val = (int16_t)(val - (int16_t)v->seen[k] * 6);
    } else {
        uint8_t r = rank_of(k);
        int16_t near = 0;
        if (r >= 2u && v->counts[k - 1u]) near += 12;
        if (r <= 8u && v->counts[k + 1u]) near += 12;
        if (r >= 3u && v->counts[k - 2u]) near += 6;
        if (r <= 7u && v->counts[k + 2u]) near += 6;
        val = (int16_t)(near + (c >= 2u ? 20 : 0) + ((r >= 3u && r <= 7u) ? 6 : (r == 2u || r == 8u) ? 3 : 0));
    }
    return val;
}

static int8_t NEOGEO_USER shanten_after(const uint8_t *counts, uint8_t melds, uint8_t drop)
{
    uint8_t c[NGM_KINDS], i;
    for (i = 0; i < NGM_KINDS; i++) c[i] = counts[i];
    if (drop < NGM_KINDS && c[drop]) c[drop]--;
    return ng_mahjong_shanten(c, melds);
}

/* Best discard from 14 (or 11, 8...) tiles: lowest shanten, then lowest keep value. */
static uint8_t NEOGEO_USER best_discard(const NGMahjongView *v, uint8_t level, int8_t *out_shanten)
{
    uint8_t k, best = NGM_NO_TILE;
    int8_t bs = 9;
    int16_t bv = 0x7FFF;
    for (k = 0; k < NGM_KINDS; k++) {
        int8_t s;
        int16_t val;
        if (!v->counts[k]) continue;
        s = shanten_after(v->counts, v->nmelds, k);
        val = keep_value(v, k);
        if (level >= 2u) val = (int16_t)(val - (int16_t)v->seen[k] * 4);   /* prefer tiles others threw */
        if (s < bs || (s == bs && val < bv)) { bs = s; bv = val; best = k; }
    }
    if (out_shanten) *out_shanten = bs;
    return best;
}

void NEOGEO_USER ng_mahjong_cpu_choose(const NGMahjongView *v, uint8_t level, uint8_t *action, uint8_t *arg)
{
    uint8_t act = NGM_ACT_PASS, a = 0;
    if (!v) return;
    if (v->phase == NGM_PH_TURN && v->turn == v->seat) {
        int8_t s;
        uint8_t k;
        if (v->wants & NGM_CAN_TSUMO) { act = NGM_ACT_TSUMO; goto done; }
        a = best_discard(v, level, &s);
        act = NGM_ACT_DISCARD;
        if ((v->wants & NGM_CAN_KONG_SELF) && level >= 1u) {
            for (k = 0; k < NGM_KINDS; k++) {
                uint8_t c[NGM_KINDS], i, melds = v->nmelds, j, added = 0;
                if (!v->counts[k]) continue;
                for (j = 0; j < v->nmelds; j++) if (v->melds[j].type == NGM_MELD_PUNG && v->melds[j].kind == k) added = 1;
                if (v->counts[k] != 4u && !added) continue;
                for (i = 0; i < NGM_KINDS; i++) c[i] = v->counts[i];
                if (added) { c[k]--; } else { c[k] = 0; melds++; }
                /* a kong is worth it when the hand isn't worse without those tiles */
                if (ng_mahjong_shanten(c, melds) <= s) { act = NGM_ACT_KONG_SELF; a = k; break; }
            }
        }
        goto done;
    }
    if (v->phase == NGM_PH_CLAIM && v->wants) {
        uint8_t k = v->last_discard, c[NGM_KINDS], i;
        int8_t now, after;
        if (v->wants & NGM_CAN_RON) { act = NGM_ACT_RON; goto done; }
        for (i = 0; i < NGM_KINDS; i++) c[i] = v->counts[i];
        now = ng_mahjong_shanten(c, v->nmelds);
        if (level == 0u) goto done;   /* the easy CPU never calls */
        if (v->wants & (NGM_CAN_PUNG | NGM_CAN_KONG)) {
            c[k] = (uint8_t)(c[k] - 2u);
            after = 9;
            {   /* after the pung it must discard: the best it could then reach */
                uint8_t d;
                for (d = 0; d < NGM_KINDS; d++) {
                    int8_t s;
                    if (!c[d]) continue;
                    c[d]--;
                    s = ng_mahjong_shanten(c, (uint8_t)(v->nmelds + 1u));
                    c[d]++;
                    if (s < after) after = s;
                }
            }
            c[k] = (uint8_t)(c[k] + 2u);
            if (after < now || (k >= 27u && (k >= NGM_RED || k == v->seat_wind || k == v->round_wind) && after <= now)) {
                act = NGM_ACT_PUNG;
                goto done;
            }
        }
        if (v->wants & (NGM_CAN_CHOW_LOW | NGM_CAN_CHOW_MID | NGM_CAN_CHOW_HIGH)) {
            static const uint16_t bit[3] = { NGM_CAN_CHOW_LOW, NGM_CAN_CHOW_MID, NGM_CAN_CHOW_HIGH };
            uint8_t var;
            for (var = 0; var < 3u; var++) {
                uint8_t lo, j, d;
                if (!(v->wants & bit[var])) continue;
                lo = (uint8_t)(k - var);
                for (j = 0; j < 3u; j++) if ((uint8_t)(lo + j) != k) c[lo + j]--;
                after = 9;
                for (d = 0; d < NGM_KINDS; d++) {
                    int8_t s;
                    if (!c[d]) continue;
                    c[d]--;
                    s = ng_mahjong_shanten(c, (uint8_t)(v->nmelds + 1u));
                    c[d]++;
                    if (s < after) after = s;
                }
                for (j = 0; j < 3u; j++) if ((uint8_t)(lo + j) != k) c[lo + j]++;
                if (after < now && level >= 1u) { act = NGM_ACT_CHOW; a = var; goto done; }
            }
        }
    }
done:
    if (action) *action = act;
    if (arg) *arg = a;
}

static void NEOGEO_USER cpu_act(NGMahjong *m, uint8_t seat)
{
    NGMahjongView v;
    uint8_t act, arg;
    ng_mahjong_view(m, seat, &v);
    ng_mahjong_cpu_choose(&v, m->cpu[seat], &act, &arg);
    if (m->cpu[seat] == 0u && act == NGM_ACT_DISCARD && range16(&m->rng, 4) == 0) {
        /* the easy CPU sometimes throws a random tile */
        const NGMahjongHand *h = &m->hand[seat];
        arg = h->tiles[range16(&m->rng, h->count)];
    }
    if (ng_mahjong_command(m, seat, act, arg) != NGM_OK) {
        /* never stall: fall back to a plain discard or a pass */
        if (m->phase == NGM_PH_TURN) ng_mahjong_command(m, seat, NGM_ACT_DISCARD, m->hand[seat].tiles[m->hand[seat].count - 1u]);
        else ng_mahjong_command(m, seat, NGM_ACT_PASS, 0);
    }
}

void NEOGEO_USER ng_mahjong_update(NGMahjong *m)
{
    if (!m) return;
    switch (m->phase) {
    case NGM_PH_START:
        deal(m);
        if (m->cpu[m->turn] == NGM_HUMAN) emit(m, NGM_EV_TURN, m->turn, m->drawn, 0, 0);
        break;
    case NGM_PH_TURN:
        if (m->cpu[m->turn] != NGM_HUMAN) cpu_act(m, m->turn);
        break;
    case NGM_PH_CLAIM: {
        uint8_t s, waiting = 0;
        for (s = 0; s < NGM_SEATS; s++) {
            uint16_t w;
            if (s == m->discarder || m->claim_want[s]) continue;
            w = ng_mahjong_wants(m, s);
            if (!w) { m->claim_want[s] = NGM_ACT_PASS; continue; }
            if (m->cpu[s] != NGM_HUMAN) cpu_act(m, s);
            else waiting = 1;
        }
        if (waiting) break;
        resolve_claims(m);
        if (m->phase == NGM_PH_TURN && m->cpu[m->turn] == NGM_HUMAN) emit(m, NGM_EV_TURN, m->turn, m->drawn, 0, 0);
        break;
    }
    default:
        break;
    }
}

int NEOGEO_USER ng_mahjong_next_hand(NGMahjong *m)
{
    if (!m || m->phase != NGM_PH_HAND_OVER) return NGM_ERR_STATE;
    m->hand_no++;
    if (m->result.winner != m->dealer && m->result.winner != NGM_NO_TILE) {
        m->dealer = next_seat(m->dealer);
        m->dealer_moves++;
        if (!(m->dealer_moves & 3u)) {
            m->round_wind++;
            if ((uint8_t)(m->round_wind - NGM_EAST) >= m->rules.rounds) {
                m->phase = NGM_PH_GAME_OVER;
                emit(m, NGM_EV_GAME_OVER, NGM_NO_TILE, NGM_NO_TILE, 0, 0);
                return NGM_ERR_STATE;
            }
        }
    }
    m->phase = NGM_PH_START;
    return NGM_OK;
}

uint8_t NEOGEO_USER ng_mahjong_event(NGMahjong *m, NGMahjongEvent *out)
{
    NGMahjongEvent *e;
    if (!m || !m->ev_count) return 0;
    e = &m->ev[m->ev_head];
    if (out) { out->type = e->type; out->seat = e->seat; out->tile = e->tile; out->a = e->a; out->b = e->b; out->pad = 0; }
    m->ev_head = (uint8_t)((m->ev_head + 1u) & (NGM_EVENT_QUEUE - 1u));
    m->ev_count--;
    return 1;
}

uint8_t NEOGEO_USER ng_mahjong_tiles_ok(const NGMahjong *m)
{
    uint8_t c[NGM_KINDS_ALL], s, i, k;
    if (!m) return 0;
    zero(c, NGM_KINDS_ALL);
    if (m->phase != NGM_PH_START) {
        for (i = 0; i < m->wall_left; i++) {
            uint8_t t = m->wall[(uint8_t)(m->wall_head + i)];
            if (t < NGM_KINDS_ALL) c[t]++;
        }
    }
    for (s = 0; s < NGM_SEATS; s++) {
        const NGMahjongHand *h = &m->hand[s];
        for (i = 0; i < h->count; i++) if (h->tiles[i] < NGM_KINDS) c[h->tiles[i]]++;
        for (i = 0; i < h->nmelds; i++) {
            const NGMahjongMeld *md = &h->melds[i];
            if (md->type == NGM_MELD_CHOW) { c[md->kind]++; c[md->kind + 1u]++; c[md->kind + 2u]++; }
            else c[md->kind] = (uint8_t)(c[md->kind] + (md->type == NGM_MELD_PUNG ? 3u : 4u));
        }
        for (i = 0; i < h->ndiscards; i++)
            if (!(h->discard_taken[i >> 3] & (1u << (i & 7u)))) c[h->discards[i]]++;
        for (i = 0; i < 8u; i++) if (h->flowers & (1u << i)) c[NGM_FLOWER + i]++;
    }
    if (m->phase == NGM_PH_START) return 1;
    /* a discard won on is in the winner's hand already */
    for (k = 0; k < NGM_KINDS; k++) if (c[k] != 4u) return 0;
    for (k = NGM_FLOWER; k < NGM_KINDS_ALL; k++) if (c[k] != (m->rules.flowers ? 1u : 0u)) return 0;
    return 1;
}

/* ======================================================================
 * hash, save, load
 * ==================================================================== */
static uint32_t NEOGEO_USER fnv(uint32_t h, const uint8_t *d, uint16_t n)
{
    while (n--) {
        h ^= *d++;
        h += (h << 1) + (h << 4) + (h << 7) + (h << 8) + (h << 24);
    }
    return h;
}

typedef struct { uint8_t *b; uint16_t o, size; const uint8_t *rb; uint8_t err, hashing; uint32_t h; } Ser;

static void NEOGEO_USER w8(Ser *s, uint8_t v)
{
    if (s->hashing) { s->h = fnv(s->h, &v, 1); s->o++; return; }
    if (s->o < s->size) s->b[s->o] = v; else s->err = 1;
    s->o++;
}
static void NEOGEO_USER w16(Ser *s, uint16_t v) { w8(s, (uint8_t)(v >> 8)); w8(s, (uint8_t)v); }
static void NEOGEO_USER w32(Ser *s, uint32_t v) { w16(s, (uint16_t)(v >> 16)); w16(s, (uint16_t)v); }
static void NEOGEO_USER wn(Ser *s, const uint8_t *d, uint16_t n) { uint16_t i; for (i = 0; i < n; i++) w8(s, d[i]); }
static uint8_t NEOGEO_USER r8(Ser *s) { uint8_t v = 0; if (s->o < s->size) v = s->rb[s->o]; else s->err = 1; s->o++; return v; }
static uint16_t NEOGEO_USER r16(Ser *s) { uint16_t v = (uint16_t)((uint16_t)r8(s) << 8); return (uint16_t)(v | r8(s)); }
static uint32_t NEOGEO_USER r32(Ser *s) { uint32_t v = (uint32_t)r16(s) << 16; return v | r16(s); }
static void NEOGEO_USER rn(Ser *s, uint8_t *d, uint16_t n) { uint16_t i; for (i = 0; i < n; i++) d[i] = r8(s); }

/* One routine walks the fields both ways, so save and load can't drift. */
static void NEOGEO_USER walk_fields(NGMahjong *m, Ser *s, uint8_t load)
{
    uint8_t i, j;
#define F8(x)  do { if (load) (x) = r8(s); else w8(s, (x)); } while (0)
#define F16(x) do { if (load) (x) = r16(s); else w16(s, (x)); } while (0)
#define F32(x) do { if (load) (x) = r32(s); else w32(s, (uint32_t)(x)); } while (0)
#define FN(p, n) do { if (load) rn(s, (p), (n)); else wn(s, (p), (n)); } while (0)
    F8(m->rules.ruleset); F8(m->rules.flowers); F8(m->rules.min_faan); F8(m->rules.limit_faan);
    F8(m->rules.rounds); F16(m->rules.base_points);
    if (load) m->rules.start_score = (int32_t)r32(s); else w32(s, (uint32_t)m->rules.start_score);
    for (i = 0; i < NGM_SEATS; i++) {
        NGMahjongHand *h = &m->hand[i];
        FN(h->tiles, NGM_HAND_MAX); F8(h->count); F8(h->nmelds);
        for (j = 0; j < NGM_MELDS_MAX; j++) { F8(h->melds[j].type); F8(h->melds[j].kind); F8(h->melds[j].from); F8(h->melds[j].claimed); }
        F8(h->flowers); F8(h->ndiscards); FN(h->discards, NGM_DISCARDS_MAX); FN(h->discard_taken, (NGM_DISCARDS_MAX + 7u) / 8u);
        if (load) m->score[i] = (int32_t)r32(s); else w32(s, (uint32_t)m->score[i]);
        F8(m->cpu[i]); F8(m->claim_want[i]); F8(m->claim_arg[i]);
        if (load) m->result.delta[i] = (int32_t)r32(s); else w32(s, (uint32_t)m->result.delta[i]);
    }
    FN(m->wall, NGM_WALL_MAX);
    F8(m->wall_size); F8(m->wall_head); F8(m->wall_tail); F8(m->wall_left);
    F8(m->phase); F8(m->turn); F8(m->dealer); F8(m->round_wind); F8(m->hand_no); F8(m->dealer_moves);
    F8(m->last_discard); F8(m->discarder); F8(m->drawn); F8(m->kong_draw); F8(m->first_turn);
    F8(m->result.winner); F8(m->result.from); F8(m->result.tile); F8(m->result.faan); F32(m->result.patterns);
    F32(m->seed); F32(m->rng); F16(m->turns);
#undef F8
#undef F16
#undef F32
#undef FN
}

uint32_t NEOGEO_USER ng_mahjong_hash(const NGMahjong *m)
{
    Ser s;
    if (!m) return 0;
    s.b = 0; s.o = 0; s.size = 0; s.rb = 0; s.err = 0; s.hashing = 1; s.h = 2166136261u;
    walk_fields((NGMahjong *)m, &s, 0);
    return s.h;
}

uint16_t NEOGEO_USER ng_mahjong_save(const NGMahjong *m, uint8_t *buf, uint16_t size)
{
    Ser s;
    if (!m || !buf || size < 8u) return 0;
    s.b = buf; s.o = 0; s.size = size; s.rb = 0; s.err = 0; s.hashing = 0; s.h = 0;
    w32(&s, SAVE_MAGIC); w8(&s, SAVE_VERSION);
    walk_fields((NGMahjong *)m, &s, 0);
    return s.err ? 0 : s.o;
}

int NEOGEO_USER ng_mahjong_load(NGMahjong *m, const uint8_t *buf, uint16_t size)
{
    Ser s;
    if (!m || !buf || size < 8u) return NGM_ERR_ARG;
    s.b = 0; s.o = 0; s.size = size; s.rb = buf; s.err = 0; s.hashing = 0; s.h = 0;
    if (r32(&s) != SAVE_MAGIC || r8(&s) != SAVE_VERSION) return NGM_ERR_SAVE;
    zero((uint8_t *)m, (uint16_t)sizeof(NGMahjong));
    walk_fields(m, &s, 1);
    if (s.err || m->rules.ruleset != NGM_RULES_HK_OLD_STYLE || m->phase > NGM_PH_GAME_OVER) return NGM_ERR_SAVE;
    return NGM_OK;
}

/* ======================================================================
 * solitaire
 * ==================================================================== */
static uint8_t NEOGEO_USER sol_gone(const uint8_t *g, uint8_t i) { return (uint8_t)((g[i >> 3] >> (i & 7u)) & 1u); }
static void NEOGEO_USER sol_set(uint8_t *g, uint8_t i) { g[i >> 3] = (uint8_t)(g[i >> 3] | (1u << (i & 7u))); }
static void NEOGEO_USER sol_clr(uint8_t *g, uint8_t i) { g[i >> 3] = (uint8_t)(g[i >> 3] & ~(1u << (i & 7u))); }

/* The row index: positions sorted by row (y), and where each row starts. */
static void NEOGEO_USER sol_rows(NGMahjongSol *s)
{
    uint8_t r, i, k = 0;
    for (r = 0; r < NGMS_ROWS; r++) {
        s->row_first[r] = k;
        for (i = 0; i < s->n; i++) if (s->pos[i].y == r) s->by_row[k++] = i;
    }
    s->row_first[NGMS_ROWS] = k;
}

/* The part of the row index that can meet a tile in row y: rows y-1 to
 * y+1 (two half-tile rows apart, tiles no longer overlap). */
static uint8_t NEOGEO_USER rows_from(const NGMahjongSol *s, uint8_t y)
{
    return s->row_first[y ? y - 1u : 0u];
}

static uint8_t NEOGEO_USER rows_to(const NGMahjongSol *s, uint8_t y)
{
    return s->row_first[(uint8_t)(y + 2u) < NGMS_ROWS ? (uint8_t)(y + 2u) : NGMS_ROWS];
}

/* s->cover and s->side for the tiles not in g: the tiles on top of each
 * position (any higher layer, overlapping it) and beside it (its own
 * layer, overlapping rows, touching on the left: low four bits; on the
 * right: high four). Every position is counted, present or not, so a
 * tile put back has its counts ready. */
static void NEOGEO_USER sol_count(NGMahjongSol *s, const uint8_t *g)
{
    uint8_t i;
    for (i = 0; i < s->n; i++) {
        const NGMahjongPos *p = &s->pos[i];
        uint8_t px = p->x, pz = p->z, cover = 0, side = 0, k, end;
        end = rows_to(s, p->y);
        for (k = rows_from(s, p->y); k < end; k++) {
            uint8_t j = s->by_row[k];
            const NGMahjongPos *q;
            if (j == i || sol_gone(g, j)) continue;
            q = &s->pos[j];
            if (q->z > pz) { if ((uint8_t)(q->x - px + 1u) < 3u) cover++; }
            else if (q->z == pz) {
                if ((uint8_t)(q->x + 2u) == px) side = (uint8_t)(side + 0x01u);
                else if ((uint8_t)(px + 2u) == q->x) side = (uint8_t)(side + 0x10u);
            }
        }
        s->cover[i] = cover;
        s->side[i] = side;
    }
}

/* Tile t leaves g (or comes back, back != 0): the counts around it follow. */
static void NEOGEO_USER sol_move(NGMahjongSol *s, uint8_t *g, uint8_t t, uint8_t back)
{
    const NGMahjongPos *tp = &s->pos[t];
    uint8_t tx = tp->x, tz = tp->z, k, end = rows_to(s, tp->y);
    for (k = rows_from(s, tp->y); k < end; k++) {
        uint8_t j = s->by_row[k];
        const NGMahjongPos *q;
        if (j == t) continue;
        q = &s->pos[j];
        if (tz > q->z) {                                   /* t lies on j */
            if ((uint8_t)(tx - q->x + 1u) < 3u) s->cover[j] = (uint8_t)(back ? s->cover[j] + 1u : s->cover[j] - 1u);
        } else if (tz == q->z) {
            uint8_t d = (uint8_t)((uint8_t)(tx + 2u) == q->x ? 0x01u : (uint8_t)(q->x + 2u) == tx ? 0x10u : 0u);
            if (d) s->side[j] = (uint8_t)(back ? s->side[j] + d : s->side[j] - d);
        }
    }
    if (back) sol_clr(g, t);
    else sol_set(g, t);
}

/* Free in the set the counts were made for (g): nothing on top, a side open. */
static uint8_t NEOGEO_USER free_in(const NGMahjongSol *s, const uint8_t *g, uint8_t i)
{
    if (sol_gone(g, i) || s->cover[i]) return 0;
    return (uint8_t)(!(s->side[i] & 0x0Fu) || !(s->side[i] & 0xF0u));
}

uint8_t NEOGEO_USER ng_mjsol_is_free(const NGMahjongSol *s, uint8_t i)
{
    if (!s || i >= s->n) return 0;
    return free_in(s, s->gone, i);
}

void NEOGEO_USER ng_mjsol_recount(NGMahjongSol *s)
{
    if (!s || !s->pos || s->n > NGMS_MAX_TILES) return;
    sol_rows(s);
    sol_count(s, s->gone);
}

uint8_t NEOGEO_USER ng_mjsol_match(uint8_t a, uint8_t b)
{
    if (a >= NGM_KINDS_ALL || b >= NGM_KINDS_ALL) return 0;
    if (a == b) return 1;
    if (a >= NGM_FLOWER && a < NGM_SEASON && b >= NGM_FLOWER && b < NGM_SEASON) return 1;
    if (a >= NGM_SEASON && b >= NGM_SEASON) return 1;
    return 0;
}

uint8_t NEOGEO_USER ng_mjsol_layout_turtle(NGMahjongPos *out)
{
    static const uint8_t widths[8] = { 12, 8, 10, 12, 12, 10, 8, 12 };
    uint8_t n = 0, row, i, layer;
    if (!out) return 0;
    for (row = 0; row < 8u; row++) {
        uint8_t x0 = (uint8_t)(2u + (12u - widths[row]));     /* centred: (12 - w) / 2 tiles of 2 units */
        for (i = 0; i < widths[row]; i++) { out[n].x = (uint8_t)(x0 + 2u * i); out[n].y = (uint8_t)(row * 2u); out[n].z = 0; n++; }
    }
    out[n].x = 0; out[n].y = 7; out[n].z = 0; n++;
    out[n].x = 26; out[n].y = 7; out[n].z = 0; n++;
    out[n].x = 28; out[n].y = 7; out[n].z = 0; n++;
    for (layer = 1; layer <= 3u; layer++) {
        uint8_t side = (uint8_t)(8u - 2u * layer);           /* 6, 4, 2 tiles */
        uint8_t x0 = (uint8_t)(2u + (12u - side)), y0 = (uint8_t)(8u - side);
        uint8_t r, c;
        for (r = 0; r < side; r++)
            for (c = 0; c < side; c++) { out[n].x = (uint8_t)(x0 + 2u * c); out[n].y = (uint8_t)(y0 + 2u * r); out[n].z = layer; n++; }
    }
    out[n].x = 13; out[n].y = 7; out[n].z = 4; n++;
    return n;
}

static void NEOGEO_USER sol_emit(NGMahjongSol *s, uint8_t type, uint8_t a, uint8_t b)
{
    NGMahjongSolEvent *e;
    if (s->ev_count >= 16u) return;
    e = &s->ev[(s->ev_head + s->ev_count) & 15u];
    e->type = type; e->a = a; e->b = b; e->pad = 0;
    s->ev_count++;
}

/* Lay the given kinds (in pairs: kinds[2p], kinds[2p+1]) over the
 * positions not in `skip`, by removing random free pairs from the full
 * layout: the reverse of that order clears the deal. The counts follow
 * the removals, so each pair costs one pass over the layout; they are
 * made again for s->gone at the end. */
static uint8_t NEOGEO_USER lay(NGMahjongSol *s, const uint8_t *kinds, uint8_t count, const uint8_t *skip)
{
    uint8_t g[(NGMS_MAX_TILES + 7u) / 8u], freev[NGMS_MAX_TILES], attempt, i, done = 0;
    for (attempt = 0; attempt < 64u && !done; attempt++) {
        uint8_t p, ok = 1;
        for (i = 0; i < (uint8_t)sizeof(g); i++) g[i] = skip[i];
        sol_count(s, g);
        for (p = 0; p < (uint8_t)(count >> 1); p++) {
            uint8_t nf = 0, a, b, j;
            for (j = 0; j < s->n; j++) if (free_in(s, g, j)) freev[nf++] = j;
            if (nf < 2u) { ok = 0; break; }
            a = (uint8_t)range16(&s->rng, nf);
            b = (uint8_t)range16(&s->rng, (uint16_t)(nf - 1u));
            if (b >= a) b++;
            a = freev[a]; b = freev[b];
            s->kind[a] = kinds[2u * p]; s->kind[b] = kinds[2u * p + 1u];
            s->solution[2u * p] = a; s->solution[2u * p + 1u] = b;
            sol_move(s, g, a, 0); sol_move(s, g, b, 0);
        }
        done = ok;
    }
    sol_count(s, s->gone);
    return done;
}

static void NEOGEO_USER shuffle_pairs(NGMahjongSol *s, uint8_t *kinds, uint8_t count)
{
    uint8_t p, np = (uint8_t)(count >> 1);
    for (p = (uint8_t)(np - 1u); p > 0; p--) {
        uint8_t q = (uint8_t)range16(&s->rng, (uint16_t)(p + 1u)), t0 = kinds[2u * p], t1 = kinds[2u * p + 1u];
        kinds[2u * p] = kinds[2u * q]; kinds[2u * p + 1u] = kinds[2u * q + 1u];
        kinds[2u * q] = t0; kinds[2u * q + 1u] = t1;
    }
}

int NEOGEO_USER ng_mjsol_init(NGMahjongSol *s, const NGMahjongPos *pos, uint8_t n, uint32_t seed)
{
    uint8_t kinds[NGMS_MAX_TILES], skip[(NGMS_MAX_TILES + 7u) / 8u], i;
    if (!s || !pos || n < 2u || (n & 1u) || n > NGMS_MAX_TILES) return NGM_ERR_LAYOUT;
    for (i = 0; i < n; i++) if (pos[i].y >= NGMS_ROWS) return NGM_ERR_LAYOUT;
    zero((uint8_t *)s, (uint16_t)sizeof(NGMahjongSol));
    s->pos = pos; s->n = n;
    sol_rows(s);
    s->seed = seed ? seed : DEFAULT_SEED;
    s->rng = s->seed;
    s->selected = NGMS_NONE;
    /* the tile set: a full set for 144, otherwise pairs of the 34 kinds in turn */
    if (n == NGMS_MAX_TILES) {
        uint8_t k, c, o = 0;
        for (k = 0; k < NGM_KINDS; k++) for (c = 0; c < 4u; c++) kinds[o++] = k;
        for (k = NGM_FLOWER; k < NGM_KINDS_ALL; k++) kinds[o++] = k;
    } else {
        for (i = 0; i < (uint8_t)(n >> 1); i++) {
            uint8_t k = i;
            while (k >= NGM_KINDS) k = (uint8_t)(k - NGM_KINDS);
            kinds[2u * i] = k; kinds[2u * i + 1u] = k;
        }
    }
    shuffle_pairs(s, kinds, n);
    zero(skip, sizeof(skip));
    if (!lay(s, kinds, n, skip)) return NGM_ERR_LAYOUT;
    s->left = n;
    s->status = NGMS_PLAYING;
    return NGM_OK;
}

uint8_t NEOGEO_USER ng_mjsol_hint(const NGMahjongSol *s, uint8_t *a, uint8_t *b)
{
    uint8_t fr[NGMS_MAX_TILES], nf = 0, i, j;
    if (!s) return 0;
    for (i = 0; i < s->n; i++) if (free_in(s, s->gone, i)) fr[nf++] = i;
    for (i = 0; i < nf; i++)
        for (j = (uint8_t)(i + 1u); j < nf; j++)
            if (ng_mjsol_match(s->kind[fr[i]], s->kind[fr[j]])) {
                if (a) *a = fr[i];
                if (b) *b = fr[j];
                return 1;
            }
    return 0;
}

uint8_t NEOGEO_USER ng_mjsol_free_pairs(const NGMahjongSol *s)
{
    uint8_t fr[NGMS_MAX_TILES], nf = 0, i, j, pairs = 0;
    if (!s) return 0;
    for (i = 0; i < s->n; i++) if (free_in(s, s->gone, i)) fr[nf++] = i;
    for (i = 0; i < nf; i++)
        for (j = (uint8_t)(i + 1u); j < nf; j++)
            if (ng_mjsol_match(s->kind[fr[i]], s->kind[fr[j]]) && pairs < 255u) pairs++;
    return pairs;
}

int NEOGEO_USER ng_mjsol_select(NGMahjongSol *s, uint8_t i)
{
    uint8_t sel;
    if (!s || i >= s->n || sol_gone(s->gone, i)) return NGM_ERR_ARG;
    if (s->status == NGMS_CLEARED) return NGM_ERR_STATE;
    if (!free_in(s, s->gone, i)) { sol_emit(s, NGMS_EV_BLOCKED, i, 0); return NGM_ERR_ILLEGAL; }
    sel = s->selected;
    if (sel == NGMS_NONE) { s->selected = i; sol_emit(s, NGMS_EV_SELECT, i, 0); return NGM_OK; }
    if (sel == i) { s->selected = NGMS_NONE; sol_emit(s, NGMS_EV_DESELECT, i, 0); return NGM_OK; }
    if (!ng_mjsol_match(s->kind[sel], s->kind[i])) {
        sol_emit(s, NGMS_EV_MISMATCH, sel, i);
        s->selected = i;
        return NGM_ERR_ILLEGAL;
    }
    sol_move(s, s->gone, sel, 0); sol_move(s, s->gone, i, 0);
    s->left = (uint8_t)(s->left - 2u);
    if (s->undo_n + 2u <= NGMS_MAX_TILES) { s->undo[s->undo_n++] = sel; s->undo[s->undo_n++] = i; }
    s->pairs_removed++;
    s->selected = NGMS_NONE;
    sol_emit(s, NGMS_EV_PAIR, sel, i);
    if (!s->left) { s->status = NGMS_CLEARED; sol_emit(s, NGMS_EV_CLEARED, 0, 0); }
    else if (!ng_mjsol_hint(s, 0, 0)) { s->status = NGMS_STUCK; sol_emit(s, NGMS_EV_STUCK, 0, 0); }
    return NGM_OK;
}

int NEOGEO_USER ng_mjsol_undo(NGMahjongSol *s)
{
    uint8_t a, b;
    if (!s || s->undo_n < 2u) return NGM_ERR_STATE;
    b = s->undo[--s->undo_n]; a = s->undo[--s->undo_n];
    sol_move(s, s->gone, a, 1); sol_move(s, s->gone, b, 1);
    s->left = (uint8_t)(s->left + 2u);
    s->selected = NGMS_NONE;
    s->status = NGMS_PLAYING;
    s->undos++;
    sol_emit(s, NGMS_EV_UNDO, a, b);
    return NGM_OK;
}

int NEOGEO_USER ng_mjsol_shuffle(NGMahjongSol *s)
{
    uint8_t kinds[NGMS_MAX_TILES], cnt = 0, i, j;
    if (!s || !s->left) return NGM_ERR_STATE;
    /* the tiles left, sorted so that matching ones sit next to each other */
    for (i = 0; i < s->n; i++) if (!sol_gone(s->gone, i)) kinds[cnt++] = s->kind[i];
    for (i = 1; i < cnt; i++) {
        uint8_t k = kinds[i];
        j = i;
        while (j > 0 && kinds[j - 1u] > k) { kinds[j] = kinds[j - 1u]; j--; }
        kinds[j] = k;
    }
    for (i = 0; i + 1u < cnt; i = (uint8_t)(i + 2u))
        if (!ng_mjsol_match(kinds[i], kinds[i + 1u])) return NGM_ERR_LAYOUT;
    shuffle_pairs(s, kinds, cnt);
    if (!lay(s, kinds, cnt, s->gone)) return NGM_ERR_LAYOUT;
    s->selected = NGMS_NONE;
    s->undo_n = 0;
    s->status = NGMS_PLAYING;
    s->shuffles++;
    sol_emit(s, NGMS_EV_SHUFFLE, 0, 0);
    return NGM_OK;
}

uint8_t NEOGEO_USER ng_mjsol_event(NGMahjongSol *s, NGMahjongSolEvent *out)
{
    NGMahjongSolEvent *e;
    if (!s || !s->ev_count) return 0;
    e = &s->ev[s->ev_head];
    if (out) { out->type = e->type; out->a = e->a; out->b = e->b; out->pad = 0; }
    s->ev_head = (uint8_t)((s->ev_head + 1u) & 15u);
    s->ev_count--;
    return 1;
}

uint32_t NEOGEO_USER ng_mjsol_hash(const NGMahjongSol *s)
{
    uint32_t h = 2166136261u;
    uint8_t misc[4];
    if (!s) return 0;
    h = fnv(h, s->kind, s->n);
    h = fnv(h, s->gone, (uint16_t)sizeof(s->gone));
    misc[0] = s->left; misc[1] = s->selected; misc[2] = s->status; misc[3] = s->undo_n;
    return fnv(h, misc, 4);
}
