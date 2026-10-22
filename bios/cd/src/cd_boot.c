#include "eagle_cd.h"

volatile uint32_t eagle_cd_frames;
volatile uint16_t eagle_cd_pad_seen;
static volatile uint16_t pad;

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
    eagle_cd_text(7, 7, "CDZ PROFILE");
#endif
    eagle_cd_text(7, 11, "ORIGINAL FIRMWARE DIAGNOSTICS");
    eagle_cd_text(7, 13, "DISC BOOT: NOT IMPLEMENTED");
    eagle_cd_text(7, 17, "A: CONTROLLER TEST");
    ECD_REG16(0xFF0004) = 0x0030;
    ECD_REG16(0x3C000C) = 7;
    for (;;) {
        eagle_cd_wait();
        ECD_REG8(0x300001) = 0;
        eagle_cd_text(7, 19, (pad & 0x10u) ? "A PRESSED " : "A RELEASED");
        eagle_cd_text(7, 21, (pad & 0x08u) ? "RIGHT PRESSED " : "RIGHT RELEASED");
    }
}
