/*******
https://eaglesoftware.biz
https://github.com/eaglesoftware777
https://github.com/eaglesoftware777/neogeosdk
******/

#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/bsp/bsp.h"
#pragma GCC push_options
#pragma GCC optimize ("O0")

#define NGO_START_FLAG  0xD00100

/* Supplied by games/maiya/main.c.  NEOGEO_USER puts them in the section
 * the ROM link keeps; in plain .text they would be stripped before linking. */
void NEOGEO_USER game_boot(void);
void NEOGEO_USER game_frame(void);
void NEOGEO_USER maiya_title(void);
uint8_t NEOGEO_USER maiya_session_over(void);
void NEOGEO_USER GAME_ATTRACT(void);
void NEOGEO_USER TITLE_WAIT(void);
void NEOGEO_USER START_GAME(void);

NEOGEO_INTERRUPT void NEOGEO_USER ZD_ENTRY(void) {}
NEOGEO_INTERRUPT void NEOGEO_USER CHK_ENTRY(void) {}
NEOGEO_INTERRUPT void NEOGEO_USER TRAPV_ENTRY(void) {}

NEOGEO_INTERRUPT void NEOGEO_USER VBlank(void) {
    ASM_START
    ASM_L(VBLANK)
    ASM_MVW(#1, USER_WORKRAM)
    ASM_BTST(#7,BIOS_SYSTEM_MODE)
    ASM_BNE(.getvbl)
    ASM_JMP(SYS_INT1)
    ASM_L(.getvbl)
    ASM_MVEML(%%d0-%%d7/%%a0-%%a6,-(%%sp))
    ASM_MVW(#4,REG_IRQACK)
    ASM_MVB(%%d0,REG_DIPSW)
    ASM_JSR(SYS_IO)
    ASM_MVEML((%%sp)+, %%d0-%%d7/%%a0-%%a6)
    ::: ASM_END
}

NEOGEO_INTERRUPT void NEOGEO_USER IRQ2(void) {
    ASM_START ASM_MVW(#2,REG_IRQACK) ::: ASM_END
}

NEOGEO_INTERRUPT void NEOGEO_USER IRQ3(void) {
    ASM_START ASM_MVW(#1,REG_IRQACK) ASM_MVB(%%d0,REG_DIPSW) ::: ASM_END
}

NEOGEO_INTERRUPT void NEOGEO_USER INT4(void) {}
NEOGEO_INTERRUPT void NEOGEO_USER INT5(void) {}
NEOGEO_INTERRUPT void NEOGEO_USER INT6(void) {}
NEOGEO_INTERRUPT void NEOGEO_USER INT7(void) {}

void NEOGEO_USER USER(void) {
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
    ::: ASM_END
}

void NEOGEO_USER PLAYER_START(void) {
    uint16_t start_flag = NEO_REGISTER8(BIOS_START_FLAG);
    uint16_t country_code = NEO_REGISTER8(BIOS_COUNTRY_CODE);
    NEO_REGISTER8(BIOS_USER_MODE) = 2;
    NEO_REGISTER8(NGO_START_FLAG) = 1;
    NEO_REGISTER8(BIOS_PLAYER1_MODE) = 1;
    NEO_REGISTER8(BIOS_START_FLAG) = start_flag | 1;
    if ((start_flag >> 1) & 1)
        NEO_REGISTER8(BIOS_PLAYER2_MODE) = (country_code == 1) ? 2 : 3;
    soundSetADPCMAVolume(0x3C);
    playSFX(SOUND_SFX_3);
#ifndef NG_AES
    CALLNEOGEOF(SYS_CREDIT_CHECK);
    CALLNEOGEOF(SYS_CREDIT_DOWN);
#endif
}

void NEOGEO_USER DEMO_END(void)   { soundStopAll(); }
void NEOGEO_USER COIN_SOUND(void) {
    isZ80Ready();
    soundSetADPCMAVolume(0x3C);
    isZ80Ready();
    playSFX(SOUND_SFX_1);
}

void NEOGEO_USER POWER_ON(void) {
    NEO_REGISTER8(NGO_START_FLAG) = 0;
    ASM_START
    ASM_MVB(#0x00,BIOS_USER_MODE)
    ASM_LEA(USER_WORKRAM+64,%%a0)
    ASM_MV(#0x001F,%%d1)
    ASM_MVQ(#0,%%d0)
    ASM_L(.cl) ASM_MVL(%%d0,(%%a0)+) ASM_MVL(%%d0,(%%a0)+)
    ASM_MVL(%%d0,(%%a0)+) ASM_MVL(%%d0,(%%a0)+)
    ASM_DBF(%%d1,.cl)
    ASM_MVB(%%d0,REG_DIPSW)
    ASM_JMP(SYS_RETURN)
    ::: ASM_END
}

void NEOGEO_USER EYE_CATCHER(void) {
    soundStopAll();
    clearFix();
    clearSprs();
    setBACKDROP(BLACK);
    fixtext_out(14, 13, "MAIYA", 0);
    int i;
    for (i = 0; i < 180; i++) waitVbl();
    soundStopAll();
}

void NEOGEO_USER GAME(void) {
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
    ::: ASM_END
}

void NEOGEO_USER TITLE(void) {
    ASM_START
    ASM_LEA(BIOS_WORKRAM,%%sp)
    ASM_MVB(%%d0,REG_DIPSW)
    ASM_MVW(#0x0000,REG_LSPCMODE)
    ASM_MVW(#7,REG_IRQACK)
    ASM_ADDQB(#1,BIOS_MESS_BUSY)
    ASM_BCLRB(#7,BIOS_SYSTEM_MODE)
    ASM_MVW(#0x2000,%%sr)
    ASM_MVB(#0x03,BIOS_USER_MODE)
    ASM_SUBQB(#1,BIOS_MESS_BUSY)
    ASM_BSETB(#7,BIOS_SYSTEM_MODE)
    ASM_JSR(INIT_GAME)
    ASM_JSR(TITLE_WAIT)
    ASM_JSR(START_GAME)
    ASM_JMP(SYS_RETURN)
    ::: ASM_END
}

void NEOGEO_USER WORK_INIT(void) {
    uint32_t *p = (uint32_t *)RAMSTART;
    int i;
    for (i = 0; i < 15360; i++) *p++ = 0;
}

void NEOGEO_USER DISPLAY_INIT(void) {
    ASM_START
    ASM_MVW(#0x8000,PALETTES)
    ASM_MVW(#0x8000,PALETTES+8190)   /* backdrop = black */
    ASM_JSR(SYS_FIX_CLEAR)
    ASM_JSR(SYS_LSP_1ST)
    ASM_JSR(clearSprs)
    ASM_JSR(clearFix)
    ::: ASM_END
}

void NEOGEO_USER setup_fix_palettes(void) {
    uint16_t p[16];
    uint8_t i;

    /* Bank 0: White (PAL_TEXT) */
    p[0] = 0x8000;
    for (i = 1; i < 16; i++) p[i] = 0x7FFF;
    load_palettes(p, PALETTES);

    /* Bank 1: Gold / Yellow (PAL_GOLD) */
    p[0] = 0x8000;
    for (i = 1; i < 16; i++) p[i] = 0x6FE0;
    load_palettes(p, PALETTES + PALOFFSET);

    /* Bank 2: Red / Warn (PAL_WARN) */
    p[0] = 0x8000;
    for (i = 1; i < 16; i++) p[i] = 0x4F44;
    load_palettes(p, PALETTES + PALOFFSET * 2);

    /* Bank 3: Sky / Cyan (PAL_SKY) */
    p[0] = 0x8000;
    for (i = 1; i < 16; i++) p[i] = 0x39FF;
    load_palettes(p, PALETTES + PALOFFSET * 3);
}

void NEOGEO_USER INIT_GAME(void) {
    ASM_START
    ASM_JSR(soundInit)
    ASM_JSR(WORK_INIT)
    ASM_JSR(DISPLAY_INIT)
    ASM_JSR(setup_fix_palettes)
    ::: ASM_END
}

void NEOGEO_USER GAME_DISPATCH(void) {
#ifndef NG_AES
    if (!NEO_REGISTER8(NGO_START_FLAG)) {
        NEO_REGISTER8(BIOS_USER_MODE) = 1;
        GAME_ATTRACT();
    }
    if (NEO_REGISTER8(NGO_START_FLAG)) {
        NEO_REGISTER8(BIOS_USER_MODE) = 2;
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

void NEOGEO_USER DEMO_GAME(void)    { GAME_ATTRACT(); }

/*
 * Attract loop.
 *
 * The title screen with INSERT COIN blinking.  A credit or a start ends it
 * and hands control back to the BIOS, which then asks for TITLE: the
 * "hit start" wait below.  On MVS the BIOS calls PLAYER_START itself when
 * Start is pressed with a credit in, so the button is not polled here.
 */
void NEOGEO_USER GAME_ATTRACT(void) {
    int i;
    clearFix(); clearSprs(); setBACKDROP(BLACK);
    maiya_title();
    for (i = 0; ; i++) {
        if (NEO_REGISTER8(NGO_START_FLAG)) break;
#ifndef NG_AES
        if (read_p1credit() > 0) break;
#else
        fixtext_out(14, 25, "PUSH START", 1);
        if (NEO_REGISTER8(BIOS_P1CHANGE) & 0x01) {
            NEO_REGISTER8(BIOS_USER_MODE) = 2;
            NEO_REGISTER8(NGO_START_FLAG) = 1;
            break;
        }
#endif
        waitVbl();
    }
}

/* The credited title: PUSH 1P START, or an automatic start after fifteen seconds. */
void NEOGEO_USER TITLE_WAIT(void) {
    int i;
    int auto_frames = 15 * 60;
    clearFix(); clearSprs(); setBACKDROP(BLACK);
    maiya_title();
    for (i = 0; ; i++) {
        if (NEO_REGISTER8(NGO_START_FLAG) || NEO_REGISTER8(BIOS_USER_MODE) == 2) break;
#ifndef NG_AES
        if (read_p1credit() > 0) {
            char timer[10];
            int secs = (auto_frames + 59) / 60;
            timer[0] = 'T'; timer[1] = 'I'; timer[2] = 'M'; timer[3] = 'E';
            timer[4] = ' '; timer[5] = ' ';
            timer[6] = (char)('0' + secs / 10);
            timer[7] = (char)('0' + secs % 10);
            timer[8] = '\0';
            fixtext_out(16, 27, timer, 3);
            if ((i >> 4) & 1) fixtext_out(13, 25, "PUSH 1P START", 1);
            else fixtext_out(13, 25, "             ", 1);
            if (auto_frames > 0) auto_frames--;
            else {
                NEO_REGISTER8(NGO_START_FLAG) = 1;
                NEO_REGISTER8(BIOS_USER_MODE) = 2;
                break;
            }
        } else {
            fixtext_out(16, 27, "        ", 0);
            fixtext_out(13, 25, "             ", 0);
        }
#else
        break;
#endif
        waitVbl();
    }
}

/* One frame boundary per loop; Start belongs to the game's pause state.
 * When the session ends the loop returns and the BIOS restarts the attract. */
void NEOGEO_USER START_GAME(void) {
    NEO_REGISTER8(BIOS_USER_MODE) = 2;
    clearFix();
    clearSprs();
    game_boot();
    for (;;) {
        waitVbl();
        game_frame();
        if (maiya_session_over()) break;
    }
    soundStopAll();
    NEO_REGISTER8(NGO_START_FLAG) = 0;
    NEO_REGISTER8(BIOS_USER_MODE) = 1;
}
#pragma GCC pop_options
