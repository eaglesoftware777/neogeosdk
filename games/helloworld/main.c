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
void NEOGEO_USER game_boot(void) {
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

/* Called once per frame.  Poll the joystick and play a sound on an
 * edge-triggered Button A press.  Nothing else is drawn here since
 * the FIX layer was set up in game_boot().
 *
 * There is deliberately no file-scope state here: the ROM link strips
 * .text/.data/.bss from this translation unit and keeps only the
 * neogeo_user section, so a `static` would be dropped and the link would
 * fail with "symbol `.data' required but not present".  The BIOS already
 * tracks which buttons changed this frame, so poll_joystick_edge() gives
 * the edge without storing anything. */
void NEOGEO_USER game_frame(void) {
    /* poll_joystick() is the current state, poll_joystick_edge() the bits
     * that changed since last frame; their intersection is "pressed now".
     * Both return a bitfield of JOY_* and BUTTON_* flags from macro.h. */
    uint16_t joy     = poll_joystick();
    uint16_t pressed = (uint16_t)(joy & poll_joystick_edge());

    /* On a fresh Button A press, trigger sample 0.  The first ADPCM-A
     * sample for this game is encoded from 1.wav in sound/samples/in_wav_a.
     */
    if (pressed & BUTTON_A) {
        playSFX(0);
    }
}
