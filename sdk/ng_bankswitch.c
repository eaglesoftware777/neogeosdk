#include "ng_bankswitch.h"

static uint8_t ng_current_bank = 0;

void NEOGEO_USER ng_bankswitch(uint8_t bank)
{
    ng_current_bank = bank;
    NG_BANKSWITCH_REG = (uint16_t)bank;
}

uint8_t NEOGEO_USER ng_bankswitch_current(void)
{
    return ng_current_bank;
}
