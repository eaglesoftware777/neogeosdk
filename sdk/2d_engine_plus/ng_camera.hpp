/*
 * ng_camera.h — Camera system for the NeoGeo 2D engine (Stage 5)
 *
 * Provides smooth-follow, look-ahead, dead zone, camera shake,
 * stage border clamping, and cinematic pan support.
 *
 * All positions are in 16.16 fixed-point (NGFixed) internally for
 * sub-pixel smoothness.  Outputs are integer pixel coordinates
 * that are fed to ng_level_set_scroll() and ng_bg_draw().
 *
 * NeoGeo screen dimensions:
 *   Full screen:  320 x 224 pixels
 *   Safe area:    304 x 208 pixels (overscan-safe)
 */

#ifndef NG_CAMERA_HPP
#define NG_CAMERA_HPP

#include "ng_defs.hpp"
#include "ng_fixed.hpp"

#ifdef __cplusplus
extern "C" {
#endif


/* Screen dimensions */
#define NG_SCREEN_W   320
#define NG_SCREEN_H   224

/* Camera mode flags */
#define NG_CAM_FREE        0x00   /* no follow, pure manual control */
#define NG_CAM_FOLLOW      0x01   /* follow a target with smoothing */
#define NG_CAM_CINEMATIC   0x02   /* lerp to a fixed destination */

/*
 * NGCamera — all state in one 52-byte struct.
 * Keep it value-type (no pointers to external objects) for safety.
 */
typedef struct {
    /* Current camera position in 16.16 fixed-point */
    NGFixed x_fp;
    NGFixed y_fp;

    /* Camera output (integer pixels, clamped to world bounds) */
    int16_t x;
    int16_t y;

    /* World boundary clamps (set from ng_level world bounds) */
    int16_t bound_left;
    int16_t bound_top;
    int16_t bound_right;    /* max scroll_x = world_right - screen_w */
    int16_t bound_bottom;

    /* Smooth follow speed (0x00FF = no smoothing, 0x0010 = gentle) */
    uint8_t follow_speed;   /* 1..255, applied as (delta * speed) >> 8 */

    /* Look-ahead: camera shifts forward in the direction of player motion */
    int16_t look_ahead_x;   /* pixels to look ahead, applied gradually */
    int16_t look_ahead_y;
    uint8_t look_ahead_rate; /* how fast look-ahead accumulates (1..8) */

    /* Dead zone: target must leave this box before camera moves */
    uint8_t dead_zone_x;    /* half-width  in pixels */
    uint8_t dead_zone_y;    /* half-height in pixels */

    /* Camera shake */
    uint8_t shake_frames;   /* frames remaining in shake */
    uint8_t shake_amp;      /* initial amplitude in pixels */

    /* Cinematic pan */
    NGFixed pan_dest_x;     /* destination in 16.16 */
    NGFixed pan_dest_y;
    uint8_t pan_speed;      /* lerp step per frame (1..255) */
    uint8_t mode;           /* NG_CAM_* flags */

    /* Shake output offset added to x/y before clamping */
    int8_t  shake_offset_x;
    int8_t  shake_offset_y;
} NGCamera;

/* Initialise camera to default (follow mode, centred on world, no shake). */
void NEOGEO_USER ng_camera_init(NGCamera *cam);

/* Set world bounds from NGLevelState (call after ng_level_set_world_bounds). */
void NEOGEO_USER ng_camera_set_bounds(NGCamera *cam,
                                      int16_t left, int16_t top,
                                      int16_t right, int16_t bottom);

/* Set the smooth follow speed (1 = very slow, 255 = instant snap). */
void NEOGEO_USER ng_camera_set_follow_speed(NGCamera *cam, uint8_t speed);

/* Set dead zone (pixels target can move without camera tracking). */
void NEOGEO_USER ng_camera_set_dead_zone(NGCamera *cam, uint8_t half_w, uint8_t half_h);

/* Set look-ahead parameters. */
void NEOGEO_USER ng_camera_set_look_ahead(NGCamera *cam, int16_t max_x, int16_t max_y, uint8_t rate);

/* Snap camera to pixel-exact position (no lerp). */
void NEOGEO_USER ng_camera_snap(NGCamera *cam, int16_t x, int16_t y);

/* Start camera shake.  amp in pixels, frames = duration. */
void NEOGEO_USER ng_camera_shake(NGCamera *cam, uint8_t amp, uint8_t frames);

/* Start cinematic pan to (dest_x, dest_y) at given speed. */
void NEOGEO_USER ng_camera_pan_to(NGCamera *cam, int16_t dest_x, int16_t dest_y, uint8_t speed);

/*
 * Per-frame update.  Call once per frame after game logic.
 *
 * target_x / target_y:  pixel position of the entity to follow.
 *                        Pass cam->x / cam->y to keep current position.
 * target_vx:            horizontal velocity hint for look-ahead (pixels/frame).
 *
 * After this call, cam->x and cam->y hold the final integer pixel offset
 * to pass to ng_level_set_scroll() and ng_bg_draw().
 */
void NEOGEO_USER ng_camera_update(NGCamera *cam,
                                  int16_t target_x, int16_t target_y,
                                  int16_t target_vx);

/*
 * Convenience: update camera + apply to level scroll + background in one call.
 * Requires ng_level already initialised.
 */
void NEOGEO_USER ng_camera_apply(NGCamera *cam,
                                  int16_t target_x, int16_t target_y,
                                  int16_t target_vx);

/* World-to-screen coordinate helpers (macros to avoid inline function issues). */
#define ng_camera_world_to_screen_x(cam, world_x) ((int16_t)((world_x) - (cam)->x))
#define ng_camera_world_to_screen_y(cam, world_y) ((int16_t)((world_y) - (cam)->y))


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif