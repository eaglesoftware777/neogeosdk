/*
 * ng_soccer.c -- the football genre engine (see ng_soccer.h).
 *
 * Sections: arithmetic, the ball, the lines, offside, the championship.
 *
 * 68000 notes: every multiply that matters is one MULS (ngs_mul) and every
 * divide one DIVS or DIVU; the championship's remainders are taken in two
 * 16-bit steps (umod), so there is no library divide. No recursion, no
 * state outside the caller's structures.
 */
#include "ng_soccer.h"

#define U(v) ((int16_t)((v) >> 4))

/* ---------------------------------------------------------------- arithmetic */

/* a / b with a 32-bit dividend: one DIVS. The quotient must fit 16 bits. */
NGS_INLINE int16_t div32(int32_t a, int16_t b)
{
#ifdef __m68k__
    __asm__ ("divs.w %1,%0" : "+d" (a) : "dmi" (b) : "cc");
    return (int16_t)a;
#else
    return (int16_t)(a / b);
#endif
}

/* x % m for any 32-bit x and a 16-bit m, in two DIVU steps (each quotient fits 16 bits). */
static uint16_t NEOGEO_USER umod(uint32_t x, uint16_t m)
{
#ifdef __m68k__
    uint32_t hi = x >> 16, r;
    __asm__ ("divu.w %1,%0" : "+d" (hi) : "dmi" (m) : "cc");
    r = (hi & 0xFFFF0000uL) | (x & 0xFFFFu);
    __asm__ ("divu.w %1,%0" : "+d" (r) : "dmi" (m) : "cc");
    return (uint16_t)(r >> 16);
#else
    return (uint16_t)(x % m);
#endif
}

uint16_t NEOGEO_USER ngs_isqrt32(int32_t v)
{
    uint32_t x = (uint32_t)(v < 0 ? 0 : v), r = 0, bit = 1uL << 30;
    while (bit > x) bit >>= 2;
    while (bit) {
        if (x >= r + bit) { x -= r + bit; r = (r >> 1) + bit; }
        else r >>= 1;
        bit >>= 2;
    }
    return (uint16_t)r;
}

/* ---------------------------------------------------------------- the ball */

/* A share of the speed a frame (a 64th and a 256th), and at least 2: a slow ball stops. */
int16_t NEOGEO_USER ngs_grip(int16_t v)
{
    int16_t a = ngs_abs(v), f = (int16_t)((a >> 6) + (a >> 8));
    if (f < 2) f = 2;
    if (a <= f) return 0;
    return (int16_t)(v > 0 ? v - f : v + f);
}

uint8_t NEOGEO_USER ngs_ball_fly(NGSBall *b, int16_t gravity, uint16_t frame)
{
    uint8_t ev = 0;
    b->x = (int16_t)(b->x + b->vx);
    b->y = (int16_t)(b->y + b->vy);
    b->h = (int16_t)(b->h + b->vh);
    if (b->curve) {
        /* the swerve turns its flight, whichever way it is going: a push of curve/4 Q a
         * frame square to its line, so a curled ball keeps its pace. The push is mostly a
         * fraction of a step: the remainders are kept, so it adds up. */
        int16_t ax = ngs_abs(b->vx), ay = ngs_abs(b->vy);
        int16_t sp = (int16_t)(ax > ay ? ax + ay * 3 / 8 : ay + ax * 3 / 8);
        if (sp > 8) {
            int16_t k = ngs_div((int16_t)(b->curve * 256), sp), ox = b->vx;
            int32_t nx = ngs_mul((int16_t)(-b->vy), k) + b->bend_rx, ny = ngs_mul(ox, k) + b->bend_ry;
            int16_t dx = (int16_t)(nx >> 10), dy = (int16_t)(ny >> 10);
            b->bend_rx = (int16_t)(nx - (int32_t)dx * 1024);
            b->bend_ry = (int16_t)(ny - (int32_t)dy * 1024);
            b->vx = (int16_t)(b->vx + dx);
            b->vy = (int16_t)(b->vy + dy);
        }
    }
    if (b->h > 0) {
        b->vh = (int16_t)(b->vh - gravity);
        /* air drag, the same both ways: a shift would round a leftward ball down a whole
         * step every frame, and shots at the left-hand goal would die in the air */
        b->vx = (int16_t)(b->vx - b->vx / 128);
        b->vy = (int16_t)(b->vy - b->vy / 128);
    } else {
        b->h = 0;
        if (b->vh < -10) {
            /* the bounce takes pace off it too: kept whole, a long ball skips on past the
             * man it was for */
            b->vh = div32(ngs_mul((int16_t)-b->vh, 45), 100);
            b->vx = (int16_t)(b->vx - b->vx / 4);
            b->vy = (int16_t)(b->vy - b->vy / 4);
            ev |= NGS_FLY_BOUNCE;
        }
        else b->vh = 0;
        b->curve = (int16_t)(b->curve / 2);
        /* a struck ball skims: the grass bites a quarter as often until it has slowed */
        if (!b->fast || (frame & 3u) == 0) {
            b->vx = ngs_grip(b->vx);
            b->vy = ngs_grip(b->vy);
        }
        if (ngs_abs(b->vx) + ngs_abs(b->vy) < 60) b->fast = 0;
    }
    if (b->vx || b->vy) b->spin = (uint8_t)(b->spin + 1 + ngs_div((int16_t)(ngs_abs(b->vx) + ngs_abs(b->vy)), 24));
    return ev;
}

