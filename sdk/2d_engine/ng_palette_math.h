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

static inline uint8_t ng_color_mix(uint8_t a, uint8_t b, uint8_t amount)
{
    return (uint8_t)(((uint16_t)a * (255u - amount) + (uint16_t)b * amount + 127u) / 255u);
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
    out[0] = base[0];
    for (i = 1u; i < 16u; i++) {
        uint16_t c = base[i];
        /* Index zero alone is transparent. Opaque black also accepts a tint. */
        out[i] = (uint16_t)(ng_color_pack(ng_color_mix(ng_color_r(c), r >> 3, amount),
                                          ng_color_mix(ng_color_g(c), g >> 3, amount),
                                          ng_color_mix(ng_color_b(c), b >> 3, amount))
                            | (amount == 255u ? 0u : (c & 0x8000u)));
    }
}

#endif
