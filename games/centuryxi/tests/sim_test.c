/* Host checks for the Century XI match (cc -std=c99 -DCX_CODE= ...). */
#include <stdio.h>
#include <string.h>
#include "cx_internal.h"
#include "../artbox/generated/cx_data.h"     /* the recordings' counts, for the voice test */

static int failures;
#define CHECK(c, msg) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, msg); failures++; } } while (0)
#undef U
#define UNITS(v) ((v) / CX_Q)
#define U_OF(v) ((v) / CX_Q)

static void run(CxWorld *w, int n)
{
    int i;
    for (i = 0; i < n; i++) cx_tick(w, 0, 0, 0, 0);
}

static void test_kickoff(void)
{
    CxWorld w;
    int i, left = 0;
    cx_init(&w, 3, 5, 0, 0);
    CHECK(w.phase == CX_P_KICKOFF, "a match starts with a kick-off");
    CHECK(w.ball.owner != 0xFF && w.ball.owner / 11 == 0, "team 0 kicks off");
    CHECK(UNITS(w.ball.x) == CX_LEN / 2, "from the centre spot");
    for (i = 0; i < 11; i++) if (UNITS(w.m[i].x) <= CX_LEN / 2) left++;
    CHECK(left == 11, "team 0 starts in its own half");
    run(&w, 120);
    CHECK(w.phase == CX_P_PLAY, "the CPU kicks off");
}

static void test_cpu_match(void)
{
    CxWorld w;
    int i, kicks = 0, saves = 0, goals = 0, restarts = 0, passes = 0;
    uint8_t seen_phase[16];
    memset(seen_phase, 0, sizeof(seen_phase));
    cx_init(&w, 10, 2, 0, 1);
    /* the clock stops for goals, restarts and the interval: allow for them */
    for (i = 0; i < 2 * w.half_frames + 5000 && w.phase != CX_P_OVER; i++) {
        cx_tick(&w, 0, 0, 0, 0);
        seen_phase[w.phase] = 1;
        if (w.ev & CX_EV_KICK) kicks++;
        if (w.ev & CX_EV_PASS) passes++;
        if (w.ev & CX_EV_SAVE) saves++;
        if (w.ev & CX_EV_GOAL) goals++;
        if (w.ev & CX_EV_OUT) restarts++;
    }
    CHECK(w.phase == CX_P_OVER, "a CPU match plays to the end");
    CHECK(kicks > 60, "the ball is kicked about");
    CHECK(passes > 30, "sides pass");
    CHECK(goals == w.score[0] + w.score[1], "every goal counts once");
    /* A legal match can have no out-of-play events. Cover all nations rather
     * than tying restart coverage to one old fictional-squad RNG sequence. */
    {
        int side;
        for (side = 0; side < 16; side++) {
            CxWorld other;
            int tick;
            cx_init(&other, (uint8_t)side, (uint8_t)(side + 16), 0, 1);
            for (tick = 0; tick < 2 * other.half_frames + 5000 && other.phase != CX_P_OVER; tick++) {
                cx_tick(&other, 0, 0, 0, 0);
                if (other.ev & CX_EV_OUT) restarts++;
                if (other.ev & CX_EV_GOAL) goals++;
            }
            CHECK(other.phase == CX_P_OVER, "all national-team fixtures finish");
        }
    }
    CHECK(restarts > 0, "national-team fixture suite exercises out-of-play restarts");
    CHECK(goals >= 8, "goals are scored across the fixtures");
    CHECK(seen_phase[CX_P_HALF], "half time");
    CHECK(seen_phase[CX_P_FULL], "full time");
    printf("cpu match: %d-%d, %d kicks, %d passes, %d saves; national fixture suite: %d restarts\n", w.score[0], w.score[1], kicks, passes, saves, restarts);
}

static void test_human_controls(void)
{
    CxWorld w;
    int i;
    uint8_t c, owner;
    int16_t x0;
    cx_init(&w, 0, 1, 1, 0);
    run(&w, 1);
    cx_tick(&w, 0, CX_A, 0, 0);      /* the player kicks off */
    run(&w, 30);
    CHECK(w.phase == CX_P_PLAY, "A kicks off");
    c = w.ctrl[0];
    CHECK(c != 0xFF && c / 11 == 0 && c % 11 != 0, "the player holds an outfielder");
    /* give him the ball in midfield and run right */
    w.ball.owner = c; w.ball.last = c;
    for (i = 0; i < CX_MEN; i++) if (i / 11 == 1 && i % 11 != 0) { w.m[i].x = 100 * CX_Q; w.m[i].y = 20 * CX_Q; }
    w.m[c].x = 700 * CX_Q; w.m[c].y = CX_MID_Y * CX_Q;
    x0 = w.m[c].x;
    for (i = 0; i < 60; i++) cx_tick(&w, CX_RIGHT, 0, 0, 0);
    CHECK(w.m[c].x > x0 + 40 * CX_Q, "the stick dribbles");
    CHECK(w.ball.owner == c, "the ball stays at his feet");
    /* A tap of B is a shot at once (it leaves as B comes up); held, B winds up a harder one. */
    w.m[c].x = 1200 * CX_Q; w.m[c].y = CX_MID_Y * CX_Q; w.m[c].dir = 0;
    run(&w, 12);
    cx_tick(&w, CX_B, CX_B, 0, 0);
    cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.ball.owner == 0xFF && w.ball.vx > 40, "a tap of B shoots at goal at once");
    owner = 0xFF;
    {
        int saved = 0;
        for (i = 0; i < 200 && w.phase == CX_P_PLAY && !saved; i++) {
            cx_tick(&w, 0, 0, 0, 0);
            owner = w.ball.owner;
            if ((w.ev & (CX_EV_SAVE | CX_EV_POST)) || owner == 11) saved = 1;     /* held, parried or tipped round */
        }
        CHECK(w.phase != CX_P_PLAY || owner == 11 || saved, "the shot is scored, saved or out");
    }
    (void)owner;
}

