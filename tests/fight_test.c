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
    0, 0, 0,
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

static void test_frame_contacts(void)
{
    static const NGFHitSet sets[] = {
        {{{0,0,0,0}},0,0},
        {{{10,60,18,14}},1,0},
        {{{10,60,50,14}},1,0},
        {{{10,4,50,8},{10,60,50,14}},2,0}
    };
    uint8_t indices[sizeof(frames)/sizeof(frames[0])] = {0};
    NGFFighterDef authored = tester;
    uint8_t k, side;
    authored.hit_sets = sets; authored.frame_hits = indices;
    for (side=0;side<2;side++) for (k=0;k<4;k++) {
        NGFFighter *attacker;
        uint16_t life;
        fight_at(60);
        attacker = &m.f[side]; attacker->def = &authored;
        attacker->state = NGF_S_ATTACK; attacker->move = &moves[5];
        attacker->anim = 19; attacker->frame = 1; attacker->frame_timer = 0;
        attacker->move_hit = 0; indices[6] = k;
        life = m.f[side^1].life;
        run(1,0,0);
        if (k<2) assert(m.f[side^1].life==life); /* Empty/early reach whiffs despite move union. */
        else assert(m.f[side^1].life<life); /* Extended/multiple boxes contact. */
    }
}

static void test_ordered_rotations(void)
{
    static const uint8_t cw[] = {6,3,2,1,4,7,8,9,6,3,2,1,4,7,8};
    static const uint8_t ccw[] = {6,9,8,7,4,1,2,3,6,9,8,7,4,1,2};
    static const uint8_t scrambled[] = {6,4,2,8,6,4,2,8};
    static const uint8_t reverse[] = {6,2,4,2,8};
    NGFInput in;
    uint8_t side, turn, i;
    for (side=0;side<2;side++) for (turn=0;turn<2;turn++) {
        const uint8_t *path = turn ? ccw : cw;
        ngf_input_reset(&in);
        for (i=0;i<7;i++) ngf_input_push(&in,ngf_raw_from_numpad(path[i],side),side);
        assert(ngf_input_motion(&in,NGF_MOTION_360,side,0));
        assert(!ngf_input_motion(&in,NGF_MOTION_720,side,0));
        for (;i<15;i++) ngf_input_push(&in,ngf_raw_from_numpad(path[i],side),side);
        assert(ngf_input_motion(&in,NGF_MOTION_720,side,0));
        for (i=0;i<8;i++) ngf_input_push(&in,0,side);   /* past the button's 7-frame grace */
        assert(!ngf_input_motion(&in,NGF_MOTION_360,side,0));
    }
    ngf_input_reset(&in);
    for (i=0;i<sizeof(scrambled);i++) ngf_input_push(&in,ngf_raw_from_numpad(scrambled[i],0),0);
    assert(!ngf_input_motion(&in,NGF_MOTION_360,0,0));
    assert(!ngf_input_motion(&in,NGF_MOTION_720,0,0));
    ngf_input_reset(&in);
    for (i=0;i<sizeof(reverse);i++) ngf_input_push(&in,ngf_raw_from_numpad(reverse[i],0),0);
    assert(!ngf_input_motion(&in,NGF_MOTION_360,0,0));
    ngf_input_reset(&in);
    for(i=0;i<7;i++) { ngf_input_push(&in,ngf_raw_from_numpad(cw[i],0),0);ngf_input_push(&in,ngf_raw_from_numpad(cw[i],0),0); }
    ngf_input_push(&in,0,0);
    assert(ngf_input_motion(&in,NGF_MOTION_360,0,0));
    /* Motion history is consumed by successful motion moves, not by normal
     * buttons: a player can buffer directions while chaining a normal. */
    {
        NGFFighterDef spinning = tester;
        NGFMove spin = moves[1];
        spin.motion = NGF_MOTION_360;
        spin.buttons = NGF_BTN_A;
        spinning.moves = &spin; spinning.move_count = 1;
        fight_at(100);
        m.f[0].def = &spinning;
        for(i=0;i<6;i++) run(1,(uint8_t)(ngf_raw_from_numpad(cw[i],0)|(i==5?NGF_BTN_A:0)),0);
        assert(m.f[0].move == &spin);
        assert(m.f[0].input.motion_frames == 0);
        assert(!ngf_input_motion(&m.f[0].input,NGF_MOTION_360,0,0));
    }
}

