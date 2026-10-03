/*
 * Maiya's game feel: how long a blow holds the scene still (hitstop, in
 * frames at 60 Hz) and how the camera shakes. Every value lives here.
 *
 * Kills and guardian hits go through ng_impact_event(), whose presets
 * (sdk/2d_engine/ng_feedback.c) set the hold, the shake and the flash:
 *   NG_IMPACT_LIGHT   3 frames held, 1 px shake for 4 frames, white flash 4
 *   NG_IMPACT_MEDIUM  5 frames held, 2 px shake for 6 frames, red flash 6
 *   NG_IMPACT_HEAVY   8 frames held, 3 px shake for 8 frames, white flash 8
 * The flash is given only where the target has a palette bank of its own:
 * a guardian does; the creatures of one kind share a bank, so a kill
 * shakes and holds but doesn't flash (it would flash every one alike).
 * No engine particles: their sprite slots (256-287) are Maiya's vines and
 * foreground, so her own sparks and dust stay the particles of a hit.
 */
#ifndef MAIYA_FEEL_H
#define MAIYA_FEEL_H

#include "ng_feedback.h"

#define MG_IMPACT_KILL         NG_IMPACT_LIGHT    /* a creature beaten            */
#define MG_IMPACT_BOSS_HIT     NG_IMPACT_MEDIUM   /* a guardian struck            */
#define MG_IMPACT_BOSS_DOWN    NG_IMPACT_HEAVY    /* a guardian beaten            */

#define MG_HEAVY_DAMAGE        3    /* a blow this strong (Surge, stomp, Secret Art) is heavy */
#define MG_HITSTOP_HEAVY_HIT   4    /* frames held by a heavy blow a creature survives        */
#define MG_HITSTOP_HURT        5    /* frames held when Maiya is struck                       */
#define MG_HITSTOP_BOSS_DOWN   14   /* frames held on a guardian's last blow (on top of HEAVY) */

#define MG_HURT_FLASH          12   /* frames of her red flash when struck (her bank only)    */

#define MG_SHAKE_HURT          10   /* frames of Maiya's own 2 px shake when she is struck    */
#define MG_SHAKE_BOSS_DOWN     16   /* ... and when a guardian falls                          */

/*
 * Her squash and stretch (maiya_game.c, mg_squash_step), by the hardware's
 * shrink -- it only ever makes a sprite smaller, so "stretched" is drawn
 * narrower. Per kind and frame: the horizontal shrink (top nibble:
 * sixteenths of each strip, less one) and the vertical (256ths, less one).
 * Her feet and her middle stay where they are.
 */
enum { MG_SQ_LAND = 1, MG_SQ_JUMP, MG_SQ_HURT, MG_SQ_STRIKE, MG_SQ_KINDS };
/* her run's footfall: how tall she is on the two poses that plant a foot
 * (256ths, less one): 97% walking, 95% at a run */
#define MG_RUN_FOOTFALL       0xF7
#define MG_RUN_FOOTFALL_FAST  0xF2
enum { MG_SQ_FRAMES = 6 };
static const uint8_t mg_sq_len[MG_SQ_KINDS] = { 0, 6, 4, 4, 3 };
static const uint8_t mg_sq_x[MG_SQ_KINDS][MG_SQ_FRAMES] = {
    { 0 },
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },          /* landing                          */
    { 0xCF, 0xDF, 0xEF, 0xFF },                      /* springing up: 13/16 wide, back    */
    { 0xDF, 0xDF, 0xEF, 0xFF },                      /* struck                           */
    { 0xFF, 0xFF, 0xFF },                            /* striking                         */
};
static const uint8_t mg_sq_y[MG_SQ_KINDS][MG_SQ_FRAMES] = {
    { 0 },
    { 0xCB, 0xD7, 0xE3, 0xEF, 0xF7, 0xFF },          /* landing: 80% tall, back over six  */
    { 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xDF, 0xE7, 0xF3, 0xFF },
    { 0xEF, 0xF7, 0xFF },
};

#endif
