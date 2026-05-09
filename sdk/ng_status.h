#ifndef NG_STATUS_H
#define NG_STATUS_H

#include "ng_defs.h"

void status_init(void);
void status_set(uint16_t status_id);
void status_clear(uint16_t status_id);
uint8_t status_has(uint16_t status_id);
void status_toggle(uint16_t status_id);

#endif
