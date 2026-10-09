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
 * shakes but doesn't flash (it would flash every one alike).
 *
 * The presets' holds are cut to Maiya's own below (mg_impact): a hold
 * stops her too, and three frames on every creature she knocked down read
 * as the game catching on her run and her jumps, not as a blow landing.
 * A creature beaten or struck now holds nothing -- the shake, the sparks
 * and the sound carry it -- and only a guardian's blows and her own hurt
 * hold, briefly.
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
#define MG_HITSTOP_KILL        0    /* frames held when a creature is beaten (the preset: 3)  */
#define MG_HITSTOP_HEAVY_HIT   0    /* frames held by a heavy blow a creature survives        */
#define MG_HITSTOP_HURT        2    /* frames held when Maiya is struck                       */
#define MG_HITSTOP_BOSS_HIT    2    /* frames held when a guardian is struck (the preset: 5)  */
#define MG_HITSTOP_BOSS_DOWN   10   /* frames held on a guardian's last blow                  */

#define MG_HURT_FLASH          12   /* frames of her red flash when struck (her bank only)    */

/*
 * Her stride on the road, in 8.8 pixels a frame (WALK_SPEED 512 is 2 px a
 * frame, RUN_SPEED 800 is 3.125): the speed steps toward what the stick
 * asks, never snaps to it.
 *   - from a stand to a walk in 8 frames; on, holding B, to a run in 6 more
 *   - let go, she stops in 4 frames from a walk, 7 from a run
 *   - the stick against her momentum: she brakes harder, still facing the
 *     way she ran (a skid), and turns only once nearly stopped -- 5 frames
 *     from a full run -- then sets off the other way
 *   - from a run to a walk (B let go) she eases down
 * In the air she has a little less grip and turns at once (a throw has to
 * go the way she faces); on ice the road's own slide (maiya_game.c).
 */
#define MG_WALK_ACCEL          64   /* up to a walk                                 */
#define MG_RUN_ACCEL           48   /* from a walk up to a run                      */
#define MG_STOP_BRAKE         128   /* stick let go                                 */
#define MG_TURN_BRAKE         176   /* stick against her momentum                   */
#define MG_RUN_EASE            64   /* a run down to a walk                         */
#define MG_TURN_FLIP           96   /* slow enough to turn round (3/8 px a frame)   */
#define MG_AIR_ACCEL           96
#define MG_AIR_BRAKE           64
#define MG_AIR_TURN           128

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
/* ... and while her legs pass under her: 98% walking, 97% at a run */
#define MG_RUN_PASSING        0xFA
#define MG_RUN_PASSING_FAST   0xF7
/* Her jump's poses by her climb speed (1/256 px a frame): the push-off
 * while she is still faster than this going up, the top of the arc within
 * this of still, rising or falling either side of it. */
#define MG_AIR_PUSH           (4 * 256 + 192)
#define MG_AIR_TOP            (256 + 128)
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
