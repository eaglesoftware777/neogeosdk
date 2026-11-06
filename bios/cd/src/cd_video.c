#include "eagle_cd.h"

void eagle_cd_text(uint8_t x, uint8_t y, const char *text)
{
    ECD_REG16(0x3C0004) = 32;
    ECD_REG16(0x3C0000) = (uint16_t)(0x7000u + x * 32u + y + 2u);
    while (*text && x++ < 40u) ECD_REG16(0x3C0002) = (uint8_t)*text++;
}

void eagle_cd_video_init(void)
{
    uint16_t i;
    /* Native FIX bytes on the odd lane of the CD upload window. This
     * source-original font is embedded in this CD ROM, not motherboard SFIX. */
    ECD_REG8(0xFF0129) = 0;
    ECD_REG8(0xFF0105) = 5;
    for (i=0; i<4096u; ++i) {
        ECD_REG8(0xE00001ul + (uint32_t)i * 2u) = eagle_cd_font[i];
        if (!(i & 255u)) ECD_REG8(0x300001) = 0;
    }
    ECD_REG8(0xFF0149) = 0;
    ECD_REG8(0x3A0001) = 0;
    ECD_REG8(0x3A000F) = 0;
    ECD_REG16(0x3C0006) = 0;
    ECD_REG16(0x400000) = 0x8000;
    ECD_REG16(0x400002) = 0x0FFF;
    ECD_REG16(0x401FFE) = 0x8005;
    ECD_REG16(0x3C0004) = 1;
    ECD_REG16(0x3C0000) = 0x7000;
    for (i=0; i<1280u; ++i) ECD_REG16(0x3C0002) = 32;
    ECD_REG16(0x3C0000) = 0x8200;
    for (i=0; i<384u; ++i) ECD_REG16(0x3C0002) = 0;
    ECD_REG8(0xFF0115) = 0;
    ECD_REG8(0xFF0119) = 1;
}
