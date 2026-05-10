#ifndef NG_PROPERTIES_H
#define NG_PROPERTIES_H

#include "ng_defs.h"

/*
 * Properties Matrix: live game values at time T.
 * Status answers "did this happen?"; properties answer "what is the value now?".
 */

enum {
    NG_PROP_GROUP_TIME = 0,
    NG_PROP_GROUP_PLAYER,
    NG_PROP_GROUP_CAMERA,
    NG_PROP_GROUP_LEVEL,
    NG_PROP_GROUP_WORLD,
    NG_PROP_GROUP_BOSS,
    NG_PROP_GROUP_AUDIO,
    NG_PROP_GROUP_PROGRESS,
    NG_PROP_GROUP_RPG,
    NG_PROP_GROUP_DEBUG
};

enum {
    NG_PROP_TIME_FRAME = 0,
    NG_PROP_TIME_SECOND,
    NG_PROP_TIME_STAGE_FRAME
};

enum {
    NG_PROP_PLAYER_X = 0,
    NG_PROP_PLAYER_Y,
    NG_PROP_PLAYER_W,
    NG_PROP_PLAYER_H,
    NG_PROP_PLAYER_HP,
    NG_PROP_PLAYER_POWER,
    NG_PROP_PLAYER_LIVES,
    NG_PROP_PLAYER_SCORE,
    NG_PROP_PLAYER_ITEM_COUNT
};

enum {
    NG_PROP_CAMERA_X = 0,
    NG_PROP_CAMERA_Y,
    NG_PROP_CAMERA_MODE,
    NG_PROP_CAMERA_W,
    NG_PROP_CAMERA_H
};

enum {
    NG_PROP_LEVEL_ID = 0,
    NG_PROP_LEVEL_MODE,
    NG_PROP_LEVEL_SCROLL_X,
    NG_PROP_LEVEL_SCROLL_Y,
    NG_PROP_LEVEL_BG_SCREEN,
    NG_PROP_LEVEL_OVERLAY_SCREEN,
    NG_PROP_LEVEL_BACKDROP,
    NG_PROP_LEVEL_FLAGS,
    NG_PROP_LEVEL_FIX_PALETTE
};

enum {
    NG_PROP_WORLD_LEFT = 0,
    NG_PROP_WORLD_TOP,
    NG_PROP_WORLD_RIGHT,
    NG_PROP_WORLD_BOTTOM,
    NG_PROP_WORLD_FLAGS,
    NG_PROP_WORLD_GRAVITY
};

enum {
    NG_PROP_BOSS_ID = 0,
    NG_PROP_BOSS_HP,
    NG_PROP_BOSS_PHASE,
    NG_PROP_BOSS_TIMER
};

void properties_init(void);
void prop_set(uint8_t group, uint8_t id, int32_t value);
int32_t prop_get(uint8_t group, uint8_t id);
void prop_add(uint8_t group, uint8_t id, int32_t amount);
void prop_clear_group(uint8_t group);

#endif
