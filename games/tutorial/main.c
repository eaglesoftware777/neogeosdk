/*
 * Minimal tutorial skeleton demonstrating the high-level game engine.
 *
 * This example shows how to initialise and drive the Neo Geo SDK's
 * game engine.  The engine handles sprite updates, background
 * scrolling, particle effects, palette fades and more.  It will call
 * through to your own hooks (if registered) before and after the
 * logic and drawing phases.
 */

#include <stdint.h>
#include "sdk/2d_engine/ng_game_engine.h"
#include "sdk/neogeo.h"
#include "sdk/macro.h"

/* Called once at game boot.  Initialise the game engine and sound. */
void game_boot(void) {
    game_engine_init();
    /* Set up the audio driver.  This enables the use of playSFX() and
     * other sound functions provided by the SDK. */
    soundInit();
}

/* Called once per frame.  Wait for vertical blank and run one frame
 * of the game engine.  After the first frame, play a sound effect
 * exactly once to demonstrate triggering audio from the high-level
 * engine loop.
 */
void game_frame(void) {
    static uint8_t played = 0;
    waitVbl();
    game_engine_frame();
    /* Trigger a simple sample on the first frame after boot.  This
     * uses ADPCM-A sample index 0 which must be provided in
     * sound/samples/in_wav_a/1.wav. */
    if (!played) {
        playSFX(0);
        played = 1;
    }
}