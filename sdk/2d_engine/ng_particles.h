/*
 * ng_particles.h — Particle and impact effect system (Stage 8)
 *
 * Fixed-size pool, no malloc.  Low-priority particles are silently dropped
 * when the sprite budget is tight.  Each particle is a 1-strip sprite group
 * (16 px wide).
 *
 * NeoGeo sprite budget note:
 *   Sprite slots 0..299 are for characters, effects, NPCs.
 *   Particles share this budget.  The particle system tracks the sprite
 *   cursor and refuses to allocate if the cursor exceeds NG_SPR_CHAR_LAST.
 *
 * Velocity is 8.8 fixed-point (matches ng_defs.h format).
 */

#ifndef NG_PARTICLES_H
#define NG_PARTICLES_H

#include "ng_defs.h"
#include "ng_sprite_group.h"

/* Total particle slots — must not exceed hardware sprite budget */
#define NG_PART_MAX_PARTICLES   32

/* Priority levels */
#define NG_PART_PRI_CRITICAL    0   /* always drawn: hit sparks, death explosions */
#define NG_PART_PRI_NORMAL      1   /* usually drawn: dust, debris */
#define NG_PART_PRI_OPTIONAL    2   /* decorative: background ambient particles */

/* Particle type IDs */
#define NG_PART_DUST            0
#define NG_PART_HIT_SPARK       1
#define NG_PART_SLASH_TRAIL     2
#define NG_PART_EXPLOSION       3
#define NG_PART_MAGIC_SPARK     4
#define NG_PART_SMOKE           5
#define NG_PART_WATER_SPLASH    6
#define NG_PART_METAL_HIT       7

/* Number of frames per particle animation tile */
#define NG_PART_ANIM_FRAMES     4

typedef struct {
    uint8_t  active;
    uint8_t  priority;      /* NG_PART_PRI_* */
    uint8_t  type;          /* NG_PART_* */

    int16_t  x;             /* world position */
    int16_t  y;

    int32_t  vx_fp;         /* velocity in 8.8 fixed-point */
    int32_t  vy_fp;

    uint8_t  lifetime;      /* frames remaining */
    uint8_t  max_life;      /* initial lifetime (for anim phase) */

    uint16_t tile_base;     /* first tile in the animation strip */
    uint8_t  palette;       /* hardware palette slot */
    uint8_t  anim_timer;    /* counts down to advance tile */
    uint8_t  anim_period;   /* frames per tile */
    uint8_t  frame;         /* current animation frame index */
    uint8_t  frame_count;   /* number of animation frames */
} NGParticle;

void NEOGEO_USER ng_particles_init(void);

/*
 * Per-frame update: advance physics, update animation, expire dead particles.
 * Call once per frame before ng_particles_draw().
 */
void NEOGEO_USER ng_particles_update(void);

/*
 * Draw all active particles using sprite slots starting at first_slot.
 * Returns the next free sprite slot after the particles.
 * Drops NG_PART_PRI_OPTIONAL particles when sprite_budget_used >= threshold.
 */
uint16_t NEOGEO_USER ng_particles_draw(uint16_t first_slot, uint16_t sprite_budget_used);

/* Spawn helpers — return pointer to the new particle or NULL if pool full */
NGParticle * NEOGEO_USER ng_spawn_hit_spark(int16_t x, int16_t y,
                                              uint16_t tile_base, uint8_t palette);
NGParticle * NEOGEO_USER ng_spawn_dust(int16_t x, int16_t y,
                                         uint16_t tile_base, uint8_t palette);
NGParticle * NEOGEO_USER ng_spawn_slash_trail(int16_t x, int16_t y, int8_t dir,
                                               uint16_t tile_base, uint8_t palette);
NGParticle * NEOGEO_USER ng_spawn_explosion(int16_t x, int16_t y,
                                             uint16_t tile_base, uint8_t palette);
NGParticle * NEOGEO_USER ng_spawn_smoke(int16_t x, int16_t y,
                                         uint16_t tile_base, uint8_t palette);
NGParticle * NEOGEO_USER ng_spawn_magic_spark(int16_t x, int16_t y,
                                               uint16_t tile_base, uint8_t palette);

/* Low-level: spawn with full control */
NGParticle * NEOGEO_USER ng_particle_spawn(uint8_t type, uint8_t priority,
                                            int16_t x, int16_t y,
                                            int32_t vx_fp, int32_t vy_fp,
                                            uint8_t lifetime,
                                            uint16_t tile_base, uint8_t palette,
                                            uint8_t frame_count, uint8_t anim_period);

/* Count active particles by priority level. */
uint8_t NEOGEO_USER ng_particles_count(void);
uint8_t NEOGEO_USER ng_particles_count_priority(uint8_t priority);

#endif
