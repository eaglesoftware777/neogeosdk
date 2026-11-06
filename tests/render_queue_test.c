#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef __cplusplus
#include "ng_render_queue.hpp"
#include "ng_hw.hpp"
#else
#include "ng_render_queue.h"
#include "neogeo.h"
#endif
static uint16_t output[256][16];
static unsigned writes;
static uint8_t left = 255u;
uint8_t ng_vram_lines_left(void) { return left; }
void vram_SCB234(uint16_t addr, uint16_t value)
{ (void)addr; (void)value; writes++; if (left) left--; }
void load_palettes(uint16_t *colors, uintptr_t address)
{
    unsigned bank = (unsigned)((address - PALETTES) / 32u);
    assert(bank < 256u);
    memcpy(output[bank], colors, 32u);
    left = left >= 4u ? (uint8_t)(left - 4u) : 0u;
}

int main(void)
{
    uint16_t colors[16];
    unsigned i;
    ng_render_queue_init();
    for (i = 0u; i < NG_RQ_MAX_CMDS + 1u; i++) ng_rq_sprite_shrink((uint16_t)(i + 1u), 0x0FFFu);
    assert(!ng_rq_free() && ng_rq_stats.command_overflows == 1u && ng_rq_stats.command_peak == 128u);
#ifdef NG_VRAM_DEFER
    left = 100u;
    ng_render_queue_flush();
    assert(writes == 98u && ng_rq_free() == 98u);
#endif
    left = 255u; ng_render_queue_flush();
    assert(writes == 128u && ng_rq_free() == 128u);
    for (i = 0u; i < 16u; i++) colors[i] = (uint16_t)(0x1230u + i);
    ng_rq_palette_upload(20u, colors);
    colors[1] = 0x2345u;
    ng_rq_palette_upload(21u, colors);
    colors[1] = 0x3456u;
    ng_rq_palette_upload(21u, colors); /* Same bank replaces its snapshot. */
    assert(ng_rq_pal_free() == 6u);
    colors[1] = 0xFFFFu;
    left = 4u; ng_render_queue_flush();
    assert(output[20][1] == 0x1231u);
#ifdef NG_VRAM_DEFER
    assert(ng_rq_pal_free() == 7u && output[21][1] == 0u);
    /* A new palette reuses a compacted slot. It must not alter the
     * snapshot of the bank retained from the previous blank. */
    ng_rq_palette_upload(22u, colors);
    left = 255u; ng_render_queue_flush();
#endif
    assert(output[21][1] == 0x3456u);
    assert(ng_rq_pal_free() == NG_RQ_MAX_PAL_UPLOADS);
    for (i = 0u; i < 9u; i++) ng_rq_palette_upload((uint16_t)(40u + i), colors);
    assert(ng_rq_stats.palette_overflows == 1u && ng_rq_stats.palette_peak == 8u);
    puts("render queue: bounded commands, owned palettes, compaction and overflow: PASS");
    return 0;
}
