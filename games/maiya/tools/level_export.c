/*
 * Maiya's level tables as JSON, for tools/level_check.py.
 *
 * Built with the host compiler from the very headers the game is built
 * from, so the checker reads the tables themselves, not a copy or a guess
 * at the C. Unused entries (x == 0, or a width of 0) are left out; every
 * object keeps its index in its own table, so a finding names the entry to
 * change.
 */
#include <stdio.h>
#include "artbox/generated/maiya_assets.h"   /* the MG_K_* pickup kinds */
#include "scenes/maiya_levels.h"

#define PICKUP_SIZE 32   /* pick-up art: 32 x 32, placed by its top-left corner */
#define LEDGE_BLOCK 32   /* a ledge is drawn as 32 x 32 blocks hanging from its top */

/* The width a ledge is drawn at, as mg_draw_ledges() lays its blocks. */
static int drawn_width(int width)
{
    int blocks = (width + 16) / LEDGE_BLOCK;
    return (blocks < 2 ? 2 : blocks) * LEDGE_BLOCK;
}

static void comma(int *first)
{
    if (!*first) printf(",");
    *first = 0;
}

int main(void)
{
    int l, i, first;
    printf("{\"ground_y\": %d, \"pickup_size\": %d, \"ledge_depth\": %d, \"levels\": [\n",
           MG_GROUND_Y, PICKUP_SIZE, LEDGE_BLOCK);
    for (l = 0; l < MG_LEVEL_COUNT; l++) {
        const MGLevel *lv = &mg_levels[l];
        printf("%s{\"index\": %d, \"name\": \"%s\", \"width\": %u, \"gate_x\": %u,\n",
               l ? ",\n" : "", l, lv->name, (unsigned)lv->width, (unsigned)lv->gate_x);

        printf(" \"platforms\": [");
        for (first = 1, i = 0; i < MG_PLATFORM_COUNT; i++) {
            const MGPlatform *p = &lv->platforms[i];
            if (!p->width) continue;
            comma(&first);
            printf("{\"i\": %d, \"x\": %d, \"y\": %d, \"w\": %d, \"draw_w\": %d}",
                   i, p->x, p->y, p->width, drawn_width(p->width));
        }
        printf("],\n \"hazards\": [");
        for (first = 1, i = 0; i < MG_HAZARD_COUNT; i++) {
            const MGHazard *h = &lv->hazards[i];
            if (!h->width) continue;
            comma(&first);
            printf("{\"i\": %d, \"x\": %d, \"w\": %d, \"type\": %u, \"pit\": %s}",
                   i, h->x, h->width, h->type, h->type == MG_H_PIT ? "true" : "false");
        }
        printf("],\n \"pickups\": [");
        for (first = 1, i = 0; i < MG_PICK_COUNT; i++) {
            const MGPickup *p = &mg_picks[l][i];
            if (!p->x) continue;
            comma(&first);
            printf("{\"i\": %d, \"x\": %d, \"y\": %u, \"kind\": %u}", i, p->x, p->y, p->kind);
        }
        printf("],\n \"secrets\": [");
        for (first = 1, i = 0; i < MG_SECRET_COUNT; i++) {
            const MGSecret *s = &lv->secrets[i];
            if (!s->x) continue;
            comma(&first);
            printf("{\"i\": %d, \"x\": %d, \"y\": %d, \"type\": %u}", i, s->x, s->y, s->type);
        }
        printf("],\n \"encounters\": [");
        for (first = 1, i = 0; i < MG_ENCOUNTER_COUNT; i++) {
            const MGEncounter *e = &lv->encounters[i];
            if (!e->x) continue;
            comma(&first);
            printf("{\"i\": %d, \"x\": %d, \"type\": %u}", i, e->x, e->type);
        }
        printf("],\n \"archers\": [");
        for (first = 1, i = 0; i < MG_ARCHER_COUNT; i++) {
            const MGArcher *a = &lv->archers[i];
            if (!a->x) continue;
            comma(&first);
            printf("{\"i\": %d, \"x\": %d, \"y\": %d}", i, a->x, a->y);
        }
        printf("],\n \"rescues\": [");
        for (first = 1, i = 0; i < 4; i++) {
            if (!lv->rescue_x[i]) continue;
            comma(&first);
            printf("{\"i\": %d, \"x\": %u, \"type\": %u}", i, (unsigned)lv->rescue_x[i], lv->rescue_type[i]);
        }
        printf("]}");
    }
    printf("\n]}\n");
    return 0;
}
