/* Host checks for ng_soccer: arithmetic, the ball, the lines, offside and the championship. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ng_soccer.h"

static const NGSPitch PITCH = { 1536, 704, 88, 40, 3 };
#define Q NGS_Q
#define G 3

static void free_ball(NGSBall *b, int16_t x, int16_t y)
{
    memset(b, 0, sizeof(*b));
    b->x = (int16_t)(x * Q);
    b->y = (int16_t)(y * Q);
    b->owner = NGS_NOBODY;
    b->last = NGS_NOBODY;
    b->striker = NGS_NOBODY;
}

static void arithmetic(void)
{
    int32_t v;
    assert(ngs_div(100, 7) == 14 && ngs_div(-100, 7) == -14);
    assert(ngs_mul(-300, 300) == -90000L && ngs_mul(32767, 2) == 65534L);
    for (v = 0; v < 2000000L; v += 7919) {
        uint16_t r = ngs_isqrt32(v);
        assert((int32_t)r * r <= v && (int32_t)(r + 1) * (r + 1) > v);
    }
    assert(ngs_isqrt32(-5) == 0);
    assert(ngs_abs(-12) == 12 && ngs_clamp(50, 0, 20) == 20);
}

/* A ground pass struck at ngs_pass_speed(d) comes to rest a little past d. */
static void passes(void)
{
    int16_t d;
    for (d = 32; d <= 512; d = (int16_t)(d + 16)) {
        NGSBall b;
        int f, past;
        free_ball(&b, 100, 352);
        ngs_ball_strike(&b, 3, (int16_t)(100 + d), 352, ngs_pass_speed(d), 0);
        assert(b.owner == NGS_NOBODY && b.last == 3 && b.striker == 3 && b.from_x == 100);
        b.h = 0;
        for (f = 0; f < 600 && (b.vx || b.vy); f++) ngs_ball_fly(&b, G, (uint16_t)f);
        assert(!b.vx && !b.vy);
        past = (b.x >> 4) - (100 + d);
        assert(past >= 0 && past <= 90);
    }
    assert(ngs_pass_speed(-5) == ngs_pass_speed(0) && ngs_pass_speed(9000) == ngs_pass_speed(520));
}

static void flight(void)
{
    NGSBall b;
    int f, bounces = 0, peak = 0;
    /* up, down, bounces that lose height, then it rolls and stops */
    free_ball(&b, 200, 352);
    b.vx = 60;
    b.vh = 48;
    b.h = 2;
    for (f = 0; f < 900; f++) {
        if (ngs_ball_fly(&b, G, (uint16_t)f) & NGS_FLY_BOUNCE) {
            bounces++;
            assert(b.vh > 0 && b.vh < 48);
        }
        if (b.h > peak) peak = b.h;
        assert(b.h >= 0);
    }
    assert(bounces >= 2 && peak > 300 && !b.vx && !b.vh && b.x > 200 * Q);
    /* drag is the same both ways: mirrored flights land mirrored */
    {
        NGSBall l, r;
        free_ball(&l, 768, 352);
        free_ball(&r, 768, 352);
        l.vx = -150; r.vx = 150; l.vh = r.vh = 30; l.h = r.h = 2;
        for (f = 0; f < 200; f++) { ngs_ball_fly(&l, G, (uint16_t)f); ngs_ball_fly(&r, G, (uint16_t)f); }
        assert(768 * Q - l.x == r.x - 768 * Q);
    }
    /* a positive swerve turns a ball flying right toward +y, and keeps most of its pace */
    {
        int16_t sp0, sp1;
        free_ball(&b, 300, 352);
        b.vx = 200; b.vh = 20; b.h = 2; b.curve = 6;
        sp0 = 200;
        for (f = 0; f < 12; f++) ngs_ball_fly(&b, G, (uint16_t)f);      /* still in the air */
        assert(b.h > 0);
        sp1 = (int16_t)(ngs_abs(b.vx) > ngs_abs(b.vy) ? ngs_abs(b.vx) + ngs_abs(b.vy) * 3 / 8 : ngs_abs(b.vy) + ngs_abs(b.vx) * 3 / 8);
        assert(b.vy > 0 && b.y > 352 * Q);
        assert(sp1 > sp0 * 3 / 4 && sp1 < sp0 * 5 / 4);
        assert(ngs_bend_toward(&b, 0, 10, 4) == 4 && ngs_bend_toward(&b, 0, -10, 4) == -4);
        assert(ngs_bend_y(&b, 3) == 3);
    }
    /* a struck (fast) ball skims: it goes further than a plain one */
    {
        NGSBall a, s;
        free_ball(&a, 100, 352);
        free_ball(&s, 100, 352);
        a.vx = s.vx = 120;
        s.fast = 1;
        for (f = 0; f < 120; f++) { ngs_ball_fly(&a, G, (uint16_t)f); ngs_ball_fly(&s, G, (uint16_t)f); }
        assert(s.x > a.x);
    }
}

