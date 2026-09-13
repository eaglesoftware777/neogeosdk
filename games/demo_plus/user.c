/*
 * games/demo_plus/user.c
 *
 * NeoGeo SDK Demo PLUS — BIOS entry points and attract/start handlers.
 *
 * demo_plus runs the demo's flow on the C++ engine, so the handlers are the
 * demo's, verbatim.  Compiling that translation unit here rather than keeping
 * a copy means the two games cannot drift apart: a change to the demo's
 * attract loop or start handler shows up in demo_plus on the next build.
 *
 * games/demo and games/demo/artbox are on the include path via
 * GAME_EXTRA_INCLUDES in this game's game.mk.
 */
#include "../demo/user.c"
