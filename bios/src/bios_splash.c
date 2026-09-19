#include "bios.h"

/*
 * ============================================================================
 *  EagleBIOS Splash Screen & Boot Presentation
 * ============================================================================
 */



void bios_splash_show(void)
{
    uint16_t frame;

    sys_fix_clear_c();
    sys_lsp_1st_c();
    bios_set_backdrop(COLOR_BLACK);
    bios_init_palette_banks();

    /* Draw EagleBIOS Banner */
    bios_fix_puts(4,  5, "==================================", 2); /* Gold border */
    bios_fix_puts(9,  7, "E A G L E   B I O S", 1);             /* White title */
    bios_fix_puts(6,  9, "OPEN SOURCE NEO-GEO FIRMWARE", 3);     /* Cyan subtitle */
    bios_fix_puts(4, 11, "==================================", 2);

    bios_fix_puts(8, 14, "MVS ARCADE & AES COMPATIBLE", 1);
    bios_fix_puts(10, 16, "(C) 1996 EAGLE SOFTWARE", 2);

    /* Display detected cartridge info */
    if (CART_HEADER->magic[0] == 'N' && CART_HEADER->magic[1] == 'E' &&
        CART_HEADER->magic[2] == 'O' && CART_HEADER->magic[3] == '-') {
        bios_fix_puts(8, 20, "CARTRIDGE NGH ID:  0x", 3);
        bios_fix_put_hex16(29, 20, CART_HEADER->ngh_id, 2);
        bios_fix_puts(12, 23, "SYSTEM INITIALIZED", 1);
    } else {
        bios_fix_puts(10, 20, "NO CARTRIDGE INSERTED", 4); /* Red */
    }

    /* Sound initialization: send 0x03 to soft-reset Z80 sound driver */
    REG_CRTFIX = 0;
    REG_SOUND = 0x03;

    /* Hold splash for 60 frames (~1.0 second) or until any button is pressed */
    for (frame = 0; frame < 60; frame++) {
        bios_wait_vbl();
        sys_io_c();

        /* Skip splash on button press */
        if (BIOS_P1CHANGE || (BIOS_STATCHANGE & (STAT_START1 | STAT_COIN1))) {
            break;
        }
    }

    /* Clear screen before handing off to game */
    sys_fix_clear_c();
    bios_wait_vbl();
}
