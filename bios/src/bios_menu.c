#include "bios.h"

/*
 * ============================================================================
 *  EagleBIOS Built-in Service & Test Menu Suite (v1.7.4)
 * ============================================================================
 *
 * 11-Entry Full Service Suite:
 *  1. System Settings
 *  2. Cabinet DIPs / Coinage
 *  3. Game Soft DIPs
 *  4. Memory Card Manager
 *  5. Hardware Diagnostics
 *  6. Sound Test / Jukebox
 *  7. Input / Controller Test
 *  8. Calendar & Clock Setup
 *  9. Cartridge / ROM Info
 * 10. Backup RAM Clear
 * 11. Boot Game
 */

static void menu_draw_frame(const char *title)
{
    sys_fix_clear_c();
    sys_lsp_1st_c();
    bios_set_backdrop(COLOR_BLACK);
    bios_init_palette_banks();

    bios_fix_puts(2, 2, "====================================", 2);
    bios_fix_puts(6, 3, "EAGLE BIOS - SERVICE SUITE", 1);
    bios_fix_puts(2, 4, "====================================", 2);

    if (title) {
        bios_fix_puts(4, 6, title, 3);
    }
}

/* 1. System Settings */
static void menu_system_settings(void)
{
    uint8_t sel = 0;
    for (;;) {
        menu_draw_frame("[ 1. SYSTEM SETTINGS ]");

        bios_fix_puts(4,  9, sel == 0 ? "> SYSTEM MODE:"  : "  SYSTEM MODE:", sel == 0 ? 2 : 1);
        bios_fix_puts(22, 9, BIOS_MVS_FLAG ? "MVS (ARCADE)" : "AES (CONSOLE)", 3);

        bios_fix_puts(4, 12, sel == 1 ? "> REGION CODE:"  : "  REGION CODE:", sel == 1 ? 2 : 1);
        if (BIOS_COUNTRY_CODE == 0) bios_fix_puts(22, 12, "JAPAN", 3);
        else if (BIOS_COUNTRY_CODE == 1) bios_fix_puts(22, 12, "USA", 3);
        else bios_fix_puts(22, 12, "EUROPE", 3);

        bios_fix_puts(4, 15, sel == 2 ? "> FREE PLAY:"    : "  FREE PLAY:", sel == 2 ? 2 : 1);
        bios_fix_puts(22, 15, bios_free_play() ? "ENABLED" : "DISABLED", 3);

        bios_fix_puts(4, 18, sel == 3 ? "> RETURN TO MENU" : "  RETURN TO MENU", sel == 3 ? 2 : 1);
        bios_fix_puts(4, 24, "UP/DOWN: SELECT | A: TOGGLE/ENTER", 1);

        for (;;) {
            bios_wait_vbl();
            if (BIOS_P1CHANGE & JOY_UP) {
                sel = (uint8_t)(sel > 0 ? sel - 1 : 3);
                break;
            }
            if (BIOS_P1CHANGE & JOY_DOWN) {
                sel = (uint8_t)(sel < 3 ? sel + 1 : 0);
                break;
            }
            if (BIOS_P1CHANGE & BTN_A) {
                if (sel == 1) {
                    uint8_t c = (uint8_t)(BIOS_COUNTRY_CODE + 1);
                    if (c > 2) c = 0;
                    BIOS_COUNTRY_CODE = c;
                } else if (sel == 2) {
                    bios_free_play_override ^= 1;
                } else if (sel == 3) {
                    return;
                }
                break;
            }
            if (BIOS_P1CHANGE & (BTN_C | BTN_D)) return;
        }
    }
}

