/*******
https://eaglesoftware.biz
https://github.com/eaglesoftware777
https://github.com/eaglesoftware777/neogeosdk
******/

/*
 * Maiya & Luna — BIOS entry points.
 *
 * The interrupt vectors and the POWER_ON / EYE_CATCHER / GAME / TITLE
 * request dispatch use the SDK's BIOS handshake, with board-aware credit
 * handling and sound initialization after the game's work RAM is cleared.
 */

#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/bsp/bsp.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "games/maiyaluna/scenes/ml.h"
#pragma GCC push_options
#pragma GCC optimize ("O0")

#define NGO_START_FLAG  ML_START_FLAG

void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER maingame(void);
void NEOGEO_USER GAME_ATTRACT(void);
void NEOGEO_USER START_GAME(void);
void NEOGEO_USER GAME_DISPATCH(void);
void NEOGEO_USER showTitleMVS(void);
void NEOGEO_USER showEyeCatcherMVS(void);

//ZD_ENTRY interrupt subroutine
NEOGEO_INTERRUPT void NEOGEO_USER ZD_ENTRY(void) {

}

//CHK_ENTRY interrupt subroutine
NEOGEO_INTERRUPT void NEOGEO_USER CHK_ENTRY(void) {

}

//TRAPV_ENTRY interrupt subroutine
NEOGEO_INTERRUPT void NEOGEO_USER TRAPV_ENTRY(void) {

}


