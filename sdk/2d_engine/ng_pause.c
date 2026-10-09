/*
 * ng_pause.c — Pause (see ng_pause.h). The freeze it sets is ng_freeze,
 * kept by ng_feedback; this file is in the on-demand library.
 */

#include "ng_pause.h"
#include "neogeo.h"   /* the sound wrapper */

void NEOGEO_USER ng_pause_set_music_levels(uint8_t adpcmb, uint8_t ssg, uint8_t fm)
{
    ng_freeze.levels[0] = adpcmb;
    ng_freeze.levels[1] = ssg;
    ng_freeze.levels[2] = fm;
    ng_freeze.music = 1;
}

void NEOGEO_USER ng_pause_set(int on)
{
    uint8_t now = (uint8_t)(on ? 1 : 0);
    if (now == ng_freeze.paused) return;
    ng_freeze.paused = now;
    if (!ng_freeze.music) return;
    soundSetADPCMBVolume(now ? 0 : ng_freeze.levels[0]);
    soundSetSSGVolume(now ? 0 : ng_freeze.levels[1]);
    soundSetFMVolume(now ? 0 : ng_freeze.levels[2]);
}

int NEOGEO_USER ng_pause_is_on(void)
{
    return ng_freeze.paused;
}
