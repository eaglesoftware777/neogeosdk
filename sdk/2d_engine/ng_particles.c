/*
 * ng_particles.c — Particle system implementation (Stage 8)
 *
 * Design decisions:
 *   - Fixed pool of NG_PART_MAX_PARTICLES, no malloc.
 *   - Priority-based drop: when sprite budget is tight, NG_PART_PRI_OPTIONAL
 *     particles are not drawn (but remain alive in the pool until they expire).
 *   - Gravity is optional per particle type.
 *   - Velocity is 8.8 fixed-point: int32_t, NG_FP_SHIFT=8.
 *   - Each particle uses exactly 1 hardware sprite strip (16px wide).
 */

#include "ng_particles.h"
#include "ng_sprite_pool.h"
#include "macro.h"
#include "neogeo.h"

/* Sprite budget threshold: above this, drop OPTIONAL particles */
#define NG_PART_BUDGET_THRESHOLD  200

/* Gravity constant in 8.8 fixed-point: 0.5 pixels per frame per frame */
#define NG_PART_GRAVITY_FP  0x80   /* 0.5 in 8.8 */

/* Max fall speed in 8.8 fixed-point */
#define NG_PART_MAX_FALL_FP 0x400  /* 4.0 px/frame */

#define NG_PART_DRAW_TRACKS 4

static NGParticle ng_part_pool[NG_PART_MAX_PARTICLES];

static uint16_t ng_part_track_first[NG_PART_DRAW_TRACKS];
static uint16_t ng_part_track_end[NG_PART_DRAW_TRACKS];
static uint8_t  ng_part_track_used[NG_PART_DRAW_TRACKS];

/* Scratch tile/attr arrays for sprite upload */
static uint16_t ng_part_tiles[1];
static uint16_t ng_part_attrs[1];

static void NEOGEO_USER ng_part_hide_slots(uint16_t first, uint16_t end)
{
    while (first < end && first <= NG_SPR_PART_LAST) {
        vram_SCB234((uint16_t)(SCB3_ADDR + first), 0);
        first++;
    }
}

static uint8_t NEOGEO_USER ng_part_track_for(uint16_t first_slot)
{
    uint8_t i;
    uint8_t free_idx = 0xffu;

    for (i = 0; i < NG_PART_DRAW_TRACKS; i++) {
        if (ng_part_track_used[i] && ng_part_track_first[i] == first_slot)
            return i;
        if (!ng_part_track_used[i] && free_idx == 0xffu)
            free_idx = i;
    }

    if (free_idx != 0xffu) {
        ng_part_track_used[free_idx] = 1u;
        ng_part_track_first[free_idx] = first_slot;
        ng_part_track_end[free_idx] = first_slot;
        return free_idx;
    }

    return 0u;
}

void NEOGEO_USER ng_particles_init(void)
{
    uint8_t i;
    for (i = 0; i < NG_PART_DRAW_TRACKS; i++) {
        if (ng_part_track_used[i])
            ng_part_hide_slots(ng_part_track_first[i], ng_part_track_end[i]);
        ng_part_track_used[i] = 0u;
        ng_part_track_first[i] = 0u;
        ng_part_track_end[i] = 0u;
    }

    for (i = 0; i < NG_PART_MAX_PARTICLES; i++) {
        ng_part_pool[i].active = 0;
    }
}

void NEOGEO_USER ng_particles_update(void)
{
    uint8_t i;

    for (i = 0; i < NG_PART_MAX_PARTICLES; i++) {
        NGParticle *p = &ng_part_pool[i];

        if (!p->active) continue;

        /* Expire */
        if (p->lifetime == 0) {
            p->active = 0;
            continue;
        }
        p->lifetime--;

        /* Physics: apply velocity */
        p->x = (int16_t)(p->x + (p->vx_fp >> NG_FP_SHIFT));
        p->y = (int16_t)(p->y + (p->vy_fp >> NG_FP_SHIFT));

        /* Gravity for smoke/dust/explosion (not sparks or slash trails) */
        if (p->type == NG_PART_DUST ||
            p->type == NG_PART_SMOKE ||
            p->type == NG_PART_EXPLOSION) {
            p->vy_fp += NG_PART_GRAVITY_FP;
            if (p->vy_fp > NG_PART_MAX_FALL_FP)
                p->vy_fp = NG_PART_MAX_FALL_FP;
        }

        /* Animation: advance tile every anim_period frames */
        if (p->frame_count > 1) {
            p->anim_timer++;
            if (p->anim_timer >= p->anim_period) {
                p->anim_timer = 0;
                p->frame++;
                if (p->frame >= p->frame_count) {
                    /* Looping animation */
                    p->frame = 0;
                }
            }
        }
    }
}

