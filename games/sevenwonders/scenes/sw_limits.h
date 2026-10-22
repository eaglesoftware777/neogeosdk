#ifndef SW_LIMITS_H
#define SW_LIMITS_H
#include "sdk/2d_engine/ng_puzzle.h"

/* Fixed renderer reservations, deliberately narrower than the SDK board. */
#define SW_MATCH_CELLS 49u
#define SW_BUBBLE_CELLS 120u

static inline uint8_t NEOGEO_USER sw_board_dimensions_valid(uint8_t mode, uint8_t w, uint8_t h)
{
    if (w < 3u || h < 3u) return 0u;
    if (mode == NGP_MODE_MATCH) return (uint8_t)(w <= 7u && h <= 7u);
    if (mode == NGP_MODE_BUBBLE) return (uint8_t)(w <= 10u && h <= 12u);
    return 0u;
}

static inline int NEOGEO_USER sw_level_init(NGPuzzle *p, const NGPuzzleLevel *lv, uint32_t seed)
{
    if (!p || !lv) return NGP_ERR_ARG;
    if (!sw_board_dimensions_valid(lv->mode, lv->w, lv->h)) return NGP_ERR_LEVEL;
    return ng_puzzle_init(p, lv, seed);
}
#endif
