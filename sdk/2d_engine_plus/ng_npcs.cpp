#include "ng_npcs.hpp"

extern "C" {


static NGNpc ng_npcs[NG_MAX_NPCS];

void NEOGEO_USER ng_npcs_init(void)
{
    uint8_t i;

    for (i = 0; i < NG_MAX_NPCS; i++) {
        ng_npcs[i].active = 0;
    }
}

NGNpc *NEOGEO_USER npc_spawn(uint8_t npc_kind, uint8_t char_kind, int16_t x, int16_t y)
{
    uint8_t i;
    NGCharacter *c = chars_add(char_kind, x, y);

    if (!c) return 0;

    for (i = 0; i < NG_MAX_NPCS; i++) {
        NGNpc *npc = &ng_npcs[i];
        if (!npc->active) {
            npc->active = 1;
            npc->npc_kind = npc_kind;
            npc->char_slot = ng_chars_index(c);
            npc->team = 0;
            npc->flags = 0;
            npc->think_interval = 1;
            npc->think_timer = 0;
            npc->state = 0;
            npc->home_x = x;
            npc->home_y = y;
            npc->min_x = x;
            npc->max_x = x;
            npc->min_y = y;
            npc->max_y = y;
            npc->data0 = 0;
            npc->data1 = 0;
            npc->think = 0;
            return npc;
        }
    }

    ng_chars_remove(c);
    return 0;
}

void NEOGEO_USER ng_npc_remove(NGNpc *npc)
{
    NGCharacter *c;

    if (!npc || !npc->active) return;
    c = npc_char(npc);
    if (c) ng_chars_remove(c);
    npc->active = 0;
}

NGNpc *NEOGEO_USER npc_at(uint8_t index)
{
    if (index >= NG_MAX_NPCS) return 0;
    return &ng_npcs[index];
}

NGNpc *NEOGEO_USER npc_find(uint8_t npc_kind)
{
    uint8_t i;

    for (i = 0; i < NG_MAX_NPCS; i++) {
        if (ng_npcs[i].active && ng_npcs[i].npc_kind == npc_kind) return &ng_npcs[i];
    }
    return 0;
}

NGCharacter *NEOGEO_USER npc_char(NGNpc *npc)
{
    if (!npc || !npc->active) return 0;
    return chars_at(npc->char_slot);
}

uint8_t NEOGEO_USER ng_npc_count(void)
{
    uint8_t i;
    uint8_t count = 0;

    for (i = 0; i < NG_MAX_NPCS; i++) {
        if (ng_npcs[i].active) count++;
    }
    return count;
}

void NEOGEO_USER ng_npc_set_think(NGNpc *npc, NGNpcThink think, uint16_t interval_frames)
{
    if (!npc) return;
    npc->think = think;
    npc->think_interval = interval_frames ? interval_frames : 1;
    npc->think_timer = 0;
}

void NEOGEO_USER ng_npc_set_home(NGNpc *npc, int16_t x, int16_t y)
{
    if (!npc) return;
    npc->home_x = x;
    npc->home_y = y;
}

void NEOGEO_USER ng_npc_set_patrol_bounds(
    NGNpc *npc, int16_t min_x, int16_t max_x, int16_t min_y, int16_t max_y
)
{
    if (!npc) return;
    npc->min_x = min_x;
    npc->max_x = max_x;
    npc->min_y = min_y;
    npc->max_y = max_y;
}

void NEOGEO_USER ng_npc_think_patrol(NGNpc *npc, NGCharacter *c)
{
    if (!npc || !c) return;

    if (npc->flags & NG_NPC_FLAG_PATROL_X) {
        if (c->x <= npc->min_x) c->vx_fp = NG_TO_FP(1);
        if (c->x >= npc->max_x) c->vx_fp = NG_TO_FP(-1);
    }
    if (npc->flags & NG_NPC_FLAG_PATROL_Y) {
        if (c->y <= npc->min_y) c->vy_fp = NG_TO_FP(1);
        if (c->y >= npc->max_y) c->vy_fp = NG_TO_FP(-1);
    }
    if (npc->flags & NG_NPC_FLAG_FACE_MOTION) {
        if (c->vx_fp > 0) c->facing = 0;
        else if (c->vx_fp < 0) c->facing = 1;
    }
}

void NEOGEO_USER ng_npc_think_hover(NGNpc *npc, NGCharacter *c)
{
    if (!npc || !c) return;

    npc->data0 = (uint16_t)((npc->data0 + 1u) & 31u);
    c->y_fp = NG_TO_FP((int16_t)(npc->home_y + ((npc->data0 & 16u) ? 2 : 0)));
    c->y = NG_FROM_FP(c->y_fp);
}

void NEOGEO_USER ng_npcs_update(void)
{
    uint8_t i;

    for (i = 0; i < NG_MAX_NPCS; i++) {
        NGNpc *npc = &ng_npcs[i];
        NGCharacter *c;

        if (!npc->active) continue;
        c = npc_char(npc);
        if (!c || !c->active) {
            npc->active = 0;
            continue;
        }

        if (npc->think_timer > 0) {
            npc->think_timer--;
            continue;
        }
        npc->think_timer = (uint16_t)(npc->think_interval - 1u);
        if (npc->think) {
            npc->think(npc, c);
        }
    }
}


} /* extern "C" */
