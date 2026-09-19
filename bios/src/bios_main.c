#include "bios.h"

/*
 * ============================================================================
 *  EagleBIOS Main Boot, Dispatcher & Exception Handlers
 * ============================================================================
 */



void bios_watchdog(void)
{
    REG_DIPSW = 0;
}

void bios_wait_vbl(void)
{
    bios_watchdog();
    uint32_t cur = BIOS_VBL_TICK;
    uint32_t timeout = 500000;
    while (BIOS_VBL_TICK == cur && --timeout) {
        bios_watchdog();
    }
}

void bios_delay_frames(uint16_t frames)
{
    while (frames--) {
        bios_wait_vbl();
    }
}

extern void call_cart_user_asm(void);

static void call_cart_user(void)
{
    call_cart_user_asm();
}

/*
 * Cold / Warm Boot Reset Sequence (Entry at 0xC00402)
 */
void bios_reset(void)
{
    /* 1. Disable interrupts during boot */
    asm volatile ("move.w #0x2700, %sr");

    /* 2. Kick hardware watchdog */
    bios_watchdog();

    /* 3. Hardware subsystem reset */
    REG_LSPCMODE = 0x0000u;
    REG_IRQACK   = 7u; /* Acknowledge IRQ 1, 2, 3 */
    REG_NOSHADOW = 0;
    REG_PALBANK0 = 0;
    REG_BRDFIX   = 0;

    /* 4. Clear 68000 User Work RAM (0x100000 .. 0x10EFFF) without touching stack at 0x10F300 */
    volatile uint32_t *ram = (volatile uint32_t *)ADDR_USER_RAM;
    for (uint32_t i = 0; i < 15360u; i++) {
        *ram++ = 0;
    }

    /* 5. Initialize BIOS Work RAM variables */
    BIOS_VBL_TICK     = 0;
    BIOS_SYSTEM_MODE  = 0x00;
    /* Hardware MVS vs AES detection: REG_STATUS_B bit 7 is 1 for MVS Arcade, 0 for AES Console */
    if (REG_STATUS_B & 0x80) {
        BIOS_MVS_FLAG = 1;  /* MVS Arcade mode */
        P1_CREDITS    = 0;
        P2_CREDITS    = 0;
    } else {
        BIOS_MVS_FLAG = 0;  /* AES Home Console mode */
        P1_CREDITS    = 99;
        P2_CREDITS    = 99;
    }
    BIOS_COUNTRY_CODE = 2;  /* Default Europe */
    BIOS_USER_REQUEST = 0;  /* Initial request = POWER_ON */
    BIOS_USER_MODE    = 0;  /* Initial mode = Boot */
    BIOS_START_FLAG   = 0;
    BIOS_PLAYER1_MODE = 0;
    BIOS_PLAYER2_MODE = 0;
    BIOS_MESS_POINT   = 0;
    BIOS_MESS_BUSY    = 0;

    /* 6. Video subsystem initialization */
    sys_lsp_1st_c();
    sys_fix_clear_c();
    bios_set_backdrop(COLOR_BLACK);
    bios_init_palette_banks();

    /* 7. Enable interrupts so VBlank and timers run */
    asm volatile ("move.w #0x2000, %sr");

    /* 8. Check Test switch on cabinet (MVS only: DIP switch 1 active low: REG_DIPSW bit 0, or Service bit 2) */
    if (BIOS_MVS_FLAG) {
        if (!(REG_DIPSW & 0x01) || !(REG_STATUS_A & 0x04)) {
            bios_test_menu();
        }
    }

    /* 9. Verify cartridge header */
    if (CART_HEADER->magic[0] == 'N' && CART_HEADER->magic[1] == 'E' &&
        CART_HEADER->magic[2] == 'O' && CART_HEADER->magic[3] == '-' &&
        CART_HEADER->magic[4] == 'G' && CART_HEADER->magic[5] == 'E' &&
        CART_HEADER->magic[6] == 'O') {

        /* BIOS animated splash on cold boot */
        bios_splash_show();

        /* Dispatch initial request: POWER_ON (Request 0) */
        BIOS_USER_REQUEST = 0;
        BIOS_USER_MODE    = 0;
        call_cart_user();

    } else {
        /* No valid cartridge detected */
        sys_fix_clear_c();
        bios_init_palette_banks();
        bios_fix_puts(4,  6, "==================================", 4);
        bios_fix_puts(11, 8, "E A G L E   B I O S", 1);
        bios_fix_puts(9, 10, "NO CARTRIDGE DETECTED", 4);
        bios_fix_puts(4, 12, "==================================", 4);

        bios_fix_puts(6, 16, "PLEASE INSERT A NEO-GEO CARTRIDGE", 1);
        bios_fix_puts(8, 18, "OR HOLD TEST SWITCH FOR MENU", 2);

        for (;;) {
            bios_wait_vbl();
            sys_io_c();
            if ((BIOS_P1CHANGE & BTN_A) || (BIOS_MVS_FLAG && (!(REG_DIPSW & 0x01) || !(REG_STATUS_A & 0x04)))) {
                bios_test_menu();
                break;
            }
        }
    }

    /* Fallback return loop */
    sys_return_c();
}