static void test_720_and_teams(void)
{
    NGFInput in;
    /* 720 motion */
    ngf_input_reset(&in);
    ngf_input_push(&in, NGF_RAW_RIGHT, 0);
    ngf_input_push(&in, NGF_RAW_DOWN, 0);
    ngf_input_push(&in, NGF_RAW_LEFT, 0);
    ngf_input_push(&in, NGF_RAW_UP, 0);
    assert(ngf_input_motion(&in, NGF_MOTION_360, 0, 0));
    assert(!ngf_input_motion(&in, NGF_MOTION_720, 0, 0));
    ngf_input_push(&in, NGF_RAW_RIGHT, 0);
    ngf_input_push(&in, NGF_RAW_DOWN, 0);
    ngf_input_push(&in, NGF_RAW_LEFT, 0);
    ngf_input_push(&in, NGF_RAW_UP, 0);
    assert(ngf_input_motion(&in, NGF_MOTION_720, 0, 0));

    /* Super freeze */
    fight_at(60);
    m.f[0].power = NGF_POWER_STOCK;
    {
        static const uint8_t dqcfa[] = {NGF_RAW_DOWN, NGF_RAW_DOWN | NGF_RAW_RIGHT, NGF_RAW_RIGHT,
                                        NGF_RAW_DOWN, NGF_RAW_DOWN | NGF_RAW_RIGHT, NGF_RAW_RIGHT | NGF_BTN_A};
        tap(dqcfa, 6, 0);
        assert(m.super_freeze > 0);
        assert(m.super_who == 0);
    }

    /* 3v3 Team battle KO swap */
    ngf_match_init(&m, &tester, &tester, 640, 7);
    ngf_match_set_mode(&m, NGF_MODE_3V3);
    ngf_team_set(&m, 1, &tester, &tester, &tester);
    assert(m.team_count[1] == 3);
    assert(m.team_active[1] == 0);
    run(100, 0, 0); /* into fight */
    m.f[1].life = 0;
    m.state = NGF_M_KO;
    m.state_timer = 150;
    ngf_match_step(&m, 0, 0);
    assert(m.team_active[1] == 1);
    assert(m.f[1].life == tester.life);

    /* Striker summon */
    run(100, 0, 0);
    ngf_striker_set(&m, 0, &tester);
    assert(ngf_call_striker(&m, 0));
    assert(m.striker[0].active == 1);
    assert(m.striker[0].cooldown > 0);

    /* Tag duo swap */
    ngf_match_set_mode(&m, NGF_MODE_TAG);
    ngf_team_set(&m, 0, &tester, &tester, NULL);
    assert(m.team_count[0] == 2);
    assert(m.team_active[0] == 0);
    assert(ngf_tag_switch(&m, 0));
    assert(m.team_active[0] == 1);
}

static void test_freeze_and_input_buffer(void)
{
    static const uint8_t super[] = {NGF_RAW_DOWN, NGF_RAW_DOWN | NGF_RAW_RIGHT, NGF_RAW_RIGHT,
        NGF_RAW_DOWN, NGF_RAW_DOWN | NGF_RAW_RIGHT, NGF_RAW_RIGHT | NGF_BTN_A};
    uint8_t i, pause, frame, timer;
    int32_t x, h;
    uint16_t life, clock;
    fight_at(100);
    m.f[0].power = NGF_POWER_STOCK;
    tap(super, 6, 0);
    pause = m.super_freeze; frame = m.f[0].frame; timer = m.f[0].frame_timer;
    x = m.f[0].x; h = m.f[0].h; life = m.f[1].life; clock = m.clock;
    assert(pause > 0);
    for (i = 0; i < pause; i++) {
        run(1, NGF_RAW_RIGHT, 0);
        assert(m.f[0].x == x && m.f[0].h == h);
        assert(m.f[0].frame == frame && m.f[0].frame_timer == timer);
        assert(m.f[1].life == life && m.clock == clock);
    }
    assert(m.super_freeze == 0);
    fight_at(180);
    m.hitstop = 12;
    run(1, NGF_BTN_A, 0); run(11, 0, 0);
    assert(m.f[0].state == NGF_S_IDLE);
    run(1, 0, 0);
    assert(m.f[0].state == NGF_S_ATTACK && m.f[0].move_index == 5);
    run(40, 0, 0);
    assert(m.f[0].state == NGF_S_IDLE);
    fight_at(180);
    m.hitstop = 12;
    ngf_set_cpu(&m, 1, 4); m.cpu[1].wait = 8;
    run(12, 0, 0);
    assert(m.cpu[1].wait == 8);
}

static void test_assist_collision(void)
{
    uint16_t life;
    fight_at(260); ngf_striker_set(&m, 0, &tester); m.hitstop = 12;
    run(1, NGF_BTN_A | NGF_BTN_C, 0); run(11, 0, 0); run(1, 0, 0);
    assert(m.striker[0].active && m.assist_uses[0] == 2);
    fight_at(260);
    assert(!ngf_call_striker(&m, 0));
    assert(!ngf_call_striker(&m, 255));
    ngf_striker_set(&m, 0, &tester); life = m.f[1].life;
    assert(ngf_call_striker(&m, 0));
    assert(m.assist_uses[0] == 2);
    assert(!ngf_call_striker(&m, 0));
    run(120, 0, 0);
    assert(m.f[1].life == life && !m.striker[0].active);
    fight_at(40); ngf_striker_set(&m, 0, &tester); life = m.f[1].life;
    assert(ngf_call_striker(&m, 0)); run(100, 0, 0);
    assert(m.f[1].life < life);
    assert(m.f[1].life == life - (uint16_t)moves[6].damage * 3u / 4u);
    m.assist_uses[0] = 0; m.striker[0].cooldown = 0;
    assert(!ngf_call_striker(&m, 0));
}

static void ko_side(uint8_t who)
{
    m.f[who].life = 0; m.hitstop = m.super_freeze = 0;
    m.state = NGF_M_KO; m.state_timer = 150; run(1, 0, 0);
}

static void test_team_lifecycle(void)
{
    NGFFighterDef other = tester;
    other.name = "OTHER"; other.life = 900;
    ngf_match_init(&m, &tester, &tester, 640, 7);
    ngf_match_set_mode(&m, NGF_MODE_3V3);
    ngf_team_set(&m, 0, &tester, &tester, &tester);
    ngf_team_set(&m, 1, &tester, &other, &tester);
    run(100, 0, 0); m.f[0].life = 480; m.f[1].h = 500; m.f[1].push_vx = 300;
    ko_side(1);
    assert(m.state == NGF_M_INTRO && m.team_active[1] == 1);
    assert(m.f[1].def == &other && m.f[1].life == 900);
    assert(m.f[1].grounded && m.f[1].h == 0 && m.f[1].push_vx == 0);
    assert(m.f[0].life == 555 && m.f[0].wins == 0);
    assert(m.clock == NGF_ROUND_SECONDS * NGF_SECOND_FRAMES);
    run(100, 0, 0); ko_side(1);
    assert(m.team_active[1] == 2 && m.f[1].life == tester.life);
    run(100, 0, 0); ko_side(1);
    assert(m.state == NGF_M_ROUND_END && m.f[0].wins == 1 && m.f[1].life == 0);
    run(140, 0, 0); assert(m.state == NGF_M_OVER && m.winner == 0);

    ngf_match_init(&m, &tester, &tester, 640, 7);
    ngf_match_set_mode(&m, NGF_MODE_3V3);
    ngf_team_set(&m, 0, &tester, &tester, NULL);
    ngf_team_set(&m, 1, &tester, &tester, NULL);
    run(100, 0, 0); m.f[0].life = 0; ko_side(1);
    assert(m.team_active[0] == 1 && m.team_active[1] == 1 && m.state == NGF_M_INTRO);
    run(100, 0, 0); m.f[0].life = 0; ko_side(1);
    assert(m.round_winner == 2 && m.state == NGF_M_ROUND_END);

    ngf_match_init(&m, &tester, &tester, 640, 7);
    ngf_match_set_mode(&m, NGF_MODE_3V3);
    ngf_team_set(&m, 1, &tester, &other, NULL);
    run(100, 0, 0); m.f[0].life = 300; m.f[1].life = 100;
    m.state = NGF_M_TIME_UP; m.state_timer = 90; run(1, 0, 0);
    assert(m.team_active[1] == 1 && m.f[1].life == 900 && m.team_life[1][0] == 0);

    fight_at(180); ngf_match_set_mode(&m, NGF_MODE_TAG);
    ngf_team_set(&m, 0, &tester, &other, &tester);
    m.f[0].life = 321;
    assert(ngf_tag_switch(&m, 0)); assert(m.f[0].life == 900);
    m.f[0].state = NGF_S_IDLE; m.f[0].life = 654;
    assert(ngf_tag_switch(&m, 0)); assert(m.f[0].life == 600);
    m.f[0].state = NGF_S_IDLE;
    assert(ngf_tag_switch(&m, 0)); assert(m.f[0].life == 321);
    assert(m.team_life[0][1] == 654);
    m.f[0].state = NGF_S_IDLE; m.team_life[0][1] = m.team_life[0][2] = 0;
    assert(!ngf_tag_switch(&m, 0)); assert(!ngf_tag_switch(&m, 255));
}

static void test_simultaneous_contact(void)
{
    fight_at(40); m.f[0].state_timer = 100; run(1, NGF_BTN_A, NGF_BTN_A);
    assert(m.f[0].state_timer == 0); run(8, 0, 0);
    assert(m.f[0].life < tester.life && m.f[1].life < tester.life);
    assert(m.f[0].life == m.f[1].life);
}

static void test_replay_hash(void)
{
    uint32_t before;
    fight_at(120); before = ngf_match_hash(&m);
    m.f[0].frame_timer++;
    assert(ngf_match_hash(&m) != before); m.f[0].frame_timer--;
    m.f[0].input.pending = NGF_BTN_A;
    assert(ngf_match_hash(&m) != before); m.f[0].input.pending = 0;
    m.f[0].max_timer = 1u;
    assert(ngf_match_hash(&m) != before); m.f[0].max_timer = 0u;
    m.f[0].input.kara = NGF_BTN_A;
    assert(ngf_match_hash(&m) != before); m.f[0].input.kara = 0u;
    m.f[0].route_uses[0] = 1u;
    assert(ngf_match_hash(&m) != before); m.f[0].route_uses[0] = 0u;
    m.team_life[0][1] = 321;
    assert(ngf_match_hash(&m) != before); m.team_life[0][1] = 0;
    m.cpu[0].wait++;
    assert(ngf_match_hash(&m) != before);
}

static void test_cpu_determinism(void)
{
    uint32_t n1, n2, a = cpu_match(99, &n1), b = cpu_match(99, &n2);
    assert(a == b && n1 == n2);
    assert(m.state == NGF_M_OVER);
    assert(m.winner <= 2u);
    printf("cpu match: %lu frames, winner %u, rounds %u\n", (unsigned long)n1, m.winner, m.round);
}


static void test_seven_levels_and_round_options(void)
{
    uint8_t level, previous = 255u;
    uint32_t before;
    fight_at(100);
    for (level = 1u; level <= NGF_DIFFICULTY_LEVELS; level++) {
        assert(ngf_set_cpu_difficulty(&m, 1u, level));
        assert(m.cpu[1].difficulty == level);
        assert(ngf_cpu_reaction_frames(level) < previous);
        previous = ngf_cpu_reaction_frames(level);
    }
    before = ngf_match_hash(&m);
    assert(!ngf_set_cpu_difficulty(&m, 2u, 3u));
    assert(!ngf_set_cpu_difficulty(&m, 1u, 0u));
    assert(!ngf_set_cpu_difficulty(&m, 1u, 8u));
    assert(before == ngf_match_hash(&m));
    assert(ngf_set_cpu_difficulty(&m, 1u, NGF_DIFFICULTY_DEFAULT));
    assert(ngf_set_round_seconds(&m, 90u));
    ngf_round_start(&m);
    assert(m.cpu[1].difficulty == 3u);
    assert(m.seconds == 90u && m.clock == 90u * 60u);
    assert(!ngf_set_round_seconds(&m, 0u));
    assert(!ngf_set_round_seconds(&m, 100u));
    assert(m.round_seconds == 90u);
    ngf_set_cpu(&m, 1u, 255u);
    assert(m.cpu[1].level == 9u && m.cpu[1].difficulty == 0u);
}

static void test_training_modes_and_recording(void)
{
    NGFTraining t;
    uint16_t i;
    int32_t player_x;
    fight_at(50);
    ngf_training_init(&t, &m);
    assert(m.seconds == 99u && m.clock == 99u * NGF_SECOND_FRAMES);
    assert(m.training && m.state == NGF_M_FIGHT);
    m.f[1].x = m.f[0].x + 50 * 256;
    m.f[1].life = 1u;
    ngf_training_step(&t, &m, NGF_BTN_A, 0u);
    for (i = 0u; i < 70u; i++) ngf_training_step(&t, &m, 0u, 0u);
    assert(m.state == NGF_M_FIGHT && m.f[1].life > 0u);
    assert(m.f[0].power == NGF_POWER_MAX && m.seconds >= 98u);
    assert(ngf_training_set_dummy(&t, &m, NGF_DUMMY_RECORD));
    ngf_training_reset(&t, &m);
    player_x = m.f[0].x;
    for (i = 0u; i < 30u; i++) ngf_training_step(&t, &m, NGF_RAW_RIGHT, 0u);
    assert(t.record_size == 30u && m.f[0].x == player_x);
    for (i = 0u; i < 30u; i++) assert(t.record[i] == NGF_RAW_RIGHT);
    assert(ngf_training_set_dummy(&t, &m, NGF_DUMMY_PLAYBACK));
    for (i = 0u; i < 90u; i++) ngf_training_step(&t, &m, 0u, 0u);
    assert(t.record_pos == 0u && t.record_size == 30u);
    assert(ngf_training_set_dummy(&t, &m, NGF_DUMMY_RECORD));
    for (i = 0u; i < 300u; i++) ngf_training_step(&t, &m, 0u, 0u);
    assert(t.record_size == NGF_TRAIN_RECORD_FRAMES);
    assert(!ngf_training_set_dummy(&t, &m, NGF_DUMMY_COUNT));
    assert(ngf_training_set_dummy(&t, &m, NGF_DUMMY_CPU));
    assert(m.cpu[1].difficulty == 3u && m.f[1].cpu);
    assert(ngf_training_set_dummy(&t, &m, NGF_DUMMY_PLAYER2));
    assert(!m.f[1].cpu);
    assert(ngf_training_set_dummy(&t, &m, NGF_DUMMY_JUMP));
    ngf_training_reset(&t, &m);
    for (i = 0u; i < 8u; i++) ngf_training_step(&t, &m, 0u, 0u);
    assert(!m.f[1].grounded && m.f[1].h > 0);
    printf("training: nonlethal combat, timers, resource refill, record/playback and dummy modes: ok\n");
}

static void test_retreat_is_not_proximity_locked(void)
{
    uint8_t side;
    for (side = 0u; side < 2u; side++) {
        NGFFighter *f, *o;
        int32_t before;
        uint8_t back = side ? NGF_RAW_RIGHT : NGF_RAW_LEFT;
        fight_at(120);
        f = &m.f[side]; o = &m.f[side ^ 1u]; before = f->x;
        /* A live distant projectile and the opponent starting a heavy out of reach. */
        m.proj[0].active = 1u; m.proj[0].owner = (uint8_t)(side ^ 1u);
        m.proj[0].def = &projectiles[0]; m.proj[0].life = 50u;
        m.proj[0].x = o->x; m.proj[0].h = 120 * 256;
        ngf_match_step(&m, side ? NGF_BTN_C : back, side ? back : NGF_BTN_C);
        assert(f->vx == (side ? tester.walk_back : -tester.walk_back));
        assert(f->anim == NGF_A_WALK_B);
        assert(side ? f->x > before : f->x < before);
        /* Within reach during active startup: proximity guard appears in place. */
        fight_at(70);
        f = &m.f[side]; o = &m.f[side ^ 1u];
        ngf_match_step(&m, side ? NGF_BTN_C : 0u, side ? 0u : NGF_BTN_C);
        before = f->x;
        ngf_match_step(&m, side ? 0u : back, side ? back : 0u);
        assert(f->vx == 0 && f->anim == NGF_A_GUARD && f->x == before);
        /* Once the strike has no active frames remaining (recovery), retreat resumes. */
        o->frame = 2u; o->frame_timer = 0u;
        ngf_match_step(&m, side ? 0u : back, side ? back : 0u);
        assert(f->vx == (side ? tester.walk_back : -tester.walk_back));
        assert(f->anim == NGF_A_WALK_B);
    }
}

