#include "bios.h"

/*
 * ============================================================================
 *  EagleBIOS eye-catcher
 *
 *  The old home-computer way: no sprites, no tile art, no palette ROM.  The
 *  wordmark is five block letters laid out on the FIX layer, every "pixel"
 *  a cell of the plain text font, so it draws the same from the system font
 *  on an MVS and from whatever cartridge font an AES has.  Letters land one
 *  at a time to a short chime, hold, then the ink cools to black.
 * ============================================================================
 */

/* 5 x 7 block letters, one row per byte, most significant bit on the left. */
static const uint8_t LETTER_E[7] = { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F };
static const uint8_t LETTER_A[7] = { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 };
static const uint8_t LETTER_G[7] = { 0x0F, 0x10, 0x10, 0x13, 0x11, 0x11, 0x0F };
static const uint8_t LETTER_L[7] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F };

static const struct {
    const uint8_t *rows;
    char ink;
} WORDMARK[5] = {
    { LETTER_E, 'E' }, { LETTER_A, 'A' }, { LETTER_G, 'G' }, { LETTER_L, 'L' }, { LETTER_E, 'E' },
};

enum {
    MARK_X = 5,          /* five letters, five cells each, one cell apart  */
    MARK_Y = 6,
    MARK_PAL = 2,        /* gold ink from bios_init_palette_banks()        */
    LINE_PAL = 7,        /* grey rule                                      */
    TEXT_PAL = 1,        /* white                                          */
    REVEAL_FRAMES = 9,   /* one letter every nine frames                   */
    HOLD_FRAMES = 60,
    COOL_STEPS = 8        /* eight steps of two shades: sixteen levels    */
};

static uint8_t skip_pressed(void)
{
    return (BIOS_P1CHANGE & (BTN_A | BTN_B | BTN_C | BTN_D)) ||
           (BIOS_STATCHANGE & (STAT_START1 | STAT_START2));
}

static void draw_letter(uint8_t index)
{
    const uint8_t *rows = WORDMARK[index].rows;
    uint8_t x0 = (uint8_t)(MARK_X + index * 6);

    for (uint8_t y = 0; y < 7; y++) {
        for (uint8_t x = 0; x < 5; x++) {
            if (rows[y] & (uint8_t)(0x10u >> x)) {
                bios_fix_putc((uint8_t)(x0 + x), (uint8_t)(MARK_Y + y), WORDMARK[index].ink, MARK_PAL);
            }
        }
    }
}

/* Darken one 16 colour bank by a fixed step on each channel. */
static void cool_bank(uint8_t bank, const uint16_t *colors, uint8_t step)
{
    uint16_t cooled[16];

    for (uint8_t i = 0; i < 16; i++) {
        uint16_t c = colors[i];
        uint16_t r = (uint16_t)((c >> 8) & 15u), g = (uint16_t)((c >> 4) & 15u), b = (uint16_t)(c & 15u);
        r = r > step ? (uint16_t)(r - step) : 0;
        g = g > step ? (uint16_t)(g - step) : 0;
        b = b > step ? (uint16_t)(b - step) : 0;
        cooled[i] = (r | g | b) ? (uint16_t)((r << 8) | (g << 4) | b) : COLOR_BLACK;
    }
    bios_set_palette(bank, cooled);
}

/*
 * Draw the wordmark and hold it.  Returns early on any button so a player
 * who has seen it a hundred times is never made to wait.
 */
void bios_eyecatcher(void)
{
    static const uint16_t gold[16] = {
        COLOR_BLACK, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD,
        COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD
    };
    static const uint16_t white[16] = {
        COLOR_BLACK, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE,
        COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE
    };
    static const uint16_t grey[16] = {
        COLOR_BLACK, COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY,
        COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY
    };
    uint8_t skipped = 0;

    sys_fix_clear_c();
    sys_lsp_1st_c();
    bios_set_backdrop(COLOR_BLACK);
    bios_set_palette(MARK_PAL, gold);
    bios_set_palette(TEXT_PAL, white);
    bios_set_palette(LINE_PAL, grey);
    bios_wait_vbl();

    for (uint8_t letter = 0; letter < 5 && !skipped; letter++) {
        draw_letter(letter);
        /* A tick per letter, from the system sound program when it is live. */
        if (BIOS_MVS_FLAG && !bios_cart_active) REG_SOUND = 4;
        for (uint8_t frame = 0; frame < REVEAL_FRAMES; frame++) {
            bios_wait_vbl();
            if (skip_pressed()) { skipped = 1; break; }
        }
    }

    bios_fix_puts(MARK_X, MARK_Y + 8, "-----------------------------", LINE_PAL);
    bios_fix_puts(12, MARK_Y + 10, "SYSTEM ROM  2026", TEXT_PAL);
    bios_fix_puts(12, MARK_Y + 12, "EAGLE SOFTWARE", TEXT_PAL);
    bios_fix_puts(14, MARK_Y + 15, BIOS_MVS_FLAG ? "MVS ARCADE" : "AES CONSOLE", LINE_PAL);

    for (uint16_t frame = 0; frame < HOLD_FRAMES && !skipped; frame++) {
        bios_wait_vbl();
        if (skip_pressed()) skipped = 1;
    }

    /* Cool the ink down to black instead of cutting to the game. */
    for (uint8_t step = 1; step <= COOL_STEPS; step++) {
        uint8_t amount = (uint8_t)(step * (16 / COOL_STEPS));
        cool_bank(MARK_PAL, gold, amount);
        cool_bank(TEXT_PAL, white, amount);
        cool_bank(LINE_PAL, grey, amount);
        bios_wait_vbl();
        bios_wait_vbl();
    }

    sys_fix_clear_c();
    bios_init_palette_banks();
    bios_wait_vbl();
}

/*
 * Power-on title screen: the firmware banner, the board it found and the
 * cartridge it is about to start.  Holds a moment; any button moves on.
 */
void bios_splash_show(void)
{
    sys_fix_clear_c();
    sys_lsp_1st_c();
    bios_set_backdrop(COLOR_BLACK);
    bios_init_palette_banks();
    bios_wait_vbl();

    bios_fix_puts(4,  5, "==================================", 2);
    bios_fix_puts(9,  7, "E A G L E   B I O S", 1);
    bios_fix_puts(6,  9, "OPEN SOURCE NEO-GEO FIRMWARE", 3);
    bios_fix_puts(4, 11, "==================================", 2);

    bios_fix_puts(7, 14, BIOS_MVS_FLAG ? "MODE: MVS ARCADE SYSTEM" : "MODE: AES CONSOLE SYSTEM", 1);
    bios_fix_puts(10, 16, "(C) 2026 EAGLE SOFTWARE", 2);

    if (bios_cart_valid()) {
        bios_fix_puts(8, 20, "CARTRIDGE NGH ID:  0x", 3);
        bios_fix_put_hex16(29, 20, CART_HEADER->ngh_id, 2);
        bios_fix_puts(12, 23, "SYSTEM INITIALIZED", 1);
    } else {
        bios_fix_puts(10, 20, "NO CARTRIDGE INSERTED", 4);
    }

    /* The Eagle fanfare, from the system sound program on an arcade board. */
    if (BIOS_MVS_FLAG && !bios_cart_active) REG_SOUND = 2;
    for (uint16_t frame = 0; frame < 66; frame++) {
        bios_wait_vbl();
        if (skip_pressed() || (BIOS_STATCHANGE & STAT_COIN1)) break;
    }

    sys_fix_clear_c();
    bios_wait_vbl();
}
