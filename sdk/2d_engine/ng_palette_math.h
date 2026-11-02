#ifndef NG_PALETTE_MATH_H
#define NG_PALETTE_MATH_H

#include <stdint.h>

/* Neo Geo words: D R0 G0 B0 R4..R1 G4..G1 B4..B1, not RGB555. */
static inline uint8_t ng_color_r(uint16_t c) { return (uint8_t)(((c >> 7) & 30u) | ((c >> 14) & 1u)); }
static inline uint8_t ng_color_g(uint16_t c) { return (uint8_t)(((c >> 3) & 30u) | ((c >> 13) & 1u)); }
static inline uint8_t ng_color_b(uint16_t c) { return (uint8_t)(((c << 1) & 30u) | ((c >> 12) & 1u)); }

static inline uint16_t ng_color_pack(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((r & 30u) << 7) | ((g & 30u) << 3) | ((b & 30u) >> 1)
                    | ((r & 1u) << 14) | ((g & 1u) << 13) | ((b & 1u) << 12));
}

/* (a * (255 - amount) + b * amount + 127) / 255, rounded down. The 68000
 * has no 32-bit divide, and C's "/ 255u" on a promoted int calls the
 * library's, about 450 clocks: 45 of them a 16-colour bank. For x up to
 * 65534, x / 255 is (x + 1 + (x >> 8)) >> 8 exactly, and x is at most
 * 255 * 255 + 127 here, so the result is the same, in 16-bit sums. */
static inline uint8_t ng_color_div255(uint16_t x)
{
    return (uint8_t)((uint16_t)(x + 1u + (x >> 8)) >> 8);
}

static inline uint8_t ng_color_mix(uint8_t a, uint8_t b, uint8_t amount)
{
    return ng_color_div255((uint16_t)((uint16_t)a * (uint16_t)(255u - amount)
                                      + (uint16_t)b * (uint16_t)amount + 127u));
}

static inline void ng_palette_scale_colors(uint16_t *out, const uint16_t *base, uint8_t amount)
{
    uint8_t i;
    out[0] = base[0];
    for (i = 1u; i < 16u; i++) {
        uint16_t c = base[i];
        out[i] = (uint16_t)(ng_color_pack(ng_color_mix(0u, ng_color_r(c), amount),
                                          ng_color_mix(0u, ng_color_g(c), amount),
                                          ng_color_mix(0u, ng_color_b(c), amount)) | (c & 0x8000u));
    }
}

static inline void ng_palette_tint_colors(uint16_t *out, const uint16_t *base,
                                          uint8_t r, uint8_t g, uint8_t b, uint8_t amount)
{
    uint8_t i;
    /* ng_color_mix(channel, target >> 3, amount), the target's share
     * worked out once a bank instead of once a colour */
    uint16_t keep = (uint16_t)(255u - amount);
    uint16_t add_r = (uint16_t)((uint16_t)(r >> 3) * (uint16_t)amount + 127u);
    uint16_t add_g = (uint16_t)((uint16_t)(g >> 3) * (uint16_t)amount + 127u);
    uint16_t add_b = (uint16_t)((uint16_t)(b >> 3) * (uint16_t)amount + 127u);
    uint16_t dark = amount == 255u ? 0u : 0x8000u;
    out[0] = base[0];
    for (i = 1u; i < 16u; i++) {
        uint16_t c = base[i];
        /* Index zero alone is transparent. Opaque black also accepts a tint. */
        out[i] = (uint16_t)(ng_color_pack(ng_color_div255((uint16_t)((uint16_t)ng_color_r(c) * keep + add_r)),
                                          ng_color_div255((uint16_t)((uint16_t)ng_color_g(c) * keep + add_g)),
                                          ng_color_div255((uint16_t)((uint16_t)ng_color_b(c) * keep + add_b)))
                            | (c & dark));
    }
}

#endif
