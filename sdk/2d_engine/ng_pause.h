/*
 * ng_pause.h — Pause, and the freeze underneath pause, hitstop and slow motion
 *
 * One state holds the game still, for three reasons:
 *
 *   pause         ng_pause_set(1): nothing moves -- characters, timers
 *                 (ng_game_time too), ng_palette_fx, particles and the
 *                 camera all hold. The engine frame still reads the input,
 *                 so a game can unpause, and the VBlank interrupt still
 *                 kicks the watchdog.
 *   hitstop       ng_feedback_hitstop(n): for n frames the world's logic
 *                 holds (for a game that opted in with
 *                 ng_game_engine_set_hitstop_freeze), while the frame is
 *                 still drawn, so a shake or a flash plays over it.
 *   slow motion   ng_feedback_slow_motion(n): for n frames the logic holds
 *                 on every other frame, the same way.
 *
 * Hitstop and slow motion count down once per unpaused frame
 * (ng_feedback_update); a pause holds them too.
 *
 * Music: pausing silences the music channels (ADPCM-B, SSG, FM) through the
 * sound wrapper, and resuming puts back the levels the game last gave to
 * ng_pause_set_music_levels(); effects (ADPCM-A) stay, so a pause jingle is
 * heard. The Z80 driver has no pause of its own, so the music runs on
 * silently and comes back a little further on. Until a game gives its
 * levels, pausing leaves the sound alone. ng_game_engine_init() clears the
 * pause, hitstop, slow motion and the levels.
 *
 * The state lives with ng_feedback, which every game links; ng_pause_set()
 * and the rest come from the on-demand library, so only a game that pauses
 * carries them. C engine only.
 */

#ifndef NG_PAUSE_H
#define NG_PAUSE_H

#include <stdint.h>
#include "ng_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t paused;      /* ng_pause_set() */
    uint8_t hitstop;     /* frames of hitstop left */
    uint8_t slow;        /* frames of slow motion left */
    uint8_t music;       /* 1 once the game has given its music levels */
    uint8_t levels[3];   /* ADPCM-B, SSG, FM */
} NGFreeze;

extern NGFreeze ng_freeze;

/* 1 when this frame the world's logic holds: paused, in a hitstop, or on
 * slow motion's held frame. */
#define NG_FREEZE_LOGIC() (ng_freeze.paused || ng_freeze.hitstop || (ng_freeze.slow & 1u))

void NEOGEO_USER ng_pause_set(int on);
int  NEOGEO_USER ng_pause_is_on(void);
void NEOGEO_USER ng_pause_set_music_levels(uint8_t adpcmb, uint8_t ssg, uint8_t fm);

#ifdef __cplusplus
}
#endif
#endif
