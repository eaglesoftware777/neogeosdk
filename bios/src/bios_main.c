#include "bios.h"

uint8_t bios_cart_active;

void bios_watchdog(void)
{
    REG_DIPSW = 0;
}

void bios_wait_vbl(void)
{
    uint32_t tick = BIOS_VBL_TICK;
    while (BIOS_VBL_TICK == tick) bios_watchdog();
}

void bios_delay_frames(uint16_t frames)
{
    while (frames--) bios_wait_vbl();
}

uint8_t bios_cart_valid(void)
{
    static const char signature[] = "NEO-GEO";
    const volatile uint8_t *rom = (const volatile uint8_t *)0x100u;
    for (uint8_t i = 0; i < 7; i++)
        if (rom[i] != (uint8_t)signature[i]) return 0;
    return 1;
}

extern uint8_t __data_load[], __data_start[], __data_end[];
extern void call_cart_user_asm(void) __attribute__((noreturn));

void bios_cart_prepare(void)
{
    if (BIOS_MVS_FLAG) {
        /* The system sound program moves entirely into RAM before replying.
         * Its RAM handoff polls RESET, so cartridge NMIs need not implement
         * the original slot-switch convention. Never switch live ROM code. */
        REG_SOUND = 1;
        bios_delay_frames(2);
        uint16_t wait = 120;
        while (REG_SOUND != 1 && wait--) bios_wait_vbl();
        if (REG_SOUND != 1) {
            bios_fix_puts(8, 22, "SOUND HANDOFF TIMEOUT", 4);
            for (;;) bios_watchdog();
        }
    }
    REG_CRTFIX = 0;
    REG_SOUND = 3;
    bios_delay_frames(6);
    bios_cart_active = 1;
}

static void call_cart_user(void)
{
    asm volatile ("move.w #0x2700, %sr");
    BIOS_SYSTEM_MODE = 0;
    BIOS_MESS_POINT = 0x10FF00u;
    BIOS_MESS_BUSY = 0;
    REG_LSPCMODE = 0x4000;
    REG_IRQACK = 7;
    sys_fix_clear_c();
    sys_lsp_1st_c();
    bios_palettes_clear();
    call_cart_user_asm();
}

