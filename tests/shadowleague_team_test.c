#include <assert.h>
#include <stdio.h>
#include "ng_fight.h"
#include "games/shadowleague/artbox/generated/slf_assets.h"

int main(void)
{
    static NGFMatch m;
    uint8_t captain, side;
    unsigned long total = 0, calls = 0, entries = 0;
    for (captain = 0; captain < SLF_FIGHTER_COUNT; captain++) {
        uint32_t frames = 0;
        NGFEvent event;
        ngf_match_init(&m, slf_fighter_defs[captain], slf_fighter_defs[(captain + 19u) % SLF_FIGHTER_COUNT],
            SLF_STAGE_W, (uint16_t)(captain * 31u + 9u));
        ngf_match_set_mode(&m, NGF_MODE_3V3);
        for (side = 0; side < 2u; side++) {
            uint8_t base = (uint8_t)((captain + side * 19u) % SLF_FIGHTER_COUNT);
            ngf_team_set(&m, side, slf_fighter_defs[base],
                slf_fighter_defs[(base + 7u) % SLF_FIGHTER_COUNT],
                slf_fighter_defs[(base + 14u) % SLF_FIGHTER_COUNT]);
            ngf_striker_set(&m, side, slf_fighter_defs[(base + 21u) % SLF_FIGHTER_COUNT]);
            ngf_set_cpu(&m, side, (uint8_t)(5u + side * 2u));
        }
        while (m.state != NGF_M_OVER && frames < 80000u) {
            ngf_match_step(&m, 0, 0);
            while (ngf_pop_event(&m, &event)) {
                calls += event.type == NGF_EV_STRIKER;
                entries += event.type == NGF_EV_TAG;
            }
            for (side = 0; side < 2u; side++) {
                const NGFFighter *f = &m.f[side];
                assert(f->life <= f->def->life);
                assert(m.team_active[side] < 3u && m.assist_uses[side] <= 3u);
                assert(f->anim < f->def->anim_count && f->frame < f->def->anims[f->anim].count);
            }
            frames++;
        }
        assert(m.state == NGF_M_OVER && m.winner <= 2u);
        if (m.winner < 2u) assert(m.team_active[m.winner ^ 1u] == 2u);
        total += frames;
    }
    assert(calls > 0 && entries >= 2u * SLF_FIGHTER_COUNT);
    printf("shadowleague_team_test: ok (%lu frames, %lu assists, %lu reserve entries)\n", total, calls, entries);
    return 0;
}
