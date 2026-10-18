/* Shadow League's controls with the cooked fighters, entered the way a player does. */
#include <assert.h>
#include <stdio.h>
#include "ng_fight.h"
#include "games/shadowleague/artbox/generated/slf_assets.h"

static NGFMatch m;
static NGFEvent seen[64];
static uint8_t seen_n;

static uint8_t pad(uint8_t numpad) { return ngf_raw_from_numpad(numpad, m.f[0].facing_left); }

static void step(uint8_t raw0, uint8_t raw1)
{
    NGFEvent e;
    ngf_match_step(&m, raw0, raw1);
    while (ngf_pop_event(&m, &e)) if (seen_n < 64u) seen[seen_n++] = e;
}

static void hold(uint8_t raw0, uint8_t frames) { while (frames--) step(raw0, 0); }

static void start(uint8_t def, int16_t gap)
{
    ngf_match_init(&m, slf_fighter_defs[def], slf_fighter_defs[def], SLF_STAGE_W, 3);
    while (m.state != NGF_M_FIGHT) step(0, 0);
    m.f[0].x = (int32_t)(SLF_STAGE_W / 2 - gap / 2) << 8;
    m.f[1].x = (int32_t)(SLF_STAGE_W / 2 + gap / 2) << 8;
    hold(0, 4);
    seen_n = 0;
}

/* The kind of the first move side 0 started since the last start(), 0xFF none. */
static uint8_t started(void)
{
    uint8_t i;
    for (i = 0; i < seen_n; i++) if (seen[i].type == NGF_EV_MOVE && seen[i].who == 0u) return seen[i].data;
    return 0xFFu;
}

static uint8_t happened(uint8_t type)
{
    uint8_t i;
    for (i = 0; i < seen_n; i++) if (seen[i].type == type && seen[i].who == 0u) return 1u;
    return 0;
}

/* Every special and super from its stick motion at six frames a direction. */
static unsigned specials(uint8_t def)
{
    static const uint8_t pads[][6] = {{0}, {2, 3, 6}, {2, 1, 4}, {6, 2, 3}, {4, 2, 1}, {4, 1, 2, 3, 6}, {6, 3, 2, 1, 4},
                                      {2, 3, 6, 2, 3, 6}, {2, 1, 4, 2, 1, 4}};
    static const uint8_t lens[] = {0, 3, 3, 3, 3, 5, 5, 6, 6};
    const NGFFighterDef *d = slf_fighter_defs[def];
    unsigned n = 0;
    uint8_t k, i;
    for (k = 0; k < d->move_count; k++) {
        const NGFMove *mv = &d->moves[k];
        if ((mv->kind != NGF_KIND_SPECIAL && mv->kind != NGF_KIND_SUPER) || !mv->buttons ||
            mv->motion < NGF_MOTION_QCF || mv->motion > NGF_MOTION_DQCB) continue;
        start(def, 160);
        m.f[0].power = NGF_POWER_MAX;
        for (i = 0; i < lens[mv->motion]; i++) hold(pad(pads[mv->motion][i]), 6);
        hold((uint8_t)(pad(pads[mv->motion][lens[mv->motion] - 1u]) | mv->buttons), 1);
        hold(0, 2);
        if (m.f[0].move_index != k || started() != mv->kind) {
            fprintf(stderr, "%s: move %u (motion %u, buttons %02x) did not start\n", d->name, k, mv->motion, mv->buttons);
            assert(0);
        }
        n++;
    }
    return n;
}

static void two_buttons(uint8_t def)
{
    /* A then B a frame later: the roll, not the jab. */
    start(def, 160);
    hold(NGF_BTN_A, 1);
    hold(NGF_BTN_A | NGF_BTN_B, 1);
    hold(0, 2);
    assert(m.f[0].state == NGF_S_ATTACK && m.f[0].move->kind == NGF_KIND_EVADE);
    /* C and D together: the blowback. */
    start(def, 160);
    hold(NGF_BTN_C | NGF_BTN_D, 1);
    hold(0, 2);
    assert(m.f[0].state == NGF_S_ATTACK && m.f[0].move->buttons == (NGF_BTN_C | NGF_BTN_D));
    assert(m.f[0].move->kind == NGF_KIND_COMMAND && (m.f[0].move->on_hit & NGF_ON_KNOCKDOWN));
    /* B+C without a stock: no MAX. */
    start(def, 160);
    hold(NGF_BTN_B | NGF_BTN_C, 1);
    hold(0, 2);
    assert(!m.f[0].max_timer && !happened(NGF_EV_MAX));
    /* B+C with a stock, B landing a frame early: MAX. */
    start(def, 160);
    m.f[0].power = NGF_POWER_STOCK + 10u;
    hold(NGF_BTN_B, 1);
    hold(NGF_BTN_B | NGF_BTN_C, 1);
    hold(0, 30);
    assert(m.f[0].max_timer && happened(NGF_EV_MAX) && m.f[0].power == 10u);
    /* A+B+C works too. */
    start(def, 160);
    m.f[0].power = NGF_POWER_STOCK;
    hold(NGF_BTN_A | NGF_BTN_B | NGF_BTN_C, 1);
    hold(0, 30);
    assert(m.f[0].max_timer && m.f[0].state != NGF_S_ATTACK);
}

static void max_super(uint8_t def)
{
    const NGFFighterDef *d = slf_fighter_defs[def];
    uint8_t k;
    for (k = 0; k < d->move_count; k++)
        if (d->moves[k].kind == NGF_KIND_SUPER &&
            (d->moves[k].motion == NGF_MOTION_DQCF || d->moves[k].motion == NGF_MOTION_360)) break;
    assert(k < d->move_count);
    start(def, 160);
    m.f[0].power = NGF_POWER_STOCK;
    hold(NGF_BTN_B | NGF_BTN_C, 1);
    hold(0, 30);
    assert(m.f[0].max_timer && m.f[0].power == 0u);
    if (d->moves[k].motion == NGF_MOTION_360) {
        hold(pad(4), 2); hold(pad(1), 2); hold(pad(2), 2); hold(pad(3), 2); hold(pad(6), 2);
        hold((uint8_t)(pad(9) | d->moves[k].buttons), 1);
    } else {
        hold(pad(2), 2); hold(pad(3), 2); hold(pad(6), 2); hold(pad(2), 2); hold(pad(3), 2);
        hold((uint8_t)(pad(6) | d->moves[k].buttons), 1);
    }
    hold(0, 2);
    assert(m.f[0].state == NGF_S_ATTACK && m.f[0].move->kind == NGF_KIND_SUPER);
    assert(m.f[0].max_super && !m.f[0].max_timer);
}

static void throws(uint8_t def)
{
    uint8_t back, button;
    for (back = 0; back < 2u; back++) for (button = 0; button < 2u; button++) {
        start(def, (int16_t)(slf_fighter_defs[def]->push_w * 2 + 4));
        if (!back) hold(pad(6), 3);                   /* holding back first would walk away */
        hold((uint8_t)(pad(back ? 4u : 6u) | (button ? NGF_BTN_D : NGF_BTN_C)), 1);
        if (!(m.f[0].move && m.f[0].move->kind == NGF_KIND_THROW && m.f[1].state == NGF_S_THROWN)) {
            fprintf(stderr, "%s: no throw (back %u, %c; state %u, move kind %d, gap %d)\n", slf_fighter_defs[def]->name,
                    back, button ? 'D' : 'C', m.f[0].state, m.f[0].move ? m.f[0].move->kind : -1, (int)ngf_distance(&m));
            assert(0);
        }
    }
}

static void throw_trajectories(uint8_t def)
{
    int16_t landing[2], height[2];
    uint8_t button, back;
    for (back=0u;back<2u;back++) {
        for (button=0u;button<2u;button++) {
            uint16_t t;
            int16_t x,peak=0;
            start(def,(int16_t)(slf_fighter_defs[def]->push_w*2+4));
            x=ngf_fighter_px(&m.f[0]);
            hold((uint8_t)(pad(back ? 4u:6u)|(button?NGF_BTN_D:NGF_BTN_C)),1);
            assert(m.f[1].state==NGF_S_THROWN);
            for (t=0u;t<160u;t++) {
                hold(0,1);
                if (m.f[1].state==NGF_S_THROWN && ngf_fighter_py(&m.f[1])>peak) peak=ngf_fighter_py(&m.f[1]);
                if (m.f[1].state!=NGF_S_THROWN && m.f[1].grounded) break;
            }
            assert(t<160u);
            landing[button]=(int16_t)(ngf_fighter_px(&m.f[1])-x);
            height[button]=peak;
            assert(back?landing[button]<-30:landing[button]>30);
        }
        assert(back?landing[0]<landing[1]-20:landing[0]>landing[1]+20);
        assert(height[0]>=14 && height[1]>=14);
    }
}

