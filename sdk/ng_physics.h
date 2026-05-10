#ifndef NG_PHYSICS_H
#define NG_PHYSICS_H

#include "ng_chars.h"

typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    uint16_t flags;
} NGSolidRect;

typedef struct {
    uint8_t enabled;
    uint8_t grounded;
    uint16_t flags;
    int16_t prev_x;
    int16_t prev_y;
    int32_t gravity_fp;
    int32_t max_fall_fp;
    int32_t drag_x_fp;
    int32_t drag_y_fp;
} NGPhysicsBody;

#define NG_PHYSICS_GRAVITY   0x0001
#define NG_PHYSICS_WORLD     0x0002
#define NG_PHYSICS_SOLIDS    0x0004
#define NG_PHYSICS_DRAG_X    0x0008
#define NG_PHYSICS_DRAG_Y    0x0010

void physics_init(void);
void physics_attach(NGCharacter *c, uint16_t flags);
void physics_detach(NGCharacter *c);
NGPhysicsBody *physics_body(NGCharacter *c);
void physics_set_gravity(NGCharacter *c, int32_t gravity_fp, int32_t max_fall_fp);
void physics_set_drag(NGCharacter *c, int32_t drag_x_fp, int32_t drag_y_fp);
void physics_clear_solids(void);
uint8_t physics_add_solid(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t flags);
void physics_update_pre(void);
void physics_resolve(void);
uint8_t physics_is_grounded(NGCharacter *c);

#endif
