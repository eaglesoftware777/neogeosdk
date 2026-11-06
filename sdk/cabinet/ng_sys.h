/*
 * ng_sys.h — Which machine the game is running on
 *
 * Read at run time from what the system ROM leaves in its RAM before it
 * starts the game (the addresses are in macro.h), so one ROM behaves right
 * on an arcade board and on a console, and under a system ROM that can
 * switch between them (the UniBIOS).
 *
 *   ng_sys_is_mvs()   1 on an arcade board (MVS): coins and credits
 *                     0 on a console (AES): Start alone begins a game
 *   ng_sys_region()   NG_REGION_JP, NG_REGION_US or NG_REGION_EU;
 *                     any other code reads as NG_REGION_US
 *
 * Plain RAM reads: cheap, safe to call at any time after the system ROM
 * has started the game. Only a game that calls them links them (the
 * cabinet modules are linked from a library).
 */

#ifndef NG_SYS_H
#define NG_SYS_H

#ifdef __cplusplus
extern "C" {
#endif

/* The region codes are the system ROM's own. */
#define NG_REGION_JP  0
#define NG_REGION_US  1
#define NG_REGION_EU  2

int ng_sys_is_mvs(void);
int ng_sys_region(void);

#ifdef __cplusplus
}
#endif
#endif