void bios_reset(void)
{
    asm volatile ("move.w #0x2700, %sr");
    bios_watchdog();
    REG_SWPBIOS = 0;
    REG_LSPCMODE = 0;
    REG_IRQACK = 7;
    REG_NOSHADOW = 0;
    REG_PALBANK0 = 0;

    /* Do not clear the live supervisor stack below 10F300. */
    volatile uint32_t *ram = (volatile uint32_t *)ADDR_USER_RAM;
    for (uint32_t i = 0; i < 15360u; i++) {
        *ram++ = 0;
        if ((i & 255u) == 0) bios_watchdog();
    }
    ram = (volatile uint32_t *)0x10F400u;
    for (uint16_t i = 0; i < 0xC00u / 4u; i++) *ram++ = 0;
    uint8_t *dst = __data_start;
    const uint8_t *src = __data_load;
    while (dst < __data_end) *dst++ = *src++;

    BIOS_MVS_FLAG = (REG_STATUS_B & 0x80u) ? 0x80 : 0;
    BIOS_COUNTRY_CODE = *(const volatile uint8_t *)0xC00401u;
    BIOS_MESS_POINT = 0x10FF00u;
    BIOS_SELECT_TIMER = 0x30;
    bios_controller_setup();
    if (BIOS_MVS_FLAG) {
        REG_BRDFIX = 0;
        REG_SLOT = 0;
        /* Release both coin lockouts and leave counter coils inactive. */
        *(volatile uint8_t *)0x380061u = 0;
        *(volatile uint8_t *)0x380063u = 0;
        *(volatile uint8_t *)0x380065u = 0;
        *(volatile uint8_t *)0x380067u = 0;
        REG_SRAMUNLOCK = 0;
        P1_CREDITS = P2_CREDITS = 0;
        *(volatile uint8_t *)0xD00046u = 0;
        *(volatile uint8_t *)0xD00047u = 1;
        REG_SRAMLOCK = 0;
    } else {
        /* AES has no motherboard SFIX/SM1; use the cartridge font and M1. */
        REG_CRTFIX = 0;
    }
    sys_lsp_1st_c();
    sys_fix_clear_c();
    bios_set_backdrop(COLOR_BLACK);
    bios_init_palette_banks();
    asm volatile ("move.w #0x2000, %sr");

    if (BIOS_MVS_FLAG && (!(REG_DIPSW & 1) || !(REG_SYSTYPE & 0x80)))
        bios_test_menu();

    while (!bios_cart_valid()) {
        bios_fix_puts(15, 10, "EAGLE BIOS", 1);
        bios_fix_puts(10, 14, "NO CARTRIDGE DETECTED", 3);
        bios_wait_vbl();
        if (BIOS_P1CHANGE & BTN_A) bios_test_menu();
    }

    /* Load the cartridge's documented regional soft-DIP defaults. */
    uint32_t ptr = CART_HEADER->reserved0[BIOS_COUNTRY_CODE];
    if (ptr >= 0x140u && ptr <= 0x0FFFE0u && !(ptr & 1u)) {
        const volatile uint8_t *defaults = (const volatile uint8_t *)(ptr + 16u);
        /* Time/count fields are literal; option descriptors pack the
         * default selection above the number of available choices. */
        for (uint8_t i = 0; i < 6; i++) BIOS_GAME_DIP[i] = defaults[i];
        for (uint8_t i = 6; i < 16; i++) {
            uint8_t descriptor = defaults[i];
            uint8_t choice = descriptor >> 4;
            uint8_t count = descriptor & 15u;
            BIOS_GAME_DIP[i] = choice < count ? choice : 0u;
        }
    }

    /* An arcade board gets the eye-catcher and then the title at power-on.
     * A console shows both after the cartridge's own power-on step when it
     * asks for the system eye-catcher, the order a home cartridge is
     * written for; a cartridge that brings its own, or wants none, still
     * gets the title here. */
    if (BIOS_MVS_FLAG) {
        bios_eyecatcher();
        bios_splash_show();
    } else if (CART_HEADER->logoflag != 0) {
        bios_splash_show();
    }
    bios_cart_prepare();
    /* The game's saved block comes back from backup RAM. Only a board that
     * has never held this game's data asks it for its first power-on set-up
     * (command 0); every other boot goes straight into the attract. */
    if (bios_backup_load()) {
        BIOS_USER_REQUEST = 2;
        BIOS_USER_MODE = 1;
    } else {
        BIOS_USER_REQUEST = 0;
        BIOS_USER_MODE = 0;
    }
    call_cart_user();
}

/* Credits waiting on an arcade board that isn't on free play. */
static uint8_t credits_waiting(void)
{
    return (uint8_t)(BIOS_MVS_FLAG && !bios_free_play() && (P1_CREDITS || P2_CREDITS));
}

void sys_return_c(void)
{
    uint8_t previous = BIOS_USER_REQUEST;
    /* Every return hands the game's block back to backup RAM. */
    bios_backup_save();
    BIOS_START_FLAG = 0;
    BIOS_PLAYER1_MODE = BIOS_PLAYER2_MODE = 0;
    if (previous == 0 && !BIOS_MVS_FLAG && CART_HEADER->logoflag == 1) {
        /* The cartridge draws its own eye-catcher. */
        BIOS_USER_REQUEST = 1;
        BIOS_USER_MODE = 0;
    } else {
        /* Flag 0 leaves the eye-catcher to the system; flag 2 wants none.
         * SYS_RETURN arrives with interrupts masked: the system vectors are
         * back in place, so let VBlank run for the presentation. */
        if (previous == 0 && !BIOS_MVS_FLAG && CART_HEADER->logoflag == 0) {
            asm volatile ("move.w #0x2000, %sr");
            bios_eyecatcher();
            bios_splash_show();
        }
        /* Command 2 is attract/game; command 3 is the title a credit waits
         * on. With credits already in, the demo would see them at once and
         * hand straight back, and the pair would loop showing its opening
         * over and over: go to the title instead. Command 3 is never a
         * cold-boot entry. */
        if (previous != 0 && credits_waiting()) {
            BIOS_USER_REQUEST = 3;
            BIOS_SELECT_TIMER = 0x30;
        } else {
            BIOS_USER_REQUEST = 2;
        }
        BIOS_USER_MODE = 1;
    }
    call_cart_user();
}

void bios_enter_title(void)
{
    BIOS_USER_REQUEST = 3;
    BIOS_USER_MODE = 1;
    BIOS_SELECT_TIMER = 0x30;
    call_cart_user();
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
