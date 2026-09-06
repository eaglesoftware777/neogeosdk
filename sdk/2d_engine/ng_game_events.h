#ifndef NG_GAME_EVENTS_H
#define NG_GAME_EVENTS_H

#include "ng_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t id;
    uint16_t a;
    uint16_t b;
    uint16_t c;
} NGGameEvent;

typedef void(*NGGameEventHandler)(const NGGameEvent *e);

void NEOGEO_USER ng_game_events_init(void);
void NEOGEO_USER ng_game_events_set_handler(NGGameEventHandler handler);
uint8_t NEOGEO_USER ng_game_events_send(uint16_t id, uint16_t a, uint16_t b, uint16_t c);
uint8_t NEOGEO_USER ng_game_events_read(NGGameEvent *out);
void NEOGEO_USER ng_game_events_update(void);
uint8_t NEOGEO_USER ng_game_events_count(void);


#ifdef __cplusplus
}
#endif
#endif
