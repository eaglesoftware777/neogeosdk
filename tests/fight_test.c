/* Host checks for ng_fight with a small hand-built fighter. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ng_fight.h"

#define F(t, hurt, flags, dx, dy) {0, t, hurt, flags, dx, dy}
static const NGFFrame frames[] = {
    F(6, 0, 0, 0, 0),                       /* 0 idle / generic stand */
    F(6, 1, 0, 0, 0),                       /* 1 crouch */
    F(6, 2, 0, 0, 0),                       /* 2 air */
    F(255, 0, 0, 0, 0),                     /* 3 hold */
    F(20, 3, 0, 0, 0),                      /* 4 getup */
    /* jab 5-7 */
    F(3, 0, 0, 0, 0), F(3, 0, NGF_FF_ACTIVE, 0, 0), F(6, 0, 0, 0, 0),
    /* heavy 8-10 */
    F(6, 0, 0, 8, 0), F(4, 0, NGF_FF_ACTIVE, 0, 0), F(14, 0, 0, 0, 0),
    /* crouch low 11-13 */
    F(4, 1, 0, 0, 0), F(3, 1, NGF_FF_ACTIVE, 0, 0), F(10, 1, 0, 0, 0),
    /* fireball 14-16 */
    F(8, 0, 0, 0, 0), F(4, 0, NGF_FF_SPAWN, 0, 0), F(16, 0, 0, 0, 0),
    /* rising uppercut 17-19 */
    F(3, 0, NGF_FF_INVULN, 0, 0), F(10, 2, NGF_FF_ACTIVE | NGF_FF_AIRBORNE, 12, 60), F(30, 2, 0, 0, 0),
    /* throw 20-21 */
    F(4, 0, 0, 0, 0), F(16, 0, 0, 0, 0),
    /* air kick 22-23 */
    F(3, 2, 0, 0, 0), F(10, 2, NGF_FF_ACTIVE, 0, 0),
    /* fireball loop 24 */
    F(4, 0, 0, 0, 0),
    /* super 25-27 */
    F(10, 0, NGF_FF_INVULN, 0, 0), F(12, 0, NGF_FF_ACTIVE | NGF_FF_REHIT, 16, 0), F(20, 0, 0, 0, 0),
};
static const NGFAnim anims[] = {
    {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {1, 1, 0}, {2, 1, 0xFF},       /* idle walkf walkb crouch jump */
    {0, 1, 0}, {1, 1, 0}, {3, 1, 0xFF}, {3, 1, 0xFF}, {2, 1, 0xFF}, /* guard guardlow hit hitlow fall */
    {3, 1, 0xFF}, {4, 1, 0xFF}, {0, 1, 0}, {2, 1, 0xFF}, {3, 1, 0xFF}, /* down getup dash backdash thrown */
    {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {3, 1, 0xFF},                 /* win lose intro dizzy */
    {5, 3, 0xFF}, {8, 3, 0xFF}, {11, 3, 0xFF}, {14, 3, 0xFF}, {17, 3, 0xFF}, /* 19-23 */
    {20, 2, 0xFF}, {22, 2, 0xFF}, {24, 1, 0}, {25, 3, 0xFF},       /* 24-27 */
};
static const NGFHurtSet hurt[] = {
    {{{-14, 0, 28, 92}}, 1, 0},
    {{{-16, 0, 32, 56}}, 1, 0},
    {{{-14, 10, 28, 70}}, 1, 0},
    {{{0, 0, 0, 0}}, 0, 0},
};
/*            anim kind motion buttons stance level dmg gd chip hs bs push onhit cancel meter cost stop proj snd hsnd lvx lvy win range hit */
static const NGFMove moves[] = {
    {27, NGF_KIND_SUPER, NGF_MOTION_DQCF, NGF_BTN_A, NGF_STANCE_GROUND, NGF_LV_MID, 50, 20, 8, 20, 18, 12, NGF_ON_KNOCKDOWN, 0, 0, 1, 10, 0, 0, 0, 20, 40, 0, 90, {0, 20, 60, 50}, 0, 0},
    {23, NGF_KIND_SPECIAL, NGF_MOTION_DP, NGF_BTN_C, NGF_STANCE_GROUND, NGF_LV_MID, 90, 20, 6, 20, 20, 8, NGF_ON_KNOCKDOWN, 0, 20, 0, 8, 0, 0, 0, 10, 40, 0, 40, {0, 40, 40, 60}, 0, 0},
    {22, NGF_KIND_SPECIAL, NGF_MOTION_QCF, NGF_BTN_A, NGF_STANCE_GROUND, NGF_LV_MID, 60, 15, 6, 18, 16, 8, 0, 0, 15, 0, 6, 1, 0, 0, 0, 0, 0, 200, {0, 0, 0, 0}, 0, 0},
    {24, NGF_KIND_THROW, NGF_MOTION_FWD, NGF_BTN_C, NGF_STANCE_STAND, NGF_LV_THROW, 80, 0, 0, 0, 0, 0, NGF_ON_KNOCKDOWN, 0, 20, 0, 4, 0, 0, 0, 20, 30, 0, 44, {0, 0, 0, 0}, 0, 0},
    {21, NGF_KIND_NORMAL, NGF_MOTION_NONE, NGF_BTN_B, NGF_STANCE_CROUCH, NGF_LV_LOW, 30, 10, 0, 14, 10, 8, 0, NGF_CANCEL_SPECIAL, 6, 0, 5, 0, 0, 0, 0, 0, 0, 50, {10, 0, 50, 16}, 0, 0},
    {19, NGF_KIND_NORMAL, NGF_MOTION_NONE, NGF_BTN_A, NGF_STANCE_STAND, NGF_LV_MID, 25, 8, 0, 12, 9, 6, 0, NGF_CANCEL_SPECIAL | NGF_CANCEL_CHAIN, 5, 0, 5, 0, 0, 0, 0, 0, 0, 50, {10, 60, 46, 14}, 0, 0},
    {20, NGF_KIND_NORMAL, NGF_MOTION_NONE, NGF_BTN_C, NGF_STANCE_STAND, NGF_LV_MID, 60, 14, 0, 18, 14, 10, 0, NGF_CANCEL_SPECIAL, 10, 0, 8, 0, 0, 0, 0, 0, 0, 60, {10, 50, 56, 20}, 0, 0},
    {25, NGF_KIND_NORMAL, NGF_MOTION_NONE, NGF_BTN_D, NGF_STANCE_AIR, NGF_LV_OVERHEAD, 50, 10, 0, 16, 12, 6, 0, 0, 8, 0, 7, 0, 0, 0, 0, 0, 0, 50, {0, 10, 50, 26}, 0, 0},
};
static const NGFProjectileDef projectiles[] = {
    {26, 6, 40, 60, 50, 6, 18, 16, NGF_LV_MID, 90, 0, 0, {-14, -10, 28, 20}},
};
static const NGFFighterDef tester = {
    "TESTER", 600, 3 * 256 / 2, 256, 4 * 256, 4 * 256, 3 * 256, 3 * 256, 9 * 256, 100,
    20, 90, 56, 52, 100, 0, anims, frames, hurt, moves, projectiles,
    sizeof(anims) / sizeof(anims[0]), sizeof(moves) / sizeof(moves[0]), 1, 4,
    {160, 70, 40, 5, 4, 1, 2, 6, 3, 0},
};

static NGFMatch m;

static void run(uint16_t frames_n, uint8_t a, uint8_t b) { while (frames_n--) ngf_match_step(&m, a, b); }
static void fight_at(int16_t distance)
{
    ngf_match_init(&m, &tester, &tester, 640, 7);
    run(100, 0, 0);
    assert(m.state == NGF_M_FIGHT);
    m.f[0].x = (int32_t)(320 - distance / 2) << 8;
    m.f[1].x = (int32_t)(320 + distance / 2) << 8;
    run(2, 0, 0);
}
static void tap(const uint8_t *seq, uint8_t n, uint8_t who)
{
    uint8_t i;
    for (i = 0; i < n; i++) {
        ngf_match_step(&m, who ? 0 : seq[i], who ? seq[i] : 0);
        ngf_match_step(&m, who ? 0 : seq[i], who ? seq[i] : 0);
    }
}

static void test_motions(void)
{
    NGFInput in;
    static const uint8_t qcf[] = {NGF_RAW_DOWN, NGF_RAW_DOWN | NGF_RAW_RIGHT, NGF_RAW_RIGHT};
    static const uint8_t miss[] = {NGF_RAW_DOWN, NGF_RAW_RIGHT};
    static const uint8_t dp[] = {NGF_RAW_RIGHT, NGF_RAW_DOWN, NGF_RAW_DOWN | NGF_RAW_RIGHT};
    uint8_t i;
    ngf_input_reset(&in);
    for (i = 0; i < 3; i++) ngf_input_push(&in, qcf[i], 0);
    assert(ngf_input_motion(&in, NGF_MOTION_QCF, 0, 0));
    assert(!ngf_input_motion(&in, NGF_MOTION_QCB, 0, 0));
    assert(ngf_input_motion(&in, NGF_MOTION_QCB, 1, 0));   /* mirrored when facing left */
    ngf_input_reset(&in);
    for (i = 0; i < 2; i++) ngf_input_push(&in, miss[i], 0);
    assert(!ngf_input_motion(&in, NGF_MOTION_QCF, 0, 0));  /* skipped the diagonal */
    ngf_input_reset(&in);
    for (i = 0; i < 3; i++) ngf_input_push(&in, dp[i], 0);
    assert(ngf_input_motion(&in, NGF_MOTION_DP, 0, 0));
    ngf_input_reset(&in);
    for (i = 0; i < 3; i++) ngf_input_push(&in, qcf[i], 0);
    for (i = 0; i < 8; i++) ngf_input_push(&in, 0, 0);
    assert(!ngf_input_motion(&in, NGF_MOTION_QCF, 0, 0));  /* stale */
    ngf_input_reset(&in);
    for (i = 0; i < 45; i++) ngf_input_push(&in, NGF_RAW_LEFT, 0);
    ngf_input_push(&in, NGF_RAW_RIGHT | NGF_BTN_A, 0);
    assert(ngf_input_motion(&in, NGF_MOTION_CHARGE_BF, 0, 0));
    assert(ngf_input_pressed(&in, NGF_BTN_A, 1));
    ngf_input_reset(&in);
    for (i = 0; i < 20; i++) ngf_input_push(&in, NGF_RAW_LEFT, 0);
    ngf_input_push(&in, NGF_RAW_RIGHT, 0);
    assert(!ngf_input_motion(&in, NGF_MOTION_CHARGE_BF, 0, 0));  /* not charged */
    ngf_input_reset(&in);
    ngf_input_push(&in, NGF_RAW_RIGHT, 0); ngf_input_push(&in, 0, 0); ngf_input_push(&in, NGF_RAW_RIGHT, 0);
    assert(ngf_input_motion(&in, NGF_MOTION_DASH, 0, 0));
    ngf_input_reset(&in);
    ngf_input_push(&in, NGF_BTN_A, 0); ngf_input_push(&in, NGF_BTN_A | NGF_BTN_B, 0);
    assert(ngf_input_pressed(&in, NGF_BTN_A | NGF_BTN_B, 3));
    for (i = 1; i <= 9; i++) {
        ngf_input_reset(&in);
        ngf_input_push(&in, ngf_raw_from_numpad(i, 1), 1);
        assert(ngf_input_dir(&in, 0, 1) == i);
    }
}

static void test_hit_and_block(void)
{
    uint16_t life;
    fight_at(50);
    life = m.f[1].life;
    run(1, NGF_BTN_A, 0);
    run(12, 0, 0);
    assert(m.f[1].life < life);
    assert(m.f[0].power > 0);
    /* blocking: P2 faces left, so back is right */
    fight_at(50);
    life = m.f[1].life;
    run(1, NGF_BTN_A, NGF_RAW_RIGHT);
    run(12, 0, NGF_RAW_RIGHT);
    assert(m.f[1].life == life);
    assert(m.f[1].guard < tester.guard_max);
    /* a low attack beats a standing guard, not a crouching one */
    fight_at(50);
    life = m.f[1].life;
    run(1, NGF_RAW_DOWN | NGF_BTN_B, NGF_RAW_RIGHT);
    run(14, NGF_RAW_DOWN, NGF_RAW_RIGHT);
    assert(m.f[1].life < life);
    fight_at(50);
    life = m.f[1].life;
    run(1, NGF_RAW_DOWN | NGF_BTN_B, NGF_RAW_RIGHT | NGF_RAW_DOWN);
    run(14, NGF_RAW_DOWN, NGF_RAW_RIGHT | NGF_RAW_DOWN);
    assert(m.f[1].life == life);
}

static void test_throw_projectile_super(void)
{
    static const uint8_t qcfa[] = {NGF_RAW_DOWN, NGF_RAW_DOWN | NGF_RAW_RIGHT, NGF_RAW_RIGHT | NGF_BTN_A};
    static const uint8_t dqcfa[] = {NGF_RAW_DOWN, NGF_RAW_DOWN | NGF_RAW_RIGHT, NGF_RAW_RIGHT,
                                    NGF_RAW_DOWN, NGF_RAW_DOWN | NGF_RAW_RIGHT, NGF_RAW_RIGHT | NGF_BTN_A};
    uint16_t life;
    uint8_t thrown = 0, i;
    fight_at(40);
    life = m.f[1].life;
    run(1, NGF_RAW_RIGHT | NGF_BTN_C, 0);
    for (i = 0; i < 40; i++) { run(1, 0, 0); if (m.f[1].state == NGF_S_THROWN) thrown = 1; }
    assert(thrown);
    assert(m.f[1].life < life);
    fight_at(220);
    life = m.f[1].life;
    tap(qcfa, 3, 0);
    assert(m.f[0].move && m.f[0].move->kind == NGF_KIND_SPECIAL);
    run(12, 0, 0);
    assert(m.proj[0].active || m.f[1].life < life);
    run(60, 0, 0);
    assert(m.f[1].life < life);
    fight_at(60);
    m.f[0].power = NGF_POWER_STOCK;
    life = m.f[1].life;
    tap(dqcfa, 6, 0);
    assert(m.f[0].move && m.f[0].move->kind == NGF_KIND_SUPER);
    assert(m.f[0].power == 0);
    run(50, 0, 0);
    assert(m.f[1].life < life);
}

static void test_timer_draw(void)
{
    uint32_t n = 0;
    ngf_match_init(&m, &tester, &tester, 640, 3);
    while (m.state != NGF_M_OVER && n < 20000u) { ngf_match_step(&m, 0, 0); n++; }
    assert(m.state == NGF_M_OVER);
    assert(m.winner == 2u);
}

static uint32_t cpu_match(uint16_t seed, uint32_t *frames_out)
{
    uint32_t n = 0, h = 0;
    ngf_match_init(&m, &tester, &tester, 640, seed);
    ngf_set_cpu(&m, 0, 4);
    ngf_set_cpu(&m, 1, 6);
    while (m.state != NGF_M_OVER && n < 60000u) {
        NGFEvent e;
        ngf_match_step(&m, 0, 0);
        while (ngf_pop_event(&m, &e)) h = h * 31u + e.type;
        n++;
    }
    *frames_out = n;
    return ngf_match_hash(&m) ^ h;
}

static void test_cpu_determinism(void)
{
    uint32_t n1, n2, a = cpu_match(99, &n1), b = cpu_match(99, &n2);
    assert(a == b && n1 == n2);
    assert(m.state == NGF_M_OVER);
    assert(m.winner <= 2u);
    printf("cpu match: %lu frames, winner %u, rounds %u\n", (unsigned long)n1, m.winner, m.round);
}

int main(void)
{
    test_motions();
    test_hit_and_block();
    test_throw_projectile_super();
    test_timer_draw();
    test_cpu_determinism();
    printf("fight_test: ok (NGFMatch %u bytes)\n", (unsigned)sizeof(NGFMatch));
    return 0;
}