void NEOGEO_USER ngs_ball_strike(NGSBall *b, uint8_t from, int16_t tx, int16_t ty, int16_t speed, int16_t vh)
{
    int32_t dx = (int32_t)tx * NGS_Q - b->x, dy = (int32_t)ty * NGS_Q - b->y;
    int32_t ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
    int32_t len = ax > ay ? ax + ay * 3 / 8 : ay + ax * 3 / 8;
    /* the octagonal length: a pitch up to 2048 units never needs the scaling */
    while (len > 32767) { dx /= 2; dy /= 2; len /= 2; }
    if (len < 1) len = 1;
    b->vx = div32(ngs_mul((int16_t)dx, speed), (int16_t)len);
    b->vy = div32(ngs_mul((int16_t)dy, speed), (int16_t)len);
    b->vh = vh;
    b->h = 2;
    b->curve = 0;
    b->bend_rx = b->bend_ry = 0;
    b->owner = NGS_NOBODY;
    b->last = from;
    b->from_x = U(b->x);
    b->striker = from;
    b->fast = 0;
}

int16_t NEOGEO_USER ngs_bend_toward(const NGSBall *b, int16_t dx, int16_t dy, int16_t amount)
{
    int32_t cross = ngs_mul(b->vx, dy) - ngs_mul(b->vy, dx);
    return (int16_t)(cross >= 0 ? amount : -amount);
}

/* The speed for a ball to come to rest 40 units past d, every 16 units of d. */
static const uint8_t k_weight[34] = {50, 59, 67, 75, 81, 87, 93, 98, 103, 108, 113, 117, 121, 125, 129, 133, 137, 141, 144, 148,
                                     151, 155, 158, 161, 164, 167, 170, 173, 176, 179, 182, 185, 187, 190};

int16_t NEOGEO_USER ngs_pass_speed(int16_t d)
{
    uint16_t i;
    if (d < 0) d = 0;
    if (d > 520) d = 520;
    i = (uint16_t)d >> 4;
    return (int16_t)(k_weight[i] + (int16_t)(ngs_mul((int16_t)(k_weight[i + 1u] - k_weight[i]), (int16_t)(d & 15)) >> 4));
}

uint8_t NEOGEO_USER ngs_ball_meet(const NGSBall *b, int16_t meet_h, int16_t gravity, int16_t *mx, int16_t *my)
{
    int32_t disc;
    int16_t above, t;
    if (b->owner != NGS_NOBODY || b->h <= 0) return 0;
    above = (int16_t)(b->h - meet_h);
    disc = ngs_mul(b->vh, b->vh) + ngs_mul((int16_t)(2 * gravity), above);
    if (disc < 0 || (above < 0 && b->vh < 0)) {
        /* low, or already under that height: where it lands */
        above = b->h;
        disc = ngs_mul(b->vh, b->vh) + ngs_mul((int16_t)(2 * gravity), above);
    }
    t = ngs_div((int16_t)(b->vh + (int16_t)ngs_isqrt32(disc)), gravity);
    if (t < 1) t = 1;
    if (t > 90) t = 90;
    *mx = (int16_t)(U(b->x) + (int16_t)(ngs_mul(b->vx, t) >> 4));
    *my = (int16_t)(U(b->y) + (int16_t)(ngs_mul(b->vy, t) >> 4));
    return (uint8_t)t;
}

/* ---------------------------------------------------------------- the lines */

uint8_t NEOGEO_USER ngs_frame_test(const NGSBall *b, const NGSPitch *p, int16_t off)
{
    int16_t h = U(b->h);
    return (uint8_t)((off >= p->post && off < p->post + 6 && h < p->bar + 6) ||
                     (off < p->post + 6 && h >= p->bar && h < p->bar + 6));
}

