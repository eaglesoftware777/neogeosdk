# 2d_engine — NeoGeo 2D Engine (C build)

A bare-metal 2D engine for the NeoGeo, written in C against the
68000 freestanding toolchain. This directory is the C build of the
engine; the parallel `sdk/2d_engine_plus` directory is the same engine
in C++14, with identical public ABI.

> **Compiler flags this engine is built under**
> `-std=gnu99 -ffreestanding -march=68000`. No stdlib, no malloc,
> no floats. All identifiers are `extern "C"` from the C++ side's
> point of view, which means a C++ game can link against this build
> without changes.

Both builds compile down to the same hardware writes. Choose one
per ROM via the `USE_2D_PLUS` make flag.

```sh
make p1                   # link C engine (this directory) — default
make p1 USE_2D_PLUS=1     # link C++ engine
```

---

## Why two builds?

| Build | When to use it |
|-------|----------------|
| `sdk/2d_engine`      | You're writing a C game; you want the smallest call chain and the simplest static analysis. |
| `sdk/2d_engine_plus` | You're writing a C++ game; you want member methods, singletons, and scene-stack helpers on top of this API. |

The class methods and singletons that `2d_engine_plus` adds are
wrappers around the functions defined in this directory. Both
directories share the same struct layouts.

---

## Hardware constraints the engine respects

NeoGeo display is 320 × 224. Hardware gives you:

* **381 sprite slots**, each up to 32 tiles tall, chainable horizontally
  via the SCB3 "sticky" bit. The engine groups them into
  `NGSpriteGroup`s (strip 0 anchor + N chained strips).
* **256 colours per palette bank, 256 banks**. Bank 0 reserved for FIX.
* **The FIX layer** — a 40 × 32 cell text/UI layer that overlays
  sprites. The engine treats it as a UI surface (`ng_fix_*`).
* **VBlank ~3 ms wide**. All VRAM writes from gameplay are queued and
  flushed during VBlank by `ng_render_queue_flush()`.

The engine never touches VRAM from the game logic path. Any module
that needs to write hardware posts a command to the render queue.

---

## Module index

Modules are grouped by responsibility. Each entry links a one-line
purpose to its primary header.

### Core / glue

| Header | Purpose |
|---|---|
| `ng_engine.h`             | Umbrella include — pulls in every public module. |
| `ng_defs.h`               | Sizes, constants, `NEOGEO_USER` linkage tag. |

### Math / state primitives

| Header | Purpose |
|---|---|
| `ng_fixed.h`              | 8.8 and 16.16 fixed-point arithmetic. |
| `ng_game_time.h`          | Frame and stage counters (60 Hz). |
| `ng_timers.h`             | One-shot countdown timers. |
| `ng_progress.h`           | 0..max progress bars (charge, load, hp ramp). |
| `ng_status.h`             | Bitset of "did this happen?" flags. |
| `ng_properties.h`         | Live "what is the value now?" matrix. |
| `ng_game_events.h`        | FIFO of events with a single handler. |

### Hardware / VRAM management

| Header | Purpose |
|---|---|
| `ng_sprite_pool.h`        | Slot reservations (chars, FX, particles, BG). |
| `ng_sprite_window.h`      | Owned strip range with auto tail-clear. |
| `ng_vram.h`               | Slot ↔ SCB1 address conversions; bulk clears. |
| `ng_render_queue.h`       | Deferred VRAM/palette writes drained at VBlank. |

### Rendering

| Header | Purpose |
|---|---|
| `ng_fix.h`                | FIX layer text/cells with dirty-cell cache. |
| `ng_sprite_group.h`       | `NGSpriteGroup` — chained strip group + upload(). |
| `ng_chars.h`              | Depth-sorted character pool. |
| `ng_actions.h`            | Animation script DSL (`FRAME / WAIT / LOOP / ...`). |
| `ng_npcs.h`               | AI layer on top of `NGCharacter`. |
| `ng_art_asset.h`          | Per-asset metadata (tile_base, strips, rows). |
| `ng_palette_assets.h`     | Palette asset lookup. |
| `ng_bg.h`                 | Two background layers using slots 300+. |
| `ng_camera.h`             | Smooth follow, look-ahead, dead zone, shake. |
| `ng_palette_fx.h`         | Fade / flash / pulse / cycle palette FX. |
| `ng_particles.h`          | Particle pool with priority-based drop. |
| `ng_feedback.h`           | Hitstop, slow-motion, `ng_impact_event()`. |
| `ng_depthfx.h`            | 2.5D depth projection (z → scale + sort). |

