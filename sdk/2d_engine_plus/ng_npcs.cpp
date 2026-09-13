#include "ng_npcs.hpp"

/* --- NpcManager singleton --- */

NpcManager& NpcManager::instance()
{
    static NpcManager mgr;
    return mgr;
}

/* --- NpcManager public methods --- */

void NpcManager::init()
{
    uint8_t i;
    for (i = 0; i < NG_MAX_NPCS; i++) pool[i].active = 0;
}

NGNpc* NpcManager::spawn(uint8_t npc_kind, uint8_t char_kind, int16_t x, int16_t y)
{
    uint8_t i;
    NGCharacter *c = chars_add(char_kind, x, y);
    if (!c) return 0;

    for (i = 0; i < NG_MAX_NPCS; i++) {
        NGNpc *npc = &pool[i];
        if (!npc->active) {
            npc->active         = 1;
            npc->npc_kind       = npc_kind;
            npc->char_slot      = ng_chars_index(c);
            npc->team           = 0;
            npc->flags          = 0;
            npc->think_interval = 1;
            npc->think_timer    = 0;
            npc->state          = 0;
            npc->home_x = x;  npc->home_y = y;
            npc->min_x  = x;  npc->max_x  = x;
            npc->min_y  = y;  npc->max_y  = y;
            npc->data0  = 0;  npc->data1  = 0;
            npc->think  = 0;
            return npc;
        }
    }

    ng_chars_remove(c);
    return 0;
}

void NpcManager::remove(NGNpc *npc)
{
    NGCharacter *c;
    if (!npc || !npc->active) return;
    c = charOf(npc);
    if (c) ng_chars_remove(c);
    npc->active = 0;
}

NGNpc* NpcManager::at(uint8_t index) const
{
    if (index >= NG_MAX_NPCS) return 0;
    return const_cast<NGNpc*>(&pool[index]);
}

NGNpc* NpcManager::find(uint8_t npc_kind) const
{
    uint8_t i;
    for (i = 0; i < NG_MAX_NPCS; i++) {
        if (pool[i].active && pool[i].npc_kind == npc_kind)
            return const_cast<NGNpc*>(&pool[i]);
    }
    return 0;
}

NGCharacter* NpcManager::charOf(NGNpc *npc) const
{
    if (!npc || !npc->active) return 0;
    return chars_at(npc->char_slot);
}

uint8_t NpcManager::count() const
{
    uint8_t i, n = 0;
    for (i = 0; i < NG_MAX_NPCS; i++)
        if (pool[i].active) n++;
    return n;
}

void NpcManager::setThink(NGNpc *npc, NGNpcThink think, uint16_t interval_frames)
{
    if (!npc) return;
    npc->think          = think;
    npc->think_interval = interval_frames ? interval_frames : 1;
    npc->think_timer    = 0;
}

void NpcManager::setHome(NGNpc *npc, int16_t x, int16_t y)
{
    if (!npc) return;
    npc->home_x = x; npc->home_y = y;
}

void NpcManager::setPatrolBounds(NGNpc *npc, int16_t min_x, int16_t max_x, int16_t min_y, int16_t max_y)
{
    if (!npc) return;
    npc->min_x = min_x; npc->max_x = max_x;
    npc->min_y = min_y; npc->max_y = max_y;
}

void NpcManager::update()
{
    uint8_t i;

    for (i = 0; i < NG_MAX_NPCS; i++) {
        NGNpc *npc = &pool[i];
        NGCharacter *c;

        if (!npc->active) continue;
        c = charOf(npc);
        if (!c || !c->active) { npc->active = 0; continue; }

        if (npc->think_timer > 0) { npc->think_timer--; continue; }
        npc->think_timer = (uint16_t)(npc->think_interval - 1u);
        if (npc->think) npc->think(npc, c);
    }
}

void NpcManager::thinkPatrol(NGNpc *npc, NGCharacter *c)
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

void NpcManager::thinkHover(NGNpc *npc, NGCharacter *c)
{
    if (!npc || !c) return;
    npc->data0 = (uint16_t)((npc->data0 + 1u) & 31u);
    c->y_fp = NG_TO_FP((int16_t)(npc->home_y + ((npc->data0 & 16u) ? 2 : 0)));
    c->y    = NG_FROM_FP(c->y_fp);
}

/* --- extern "C" wrappers --- */

extern "C" {

void NEOGEO_USER ng_npcs_init(void)
{
    NpcManager::instance().init();
}

NGNpc* npc_spawn(uint8_t npc_kind, uint8_t char_kind, int16_t x, int16_t y)
{
    return NpcManager::instance().spawn(npc_kind, char_kind, x, y);
}

void NEOGEO_USER ng_npc_remove(NGNpc *npc)
{
    NpcManager::instance().remove(npc);
}

NGNpc* npc_at(uint8_t index)
{
    return NpcManager::instance().at(index);
}

NGNpc* npc_find(uint8_t npc_kind)
{
    return NpcManager::instance().find(npc_kind);
}

NGCharacter* npc_char(NGNpc *npc)
{
    return NpcManager::instance().charOf(npc);
}

uint8_t NEOGEO_USER ng_npc_count(void)
{
    return NpcManager::instance().count();
}

void NEOGEO_USER ng_npc_set_think(NGNpc *npc, NGNpcThink think, uint16_t interval_frames)
{
    NpcManager::instance().setThink(npc, think, interval_frames);
}

void NEOGEO_USER ng_npc_set_home(NGNpc *npc, int16_t x, int16_t y)
{
    NpcManager::instance().setHome(npc, x, y);
}

void NEOGEO_USER ng_npc_set_patrol_bounds(NGNpc *npc, int16_t min_x, int16_t max_x, int16_t min_y, int16_t max_y)
{
    NpcManager::instance().setPatrolBounds(npc, min_x, max_x, min_y, max_y);
}

void NEOGEO_USER ng_npcs_update(void)
{
    NpcManager::instance().update();
}

void NEOGEO_USER ng_npc_think_patrol(NGNpc *npc, NGCharacter *c)
{
    NpcManager::thinkPatrol(npc, c);
}

void NEOGEO_USER ng_npc_think_hover(NGNpc *npc, NGCharacter *c)
{
    NpcManager::thinkHover(npc, c);
}

} /* extern "C" */
