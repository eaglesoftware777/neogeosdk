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
#include "games/maiya/scenes/maiya_game.h"
#pragma GCC push_options
#pragma GCC optimize ("O0")

/*
 * "Start has been pressed": set by PLAYER_START (or the console's own
 * Start check), read by the attract and title loops of the same USER
 * call. It lives in work RAM on both boards. It used to live in backup RAM
 * at D00100 on an arcade board -- the system ROM's own bookkeeping, and
 * write-protected while the game runs on some system ROMs (the US one),
 * where the write was simply lost and Start never started a game.
 */
#define MAIYA_START_LATCH maiya_console_start

/* Supplied by games/maiya/main.c.  NEOGEO_USER puts them in the section
 * the ROM link keeps; in plain .text they would be stripped before linking. */
void NEOGEO_USER game_boot(void);
void NEOGEO_USER game_frame(void);
void NEOGEO_USER maiya_title(void);
void NEOGEO_USER maiya_vblank(void);
void NEOGEO_USER maiya_eyecatcher(void);
uint8_t NEOGEO_USER maiya_session_over(void);
void NEOGEO_USER maiya_demo_begin(void);
void NEOGEO_USER maiya_demo_end(void);
uint8_t NEOGEO_USER maiya_demo_spent(void);
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
    /* Only player 1 plays, and only once per credit: a Start while she is
     * already in the game (mode 1) is declined -- the system asks on every
     * press and leaves the answer to the game, and saying yes again spent a
     * second credit. The continue offer (2) and game over (3) still take
     * one. */
    if (!(start_flag & 1u) || NEO_REGISTER8(BIOS_PLAYER1_MODE) == 1) {
        NEO_REGISTER8(BIOS_START_FLAG) = 0;
        return;
    }
    NEO_REGISTER8(BIOS_USER_MODE) = 2;
    MAIYA_START_LATCH = 1;
    NEO_REGISTER8(BIOS_PLAYER1_MODE) = 1;
    NEO_REGISTER8(BIOS_START_FLAG) = start_flag | 1;
    if ((start_flag >> 1) & 1)
        NEO_REGISTER8(BIOS_PLAYER2_MODE) = (country_code == 1) ? 2 : 3;
    soundSetADPCMAVolume(0x3C);
    playSFX(SOUND_SFX_3);
    if (ng_sys_is_mvs()) {   /* an arcade board: the start was paid for */
        CALLNEOGEOF(SYS_CREDIT_CHECK);
        CALLNEOGEOF(SYS_CREDIT_DOWN);
    }
}

void NEOGEO_USER DEMO_END(void)   { soundStopAll(); }
void NEOGEO_USER COIN_SOUND(void) {
    isZ80Ready();
    soundSetADPCMAVolume(0x3C);
    isZ80Ready();
    playSFX(SOUND_SFX_1);
}

