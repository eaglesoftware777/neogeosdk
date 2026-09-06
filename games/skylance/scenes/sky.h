#ifndef SKY_H
#define SKY_H

#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/2d_engine/ng_sprite_pool.h"

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

/* ------------------------------------------------------------------ */
/*  Screen geometry                                                     */
/* ------------------------------------------------------------------ */
/*
 * The CRT is 320x224, but an artbox page is 16x16 tiles = 256x256 px and
 * the hardware can only SHRINK a sprite, never stretch it, so a
 * background page covers 256 of the 320 columns.  Sky Lance therefore
 * flies inside that 256-wide column, drawn at x = SKY_FIELD_X, with the
 * 32 px either side left as backdrop - the pillars the HUD sits on.
 */
#define SKY_FIELD_X        32
#define SKY_FIELD_W        256
#define SKY_FIELD_LEFT     (SKY_FIELD_X + 12)
#define SKY_FIELD_RIGHT    (SKY_FIELD_X + SKY_FIELD_W - 12)
#define SKY_FIELD_TOP      24
#define SKY_FIELD_BOTTOM   206

/* Backdrop behind the pillars, and the colour the wipes run on. */
#define SKY_BG_CLEAR       BLACK

/* FIX palette banks, loaded by setup_fix_palettes() in user.c. */
#define SKY_PAL_BODY       0u
#define SKY_PAL_SCORE      1u
#define SKY_PAL_TITLE      2u
#define SKY_PAL_WARN       3u

/* Sprite slot for the one-off full-screen showScreenN() calls. */
#define SKY_SHOWSCREEN_BASE  NG_SPR_VRAM_BASE(1u)

/* ------------------------------------------------------------------ */
/*  Artbox asset ids (1-based; see games/skylance/artbox/sprite_meta.h) */
/* ------------------------------------------------------------------ */
#define SKY_BG_COAST         1u
#define SKY_BG_MOUNTAIN      2u

#define SKY_P1_PILOT         3u
#define SKY_P1_PLANE         4u
#define SKY_P2_PILOT         5u
#define SKY_P2_PLANE         6u
#define SKY_P3_PILOT         7u
#define SKY_P3_PLANE         8u
#define SKY_SHOT_PLAYER      9u

#define SKY_E_BOMBER        10u
#define SKY_B_CATHEDRAL     11u
#define SKY_B_GOLD_CORE     12u
#define SKY_B_HELI_CARRIER  13u
#define SKY_B_BATTLESHIP    14u
#define SKY_B_RED_FORTRESS  15u
#define SKY_B_STEALTH       16u
#define SKY_B_TANK_FORT     17u
#define SKY_E_DRONE         18u
#define SKY_E_FIGHTER       19u
#define SKY_E_HELI          20u
#define SKY_E_INTERCEPTOR   21u
#define SKY_E_MISSILE_BOAT  22u
#define SKY_SHOT_ORB        23u
#define SKY_SHOT_RING       24u
#define SKY_E_TANK          25u

/* ------------------------------------------------------------------ */
/*  Hardware scale presets (SCB2 shrink, 0xFF = full size)              */
/* ------------------------------------------------------------------ */
/*
 * The art set was authored far larger than a 320x224 playfield wants, so the
 * whole game runs shrunk.  These are the only four values used; keeping them
 * named makes a global re-tune one edit rather than a hunt through every
 * draw call.
 *
 * The hardware shrinks the two axes differently: X shows ((value >> 4) + 1)/16
 * of the width, Y shows (value + 1)/256 of the height.  Those agree only when
 * the low nibble is F, so every value here ends in F - anything else squashes
 * the sprite horizontally and desyncs sky_scaled(), which uses value/256 for
 * both axes when it positions the artwork.
 *
 * Sizes these produce on a 320x224 screen:
 *   player plane 112x160 -> 30x43     enemy fighter 128x144 -> 26x29
 *   drone         56x56  -> 15x15     boss gold core 240x240 -> 96x97
 */
#define SKY_SCALE_PLAYER   0x4Fu   /* 5/16 - player ship, 22% of screen height */
#define SKY_SCALE_ENEMY    0x3Fu   /* 4/16 - standard opponents                 */
#define SKY_SCALE_SMALL    0x4Fu   /* 5/16 - already-small art (drone, props)   */
#define SKY_SCALE_BOSS     0x6Fu   /* 7/16 - bosses, 47% of screen height       */
#define SKY_SCALE_FULL     0xFFu

/* ------------------------------------------------------------------ */
/*  Entry points                                                        */
/* ------------------------------------------------------------------ */
/* One credited sortie: pilot select, then stages until the squad is out. */
void NEOGEO_USER sky_run(void);
/* Attract loop: title card and the pilot roster, until a coin lands. */
void NEOGEO_USER sky_run_attract(void);

#endif /* SKY_H */
