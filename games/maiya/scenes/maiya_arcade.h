#ifndef MAIYA_ARCADE_H
#define MAIYA_ARCADE_H
#include <stdint.h>

enum {
    MG_A_INTRO, MG_A_HAZARD, MG_A_MINIBOSS, MG_A_GAUNTLET, MG_A_BOSS,
    MG_A_BRIDGE=0, MG_A_RAFT, MG_A_SWAY, MG_A_BRUSHFIRE, MG_A_MINECART,
    MG_A_LIFT, MG_A_CONVEYOR, MG_A_BUOYANCY, MG_A_ICE, MG_A_QUICKSAND,
    MG_A_FLAK, MG_A_REACTOR
};

typedef struct {
    uint16_t end[5];
    int16_t miniboss_x;
    uint8_t enemy, hp, gimmick, platform;
    const char *name;
} MGArcadeProfile;

static inline uint8_t mg_arcade_phase(const MGArcadeProfile *p, int16_t x)
{
    uint8_t i;
    for (i=0; i<4u; ++i) if (x<(int16_t)p->end[i]) return i;
    return MG_A_BOSS;
}

/* An integer triangle, +/-8 px, with no divide or sine in the world step. */
static inline int8_t mg_arcade_sway(uint16_t tick)
{
    uint8_t t=(uint8_t)(tick&127u);
    return (int8_t)((t<64u ? t : 127u-t)/4u)-8;
}

static inline uint8_t mg_arcade_multiplier(uint8_t chain)
{ return chain>10u ? 10u : chain ? chain : 1u; }

/* Performance rank is restrained and reversible; lives lost lower it. */
static inline uint8_t mg_arcade_rank(uint8_t chain, uint8_t lives, uint8_t hits)
{
    uint8_t rank=(uint8_t)((chain>=5u)+(lives>=4u));
    return hits>=3u || lives<2u ? 0u : rank;
}

/* 0 misses the body, 1 touches armour, 2 reaches its authored weak region.
 * Coordinates are feet-relative; this is independent of canvas padding. */
static inline uint8_t mg_arcade_boss_contact(int16_t x,int16_t y,
    int16_t bx,int16_t by,uint8_t style)
{
    int16_t dx=(int16_t)(x-bx),dy=(int16_t)(y-by);
    if (dx<=-40 || dx>=40 || dy<=-90 || dy>0) return 0;
    if (style==0u) return (uint8_t)(dx>-29 && dx<29 && dy>-42 && dy<-5 ? 2 : 1);
    if (style==4u || style==6u) return (uint8_t)(dx>-26 && dx<26 && dy>-76 && dy<-22 ? 2 : 1);
    return (uint8_t)(dx>-32 && dx<32 && dy>-72 && dy<-8 ? 2 : 1);
}

#endif
