#include "eagle_cd.h"

volatile uint32_t eagle_cd_frames;
volatile uint16_t eagle_cd_pad_seen;
static volatile uint16_t pad;
static uint8_t user_request;

void eagle_cd_io(void)
{
    uint8_t i, start = (uint8_t)~ECD_REG8(0x380000) & 15u;
    for (i = 0; i < 2u; ++i) {
        uint32_t base = i ? 0x10FD9Aul : 0x10FD94ul;
        uint8_t old = ECD_REG8(base+2u);
        uint8_t now = (uint8_t)~ECD_REG8(i ? 0x340000ul : 0x300000ul);
        ECD_REG8(base) = 1;
        ECD_REG8(base+1u) = old;
        ECD_REG8(base+2u) = now;
        ECD_REG8(base+3u) = now & (uint8_t)~old;
        ECD_REG8(base+4u) = now & (uint8_t)~old;
    }
    ECD_REG8(0x10FDAD) = start & (uint8_t)~ECD_REG8(0x10FDAC);
    ECD_REG8(0x10FDAC) = start;
    pad = (uint16_t)((uint8_t)~ECD_REG8(0x300000));
    pad |= (uint16_t)(((uint8_t)~ECD_REG8(0x380000) & 3u) << 8);
    eagle_cd_pad_seen |= pad;
}

void eagle_cd_vblank(void)
{
    ++eagle_cd_frames;
    if (!(ECD_REG8(0x10FD80) & 0x80u)) eagle_cd_io();
}

static void launch_request(void)
{
    ECD_REG8(0x10FD80) = (uint8_t)(0x80u | user_request);
    ECD_REG8(0x10FDAE) = user_request;
    ECD_REG8(0x10FDAF) = user_request >= 2u ? 1u : 0u;
    ECD_REG8(0x10FD82) = 0; /* CD console, not an MVS cabinet */
    ECD_REG8(0x10FD83) = 1; /* export console language */
    ECD_REG8(0x10F6D9) = 1;
    eagle_cd_phase = 7;
    eagle_cd_enter_user();
}

void eagle_cd_next_user(void)
{
    if (user_request < 2u) ++user_request;
    launch_request();
}

void eagle_cd_boot(void)
{
    ECD_REG8(0x10FD80) = 0;
    eagle_cd_video_init();
    eagle_cd_text(7, 5, "EAGLE CD BIOS 1.7.7");
#if CD_MODEL == 0
    eagle_cd_text(7, 7, "FRONT-LOADER PROFILE");
#elif CD_MODEL == 1
    eagle_cd_text(7, 7, "TOP-LOADER PROFILE");
#else
    eagle_cd_text(7, 7, "CDZ DISC LOADER");
#endif
    eagle_cd_text(5, 11, "READING GAME DISC");
    ECD_REG16(0xFF0004) = 0x0030;
    ECD_REG16(0x3C000C) = 7;
    ECD_REG8(0x300001) = 0;
    __asm__ volatile ("move.w #0x2000,%sr");
    eagle_cd_error = (int16_t)eagle_cd_drive_init();
    if (!eagle_cd_error) eagle_cd_error = (int16_t)eagle_cd_mount();
    if (!eagle_cd_error) {
        eagle_cd_text(5, 13, "LOADING PROGRAM AND ASSETS");
        eagle_cd_error = (int16_t)eagle_cd_load_ipl();
    }
    if (!eagle_cd_error) {
        uint16_t i;
        for (i = 0; i < 0xD00u; ++i) ECD_REG8(0x10F300ul+i) = 0;
        eagle_cd_decoder_enable(0);
        user_request = 0;
        launch_request();
    }
    eagle_cd_video_init();
    eagle_cd_text(7, 5, "EAGLE CD BIOS 1.7.7");
    eagle_cd_text(5, 11, eagle_cd_error == -2 ? "GAME FILE OR DISC MISSING" :
                        eagle_cd_error == -110 ? "DISC READ TIMED OUT" :
                        eagle_cd_error == -22 ? "INVALID GAME DISC" :
                        eagle_cd_error == -34 ? "INVALID ASSET ADDRESS" : "DISC READ FAILED");
    eagle_cd_text(5, 13, "PRESS A OR START TO RETRY");
    for (;;) {
        eagle_cd_wait();
        ECD_REG8(0x300001) = 0;
        if (pad & 0x110u) eagle_cd_reset();
    }
}
