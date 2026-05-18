#include "ng_palette_assets.hpp"

const NGPaletteAsset * NEOGEO_USER ng_palette_asset_find(const NGPaletteAsset *assets,
                                                         uint16_t count,
                                                         uint16_t asset_id)
{
    if (!assets) return 0;

    for (uint16_t i = 0u; i < count; i++) {
        if (assets[i].asset_id == asset_id) return &assets[i];
    }

    return 0;
}
