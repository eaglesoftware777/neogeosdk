/* The laws, the one-on-one and the shape of a match. */
static void open_play(CxWorld *w)
{
    int i;
    w->phase = CX_P_PLAY;
    w->phase_t = 0;
    for (i = 0; i < CX_MEN; i++) { w->m[i].vx = w->m[i].vy = 0; w->m[i].timer = 0; w->m[i].state = CX_M_RUN; }
}

/* Everybody out of the way so a test sees only the men it places. */
static void clear_pitch(CxWorld *w)
{
    int i;
    for (i = 0; i < CX_MEN; i++) {
        w->m[i].x = (int16_t)((80 + (i % 11) * 20) * CX_Q);
        w->m[i].y = (int16_t)((i < 11 ? 30 : 670) * CX_Q);
    }
    w->m[0].x = 6 * CX_Q; w->m[0].y = CX_MID_Y * CX_Q;
    w->m[11].x = (CX_LEN - 6) * CX_Q; w->m[11].y = CX_MID_Y * CX_Q;
}

/* A whistle stops play first: wait for the restart it gave. */
static void settle(CxWorld *w)
{
    int i;
    for (i = 0; i < 200 && w->phase == CX_P_STOP; i++) cx_tick(w, 0, 0, 0, 0);
}

static void test_periods(void)
{
    CxWorld w;
    int i, added = 0, shown = 0;
    cx_init(&w, 7, 8, 0, 2);
    CHECK(cx_attacks_right(&w, 0), "team 0 attacks right first");
    open_play(&w);
    w.clock = w.half_frames - 2;
    for (i = 0; i < 3000 && w.half == 0; i++) { cx_tick(&w, 0, 0, 0, 0); if (w.ev & CX_EV_ADDED) { added = 1; shown = w.added; } }
    CHECK(added && shown >= 1 && shown <= 4, "the board shows added time");
    CHECK(w.half == 1 && !cx_attacks_right(&w, 0), "ends change at half time");

    cx_init(&w, 7, 8, 0, 2);
    w.et = 1; w.half = 1; open_play(&w);
    w.clock = w.half_frames - 1; w.added = 1; w.limit = w.half_frames;
    w.score[0] = w.score[1] = 2;
    for (i = 0; i < 700 && w.half < 2; i++) { w.score[0] = w.score[1] = 2; cx_tick(&w, 0, 0, 0, 0); }
    CHECK(w.half == 2, "a level cup tie goes to extra time");
    CHECK(w.limit == w.half_frames / 3, "extra time is a third of a half");
    open_play(&w); w.clock = w.limit - 1; w.added = 1;
    for (i = 0; i < 700 && w.half < 3; i++) { w.score[0] = w.score[1] = 2; cx_tick(&w, 0, 0, 0, 0); }
    CHECK(w.half == 3, "two halves of it");
    open_play(&w); w.clock = w.limit - 1; w.added = 1;
    for (i = 0; i < 400 && w.phase != CX_P_PENS; i++) { w.score[0] = w.score[1] = 2; cx_tick(&w, 0, 0, 0, 0); }
    CHECK(w.phase == CX_P_PENS, "still level: penalties");

    cx_init(&w, 7, 8, 0, 2);
    w.et = 1; w.half = 1; open_play(&w);
    w.clock = w.half_frames - 1; w.added = 1; w.limit = w.half_frames; w.score[0] = 3; w.score[1] = 1;
    for (i = 0; i < 500 && w.phase != CX_P_OVER; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.phase == CX_P_OVER && w.half == 1, "a decided tie ends at full time");
}

static void test_offside(void)
{
    CxWorld w;
    uint8_t passer = 6, runner = 9;
    cx_init(&w, 0, 1, 0, 0); open_play(&w); clear_pitch(&w);
    /* defenders at 1000 and 1040, keeper on his line: the second-last is at 1040 */
    w.m[13].x = 1000 * CX_Q; w.m[13].y = 300 * CX_Q;
    w.m[14].x = 1040 * CX_Q; w.m[14].y = 400 * CX_Q;
    w.m[passer].x = 900 * CX_Q; w.m[passer].y = 352 * CX_Q;
    w.m[runner].x = 1100 * CX_Q; w.m[runner].y = 352 * CX_Q;
    w.ball.owner = passer; w.ball.x = w.m[passer].x; w.ball.y = w.m[passer].y; w.ball.last = passer;
    cx_offside_mark(&w, passer);
    CHECK(w.off_mask[0] & (1u << runner), "a man 60 past the line is marked");
    w.ball.owner = 0xFF;
    w.ball.x = w.m[runner].x; w.ball.y = w.m[runner].y; w.ball.vx = w.ball.vy = 0; w.ball.h = 0;
    w.m[runner].timer = 0;
    cx_man_step(&w, runner);
    settle(&w);
    CHECK(w.phase == CX_P_FREEKICK && w.restart_team == 1, "playing it from there is a free kick the other way");

    /* level is onside */
    cx_init(&w, 0, 1, 0, 0); open_play(&w); clear_pitch(&w);
    w.m[13].x = 1000 * CX_Q; w.m[14].x = 1040 * CX_Q;
    w.m[runner].x = 1040 * CX_Q;
    w.ball.x = 900 * CX_Q;
    cx_offside_mark(&w, passer);
    CHECK(!(w.off_mask[0] & (1u << runner)) && w.off_man == 0xFF, "level with the second-last man is on");

    /* own half is never off */
    cx_init(&w, 0, 1, 0, 0); open_play(&w); clear_pitch(&w);
    w.m[runner].x = 700 * CX_Q; w.ball.x = 300 * CX_Q;
    cx_offside_mark(&w, passer);
    CHECK(!(w.off_mask[0] & (1u << runner)), "in his own half he cannot be off");
}

static void test_var_offside(void)
{
    CxWorld w;
    uint8_t passer = 6, runner = 9;
    int i;
    /* a marginal offside the flag misses: review takes the goal away */
    cx_init(&w, 0, 1, 0, 0); w.var = CX_VAR_FULL; open_play(&w); clear_pitch(&w);
    w.m[13].x = 1000 * CX_Q; w.m[14].x = 1040 * CX_Q;
    w.m[runner].x = 1046 * CX_Q;
    w.ball.x = 900 * CX_Q;
    cx_offside_mark(&w, passer);
    CHECK(w.off_man == runner && w.off_margin == 6, "the record keeps the marginal man and by how much");
    CHECK(!(w.off_mask[0] & (1u << runner)), "the flag stays down");
    cx_offside_check(&w, runner);
    CHECK(w.off_live, "he played it");
    w.ball.owner = 0xFF; w.ball.last = runner; w.ball.h = 8 * CX_Q; w.ball.vh = 0;
    w.ball.x = (CX_LEN - 2) * CX_Q; w.ball.y = CX_MID_Y * CX_Q; w.ball.vx = 60; w.ball.vy = 0;
    w.m[11].y = 0;
    for (i = 0; i < 3 && w.phase == CX_P_PLAY; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.phase == CX_P_GOAL && w.score[0] == 1, "the goal is given at first");
    CHECK(w.review == 5 - 1, "and a review is due");
    w.var = CX_VAR_FULL;
    for (i = 0; i < 270 + CX_VAR_FRAMES + 10 && w.phase != CX_P_FREEKICK; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.phase == CX_P_FREEKICK && w.score[0] == 0 && w.restart_team == 1, "it is taken away, and the free kick is theirs");

    /* review off: the same goal stands, as in 1994 */
    cx_init(&w, 0, 1, 0, 0); w.var = CX_VAR_OFF; w.rules = CX_RULES_RETRO; open_play(&w); clear_pitch(&w);
    w.m[13].x = 1000 * CX_Q; w.m[14].x = 1040 * CX_Q;
    w.m[runner].x = 1046 * CX_Q; w.ball.x = 900 * CX_Q;
    cx_offside_mark(&w, passer);
    cx_offside_check(&w, runner);
    w.ball.owner = 0xFF; w.ball.last = runner; w.ball.h = 8 * CX_Q; w.ball.vh = 0;
    w.ball.x = (CX_LEN - 2) * CX_Q; w.ball.y = CX_MID_Y * CX_Q; w.ball.vx = 60; w.ball.vy = 0;
    w.m[11].y = 0;
    for (i = 0; i < 3 && w.phase == CX_P_PLAY; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.phase == CX_P_GOAL && !w.review, "without review it stands");
}

static void test_fouls(void)
{
    CxWorld w;
    int i, frees = 0, pens = 0, cards = 0, n;
    /* a foul in midfield: a free kick where it happened (no advantage on, nobody near) */
    cx_init(&w, 0, 1, 0, 0); open_play(&w); clear_pitch(&w);
    w.m[5].x = 700 * CX_Q; w.m[5].y = 352 * CX_Q;
    w.m[16].x = 720 * CX_Q; w.m[16].y = 352 * CX_Q;
    w.ball.x = w.m[5].x; w.ball.y = w.m[5].y; w.ball.owner = 5;
    cx_foul(&w, 16, 5, 0);
    CHECK(w.phase == CX_P_STOP, "a foul stops the match");
    settle(&w);
    CHECK(w.phase == CX_P_FREEKICK && w.restart_team == 0, "the fouled side has the free kick");
    CHECK(w.fouls[1] == 1, "the foul is counted");

    /* inside the box from behind: a penalty, and a card */
    cx_init(&w, 0, 1, 0, 0); open_play(&w); clear_pitch(&w);
    w.m[9].x = 1400 * CX_Q; w.m[9].y = 340 * CX_Q;
    w.ball.x = w.m[9].x; w.ball.y = w.m[9].y;
    cx_foul(&w, 13, 9, 1);
    settle(&w);
    CHECK(w.phase == CX_P_PENALTY && w.restart_team == 0, "a foul in the box from behind is a penalty");
    CHECK(UNITS(w.ball.x) == CX_LEN - CX_SPOT && UNITS(w.ball.y) == CX_MID_Y, "from the spot");
    CHECK(w.m[13].card == 1 || w.m[13].state == CX_M_OFF, "and a card");

    /* a second yellow is a red, and the man leaves */
    cx_init(&w, 0, 1, 0, 0); w.rules = CX_RULES_RETRO; open_play(&w); clear_pitch(&w);
    w.m[13].x = 700 * CX_Q; w.m[5].x = 720 * CX_Q; w.m[5].y = 352 * CX_Q; w.ball.x = w.m[5].x; w.ball.y = w.m[5].y;
    w.m[20].x = 900 * CX_Q; w.m[20].y = 352 * CX_Q;       /* a defender behind the man: not the last man */
    cx_foul(&w, 13, 5, 1); open_play(&w);
    CHECK(w.m[13].card == 1 && w.yellows[1] == 1, "a tackle from behind is a booking");
    cx_foul(&w, 13, 5, 1);
    CHECK(w.m[13].state == CX_M_OFF && w.reds[1] == 1, "two bookings send him off");
    w.phase = CX_P_PLAY;
    CHECK(UNITS(w.m[13].y) < 0, "off the pitch");
    run(&w, 200);
    CHECK(w.m[13].state == CX_M_OFF, "and he stays off");

    /* last man, modern laws: straight red */
    cx_init(&w, 0, 1, 0, 0); open_play(&w); clear_pitch(&w);
    w.m[9].x = 1240 * CX_Q; w.m[9].y = 352 * CX_Q; w.ball.x = w.m[9].x; w.ball.y = w.m[9].y;
    w.m[13].x = 1220 * CX_Q; w.m[13].y = 352 * CX_Q;
    for (i = 12; i < 22; i++) if (i != 13) w.m[i].x = 600 * CX_Q;
    cx_foul(&w, 13, 9, 1);
    CHECK(w.m[13].state == CX_M_OFF, "denying a clear chance from behind is a red under modern laws");

    /* a full CPU match sees the system used */
    for (n = 0; n < 8; n++) {
        cx_init(&w, (uint8_t)n, (uint8_t)(n + 12), 0, 0);
        for (i = 0; i < 2 * w.half_frames + 8000 && w.phase != CX_P_OVER; i++) {
            cx_tick(&w, 0, 0, 0, 0);
            if (w.ev & CX_EV_CARD) cards++;
            if (w.ev & CX_EV_FOUL) frees++;
            if (w.phase == CX_P_PENALTY && w.phase_t == 1) pens++;
        }
        CHECK(w.phase == CX_P_OVER, "matches with fouls in them finish");
    }
    printf("eight CPU matches: %d fouls, %d cards, %d penalties\n", frees, cards, pens);
    CHECK(frees > 0, "CPU sides commit fouls");
}

static void test_advantage(void)
{
    CxWorld w;
    int i;
    cx_init(&w, 0, 1, 0, 0); open_play(&w); clear_pitch(&w);
    w.m[9].x = 1000 * CX_Q; w.m[9].y = 300 * CX_Q;        /* the man fouled */
    w.m[10].x = 1010 * CX_Q; w.m[10].y = 340 * CX_Q;      /* a team-mate on the loose ball */
    w.ball.x = 1010 * CX_Q; w.ball.y = 330 * CX_Q; w.ball.owner = 0xFF;
    w.m[13].x = 1020 * CX_Q; w.m[13].y = 300 * CX_Q;
    w.m[14].x = 1300 * CX_Q; w.m[14].y = 200 * CX_Q;      /* someone behind: not a last-man foul */
    cx_foul(&w, 13, 9, 0);
    CHECK(w.adv_t > 0 || w.phase == CX_P_STOP, "play goes on only with the ball at the feet near goal; otherwise it stops");
    if (w.adv_t) {
        /* the other side wins it: back to the foul */
        w.ball.owner = 14;
        for (i = 0; i < 2 && w.phase == CX_P_PLAY; i++) cx_rules_step(&w);
    }
    settle(&w);
    CHECK(w.phase == CX_P_FREEKICK && w.restart_team == 0, "if nothing comes of it the free kick is given after all");
}

