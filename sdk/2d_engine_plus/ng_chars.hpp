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

    void NEOGEO_USER setPos(int16_t x, int16_t y);
    void NEOGEO_USER setSpeed(int16_t vx_px, int16_t vy_px);
    void NEOGEO_USER setSpeedFp(int32_t vx_fp, int32_t vy_fp);
    void NEOGEO_USER addSpeedFp(int32_t ax_fp, int32_t ay_fp);
    void NEOGEO_USER setSprite(uint16_t firstSprite, uint8_t strips, uint8_t heightTiles,
                               uint16_t tileBase, uint8_t palette);
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
void        NEOGEO_USER ng_chars_update(void);
void        NEOGEO_USER ng_chars_draw(void);

void NEOGEO_USER ng_char_set_sprite(NGCharacter *c, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette);
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
