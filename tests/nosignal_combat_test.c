/*
 * No Signal's moves on the engine (tests/test_nosignal_combat.py builds it):
 * the heroes' A chain lands every blow from as far as the jab reaches, a
 * blow misses a step past what its picture reaches and a lane away, and the
 * crowd's strikes reach a hero standing where they stop to attack.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "games/nosignal/scenes/ns.h"
#include "games/nosignal/scenes/ns_data.c"

static NGBeatEmUp B;
static NGBStage ST;
static NGBFighterDef FOE;
static int hits, knockdowns;

static void frames(uint8_t dirs, uint8_t btns, int n)
{
    NGBEvent e;
    while (n-- > 0) {
        ng_beatemup_input(&B, dirs, btns);
        ng_beatemup_update(&B);
        while (ng_beatemup_event(&B, &e)) {
            if (e.type == NGB_EV_HIT && e.b != NGB_PLAYER) hits++;
            if (e.type == NGB_EV_KNOCKDOWN && e.b != NGB_PLAYER) knockdowns++;
        }
    }
}

/* the hero facing right at x 200, depth 170; a foe that stands still dx ahead, dy across */
static uint8_t arena(const NGBFighterDef *hero, const NGBFighterDef *foe, int dx, int dy)
{
    int r;
    memset(&ST, 0, sizeof(ST));
    ST.length = 1200; ST.y_min = 120; ST.y_max = 220; ST.view_w = 320; ST.max_attackers = 2; ST.max_active = 6;
    ST.camera_max = 6;
    memcpy(&FOE, foe, sizeof(FOE));
    FOE.speed = 0; FOE.depth_speed = 0; FOE.max_hp = 250;
    assert(ng_beatemup_init(&B, &ST, hero, 3, 7) == NGB_OK);
    B.camera_x = 40 << 8;
    B.f[0].x = 200 << 8; B.f[0].y = 170 << 8; B.f[0].facing = 0;
    r = ng_beatemup_spawn(&B, &FOE, (int16_t)(200 + dx), (uint8_t)(170 + dy), 2);
    assert(r > 0);
    B.attack_cool = 255;                      /* it never attacks */
    hits = knockdowns = 0;
    return (uint8_t)r;
}

static const char *name_of(const NGBFighterDef *d)
{
    uint8_t i;
    for (i = 0; i < ns_nlooks; i++) if (ns_looks[i].def == d) return ns_looks[i].name;
    return "?";
}

static int reach(const NGBFighterDef *hero, uint8_t move, const NGBFighterDef *foe)
{
    const NGBMove *m = &hero->moves[move];
    return m->box_x + m->box_w + foe->half_w;
}

int main(void)
{
    /* the guard is left out: it blocks from the front (throws and the special open it) */
    static const NGBFighterDef *const crew[] = { &ns_thief, &ns_skater, &ns_runner, &ns_android, &ns_biker, &ns_bloater };
    uint8_t h, c, e, presses;
    for (h = 0; h < NS_HEROES; h++) {
        const NGBFighterDef *hero = &ns_heroes[h];
        for (c = 0; c < sizeof(crew) / sizeof(crew[0]); c++) {
            const NGBFighterDef *foe = crew[c];
            int far = reach(hero, hero->chain, foe) - 1;
            /* A A A A from as far as the jab reaches: four blows, the last knocks down */
            e = arena(hero, foe, far, 0);
            for (presses = 0; presses < 40; presses++) frames(0, (presses & 3) == 0 ? NGB_BTN_A : 0, 1);
            frames(0, 0, 30);
            if (hits < 4 || !knockdowns)
                fprintf(stderr, "FAIL: %s's chain on %s from %d: %d blows, %d down\n", ns_hero_info[h].name,
                        name_of(foe), far, hits, knockdowns);
            assert(hits >= 4 && knockdowns >= 1);
            /* a jab a step past the fist misses; one a lane away (17 pixels across) misses */
            e = arena(hero, foe, far + 4, 0);
            frames(0, NGB_BTN_A, 1); frames(0, 0, 20);
            assert(hits == 0);
            e = arena(hero, foe, far - 8, 17);
            frames(0, NGB_BTN_A, 1); frames(0, 0, 20);
            assert(hits == 0);
            e = arena(hero, foe, far - 8, 10);
            frames(0, NGB_BTN_A, 1); frames(0, 0, 20);
            assert(hits == 1);
            (void)e;
        }
    }
    /* a brawler stops `reach` pixels from the hero to strike: its blows reach that far */
    {
        static const NGBFighterDef *const brawlers[] = { &ns_thief, &ns_guard, &ns_bloater, &ns_android, &ns_walker };
        for (c = 0; c < sizeof(brawlers) / sizeof(brawlers[0]); c++) {
            const NGBFighterDef *d = brawlers[c];
            uint8_t k;
            for (k = 0; k < d->nattacks; k++) {
                const NGBMove *m = &d->moves[d->attacks[k]];
                if (m->box_x + m->box_w + 11 < d->reach) fprintf(stderr, "FAIL: %s's move %d is short of its reach\n", name_of(d), d->attacks[k]);
                assert(m->box_x + m->box_w + 11 >= d->reach);
            }
        }
    }
    printf("No Signal: chains land from the jab's reach, blows miss past the fist and a lane away, the crowd reaches where it strikes from\n");
    return 0;
}
