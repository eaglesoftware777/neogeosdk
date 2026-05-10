#ifndef NG_BORDER_CONSTRAINTS_H
#define NG_BORDER_CONSTRAINTS_H

#include "ng_defs.h"

/*
 * Border Constraint: invisible rectangle/area that sends a game_event.
 * It uses the Properties Matrix player values at time T.
 */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    uint16_t event_id;
    uint16_t a;
    uint16_t b;
    uint8_t once;
    uint8_t used;
} NGBorderConstraint;

void NEOGEO_USER ng_border_constraints_init(void);
void NEOGEO_USER ng_border_constraints_load(NGBorderConstraint *list, uint16_t count);
void NEOGEO_USER ng_border_constraints_update(void);
void NEOGEO_USER ng_border_constraints_reset_used(void);

#endif