static void test_event_pressure_preserves_ko(void)
{
    uint8_t i, found = 0u;
    NGFEvent ev;
    fight_at(50);
    m.ev_count = NGF_EVENTS;
    for (i = 0u; i < NGF_EVENTS; i++) {
        memset(&m.ev[i], 0, sizeof(m.ev[i]));
        m.ev[i].type = NGF_EV_MOVE;
    }
    m.f[1].life = 1u;
    run(1, NGF_BTN_A, 0u); run(20, 0u, 0u);
    assert(m.f[1].life == 0u);
    assert(m.events_dropped > 0u && !m.events_critical_dropped && m.events_peak == NGF_EVENTS);
    while (ngf_pop_event(&m, &ev)) if (ev.type == NGF_EV_KO) found++;
    assert(found == 1u);
}

static void test_authored_cancel_routes(void)
{
    static const NGFCancelRule routes[] = {
        {6u, 5u, 6u, 11u, NGF_CONTACT_HIT, 1u}, /* reverse array order */
        {5u, 6u, 3u, 8u, NGF_CONTACT_BLOCK, 1u},
        {6u, 0u, 6u, 11u, NGF_CONTACT_HIT, 1u},
    };
    static const NGFCombatRules policy = {routes, 3u, 2u, 1u, 6u, 0, 0u, 0u};
    NGFFighterDef authored = tester;
    NGFFighter *f;
    uint8_t rule, side;
    authored.combat = &policy;
    for (side = 0u; side < 2u; side++) {
        fight_at(100);
        f = &m.f[side]; f->def = &authored;
        f->state = NGF_S_ATTACK; f->move_index = 6u; f->move = &moves[6];
        f->anim = moves[6].anim; f->frame = 1u; f->frame_timer = 0u;
        f->state_timer = 5u; f->move_contact = NGF_CONTACT_HIT; f->move_hit = 1u;
        assert(ngf_cancel_check(f, 5u, &rule) == NGF_CANCEL_WINDOW);
        f->state_timer = 6u; f->move_contact = NGF_CONTACT_BLOCK;
        assert(ngf_cancel_check(f, 5u, &rule) == NGF_CANCEL_CONTACT);
        f->move_contact = NGF_CONTACT_HIT;
        assert(ngf_cancel_check(f, 5u, &rule) == NGF_CANCEL_OK && rule == 0u);
        assert(ngf_cancel_check(f, 0u, 0) == NGF_CANCEL_RESOURCE);
        f->power = NGF_POWER_STOCK;
        assert(ngf_cancel_check(f, 0u, 0) == NGF_CANCEL_OK);
        f->route_uses[0] = 1u;
        assert(ngf_cancel_check(f, 5u, 0) == NGF_CANCEL_REPEAT);
        f->route_uses[0] = 0u; f->state_timer = 12u;
        assert(ngf_cancel_check(f, 5u, 0) == NGF_CANCEL_WINDOW);
        f->state_timer = 6u;
        ngf_match_step(&m, side ? 0u : NGF_BTN_A, side ? NGF_BTN_A : 0u);
        assert(f->move_index == 5u && f->route_uses[0] == 1u);
        assert(!f->move_contact);
        ngf_round_start(&m);
        assert(!f->route_uses[0] && !f->juggle_points && !f->move_contact);
    }
    fight_at(50);
    ngf_striker_set(&m, 0u, &authored);
    assert(ngf_call_striker(&m, 0u));
    assert(m.striker[0].move_index == 5u); /* Selected jab, not the strongest normal. */
}

