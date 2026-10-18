/* Shadow League's cooked fighters in full CPU matches on the host. */
#include <assert.h>
#include <stdio.h>
#include "ng_fight.h"
#include "games/shadowleague/artbox/generated/slf_assets.h"

int main(void)
{
    static NGFMatch m;
    uint8_t a, b, lv;
    unsigned long total = 0, hits = 0, blocks = 0, specials = 0;
    for (a = 0; a < SLF_FIGHTER_COUNT; a++) for (b = 0; b < SLF_FIGHTER_COUNT; b++) for (lv = 2; lv <= 8; lv += 3) {
        uint32_t n = 0;
        NGFEvent e;
        assert(slf_fighter_defs[a]->hit_sets && slf_fighter_defs[a]->frame_hits);
        assert(slf_fighter_defs[b]->hit_sets && slf_fighter_defs[b]->frame_hits);
        ngf_match_init(&m, slf_fighter_defs[a], slf_fighter_defs[b], SLF_STAGE_W, (uint16_t)(a * 31 + b * 7 + lv));
        ngf_set_cpu(&m, 0, lv);
        ngf_set_cpu(&m, 1, (uint8_t)(10 - lv));
        while (m.state != NGF_M_OVER && n < 80000u) {
            ngf_match_step(&m, 0, 0);
            while (ngf_pop_event(&m, &e)) {
                hits += e.type == NGF_EV_HIT;
                blocks += e.type == NGF_EV_BLOCK;
                specials += e.type == NGF_EV_MOVE && e.data >= NGF_KIND_SPECIAL;
            }
            n++;
        }
        assert(m.state == NGF_M_OVER);
        total += n;
        printf("%-12s vs %-12s L%u: %5lu frames, %u rounds, winner %u\n", slf_fighter_defs[a]->name,
               slf_fighter_defs[b]->name, lv, (unsigned long)n, m.round, m.winner);
    }
    assert(hits > 50 && blocks > 10 && specials > 10);
    printf("shadowleague_data_test: ok (%lu frames, %lu hits, %lu blocks, %lu specials)\n", total, hits, blocks, specials);
    return 0;
}
