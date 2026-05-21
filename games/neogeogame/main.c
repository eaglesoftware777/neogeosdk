/*
 * Simple horizontal shooter example using the Neo Geo SDK.
 *
 * The player controls a ship drawn on the FIX text layer using the
 * joystick.  The ship (>) moves left and right along the bottom row.
 * Press button A to fire a projectile (-) that travels to the right.
 * An enemy ship (X) moves from right to left along the top row.  When
 * the projectile hits the enemy, the score is incremented and the
 * enemy resets to the starting position.
 *
 * This program demonstrates basic polling of the joystick, simple
 * state management and rendering using the FIX layer.  It intentionally
 * keeps the logic simple so that it can serve as a clear example for
 * new developers.
 */

#include <stdint.h>
#include "sdk/2d_engine/ng_fix.h"
#include "sdk/neogeo.h"
#include "sdk/macro.h"
#include <stdio.h>

/* Player, bullet and enemy positions (X coordinate on the FIX layer). */
static int player_x;
static int bullet_x;
static int bullet_active;
static int enemy_x;
static int enemy_timer;
static int score;

/* Called once at game start to initialise game state and the FIX layer. */
void game_boot(void) {
    ng_fix_init();
    ng_fix_clear();
    /* Place the player near the left side of the screen. */
    player_x = 5;
    bullet_active = 0;
    bullet_x = 0;
    /* Start the enemy on the right-hand side. */
    enemy_x = 28;
    enemy_timer = 0;
    score = 0;

    /* Initialise the sound system so we can play SFX.  Without this
     * call the audio driver is not ready and playSFX() will have no
     * effect. */
    soundInit();
}

/* Called once per frame to update game state and render. */
void game_frame(void) {
    /* Read current joystick state. */
    uint16_t joy = poll_joystick();

    /* Handle player movement.  Constrain within screen bounds (0–30). */
    if (joy & JOY_LEFT) {
        if (player_x > 0) player_x--;
    }
    if (joy & JOY_RIGHT) {
        if (player_x < 30) player_x++;
    }

    /* Fire projectile when Button A is pressed (edge-triggered). */
    static uint16_t prev_joy = 0;
    if (!(prev_joy & BUTTON_A) && (joy & BUTTON_A)) {
        if (!bullet_active) {
            bullet_active = 1;
            /* Start bullet just ahead of the player ship. */
            bullet_x = player_x + 1;
            /* Play the laser SFX (ADPCM-A index 0).  See games/neogeogame/sound/samples/in_wav_a/1.wav */
            playSFX(0);
        }
    }
    prev_joy = joy;

    /* Update bullet position if active. */
    if (bullet_active) {
        bullet_x++;
        /* Deactivate bullet if it leaves the screen. */
        if (bullet_x > 31) {
            bullet_active = 0;
        }
        /* Check collision with enemy.  A simple equality test since
         * everything is drawn on a single row. */
        if (bullet_x == enemy_x) {
            bullet_active = 0;
            /* Reset enemy and increment score. */
            enemy_x = 28;
            score++;
            /* Play explosion SFX (ADPCM-A index 1).  See games/neogeogame/sound/samples/in_wav_a/2.wav */
            playSFX(1);
        }
    }

    /* Move enemy leftwards every few frames to slow it down. */
    enemy_timer++;
    if (enemy_timer >= 8) {
        enemy_timer = 0;
        enemy_x--;
        if (enemy_x < 0) enemy_x = 28;
    }

    /* Draw to the FIX layer.  Always clear the layer first to avoid
     * leftover tiles from previous frames. */
    ng_fix_clear();

    /* Display score at the top-left. */
    ng_fix_puts(0, 0, "SCORE:", 0);
    char buf[6];
    sprintf(buf, "%d", score);
    ng_fix_puts(6, 0, buf, 0);

    /* Draw player ship on the bottom row (y=25). */
    ng_fix_putc((uint8_t)player_x, 25, '>', 0);
    /* Draw projectile if active. */
    if (bullet_active) {
        ng_fix_putc((uint8_t)bullet_x, 25, '-', 0);
    }
    /* Draw enemy ship on the top row (y=4). */
    ng_fix_putc((uint8_t)enemy_x, 4, 'X', 0);
}