static void test_feint(void)
{
    CxWorld w;
    int n, beat_timed = 0, beat_close = 0, res[6];
    memset(res, 0, sizeof(res));
    /* the same man, at the right moment and too late, over many seeds */
    for (n = 0; n < 400; n++) {
        cx_init(&w, 26, 9, 1, 0); open_play(&w); clear_pitch(&w);
        w.rng = 0x12345u + (uint32_t)n * 2654435761u;
        w.m[5].x = 700 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[5].dir = 0; w.m[5].vx = 22;
        w.m[16].x = 728 * CX_Q; w.m[16].y = 352 * CX_Q; w.m[16].dir = 4;
        w.ball.owner = 5;
        cx_duel_scan(&w);
        CHECK(w.duel_a == 5 && w.duel_d == 16, "a man square in front is a one-on-one");
        cx_feint(&w, 5, 16, CX_WAY_SWERVE);
        res[w.duel_res]++;
        if (w.duel_res == CX_DUEL_BEAT) beat_timed++;
        CHECK(w.duel_why != CX_WHY_NONE, "every result has a reason to show");

        cx_init(&w, 26, 9, 1, 0); open_play(&w); clear_pitch(&w);
        w.rng = 0x12345u + (uint32_t)n * 2654435761u;
        w.m[5].x = 700 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[5].dir = 0; w.m[5].vx = 22;
        w.m[16].x = 708 * CX_Q; w.m[16].y = 352 * CX_Q; w.m[16].dir = 4;
        w.ball.owner = 5;
        cx_feint(&w, 5, 16, CX_WAY_SWERVE);
        if (w.duel_res == CX_DUEL_BEAT) beat_close++;
    }
    CHECK(beat_timed > beat_close, "timing it well beats trying it too close");
    /* a man already sliding is committed */
    cx_init(&w, 26, 9, 1, 0); open_play(&w); clear_pitch(&w);
    w.m[5].x = 700 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[5].dir = 0; w.m[5].vx = 22; w.m[5].ctrl = 12;
    w.m[16].x = 728 * CX_Q; w.m[16].y = 352 * CX_Q; w.m[16].dir = 4; w.m[16].state = CX_M_SLIDE; w.m[16].tack = 10;
    w.ball.owner = 5;
    cx_feint(&w, 5, 16, CX_WAY_UP);
    CHECK(w.duel_why == CX_WHY_COMMITTED, "the reason says he was committed");
    CHECK(w.duel_res == CX_DUEL_BEAT, "and he is beaten");
    CHECK(w.m[16].state == CX_M_BEATEN, "the defender is left");
    printf("feints from 28 units: beat %d step %d fail %d lost %d\n", res[CX_DUEL_BEAT], res[CX_DUEL_STEP], res[CX_DUEL_FAIL], res[CX_DUEL_LOST]);
}

static void test_feint_input(void)
{
    CxWorld w;
    int i;
    cx_init(&w, 26, 9, 1, 0);
    open_play(&w); clear_pitch(&w);
    w.ctrl[0] = 5; w.m[5].x = 700 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[5].dir = 0;
    w.m[16].x = 728 * CX_Q; w.m[16].y = 352 * CX_Q; w.m[16].dir = 4;
    w.ball.owner = 5; w.ball.x = w.m[5].x; w.ball.y = w.m[5].y;
    cx_tick(&w, CX_A | CX_B, CX_A | CX_B, 0, 0);
    CHECK(w.duel_res != CX_DUEL_NONE, "A and B together is the feint");
    /* A alone in a one-on-one still passes, two frames later */
    cx_init(&w, 26, 9, 1, 0);
    open_play(&w); clear_pitch(&w);
    w.ctrl[0] = 5; w.m[5].x = 700 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[5].dir = 0;
    w.m[16].x = 728 * CX_Q; w.m[16].y = 352 * CX_Q; w.m[16].dir = 4;
    w.ball.owner = 5; w.ball.x = w.m[5].x; w.ball.y = w.m[5].y;
    cx_tick(&w, CX_A, CX_A, 0, 0);
    for (i = 0; i < 3; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.ball.owner != 5, "a lone press is the pass, a moment late");
}

static void test_penalty_in_play(void)
{
    CxWorld w;
    int i;
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    cx_restart(&w, CX_P_PENALTY, 0, CX_LEN - CX_SPOT, CX_MID_Y);
    CHECK(UNITS(w.m[11].x) > CX_LEN - 10, "the keeper is on his line");
    for (i = 0; i < 70; i++) cx_tick(&w, CX_UP, 0, 0, 0);
    cx_tick(&w, CX_UP, CX_B, 0, 0);
    for (i = 0; i < 5; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.phase == CX_P_PLAY || w.phase == CX_P_GOAL, "the kick is taken");
    CHECK(w.ball.vy < 0 || w.phase == CX_P_GOAL, "where the stick aimed it");
}

static void test_free_kick(void)
{
    CxWorld w;
    int i;
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    cx_restart(&w, CX_P_FREEKICK, 0, CX_LEN - 260, CX_MID_Y);
    w.phase = CX_P_FREEKICK;
    {
        int wall = 0;
        for (i = 12; i < 22; i++) if (UNITS(w.m[i].x) > CX_LEN - 260 && UNITS(w.m[i].x) < CX_LEN - 150) wall++;
        CHECK(wall >= 3, "a wall forms between the ball and the goal");
    }
    for (i = 0; i < 40; i++) cx_tick(&w, 0, 0, 0, 0);
    cx_tick(&w, 0, CX_B, 0, 0);
    cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.phase == CX_P_PLAY && w.ball.vx > 40, "B shoots at goal from a free kick");
}

static void test_shootout(void)
{
    CxShootout s;
    uint32_t seed;
    unsigned sudden = 0, early = 0;
    for (seed = 1; seed <= 20000; seed++) {
        uint8_t last[2];
        cx_shootout_init(&s, (uint8_t)(seed & 1u), seed);
        cx_shootout_auto(&s, (uint8_t)(seed % 32u), (uint8_t)((seed / 32u) % 32u));
        CHECK(s.over && s.winner < 2, "a shoot-out always ends");
        CHECK(s.score[s.winner] > s.score[s.winner ^ 1u], "the winner scored more: nothing is forced");
        last[0] = s.taken[0]; last[1] = s.taken[1];
        if (last[0] > 5 || last[1] > 5) { sudden++; CHECK(last[0] == last[1], "sudden death ends on level turns"); }
        if (last[0] < 5 || last[1] < 5) early++;
        CHECK(last[0] - last[1] <= 1 && last[1] - last[0] <= 1, "the sides take turns");
    }
    CHECK(sudden > 0 && early > 0, "some go to sudden death and some are over early");
    printf("20000 shoot-outs: %u sudden death, %u over early\n", sudden, early);
}

static void test_determinism(void)
{
    CxWorld a, b;
    int i;
    uint32_t r = 12345;
    cx_init(&a, 3, 27, 1, 0);
    cx_init(&b, 3, 27, 1, 0);
    for (i = 0; i < 9000; i++) {
        uint16_t held, pressed;
        r = r * 1664525u + 1013904223u;
        held = (uint16_t)((r >> 16) & 0x000Fu);
        pressed = (uint16_t)(((r >> 24) & 7u) == 0 ? ((r >> 20) & 0x00F0u) : 0);
        cx_tick(&a, held, pressed, 0, 0);
        cx_tick(&b, held, pressed, 0, 0);
    }
    CHECK(memcmp(&a, &b, sizeof(a)) == 0, "the same inputs give the same match");
}

static void test_squads(void)
{
    CxCup c;
    char name[16];
    int i, j, changes = 0;
    uint32_t seed = 9;
    uint8_t ga, gb;
    unsigned total = 0, wins_strong = 0, wins_weak = 0;
    CHECK(cx_squad_count() == 48, "48 nations");
    cx_squad_name(5, name);
    CHECK(strcmp(name, "BURGERLAND") == 0, "squad names (the parody names)");
    cx_squad_name(32, name);
    CHECK(strcmp(name, "SILKINA") == 0, "the silk road's side");
    for (i = 0; i < 48; i++) {
        CHECK(cx_squad_power((uint8_t)i) >= 8 && cx_squad_power((uint8_t)i) <= 16, "each side has a rating");
        for (j = 0; j < 48; j++) if (i != j && cx_kit_for((uint8_t)i, (uint8_t)j)) changes++;
    }
    /* the recordings: the first 316 names keep their numbers, the 16 new chants follow them,
     * then the new names */
    CHECK(cx_chant_voice(32) == CX_VOICE_NAMES_V1 && cx_chant_voice(47) == CX_VOICE_NAMES_V1 + 15, "new chants after the first names");
    for (i = 0; i < 48; i++) for (j = 0; j < 16; j++) {
        uint16_t v = cx_man_voice((uint8_t)i, (uint8_t)j);
        CHECK(v < CX_VOICE_NAMES_V1 || (v >= CX_VOICE_NAMES_V1 + 16 && v < CX_VOICE_NAMES + 16), "names clear of the chants");
    }
    CHECK(cx_kit_for(0, 25) == 0 && changes > 0, "kit clashes change the second side only");
    cx_cup_init(&c, 9);
    CHECK(c.team[0] == 9, "the player's side first");
    cx_cup_apply(&c, 1); cx_cup_apply(&c, 1); cx_cup_apply(&c, 1);
    CHECK(c.won && c.round == 3, "three wins take the cup");
    for (i = 0; i < 2000; i++) {
        cx_quick_result(8, 31, &seed, &ga, &gb);       /* a strong side against a weak one */
        total += ga + gb;
        if (ga > gb) wins_strong++; else if (gb > ga) wins_weak++;
    }
    CHECK(wins_strong > wins_weak * 3, "strength shows in quick results");
    CHECK(total / 2000u >= 1 && total / 2000u <= 5, "and the scores look like football");
    printf("quick results: strong %u weak %u, %.2f goals a game\n", wins_strong, wins_weak, total / 2000.0);
}

static void test_substitution(void)
{
    CxWorld w;
    uint8_t before;
    int i;
    cx_init(&w, 0, 1, 1, 0);
    before = w.m[3].squad;
    CHECK(cx_sub_request(&w, 0, 3, 0), "a substitution can be asked for at any time");
    CHECK(w.m[3].squad == before, "he waits for the ball to go dead");
    cx_subs_apply(&w);
    CHECK(w.m[3].squad == 11 && w.subs[0] == 1, "then he comes on");
    CHECK(!cx_sub_request(&w, 0, 0, 1), "not the keeper's slot");
    for (i = 0; i < 4; i++) { cx_sub_request(&w, 0, (uint8_t)(4 + i), 1); cx_subs_apply(&w); }
    CHECK(w.subs[0] == 5 && !cx_sub_request(&w, 0, 9, 2), "five under modern laws and no more");
    cx_init(&w, 0, 1, 1, 0); w.rules = CX_RULES_RETRO;
    for (i = 0; i < 3; i++) { cx_sub_request(&w, 0, (uint8_t)(1 + i), 0); cx_subs_apply(&w); }
    CHECK(!cx_sub_request(&w, 0, 7, 1), "three under 1990s laws");
    /* a fresh man is fresh */
    cx_init(&w, 0, 1, 1, 0); w.m[4].sta = 10;
    cx_sub_request(&w, 0, 4, 2); cx_subs_apply(&w);
    CHECK(w.m[4].sta == 255, "the substitute has fresh legs");
}

static void test_clock_and_length(void)
{
    CxWorld w;
    int i, frames;
    /* the option is real minutes for the whole match, stoppages included */
    cx_init(&w, 0, 1, 0, 0);
    cx_set_length(&w, 3, CX_ET_NORMAL);
    CHECK(w.half_frames == 5400, "a three-minute match has 90-second halves");
    for (frames = 0; frames < 30000 && w.phase != CX_P_OVER; frames++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.phase == CX_P_OVER, "it ends");
    CHECK(frames < 3 * 3600 + 2500, "and lasts about what the option says, whatever the stoppages");
    cx_init(&w, 0, 1, 0, 0);
    cx_set_length(&w, 14, CX_ET_NORMAL);
    CHECK(w.half_frames == 25200 && w.fpm == 560, "fourteen minutes is the longest");
    /* golden goal: the first goal of extra time ends it */
    cx_init(&w, 7, 8, 0, 2);
    cx_set_length(&w, 3, CX_ET_GOLDEN);
    w.et = 1; w.half = 2; w.limit = w.half_frames / 3; open_play(&w);
    w.ball.owner = 0xFF; w.ball.last = 9; w.ball.x = (CX_LEN - 2) * CX_Q; w.ball.y = CX_MID_Y * CX_Q; w.ball.vx = 60; w.ball.vy = 0; w.ball.h = 8 * CX_Q; w.ball.vh = 0;
    w.m[11].y = 0;
    for (i = 0; i < 3 && w.phase == CX_P_PLAY; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.phase == CX_P_GOAL, "a goal in extra time");
    for (i = 0; i < 470 && w.phase != CX_P_OVER; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.phase == CX_P_OVER, "golden goal: the match is over at once");
    /* straight to penalties */
    cx_init(&w, 7, 8, 0, 2);
    cx_set_length(&w, 3, CX_ET_NONE);
    w.et = 1; w.half = 1; open_play(&w); w.score[0] = w.score[1] = 1;
    w.clock = w.half_frames - 1; w.added = 1; w.limit = w.half_frames;
    for (i = 0; i < 400 && w.phase != CX_P_PENS; i++) { w.score[0] = w.score[1] = 1; cx_tick(&w, 0, 0, 0, 0); }
    CHECK(w.phase == CX_P_PENS, "no extra time: a level tie goes straight to penalties");
}

static void test_goal_kinds(void)
{
    CxWorld w;
    int i, n, kinds[12];
    /* a ball a man is under, by height and stretch */
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.m[9].x = 1300 * CX_Q; w.m[9].y = 340 * CX_Q; w.m[9].dir = 0; w.ctrl[0] = 9;
    w.ball.owner = 0xFF; w.ball.x = w.m[9].x; w.ball.y = w.m[9].y; w.ball.h = 10 * CX_Q;
    cx_aerial(&w, 9, 1);
    CHECK(w.ball.kind == CX_G_VOLLEY && w.m[9].state == CX_M_VOLLEY, "a low ball is volleyed");
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.m[9].x = 1300 * CX_Q; w.m[9].y = 340 * CX_Q; w.m[9].dir = 0;
    w.ball.owner = 0xFF; w.ball.x = w.m[9].x; w.ball.y = w.m[9].y; w.ball.h = 20 * CX_Q;
    cx_aerial(&w, 9, 1);
    CHECK(w.ball.kind == CX_G_HEADER, "a ball at head height is headed");
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.m[9].x = 1300 * CX_Q; w.m[9].y = 340 * CX_Q; w.m[9].dir = 0;
    w.ball.owner = 0xFF; w.ball.x = (1300 + 26) * CX_Q; w.ball.y = w.m[9].y; w.ball.h = 20 * CX_Q;
    cx_aerial(&w, 9, 1);
    CHECK(w.ball.kind == CX_G_DIVING && w.m[9].state == CX_M_FLY, "a stretch at head height is a diving header");
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.m[9].x = 1300 * CX_Q; w.m[9].y = 340 * CX_Q; w.m[9].dir = 4;
    w.ball.owner = 0xFF; w.ball.x = w.m[9].x; w.ball.y = w.m[9].y; w.ball.h = 30 * CX_Q;
    cx_aerial(&w, 9, 1);
    CHECK(w.ball.kind == CX_G_BIKE && w.m[9].state == CX_M_BIKE, "back to goal under a high ball: an overhead kick");
    /* a chip: C near goal lifts it, far from goal it is a long pass */
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.ctrl[0] = 9; w.m[9].x = 1260 * CX_Q; w.m[9].y = 340 * CX_Q; w.m[9].dir = 0;
    w.ball.owner = 9; w.ball.x = w.m[9].x; w.ball.y = w.m[9].y;
    cx_tick(&w, 0, CX_C, 0, 0); cx_tick(&w, 0, 0, 0, 0); cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.ball.kind == CX_G_CHIP && w.ball.vh > 20, "C near goal chips the keeper");
    /* an own goal is called one */
    cx_init(&w, 0, 1, 0, 0); open_play(&w); clear_pitch(&w);
    w.ball.owner = 0xFF; w.ball.last = 14; w.ball.striker = 14; w.ball.kind = CX_G_SHOT;
    w.ball.x = (CX_LEN - 2) * CX_Q; w.ball.y = CX_MID_Y * CX_Q; w.ball.vx = 60; w.ball.vy = 0; w.ball.h = 8 * CX_Q; w.ball.vh = 0; w.m[11].y = 0;
    for (i = 0; i < 3 && w.phase == CX_P_PLAY; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.phase == CX_P_GOAL && w.goal_kind == CX_G_OWN && w.score[0] == 1, "off the defender: an own goal, for the attackers");
    /* a shot deflected in is the striker's goal, not an own goal */
    cx_init(&w, 0, 1, 0, 0); open_play(&w); clear_pitch(&w);
    w.ball.owner = 0xFF; w.ball.last = 14; w.ball.striker = 9; w.ball.kind = CX_G_SHOT;
    w.ball.x = (CX_LEN - 2) * CX_Q; w.ball.y = CX_MID_Y * CX_Q; w.ball.vx = 60; w.ball.vy = 0; w.ball.h = 8 * CX_Q; w.ball.vh = 0; w.m[11].y = 0;
    for (i = 0; i < 3 && w.phase == CX_P_PLAY; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.phase == CX_P_GOAL && w.scorer == 9 && w.goal_kind != CX_G_OWN, "a deflection stays the striker's goal");
    /* a season's worth shows the variety */
    memset(kinds, 0, sizeof(kinds));
    for (n = 0; n < 32; n++) {
        cx_init(&w, (uint8_t)n, (uint8_t)((n * 7 + 5) % 32), 0, 0);
        cx_set_length(&w, 7, CX_ET_NORMAL);
        for (i = 0; i < 40000 && w.phase != CX_P_OVER; i++) {
            cx_tick(&w, 0, 0, 0, 0);
            if (w.ev & CX_EV_GOAL) kinds[w.goal_kind]++;
        }
    }
    printf("goals by kind:");
    for (i = 0; i < 12; i++) printf(" %d", kinds[i]);
    printf("\n");
    CHECK(kinds[CX_G_SHOT] + kinds[CX_G_TAPIN] + kinds[CX_G_LONG] > 0, "goals from shots");
    CHECK(kinds[CX_G_HEADER] + kinds[CX_G_DIVING] + kinds[CX_G_VOLLEY] + kinds[CX_G_BIKE] > 0, "and from the air");
}

static void test_dribble(void)
{
    CxWorld w;
    int i, far = 0, near = 9999;
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.ctrl[0] = 5; w.m[5].x = 700 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[5].dir = 0;
    w.ball.owner = 5; w.ball.x = w.m[5].x; w.ball.y = w.m[5].y;
    for (i = 0; i < 60; i++) {
        int d;
        cx_tick(&w, CX_RIGHT, 0, 0, 0);
        d = (w.ball.x - w.m[5].x) / CX_Q;
        if (i > 20) { if (d > far) far = d; if (d < near) near = d; }
    }
    CHECK(far > near + 3, "the ball is knocked ahead and caught up with");
    /* it swings round late on a turn */
    cx_tick(&w, CX_LEFT, 0, 0, 0);
    CHECK(w.ball.x > w.m[5].x, "the ball is still ahead the moment he turns");
    /* speed builds */
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.ctrl[0] = 5; w.m[5].x = 700 * CX_Q; w.m[5].y = 352 * CX_Q; w.ball.owner = 5; w.m[5].vx = 0;
    cx_tick(&w, CX_RIGHT, 0, 0, 0);
    CHECK(w.m[5].vx > 0 && w.m[5].vx < 12, "a man does not reach full speed at once");
}

static void test_passing(void)
{
    CxWorld w;
    int i, n, good = 0, tries = 0, over = 0;
    /* a short pass to a free man arrives at his feet and does not run through them */
    for (n = 0; n < 200; n++) {
        cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
        w.rng = 0x51u + (uint32_t)n * 2654435761u;
        w.m[12].x = (CX_LEN - 30) * CX_Q;              /* a deep defender: nobody here is offside */
        w.ctrl[0] = 5; w.m[5].x = 600 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[5].dir = 0; w.m[5].ctrl = 12; w.m[5].timer = 0;
        w.m[6].x = (int16_t)((600 + 80 + (n % 5) * 40) * CX_Q); w.m[6].y = 352 * CX_Q;
        w.ball.owner = 5; w.ball.x = w.m[5].x; w.ball.y = w.m[5].y;
        cx_pass_to(&w, 5, 6, 0);
        tries++;
        for (i = 0; i < 160 && w.ball.owner == 0xFF; i++) cx_tick(&w, 0, 0, 0, 0);
        if (w.ball.owner == 6) good++;
        if (w.ball.owner == 0xFF && UNITS(w.ball.x) > UNITS(w.m[6].x) + 60) over++;
    }
    printf("short passes: %d of %d reached the man, %d ran past\n", good, tries, over);
    CHECK(good * 100 >= tries * 90, "a short pass to a free man nearly always arrives");
    CHECK(over == 0, "and it is never overhit past him");
    /* each button is a different pass */
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.ctrl[0] = 5; w.m[5].x = 600 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[5].dir = 0;
    w.m[6].x = 760 * CX_Q; w.m[6].y = 352 * CX_Q; w.ball.owner = 5; w.ball.x = w.m[5].x; w.ball.y = w.m[5].y;
    cx_tick(&w, CX_RIGHT, CX_A, 0, 0); cx_tick(&w, CX_RIGHT, 0, 0, 0); cx_tick(&w, CX_RIGHT, 0, 0, 0);
    CHECK(w.ball.owner == 0xFF && w.ball.vh <= 0 && U_OF(w.ball.h) < 4, "A is along the ground");
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.ctrl[0] = 5; w.m[5].x = 300 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[5].dir = 0;
    w.m[9].x = 700 * CX_Q; w.m[9].y = 352 * CX_Q; w.ball.owner = 5; w.ball.x = w.m[5].x; w.ball.y = w.m[5].y;
    cx_tick(&w, CX_RIGHT, CX_C, 0, 0); cx_tick(&w, CX_RIGHT, 0, 0, 0); cx_tick(&w, CX_RIGHT, 0, 0, 0);
    CHECK(w.ball.owner == 0xFF && w.ball.vh > 20, "C is in the air");
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.ctrl[0] = 5; w.m[5].x = 600 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[5].dir = 0;
    w.m[9].x = 800 * CX_Q; w.m[9].y = 352 * CX_Q; w.ball.owner = 5; w.ball.x = w.m[5].x; w.ball.y = w.m[5].y;
    cx_tick(&w, CX_RIGHT, CX_A | CX_C, 0, 0);
    CHECK((w.ev & CX_EV_THROUGH) && w.pass_to == 9, "A and C is a through ball");
    /* a one-two */
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.ctrl[0] = 5; w.m[5].x = 600 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[5].dir = 0;
    w.m[6].x = 700 * CX_Q; w.m[6].y = 352 * CX_Q; w.ball.owner = 5; w.ball.x = w.m[5].x; w.ball.y = w.m[5].y;
    cx_tick(&w, CX_RIGHT, CX_A, 0, 0); cx_tick(&w, CX_RIGHT, 0, 0, 0); cx_tick(&w, CX_RIGHT, 0, 0, 0);
    CHECK(w.ctrl[0] == 6, "the player follows his pass");
    cx_tick(&w, 0, CX_A, 0, 0);
    {
        int back = 0;
        for (i = 0; i < 120; i++) { cx_tick(&w, 0, 0, 0, 0); if (w.ev & CX_EV_ONETWO) back = 1; }
        CHECK(back, "A again while it travels plays it back first time");
    }
}

static void test_throw_in(void)
{
    CxWorld w;
    int i;
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.m[5].x = 600 * CX_Q; w.m[5].y = 650 * CX_Q; w.m[6].x = 660 * CX_Q; w.m[6].y = 620 * CX_Q;
    cx_restart(&w, CX_P_THROW, 0, 600, CX_WID);
    cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.m[w.restart_man].state == CX_M_THROW && U_OF(w.ball.h) > 20, "he holds it over his head");
    for (i = 0; i < 30; i++) cx_tick(&w, 0, 0, 0, 0);
    cx_tick(&w, 0, CX_A, 0, 0);
    CHECK((w.ev & CX_EV_THROW) && !(w.m[w.restart_man].state == CX_M_KICK), "a throw-in is thrown, with the hands");
    CHECK(U_OF(w.ball.h) > 20, "from up there");
}

static void test_keeper_protected(void)
{
    CxWorld w;
    int n, reds = 0;
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.m[11].x = 1480 * CX_Q; w.m[11].y = 352 * CX_Q;
    w.ball.owner = 11; w.ball.x = w.m[11].x; w.ball.y = w.m[11].y; w.m[11].state = CX_M_HOLD; w.m[11].timer = 60;
    w.m[9].x = 1466 * CX_Q; w.m[9].y = 352 * CX_Q; w.m[9].dir = 0;
    cx_tackle(&w, 9, 0);
    CHECK(w.m[9].card >= 1 && (w.ev & CX_EV_CARD), "a tackle on a keeper holding the ball is a booking");
    CHECK(w.phase == CX_P_STOP && w.pend_team == 1, "and the keeper's free kick");
    w.phase = CX_P_PLAY; w.ball.owner = 11; w.m[9].state = CX_M_RUN;
    cx_tackle(&w, 9, 0);
    CHECK(w.m[9].state == CX_M_OFF, "a second is a red");
    for (n = 0; n < 2; n++) reds += w.reds[n];
    CHECK(reds == 1, "counted once");
}

