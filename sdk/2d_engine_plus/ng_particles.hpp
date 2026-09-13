/*
 * ng_particles.hpp — Particle and impact effect system.
 *
 * Fixed-size pool, no malloc.  Priority-based drop when sprite budget is tight.
 * Each particle is a 1-strip sprite (16 px wide).
 */

#ifndef NG_PARTICLES_HPP
#define NG_PARTICLES_HPP

#include "ng_defs.hpp"
#include "ng_sprite_group.hpp"

#define NG_PART_MAX_PARTICLES   32

#define NG_PART_PRI_CRITICAL    0
#define NG_PART_PRI_NORMAL      1
#define NG_PART_PRI_OPTIONAL    2

#define NG_PART_DUST            0
#define NG_PART_HIT_SPARK       1
#define NG_PART_SLASH_TRAIL     2
#define NG_PART_EXPLOSION       3
#define NG_PART_MAGIC_SPARK     4
#define NG_PART_SMOKE           5
#define NG_PART_WATER_SPLASH    6
#define NG_PART_METAL_HIT       7

#define NG_PART_ANIM_FRAMES     4

/*
 * NGParticle — per-particle state.  All fields public for direct C access.
 */
struct NGParticle {
    uint8_t  active;
    uint8_t  priority;
    uint8_t  type;

    int16_t  x;
    int16_t  y;
    int32_t  vx_fp;
    int32_t  vy_fp;

    uint8_t  lifetime;
    uint8_t  max_life;
    uint16_t tile_base;
    uint8_t  palette;
    uint8_t  anim_timer;
    uint8_t  anim_period;
    uint8_t  frame;
    uint8_t  frame_count;
};

/*
 * ParticleSystem — singleton that owns the particle pool and draw tracks.
 */
class ParticleSystem {
public:
    static ParticleSystem& instance();

    void NEOGEO_USER init();
    void NEOGEO_USER update();
    uint16_t NEOGEO_USER draw(uint16_t first_slot, uint16_t sprite_budget_used);

    NGParticle* NEOGEO_USER spawn(uint8_t type, uint8_t priority,
                                  int16_t x, int16_t y,
                                  int32_t vx_fp, int32_t vy_fp,
                                  uint8_t lifetime,
                                  uint16_t tile_base, uint8_t palette,
                                  uint8_t frame_count, uint8_t anim_period);

    NGParticle* NEOGEO_USER spawnHitSpark(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
    NGParticle* NEOGEO_USER spawnDust(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
    NGParticle* NEOGEO_USER spawnSlashTrail(int16_t x, int16_t y, int8_t dir, uint16_t tile_base, uint8_t palette);
    NGParticle* NEOGEO_USER spawnExplosion(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
    NGParticle* NEOGEO_USER spawnSmoke(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
    NGParticle* NEOGEO_USER spawnMagicSpark(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);

    uint8_t NEOGEO_USER count() const;
    uint8_t NEOGEO_USER countPriority(uint8_t priority) const;

private:
    ParticleSystem() {}

    static constexpr uint8_t DRAW_TRACKS = 4;
    static constexpr uint16_t BUDGET_THRESHOLD = 200;

    NGParticle pool[NG_PART_MAX_PARTICLES];
    uint16_t   track_first[DRAW_TRACKS];
    uint16_t   track_end[DRAW_TRACKS];
    uint8_t    track_used[DRAW_TRACKS];

    void     hideSlots(uint16_t first, uint16_t end);
    uint8_t  trackFor(uint16_t first_slot);
};

#ifdef __cplusplus
extern "C" {
#endif

void      NEOGEO_USER ng_particles_init(void);
void      NEOGEO_USER ng_particles_update(void);
uint16_t  NEOGEO_USER ng_particles_draw(uint16_t first_slot, uint16_t sprite_budget_used);

NGParticle* NEOGEO_USER ng_spawn_hit_spark(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
NGParticle* NEOGEO_USER ng_spawn_dust(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
NGParticle* NEOGEO_USER ng_spawn_slash_trail(int16_t x, int16_t y, int8_t dir, uint16_t tile_base, uint8_t palette);
NGParticle* NEOGEO_USER ng_spawn_explosion(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
NGParticle* NEOGEO_USER ng_spawn_smoke(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
NGParticle* NEOGEO_USER ng_spawn_magic_spark(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);

NGParticle* NEOGEO_USER ng_particle_spawn(uint8_t type, uint8_t priority,
                                           int16_t x, int16_t y,
                                           int32_t vx_fp, int32_t vy_fp,
                                           uint8_t lifetime,
                                           uint16_t tile_base, uint8_t palette,
                                           uint8_t frame_count, uint8_t anim_period);

uint8_t NEOGEO_USER ng_particles_count(void);
uint8_t NEOGEO_USER ng_particles_count_priority(uint8_t priority);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
