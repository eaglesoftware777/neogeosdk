/*******
https://eaglesoftware.biz
https://github.com/eaglesoftware777
https://github.com/eaglesoftware777/neogeosdk
******/

#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include <stdint.h>

#pragma GCC push_options
#pragma GCC optimize("O0")

void NEOGEO_USER showEagleIntro(void);
void NEOGEO_USER showWalkDemo(int loops, int delay_ms);
void NEOGEO_USER maingame(void);

/* ZD_ENTRY interrupt subroutine */
NEOGEO_INTERRUPT void NEOGEO_USER ZD_ENTRY(void) {

}

/* CHK_ENTRY interrupt subroutine */
NEOGEO_INTERRUPT void NEOGEO_USER CHK_ENTRY(void) {

}

/* TRAPV_ENTRY interrupt subroutine */
NEOGEO_INTERRUPT void NEOGEO_USER TRAPV_ENTRY(void) {

}


/* VBlank interrupt subroutine */
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

/* IRQ2 interrupt */
NEOGEO_INTERRUPT void  NEOGEO_USER IRQ2(void) {

	ASM_START
	ASM_MVW(#2,REG_IRQACK)				//;IRQ2
	:
	:
	:
	ASM_END
}

/* IRQ3 interrupt */
NEOGEO_INTERRUPT void  NEOGEO_USER IRQ3 (void) {

	ASM_START
	ASM_MVW(#1,REG_IRQACK)
	ASM_MVB(%%d0,REG_DIPSW)
	:
	:
	:
	ASM_END
}

/* INT4 interrupt */
NEOGEO_INTERRUPT void  NEOGEO_USER INT4 (void) {

}
/* INT5 interrupt */
NEOGEO_INTERRUPT void  NEOGEO_USER INT5 (void) {

}

/* INT6 interrupt */
NEOGEO_INTERRUPT void  NEOGEO_USER INT6 (void) {

}

/* INT7 interrupt */
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
// Called from SYSTEM_IO when start button + credit detected.
// Set accepted START_FLAG bits to 1; BIOS auto-deducts credits on return.
// Set USER_MODE=2 when any player's start is accepted (game in progress).
// Do NOT call SYS_CREDIT_DOWN or SYS_RETURN — this is a callback, just return.
void NEOGEO_USER PLAYER_START (void) {

	uint16_t start_flag, country_code = 0;
	uint8_t accepted = 0;
	register short P1 = 0;
	register short P2 = 0;
	register short P3 = 0;
	register short P4 = 0;
	start_flag = NEO_REGISTER8(BIOS_START_FLAG);
	country_code = NEO_REGISTER8(BIOS_COUNTRY_CODE);
	P1 = (start_flag >> 0) & 1;
	P2 = (start_flag >> 1) & 1;
	P3 = (start_flag >> 2) & 1;
	P4 = (start_flag >> 3) & 1;
	if (P1 == 1) {
		soundStopAll();
		playSFX(SOUND_SFX_START_SLASH);
		cyclexms(10);
		start_flag |= 1 << 0;
		NEO_REGISTER8(BIOS_PLAYER1_MODE) = 0x01; // bit0=playing, 1P normal
		accepted = 1;
	}
	if (P2 == 1) {
		soundStopAll();
		playSFX(SOUND_SFX_START_SLASH);
		cyclexms(10);
		start_flag |= 1 << 1;
		if (country_code == 1) {
			// USA: P2 starts alone
			NEO_REGISTER8(BIOS_PLAYER2_MODE) = 0x02;
		} else {
			// Japan/Europe: P2 start implies 2-player game
			NEO_REGISTER8(BIOS_PLAYER2_MODE) = 0x03;
		}
		accepted = 1;
	}
	if (P3 == 1) {
		start_flag |= 1 << 2;
		accepted = 1;
	}
	if (P4 == 1) {
		start_flag |= 1 << 3;
		accepted = 1;
	}
	NEO_REGISTER8(BIOS_START_FLAG) = start_flag;
	if (accepted) {
		NEO_REGISTER8(BIOS_USER_MODE) = 0x02; // game in progress
	}
	// Return normally; BIOS deducts credits for every accepted flag bit.
}

// NeoGeo DEMO_END handler
void NEOGEO_USER DEMO_END (void) {

	//only MVS
	int i =0;
	i++;
	i++;
}

/* NeoGeo COIN_SOUND handler */
void NEOGEO_USER COIN_SOUND (void) {

	isZ80Ready();
	soundStopAll();
	isZ80Ready();
	soundSetADPCMAVolume(0x3C);
	isZ80Ready();
	playSFX(SOUND_SFX_COIN_CHIME);
	cyclexms(12);
	
	int i =0;
	i++;
	i++;
}

// NeoGeo POWER_ON handler
void  NEOGEO_USER POWER_ON (void) {

	//MVS only
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

// NeoGeo MVS EYE_CATCHER handler
void  NEOGEO_USER EYE_CATCHER (void) {

	//AES only ; May be used in MVS :
	// MVS : Eye catcher call within command 2 (attract mode) advised
}

// NeoGeo GAME Mode
void  NEOGEO_USER GAME (void) {

	ASM_START
	ASM_LEA(BIOS_WORKRAM,%%sp)              // A7 (SSP) = 10F300H Init stack pointer
	ASM_MVB(%%d0,REG_DIPSW)           // Kick watchdog
	ASM_MVW(#0x0000,REG_LSPCMODE)    // Pixel timer is disabled (Timer Interrupt: Not allowed)
	ASM_MVW(#7,REG_IRQACK)           // Clear all interrupts ; Other I/O: Undefined
	ASM_ADDQB(#1,BIOS_MESS_BUSY) //mess out disable
	ASM_BCLRB(#7,BIOS_SYSTEM_MODE) //  System mode
	ASM_MVW(#0x2000,%%sr)              // Enable interrupts SR = 2700H supervisor mode
	ASM_MVB(#0x01,BIOS_USER_MODE) // demo mode: USER_MODE=1 (game selection allowed on MVS)
	ASM_SUBQB(#1,BIOS_MESS_BUSY) //mess out enable
	ASM_BSETB(#7,BIOS_SYSTEM_MODE) //  game mode
	ASM_JSR(INIT_GAME)
	ASM_JSR(DEMO_GAME)
	ASM_JMP(SYS_RETURN)
	:
	:
	:
	ASM_END
}

// NeoGeo MVS TITLE Mode
void NEOGEO_USER TITLE(void) {

	// MVS forced-start mode only (USER_REQUEST=3).
	// BIOS drives the SELECT_TIMER; when it expires the BIOS triggers
	// game start on its own — no need to jump to START_GAME here.
	// Return to SYS_RETURN; the system handles the rest.
	ASM_START
	ASM_LEA(BIOS_WORKRAM,%%sp)              // A7 (SSP) = 10F300H Init stack pointer
	ASM_MVB(%%d0,REG_DIPSW)           // Kick watchdog
	ASM_MVW(#0x0000,REG_LSPCMODE)    // Pixel timer is disabled (Timer Interrupt: Not allowed)
	ASM_MVW(#7,REG_IRQACK)           // Clear all interrupts ; Other I/O: Undefined
	ASM_ADDQB(#1,BIOS_MESS_BUSY) //mess out disable
	ASM_BCLRB(#7,BIOS_SYSTEM_MODE) //  system mode
	ASM_MVW(#0x2000,%%sr)              // Enable interrupts SR = 2700H supervisor mode
	ASM_MVB(#0x01,BIOS_USER_MODE) // title mode: USER_MODE=1 (same as demo)
	ASM_SUBQB(#1,BIOS_MESS_BUSY) //mess out enable
	ASM_BSETB(#7,BIOS_SYSTEM_MODE) //  game mode
	ASM_JSR(INIT_GAME)
	ASM_JSR(showTitleMVS)
	ASM_JMP(SYS_RETURN)
	:
	:
	:
	ASM_END
}

// NeoGeo AES EYE_CATCHER handler
void  NEOGEO_USER eye_cactherAES (void) {
	//AES ONLY
	ASM_START
	ASM_JMP(SYS_RETURN)
	:
	:
	:
	ASM_END
}


void  NEOGEO_USER showTitleMVS(void) {
	uint16_t pal_tile0[16];
	int i = 0;

	setpal(pal_tile0,BLACK,BLACK,0xFFF,RED,BLUE,MIDGREEN,CYAN,ORANGE,MAGENTA,RED,WHITE,BLUE,RED,BLUE,CYAN,RED);
	load_palettes(pal_tile0,PALETTES);
	soundPlayTitleMusic(0);
	waitVbl();
	fixtext_out(15,10,"TITLE MODE MVS",0);
	for (i = 0; i < 3; i++) {
		fix_svalue1(13,15,i,0,48);
		cycle1s();
	}
	soundStopAll();
	// Return to TITLE handler which jumps to SYS_RETURN.
	// BIOS controls what happens next (forced start or game select).
}

void  NEOGEO_USER showTitleAES(void) {
	/* AES system call from GAME. */
	uint16_t pal_tile0[16];
	int i = 0;

	setpal(pal_tile0,BLACK,BLACK,0xFFF,RED,BLUE,MIDGREEN,CYAN,ORANGE,MAGENTA,RED,WHITE,BLUE,RED,BLUE,CYAN,RED);
	load_palettes(pal_tile0,PALETTES);
	soundPlayTitleMusic(0);
	waitVbl();
	fixtext_out(15,10,"TITLE MODE AES",0);
	for (i = 0; i < 5; i++) {
		fix_svalue1(13,15,i,0,48);
		cycle1s();
	}
	soundStopAll();
}

/* Clear the user work RAM block before entering the active game flow. */
void NEOGEO_USER WORK_INIT(void) {
	uint32_t *p1 = (uint32_t *)RAMSTART;
	int i = 0;

	for (i = 1; i <= 32768; i++) {
		*p1++ = 0;
	}
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


/* Common display and audio setup shared by title and demo paths. */
void NEOGEO_USER INIT_GAME(void) {
	ASM_START
	ASM_JSR(soundInit)
	ASM_JSR(WORK_INIT)
	ASM_JSR(DISPLAY_INIT)
	:
	:
	:
	ASM_END
}

/* NeoGeo DEMO MODE */
void NEOGEO_USER DEMO_GAME(void) {
	uint16_t pal_tile0[16];
	uint16_t pal_tile1[16];
	uint16_t pal_tile2[16];
	int p1c = 0;
	int i = 0;

	setpal(pal_tile0,BLACK,BLACK,0xFFF,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE);
	load_palettes(pal_tile0,PALETTES);
	setpal(pal_tile1,BLACK,BLACK,RED,RED,RED,RED,RED,RED,RED,RED,RED,RED,RED,RED,RED,RED);
	load_palettes(pal_tile1,PALETTES+PALOFFSET);
	setpal(pal_tile2,BLACK,BLACK,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN);
	load_palettes(pal_tile2,PALETTES+PALOFFSET*2);
	//waitVbl()	;
	//showScreen9();
	//cyclexs(3);
	clearSprs();
	clearFix();
showEagleIntro();
	waitVbl();
	fixtext_out(15,10,"DEMO MODE",0);
	fixtext_out(15,11,"EAGLE SOFTWARE",0);
	fixtext_out(15,12,"HELLO WORLD",0x2);
	fixtext_out(15,13,"NEO GEO SDK 1.2.1",0x2);
	mess_outtest();

	soundPlayTitleMusic(0);

	p1c = read_p1credit();
	display_digit(15,14,777,0,48);
	fixtext_out(15,15,"P1C: ",0);
	display_digit(20,15,p1c,0,48);
	for (i = 0; i < 10; i++) {
		fix_svalue1(27,8,i,0,48);
		p1c = read_p1credit();
		display_digit(15,14,777,0,48);
		fixtext_out(15,15,"P1C: ",0);
		display_digit(20,15,p1c,0,48);
		cycle1s();
	}
	soundStopAll();
}


/* START_GAME handler */
void NEOGEO_USER START_GAME(void) {
	uint16_t pal_tile0[16];
	uint16_t pal_tile1[16];

	setpal(pal_tile0,BLACK,BLACK,0xFFF,BLUE,BLUE,BLUE,BLACK,BLUE,BLUE,BLUE,BLUE,BLUE,BLACK,BLUE,BLACK,BLUE);
	load_palettes(pal_tile0,PALETTES);
	setpal(pal_tile1,BLACK,BLACK,0xFFF,RED,RED,RED,BLACK,RED,RED,RED,RED,RED,RED,RED,BLACK,RED);
	load_palettes(pal_tile1,PALETTES+PALOFFSET*2);
	clearFix();
	clearSprs();
	fixtext_out(15,10,"STARTING GAME",0);
	cyclexs(2);
	fixtext_out(15,10,"LOADING   ...",0);
	cyclexs(2);
	maingame();
	CALLNEOGEOF(GAME);
}



#pragma GCC pop_options
