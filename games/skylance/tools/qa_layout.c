/* Cross-compiled layout data for the emulator inspection harness. */
#include <stddef.h>
#include "sdk/2d_engine/ng_chars.h"
#include "games/skylance/scenes/sky_stage.h"

const uint16_t sky_qa_layout[] = {
    sizeof(NGCharacter), NG_MAX_CHARS,
    offsetof(NGCharacter, active), offsetof(NGCharacter, kind),
    offsetof(NGCharacter, visible), offsetof(NGCharacter, x),
    offsetof(NGCharacter, y), offsetof(NGCharacter, x_fp),
    offsetof(NGCharacter, y_fp), offsetof(NGCharacter, sprite_first),
    SKY_KIND_ENEMY, SKY_KIND_ESHOT,
    offsetof(NGCharacter, data0), offsetof(NGCharacter, flags),
    offsetof(NGCharacter, data2), offsetof(NGCharacter, hp)
};
