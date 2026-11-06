/*******
https://eaglesoftware.biz
https://github.com/eaglesoftware777
https://github.com/eaglesoftware777/neogeosdk
******/

/*
 * Seven Wonders eyecatcher.
 *
 * The cartridge shows nothing of its own here: EagleBIOS and the console
 * system ROM draw their own boot screens, and the BIOS dispatches
 * EYE_CATCHER before INIT_GAME has loaded the FIX palettes, so no text
 * is drawn either.
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
    setBACKDROP(BLACK);
}

#pragma GCC pop_options
