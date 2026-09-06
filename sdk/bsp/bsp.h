#ifndef BSP_H
#define BSP_H

#include "sdk/macro.h"
#include "sdk/neogeo.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Contract: every game's user.c must define all symbols below.
 * The BIOS and interrupt vector table reference these by name at link time.
 */

/* ── Interrupt handlers ─────────────────────────────────────── */
NEOGEO_INTERRUPT void NEOGEO_USER VBlank(void);     /* IRQ1 — vertical blank        */
NEOGEO_INTERRUPT void NEOGEO_USER IRQ2(void);       /* IRQ2 — timer / LSPC          */
NEOGEO_INTERRUPT void NEOGEO_USER IRQ3(void);       /* IRQ3 — watchdog kick         */
NEOGEO_INTERRUPT void NEOGEO_USER INT4(void);
NEOGEO_INTERRUPT void NEOGEO_USER INT5(void);
NEOGEO_INTERRUPT void NEOGEO_USER INT6(void);
NEOGEO_INTERRUPT void NEOGEO_USER INT7(void);
NEOGEO_INTERRUPT void NEOGEO_USER ZD_ENTRY(void);
NEOGEO_INTERRUPT void NEOGEO_USER CHK_ENTRY(void);
NEOGEO_INTERRUPT void NEOGEO_USER TRAPV_ENTRY(void);

/* ── BIOS dispatch ──────────────────────────────────────────── */
void NEOGEO_USER USER(void);            /* main BIOS entry — contains REQUEST_VECTOR table */
void NEOGEO_USER POWER_ON(void);        /* cold boot: clear RAM, return to BIOS            */
void NEOGEO_USER EYE_CATCHER(void);     /* SNK eye-catcher animation                       */
void NEOGEO_USER GAME(void);            /* game mode entry — sets up stack, calls INIT_GAME */
void NEOGEO_USER TITLE(void);           /* MVS title mode entry                            */

/* ── BIOS hooks ─────────────────────────────────────────────── */
void NEOGEO_USER PLAYER_START(void);    /* credit insert + start press                     */
void NEOGEO_USER DEMO_END(void);        /* BIOS demo mode ending — stop sound              */
void NEOGEO_USER COIN_SOUND(void);      /* coin inserted — play chime                      */

/* ── Game init ──────────────────────────────────────────────── */
void NEOGEO_USER WORK_INIT(void);       /* clear game RAM                                  */
void NEOGEO_USER DISPLAY_INIT(void);    /* clear FIX, clear sprites, init palettes         */
void NEOGEO_USER INIT_GAME(void);       /* called by GAME and TITLE before game code runs  */

/* ── Game entry points ──────────────────────────────────────── */
void NEOGEO_USER GAME_DISPATCH(void);   /* branch MVS attract vs start-game                */
void NEOGEO_USER GAME_ATTRACT(void);    /* attract / demo loop                             */
void NEOGEO_USER START_GAME(void);      /* credit accepted — run the game                  */
void NEOGEO_USER DEMO_GAME(void);       /* BIOS demo mode hook                             */


#ifdef __cplusplus
}
#endif
#endif
