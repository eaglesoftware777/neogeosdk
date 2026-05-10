#include "ng_game_events.h"

static NGGameEvent ng_game_events[NG_MAX_GAME_EVENTS];
static uint8_t ng_game_events_head;
static uint8_t ng_game_events_tail;
static NGGameEventHandler ng_game_events_handler;

static uint8_t NEOGEO_USER ng_game_events_next(uint8_t v)
{
    v++;
    if (v >= NG_MAX_GAME_EVENTS) v = 0;
    return v;
}

void NEOGEO_USER ng_game_events_init(void)
{
    ng_game_events_head = 0;
    ng_game_events_tail = 0;
    ng_game_events_handler = 0;
}

void NEOGEO_USER ng_game_events_set_handler(NGGameEventHandler handler)
{
    ng_game_events_handler = handler;
}

uint8_t NEOGEO_USER ng_game_events_send(uint16_t id, uint16_t a, uint16_t b, uint16_t c)
{
    uint8_t next = ng_game_events_next(ng_game_events_tail);
    if (next == ng_game_events_head) {
        return 0; /* queue full */
    }

    ng_game_events[ng_game_events_tail].id = id;
    ng_game_events[ng_game_events_tail].a = a;
    ng_game_events[ng_game_events_tail].b = b;
    ng_game_events[ng_game_events_tail].c = c;
    ng_game_events_tail = next;
    return 1;
}

uint8_t NEOGEO_USER ng_game_events_read(NGGameEvent *out)
{
    if (ng_game_events_head == ng_game_events_tail) return 0;
    if (out) *out = ng_game_events[ng_game_events_head];
    ng_game_events_head = ng_game_events_next(ng_game_events_head);
    return 1;
}

void NEOGEO_USER ng_game_events_update(void)
{
    NGGameEvent e;
    while (ng_game_events_read(&e)) {
        if (ng_game_events_handler) {
            ng_game_events_handler(&e);
        }
    }
}

uint8_t NEOGEO_USER ng_game_events_count(void)
{
    uint8_t count = 0;
    uint8_t i = ng_game_events_head;
    while (i != ng_game_events_tail) {
        count++;
        i = ng_game_events_next(i);
    }
    return count;
}
