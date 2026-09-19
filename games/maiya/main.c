#include "sdk/neogeo.h"
#include "scenes/maiya_game.h"

void NEOGEO_USER game_boot(void)
{
    maiya_boot();
}

void NEOGEO_USER game_frame(void)
{
    maiya_frame();
}
