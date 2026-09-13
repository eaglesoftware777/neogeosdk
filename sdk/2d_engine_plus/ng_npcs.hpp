#ifndef NG_NPCS_HPP
#define NG_NPCS_HPP

#include "ng_chars.hpp"

struct NGNpc;
typedef void(*NGNpcThink)(NGNpc *npc, NGCharacter *c);

struct NGNpc {
    uint8_t    active;
    uint8_t    npc_kind;
    uint8_t    char_slot;
    uint8_t    team;
    uint16_t   flags;
    uint16_t   think_interval;
    uint16_t   think_timer;
    uint16_t   state;
    int16_t    home_x;
    int16_t    home_y;
    int16_t    min_x;
    int16_t    max_x;
    int16_t    min_y;
    int16_t    max_y;
    uint16_t   data0;
    uint16_t   data1;
    NGNpcThink think;
};

#define NG_NPC_FLAG_PATROL_X    0x0001
#define NG_NPC_FLAG_PATROL_Y    0x0002
#define NG_NPC_FLAG_FACE_MOTION 0x0004

/*
 * NpcManager — singleton that owns the NPC pool and drives per-frame think.
 */
class NpcManager {
public:
    static NpcManager& instance();

    void      NEOGEO_USER init();
    NGNpc*    NEOGEO_USER spawn(uint8_t npc_kind, uint8_t char_kind, int16_t x, int16_t y);
    void      NEOGEO_USER remove(NGNpc *npc);
    NGNpc*    NEOGEO_USER at(uint8_t index) const;
    NGNpc*    NEOGEO_USER find(uint8_t npc_kind) const;
    NGCharacter* NEOGEO_USER charOf(NGNpc *npc) const;
    uint8_t   NEOGEO_USER count() const;
    void      NEOGEO_USER setThink(NGNpc *npc, NGNpcThink think, uint16_t interval_frames);
    void      NEOGEO_USER setHome(NGNpc *npc, int16_t x, int16_t y);
    void      NEOGEO_USER setPatrolBounds(NGNpc *npc, int16_t min_x, int16_t max_x, int16_t min_y, int16_t max_y);
    void      NEOGEO_USER update();

    static void NEOGEO_USER thinkPatrol(NGNpc *npc, NGCharacter *c);
    static void NEOGEO_USER thinkHover(NGNpc *npc, NGCharacter *c);

private:
    NpcManager() {}
    NGNpc pool[NG_MAX_NPCS];
};

#ifdef __cplusplus
extern "C" {
#endif

void         NEOGEO_USER ng_npcs_init(void);
NGNpc*       npc_spawn(uint8_t npc_kind, uint8_t char_kind, int16_t x, int16_t y);
void         NEOGEO_USER ng_npc_remove(NGNpc *npc);
NGNpc*       npc_at(uint8_t index);
NGNpc*       npc_find(uint8_t npc_kind);
NGCharacter* npc_char(NGNpc *npc);
uint8_t      NEOGEO_USER ng_npc_count(void);
void         NEOGEO_USER ng_npc_set_think(NGNpc *npc, NGNpcThink think, uint16_t interval_frames);
void         NEOGEO_USER ng_npc_set_home(NGNpc *npc, int16_t x, int16_t y);
void         NEOGEO_USER ng_npc_set_patrol_bounds(NGNpc *npc, int16_t min_x, int16_t max_x, int16_t min_y, int16_t max_y);
void         NEOGEO_USER ng_npcs_update(void);
void         NEOGEO_USER ng_npc_think_patrol(NGNpc *npc, NGCharacter *c);
void         NEOGEO_USER ng_npc_think_hover(NGNpc *npc, NGCharacter *c);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
