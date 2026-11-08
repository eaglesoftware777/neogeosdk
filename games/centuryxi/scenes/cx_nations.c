/*
 * cx_nations.c -- Century XI: the teams, their shapes and their men.
 *
 * The tables come from tools/nations.py (artbox/generated/cx_data.h). The men
 * are original characters; a squad is sixteen: the keeper, ten outfielders
 * and five on the bench.
 */
#include "cx_internal.h"
#include "../artbox/generated/cx_data.h"

/* Attacking right, in units: the keeper, then the back line, the middle, the
 * front. A shape's `line` says which is which, for the team's thinking. */
static const int16_t k_form_x[4][11] = {
    {36, 250, 230, 230, 250, 520, 480, 480, 520, 740, 740},       /* 4-4-2 */
    {36, 250, 230, 230, 250, 470, 450, 470, 760, 790, 760},       /* 4-3-3 */
    {36, 230, 220, 230, 500, 450, 440, 450, 500, 740, 740},       /* 3-5-2 */
    {36, 260, 230, 220, 230, 260, 470, 450, 470, 740, 740},       /* 5-3-2 */
};
static const int16_t k_form_y[4][11] = {
    {352, 110, 260, 444, 594, 96, 270, 434, 608, 270, 434},
    {352, 110, 260, 444, 594, 190, 352, 514, 120, 352, 584},
    {352, 170, 352, 534, 80, 230, 352, 474, 624, 270, 434},
    {352, 80, 210, 352, 494, 624, 200, 352, 504, 270, 434},
};
static const uint8_t k_form_line[4][11] = {
    {0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3},
    {0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3},
    {0, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3},
    {0, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3},
};

/* Puts one roster man on a slot of the side. */
static void CX_CODE dress(CxMan *m, uint8_t nation, uint8_t squad)
{
    const uint8_t *r = cx_roster_stat[nation][squad];
    m->squad = squad;
    m->pace = r[0];
    m->shot = r[1];
    m->ctrl = r[2];
    m->tack = r[3];
    m->fig = r[4];
    m->skin = r[5];
    m->hair = r[6];
    m->type = r[7];
    m->build = r[8];
}

void CX_CODE cx_set_shape(CxWorld *w, uint8_t team, uint8_t form)
{
    uint8_t k;
    form = (uint8_t)(form & 3u);
    w->form[team] = form;
    for (k = 0; k < 11u; k++) {
        CxMan *m = &w->m[team * 11u + k];
        m->hx = k_form_x[form][k];
        m->hy = k_form_y[form][k];
        m->line = k_form_line[form][k];
    }
}

uint16_t cx_absent[2];        /* men a side is without (suspended), a bit by squad number: set by the championship */

/* The eleven a side puts out: its first eleven, an absent man's place taken from the bench
 * (the keeper's by the reserve keeper). out[position] = squad number. */
void CX_CODE cx_lineup(uint16_t absent, uint8_t out[11])
{
    uint8_t k, next = 12;
    for (k = 0; k < 11u; k++) {
        out[k] = k;
        if (!(absent & (1u << k))) continue;
        if (k == 0u) { out[k] = 11; continue; }
        while (next < CX_SQUAD_MEN && (absent & (1u << next))) next++;
        if (next < CX_SQUAD_MEN) out[k] = next++;
    }
}

void CX_CODE cx_field_side(CxWorld *w, uint8_t t)
{
    uint8_t nation = w->team[t], k, eleven[11];
    cx_lineup(cx_absent[t], eleven);
    for (k = 0; k < 11u; k++) {
        CxMan *m = &w->m[t * 11u + k];
        m->team = t;
        m->role = k;
        m->sta = 255;
        m->mark = 0xFFu;
        dress(m, nation, eleven[k]);
    }
    cx_set_shape(w, t, cx_nation_form[nation]);
}

uint8_t CX_CODE cx_sub_request(CxWorld *w, uint8_t team, uint8_t role, uint8_t bench)
{
    uint8_t cap = w->rules == CX_RULES_MODERN ? 5u : 3u;
    if (team > 1u || role == 0u || role > 10u || bench > 4u || w->subs[team] >= cap) return 0;
    if (w->m[team * 11u + role].state == CX_M_OFF) return 0;
    w->sub_role[team] = role;
    w->sub_in[team] = (uint8_t)(11u + bench);
    return 1;
}

