/*
 * ng_sys.c — Which machine the game is running on (see ng_sys.h)
 *
 * BIOS_MVS_FLAG and BIOS_COUNTRY_CODE, and where they are documented, are
 * in macro.h.
 */

#include <stdint.h>
#include "macro.h"
#include "ng_sys.h"

int NEOGEO_USER ng_sys_is_mvs(void)
{
    /* 0x80 on an arcade board, 0 on a console */
    return *(volatile uint8_t *)BIOS_MVS_FLAG ? 1 : 0;
}

int NEOGEO_USER ng_sys_region(void)
{
    uint8_t code = *(volatile uint8_t *)BIOS_COUNTRY_CODE;
    if (code == NG_REGION_JP || code == NG_REGION_EU) return code;
    return NG_REGION_US;   /* USA, and any code the system ROM should never leave */
}
