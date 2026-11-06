/* Cross-compiled layout data for tools/shot_probe.py. */
#include <stddef.h>
#include "sdk/2d_engine/ng_chars.h"
#include "games/skylance/scenes/sky_stage.h"

const uint16_t sky_shot_probe_layout[] = {
    sizeof(NGCharacter), NG_MAX_CHARS,
    offsetof(NGCharacter, active), offsetof(NGCharacter, kind),
    offsetof(NGCharacter, x), offsetof(NGCharacter, y),
    offsetof(NGCharacter, vx_fp), offsetof(NGCharacter, vy_fp),
    offsetof(NGCharacter, data0), offsetof(NGCharacter, sprite_first),
    SKY_KIND_PLAYER, SKY_KIND_ENEMY, SKY_KIND_ESHOT, SKY_KIND_PSHOT, SKY_KIND_CANOPY, SKY_KIND_BOSS,
    offsetof(NGCharacter, x_fp), offsetof(NGCharacter, y_fp)
};
