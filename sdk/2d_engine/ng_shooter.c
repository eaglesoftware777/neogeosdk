#include "ng_shooter.h"

static int32_t NEOGEO_USER wrap(int32_t value, int32_t period)
{
    int32_t result = value % period;
    return result < 0 ? result + period : result;
}

static int32_t NEOGEO_USER follow(int32_t current, int32_t target, uint8_t limit,
                                 uint8_t dead_zone, uint8_t shift)
{
    int32_t delta, step;
    if (target > dead_zone) target -= dead_zone;
    else if (target < -(int16_t)dead_zone) target += dead_zone;
    else target = 0;
    if (target > limit) target = limit;
    if (target < -(int16_t)limit) target = -(int16_t)limit;
    delta = target * 256 - current;
    step = delta / (1L << shift);
    if (!step && delta) step = delta > 0 ? 1 : -1;
    return current + step;
}

void NEOGEO_USER ng_shooter_camera_init(NGShooterCamera *camera)
{
    if (!camera) return;
    camera->travel_q8 = camera->x_q8 = camera->y_q8 = 0;
    camera->speed_q8 = 256;
    camera->follow_shift = 4;
    camera->dead_zone = 8;
    camera->limit_x = camera->limit_y = 0;
}
void NEOGEO_USER ng_shooter_camera_set_speed(NGShooterCamera *camera, int16_t speed_q8)
{
    if (camera) camera->speed_q8 = speed_q8;
}
void NEOGEO_USER ng_shooter_camera_set_follow(NGShooterCamera *camera, uint8_t limit_x,
                                              uint8_t limit_y, uint8_t dead_zone, uint8_t shift)
{
    if (!camera) return;
    camera->limit_x = limit_x;
    camera->limit_y = limit_y;
    camera->dead_zone = dead_zone;
    camera->follow_shift = shift > 8 ? 8 : shift;
}
void NEOGEO_USER ng_shooter_camera_step(NGShooterCamera *camera, int16_t target_x, int16_t target_y)
{
    int32_t next;
    if (!camera) return;
    next = camera->travel_q8 + camera->speed_q8;
    /* Saturate before a long-running session can overflow signed travel. */
    if (next > 0x3fffffffL) next = 0x3fffffffL;
    if (next < -0x3fffffffL) next = -0x3fffffffL;
    camera->travel_q8 = next;
    camera->x_q8 = follow(camera->x_q8, (int32_t)target_x - 160, camera->limit_x,
                           camera->dead_zone, camera->follow_shift);
    camera->y_q8 = follow(camera->y_q8, (int32_t)target_y - 112, camera->limit_y,
                           camera->dead_zone, camera->follow_shift);
}

uint8_t NEOGEO_USER ng_vertical_layer_init(NGVerticalLayer *layer, uint16_t first_slot,
                                           uint8_t strips, uint8_t rows, uint16_t tile_base,
                                           uint8_t palette, const uint8_t *bank_map, int16_t origin_x)
{
    uint8_t page;
    uint32_t tiles = (uint32_t)strips * rows;
    if (!layer) return 0;
    layer->ready = 0;
    if (!strips || strips > 32 || rows < 14 || rows > 16 || !first_slot ||
        (uint32_t)first_slot + strips * 2u > 381u ||
        (uint32_t)tile_base + tiles * 2u > 65536u) return 0;
    layer->page_height = rows * 16u;
    layer->origin_x = origin_x;
    for (page = 0; page < 2; page++) {
        NGSpriteGroup *g = &layer->pages[page];
        ng_sprite_group_init(g, first_slot + page * strips, strips, rows,
                             tile_base + page * tiles, palette);
        ng_sprite_group_set_tile_stride(g, strips);
        ng_sprite_group_set_palette_map(g, bank_map ? bank_map + page * tiles : 0);
        ng_sprite_group_set_priority(g, NG_SG_PRIO_HIGH);
        ng_sprite_group_set_pos(g, origin_x, page ? -(int16_t)layer->page_height : 0);
        ng_sprite_group_upload(g);
    }
    layer->ready = 1;
    return 1;
}

void NEOGEO_USER ng_vertical_layer_draw(NGVerticalLayer *layer, const NGShooterCamera *camera,
                                        uint16_t ratio_q8)
{
    uint8_t page;
    int32_t period, scaled, phase;
    int16_t x;
    if (!layer || !layer->ready || !camera) return;
    if (ratio_q8 > 1024u) ratio_q8 = 1024u;
    period = (int32_t)layer->page_height * 2;
    /* Split before multiplying: exact Q8 parallax without 64-bit arithmetic
     * or early rounding, even after thousands of map wraps. */
    scaled = wrap(camera->travel_q8 / 256, period * 256) * ratio_q8;
    scaled += (camera->travel_q8 % 256) * ratio_q8 / 256;
    scaled -= camera->y_q8 * ratio_q8 / 256;
    phase = wrap(scaled, period * 256) / 256;
    x = layer->origin_x - (int16_t)(camera->x_q8 * ratio_q8 / 65536L);
    for (page = 0; page < 2; page++) {
        int16_t y = (int16_t)wrap(phase + page * layer->page_height, period);
        if (y >= layer->page_height) y -= (int16_t)period;
        ng_sprite_group_set_pos(&layer->pages[page], x, y);
        ng_sprite_group_flush(&layer->pages[page]);
    }
}
void NEOGEO_USER ng_vertical_layer_hide(NGVerticalLayer *layer)
{
    uint8_t page;
    if (!layer || !layer->ready) return;
    for (page = 0; page < 2; page++) {
#ifdef NG_VRAM_DEFER
        ng_sprite_group_cancel(&layer->pages[page]);
#endif
        ng_sprite_group_hide(&layer->pages[page]);
    }
    layer->ready = 0;
}