void NEOGEO_USER POWER_ON(void) {
    MAIYA_START_LATCH = 0;
    ASM_START
    ASM_MVB(#0x00,BIOS_USER_MODE)
    ASM_LEA(USER_WORKRAM+64,%%a0)
    ASM_MV(#0x001F,%%d1)
    ASM_MVQ(#0,%%d0)
    ASM_L(.cl) ASM_MVL(%%d0,(%%a0)+) ASM_MVL(%%d0,(%%a0)+)
    ASM_MVL(%%d0,(%%a0)+) ASM_MVL(%%d0,(%%a0)+)
    ASM_DBF(%%d1,.cl)
    ASM_MVB(%%d0,REG_DIPSW)
    ASM_JSR(maiya_save_reset)       /* first power-on: the score table's defaults */
    ASM_JMP(SYS_RETURN)
    ::: ASM_END
}

/*
 * The BIOS eyecatcher slot.  The FIX layer is not on screen in this phase,
 * so the house logo is drawn at the head of the attract loop instead, where
 * the game owns the display; this just leaves a clean black screen.
 */
void NEOGEO_USER EYE_CATCHER(void) {
    int i;
    soundStopAll();
    clearFix();
    clearSprs();
    setBACKDROP(BLACK);
    for (i = 0; i < 30; i++) waitVbl();
}

void NEOGEO_USER GAME(void) {
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
    ASM_MVW(#0x2700,%%sr)
    ASM_MVB(#0x01,BIOS_USER_MODE)   /* title: 1, the same as the demo */
    ASM_SUBQB(#1,BIOS_MESS_BUSY)
    ASM_BSETB(#7,BIOS_SYSTEM_MODE)
    ASM_JSR(INIT_GAME)
    ASM_JSR(TITLE_WAIT)
    ASM_JSR(START_GAME)
    ASM_JMP(SYS_RETURN)
    ::: ASM_END
}

/* Clears the game's work RAM -- all of it but the save block the header
 * names, which holds what the system keeps for the game between sessions. */
void NEOGEO_USER WORK_INIT(void) {
    uint32_t keep_lo = NG_CART_SAVE_START();
    uint32_t keep_hi = keep_lo + NG_CART_SAVE_SIZE();
    uint32_t *p = (uint32_t *)RAMSTART;
    int i;
    for (i = 0; i < 15360; i++, p++) {
        uint32_t at = (uint32_t)(uintptr_t)p;
        if ((i & 255) == 0) kickWatchDog();
        if (at + 4u > keep_lo && at < keep_hi) continue;
        *p = 0;
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
    /* No input callback may write into RAM while it is being cleared. */
    ASM_MVW(#0x2700,%%sr)
    ASM_JSR(WORK_INIT)
    ASM_JSR(maiya_save_check)
    ASM_JSR(soundInit)
    ASM_JSR(DISPLAY_INIT)
    ASM_JSR(setup_fix_palettes)
    ASM_MVB(#1,BIOS_USER_MODE)
    ASM_MVW(#0x2000,%%sr)
    ::: ASM_END
}

void NEOGEO_USER GAME_DISPATCH(void) {
    /* INIT_GAME cleared the latch before enabling input. Do not clear it
     * again here: a VBlank may already have accepted a paid Start. */
    if (!MAIYA_START_LATCH) {
        GAME_ATTRACT();
    }
    if (MAIYA_START_LATCH || (ng_sys_is_mvs() && read_p1credit() > 0)) {
        TITLE_WAIT();
        START_GAME();
    }
}

void NEOGEO_USER DEMO_GAME(void)    { GAME_ATTRACT(); }

/* A credit, or Start on a console, ends whatever the cabinet is showing. */
static int NEOGEO_USER attract_interrupted(void) {
    if (MAIYA_START_LATCH) return 1;
    if (ng_sys_is_mvs()) return read_p1credit() > 0;
    if (NEO_REGISTER8(BIOS_STATCHANGE) & 0x01) {
        NEO_REGISTER8(BIOS_USER_MODE) = 2;
        MAIYA_START_LATCH = 1;
        return 1;
    }
    return 0;
}

uint8_t NEOGEO_USER maiya_start_pending(void) {
    return (uint8_t)attract_interrupted();
}

/*
 * Attract loop.
 *
 * The cabinet alternates between the game playing itself -- a different
 * valley each time round -- and the title screen with INSERT COIN.  A credit
 * or a start ends it and hands control back to the BIOS, which then asks for
 * TITLE: the "hit start" wait below.  On MVS the BIOS calls PLAYER_START
 * itself when Start is pressed with a credit in, so the button is not polled
 * here.
 */
void NEOGEO_USER GAME_ATTRACT(void) {
    int i;
    int round;

    for (round = 0; ; round++) {
        /* --- the house logo, at the head of the attract -------------- */
        /* Not when a credit or a start is already waiting: this would show
         * the logo, see it and hand straight back to the system, which
         * sends the demo round again -- the logo looping on its own. */
        if (round == 0 && attract_interrupted()) return;
        if (round == 0) {
            maiya_eyecatcher();
            if (attract_interrupted()) return;
        }

        /* --- the title card comes first ------------------------------- */
        clearFix(); clearSprs(); setBACKDROP(BLACK);
        maiya_title();
        if (!maiya_dip_demo_sound()) { isZ80Ready(); soundApplyMix(0x00, 0x00, 0x00, 0x00); }
        for (i = 0; i < 60 * 12; i++) {
            if (!ng_sys_is_mvs()) fixtext_out(14, 25, "PUSH START", 1);
            if (attract_interrupted()) return;
            maiya_vblank();
        }

        /* --- then a slice of the game, played by the machine ---------- */
        clearFix(); clearSprs(); setBACKDROP(BLACK);
        maiya_demo_begin();
        for (i = 0; i < 60 * 22; i++) {
            maiya_vblank();
            game_frame();
            if (attract_interrupted()) { maiya_demo_end(); return; }
            if (maiya_demo_spent()) break;
        }
        maiya_demo_end();
    }
}

/* The credited title: PUSH 1P START, or an automatic start after fifteen seconds. */
void NEOGEO_USER TITLE_WAIT(void) {
    int i;
    int auto_frames = 15 * 60;
    clearFix(); clearSprs(); setBACKDROP(BLACK);
    maiya_title();
    for (i = 0; ; i++) {
        if (MAIYA_START_LATCH || NEO_REGISTER8(BIOS_USER_MODE) == 2) break;
        if (!ng_sys_is_mvs()) {
            /* a console: no credits and no countdown, Start alone */
            fixtext_out(14, 25, "PUSH START", 1);
            if (attract_interrupted()) break;
        } else if (read_p1credit() > 0) {
            char timer[10];
            int secs = (auto_frames + 59) / 60;
            timer[0] = 'T'; timer[1] = 'I'; timer[2] = 'M'; timer[3] = 'E';
            timer[4] = ' '; timer[5] = ' ';
            timer[6] = (char)('0' + secs / 10);
            timer[7] = (char)('0' + secs % 10);
            timer[8] = '\0';
            fixtext_out(30, 3, timer, 3);   /* top right, clear of the painted copyright line */
            if ((i >> 4) & 1) fixtext_out(13, 25, "PUSH 1P START", 1);
            else fixtext_out(13, 25, "             ", 1);
            if (auto_frames > 0) auto_frames--;
            else {
                uint16_t saved_sr;
                /* Debit once, atomically with respect to PLAYER_START. */
                __asm__ volatile ("move.w %%sr,%0\n\tmove.w #0x2700,%%sr"
                                  : "=d" (saved_sr) : : "memory");
                if (!MAIYA_START_LATCH) {
                    NEO_REGISTER8(0x10FDB0) = 1;
                    NEO_REGISTER8(0x10FDB1) = 0;
                    NEO_REGISTER8(0x10FDB2) = 0;
                    NEO_REGISTER8(0x10FDB3) = 0;
                    CALLNEOGEOF(SYS_CREDIT_CHECK);
                    if (NEO_REGISTER8(0x10FDB0)) {
                        CALLNEOGEOF(SYS_CREDIT_DOWN);
                        MAIYA_START_LATCH = 1;
                        NEO_REGISTER8(BIOS_USER_MODE) = 2;
                        NEO_REGISTER8(BIOS_PLAYER1_MODE) = 1;
                    }
                }
                __asm__ volatile ("move.w %0,%%sr" : : "d" (saved_sr) : "memory");
                if (MAIYA_START_LATCH) break;
            }
        } else {
            fixtext_out(30, 3, "        ", 0);
            fixtext_out(13, 25, "             ", 0);
        }
        maiya_vblank();
    }
    (void)i;
    maiya_hero_select();   /* her own screen, now that Start has actually landed */
}

/* One frame boundary per loop; Start belongs to the game's pause state.
 * When the session ends the loop returns and the BIOS restarts the attract. */
void NEOGEO_USER START_GAME(void) {
    NEO_REGISTER8(BIOS_USER_MODE) = 2;
    NEO_REGISTER8(BIOS_PLAYER1_MODE) = 1;
    clearFix();
    clearSprs();
    game_boot();
    for (;;) {
        maiya_vblank();
        game_frame();
        if (maiya_session_over()) break;
    }
    soundStopAll();
    MAIYA_START_LATCH = 0;
    NEO_REGISTER8(BIOS_PLAYER1_MODE) = 3;
    NEO_REGISTER8(BIOS_USER_MODE) = 1;
}
#pragma GCC pop_options
