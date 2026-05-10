#ifndef NG_PROGRESS_H
#define NG_PROGRESS_H

#include "ng_defs.h"

typedef struct {
    uint8_t active;
    uint16_t current;
    uint16_t max;
} NGProgress;

void NEOGEO_USER ng_progress_init(void);
void NEOGEO_USER ng_progress_start(uint8_t id, uint16_t max);
void NEOGEO_USER ng_progress_set(uint8_t id, uint16_t value);
void NEOGEO_USER ng_progress_add(uint8_t id, uint16_t amount);
uint16_t NEOGEO_USER ng_progress_value(uint8_t id);
uint8_t NEOGEO_USER ng_progress_percent(uint8_t id);
uint8_t NEOGEO_USER ng_progress_done(uint8_t id);
void NEOGEO_USER ng_progress_update(void);

#endif
