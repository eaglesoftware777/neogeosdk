#include <assert.h>
#include <stdint.h>
#include "sdk/2d_engine/ng_frame_sync.h"

static NGFrameSync sync;
static uint16_t payload[8], expected, uploads;
static uint8_t busy, window = 1;

static void interrupt(void)
{
    unsigned i;
    if (busy || !window || !ng_frame_sync_take(&sync)) return;
    for (i = 0; i < 8; i++) assert(payload[i] == expected);
    uploads++;
}

int main(void)
{
    uint16_t frame;
    unsigned i;
    interrupt();
    assert(uploads == 0);
    for (frame = 1; frame <= 200; frame++) {
        expected = frame;
        /* Interrupts at every point in an incomplete producer batch must
         * neither upload partial data nor acknowledge the next frame. */
        for (i = 0; i < 8; i++) {
            interrupt();
            payload[i] = frame;
            interrupt();
        }
        assert(uploads == frame - 1);
        assert(ng_frame_sync_publish(&sync));
        assert(!ng_frame_sync_publish(&sync));
        /* Old VBlank flags are only wakeups. Busy/deadline guards leave
         * the published batch pending for a later valid interrupt. */
        busy = 1; interrupt(); assert(sync.pending);
        busy = 0; window = 0; interrupt(); assert(sync.pending);
        window = 1; interrupt();
        assert(!sync.pending && uploads == frame);
        interrupt(); assert(uploads == frame);
    }
    return 0;
}
