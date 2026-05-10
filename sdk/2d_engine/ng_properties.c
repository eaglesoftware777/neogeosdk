#include "ng_properties.h"

static int32_t ng_properties[NG_PROP_GROUPS][NG_PROP_COUNT];

void NEOGEO_USER ng_properties_init(void)
{
    uint8_t g, i;
    for (g = 0; g < NG_PROP_GROUPS; g++) {
        for (i = 0; i < NG_PROP_COUNT; i++) {
            ng_properties[g][i] = 0;
        }
    }
}

void NEOGEO_USER ng_prop_set(uint8_t group, uint8_t id, int32_t value)
{
    if (group >= NG_PROP_GROUPS) return;
    if (id >= NG_PROP_COUNT) return;
    ng_properties[group][id] = value;
}

int32_t NEOGEO_USER ng_prop_get(uint8_t group, uint8_t id)
{
    if (group >= NG_PROP_GROUPS) return 0;
    if (id >= NG_PROP_COUNT) return 0;
    return ng_properties[group][id];
}

void NEOGEO_USER ng_prop_add(uint8_t group, uint8_t id, int32_t amount)
{
    if (group >= NG_PROP_GROUPS) return;
    if (id >= NG_PROP_COUNT) return;
    ng_properties[group][id] += amount;
}

void NEOGEO_USER ng_prop_clear_group(uint8_t group)
{
    uint8_t i;
    if (group >= NG_PROP_GROUPS) return;
    for (i = 0; i < NG_PROP_COUNT; i++) {
        ng_properties[group][i] = 0;
    }
}
