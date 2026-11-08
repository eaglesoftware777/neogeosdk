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

/* The registers a compiled caller keeps across a call are saved around the
 * system ROM's routine, which changes several of them. */
#define NG_SYS_STR2(x) #x
#define NG_SYS_STR(x) NG_SYS_STR2(x)
#define NG_SYS_SAFE_CALL(addr) \
    __asm__ volatile ("movem.l %%d2-%%d7/%%a2-%%a6,-(%%sp)\n\t" \
                      "jsr " NG_SYS_STR(addr) "\n\t" \
                      "movem.l (%%sp)+,%%d2-%%d7/%%a2-%%a6" \
                      : : : "d0", "d1", "a0", "a1", "cc", "memory")

void NEOGEO_USER ng_sys_credit_check(void)
{
    NG_SYS_SAFE_CALL(SYS_CREDIT_CHECK);
}

void NEOGEO_USER ng_sys_credit_down(void)
{
    NG_SYS_SAFE_CALL(SYS_CREDIT_DOWN);
}
