/*
 * ng_debug.h — Performance / debug overlay using the FIX layer (Stage 11)
 *
 * Compile-time flag: NG_DEBUG_PERF
 *   Define NG_DEBUG_PERF=1 in CFLAGS to enable the overlay.
 *   When 0 (default release), all debug functions compile to nothing.
 *
 * FIX layer mapping (uses right side of screen to avoid UI conflicts):
 *   Row 1:  Sprite groups active
 *   Row 2:  Hardware sprites used
 *   Row 3:  Particle count
 *   Row 4:  Render queue usage
 *   Row 5:  Palette queue usage
 *   Row 6:  Camera X
 *   Row 7:  Camera Y
 *   Row 8:  Frame timer / game time
 *   Row 9:  Hitstop remaining
 *   Row 10: VBlank overflow (if detected)
 *
 * All output uses ng_fix_puts() which has its own dirty-flag cache,
 * so only changed values incur VRAM writes.
 */

#ifndef NG_DEBUG_HPP
#define NG_DEBUG_HPP

#include "ng_defs.hpp"

/* Column for debug overlay (right side, FIX safe area = 1..38) */
#define NG_DBG_X   22
/* First row for debug overlay */
#define NG_DBG_Y_START  2
/* FIX palette for debug text (use NG_PAL_UI_BASE = 4) */
#define NG_DBG_PAL  4

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NG_DEBUG_PERF

/*
 * Debug counters — set these each frame before calling ng_debug_draw().
 * They are plain globals: fast to write, no function call overhead.
 */
extern uint16_t ng_dbg_sprite_groups;
extern uint16_t ng_dbg_sprites_used;
extern uint16_t ng_dbg_particles;
extern uint8_t  ng_dbg_rq_used;
extern uint8_t  ng_dbg_pal_q_used;
extern int16_t  ng_dbg_camera_x;
extern int16_t  ng_dbg_camera_y;
extern uint32_t ng_dbg_frame;
extern uint8_t  ng_dbg_hitstop;
extern uint8_t  ng_dbg_vblank_overflow;

void NEOGEO_USER ng_debug_init(void);
void NEOGEO_USER ng_debug_draw(void);
void NEOGEO_USER ng_debug_clear(void);

#else  /* NG_DEBUG_PERF == 0 → compile out everything */

/* In release builds, expose the counters as extern so game code can still
 * write them without getting "defined but not used" warnings. Storage is in
 * ng_debug.c regardless of NG_DEBUG_PERF so the link always resolves. */
extern uint16_t ng_dbg_sprite_groups;
extern uint16_t ng_dbg_sprites_used;
extern uint16_t ng_dbg_particles;
extern uint8_t  ng_dbg_rq_used;
extern uint8_t  ng_dbg_pal_q_used;
extern int16_t  ng_dbg_camera_x;
extern int16_t  ng_dbg_camera_y;
extern uint32_t ng_dbg_frame;
extern uint8_t  ng_dbg_hitstop;
extern uint8_t  ng_dbg_vblank_overflow;

/* Suppress unused-variable warnings on the stubs above */
#define NG_DBG_SUPPRESS_UNUSED \
    (void)ng_dbg_sprite_groups; (void)ng_dbg_sprites_used; \
    (void)ng_dbg_particles; (void)ng_dbg_rq_used; \
    (void)ng_dbg_pal_q_used; (void)ng_dbg_camera_x; \
    (void)ng_dbg_camera_y; (void)ng_dbg_frame; \
    (void)ng_dbg_hitstop; (void)ng_dbg_vblank_overflow;

#define ng_debug_init()  ((void)0)
#define ng_debug_draw()  ((void)0)
#define ng_debug_clear() ((void)0)

#endif /* NG_DEBUG_PERF */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NG_DEBUG_HPP */
