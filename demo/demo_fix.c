#include "demo_fix.h"
#include "demo_screen.h"
#include "sdk/neogeo.h"
#include <stdint.h>

void NEOGEO_USER waitVbl(void);
void NEOGEO_USER ngfix_write_tile(uint8_t x, uint8_t y, uint16_t tile, uint8_t pal);

static uint8_t NEOGEO_USER fix_page_wait(uint16_t frames)
{
    return demo_wait_frames_or_a(frames);
}

void NEOGEO_USER demo_fix_showcase(void)
{
    uint8_t i, col, row;
    char ch[2];

    /* Page 1 — overview */
    demo_clear_scene();
    demo_scene_caption("FIX LAYER DEMO", "NEO GEO 40x28 TILE OVERLAY", "8x8 PIXELS  4-BIT PALETTE");

    demo_fix_puts(2, 7,  "THE FIX LAYER IS A FIXED-POSITION", 0);
    demo_fix_puts(2, 8,  "TEXT PLANE ALWAYS DRAWN ON TOP OF", 0);
    demo_fix_puts(2, 9,  "ALL SPRITES AND BACKGROUND LAYERS.", 0);
    demo_fix_puts(2, 11, "40 COLUMNS   28 ROWS   1120 TILES", 2);
    demo_fix_puts(2, 12, "EACH TILE: 8x8 PIXELS  4-BIT COLOR", 1);
    demo_fix_puts(2, 14, "USED FOR: HUD  STATUS BARS  TEXT", 0);
    demo_fix_puts(2, 15, "          SCORES  TIMERS  OVERLAYS", 0);

    if (fix_page_wait(300)) goto fix_done;

    /* Page 2 — palette color slots */
    demo_clear_scene();
    demo_scene_caption("FIX PALETTE DEMO", "THREE ACTIVE PALETTE SLOTS", "EACH TILE CARRIES A 4-BIT PAL INDEX");

    demo_fix_puts(4, 8,  "PAL 0  DEFAULT  TITLE TEXT", 0);
    demo_fix_puts(4, 10, "PAL 1  ACCENT   SUBTITLES", 1);
    demo_fix_puts(4, 12, "PAL 2  BRIGHT   HIGHLIGHTS", 2);

    demo_fix_puts(2, 16, "TEXT COLOR IS SET PER TILE —", 0);
    demo_fix_puts(2, 17, "NO REWRITE NEEDED TO CHANGE IT.", 0);

    /* Animated blink — alternate palette 1 and 2 on one row */
    for (i = 0; i < 120; i++) {
        demo_fix_puts(4, 20, "WATCH THIS LINE BLINK", (uint8_t)((i >> 3) & 1) ? 1 : 2);
        waitVbl();
        if (demo_advance_requested()) goto fix_done;
    }

    /* Page 3 — full printable ASCII charset */
    demo_clear_scene();
    demo_scene_caption("FIX CHARSET", "PRINTABLE ASCII  0x20 TO 0x7E", "95 CHARACTERS DIRECT TO FIX RAM");

    ch[1] = '\0';
    col = 2;
    row = 7;
    for (i = 0x20; i < 0x7F; i++) {
        ch[0] = (char)i;
        demo_fix_puts(col, row, ch, (uint8_t)(((i - 0x20u) / 16u) & 3u));
        col++;
        if (col >= 38u) { col = 2u; row++; }
    }

    if (fix_page_wait(300)) goto fix_done;

    /* Page 4 — color cycling animation */
    demo_clear_scene();
    demo_scene_caption("FIX ANIMATION", "COLOR CYCLING WITH NO SPRITE COST", "PALETTE SWAP IS INSTANT");

    demo_fix_puts(5, 8,  "NEO GEO SDK   FIX LAYER", 0);
    demo_fix_puts(5, 10, "HARDWARE TEXT OVERLAY", 1);
    demo_fix_puts(5, 12, "ALWAYS SHARP  ALWAYS FAST", 2);

    for (i = 0; i < 180; i++) {
        static const uint8_t cycle[4] = {0, 1, 2, 1};
        demo_fix_puts(5, 14, "-- COLOR CYCLING LINE --", cycle[((uint8_t)(i >> 4)) & 3u]);
        waitVbl();
        if (demo_advance_requested()) goto fix_done;
    }

    /* Page 5 — custom infix tiles from artbox/infix/ PNGs */
    demo_clear_scene();
    demo_scene_caption("CUSTOM FIX TILES", "artbox/infix/*.png -> S1 ROM TILE 256+", "PIXEL ART BURNED IN AT BUILD TIME");

    demo_fix_puts(2,  7, "CUSTOM 8x8 TILES LIVE IN S1 ROM", 0);
    demo_fix_puts(2,  8, "DRAWN FROM artbox/infix/0.png", 1);
    demo_fix_puts(2,  9, "16x8 TILE GRID  TILE INDEX 256-383", 2);

    for (row = 0; row < 8u; row++) {
        for (col = 0; col < 16u; col++) {
            ngfix_write_tile((uint8_t)(12u + col), (uint8_t)(12u + row),
                             (uint16_t)(256u + (uint16_t)row * 16u + col), 1u);
        }
    }

    if (fix_page_wait(300)) goto fix_done;

fix_done:
    demo_clear_scene();
}
