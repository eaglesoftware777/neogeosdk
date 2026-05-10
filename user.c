/*******
https://eaglesoftware.biz
https://github.com/eaglesoftware777
https://github.com/eaglesoftware777/neogeosdk
******/

#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include "sdk/2d_engine/ng_engine.h"
#include <stdint.h>

#pragma GCC push_options
#pragma GCC optimize("O0")

extern void NEOGEO_USER maingame(void);
extern void NEOGEO_USER showEagleIntro(void);

void NEOGEO_USER POWER_ON(void);
void NEOGEO_USER EYE_CATCHER(void);
void NEOGEO_USER GAME(void);
void NEOGEO_USER TITLE(void);

/* ZD_ENTRY interrupt subroutine */
NEOGEO_INTERRUPT void NEOGEO_USER ZD_ENTRY(void) {}

/* CHK_ENTRY interrupt subroutine */
NEOGEO_INTERRUPT void NEOGEO_USER CHK_ENTRY(void) {}

/* TRAPV_ENTRY interrupt subroutine */
NEOGEO_INTERRUPT void NEOGEO_USER TRAPV_ENTRY(void) {}

/* VBlank interrupt subroutine */
NEOGEO_INTERRUPT void NEOGEO_USER VBlank(void) {
    asm volatile (
        "move.w #1, 0x100000\n\t"
        "btst #7, 0x10FD80\n\t"
        "bne .getvbl\n\t"
        "jmp 0xC00438\n"
        ".getvbl:\n\t"
        "movem.l %%d0-%%d7/%%a0-%%a6,-(%%sp)\n\t"
        "move.w #4, 0x3C000C\n\t"
        "move.b %%d0, 0x300001\n\t"
        "jsr 0xC0044A\n\t"
        "movem.l (%%sp)+, %%d0-%%d7/%%a0-%%a6"
        : : : "memory"
    );
}

/* IRQ2 interrupt */
NEOGEO_INTERRUPT void NEOGEO_USER IRQ2(void) {
    asm volatile ("move.w #2, 0x3C000C" : : : "memory");
}

/* IRQ3 interrupt */
NEOGEO_INTERRUPT void NEOGEO_USER IRQ3 (void) {
    asm volatile ("move.w #1, 0x3C000C\n\tmove.b %%d0, 0x300001" : : : "memory");
}

NEOGEO_INTERRUPT void NEOGEO_USER INT4 (void) {}
NEOGEO_INTERRUPT void NEOGEO_USER INT5 (void) {}
NEOGEO_INTERRUPT void NEOGEO_USER INT6 (void) {}
NEOGEO_INTERRUPT void NEOGEO_USER INT7 (void) {}

/* NeoGeo USER Mode Entry Point */
void NEOGEO_USER USER(void) {
    asm volatile (
        "lea .rv(%%pc), %%a0\n\t"
        "moveq #0, %%d0\n\t"
        "move.b 0x10FDAE, %%d0\n\t"
        "lsl.b #2, %%d0\n\t"
        "movea.l (%%a0, %%d0), %%a0\n\t"
        "jsr (%%a0)\n\t"
        "jmp 0xC00444\n"
        ".rv:\n\t"
        ".dc.l POWER_ON\n\t"
        ".dc.l EYE_CATCHER\n\t"
        ".dc.l GAME\n\t"
        ".dc.l TITLE"
        : : : "d0", "a0", "memory"
    );
}

void NEOGEO_USER PLAYER_START(void) {
    uint16_t start_flag = NEO_REGISTER8(0x10FDB4);
    CALLNEOGEOF(0xC00450); /* SYS_CREDIT_CHECK */
    if (start_flag & 1) {
        soundStopAll();
        playSFX(SOUND_SFX_START_SLASH);
        cyclexms(10);
    }
    CALLNEOGEOF(0xC00450);
    CALLNEOGEOF(0xC00456); /* SYS_CREDIT_DOWN */
    CALLNEOGEOF(0xC00444); /* SYS_RETURN */
}

void NEOGEO_USER DEMO_END(void) {}

void NEOGEO_USER COIN_SOUND(void) {
    isZ80Ready();
    soundStopAll();
    soundSetADPCMAVolume(0x3C);
    playSFX(SOUND_SFX_COIN_CHIME);
    cyclexms(12);
}

void NEOGEO_USER POWER_ON(void) {
    NEO_REGISTER8(0x10FDAF) = 0x00; /* BIOS_USER_MODE */
    uint32_t *p = (uint32_t *)(0x100000 + 64);
    for (int i = 0; i < 128; i++) *p++ = 0;
    ng_game_engine_init();
    ng_engine_init_hardware(0x0020);
    asm volatile ("move.b %%d0, 0x300001\n\tjmp 0xC00444" : : : "memory");
}

void NEOGEO_USER EYE_CATCHER(void) {
    /* Optional AES Eye Catcher */
}

void NEOGEO_USER GAME(void) {
    asm volatile (
        "movea.l #0x10F300, %%sp\n\t"
        "move.b %%d0, 0x300001\n\t"
        "move.w #0, 0x3C0006\n\t"
        "move.w #7, 0x3C000C\n\t"
        "addq.b #1, 0x10FDC2\n\t"
        "bclr #7, 0x10FD80\n\t"
        "move.w #0x2000, %%sr\n\t"
        "move.b #2, 0x10FDAF\n\t"
        "subq.b #1, 0x10FDC2\n\t"
        "bset #7, 0x10FD80\n"
        : : : "memory"
    );
    /* Internal Game Setup */
    ng_fix_init();
    ng_fix_clear();
    /* MVS Masking */
    ng_fix_clear_rect(0, 0, 1, 28, 0); 
    ng_fix_clear_rect(39, 0, 1, 28, 0);
    
    showEagleIntro();
    maingame();
    asm volatile ("jmp 0xC00444");
}

void NEOGEO_USER TITLE(void) {
    NEO_REGISTER8(0x10FDAF) = 0x03;
    GAME();
}

#pragma GCC pop_options