static void learned_combo(uint8_t def)
{
    uint16_t t;
    uint8_t heavy=0u,special=0u,peak=0u;
    start(def,(int16_t)(slf_fighter_defs[def]->push_w*2+4));
    hold(NGF_BTN_A,1);
    for (t=0u;t<45u;t++) {
        hold(0,1);
        if (m.f[0].move_hit && !m.hitstop) break;
    }
    assert(t<45u);
    hold(NGF_BTN_C,1);
    for (t=0u;t<45u;t++) {
        hold(0,1);
        if (m.f[1].combo>peak) peak=m.f[1].combo;
        if (m.f[0].move && m.f[0].move->kind==NGF_KIND_NORMAL && m.f[0].move->buttons==NGF_BTN_C) heavy=1u;
        if (heavy && m.f[0].move_hit && !m.hitstop) break;
    }
    assert(heavy && peak>=2u && t<45u);
    hold(pad(2),2);hold(pad(3),2);hold((uint8_t)(pad(6)|NGF_BTN_C),1);
    for (t=0u;t<60u;t++) {
        hold(0,1);
        if (m.f[0].move && m.f[0].move->kind==NGF_KIND_SPECIAL) special=1u;
        if (m.f[1].combo>peak) peak=m.f[1].combo;
    }
    if (!(special && peak>=3u)) fprintf(stderr,"%s learned combo: heavy %u special %u hits %u\n",slf_fighter_defs[def]->name,heavy,special,peak);
    assert(special && peak>=3u);
}

static void jumps(uint8_t def)
{
    int16_t full = 0, hop = 0, run_x;
    uint8_t i;
    start(def, 200);
    for (i = 0; i < 60u; i++) { hold(i < 8u ? pad(8) : 0u, 1); if (ngf_fighter_py(&m.f[0]) > full) full = ngf_fighter_py(&m.f[0]); }
    start(def, 200);
    for (i = 0; i < 60u; i++) { hold(i < 2u ? pad(8) : 0u, 1); if (ngf_fighter_py(&m.f[0]) > hop) hop = ngf_fighter_py(&m.f[0]); }
    assert(hop > 10 && hop * 10 < full * 7);   /* a tap is a short hop */
    /* Up-forward out of a run carries further than from standing. */
    start(def, 240);
    run_x = ngf_fighter_px(&m.f[0]);
    hold(pad(9), 8);
    while (!m.f[0].grounded) hold(0, 1);
    run_x = (int16_t)(ngf_fighter_px(&m.f[0]) - run_x);
    start(def, 240);
    hold(pad(6), 1); hold(0, 1); hold(pad(6), 8);
    assert(m.f[0].state == NGF_S_DASH);
    {
        int16_t x = ngf_fighter_px(&m.f[0]);
        hold(pad(9), 8);
        assert(m.f[0].state == NGF_S_JUMP || m.f[0].state == NGF_S_ATTACK);
        while (!m.f[0].grounded) hold(0, 1);
        assert(ngf_fighter_px(&m.f[0]) - x > run_x + run_x / 4);
    }
}

static void guard_cancel(uint8_t def)
{
    uint8_t i, rolled = 0;
    start(def, (int16_t)(slf_fighter_defs[def]->push_w * 2 + 6));
    m.f[0].power = NGF_POWER_STOCK;
    for (i = 0; i < 40u && m.f[0].state != NGF_S_BLOCK; i++) step(pad(4), i == 0u ? NGF_BTN_C : 0u);
    if (m.f[0].state != NGF_S_BLOCK) {
        fprintf(stderr, "%s: no block (side 1 state %u, move %d, gap %d)\n", slf_fighter_defs[def]->name, m.f[1].state,
               m.f[1].move ? m.f[1].move->kind : -1, (int)ngf_distance(&m));
        assert(0);
    }
    step(NGF_BTN_A | NGF_BTN_B, 0);              /* during the block's hitstop: it waits */
    for (i = 0; i < 12u; i++) {
        step(0, 0);
        if (m.f[0].move && m.f[0].move->kind == NGF_KIND_EVADE) rolled = 1;
    }
    assert(rolled && m.f[0].power == 0u);
}

int main(void)
{
    unsigned moves = 0, fighters = 0;
    uint8_t def;
    for (def = 0; def < SLF_FIGHTER_COUNT; def++) {
        if (!slf_fighter_ready[def]) continue;
        moves += specials(def);
        two_buttons(def);
        max_super(def);
        throws(def);
        throw_trajectories(def);
        if (def==0u || def==3u || def==4u || def==12u) learned_combo(def);
        jumps(def);
        guard_cancel(def);
        fighters++;
    }
    assert(fighters > 0u);
    printf("shadowleague_controls_test: ok (%u fighters, %u specials and supers)\n", fighters, moves);
    return 0;
}
