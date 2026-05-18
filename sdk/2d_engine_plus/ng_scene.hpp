#ifndef NG_SCENE_HPP
#define NG_SCENE_HPP

#include "ng_defs.hpp"

#define NG_SCENE_CLEAN_FIX        0x01u
#define NG_SCENE_CLEAN_SPRITES    0x02u
#define NG_SCENE_CLEAN_CHARS      0x04u
#define NG_SCENE_CLEAN_PHYSICS    0x08u
#define NG_SCENE_CLEAN_EVENTS     0x10u
#define NG_SCENE_CLEAN_TIMERS     0x20u
#define NG_SCENE_CLEAN_PROGRESS   0x40u
#define NG_SCENE_CLEAN_PALETTEFX  0x80u

#define NG_SCENE_CLEAN_DEFAULT ( \
    NG_SCENE_CLEAN_FIX       | \
    NG_SCENE_CLEAN_SPRITES   | \
    NG_SCENE_CLEAN_CHARS     | \
    NG_SCENE_CLEAN_PHYSICS   | \
    NG_SCENE_CLEAN_EVENTS    | \
    NG_SCENE_CLEAN_TIMERS    | \
    NG_SCENE_CLEAN_PROGRESS  | \
    NG_SCENE_CLEAN_PALETTEFX )

#ifdef __cplusplus
extern "C" {
#endif

void NEOGEO_USER ng_scene_clean(uint8_t flags);
void NEOGEO_USER ng_scene_clean_default(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

