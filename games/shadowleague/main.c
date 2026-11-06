/* Shadow League Fighting: the engine's art-asset hooks (unused: fighters,
 * effects and the stage are addressed through slf_assets.h) and entry. */
#include <stdint.h>
#include "sdk/neogeo.h"
#include "sdk/2d_engine/ng_art_asset.h"
#include "games/shadowleague/scenes/slf.h"

const NGArtAsset * NEOGEO_USER ng_screen_art_asset(uint16_t id) { (void)id; return 0; }
const uint16_t * NEOGEO_USER ng_get_screen_palette(uint16_t id) { (void)id; return 0; }
uint8_t NEOGEO_USER ng_load_screen_palette(uint16_t id) { (void)id; return 0u; }

void NEOGEO_USER maingame(void) { slf_run(); }
int NEOGEO_USER playgame(void) { maingame(); return 0; }