static void test_var_request(void)
{
    CxWorld w;
    int i;
    /* a goal with the marginal man: the side that asks gets it taken away */
    cx_init(&w, 0, 1, 2, 0); w.var = CX_VAR_CHALLENGE; open_play(&w); clear_pitch(&w);
    w.m[13].x = 1000 * CX_Q; w.m[14].x = 1040 * CX_Q; w.m[9].x = 1046 * CX_Q; w.ball.x = 900 * CX_Q;
    cx_offside_mark(&w, 6);
    cx_offside_check(&w, 9);
    w.ball.owner = 0xFF; w.ball.last = 9; w.ball.striker = 9; w.ball.h = 8 * CX_Q; w.ball.vh = 0;
    w.ball.x = (CX_LEN - 2) * CX_Q; w.ball.y = CX_MID_Y * CX_Q; w.ball.vx = 60; w.ball.vy = 0; w.m[11].y = 0;
    for (i = 0; i < 3 && w.phase == CX_P_PLAY; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.phase == CX_P_GOAL && !w.review, "nobody reviews it unasked");
    CHECK(cx_var_open(&w, 1) && !cx_var_open(&w, 0), "the side that conceded may ask");
    CHECK(cx_var_challenge(&w, 1) && w.phase == CX_P_VAR && !w.review_keep, "and it is overturned");
    for (i = 0; i < CX_VAR_FRAMES + 5; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.score[0] == 0 && w.challenges[1] == 2, "no goal, and a right call keeps the chance");
    /* a good goal: the chance is lost */
    cx_init(&w, 0, 1, 2, 0); w.var = CX_VAR_CHALLENGE; open_play(&w); clear_pitch(&w);
    w.ball.owner = 0xFF; w.ball.last = 9; w.ball.striker = 9; w.ball.h = 8 * CX_Q; w.ball.vh = 0;
    w.ball.x = (CX_LEN - 2) * CX_Q; w.ball.y = CX_MID_Y * CX_Q; w.ball.vx = 60; w.ball.vy = 0; w.m[11].y = 0;
    for (i = 0; i < 3 && w.phase == CX_P_PLAY; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(cx_var_challenge(&w, 1) && w.review_keep, "a good goal stands");
    CHECK(w.challenges[1] == 1, "and the request is used up");
    for (i = 0; i < CX_VAR_FRAMES + 5; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.score[0] == 1, "the goal counts");
    /* a wrong foul: the booking goes */
    cx_init(&w, 0, 1, 2, 0); w.var = CX_VAR_CHALLENGE; open_play(&w); clear_pitch(&w);
    w.m[5].x = 700 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[16].x = 720 * CX_Q; w.m[16].y = 352 * CX_Q;
    w.ball.x = w.m[5].x; w.ball.y = w.m[5].y; w.ball.owner = 5;
    cx_stop_play(&w, CX_P_FREEKICK, 0, 700, 352);
    w.wrong = 1; w.wrong_card = 16; w.m[16].card = 1; w.yellows[1] = 1;
    CHECK(cx_var_challenge(&w, 1) && !w.review_keep, "the record says he got the ball");
    for (i = 0; i < CX_VAR_FRAMES + 5; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.m[16].card == 0 && w.restart_team == 1, "the card is taken back and the free kick goes the other way");
}

/* A player's keeper with the ball in his hands: he keeps it until the player plays it,
 * A rolls it out short, B kicks it long up the middle, C throws it long. */
static void keeper_holds(CxWorld *w)
{
    cx_init(w, 0, 1, 1, 0); open_play(w); clear_pitch(w);
    w->m[12].x = (CX_LEN - 30) * CX_Q;               /* a deep defender: nobody is offside */
    w->m[5].x = 200 * CX_Q; w->m[5].y = 300 * CX_Q;   /* close by */
    w->m[7].x = 420 * CX_Q; w->m[7].y = 90 * CX_Q;    /* out wide in midfield */
    w->m[9].x = 760 * CX_Q; w->m[9].y = 340 * CX_Q;   /* the centre forward */
    w->m[0].x = 40 * CX_Q; w->m[0].y = CX_MID_Y * CX_Q;
    cx_ball_take(w, 0);
    w->ball.x = w->m[0].x; w->ball.y = w->m[0].y;
    w->m[0].state = CX_M_HOLD; w->m[0].timer = 210;
}

static void test_keeper_hands(void)
{
    CxWorld w;
    int i, d;
    keeper_holds(&w);
    for (i = 0; i < 100; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.ball.owner == 0 && w.m[0].state == CX_M_HOLD, "a player's keeper keeps it until he is told");
    CHECK(w.ctrl[0] == 0, "and the player has him meanwhile");
    cx_tick(&w, CX_UP, 0, 0, 0);
    cx_tick(&w, CX_UP, 0, 0, 0);
    CHECK(w.m[0].y < CX_MID_Y * CX_Q, "the stick walks him about his box");
    for (i = 0; i < 200; i++) cx_tick(&w, CX_RIGHT, 0, 0, 0);
    CHECK(UNITS(w.m[0].x) < CX_BOX_D, "never out of it");

    keeper_holds(&w);
    cx_tick(&w, 0, CX_B, 0, 0);
    CHECK(w.ball.owner == 0xFF && w.ball.vh > 40 && w.pass_to == 9, "B kicks it long, to the man up the middle");
    for (i = 0; i < 100 && w.ball.owner == 0xFF; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(UNITS(w.ball.x) > 520, "a long way up the pitch");
    CHECK(w.ctrl[0] != 0, "the player is off the keeper once it has gone");

    keeper_holds(&w);
    cx_tick(&w, 0, CX_C, 0, 0);
    d = w.pass_to == 0xFF ? 0 : UNITS(w.m[w.pass_to].x) - 40;
    CHECK(w.ball.owner == 0xFF && w.pass_to != 0xFF && d > 110 && U_OF(w.ball.h) > 20, "C throws it long, from over his head");

    keeper_holds(&w);
    cx_tick(&w, 0, CX_A, 0, 0);
    CHECK(w.ball.owner == 0xFF && w.pass_to == 5 && w.ball.vh <= 0, "A rolls it out to the man close by");
    for (i = 0; i < 120 && w.ball.owner == 0xFF; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.ball.owner == 5, "and he has it");

    keeper_holds(&w);
    for (i = 0; i < 230 && w.ball.owner == 0; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.ball.owner != 0 && i < 230, "kept too long, he kicks it himself");
}

/* A pass on the run: the player still holds the stick the way he passed, and presses A
 * as it travels (the call for the one-two). The man it is for runs across to meet it
 * and takes it; A never turns him into a tackle on nobody. */
static void test_receive_on_the_run(void)
{
    CxWorld w;
    int n, i, got = 0, tries = 0;
    for (n = 0; n < 60; n++) {
        int took = 0;
        cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
        w.rng = 0x77u + (uint32_t)n * 2654435761u;
        w.m[12].x = (CX_LEN - 30) * CX_Q;
        w.ctrl[0] = 5; w.m[5].x = 500 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[5].dir = 0;
        w.m[5].vx = 30; w.m[5].ctrl = 10;
        w.m[6].x = (int16_t)((620 + (n % 4) * 30) * CX_Q); w.m[6].y = (int16_t)((352 + ((n % 3) - 1) * 50) * CX_Q);
        w.m[6].vx = (int16_t)((n % 5) * 8);
        w.ball.owner = 5; w.ball.x = w.m[5].x; w.ball.y = w.m[5].y;
        cx_tick(&w, CX_RIGHT, CX_A, 0, 0); cx_tick(&w, CX_RIGHT, 0, 0, 0); cx_tick(&w, CX_RIGHT, 0, 0, 0);
        if (w.pass_to != 6) continue;
        tries++;
        for (i = 0; i < 150 && !took; i++) {
            cx_tick(&w, CX_RIGHT, i == 4 ? CX_A : 0, 0, 0);
            /* taken, played back (the one-two) or played on first time by him (A with the stick) */
            if (w.ball.owner == 6 || (w.ev & CX_EV_ONETWO) || ((w.ev & CX_EV_PASS) && w.ball.last == 6)) took = 1;
            if (w.ball.owner != 0xFF && cx_team_of(w.ball.owner) != 0) break;
        }
        got += took;
    }
    printf("passes on the run: %d of %d taken by the man they were for\n", got, tries);
    CHECK(tries >= 40 && got * 100 >= tries * 90, "a pass on the run reaches its man");
    /* and the player steering the receiver straight away from it, as players do: on the
     * ground he still goes to meet it */
    got = tries = 0;
    for (n = 0; n < 60; n++) {
        int took = 0;
        uint16_t away = (uint16_t)((n & 1) ? CX_UP : CX_DOWN);
        cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
        w.rng = 0x99u + (uint32_t)n * 2654435761u;
        w.m[12].x = (CX_LEN - 30) * CX_Q;
        w.ctrl[0] = 5; w.m[5].x = 500 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[5].dir = 0; w.m[5].ctrl = 10;
        w.m[6].x = (int16_t)((620 + (n % 4) * 30) * CX_Q); w.m[6].y = (int16_t)((352 + ((n % 3) - 1) * 50) * CX_Q);
        w.ball.owner = 5; w.ball.x = w.m[5].x; w.ball.y = w.m[5].y;
        cx_tick(&w, CX_RIGHT, CX_A, 0, 0); cx_tick(&w, CX_RIGHT, 0, 0, 0); cx_tick(&w, CX_RIGHT, 0, 0, 0);
        if (w.pass_to != 6) continue;
        tries++;
        for (i = 0; i < 150 && !took; i++) {
            cx_tick(&w, (uint16_t)(away | CX_D), 0, 0, 0);
            if (w.ball.owner == 6) took = 1;
            if (w.ball.owner != 0xFF && cx_team_of(w.ball.owner) != 0) break;
        }
        got += took;
    }
    printf("passes with the receiver steered away: %d of %d taken\n", got, tries);
    CHECK(tries >= 40 && got * 100 >= tries * 90, "steering away does not lose a ground pass");
}

/* A pressed, and the ball lost before the pass went (it waits two frames for a B): when
 * the side wins it back, nothing is passed that the player did not ask for. */
static void test_stale_press(void)
{
    CxWorld w;
    int i, passed = 0;
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.ctrl[0] = 5; w.m[5].x = 600 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[5].dir = 0;
    w.m[6].x = 700 * CX_Q; w.m[6].y = 352 * CX_Q;
    w.ball.owner = 5; w.ball.x = w.m[5].x; w.ball.y = w.m[5].y;
    cx_tick(&w, 0, CX_A, 0, 0);
    /* an opponent takes it before the pass goes */
    w.m[16].x = 610 * CX_Q; w.m[16].y = 352 * CX_Q;
    cx_ball_take(&w, 16); w.ball.x = w.m[16].x; w.ball.y = w.m[16].y;
    for (i = 0; i < 4; i++) cx_tick(&w, 0, 0, 0, 0);
    /* and ours wins it back */
    w.m[16].x = 900 * CX_Q;
    w.m[6].state = CX_M_RUN; w.m[6].timer = 0;
    cx_ball_take(&w, 6); w.ball.x = w.m[6].x; w.ball.y = w.m[6].y; w.ctrl[0] = 6;
    for (i = 0; i < 20; i++) { cx_tick(&w, 0, 0, 0, 0); if (w.ev & CX_EV_PASS) passed = 1; }
    CHECK(!passed, "a press made before the ball was lost does not pass it when it is won back");
}

/* B for a high ball in his own half: headed or volleyed up the pitch, never "at goal"
 * from a thousand units out. */
static void test_deep_aerial(void)
{
    CxWorld w;
    int i, shot = 0, met = 0;
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.m[12].x = (CX_LEN - 30) * CX_Q;
    w.ctrl[0] = 3; w.m[3].x = 300 * CX_Q; w.m[3].y = 352 * CX_Q; w.m[3].dir = 0;
    w.m[9].x = 760 * CX_Q; w.m[9].y = 340 * CX_Q;
    w.ball.owner = 0xFF; w.ball.x = 330 * CX_Q; w.ball.y = 352 * CX_Q; w.ball.h = 40 * CX_Q;
    w.ball.vx = -20; w.ball.vy = 0; w.ball.vh = 0;
    cx_tick(&w, 0, 0, 0, 0);                 /* where it drops is worked out first */
    cx_tick(&w, 0, CX_B, 0, 0);
    for (i = 0; i < 60; i++) {
        cx_tick(&w, 0, 0, 0, 0);
        if (w.ev & CX_EV_SHOT) shot = 1;
        if (w.ball.last == 3 && w.ball.vx > 0) met = 1;
    }
    CHECK(!shot, "B for a high ball deep in his own half is no shot");
    CHECK(met, "he heads or volleys it up the pitch");
}

/* A shot at a player's goal: he has his keeper for it. Left alone the CPU saves for him;
 * the stick and A take him over and dive the stick's way. */
static void test_player_keeper(void)
{
    CxWorld w;
    int n, i, ai_goals = 0, dive_goals = 0, took = 0;
    for (n = 0; n < 40; n++) {
        int pass;
        for (pass = 0; pass < 2; pass++) {
            int off = (n & 1) ? 60 : -60, armed = 0;
            cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
            w.rng = 0x4321u + (uint32_t)n * 2654435761u;
            w.m[0].x = 16 * CX_Q; w.m[0].y = CX_MID_Y * CX_Q;
            w.ctrl[0] = 5;
            w.m[19].x = 220 * CX_Q; w.m[19].y = (int16_t)((CX_MID_Y + (n & 2 ? 40 : -40)) * CX_Q);
            w.ball.owner = 19; w.ball.x = w.m[19].x; w.ball.y = w.m[19].y;
            cx_kick_to(&w, 19, -8, (int16_t)(CX_MID_Y + off), 200, 6);
            w.ball.fast = 1; w.ball.kind = CX_G_SHOT; w.ball.striker = 19;
            for (i = 0; i < 120 && w.phase == CX_P_PLAY; i++) {
                uint16_t held = 0, pr = 0;
                if (i == 1 && w.ctrl[0] == 0) took++;
                if (pass == 1) {
                    held = off > 0 ? CX_DOWN : CX_UP;
                    if (!armed && cx_shot_at(&w, 0) && cx_shot_at(&w, 0) < 12) { pr = CX_A; armed = 1; }
                }
                cx_tick(&w, held, pr, 0, 0);
            }
            if (w.phase == CX_P_GOAL) { if (pass) dive_goals++; else ai_goals++; }
        }
    }
    printf("shots at a player's keeper: left to the CPU %d goals of 40, dived by the player %d\n", ai_goals, dive_goals);
    CHECK(took >= 38, "a shot at his goal gives the player his keeper");
    CHECK(dive_goals < ai_goals, "a dive the right way at the right time saves more than the CPU does");
}

/* B is wound up while held and struck when let go: a tap is placed, a long hold is hard. */
static void test_shot_charge(void)
{
    CxWorld w;
    int hold, sp[2], i;
    for (hold = 0; hold < 2; hold++) {
        cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
        w.m[12].x = (CX_LEN - 30) * CX_Q;
        w.ctrl[0] = 9; w.m[9].x = 1200 * CX_Q; w.m[9].y = 352 * CX_Q; w.m[9].dir = 0;
        w.ball.owner = 9; w.ball.x = w.m[9].x; w.ball.y = w.m[9].y;
        cx_tick(&w, 0, CX_B, 0, 0);
        CHECK(w.ball.owner == 9, "pressing B does not strike it yet");
        for (i = 0; i < (hold ? 30 : 2) && w.ball.owner == 9; i++) cx_tick(&w, CX_B, 0, 0, 0);
        for (i = 0; i < 3 && w.ball.owner == 9; i++) cx_tick(&w, 0, 0, 0, 0);
        sp[hold] = cx_abs(w.ball.vx) + cx_abs(w.ball.vy);
        CHECK(w.ball.owner == 0xFF && (w.ev & CX_EV_SHOT || w.m[9].kick == CX_K_SHOT), "letting B go shoots");
    }
    printf("shot speed: a tap %d, wound right up %d\n", sp[0], sp[1]);
    CHECK(sp[1] > sp[0] + 40, "the longer B is held the harder it is struck");
}

/* B as a pass comes to the player's man: he hits it first time, no touch to set it. */
static void test_first_time(void)
{
    CxWorld w;
    int i, shot = 0, held_it = 0;
    cx_init(&w, 0, 1, 1, 0); open_play(&w); clear_pitch(&w);
    w.m[12].x = (CX_LEN - 30) * CX_Q;
    w.ctrl[0] = 5; w.m[5].x = 1000 * CX_Q; w.m[5].y = 352 * CX_Q; w.m[5].dir = 0;
    w.m[9].x = 1160 * CX_Q; w.m[9].y = 330 * CX_Q;
    w.ball.owner = 5; w.ball.x = w.m[5].x; w.ball.y = w.m[5].y;
    cx_pass_to(&w, 5, 9, 0);
    cx_tick(&w, 0, 0, 0, 0);
    cx_tick(&w, 0, CX_B, 0, 0);
    for (i = 0; i < 80; i++) {
        cx_tick(&w, 0, 0, 0, 0);
        if (w.ball.owner == 9) held_it++;
        if ((w.ev & CX_EV_SHOT) && w.ball.last == 9) shot = 1;
    }
    CHECK(shot, "B as it comes: a first-time shot");
    CHECK(held_it <= 1, "without stopping it first");
}

static void rules_tests(void)
{
    test_periods();
    test_offside();
    test_var_offside();
    test_fouls();
    test_advantage();
    test_feint();
    test_feint_input();
    test_penalty_in_play();
    test_free_kick();
    test_shootout();
    test_determinism();
    test_squads();
    test_substitution();
    test_clock_and_length();
    test_goal_kinds();
    test_dribble();
    test_passing();
    test_throw_in();
    test_keeper_protected();
    test_keeper_hands();
    test_receive_on_the_run();
    test_stale_press();
    test_deep_aerial();
    test_player_keeper();
    test_shot_charge();
    test_first_time();
    test_var_request();
}
