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
    MARK_PAL = 2,        /* gold ink                                       */
    DIM_PAL = 6,         /* the edge-on shade of a turning plate           */
    LINE_PAL = 7,        /* grey rule                                      */
    TEXT_PAL = 1,        /* white                                          */
    TURN_FRAMES = 2,     /* frames per step of a plate's half turn        */
    TURN_STAGGER = 8,    /* frames between one plate starting and the next */
    HOLD_FRAMES = 180,   /* the wordmark stays three seconds               */
    GLINT_STEP = 4,      /* frames the glint rests on each plate           */
    GLINT_PERIOD = 90,   /* one glint every second and a half              */
    COOL_STEPS = 8       /* eight steps of two shades: sixteen levels      */
};

static uint8_t skip_pressed(void)
{
    return (BIOS_P1CHANGE & (BTN_A | BTN_B | BTN_C | BTN_D)) ||
           (BIOS_STATCHANGE & (STAT_START1 | STAT_START2));
}

/*
 * Draw one letter squeezed to `width` cells (0..5) around its own centre
 * line, mirrored when it is showing its back: a 5 x 7 plate turning on a
 * vertical axis.  The narrow, edge-on frames take the dim ink.
 */
/* Which of the five source columns each cell shows at a given width. */
static const uint8_t SQUEEZE[6][5] = {
    { 0, 0, 0, 0, 0 }, { 2, 0, 0, 0, 0 }, { 1, 3, 0, 0, 0 },
    { 0, 2, 4, 0, 0 }, { 0, 1, 3, 4, 0 }, { 0, 1, 2, 3, 4 },
};

static void draw_letter_turned(uint8_t index, uint8_t width, uint8_t mirrored, uint8_t pal)
{
    const uint8_t *rows = WORDMARK[index].rows;
    uint8_t x0 = (uint8_t)(MARK_X + index * 6);
    uint8_t left = (uint8_t)(x0 + (5 - width) / 2);

    for (uint8_t y = 0; y < 7; y++) {
        for (uint8_t x = 0; x < 5; x++) bios_fix_putc((uint8_t)(x0 + x), (uint8_t)(MARK_Y + y), ' ', 0);
        for (uint8_t j = 0; j < width; j++) {
            uint8_t source = SQUEEZE[width][j];
            if (mirrored) source = (uint8_t)(4 - source);
            if (rows[y] & (uint8_t)(0x10u >> source)) {
                bios_fix_putc((uint8_t)(left + j), (uint8_t)(MARK_Y + y), WORDMARK[index].ink, pal);
            }
        }
    }
}

static void draw_letter(uint8_t index)
{
    draw_letter_turned(index, 5, 0, MARK_PAL);
}

/* One half turn: the back of the plate narrows to an edge, the face opens. */
static const int8_t TURN_STEPS[10] = { -5, -4, -3, -2, -1, 1, 2, 3, 4, 5 };

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
 * The eye-catcher.  Five plates turn into place one after another to the
 * fanfare, a glint runs along the wordmark while it holds, then the ink
 * cools to black.  Any button ends it early.
 */
void bios_eyecatcher(void)
{
    static const uint16_t gold[16] = {
        COLOR_BLACK, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD,
        COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD
    };
    static const uint16_t dim[16] = {
        COLOR_BLACK, 0x4860u, 0x4860u, 0x4860u, 0x4860u, 0x4860u, 0x4860u, 0x4860u,
        0x4860u, 0x4860u, 0x4860u, 0x4860u, 0x4860u, 0x4860u, 0x4860u, 0x4860u
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
    bios_set_palette(DIM_PAL, dim);
    bios_set_palette(TEXT_PAL, white);
    bios_set_palette(LINE_PAL, grey);
    bios_wait_vbl();

    /* The fanfare, from the system sound program when it is in charge. */
    if (BIOS_MVS_FLAG && !bios_cart_active) REG_SOUND = 2;

    /* Each plate starts its half turn TURN_STAGGER frames after the last;
     * every step of the turn lasts TURN_FRAMES frames. */
    for (uint8_t frame = 0; frame < 10 * TURN_FRAMES + 4 * TURN_STAGGER && !skipped; frame++) {
        for (uint8_t letter = 0; letter < 5; letter++) {
            int16_t local = (int16_t)frame - (int16_t)(letter * TURN_STAGGER);
            if (local < 0 || local % TURN_FRAMES) continue;
            uint8_t step = (uint8_t)(local / TURN_FRAMES);
            if (step >= 10) continue;
            int8_t turn = TURN_STEPS[step];
            uint8_t width = (uint8_t)(turn < 0 ? -turn : turn);
            draw_letter_turned(letter, width, turn < 0, width <= 2 ? DIM_PAL : MARK_PAL);
        }
        bios_wait_vbl();
        if (skip_pressed()) skipped = 1;
    }
    for (uint8_t letter = 0; letter < 5; letter++) draw_letter(letter);

    bios_fix_puts(MARK_X, MARK_Y + 8, "-----------------------------", LINE_PAL);
    bios_fix_puts(12, MARK_Y + 10, "SYSTEM ROM  2026", TEXT_PAL);
    bios_fix_puts(12, MARK_Y + 12, "EAGLE SOFTWARE", TEXT_PAL);

    /* Hold, with a glint running left to right across the plates. */
    for (uint16_t frame = 0; frame < HOLD_FRAMES && !skipped; frame++) {
        if ((frame % GLINT_PERIOD) < 5 * GLINT_STEP && (frame % GLINT_STEP) == 0) {
            uint8_t lit = (uint8_t)((frame % GLINT_PERIOD) / GLINT_STEP);
            if (lit) draw_letter_turned((uint8_t)(lit - 1), 5, 0, MARK_PAL);
            draw_letter_turned(lit, 5, 0, TEXT_PAL);
        } else if ((frame % GLINT_PERIOD) == 5 * GLINT_STEP) {
            draw_letter(4);
        }
        bios_wait_vbl();
        if (skip_pressed()) skipped = 1;
    }
    for (uint8_t letter = 0; letter < 5; letter++) draw_letter(letter);

    /* Cool the ink down to black instead of cutting away. */
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
    bios_fix_puts(8,  9, "NEO-GEO SYSTEM FIRMWARE", 3);
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

    for (uint16_t frame = 0; frame < 90; frame++) {
        bios_wait_vbl();
        if (skip_pressed() || (BIOS_STATCHANGE & STAT_COIN1)) break;
    }

    sys_fix_clear_c();
    bios_wait_vbl();
}
