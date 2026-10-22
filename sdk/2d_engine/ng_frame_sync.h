#ifndef NG_FRAME_SYNC_H
#define NG_FRAME_SYNC_H

#include <stdint.h>

/* Header-only code must stay in the caller's cartridge ROM section at -O0. */
#if defined(__GNUC__)
#define NG_FRAME_SYNC_INLINE static inline __attribute__((__always_inline__))
#else
#define NG_FRAME_SYNC_INLINE static inline
#endif

/* One completed batch, one main-loop producer, one VBlank IRQ consumer.
 * Publish AFTER all video mutations and wait until pending clears before
 * editing again. The IRQ must check write-window/reentrancy guards BEFORE
 * take(). Taking acknowledges ownership; the main loop resumes only after
 * that IRQ finishes the bounded upload. This is not a job queue or mutex. */
typedef struct {
    volatile uint8_t pending;
} NGFrameSync;

NG_FRAME_SYNC_INLINE uint8_t ng_frame_sync_publish(NGFrameSync *sync)
{
    if (sync->pending) return 0u;
    __asm__ volatile ("" ::: "memory");
    sync->pending = 1u;
    return 1u;
}

NG_FRAME_SYNC_INLINE uint8_t ng_frame_sync_take(NGFrameSync *sync)
{
    if (!sync->pending) return 0u;
    sync->pending = 0u;
    __asm__ volatile ("" ::: "memory");
    return 1u;
}

#undef NG_FRAME_SYNC_INLINE
#endif
