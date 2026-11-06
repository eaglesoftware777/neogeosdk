/* Deterministic direction repeat; call once per ordered input sample.
 * Keep one zero-initialized state per axis so changing one axis never
 * inherits or resets the other's delay. No hardware access or edge queue. */
#ifndef NG_INPUT_REPEAT_H
#define NG_INPUT_REPEAT_H
#include <stdint.h>

/* The cartridge linker discards ordinary .text. Keep header-only helpers
 * inside their caller's ROM section even in unoptimized game builds. */
#if defined(__GNUC__)
#define NG_INPUT_REPEAT_INLINE static inline __attribute__((__always_inline__))
#else
#define NG_INPUT_REPEAT_INLINE static inline
#endif

typedef struct {
    uint16_t held;
    uint8_t wait;
} NGInputRepeat;

/* A changed nonzero direction emits immediately. First repeat follows
 * delay samples later (delay 0 uses interval), then every interval samples.
 * interval 0 disables repeats. Return 0 on release. The caller normalizes
 * opposing directions and supplies direction bits only; buttons use edges.
 * Changing delay/interval while held takes effect after the current wait. */
NG_INPUT_REPEAT_INLINE uint16_t ng_input_repeat_step(NGInputRepeat *r, uint16_t held,
                                          uint8_t delay, uint8_t interval)
{
    if (held != r->held) {
        r->held = held;
        r->wait = delay ? delay : interval;
        return held;
    }
    if (!held || !interval) return 0u;
    if (r->wait > 1u) { r->wait--; return 0u; }
    r->wait = interval;
    return held;
}
#undef NG_INPUT_REPEAT_INLINE
#endif
