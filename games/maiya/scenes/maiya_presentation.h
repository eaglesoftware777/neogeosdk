#ifndef MAIYA_PRESENTATION_H
#define MAIYA_PRESENTATION_H

#include <stdint.h>

enum { MG_GAIT_FRAMES = 8, MG_GAIT_STEP = 10 * 256 };

static inline uint16_t mg_gait_advance(uint16_t phase, uint16_t distance)
{
    uint32_t next = (uint32_t)phase + distance;
    while (next >= MG_GAIT_FRAMES * MG_GAIT_STEP)
        next -= MG_GAIT_FRAMES * MG_GAIT_STEP;
    return (uint16_t)next;
}

static inline uint8_t mg_gait_frame(uint16_t phase)
{
    return (uint8_t)(phase / MG_GAIT_STEP);
}

/* Start with action buttons, or Select, belongs to the system firmware. */
static inline uint8_t mg_pause_start_allowed(uint8_t changed, uint8_t status,
                                             uint8_t joy)
{
    return (uint8_t)((changed & 1u) && !(status & 2u) && !(joy & 0xF0u));
}

static inline uint16_t mg_palette_distance(uint16_t a, uint16_t b)
{
    int16_t r = (int16_t)(((a >> 7) & 30u) | ((a >> 14) & 1u)) -
                (int16_t)(((b >> 7) & 30u) | ((b >> 14) & 1u));
    int16_t g = (int16_t)(((a >> 3) & 30u) | ((a >> 13) & 1u)) -
                (int16_t)(((b >> 3) & 30u) | ((b >> 13) & 1u));
    int16_t blue = (int16_t)(((a << 1) & 30u) | ((a >> 12) & 1u)) -
                   (int16_t)(((b << 1) & 30u) | ((b >> 12) & 1u));
    return (uint16_t)(r * r + g * g + blue * blue);
}

static inline uint8_t mg_palette_nearest(const uint16_t *palette,
                                         uint16_t color, uint8_t exclude)
{
    uint8_t i, best = 0u;
    uint16_t distance = 0xFFFFu;
    for (i = 1u; i < 16u; i++) {
        uint16_t candidate;
        if (i == exclude) continue;
        candidate = mg_palette_distance(palette[i], color);
        if (candidate < distance) { distance = candidate; best = i; }
    }
    return best;
}

#endif
