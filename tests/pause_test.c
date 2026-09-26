/*
 * ng_pause host tests: the pause state and the freeze it shares with
 * hitstop and slow motion, through their public calls -- the pause turning
 * on and off (and not again when already so), the music muted and brought
 * back only once a game has given its levels, hitstop and slow motion
 * holding while paused and counting down after, slow motion holding the
 * logic on every other frame, and ng_feedback_init clearing it all.
 */
#include <stdio.h>
#include <string.h>
#include "../sdk/2d_engine/ng_pause.h"
#include "../sdk/2d_engine/ng_feedback.h"

/* Stand-ins for the sound wrapper: a log of what was sent. */
static char sound_log[256];
static void logged(const char *what, uint8_t v)
{
    char item[32];
    snprintf(item, sizeof item, "%s=%02X ", what, v);
    strncat(sound_log, item, sizeof sound_log - strlen(sound_log) - 1);
}
void soundSetADPCMBVolume(uint8_t v) { logged("B", v); }
void soundSetSSGVolume(uint8_t v)    { logged("S", v); }
void soundSetFMVolume(uint8_t v)     { logged("F", v); }

/* ng_feedback's other calls, not under test here. */
void ng_palfx_flash_white(uint8_t s, const uint16_t *p, uint8_t f) { (void)s; (void)p; (void)f; }
void ng_palfx_flash_red(uint8_t s, const uint16_t *p, uint8_t f)   { (void)s; (void)p; (void)f; }
void ng_camera_shake(NGCamera *c, uint8_t a, uint8_t f)            { (void)c; (void)a; (void)f; }
NGParticle *ng_spawn_hit_spark(int16_t x, int16_t y, uint16_t t, uint8_t p) { (void)x; (void)y; (void)t; (void)p; return 0; }

static int failures;
static void expect(int ok, const char *what)
{
    if (!ok) { printf("FAIL %s\n", what); failures++; }
}
static void expect_sound(const char *want, const char *what)
{
    if (strcmp(sound_log, want) != 0) {
        printf("FAIL %s: sent \"%s\", expected \"%s\"\n", what, sound_log, want);
        failures++;
    }
    sound_log[0] = '\0';
}

int main(void)
{
    int i;
    static const int slow_held[5] = { 0, 1, 0, 1, 0 };

    ng_feedback_init();
    expect(!ng_pause_is_on() && !ng_feedback_is_hitstop() && !NG_FREEZE_LOGIC(), "after init: nothing held");

    /* No music levels yet: the pause leaves the sound alone. */
    ng_pause_set(1);
    expect(ng_pause_is_on() && NG_FREEZE_LOGIC(), "pause on");
    expect_sound("", "pause with no music levels");
    ng_pause_set(0);
    expect(!ng_pause_is_on(), "pause off");
    expect_sound("", "resume with no music levels");

    /* With levels: muted on pause, put back on resume, once each. */
    ng_pause_set_music_levels(0xB8, 0x00, 0x0C);
    ng_pause_set(1);
    expect_sound("B=00 S=00 F=00 ", "pause mutes the music");
    ng_pause_set(5);
    expect(ng_pause_is_on(), "any non-zero is on");
    expect_sound("", "pausing again sends nothing");

    /* A hitstop started while paused holds until the pause ends. */
    ng_feedback_hitstop(5);
    for (i = 0; i < 3; i++) ng_feedback_update();
    expect(ng_feedback_hitstop_remaining() == 5, "hitstop holds while paused");

    ng_pause_set(0);
    expect_sound("B=B8 S=00 F=0C ", "resume puts the levels back");
    ng_pause_set(0);
    expect_sound("", "resuming again sends nothing");
    expect(NG_FREEZE_LOGIC() && ng_feedback_is_hitstop(), "hitstop still holds the logic");
    for (i = 0; i < 5; i++) ng_feedback_update();
    expect(ng_feedback_hitstop_remaining() == 0 && !ng_feedback_is_hitstop() && !NG_FREEZE_LOGIC(),
           "hitstop counts down once unpaused");

    /* The longer hitstop wins. */
    ng_feedback_hitstop(3);
    ng_feedback_hitstop(2);
    expect(ng_feedback_hitstop_remaining() == 3, "longer hitstop wins");
    for (i = 0; i < 3; i++) ng_feedback_update();

    /* Slow motion holds the logic on every other frame. */
    ng_feedback_slow_motion(4);
    expect(ng_feedback_is_slow_motion() && ng_feedback_slow_motion_remaining() == 4, "slow motion on");
    for (i = 0; i < 5; i++) {
        expect((NG_FREEZE_LOGIC() ? 1 : 0) == slow_held[i], "slow motion holds every other frame");
        ng_feedback_update();
    }
    expect(!ng_feedback_is_slow_motion(), "slow motion runs out");

    /* init clears the pause and forgets the levels (no sound sent). */
    ng_pause_set(1);
    sound_log[0] = '\0';
    ng_feedback_init();
    expect(!ng_pause_is_on() && !NG_FREEZE_LOGIC(), "init clears the pause");
    ng_pause_set(1);
    expect_sound("", "levels forgotten after init");

    if (failures) {
        printf("ng_pause: %d failure(s)\n", failures);
        return 1;
    }
    printf("ng_pause: all tests passed\n");
    return 0;
}
