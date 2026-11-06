/*
 * ng_particles_none.c — the particle system left out.
 *
 * Linked in place of ng_particles.c by a game whose game.mk sets
 *   GAME_ENGINE_EXCLUDE = ng_particles
 * (a game that draws its own particles). Every entry point is kept, so the
 * engine and any caller still link, but none holds a pool or does work:
 * nothing is updated each frame, nothing is spawned, nothing is drawn, and
 * the game pays neither the code nor the RAM of the real pool.
 */

#include "ng_particles.h"

void NEOGEO_USER ng_particles_init(void)
{
}

void NEOGEO_USER ng_particles_update(void)
{
}

uint16_t NEOGEO_USER ng_particles_draw(uint16_t first_slot, uint16_t sprite_budget_used)
{
    (void)sprite_budget_used;
    return first_slot;
}

NGParticle * NEOGEO_USER ng_particle_spawn(uint8_t type, uint8_t priority,
                                            int16_t x, int16_t y,
                                            int32_t vx_fp, int32_t vy_fp,
                                            uint8_t lifetime,
                                            uint16_t tile_base, uint8_t palette,
                                            uint8_t frame_count, uint8_t anim_period)
{
    (void)type; (void)priority; (void)x; (void)y; (void)vx_fp; (void)vy_fp;
    (void)lifetime; (void)tile_base; (void)palette; (void)frame_count; (void)anim_period;
    return 0;
}

NGParticle * NEOGEO_USER ng_spawn_hit_spark(int16_t x, int16_t y,
                                              uint16_t tile_base, uint8_t palette)
{
    (void)x; (void)y; (void)tile_base; (void)palette;
    return 0;
}

NGParticle * NEOGEO_USER ng_spawn_dust(int16_t x, int16_t y,
                                         uint16_t tile_base, uint8_t palette)
{
    (void)x; (void)y; (void)tile_base; (void)palette;
    return 0;
}

NGParticle * NEOGEO_USER ng_spawn_slash_trail(int16_t x, int16_t y, int8_t dir,
                                               uint16_t tile_base, uint8_t palette)
{
    (void)x; (void)y; (void)dir; (void)tile_base; (void)palette;
    return 0;
}

NGParticle * NEOGEO_USER ng_spawn_explosion(int16_t x, int16_t y,
                                             uint16_t tile_base, uint8_t palette)
{
    (void)x; (void)y; (void)tile_base; (void)palette;
    return 0;
}

NGParticle * NEOGEO_USER ng_spawn_smoke(int16_t x, int16_t y,
                                         uint16_t tile_base, uint8_t palette)
{
    (void)x; (void)y; (void)tile_base; (void)palette;
    return 0;
}

NGParticle * NEOGEO_USER ng_spawn_magic_spark(int16_t x, int16_t y,
                                               uint16_t tile_base, uint8_t palette)
{
    (void)x; (void)y; (void)tile_base; (void)palette;
    return 0;
}

uint8_t NEOGEO_USER ng_particles_count(void)
{
    return 0;
}

uint8_t NEOGEO_USER ng_particles_count_priority(uint8_t priority)
{
    (void)priority;
    return 0;
}
