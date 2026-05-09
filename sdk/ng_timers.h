#ifndef NG_TIMERS_H
#define NG_TIMERS_H

#include "ng_defs.h"

typedef struct {
    uint8_t active;
    uint16_t value;
    uint16_t initial;
} NGTimer;

void timers_init(void);
void timer_start(uint8_t id, uint16_t frames);
void timer_stop(uint8_t id);
void timers_update(void);
uint8_t timer_active(uint8_t id);
uint8_t timer_done(uint8_t id);
uint16_t timer_value(uint8_t id);
uint8_t timer_percent_left(uint8_t id);

#endif
