#include "ng_level.h"
#include "ng_properties.h"

static NGLevelState ng_level_state;

static void ng_level_sync(void)
{
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_ID, ng_level_state.level_id);
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_MODE, ng_level_state.mode);
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_SCROLL_X, ng_level_state.scroll_x);
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_SCROLL_Y, ng_level_state.scroll_y);
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_BG_SCREEN, ng_level_state.background_screen);
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_OVERLAY_SCREEN, ng_level_state.overlay_screen);
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_BACKDROP, ng_level_state.backdrop);
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_FLAGS, ng_level_state.flags);
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_FIX_PALETTE, ng_level_state.fix_palette);

    prop_set(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_LEFT, ng_level_state.world_left);
    prop_set(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_TOP, ng_level_state.world_top);
    prop_set(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_RIGHT, ng_level_state.world_right);
    prop_set(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_BOTTOM, ng_level_state.world_bottom);
    prop_set(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_FLAGS, ng_level_state.flags);
}

void NEOGEO_USER level_init(void)
{
    ng_level_state.level_id = 0;
    ng_level_state.mode = 0;
    ng_level_state.background_screen = 0;
    ng_level_state.overlay_screen = 0;
    ng_level_state.backdrop = 0x0000;
    ng_level_state.fix_palette = 0;
    ng_level_state.scroll_x = 0;
    ng_level_state.scroll_y = 0;
    ng_level_state.world_left = 0;
    ng_level_state.world_top = 0;
    ng_level_state.world_right = 319;
    ng_level_state.world_bottom = 223;
    ng_level_state.flags = 0;
    ng_level_sync();
}

void NEOGEO_USER level_set_id(uint8_t level_id)
{
    ng_level_state.level_id = level_id;
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_ID, level_id);
}

void NEOGEO_USER level_set_mode(uint8_t mode)
{
    ng_level_state.mode = mode;
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_MODE, mode);
}

void NEOGEO_USER level_set_background(uint16_t screen_id)
{
    ng_level_state.background_screen = screen_id;
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_BG_SCREEN, screen_id);
}

void NEOGEO_USER level_set_overlay(uint16_t screen_id)
{
    ng_level_state.overlay_screen = screen_id;
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_OVERLAY_SCREEN, screen_id);
}

void NEOGEO_USER level_set_backdrop(uint16_t color)
{
    ng_level_state.backdrop = color;
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_BACKDROP, color);
}

void NEOGEO_USER level_set_fix_palette(uint8_t palette)
{
    ng_level_state.fix_palette = palette;
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_FIX_PALETTE, palette);
}

void NEOGEO_USER level_set_scroll(int16_t x, int16_t y)
{
    ng_level_state.scroll_x = x;
    ng_level_state.scroll_y = y;
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_SCROLL_X, x);
    prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_SCROLL_Y, y);
}

void NEOGEO_USER level_move_scroll(int16_t dx, int16_t dy)
{
    level_set_scroll(
        (int16_t)(ng_level_state.scroll_x + dx),
        (int16_t)(ng_level_state.scroll_y + dy)
    );
}

void NEOGEO_USER level_set_world_bounds(int16_t left, int16_t top, int16_t right, int16_t bottom)
{
    ng_level_state.world_left = left;
    ng_level_state.world_top = top;
    ng_level_state.world_right = right;
    ng_level_state.world_bottom = bottom;
    prop_set(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_LEFT, left);
    prop_set(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_TOP, top);
    prop_set(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_RIGHT, right);
    prop_set(NG_PROP_GROUP_WORLD, NG_PROP_WORLD_BOTTOM, bottom);
}

void NEOGEO_USER level_camera_follow(int16_t target_x, int16_t target_y, int16_t screen_w, int16_t screen_h)
{
    int16_t cx = (int16_t)(target_x - (screen_w / 2));
    int16_t cy = (int16_t)(target_y - (screen_h / 2));

    if (ng_level_state.world_right > ng_level_state.world_left) {
        int16_t max_x = (int16_t)(ng_level_state.world_right - screen_w);
        if (cx < ng_level_state.world_left) cx = ng_level_state.world_left;
        if (cx > max_x) cx = max_x;
    }
    if (ng_level_state.world_bottom > ng_level_state.world_top) {
        int16_t max_y = (int16_t)(ng_level_state.world_bottom - screen_h);
        if (cy < ng_level_state.world_top) cy = ng_level_state.world_top;
        if (cy > max_y) cy = max_y;
    }

    prop_set(NG_PROP_GROUP_CAMERA, NG_PROP_CAMERA_X, cx);
    prop_set(NG_PROP_GROUP_CAMERA, NG_PROP_CAMERA_Y, cy);
    prop_set(NG_PROP_GROUP_CAMERA, NG_PROP_CAMERA_W, screen_w);
    prop_set(NG_PROP_GROUP_CAMERA, NG_PROP_CAMERA_H, screen_h);
}

const NGLevelState *NEOGEO_USER level_state(void)
{
    return &ng_level_state;
}

void NEOGEO_USER level_update(void)
{
    ng_level_sync();
}
