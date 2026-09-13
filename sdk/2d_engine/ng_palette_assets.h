#ifndef NG_PALETTE_ASSETS_H
#define NG_PALETTE_ASSETS_H

#include "ng_palette_fx.h"

#ifdef __cplusplus
extern "C" {
#endif

const NGPaletteAsset * NEOGEO_USER ng_palette_asset_find(const NGPaletteAsset *assets,
                                                         uint16_t count,
                                                         uint16_t asset_id);


#ifdef __cplusplus
}
#endif
#endif
