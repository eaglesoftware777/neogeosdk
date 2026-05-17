#ifndef NG_DEFS_HPP
#define NG_DEFS_HPP

#include <stdint.h>
#include "ng_hw.hpp"

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

/* Boolean */
#define NG_TRUE  1
#define NG_FALSE 0

/* Hardware timing */
#ifdef __cplusplus
static constexpr int NG_FRAME_RATE     = 60;
static constexpr int NG_FRAMES_PER_SEC = 60;
#define NG_MS_TO_FRAMES(ms) (static_cast<int>((ms) * NG_FRAME_RATE / 1000))
#else
#define NG_FRAME_RATE       60
#define NG_FRAMES_PER_SEC   NG_FRAME_RATE
#define NG_MS_TO_FRAMES(ms) ((ms) * NG_FRAME_RATE / 1000)
#endif

/* Logical visible area */
#ifdef __cplusplus
static constexpr int16_t NG_SCREEN_WIDTH  = 320;
static constexpr int16_t NG_SCREEN_HEIGHT = 224;
#else
#define NG_SCREEN_WIDTH  320
#define NG_SCREEN_HEIGHT 224
#endif

/* Conservative sprite culling range */
#define NG_SPRITE_CULL_LEFT     -64
#define NG_SPRITE_CULL_RIGHT    384
#define NG_SPRITE_CULL_TOP      -128
#define NG_SPRITE_CULL_BOTTOM   640

/* Render priority bands — use enum in C++, macros in C */
#ifdef __cplusplus
enum NGRenderBand : uint8_t {
    NG_RENDER_BAND_BACK   = 0,
    NG_RENDER_BAND_NPC    = 1,
    NG_RENDER_BAND_ENEMY  = 2,
    NG_RENDER_BAND_PLAYER = 3,
    NG_RENDER_BAND_FX     = 4,
    NG_RENDER_BAND_FRONT  = 5,
};
#else
#define NG_RENDER_BAND_BACK      0
#define NG_RENDER_BAND_NPC       1
#define NG_RENDER_BAND_ENEMY     2
#define NG_RENDER_BAND_PLAYER    3
#define NG_RENDER_BAND_FX        4
#define NG_RENDER_BAND_FRONT     5
#endif

/* Fixed-point math (8.8 format, shift = 8) */
#define NG_FP_SHIFT   8
#define NG_FP_ONE     (1 << NG_FP_SHIFT)
#define NG_TO_FP(x)   (static_cast<int32_t>(x) << NG_FP_SHIFT)
#define NG_FROM_FP(x) (static_cast<int16_t>((x) >> NG_FP_SHIFT))
#define NG_FP_MUL(a,b) ((static_cast<int32_t>(a) * (b)) >> NG_FP_SHIFT)
#define NG_FP_FROM_FRAC(num,den) (static_cast<int32_t>((static_cast<int32_t>(num) * NG_FP_ONE) / (den)))

/* Utility math */
#define NG_MIN(a,b)        ((a) < (b) ? (a) : (b))
#define NG_MAX(a,b)        ((a) > (b) ? (a) : (b))
#define NG_CLAMP(v,lo,hi)  NG_MIN(NG_MAX((v),(lo)),(hi))
#define NG_ABS(x)          ((x) < 0 ? -(x) : (x))
#define NG_SIGN(x)         ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
#define NG_LERP(a,b,t)     ((a) + (((b)-(a)) * (t) >> 8))

/* Compile-time array length — std::size equivalent without header dependency */
#ifdef __cplusplus
template<typename T, int N>
constexpr int NG_ARRAY_SIZE_impl(T (&)[N]) { return N; }
#define NG_ARRAY_SIZE(a) NG_ARRAY_SIZE_impl(a)
#else
#define NG_ARRAY_SIZE(a)   ((int)(sizeof(a)/sizeof((a)[0])))
#endif

/* Suppress unused-variable warnings for unused parameters */
#define NG_UNUSED(x)       ((void)(x))

/* Pool size constants */
#ifdef __cplusplus
static constexpr int NG_MAX_CHARS              = 64;
static constexpr int NG_MAX_CHAR_KINDS         = 128;
static constexpr int NG_MAX_ACTIONS            = 256;
static constexpr int NG_MAX_GAME_EVENTS        = 32;
static constexpr int NG_MAX_TIMERS             = 64;
static constexpr int NG_MAX_PROGRESS           = 64;
static constexpr int NG_MAX_STATUS             = 256;
static constexpr int NG_PROP_GROUPS            = 16;
static constexpr int NG_PROP_COUNT             = 32;
static constexpr int NG_MAX_BORDER_CONSTRAINTS = 128;
static constexpr int NG_MAX_NPCS               = 32;
static constexpr int NG_MAX_SOLIDS             = 64;
static constexpr int NG_FIX_WIDTH              = 40;
static constexpr int NG_FIX_HEIGHT             = 32;
static constexpr int NG_SPRITE_MAX_STRIPS        = 32;
static constexpr int NG_SPRITE_MAX_HEIGHT_TILES  = 32;
static constexpr uint8_t NG_SPRITE_FULL_XSCALE   = 0xff;
static constexpr uint8_t NG_SPRITE_FULL_YSCALE   = 0xff;
#else
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
#define NG_MAX_NPCS               32
#define NG_MAX_SOLIDS             64
#define NG_FIX_WIDTH              40
#define NG_FIX_HEIGHT             32
#define NG_SPRITE_MAX_STRIPS        32
#define NG_SPRITE_MAX_HEIGHT_TILES  32
#define NG_SPRITE_FULL_XSCALE       0xff
#define NG_SPRITE_FULL_YSCALE       0xff
#endif

/* Palette slot base indices */
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

#ifdef __cplusplus
extern "C" {
#endif

uint8_t NEOGEO_USER ng_rect_hit(NGRect a, NGRect b);
uint16_t NEOGEO_USER ng_abs16(int16_t v);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif