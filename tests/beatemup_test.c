/* Host checks for ng_beatemup: hits, crowd, grabs, vehicle, waves, bosses. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ng_beatemup.h"

static NGBeatEmUp B, C;
static int ev[64];

/*              su ac re dmg hs flags                         bx bw bz bh bd  px  pz lunge cancel next  snd anim */
static const NGBMove P_MOVES[] = {
    /* 0 jab   */ { 3, 2, 6, 3, 14, 0,                         4, 30, 20, 30, 8, 16,  0,  0,  5,  1,  0, 0 },
    /* 1 jab2  */ { 3, 2, 7, 3, 14, 0,                         4, 30, 20, 30, 8, 16,  0,  0,  5,  2,  0, 0 },
    /* 2 hook  */ { 5, 3, 12, 6, 0, NGB_MV_KNOCKDOWN,          4, 32, 20, 30, 8, 40, 40,  0,  0, NGB_NONE, 0, 0 },
    /* 3 air   */ { 2, 8, 4, 5, 0, NGB_MV_KNOCKDOWN,           0, 30,  0, 30, 8, 32, 24,  0,  0, NGB_NONE, 0, 0 },
    /* 4 low   */ { 4, 3, 10, 4, 0, NGB_MV_LOW | NGB_MV_KNOCKDOWN, 4, 30, 0, 12, 8, 24, 16, 0, 0, NGB_NONE, 0, 0 },
    /* 5 dash  */ { 3, 6, 10, 6, 0, NGB_MV_KNOCKDOWN,          0, 32, 10, 40, 8, 48, 32, 40,  0, NGB_NONE, 0, 0 },
    /* 6 spec  */ { 4, 10, 12, 8, 0, NGB_MV_KNOCKDOWN | NGB_MV_UNBLOCKABLE | NGB_MV_INVULN, -48, 96, 0, 60, 16, 48, 48, 0, 0, NGB_NONE, 0, 0 },
    /* 7 pummel*/ { 2, 2, 6, 2, 10, 0,                         0, 30, 10, 40, 8,  0,  0,  0,  0, NGB_NONE, 0, 0 },
};
/* max_hp, speed, depth_speed, jump, half_w, height, half_depth, weight */
static const NGBFighterDef PLAYER = { 40, 24, 16, 64, 12, 56, 4, 3, P_MOVES, 8, 0, 3, 4, 5, 6, 7, NGB_NONE, NGB_NONE,
                                      0, 0, 0, {0}, 0, 0, 0, NULL, 0, NULL };

static const NGBMove E_MOVES[] = {
    /* 0 punch */ { 10, 3, 14, 4, 12, 0,                       4, 30, 20, 30, 8, 16,  0,  0,  0, NGB_NONE, 0, 0 },
    /* 1 charge*/ { 12, 30, 20, 6, 0, NGB_MV_KNOCKDOWN | NGB_MV_RUN, 0, 30, 0, 50, 8, 40, 32, 64, 0, NGB_NONE, 0, 0 },
    /* 2 shot  */ { 14, 1, 20, 5, 0, NGB_MV_PROJECTILE,        8, 0, 30, 10, 8,  0,  0, 48,  0, NGB_NONE, 0, 0 },
    /* 3 grab  */ { 8, 4, 16, 0, 0, NGB_MV_GRAB,               4, 24, 0, 50, 8,  0,  0,  0,  0, NGB_NONE, 0, 0 },
    /* 4 slam  */ { 16, 4, 24, 10, 0, NGB_MV_KNOCKDOWN,        0, 48, 0, 60, 12, 48, 48, 0, 0, NGB_NONE, 0, 0 },
};
static const NGBFighterDef PUNK = { 20, 16, 12, 56, 12, 56, 4, 1, E_MOVES, 5, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                    NGB_AI_BRAWLER, 28, 8, {0}, 1, 0, 100, NULL, 0, NULL };
static const NGBFighterDef BIKER = { 20, 18, 12, 56, 12, 56, 4, 1, E_MOVES, 5, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                     NGB_AI_CHARGER, 28, 8, {1}, 1, 0, 150, NULL, 0, NULL };
static const NGBFighterDef DRONE = { 12, 14, 12, 56, 12, 56, 4, 1, E_MOVES, 5, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                     NGB_AI_RANGED, 28, 8, {2}, 1, 0, 120, NULL, 0, NULL };
static const NGBFighterDef GUARD = { 30, 12, 10, 56, 14, 60, 4, 2, E_MOVES, 5, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                     NGB_AI_SHIELD, 28, 8, {0}, 1, NGB_FD_SHIELD, 200, NULL, 0, NULL };
static const NGBFighterDef GRAPPLER = { 30, 14, 10, 56, 14, 60, 4, 3, E_MOVES, 5, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                        NGB_AI_GRAPPLER, 24, 8, {3}, 1, 0, 200, NULL, 0, NULL };
static const NGBBossPhase PHASES[3] = {
    { 100, 0, {0, 4}, 2, 0, NGB_FD_ARMOR, 1 },
    { 66, 60, {4}, 1, 4, NGB_FD_ARMOR, 2 },
    { 33, 60, {1, 4}, 2, 8, 0, 3 },
};
static const NGBFighterDef BOSS = { 90, 14, 10, 56, 24, 90, 8, 8, E_MOVES, 5, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE,
                                    NGB_AI_BOSS, 36, 12, {0}, 1, NGB_FD_HEAVY, 5000, PHASES, 3, NULL };