//v-blank interrupt subroutine
NEOGEO_INTERRUPT void NEOGEO_USER VBlank(void) {

	ASM_START
	ASM_L(VBLANK)                  // Label defined in header.asm
	ASM_MVW(#1, USER_WORKRAM)       // A word in USER_WORKRAM : vblank flag
	ASM_BTST(#7,BIOS_SYSTEM_MODE)  // Check if the system ROM wants to take care of the interrupt
	ASM_BNE(.getvbl)               // No: jump to .getvbl
	ASM_JMP(SYS_INT1)              // Yes: jump to system ROM
	ASM_L(.getvbl)
	ASM_MVEML(%%d0-%%d7/%%a0-%%a6,-(%%sp))
	ASM_MVW(#4,REG_IRQACK) // Acknowledge the VBlank interrupt
	ASM_MVB(%%d0,REG_DIPSW)        // Kick watchdog
	ASM_JSR(ml_vblank)
	ASM_JSR(SYS_IO)
	ASM_MVEML((%%sp)+, %%d0-%%d7/%%a0-%%a6)
	:
	:
	:
	ASM_END
}

//IRQ2 interrupt
NEOGEO_INTERRUPT void  NEOGEO_USER IRQ2(void) {

	ASM_START
	ASM_MVW(#2,REG_IRQACK)				//;IRQ2
	:
	:
	:
	ASM_END
}

//IRQ3 interrupt
NEOGEO_INTERRUPT void  NEOGEO_USER IRQ3 (void) {

	ASM_START
	ASM_MVW(#1,REG_IRQACK)
	ASM_MVB(%%d0,REG_DIPSW)
	:
	:
	:
	ASM_END
}

//INT4 interrupt
NEOGEO_INTERRUPT void  NEOGEO_USER INT4 (void) {

}
//INT5 interrupt
NEOGEO_INTERRUPT void  NEOGEO_USER INT5 (void) {

}

//INT6 interrupt
NEOGEO_INTERRUPT void  NEOGEO_USER INT6 (void) {

}

//INT7 interrupt
NEOGEO_INTERRUPT void  NEOGEO_USER INT7 (void) {

}

// NeoGeo USER Mode
void  NEOGEO_USER USER(void) {

	ASM_START
	ASM_L(REQUEST_VECTOR)
	ASM_DL(POWER_ON)
	ASM_DL(EYE_CATCHER)
	ASM_DL(GAME)
	ASM_DL(TITLE)
	ASM_MVB(BIOS_USER_REQUEST,%%d0)
	ASM_LSLB(#2,%%d0)
	ASM_LEA(REQUEST_VECTOR,%%a0)
	ASM_MVAL((%%a0,%%d0),%%a0)
	ASM_JSR((%%a0))
	ASM_JMP(SYS_RETURN)
	:
	:
	:
	ASM_END
}

// NeoGeo PLAYER_START handler
void NEOGEO_USER PLAYER_START (void) {

	uint16_t start_flag = NEO_REGISTER8(BIOS_START_FLAG);
	uint16_t country_code = NEO_REGISTER8(BIOS_COUNTRY_CODE);
	if (!(start_flag & 1u) || NEO_REGISTER8(BIOS_PLAYER1_MODE) == 1u) {
		NEO_REGISTER8(BIOS_START_FLAG) = 0;
		return;
	}

	// US BIOS clears BIOS_START_FLAG before calling PLAYER_START (Japan BIOS leaves
	// the bit set). Setting BIOS_PLAYER1_MODE and BIOS_START_FLAG unconditionally
	// ensures the BIOS does not see "no player active" after we return, which would
	// cause it to invoke POWER_ON and wipe NGO_START_FLAG.
	NEO_REGISTER8(BIOS_USER_MODE) = 2;
	NEO_REGISTER8(NGO_START_FLAG) = 1;
	NEO_REGISTER8(BIOS_PLAYER1_MODE) = 1;
	NEO_REGISTER8(BIOS_START_FLAG) = start_flag | 1;

	if ((start_flag >> 1) & 1) {
		NEO_REGISTER8(BIOS_PLAYER2_MODE) = (country_code == 1) ? 2 : 3;
	}
	soundSetADPCMAVolume(0x3C);
	playSFX(SOUND_SFX_3);
	if (ng_sys_is_mvs()) {
		CALLNEOGEOF(SYS_CREDIT_CHECK);
		CALLNEOGEOF(SYS_CREDIT_DOWN);
	}
}

// NeoGeo DEMO_END handler
void NEOGEO_USER DEMO_END (void) {

	soundStopAll();
}

// NeoGeo COIN_SOUND handler
void NEOGEO_USER COIN_SOUND (void) {

	isZ80Ready();
	soundSetADPCMAVolume(0x3C);
	isZ80Ready();
	playSFX(SOUND_SFX_1);
}

// NeoGeo POWER_ON handler
void  NEOGEO_USER POWER_ON (void) {

	//MVS only
	NEO_REGISTER8(NGO_START_FLAG) = 0;
	ASM_START
	ASM_MVB(#0x00,BIOS_USER_MODE) //user_request = 0
	ASM_LEA(USER_WORKRAM+64,%%a0)	//Start of game save block
	ASM_MV(#0x001F,%%d1)		//32*4 longword writes = 512 bytes
	ASM_MVQ(#0,%%d0)
	ASM_L(.cl)                    //init game save block
	ASM_MVL(%%d0,(%%a0)+)
	ASM_MVL(%%d0,(%%a0)+)
	ASM_MVL(%%d0,(%%a0)+)
	ASM_MVL(%%d0,(%%a0)+)
	ASM_DBF(%%d1,.cl)
	ASM_MVB(%%d0,REG_DIPSW)   // Kick watchdog
	ASM_JMP(SYS_RETURN)
	:
	:
	:
	ASM_END
}

// NeoGeo EYE_CATCHER handler — same sprite animation for both AES and MVS
void  NEOGEO_USER EYE_CATCHER (void) {

	clearFix();
	clearSprs();
	setBACKDROP(BLACK);
}

// NeoGeo GAME Mode
void  NEOGEO_USER GAME (void) {

	ASM_START
	ASM_LEA(BIOS_WORKRAM,%%sp)
	ASM_MVB(%%d0,REG_DIPSW)
	ASM_MVW(#0x0000,REG_LSPCMODE)
	ASM_MVW(#7,REG_IRQACK)
	ASM_ADDQB(#1,BIOS_MESS_BUSY)
	ASM_BCLRB(#7,BIOS_SYSTEM_MODE)
	ASM_MVW(#0x2700,%%sr)
	ASM_SUBQB(#1,BIOS_MESS_BUSY)
	ASM_BSETB(#7,BIOS_SYSTEM_MODE)
	ASM_JSR(INIT_GAME)
	ASM_JSR(GAME_DISPATCH)
	ASM_JMP(SYS_RETURN)
	:
	:
	:
	ASM_END
}

/* C-based GAME dispatch — avoids inline-asm address read issues */
void NEOGEO_USER GAME_DISPATCH(void) {
	if (!NEO_REGISTER8(NGO_START_FLAG)) {
		NEO_REGISTER8(BIOS_USER_MODE) = 1;
		GAME_ATTRACT();
	}
	if (NEO_REGISTER8(NGO_START_FLAG) || (ng_sys_is_mvs() && read_p1credit() > 0)) {
		NEO_REGISTER8(BIOS_USER_MODE) = 2;
		showTitleMVS();
		START_GAME();
	}
}

/* NeoGeo MVS TITLE mode */
void NEOGEO_USER TITLE(void) {
	ASM_START
	ASM_LEA(BIOS_WORKRAM,%%sp)
	ASM_MVB(%%d0,REG_DIPSW)
	ASM_MVW(#0x0000,REG_LSPCMODE)
	ASM_MVW(#7,REG_IRQACK)
	ASM_ADDQB(#1,BIOS_MESS_BUSY)
	ASM_BCLRB(#7,BIOS_SYSTEM_MODE)
	ASM_MVW(#0x2700,%%sr)
	ASM_MVB(#0x01,BIOS_USER_MODE)
	ASM_SUBQB(#1,BIOS_MESS_BUSY)
	ASM_BSETB(#7,BIOS_SYSTEM_MODE)
	ASM_JSR(INIT_GAME)
	ASM_JSR(showTitleMVS)
	ASM_MVB(#0x02,BIOS_USER_MODE)
	ASM_JSR(START_GAME)
	ASM_JMP(SYS_RETURN)
	:
	:
	:
	ASM_END
}

/* A credit or a console START is waiting: the game's screens end early. */
uint8_t NEOGEO_USER ml_start_pending(void) {
	if (NEO_REGISTER8(NGO_START_FLAG)) return 1u;
	if (ng_sys_is_mvs()) return (uint8_t)(read_p1credit() > 0);
	if (NEO_REGISTER8(BIOS_STATCHANGE) & 1u) {
		NEO_REGISTER8(NGO_START_FLAG) = 1u;
		NEO_REGISTER8(BIOS_USER_MODE) = 2u;
		return 1u;
	}
	return 0u;
}

/* An arcade board with a credit and no START yet: take it after the title's countdown. */
void NEOGEO_USER ml_auto_start(void) {
	uint16_t saved_sr;
	__asm__ volatile ("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d" (saved_sr) : : "memory");
	if (!NEO_REGISTER8(NGO_START_FLAG)) {
		NEO_REGISTER8(0x10FDB0) = 1;
		NEO_REGISTER8(0x10FDB1) = 0;
		NEO_REGISTER8(0x10FDB2) = 0;
		NEO_REGISTER8(0x10FDB3) = 0;
		CALLNEOGEOF(SYS_CREDIT_CHECK);
		if (NEO_REGISTER8(0x10FDB0)) {
			CALLNEOGEOF(SYS_CREDIT_DOWN);
			NEO_REGISTER8(NGO_START_FLAG) = 1;
			NEO_REGISTER8(BIOS_PLAYER1_MODE) = 1;
			NEO_REGISTER8(BIOS_USER_MODE) = 2;
		}
	}
	__asm__ volatile ("move.w %0,%%sr" : : "d" (saved_sr) : "memory");
}

/* A credit spent on a continue (arcade board): 1 if there was one. */
uint8_t NEOGEO_USER ml_spend_credit(void) {
	uint8_t ok = 0u;
	uint16_t saved_sr;
	if (!ng_sys_is_mvs()) return 1u;
	__asm__ volatile ("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d" (saved_sr) : : "memory");
	NEO_REGISTER8(0x10FDB0) = 1;
	NEO_REGISTER8(0x10FDB1) = 0;
	NEO_REGISTER8(0x10FDB2) = 0;
	NEO_REGISTER8(0x10FDB3) = 0;
	CALLNEOGEOF(SYS_CREDIT_CHECK);
	if (NEO_REGISTER8(0x10FDB0)) { CALLNEOGEOF(SYS_CREDIT_DOWN); ok = 1u; }
	__asm__ volatile ("move.w %0,%%sr" : : "d" (saved_sr) : "memory");
	return ok;
}

void NEOGEO_USER showTitleMVS(void) {
	ml_title_screen();
}

void NEOGEO_USER WORK_INIT(void) {
	uint32_t *p1 = (uint32_t *)RAMSTART;
	int i = 0;
	/* Clear 0x100000-0x10EFFF (game area, 60 KB). Must stop before
	   BIOS_WORKRAM at 0x10F300 or the BIOS stack gets corrupted. */
	for (i = 0; i < 15360; i++) {
		if (!(i & 255)) kickWatchDog();
		if ((uintptr_t)p1 < 0x100400u || (uintptr_t)p1 >= 0x100600u) *p1 = 0;   /* the save block survives */
		p1++;
	}
}

void NEOGEO_USER DISPLAY_INIT(void) {
	ASM_START
	ASM_MVW(#0x8000,PALETTES)
	ASM_MVW(#0x8000,PALETTES+8190)   /* backdrop = black */
	ASM_JSR(SYS_FIX_CLEAR)
	ASM_JSR(SYS_LSP_1ST)
	ASM_JSR(clearSprs)
	ASM_JSR(clearFix)
	:
	:
	:
	ASM_END
}

/* FIX text palettes 0-4: see ml_fix_palettes() in ml_draw.c. */
void NEOGEO_USER setup_fix_palettes(void) {
	ml_fix_palettes();
}

void NEOGEO_USER INIT_GAME(void) {
	ASM_START
	ASM_MVW(#0x2700,%%sr)
	ASM_JSR(WORK_INIT)
	ASM_JSR(soundInit)
	ASM_JSR(DISPLAY_INIT)
	ASM_JSR(ml_presentation_init)
	ASM_JSR(setup_fix_palettes)
	ASM_MVB(#1,BIOS_USER_MODE)
	ASM_MVW(#0x2000,%%sr)
	:
	:
	:
	ASM_END
}

void NEOGEO_USER DEMO_GAME(void) {
	ml_run_attract();
}

void NEOGEO_USER GAME_ATTRACT(void) {
	DEMO_GAME();
}

/* START_GAME handler — one credited game, then back to attract. */
void NEOGEO_USER START_GAME(void) {
	setBACKDROP(BLACK);
	waitVbl();
	clearFix();
	clearSprs();
	soundSceneReset();
	ml_run();
	NEO_REGISTER8(BIOS_PLAYER1_MODE) = 3;
	NEO_REGISTER8(NGO_START_FLAG) = 0;
	NEO_REGISTER8(BIOS_USER_MODE) = 1;
	ASM_START
	ASM_JMP(SYS_RETURN)
	:
	:
	:
	ASM_END
}

#pragma GCC pop_options
