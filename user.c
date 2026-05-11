/*******
https://eaglesoftware.biz
https://github.com/eaglesoftware777
https://github.com/eaglesoftware777/neogeosdk
******/

#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/neogeo.h"
#pragma GCC push_options
#pragma GCC optimize ("O0")

#define NGO_START_FLAG  0xD00100

void NEOGEO_USER showEagleIntro(void);
void NEOGEO_USER showWalkDemo(int loops, int delay_ms);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER maingame(void);
void NEOGEO_USER GAME_ATTRACT(void);
void NEOGEO_USER START_GAME(void);
void NEOGEO_USER GAME_DISPATCH(void);
void NEOGEO_USER showTitleMVS(void);
void NEOGEO_USER showEyeCatcherMVS(void);
void NEOGEO_USER showCharacterParade(void);
void NEOGEO_USER showPseudo3DLoop(void);
void NEOGEO_USER show3DRaycaster(void);
void NEOGEO_USER showScreen106(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen108(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen109(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);

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
#ifndef NG_AES
	CALLNEOGEOF(SYS_CREDIT_CHECK);
	CALLNEOGEOF(SYS_CREDIT_DOWN);
#endif
}

// NeoGeo DEMO_END handler
void NEOGEO_USER DEMO_END (void) {

	soundStopAll();
}

// NeoGeo COIN_SOUND handler
void NEOGEO_USER COIN_SOUND (void) {

	isZ80Ready();
	soundStopAll();
	isZ80Ready();
	soundSetADPCMAVolume(0x3C);
	isZ80Ready();
	playSFX(SOUND_SFX_COIN_CHIME);
	cyclexms(7);
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

// NeoGeo EYE_CATCHER handler
void  NEOGEO_USER EYE_CATCHER (void) {

#ifdef NG_AES
	showEagleIntro();
#else
	showEyeCatcherMVS();
#endif
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
	ASM_MVW(#0x2000,%%sr)
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

// C-based GAME dispatch — avoids inline-asm address read issues
void NEOGEO_USER GAME_DISPATCH(void) {
#ifndef NG_AES
	if (!NEO_REGISTER8(NGO_START_FLAG)) {
		NEO_REGISTER8(BIOS_USER_MODE) = 1;
		GAME_ATTRACT();
	}
	if (NEO_REGISTER8(NGO_START_FLAG)) {
		NEO_REGISTER8(BIOS_USER_MODE) = 2;
		/* US BIOS skips the TITLE handler and calls GAME directly, so
		   showTitleMVS here guarantees the title screen on all BIOS variants. */
		showTitleMVS();
		START_GAME();
	}
#else
	NEO_REGISTER8(BIOS_USER_MODE) = 1;
	GAME_ATTRACT();
	if (NEO_REGISTER8(NGO_START_FLAG)) {
		NEO_REGISTER8(BIOS_USER_MODE) = 2;
		START_GAME();
	}
#endif
}

// NeoGeo MVS TITLE Mode
void NEOGEO_USER TITLE(void) {

	// MVS systems only
	ASM_START
	ASM_LEA(BIOS_WORKRAM,%%sp)              // A7 (SSP) = 10F300H Init stack pointer
	ASM_MVB(%%d0,REG_DIPSW)           // Kick watchdog
	ASM_MVW(#0x0000,REG_LSPCMODE)    // Pixel timer is disabled (Timer Interrupt: Not allowed)
	ASM_MVW(#7,REG_IRQACK)           // Clear all interrupts ; Other I/O: Undefined
	ASM_ADDQB(#1,BIOS_MESS_BUSY) //mess out disable
	ASM_BCLRB(#7,BIOS_SYSTEM_MODE) //  system mode
	ASM_MVW(#0x2000,%%sr)              // Enable interrupts SR = 2700H supervisor mode
	ASM_MVB(#0x03,BIOS_USER_MODE) //user_request = 2
	ASM_SUBQB(#1,BIOS_MESS_BUSY) //mess out enable
	ASM_BSETB(#7,BIOS_SYSTEM_MODE) //  game mode
	ASM_JSR(INIT_GAME)
	ASM_JSR(showTitleMVS)
	ASM_MVB(#0x02,BIOS_USER_MODE)
	ASM_JMP(START_GAME)
	:
	:
	:
	ASM_END
}

void  NEOGEO_USER showTitleMVS(void) {
	int i;
	clearFix();
	clearSprs();
	setBACKDROP(BLACK);
	showScreen108(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
	waitVbl();
	fixtext_out(14, 26, "HIT START", 0);
	for (i = 0; i < 180; i++) {
		waitVbl();
		if (NEO_REGISTER8(NGO_START_FLAG) || NEO_REGISTER8(BIOS_USER_MODE) == 2)
			break;
	}
	if (!NEO_REGISTER8(NGO_START_FLAG) && NEO_REGISTER8(BIOS_USER_MODE) != 2) {
		clearSprs();
		showScreen109(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
		waitVbl();
		fixtext_out(14, 26, "HIT START", 0);
		while (!NEO_REGISTER8(NGO_START_FLAG) && NEO_REGISTER8(BIOS_USER_MODE) != 2) {
			waitVbl();
		}
	}
}

void  NEOGEO_USER showTitleAES(void) {
	int i = 0;
	NEO_REGISTER8(NGO_START_FLAG) = 0;
	clearFix();
	clearSprs();
	setBACKDROP(BLACK);
	showScreen108(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
	waitVbl();
	fixtext_out(14, 26, "HIT START", 0);
	soundPlayTitleMusic(0);
	for (i = 0; i < 10; i++) {
		if (NEO_REGISTER8(NGO_START_FLAG)) break;
		if (NEO_REGISTER8(BIOS_P1CHANGE) & 0x01) {
			NEO_REGISTER8(NGO_START_FLAG) = 1;
			break;
		}
		cycle1s();
		if (NEO_REGISTER8(NGO_START_FLAG)) break;
	}
	if (!NEO_REGISTER8(NGO_START_FLAG)) {
		clearSprs();
		showScreen109(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
		waitVbl();
		fixtext_out(14, 26, "HIT START", 0);
		for (i = 0; i < 10; i++) {
			if (NEO_REGISTER8(NGO_START_FLAG)) break;
			if (NEO_REGISTER8(BIOS_P1CHANGE) & 0x01) {
				NEO_REGISTER8(NGO_START_FLAG) = 1;
				break;
			}
			cycle1s();
		}
	}
	soundStopAll();
}

//INIT work RAM
void NEOGEO_USER WORK_INIT(void) {
	uint32_t *p1 = (uint32_t *)RAMSTART;
	int i = 0;
	/* Clear 0x100000-0x10EFFF (game area, 60 KB). Must stop before
	   BIOS_WORKRAM at 0x10F300 or the BIOS stack gets corrupted. */
	for (i = 0; i < 15360; i++)
		*p1++ = 0;
}

void NEOGEO_USER DISPLAY_INIT(void) {
	ASM_START
	ASM_MVW(#0x8000,PALETTES)
	ASM_MVW(#0xFFF,PALETTES+8190)
	ASM_JSR(SYS_FIX_CLEAR) // jump to the FIX_CLEAR subroutine
	ASM_JSR(SYS_LSP_1ST) // jump to the LSP_1st subroutine
	ASM_JSR(clearSprs)
	ASM_JSR(clearFix)
	:
	:
	:
	ASM_END
}

/* FIX text palette banks 0-2: white / yellow / cyan on black background. */
void NEOGEO_USER setup_fix_palettes(void) {
	uint16_t fix_pal[16];
	setpal(fix_pal, 0x8000, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	       BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
	load_palettes(fix_pal, PALETTES);
	setpal(fix_pal, 0x8000, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	       BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
	load_palettes(fix_pal, PALETTES + PALOFFSET);
	setpal(fix_pal, 0x8000, CYAN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
	       BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
	load_palettes(fix_pal, PALETTES + PALOFFSET * 2);
}

//INIT GAME MODE
void NEOGEO_USER INIT_GAME(void) {
	ASM_START
	ASM_JSR(soundInit)
	ASM_JSR(WORK_INIT)
	ASM_JSR(DISPLAY_INIT)
	ASM_JSR(setup_fix_palettes)
	:
	:
	:
	ASM_END
}

/* Full SDK showcase attract loop — demonstrates all major engine features. */
void NEOGEO_USER DEMO_GAME(void) {
	clearFix();
	clearSprs();
	soundSceneReset();

	/* 1. Typewriter intro */
	showEagleIntro();
	if (NEO_REGISTER8(NGO_START_FLAG)) return;

	/* 2. Title screens 108 + 109 with FIX text overlay */
	clearFix();
	clearSprs();
	setBACKDROP(BLACK);
	showScreen108(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
	fixtext_out(2,  1, "EAGLE SOFTWARE  2026", 0);
	fixtext_out(2,  2, "NEO GEO SDK", 1);
	fixtext_out(2, 26, "INSERT COIN", 2);
	soundPlayTitleMusic(0);
	cyclexs(4);
	if (NEO_REGISTER8(NGO_START_FLAG)) { soundStopAll(); return; }

	clearSprs();
	showScreen109(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
	fixtext_out(2, 26, "INSERT COIN", 2);
	cyclexs(4);
	soundStopAll();
	if (NEO_REGISTER8(NGO_START_FLAG)) return;

	/* 3. Eye-catcher animation (MVS only) */
#ifndef NG_AES
	showEyeCatcherMVS();
	if (NEO_REGISTER8(NGO_START_FLAG)) return;
#endif

	/* 4. Character parade — sprite sheets walk across screen */
	showCharacterParade();
	if (NEO_REGISTER8(NGO_START_FLAG)) return;

	/* 5. Pseudo-3D floor — Mode-7 hardware scaling trick */
	showPseudo3DLoop();
	if (NEO_REGISTER8(NGO_START_FLAG)) return;

	/* 6. 3D DDA raycaster — software 3D on 68000 */
	show3DRaycaster();
}


void NEOGEO_USER GAME_ATTRACT(void) {
#ifndef NG_AES
	DEMO_GAME();
#else
	showTitleAES();
#endif
}

//START_GAME handler
void NEOGEO_USER START_GAME(void) {
	uint16_t  pal_tile0[16];
	uint16_t  pal_tile1[16];
	setpal(pal_tile0,BLACK,WHITE,0xFFF,BLUE,BLUE,BLUE,BLACK,BLUE,BLUE,BLUE,BLUE,BLUE,BLACK,BLUE,BLACK,BLUE);
	load_palettes(pal_tile0,PALETTES);
	setpal(pal_tile1,BLACK,CYAN,0xFFF,RED,RED,RED,BLACK,RED,RED,RED,RED,RED,RED,RED,BLACK,RED);
	load_palettes(pal_tile1,PALETTES+PALOFFSET*2);
	clearFix();
	clearSprs();
	fixtext_out(15,10,"STARTING GAME",0);
	cyclexs(2);
	fixtext_out(15,10,"LOADING   ...",0);
	cyclexs(2);
	maingame();
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
