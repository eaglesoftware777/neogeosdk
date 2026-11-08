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
 *
 *   ng_sys_credit_check()  the system ROM's SYS_CREDIT_CHECK and
 *   ng_sys_credit_down()   SYS_CREDIT_DOWN, safe from compiled code
 *
 * Both routines change registers a C caller expects a call to keep (D2, D4,
 * A3 to A5; the check clears D2 as it counts), which CALLNEOGEOF does not
 * save: a value the compiler held there came back wrong, or a function
 * pointer in A4 sent the CPU into backup RAM. These save them. Set the
 * decrements first, one BCD byte per player from BIOS_CREDIT_DEC (0x10FDB0):
 * the check clears those a player cannot pay, the down call takes the rest.
 * Arcade board only.
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

#define BIOS_CREDIT_DEC  0x10FDB0   /* 4 bytes: the credits to take per player */
void ng_sys_credit_check(void);
void ng_sys_credit_down(void);

#ifdef __cplusplus
}
#endif
#endif