uint16_t NEOGEO_USER ng_particles_draw(uint16_t first_slot, uint16_t sprite_budget_used)
{
    uint8_t  i;
    uint8_t  track;
    uint16_t slot = first_slot;
    uint8_t  drop_optional = (sprite_budget_used >= NG_PART_BUDGET_THRESHOLD) ? 1 : 0;

    if (slot < NG_SPR_PART_FIRST) slot = NG_SPR_PART_FIRST;

    for (i = 0; i < NG_PART_MAX_PARTICLES; i++) {
        NGParticle *p = &ng_part_pool[i];

        if (!p->active) continue;

        /* Budget enforcement */
        if (drop_optional && p->priority == NG_PART_PRI_OPTIONAL) continue;

        /* Stop if we'd exceed the character sprite range */
        if (slot > NG_SPR_PART_LAST) break;

        /* Build tile / attr words for this frame */
        ng_part_tiles[0] = (uint16_t)(p->tile_base + p->frame);
        ng_part_attrs[0] = setSCB1_2(p->palette, 0, 0, 0, 0, 0);

        /* Write SCB1 tile data */
        {
            uint16_t scb1_addr = (uint16_t)(64u * slot);
            vram_sprite(
                scb1_addr,
                1,       /* VRAM_INC */
                slot,
                ng_part_tiles,
                ng_part_attrs,
                1,       /* 1 tile high */
                setSCB2(0x0F, 0xFF),               /* full size */
                setSCB3((uint16_t)(496 - p->y), 0, 1), /* Y, not sticky, 1 tile */
                setSCB4((uint16_t)p->x)
            );
        }

        slot++;
    }

    track = ng_part_track_for(first_slot);
    if (ng_part_track_end[track] > slot)
        ng_part_hide_slots(slot, ng_part_track_end[track]);
    ng_part_track_end[track] = slot;

    return slot;
}

NGParticle * NEOGEO_USER ng_particle_spawn(uint8_t type, uint8_t priority,
                                            int16_t x, int16_t y,
                                            int32_t vx_fp, int32_t vy_fp,
                                            uint8_t lifetime,
                                            uint16_t tile_base, uint8_t palette,
                                            uint8_t frame_count, uint8_t anim_period)
{
    uint8_t i;
    uint8_t drop_pri = 0;
    uint8_t drop_idx = 0xff;

    /* Find a free slot; track the lowest-priority active particle as fallback */
    for (i = 0; i < NG_PART_MAX_PARTICLES; i++) {
        if (!ng_part_pool[i].active) {
            NGParticle *p = &ng_part_pool[i];
            p->active      = 1;
            p->type        = type;
            p->priority    = priority;
            p->x           = x;
            p->y           = y;
            p->vx_fp       = vx_fp;
            p->vy_fp       = vy_fp;
            p->lifetime    = lifetime;
            p->max_life    = lifetime;
            p->tile_base   = tile_base;
            p->palette     = palette;
            p->anim_timer  = 0;
            p->anim_period = anim_period ? anim_period : 4;
            p->frame       = 0;
            p->frame_count = frame_count ? frame_count : 1;
            return p;
        }
        /* Track weakest candidate to evict */
        if (drop_idx == 0xffu ||
            ng_part_pool[i].priority > drop_pri ||
            (ng_part_pool[i].priority == drop_pri &&
             ng_part_pool[i].lifetime < ng_part_pool[drop_idx].lifetime)) {
            drop_pri = ng_part_pool[i].priority;
            drop_idx = i;
        }
    }

    /* Pool full: evict the lowest-priority, nearest-to-dead particle if we
     * are higher priority */
    if (drop_idx != 0xff && ng_part_pool[drop_idx].priority > priority) {
        NGParticle *p = &ng_part_pool[drop_idx];
        p->active      = 1;
        p->type        = type;
        p->priority    = priority;
        p->x           = x;
        p->y           = y;
        p->vx_fp       = vx_fp;
        p->vy_fp       = vy_fp;
        p->lifetime    = lifetime;
        p->max_life    = lifetime;
        p->tile_base   = tile_base;
        p->palette     = palette;
        p->anim_timer  = 0;
        p->anim_period = anim_period ? anim_period : 4;
        p->frame       = 0;
        p->frame_count = frame_count ? frame_count : 1;
        return p;
    }

    return 0; /* pool full, nothing evictable */
}

