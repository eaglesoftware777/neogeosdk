#ifndef NG_SHOOTER_H
#define NG_SHOOTER_H
#include "ng_sprite_group.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Opt-in C module (-DNG_SHOOTER=1). Q8 values are pixels * 256. */
typedef struct {
    int32_t travel_q8, x_q8, y_q8;
    int16_t speed_q8;
    uint8_t follow_shift, dead_zone, limit_x, limit_y;
} NGShooterCamera;
typedef struct {
    NGSpriteGroup pages[2];
    uint16_t page_height;
    int16_t origin_x;
    uint8_t ready;
} NGVerticalLayer;

void NEOGEO_USER ng_shooter_camera_init(NGShooterCamera *camera);
void NEOGEO_USER ng_shooter_camera_set_speed(NGShooterCamera *camera, int16_t speed_q8);
void NEOGEO_USER ng_shooter_camera_set_follow(NGShooterCamera *camera, uint8_t limit_x,
                                              uint8_t limit_y, uint8_t dead_zone, uint8_t shift);
/* Positive speed flies up (scenery moves down). Target is screen-space;
 * existing character collision coordinates are not changed. */
void NEOGEO_USER ng_shooter_camera_step(NGShooterCamera *camera, int16_t target_x, int16_t target_y);
/* Reserve 2*strips slots; supply two row-major pages of rows*strips tiles.
 * Each page has 14..16 rows. Invalid geometry disables the layer. */
uint8_t NEOGEO_USER ng_vertical_layer_init(NGVerticalLayer *layer, uint16_t first_slot,
                                           uint8_t strips, uint8_t rows, uint16_t tile_base,
                                           uint8_t palette, const uint8_t *bank_map, int16_t origin_x);
/* ratio_q8: 0=static, 128=half speed, 256=terrain, 384=foreground.
 * 0..1024 supported. Scrolling changes positions only, not tilemaps. */
void NEOGEO_USER ng_vertical_layer_draw(NGVerticalLayer *layer, const NGShooterCamera *camera,
                                        uint16_t ratio_q8);
void NEOGEO_USER ng_vertical_layer_hide(NGVerticalLayer *layer);
#ifdef __cplusplus
}
#endif
#endif
