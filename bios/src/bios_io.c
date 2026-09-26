#include "bios.h"

uint8_t bios_coin_state, bios_coin_change, bios_free_play_override;
static uint8_t start_callback, spent_mask;
extern void bios_call_cart(uint32_t address);
extern void bios_enter_title(void) __attribute__((noreturn));

static uint8_t bcd_value(uint8_t value)
{
    return (uint8_t)((value >> 4) * 10u + (value & 15u));
}

static uint8_t bcd_encode(uint8_t value)
{
    return (uint8_t)((value / 10u) * 16u + value % 10u);
}

uint8_t bios_free_play(void)
{
    return !BIOS_MVS_FLAG || bios_free_play_override || !(REG_DIPSW & 0x40u);
}

void bios_controller_setup(void)
{
    REG_POUTPUT = 0;
    BIOS_P1STATUS = BIOS_P2STATUS = 1;
}

static void read_pad(volatile uint8_t *pad, uint8_t value)
{
    pad[0] = 1;
    pad[1] = pad[2];
    pad[2] = value;
    pad[3] = (uint8_t)(value & ~pad[1]);
    pad[4] = pad[3];
    if (!value) pad[5] = 0;
    else if (pad[3]) pad[5] = 16;
    else if (pad[5] > 1) pad[5]--;
    else { pad[4] = value; pad[5] = 8; }
}

uint32_t sys_credit_check_c(void)
{
    uint8_t shared = !(REG_DIPSW & 2u);
    uint8_t available = BIOS_MVS_FLAG ? bcd_value(P1_CREDITS) : 99;
    for (uint8_t p = 0; p < 2; p++) {
        uint8_t request = bcd_value(BIOS_CREDIT_DEC[p]);
        if (bios_free_play()) continue;
        if (!shared && p) available = bcd_value(P2_CREDITS);
        if (request > available) BIOS_CREDIT_DEC[p] = 0;
        else available -= request;
    }
    return bios_free_play() ? 99 : bcd_value(P1_CREDITS);
}

void sys_credit_down_c(void)
{
    if (BIOS_MVS_FLAG) REG_SRAMUNLOCK = 0;
    for (uint8_t p = 0; p < 2; p++) {
        uint8_t bit = (uint8_t)(1u << p);
        if (!BIOS_CREDIT_DEC[p] || (start_callback && (spent_mask & bit))) continue;
        if (!bios_free_play()) {
            volatile uint8_t *credit = (p && (REG_DIPSW & 2u)) ? &P2_CREDITS : &P1_CREDITS;
            uint8_t value = bcd_value(*credit), cost = bcd_value(BIOS_CREDIT_DEC[p]);
            if (cost > value) continue;
            *credit = bcd_encode((uint8_t)(value - cost));
        }
        if (start_callback) spent_mask |= bit;
    }
    if (BIOS_MVS_FLAG) REG_SRAMLOCK = 0;
}

void sys_io_c(void)
{
    BIOS_FRAME_COUNTER++;
    read_pad(&BIOS_P1STATUS, (uint8_t)~REG_P1CNT);
    read_pad(&BIOS_P2STATUS, (uint8_t)~REG_P2CNT);
    uint8_t raw = (uint8_t)(~REG_STATUS_B & 15u);
    uint8_t buttons = BIOS_MVS_FLAG ? (uint8_t)(raw & 5u) : raw;
    BIOS_STATCHANGE = (uint8_t)(buttons & ~BIOS_STATCURNT);
    BIOS_STATCURNT = buttons;
    *(volatile uint8_t *)0x10FEDDu = (uint8_t)(raw & ~*(volatile uint8_t *)0x10FEDCu);
    *(volatile uint8_t *)0x10FEDCu = raw;

    /* Coins are not player 3/4 START bits in the public status byte. */
    uint8_t coins = BIOS_MVS_FLAG ? (uint8_t)(~REG_STATUS_A & 7u) : 0;
    bios_coin_change = (uint8_t)(coins & ~bios_coin_state);
    bios_coin_state = coins;
    if (bios_coin_change) {
        REG_SRAMUNLOCK = 0;
        for (uint8_t p = 0; p < 3; p++) {
            if (!(bios_coin_change & (1u << p))) continue;
            volatile uint8_t *credit = (p == 1 && (REG_DIPSW & 2u)) ? &P2_CREDITS : &P1_CREDITS;
            uint8_t value = bcd_value(*credit);
            if (value < 99) *credit = bcd_encode((uint8_t)(value + 1));
        }
        REG_SRAMLOCK = 0;
        if (bios_cart_active) bios_call_cart(0x134u);
    }

    if (!bios_cart_active || !(BIOS_SYSTEM_MODE & 0x80u) || !BIOS_USER_MODE) return;
    uint8_t requested = 0;
    if ((BIOS_STATCHANGE & STAT_START1) && BIOS_PLAYER1_MODE != 1) requested |= 1;
    if ((BIOS_STATCHANGE & STAT_START2) && BIOS_PLAYER2_MODE != 1) requested |= 2;
    if (requested) {
        BIOS_CREDIT_DEC[0] = requested & 1u;
        BIOS_CREDIT_DEC[1] = (requested >> 1) & 1u;
        sys_credit_check_c();
        BIOS_START_FLAG = (BIOS_CREDIT_DEC[0] ? 1u : 0u) | (BIOS_CREDIT_DEC[1] ? 2u : 0u);
        if (BIOS_START_FLAG) {
            start_callback = 1;
            spent_mask = 0;
            bios_call_cart(0x128u);
            if (!(BIOS_START_FLAG & 1u)) BIOS_CREDIT_DEC[0] = 0;
            if (!(BIOS_START_FLAG & 2u)) BIOS_CREDIT_DEC[1] = 0;
            sys_credit_down_c();
            start_callback = 0;
        }
    }
    if (bios_coin_change && BIOS_USER_MODE == 1 && BIOS_USER_REQUEST == 2) {
        bios_call_cart(0x12Eu);
        bios_enter_title();
    }
}

void sys_int1_c(void)
{
    REG_IRQACK = 4;
    bios_watchdog();
    BIOS_VBL_TICK++;
    sys_io_c();
    if (!(BIOS_SYSTEM_MODE & 0x80u)) sys_mess_out_c();
}

void sys_int2_c(void)
{
    REG_IRQACK = 2;
}

void sys_howtoplay_c(void)
{
}
