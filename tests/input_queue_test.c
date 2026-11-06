#include <assert.h>
#include <stdio.h>
#include "ng_input_queue.h"

int main(void)
{
    static NGInputQueue q;
    NGInputSample s;
    unsigned tick, received = 0u;
    /* Ten thousand ticks, including brief stalls and byte-index wrap. */
    for (tick = 1u; tick <= 10000u; tick++) {
        assert(ng_input_queue_push(&q, (uint8_t)tick, (uint8_t)~tick, (uint8_t)(tick & 5u)));
        if (tick % 5u == 0u) while (ng_input_queue_pop(&q, &s)) {
            assert(s.tick == ++received);
            assert(s.joy[0] == (uint8_t)s.tick && s.joy[1] == (uint8_t)~s.tick);
            assert(s.start == (s.tick & 5u));
        }
    }
    assert(received == 10000u && q.ticks == 10000u && !q.dropped && q.peak == 5u);
    /* A press and release remain separate samples while the main loop stalls. */
    assert(ng_input_queue_push(&q, 16u, 0u, 1u));
    assert(ng_input_queue_push(&q, 0u, 0u, 0u));
    assert(ng_input_queue_pop(&q, &s) && s.joy[0] == 16u && s.start == 1u);
    assert(ng_input_queue_pop(&q, &s) && !s.joy[0] && !s.start);
    for (tick = 0u; tick < NG_INPUT_QUEUE_CAPACITY; tick++) assert(ng_input_queue_push(&q, (uint8_t)tick, 0u, 0u));
    assert(!ng_input_queue_push(&q, 255u, 0u, 0u) && q.dropped == 1u);
    for (tick = 0u; tick < NG_INPUT_QUEUE_CAPACITY; tick++) assert(ng_input_queue_pop(&q, &s) && s.joy[0] == tick);
    assert(!ng_input_queue_pop(&q, &s));
    assert(ng_input_queue_push(&q, 0u, 0u, 0u));
    assert(ng_input_queue_discard(&q) == 1u && !ng_input_queue_pop(&q, &s));
    assert(q.dropped == 1u); /* Scene discard does not hide overflow history. */
    puts("input queue: 10000 ordered ticks, transitions, overflow and wrap: PASS");
    return 0;
}