static const NGBMove PIPE_SWING = { 4, 3, 10, 6, 0, NGB_MV_KNOCKDOWN, 4, 40, 10, 40, 8, 32, 24, 0, 0, NGB_NONE, 0, 0 };
static const NGBWeaponDef WEAPONS[2] = { { &PIPE_SWING, 3, 10, 0 }, { &PIPE_SWING, 2, 8, 1 } };
static const NGBFighterDef *const ENEMIES[] = { &PUNK, &GUARD, &DRONE, &BOSS };
static const NGBWave WAVES[3] = {
    { 0, NGB_WAVE_LOCK, 3, {0, 0, 0}, 0x2, 20 },
    { 400, 0, 2, {1, 2}, 0x1, 30 },
    { 800, NGB_WAVE_LOCK | NGB_WAVE_BOSS, 1, {3}, 0, 0 },
};
static const NGBPropSpawn PROPS[] = {
    { NGB_PROP_CRATE, 170, 200, NGB_PROP_FOOD, 2 },
    { NGB_PROP_WEAPON, 180, 260, 0, 0 },
    { NGB_PROP_VEHICLE, 160, 300, 0, 0 },
    { NGB_PROP_HAZARD, 150, 520, 0, 0 },
    { NGB_PROP_RAMP, 170, 600, 0, 0 },
};
static NGBStage STAGE, EMPTY;

static void setup(void)
{
    memset(&STAGE, 0, sizeof(STAGE));
    STAGE.length = 1200; STAGE.y_min = 140; STAGE.y_max = 200; STAGE.view_w = 320;
    STAGE.waves = WAVES; STAGE.nwaves = 3;
    STAGE.props = PROPS; STAGE.nprops = sizeof(PROPS) / sizeof(PROPS[0]);
    STAGE.enemies = ENEMIES; STAGE.nenemies = 4;
    STAGE.weapons = WEAPONS; STAGE.nweapons = 2;
    STAGE.max_attackers = 2; STAGE.max_active = 6; STAGE.camera_max = 6;
    STAGE.vehicle_max = 96; STAGE.vehicle_accel = 4; STAGE.vehicle_durability = 3; STAGE.vehicle_ram = 6;
    memcpy(&EMPTY, &STAGE, sizeof(STAGE));
    EMPTY.nwaves = 0; EMPTY.nprops = 0;
}

static void drain(NGBeatEmUp *b) { NGBEvent e; while (ng_beatemup_event(b, &e)) ev[e.type]++; }

static void step(NGBeatEmUp *b, uint8_t dirs, uint8_t btns, int n)
{
    int i;
    for (i = 0; i < n; i++) { ng_beatemup_input(b, dirs, btns); ng_beatemup_update(b); drain(b); }
}

static void tap(NGBeatEmUp *b, uint8_t dirs, uint8_t btn) { step(b, dirs, btn, 1); step(b, dirs, 0, 1); }

static uint8_t enemy_at(NGBeatEmUp *b, const NGBFighterDef *d, int x, int y)
{
    int r = ng_beatemup_spawn(b, d, (int16_t)x, (uint8_t)y, 2);
    assert(r > 0);
    b->f[r].ai = NGB_AI_WAIT;
    return (uint8_t)r;
}

static void fresh(void)
{
    assert(ng_beatemup_init(&B, &EMPTY, &PLAYER, 3, 11) == NGB_OK);
    memset(ev, 0, sizeof(ev));
}


