#ifndef SDK_NG_SCENE_NG_SCENE_H
#define SDK_NG_SCENE_NG_SCENE_H

#include <stdint.h>
#include "macro.h"

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

typedef struct NGScene NGScene;
typedef void (*NGSceneFn)(void);
typedef uint8_t (*NGSceneUpdateFn)(uint16_t frame);

#define NGSCENE_FLAG_MANUAL_ADVANCE 0x0001u
#define NGSCENE_FLAG_AUTOCLEAR      0x0002u

struct NGScene {
    const char *name;
    NGSceneFn enter;
    NGSceneUpdateFn update;
    NGSceneFn exit;
    uint16_t duration;
    uint16_t flags;
};

void NEOGEO_USER ngscene_run(const NGScene *scenes, uint8_t count);
void NEOGEO_USER ngscene_run_one(const NGScene *scene);

#endif
