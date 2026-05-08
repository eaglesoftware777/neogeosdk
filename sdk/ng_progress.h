#ifndef NG_PROGRESS_H
#define NG_PROGRESS_H

#include "ng_defs.h"

typedef struct {
    uint8_t active;
    uint16_t current;
    uint16_t max;
} NGProgress;

void progress_init(void);
void progress_start(uint8_t id, uint16_t max);
void progress_set(uint8_t id, uint16_t value);
void progress_add(uint8_t id, uint16_t amount);
uint16_t progress_value(uint8_t id);
uint8_t progress_percent(uint8_t id);
uint8_t progress_done(uint8_t id);
void progress_update(void);

#endif
