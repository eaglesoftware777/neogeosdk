#include <assert.h>
#include <stdio.h>
#include "../scenes/maiya_levels.h"

int main(void)
{
    unsigned int i, j;
    for (i = 0; i < MG_LEVEL_COUNT; i++) {
        const MGLevel *level = &mg_levels[i];
        assert(level->width > 1500 && level->width < 32767);
        assert(level->background < 6);
        assert(level->music >= 1 && level->music <= 7);
        assert(level->boss_hp > 0);
        for (j = 0; j < MG_PLATFORM_COUNT; j++) {
            const MGPlatform *p = &level->platforms[j];
            if (!p->width) continue;
            assert(p->width % 16 == 0);
            assert(p->x >= 0 && p->x + p->width < level->width - 320);
            /* A low ledge is one jump up; a high one is reachable from a low one. */
            assert(p->y == 104 || p->y == 120 || p->y == 144);
        }
        for (j = 0; j < MG_ENCOUNTER_COUNT; j++) {
            const MGEncounter *e = &level->encounters[j];
            if (!e->x) continue;
            assert(e->x < level->width - 310);
            assert(e->type <= MG_E_PAIR);
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
            assert(h->type == MG_H_FIRE || h->type == MG_H_SPIKES);
            assert(h->x + h->width < level->width - 320);
        }
        assert(level->rescue_x[0] < level->width / 2);
        assert(level->rescue_x[1] > level->width / 2);
        assert(level->rescue_x[1] < level->width - 320);
    }
    /* Strips on the road scanline: the road chain, the heroine, four
     * rivals, a captive and its cage, shots, sparks, pick-ups and the
     * widest hazard on screen. */
    assert(32 + 7 + 4 * 7 + 7 + 1 + 8 + 6 + 3 + 4 <= 96);
    puts("Seven mission definitions and the scanline sprite budget pass.");
    return 0;
}
