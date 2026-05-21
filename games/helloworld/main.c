/*
 * Simple Hello World example for the Neo Geo SDK.
 *
 * This example clears the FIX layer and prints a message on the screen.
 * It uses the minimal game loop of the SDK: `game_boot` is called once
 * at boot and `game_frame` is called once per frame.  There is no
 * gameplay logic in this example.
 */

#include <stdint.h>
#include "sdk/2d_engine/ng_fix.h"
#include "sdk/neogeo.h"
#include "sdk/macro.h"    /* button and joystick bit masks */

/* Called once at game start.  Initialize the FIX layer, draw text and
 * set up the sound system.  This example now demonstrates how to
 * initialise the Neo Geo sound driver and trigger a sample when the
 * player presses a button.  See neogeo.h for soundInit() and playSFX().
 */
void game_boot(void) {
    /* Initialise the FIX text layer */
    ng_fix_init();
    /* Clear the entire FIX layer so nothing from previous games remains */
    ng_fix_clear();
    /* Print our message at a safe position on the screen (x,y) coordinates
     * are in tile units.  The third argument selects the palette (0=default). */
    ng_fix_puts(3, 15, "Hello World!", 0);
    /* Provide a short hint to the player about the new sound feature. */
    ng_fix_puts(3, 17, "Press A for sound", 0);
    /* Initialise the sound hardware.  This must be called once before
     * playing any samples or music.  It sets up the YM2610 and resets
     * any previous state. */
    soundInit();
}

/* State used to detect button edges.  Placed at file scope so it
 * persists between frames. */
static uint16_t prev_joy = 0;

/* Called once per frame.  Poll the joystick and play a sound on an
 * edge-triggered Button A press.  Nothing else is drawn here since
 * the FIX layer was set up in game_boot(). */
void game_frame(void) {
    /* Read the current joystick state.  poll_joystick() returns a
     * bitfield of JOY_* and BUTTON_* flags defined in macro.h. */
    uint16_t joy = poll_joystick();

    /* Detect a rising edge on Button A (pressed this frame but not
     * last frame).  If detected, trigger sample 0.  The first ADPCM-A
     * sample for this game is encoded from 1.wav in sound/samples/in_wav_a.
     */
    if (!(prev_joy & BUTTON_A) && (joy & BUTTON_A)) {
        playSFX(0);
    }
    /* Save for next frame */
    prev_joy = joy;
}