static void test_chain_and_volumes(void)
{
    uint8_t e;
    int i;
    fresh();
    e = enemy_at(&B, &PUNK, 64 + 28, 170);
    B.f[e].hp = 200;
    B.f[0].y = 170 << 8;
    B.attack_cool = 255;                     /* the punk never gets a turn */
    /* A, A, A: jab, jab2, hook (buffered presses start the next at its cancel frame) */
    tap(&B, 0, NGB_BTN_A);
    tap(&B, 0, NGB_BTN_A);
    tap(&B, 0, NGB_BTN_A);
    for (i = 0; i < 60; i++) { if (B.f[0].state == NGB_ST_ATTACK && B.f[0].move_frame == 0) {} step(&B, 0, i < 20 && (i & 3) == 0 ? NGB_BTN_A : 0, 1); }
    assert(ev[NGB_EV_HIT] >= 3 && ev[NGB_EV_KNOCKDOWN] == 1 && ev[NGB_EV_COMBO] >= 2);
    assert(B.f[e].state == NGB_ST_FALL || B.f[e].state == NGB_ST_DOWN);
    printf("attack chain with buffered presses, combo, knockdown: OK\n");

    /* depth: same x, 30 pixels across the belt: no hit */
    fresh();
    e = enemy_at(&B, &PUNK, 64 + 28, 200);
    B.f[0].y = 140 << 8; B.attack_cool = 255;
    tap(&B, 0, NGB_BTN_A);
    step(&B, 0, 0, 20);
    assert(ev[NGB_EV_HIT] == 0);
    /* height: a jab misses someone high in the air */
    fresh();
    e = enemy_at(&B, &PUNK, 64 + 28, 170);
    B.f[0].y = 170 << 8; B.attack_cool = 255;
    B.f[e].z = 120 << 8; B.f[e].vz = 0; B.f[e].state = NGB_ST_JUMP;
    tap(&B, 0, NGB_BTN_A);
    step(&B, 0, 0, 4);
    assert(ev[NGB_EV_HIT] == 0);
    printf("depth and height keep strikes apart: OK\n");

    /* hitstop freezes both */
    fresh();
    e = enemy_at(&B, &PUNK, 64 + 28, 170);
    B.f[0].y = 170 << 8; B.attack_cool = 255;
    tap(&B, 0, NGB_BTN_A);
    for (i = 0; i < 10 && !ev[NGB_EV_HIT]; i++) step(&B, 0, 0, 1);
    assert(ev[NGB_EV_HIT] == 1 && B.f[0].hitstop && B.f[e].hitstop);
    {
        int32_t ex = B.f[e].x;
        uint8_t mf = B.f[0].move_frame;
        step(&B, 0, 0, 1);
        assert(B.f[e].x == ex && B.f[0].move_frame == mf);
    }
    printf("hitstop holds attacker and target: OK\n");

    /* knocked down: only low attacks touch them; up again, briefly untouchable */
    fresh();
    e = enemy_at(&B, &PUNK, 64 + 28, 170);
    B.f[0].y = 170 << 8; B.attack_cool = 255; B.f[e].hp = 100;
    B.f[e].state = NGB_ST_DOWN; B.f[e].timer = 0;
    tap(&B, 0, NGB_BTN_A);
    step(&B, 0, 0, 12);
    assert(ev[NGB_EV_HIT] == 0);
    B.f[e].state = NGB_ST_DOWN; B.f[e].timer = 0;
    tap(&B, NGB_IN_DOWN, NGB_BTN_A);
    step(&B, 0, 0, 10);
    assert(ev[NGB_EV_HIT] == 1);
    step(&B, 0, 0, 100);
    assert(ev[NGB_EV_GETUP] >= 1);
    memset(ev, 0, sizeof(ev));
    B.f[e].state = NGB_ST_GETUP; B.f[e].timer = 0; B.f[e].invuln = 40;
    tap(&B, 0, NGB_BTN_A);
    step(&B, 0, 0, 10);
    assert(ev[NGB_EV_HIT] == 0);
    printf("down only to low attacks, invulnerable get-up: OK\n");

    /* beaten (no hit points left): low attacks on the fallen one don't knock it
     * down again and again, so it goes out (a boss once never did) */
    fresh();
    e = enemy_at(&B, &PUNK, 64 + 28, 170);
    B.f[0].y = 170 << 8; B.attack_cool = 255;
    B.f[e].hp = 0; B.f[e].state = NGB_ST_DOWN; B.f[e].timer = 0;
    for (i = 0; i < 30 && !ev[NGB_EV_KO]; i++) { tap(&B, NGB_IN_DOWN, NGB_BTN_A); step(&B, 0, 0, 4); }
    assert(ev[NGB_EV_KO] == 1 && ev[NGB_EV_HIT] == 0 && !B.f[e].alive);
    printf("a beaten fighter on the ground can't be hit again, and goes out: OK\n");
}

static void test_grabs(void)
{
    uint8_t e, e2;
    int i;
    fresh();
    e = enemy_at(&B, &PUNK, 64 + 14, 170);
    e2 = enemy_at(&B, &PUNK, 64 + 50, 170);
    B.f[0].y = 170 << 8; B.attack_cool = 255;
    step(&B, NGB_IN_RIGHT, 0, 10);                /* walk into him */
    assert(B.f[0].state == NGB_ST_GRAB && B.f[e].state == NGB_ST_GRABBED && ev[NGB_EV_GRAB] == 1);
    for (i = 0; i < 3; i++) { tap(&B, 0, NGB_BTN_A); step(&B, 0, 0, 12); }
    assert(ev[NGB_EV_THROW] == 1);                /* the third pummel throws */
    step(&B, 0, 0, 60);
    assert(B.f[e].hp == 20 - 3 * 2 - 8);         /* three pummels and the landing */
    assert(B.f[e2].hp < 20);                      /* thrown into his friend */
    printf("walk-in grab, pummels, throw into another: OK\n");

    /* back throw */
    fresh();
    e = enemy_at(&B, &PUNK, 64 + 14, 170);
    B.f[0].y = 170 << 8; B.attack_cool = 255;
    B.f[0].facing = 0;
    tap(&B, 0, NGB_BTN_C);
    assert(B.f[0].state == NGB_ST_GRAB);
    tap(&B, NGB_IN_LEFT, NGB_BTN_C);
    assert(ev[NGB_EV_THROW] == 1 && B.f[0].facing == 1 && B.f[e].vx < 0);
    printf("back throw: OK\n");

    /* a grappler takes hold of the player: mash free */
    fresh();
    e = enemy_at(&B, &GRAPPLER, 64 + 20, 170);
    B.f[0].y = 170 << 8;
    B.f[e].token = 1; B.f[e].ai = NGB_AI_APPROACH;
    for (i = 0; i < 200 && B.f[0].state != NGB_ST_GRABBED; i++) step(&B, 0, 0, 1);
    assert(B.f[0].state == NGB_ST_GRABBED && ev[NGB_EV_TELEGRAPH] >= 1);
    for (i = 0; i < 8; i++) tap(&B, 0, NGB_BTN_B);
    assert(B.f[0].state != NGB_ST_GRABBED && ev[NGB_EV_ESCAPE] == 1);
    printf("grappler: telegraphed hold, mashed free: OK\n");
}

