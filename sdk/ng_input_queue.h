/* Single-producer interrupt / single-consumer main-loop input snapshots.
 * No hardware access: sample BIOS controls after SYS_IO, once per VBlank.
 * Thirty-two samples bound latency and RAM; overflow drops the newest sample
 * and records it rather than overwriting data the consumer may be reading.
 * Indices are byte-sized atomic stores on the 68000. */
#ifndef NG_INPUT_QUEUE_H
#define NG_INPUT_QUEUE_H
#include <stdint.h>

#define NG_INPUT_QUEUE_CAPACITY 32u
typedef struct {
    uint32_t tick;
    uint8_t joy[2], start;
} NGInputSample;
typedef struct {
    volatile NGInputSample sample[NG_INPUT_QUEUE_CAPACITY];
    volatile uint8_t written, read, peak;
    volatile uint32_t ticks, dropped;
} NGInputQueue;

#define NG_INPUT_BARRIER() __asm__ volatile ("" ::: "memory")

/* Producer only. Initialise the queue to zero before enabling its IRQ. */
static inline uint8_t ng_input_queue_push(NGInputQueue *q, uint8_t p1, uint8_t p2, uint8_t start)
{
    uint8_t w = q->written, used = (uint8_t)(w - q->read);
    volatile NGInputSample *s;
    uint32_t tick = q->ticks + 1u;
    q->ticks = tick;
    if (used >= NG_INPUT_QUEUE_CAPACITY) { q->dropped++; return 0u; }
    s = &q->sample[w & (NG_INPUT_QUEUE_CAPACITY - 1u)];
    s->tick = tick; s->joy[0] = p1; s->joy[1] = p2; s->start = start;
    NG_INPUT_BARRIER();
    q->written = (uint8_t)(w + 1u);
    if (++used > q->peak) q->peak = used;
    return 1u;
}

/* Consumer only. Process every returned sample, in order. Rendering may
 * present the newest completed state after several catch-up simulation ticks. */
static inline uint8_t ng_input_queue_pop(NGInputQueue *q, NGInputSample *out)
{
    uint8_t r = q->read;
    const volatile NGInputSample *s;
    if (r == q->written) return 0u;
    NG_INPUT_BARRIER();
    s = &q->sample[r & (NG_INPUT_QUEUE_CAPACITY - 1u)];
    out->tick = s->tick; out->joy[0] = s->joy[0]; out->joy[1] = s->joy[1]; out->start = s->start;
    NG_INPUT_BARRIER();
    q->read = (uint8_t)(r + 1u);
    return 1u;
}

/* Deliberate scene boundary: discard old scene input without resetting the
 * producer's sequence or overflow counters. Returns the number discarded. */
static inline uint8_t ng_input_queue_discard(NGInputQueue *q)
{
    uint8_t w = q->written, n = (uint8_t)(w - q->read);
    NG_INPUT_BARRIER();
    q->read = w;
    return n;
}
#undef NG_INPUT_BARRIER
#endif