void CX_CODE cx_subs_apply(CxWorld *w)
{
    uint8_t t;
    for (t = 0; t < 2u; t++) {
        if (w->sub_role[t]) {
            CxMan *m = &w->m[t * 11u + w->sub_role[t]];
            dress(m, w->team[t], w->sub_in[t]);
            m->card = 0;
            m->sta = 255;
            w->sub_role[t] = 0;
            w->subs[t]++;
            w->ev |= CX_EV_SUB;
        }
    }
}

typedef char cx_squads_agree[(CX_SQUADS == CX_NATIONS) ? 1 : -1];

uint8_t CX_CODE cx_squad_count(void) { return CX_SQUADS; }

/* One number for a side, for draws and quick results. */
uint8_t CX_CODE cx_squad_power(uint8_t id)
{
    const uint8_t *s;
    if (id >= CX_SQUADS) return 0;
    s = cx_nation_stat[id];
    return (uint8_t)((s[0] + s[1] + s[2] + s[3] + s[4] + s[5]) / 6u);
}

uint8_t CX_CODE cx_squad_pace(uint8_t id)
{
    if (id >= CX_SQUADS) return 1;
    return (uint8_t)(cx_nation_stat[id][0] >> 2);
}

uint8_t CX_CODE cx_squad_style(uint8_t id) { return cx_nation_style[id < CX_NATIONS ? id : 0]; }

uint8_t CX_CODE cx_kit_ink(uint8_t nation, uint8_t kit) { return cx_nation_ink[nation < CX_NATIONS ? nation : 0][kit > 2u ? 0u : kit]; }

uint8_t CX_CODE cx_nation_celebration(uint8_t team) { return cx_nation_celeb[team < CX_NATIONS ? team : 0]; }

uint8_t CX_CODE cx_man_type(uint8_t team, uint8_t squad) { return cx_roster_stat[team < CX_NATIONS ? team : 0][squad < CX_SQUAD_MEN ? squad : 0][7]; }

uint8_t CX_CODE cx_squad_stat(uint8_t id, uint8_t which)
{
    if (id >= CX_SQUADS || which > 5u) return 0;
    return cx_nation_stat[id][which];
}

void CX_CODE cx_squad_name(uint8_t id, char *dst)
{
    const char *s;
    uint8_t i;
    if (id >= CX_SQUADS) id = 0;
    s = cx_nation_name[id];
    for (i = 0; i < 13u && s[i]; i++) dst[i] = s[i];
    dst[i] = 0;
}

void CX_CODE cx_squad_code(uint8_t id, char *dst)
{
    const char *s = cx_nation_code[id < CX_SQUADS ? id : 0];
    dst[0] = s[0]; dst[1] = s[1]; dst[2] = s[2]; dst[3] = 0;
}

void CX_CODE cx_man_name(uint8_t team, uint8_t squad, char *dst)
{
    const char *s = cx_roster_name[team < CX_SQUADS ? team : 0][squad < CX_SQUAD_MEN ? squad : 0];
    uint8_t i;
    for (i = 0; i < 10u && s[i]; i++) dst[i] = s[i];
    dst[i] = 0;
}

/* The commentator's recording of the man's name: the n-th of tools/voices.py's names. */
uint16_t CX_CODE cx_man_voice(uint8_t team, uint8_t squad)
{
    uint16_t v = cx_roster_voice[team < CX_SQUADS ? team : 0][squad < CX_SQUAD_MEN ? squad : 0];
    /* the first recordings keep their numbers; the 48 sides' chants come after them, then the
     * names added with those sides (tools/voices.py --new) */
    return (uint16_t)(v < CX_VOICE_NAMES_V1 ? v : v + (CX_NATIONS - CX_CHANTS_V1));
}

/* A nation's chant after the first 32: its place after the first names' recordings. */
uint16_t CX_CODE cx_chant_voice(uint8_t nation)
{
    return (uint16_t)(CX_VOICE_NAMES_V1 + (nation - CX_CHANTS_V1));
}

