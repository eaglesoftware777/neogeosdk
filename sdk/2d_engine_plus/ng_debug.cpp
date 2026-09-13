/*
 * ng_debug.c — Performance / debug overlay (Stage 11)
 *
 * Real implementation compiled only when NG_DEBUG_PERF is defined.
 * When NG_DEBUG_PERF is not set, this file is effectively empty.
 */

#include "ng_debug.hpp"

/* Prevent the header's static stubs from being seen here */
#undef ng_debug_init
#undef ng_debug_draw
#undef ng_debug_clear

extern "C" {

#ifdef NG_DEBUG_PERF

#include "ng_fix.hpp"
#include "ng_render_queue.hpp"


/* Global debug counters — set by the game/engine each frame */
uint16_t ng_dbg_sprite_groups   = 0;
uint16_t ng_dbg_sprites_used    = 0;
uint16_t ng_dbg_particles       = 0;
uint8_t  ng_dbg_rq_used         = 0;
uint8_t  ng_dbg_pal_q_used      = 0;
int16_t  ng_dbg_camera_x        = 0;
int16_t  ng_dbg_camera_y        = 0;
uint32_t ng_dbg_frame           = 0;
uint8_t  ng_dbg_hitstop         = 0;
uint8_t  ng_dbg_vblank_overflow = 0;

/* Small integer → fixed-length string, right-justified in `width` chars */
static void NEOGEO_USER dbg_itoa(char *buf, int16_t value, uint8_t width)
{
    uint8_t i;
    uint8_t neg = 0;
    uint16_t uv;

    for (i = 0; i < width; i++) buf[i] = ' ';
    buf[width] = '\0';

    if (value < 0) {
        neg = 1;
        uv  = (uint16_t)(-(int32_t)value);
    } else {
        uv  = (uint16_t)value;
    }

    i = (uint8_t)(width - 1);
    do {
        buf[i] = (char)('0' + (uv % 10));
        uv /= 10;
        if (i == 0) break;
        i--;
    } while (uv > 0);

    if (neg && i > 0) {
        i--;
        buf[i] = '-';
    }
}

/* Print a label + value pair on row y */
static void NEOGEO_USER dbg_row(uint8_t y, const char *label, int16_t value)
{
    char num[6];
    dbg_itoa(num, value, 5);
    ng_fix_puts(NG_DBG_X, (uint8_t)(NG_DBG_Y_START + y), label, NG_DBG_PAL);
    ng_fix_puts((uint8_t)(NG_DBG_X + 4), (uint8_t)(NG_DBG_Y_START + y), num, NG_DBG_PAL);
}

void NEOGEO_USER ng_debug_init(void)
{
    ng_dbg_sprite_groups   = 0;
    ng_dbg_sprites_used    = 0;
    ng_dbg_particles       = 0;
    ng_dbg_rq_used         = 0;
    ng_dbg_pal_q_used      = 0;
    ng_dbg_camera_x        = 0;
    ng_dbg_camera_y        = 0;
    ng_dbg_frame           = 0;
    ng_dbg_hitstop         = 0;
    ng_dbg_vblank_overflow = 0;
}

void NEOGEO_USER ng_debug_draw(void)
{
    uint8_t row = 0;

    /* Update the RQ usage counter from the current queue state */
    ng_dbg_rq_used     = (uint8_t)(NG_RQ_MAX_CMDS   - ng_rq_free());
    ng_dbg_pal_q_used  = (uint8_t)(NG_RQ_MAX_PAL_UPLOADS - ng_rq_pal_free());

    dbg_row(row++, "GRP ", (int16_t)ng_dbg_sprite_groups);
    dbg_row(row++, "SPR ", (int16_t)ng_dbg_sprites_used);
    dbg_row(row++, "PAR ", (int16_t)ng_dbg_particles);
    dbg_row(row++, "RQ  ", (int16_t)ng_dbg_rq_used);
    dbg_row(row++, "PQ  ", (int16_t)ng_dbg_pal_q_used);
    dbg_row(row++, "CX  ", ng_dbg_camera_x);
    dbg_row(row++, "CY  ", ng_dbg_camera_y);
    dbg_row(row++, "HIT ", (int16_t)ng_dbg_hitstop);

    /* VBlank overflow warning */
    if (ng_dbg_vblank_overflow) {
        ng_fix_puts(NG_DBG_X, (uint8_t)(NG_DBG_Y_START + row), "VBL!", NG_DBG_PAL);
    } else {
        ng_fix_puts(NG_DBG_X, (uint8_t)(NG_DBG_Y_START + row), "    ", NG_DBG_PAL);
    }

    ng_dbg_frame++;
}

void NEOGEO_USER ng_debug_clear(void)
{
    uint8_t row;
    for (row = 0; row < 10; row++) {
        ng_fix_clear_rect(NG_DBG_X, (uint8_t)(NG_DBG_Y_START + row), 9, 1, NG_DBG_PAL);
    }
}

#else  /* !NG_DEBUG_PERF — provide storage so extern declarations always link */

uint16_t ng_dbg_sprite_groups   = 0;
uint16_t ng_dbg_sprites_used    = 0;
uint16_t ng_dbg_particles       = 0;
uint8_t  ng_dbg_rq_used         = 0;
uint8_t  ng_dbg_pal_q_used      = 0;
int16_t  ng_dbg_camera_x        = 0;
int16_t  ng_dbg_camera_y        = 0;
uint32_t ng_dbg_frame           = 0;
uint8_t  ng_dbg_hitstop         = 0;
uint8_t  ng_dbg_vblank_overflow = 0;

#endif /* NG_DEBUG_PERF */


} /* extern "C" */
