#include "demo_3d.h"
#include "demo_screen.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_defs.h"
#include <stdint.h>

void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER clearSprs(void);
void NEOGEO_USER fixtext_out(uint16_t x, uint16_t y, char *mess, short pal);
void NEOGEO_USER vram_SCB234(uint16_t SCBADDR, uint16_t SCB234);
void NEOGEO_USER vram_sprite(uint16_t vram_start, uint16_t vram_inc, uint16_t vram_offset, uint16_t *SCB1_1, uint16_t *SCB1_2, uint16_t tiles_number, uint16_t SCB2, uint16_t SCB3, uint16_t SCB4);
uint16_t NEOGEO_USER setSCB1_2(uint16_t pal_offset, uint16_t tile_offset, uint16_t bit3_autoanim, uint16_t bit2_autoanim, uint16_t vflip, uint16_t hflip);
uint16_t NEOGEO_USER setSCB2(uint16_t Xshrink, uint16_t Yshrink);
uint16_t NEOGEO_USER setSCB3(uint16_t Ypos, uint16_t sticky_flag, uint16_t height_factor);
uint16_t NEOGEO_USER setSCB4(uint16_t Xpos);

/* Pseudo-3D floor effect — Mode-7 style using sprite Y-scale perspective.
 *
 * NG_P3D_BANDS horizontal scan bands are stacked from near (screen bottom)
 * to the horizon (screen centre).  Each band is 1 tile tall; its vertical
 * shrink register (yr) increases with distance so near bands appear taller
 * and far bands appear very thin — creating foreshortening.  Horizontal
 * width is also proportional to depth so the floor tapers toward the horizon.
 * A scroll counter increments each frame to fake forward motion.
 *
 * Uses sprite slots 0..NG_P3D_BANDS*NG_P3D_STRIPS-1.  Call only when there
 * are no engine characters active (standalone demo scene).
 */
#define NG_P3D_BANDS    10
#define NG_P3D_STRIPS   16
#define NG_P3D_SLOT0    0
#define NG_P3D_NEAR_Y   220
#define NG_P3D_FAR_Y    112

void NEOGEO_USER showPseudo3DLoop(void) {
    uint16_t frame;
    uint16_t duration = (uint16_t)NG_MS_TO_FRAMES(8000);
    uint8_t  band;
    uint8_t  strip;
    uint16_t scb2, scb3, scb4;
    uint16_t tiles[1];
    uint16_t attrs[1];

    demo_clear_scene();
    soundSceneReset();
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);
    demo_scene_caption("NEOGEO FAKE 3D", "Z-ROAD BY SPRITE SHRINK", "2D STRIPS CREATE DEPTH");
    playSFX(SOUND_SFX_TITLE_GONG);

    for (frame = 0; frame < duration; frame++) {
        uint8_t scroll = (uint8_t)(frame >> 1);

        for (band = 0; band < NG_P3D_BANDS; band++) {
            /* depth: 0 = far (horizon), NG_P3D_BANDS-1 = near (bottom) */
            uint8_t depth = (uint8_t)(NG_P3D_BANDS - 1u - band);

            /* Y-scale grows with depth: far=0x08 (very thin), near=0xFF (full) */
            uint8_t yscale = (uint8_t)(0x08u + (uint16_t)depth * (0xFF - 0x08) / (NG_P3D_BANDS - 1));

            /* Screen Y: far bands cluster near horizon, near at bottom */
            int16_t screen_y = (int16_t)(NG_P3D_FAR_Y
                + (int32_t)(NG_P3D_NEAR_Y - NG_P3D_FAR_Y) * band / (NG_P3D_BANDS - 1));

            /* Width: tapers toward horizon */
            int16_t band_w = (int16_t)(16
                + (int32_t)(NG_P3D_STRIPS * 16 - 16) * depth / (NG_P3D_BANDS - 1));
            int16_t start_x = (int16_t)((320 - band_w) / 2);
            uint8_t n_strips = (uint8_t)((band_w + 15) / 16);
            if (n_strips > NG_P3D_STRIPS) n_strips = NG_P3D_STRIPS;

            /* Palette: cycle through stage palettes for depth colour bands */
            uint8_t pal = (uint8_t)(NG_PAL_STAGE_BASE + (depth & 3u));

            for (strip = 0; strip < n_strips; strip++) {
                uint16_t slot = (uint16_t)(NG_P3D_SLOT0 + band * NG_P3D_STRIPS + strip);

                /* Tile index scrolls to simulate forward motion; wrap in 256 */
                tiles[0] = (uint16_t)((scroll + strip + band * 2u) & 0xFFu);
                attrs[0] = setSCB1_2(pal, 0, 0, 0, 0, 0);

                scb2 = setSCB2(0xFu, yscale);
                if (strip == 0u) {
                    scb3 = setSCB3((uint16_t)(496 - (int)screen_y), 0, 1);
                    scb4 = setSCB4((uint16_t)start_x);
                } else {
                    scb3 = 0x0040u; /* sticky chain bit only; driver owns height */
                    scb4 = setSCB4((uint16_t)(start_x + strip * 16));
                }
                vram_sprite((uint16_t)((uint32_t)slot * 64u), 1u, slot,
                            tiles, attrs, 1u, scb2, scb3, scb4);
            }

            /* Hide unused strips in this band */
            for (strip = n_strips; strip < NG_P3D_STRIPS; strip++) {
                uint16_t slot = (uint16_t)(NG_P3D_SLOT0 + band * NG_P3D_STRIPS + strip);
                vram_SCB234((uint16_t)(SCB3_ADDR + slot), 0u);
            }
        }

        waitVbl();
        if (demo_advance_requested()) break;
    }

    /* Clear all 3D band sprites */
    for (band = 0; band < NG_P3D_BANDS; band++) {
        for (strip = 0; strip < NG_P3D_STRIPS; strip++) {
            uint16_t slot = (uint16_t)(NG_P3D_SLOT0 + band * NG_P3D_STRIPS + strip);
            vram_SCB234((uint16_t)(SCB3_ADDR + slot), 0u);
        }
    }
    demo_clear_scene();
}