uint8_t CX_CODE cx_man_look(uint8_t team, uint8_t squad, uint8_t *skin, uint8_t *hair)
{
    const uint8_t *r = cx_roster_stat[team < CX_SQUADS ? team : 0][squad < CX_SQUAD_MEN ? squad : 0];
    *skin = r[5];
    *hair = r[6];
    return r[4];
}

const uint16_t *CX_CODE cx_kit_words(uint8_t nation, uint8_t kit) { return cx_nation_kit[nation < CX_NATIONS ? nation : 0][kit > 2u ? 0u : kit]; }
uint16_t CX_CODE cx_nation_flag(uint8_t nation, uint8_t which) { return cx_nation_flagcol[nation < CX_NATIONS ? nation : 0][which & 1u]; }
const uint16_t *CX_CODE cx_skin_words(uint8_t i) { return cx_skin_pal[i < sizeof cx_skin_pal / sizeof cx_skin_pal[0] ? i : 0u]; }
const uint16_t *CX_CODE cx_hair_words(uint8_t i) { return cx_hair_pal[i < sizeof cx_hair_pal / sizeof cx_hair_pal[0] ? i : 0u]; }

static uint16_t CX_CODE colour_gap(const uint8_t *a, const uint8_t *b)
{
    uint16_t d = 0;
    uint8_t i;
    for (i = 0; i < 3u; i++) d = (uint16_t)(d + (a[i] > b[i] ? a[i] - b[i] : b[i] - a[i]));
    return d;
}

uint8_t CX_CODE cx_kit_for(uint8_t team, uint8_t foe)
{
    /* The first-named side keeps its home kit; a side too close to it changes. */
    if (team >= CX_SQUADS || foe >= CX_SQUADS) return 0;
    if (team < foe) return 0;
    return (uint8_t)(colour_gap(cx_nation_rgb[team][0], cx_nation_rgb[foe][0]) < 200u);
}

void CX_CODE cx_quick_result(uint8_t a, uint8_t b, uint32_t *seed, uint8_t *ga, uint8_t *gb)
{
    uint32_t x = *seed ? *seed : 787u;
    int16_t edge = (int16_t)((int16_t)cx_squad_power(a) - (int16_t)cx_squad_power(b));
    uint8_t chance, side;
    uint8_t g[2] = {0, 0};
    /* eight chances a game: each one falls to a side by strength, and a third go in */
    for (chance = 0; chance < 8u; chance++) {
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        side = (uint8_t)(((int16_t)(x & 31u) - 16) >= edge * 3 ? 1u : 0u);
        if (((x >> 8) & 255u) < 80u && g[side] < 9u) g[side]++;
    }
    *seed = x;
    *ga = g[0];
    *gb = g[1];
}

void CX_CODE cx_cup_init(CxCup *c, uint8_t human_team)
{
    uint8_t i;
    if (human_team >= CX_SQUADS) human_team = 0;
    for (i = 0; i < 8u; i++) c->team[i] = i;
    if (human_team != 0) {
        if (human_team < 8u) c->team[human_team] = 0;
        c->team[0] = human_team;
    }
    c->round = 0;
    c->won = 0;
    c->foe = c->team[1];
    c->pad = 0;
}

void CX_CODE cx_cup_apply(CxCup *c, uint8_t human_won)
{
    uint8_t slots, i, n, out[8];
    if (c->round >= 3u) return;
    if (!human_won) {
        c->round = 3;
        c->won = 0;
        return;
    }
    slots = (uint8_t)(8u >> c->round);
    n = 0;
    out[n++] = c->team[0];
    for (i = 2; (uint8_t)(i + 1u) < slots; i = (uint8_t)(i + 2u)) {
        uint8_t a = c->team[i], b = c->team[(uint8_t)(i + 1u)];
        uint8_t pa = cx_squad_power(a), pb = cx_squad_power(b);
        out[n++] = (pa > pb || (pa == pb && a < b)) ? a : b;
    }
    for (i = 0; i < n; i++) c->team[i] = out[i];
    c->round = (uint8_t)(c->round + 1u);
    if (c->round >= 3u || n < 2u) {
        c->won = 1;
        c->round = 3;
        c->foe = 0;
    } else {
        c->foe = c->team[1];
    }
}
