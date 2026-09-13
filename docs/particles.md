# Particle System

## What It Does

`ng_particles` (Stage 8) manages a fixed pool of short-lived sprite effects:
hit sparks, dust, slash trails, explosions, smoke, magic sparks.

Each particle is one 16 px wide sprite strip, animated via tile cycling.

## Why It Matters on NeoGeo

Particles add enormous visual richness with minimal artwork — a few tile frames
shared across dozens of simultaneous particles.  The pool avoids malloc entirely.

## Hardware Limitation It Solves

- Fixed pool (32 slots): no memory allocation, deterministic worst-case cost.
- Priority drop: when `sprite_budget_used >= 200`, optional-priority particles
  are skipped in the draw call but remain alive.  Critical particles always draw.
- Gravity is table-free: simple integer add per frame, no lookup needed.

## Priority Levels

| Level              | Behaviour |
|-------------------|-----------|
| `NG_PART_PRI_CRITICAL` | Always drawn regardless of sprite budget |
| `NG_PART_PRI_NORMAL`   | Drawn unless extreme budget pressure |
| `NG_PART_PRI_OPTIONAL` | Dropped when sprite_budget_used >= threshold |

## Example

```c
// At scene init:
ng_particles_init();

// On sword hit:
ng_spawn_hit_spark(hit_x, hit_y, TILE_SPARK_BASE, PAL_FX);

// On landing:
ng_spawn_dust(foot_x, foot_y, TILE_DUST_BASE, PAL_FX);

// Per frame:
ng_particles_update();
next_slot = ng_particles_draw(first_char_slot + chars_drawn, sprites_used);
```

## Velocity Format

Velocities are in 8.8 fixed-point (matching `NG_FP_SHIFT = 8`):
- `1 << 8` = 1.0 pixel per frame
- `1 << 7` = 0.5 pixel per frame

## Common Mistakes

- Calling `ng_particles_draw()` before `ng_particles_update()`: animation frames
  will be one frame behind.
- Not reserving sprite slots: particles need slots from the character range (0..299).
  Pass the first available slot after characters are drawn.

## Performance Advice

- 32 particles at 1 strip each = 32 SCB1+SCB2+SCB3+SCB4 writes = fast.
- Limit long-lived optional particles (smoke) to avoid pool saturation.
- Use `ng_particles_count()` to feed the debug HUD sprite counter.

## Recent Fixes

- **Sprite budget reporting** (`29c5787`): the budget counter now correctly reflects
  the actual number of sprite slots consumed by active particles.  Previously the
  counter was miscounted, causing the debug HUD to show incorrect occupancy.
- **Stale slot cleanup** (`3fac09d`): when a particle expires, its sprite slot is
  now explicitly cleared (SCB3 height set to 0).  Previously expired particles could
  leave visible ghost sprites until another particle reused the same slot.
- **Valid slot bounds** (`97e14e0`): the demo sprite cleanup loop is clamped to
  valid slot indices, preventing out-of-bounds VRAM writes during scene transitions.
