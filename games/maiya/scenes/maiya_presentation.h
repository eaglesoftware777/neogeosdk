#ifndef MAIYA_PRESENTATION_H
#define MAIYA_PRESENTATION_H

#include <stdint.h>

enum { MG_GAIT_FRAMES = 8, MG_GAIT_CYCLE = 80 * 256, MG_GAIT_STOP = 6 };

/*
 * Her stride, by a gait clock of 80 units per cycle (two steps). Walking:
 * contact, down, passing, up, then the other foot; the clock runs 5/4 of
 * the road at a walk, so a walking cycle covers 64 px -- the length of her
 * drawn step, which keeps the planted foot nearly still on the road.
 * Running: contact, down, push-off with the knee driving through, and a
 * flight; a cycle is 80 px. Each pose is held for the road its planted
 * foot covers in the drawing, so the foot stays put on the ground.
 * The bob is drawn into the frames and every frame keeps the same hip, so
 * the sprite moves with her and never jumps. Her arms swing with her legs.
 * When she stops she settles feet together, arms down, for MG_GAIT_STOP
 * frames and is straight back in her idle pose.
 */
static const uint8_t mg_walk_distance[MG_GAIT_FRAMES] = { 8, 10, 14, 8, 8, 10, 14, 8 };
static const uint16_t mg_walk_threshold[MG_GAIT_FRAMES] = {
    8 * 256, 18 * 256, 32 * 256, 40 * 256,
    48 * 256, 58 * 256, 72 * 256, 80 * 256
};
static const uint8_t mg_run_distance[MG_GAIT_FRAMES] = { 10, 14, 8, 8, 10, 14, 8, 8 };
static const uint16_t mg_run_threshold[MG_GAIT_FRAMES] = {
    10 * 256, 24 * 256, 32 * 256, 40 * 256,
    50 * 256, 64 * 256, 72 * 256, 80 * 256
};
static const uint8_t mg_gait_footfall[MG_GAIT_FRAMES] = { 1, 0, 0, 0, 1, 0, 0, 0 };

/* Road travelled (8.8 px) to gait clock units: 5/4 at a walk, 1 at a run. */
static inline uint16_t mg_gait_units(uint16_t moved, uint8_t run)
{
    return (uint16_t)(run ? moved : moved + (moved >> 2));
}

static inline uint16_t mg_gait_advance(uint16_t phase, uint16_t distance)
{
    uint32_t next = (uint32_t)phase + distance;
    while (next >= MG_GAIT_CYCLE)
        next -= MG_GAIT_CYCLE;
    return (uint16_t)next;
}

/* At most seven comparisons; no division on the 68000. */
static inline uint8_t mg_gait_frame_mode(uint16_t phase, uint8_t run)
{
    const uint16_t *threshold = run ? mg_run_threshold : mg_walk_threshold;
    uint8_t k;
    for (k = 0; k < MG_GAIT_FRAMES - 1; k++)
        if (phase < threshold[k]) break;
    return k;
}

/*
 * B, frame by frame. Held, it throws again every `repeat` frames. On the
 * move, each throw is followed by a wait of a few steps (the caller sets
 * `wait` when she throws while walking); a press during the wait is kept
 * and thrown when it ends, so tapping never freezes her in a throw.
 * Standing still, nothing waits. Returns 1 when she should throw now.
 */
typedef struct { uint8_t held, wait, queued; } MGFire;

static inline uint8_t mg_fire_tick(MGFire *f, uint8_t pressed, uint8_t down,
                                   uint8_t moving, uint8_t repeat)
{
    uint8_t fire;
    if (!moving) f->wait = 0u;
    else if (f->wait) f->wait--;
    if (!down) f->held = 0u;
    else if (!pressed && f->held < 255u) f->held++;
    fire = (uint8_t)(pressed || f->queued || (down && f->held >= repeat));
    if (!fire) return 0u;
    if (f->wait) {
        f->queued = 1u;
        return 0u;
    }
    f->held = 0u;
    f->queued = 0u;
    return 1u;
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