static void test_restarts(void)
{
    CxWorld w;
    int i;
    cx_init(&w, 4, 6, 0, 0);
    run(&w, 100);
    /* over the near touchline */
    w.ball.owner = 0xFF; w.ball.last = 3;
    w.ball.x = 600 * CX_Q; w.ball.y = (CX_WID - 2) * CX_Q; w.ball.vx = 0; w.ball.vy = 40; w.ball.h = 0; w.ball.vh = 0;
    w.phase = CX_P_PLAY;
    run(&w, 2);
    CHECK(w.phase == CX_P_THROW && w.restart_team == 1, "out off team 0: team 1 throws in");
    run(&w, 120);
    CHECK(w.phase == CX_P_PLAY, "the throw is taken");
    /* over the goal line wide of the posts, off an attacker: goal kick */
    w.ball.owner = 0xFF; w.ball.last = 9;            /* team 0 attacks right in the first half */
    w.ball.x = (CX_LEN - 2) * CX_Q; w.ball.y = 100 * CX_Q; w.ball.vx = 60; w.ball.vy = 0; w.ball.h = 0;
    w.phase = CX_P_PLAY;
    run(&w, 2);
    CHECK(w.phase == CX_P_GOALKICK && w.restart_team == 1, "goal kick to the defenders");
    run(&w, 120);
    /* off a defender: corner */
    w.ball.owner = 0xFF; w.ball.last = 13;
    w.ball.x = (CX_LEN - 2) * CX_Q; w.ball.y = 600 * CX_Q; w.ball.vx = 60; w.ball.vy = 0; w.ball.h = 0;
    w.phase = CX_P_PLAY;
    run(&w, 2);
    CHECK(w.phase == CX_P_CORNER && w.restart_team == 0, "corner to the attackers");
    /* in at the posts under the bar */
    run(&w, 120);
    w.ball.owner = 0xFF; w.ball.last = 9;
    w.ball.x = (CX_LEN - 2) * CX_Q; w.ball.y = CX_MID_Y * CX_Q; w.ball.vx = 60; w.ball.vy = 0; w.ball.h = 8 * CX_Q; w.ball.vh = 0;
    w.m[11].y = 0;                                   /* the keeper is elsewhere */
    w.phase = CX_P_PLAY;
    for (i = 0; i < 3 && w.phase == CX_P_PLAY; i++) cx_tick(&w, 0, 0, 0, 0);
    CHECK(w.phase == CX_P_GOAL && w.score[0] == 1, "a goal");
    run(&w, 260);                                    /* the goal's minute: celebration, then the restart */
    CHECK(w.phase == CX_P_KICKOFF || w.phase == CX_P_PLAY, "kick-off after the goal");
}

static void test_friction_and_classic(void)
{
    CxWorld w; int i;
    cx_init(&w, 0, 1, 1, 0); w.phase = CX_P_PLAY;
    for (i = 0; i < CX_MEN; i++) { w.m[i].x = 100 * CX_Q; w.m[i].y = 100 * CX_Q; }
    w.ball.owner = 255; w.ball.x = 700 * CX_Q; w.ball.y = 500 * CX_Q;
    w.ball.h = w.ball.vh = 0; w.ball.vx = 15; w.ball.vy = -15;
    run(&w, 60);
    CHECK(w.ball.owner == 255 && w.ball.vx == 0 && w.ball.vy == 0, "subpixel ball velocity settles symmetrically");
    cx_init(&w, 0, 1, 1, 0); w.phase = CX_P_PLAY; w.classic_controls = 1;
    w.ctrl[0] = 1; w.ball.owner = 12;
    w.m[1].x = w.ball.x = 700 * CX_Q; w.m[1].y = w.ball.y = 352 * CX_Q;
    cx_tick(&w, CX_A, CX_A, 0, 0);
    CHECK(w.m[w.ctrl[0]].state == CX_M_SLIDE, "classic A performs a sliding tackle");
}

#include "sim_rules_test.inc"

int main(void)
{
    test_friction_and_classic();
    test_kickoff();
    test_cpu_match();
    test_human_controls();
    test_restarts();
    rules_tests();
    if (failures) { printf("%d failures\n", failures); return 1; }
    printf("century xi sim: all checks passed\n");
    return 0;
}
