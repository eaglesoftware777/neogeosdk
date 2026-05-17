#include "ng_particles.hpp"
#include "ng_sprite_pool.hpp"
#include "ng_hw.hpp"

/* --- ParticleSystem singleton --- */

ParticleSystem& ParticleSystem::instance()
{
    static ParticleSystem ps;
    return ps;
}

/* --- ParticleSystem private helpers --- */

void ParticleSystem::hideSlots(uint16_t first, uint16_t end)
{
    while (first < end && first <= NG_SPR_CHAR_LAST) {
        vram_SCB234((uint16_t)(SCB3_ADDR + first), 0);
        first++;
    }
}

uint8_t ParticleSystem::trackFor(uint16_t first_slot)
{
    uint8_t i, free_idx = 0xffu;

    for (i = 0; i < DRAW_TRACKS; i++) {
        if (track_used[i] && track_first[i] == first_slot) return i;
        if (!track_used[i] && free_idx == 0xffu) free_idx = i;
    }

    if (free_idx != 0xffu) {
        track_used[free_idx]  = 1u;
        track_first[free_idx] = first_slot;
        track_end[free_idx]   = first_slot;
        return free_idx;
    }
    return 0u;
}

/* --- ParticleSystem public methods --- */

void ParticleSystem::init()
{
    uint8_t i;
    for (i = 0; i < DRAW_TRACKS; i++) {
        if (track_used[i]) hideSlots(track_first[i], track_end[i]);
        track_used[i]  = 0u;
        track_first[i] = 0u;
        track_end[i]   = 0u;
    }
    for (i = 0; i < NG_PART_MAX_PARTICLES; i++)
        pool[i].active = 0;
}

void ParticleSystem::update()
{
    uint8_t i;

    for (i = 0; i < NG_PART_MAX_PARTICLES; i++) {
        NGParticle *p = &pool[i];
        if (!p->active) continue;

        if (p->lifetime == 0) { p->active = 0; continue; }
        p->lifetime--;

        p->x = (int16_t)(p->x + (p->vx_fp >> NG_FP_SHIFT));
        p->y = (int16_t)(p->y + (p->vy_fp >> NG_FP_SHIFT));

        if (p->type == NG_PART_DUST ||
            p->type == NG_PART_SMOKE ||
            p->type == NG_PART_EXPLOSION) {
            p->vy_fp += 0x80; /* gravity 0.5 px/f in 8.8 */
            if (p->vy_fp > 0x400) p->vy_fp = 0x400;
        }

        if (p->frame_count > 1) {
            p->anim_timer++;
            if (p->anim_timer >= p->anim_period) {
                p->anim_timer = 0;
                p->frame++;
                if (p->frame >= p->frame_count) p->frame = 0;
            }
        }
    }
}

uint16_t ParticleSystem::draw(uint16_t first_slot, uint16_t sprite_budget_used)
{
    uint8_t  i, track;
    uint16_t slot        = first_slot;
    uint8_t  drop_opt    = (sprite_budget_used >= BUDGET_THRESHOLD) ? 1 : 0;
    uint16_t tiles[1], attrs[1];

    for (i = 0; i < NG_PART_MAX_PARTICLES; i++) {
        NGParticle *p = &pool[i];
        if (!p->active) continue;
        if (drop_opt && p->priority == NG_PART_PRI_OPTIONAL) continue;
        if (slot > NG_SPR_CHAR_LAST) break;

        tiles[0] = (uint16_t)(p->tile_base + p->frame);
        attrs[0] = setSCB1_2(p->palette, 0, 0, 0, 0, 0);

        vram_sprite(
            (uint16_t)(64u * slot), 1, slot,
            tiles, attrs, 1,
            setSCB2(0x0F, 0xFF),
            setSCB3((uint16_t)(496 - p->y), 0, 1),
            setSCB4((uint16_t)p->x)
        );
        slot++;
    }

    track = trackFor(first_slot);
    if (track_end[track] > slot) hideSlots(slot, track_end[track]);
    track_end[track] = slot;

    return slot;
}

NGParticle* ParticleSystem::spawn(uint8_t type, uint8_t priority,
                                   int16_t x, int16_t y,
                                   int32_t vx_fp, int32_t vy_fp,
                                   uint8_t lifetime,
                                   uint16_t tile_base, uint8_t palette,
                                   uint8_t frame_count, uint8_t anim_period)
{
    uint8_t i, drop_pri = 0, drop_idx = 0xff;

    for (i = 0; i < NG_PART_MAX_PARTICLES; i++) {
        if (!pool[i].active) {
            NGParticle *p = &pool[i];
            p->active      = 1;
            p->type        = type;
            p->priority    = priority;
            p->x           = x; p->y = y;
            p->vx_fp       = vx_fp; p->vy_fp = vy_fp;
            p->lifetime    = lifetime; p->max_life = lifetime;
            p->tile_base   = tile_base; p->palette = palette;
            p->anim_timer  = 0;
            p->anim_period = anim_period ? anim_period : 4;
            p->frame       = 0;
            p->frame_count = frame_count ? frame_count : 1;
            return p;
        }
        if (pool[i].priority > drop_pri ||
            (pool[i].priority == drop_pri && drop_idx != 0xff &&
             pool[i].lifetime < pool[drop_idx].lifetime)) {
            drop_pri = pool[i].priority;
            drop_idx = i;
        }
    }

    if (drop_idx != 0xff && pool[drop_idx].priority > priority) {
        NGParticle *p = &pool[drop_idx];
        p->active      = 1;
        p->type        = type;
        p->priority    = priority;
        p->x           = x; p->y = y;
        p->vx_fp       = vx_fp; p->vy_fp = vy_fp;
        p->lifetime    = lifetime; p->max_life = lifetime;
        p->tile_base   = tile_base; p->palette = palette;
        p->anim_timer  = 0;
        p->anim_period = anim_period ? anim_period : 4;
        p->frame       = 0;
        p->frame_count = frame_count ? frame_count : 1;
        return p;
    }

    return 0;
}

NGParticle* ParticleSystem::spawnHitSpark(int16_t x, int16_t y, uint16_t tb, uint8_t pal)
{
    return spawn(NG_PART_HIT_SPARK, NG_PART_PRI_CRITICAL, x, y, 0, -(1 << 8), 12, tb, pal, 4, 3);
}

NGParticle* ParticleSystem::spawnDust(int16_t x, int16_t y, uint16_t tb, uint8_t pal)
{
    return spawn(NG_PART_DUST, NG_PART_PRI_NORMAL, x, y, 0, -(1 << 7), 20, tb, pal, 4, 5);
}

NGParticle* ParticleSystem::spawnSlashTrail(int16_t x, int16_t y, int8_t dir, uint16_t tb, uint8_t pal)
{
    int32_t vx = (int32_t)(dir > 0 ? (2 << 8) : -(2 << 8));
    return spawn(NG_PART_SLASH_TRAIL, NG_PART_PRI_NORMAL, x, y, vx, 0, 8, tb, pal, 4, 2);
}

NGParticle* ParticleSystem::spawnExplosion(int16_t x, int16_t y, uint16_t tb, uint8_t pal)
{
    return spawn(NG_PART_EXPLOSION, NG_PART_PRI_CRITICAL, x, y, 0, 0, 24, tb, pal, 8, 3);
}

NGParticle* ParticleSystem::spawnSmoke(int16_t x, int16_t y, uint16_t tb, uint8_t pal)
{
    return spawn(NG_PART_SMOKE, NG_PART_PRI_OPTIONAL, x, y, 0, -(1 << 7), 30, tb, pal, 4, 6);
}

NGParticle* ParticleSystem::spawnMagicSpark(int16_t x, int16_t y, uint16_t tb, uint8_t pal)
{
    return spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_NORMAL, x, y, 0, -(1 << 7), 16, tb, pal, 4, 4);
}

uint8_t ParticleSystem::count() const
{
    uint8_t i, n = 0;
    for (i = 0; i < NG_PART_MAX_PARTICLES; i++)
        if (pool[i].active) n++;
    return n;
}

uint8_t ParticleSystem::countPriority(uint8_t priority) const
{
    uint8_t i, n = 0;
    for (i = 0; i < NG_PART_MAX_PARTICLES; i++)
        if (pool[i].active && pool[i].priority == priority) n++;
    return n;
}

/* --- extern "C" wrappers --- */

extern "C" {

void NEOGEO_USER ng_particles_init(void)
{
    ParticleSystem::instance().init();
}

void NEOGEO_USER ng_particles_update(void)
{
    ParticleSystem::instance().update();
}

uint16_t NEOGEO_USER ng_particles_draw(uint16_t first_slot, uint16_t sprite_budget_used)
{
    return ParticleSystem::instance().draw(first_slot, sprite_budget_used);
}

NGParticle* NEOGEO_USER ng_particle_spawn(uint8_t type, uint8_t priority,
                                           int16_t x, int16_t y,
                                           int32_t vx_fp, int32_t vy_fp,
                                           uint8_t lifetime,
                                           uint16_t tile_base, uint8_t palette,
                                           uint8_t frame_count, uint8_t anim_period)
{
    return ParticleSystem::instance().spawn(type, priority, x, y, vx_fp, vy_fp,
                                            lifetime, tile_base, palette, frame_count, anim_period);
}

NGParticle* NEOGEO_USER ng_spawn_hit_spark(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette)
{
    return ParticleSystem::instance().spawnHitSpark(x, y, tile_base, palette);
}

NGParticle* NEOGEO_USER ng_spawn_dust(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette)
{
    return ParticleSystem::instance().spawnDust(x, y, tile_base, palette);
}

NGParticle* NEOGEO_USER ng_spawn_slash_trail(int16_t x, int16_t y, int8_t dir, uint16_t tile_base, uint8_t palette)
{
    return ParticleSystem::instance().spawnSlashTrail(x, y, dir, tile_base, palette);
}

NGParticle* NEOGEO_USER ng_spawn_explosion(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette)
{
    return ParticleSystem::instance().spawnExplosion(x, y, tile_base, palette);
}

NGParticle* NEOGEO_USER ng_spawn_smoke(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette)
{
    return ParticleSystem::instance().spawnSmoke(x, y, tile_base, palette);
}

NGParticle* NEOGEO_USER ng_spawn_magic_spark(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette)
{
    return ParticleSystem::instance().spawnMagicSpark(x, y, tile_base, palette);
}

uint8_t NEOGEO_USER ng_particles_count(void)
{
    return ParticleSystem::instance().count();
}

uint8_t NEOGEO_USER ng_particles_count_priority(uint8_t priority)
{
    return ParticleSystem::instance().countPriority(priority);
}

} /* extern "C" */