static void test_shield_and_special(void)
{
    uint8_t e;
    fresh();
    e = enemy_at(&B, &GUARD, 64 + 28, 170);
    B.f[0].y = 170 << 8; B.attack_cool = 255;
    B.f[e].facing = 1;                             /* facing the player */
    tap(&B, 0, NGB_BTN_A);
    step(&B, 0, 0, 12);
    assert(ev[NGB_EV_BLOCKED] == 1 && ev[NGB_EV_HIT] == 0);
    /* a shield doesn't cover a guard in the middle of its own attack */
    step(&B, 0, 0, 20);
    B.f[e].state = NGB_ST_ATTACK; B.f[e].move = 4; B.f[e].move_frame = 0;   /* a slow slam */
    tap(&B, 0, NGB_BTN_A);
    step(&B, 0, 0, 6);
    assert(ev[NGB_EV_HIT] == 1);
    /* nor against a knockdown blow */
    step(&B, 0, 0, 120);
    memset(ev, 0, sizeof(ev));
    B.f[e].state = NGB_ST_IDLE; B.f[e].invuln = 0; B.f[e].x = B.f[0].x + (28 << 8); B.f[e].y = B.f[0].y; B.f[e].z = 0;
    B.f[0].state = NGB_ST_ATTACK; B.f[0].facing = 0; B.f[0].move = 2; B.f[0].move_frame = 0; B.f[0].serial++;
    step(&B, 0, 0, 10);
    assert(ev[NGB_EV_HIT] == 1 && ev[NGB_EV_BLOCKED] == 0);
    /* the special costs health, clears both sides and can't be hit */
    fresh();
    e = enemy_at(&B, &PUNK, 64 - 30, 170);
    enemy_at(&B, &PUNK, 64 + 30, 170);
    B.f[0].y = 170 << 8; B.attack_cool = 255;
    tap(&B, 0, NGB_BTN_D);
    assert(B.f[0].hp == 38 && B.f[0].invuln > 0 && ev[NGB_EV_SPECIAL] == 1);
    step(&B, 0, 0, 30);
    assert(ev[NGB_EV_KNOCKDOWN] == 2);
    printf("shield blocks from the front only, special on both sides: OK\n");
}

static void test_crowd(void)
{
    int i, max_att = 0, telegraphs = 0;
    fresh();
    for (i = 0; i < 6; i++) enemy_at(&B, &PUNK, 64 + (i & 1 ? 120 : -40) + 10 * i, 150 + 8 * i);
    B.f[0].x = 160 << 8; B.f[0].y = 170 << 8;
    for (i = 0; i < 900; i++) {
        uint8_t k, att = 0;
        B.f[0].invuln = 10; B.f[0].hp = 40;        /* the player just stands there */
        step(&B, 0, 0, 1);
        for (k = 1; k < NGB_FIGHTERS; k++) if (B.f[k].alive && B.f[k].state == NGB_ST_ATTACK) att++;
        if (att > max_att) max_att = att;
    }
    telegraphs = ev[NGB_EV_TELEGRAPH];
    assert(max_att <= 2 && telegraphs > 4);
    /* the ones waiting don't stand on each other */
    {
        uint8_t a, c;
        for (a = 1; a < NGB_FIGHTERS; a++)
            for (c = (uint8_t)(a + 1); c < NGB_FIGHTERS; c++) {
                NGBFighter *x = &B.f[a], *y = &B.f[c];
                if (!x->alive || !y->alive || x->token || y->token || x->ai != NGB_AI_WAIT || y->ai != NGB_AI_WAIT) continue;
                if (x->state != NGB_ST_IDLE || y->state != NGB_ST_IDLE) continue;
                assert(!(((x->x - y->x) < (8 << 8)) && ((y->x - x->x) < (8 << 8)) && ((x->y - y->y) < (4 << 8)) && ((y->y - x->y) < (4 << 8))));
            }
    }
    printf("crowd: at most %d attacking at once, %d telegraphed attacks, waiters spaced: OK\n", max_att, telegraphs);

    /* a player who keeps getting hit goes down instead of being held in hitstun */
    fresh();
    {
        uint8_t e = enemy_at(&B, &PUNK, 64 + 28, 170);
        NGBMove jab;
        memcpy(&jab, &E_MOVES[0], sizeof(jab));
        B.f[0].y = 170 << 8;
        (void)e;
        for (i = 0; i < 3; i++) {
            B.f[0].state = NGB_ST_IDLE; B.f[0].invuln = 0; B.f[0].hitstop = 0; B.f[e].hitstop = 0;
            B.f[e].x = B.f[0].x + (28 << 8); B.f[e].y = B.f[0].y;
            B.f[e].facing = 1; B.f[e].state = NGB_ST_ATTACK; B.f[e].move = 0; B.f[e].move_frame = 10; B.f[e].serial++;
            step(&B, 0, 0, 1);
        }
        assert(B.f[0].state == NGB_ST_FALL || B.f[0].state == NGB_ST_DOWN);
    }
    printf("third hit in a row knocks the player down: OK\n");
}