/*
 * SYS_RETURN: Cartridge return dispatcher (Entry at 0xC00444)
 * Handles state transitions between POWER_ON, EYE_CATCHER, TITLE, and GAME.
 * Supports both MVS Arcade flow and AES Console direct boot.
 */
void sys_return_c(void)
{
    for (;;) {
        bios_watchdog();

        uint8_t req = BIOS_USER_REQUEST;

        if (req == 0) {
            /* POWER_ON finished -> transition to EYE_CATCHER, TITLE (MVS) or GAME (AES) */
            if (CART_HEADER->logoflag != 0) {
                BIOS_USER_REQUEST = 1; /* EYE_CATCHER */
                BIOS_USER_MODE    = 1; /* Attract */
            } else if (BIOS_MVS_FLAG) {
                BIOS_USER_REQUEST = 3; /* TITLE (MVS) */
                BIOS_USER_MODE    = 1; /* Attract */
            } else {
                BIOS_USER_REQUEST = 2; /* GAME (AES Home) */
                BIOS_USER_MODE    = 1; /* Attract */
            }
            call_cart_user();

        } else if (req == 1) {
            /* EYE_CATCHER finished -> transition to TITLE (MVS) or GAME (AES) */
            if (BIOS_MVS_FLAG) {
                BIOS_USER_REQUEST = 3; /* TITLE */
                BIOS_USER_MODE    = 1; /* Attract */
            } else {
                BIOS_USER_REQUEST = 2; /* GAME */
                BIOS_USER_MODE    = 1; /* Attract */
            }
            call_cart_user();

        } else if (req == 3) {
            /* TITLE loop returned (MVS only) */
            if (BIOS_START_FLAG != 0 || BIOS_USER_MODE == 2) {
                /* Player started game! */
                BIOS_USER_REQUEST = 2; /* GAME */
                BIOS_USER_MODE    = 2; /* Game playing */
                call_cart_user();
            } else {
                /* Attract timeout: loop back to EYE_CATCHER or TITLE */
                BIOS_USER_REQUEST = 1; /* EYE_CATCHER */
                BIOS_USER_MODE    = 1;
                call_cart_user();
            }

        } else if (req == 2) {
            /* GAME finished (Game Over / Returned to Attract) */
            BIOS_START_FLAG   = 0;
            if (BIOS_MVS_FLAG) {
                BIOS_USER_REQUEST = 3; /* TITLE (MVS) */
            } else {
                BIOS_USER_REQUEST = 2; /* GAME (AES) */
            }
            BIOS_USER_MODE    = 1; /* Attract */
            call_cart_user();

        } else {
            /* Default fallback */
            BIOS_USER_REQUEST = BIOS_MVS_FLAG ? 3 : 2;
            BIOS_USER_MODE    = 1;
            call_cart_user();
        }
    }
}

/*
 * Exception Handlers
 */
static void exception_halt(const char *name)
{
    asm volatile ("move.w #0x2700, %sr");
    bios_set_backdrop(COLOR_RED);
    sys_fix_clear_c();
    sys_lsp_1st_c();

    bios_fix_puts(4,  8, "==================================", 1);
    bios_fix_puts(10, 10, "SYSTEM EXCEPTION OCCURRED", 1);
    bios_fix_puts(4, 12, "==================================", 1);
    bios_fix_puts(6, 15, "EXCEPTION TYPE:", 2);
    bios_fix_puts(22, 15, name, 1);

    bios_fix_puts(6, 20, "SYSTEM HALTED - REBOOT MACHINE", 1);

    for (;;) {
        bios_watchdog();
    }
}