/* The meet point agrees with the flight it predicts. */
static void meet(void)
{
    int16_t vh, mx, my;
    NGSBall b;
    free_ball(&b, 100, 100);
    b.owner = 4;
    b.h = 100;
    assert(ngs_ball_meet(&b, 24 * Q, G, &mx, &my) == 0);
    for (vh = 10; vh <= 60; vh = (int16_t)(vh + 5)) {
        uint8_t t;
        int f;
        free_ball(&b, 400, 300);
        b.vx = 90; b.vy = -30; b.vh = vh; b.h = 2;
        t = ngs_ball_meet(&b, 24 * Q, G, &mx, &my);
        assert(t > 0);
        for (f = 0; f < t; f++) ngs_ball_fly(&b, G, (uint16_t)f);
        /* within a few units of where it was said to be, near or under head height */
        assert(ngs_abs((int16_t)((b.x >> 4) - mx)) <= 8 && ngs_abs((int16_t)((b.y >> 4) - my)) <= 4);
        assert(b.h <= 24 * Q + 3 * vh);
    }
}

static void lines(void)
{
    NGSBall b;
    uint8_t right = 9;
    free_ball(&b, 700, 352);
    assert(ngs_ball_line(&b, &PITCH, &right) == NGS_LINE_NONE);
    b.x = (int16_t)(1540 * Q); b.y = 352 * Q; b.h = 10 * Q;
    assert(ngs_ball_line(&b, &PITCH, &right) == NGS_LINE_GOAL && right == 1);
    b.x = (int16_t)(-3 * Q); b.y = (352 + 90) * Q; b.h = 0;
    assert(ngs_ball_line(&b, &PITCH, &right) == NGS_LINE_FRAME && right == 0);       /* the near post */
    b.y = 352 * Q; b.h = 42 * Q;
    assert(ngs_ball_line(&b, &PITCH, &right) == NGS_LINE_FRAME);                     /* the bar */
    b.h = 60 * Q;
    assert(ngs_ball_line(&b, &PITCH, &right) == NGS_LINE_WIDE);                      /* over */
    b.y = 100 * Q; b.h = 0;
    assert(ngs_ball_line(&b, &PITCH, &right) == NGS_LINE_WIDE);
    b.x = 600 * Q; b.y = (int16_t)(710 * Q);
    assert(ngs_ball_line(&b, &PITCH, &right) == NGS_LINE_TOUCH && right == 1);
    /* the rebound: back into play off the line, half its pace */
    free_ball(&b, 1538, 352 + 89);
    b.vx = 200; b.vy = 10; b.curve = 5;
    assert(ngs_frame_test(&b, &PITCH, 89));
    ngs_frame_rebound(&b, &PITCH, 1, 20);
    assert(b.x == (1536 - 2) * Q && b.vx == -100 && b.vy == 14 && b.curve == 0);
}

static void offside(void)
{
    /* attacking right: defenders at 100 (keeper) ... the second-last at 1300 */
    const int16_t def[5] = { 1500, 1300, 1000, 900, 700 };
    int16_t att[4] = { 1320, 1290, 1350, -32768 };
    uint8_t close;
    int16_t by, line = ngs_offside_line(def, 5, 1100, 768);
    uint16_t mask;
    assert(line == 1300);
    assert(ngs_offside_line(def, 5, 1400, 768) == 1400);          /* the ball is further */
    assert(ngs_offside_line(def, 0, -500, 768) == 768);            /* never past halfway */
    mask = ngs_offside_mask(att, 4, line, 10, &close, &by);
    assert(mask == 0x5u && close == NGS_NOBODY && by == 0);        /* 20 and 50 past: both flagged */
    mask = ngs_offside_mask(att, 4, line, 30, &close, &by);
    assert(mask == 0x4u && close == 0 && by == 20);                /* 50 past is flagged; 20 is close */
    att[2] = 1305;
    mask = ngs_offside_mask(att, 4, line, 30, &close, &by);
    assert(mask == 0 && close == 0 && by == 20);
    att[0] = att[2] = 1200;
    mask = ngs_offside_mask(att, 4, line, 10, &close, &by);
    assert(mask == 0 && close == NGS_NOBODY && by == 0);
}

/* ---- the championship ---------------------------------------------------- */
static uint8_t power(uint8_t n) { return (uint8_t)(10u + (n * 7u) % 9u); }
static void result(uint8_t home, uint8_t away, uint32_t *seed, uint8_t *a, uint8_t *b)
{
    uint32_t x = *seed;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *seed = x;
    *a = (uint8_t)((x >> 3) % 4u + (power(home) > power(away)));
    *b = (uint8_t)((x >> 9) % 3u);
}

static void cup(void)
{
    static NGSCup c, d;
    int s, champions[48] = {0};
    for (s = 1; s <= 1000; s++) {
        uint8_t seen[48] = {0}, per[48] = {0}, i, human = (uint8_t)(s % 48);
        int16_t n;
        int played = 0;
        assert(ngs_cup_init(&c, human, (uint32_t)s * 2654435761u, 48, power) == NGS_OK);
        for (i = 0; i < NGS_CUP_SIDES; i++) { assert(c.groups[i] < 48 && !seen[c.groups[i]]); seen[c.groups[i]] = 1; }
        assert(seen[human] && c.generated == 48 && c.champion == NGS_NOBODY);
        for (i = 0; i < 48; i++) {
            const NGSFixture *f = &c.fixture[i];
            assert(ngs_cup_slot(&c, f->home) / 4 == i / 6 && ngs_cup_slot(&c, f->away) / 4 == i / 6 && f->home != f->away);
            per[f->home]++; per[f->away]++;
            assert(ngs_cup_round(i) == NGS_ROUND_GROUP);
        }
        for (i = 0; i < 48; i++) assert(per[i] == (seen[i] ? 3 : 0));
        while ((n = ngs_cup_next(&c, result)) >= 0) {
            const NGSFixture *f = &c.fixture[n];
            uint8_t x = (uint8_t)(s % 3), y = (uint8_t)((n + s) % 3);
            uint8_t w = x == y ? (n < 48 ? NGS_NOBODY : f->away) : (x > y ? f->home : f->away);
            assert(f->home == human || f->away == human);
            assert(!ngs_cup_result(&c, (uint8_t)n, x, y, x == y ? f->home ^ f->away ^ w ^ 0x80u : (x > y ? f->away : f->home)));
            assert(ngs_cup_result(&c, (uint8_t)n, x, y, w));
            assert(!ngs_cup_result(&c, (uint8_t)n, x, y, w));      /* once only */
            played++;
        }
        assert(c.completed == 64 && c.generated == 64 && c.champion != NGS_NOBODY && seen[c.champion]);
        champions[c.champion]++;
        assert(played >= 3 && played <= 7);
        /* the bracket: group winners meet the paired group's runners-up, winners go on */
        for (i = 0; i < 4; i++) {
            uint8_t a[4], b[4];
            ngs_cup_ranking(&c, (uint8_t)(i * 2), a);
            ngs_cup_ranking(&c, (uint8_t)(i * 2 + 1), b);
            assert(c.fixture[48 + i * 2].home == a[0] && c.fixture[48 + i * 2].away == b[1]);
            assert(c.fixture[49 + i * 2].home == b[0] && c.fixture[49 + i * 2].away == a[1]);
        }
        for (i = 0; i < 4; i++) assert(c.fixture[56 + i].home == c.fixture[48 + i * 2].winner && c.fixture[56 + i].away == c.fixture[49 + i * 2].winner);
        for (i = 0; i < 2; i++) assert(c.fixture[60 + i].home == c.fixture[56 + i * 2].winner);
        assert(c.fixture[63].home == c.fixture[60].winner && c.fixture[63].away == c.fixture[61].winner);
        assert(c.fixture[62].home != c.fixture[60].winner && c.fixture[62].away != c.fixture[61].winner);
        assert(ngs_cup_round(48) == NGS_ROUND_16 && ngs_cup_round(59) == NGS_ROUND_QUARTER && ngs_cup_round(61) == NGS_ROUND_SEMI
               && ngs_cup_round(62) == NGS_ROUND_THIRD && ngs_cup_round(63) == NGS_ROUND_FINAL);
        /* tables: every group side played three; points are wins * 3 + draws; ranked by the rule */
        for (i = 0; i < 8; i++) {
            uint8_t r[4], k;
            ngs_cup_ranking(&c, i, r);
            for (k = 0; k < 4; k++) {
                const NGSStanding *t = &c.table[ngs_cup_slot(&c, r[k])];
                assert(t->played == 3 && t->wins + t->draws + t->losses == 3 && t->points == t->wins * 3 + t->draws);
                if (k) {
                    const NGSStanding *u = &c.table[ngs_cup_slot(&c, r[k - 1])];
                    assert(u->points > t->points || (u->points == t->points &&
                           (u->scored - u->conceded > t->scored - t->conceded ||
                            (u->scored - u->conceded == t->scored - t->conceded && u->scored >= t->scored))));
                }
            }
        }
        /* the same seed, the same championship */
        assert(ngs_cup_init(&d, human, (uint32_t)s * 2654435761u, 48, power) == NGS_OK);
        while ((n = ngs_cup_next(&d, result)) >= 0) {
            const NGSFixture *f = &d.fixture[n];
            uint8_t x = (uint8_t)(s % 3), y = (uint8_t)((n + s) % 3);
            ngs_cup_result(&d, (uint8_t)n, x, y, x == y ? (n < 48 ? NGS_NOBODY : f->away) : (x > y ? f->home : f->away));
        }
        assert(!memcmp(&c, &d, sizeof(c)));
    }
    /* the strong win it more often than the weak, and nobody always does */
    {
        int strong = 0, weak = 0, i, most = 0;
        for (i = 0; i < 48; i++) {
            if (power((uint8_t)i) >= 17) strong += champions[i];
            if (power((uint8_t)i) <= 11) weak += champions[i];
            if (champions[i] > most) most = champions[i];
        }
        assert(strong > weak && most < 500);
    }
    assert(ngs_cup_init(&c, 0, 1, 31, power) == NGS_ERR_ARG && ngs_cup_init(&c, 0, 1, 65, power) == NGS_ERR_ARG);
    assert(ngs_cup_init(&c, 0, 1, 48, 0) == NGS_ERR_ARG);
}

