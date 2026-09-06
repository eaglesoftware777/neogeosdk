#ifndef NG_SPRITE_HW_H
#define NG_SPRITE_HW_H

/* Shared by the C and C++ renderers. SCB3 measures destination rows;
 * SCB1 contains source rows. The L0 lookup may read past the last source
 * tile when shrinking, so every unused map entry must be transparent. */
static inline uint8_t ng_sprite_display_rows(uint8_t source_rows,
                                             uint8_t scale_y)
{
    uint16_t rows = ((uint16_t)source_rows * ((uint16_t)scale_y + 1u) + 255u) >> 8;
    return (uint8_t)(rows ? rows : 1u);
}

#endif
