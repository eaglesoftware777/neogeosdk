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

/* ---- SYS_HOWTOPLAY ------------------------------------------------------
 * A game may hand over its own instructions: A0 at the call points at an
 * HTP1 block in its program ROM (sdk/ng_system.h: NGHowToPlay and
 * ng_sys_howtoplay). Every field is checked before anything is drawn; any
 * other A0 gets a short screen that names no game's buttons. The game's
 * own VBlank handler is running (it calls SYS_IO, not SYS_INT1), so frames
 * are counted from the LSPC line counter, which wraps once a frame, and the
 * pad edges are the ones that handler already read. */
#define HTP_LINES   12u
#define HTP_WIDTH   31u     /* a line holds up to 30 characters and its end */

static uint8_t htp_text_ok(const uint8_t *s)
{
    uint8_t i;
    for (i = 0; i < HTP_WIDTH; i++) {
        if (!s[i]) return 1;
        if (s[i] < 0x20u || s[i] > 0x7Eu) return 0;
    }
    return 0;
}

static uint8_t htp_len(const uint8_t *s)
{
    uint8_t n = 0;
    while (n < HTP_WIDTH && s[n]) n++;
    return n;
}

static uint8_t htp_valid(const uint8_t *d)
{
    uint32_t a = (uint32_t)d;
    uint8_t i;
    if (!d) return 0;
    /* in the cartridge's program ROM: the fixed bank or the switched one */
    if (!((a + 6u + 32u * (1u + HTP_LINES) <= 0x100000u) || (a >= 0x200000u && a + 6u + 32u * (1u + HTP_LINES) <= 0x300000u)))
        return 0;
    if (d[0] != 'H' || d[1] != 'T' || d[2] != 'P' || d[3] != '1') return 0;
    if (!d[4] || d[4] > HTP_LINES) return 0;
    if (!htp_text_ok(d + 6)) return 0;
    for (i = 0; i < d[4]; i++) if (!htp_text_ok(d + 38u + 32u * i)) return 0;
    return 1;
}

/* One frame: the line counter (bits 15..7 of LSPCMODE) going back down. */
static void htp_frame(void)
{
    uint16_t last = (uint16_t)(REG_LSPCMODE >> 7), now;
    uint16_t guard = 0;
    for (;;) {
        bios_watchdog();
        now = (uint16_t)(REG_LSPCMODE >> 7);
        if (now < last || ++guard > 60000u) break;
        last = now;
    }
}

void sys_howtoplay_c(const uint8_t *desc)
{
    uint16_t frames;
    uint8_t i, n, spacing, y;
    sys_fix_clear_c();
    sys_lsp_1st_c();
    bios_set_backdrop(COLOR_BLACK);
    bios_init_palette_banks();

    if (htp_valid(desc)) {
        const char *title = (const char *)(desc + 6);
        n = desc[4];
        frames = (uint16_t)((desc[5] ? desc[5] : 10u) * 60u);
        bios_fix_puts((uint8_t)((40u - htp_len(desc + 6)) >> 1), 3, title, 1);
        bios_fix_puts(6, 4, "============================", 2);
        spacing = (uint8_t)(n <= 9u ? 2u : 1u);
        y = 7;
        for (i = 0; i < n; i++) {
            bios_fix_puts(5, y, (const char *)(desc + 38u + 32u * i), (uint8_t)(1u + (i & 3u)));
            y = (uint8_t)(y + spacing);
        }
    } else {
        frames = 300u;
        bios_fix_puts(14, 3, "HOW TO PLAY", 1);
        bios_fix_puts(6, 4, "============================", 2);
        bios_fix_puts(6, 8, "JOYSTICK   MOVE", 3);
        bios_fix_puts(6, 11, "A B C D    THE GAME'S ACTIONS", 1);
        bios_fix_puts(6, 14, "START      PLAY", 2);
        bios_fix_puts(6, 18, "THIS GAME SHOWS ITS OWN", 4);
        bios_fix_puts(6, 20, "CONTROLS WHILE IT PLAYS", 4);
    }
    bios_fix_puts(8, 26, "PRESS A BUTTON TO PLAY", 2);

    while (frames--) {
        htp_frame();
        if (BIOS_P1CHANGE || (BIOS_STATCHANGE & (STAT_START1 | STAT_START2 | STAT_COIN1 | STAT_COIN2))) break;
    }
    sys_fix_clear_c();
}
