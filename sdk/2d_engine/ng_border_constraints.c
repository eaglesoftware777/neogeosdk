#include "ng_border_constraints.h"
#include "ng_properties.h"
#include "ng_game_events.h"

static NGBorderConstraint *ng_borders;
static uint16_t ng_border_count;

void NEOGEO_USER ng_border_constraints_init(void)
{
    ng_borders = 0;
    ng_border_count = 0;
}

void NEOGEO_USER ng_border_constraints_load(NGBorderConstraint *list, uint16_t count)
{
    ng_borders = list;
    ng_border_count = count;
}

void NEOGEO_USER ng_border_constraints_update(void)
{
    uint16_t i;
    NGRect player;

    if (!ng_borders) return;

    player.x = (int16_t)ng_prop_get(NG_PROP_GROUP_PLAYER, NG_PROP_PLAYER_X);
    player.y = (int16_t)ng_prop_get(NG_PROP_GROUP_PLAYER, NG_PROP_PLAYER_Y);
    player.w = (int16_t)ng_prop_get(NG_PROP_GROUP_PLAYER, NG_PROP_PLAYER_W);
    player.h = (int16_t)ng_prop_get(NG_PROP_GROUP_PLAYER, NG_PROP_PLAYER_H);

    if (player.w == 0) player.w = 16;
    if (player.h == 0) player.h = 16;

    for (i = 0; i < ng_border_count; i++) {
        NGBorderConstraint *b = &ng_borders[i];
        NGRect r;

        if (b->once && b->used) continue;

        r.x = b->x;
        r.y = b->y;
        r.w = b->w;
        r.h = b->h;

        if (ng_rect_hit(player, r)) {
            ng_game_events_send(b->event_id, b->a, b->b, 0);
            b->used = 1;
        }
    }
}

void NEOGEO_USER ng_border_constraints_reset_used(void)
{
    uint16_t i;
    if (!ng_borders) return;
    for (i = 0; i < ng_border_count; i++) ng_borders[i].used = 0;
}