static void test_lives(void)
{
    int i;
    fresh();
    B.lives = 2;
    B.f[0].hp = 1;
    {
        uint8_t e = enemy_at(&B, &PUNK, 64 + 28, 170);
        B.f[0].y = 170 << 8;
        B.f[e].facing = 1; B.f[e].state = NGB_ST_ATTACK; B.f[e].move = 0; B.f[e].move_frame = 10;
    }
    step(&B, 0, 0, 80);
    assert(ev[NGB_EV_PLAYER_DOWN] == 1 && B.lives == 1);
    assert(ng_beatemup_status(&B) == NGB_PLAYER_OUT);
    step(&B, 0, 0, 100);
    assert(ev[NGB_EV_RESPAWN] == 1 && B.f[0].hp == 40 && B.f[0].invuln > 0);
    B.f[0].hp = 0; B.f[0].state = NGB_ST_DOWN; B.f[0].timer = 0; B.f[0].invuln = 0;
    for (i = 0; i < 60; i++) step(&B, 0, 0, 1);
    assert(ev[NGB_EV_GAME_OVER] == 1 && ng_beatemup_status(&B) == NGB_GAME_OVER);
    ng_beatemup_continue(&B, 3);
    assert(ng_beatemup_status(&B) == NGB_PLAYING && B.lives == 3 && B.f[0].hp == 40);
    printf("lives, respawn, game over, continue: OK\n");
}

static void test_vehicle_and_props(void)
{
    uint8_t e;
    int i;
    assert(ng_beatemup_init(&B, &STAGE, &PLAYER, 3, 5) == NGB_OK);
    memset(ev, 0, sizeof(ev));
    B.wave_next = 3;                              /* no waves here */
    /* the crate breaks into food */
    B.f[0].x = 172 << 8; B.f[0].y = 170 << 8; B.f[0].facing = 0;
    for (i = 0; i < 4; i++) { tap(&B, 0, NGB_BTN_A); step(&B, 0, 0, 16); }
    assert(ev[NGB_EV_BREAK] == 1 && B.prop[0].kind == NGB_PROP_FOOD);
    B.f[0].hp = 20;
    B.f[0].x = B.prop[0].x; B.f[0].y = B.prop[0].y;
    tap(&B, 0, NGB_BTN_C);
    assert(ev[NGB_EV_FOOD] == 1 && B.f[0].hp == 32 && B.prop[0].kind == NGB_PROP_NONE);
    /* the pipe: swing it until it breaks */
    B.f[0].x = 260 << 8; B.f[0].y = 180 << 8;
    tap(&B, 0, NGB_BTN_C);
    assert(ev[NGB_EV_PICKUP] == 1 && B.f[0].weapon == 1);
    e = enemy_at(&B, &PUNK, 260 + 30, 180);
    B.f[e].hp = 200; B.attack_cool = 255;
    for (i = 0; i < 6 && B.f[0].weapon != NGB_NONE; i++) {
        B.f[e].x = (260 + 30) << 8; B.f[e].y = 180 << 8; B.f[e].state = NGB_ST_IDLE; B.f[e].z = 0; B.f[e].invuln = 0;
        B.f[0].x = 260 << 8; B.f[0].facing = 0;
        tap(&B, 0, NGB_BTN_A);
        step(&B, 0, 0, 20);
    }
    assert(ev[NGB_EV_WEAPON_BREAK] == 1 && B.f[0].weapon == NGB_NONE);
    B.f[e].alive = 0;
    printf("crate to food, weapon swings until it breaks: OK\n");

    /* the scooter: mount, speed up, ram, ramp, break */
    B.f[0].x = 300 << 8; B.f[0].y = 160 << 8;
    tap(&B, 0, NGB_BTN_C);
    assert(B.f[0].state == NGB_ST_RIDE && ev[NGB_EV_MOUNT] == 1);
    e = enemy_at(&B, &PUNK, 420, 160);
    B.f[e].hitstop = 200;                          /* stands still to be rammed */
    step(&B, NGB_IN_RIGHT, 0, 40);
    assert(ev[NGB_EV_RAM] == 1 && (B.f[e].state == NGB_ST_FALL || B.f[e].state == NGB_ST_DOWN));
    assert(B.prop[2].hp == 2);
    /* the ramp sends it up */
    B.f[0].y = 170 << 8;
    for (i = 0; i < 80 && B.f[0].x < (600 << 8); i++) step(&B, NGB_IN_RIGHT, 0, 1);
    step(&B, NGB_IN_RIGHT, 0, 4);
    assert(ev[NGB_EV_JUMP] >= 1);
    tap(&B, 0, NGB_BTN_C);
    assert(ev[NGB_EV_DISMOUNT] == 1 && B.f[0].state != NGB_ST_RIDE);
    /* ram until it breaks: the wreck is the stage's last weapon */
    B.f[0].x = B.prop[2].x; B.f[0].y = B.prop[2].y;
    step(&B, 0, 0, 30);
    tap(&B, 0, NGB_BTN_C);
    assert(B.f[0].state == NGB_ST_RIDE);
    for (i = 0; i < 2; i++) {
        uint8_t t = enemy_at(&B, &PUNK, (int)(B.f[0].x >> 8) + 40, (int)(B.f[0].y >> 8));
        B.f[t].hitstop = 200;
        B.camera_x = B.f[0].x - (100 << 8);
        step(&B, NGB_IN_RIGHT, 0, 30);
        B.f[t].alive = 0;
    }
    assert(ev[NGB_EV_VEHICLE_BREAK] == 1 && B.prop[2].kind == NGB_PROP_WEAPON && B.prop[2].contents == 1);
    assert(B.f[0].state != NGB_ST_RIDE);
    printf("scooter: mount, ram, ramp jump, dismount, break into a wreck: OK\n");

    /* hazards hurt every 32 frames */
    memset(ev, 0, sizeof(ev));
    B.f[0].x = 520 << 8; B.f[0].y = 150 << 8; B.f[0].invuln = 0; B.f[0].state = NGB_ST_IDLE;
    B.camera_x = 400 << 8;
    step(&B, 0, 0, 40);
    assert(ev[NGB_EV_HIT] >= 1);
    printf("hazard: OK\n");
}

static void test_waves_and_boss(void)
{
    int i, guard;
    assert(ng_beatemup_init(&B, &STAGE, &PLAYER, 3, 9) == NGB_OK);
    memset(ev, 0, sizeof(ev));
    step(&B, 0, 0, 2);
    assert(ev[NGB_EV_WAVE] == 1 && B.locked);
    step(&B, 0, 0, 80);
    assert(ng_beatemup_enemies_alive(&B) == 3);
    {   /* they came from both sides, off-screen */
        uint8_t k, left = 0, right = 0;
        for (k = 1; k < NGB_FIGHTERS; k++) if (B.f[k].alive) { if (B.f[k].wave == 1) { if (B.f[k].x < (160 << 8)) left++; else right++; } }
        assert(left >= 1 && right >= 1);
    }
    /* the camera holds until they're beaten */
    for (i = 0; i < 60; i++) { B.f[0].invuln = 5; step(&B, NGB_IN_RIGHT, 0, 1); }
    assert(B.camera_x == 0);
    for (i = 1; i < (int)NGB_FIGHTERS; i++) if (B.f[i].alive) { B.f[i].hp = 0; B.f[i].state = NGB_ST_DOWN; B.f[i].timer = 0; }
    step(&B, 0, 0, 50);
    assert(ev[NGB_EV_KO] == 3 && ev[NGB_EV_WAVE_CLEAR] == 1 && ev[NGB_EV_GO] == 1 && !B.locked);
    assert(B.score >= 300);
    /* on to the boss */
    guard = 0;
    while (ev[NGB_EV_WAVE] < 3 && guard++ < 2000) {
        uint8_t k;
        B.f[0].invuln = 5;
        for (k = 1; k < NGB_FIGHTERS; k++) if (B.f[k].alive && B.f[k].def != &BOSS) { B.f[k].hp = 0; B.f[k].state = NGB_ST_DOWN; }
        step(&B, NGB_IN_RIGHT, 0, 1);
    }
    assert(ev[NGB_EV_WAVE] == 3);
    step(&B, 0, 0, 10);
    {
        uint8_t k, boss = 0;
        for (k = 1; k < NGB_FIGHTERS; k++) if (B.f[k].alive && B.f[k].def == &BOSS) boss = k;
        assert(boss);
        /* phases at 66% and 33%: each with an invulnerable moment */
        for (i = 0; i < 400 && B.f[boss].alive && B.f[boss].hp; i++) {
            NGBMove hit;
            memcpy(&hit, &P_MOVES[0], sizeof(hit));
            B.f[boss].invuln = 0;
            B.f[boss].hp = (uint8_t)(B.f[boss].hp > 6 ? B.f[boss].hp - 6 : 0);
            if (B.f[boss].hp && B.f[boss].phase < 2) {
                /* run the phase check through a real hit */
                B.f[0].x = B.f[boss].x - (28 << 8); B.f[0].y = B.f[boss].y; B.f[0].facing = 0; B.f[0].state = NGB_ST_IDLE;
                tap(&B, 0, NGB_BTN_A);
                step(&B, 0, 0, 14);
            }
            B.f[0].invuln = 30;
        }
        assert(ev[NGB_EV_BOSS_PHASE] == 2 && B.f[boss].phase == 2);
        B.f[boss].hp = 0; B.f[boss].state = NGB_ST_DOWN; B.f[boss].timer = 0;
        step(&B, 0, 0, 50);
        assert(ev[NGB_EV_STAGE_CLEAR] == 1 && ng_beatemup_status(&B) == NGB_CLEAR);
    }
    printf("waves from both sides, scene lock, GO, boss phases, stage clear: OK\n");
}

