#include "sdk/neogeo.h"
#include "scenes/maiya_game.h"

void NEOGEO_USER game_boot(void)
{
    maiya_boot();
}

void NEOGEO_USER game_frame(void)
{
    maiya_frame();
    /* The frame's last sounds (a kill, a pick-up) go to the sound CPU now
     * when it is ready, not a frame later in the blank (sdk/neogeolib.c). */
    ng_sound_pump();
}
