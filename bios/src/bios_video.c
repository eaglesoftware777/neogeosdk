#include "bios.h"

/*
 * ============================================================================
 *  EagleBIOS Video & Graphics Sub-system
 * ============================================================================
 */

/*
 * SYS_FIX_CLEAR: Clear FIX layer in VRAM.
 * The FIX map is 40 columns x 32 rows = 1280 words at VRAM 0x7000.
 * Every cell becomes tile 0x20, the space glyph of any text font, which is
 * the state a game program is promised on entry.
 */
void sys_fix_clear_c(void)
{
    uint16_t i;

    REG_VRAM_ADDR = VRAM_FIXMAP;
    REG_VRAM_INC = 1;

    for (i = 0; i < 1280; i++) {
        REG_VRAM_RW = FIX_BLANK;
    }
}

/* Both palette banks to black: the other promise made to a game on entry. */
void bios_palettes_clear(void)
{
    for (uint8_t bank = 0; bank < 2; bank++) {
        volatile uint16_t *pal = (volatile uint16_t *)ADDR_PALETTES;
        if (bank) REG_PALBANK1 = 0; else REG_PALBANK0 = 0;
        for (uint16_t i = 0; i < 4096; i++) *pal++ = 0;
        *(volatile uint16_t *)ADDR_BACKDROP = COLOR_BLACK;
    }
    REG_PALBANK0 = 0;
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
        REG_VRAM_RW = 0x0FFFu; /* Full size */
    }

    /* Clear SCB4: X position (512 words) */
    REG_VRAM_ADDR = VRAM_SCB4;
    REG_VRAM_INC = 1;
    for (i = 0; i < 512; i++) {
        REG_VRAM_RW = 0x0000u;
    }
}

/* MESS_POINT is the END of a queue of stream pointers, not a stream.
 * Bound work per call so malformed cartridge data cannot wedge VBlank. */
static uint8_t readable(uint32_t address, uint16_t bytes)
{
    uint32_t end = address + bytes;
    if (end < address) return 0;
    return end <= 0x100000u ||
           (address >= 0x100000u && end <= 0x110000u) ||
           (address >= 0x200000u && end <= 0x300000u) ||
           (address >= 0xC00000u && end <= 0xC20000u);
}

static uint16_t get_word(uint32_t address)
{
    const volatile uint8_t *p = (const volatile uint8_t *)address;
    return (uint16_t)((p[0] << 8) | p[1]);
}

static uint32_t get_long(uint32_t address)
{
    return ((uint32_t)get_word(address) << 16) | get_word(address + 2);
}

static void mess_stream(uint32_t pc)
{
    uint32_t stack[4], data = 0;
    uint16_t parameter = 0x00FFu, anchor = VRAM_FIXMAP, limit = 4096;
    uint8_t format = 0, depth = 0;
    REG_VRAM_INC = 32;
    for (uint16_t commands = 0; commands < 1024 && limit; commands++) {
        if ((pc & 1u) || !readable(pc, 2)) return;
        uint16_t instruction = get_word(pc);
        uint8_t opcode = (uint8_t)instruction, arg = (uint8_t)(instruction >> 8);
        pc += 2;
        if (!opcode) return;
        if (opcode == 1 || opcode == 3 || opcode == 5 || opcode == 12 || opcode == 13) {
            if (!readable(pc, 2)) return;
            uint16_t value = get_word(pc);
            pc += 2;
            if (opcode == 1) { format = arg & 3; parameter = value; }
            else if (opcode == 3) { anchor = value; REG_VRAM_ADDR = anchor; }
            else if (opcode == 5) { anchor += value; REG_VRAM_ADDR = anchor; }
            else for (uint16_t i = 0; i < arg && limit; i++, limit--) {
                REG_VRAM_RW = value;
                if (opcode == 13) value = (value & 0xFF00u) | ((value + 1u) & 0xFFu);
            }
        } else if (opcode == 2) {
            REG_VRAM_INC = (uint16_t)(int16_t)(int8_t)arg;
        } else if (opcode == 10) {
            if (depth == 4 || !readable(pc, 4)) return;
            stack[depth++] = pc + 4;
            pc = get_long(pc);
        } else if (opcode == 11) {
            if (!depth) return;
            pc = stack[--depth];
        } else if (opcode == 8) {
            REG_VRAM_INC = 32;
            uint16_t x = anchor;
            while (limit && readable(pc, 1)) {
                uint8_t ch = *(const volatile uint8_t *)pc++;
                if (ch == 255) break;
                REG_VRAM_ADDR = x;
                REG_VRAM_RW = ((uint16_t)arg << 8) | ch;
                REG_VRAM_ADDR = x + 1u;
                REG_VRAM_RW = ((uint16_t)(arg + 1u) << 8) | ch;
                x += 32;
                limit--;
            }
            pc = (pc + 1u) & ~1u;
        } else if (opcode == 4 || opcode == 6 || opcode == 7) {
            if (opcode == 4) {
                if (!readable(pc, 4)) return;
                data = get_long(pc);
                pc += 4;
            } else if (opcode == 7) data = pc;
            uint16_t count = (format & 2) ? parameter : parameter & 255u;
            uint16_t n = 0;
            while (limit && (!(format & 1) || n < count)) {
                uint8_t bytes = (format & 2) ? 2 : 1;
                if (!readable(data, bytes)) return;
                uint16_t value = bytes == 2 ? get_word(data) : *(const volatile uint8_t *)data;
                data += bytes;
                if (!(format & 1) && value == count) break;
                REG_VRAM_RW = bytes == 2 ? value : (parameter & 0xFF00u) | value;
                n++;
                limit--;
            }
            if (opcode == 7) pc = (data + 1u) & ~1u;
        } else {
            /* Japanese common-FIX translation (9) is not provided. */
            return;
        }
    }
}

void sys_mess_out_c(void)
{
    uint32_t end = BIOS_MESS_POINT;
    if (BIOS_MESS_BUSY) return;
    if (end < 0x10FF00u || end > 0x110000u || (end & 3u)) {
        BIOS_MESS_POINT = 0x10FF00u;
        return;
    }
    for (uint32_t entry = 0x10FF00u; entry < end; entry += 4) {
        uint32_t stream = get_long(entry);
        if (!stream) { mess_stream(entry + 4); break; }
        mess_stream(stream);
    }
    BIOS_MESS_POINT = 0x10FF00u;
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
        REG_VRAM_RW = FIX_BLANK;
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

/*
 * Copy one 16 colour bank into palette RAM.
 *
 * Kept in assembly on purpose.  The C loop was compiled to
 * "move.w (a0)+,(0,a0,d0.l)", and a 68000 computes that destination with
 * the already incremented a0, so every colour landed one entry late and
 * index 0 (transparent) took the ink.  Two address registers, no surprise.
 */
__attribute__((noinline))
void bios_set_palette(uint8_t bank, const uint16_t *colors)
{
    volatile uint16_t *pal_dst = (volatile uint16_t *)(ADDR_PALETTES + ((uint32_t)bank * 32u));
    uint16_t count = 15;
    asm volatile (
        "1:\n\t"
        "move.w (%0)+,(%1)+\n\t"
        "dbf %2,1b"
        : "+a" (colors), "+a" (pal_dst), "+d" (count)
        :
        : "memory");
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
