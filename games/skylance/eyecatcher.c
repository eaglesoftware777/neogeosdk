/*******
https://eaglesoftware.biz
https://github.com/eaglesoftware777
https://github.com/eaglesoftware777/neogeosdk
******/

/*
 * Sky Lance eyecatcher.
 *
 * artbox/gen_eyecatcher.py only writes this file when the art set has an
 * in/eyecatcher/ category; Sky Lance has none, so this hand-written
 * version stands.  It shows the sortie backdrop for a beat - no FIX text,
 * because the BIOS dispatches EYE_CATCHER before INIT_GAME has loaded the
 * game's FIX palettes and any text drawn here would come up unreadable.
 */

#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include <stdint.h>

#define EC_SPRITE_BASE 0x0040u

#pragma GCC push_options
#pragma GCC optimize ("O0")

void NEOGEO_USER showScreen2(int x0, int y0, int xr, int yr, int min_crt_sz,
                             uint16_t backdrop, uint16_t sprite_base);

void NEOGEO_USER showEyeCatcherMVS(void)
{
    clearFix();
    clearSprs();
    showScreen2(32, 0, 0xF, 0xFF, 16, 0x0000, EC_SPRITE_BASE);
    cyclexms(900);
    clearSprs();
}

#pragma GCC pop_options
