/*******
https://eaglesoftware.biz
https://github.com/eaglesoftware777
https://github.com/eaglesoftware777/neogeosdk
******/

#include "sdk/2d_engine/ng_join.h"
#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include "sdk/ng_system.h"
#include "sdk/sound_ids.h"
#include "sdk/bsp/bsp.h"
#include "sdk/cabinet/ng_sys.h"
#pragma GCC push_options
#pragma GCC optimize ("O0")

/* "Start has been pressed" (scenes/cx_show.c): set by PLAYER_START or the
 * console's own Start check, read by the attract of the same USER call. */
extern volatile uint8_t cx_start_latch;

/* Play and attract entry points in scenes/cx_menu.c. */
void NEOGEO_USER cx_run(void);
extern uint8_t cx_match_on;          /* the players in a match under way (bit 0 P1, bit 1 P2); 0 between matches */
void NEOGEO_USER cx_attract_init(void);
void NEOGEO_USER cx_frame_wait(void);
void NEOGEO_USER cx_attract_step(uint16_t tick);

NEOGEO_INTERRUPT void NEOGEO_USER ZD_ENTRY(void) {}
NEOGEO_INTERRUPT void NEOGEO_USER CHK_ENTRY(void) {}
NEOGEO_INTERRUPT void NEOGEO_USER TRAPV_ENTRY(void) {}