static void test_order_and_determinism(void)
{
    uint8_t ids[32], n, i;
    fresh();
    enemy_at(&B, &PUNK, 120, 160);
    enemy_at(&B, &PUNK, 140, 150);
    enemy_at(&B, &PUNK, 160, 160);
    B.f[0].y = 160 << 8;
    n = ng_beatemup_draw_list(&B, ids, 32);
    assert(n == 4);
    for (i = 1; i < n; i++) {
        int32_t a = ids[i - 1] < 0x40 ? B.f[ids[i - 1]].y : 0, c = ids[i] < 0x40 ? B.f[ids[i]].y : 0;
        assert(a <= c);
        if (a == c) assert(ids[i - 1] < ids[i]);     /* equal depth: stable */
    }
    {
        uint32_t h1, h2;
        int r;
        for (r = 0; r < 2; r++) {
            NGBeatEmUp *b = r ? &C : &B;
            assert(ng_beatemup_init(b, &STAGE, &PLAYER, 3, 1234) == NGB_OK);
            for (i = 0; i < 200; i++) {
                uint8_t d = (uint8_t)((i >> 4) & 1 ? NGB_IN_RIGHT : NGB_IN_UP | NGB_IN_LEFT);
                uint8_t bt = (uint8_t)((i % 7) == 0 ? NGB_BTN_A : (i % 23) == 0 ? NGB_BTN_B : 0);
                ng_beatemup_input(b, d, bt);
                ng_beatemup_update(b);
                { NGBEvent e; while (ng_beatemup_event(b, &e)) {} }
            }
        }
        h1 = ng_beatemup_hash(&B); h2 = ng_beatemup_hash(&C);
        assert(h1 == h2);
    }
    fresh();
    for (i = 0; i < 100; i++) { ng_beatemup_input(&B, 0, (uint8_t)((i & 1) ? NGB_BTN_B : 0)); ng_beatemup_update(&B); }
    assert(B.ev_count <= NGB_EVENT_QUEUE);
    printf("draw order far to near (stable), determinism, bounded events: OK\n");
}

static void test_ranged(void)
{
    uint8_t e;
    int i;
    fresh();
    e = enemy_at(&B, &DRONE, 64 + 110, 170);
    B.f[0].y = 170 << 8;
    B.f[e].token = 1; B.f[e].ai = NGB_AI_APPROACH;
    for (i = 0; i < 200 && !ev[NGB_EV_SHOT]; i++) step(&B, 0, 0, 1);
    assert(ev[NGB_EV_SHOT] == 1);
    for (i = 0; i < 120 && !ev[NGB_EV_HIT]; i++) step(&B, 0, 0, 1);
    assert(ev[NGB_EV_HIT] == 1 && B.f[0].hp < 40);
    printf("ranged enemy: telegraphed shot hits along the belt: OK\n");
}

static void test_charger(void)
{
    uint8_t e;
    int i;
    int32_t start;
    fresh();
    e = enemy_at(&B, &BIKER, 64 + 90, 170);
    B.f[0].y = 170 << 8;
    B.f[e].token = 1; B.f[e].ai = NGB_AI_APPROACH;
    start = B.f[e].x;
    for (i = 0; i < 300 && !ev[NGB_EV_HIT]; i++) step(&B, 0, 0, 1);
    assert(ev[NGB_EV_TELEGRAPH] >= 1 && ev[NGB_EV_HIT] == 1 && ev[NGB_EV_KNOCKDOWN] == 1);
    assert(B.f[e].x < start);                     /* it ran at the player */
    printf("charger: telegraphed run across the belt knocks down: OK\n");
}

/* An enemy knocked to the stage's left end, the player at the view's left
 * edge: its usual run-up spot is off the stage, so it lines up on the other
 * side instead of walking into the end of the stage for good. */
static void test_approach_at_the_edge(void)
{
    uint8_t e;
    int i;
    fresh();
    B.camera_x = 40 << 8;
    B.f[0].x = 48 << 8; B.f[0].y = 170 << 8;
    e = enemy_at(&B, &BIKER, 0, 170);
    B.f[e].token = 1; B.f[e].ai = NGB_AI_APPROACH;
    for (i = 0; i < 400 && !ev[NGB_EV_TELEGRAPH]; i++) step(&B, 0, 0, 1);
    assert(ev[NGB_EV_TELEGRAPH] >= 1);
    assert(B.f[e].x > B.f[0].x);                  /* it came round to the open side */
    printf("approach spot off the stage: the enemy lines up on the other side: OK\n");
}

/* What has come into view stays in it, whole: a knocked-back enemy stops at
 * the screen's edge, and a tall fighter's head stays below the top. */
static void test_on_screen(void)
{
    static const NGBFighterDef TALL = { 90, 14, 10, 0, 30, 170, 8, 8, E_MOVES, 5, NGB_NONE, NGB_NONE, NGB_NONE,
                                        NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_NONE, NGB_AI_BOSS, 64, 8,
                                        { 0, 1, 2 }, 3, NGB_FD_HEAVY, 1000, 0, 0, 0 };
    uint8_t e, t;
    int i;
    fresh();
    B.f[0].x = 200 << 8; B.f[0].y = 170 << 8;
    e = enemy_at(&B, &PUNK, 290, 170);
    B.f[e].state = NGB_ST_FALL; B.f[e].vx = 8 << 8; B.f[e].z = 1 << 8; B.f[e].vz = 3 << 8; B.f[e].hp = 50;
    for (i = 0; i < 60; i++) {
        step(&B, 0, 0, 1);
        assert(B.f[e].x <= B.camera_x + ((320 - 24) << 8));
    }
    t = enemy_at(&B, &TALL, 100, 140);
    step(&B, 0, 0, 2);
    assert(B.f[t].y >= (170 << 8));
    printf("knocked to the edge, it stays on screen; a tall one's head stays on it: OK\n");

    /* the hero near the right edge: an enemy waiting on that side moves round to the open side */
    fresh();
    B.locked = 1; B.camera_x = B.lock_left = B.lock_right = 0;   /* a wave holds the camera */
    B.f[0].x = 290 << 8; B.f[0].y = 170 << 8;
    e = enemy_at(&B, &PUNK, 300, 170);
    B.f[e].slot = 0x80u;                     /* its waiting place is on the hero's right */
    for (i = 0; i < 240; i++) { B.f[0].invuln = 5; B.f[e].token = 0; B.f[e].ai = NGB_AI_WAIT; step(&B, 0, 0, 1); }
    assert(B.f[e].x < B.f[0].x);
    printf("a waiting spot off the screen: the enemy waits on the other side: OK\n");
}

/* NGB_SF_AUTO_PICKUP: walking over food, points and (empty-handed) a weapon */
static void test_walk_over_pickups(void)
{
    static NGBStage walk;
    int i;
    memcpy(&walk, &EMPTY, sizeof(walk));
    walk.flags = NGB_SF_AUTO_PICKUP;
    assert(ng_beatemup_init(&B, &walk, &PLAYER, 3, 7) == NGB_OK);
    memset(ev, 0, sizeof(ev));
    assert(ng_beatemup_add_prop(&B, NGB_PROP_FOOD, 120, 176, 12, 0) == 0);
    assert(ng_beatemup_add_prop(&B, NGB_PROP_SCORE, 170, 170, 5, 0) == 1);
    assert(ng_beatemup_add_prop(&B, NGB_PROP_WEAPON, 220, 178, 0, 0) == 2);
    assert(ng_beatemup_add_prop(&B, NGB_PROP_WEAPON, 260, 172, 1, 0) == 3);
    B.f[0].x = 64 << 8; B.f[0].y = 172 << 8; B.f[0].hp = 20;
    /* the food lies 4 pixels deeper than the path: still taken in passing */
    for (i = 0; i < 120 && B.prop[0].kind != NGB_PROP_NONE; i++) step(&B, NGB_IN_RIGHT, 0, 1);
    assert(ev[NGB_EV_FOOD] == 1 && B.f[0].hp == 32);
    for (i = 0; i < 120 && B.prop[1].kind != NGB_PROP_NONE; i++) step(&B, NGB_IN_RIGHT, 0, 1);
    assert(ev[NGB_EV_SCORE_ITEM] == 1);
    /* the first weapon is taken; the second is left lying: the hands are full */
    for (i = 0; i < 120 && B.f[0].x < (290 << 8); i++) step(&B, NGB_IN_RIGHT, 0, 1);
    assert(ev[NGB_EV_PICKUP] == 1 && B.f[0].weapon == 2 && B.prop[3].state == 0);
    /* nothing is taken in the air or from the edge of the walk */
    B.f[0].weapon = NGB_NONE; B.prop[2].kind = NGB_PROP_NONE;
    B.f[0].x = 200 << 8; B.f[0].y = 150 << 8;
    step(&B, NGB_IN_RIGHT, 0, 1);
    for (i = 0; i < 40; i++) step(&B, NGB_IN_RIGHT, 0, 1);
    assert(ev[NGB_EV_PICKUP] == 1 && B.prop[3].state == 0);
    B.f[0].x = 240 << 8; B.f[0].y = 172 << 8;
    tap(&B, 0, NGB_BTN_B);
    for (i = 0; i < 8; i++) step(&B, NGB_IN_RIGHT, 0, 1);
    assert(B.f[0].z > 0 && ev[NGB_EV_PICKUP] == 1);
    for (i = 0; i < 60 && B.f[0].weapon == NGB_NONE; i++) step(&B, 0, 0, 1);    /* lands on it */
    assert(ev[NGB_EV_PICKUP] == 2 && B.f[0].weapon == 3);
    /* without the flag, walking over leaves everything where it lies */
    fresh();
    assert(ng_beatemup_add_prop(&B, NGB_PROP_FOOD, 100, 172, 12, 0) == 0);
    B.f[0].y = 172 << 8; B.f[0].hp = 20;
    step(&B, NGB_IN_RIGHT, 0, 60);
    assert(ev[NGB_EV_FOOD] == 0 && B.prop[0].kind == NGB_PROP_FOOD);
    printf("walking over pick-ups: OK\n");
}

int main(void)
{
    setup();
    test_chain_and_volumes();
    test_grabs();
    test_shield_and_special();
    test_crowd();
    test_lives();
    test_vehicle_and_props();
    test_walk_over_pickups();
    test_waves_and_boss();
    test_order_and_determinism();
    test_ranged();
    test_charger();
    test_approach_at_the_edge();
    test_on_screen();
    printf("ng_beatemup: all tests passed (context %u bytes)\n", (unsigned)sizeof(NGBeatEmUp));
    return 0;
}
