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

#endif
