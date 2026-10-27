/* The two depth-layout banks must fit every shipped stage on expert. */
#include <assert.h>
#include <stdio.h>
#include "sdk/2d_engine/ng_art_asset.h"
#include "games/nosignal/scenes/ns.h"
#include "games/nosignal/artbox/generated/ns_assets.h"
#include "games/nosignal/scenes/ns_data.c"

static unsigned strips_for(const NGBFighterDef *def)
{
    unsigned i;
    for (i = 0; i < ns_nlooks; i++)
        if (ns_looks[i].def == def) return ns_art[ns_looks[i].art - 1u].strips;
    assert(!"fighter missing an art mapping");
    return 0;
}

static void check_stage(const NGBStage *stage)
{
    unsigned i, maximum = 0u, heroes = 0u, expert = stage->max_active + 1u;
    for (i = 0; i < NS_HEROES; i++) {
        unsigned width = strips_for(&ns_heroes[i]);
        if (width > heroes) heroes = width;
        assert(strips_for(&ns_heroes_space[i]) == width);
    }
    for (i = 0; i < stage->nenemies; i++) {
        unsigned width = strips_for(stage->enemies[i]);
        if (width > maximum) maximum = width;
    }
    /* Conservatively allow every simultaneous enemy to be the widest,
     * including the boss. This also covers runtime reinforcements. */
    assert(stage->max_active && expert <= NGB_MAX_ENEMIES);
    assert(heroes + expert * maximum <= 80u);
}

int main(void)
{
    unsigned i;
    for (i = 0; i < NS_STAGES; i++) check_stage(ns_stages[i].stage);
    puts("No Signal: all 8 stages fit 80-strip banks on expert");
    return 0;
}