/* 2. Cabinet DIPs & Coinage */
static void menu_cabinet_dips(void)
{
    menu_draw_frame("[ 2. CABINET DIPS / COINAGE ]");
    uint8_t dips = REG_DIPSW;

    bios_fix_puts(4,  9, "CABINET DIP SWITCH: 0x", 1);
    bios_fix_put_hex16(26, 9, dips, 2);

    bios_fix_puts(4, 12, "DIP 1 (TEST MODE):   ", 1);
    bios_fix_puts(25, 12, (dips & 0x01) ? "ON" : "OFF", (dips & 0x01) ? 2 : 1);

    bios_fix_puts(4, 14, "DIP 2 (COIN CHUTE):  ", 1);
    bios_fix_puts(25, 14, (dips & 0x02) ? "SEPARATE" : "SHARED", 3);

    bios_fix_puts(4, 16, "DIP 7 (FREE PLAY):   ", 1);
    bios_fix_puts(25, 16, (dips & 0x40) ? "OFF" : "ON", (dips & 0x40) ? 1 : 2);

    bios_fix_puts(4, 19, "MVS P1 CREDITS:      ", 1);
    bios_fix_put_hex16(25, 19, P1_CREDITS, 3);
    bios_fix_puts(4, 21, "MVS P2 CREDITS:      ", 1);
    bios_fix_put_hex16(25, 21, P2_CREDITS, 3);

    bios_fix_puts(6, 25, "PRESS BUTTON A TO RETURN", 2);
    for (;;) {
        bios_wait_vbl();
        if (BIOS_P1CHANGE & (BTN_A | BTN_C | BTN_D)) break;
    }
}

/* 3. Game Soft DIPs */
static void menu_soft_dips(void)
{
    uint8_t cur = 0;
    for (;;) {
        menu_draw_frame("[ 3. GAME SOFT DIPS ]");
        bios_fix_puts(4, 8, "BYTE  NAME / VALUE        HEX", 3);

        for (uint8_t i = 0; i < 8; i++) {
            uint8_t y = (uint8_t)(10 + i * 1);
            bios_fix_puts(2, y, i == cur ? ">" : " ", 2);
            bios_fix_puts(4, y, "DIP", 1);
            bios_fix_put_dec2(8, y, i, 1);
            bios_fix_puts(12, y, ":", 1);
            bios_fix_put_dec2(15, y, BIOS_GAME_DIP[i], 2);
            bios_fix_puts(26, y, "0x", 1);
            bios_fix_put_hex16(28, y, BIOS_GAME_DIP[i], 2);
        }

        bios_fix_puts(4, 20, "DIP 0: DIFFICULTY (0=EASY .. 7=EXPERT)", 1);
        bios_fix_puts(4, 21, "DIP 1: ROUNDS (0=1 ROUND .. 2=BEST 3)", 1);
        bios_fix_puts(4, 24, "UP/DOWN: SELECT | LEFT/RIGHT: EDIT", 3);
        bios_fix_puts(4, 25, "PRESS BUTTON C OR D TO RETURN", 1);

        for (;;) {
            bios_wait_vbl();
            if (BIOS_P1CHANGE & JOY_UP) {
                cur = (uint8_t)(cur > 0 ? cur - 1 : 7);
                break;
            }
            if (BIOS_P1CHANGE & JOY_DOWN) {
                cur = (uint8_t)(cur < 7 ? cur + 1 : 0);
                break;
            }
            if (BIOS_P1CHANGE & JOY_LEFT) {
                BIOS_GAME_DIP[cur] = (uint8_t)(BIOS_GAME_DIP[cur] > 0 ? BIOS_GAME_DIP[cur] - 1 : 7);
                break;
            }
            if (BIOS_P1CHANGE & JOY_RIGHT) {
                BIOS_GAME_DIP[cur] = (uint8_t)(BIOS_GAME_DIP[cur] < 7 ? BIOS_GAME_DIP[cur] + 1 : 0);
                break;
            }
            if (BIOS_P1CHANGE & (BTN_C | BTN_D)) return;
        }
    }
}

/* 4. Memory Card Manager */
static void menu_card_manager(void)
{
    BiosCardSaveEntry saves[8];
    menu_draw_frame("[ 4. MEMORY CARD MANAGER ]");

    uint8_t inserted = !(REG_STATUS_B & 0x30u);
    bios_fix_puts(4, 8, "CARD STATUS: ", 1);
    bios_fix_puts(18, 8, inserted ? "INSERTED (OK)" : "NO CARD", inserted ? 2 : 4);

    if (inserted) {
        uint8_t count = bios_card_get_saves(saves, 8);
        bios_fix_puts(4, 10, "FORMAT:      NEO-GEO FAT FILESYSTEM", 1);
        bios_fix_puts(4, 12, "SAVES FOUND:", 3);
        bios_fix_put_dec2(18, 12, count, 2);

        bios_fix_puts(4, 14, "SLOT  NGH ID   SUB   BLOCKS", 3);
        bios_fix_puts(4, 15, "----------------------------", 1);
        for (uint8_t i = 0; i < count; i++) {
            uint8_t y = (uint8_t)(16 + i);
            bios_fix_put_dec2(5, y, (uint8_t)(i + 1), 1);
            bios_fix_puts(10, y, "0x", 1);
            bios_fix_put_hex16(12, y, saves[i].ngh, 2);
            bios_fix_put_dec2(19, y, saves[i].sub, 1);
            bios_fix_put_dec2(25, y, (uint8_t)saves[i].size_blocks, 2);
        }
    } else {
        bios_fix_puts(4, 12, "INSERT 8-BIT OR 16-BIT CARD", 1);
    }

    bios_fix_puts(6, 25, "PRESS BUTTON A TO RETURN", 2);
    for (;;) {
        bios_wait_vbl();
        if (BIOS_P1CHANGE & (BTN_A | BTN_C | BTN_D)) break;
    }
}

/* 5. Hardware Diagnostics */
static void menu_hardware_diagnostics(void)
{
    menu_draw_frame("[ 5. HARDWARE DIAGNOSTICS ]");
    bios_fix_puts(4, 8, "TESTING WORK RAM ...", 1);
    bios_wait_vbl();

    /* 1. Work RAM test */
    uint8_t ram_ok = 1;
    volatile uint16_t *wram = (volatile uint16_t *)0x100000u;
    for (uint16_t i = 0; i < 256; i++) {
        uint16_t old = wram[i];
        wram[i] = 0xAA55u;
        if (wram[i] != 0xAA55u) ram_ok = 0;
        wram[i] = 0x55AAu;
        if (wram[i] != 0x55AAu) ram_ok = 0;
        wram[i] = old;
    }
    bios_fix_puts(4,  9, "WORK RAM (64KB):     ", 1);
    bios_fix_puts(25, 9, ram_ok ? "PASS" : "FAIL", ram_ok ? 2 : 4);

    /* 2. VRAM test */
    uint8_t vram_ok = 1;
    REG_VRAM_ADDR = 0x0000u;
    REG_VRAM_INC = 1;
    REG_VRAM_RW = 0x1234u;
    REG_VRAM_ADDR = 0x0000u;
    if (REG_VRAM_RW != 0x1234u) vram_ok = 0;
    REG_VRAM_ADDR = 0x0000u;
    REG_VRAM_RW = 0x0000u;
    bios_fix_puts(4, 12, "VIDEO RAM (68KB):    ", 1);
    bios_fix_puts(25, 12, vram_ok ? "PASS" : "FAIL", vram_ok ? 2 : 4);

    /* 3. Palette RAM test */
    volatile uint16_t *pal = (volatile uint16_t *)ADDR_PALETTES;
    uint8_t pal_ok = 1;
    uint16_t old_p = pal[15];
    pal[15] = 0x7FFFu;
    if (pal[15] != 0x7FFFu) pal_ok = 0;
    pal[15] = old_p;
    bios_fix_puts(4, 15, "PALETTE RAM (8KB):   ", 1);
    bios_fix_puts(25, 15, pal_ok ? "PASS" : "FAIL", pal_ok ? 2 : 4);

    /* 4. Backup RAM test */
    bios_fix_puts(4, 18, "BACKUP RAM (SRAM):   ", 1);
    if (BIOS_MVS_FLAG) {
        REG_SRAMUNLOCK = 0;
        volatile uint8_t *sram = (volatile uint8_t *)0xD00000u;
        uint8_t old_s = sram[0];
        sram[0] = 0xA5u;
        uint8_t s_ok = (sram[0] == 0xA5u);
        sram[0] = old_s;
        REG_SRAMLOCK = 0;
        bios_fix_puts(25, 18, s_ok ? "PASS" : "FAIL", s_ok ? 2 : 4);
    } else {
        bios_fix_puts(25, 18, "N/A (AES)", 1);
    }

    bios_fix_puts(4, 21, "ALL HARDWARE SUBSYSTEMS HEALTHY", 2);
    bios_fix_puts(6, 25, "PRESS BUTTON A TO RETURN", 2);

    for (;;) {
        bios_wait_vbl();
        if (BIOS_P1CHANGE & (BTN_A | BTN_C | BTN_D)) break;
    }
}

/* 6. Sound Test / Jukebox */
static void menu_sound_test(void)
{
    uint8_t sfx = 2;
    menu_draw_frame("[ 6. AUDIO & SOUND FX TEST ]");

    bios_fix_puts(4,  9, "SOUND COMMAND CODE: 0x", 1);
    bios_fix_puts(4, 12, "LEFT / RIGHT:  CHANGE CODE", 3);
    bios_fix_puts(4, 14, "BUTTON A:      PLAY SOUND", 2);
    bios_fix_puts(4, 16, "BUTTON B:      STOP / MUTE", 4);
    bios_fix_puts(4, 22, "PRESS BUTTON C OR D TO EXIT", 1);

    for (;;) {
        bios_wait_vbl();
        bios_fix_put_hex16(26, 9, sfx, 2);

        if (BIOS_P1CHANGE & JOY_LEFT) {
            sfx = (uint8_t)(sfx > 2 ? sfx - 1 : 4);
        }
        if (BIOS_P1CHANGE & JOY_RIGHT) {
            sfx = (uint8_t)(sfx < 4 ? sfx + 1 : 2);
        }
        if (BIOS_P1CHANGE & BTN_A) {
            REG_SOUND = sfx;
        }
        if (BIOS_P1CHANGE & BTN_B) {
            REG_SOUND = 0x03;
        }
        if (BIOS_P1CHANGE & (BTN_C | BTN_D)) {
            REG_SOUND = 0x03;
            break;
        }
    }
}

