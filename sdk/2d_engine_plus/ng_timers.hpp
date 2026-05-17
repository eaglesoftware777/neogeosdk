#ifndef NG_TIMERS_HPP
#define NG_TIMERS_HPP

#include "ng_defs.hpp"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    uint8_t active;
    uint16_t value;
    uint16_t initial;
} NGTimer;

void NEOGEO_USER ng_timers_init(void);
void NEOGEO_USER ng_timer_start(uint8_t id, uint16_t frames);
void NEOGEO_USER ng_timer_stop(uint8_t id);
void NEOGEO_USER ng_timers_update(void);
uint8_t NEOGEO_USER ng_timer_active(uint8_t id);
uint8_t NEOGEO_USER ng_timer_done(uint8_t id);
uint16_t NEOGEO_USER ng_timer_value(uint8_t id);
uint8_t NEOGEO_USER ng_timer_percent_left(uint8_t id);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif