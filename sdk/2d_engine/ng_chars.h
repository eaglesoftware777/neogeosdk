#ifndef NG_CHARS_H
#define NG_CHARS_H

#include "ng_defs.h"
#include "ng_sprite_group.h"

/*
 * Public character handle.
 *
 * Several SDK modules use NGCharacter directly in their public prototypes:
 * ng_npcs.h, ng_physics.h, ng_actions.h, and game code.  Therefore the
 * typedef must exist before those headers are parsed.
 */
typedef struct NGCharacter NGCharacter;

typedef void(*NGCharInterupt)(NGCharacter *c);

#define NG_CHAR_LIFE_FREE            0u
#define NG_CHAR_LIFE_ALLOCATED       1u
#define NG_CHAR_LIFE_VISIBLE         2u
#define NG_CHAR_LIFE_HIDDEN          3u
#define NG_CHAR_LIFE_DESTROY_PENDING 4u

#ifndef NG_STRICT_RENDER_VALIDATE
#define NG_STRICT_RENDER_VALIDATE 1
#endif

typedef struct {
    uint16_t tile_base;
    uint8_t strips;
    uint8_t rows;
    uint8_t palette;
    int16_t offset_x;
    int16_t offset_y;
    uint16_t tile_stride;
    uint16_t tile_start;
    uint16_t tile_end;
} NGSpriteAssetView;

typedef struct {
    const uint8_t *frame_ids;
    uint8_t frame_count;
    uint8_t frame_period;
    uint8_t loop;
} NGAnimClip;

/*
 * Render priority bands.  Lower hardware sprite slots draw in front on Neo Geo,
 * so the character renderer packs higher-priority bands first, then sorts by Y.
 */
struct NGCharacter {
    uint8_t active;
    uint8_t kind;
    uint8_t state;
    uint8_t facing;
    uint8_t visible;

    int16_t x;
    int16_t y;
    int32_t x_fp;
    int32_t y_fp;
    int32_t vx_fp;
    int32_t vy_fp;

    uint16_t action;
    uint16_t action_pos;
    uint16_t action_timer;

    uint16_t sprite_first;
    uint16_t sprite_tile;
    uint16_t sprite_stride;
    uint8_t sprite_strips;
    uint8_t sprite_height;
    uint8_t sprite_active_rows;
    uint8_t palette;
    uint8_t scale_x;
    uint8_t scale_y;
    uint8_t flip_x;
    uint8_t flip_y;
    uint8_t sprite_dirty;
    uint8_t priority_band;
    int16_t depth_offset;

    /*
     * Per-frame draw offsets from the generated Artbox metadata.
     * They keep cropped sprite frames aligned to the original source canvas.
     */
    int16_t sprite_offset_x;
    int16_t sprite_offset_y;

    int16_t body_x;
    int16_t body_y;
    int16_t body_w;
    int16_t body_h;

    int16_t hit_x;
    int16_t hit_y;
    int16_t hit_w;
    int16_t hit_h;

    uint8_t hp;
    uint8_t max_hp;
    uint16_t flags;
    uint16_t data0;
    uint16_t data1;
    uint16_t data2;

    uint16_t asset_tile_start;
    uint16_t asset_tile_end;
    uint8_t asset_bounds_enabled;
    uint8_t life_state;
    uint8_t arena_id;
    int16_t cull_margin_left;
    int16_t cull_margin_right;
    int16_t cull_margin_top;
    int16_t cull_margin_bottom;
    const NGAnimClip *anim_clip;
    uint8_t anim_frame;
    uint8_t anim_timer;
};

void NEOGEO_USER ng_chars_init(void);
NGCharacter* NEOGEO_USER chars_add(uint8_t kind, int16_t x, int16_t y);
void NEOGEO_USER ng_chars_remove(NGCharacter *c);
void NEOGEO_USER ng_chars_clear_kind(uint8_t kind);
NGCharacter* NEOGEO_USER chars_find(uint8_t kind);
NGCharacter* NEOGEO_USER chars_at(uint8_t index);
uint8_t NEOGEO_USER ng_chars_count(void);
uint8_t NEOGEO_USER ng_chars_index(NGCharacter *c);
void NEOGEO_USER ng_chars_set_game_interupt(uint8_t kind, NGCharInterupt fn);
void NEOGEO_USER ng_chars_reset_slot(uint8_t index);
void NEOGEO_USER ng_chars_begin_scene_arena(uint8_t arena_id);
void NEOGEO_USER ng_chars_clear_arena(uint8_t arena_id);
void NEOGEO_USER ng_chars_set_default_arena(uint8_t arena_id);
void NEOGEO_USER ng_chars_set_fixed_step(uint8_t updates_per_frame);
void NEOGEO_USER ng_chars_update_fixed(void);
void NEOGEO_USER ng_chars_defrag_slots(void);
void NEOGEO_USER ng_chars_update(void);
void NEOGEO_USER ng_chars_draw(void);

/* firstSprite is used only as an initial seed; ng_chars_draw re-assigns
 * hardware slots each frame via Y-depth sorting. */
void NEOGEO_USER ng_char_set_sprite(NGCharacter *c, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette);
void NEOGEO_USER ng_char_set_asset_bounds(NGCharacter *c, uint16_t tileStart, uint16_t tileEnd);
uint8_t NEOGEO_USER ng_char_bind_asset(NGCharacter *c, const NGSpriteAssetView *asset);
void NEOGEO_USER ng_char_set_cull_margin(NGCharacter *c, int16_t l, int16_t r, int16_t t, int16_t b);
void NEOGEO_USER ng_char_set_anim_clip(NGCharacter *c, const NGAnimClip *clip);
void NEOGEO_USER ng_char_anim_update(NGCharacter *c);
uint8_t NEOGEO_USER ng_char_validate_asset_window(uint16_t tileBase,
                                                  uint8_t strips,
                                                  uint8_t rows,
                                                  uint16_t stride,
                                                  uint16_t tileStart,
                                                  uint16_t tileEnd);
uint8_t NEOGEO_USER ng_palette_claim(uint8_t palette_slot, uint8_t owner_kind);
void NEOGEO_USER ng_palette_release(uint8_t palette_slot, uint8_t owner_kind);
void NEOGEO_USER ng_char_set_tile_stride(NGCharacter *c, uint16_t stride);
void NEOGEO_USER ng_char_set_body(NGCharacter *c, int16_t x, int16_t y, int16_t w, int16_t h);
void NEOGEO_USER ng_char_set_pos(NGCharacter *c, int16_t x, int16_t y);
void NEOGEO_USER ng_char_set_speed(NGCharacter *c, int16_t vx_px, int16_t vy_px);
void NEOGEO_USER ng_char_set_speed_fp(NGCharacter *c, int32_t vx_fp, int32_t vy_fp);
void NEOGEO_USER ng_char_add_speed_fp(NGCharacter *c, int32_t ax_fp, int32_t ay_fp);
void NEOGEO_USER ng_char_set_priority(NGCharacter *c, uint8_t priority_band, int16_t depth_offset);
void NEOGEO_USER ng_char_damage(NGCharacter *c, uint8_t amount);
void NEOGEO_USER ng_char_heal(NGCharacter *c, uint8_t amount);
NGRect NEOGEO_USER ng_char_body_rect(NGCharacter *c);
NGRect NEOGEO_USER ng_char_hit_rect(NGCharacter *c);

#endif
