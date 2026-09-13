#include "ng_scene/ng_scene.h"
#include "ng_video/ng_video.h"
#include "neogeo.h"

void NEOGEO_USER ngscene_run_one(const NGScene *scene)
{
    uint16_t frame;
    uint16_t duration;
    uint8_t done = 0;

    if (!scene) return;

    if (scene->enter) scene->enter();

    duration = scene->duration ? scene->duration : 1;
    for (frame = 0; frame < duration && !done; frame++) {
        if (scene->update) done = scene->update(frame);
        waitVbl();
        if ((scene->flags & NGSCENE_FLAG_MANUAL_ADVANCE) && ngvideo_advance_requested())
            done = 1;
    }

    if (scene->exit) scene->exit();
    if (scene->flags & NGSCENE_FLAG_AUTOCLEAR) ngvideo_end_scene(1);
}

void NEOGEO_USER ngscene_run(const NGScene *scenes, uint8_t count)
{
    uint8_t i;

    if (!scenes) return;
    for (i = 0; i < count; i++) {
        ngscene_run_one(&scenes[i]);
    }
}
