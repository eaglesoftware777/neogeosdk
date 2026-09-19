#include "bios.h"

/*
 * ============================================================================
 *  EagleBIOS Video & Graphics Sub-system
 * ============================================================================
 */

/*
 * SYS_FIX_CLEAR: Clear FIX layer in VRAM.
 * The FIX map is 40 columns x 32 rows = 1280 words at VRAM 0x7000.
 * Writing 0x00FF sets each tile to transparent blank space.
 */
void sys_fix_clear_c(void)
{
    uint16_t i;

    REG_VRAM_ADDR = VRAM_FIXMAP;
    REG_VRAM_INC = 1;

    for (i = 0; i < 1280; i++) {
        REG_VRAM_RW = 0x00FFu;
    }
}

/*
 * SYS_LSP_1ST: Initialize and hide all sprite strips in VRAM.
 * - SCB3 (0x8200): Sets sprite strip heights to 0, hiding all sprites.
 * - SCB2 (0x8000): Clears zoom/shrink attributes.
 * - SCB4 (0x8400): Clears horizontal positions.
 */
void sys_lsp_1st_c(void)
{
    uint16_t i;

    /* Clear SCB3: Height & Y position (512 words) */
    REG_VRAM_ADDR = VRAM_SCB3;
    REG_VRAM_INC = 1;
    for (i = 0; i < 512; i++) {
        REG_VRAM_RW = 0x0000u;
    }

    /* Clear SCB2: Shrinkage (512 words) */
    REG_VRAM_ADDR = VRAM_SCB2;
    REG_VRAM_INC = 1;
    for (i = 0; i < 512; i++) {
        REG_VRAM_RW = 0x000Fu; /* Full size */
    }

    /* Clear SCB4: X position (512 words) */
    REG_VRAM_ADDR = VRAM_SCB4;
    REG_VRAM_INC = 1;
    for (i = 0; i < 512; i++) {
        REG_VRAM_RW = 0x0000u;
    }
}

/*
 * SYS_MESS_OUT: Process formatted text command stream.
 * Supported commands:
 *   0x0301 (COMMAND1WL): word length, followed by inc command (0x2002)
 *   0x0003 (COMMAND3): target VRAM address
 *   0x0004 (COMMAND4): pointer (32-bit) to word data
 *   0x0000 (COMMAND0): end of message stream
 */
void sys_mess_out_c(void)
{
    const volatile uint16_t *cmd = BIOS_MESS_BUFFER;
    uint16_t len = 0;
    uint16_t inc = 0x20;
    uint16_t vram_addr = VRAM_FIXMAP;
    const uint16_t *data_ptr = 0;
    uint8_t has_work = 0;

    if (BIOS_MESS_POINT != 0) {
        /* If point points within RAM, use it if buffer not used */
    }

    while (*cmd != 0x0000u) {
        uint16_t opcode = *cmd++;

        if (opcode == 0x0301u) {
            /* COMMAND1WL: length, increment */
            len = *cmd++;
            uint16_t inc_cmd = *cmd++;
            inc = (uint16_t)((inc_cmd >> 8) & 0xFFu);
            if (inc == 0) inc = 0x20;
            has_work = 1;
        } else if (opcode == 0x0003u) {
            /* COMMAND3: VRAM address */
            vram_addr = *cmd++;
        } else if (opcode == 0x0004u) {
            /* COMMAND4: 32-bit data address */
            uint32_t hi = *cmd++;
            uint32_t lo = *cmd++;
            data_ptr = (const uint16_t *)((hi << 16) | lo);
        } else if (opcode == 0x2002u) {
            inc = 0x20;
        } else {
            /* Unknown or skip */
            cmd++;
        }

        if (has_work && data_ptr && len > 0) {
            REG_VRAM_ADDR = vram_addr;
            REG_VRAM_INC = inc;
            for (uint16_t i = 0; i < len; i++) {
                REG_VRAM_RW = data_ptr[i];
            }
            has_work = 0;
            data_ptr = 0;
            len = 0;
        }
    }
}

/*
 * BIOS FIX Layer Direct Output Helpers
 * Neo Geo visible screen is 40 columns (x = 0..39), 28 visible rows (y = 0..27).
 * Vertical blanking occupies rows 0, 1 and 30, 31, so visible row y is map row y + 2.
 */
void bios_fix_putc(uint8_t x, uint8_t y, char ch, uint8_t pal)
{
    if (x >= 40 || y >= 28) return;

    uint16_t addr = (uint16_t)(VRAM_FIXMAP + (y + 2u) + ((uint16_t)x * 32u));
    REG_VRAM_ADDR = addr;
    REG_VRAM_INC = 1;

    if (ch == ' ') {
        REG_VRAM_RW = 0x00FFu; /* Blank tile */
    } else {
        uint16_t tile = (uint16_t)(((pal & 0x0Fu) << 12) | ((uint8_t)ch));
        REG_VRAM_RW = tile;
    }
}

void bios_fix_puts(uint8_t x, uint8_t y, const char *str, uint8_t pal)
{
    if (!str || y >= 28) return;

    while (*str && x < 40) {
        bios_fix_putc(x, y, *str, pal);
        x++;
        str++;
    }
}