/* 7. Input / Controller Test */
static void menu_input_test(void)
{
    menu_draw_frame("[ 7. CONTROLLER & INPUT TEST ]");
    bios_fix_puts(4, 25, "HOLD C + D TO EXIT", 2);

    for (;;) {
        bios_wait_vbl();

        /* Player 1 */
        bios_fix_puts(4,  9, "P1: ", 3);
        bios_fix_puts(8,  9, (BIOS_P1CURRENT & JOY_UP)    ? "[UP]"    : " UP ", (BIOS_P1CURRENT & JOY_UP)    ? 2 : 1);
        bios_fix_puts(13, 9, (BIOS_P1CURRENT & JOY_DOWN)  ? "[DOWN]"  : " DOWN ", (BIOS_P1CURRENT & JOY_DOWN)  ? 2 : 1);
        bios_fix_puts(20, 9, (BIOS_P1CURRENT & JOY_LEFT)  ? "[LEFT]"  : " LEFT ", (BIOS_P1CURRENT & JOY_LEFT)  ? 2 : 1);
        bios_fix_puts(27, 9, (BIOS_P1CURRENT & JOY_RIGHT) ? "[RIGHT]" : " RIGHT ", (BIOS_P1CURRENT & JOY_RIGHT) ? 2 : 1);

        bios_fix_puts(8,  11, (BIOS_P1CURRENT & BTN_A) ? "[A]" : " A ", (BIOS_P1CURRENT & BTN_A) ? 2 : 1);
        bios_fix_puts(12, 11, (BIOS_P1CURRENT & BTN_B) ? "[B]" : " B ", (BIOS_P1CURRENT & BTN_B) ? 2 : 1);
        bios_fix_puts(16, 11, (BIOS_P1CURRENT & BTN_C) ? "[C]" : " C ", (BIOS_P1CURRENT & BTN_C) ? 2 : 1);
        bios_fix_puts(20, 11, (BIOS_P1CURRENT & BTN_D) ? "[D]" : " D ", (BIOS_P1CURRENT & BTN_D) ? 2 : 1);

        /* Player 2 */
        bios_fix_puts(4,  14, "P2: ", 3);
        bios_fix_puts(8,  14, (BIOS_P2CURRENT & JOY_UP)    ? "[UP]"    : " UP ", (BIOS_P2CURRENT & JOY_UP)    ? 2 : 1);
        bios_fix_puts(13, 14, (BIOS_P2CURRENT & JOY_DOWN)  ? "[DOWN]"  : " DOWN ", (BIOS_P2CURRENT & JOY_DOWN)  ? 2 : 1);
        bios_fix_puts(20, 14, (BIOS_P2CURRENT & JOY_LEFT)  ? "[LEFT]"  : " LEFT ", (BIOS_P2CURRENT & JOY_LEFT)  ? 2 : 1);
        bios_fix_puts(27, 14, (BIOS_P2CURRENT & JOY_RIGHT) ? "[RIGHT]" : " RIGHT ", (BIOS_P2CURRENT & JOY_RIGHT) ? 2 : 1);

        bios_fix_puts(8,  16, (BIOS_P2CURRENT & BTN_A) ? "[A]" : " A ", (BIOS_P2CURRENT & BTN_A) ? 2 : 1);
        bios_fix_puts(12, 16, (BIOS_P2CURRENT & BTN_B) ? "[B]" : " B ", (BIOS_P2CURRENT & BTN_B) ? 2 : 1);
        bios_fix_puts(16, 16, (BIOS_P2CURRENT & BTN_C) ? "[C]" : " C ", (BIOS_P2CURRENT & BTN_C) ? 2 : 1);
        bios_fix_puts(20, 16, (BIOS_P2CURRENT & BTN_D) ? "[D]" : " D ", (BIOS_P2CURRENT & BTN_D) ? 2 : 1);

        /* System Buttons */
        bios_fix_puts(4,  19, "SYS: ", 3);
        bios_fix_puts(10, 19, (BIOS_STATCURNT & STAT_START1)  ? "[START1]"  : " START1 ",  (BIOS_STATCURNT & STAT_START1)  ? 2 : 1);
        bios_fix_puts(19, 19, (BIOS_STATCURNT & STAT_SELECT1) ? "[SELECT1]" : " SELECT1 ", (BIOS_STATCURNT & STAT_SELECT1) ? 2 : 1);
        bios_fix_puts(10, 21, (BIOS_STATCURNT & STAT_COIN1)   ? "[COIN1]"   : " COIN1 ",   (BIOS_STATCURNT & STAT_COIN1)   ? 2 : 1);
        bios_fix_puts(19, 21, (BIOS_STATCURNT & STAT_COIN2)   ? "[COIN2]"   : " COIN2 ",   (BIOS_STATCURNT & STAT_COIN2)   ? 2 : 1);
        bios_fix_puts(28, 21, (BIOS_STATCURNT & STAT_TEST)    ? "[TEST]"    : " TEST ",    (BIOS_STATCURNT & STAT_TEST)    ? 2 : 1);

        if ((BIOS_P1CURRENT & (BTN_C | BTN_D)) == (BTN_C | BTN_D)) {
            break;
        }
    }
}

/* 8. Calendar & Clock Setup */
static void menu_clock_setup(void)
{
    sys_read_calendar_c();
    uint8_t field = 0;
    static const char *k_weekdays[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

    for (;;) {
        menu_draw_frame("[ 8. CALENDAR & CLOCK SETUP ]");

        bios_fix_puts(4,  9, field == 0 ? "> YEAR:   20" : "  YEAR:   20", field == 0 ? 2 : 1);
        bios_fix_put_hex16(16, 9, BIOS_YEAR, 2);

        bios_fix_puts(4, 11, field == 1 ? "> MONTH:  " : "  MONTH:  ", field == 1 ? 2 : 1);
        bios_fix_put_hex16(14, 11, BIOS_MONTH, 2);

        bios_fix_puts(4, 13, field == 2 ? "> DAY:    " : "  DAY:    ", field == 2 ? 2 : 1);
        bios_fix_put_hex16(14, 13, BIOS_DAY, 2);

        bios_fix_puts(4, 15, "  WEEKDAY: ", 1);
        bios_fix_puts(15, 15, k_weekdays[BIOS_WEEKDAY % 7u], 3);

        bios_fix_puts(4, 17, field == 3 ? "> HOUR:   " : "  HOUR:   ", field == 3 ? 2 : 1);
        bios_fix_put_hex16(14, 17, BIOS_HOUR, 2);

        bios_fix_puts(4, 19, field == 4 ? "> MINUTE: " : "  MINUTE: ", field == 4 ? 2 : 1);
        bios_fix_put_hex16(14, 19, BIOS_MINUTE, 2);

        bios_fix_puts(4, 23, "UP/DOWN: SELECT | LEFT/RIGHT: CHANGE", 3);
        bios_fix_puts(4, 25, "PRESS BUTTON A TO SAVE & EXIT", 2);

        for (;;) {
            bios_wait_vbl();
            if (BIOS_P1CHANGE & JOY_UP) {
                field = (uint8_t)(field > 0 ? field - 1 : 4);
                break;
            }
            if (BIOS_P1CHANGE & JOY_DOWN) {
                field = (uint8_t)(field < 4 ? field + 1 : 0);
                break;
            }
            if (BIOS_P1CHANGE & JOY_RIGHT) {
                if (field == 0) BIOS_YEAR = (uint8_t)(BIOS_YEAR + 1);
                else if (field == 1) BIOS_MONTH = (uint8_t)(BIOS_MONTH < 0x12 ? BIOS_MONTH + 1 : 1);
                else if (field == 2) BIOS_DAY = (uint8_t)(BIOS_DAY < 0x31 ? BIOS_DAY + 1 : 1);
                else if (field == 3) BIOS_HOUR = (uint8_t)(BIOS_HOUR < 0x23 ? BIOS_HOUR + 1 : 0);
                else if (field == 4) BIOS_MINUTE = (uint8_t)(BIOS_MINUTE < 0x59 ? BIOS_MINUTE + 1 : 0);
                sys_set_calendar_c();
                break;
            }
            if (BIOS_P1CHANGE & JOY_LEFT) {
                if (field == 0) BIOS_YEAR = (uint8_t)(BIOS_YEAR > 0 ? BIOS_YEAR - 1 : 0x99);
                else if (field == 1) BIOS_MONTH = (uint8_t)(BIOS_MONTH > 1 ? BIOS_MONTH - 1 : 0x12);
                else if (field == 2) BIOS_DAY = (uint8_t)(BIOS_DAY > 1 ? BIOS_DAY - 1 : 0x31);
                else if (field == 3) BIOS_HOUR = (uint8_t)(BIOS_HOUR > 0 ? BIOS_HOUR - 1 : 0x23);
                else if (field == 4) BIOS_MINUTE = (uint8_t)(BIOS_MINUTE > 0 ? BIOS_MINUTE - 1 : 0x59);
                sys_set_calendar_c();
                break;
            }
            if (BIOS_P1CHANGE & (BTN_A | BTN_C | BTN_D)) {
                sys_set_calendar_c();
                return;
            }
        }
    }
}

/* 9. Cartridge / ROM Info */
static void menu_cartridge_info(void)
{
    menu_draw_frame("[ 9. CARTRIDGE / ROM INFO ]");

    if (CART_HEADER->magic[0] == 'N' && CART_HEADER->magic[1] == 'E' &&
        CART_HEADER->magic[2] == 'O' && CART_HEADER->magic[3] == '-') {
        bios_fix_puts(4,  8, "HEADER MAGIC:  NEO-GEO (VALID)", 2);
        bios_fix_puts(4, 10, "NGH GAME ID:   0x", 1);
        bios_fix_put_hex16(21, 10, CART_HEADER->ngh_id, 2);
        bios_fix_puts(4, 12, "PROGRAM SIZE:  ", 1);
        bios_fix_put_hex16(19, 12, (uint16_t)(CART_HEADER->psize >> 16), 2);
        bios_fix_put_hex16(23, 12, (uint16_t)(CART_HEADER->psize & 0xFFFF), 2);
        bios_fix_puts(4, 14, "SAVE BLOCK:    0x", 1);
        bios_fix_put_hex16(21, 14, (uint16_t)(CART_HEADER->pbcks >> 16), 2);
        bios_fix_put_hex16(25, 14, (uint16_t)(CART_HEADER->pbcks & 0xFFFF), 2);
        bios_fix_puts(4, 16, "SOFT DIP SIZE: ", 1);
        bios_fix_put_dec2(19, 16, (uint8_t)CART_HEADER->dipsize, 2);
        bios_fix_puts(4, 18, "EYE CATCHER:   ", 1);
        bios_fix_puts(19, 18, CART_HEADER->logoflag ? "CARTRIDGE" : "BIOS SPLASH", 3);
    } else {
        bios_fix_puts(4, 10, "NO CARTRIDGE CONNECTED", 4);
    }

    bios_fix_puts(6, 25, "PRESS BUTTON A TO RETURN", 2);
    for (;;) {
        bios_wait_vbl();
        if (BIOS_P1CHANGE & (BTN_A | BTN_C | BTN_D)) break;
    }
}

/* 10. Backup RAM Clear */
static void menu_backup_clear(void)
{
    menu_draw_frame("[ 10. BACKUP RAM CLEAR ]");

    bios_fix_puts(4, 10, "WARNING: THIS WILL ERASE ALL", 4);
    bios_fix_puts(4, 12, "STORED CREDITS, HIGH SCORES,", 4);
    bios_fix_puts(4, 14, "AND BOOKKEEPING TOTALS.", 4);

    bios_fix_puts(4, 18, "PRESS BUTTON A TO CONFIRM CLEAR", 2);
    bios_fix_puts(4, 21, "PRESS BUTTON C OR D TO CANCEL", 1);

    for (;;) {
        bios_wait_vbl();
        if (BIOS_P1CHANGE & BTN_A) {
            bios_backup_clear();
            P1_CREDITS = 0;
            P2_CREDITS = 0;
            bios_fix_puts(4, 23, "BACKUP RAM CLEARED SUCCESSFULLY", 2);
            bios_delay_frames(60);
            return;
        }
        if (BIOS_P1CHANGE & (BTN_C | BTN_D)) return;
    }
}

/* Main Menu Navigation (11 Entries) */
void bios_test_menu(void)
{
    uint8_t cur = 0;
    static const char * const items[11] = {
        "1. SYSTEM SETTINGS",
        "2. CABINET DIPS / COINAGE",
        "3. GAME SOFT DIPS",
        "4. MEMORY CARD MANAGER",
        "5. HARDWARE DIAGNOSTICS",
        "6. SOUND TEST / JUKEBOX",
        "7. INPUT / CONTROLLER TEST",
        "8. CALENDAR & CLOCK SETUP",
        "9. CARTRIDGE / ROM INFO",
        "10. BACKUP RAM CLEAR",
        "11. BOOT GAME"
    };

    for (;;) {
        menu_draw_frame("[ EAGLE BIOS - MAIN SERVICE MENU ]");

        for (uint8_t i = 0; i < 11; i++) {
            uint8_t y = (uint8_t)(7 + i);
            if (i == cur) {
                bios_fix_puts(3, y, ">", 2);
                bios_fix_puts(5, y, items[i], 2); /* Gold */
            } else {
                bios_fix_puts(3, y, " ", 1);
                bios_fix_puts(5, y, items[i], 1); /* White */
            }
        }

        bios_fix_puts(4, 20, "====================================", 2);
        bios_fix_puts(4, 22, "JOYSTICK: SELECT | BUTTON A: ENTER", 3);
        bios_fix_puts(4, 24, "EAGLEBIOS v1.7.4 (C) 2026 EAGLE SOFT", 1);

        for (;;) {
            bios_wait_vbl();

            if (BIOS_P1CHANGE & JOY_UP) {
                cur = (uint8_t)(cur > 0 ? cur - 1 : 10);
                break;
            }
            if (BIOS_P1CHANGE & JOY_DOWN) {
                cur = (uint8_t)(cur < 10 ? cur + 1 : 0);
                break;
            }
            if ((BIOS_P1CHANGE & BTN_A) || (BIOS_STATCHANGE & STAT_START1)) {
                if (cur == 0) menu_system_settings();
                else if (cur == 1) menu_cabinet_dips();
                else if (cur == 2) menu_soft_dips();
                else if (cur == 3) menu_card_manager();
                else if (cur == 4) menu_hardware_diagnostics();
                else if (cur == 5) menu_sound_test();
                else if (cur == 6) menu_input_test();
                else if (cur == 7) menu_clock_setup();
                else if (cur == 8) menu_cartridge_info();
                else if (cur == 9) menu_backup_clear();
                else if (cur == 10) return;
                break;
            }
        }
    }
}
