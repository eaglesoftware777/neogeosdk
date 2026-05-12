/*
 * eyecatcher.c — stable MVS/AES eyecatcher preview.
 *
 * Eagle mascot frames are generated as screen IDs 79..92 in this art set.
 * Those generated functions mostly load palettes, so visible playback must draw
 * the corresponding C-ROM tile blocks through NGSpriteGroup.
 */

#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include "demo/demo_screen.h"
#include <stdint.h>

#pragma GCC push_options
#pragma GCC optimize ("O0")

void NEOGEO_USER showEyeCatcherMVS(void)
{
    uint16_t t;

    demo_clear_scene();
    demo_scene_caption("EYECATCHER ANIMATION", "EAGLE MASCOT FRAMES 79-92", "STABLE C-ROM SPRITEGROUP DRAW");

    for (t = 0; t < 14u * 22u; t++) {
        uint8_t frame = (uint8_t)(79u + ((t / 22u) % 14u));
        demo_draw_sprite_screen(frame, 1, 76, 18, 16, 16, 0xFF, 0xFF);
        if (demo_wait_frames_or_a(1)) break;
    }

    demo_clear_scene();
}

#pragma GCC pop_options
