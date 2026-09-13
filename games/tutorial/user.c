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
#ifndef NG_AES
    CALLNEOGEOF(SYS_CREDIT_CHECK);
    CALLNEOGEOF(SYS_CREDIT_DOWN);
#endif
}

void NEOGEO_USER DEMO_END(void)   { soundStopAll(); }
void NEOGEO_USER COIN_SOUND(void) {
    isZ80Ready(); soundStopAll();
    isZ80Ready(); soundSetADPCMAVolume(0x3C);
    isZ80Ready(); playSFX(SOUND_SFX_1);
    cyclexms(7);
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
    fixtext_out(14, 13, "HELLO WORLD", 0);
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
    setpal(p, 0x8000, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(p, PALETTES);
    setpal(p, 0x8000, CYAN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(p, PALETTES + PALOFFSET);
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

void NEOGEO_USER GAME_ATTRACT(void) {
    int i;
    clearFix(); clearSprs(); setBACKDROP(BLACK);
    fixtext_out(11, 13, "HELLO WORLD", 0);
    fixtext_out(8,  15, "INSERT COIN", 0);
    for (i = 0; ; i++) {
        if (NEO_REGISTER8(NGO_START_FLAG)) break;
        waitVbl();
    }
}

void NEOGEO_USER START_GAME(void) {
    int i;
    clearFix(); clearSprs(); setBACKDROP(BLACK);
    fixtext_out(9, 13, "PRESS START", 0);
    for (i = 0; i < 300; i++) waitVbl();
    NEO_REGISTER8(NGO_START_FLAG) = 0;
    NEO_REGISTER8(BIOS_USER_MODE) = 1;
    ASM_START ASM_JMP(SYS_RETURN) ::: ASM_END
}

#pragma GCC pop_options
