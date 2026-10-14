#include <assert.h>
#include "games/skylance/scenes/sky_video_sync.h"

int main(void)
{
    SkyVideoSync sync = {0};
    unsigned waits = 0, uploads = 0;
    assert(!sky_video_take(&sync));
    sky_video_publish(&sync);
    assert(sync.pending);
    /* Two stale wait flags do not acknowledge a newly published batch. */
    do {
        waits++;
        if (waits == 3 && sky_video_take(&sync)) uploads++;
    } while (sync.pending);
    assert(waits == 3 && uploads == 1);
    assert(!sky_video_take(&sync));
    /* An idle IRQ cannot upload a buffer while the game is editing it. */
    assert(!sky_video_take(&sync));
    sky_video_publish(&sync);
    assert(sky_video_take(&sync));
    assert(!sky_video_take(&sync));
    return 0;
}
