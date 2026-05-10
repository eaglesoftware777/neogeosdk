#ifndef NG_LEVEL_H
#define NG_LEVEL_H

#include "ng_defs.h"

typedef struct {
    uint8_t level_id;
    uint8_t mode;
    uint16_t background_screen;
    uint16_t overlay_screen;
    uint16_t backdrop;
    uint8_t fix_palette;
    int16_t scroll_x;
    int16_t scroll_y;
    int16_t world_left;
    int16_t world_top;
    int16_t world_right;
    int16_t world_bottom;
    uint16_t flags;
} NGLevelState;

void level_init(void);
void level_set_id(uint8_t level_id);
void level_set_mode(uint8_t mode);
void level_set_background(uint16_t screen_id);
void level_set_overlay(uint16_t screen_id);
void level_set_backdrop(uint16_t color);
void level_set_fix_palette(uint8_t palette);
void level_set_scroll(int16_t x, int16_t y);
void level_move_scroll(int16_t dx, int16_t dy);
void level_set_world_bounds(int16_t left, int16_t top, int16_t right, int16_t bottom);
void level_camera_follow(int16_t target_x, int16_t target_y, int16_t screen_w, int16_t screen_h);
const NGLevelState *level_state(void);
void level_update(void);

#endif