void NEOGEO_USER ngs_frame_rebound(NGSBall *b, const NGSPitch *p, uint8_t right, uint16_t rnd)
{
    b->x = (int16_t)((right ? p->len - 2 : 2) * NGS_Q);
    b->vx = (int16_t)(-b->vx / 2);
    b->vy = (int16_t)(b->vy + (int16_t)(rnd & 31u) - 16);
    if (U(b->h) >= p->bar) b->vh = (int16_t)(-b->vh / 2);
    b->curve = 0;
}

uint8_t NEOGEO_USER ngs_ball_line(const NGSBall *b, const NGSPitch *p, uint8_t *right)
{
    int16_t x = U(b->x), y = U(b->y);
    if (x < 0 || x > p->len) {
        int16_t off = ngs_abs((int16_t)(y - (p->wid >> 1)));
        *right = (uint8_t)(x > p->len);
        if (off < p->post && U(b->h) < p->bar) return NGS_LINE_GOAL;
        return ngs_frame_test(b, p, off) ? NGS_LINE_FRAME : NGS_LINE_WIDE;
    }
    if (y < 0 || y > p->wid) {
        *right = (uint8_t)(y > p->wid);
        return NGS_LINE_TOUCH;
    }
    return NGS_LINE_NONE;
}

/* ---------------------------------------------------------------- offside */

int16_t NEOGEO_USER ngs_offside_line(const int16_t *def, uint8_t n, int16_t ball, int16_t halfway)
{
    int16_t a = -30000, b = -30000, line;
    uint8_t i;
    for (i = 0; i < n; i++) {
        int16_t x = def[i];
        if (x > a) { b = a; a = x; }
        else if (x > b) b = x;
    }
    line = b;
    if (ball > line) line = ball;
    if (halfway > line) line = halfway;
    return line;
}

uint16_t NEOGEO_USER ngs_offside_mask(const int16_t *att, uint8_t n, int16_t line, int16_t margin,
                                      uint8_t *close, int16_t *by)
{
    uint16_t mask = 0;
    int16_t best = 0;
    uint8_t i;
    *close = NGS_NOBODY;
    for (i = 0; i < n && i < 16u; i++) {
        int16_t past;
        if (att[i] == -32768) continue;
        past = (int16_t)(att[i] - line);
        if (past > margin) mask = (uint16_t)(mask | (1u << i));
        else if (past > 0 && past > best) { best = past; *close = i; }
    }
    *by = best;
    return mask;
}

/* ---------------------------------------------------------------- the championship */

uint32_t NEOGEO_USER ngs_cup_roll(NGSCup *c)
{
    uint32_t x = c->seed;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return c->seed = x;
}

static void NEOGEO_USER fixture(NGSCup *c, uint8_t n, uint8_t a, uint8_t b)
{
    c->fixture[n].home = a;
    c->fixture[n].away = b;
    c->fixture[n].winner = NGS_NOBODY;
}

uint8_t NEOGEO_USER ngs_cup_slot(const NGSCup *c, uint8_t nation)
{
    uint8_t i;
    for (i = 0; i < NGS_CUP_SIDES; i++) if (c->groups[i] == nation) return i;
    return 0;
}

uint8_t NEOGEO_USER ngs_cup_round(uint8_t n)
{
    if (n < 48u) return NGS_ROUND_GROUP;
    if (n < 56u) return NGS_ROUND_16;
    if (n < 60u) return NGS_ROUND_QUARTER;
    if (n < 62u) return NGS_ROUND_SEMI;
    return (uint8_t)(n == 62u ? NGS_ROUND_THIRD : NGS_ROUND_FINAL);
}

