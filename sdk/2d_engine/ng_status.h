#ifndef NG_STATUS_H
#define NG_STATUS_H

#include "ng_defs.h"

void NEOGEO_USER ng_status_init(void);
void NEOGEO_USER ng_status_set(uint16_t status_id);
void NEOGEO_USER ng_status_clear(uint16_t status_id);
uint8_t NEOGEO_USER ng_status_has(uint16_t status_id);
void NEOGEO_USER ng_status_toggle(uint16_t status_id);

#endif
