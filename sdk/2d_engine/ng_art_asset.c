#include "ng_art_asset.h"

const NGArtAsset * NEOGEO_USER ng_art_asset_find(const NGArtAsset *assets,
                                                 uint16_t count,
                                                 uint16_t asset_id)
{
    uint16_t i;

    if (!assets) return 0;

    /* Generated registries have contiguous 1-based IDs. Sparse caller
     * tables retain the general lookup path below. */
    if (asset_id != 0u && asset_id <= count &&
        assets[asset_id - 1u].asset_id == asset_id)
        return &assets[asset_id - 1u];

    for (i = 0u; i < count; i++) {
        if (assets[i].asset_id == asset_id) return &assets[i];
    }

    return 0;
}

uint8_t NEOGEO_USER ng_art_asset_is_type(const NGArtAsset *asset, uint8_t type)
{
    if (!asset) return 0u;
    return (uint8_t)(asset->type == type);
}
