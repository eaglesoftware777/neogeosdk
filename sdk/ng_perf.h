/*
 * ng_perf.h -- frame timing and VRAM traffic, counted in RAM
 *
 * Built only with NG_DEBUG_PERF (make PERF=1, or PERF=2 for the frame's
 * timing alone, without the per-write counting); in any other build every
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
    /* NG_VRAM_DEFER: where the rest comes from */
    uint32_t outside_sum;        /* words written outside ng_vram_commit()        */
    uint32_t outside_active_sum; /* ...of them on drawn lines                     */
    uint16_t commit_late;        /* commits that began on a drawn line            */
    uint16_t commit_spill;       /* commits that began in the blank, ended drawn  */
    uint32_t commit_lines_sum;   /* scanlines the commits took, all together      */
    uint16_t commit_lines_peak;
    uint16_t commit_begin;       /* where the current commit began                */
    uint16_t commit_groups_peak; /* the most groups one commit wrote              */
    uint32_t commit_groups_sum;
    uint32_t commit_fix_lines_sum; /* ...of the commits' lines, the text cells'   */
    uint32_t commit_fix_cells_sum;
    uint8_t  in_commit;
    uint8_t  pad0;
    /* NG_VRAM_DEFER's deadline: what a commit left for the next blank */
    uint32_t deferred_sum;       /* groups with content left unwritten, all commits */
    uint16_t deferred_peak;      /* the most one commit left                       */
    uint16_t queue_peak;         /* the most groups listed at a commit's start     */
    uint16_t queue_full;         /* flushes that found their list full (never written at once) */
    uint16_t fix_rows_sum;       /* FIX rows rewritten whole after a full run list */
} NGPerf;

extern NGPerf ng_perf;

void ng_perf_frame_begin(void);
void ng_perf_frame_end(void);
void ng_perf_vram(uint16_t words);
uint16_t ng_perf_line(void);
void ng_perf_commit(uint8_t begin);
void ng_perf_commit_part(uint16_t groups, uint16_t fix_cells, uint16_t fix_from_line);

#define NG_PERF_FRAME_BEGIN() ng_perf_frame_begin()
#define NG_PERF_FRAME_END()   ng_perf_frame_end()
#ifdef NG_DEBUG_PERF_LITE
/* PERF=2: the frame's timing only. Counting every VRAM write costs a call
 * and a counter read each, enough to add overruns of its own; the commit's
 * start and end (two line reads a frame) and its leftovers are kept. */
#define NG_PERF_VRAM(n)       ((void)0)
#define NG_PERF_COMMIT(begin) ng_perf_commit(begin)
#define NG_PERF_COMMIT_PART(g, c, l) ng_perf_commit_part((g), (c), (l))
#define NG_PERF_LINE() ng_perf_line()
#else
#define NG_PERF_VRAM(n)       ng_perf_vram((uint16_t)(n))
#define NG_PERF_COMMIT(begin) ng_perf_commit(begin)
#define NG_PERF_COMMIT_PART(g, c, l) ng_perf_commit_part((g), (c), (l))
#define NG_PERF_LINE() ng_perf_line()
#endif
#define NG_PERF_DEFERRED(n)   do { uint16_t n_ = (uint16_t)(n); ng_perf.deferred_sum += n_; \
                                   if (n_ > ng_perf.deferred_peak) ng_perf.deferred_peak = n_; } while (0)
#define NG_PERF_QUEUE(n)      do { uint16_t n_ = (uint16_t)(n); \
                                   if (n_ > ng_perf.queue_peak) ng_perf.queue_peak = n_; } while (0)
#define NG_PERF_QUEUE_FULL()  (ng_perf.queue_full++)
#define NG_PERF_FIX_ROWS(n)   (ng_perf.fix_rows_sum = (uint16_t)(ng_perf.fix_rows_sum + (n)))

#else

#define NG_PERF_FRAME_BEGIN() ((void)0)
#define NG_PERF_FRAME_END()   ((void)0)
#define NG_PERF_VRAM(n)       ((void)0)
#define NG_PERF_COMMIT(begin) ((void)0)
#define NG_PERF_COMMIT_PART(g, c, l) ((void)0)
#define NG_PERF_LINE() 0u
#define NG_PERF_DEFERRED(n)   ((void)0)
#define NG_PERF_QUEUE(n)      ((void)0)
#define NG_PERF_QUEUE_FULL()  ((void)0)
#define NG_PERF_FIX_ROWS(n)   ((void)0)

#endif

#ifdef __cplusplus
}
#endif
#endif
