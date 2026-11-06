#ifndef SKY_VIDEO_SYNC_H
#define SKY_VIDEO_SYNC_H

#include <stdint.h>

/* The main loop publishes completed video data immediately before waiting.
 * Only the game-owned VBlank interrupt may consume it, before BIOS I/O and
 * the sound pump spend the short atomic-upload window. */
typedef struct {
    volatile uint8_t pending;
} SkyVideoSync;

static inline void sky_video_publish(SkyVideoSync *sync)
{
    __asm__ volatile ("" ::: "memory");
    sync->pending = 1u;
}

static inline uint8_t sky_video_take(SkyVideoSync *sync)
{
    if (!sync->pending) return 0u;
    sync->pending = 0u;
    __asm__ volatile ("" ::: "memory");
    return 1u;
}

#endif
