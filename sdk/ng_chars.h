#ifndef NG_CHARS_H
#define NG_CHARS_H

#include "ng_defs.h"
#include "ng_sprite_group.h"

/*
 * NGCharacter — one active entity in the game world.
 *
 * Covers players, enemies, NPCs, projectiles, FX overlays, bosses, etc.
 * Characters are allocated from a fixed pool (NG_MAX_CHARS entries).
 * Each character can have one active action script, one sprite group, and
 * one axis-aligned body rectangle used for collision detection.
 *
 * Fixed-point positions (x_fp / y_fp) drive physics; the integer fields
 * (x / y) are updated by chars_update() each frame from the FP values.
 *
 * Per-character logic runs through the NGCharInterupt callback registered
 * with chars_set_game_interupt(). The callback fires once per frame BEFORE
 * collision and draw.
 *
 * User scratch space: data0, data1, data2 are free for game-specific state
 * (e.g. counters, state machines, cooldown timers).
 */
typedef struct NGCharacter {
    uint8_t  active;        /* non-zero if slot is in use                    */
    uint8_t  kind;          /* user-defined entity type (see DEMO_CHAR_* etc) */
    uint8_t  state;         /* user-defined sub-state                         */
    uint8_t  facing;        /* 0 = right, 1 = left                            */
    uint8_t  visible;       /* 0 = hidden (sprite not uploaded to VRAM)       */

    int16_t  x;             /* screen X in pixels, derived from x_fp          */
    int16_t  y;             /* screen Y in pixels, derived from y_fp          */
    int32_t  x_fp;          /* fixed-point X (NG_FP_SHIFT bits of fraction)   */
    int32_t  y_fp;          /* fixed-point Y                                  */
    int32_t  vx_fp;         /* horizontal velocity (pixels/frame, fixed-point) */
    int32_t  vy_fp;         /* vertical velocity (pixels/frame, fixed-point)   */

    uint16_t action;        /* current action script ID                        */
    uint16_t action_pos;    /* current byte offset within the action script    */
    uint16_t action_timer;  /* ticks remaining on the current FRAME command    */

    /* Sprite rendering --------------------------------------------------- */
    uint16_t sprite_first;  /* first hardware sprite slot                      */
    uint16_t sprite_tile;   /* current tileBase written to VRAM                */
    uint16_t sprite_stride; /* tile columns in ROM per image row (set manually) */
    uint8_t  sprite_strips; /* number of visible strips (hardware sprites)     */
    uint8_t  sprite_height; /* height in tiles per strip                       */
    uint8_t  sprite_active_rows; /* SCB3 active-character count for the strip   */
    uint8_t  palette;       /* palette bank index (0–255)                      */
    uint8_t  scale_x;       /* horizontal scale (0xFF = full, 0x00 = 1px)     */
    uint8_t  scale_y;       /* vertical scale   (0xFF = full, 0x00 = 1px)     */
    uint8_t  flip_x;        /* mirror horizontally if non-zero                 */
    uint8_t  flip_y;        /* mirror vertically if non-zero                   */
    uint8_t  sprite_dirty;  /* set to force VRAM re-upload this frame          */

    /* Collision body (relative to character origin) ----------------------- */
    int16_t  body_x;
    int16_t  body_y;
    int16_t  body_w;
    int16_t  body_h;

    /* Hit-stun / attack box (relative to character origin) --------------- */
    int16_t  hit_x;
    int16_t  hit_y;
    int16_t  hit_w;
    int16_t  hit_h;

    /* Health -------------------------------------------------------------- */
    uint8_t  hp;
    uint8_t  max_hp;

    uint16_t flags;         /* generic bitfield for game-specific boolean flags */

    /* Scratch space for game logic ---------------------------------------- */
    uint16_t data0;
    uint16_t data1;
    uint16_t data2;
} NGCharacter;

/* Per-character update callback, called once per frame. */
typedef void (*NGCharInterupt)(NGCharacter *c);

/* --- Pool management ----------------------------------------------------- */

void         chars_init(void);
NGCharacter* chars_add(uint8_t kind, int16_t x, int16_t y);
void         chars_remove(NGCharacter *c);
void         chars_clear_kind(uint8_t kind);
NGCharacter* chars_find(uint8_t kind);
NGCharacter* chars_at(uint8_t index);
uint8_t      chars_count(void);
uint8_t      chars_index(NGCharacter *c);

/* Register a per-frame logic callback for all characters of `kind`. */
void         chars_set_game_interupt(uint8_t kind, NGCharInterupt fn);

/* Run logic + upload sprites for all active characters. */
void         chars_update(void);
void         chars_draw(void);

/* --- Per-character helpers ----------------------------------------------- */

/*
 * char_set_sprite — bind a sprite group to a character.
 *
 * firstSprite  hardware sprite slot for the first strip (e.g. 32).
 * strips       number of horizontal strips to render.
 * heightTiles  number of tiles per strip (height).
 * tileBase     ROM tile index of the first tile in frame 0.
 * palette      palette bank (0–255).
 *
 * NOTE: char_set_sprite sets sprite_stride = strips by default.
 *       If your art has a wider row in ROM (e.g. 16 columns for a 256px image
 *       when only showing 6 strips), set c->sprite_stride = 16 AFTER this call.
 */
void char_set_sprite(NGCharacter *c, uint16_t firstSprite, uint8_t strips,
                     uint8_t heightTiles, uint16_t tileBase, uint8_t palette);

/* Set the local-space body rectangle used for collision (x/y relative to c->x/y). */
void char_set_body(NGCharacter *c, int16_t x, int16_t y, int16_t w, int16_t h);

/* Teleport a character to (x, y), syncing fixed-point position. */
void char_set_pos(NGCharacter *c, int16_t x, int16_t y);

/* Set velocity in whole pixels/frame (converted to fixed-point internally). */
void char_set_speed(NGCharacter *c, int16_t vx_px, int16_t vy_px);

void char_damage(NGCharacter *c, uint8_t amount);
void char_heal(NGCharacter *c, uint8_t amount);

NGRect char_body_rect(NGCharacter *c);
NGRect char_hit_rect(NGCharacter *c);

/* Convenience macros ------------------------------------------------------ */

/* Show or hide a character's sprite without removing it from the pool. */
#define char_show(c)           ((c)->visible = 1)
#define char_hide(c)           ((c)->visible = 0)

/* Park a character's sprite off the right edge of the screen. */
#define char_park_offscreen(c) do { \
    (c)->x = 400; (c)->x_fp = NG_TO_FP(400); } while(0)

/* Test whether character c is currently running action script `id`. */
#define char_in_action(c, id)  ((c)->action == (id))

#endif
