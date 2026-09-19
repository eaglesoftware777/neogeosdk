#include "bios.h"

/*
 * ============================================================================
 *  EagleBIOS Input / Output & Interrupts Sub-system
 * ============================================================================
 */

static void call_cart_entry(uint32_t addr)
{
    typedef void (*CartEntryFunc)(void);
    CartEntryFunc func = (CartEntryFunc)addr;
    func();
}

/*
 * SYS_IO: Sample joysticks, buttons, coins, and system switches.
 */
void sys_io_c(void)
{
    /* 1. Player 1 inputs */
    uint8_t in1 = (uint8_t)(~REG_P1CNT);
    BIOS_P1PREVIOUS = BIOS_P1CURRENT;
    BIOS_P1CURRENT  = in1;
    BIOS_P1CHANGE   = (uint8_t)(in1 & (uint8_t)(~BIOS_P1PREVIOUS));
    BIOS_P1STATUS   = (uint8_t)(in1 != 0);

    /* Repeat timing */
    if (in1 != 0) {
        if (BIOS_P1CHANGE != 0) {
            BIOS_P1REPEAT = BIOS_P1CHANGE;
            BIOS_P1TIMER = 20; /* 20 frames initial delay */
        } else if (BIOS_P1TIMER > 0) {
            BIOS_P1TIMER--;
            BIOS_P1REPEAT = 0;
        } else {
            BIOS_P1REPEAT = in1;
            BIOS_P1TIMER = 4;  /* 4 frames repeat interval */
        }
    } else {
        BIOS_P1REPEAT = 0;
        BIOS_P1TIMER = 0;
    }

    /* 2. Player 2 inputs */
    uint8_t in2 = (uint8_t)(~REG_P2CNT);
    BIOS_P2PREVIOUS = BIOS_P2CURRENT;
    BIOS_P2CURRENT  = in2;
    BIOS_P2CHANGE   = (uint8_t)(in2 & (uint8_t)(~BIOS_P2PREVIOUS));
    BIOS_P2STATUS   = (uint8_t)(in2 != 0);

    if (in2 != 0) {
        if (BIOS_P2CHANGE != 0) {
            BIOS_P2REPEAT = BIOS_P2CHANGE;
            BIOS_P2TIMER = 20;
        } else if (BIOS_P2TIMER > 0) {
            BIOS_P2TIMER--;
            BIOS_P2REPEAT = 0;
        } else {
            BIOS_P2REPEAT = in2;
            BIOS_P2TIMER = 4;
        }
    } else {
        BIOS_P2REPEAT = 0;
        BIOS_P2TIMER = 0;
    }

    /* 3. System status (Coins, Start, Select, Service, Test) */
    uint8_t stat_a = (uint8_t)(~REG_STATUS_A);
    uint8_t stat_b = (uint8_t)(~REG_STATUS_B);

    uint8_t current_stat = 0;
    if (stat_b & 0x01) current_stat |= STAT_START1;
    if (stat_b & 0x02) current_stat |= STAT_SELECT1;
    if (stat_b & 0x04) current_stat |= STAT_START2;
    if (stat_b & 0x08) current_stat |= STAT_SELECT2;
    if (stat_a & 0x01) current_stat |= STAT_COIN1;
    if (stat_a & 0x02) current_stat |= STAT_COIN2;
    if (stat_a & 0x04) current_stat |= STAT_SERVICE;
    if (stat_a & 0x80) current_stat |= STAT_TEST;

    BIOS_STATCHANGE = (uint8_t)(current_stat & (uint8_t)(~BIOS_STATCURNT));
    BIOS_STATCURNT  = current_stat;

    /* 4. Coin detection (MVS Arcade mode only) */
    if (BIOS_MVS_FLAG) {
        if (BIOS_STATCHANGE & STAT_COIN1) {
            REG_SRAMUNLOCK = 0;
            if (P1_CREDITS < 99) {
                P1_CREDITS++;
            }
            REG_SRAMLOCK = 0;
            if (CART_HEADER->magic[0] == 'N' && CART_HEADER->magic[1] == 'E') {
                call_cart_entry(0x000134u);
            }
        }

        if (BIOS_STATCHANGE & STAT_COIN2) {
            REG_SRAMUNLOCK = 0;
            if (P2_CREDITS < 99) {
                P2_CREDITS++;
            }
            REG_SRAMLOCK = 0;
            if (CART_HEADER->magic[0] == 'N' && CART_HEADER->magic[1] == 'E') {
                call_cart_entry(0x000134u);
            }
        }
    }
}

/*
 * SYS_CREDIT_CHECK: Test if credits are sufficient.
 * Returns credit count in D0; CCR carry is cleared on success, set on zero credits.
 */
uint32_t sys_credit_check_c(void)
{
    /* In AES Console mode or Free Play (DIP switch bit 6): always sufficient credits */
    if (BIOS_MVS_FLAG == 0 || (REG_DIPSW & 0x40)) {
        return 99;
    }
    return P1_CREDITS;
}

/*
 * SYS_CREDIT_DOWN: Decrement credit on game start.
 */
void sys_credit_down_c(void)
{
    /* In AES Console mode or Free Play: do not decrement credits */
    if (BIOS_MVS_FLAG == 0 || (REG_DIPSW & 0x40)) {
        return;
    }
    REG_SRAMUNLOCK = 0;
    if (P1_CREDITS > 0) {
        P1_CREDITS--;
    }
    REG_SRAMLOCK = 0;
}

/*
 * SYS_INT1: VBlank interrupt service routine helper.
 */
void sys_int1_c(void)
{
    /* Acknowledge Level 1 IRQ (VBlank) */
    REG_IRQACK = 4;

    /* Kick hardware watchdog */
    REG_DIPSW = 0;

    /* Set vertical blank flag in user work RAM and increment BIOS tick count */
    *(volatile uint16_t *)ADDR_USER_RAM = 1;
    BIOS_VBL_TICK++;

    /* Sample inputs */
    sys_io_c();

    /* In Attract mode (BIOS_USER_MODE == 1): check for Start button */
    if (BIOS_USER_MODE == 1) {
        uint8_t can_start_p1 = (BIOS_MVS_FLAG == 0) || (REG_DIPSW & 0x40) || (P1_CREDITS > 0);
        uint8_t can_start_p2 = (BIOS_MVS_FLAG == 0) || (REG_DIPSW & 0x40) || (P2_CREDITS > 0);

        if (can_start_p1 && (BIOS_STATCHANGE & STAT_START1)) {
            BIOS_START_FLAG |= 1;
            BIOS_PLAYER1_MODE = 1;
            /* Call cartridge PLAYER_START entry point at 0x128 */
            if (CART_HEADER->magic[0] == 'N' && CART_HEADER->magic[1] == 'E') {
                call_cart_entry(0x000128u);
            }
        } else if (can_start_p2 && (BIOS_STATCHANGE & STAT_START2)) {
            BIOS_START_FLAG |= 2;
            BIOS_PLAYER2_MODE = 1;
            if (CART_HEADER->magic[0] == 'N' && CART_HEADER->magic[1] == 'E') {
                call_cart_entry(0x000128u);
            }
        }
    }
}

/*
 * SYS_INT2: Timer / HBlank interrupt service routine helper.
 */
void sys_int2_c(void)
{
    /* Acknowledge Level 2 IRQ */
    REG_IRQACK = 2;
}

/*
 * SYS_HOWTOPLAY: How to play tutorial.
 */
void sys_howtoplay_c(void)
{
    /* Simple return to keep arcade flow fast and responsive */
}