int8_t NEOGEO_USER ngs_cup_init(NGSCup *c, uint8_t human, uint32_t seed, uint8_t nations, NGSPowerFn power)
{
    static const uint8_t pairs[12] = {0, 1, 2, 3, 0, 2, 1, 3, 0, 3, 1, 2};   /* each group's six */
    uint16_t score[NGS_CUP_NATIONS_MAX];
    uint8_t *raw = (uint8_t *)c, i, n = 1, g, k;
    uint16_t z;
    if (!c || !power || nations < NGS_CUP_SIDES || nations > NGS_CUP_NATIONS_MAX) return NGS_ERR_ARG;
    for (z = 0; z < sizeof(*c); z++) raw[z] = 0;
    c->seed = seed ? seed : 1u;
    c->human = human < nations ? human : 0;
    c->champion = NGS_NOBODY;
    /* the finals' 32: the human side, then the rest by strength plus a qualifying
     * campaign's luck (scores start at 1, a side drawn in drops to 0) */
    for (i = 0; i < nations; i++) score[i] = (uint16_t)(power(i) * 4u + umod(ngs_cup_roll(c), 28u) + 1u);
    c->groups[0] = c->human;
    score[c->human] = 0;
    while (n < NGS_CUP_SIDES) {
        uint8_t best = 0;
        for (i = 1; i < nations; i++) if (score[i] > score[best]) best = i;
        c->groups[n++] = best;
        score[best] = 0;
    }
    /* the draw */
    for (i = NGS_CUP_SIDES - 1u; i > 0; i--) {
        uint8_t j = (uint8_t)umod(ngs_cup_roll(c), (uint16_t)(i + 1u)), v = c->groups[i];
        c->groups[i] = c->groups[j];
        c->groups[j] = v;
    }
    for (g = 0, n = 0; g < NGS_CUP_GROUPS; g++)
        for (k = 0; k < 12u; k += 2u, n++)
            fixture(c, n, c->groups[g * 4u + pairs[k]], c->groups[g * 4u + pairs[k + 1u]]);
    c->generated = NGS_CUP_GROUP_MATCHES;
    return NGS_OK;
}

/* Points, goal difference, goals scored; the sort keeps the draw's order after that. */
static uint8_t NEOGEO_USER better(const NGSCup *c, uint8_t a, uint8_t b)
{
    const NGSStanding *x = &c->table[ngs_cup_slot(c, a)], *y = &c->table[ngs_cup_slot(c, b)];
    int16_t dx = (int16_t)(x->scored - x->conceded), dy = (int16_t)(y->scored - y->conceded);
    if (x->points != y->points) return (uint8_t)(x->points > y->points);
    if (dx != dy) return (uint8_t)(dx > dy);
    return (uint8_t)(x->scored > y->scored);
}

void NEOGEO_USER ngs_cup_ranking(const NGSCup *c, uint8_t group, uint8_t out[4])
{
    uint8_t i, j, v;
    if (group >= NGS_CUP_GROUPS) return;
    for (i = 0; i < 4u; i++) out[i] = c->groups[group * 4u + i];
    for (i = 1; i < 4u; i++) {
        v = out[i];
        j = i;
        while (j && better(c, v, out[j - 1u])) { out[j] = out[j - 1u]; j--; }
        out[j] = v;
    }
}

/* Each round's fixtures once the one before is complete: group winners meet the
 * runners-up of the paired group (A1-B2, B1-A2, ...), then winners meet winners;
 * the semi-finals' losers play for third place. */
static void NEOGEO_USER advance(NGSCup *c)
{
    uint8_t i;
    if (c->completed != c->generated) return;
    if (c->completed == 48u) {
        for (i = 0; i < 4u; i++) {
            uint8_t a[4], b[4];
            ngs_cup_ranking(c, (uint8_t)(i * 2u), a);
            ngs_cup_ranking(c, (uint8_t)(i * 2u + 1u), b);
            fixture(c, (uint8_t)(48u + i * 2u), a[0], b[1]);
            fixture(c, (uint8_t)(49u + i * 2u), b[0], a[1]);
        }
        c->generated = 56;
    } else if (c->completed == 56u) {
        for (i = 0; i < 4u; i++) fixture(c, (uint8_t)(56u + i), c->fixture[48u + i * 2u].winner, c->fixture[49u + i * 2u].winner);
        c->generated = 60;
    } else if (c->completed == 60u) {
        for (i = 0; i < 2u; i++) fixture(c, (uint8_t)(60u + i), c->fixture[56u + i * 2u].winner, c->fixture[57u + i * 2u].winner);
        c->generated = 62;
    } else if (c->completed == 62u) {
        const NGSFixture *a = &c->fixture[60], *b = &c->fixture[61];
        fixture(c, 62, a->winner == a->home ? a->away : a->home, b->winner == b->home ? b->away : b->home);
        fixture(c, 63, a->winner, b->winner);
        c->generated = 64;
    } else if (c->completed == 64u) c->champion = c->fixture[63].winner;
}

