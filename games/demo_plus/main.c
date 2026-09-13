/*
 * games/demo_plus/main.c
 *
 * NeoGeo SDK Demo PLUS — entry point.
 *
 * The full set of artbox assets (sprite art, screens, palettes, sprite_meta,
 * showScreenN bodies) is shared verbatim with the demo game.  Re-implementing
 * those tables here would be ~3600 lines of duplicated data.  Instead the
 * demo's main.c is compiled with -Igames/demo and -Igames/demo/artbox supplied
 * by the demo_plus game.mk, so the same showScreenN bodies and asset tables
 * are linked into the demo_plus ROM.
 *
 * This file just forwards to that translation unit.
 */
#include "../demo/main.c"
