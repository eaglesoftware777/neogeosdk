#include "ng_timers.h"

static NGTimer ng_timers[NG_MAX_TIMERS];

static uint8_t NEOGEO_USER ng_timer_percent_value(uint16_t value, uint16_t max)
{
    uint8_t percent = 0;
    uint16_t step = 0;
    uint32_t accum = 0;

    if (max == 0) return 0;

    for (step = 0; step < 100; step++) {
        accum += value;
        if (accum >= max) {
            accum -= max;
            percent++;
        }
    }
    return percent;
}

void NEOGEO_USER ng_timers_init(void)
{
    uint8_t i;
    for (i = 0; i < NG_MAX_TIMERS; i++) {
        ng_timers[i].active = 0;
        ng_timers[i].value = 0;
        ng_timers[i].initial = 0;
    }
}

void NEOGEO_USER ng_timer_start(uint8_t id, uint16_t frames)
{
    if (id >= NG_MAX_TIMERS) return;
    ng_timers[id].active = (frames > 0) ? 1 : 0;
    ng_timers[id].value = frames;
    ng_timers[id].initial = frames;
}

void NEOGEO_USER ng_timer_stop(uint8_t id)
{
    if (id >= NG_MAX_TIMERS) return;
    ng_timers[id].active = 0;
    ng_timers[id].value = 0;
}

void NEOGEO_USER ng_timers_update(void)
{
    uint8_t i;
    for (i = 0; i < NG_MAX_TIMERS; i++) {
        if (!ng_timers[i].active) continue;
        if (ng_timers[i].value > 0) ng_timers[i].value--;
        if (ng_timers[i].value == 0) ng_timers[i].active = 0;
    }
}

uint8_t NEOGEO_USER ng_timer_active(uint8_t id)
{
    if (id >= NG_MAX_TIMERS) return 0;
    return ng_timers[id].active;
}

uint8_t NEOGEO_USER ng_timer_done(uint8_t id)
{
    if (id >= NG_MAX_TIMERS) return 1;
    return ng_timers[id].active == 0;
}

uint16_t NEOGEO_USER ng_timer_value(uint8_t id)
{
    if (id >= NG_MAX_TIMERS) return 0;
    return ng_timers[id].value;
}

uint8_t NEOGEO_USER ng_timer_percent_left(uint8_t id)
{
    if (id >= NG_MAX_TIMERS) return 0;
    if (ng_timers[id].initial == 0) return 0;
    return ng_timer_percent_value(ng_timers[id].value, ng_timers[id].initial);
}