static void test_juggle_budget_and_landing_reset(void)
{
    static const NGFCombatRules policy = {0, 0u, 2u, 1u, 0u, 0, 0u, 0u};
    NGFFighterDef authored = tester;
    uint16_t life;
    authored.combat = &policy;
    fight_at(50);
    m.f[1].def = &authored; m.f[1].state = NGF_S_FALL;
    m.f[1].grounded = 0u; m.f[1].h = 30 * 256; m.f[1].vh = 0;
    m.f[1].anim = NGF_A_FALL; m.f[1].frame = 0u;
    m.f[1].juggle_points = 2u;
    m.f[0].state = NGF_S_ATTACK; m.f[0].move = &moves[5]; m.f[0].move_index = 5u;
    m.f[0].anim = moves[5].anim; m.f[0].frame = 1u; m.f[0].frame_timer = 0u;
    life = m.f[1].life;
    run(1, 0u, 0u);
    assert(m.f[1].life == life && !m.f[0].move_hit);
    m.f[1].juggle_points = 1u; m.f[1].relaunches = 1u;
    run(1, 0u, 0u);
    assert(m.f[1].life < life && m.f[1].juggle_points == 2u);
    assert(m.f[1].vh <= 0); /* Budgeted hit cannot add another upward launch. */
    run(90, 0u, 0u);
    assert(m.f[1].grounded && !m.f[1].juggle_points && !m.f[1].relaunches);
}

static void test_throw_anchors_and_corners(void)
{
    static const NGFThrowAnchor anchors[] = {
        {3u, 24u, 32u, 6u, 36u, 5u}
    };
    static const NGFCombatRules policy = {0, 0u, 2u, 1u, 0u, anchors, 1u, 0u};
    NGFFighterDef authored = tester, wide = tester;
    NGFMove back_moves[sizeof(moves) / sizeof(moves[0])];
    memcpy(back_moves, moves, sizeof(moves));
    back_moves[3].motion = NGF_MOTION_BACK;
    authored.combat = &policy;
    wide.push_w = 28u;
    /* Authored grip, wider opponent adjustment, and peak lift. */
    fight_at(40);
    m.f[0].def = &authored; m.f[1].def = &wide;
    run(1, NGF_RAW_RIGHT | NGF_BTN_C, 0u);
    assert(m.f[1].state == NGF_S_THROWN);
    assert(ngf_fighter_px(&m.f[1]) - ngf_fighter_px(&m.f[0]) == 24 + (28 - 20));
    run(6, 0u, 0u);
    assert(ngf_fighter_py(&m.f[1]) >= 31 && ngf_fighter_py(&m.f[1]) <= 32);
    /* Corner back-throw keeps thrower and target separated at the stage wall. */
    authored.moves = back_moves;
    fight_at(40);
    m.f[0].def = &authored;
    m.f[0].x = 24 * 256; m.f[1].x = 64 * 256;
    m.f[0].facing_left = 0u; m.f[1].facing_left = 1u;
    run(1, NGF_RAW_LEFT | NGF_BTN_C, 0u);
    assert(m.f[1].state == NGF_S_THROWN);
    run(7, 0u, 0u);
    assert(ngf_fighter_px(&m.f[1]) == 16 && ngf_fighter_px(&m.f[0]) == 40);
}

int main(void)
{
    test_motions();
    test_hit_and_block();
    test_throw_projectile_super();
    test_frame_contacts();
    test_ordered_rotations();
    test_720_and_teams();
    test_freeze_and_input_buffer();
    test_assist_collision();
    test_team_lifecycle();
    test_timer_draw();
    test_simultaneous_contact();
    test_replay_hash();
    test_cpu_determinism();
    test_seven_levels_and_round_options();
    test_training_modes_and_recording();
    test_retreat_is_not_proximity_locked();
    test_event_pressure_preserves_ko();
    test_authored_cancel_routes();
    test_juggle_budget_and_landing_reset();
    test_throw_anchors_and_corners();
    printf("fight_test: ok (NGFMatch %u bytes)\n", (unsigned)sizeof(NGFMatch));
    return 0;
}