static void scorers_and_bans(void)
{
    static NGSCup c;
    NGSScorer top[4];
    uint8_t cards[NGS_CUP_SQUAD], k, i;
    ngs_cup_init(&c, 5, 99, 32, power);
    for (i = 0; i < 3; i++) ngs_cup_scorer(&c, 5, 9);
    ngs_cup_scorer(&c, 7, 10);
    ngs_cup_scorer(&c, 7, 10);
    for (i = 0; i < 20; i++) ngs_cup_scorer(&c, (uint8_t)(10 + i), 11);      /* more names than lines */
    k = ngs_cup_top(&c, top, 4);
    assert(k == 4 && top[0].team == 5 && top[0].squad == 9 && top[0].goals == 3 && top[1].goals == 2);
    /* a booking, then another: out of the next match; a red: out of the next; served */
    memset(cards, 0, sizeof(cards));
    cards[4] = 1; cards[6] = 2;
    ngs_cup_discipline(&c, cards);
    assert(c.suspended == (1u << 6) && c.yellows[4] == 1);
    memset(cards, 0, sizeof(cards));
    cards[4] = 1;
    ngs_cup_discipline(&c, cards);
    assert(c.suspended == (1u << 4) && c.yellows[4] == 0);
    memset(cards, 0, sizeof(cards));
    cards[4] = 1;                      /* he sat it out: nothing counts */
    ngs_cup_discipline(&c, cards);
    assert(c.suspended == 0 && c.yellows[4] == 0);
}

int main(void)
{
    arithmetic();
    passes();
    flight();
    meet();
    lines();
    offside();
    cup();
    scorers_and_bans();
    printf("ng_soccer: all tests passed (cup %u bytes on this host)\n", (unsigned)sizeof(NGSCup));
    return 0;
}