NEOGEO_INTERRUPT void NEOGEO_USER VBlank(void) {
    ASM_START
    ASM_L(VBLANK)
    ASM_MVW(#1, USER_WORKRAM)
    ASM_ADDQW(#1, USER_WORKRAM+36)     /* blanks counted: a late frame is caught up (cx_show.c) */
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
    uint16_t start_flag;
    /* a match on: START from the side nobody plays is a second player joining
     * (ng_join takes his credit, and only his) */
    if (cx_match_on & 0x80u) { NEO_REGISTER8(BIOS_START_FLAG) = 0; return; }   /* nobody may join: no credit taken */
    if (cx_match_on) { (void)ng_join_bios_start(cx_match_on); return; }
    /* Player 1 already paid (the menus and scenes between matches, where START
     * also skips): the system asks on every press and leaves the answer to
     * the game, and saying yes again spent another credit each time. */
    if (NEO_REGISTER8(BIOS_PLAYER1_MODE) == 1) { NEO_REGISTER8(BIOS_START_FLAG) = 0; return; }
    start_flag = NEO_REGISTER8(BIOS_START_FLAG);
    uint16_t country_code = NEO_REGISTER8(BIOS_COUNTRY_CODE);
    NEO_REGISTER8(BIOS_USER_MODE) = 2;
    cx_start_latch = 1;
    NEO_REGISTER8(BIOS_PLAYER1_MODE) = 1;
    NEO_REGISTER8(BIOS_START_FLAG) = start_flag | 1;
    if ((start_flag >> 1) & 1)
        NEO_REGISTER8(BIOS_PLAYER2_MODE) = (country_code == 1) ? 2 : 3;
#ifndef NG_AES
    ng_sys_credit_check();
    ng_sys_credit_down();
#endif
}

/* Whether player 2 could pay now, as the system counts credits: shared, or
 * one count per coin chute (the US system ROM keeps his apart from player
 * 1's). Nothing is taken. */
uint8_t NEOGEO_USER cx_player2_can_pay(void) {
#ifndef NG_AES
    volatile uint8_t *dec = (volatile uint8_t *)BIOS_CREDIT_DEC;
    uint16_t saved_sr;
    uint8_t ok;
    __asm__ volatile ("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d" (saved_sr) : : "memory");
    dec[0] = 0; dec[1] = 1; dec[2] = 0; dec[3] = 0;
    ng_sys_credit_check();
    ok = dec[1];
    dec[1] = 0;
    __asm__ volatile ("move.w %0,%%sr" : : "d" (saved_sr) : "memory");
    return ok ? 1u : 0u;
#else
    return 1;
#endif
}

/* The second player's START, from the versus screen: his own credit, taken
 * here (the system's PLAYER_START call for it is declined while player 1
 * plays). Answers 1 once he has paid; a console has no credits. */
uint8_t NEOGEO_USER cx_pay_player2(void) {
#ifndef NG_AES
    volatile uint8_t *dec = (volatile uint8_t *)BIOS_CREDIT_DEC;
    uint16_t saved_sr;
    uint8_t paid;
    __asm__ volatile ("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d" (saved_sr) : : "memory");
    dec[0] = 0; dec[1] = 1; dec[2] = 0; dec[3] = 0;
    ng_sys_credit_check();
    paid = dec[1];
    if (paid) {
        ng_sys_credit_down();
        NEO_REGISTER8(BIOS_START_FLAG) = NEO_REGISTER8(BIOS_START_FLAG) | 2;
        NEO_REGISTER8(BIOS_PLAYER2_MODE) = 1;
    }
    __asm__ volatile ("move.w %0,%%sr" : : "d" (saved_sr) : "memory");
    return paid ? 1u : 0u;
#else
    return 1;
#endif
}

void NEOGEO_USER DEMO_END(void)   { soundStopAll(); }
void NEOGEO_USER COIN_SOUND(void) {
    isZ80Ready();
    soundStopAll();
}

void NEOGEO_USER POWER_ON(void) {
    cx_start_latch = 0;
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

/* The system's eye-catcher slot. The FIX layer is not on screen in this phase
 * and work RAM has not been cleared yet, so this only leaves a clean black
 * screen: the game's frame wait would commit sprite queues that hold whatever
 * a board powers up with. */
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
    ASM_JSR(START_GAME)
    ASM_JMP(SYS_RETURN)
    ::: ASM_END
}

void NEOGEO_USER WORK_INIT(void) {
    uint32_t keep_lo = NG_CART_SAVE_START();
    uint32_t keep_hi = keep_lo + NG_CART_SAVE_SIZE();
    uint32_t *p = (uint32_t *)RAMSTART;
    int i;
    for (i = 0; i < 15360; i++, p++) {
        uint32_t at = (uint32_t)(uintptr_t)p;
        if ((i & 255) == 0) kickWatchDog();
        if (at + 4u > keep_lo && at < keep_hi) continue;     /* the save block survives */
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
    /* Pal 0: Crisp White text with black outline (colour 2 = outline) */
    setpal(p, 0x8000, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(p, PALETTES);
    /* Pal 1: Championship Gold text with black outline */
    setpal(p, 0x8000, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(p, PALETTES + PALOFFSET);
    /* Pal 2: Broadcast Cyan text with black outline */
    setpal(p, 0x8000, CYAN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(p, PALETTES + (PALOFFSET * 2));
    /* Pal 3: Pitch Emerald Green text with black outline */
    setpal(p, 0x8000, GREEN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(p, PALETTES + (PALOFFSET * 3));
    /* Pal 4: Alert Crimson Red text with black outline */
    setpal(p, 0x8000, RED, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(p, PALETTES + (PALOFFSET * 4));
}

void NEOGEO_USER INIT_GAME(void) {
    ASM_START
    /* No input callback may write into RAM while it is being cleared: a Start
     * the system accepted during the clear would be erased by it. The sound
     * queue is set up after the clear, which used to wipe it. */
    ASM_MVW(#0x2700,%%sr)
    ASM_JSR(WORK_INIT)
    ASM_JSR(soundInit)
    ASM_JSR(DISPLAY_INIT)
    ASM_JSR(setup_fix_palettes)
    ::: ASM_END
    /* nobody is playing between sessions: a session that never reached its
     * end must not leave player 1 "paid", or PLAYER_START declines him */
    if (NEO_REGISTER8(BIOS_PLAYER1_MODE) == 1) NEO_REGISTER8(BIOS_PLAYER1_MODE) = 3;
    NEO_REGISTER8(BIOS_USER_MODE) = 1;
    ASM_START ASM_MVW(#0x2000,%%sr) ::: ASM_END
}

/* INIT_GAME cleared the latch before enabling input. It is not cleared again
 * here: a blank may already have accepted a paid Start. */
void NEOGEO_USER GAME_DISPATCH(void) {
#ifndef NG_AES
    if (!cx_start_latch) {
        NEO_REGISTER8(BIOS_USER_MODE) = 1;
        GAME_ATTRACT();
    }
    if (cx_start_latch) {
        NEO_REGISTER8(BIOS_USER_MODE) = 2;
        START_GAME();
    }
#else
    NEO_REGISTER8(BIOS_USER_MODE) = 1;
    GAME_ATTRACT();
    if (cx_start_latch) {
        NEO_REGISTER8(BIOS_USER_MODE) = 2;
        START_GAME();
    }
#endif
}

void NEOGEO_USER DEMO_GAME(void)    { GAME_ATTRACT(); }

void NEOGEO_USER GAME_ATTRACT(void) {
    uint16_t i;
    cx_attract_init();
    for (i = 0; ; i++) {
        uint16_t joy;

        if (cx_start_latch) break;

        cx_attract_step(i);

        joy = poll_joystick();
        if (joy & START1) {
#ifndef NG_AES
            if (read_p1credit() > 0) {
                uint16_t saved_sr;
                /* once, atomically with the system's own PLAYER_START call */
                __asm__ volatile ("move.w %%sr,%0\n\tmove.w #0x2700,%%sr"
                                  : "=d" (saved_sr) : : "memory");
                if (!cx_start_latch) {
                    volatile uint8_t *dec = (volatile uint8_t *)BIOS_CREDIT_DEC;
                    dec[0] = 1; dec[1] = 0; dec[2] = 0; dec[3] = 0;   /* player 1's credit */
                    PLAYER_START();
                }
                __asm__ volatile ("move.w %0,%%sr" : : "d" (saved_sr) : "memory");
                if (cx_start_latch) break;
            }
#else
            NEO_REGISTER8(BIOS_USER_MODE) = 2;
            cx_start_latch = 1;
            break;
#endif
        }
        cx_frame_wait();
    }
}

void NEOGEO_USER START_GAME(void) {
    clearFix();
    clearSprs();
    setBACKDROP(BLACK);
    cx_run();
    cx_start_latch = 0;
    /* the session is over: whoever played may pay to start again */
    if (NEO_REGISTER8(BIOS_PLAYER1_MODE)) NEO_REGISTER8(BIOS_PLAYER1_MODE) = 3;
    if (NEO_REGISTER8(BIOS_PLAYER2_MODE)) NEO_REGISTER8(BIOS_PLAYER2_MODE) = 3;
    NEO_REGISTER8(BIOS_USER_MODE) = 1;
    ASM_START ASM_JMP(SYS_RETURN) ::: ASM_END
}

#pragma GCC pop_options
