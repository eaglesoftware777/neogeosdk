/*
 * ng_camera.cpp — Camera system implementation.
 *
 * Performance (68000):
 *   Smooth follow: (delta * speed) >> 8 — one 32-bit multiply per axis.
 *   Shake: pre-baked 8-entry pattern, no random, no divide.
 *   Cinematic pan: fixed step per frame.
 */

#include "ng_camera.hpp"
#include "ng_level.hpp"
#include "ng_hw.hpp"

static const int8_t ng_shake_x[8] = {  1, -1,  2, -2,  1, -1,  0,  0 };
static const int8_t ng_shake_y[8] = {  0,  1, -1,  0,  1, -1,  1,  0 };

/* --- NGCamera private helpers --- */

NGFixed NGCamera::clampX(NGFixed v) const
{
    NGFixed lo = NGFX_FROM_INT(bound_left);
    NGFixed hi = NGFX_FROM_INT(bound_right);
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

NGFixed NGCamera::clampY(NGFixed v) const
{
    NGFixed lo = NGFX_FROM_INT(bound_top);
    NGFixed hi = NGFX_FROM_INT(bound_bottom);
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* --- NGCamera member methods --- */

void NGCamera::init()
{
    x_fp          = 0; y_fp          = 0;
    x             = 0; y             = 0;
    bound_left    = 0; bound_top     = 0;
    bound_right   = 0; bound_bottom  = 0;
    follow_speed  = 0x40;
    look_ahead_x  = 0; look_ahead_y  = 0;
    look_ahead_rate = 2;
    dead_zone_x   = 32; dead_zone_y  = 24;
    shake_frames  = 0;  shake_amp    = 0;
    pan_dest_x    = 0;  pan_dest_y   = 0;
    pan_speed     = 8;
    mode          = NG_CAM_FOLLOW;
    shake_offset_x = 0; shake_offset_y = 0;
}

void NGCamera::setBounds(int16_t left, int16_t top, int16_t right, int16_t bottom)
{
    bound_left   = left;
    bound_top    = top;
    bound_right  = (int16_t)(right  - NG_SCREEN_W + 1);
    bound_bottom = (int16_t)(bottom - NG_SCREEN_H + 1);
    if (bound_right  < left) bound_right  = left;
    if (bound_bottom < top)  bound_bottom = top;
}

void NGCamera::setFollowSpeed(uint8_t speed)
{
    follow_speed = speed ? speed : 1;
}

void NGCamera::setDeadZone(uint8_t half_w, uint8_t half_h)
{
    dead_zone_x = half_w;
    dead_zone_y = half_h;
}

void NGCamera::setLookAhead(int16_t max_x, int16_t max_y, uint8_t rate)
{
    look_ahead_x   = max_x;
    look_ahead_y   = max_y;
    look_ahead_rate = rate ? rate : 1;
}

void NGCamera::snap(int16_t px, int16_t py)
{
    x    = px;  y    = py;
    x_fp = NGFX_FROM_INT(px);
    y_fp = NGFX_FROM_INT(py);
}

void NGCamera::shake(uint8_t amp, uint8_t frames)
{
    shake_amp    = amp;
    shake_frames = frames;
}

void NGCamera::panTo(int16_t dest_x, int16_t dest_y, uint8_t speed)
{
    pan_dest_x = NGFX_FROM_INT(dest_x);
    pan_dest_y = NGFX_FROM_INT(dest_y);
    pan_speed  = speed ? speed : 1;
    mode       = NG_CAM_CINEMATIC;
}

void NGCamera::update(int16_t target_x, int16_t target_y, int16_t target_vx)
{
    int16_t desired_x, desired_y;
    int32_t delta_x, delta_y, step_x, step_y;
    NGFixed new_fp_x, new_fp_y;

    /* Shake — runs in every mode */
    if (shake_frames > 0) {
        uint8_t phase = (uint8_t)(shake_frames & 7u);
        uint8_t amp   = shake_amp;
        if (shake_frames < shake_amp) amp = shake_frames;
        shake_offset_x = (int8_t)(ng_shake_x[phase] * (int8_t)amp);
        shake_offset_y = (int8_t)(ng_shake_y[phase] * (int8_t)amp);
        shake_frames--;
    } else {
        shake_offset_x = 0;
        shake_offset_y = 0;
    }

    if (mode == NG_CAM_CINEMATIC) {
        NGFixed dx_fp = NGFX_SUB(pan_dest_x, x_fp);
        NGFixed dy_fp = NGFX_SUB(pan_dest_y, y_fp);
        NGFixed step  = (NGFixed)((int32_t)pan_speed << (NGFX_SHIFT - 4));

        if (NGFX_ABS(dx_fp) <= step && NGFX_ABS(dy_fp) <= step) {
            x_fp = pan_dest_x;
            y_fp = pan_dest_y;
            mode = NG_CAM_FOLLOW;
        } else {
            x_fp += (dx_fp > 0) ? step : -step;
            y_fp += (dy_fp > 0) ? step : -step;
        }
        x_fp = clampX(x_fp);
        y_fp = clampY(y_fp);
        x = (int16_t)(NGFX_TO_INT(x_fp) + shake_offset_x);
        y = (int16_t)(NGFX_TO_INT(y_fp) + shake_offset_y);
        return;
    }

    if (mode == NG_CAM_FREE) {
        x = (int16_t)(NGFX_TO_INT(x_fp) + shake_offset_x);
        y = (int16_t)(NGFX_TO_INT(y_fp) + shake_offset_y);
        return;
    }

    /* Follow mode */
    desired_x = (int16_t)(target_x - (NG_SCREEN_W / 2));
    desired_y = (int16_t)(target_y - (NG_SCREEN_H / 2));

    if (target_vx > 0 && look_ahead_x > 0)
        desired_x = (int16_t)(desired_x + look_ahead_x);
    else if (target_vx < 0 && look_ahead_x > 0)
        desired_x = (int16_t)(desired_x - look_ahead_x);

    {
        int16_t stx  = (int16_t)(target_x - x);
        int16_t sty  = (int16_t)(target_y - y);
        int16_t scx  = (int16_t)(NG_SCREEN_W / 2);
        int16_t scy  = (int16_t)(NG_SCREEN_H / 2);
        int16_t ox   = (int16_t)(stx - scx);
        int16_t oy   = (int16_t)(sty - scy);

        if (ox < 0) ox = (int16_t)-ox;
        if (oy < 0) oy = (int16_t)-oy;

        if (ox <= (int16_t)dead_zone_x && oy <= (int16_t)dead_zone_y) {
            x = (int16_t)(NGFX_TO_INT(x_fp) + shake_offset_x);
            y = (int16_t)(NGFX_TO_INT(y_fp) + shake_offset_y);
            return;
        }
    }

    new_fp_x = NGFX_FROM_INT(desired_x);
    new_fp_y = NGFX_FROM_INT(desired_y);
    delta_x  = (int32_t)(new_fp_x - x_fp);
    delta_y  = (int32_t)(new_fp_y - y_fp);
    step_x   = (delta_x * (int32_t)follow_speed) >> 8;
    step_y   = (delta_y * (int32_t)follow_speed) >> 8;

    if (step_x == 0 && delta_x != 0) step_x = (delta_x > 0) ?  1 : -1;
    if (step_y == 0 && delta_y != 0) step_y = (delta_y > 0) ?  1 : -1;

    x_fp = clampX((NGFixed)(x_fp + step_x));
    y_fp = clampY((NGFixed)(y_fp + step_y));

    x = (int16_t)(NGFX_TO_INT(x_fp) + shake_offset_x);
    y = (int16_t)(NGFX_TO_INT(y_fp) + shake_offset_y);
}

void NGCamera::apply(int16_t target_x, int16_t target_y, int16_t target_vx)
{
    const NGLevelState *level = level_state();
    if (level) {
        setBounds(level->world_left, level->world_top,
                  level->world_right, level->world_bottom);
    }
    update(target_x, target_y, target_vx);
    ng_level_set_scroll(x, y);
}

int16_t NGCamera::worldToScreenX(int16_t world_x) const
{
    return (int16_t)(world_x - x);
}

int16_t NGCamera::worldToScreenY(int16_t world_y) const
{
    return (int16_t)(world_y - y);
}

/* --- extern "C" wrappers --- */

extern "C" {

void NEOGEO_USER ng_camera_init(NGCamera *cam)
{
    if (cam) cam->init();
}

void NEOGEO_USER ng_camera_set_bounds(NGCamera *cam, int16_t left, int16_t top, int16_t right, int16_t bottom)
{
    if (cam) cam->setBounds(left, top, right, bottom);
}

void NEOGEO_USER ng_camera_set_follow_speed(NGCamera *cam, uint8_t speed)
{
    if (cam) cam->setFollowSpeed(speed);
}

void NEOGEO_USER ng_camera_set_dead_zone(NGCamera *cam, uint8_t half_w, uint8_t half_h)
{
    if (cam) cam->setDeadZone(half_w, half_h);
}

void NEOGEO_USER ng_camera_set_look_ahead(NGCamera *cam, int16_t max_x, int16_t max_y, uint8_t rate)
{
    if (cam) cam->setLookAhead(max_x, max_y, rate);
}

void NEOGEO_USER ng_camera_snap(NGCamera *cam, int16_t x, int16_t y)
{
    if (cam) cam->snap(x, y);
}

void NEOGEO_USER ng_camera_shake(NGCamera *cam, uint8_t amp, uint8_t frames)
{
    if (cam) cam->shake(amp, frames);
}

void NEOGEO_USER ng_camera_pan_to(NGCamera *cam, int16_t dest_x, int16_t dest_y, uint8_t speed)
{
    if (cam) cam->panTo(dest_x, dest_y, speed);
}

void NEOGEO_USER ng_camera_update(NGCamera *cam, int16_t target_x, int16_t target_y, int16_t target_vx)
{
    if (cam) cam->update(target_x, target_y, target_vx);
}

void NEOGEO_USER ng_camera_apply(NGCamera *cam, int16_t target_x, int16_t target_y, int16_t target_vx)
{
    if (cam) cam->apply(target_x, target_y, target_vx);
}

} /* extern "C" */
