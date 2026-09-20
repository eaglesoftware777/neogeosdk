#include "bios.h"

/*
 * ============================================================================
 *  EagleBIOS Built-in Service & Test Menu Suite
 * ============================================================================
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

/* 1. System Information Screen */
static void menu_system_info(void)
{
    menu_draw_frame("[ SYSTEM HARDWARE INFORMATION ]");

    bios_fix_puts(4,  8, "MAIN CPU:      MOTOROLA 68000 @ 12.0 MHz", 1);
    bios_fix_puts(4, 10, "AUDIO CPU:     ZILOG Z80 @ 4.0 MHz", 1);
    bios_fix_puts(4, 12, "SOUND CHIP:    YAMAHA YM2610 (OPNB)", 1);
    bios_fix_puts(4, 14, "WORK RAM:      64 KBYTES (0x100000)", 1);
    bios_fix_puts(4, 16, "4096 ENTRIES / 65536 COLORS", 1);

    if (CART_HEADER->magic[0] == 'N' && CART_HEADER->magic[1] == 'E' &&
        CART_HEADER->magic[2] == 'O' && CART_HEADER->magic[3] == '-') {
        bios_fix_puts(4, 19, "CARTRIDGE:     CONNECTED (OK)", 2);
        bios_fix_puts(4, 21, "CART NGH ID:   0x", 3);
        bios_fix_put_hex16(21, 21, CART_HEADER->ngh_id, 2);
    } else {
        bios_fix_puts(4, 19, "CARTRIDGE:     NOT DETECTED", 4);
    }

    bios_fix_puts(6, 25, "PRESS BUTTON A TO RETURN", 2);

    for (;;) {
        bios_wait_vbl();
        if (BIOS_P1CHANGE & BTN_A) break;
    }
}

/* 2. Controller & Input Test */
static void menu_input_test(void)
{
    menu_draw_frame("[ CONTROLLER & INPUT TEST ]");
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

        /* Exit condition: Start 1 + Select 1 together */
        if ((BIOS_P1CURRENT & (BTN_C | BTN_D)) == (BTN_C | BTN_D)) {
            break;
        }
    }
}

/* 3. Color Bar Test */
static void menu_color_test(void)
{
    sys_fix_clear_c();
    sys_lsp_1st_c();

    /* Set palette bank 1 with vivid test colors */
    static const uint16_t bar_colors[16] = {
        COLOR_BLACK, COLOR_WHITE, COLOR_YELLOW, COLOR_CYAN,
        COLOR_GREEN, COLOR_MAGENTA, COLOR_RED, COLOR_BLUE,
        COLOR_MIDGRAY, COLOR_DARKGRAY, COLOR_GOLD, COLOR_WHITE,
        COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE
    };
    bios_set_palette(1, bar_colors);

    /* Draw color bars on FIX layer */
    for (uint8_t col = 0; col < 8; col++) {
        uint8_t x = (uint8_t)(4 + col * 4);
        for (uint8_t y = 4; y < 20; y++) {
            bios_fix_putc(x,     y, 0x01, (uint8_t)(col + 1));
            bios_fix_putc((uint8_t)(x+1), y, 0x01, (uint8_t)(col + 1));
            bios_fix_putc((uint8_t)(x+2), y, 0x01, (uint8_t)(col + 1));
            bios_fix_putc((uint8_t)(x+3), y, 0x01, (uint8_t)(col + 1));
        }
    }

    bios_fix_puts(8, 24, "COLOR PALETTE TEST - PRESS A", 1);

    for (;;) {
        bios_wait_vbl();
        if (BIOS_P1CHANGE & BTN_A) break;
    }
}

