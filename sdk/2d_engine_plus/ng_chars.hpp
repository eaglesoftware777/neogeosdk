#ifndef NG_CHARS_HPP
#define NG_CHARS_HPP

#include "ng_defs.hpp"
#include "ng_sprite_group.hpp"

/*
 * NGCharacter — per-character state with member-method API.
 * All fields are public so game .c code can read them directly.
 *
 * CharManager (below) owns the fixed-size pool and handles
 * depth sorting, VRAM slot assignment, and per-frame updates.
 */
struct NGCharacter;
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
    const uint8_t *tile_palettes;
} NGSpriteAssetView;

typedef struct {
    const uint8_t *frame_ids;
    uint8_t frame_count;
    uint8_t frame_period;
    uint8_t loop;
} NGAnimClip;

struct NGCharacter {
    uint8_t  active;
    uint8_t  kind;
    uint8_t  state;
    uint8_t  facing;
    uint8_t  visible;

    int16_t  x;
    int16_t  y;
    int32_t  x_fp;
    int32_t  y_fp;
    int32_t  vx_fp;
    int32_t  vy_fp;

    uint16_t action;
    uint16_t action_pos;
    uint16_t action_timer;

    uint16_t sprite_first;
    uint16_t sprite_tile;
    uint16_t sprite_stride;
    uint8_t  sprite_strips;
    uint8_t  sprite_height;
    uint8_t  sprite_active_rows;
    uint8_t  palette;
    uint8_t  scale_x;
    uint8_t  scale_y;
    uint8_t  flip_x;
    uint8_t  flip_y;
    uint8_t  sprite_dirty;
    uint8_t  priority_band;
    int16_t  depth_offset;

    int16_t  sprite_offset_x;
    int16_t  sprite_offset_y;

    int16_t  body_x;
    int16_t  body_y;
    int16_t  body_w;
    int16_t  body_h;

    int16_t  hit_x;
    int16_t  hit_y;
    int16_t  hit_w;
    int16_t  hit_h;

    uint8_t  hp;
    uint8_t  max_hp;
    uint16_t flags;
    uint16_t data0;
    uint16_t data1;
    uint16_t data2;

    uint16_t asset_tile_start;
    uint16_t asset_tile_end;
    uint8_t  asset_bounds_enabled;
    uint8_t  life_state;
    uint8_t  arena_id;
    int16_t  cull_margin_left;
    int16_t  cull_margin_right;
    int16_t  cull_margin_top;
    int16_t  cull_margin_bottom;
    const NGAnimClip *anim_clip;
    uint8_t  anim_frame;
    uint8_t  anim_timer;
    const uint8_t *sprite_palette_map;

    void NEOGEO_USER setPos(int16_t x, int16_t y);
    void NEOGEO_USER setSpeed(int16_t vx_px, int16_t vy_px);
    void NEOGEO_USER setSpeedFp(int32_t vx_fp, int32_t vy_fp);
    void NEOGEO_USER addSpeedFp(int32_t ax_fp, int32_t ay_fp);
    void NEOGEO_USER setSprite(uint16_t firstSprite, uint8_t strips, uint8_t heightTiles,
                               uint16_t tileBase, uint8_t palette);
    void NEOGEO_USER setAssetBounds(uint16_t tileStart, uint16_t tileEnd);
    void NEOGEO_USER setPaletteMap(const uint8_t *banks);
    uint8_t NEOGEO_USER bindAsset(const NGSpriteAssetView *asset);
    void NEOGEO_USER setCullMargin(int16_t l, int16_t r, int16_t t, int16_t b);
    void NEOGEO_USER setAnimClip(const NGAnimClip *clip);
    void NEOGEO_USER animUpdate();
    void NEOGEO_USER setTileStride(uint16_t stride);
    void NEOGEO_USER setBody(int16_t x, int16_t y, int16_t w, int16_t h);
    void NEOGEO_USER setPriority(uint8_t band, int16_t offset);
    void NEOGEO_USER damage(uint8_t amount);
    void NEOGEO_USER heal(uint8_t amount);
    NGRect NEOGEO_USER bodyRect() const;
    NGRect NEOGEO_USER hitRect() const;
};

/*
 * CharManager — singleton that owns the character pool, interrupt table,
 * VRAM upload tracking, depth sort, and per-frame update/draw.
 *
 * Access via CharManager::instance().
 * Singleton is safe under -fno-threadsafe-statics (single-core, no threads).
 */
class CharManager {
public:
    static CharManager& instance();

    void NEOGEO_USER init();
    NGCharacter* NEOGEO_USER add(uint8_t kind, int16_t x, int16_t y);
    void NEOGEO_USER remove(NGCharacter *c);
    void NEOGEO_USER clearKind(uint8_t kind);
    void NEOGEO_USER resetSlot(uint8_t index);
    void NEOGEO_USER clearArena(uint8_t arena_id);
    void NEOGEO_USER setDefaultArena(uint8_t arena_id);
    void NEOGEO_USER setFixedStep(uint8_t updates_per_frame);
    void NEOGEO_USER updateFixed();
    void NEOGEO_USER defragSlots();
    NGCharacter* NEOGEO_USER find(uint8_t kind) const;
    NGCharacter* NEOGEO_USER at(uint8_t index) const;
    uint8_t NEOGEO_USER count() const;
    uint8_t NEOGEO_USER indexOf(const NGCharacter *c) const;
    void NEOGEO_USER setInterrupt(uint8_t kind, NGCharInterupt fn);
    void NEOGEO_USER update();
    void NEOGEO_USER draw();

    void NEOGEO_USER clearUploadSlot(uint8_t index);

private:
    CharManager() {}
    friend struct NGCharacter;

    NGCharacter    pool[NG_MAX_CHARS];
    NGCharInterupt interrupts[NG_MAX_CHAR_KINDS];
    uint8_t        uploaded_strips[NG_MAX_CHARS];
    uint16_t       uploaded_first[NG_MAX_CHARS];
    uint8_t        active_top;

    void rebuildTop();
    void hideSlot(uint16_t firstSprite, uint8_t strips);
    void hideUploaded(uint8_t idx);
    uint8_t renderVisible(const NGCharacter *c, int16_t cam_x, int16_t cam_y) const;
    int16_t sortY(const NGCharacter *c) const;
    uint8_t drawsBefore(const NGCharacter *a, const NGCharacter *b) const;
    uint8_t depthSort(uint8_t *order, int16_t cam_x, int16_t cam_y) const;
};

#ifdef __cplusplus
extern "C" {
#endif

void        NEOGEO_USER ng_chars_init(void);
NGCharacter* NEOGEO_USER chars_add(uint8_t kind, int16_t x, int16_t y);
void        NEOGEO_USER ng_chars_remove(NGCharacter *c);
void        NEOGEO_USER ng_chars_clear_kind(uint8_t kind);
NGCharacter* NEOGEO_USER chars_find(uint8_t kind);
NGCharacter* NEOGEO_USER chars_at(uint8_t index);
uint8_t     NEOGEO_USER ng_chars_count(void);
uint8_t     NEOGEO_USER ng_chars_index(NGCharacter *c);
void        NEOGEO_USER ng_chars_set_game_interupt(uint8_t kind, NGCharInterupt fn);
void        NEOGEO_USER ng_chars_reset_slot(uint8_t index);
void        NEOGEO_USER ng_chars_begin_scene_arena(uint8_t arena_id);
void        NEOGEO_USER ng_chars_clear_arena(uint8_t arena_id);
void        NEOGEO_USER ng_chars_set_default_arena(uint8_t arena_id);
void        NEOGEO_USER ng_chars_set_fixed_step(uint8_t updates_per_frame);
void        NEOGEO_USER ng_chars_update_fixed(void);
void        NEOGEO_USER ng_chars_defrag_slots(void);
void        NEOGEO_USER ng_chars_update(void);
void        NEOGEO_USER ng_chars_draw(void);

void NEOGEO_USER ng_char_set_sprite(NGCharacter *c, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette);
void NEOGEO_USER ng_char_set_asset_bounds(NGCharacter *c, uint16_t tileStart, uint16_t tileEnd);
uint8_t NEOGEO_USER ng_char_bind_asset(NGCharacter *c, const NGSpriteAssetView *asset);
void NEOGEO_USER ng_char_set_palette_map(NGCharacter *c, const uint8_t *banks);
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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
