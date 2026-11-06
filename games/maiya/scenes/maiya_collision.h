/* Maiya's ledge forgiveness and close-range attacks; no shared engine tuning. */
#ifndef MAIYA_COLLISION_H
#define MAIYA_COLLISION_H
#include <stdint.h>

#define MG_LEDGE_EDGE 4

static inline uint16_t mg_ledge_blocks(int16_t width)
{
    uint16_t blocks = (uint16_t)((width + 16) >> 5);
    return blocks ? blocks : 1u;
}

static inline uint8_t mg_ledge_reaches(int16_t x, int16_t left, int16_t width)
{
    /* The renderer rounds to whole 32-pixel caps: a 48-pixel authored
     * ledge is visibly 64 wide. Support those drawn pixels, plus the boot. */
    return (uint8_t)(width > 0 && x >= left - MG_LEDGE_EDGE &&
                     x <= left + (int16_t)(mg_ledge_blocks(width) << 5) + MG_LEDGE_EDGE);
}

static inline uint8_t mg_attack_height(int16_t top, int16_t height,
                                        int16_t target_top, int16_t target_height)
{
    return (uint8_t)(height > 0 && target_height > 0 &&
                     top < target_top + target_height && target_top < top + height);
}

static inline uint8_t mg_whip_height(int16_t feet, uint8_t crouched,
                                      int16_t target_top, int16_t target_height)
{
    /* The standing whip sweeps the chest; kneeling sweeps just above the road. */
    return mg_attack_height((int16_t)(feet - (crouched ? 18 : 38)),
                            (int16_t)(crouched ? 18 : 26), target_top, target_height);
}
#endif
