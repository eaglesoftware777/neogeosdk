/* Cartridge asset access; generated data lives in artbox/generated. */
#include <stdint.h>
#include "sdk/neogeo.h"
#include "sdk/2d_engine/ng_art_asset.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "games/skylance/artbox/generated/sky_assets.h"
#include "games/skylance/scenes/sky.h"

const NGArtAsset * NEOGEO_USER ng_screen_art_asset(uint16_t id)
{
    return id && id <= SKY_ART_COUNT ? &sky_art_assets[id - 1u] : 0;
}

const uint16_t * NEOGEO_USER ng_get_screen_palette(uint16_t id)
{
    return id && id <= SKY_ART_COUNT ? sky_art_palettes[id - 1u] : 0;
}

uint8_t NEOGEO_USER ng_load_screen_palette(uint16_t id)
{
    const NGArtAsset *art = ng_screen_art_asset(id);
    uint8_t i;
    if (!art) return 0u;
    for (i = 0u; i < sky_art_bank_counts[id - 1u]; i++)
        ng_palfx_screen_load((uint8_t)(art->palette_bank + i),
                            sky_art_palettes[id - 1u] + i * 16u);
    return 1u;
}

void NEOGEO_USER maingame(void) { sky_run(); }
int NEOGEO_USER playgame(void) { maingame(); return 0; }
