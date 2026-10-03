/*
 * ng_perf.h -- frame timing and VRAM traffic, counted in RAM
 *
 * Built only with NG_DEBUG_PERF (make PERF=1); in any other build every
 * macro here is empty and nothing is added to the ROM. A test script reads
 * the counters by symbol (ng_perf) instead of having them drawn, so
 * measuring doesn't change what is measured.
 *
 * The frame is taken from where the game waits for the vertical blank:
 *   NG_PERF_FRAME_END()    just before the wait: the work for this frame
 *                          is done;
 *   NG_PERF_FRAME_BEGIN()  just after it.
 * NG_PERF_VRAM(n) counts n words written to VRAM, and those of them written
 * while the screen is being drawn (lines 16..239) as well.
 *
 * The line comes from the raster counter in REG_LSPCMODE ($3C0006) bits
 * 15..7: the scanline plus $100, running $100..$1FF and then $F8..$FF
 * (MAME src/mame/snk/neogeo_v.cpp, get_video_control: "the vertical counter
 * chain goes from 0xf8 - 0x1ff"; NeoGeoDev wiki, LSPC). Lines 16..239 are
 * drawn, the vertical blank starts at line 240 (MAME neogeo_spr.h,
 * NEOGEO_VBEND / NEOGEO_VBSTART), and a frame has 264 lines.
 */
#ifndef NG_PERF_H
#define NG_PERF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NG_DEBUG_PERF

typedef struct {
    uint32_t frames;          /* frames measured                                   */
    uint32_t overruns;        /* frames whose work ran past the next vertical blank */
    uint32_t lines_sum;       /* scanlines of work, all frames together            */
    uint32_t vram_sum;        /* VRAM words written, all frames                    */
    uint32_t vram_active_sum; /* ...of them while the screen was being drawn       */
    uint16_t lines;           /* the last frame: scanlines from the end of the wait
                               * to the end of its work (264 or more: overran)     */
    uint16_t lines_peak;
    uint16_t vram;            /* the last frame's VRAM words                       */
    uint16_t vram_peak;
    uint16_t vram_active;     /* ...of them while the screen was being drawn       */
    uint16_t vram_active_peak;
    uint16_t begin_line;      /* where the current frame's work began              */
    uint16_t vram_cur, vram_active_cur;
} NGPerf;

extern NGPerf ng_perf;

void ng_perf_frame_begin(void);
void ng_perf_frame_end(void);
void ng_perf_vram(uint16_t words);
uint16_t ng_perf_line(void);

#define NG_PERF_FRAME_BEGIN() ng_perf_frame_begin()
#define NG_PERF_FRAME_END()   ng_perf_frame_end()
#define NG_PERF_VRAM(n)       ng_perf_vram((uint16_t)(n))

#else

#define NG_PERF_FRAME_BEGIN() ((void)0)
#define NG_PERF_FRAME_END()   ((void)0)
#define NG_PERF_VRAM(n)       ((void)0)

#endif

#ifdef __cplusplus
}
#endif
#endif
