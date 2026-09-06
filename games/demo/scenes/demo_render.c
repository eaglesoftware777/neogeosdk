/*
 * demo_render.c — Software 2D rendering via FIX layer
 *
 * https://eaglesoftware.biz
 */

#include "demo_render.h"
#include "demo.h"
#include "sdk/neogeo.h"
#include <stdint.h>
#ifdef __cplusplus
/* A USE_2D_PLUS build compiles this file as C++.  Everything here is
 * reached from inline asm, the cart entry vectors or the BIOS by its
 * plain symbol name, so it must keep C linkage and not be mangled. */
extern "C" {
#endif


void NEOGEO_USER clearFix(void);

void NEOGEO_USER demo_render_init(void)
{
    clearFix();
}

void NEOGEO_USER demo_render_clear(void)
{
    clearFix();
}

void NEOGEO_USER demo_render_hbar(uint8_t x, uint8_t y, uint8_t len, uint8_t pal)
{
    uint8_t i;
    char buf[2];
    buf[1] = '\0';
    for (i = 0u; i < len && (x + i) < 40u; i++) {
        buf[0] = '-';
        demo_fix_puts((uint8_t)(x + i), y, buf, pal);
    }
}

void NEOGEO_USER demo_render_vbar(uint8_t x, uint8_t y, uint8_t h, uint8_t pal)
{
    uint8_t i;
    char buf[2];
    buf[1] = '\0';
    for (i = 0u; i < h && (y + i) < 28u; i++) {
        buf[0] = '|';
        demo_fix_puts(x, (uint8_t)(y + i), buf, pal);
    }
}

void NEOGEO_USER demo_render_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pal)
{
    uint8_t i;
    char buf[2];
    buf[1] = '\0';

    if (w < 2u || h < 2u) return;

    /* Top edge */
    buf[0] = '+'; demo_fix_puts(x, y, buf, pal);
    demo_render_hbar((uint8_t)(x + 1u), y, (uint8_t)(w - 2u), pal);
    buf[0] = '+'; demo_fix_puts((uint8_t)(x + w - 1u), y, buf, pal);

    /* Middle rows */
    for (i = 1u; i < h - 1u; i++) {
        buf[0] = '|';
        demo_fix_puts(x, (uint8_t)(y + i), buf, pal);
        demo_fix_puts((uint8_t)(x + w - 1u), (uint8_t)(y + i), buf, pal);
    }

    /* Bottom edge */
    buf[0] = '+'; demo_fix_puts(x, (uint8_t)(y + h - 1u), buf, pal);
    demo_render_hbar((uint8_t)(x + 1u), (uint8_t)(y + h - 1u), (uint8_t)(w - 2u), pal);
    buf[0] = '+'; demo_fix_puts((uint8_t)(x + w - 1u), (uint8_t)(y + h - 1u), buf, pal);
}

void NEOGEO_USER demo_render_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pal)
{
    uint8_t row, col;
    char buf[2];
    buf[0] = '#';
    buf[1] = '\0';
    for (row = 0u; row < h && (y + row) < 28u; row++) {
        for (col = 0u; col < w && (x + col) < 40u; col++) {
            demo_fix_puts((uint8_t)(x + col), (uint8_t)(y + row), buf, pal);
        }
    }
}

