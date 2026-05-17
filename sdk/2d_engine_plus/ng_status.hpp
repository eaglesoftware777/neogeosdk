#ifndef NG_STATUS_HPP
#define NG_STATUS_HPP

#include "ng_defs.hpp"

#ifdef __cplusplus
extern "C" {
#endif


void NEOGEO_USER ng_status_init(void);
void NEOGEO_USER ng_status_set(uint16_t status_id);
void NEOGEO_USER ng_status_clear(uint16_t status_id);
uint8_t NEOGEO_USER ng_status_has(uint16_t status_id);
void NEOGEO_USER ng_status_toggle(uint16_t status_id);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif