#include "ng_progress.h"

static NGProgress ng_progress[NG_MAX_PROGRESS];

static uint8_t ng_progress_percent_value(uint16_t current, uint16_t max)
{
    uint8_t percent = 0;
    uint16_t step = 0;
    uint32_t accum = 0;

    if (max == 0) return 0;

    for (step = 0; step < 100; step++) {
        accum += current;
        if (accum >= max) {
            accum -= max;
            percent++;
        }
    }
    return percent;
}

void NEOGEO_USER progress_init(void)
{
    uint8_t i;
    for (i = 0; i < NG_MAX_PROGRESS; i++) {
        ng_progress[i].active = 0;
        ng_progress[i].current = 0;
        ng_progress[i].max = 0;
    }
}

void NEOGEO_USER progress_start(uint8_t id, uint16_t max)
{
    if (id >= NG_MAX_PROGRESS) return;
    ng_progress[id].active = 1;
    ng_progress[id].current = 0;
    ng_progress[id].max = max;
}

void NEOGEO_USER progress_set(uint8_t id, uint16_t value)
{
    if (id >= NG_MAX_PROGRESS) return;
    if (ng_progress[id].max && value > ng_progress[id].max) value = ng_progress[id].max;
    ng_progress[id].current = value;
}

void NEOGEO_USER progress_add(uint8_t id, uint16_t amount)
{
    uint16_t next;
    if (id >= NG_MAX_PROGRESS) return;
    next = ng_progress[id].current + amount;
    if (ng_progress[id].max && next > ng_progress[id].max) next = ng_progress[id].max;
    ng_progress[id].current = next;
}

uint16_t NEOGEO_USER progress_value(uint8_t id)
{
    if (id >= NG_MAX_PROGRESS) return 0;
    return ng_progress[id].current;
}

uint8_t NEOGEO_USER progress_percent(uint8_t id)
{
    if (id >= NG_MAX_PROGRESS) return 0;
    if (ng_progress[id].max == 0) return 0;
    return ng_progress_percent_value(ng_progress[id].current, ng_progress[id].max);
}

uint8_t NEOGEO_USER progress_done(uint8_t id)
{
    if (id >= NG_MAX_PROGRESS) return 1;
    if (!ng_progress[id].active) return 1;
    return ng_progress[id].current >= ng_progress[id].max;
}

void NEOGEO_USER progress_update(void)
{
    uint8_t i;
    for (i = 0; i < NG_MAX_PROGRESS; i++) {
        if (ng_progress[i].active && ng_progress[i].max && ng_progress[i].current >= ng_progress[i].max) {
            ng_progress[i].active = 0;
        }
    }
}