/* ------------------------------------------------------------------ */
/*  demo_render_run — two sub-scenes                                    */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_render_run(void)
{
    uint16_t t;
    uint8_t  sweep_x;
    uint8_t  rot;
    uint8_t  wave_col;

    /* --- Sub-scene 1: basic primitives --- */
    demo_clear_scene();
    clearFix();
    demo_fix_puts(2u, 0u, "FIX LAYER SOFTWARE 2D RENDERING", 2u);
    demo_fix_puts(2u, 1u, "LINES  BOXES  BARS  CPU BLITTER", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    demo_fix_puts(2u, 3u, "HBAR:", 0u);
    demo_render_hbar(8u, 3u, 20u, 1u);
    demo_fix_puts(2u, 5u, "VBAR:", 0u);
    demo_render_vbar(8u, 4u, 8u, 2u);

    demo_fix_puts(2u, 13u, "BOX:", 0u);
    demo_render_box(7u, 12u, 10u, 5u, 1u);

    demo_fix_puts(20u, 13u, "FILL:", 0u);
    demo_render_fill(26u, 12u, 8u, 5u, 2u);

    demo_fix_puts(2u, 20u, "PRIMITIVE  LABEL", 0u);
    demo_fix_puts(2u, 21u, "HBAR:  -HORIZONTAL LINE", 0u);
    demo_fix_puts(2u, 22u, "VBAR:  |VERTICAL LINE", 0u);
    demo_fix_puts(2u, 23u, "BOX:   +--+BORDER", 0u);
    demo_fix_puts(2u, 24u, "FILL:  ##FILLED RECT", 0u);

    if (demo_wait(240u)) goto done;

    /* --- Sub-scene 2: animated primitives --- */
    clearFix();
    demo_fix_puts(2u, 0u, "ANIMATED FIX RENDERING", 2u);
    demo_fix_puts(2u, 1u, "SWEEP + ROTATING BORDER + WAVEFORM", 1u);
    demo_fix_puts(2u, 27u, "A: NEXT", 0u);

    sweep_x = 2u;
    rot = 0u;

    for (t = 0u; t < 300u; t++) {
        /* Clear animation rows */
        demo_fix_puts(1u, 4u,  "                                    ", 0u);
        demo_fix_puts(1u, 10u, "                                    ", 0u);
        demo_fix_puts(1u, 11u, "                                    ", 0u);
        demo_fix_puts(1u, 12u, "                                    ", 0u);
        demo_fix_puts(1u, 14u, "                                    ", 0u);
        demo_fix_puts(1u, 15u, "                                    ", 0u);
        demo_fix_puts(1u, 16u, "                                    ", 0u);
        demo_fix_puts(1u, 17u, "                                    ", 0u);
        demo_fix_puts(1u, 18u, "                                    ", 0u);
        demo_fix_puts(1u, 19u, "                                    ", 0u);
        demo_fix_puts(1u, 21u, "                                    ", 0u);
        demo_fix_puts(1u, 22u, "                                    ", 0u);
        demo_fix_puts(1u, 23u, "                                    ", 0u);
        demo_fix_puts(1u, 24u, "                                    ", 0u);
        demo_fix_puts(1u, 25u, "                                    ", 0u);

        /* Sweeping fill rectangle */
        demo_fix_puts(2u, 3u, "SWEEP:", 0u);
        if (sweep_x < 34u) {
            demo_render_fill(sweep_x, 4u, 4u, 1u, 1u);
        }
        sweep_x = (uint8_t)(sweep_x + 1u);
        if (sweep_x > 34u) sweep_x = 2u;

        /* Rotating box pattern */
        demo_fix_puts(2u, 9u, "ROTATE:", 0u);
        {
            uint8_t pal_a = (uint8_t)((rot & 1u) ? 1u : 2u);
            uint8_t pal_b = (uint8_t)((rot & 1u) ? 2u : 1u);
            demo_render_box(10u, 10u, 8u, 5u, pal_a);
            demo_render_box(20u, 10u, 8u, 5u, pal_b);
            demo_render_box(30u, 10u, 8u, 5u, pal_a);
        }
        rot = (uint8_t)((rot + 1u) & 0x1Fu);

        /* Sine-approximated waveform using bars */
        demo_fix_puts(2u, 13u, "WAVE:", 0u);
        {
            static const int8_t wave_tab[16] = {
                0, 2, 3, 4, 4, 4, 3, 2, 0, -2, -3, -4, -4, -4, -3, -2
            };
            uint8_t phase = (uint8_t)((t >> 1) & 0x0Fu);
            for (wave_col = 0u; wave_col < 32u; wave_col++) {
                uint8_t idx = (uint8_t)((wave_col + phase) & 0x0Fu);
                int8_t  off = wave_tab[idx];
                uint8_t wy  = (uint8_t)(18 + off);
                if (wy >= 14u && wy <= 25u) {
                    char ch[2];
                    ch[0] = '*';
                    ch[1] = '\0';
                    demo_fix_puts((uint8_t)(4u + wave_col), wy, ch,
                                  (uint8_t)((off >= 0) ? 2u : 1u));
                }
            }
        }

        if (demo_frame()) break;
    }

done:
    demo_clear_scene();
}

#ifdef __cplusplus
}  /* extern "C" */
#endif
