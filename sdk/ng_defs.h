#ifndef NG_DEFS_H
#define NG_DEFS_H

#include <stdint.h>
#include "macro.h"

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

#define NG_TRUE  1
#define NG_FALSE 0

#define NG_FRAME_RATE 60

#define NG_FP_SHIFT 8
#define NG_FP_ONE   (1 << NG_FP_SHIFT)
#define NG_TO_FP(x) ((int32_t)(x) << NG_FP_SHIFT)
#define NG_FROM_FP(x) ((int16_t)((x) >> NG_FP_SHIFT))

#define NG_MAX_CHARS              64
#define NG_MAX_CHAR_KINDS         128
#define NG_MAX_ACTIONS            256
#define NG_MAX_GAME_EVENTS        32
#define NG_MAX_TIMERS             64
#define NG_MAX_PROGRESS           64
#define NG_MAX_STATUS             256
#define NG_PROP_GROUPS            16
#define NG_PROP_COUNT             32
#define NG_MAX_BORDER_CONSTRAINTS 128

#define NG_SPRITE_MAX_STRIPS        32
#define NG_SPRITE_MAX_HEIGHT_TILES  32
#define NG_SPRITE_FULL_XSCALE       0xff
#define NG_SPRITE_FULL_YSCALE       0xff

#define NG_PAL_FIX_BASE             0x00
#define NG_PAL_UI_BASE              0x04
#define NG_PAL_STAGE_BASE           0x10
#define NG_PAL_PLAYER_BASE          0x20
#define NG_PAL_ENEMY_BASE           0x30
#define NG_PAL_FX_BASE              0x40
#define NG_PAL_BOSS_BASE            0x50
#define NG_PAL_CUTSCENE_BASE        0x60

typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} NGRect;

uint8_t ng_rect_hit(NGRect a, NGRect b);
uint16_t ng_abs16(int16_t v);

#endif