void bios_fix_put_hex16(uint8_t x, uint8_t y, uint16_t val, uint8_t pal)
{
    static const char hex_digits[] = "0123456789ABCDEF";
    char buf[5];
    buf[0] = hex_digits[(val >> 12) & 0xF];
    buf[1] = hex_digits[(val >> 8) & 0xF];
    buf[2] = hex_digits[(val >> 4) & 0xF];
    buf[3] = hex_digits[val & 0xF];
    buf[4] = '\0';
    bios_fix_puts(x, y, buf, pal);
}

void bios_fix_put_dec2(uint8_t x, uint8_t y, uint8_t val, uint8_t pal)
{
    char buf[3];
    uint8_t tens = 0;
    while (val >= 10) {
        val -= 10;
        tens++;
    }
    buf[0] = (char)('0' + tens);
    buf[1] = (char)('0' + val);
    buf[2] = '\0';
    bios_fix_puts(x, y, buf, pal);
}

void bios_fix_clear_area(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t iy, ix;
    for (iy = 0; iy < h; iy++) {
        for (ix = 0; ix < w; ix++) {
            bios_fix_putc((uint8_t)(x + ix), (uint8_t)(y + iy), ' ', 0);
        }
    }
}

void bios_set_palette(uint8_t bank, const uint16_t *colors)
{
    volatile uint16_t *pal_dst = (volatile uint16_t *)(ADDR_PALETTES + ((uint32_t)bank * 32u));
    for (uint8_t i = 0; i < 16; i++) {
        pal_dst[i] = colors[i];
    }
}

void bios_set_backdrop(uint16_t color)
{
    *(volatile uint16_t *)ADDR_BACKDROP = color;
}

void bios_init_palette_banks(void)
{
    /* Bank 0: Default white text with black outline */
    static const uint16_t pal_white[16] = {
        COLOR_BLACK, COLOR_WHITE, COLOR_BLACK, COLOR_MIDGRAY,
        COLOR_DARKGRAY, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE,
        COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE,
        COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE
    };
    /* Bank 1: Bright White */
    static const uint16_t pal_bright_white[16] = {
        COLOR_BLACK, COLOR_WHITE, COLOR_BLACK, COLOR_WHITE,
        COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE,
        COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE,
        COLOR_WHITE, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE
    };
    /* Bank 2: Gold / Yellow */
    static const uint16_t pal_gold[16] = {
        COLOR_BLACK, COLOR_GOLD, COLOR_BLACK, COLOR_YELLOW,
        COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD,
        COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD,
        COLOR_GOLD, COLOR_GOLD, COLOR_GOLD, COLOR_GOLD
    };
    /* Bank 3: Cyan */
    static const uint16_t pal_cyan[16] = {
        COLOR_BLACK, COLOR_CYAN, COLOR_BLACK, COLOR_CYAN,
        COLOR_CYAN, COLOR_CYAN, COLOR_CYAN, COLOR_CYAN,
        COLOR_CYAN, COLOR_CYAN, COLOR_CYAN, COLOR_CYAN,
        COLOR_CYAN, COLOR_CYAN, COLOR_CYAN, COLOR_CYAN
    };
    /* Bank 4: Red / Alert */
    static const uint16_t pal_red[16] = {
        COLOR_BLACK, COLOR_RED, COLOR_BLACK, COLOR_RED,
        COLOR_RED, COLOR_RED, COLOR_RED, COLOR_RED,
        COLOR_RED, COLOR_RED, COLOR_RED, COLOR_RED,
        COLOR_RED, COLOR_RED, COLOR_RED, COLOR_RED
    };
    /* Bank 5: Green / OK */
    static const uint16_t pal_green[16] = {
        COLOR_BLACK, COLOR_GREEN, COLOR_BLACK, COLOR_GREEN,
        COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN,
        COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN,
        COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN
    };
    /* Bank 6: Blue */
    static const uint16_t pal_blue[16] = {
        COLOR_BLACK, COLOR_BLUE, COLOR_BLACK, COLOR_BLUE,
        COLOR_BLUE, COLOR_BLUE, COLOR_BLUE, COLOR_BLUE,
        COLOR_BLUE, COLOR_BLUE, COLOR_BLUE, COLOR_BLUE,
        COLOR_BLUE, COLOR_BLUE, COLOR_BLUE, COLOR_BLUE
    };
    /* Bank 7: Gray / Inactive */
    static const uint16_t pal_gray[16] = {
        COLOR_BLACK, COLOR_MIDGRAY, COLOR_BLACK, COLOR_DARKGRAY,
        COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY,
        COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY,
        COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY, COLOR_MIDGRAY
    };

    bios_set_palette(0, pal_white);
    bios_set_palette(1, pal_bright_white);
    bios_set_palette(2, pal_gold);
    bios_set_palette(3, pal_cyan);
    bios_set_palette(4, pal_red);
    bios_set_palette(5, pal_green);
    bios_set_palette(6, pal_blue);
    bios_set_palette(7, pal_gray);
}

