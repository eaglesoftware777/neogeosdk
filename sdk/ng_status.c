#include "ng_status.h"

static uint8_t ng_status[NG_MAX_STATUS];

void NEOGEO_USER status_init(void)
{
    uint16_t i;
    for (i = 0; i < NG_MAX_STATUS; i++) ng_status[i] = 0;
}

void NEOGEO_USER status_set(uint16_t status_id)
{
    if (status_id >= NG_MAX_STATUS) return;
    ng_status[status_id] = 1;
}

void NEOGEO_USER status_clear(uint16_t status_id)
{
    if (status_id >= NG_MAX_STATUS) return;
    ng_status[status_id] = 0;
}

uint8_t NEOGEO_USER status_has(uint16_t status_id)
{
    if (status_id >= NG_MAX_STATUS) return 0;
    return ng_status[status_id];
}

void NEOGEO_USER status_toggle(uint16_t status_id)
{
    if (status_id >= NG_MAX_STATUS) return;
    ng_status[status_id] = ng_status[status_id] ? 0 : 1;
}