void bios_addr_err_c(uint32_t pc, uint32_t addr, uint32_t ir, uint32_t old_sp)
{
    asm volatile ("move.w #0x2700, %sr");
    bios_set_backdrop(COLOR_RED);
    sys_fix_clear_c();
    sys_lsp_1st_c();

    bios_fix_puts(4,  4, "==================================", 1);
    bios_fix_puts(10, 6, "SYSTEM EXCEPTION OCCURRED", 1);
    bios_fix_puts(4,  8, "==================================", 1);
    bios_fix_puts(6, 11, "EXCEPTION: ADDRESS ERROR", 2);

    bios_fix_puts(6, 14, "FAULT PC : 0x", 1);
    bios_fix_put_hex16(19, 14, (uint16_t)(pc >> 16), 2);
    bios_fix_put_hex16(23, 14, (uint16_t)(pc & 0xFFFF), 2);

    bios_fix_puts(6, 16, "BAD ADDR : 0x", 1);
    bios_fix_put_hex16(19, 16, (uint16_t)(addr >> 16), 2);
    bios_fix_put_hex16(23, 16, (uint16_t)(addr & 0xFFFF), 2);

    bios_fix_puts(6, 18, "OPCODE IR: 0x", 1);
    bios_fix_put_hex16(19, 18, (uint16_t)(ir & 0xFFFF), 2);

    bios_fix_puts(6, 20, "STACK SP : 0x", 1);
    bios_fix_put_hex16(19, 20, (uint16_t)(old_sp >> 16), 2);
    bios_fix_put_hex16(23, 20, (uint16_t)(old_sp & 0xFFFF), 2);

    bios_fix_puts(6, 23, "SYSTEM HALTED - REBOOT MACHINE", 1);

    for (;;) {
        bios_watchdog();
    }
}

void bios_bus_err_c(uint32_t pc, uint32_t addr, uint32_t ir, uint32_t old_sp)
{
    asm volatile ("move.w #0x2700, %sr");
    bios_set_backdrop(COLOR_RED);
    sys_fix_clear_c();
    sys_lsp_1st_c();

    bios_fix_puts(4,  4, "==================================", 1);
    bios_fix_puts(10, 6, "SYSTEM EXCEPTION OCCURRED", 1);
    bios_fix_puts(4,  8, "==================================", 1);
    bios_fix_puts(6, 11, "EXCEPTION: BUS ERROR", 2);

    bios_fix_puts(6, 14, "FAULT PC : 0x", 1);
    bios_fix_put_hex16(19, 14, (uint16_t)(pc >> 16), 2);
    bios_fix_put_hex16(23, 14, (uint16_t)(pc & 0xFFFF), 2);

    bios_fix_puts(6, 16, "BAD ADDR : 0x", 1);
    bios_fix_put_hex16(19, 16, (uint16_t)(addr >> 16), 2);
    bios_fix_put_hex16(23, 16, (uint16_t)(addr & 0xFFFF), 2);

    bios_fix_puts(6, 18, "OPCODE IR: 0x", 1);
    bios_fix_put_hex16(19, 18, (uint16_t)(ir & 0xFFFF), 2);

    bios_fix_puts(6, 20, "STACK SP : 0x", 1);
    bios_fix_put_hex16(19, 20, (uint16_t)(old_sp >> 16), 2);
    bios_fix_put_hex16(23, 20, (uint16_t)(old_sp & 0xFFFF), 2);

    bios_fix_puts(6, 23, "SYSTEM HALTED - REBOOT MACHINE", 1);

    for (;;) {
        bios_watchdog();
    }
}

void bios_bus_err(void)    { exception_halt("BUS ERROR"); }
void bios_addr_err(void)   { exception_halt("ADDRESS ERROR"); }
void bios_illegal(void)    { exception_halt("ILLEGAL INSTRUCTION"); }
void bios_privilege(void)  { exception_halt("PRIVILEGE VIOLATION"); }
void bios_trace(void)      { exception_halt("TRACE TRAP"); }
void bios_trap(void)       { exception_halt("UNHANDLED TRAP"); }
void bios_uninit(void)     { exception_halt("UNINIT INTERRUPT"); }
void bios_spurious(void)   { exception_halt("SPURIOUS INTERRUPT"); }
