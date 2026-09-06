#ifndef NG_SPRITE_HW_H
#define NG_SPRITE_HW_H

/*
 * Shared sprite geometry rules for the C and C++ renderers.
 *
 * SCB3 carries the number of ACTIVE CHARACTERS.  That field is the sprite's
 * height on screen in tiles - the hardware covers exactly rows*16 scanlines
 * with it and does not care what the shrink register says.  Vertical shrink
 * only decides which SOURCE row of the SCB1 map each of those scanlines
 * reads.  So a 16-tile image drawn at half height needs 8 active characters,
 * not 16: leave the count at 16 and the picture repeats inside the taller
 * window instead of ending where it should.
 */
static inline uint8_t ng_sprite_display_rows(uint8_t source_rows,
                                             uint8_t scale_y)
{
    uint16_t rows = ((uint16_t)source_rows * ((uint16_t)scale_y + 1u) + 255u) >> 8;
    return (uint8_t)(rows ? rows : 1u);
}

/*
 * How many SCB1 map rows an upload has to fill.
 *
 * The row lookup can land past the last row of real art, so every entry it
 * can reach must hold the transparent tile or the sprite grows a band of
 * last frame's leftovers.  Its reach is bounded: the lookup reads a source
 * row inside the first 16 only, and reaches rows 16..31 exclusively through
 * the mirror it applies once a sprite spans more than 256 scanlines.  Under
 * 17 active characters that mirror never engages, so half the map is
 * unreachable and writing it is pure vblank cost.
 */
static inline uint8_t ng_sprite_map_rows(uint8_t display_rows)
{
    return (uint8_t)(display_rows > 16u ? 32u : 16u);
}

#endif
