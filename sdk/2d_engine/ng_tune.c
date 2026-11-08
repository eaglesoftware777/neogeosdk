/*
 * ng_tune.c — Ratings into game numbers (see ng_tune.h)
 *
 * Each shape is a 32-entry table of how far along the span a rating sits, in
 * 256ths, so a value is at0 + span * share / 256: one MULS and a shift.
 */

#include "ng_tune.h"

static const uint16_t k_share[4][32] = {
    /* straight */
    {0, 8, 17, 25, 33, 41, 50, 58, 66, 74, 83, 91, 99, 107, 116, 124,
     132, 140, 149, 157, 165, 173, 182, 190, 198, 206, 215, 223, 231, 239, 248, 256},
    /* ease in: the square */
    {0, 0, 1, 2, 4, 7, 10, 13, 17, 22, 27, 32, 38, 45, 52, 60,
     68, 77, 86, 96, 107, 117, 129, 141, 153, 166, 180, 194, 209, 224, 240, 256},
    /* ease out: one minus the square of what is left */
    {0, 16, 32, 47, 62, 76, 90, 103, 115, 127, 139, 149, 160, 170, 179, 188,
     196, 204, 211, 218, 224, 229, 234, 239, 243, 246, 249, 252, 254, 255, 256, 256},
    /* step */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256},
};

uint8_t NG_TUNE_CODE ng_tune_clamp(int16_t rating)
{
    if (rating < 0) return 0;
    if (rating > NG_TUNE_RATING_MAX) return NG_TUNE_RATING_MAX;
    return (uint8_t)rating;
}

uint8_t NG_TUNE_CODE ng_tune_from16(uint8_t attribute)
{
    if (attribute == 0u) return 0;
    if (attribute > 16u) attribute = 16u;
    return (uint8_t)(attribute * 2u - 1u);
}

uint8_t NG_TUNE_CODE ng_tune_value(const NGTuneCurve *c, uint8_t level, int16_t rating)
{
    const NGTuneSpan *s;
    int16_t span, share;
    int32_t part;
    if (!c->levels) return 0;
    if (level >= c->levels) level = (uint8_t)(c->levels - 1u);
    s = &c->span[level];
    share = (int16_t)k_share[c->shape & 3u][ng_tune_clamp(rating)];
    span = (int16_t)((int16_t)s->at31 - (int16_t)s->at0);
    /* 16 x 16 into 32 bits: one MULS, where an int multiply is a library call */
    part = (int32_t)span * (int32_t)share + 128;
    return (uint8_t)((int16_t)s->at0 + (int16_t)(part >> 8));
}

int8_t NG_TUNE_CODE ng_tune_nudge(const NGTuneNudge *n, int16_t gap, uint8_t late, uint8_t round)
{
    int16_t r;
    if (gap < -3) gap = -3;
    if (gap > 3) gap = 3;
    r = late ? n->late[gap + 3] : n->early[gap + 3];
    if (n->per_round) {
        int16_t climb = (int16_t)(n->per_round * (int16_t)round);
        if (n->per_round > 0 && climb > n->round_cap) climb = n->round_cap;
        if (n->per_round < 0 && climb < n->round_cap) climb = n->round_cap;
        r = (int16_t)(r + climb);
    }
    if (r < -31) r = -31;
    if (r > 31) r = 31;
    return (int8_t)r;
}

uint8_t NG_TUNE_CODE ng_tune_level(uint8_t base, uint8_t round, uint8_t every, uint8_t levels)
{
    uint8_t up = 0;
    if (every) {
        /* rounds are few: counting beats a division */
        while (round >= every) { round = (uint8_t)(round - every); up++; }
    }
    base = (uint8_t)(base + up);
    if (levels && base >= levels) base = (uint8_t)(levels - 1u);
    return base;
}

void NG_TUNE_CODE ng_tune_fill(uint8_t *out, const NGTuneCurve *const *curves, const uint8_t *ratings,
                               uint8_t n, uint8_t level, int8_t nudge)
{
    uint8_t i;
    for (i = 0; i < n; i++) out[i] = ng_tune_value(curves[i], level, (int16_t)((int16_t)ratings[i] + nudge));
}
