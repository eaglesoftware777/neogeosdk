#ifndef NG_ART_ASSET_HPP
#define NG_ART_ASSET_HPP

#include "ng_defs.hpp"

#define NG_ART_TYPE_SCREEN      0u
#define NG_ART_TYPE_SPRITE      1u
#define NG_ART_TYPE_BACKGROUND  2u
#define NG_ART_TYPE_FIX         3u
#define NG_ART_TYPE_EFFECT      4u

struct NGArtAsset {
    uint16_t asset_id;
    uint8_t type;
    uint8_t palette_bank;
    uint16_t tile_base;
    uint16_t tile_reserved_last;
    uint8_t strips;
    uint8_t rows;
    uint8_t active_rows;
    uint16_t tile_stride;
    int16_t offset_x;
    int16_t offset_y;
    uint16_t content_width;
    uint16_t content_height;
};

#ifdef __cplusplus
extern "C" {
#endif

const NGArtAsset * NEOGEO_USER ng_art_asset_find(const NGArtAsset *assets,
                                                 uint16_t count,
                                                 uint16_t asset_id);
uint8_t NEOGEO_USER ng_art_asset_is_type(const NGArtAsset *asset, uint8_t type);

#ifdef __cplusplus
}
#endif

#endif
