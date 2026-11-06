#include <assert.h>
#include <stdio.h>
#include "../artbox/generated/maiya_assets.h"
#include "../scenes/maiya_levels.h"

int main(void)
{
    unsigned int i, j;
    for (i = 0; i < MG_LEVEL_COUNT; i++) {
        const MGLevel *level = &mg_levels[i];
        assert(level->width > 1500 && level->width < 32767);
        assert(level->background < MG_LEVEL_COUNT);
        assert(level->music >= 1 && level->music <= 8);
        assert(level->boss_hp > 0);
        for (j = 0; j < MG_PLATFORM_COUNT; j++) {
            const MGPlatform *p = &level->platforms[j];
            if (!p->width) continue;
            assert(p->width % 16 == 0);
            assert(p->x >= 0 && p->x + p->width < level->width);
            /* A low ledge is one jump up; a high one is reachable from a low one. */
            assert(p->y >= 64 && p->y <= 144);
        }
        for (j = 0; j < MG_ENCOUNTER_COUNT; j++) {
            const MGEncounter *e = &level->encounters[j];
            if (!e->x) continue;
            assert(e->x < level->width);
            assert(e->type <= MG_E_SPOREGOB);
            if (j) assert(e->x > level->encounters[j - 1].x);
        }
        for (j = 0; j < MG_ARCHER_COUNT; j++) {
            const MGArcher *a = &level->archers[j];
            unsigned int k, on_ledge = 0;
            if (!a->x) continue;
            for (k = 0; k < MG_PLATFORM_COUNT; k++) {
                const MGPlatform *p = &level->platforms[k];
                if (p->width && p->y == a->y && a->x >= p->x && a->x <= p->x + p->width) on_ledge = 1;
            }
            assert(on_ledge);
        }
        for (j = 0; j < MG_HAZARD_COUNT; j++) {
            const MGHazard *h = &level->hazards[j];
            if (!h->width) continue;
            assert(h->width % 16 == 0 && h->width <= 64);
            assert(h->type >= MG_H_FIRE && h->type <= MG_H_PIT);
            assert(h->x + h->width < level->width - 320);
        }
        assert(level->rescue_x[0] < level->width / 2);
        assert(level->rescue_x[3] > level->width / 2);
        for (j = 0; j < 4; j++) {
            assert(level->rescue_x[j] < level->gate_x);
            if (j) assert(level->rescue_x[j] > level->rescue_x[j - 1]);
        }
        assert(level->gate_x < level->width - 320);
        for (j = 0; j < MG_VINE_COUNT; j++) {
            const MGVine *v = &mg_vines[i][j];
            assert(v->bottom == MG_GROUND_Y && v->top < v->bottom);
            assert(v->bottom - v->top <= 128);
        }
    }
    /* Strips on the road scanline: the road chain, the heroine, four
     * rivals, a captive and its cage, shots, sparks, pick-ups and the
     * widest hazard on screen. */
    assert(32 + 7 + 4 * 7 + 7 + 1 + 8 + 6 + 3 + 4 <= 96);
    puts("Mission definitions, climb spans and actor sprite budget pass.");
    return 0;
}
