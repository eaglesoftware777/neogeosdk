#include "eagle_cd.h"

volatile uint32_t eagle_cd_frames;
volatile uint16_t eagle_cd_pad_seen;
static volatile uint16_t pad;

static inline uint8_t eagle_cd_read8(uint32_t addr)
{
    uint8_t val;
    asm volatile ("move.b (%1), %0" : "=d"(val) : "a"(addr));
    return val;
}

void eagle_cd_io(void)
{
    pad = (uint16_t)((uint8_t)~ECD_REG8(0x300000));
    pad |= (uint16_t)(((uint8_t)~ECD_REG8(0x380000) & 3u) << 8);
    eagle_cd_pad_seen |= pad;
}

void eagle_cd_vblank(void)
{
    ++eagle_cd_frames;
    eagle_cd_io();
}

void eagle_cd_boot(void)
{
    eagle_cd_video_init();
    eagle_cd_text(7, 5, "EAGLE CD BIOS 1.7.7");
#if CD_MODEL == 0
    eagle_cd_text(7, 7, "FRONT-LOADER PROFILE");
#elif CD_MODEL == 1
    eagle_cd_text(7, 7, "TOP-LOADER PROFILE");
#else
    eagle_cd_text(7, 7, "CDZ FAST-BOOT PROFILE");
#endif
    eagle_cd_text(7, 11, "BOOTING GAME DISC...");
    ECD_REG16(0xFF0004) = 0x0030;
    ECD_REG16(0x3C000C) = 7;
    ECD_REG8(0x300001) = 0;

    /* If a game is already resident in 68K DRAM (at 0x100), launch directly */
    if (eagle_cd_read8(0x100) == 'N' && eagle_cd_read8(0x101) == 'E' &&
        eagle_cd_read8(0x102) == 'O' && eagle_cd_read8(0x103) == '-' &&
        eagle_cd_read8(0x104) == 'G' && eagle_cd_read8(0x105) == 'E' &&
        eagle_cd_read8(0x106) == 'O') {
        asm volatile ("jmp 0x000122");
    }

    /* Wait briefly then jump to the CD disc loader */
    for (uint16_t i = 0; i < 40u; ++i) {
        eagle_cd_wait();
        ECD_REG8(0x300001) = 0;
        if (pad & 0x11u) break;
    }

    /* Jump to CD disc loader */
    asm volatile ("jmp 0xC0A5B6");
}
