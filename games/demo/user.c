/*******
https://eaglesoftware.biz
https://github.com/eaglesoftware777
https://github.com/eaglesoftware777/neogeosdk
******/

#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "games/demo/scenes/demo.h"
#include "games/demo/scenes/demo_sound.h"
#include "games/demo/scenes/demo_fix.h"
#include "infix_palettes.h"
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
void NEOGEO_USER showSoundDemo(void);
void NEOGEO_USER show3DRaycaster(void);
void NEOGEO_USER showScreen106(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen107(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
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
	soundSetADPCMAVolume(0x3C);
	playSFX(SOUND_SFX_3);
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
	playSFX(SOUND_SFX_1);
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

// NeoGeo EYE_CATCHER handler — same sprite animation for both AES and MVS
void  NEOGEO_USER EYE_CATCHER (void) {

	soundCancelFade();
	soundSceneReset();
	soundSetADPCMAVolume(0x00);
	soundSetADPCMBVolume(0xBC);
	playSFXB(SOUND_TRACK_E);
	showEyeCatcherMVS();
	soundStopAll();
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
	int credit_seen = 0;
	int auto_frames = 15 * 60;
	clearFix();
	clearSprs();
	setBACKDROP(BLACK);
	showScreen108(32, 24, 0xF, 0xAF, 16, 0x0000, DEMO_SHOWSCREEN_BASE);
	waitVbl();

	/* MVS: wait for credit, then auto-start after 15 seconds if START is not pressed. */
	for (i = 0; ; i++) {
		if (read_p1credit() > 0) {
			char timer[18];
			int secs;
			if (!credit_seen) {
				credit_seen = 1;
				auto_frames = 15 * 60;
				soundSetADPCMAVolume(0x3C);
				playSFX(SOUND_SFX_1);
			}
			secs = (auto_frames + 59) / 60;
			timer[0] = 'A'; timer[1] = 'U'; timer[2] = 'T'; timer[3] = 'O';
			timer[4] = ' '; timer[5] = 'S'; timer[6] = 'T'; timer[7] = 'A';
			timer[8] = 'R'; timer[9] = 'T'; timer[10] = ' ';
			timer[11] = (char)('0' + (secs / 10));
			timer[12] = (char)('0' + (secs % 10));
			timer[13] = 's'; timer[14] = ' '; timer[15] = ' '; timer[16] = '\0';
			/* Red (bank 3) instead of green, centred on the 40-cell
			 * row and dropped one row down the screen. */
			mess_out(13, 26, timer, 3);
			mess_out(15, 27, "HIT START", 3);
			if (auto_frames > 0) auto_frames--;
			else {
				NEO_REGISTER8(NGO_START_FLAG) = 1;
				NEO_REGISTER8(BIOS_USER_MODE) = 2;
				playSFX(SOUND_SFX_3);
				break;
			}
		} else if ((i >> 4) & 1) {
			mess_out(10, 25, "                ", 0);
			mess_out(10, 26, " INSERT COIN  ", 0);
		} else {
			mess_out(10, 25, "                ", 0);
			mess_out(10, 26, "              ", 0);
		}
		waitVbl();
		if (NEO_REGISTER8(NGO_START_FLAG) || NEO_REGISTER8(BIOS_USER_MODE) == 2)
			break;
	}
}

void  NEOGEO_USER showTitleAES(void) {
	int i = 0;
	NEO_REGISTER8(NGO_START_FLAG) = 0;
	clearFix();
	clearSprs();
	setBACKDROP(BLACK);
	showScreen108(32, 24, 0xF, 0xAF, 16, 0x0000, DEMO_SHOWSCREEN_BASE);
	waitVbl();
	mess_out(15, 27, "HIT START", 3);
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
		showScreen107(32, 24, 0xF, 0xAF, 16, 0x0000, DEMO_SHOWSCREEN_BASE);
		waitVbl();
		mess_out(15, 27, "HIT START", 3);
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
	ASM_MVW(#0x7FFF,PALETTES)
	ASM_MVW(#0x7FFF,PALETTES+8190)   /* backdrop = black */
	ASM_JSR(SYS_FIX_CLEAR) // jump to the FIX_CLEAR subroutine
	ASM_JSR(SYS_LSP_1ST) // jump to the LSP_1st subroutine
	ASM_JSR(clearSprs)
	ASM_JSR(clearFix)
	:
	:
	:
	ASM_END
}

/* FIX text palette banks 0-3, all dark inks so they read on DEMO_BG. */
void NEOGEO_USER setup_fix_palettes(void) {
	uint16_t fix_pal[16];
	uint8_t  i, j;

	setpal(fix_pal, DEMO_FIX_PAL(DEMO_INK_BODY));
	load_palettes(fix_pal, PALETTES);
	setpal(fix_pal, DEMO_FIX_PAL(DEMO_INK_SUB));
	load_palettes(fix_pal, PALETTES + PALOFFSET);
	setpal(fix_pal, DEMO_FIX_PAL(DEMO_INK_ACCENT));
	load_palettes(fix_pal, PALETTES + PALOFFSET * 2);
	/* Bank 3 = the title/insert-coin prompts.  Bank 4 up is the infix
	 * image range (INFIX_PAL_BANK_BASE), so 3 is free. */
	setpal(fix_pal, DEMO_FIX_PAL(DEMO_INK_PROMPT));
	load_palettes(fix_pal, PALETTES + PALOFFSET * 3);

	/* Banks 4..(4+N-1): per-image FIX palettes for infix photos.
	 * Each infix PNG was authored as 4bpp indexed; its original RGB
	 * palette has been packed to NeoGeo words by artbox/fixtiles.py
	 * and emitted as INFIX_PALETTES[][] in infix_palettes.h. */
	for (i = 0u; i < INFIX_IMAGE_COUNT; i++) {
		for (j = 0u; j < 16u; j++) {
			fix_pal[j] = INFIX_PALETTES[i][j];
		}
		load_palettes(fix_pal,
		              PALETTES + PALOFFSET * INFIX_IMAGES[i].pal_bank);
	}
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

/* Full SDK showcase attract loop — implemented in games/demo/scenes/. */
void NEOGEO_USER DEMO_GAME(void) {
	demo_run_attract();
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
	setBACKDROP(BLACK);
	waitVbl();
	clearFix();
	clearSprs();
	soundSceneReset();
	demo_run_full_flow();
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