/* 4. Audio Sound Test */
static void menu_sound_test(void)
{
    uint8_t sfx_code = 2;
    menu_draw_frame("[ AUDIO / SOUND HARDWARE TEST ]");

    bios_fix_puts(4,  9, "SYSTEM SOUND CODE: 0x", 1);
    bios_fix_puts(4, 12, "LEFT / RIGHT:  CHANGE SOUND CODE", 3);
    bios_fix_puts(4, 14, "BUTTON A:      TRIGGER SOUND", 2);
    bios_fix_puts(4, 16, "BUTTON B:      STOP ALL SOUND", 4);
    bios_fix_puts(4, 22, "PRESS BUTTON C OR D TO EXIT", 1);

    for (;;) {
        bios_wait_vbl();

        bios_fix_put_hex16(26, 9, sfx_code, 2);

        if (BIOS_P1CHANGE & JOY_LEFT) {
            sfx_code = (uint8_t)(sfx_code > 2 ? sfx_code - 1 : 4);
        }
        if (BIOS_P1CHANGE & JOY_RIGHT) {
            sfx_code = (uint8_t)(sfx_code < 4 ? sfx_code + 1 : 2);
        }
        if (BIOS_P1CHANGE & BTN_A) {
            REG_SOUND = sfx_code;
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

/* 5. System Settings */
static void menu_settings(void)
{
    uint8_t selected = 0;

    for (;;) {
        menu_draw_frame("[ SYSTEM CONFIGURATION ]");

        bios_fix_puts(4,  9, selected == 0 ? "> SYSTEM MODE:"  : "  SYSTEM MODE:", selected == 0 ? 2 : 1);
        bios_fix_puts(22, 9, BIOS_MVS_FLAG ? "MVS (ARCADE)" : "AES (CONSOLE)", 3);

        bios_fix_puts(4, 12, selected == 1 ? "> REGION CODE:"  : "  REGION CODE:", selected == 1 ? 2 : 1);
        if (BIOS_COUNTRY_CODE == 0) bios_fix_puts(22, 12, "JAPAN", 3);
        else if (BIOS_COUNTRY_CODE == 1) bios_fix_puts(22, 12, "USA", 3);
        else bios_fix_puts(22, 12, "EUROPE", 3);

        bios_fix_puts(4, 15, selected == 2 ? "> FREE PLAY:"    : "  FREE PLAY:", selected == 2 ? 2 : 1);
        bios_fix_puts(22, 15, bios_free_play() ? "ENABLED" : "DISABLED", 3);

        bios_fix_puts(4, 18, selected == 3 ? "> RETURN TO MENU" : "  RETURN TO MENU", selected == 3 ? 2 : 1);

        bios_fix_puts(4, 24, "UP/DOWN: SELECT | A: TOGGLE/ENTER", 1);

        for (;;) {
            bios_wait_vbl();
    
            if (BIOS_P1CHANGE & JOY_UP) {
                selected = (uint8_t)(selected > 0 ? selected - 1 : 3);
                break;
            }
            if (BIOS_P1CHANGE & JOY_DOWN) {
                selected = (uint8_t)(selected < 3 ? selected + 1 : 0);
                break;
            }
            if (BIOS_P1CHANGE & BTN_A) {
                if (selected == 0) {
                    /* Physical mode is detected, not a writable DIP. */
                } else if (selected == 1) {
                    uint8_t ccode = (uint8_t)(BIOS_COUNTRY_CODE + 1);
                    if (ccode > 2) ccode = 0;
                    BIOS_COUNTRY_CODE = ccode;
                } else if (selected == 2) {
                    bios_free_play_override ^= 1;
                } else if (selected == 3) {
                    return;
                }
                break;
            }
        }
    }
}

/* Main Menu Navigation */
void bios_test_menu(void)
{
    uint8_t cur = 0;
    static const char *items[] = {
        "1. SYSTEM HARDWARE INFO",
        "2. CONTROLLER & INPUT TEST",
        "3. COLOR PALETTE & VIDEO TEST",
        "4. AUDIO & SOUND FX TEST",
        "5. SYSTEM CONFIGURATION",
        "6. BOOT CARTRIDGE GAME"
    };

    for (;;) {
        menu_draw_frame("[ MAIN SERVICE MENU ]");

        for (uint8_t i = 0; i < 6; i++) {
            uint8_t y = (uint8_t)(8 + i * 2);
            if (i == cur) {
                bios_fix_puts(4, y, ">", 2);
                bios_fix_puts(6, y, items[i], 2); /* Gold */
            } else {
                bios_fix_puts(4, y, " ", 1);
                bios_fix_puts(6, y, items[i], 1); /* White */
            }
        }

        bios_fix_puts(4, 24, "JOYSTICK: SELECT | BUTTON A: ENTER", 3);

        for (;;) {
            bios_wait_vbl();
    
            if (BIOS_P1CHANGE & JOY_UP) {
                cur = (uint8_t)(cur > 0 ? cur - 1 : 5);
                break;
            }
            if (BIOS_P1CHANGE & JOY_DOWN) {
                cur = (uint8_t)(cur < 5 ? cur + 1 : 0);
                break;
            }
            if ((BIOS_P1CHANGE & BTN_A) || (BIOS_STATCHANGE & STAT_START1)) {
                if (cur == 0) menu_system_info();
                else if (cur == 1) menu_input_test();
                else if (cur == 2) menu_color_test();
                else if (cur == 3) menu_sound_test();
                else if (cur == 4) menu_settings();
                else if (cur == 5) return;
                break;
            }
        }
    }
}
