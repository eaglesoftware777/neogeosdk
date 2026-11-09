/*
 * ng_join.c — A second player joining (see ng_join.h)
 *
 * The credit is the system's: the newcomer's bit stays in BIOS_START_FLAG, the
 * credit check clears it if he cannot pay, and the credit-down call takes one
 * from each player still flagged (a system that takes it itself after the call
 * does not take it twice). Both calls go through ng_sys, which keeps the
 * registers the routines change: "want" lives in one of them. The value kept
 * here starts at 0: RAM is zeroed and initialised data is not loaded.
 */

#include "ng_join.h"
#include "neogeo.h"
#include "cabinet/ng_sys.h"

static uint8_t s_pending;

uint8_t NEOGEO_USER ng_join_bios_start(uint8_t playing)
{
    volatile uint8_t *flag = (volatile uint8_t *)BIOS_START_FLAG;
    uint8_t want = (uint8_t)(*flag & 3u & (uint8_t)~playing);
    *flag = want;
    if (!want) return 0;
    if (ng_sys_is_mvs()) {                   /* a console has no credits */
        ng_sys_credit_check();
        want = (uint8_t)(want & *flag);
        if (!want) return 0;
        ng_sys_credit_down();
    }
    if (want & 1u) *(volatile uint8_t *)BIOS_PLAYER1_MODE = 1;
    if (want & 2u) *(volatile uint8_t *)BIOS_PLAYER2_MODE = 1;
    s_pending = (uint8_t)(s_pending | want);
    return want;
}

uint8_t NEOGEO_USER ng_join_poll(void)
{
    uint8_t p = 0;
    if (s_pending & 1u) p = 1;
    else if (s_pending & 2u) p = 2;
    if (p) s_pending = (uint8_t)(s_pending & ~(p == 1u ? 1u : 2u));
    return p;
}

void NEOGEO_USER ng_join_leave(uint8_t p)
{
    if (p == 1u) *(volatile uint8_t *)BIOS_PLAYER1_MODE = 0;
    if (p == 2u) *(volatile uint8_t *)BIOS_PLAYER2_MODE = 0;
}
