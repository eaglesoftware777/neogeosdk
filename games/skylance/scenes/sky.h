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
/* Native 20-strip backgrounds cover the complete 320x224 CRT image. */
#define SKY_FIELD_X        0
#define SKY_FIELD_W        320
#define SKY_FIELD_LEFT     (SKY_FIELD_X + 12)
#define SKY_FIELD_RIGHT    (SKY_FIELD_X + SKY_FIELD_W - 12)
#define SKY_FIELD_TOP      24
#define SKY_FIELD_BOTTOM   206

/* Backdrop and scene-wipe colour. */
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
#define SKY_ITEM_LIFE       26u
#define SKY_ITEM_MISSILE    27u
#define SKY_ITEM_SPEED      28u
#define SKY_BG_OPEN_SEA     29u
#define SKY_BG_CONVOY       30u
#define SKY_BG_OUTPOST      31u
#define SKY_BG_ARMORED      32u
#define SKY_BG_CITADEL      33u
#define SKY_TITLE_ART       34u
#define SKY_PLANE_BANKED(pilot, right) (35u + (pilot) * 2u + (right))
#define SKY_EXPLOSION_FIRST 41u
#define SKY_CLOUD_ART       47u
#define SKY_START_FLAG      0x100700u

/* ------------------------------------------------------------------ */
/*  Hardware scale presets (SCB2 shrink, 0xFF = full size)              */
/* ------------------------------------------------------------------ */
/*
 * Craft and bosses draw at full size; shots and bursts use separate presets.
 *
 * The art set was authored far larger than a 320x224 playfield wants, and
 * the whole game used to run shrunk to compensate.  That is the wrong
 * place to lose the size: the sprite chip does not resample when it
 * shrinks, it drops rows and columns, and the rows it drops carry a
 * dither the quantiser laid down for pixels it expected to survive.  A
 * plane reduced to a quarter of its imported height arrives on screen as
 * a smear of colour with the shape mostly gone.
 *
 * The direct art builder imports each sprite at its final drawing size,
 * so there is nothing left between the pipeline's resampling and the
 * screen.  These stay named so a global re-tune is still one edit.
 *
 * If any of them goes back below full size, keep the low nibble at F: the
 * hardware shows ((value >> 4) + 1)/16 of the width but (value + 1)/256 of
 * the height, and those agree nowhere else - anything else squashes the
 * sprite horizontally. Content anchoring follows the two hardware ratios.
 *
 * Sizes these produce on a 320x224 screen:
 *   craft 48 px, bosses 112 px, explosions 32 px
 */
#define SKY_SCALE_PLAYER   0xFFu
#define SKY_SCALE_ENEMY    0xFFu
#define SKY_SCALE_SMALL    0xFFu
#define SKY_SCALE_BOSS     0xFFu
#define SKY_SCALE_FULL     0xFFu
#define SKY_SCALE_PROJECTILE NG_SCALE(6)
#define SKY_BURST_STEPS       6u

/*
 * Two exceptions, both on the select screen.
 *
 * The pilot portraits are native 64x80 images. The roster wants the
 * unselected planes visibly smaller than the one under the cursor, which
 * is a deliberate difference in size rather than a fit to the artwork.
 */
#define SKY_SCALE_PORTRAIT 0xFFu
#define SKY_SCALE_ROSTER   NG_SCALE(9)    /* unselected plane, 27x27 */

/* ------------------------------------------------------------------ */
/*  Entry points                                                        */
/* ------------------------------------------------------------------ */
/* One credited sortie: pilot select, then stages until the squad is out. */
void NEOGEO_USER sky_run(void);
/* Attract loop: title card and the pilot roster, until a coin lands. */
void NEOGEO_USER sky_run_attract(void);
uint8_t NEOGEO_USER sky_start_pending(void);

#endif /* SKY_H */
