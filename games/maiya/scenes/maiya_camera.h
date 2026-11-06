#ifndef MAIYA_CAMERA_H
#define MAIYA_CAMERA_H
#include <stdint.h>

/* feet_screen == 0 preserves the existing tall-level camera. A configured
 * framing point also bounds camera lag: the 64px hero stays below the HUD
 * and above the lower edge while jumping between canopy ledges. */
static inline int16_t mg_camera_vertical(int16_t current, int16_t feet,
                                         uint16_t upper, uint8_t feet_screen)
{
    int16_t want = 0, delta;
    if (upper) {
        want = (int16_t)(feet - (feet_screen ? feet_screen : 64));
        if (want > 0) want = 0;
        if (want < -(int16_t)upper) want = (int16_t)-upper;
    }
    delta = (int16_t)(want - current);
    if (delta > 112 || delta < -112) current = want;
    if (want != current) {
        int16_t step = (int16_t)((want - current) / 6);
        if (!step) step = (int16_t)(want > current ? 1 : -1);
        current = (int16_t)(current + step);
    }
    if (upper && feet_screen) {
        int16_t high = (int16_t)(feet - (feet_screen - 12));
        int16_t low = (int16_t)(feet - 208);
        if (high > 0) high = 0;
        if (high < -(int16_t)upper) high = (int16_t)-upper;
        if (low > 0) low = 0;
        if (low < -(int16_t)upper) low = (int16_t)-upper;
        if (current > high) current = high;
        if (current < low) current = low;
    }
    return current;
}

/* Frame the supporting terrain, not the jump arc. Elevated footing leaves
 * room for a full run jump before the next takeoff. In flight the view
 * holds, except at the viewport's safety edges for a high special or fall.
 * Only landing or climbing establishes another terrain framing point. */
static inline int16_t mg_camera_vertical_supported(int16_t current, int16_t feet,
                                                    int16_t support, int16_t ground,
                                                    uint16_t upper, uint8_t feet_screen,
                                                    uint8_t settled)
{
    int16_t next = current, high, low;
    if (!feet_screen) return mg_camera_vertical(current, feet, upper, 0);
    if (!upper) return 0;
    /* Road-side shelves belong to the same fixed screen as the road.
     * Landing on their bricks must not select a raised camera tier, or
     * walking off them makes the platforms sink against the painted floor.
     * A higher special can still leave this screen through the top edge. */
    if (support >= ground - 48 && feet >= 64) return 0;
    if (settled) {
        int16_t want = support >= ground - 4 ? 0 :
                       (int16_t)(support - feet_screen - 32);
        int16_t step;
        if (want > 0) want = 0;
        if (want < -(int16_t)upper) want = (int16_t)-upper;
        step = (int16_t)((want - current) / 6);
        if (!step && want != current) step = want > current ? 1 : -1;
        next = (int16_t)(current + step);
    }
    /* A 64px canvas stays below row32 and above the bottom of the screen.
     * Ordinary jumps from settled footing never reach either edge. */
    high = (int16_t)(feet - 96);
    low = (int16_t)(feet - 208);
    if (next > high) next = high;
    if (next < low) next = low;
    if (next > 0) next = 0;
    if (next < -(int16_t)upper) next = (int16_t)-upper;
    return next;
}
#endif