### Gameplay glue

| Header | Purpose |
|---|---|
| `ng_physics.h`            | Gravity, drag, solid AABB resolution. |
| `ng_border_constraints.h` | Invisible rects that fire game events. |
| `ng_level.h`              | Current level state struct (scroll, world bounds). |
| `ng_scene.h`              | Scene reset helpers. |
| `ng_game_interupt.h`      | VBlank hook table (before_logic / after_draw / ...). |
| `ng_joystick.h`           | Pads, edge events, buffered specials (QCF, DP). |

### Tooling

| Header | Purpose |
|---|---|
| `ng_debug.h`              | FIX-layer perf overlay (sprites used, particle count, ...). |
| `ng_demo_advanced.h`      | Self-contained demo scenes (sticky-char, palette cycle, ...). |

---

## Lifecycle

A frame in a typical game loop:

```c
ng_game_engine_init();     /* one-shot, at boot */
ng_game_runtime_init();    /* one-shot, after assets are loaded */

/* per scene */
ng_scene_reset(NG_SCENE_CLEAN_DEFAULT);

/* per frame */
waitVbl();                 /* hardware VBlank */
ng_render_queue_flush();   /* drain queued VRAM/palette writes */
ng_palette_fx_update();    /* step fades/flashes */
ng_particles_update();     /* step particle pool */
ng_feedback_update();      /* step hitstop/slow-mo */
/* ...game logic... */
ng_chars_update();         /* run per-kind interrupts */
ng_physics_update_pre();   /* apply gravity/drag */
ng_physics_resolve();      /* resolve against solids */
ng_chars_draw();           /* depth sort + upload SCB1/2/3/4 */
```

`ng_game_engine_set_hooks(before_logic, collision, after_events,
before_draw, after_draw)` lets a game register its per-frame entry
points without taking over the main loop.

---

## Minimal scene

```c
#include "ng_engine.h"

static void enter(void)
{
    NGCharacter *hero;

    ng_scene_reset(NG_SCENE_CLEAN_DEFAULT);

    hero = chars_add(0, 80, 160);
    ng_char_set_sprite(hero, 0, /*strips*/6, /*rowsTiles*/6,
                       /*tileBase*/19108, /*palette*/90);
    ng_char_set_body(hero, -12, -32, 24, 32);
    ng_physics_attach(hero, NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS);
    ng_physics_set_gravity(hero, NG_FP_FROM_FRAC(1, 4), NG_TO_FP(4));
    ng_physics_add_solid(0, 200, 320, 8, 0);
}
```

---

## Conventions

* **Naming.** Functions are `ng_<module>_<verb>()`. Types are
  `NG<PascalCase>`. Manifest constants are `NG_<ALL_CAPS>`.
* **No malloc.** Every pool is a fixed array sized in `ng_defs.h`.
  If a pool fills, `ng_*_add()` returns `NULL` and the call site
  must handle it.
* **Fixed-point.** Velocity and small deltas use 8.8 (`int16_t / int32_t`
  via `NG_FP_*`). Camera / world positions use 16.16 (`NGFixed`). Never
  mix the two without conversion macros.
* **VBlank-safe writes only.** Game logic never writes VRAM or palette
  RAM directly. Always post via `ng_render_queue_*` and let
  `ng_render_queue_flush()` drain at VBlank.
* **One owner per slot.** Hardware sprite slots are partitioned by
  `ng_sprite_pool.h`. Don't write to a slot owned by another subsystem.

---

## Where to go next

* The unified demo at `games/demo` exercises every public subsystem
  end-to-end. Start at `games/demo/scenes/demo.c`.
* The parallel C++ engine docs are in `sdk/2d_engine_plus/README.md`.
* SDK API conventions, hardware mapping, and the ROM build pipeline
  are documented at the project root (`README.md`, `SDK_API_GUIDE.md`).
