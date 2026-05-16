/*
 * ng_camera.c — Camera system implementation (Stage 5)
 *
 * Performance notes (68000):
 *   - Smooth follow uses fixed-point lerp: (delta * speed) >> 8.
 *     One 32-bit multiply per axis per frame: acceptable.
 *   - Shake offset uses a pre-baked 8-entry pattern (no random, no divide).
 *   - Look-ahead accumulates and decays without division.
 *   - All clamping uses if-comparisons, no modulo.
 */

#include "ng_camera.h"
#include "ng_level.h"
#include "neogeo.h"

/*
 * Shake offset pattern — 8 frames of alternating offset.
 * Amplitude is multiplied by the decreasing shake_frames counter.
 */
static const int8_t ng_shake_pattern_x[8] = {  1, -1,  2, -2,  1, -1,  0,  0 };
static const int8_t ng_shake_pattern_y[8] = {  0,  1, -1,  0,  1, -1,  1,  0 };

void NEOGEO_USER ng_camera_init(NGCamera *cam)
{
    if (!cam) return;

    cam->x_fp          = 0;
    cam->y_fp          = 0;
    cam->x             = 0;
    cam->y             = 0;
    cam->bound_left    = 0;
    cam->bound_top     = 0;
    cam->bound_right   = 0;
    cam->bound_bottom  = 0;
    cam->follow_speed  = 0x40;  /* ~25% per frame, smooth feel */
    cam->look_ahead_x  = 0;
    cam->look_ahead_y  = 0;
    cam->look_ahead_rate = 2;
    cam->dead_zone_x   = 32;
    cam->dead_zone_y   = 24;
    cam->shake_frames  = 0;
    cam->shake_amp     = 0;
    cam->pan_dest_x    = 0;
    cam->pan_dest_y    = 0;
    cam->pan_speed     = 8;
    cam->mode          = NG_CAM_FOLLOW;
    cam->shake_offset_x = 0;
    cam->shake_offset_y = 0;
}

void NEOGEO_USER ng_camera_set_bounds(NGCamera *cam,
                                      int16_t left, int16_t top,
                                      int16_t right, int16_t bottom)
{
    if (!cam) return;

    /* right/bottom are world extents; subtract screen size to get max scroll */
    cam->bound_left   = left;
    cam->bound_top    = top;
    cam->bound_right  = (int16_t)(right  - NG_SCREEN_W + 1);
    cam->bound_bottom = (int16_t)(bottom - NG_SCREEN_H + 1);

    if (cam->bound_right  < left) cam->bound_right  = left;
    if (cam->bound_bottom < top)  cam->bound_bottom = top;
}

void NEOGEO_USER ng_camera_set_follow_speed(NGCamera *cam, uint8_t speed)
{
    if (cam) cam->follow_speed = speed ? speed : 1;
}

void NEOGEO_USER ng_camera_set_dead_zone(NGCamera *cam, uint8_t half_w, uint8_t half_h)
{
    if (!cam) return;
    cam->dead_zone_x = half_w;
    cam->dead_zone_y = half_h;
}

void NEOGEO_USER ng_camera_set_look_ahead(NGCamera *cam, int16_t max_x, int16_t max_y, uint8_t rate)
{
    if (!cam) return;
    cam->look_ahead_x  = max_x;
    cam->look_ahead_y  = max_y;
    cam->look_ahead_rate = rate ? rate : 1;
}

void NEOGEO_USER ng_camera_snap(NGCamera *cam, int16_t x, int16_t y)
{
    if (!cam) return;

    cam->x    = x;
    cam->y    = y;
    cam->x_fp = NGFX_FROM_INT(x);
    cam->y_fp = NGFX_FROM_INT(y);
}

void NEOGEO_USER ng_camera_shake(NGCamera *cam, uint8_t amp, uint8_t frames)
{
    if (!cam) return;

    cam->shake_amp    = amp;
    cam->shake_frames = frames;
}

void NEOGEO_USER ng_camera_pan_to(NGCamera *cam, int16_t dest_x, int16_t dest_y, uint8_t speed)
{
    if (!cam) return;

    cam->pan_dest_x = NGFX_FROM_INT(dest_x);
    cam->pan_dest_y = NGFX_FROM_INT(dest_y);
    cam->pan_speed  = speed ? speed : 1;
    cam->mode       = NG_CAM_CINEMATIC;
}

/*
 * Clamp a fixed-point camera position to world bounds.
 * Returns clamped NGFixed.
 */
static NGFixed NEOGEO_USER ng_cam_clamp_x(const NGCamera *cam, NGFixed v)
{
    NGFixed lo = NGFX_FROM_INT(cam->bound_left);
    NGFixed hi = NGFX_FROM_INT(cam->bound_right);
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static NGFixed NEOGEO_USER ng_cam_clamp_y(const NGCamera *cam, NGFixed v)
{
    NGFixed lo = NGFX_FROM_INT(cam->bound_top);
    NGFixed hi = NGFX_FROM_INT(cam->bound_bottom);
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void NEOGEO_USER ng_camera_update(NGCamera *cam,
                                  int16_t target_x, int16_t target_y,
                                  int16_t target_vx)
{
    int16_t desired_x;
    int16_t desired_y;
    int32_t delta_x;
    int32_t delta_y;
    int32_t step_x;
    int32_t step_y;
    NGFixed new_fp_x;
    NGFixed new_fp_y;
    int8_t  sx;
    int8_t  sy;

    if (!cam) return;

    /*
     * --- Shake update (runs regardless of camera mode) ---
     * Shake pattern cycles every 8 frames; amplitude decreases by 1 each frame.
     */
    if (cam->shake_frames > 0) {
        uint8_t phase = (uint8_t)(cam->shake_frames & 7u);
        uint8_t amp   = cam->shake_amp;

        /* Reduce amplitude proportionally as frames count down */
        if (cam->shake_frames < cam->shake_amp)
            amp = cam->shake_frames;

        sx = (int8_t)(ng_shake_pattern_x[phase] * (int8_t)amp);
        sy = (int8_t)(ng_shake_pattern_y[phase] * (int8_t)amp);

        cam->shake_offset_x = sx;
        cam->shake_offset_y = sy;
        cam->shake_frames--;
    } else {
        cam->shake_offset_x = 0;
        cam->shake_offset_y = 0;
    }

    /* --- Cinematic pan mode --- */
    if (cam->mode == NG_CAM_CINEMATIC) {
        NGFixed dx_fp = NGFX_SUB(cam->pan_dest_x, cam->x_fp);
        NGFixed dy_fp = NGFX_SUB(cam->pan_dest_y, cam->y_fp);
        NGFixed step  = (NGFixed)((int32_t)cam->pan_speed << (NGFX_SHIFT - 4));

        if (NGFX_ABS(dx_fp) <= step && NGFX_ABS(dy_fp) <= step) {
            cam->x_fp  = cam->pan_dest_x;
            cam->y_fp  = cam->pan_dest_y;
            cam->mode  = NG_CAM_FOLLOW;
        } else {
            /* Move towards destination at pan_speed */
            cam->x_fp += (dx_fp > 0) ? step : -step;
            cam->y_fp += (dy_fp > 0) ? step : -step;
        }

        cam->x_fp = ng_cam_clamp_x(cam, cam->x_fp);
        cam->y_fp = ng_cam_clamp_y(cam, cam->y_fp);
        cam->x    = (int16_t)(NGFX_TO_INT(cam->x_fp) + cam->shake_offset_x);
        cam->y    = (int16_t)(NGFX_TO_INT(cam->y_fp) + cam->shake_offset_y);
        return;
    }

    /* --- Free mode: no follow --- */
    if (cam->mode == NG_CAM_FREE) {
        cam->x = (int16_t)(NGFX_TO_INT(cam->x_fp) + cam->shake_offset_x);
        cam->y = (int16_t)(NGFX_TO_INT(cam->y_fp) + cam->shake_offset_y);
        return;
    }

    /*
     * --- Follow mode ---
     *
     * Target camera centre is target_x - screen_half_w + look_ahead.
     * Dead zone: if target is within dead_zone of current screen centre,
     * do not move.
     */
    desired_x = (int16_t)(target_x - (NG_SCREEN_W / 2));
    desired_y = (int16_t)(target_y - (NG_SCREEN_H / 2));

    /*
     * Look-ahead: bias towards player velocity direction.
     * Accumulate gradually using the look_ahead_rate to avoid snapping.
     * No division: shift only.
     */
    if (target_vx > 0 && cam->look_ahead_x > 0) {
        desired_x = (int16_t)(desired_x + cam->look_ahead_x);
    } else if (target_vx < 0 && cam->look_ahead_x > 0) {
        desired_x = (int16_t)(desired_x - cam->look_ahead_x);
    }

    /*
     * Dead zone check: only follow if target is outside the dead zone.
     * The dead zone is relative to current camera screen position.
     */
    {
        int16_t screen_target_x = (int16_t)(target_x - cam->x);
        int16_t screen_target_y = (int16_t)(target_y - cam->y);
        int16_t screen_cx       = (int16_t)(NG_SCREEN_W / 2);
        int16_t screen_cy       = (int16_t)(NG_SCREEN_H / 2);
        int16_t off_x           = (int16_t)(screen_target_x - screen_cx);
        int16_t off_y           = (int16_t)(screen_target_y - screen_cy);

        if (off_x < 0) off_x = (int16_t)-off_x;
        if (off_y < 0) off_y = (int16_t)-off_y;

        if (off_x <= (int16_t)cam->dead_zone_x && off_y <= (int16_t)cam->dead_zone_y) {
            /* Target inside dead zone — hold camera */
            cam->x = (int16_t)(NGFX_TO_INT(cam->x_fp) + cam->shake_offset_x);
            cam->y = (int16_t)(NGFX_TO_INT(cam->y_fp) + cam->shake_offset_y);
            return;
        }
    }

    /*
     * Smooth follow: step = (desired - current) * follow_speed / 256.
     * One 32-bit multiply per axis.  At follow_speed=64, converges in ~16 frames.
     */
    new_fp_x = NGFX_FROM_INT(desired_x);
    new_fp_y = NGFX_FROM_INT(desired_y);

    delta_x  = (int32_t)(new_fp_x - cam->x_fp);
    delta_y  = (int32_t)(new_fp_y - cam->y_fp);

    /* (delta * speed) >> 8 — equivalent to dividing speed by 256 */
    step_x   = (delta_x * (int32_t)cam->follow_speed) >> 8;
    step_y   = (delta_y * (int32_t)cam->follow_speed) >> 8;

    /* Minimum 1 sub-pixel step to ensure the camera eventually converges */
    if (step_x == 0 && delta_x != 0) step_x = (delta_x > 0) ?  1 : -1;
    if (step_y == 0 && delta_y != 0) step_y = (delta_y > 0) ?  1 : -1;

    cam->x_fp = ng_cam_clamp_x(cam, (NGFixed)(cam->x_fp + step_x));
    cam->y_fp = ng_cam_clamp_y(cam, (NGFixed)(cam->y_fp + step_y));

    cam->x = (int16_t)(NGFX_TO_INT(cam->x_fp) + cam->shake_offset_x);
    cam->y = (int16_t)(NGFX_TO_INT(cam->y_fp) + cam->shake_offset_y);
}

void NEOGEO_USER ng_camera_apply(NGCamera *cam,
                                  int16_t target_x, int16_t target_y,
                                  int16_t target_vx)
{
    const NGLevelState *level;

    if (!cam) return;

    level = level_state();
    if (level) {
        ng_camera_set_bounds(cam,
                             level->world_left,
                             level->world_top,
                             level->world_right,
                             level->world_bottom);
    }

    ng_camera_update(cam, target_x, target_y, target_vx);

    ng_level_set_scroll(cam->x, cam->y);
}
