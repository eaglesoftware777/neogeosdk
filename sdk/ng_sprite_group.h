#ifndef NG_SPRITE_GROUP_H
#define NG_SPRITE_GROUP_H

#include "ng_defs.h"

typedef struct {
    uint16_t firstSprite;
    uint8_t strips;
    uint8_t heightTiles;
    uint8_t activeRows;
    uint16_t tileBase;
    uint16_t tileStride;
    uint8_t palette;
    int16_t x;
    int16_t y;
    uint8_t xScale;
    uint8_t yScale;
    uint8_t hflip;
    uint8_t vflip;
    uint8_t autoAnim4;
    uint8_t autoAnim8;
    uint8_t visible;
} NGSpriteGroup;

void ngSpriteGroupInit(NGSpriteGroup *g, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette);
void ngSpriteGroupSetTileBase(NGSpriteGroup *g, uint16_t tileBase);
void ngSpriteGroupSetTileStride(NGSpriteGroup *g, uint16_t tileStride);
void ngSpriteGroupSetPalette(NGSpriteGroup *g, uint8_t palette);
void ngSpriteGroupSetActiveRows(NGSpriteGroup *g, uint8_t activeRows);
void ngSpriteGroupSetPos(NGSpriteGroup *g, int16_t x, int16_t y);
void ngSpriteGroupMove(NGSpriteGroup *g, int16_t dx, int16_t dy);
void ngSpriteGroupSetScale(NGSpriteGroup *g, uint8_t xScale, uint8_t yScale);
void ngSpriteGroupSetFlip(NGSpriteGroup *g, uint8_t hflip, uint8_t vflip);
void ngSpriteGroupSetAutoAnim(NGSpriteGroup *g, uint8_t autoAnim4, uint8_t autoAnim8);
void ngSpriteGroupSetVisible(NGSpriteGroup *g, uint8_t visible);
void ngSpriteGroupUpload(NGSpriteGroup *g);
void ngSpriteGroupUpdateTransform(NGSpriteGroup *g);
void ngSpriteGroupHide(NGSpriteGroup *g);

#endif