/* ---- 3D Software Raycaster (DDA) -----------------------------------------
 * Camera navigates an 8x8 maze using DDA ray casting.
 * Wall textures use eyecatcher NPC sprite tiles (screens 93-99).
 * 20 columns × 16px = 320px, fisheye-corrected, full-screen.
 * --------------------------------------------------------------------------*/
#pragma GCC push_options
#pragma GCC optimize ("O0")

#define S3D_MAP_W    8
#define S3D_MAP_H    8
#define S3D_FP       6       /* fixed-point bits: 1.0 = 64 */
#define S3D_ONE      64
#define S3D_COLS     20      /* 20 strips × 16px = 320px full screen */
#define S3D_TILES    14      /* tiles per column sprite = 14×16 = 224px max */
#define S3D_MAXH     224
#define S3D_SCRCY    112     /* screen centre Y */
#define S3D_PROJ     2048    /* projection constant → wall_h = PROJ/dist */
#define S3D_FOV      43      /* ≈60° in 256-unit circle space */

static const int8_t s3d_sintab[256] = {
       0,   2,   3,   5,   6,   8,   9,  11,  12,  14,  15,  17,  18,  20,  21,  23,
      24,  26,  27,  28,  30,  31,  32,  34,  35,  36,  38,  39,  40,  41,  42,  43,
      45,  46,  47,  48,  49,  50,  51,  52,  52,  53,  54,  55,  56,  56,  57,  58,
      58,  59,  59,  60,  60,  61,  61,  61,  62,  62,  62,  63,  63,  63,  63,  63,
      63,  63,  63,  63,  63,  63,  62,  62,  62,  61,  61,  61,  60,  60,  59,  59,
      58,  58,  57,  56,  56,  55,  54,  53,  52,  52,  51,  50,  49,  48,  47,  46,
      45,  43,  42,  41,  40,  39,  38,  36,  35,  34,  32,  31,  30,  28,  27,  26,
      24,  23,  21,  20,  18,  17,  15,  14,  12,  11,   9,   8,   6,   5,   3,   2,
       0,  -2,  -3,  -5,  -6,  -8,  -9, -11, -12, -14, -15, -17, -18, -20, -21, -23,
     -24, -26, -27, -28, -30, -31, -32, -34, -35, -36, -38, -39, -40, -41, -42, -43,
     -45, -46, -47, -48, -49, -50, -51, -52, -52, -53, -54, -55, -56, -56, -57, -58,
     -58, -59, -59, -60, -60, -61, -61, -61, -62, -62, -62, -63, -63, -63, -63, -63,
     -63, -63, -63, -63, -63, -63, -62, -62, -62, -61, -61, -61, -60, -60, -59, -59,
     -58, -58, -57, -56, -56, -55, -54, -53, -52, -52, -51, -50, -49, -48, -47, -46,
     -45, -43, -42, -41, -40, -39, -38, -36, -35, -34, -32, -31, -30, -28, -27, -26,
     -24, -23, -21, -20, -18, -17, -15, -14, -12, -11,  -9,  -8,  -6,  -5,  -3,  -2
};
#define S3D_SIN(a)  s3d_sintab[(uint8_t)(a)]
#define S3D_COS(a)  s3d_sintab[(uint8_t)((uint8_t)(a) + 64u)]

static const uint8_t s3d_map[S3D_MAP_H][S3D_MAP_W] = {
    {1,1,1,1,1,1,1,1},
    {1,0,0,1,0,0,0,1},
    {1,0,1,1,0,1,0,1},
    {1,0,0,0,0,1,0,1},
    {1,0,1,0,0,0,0,1},
    {1,0,1,1,0,1,0,1},
    {1,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1},
};

/* DDA ray cast — returns approximate step count proportional to distance.
 * Dominant axis advances 8 FP units per step (1/8 cell); 72 steps max.
 * (cam_x, cam_y) in FP units where S3D_ONE = 1 map cell.
 * (rdx, rdy) from sin table: range -63..+63.  Returns 1..72. */
static uint8_t NEOGEO_USER s3d_cast(int16_t cam_x, int16_t cam_y,
                                     int8_t rdx, int8_t rdy)
{
    int8_t  ax = rdx, ay = rdy, sx, sy;
    int16_t rx = cam_x, ry = cam_y;
    uint8_t i;

    if (ax < 0) ax = -ax;
    if (ay < 0) ay = -ay;

    if (ax >= ay) {
        sx = (rdx >= 0) ? 8 : -8;
        sy = (rdx != 0) ? (int8_t)((int16_t)rdy * 8 / rdx) : 0;
    } else {
        sy = (rdy >= 0) ? 8 : -8;
        sx = (rdy != 0) ? (int8_t)((int16_t)rdx * 8 / rdy) : 0;
    }
    if (!sx && rdx) sx = (rdx > 0) ? 1 : -1;
    if (!sy && rdy) sy = (rdy > 0) ? 1 : -1;

    for (i = 1; i < 72; i++) {
        int8_t mx, my;
        rx += sx;
        ry += sy;
        mx = (int8_t)((uint16_t)rx >> S3D_FP);
        my = (int8_t)((uint16_t)ry >> S3D_FP);
        if ((uint8_t)mx >= S3D_MAP_W || (uint8_t)my >= S3D_MAP_H) return i;
        if (s3d_map[my][mx]) return i;
    }
    return 72;
}

void NEOGEO_USER show3DRaycaster(void)
{
    uint16_t s3d_tiles[S3D_TILES];
    uint16_t s3d_attrs[S3D_TILES];
    /* Camera starts in open cell (1.5, 1.5) facing right */
    int16_t  cam_x    = (int16_t)((1 << S3D_FP) + S3D_ONE / 2);
    int16_t  cam_y    = (int16_t)((1 << S3D_FP) + S3D_ONE / 2);
    uint8_t  cam_a    = 0;
    uint8_t  stuck    = 0;
    uint16_t frame;
    uint8_t  col, t;
    int8_t   fwd_dx, fwd_dy;
    uint8_t  fwd_dist;

    demo_clear_scene();
    soundSceneReset();
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);

    /* Preload palette banks off-screen. Never show these asset frames here. */
    showScreen93(-320, 1024, 0, 0, 0, 0x0000, 400); waitVbl();
    showScreen94(-320, 1024, 0, 0, 0, 0x0000, 400); waitVbl();
    showScreen95(-320, 1024, 0, 0, 0, 0x0000, 400); waitVbl();
    showScreen96(-320, 1024, 0, 0, 0, 0x0000, 400); waitVbl();
    showScreen97(-320, 1024, 0, 0, 0, 0x0000, 400); waitVbl();
    showScreen98(-320, 1024, 0, 0, 0, 0x0000, 400); waitVbl();
    showScreen99(-320, 1024, 0, 0, 0, 0x0000, 400); waitVbl();
    demo_clear_all_sprites();
    for (col = 0; col < S3D_COLS; col++)
        vram_SCB234((uint16_t)(SCB3_ADDR + col), 0u);

    demo_scene_caption("SOFTWARE 3D RAYCASTER", "DDA MAZE USING SPRITE COLUMNS", "2D C-ROM TILES AS WALLS");
    playSFX(SOUND_SFX_STRING_PHRASE);

    for (frame = 0; frame < (uint16_t)NG_MS_TO_FRAMES(12000); frame++) {
        /* Rotation: 1 angle unit per 2 frames */
        if (frame & 1u) cam_a++;

        /* Camera forward movement */
        fwd_dx = (int8_t)S3D_COS(cam_a);
        fwd_dy = (int8_t)S3D_SIN(cam_a);
        fwd_dist = s3d_cast(cam_x, cam_y, fwd_dx, fwd_dy);

        if (fwd_dist > 8u) {
            /* Path clear — advance faster than before */
            cam_x = (int16_t)(cam_x + (int16_t)((int16_t)fwd_dx >> 4));
            cam_y = (int16_t)(cam_y + (int16_t)((int16_t)fwd_dy >> 4));
            stuck = 0;
        } else {
            /* Near wall — turn right ~22.5° */
            cam_a = (uint8_t)(cam_a + 16u);
            if (++stuck > 40u) {
                cam_x = (int16_t)((3 << S3D_FP) + S3D_ONE / 2);
                cam_y = (int16_t)((3 << S3D_FP) + S3D_ONE / 2);
                stuck = 0;
            }
        }

        for (col = 0; col < S3D_COLS; col++) {
            int16_t  fov_off = (int16_t)col * (int16_t)S3D_FOV / (int16_t)S3D_COLS
                               - (int16_t)(S3D_FOV / 2);
            uint8_t  ray_a   = (uint8_t)((int16_t)cam_a + fov_off);
            int8_t   rdx     = (int8_t)S3D_COS(ray_a);
            int8_t   rdy     = (int8_t)S3D_SIN(ray_a);
            uint8_t  dist    = s3d_cast(cam_x, cam_y, rdx, rdy);

            /* Fisheye correction */
            uint8_t  abs_fov  = (fov_off < 0) ? (uint8_t)(-(int16_t)fov_off) : (uint8_t)fov_off;
            uint8_t  cos_fov  = (uint8_t)S3D_COS(abs_fov);
            uint16_t perp_d   = (uint16_t)((uint16_t)dist * cos_fov / 63u);
            if (perp_d < 1u) perp_d = 1u;

            uint16_t wall_h   = (uint16_t)(S3D_PROJ / perp_d);
            if (wall_h > S3D_MAXH) wall_h = S3D_MAXH;
            if (wall_h < 4u)       wall_h = 4u;

            uint8_t  yscale   = (uint8_t)((wall_h * 255u) / S3D_MAXH);
            uint16_t vram_y   = (uint16_t)(384u + wall_h / 2u);

            /* Each column maps to one of 7 eyecatcher NPC sprite screens (93-99) */
            uint8_t  ec_idx      = (uint8_t)((uint16_t)col * 7u / (uint16_t)S3D_COLS);
            uint16_t ec_tile_base = DEMO_SCREEN_TILE((uint16_t)(93u + ec_idx));
            uint8_t  ec_pal      = (uint8_t)DEMO_SCREEN_PALETTE((uint16_t)(93u + ec_idx));

            for (t = 0; t < S3D_TILES; t++) {
                s3d_tiles[t] = ec_tile_base + (uint16_t)t;
                s3d_attrs[t] = setSCB1_2(ec_pal, 0, 0, 0, 0, 0);
            }

            vram_sprite(
                (uint16_t)(col * 64u), 1u, (uint16_t)col,
                s3d_tiles, s3d_attrs, (uint8_t)S3D_TILES,
                setSCB2(0xFFu, yscale),
                setSCB3(vram_y, 0u, (uint16_t)S3D_TILES),
                setSCB4((uint16_t)(col * 16u))
            );
        }
        waitVbl();
        if (demo_advance_requested()) break;
    }

    for (col = 0; col < S3D_COLS; col++)
        vram_SCB234((uint16_t)(SCB3_ADDR + col), 0u);
    demo_clear_scene();
}

#pragma GCC pop_options


void NEOGEO_USER demo_3d_showcase(void)
{
    showPseudo3DLoop();
    demo_clear_scene();
    show3DRaycaster();
    demo_clear_scene();
}
