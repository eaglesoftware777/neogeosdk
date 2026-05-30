/*
 * demo_fix.c — Scene 9: FIX layer showcase
 *
 * Demonstrates: 40x28 tile overlay, palette slots, charset,
 * color cycling, scrolling text, custom infix tiles.
 *
 * https://eaglesoftware.biz
 */

#include "demo_fix.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include "sdk/2d_engine/ng_timers.h"
#include "sdk/2d_engine/ng_progress.h"
#include "sdk/2d_engine/ng_status.h"
#include <stdint.h>

void NEOGEO_USER waitVbl(void);
void NEOGEO_USER ngfix_write_tile(uint8_t x, uint8_t y, uint16_t tile, uint8_t pal);

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                     */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER fix_wait(uint16_t frames)
{
    return demo_wait(frames);
}

/* Scrolling marquee — scrolls text left across a row */
static void NEOGEO_USER fix_scroll_line(uint8_t row, const char *text, uint8_t pal, uint8_t passes)
{
    uint8_t  p;
    uint8_t  len = 0u;
    uint8_t  pos;
    char     buf[41];

    while (text[len] && len < 40u) len++;

    for (p = 0u; p < passes; p++) {
        for (pos = 0u; pos < len + 40u; pos++) {
            uint8_t c;
            for (c = 0u; c < 38u; c++) {
                int8_t ti = (int8_t)((int8_t)pos - (int8_t)(37u - c));
                if (ti >= 0 && (uint8_t)ti < len) {
                    buf[c] = text[(uint8_t)ti];
                } else {
                    buf[c] = ' ';
                }
            }
            buf[38] = '\0';
            demo_fix_puts(1u, row, buf, pal);
            waitVbl();
            if (demo_advance_requested()) return;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Sub-scene: timers, progress bars, status flags                       */
/* ------------------------------------------------------------------ */
static void NEOGEO_USER fix_timers_progress(void)
{
    uint16_t t;
    uint8_t  i;
    uint8_t  status_bits[4];
    uint16_t status_timer;
    char     bar[14];

    /* Page 1: countdown timer */
    demo_clear_scene();
    demo_caption("TIMERS / PROGRESS / STATUS", "REAL-TIME SDK COUNTERS", "NG ENGINE UTILS");

    ng_timer_start(0u, 180u);

    for (t = 0u; t < 200u; t++) {
        uint8_t pct;
        uint8_t filled;
        uint16_t val;

        ng_timers_update();
        val  = ng_timer_value(0u);
        pct  = ng_timer_percent_left(0u);
        filled = (uint8_t)(pct / 10u);
        if (filled > 10u) filled = 10u;

        bar[0] = '[';
        for (i = 0u; i < 10u; i++) bar[1u + i] = (i < filled) ? '#' : ' ';
        bar[11] = ']';
        bar[12] = '\0';

        demo_fix_puts(4u,  8u, "TIMER:", 0u);
        demo_fix_puts(11u, 8u, bar, (val > 60u) ? 1u : 2u);

        {
            char nbuf[6];
            nbuf[0] = (char)('0' + val / 100u % 10u);
            nbuf[1] = (char)('0' + val / 10u  % 10u);
            nbuf[2] = (char)('0' + val         % 10u);
            nbuf[3] = '\0';
            demo_fix_puts(24u, 8u, nbuf, 1u);
        }

        if (ng_timer_done(0u)) {
            demo_fix_puts(14u, 10u, "TIME UP!", 2u);
            demo_wait(30u);
            break;
        }
        if (demo_frame()) goto timers_done;
    }

    demo_wait(30u);

    /* Page 2: progress bar */
    demo_clear_scene();
    demo_caption("PROGRESS BAR", "FILLING UP OVER TIME", "NG_PROGRESS MODULE");

    ng_progress_start(0u, 10u);

    for (i = 0u; i < 10u; i++) {
        ng_progress_add(0u, 1u);
        {
            uint8_t pct   = ng_progress_percent(0u);
            uint8_t fill2 = (uint8_t)(pct / 10u);
            if (fill2 > 10u) fill2 = 10u;

            bar[0] = '[';
            for (t = 0u; t < 10u; t++) bar[1u + t] = ((uint8_t)t < fill2) ? '#' : ' ';
            bar[11] = ']';
            bar[12] = '\0';
            demo_fix_puts(4u, 8u, "PROGRESS:", 0u);
            demo_fix_puts(14u, 8u, bar, 1u);

            {
                char pbuf[8];
                pbuf[0] = (char)('0' + (i + 1u));
                pbuf[1] = '/';
                pbuf[2] = '1';
                pbuf[3] = '0';
                pbuf[4] = '\0';
                demo_fix_puts(27u, 8u, pbuf, 2u);
            }
        }
        if (demo_wait(24u)) goto timers_done;
    }

    demo_fix_puts(14u, 10u, "COMPLETE!", 2u);
    demo_wait(60u);

    /* Page 3: status flags */
    demo_clear_scene();
    demo_caption("STATUS FLAGS", "FOUR BITS TOGGLED EVERY 30 FRAMES", "NG_STATUS MODULE");

    for (i = 0u; i < 4u; i++) {
        ng_status_clear((uint16_t)i);
        status_bits[i] = 0u;
    }
    status_timer = 0u;

    for (t = 0u; t < 240u; t++) {
        if (status_timer == 0u) {
            for (i = 0u; i < 4u; i++) {
                ng_status_toggle((uint16_t)i);
                status_bits[i] = ng_status_has((uint16_t)i);
            }
        }
        status_timer = (uint16_t)((status_timer + 1u) % 30u);

        for (i = 0u; i < 4u; i++) {
            char sbuf[18];
            sbuf[0]  = 'S';
            sbuf[1]  = 'T';
            sbuf[2]  = 'A';
            sbuf[3]  = 'T';
            sbuf[4]  = 'U';
            sbuf[5]  = 'S';
            sbuf[6]  = ' ';
            sbuf[7]  = '[';
            sbuf[8]  = (char)('0' + i);
            sbuf[9]  = ']';
            sbuf[10] = ':';
            sbuf[11] = ' ';
            if (status_bits[i]) {
                sbuf[12] = 'O'; sbuf[13] = 'N'; sbuf[14] = ' '; sbuf[15] = '\0';
            } else {
                sbuf[12] = 'O'; sbuf[13] = 'F'; sbuf[14] = 'F'; sbuf[15] = '\0';
            }
            demo_fix_puts(10u, (uint8_t)(9u + i * 2u), sbuf,
                          status_bits[i] ? 2u : 0u);
        }

        if (demo_frame()) goto timers_done;
    }

timers_done:
    demo_clear_scene();
}

/* ------------------------------------------------------------------ */
/*  Public: FIX scene                                                    */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_fix_run(void)
{
    uint8_t  i, col, row;
    char     ch[2];

    fix_timers_progress();

    /* Page 1 — overview */
    demo_clear_scene();
    demo_caption("FIX LAYER DEMO", "NEO GEO 40x28 TILE OVERLAY", "8x8 PIXELS  4-BIT PALETTE");

    demo_fix_puts(2u, 7u,  "THE FIX LAYER IS A FIXED-POSITION", 0u);
    demo_fix_puts(2u, 8u,  "TEXT PLANE ALWAYS DRAWN ON TOP OF", 0u);
    demo_fix_puts(2u, 9u,  "ALL SPRITES AND BACKGROUND LAYERS.", 0u);
    demo_fix_puts(2u, 11u, "40 COLUMNS   28 ROWS   1120 TILES", 2u);
    demo_fix_puts(2u, 12u, "EACH TILE: 8x8 PIXELS  4-BIT COLOR", 1u);
    demo_fix_puts(2u, 14u, "USED FOR: HUD  STATUS BARS  TEXT", 0u);
    demo_fix_puts(2u, 15u, "          SCORES  TIMERS  OVERLAYS", 0u);

    if (fix_wait(240u)) goto done;

    /* Page 2 — palette slots */
    demo_clear_scene();
    demo_caption("FIX PALETTE DEMO", "THREE ACTIVE PALETTE SLOTS", "EACH TILE CARRIES A 4-BIT PAL INDEX");

    demo_fix_puts(4u, 8u,  "PAL 0  DEFAULT  TITLE TEXT", 0u);
    demo_fix_puts(4u, 10u, "PAL 1  ACCENT   SUBTITLES",  1u);
    demo_fix_puts(4u, 12u, "PAL 2  GREEN    HIGHLIGHTS", 2u);
    demo_fix_puts(2u, 16u, "TEXT COLOR IS SET PER TILE --", 0u);
    demo_fix_puts(2u, 17u, "NO REWRITE NEEDED TO CHANGE IT.", 0u);

    for (i = 0u; i < 120u; i++) {
        demo_fix_puts(4u, 20u, "WATCH THIS LINE BLINK",
                      (uint8_t)(((i >> 3) & 1u) ? 1u : 2u));
        waitVbl();
        if (demo_advance_requested()) goto done;
    }

    /* Page 3 — full printable ASCII */
    demo_clear_scene();
    demo_caption("FIX CHARSET", "PRINTABLE ASCII  0x20 TO 0x7E", "95 CHARACTERS DIRECT TO FIX RAM");

    ch[1] = '\0';
    col = 2u;
    row = 7u;
    for (i = 0x20u; i < 0x7Fu; i++) {
        ch[0] = (char)i;
        demo_fix_puts(col, row, ch, (uint8_t)(((i - 0x20u) >> 4) & 3u));
        col++;
        if (col >= 38u) { col = 2u; row++; }
    }

    if (fix_wait(240u)) goto done;

    /* Page 4 — color cycling animation */
    demo_clear_scene();
    demo_caption("FIX ANIMATION", "COLOR CYCLING WITH NO SPRITE COST", "PALETTE SWAP IS INSTANT");

    demo_fix_puts(5u, 8u,  "NEO GEO SDK   FIX LAYER", 0u);
    demo_fix_puts(5u, 10u, "HARDWARE TEXT OVERLAY", 1u);
    demo_fix_puts(5u, 12u, "ALWAYS SHARP  ALWAYS FAST", 2u);

    for (i = 0u; i < 180u; i++) {
        static const uint8_t cycle[4] = {0u, 1u, 2u, 1u};
        demo_fix_puts(5u, 14u, "-- COLOR CYCLING LINE --",
                      cycle[((uint8_t)(i >> 4)) & 3u]);
        waitVbl();
        if (demo_advance_requested()) goto done;
    }

    /* Page 5 — scrolling marquee */
    demo_clear_scene();
    demo_caption("FIX SCROLLING TEXT", "HORIZONTAL MARQUEE  NO SPRITES", "ZERO SPRITE COST");

    demo_fix_puts(2u, 7u, "SCROLLING TEXT IS PURELY FIX LAYER.", 0u);
    demo_fix_puts(2u, 8u, "NO SPRITES CONSUMED.  FREE HW TRICK.", 1u);

    fix_scroll_line(12u, "  NEO GEO SDK  HIGH-PERFORMANCE 2D ENGINE  EAGLESOFTWARE.BIZ  ", 2u, 2u);
    if (demo_advance_requested()) goto done;

    /* Page 6 — custom infix tiles */
    demo_clear_scene();
    demo_caption("CUSTOM FIX TILES", "artbox/infix/*.png -> S1 ROM", "PIXEL ART BURNED IN AT BUILD TIME");

    demo_fix_puts(2u, 7u, "CUSTOM 8x8 TILES LIVE IN S1 ROM", 0u);
    demo_fix_puts(2u, 8u, "DRAWN FROM artbox/infix/0.png",   1u);
    demo_fix_puts(2u, 9u, "16x8 TILE GRID  INDEX 256-383",   2u);

    for (row = 0u; row < 8u; row++) {
        for (col = 0u; col < 16u; col++) {
            ngfix_write_tile((uint8_t)(12u + col), (uint8_t)(12u + row),
                             (uint16_t)(256u + (uint16_t)row * 16u + col), 1u);
        }
    }

    fix_wait(240u);

done:
    demo_clear_scene();
}

/* Legacy compat */
void NEOGEO_USER demo_fix_showcase(void)
{
    demo_fix_run();
}