void NEOGEO_USER ngs_cup_scorer(NGSCup *c, uint8_t team, uint8_t squad)
{
    uint8_t i, low = 0;
    for (i = 0; i < NGS_CUP_SCORERS; i++)
        if (c->scorers[i].goals && c->scorers[i].team == team && c->scorers[i].squad == squad) {
            if (c->scorers[i].goals < 255u) c->scorers[i].goals++;
            return;
        }
    for (i = 0; i < NGS_CUP_SCORERS; i++) {
        if (!c->scorers[i].goals) break;
        if (c->scorers[i].goals < c->scorers[low].goals) low = i;
    }
    if (i == NGS_CUP_SCORERS) {
        if (c->scorers[low].goals > 1u) return;
        i = low;
    }
    c->scorers[i].team = team;
    c->scorers[i].squad = squad;
    c->scorers[i].goals = 1;
}

uint8_t NEOGEO_USER ngs_cup_top(const NGSCup *c, NGSScorer *out, uint8_t n)
{
    uint8_t i, j, k = 0;
    for (i = 0; i < NGS_CUP_SCORERS; i++) {
        const NGSScorer *s = &c->scorers[i];
        if (!s->goals) continue;
        for (j = k; j > 0 && out[j - 1u].goals < s->goals; j--)
            if (j < n) { out[j].team = out[j - 1u].team; out[j].squad = out[j - 1u].squad; out[j].goals = out[j - 1u].goals; }
        if (j < n) {
            out[j].team = s->team; out[j].squad = s->squad; out[j].goals = s->goals;
            if (k < n) k++;
        }
    }
    return k;
}

/* A computer side's goals go to the men most likely to score. */
static void NEOGEO_USER quick_scorers(NGSCup *c, uint8_t team, uint8_t goals)
{
    static const uint8_t k_who[10] = {9, 10, 9, 10, 8, 7, 9, 6, 5, 3};
    while (goals--) ngs_cup_scorer(c, team, k_who[umod(ngs_cup_roll(c), 10u)]);
}

uint8_t NEOGEO_USER ngs_cup_result(NGSCup *c, uint8_t n, uint8_t a, uint8_t b, uint8_t winner)
{
    NGSFixture *f;
    uint8_t side;
    if (n >= c->generated) return 0;
    f = &c->fixture[n];
    if (f->played) return 0;
    if (a != b && winner != (a > b ? f->home : f->away)) return 0;
    if (a == b && ((n < 48u && winner != NGS_NOBODY) || (n >= 48u && winner != f->home && winner != f->away))) return 0;
    f->goals[0] = a;
    f->goals[1] = b;
    f->winner = winner;
    f->played = 1;
    c->completed++;
    if (n < 48u)
        for (side = 0; side < 2u; side++) {
            NGSStanding *s = &c->table[ngs_cup_slot(c, side ? f->away : f->home)];
            uint8_t gf = side ? b : a, ga = side ? a : b;
            s->played++;
            s->scored = (uint16_t)(s->scored + gf);
            s->conceded = (uint16_t)(s->conceded + ga);
            if (gf > ga) { s->wins++; s->points = (uint8_t)(s->points + 3u); }
            else if (gf == ga) { s->draws++; s->points++; }
            else s->losses++;
        }
    advance(c);
    return 1;
}

int16_t NEOGEO_USER ngs_cup_next(NGSCup *c, NGSResultFn result)
{
    uint8_t i;
    for (i = 0; i < c->generated; i++)
        if (!c->fixture[i].played) {
            NGSFixture *f = &c->fixture[i];
            uint8_t a = 0, b = 0, w;
            if (f->home == c->human || f->away == c->human) return i;
            /* every computer fixture is played out as a result from the sides' strengths:
             * no finalist is ever picked */
            if (result) result(f->home, f->away, &c->seed, &a, &b);
            quick_scorers(c, f->home, a);
            quick_scorers(c, f->away, b);
            w = a == b ? (i < 48u ? NGS_NOBODY : ((ngs_cup_roll(c) & 1u) ? f->home : f->away)) : (a > b ? f->home : f->away);
            ngs_cup_result(c, i, a, b, w);
        }
    return -1;
}

void NEOGEO_USER ngs_cup_discipline(NGSCup *c, const uint8_t cards[NGS_CUP_SQUAD])
{
    uint8_t sq;
    uint16_t served = c->suspended, next = 0;
    for (sq = 0; sq < NGS_CUP_SQUAD; sq++) {
        uint16_t bit = (uint16_t)(1u << sq);
        if (served & bit) continue;                 /* he sat this one out */
        if (cards[sq] >= 2u || (cards[sq] == 1u && ++c->yellows[sq] >= 2u)) {
            next = (uint16_t)(next | bit);
            c->yellows[sq] = 0;
        }
    }
    c->suspended = next;
}
