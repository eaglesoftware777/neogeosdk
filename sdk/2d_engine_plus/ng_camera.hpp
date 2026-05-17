/*
 * ng_camera.hpp — Camera system for the NeoGeo 2D engine.
 *
 * Smooth-follow, look-ahead, dead zone, screen shake, stage clamping,
 * and cinematic pan.  All positions are 16.16 fixed-point internally.
 *
 * NeoGeo screen: 320 x 224 px.
 */

#ifndef NG_CAMERA_HPP
#define NG_CAMERA_HPP

#include "ng_defs.hpp"
#include "ng_fixed.hpp"

#define NG_SCREEN_W   320
#define NG_SCREEN_H   224

#define NG_CAM_FREE        0x00
#define NG_CAM_FOLLOW      0x01
#define NG_CAM_CINEMATIC   0x02

/*
 * NGCamera — value-type camera with member-method API.
 * Instantiate one per scene on the stack or as a global.
 * No singleton: multiple cameras may coexist (split-screen, sub-views).
 */
struct NGCamera {
    NGFixed x_fp;
    NGFixed y_fp;

    int16_t x;
    int16_t y;

    int16_t bound_left;
    int16_t bound_top;
    int16_t bound_right;
    int16_t bound_bottom;

    uint8_t follow_speed;

    int16_t look_ahead_x;
    int16_t look_ahead_y;
    uint8_t look_ahead_rate;

    uint8_t dead_zone_x;
    uint8_t dead_zone_y;

    uint8_t shake_frames;
    uint8_t shake_amp;

    NGFixed pan_dest_x;
    NGFixed pan_dest_y;
    uint8_t pan_speed;
    uint8_t mode;

    int8_t  shake_offset_x;
    int8_t  shake_offset_y;

    void NEOGEO_USER init();
    void NEOGEO_USER setBounds(int16_t left, int16_t top, int16_t right, int16_t bottom);
    void NEOGEO_USER setFollowSpeed(uint8_t speed);
    void NEOGEO_USER setDeadZone(uint8_t half_w, uint8_t half_h);
    void NEOGEO_USER setLookAhead(int16_t max_x, int16_t max_y, uint8_t rate);
    void NEOGEO_USER snap(int16_t x, int16_t y);
    void NEOGEO_USER shake(uint8_t amp, uint8_t frames);
    void NEOGEO_USER panTo(int16_t dest_x, int16_t dest_y, uint8_t speed);
    void NEOGEO_USER update(int16_t target_x, int16_t target_y, int16_t target_vx);
    void NEOGEO_USER apply(int16_t target_x, int16_t target_y, int16_t target_vx);

    int16_t NEOGEO_USER worldToScreenX(int16_t world_x) const;
    int16_t NEOGEO_USER worldToScreenY(int16_t world_y) const;

private:
    NGFixed clampX(NGFixed v) const;
    NGFixed clampY(NGFixed v) const;
};

#define ng_camera_world_to_screen_x(cam, world_x) ((int16_t)((world_x) - (cam)->x))
#define ng_camera_world_to_screen_y(cam, world_y) ((int16_t)((world_y) - (cam)->y))

#ifdef __cplusplus
extern "C" {
#endif

void NEOGEO_USER ng_camera_init(NGCamera *cam);
void NEOGEO_USER ng_camera_set_bounds(NGCamera *cam, int16_t left, int16_t top, int16_t right, int16_t bottom);
void NEOGEO_USER ng_camera_set_follow_speed(NGCamera *cam, uint8_t speed);
void NEOGEO_USER ng_camera_set_dead_zone(NGCamera *cam, uint8_t half_w, uint8_t half_h);
void NEOGEO_USER ng_camera_set_look_ahead(NGCamera *cam, int16_t max_x, int16_t max_y, uint8_t rate);
void NEOGEO_USER ng_camera_snap(NGCamera *cam, int16_t x, int16_t y);
void NEOGEO_USER ng_camera_shake(NGCamera *cam, uint8_t amp, uint8_t frames);
void NEOGEO_USER ng_camera_pan_to(NGCamera *cam, int16_t dest_x, int16_t dest_y, uint8_t speed);
void NEOGEO_USER ng_camera_update(NGCamera *cam, int16_t target_x, int16_t target_y, int16_t target_vx);
void NEOGEO_USER ng_camera_apply(NGCamera *cam, int16_t target_x, int16_t target_y, int16_t target_vx);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