/* --- Convenience spawn helpers --- */

NGParticle * NEOGEO_USER ng_spawn_hit_spark(int16_t x, int16_t y,
                                              uint16_t tile_base, uint8_t palette)
{
    /* Hit spark: burst upward, 12 frames, 4 animation frames */
    return ng_particle_spawn(NG_PART_HIT_SPARK, NG_PART_PRI_CRITICAL,
                             x, y,
                             0,           /* vx: no horizontal drift */
                             -(1 << 8),   /* vy: -1.0 px/frame upward (8.8) */
                             12,
                             tile_base, palette,
                             4, 3);
}

NGParticle * NEOGEO_USER ng_spawn_dust(int16_t x, int16_t y,
                                         uint16_t tile_base, uint8_t palette)
{
    /* Dust: drift up slowly, 20 frames, 4 animation frames */
    return ng_particle_spawn(NG_PART_DUST, NG_PART_PRI_NORMAL,
                             x, y,
                             0, -(1 << 7), /* -0.5 px/frame upward */
                             20,
                             tile_base, palette,
                             4, 5);
}

NGParticle * NEOGEO_USER ng_spawn_slash_trail(int16_t x, int16_t y, int8_t dir,
                                               uint16_t tile_base, uint8_t palette)
{
    /* Slash trail: horizontal motion, short life */
    int32_t vx = (int32_t)(dir > 0 ? (2 << 8) : -(2 << 8));  /* ±2 px/frame */
    return ng_particle_spawn(NG_PART_SLASH_TRAIL, NG_PART_PRI_NORMAL,
                             x, y,
                             vx, 0,
                             8,
                             tile_base, palette,
                             4, 2);
}

NGParticle * NEOGEO_USER ng_spawn_explosion(int16_t x, int16_t y,
                                             uint16_t tile_base, uint8_t palette)
{
    /* Explosion: large, 24 frames, 8 animation frames */
    return ng_particle_spawn(NG_PART_EXPLOSION, NG_PART_PRI_CRITICAL,
                             x, y,
                             0, 0,
                             24,
                             tile_base, palette,
                             8, 3);
}

NGParticle * NEOGEO_USER ng_spawn_smoke(int16_t x, int16_t y,
                                         uint16_t tile_base, uint8_t palette)
{
    return ng_particle_spawn(NG_PART_SMOKE, NG_PART_PRI_OPTIONAL,
                             x, y,
                             0, -(1 << 7),
                             30,
                             tile_base, palette,
                             4, 6);
}

NGParticle * NEOGEO_USER ng_spawn_magic_spark(int16_t x, int16_t y,
                                               uint16_t tile_base, uint8_t palette)
{
    return ng_particle_spawn(NG_PART_MAGIC_SPARK, NG_PART_PRI_NORMAL,
                             x, y,
                             0, -(1 << 7),
                             16,
                             tile_base, palette,
                             4, 4);
}

uint8_t NEOGEO_USER ng_particles_count(void)
{
    uint8_t i, c = 0;
    for (i = 0; i < NG_PART_MAX_PARTICLES; i++)
        if (ng_part_pool[i].active) c++;
    return c;
}

uint8_t NEOGEO_USER ng_particles_count_priority(uint8_t priority)
{
    uint8_t i, c = 0;
    for (i = 0; i < NG_PART_MAX_PARTICLES; i++)
        if (ng_part_pool[i].active && ng_part_pool[i].priority == priority) c++;
    return c;
}
