/*
 * ng_perf_impl.h -- the counters of ng_perf.h, included once by each
 * engine's debug module (ng_debug.c, ng_debug.cpp) in NG_DEBUG_PERF builds:
 * the engines' RAM is always kept by the linker scripts.
 */
#ifndef NG_PERF_IMPL_H
#define NG_PERF_IMPL_H

#include "ng_perf.h"
#include "macro.h"

#ifdef NG_DEBUG_PERF

#ifdef __cplusplus
extern "C" {
#endif

NGPerf ng_perf;
static uint8_t ng_perf_started;

/* The scanline now, 0..263, from the raster counter (see ng_perf.h). */
uint16_t ng_perf_line(void)
{
    uint16_t c = (uint16_t)(*(volatile uint16_t *)0x3C0006u >> 7);
    return (uint16_t)(c >= 0x100u ? c - 0x100u : c - 0xF8u + 0x100u);
}

void ng_perf_frame_begin(void)
{
    ng_perf.begin_line = ng_perf_line();
    ng_perf.vram_cur = 0u;
    ng_perf.vram_active_cur = 0u;
    ng_perf_started = 1u;
}

void ng_perf_frame_end(void)
{
    uint16_t now = ng_perf_line();
    uint16_t lines;
    if (!ng_perf_started) return;
    ng_perf_started = 0u;
    lines = (uint16_t)(now >= ng_perf.begin_line ? now - ng_perf.begin_line
                                                 : now + 264u - ng_perf.begin_line);
    /* The vertical blank interrupt sets this flag and the wait clears it:
     * set already means a blank went by while the frame was still working. */
    if (*(volatile uint16_t *)USER_WORKRAM) {
        lines = (uint16_t)(lines + 264u);
        ng_perf.overruns++;
    }
    ng_perf.frames++;
    ng_perf.lines = lines;
    ng_perf.lines_sum += lines;
    if (lines > ng_perf.lines_peak) ng_perf.lines_peak = lines;
    ng_perf.vram = ng_perf.vram_cur;
    ng_perf.vram_active = ng_perf.vram_active_cur;
    ng_perf.vram_sum += ng_perf.vram_cur;
    ng_perf.vram_active_sum += ng_perf.vram_active_cur;
    if (ng_perf.vram_cur > ng_perf.vram_peak) ng_perf.vram_peak = ng_perf.vram_cur;
    if (ng_perf.vram_active_cur > ng_perf.vram_active_peak) ng_perf.vram_active_peak = ng_perf.vram_active_cur;
}

void ng_perf_vram(uint16_t words)
{
    uint16_t line = ng_perf_line();
    uint8_t drawn = (uint8_t)(line >= 16u && line < 240u);
    ng_perf.vram_cur = (uint16_t)(ng_perf.vram_cur + words);
    if (drawn) ng_perf.vram_active_cur = (uint16_t)(ng_perf.vram_active_cur + words);
    if (!ng_perf.in_commit) {
        ng_perf.outside_sum += words;
        if (drawn) ng_perf.outside_active_sum += words;
    }
}

static uint8_t ng_perf_commit_began_blank;

void ng_perf_commit_part(uint16_t groups, uint16_t fix_cells, uint16_t fix_from_line)
{
    uint16_t now = ng_perf_line();
    ng_perf.commit_groups_sum += groups;
    if (groups > ng_perf.commit_groups_peak) ng_perf.commit_groups_peak = groups;
    ng_perf.commit_fix_cells_sum += fix_cells;
    ng_perf.commit_fix_lines_sum += (uint16_t)(now >= fix_from_line ? now - fix_from_line : now + 264u - fix_from_line);
}

void ng_perf_commit(uint8_t begin)
{
    uint16_t line = ng_perf_line();
    uint8_t drawn = (uint8_t)(line >= 16u && line < 240u);
    if (begin) {
        ng_perf.commit_begin = line;
        ng_perf.in_commit = 1u;
        ng_perf_commit_began_blank = (uint8_t)!drawn;
        if (drawn) ng_perf.commit_late++;
    } else {
        uint16_t lines = (uint16_t)(line >= ng_perf.commit_begin ? line - ng_perf.commit_begin
                                                                 : line + 264u - ng_perf.commit_begin);
        ng_perf.in_commit = 0u;
        if (ng_perf_commit_began_blank && drawn) ng_perf.commit_spill++;
        ng_perf.commit_lines_sum += lines;
        if (lines > ng_perf.commit_lines_peak) ng_perf.commit_lines_peak = lines;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* NG_DEBUG_PERF */
#endif
