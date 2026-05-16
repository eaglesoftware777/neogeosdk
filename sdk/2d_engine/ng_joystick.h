#ifndef NG_JOYSTICK_H
#define NG_JOYSTICK_H

#include "ng_defs.h"
#include "ng_chars.h"

/* Event IDs emitted by joystick helpers through ng_game_events_send(). */
#define NG_EVENT_JOY_JUMP            0x5001
#define NG_EVENT_JOY_FIRE_LIGHT      0x5002
#define NG_EVENT_JOY_FIRE_HEAVY      0x5003
#define NG_EVENT_JOY_HIT             0x5004
#define NG_EVENT_JOY_SPECIAL_QCF     0x5005
#define NG_EVENT_JOY_SPECIAL_DP      0x5006

/* Direction values stored in history using numpad notation:
 * 2=down, 4=left, 6=right, 8=up, diagonals 1/3/7/9, 5=neutral. */
typedef struct {
    uint16_t raw;
    uint16_t prev;
    uint16_t pressed;
    uint16_t released;
    uint16_t repeat;
    uint8_t dir;
    uint8_t prev_dir;
    uint8_t hold[12];
} NGJoystickState;

typedef struct {
    int16_t move_speed_px;        /* horizontal speed in pixels/frame */
    int32_t jump_speed_fp;        /* negative value for upward jump */
    int32_t gravity_fp;           /* per-frame gravity applied while airborne */
    int32_t max_fall_fp;          /* clamp fall speed */
    uint8_t jump_button;          /* BUTTON_A/B/C/D */
    uint8_t light_button;         /* BUTTON_A/B/C/D */
    uint8_t heavy_button;         /* BUTTON_A/B/C/D */
    uint8_t hit_button;           /* BUTTON_A/B/C/D */
    uint8_t attack_frames_light;  /* active frames for light hitbox */
    uint8_t attack_frames_heavy;  /* active frames for heavy hitbox */
    int16_t hit_light_x;
    int16_t hit_light_y;
    int16_t hit_light_w;
    int16_t hit_light_h;
    int16_t hit_heavy_x;
    int16_t hit_heavy_y;
    int16_t hit_heavy_w;
    int16_t hit_heavy_h;
} NGJoystickCharConfig;

void NEOGEO_USER ng_joystick_init(void);
void NEOGEO_USER ng_joystick_update(void);
const NGJoystickState *NEOGEO_USER ng_joystick_state(void);

uint16_t NEOGEO_USER ng_joy_down(void);
uint16_t NEOGEO_USER ng_joy_pressed(void);
uint16_t NEOGEO_USER ng_joy_released(void);
uint16_t NEOGEO_USER ng_joy_repeat(void);
uint8_t NEOGEO_USER ng_joy_direction(void);
uint8_t NEOGEO_USER ng_joy_held_frames(uint16_t mask);

/* Built-in special move checkers (edge-triggered on attack button). */
uint8_t NEOGEO_USER ng_joy_special_qcf(uint8_t facing_left, uint16_t attack_button_mask);
uint8_t NEOGEO_USER ng_joy_special_dp(uint8_t facing_left, uint16_t attack_button_mask);

/* High-level helpers for controlling a player character from joystick input. */
void NEOGEO_USER ng_joy_control_character(NGCharacter *c, const NGJoystickCharConfig *cfg);
void NEOGEO_USER ng_joy_apply_hitbox(NGCharacter *c, uint8_t active_frames, int16_t x, int16_t y, int16_t w, int16_t h);

/* Default config with classic brawler mapping:
 * A=jump, B=light, C=heavy, D=hit-test trigger. */
const NGJoystickCharConfig *NEOGEO_USER ng_joy_default_char_config(void);

#endif
