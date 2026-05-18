#include "ng_joystick.hpp"
#include "ng_game_events.hpp"
#include "ng_physics.hpp"
#include "ng_hw.hpp"

extern "C" {


#define NG_JOY_HISTORY_LEN 16
#define NG_JOY_REPEAT_DELAY 18
#define NG_JOY_REPEAT_RATE 4

typedef struct {
    uint8_t dir;
    uint8_t age;
} NGJoyDirSample;

static NGJoystickState ng_joy;
static NGJoyDirSample ng_joy_hist[NG_JOY_HISTORY_LEN];
static uint8_t ng_joy_hist_count;
static uint8_t ng_joy_attack_timer[NG_MAX_CHARS];
static NGJoyEventHandler ng_joy_event_handler;
static NGJoyCommand ng_joy_cmd_queue[8];
static uint8_t ng_joy_cmd_head;
static uint8_t ng_joy_cmd_tail;

static const NGJoystickCharConfig ng_joy_default_cfg = {
    2,                  /* move_speed_px */
    -(5 << NG_FP_SHIFT),/* jump_speed_fp */
    (1 << 7),           /* gravity_fp: +0.5 px/frame^2 */
    (6 << NG_FP_SHIFT), /* max_fall_fp: 6 px/frame */
    BUTTON_A,           /* jump button */
    BUTTON_B,           /* light attack */
    BUTTON_C,           /* heavy attack */
    BUTTON_D,           /* basic hit trigger */
    6,                  /* light attack active frames */
    10,                 /* heavy attack active frames */
    10, -18, 18, 12,    /* light hitbox */
    12, -20, 24, 14     /* heavy hitbox */
};

static uint8_t NEOGEO_USER ng_joy_button_index(uint16_t bit)
{
    switch (bit) {
        case JOY_UP: return 0;
        case JOY_DOWN: return 1;
        case JOY_LEFT: return 2;
        case JOY_RIGHT: return 3;
        case BUTTON_A: return 4;
        case BUTTON_B: return 5;
        case BUTTON_C: return 6;
        case BUTTON_D: return 7;
        case START1: return 8;
        case SELECT1: return 9;
        case START2: return 10;
        case SELECT2: return 11;
        default: return 0xff;
    }
}

static uint8_t NEOGEO_USER ng_joy_dir_from_raw(uint16_t raw)
{
    uint8_t up = (raw & JOY_UP) ? 1 : 0;
    uint8_t down = (raw & JOY_DOWN) ? 1 : 0;
    uint8_t left = (raw & JOY_LEFT) ? 1 : 0;
    uint8_t right = (raw & JOY_RIGHT) ? 1 : 0;

    if (up && left) return 7;
    if (up && right) return 9;
    if (down && left) return 1;
    if (down && right) return 3;
    if (up) return 8;
    if (down) return 2;
    if (left) return 4;
    if (right) return 6;
    return 5;
}

static void NEOGEO_USER ng_joy_hist_push(uint8_t dir)
{
    uint8_t i;

    if (ng_joy_hist_count > 0 && ng_joy_hist[0].dir == dir) {
        ng_joy_hist[0].age = 0;
        return;
    }

    if (ng_joy_hist_count < NG_JOY_HISTORY_LEN) {
        ng_joy_hist_count++;
    }
    for (i = ng_joy_hist_count; i > 1; i--) {
        ng_joy_hist[i - 1] = ng_joy_hist[i - 2];
    }
    ng_joy_hist[0].dir = dir;
    ng_joy_hist[0].age = 0;
}

static void NEOGEO_USER ng_joy_hist_tick(void)
{
    uint8_t i;
    for (i = 0; i < ng_joy_hist_count; i++) {
        if (ng_joy_hist[i].age < 255) ng_joy_hist[i].age++;
    }
}

static uint8_t NEOGEO_USER ng_joy_find_dir(uint8_t dir, uint8_t start, uint8_t max_age)
{
    uint8_t i;
    for (i = start; i < ng_joy_hist_count; i++) {
        if (ng_joy_hist[i].age > max_age) break;
        if (ng_joy_hist[i].dir == dir) return i;
    }
    return 0xff;
}

static uint16_t NEOGEO_USER ng_joy_facing_adjust_x(int16_t x, uint8_t facing_left)
{
    if (facing_left) return (uint16_t)(-x);
    return (uint16_t)x;
}

static uint8_t NEOGEO_USER ng_joy_pressed_button(uint16_t button)
{
    return (uint8_t)((ng_joy.pressed & button) != 0);
}

static void NEOGEO_USER ng_joy_emit(uint16_t event_id, uint16_t a, uint16_t b, uint16_t c)
{
    ng_game_events_send(event_id, a, b, c);
    if (ng_joy_event_handler) ng_joy_event_handler(event_id, a, b, c);
}

static void NEOGEO_USER ng_joy_push_command(uint8_t code, uint8_t arg0, uint8_t arg1)
{
    uint8_t next = (uint8_t)((ng_joy_cmd_head + 1u) & 7u);
    if (next == ng_joy_cmd_tail) return;
    ng_joy_cmd_queue[ng_joy_cmd_head].code = code;
    ng_joy_cmd_queue[ng_joy_cmd_head].arg0 = arg0;
    ng_joy_cmd_queue[ng_joy_cmd_head].arg1 = arg1;
    ng_joy_cmd_queue[ng_joy_cmd_head].age = 0u;
    ng_joy_cmd_head = next;
}

void NEOGEO_USER ng_joystick_init(void)
{
    uint8_t i;
    ng_joy.raw = 0;
    ng_joy.prev = 0;
    ng_joy.pressed = 0;
    ng_joy.released = 0;
    ng_joy.repeat = 0;
    ng_joy.dir = 5;
    ng_joy.prev_dir = 5;
    for (i = 0; i < 12; i++) ng_joy.hold[i] = 0;
    for (i = 0; i < NG_JOY_HISTORY_LEN; i++) {
        ng_joy_hist[i].dir = 5;
        ng_joy_hist[i].age = 255;
    }
    ng_joy_hist_count = 0;
    for (i = 0; i < NG_MAX_CHARS; i++) ng_joy_attack_timer[i] = 0;
    ng_joy_event_handler = 0;
    ng_joy_cmd_head = 0u;
    ng_joy_cmd_tail = 0u;
}

void NEOGEO_USER ng_joystick_update(void)
{
    static const uint16_t mask_list[12] = {
        JOY_UP, JOY_DOWN, JOY_LEFT, JOY_RIGHT,
        BUTTON_A, BUTTON_B, BUTTON_C, BUTTON_D,
        START1, SELECT1, START2, SELECT2
    };
    uint8_t i;

    ng_joy.prev = ng_joy.raw;
    ng_joy.raw = poll_joystick();
    ng_joy.pressed = (uint16_t)(ng_joy.raw & (uint16_t)(~ng_joy.prev));
    ng_joy.released = (uint16_t)(ng_joy.prev & (uint16_t)(~ng_joy.raw));
    ng_joy.repeat = 0;

    ng_joy.prev_dir = ng_joy.dir;
    ng_joy.dir = ng_joy_dir_from_raw(ng_joy.raw);
    if (ng_joy.dir != ng_joy.prev_dir) ng_joy_hist_push(ng_joy.dir);
    ng_joy_hist_tick();

    for (i = 0; i < 12; i++) {
        uint16_t m = mask_list[i];
        if (ng_joy.raw & m) {
            if (ng_joy.hold[i] < 255) ng_joy.hold[i]++;
            if (ng_joy.hold[i] == NG_JOY_REPEAT_DELAY ||
                (ng_joy.hold[i] > NG_JOY_REPEAT_DELAY &&
                 ((uint8_t)(ng_joy.hold[i] - NG_JOY_REPEAT_DELAY) % NG_JOY_REPEAT_RATE) == 0)) {
                ng_joy.repeat |= m;
            }
        } else {
            ng_joy.hold[i] = 0;
        }
    }
}

const NGJoystickState *NEOGEO_USER ng_joystick_state(void) { return &ng_joy; }
uint16_t NEOGEO_USER ng_joy_down(void) { return ng_joy.raw; }
uint16_t NEOGEO_USER ng_joy_pressed(void) { return ng_joy.pressed; }
uint16_t NEOGEO_USER ng_joy_released(void) { return ng_joy.released; }
uint16_t NEOGEO_USER ng_joy_repeat(void) { return ng_joy.repeat; }
uint8_t NEOGEO_USER ng_joy_direction(void) { return ng_joy.dir; }

uint8_t NEOGEO_USER ng_joy_held_frames(uint16_t mask)
{
    uint8_t idx = ng_joy_button_index(mask);
    if (idx == 0xff) return 0;
    return ng_joy.hold[idx];
}

uint8_t NEOGEO_USER ng_joy_special_qcf(uint8_t facing_left, uint16_t attack_button_mask)
{
    uint8_t fwd = facing_left ? 4 : 6;
    uint8_t dfd = facing_left ? 1 : 3;
    uint8_t i0, i1, i2;

    if (!(ng_joy.pressed & attack_button_mask)) return 0;
    i0 = ng_joy_find_dir(fwd, 0, 20);
    if (i0 == 0xff) return 0;
    i1 = ng_joy_find_dir(dfd, (uint8_t)(i0 + 1), 20);
    if (i1 == 0xff) return 0;
    i2 = ng_joy_find_dir(2, (uint8_t)(i1 + 1), 20);
    if (i2 == 0xff) return 0;
    ng_joy_emit(NG_EVENT_JOY_SPECIAL_QCF, fwd, dfd, attack_button_mask);
    ng_joy_push_command(1u, fwd, dfd);
    ng_joy_emit(NG_EVENT_JOY_COMMAND, 1u, fwd, dfd);
    return 1;
}

uint8_t NEOGEO_USER ng_joy_special_dp(uint8_t facing_left, uint16_t attack_button_mask)
{
    uint8_t fwd = facing_left ? 4 : 6;
    uint8_t dfd = facing_left ? 1 : 3;
    uint8_t i0, i1, i2;

    if (!(ng_joy.pressed & attack_button_mask)) return 0;
    i0 = ng_joy_find_dir(dfd, 0, 18);
    if (i0 == 0xff) return 0;
    i1 = ng_joy_find_dir(2, (uint8_t)(i0 + 1), 18);
    if (i1 == 0xff) return 0;
    i2 = ng_joy_find_dir(fwd, (uint8_t)(i1 + 1), 18);
    if (i2 == 0xff) return 0;
    ng_joy_emit(NG_EVENT_JOY_SPECIAL_DP, fwd, dfd, attack_button_mask);
    ng_joy_push_command(2u, fwd, dfd);
    ng_joy_emit(NG_EVENT_JOY_COMMAND, 2u, fwd, dfd);
    return 1;
}

void NEOGEO_USER ng_joy_apply_hitbox(NGCharacter *c, uint8_t active_frames, int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint8_t idx;
    if (!c) return;
    idx = ng_chars_index(c);
    if (idx == 0xff) return;

    c->hit_x = x;
    c->hit_y = y;
    c->hit_w = w;
    c->hit_h = h;
    ng_joy_attack_timer[idx] = active_frames;
}

void NEOGEO_USER ng_joy_control_character(NGCharacter *c, const NGJoystickCharConfig *cfg)
{
    int16_t vx = 0;
    uint8_t idx;
    NGPhysicsBody *body;
    const NGJoystickCharConfig *use_cfg = cfg ? cfg : &ng_joy_default_cfg;
    uint8_t facing_left = 0;

    if (!c || !c->active) return;
    idx = ng_chars_index(c);
    if (idx == 0xff) return;

    if (ng_joy.raw & JOY_LEFT) {
        vx = (int16_t)(vx - use_cfg->move_speed_px);
        c->facing = 0;
    }
    if (ng_joy.raw & JOY_RIGHT) {
        vx = (int16_t)(vx + use_cfg->move_speed_px);
        c->facing = 1;
    }
    c->vx_fp = NG_TO_FP(vx);
    c->flip_x = (uint8_t)(c->facing ? 0 : 1);
    facing_left = (uint8_t)(c->facing ? 0 : 1);

    body = physics_body(c);
    if (body && body->enabled) {
        if (body->gravity_fp == 0 && use_cfg->gravity_fp > 0) body->gravity_fp = use_cfg->gravity_fp;
        if (body->max_fall_fp == 0 && use_cfg->max_fall_fp > 0) body->max_fall_fp = use_cfg->max_fall_fp;
    }

    if (ng_joy_pressed_button(use_cfg->jump_button)) {
        uint8_t grounded = ng_physics_is_grounded(c);
        if (grounded || !body || !body->enabled) {
            c->vy_fp = use_cfg->jump_speed_fp;
            ng_joy_emit(NG_EVENT_JOY_JUMP, (uint16_t)idx, (uint16_t)c->x, (uint16_t)c->y);
        }
    }

    if (ng_joy_attack_timer[idx] > 0) {
        ng_joy_attack_timer[idx]--;
        if (ng_joy_attack_timer[idx] == 0) {
            c->hit_w = 0;
            c->hit_h = 0;
        }
    }

    if (ng_joy_pressed_button(use_cfg->light_button)) {
        int16_t hx = (int16_t)ng_joy_facing_adjust_x(use_cfg->hit_light_x, facing_left);
        ng_joy_apply_hitbox(c, use_cfg->attack_frames_light, hx, use_cfg->hit_light_y, use_cfg->hit_light_w, use_cfg->hit_light_h);
        ng_joy_emit(NG_EVENT_JOY_FIRE_LIGHT, (uint16_t)idx, (uint16_t)use_cfg->attack_frames_light, 0);
        (void)ng_joy_special_qcf(facing_left, use_cfg->light_button);
        (void)ng_joy_special_dp(facing_left, use_cfg->light_button);
    }

    if (ng_joy_pressed_button(use_cfg->heavy_button)) {
        int16_t hx = (int16_t)ng_joy_facing_adjust_x(use_cfg->hit_heavy_x, facing_left);
        ng_joy_apply_hitbox(c, use_cfg->attack_frames_heavy, hx, use_cfg->hit_heavy_y, use_cfg->hit_heavy_w, use_cfg->hit_heavy_h);
        ng_joy_emit(NG_EVENT_JOY_FIRE_HEAVY, (uint16_t)idx, (uint16_t)use_cfg->attack_frames_heavy, 0);
        (void)ng_joy_special_qcf(facing_left, use_cfg->heavy_button);
        (void)ng_joy_special_dp(facing_left, use_cfg->heavy_button);
    }

    if (ng_joy_pressed_button(use_cfg->hit_button)) {
        ng_joy_emit(NG_EVENT_JOY_HIT, (uint16_t)idx, 0, 0);
    }
}

const NGJoystickCharConfig *NEOGEO_USER ng_joy_default_char_config(void)
{
    return &ng_joy_default_cfg;
}

void NEOGEO_USER ng_joy_set_event_handler(NGJoyEventHandler fn)
{
    ng_joy_event_handler = fn;
}

uint8_t NEOGEO_USER ng_joy_pop_command(NGJoyCommand *out)
{
    if (ng_joy_cmd_tail == ng_joy_cmd_head) return 0u;
    if (out) *out = ng_joy_cmd_queue[ng_joy_cmd_tail];
    ng_joy_cmd_tail = (uint8_t)((ng_joy_cmd_tail + 1u) & 7u);
    return 1u;
}


} /* extern "C" */
