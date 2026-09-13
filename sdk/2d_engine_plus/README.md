# 2d_engine_plus — NeoGeo 2D Engine (C++ build)

A bare-metal 2D engine for the NeoGeo, written in C++14 against the
68000 freestanding toolchain. This directory is the C++ build of the
engine; the parallel `sdk/2d_engine` directory is the same engine in
plain C, with identical public ABI.

> **Compiler flags this engine is built under**
> `-std=c++14 -fno-exceptions -fno-rtti -fno-threadsafe-statics
> -ffreestanding -march=68000`. No stdlib, no malloc, no floats,
> no global ctors that depend on order.

The public API is `extern "C"` so a game written in C can link
against the C++ build (and vice-versa). The class methods and
singletons that this build adds are convenience wrappers; the C
entry points always exist.

---

## Why two builds?

The two directories solve different problems:

| Build | When to use it |
|-------|----------------|
| `sdk/2d_engine`      | You're writing a C game; you want the smallest possible code-size footprint and the simplest call chain. |
| `sdk/2d_engine_plus` | You're writing a C++ game; you want member methods (`cam.follow(...)`, `eagle->setBody(...)`), singletons (`CharManager::instance()`), and the scene stack helpers. |

Both builds compile down to the same hardware writes. Choose one per
ROM via the `USE_2D_PLUS` make flag.

```sh
make p1 USE_2D_PLUS=1     # link C++ engine (this directory)
make p1                   # link C engine
```

---

## Hardware constraints the engine respects

The NeoGeo's display is 320 × 224. Hardware gives you:

* **381 sprite slots**, each up to 32 tiles tall, chainable horizontally
  via the SCB3 "sticky" bit. The engine groups them into
  `NGSpriteGroup`s (a strip 0 anchor + N chained strips).
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
| `ng_engine.hpp`           | Umbrella include — pulls in every public module. |
| `ng_engine_plus.hpp`      | Same as above, but the canonical C++ entry. |
| `ng_defs.hpp`             | Sizes, constants, `NEOGEO_USER` linkage tag. |
| `ng_hw.hpp`               | `extern "C"` wrapper around `macro.h` / `neogeo.h`. |
| `ng_runtime.hpp`          | Thin alias for `ng_engine.hpp`. |

### Math / state primitives

| Header | Purpose |
|---|---|
| `ng_fixed.hpp`            | 8.8 and 16.16 fixed-point arithmetic. |
| `ng_game_time.hpp`        | Frame and stage counters (60 Hz). |
| `ng_timers.hpp`           | One-shot countdown timers. |
| `ng_progress.hpp`         | 0..max progress bars (charge, load, hp ramp). |
| `ng_status.hpp`           | Bitset of "did this happen?" flags. |
| `ng_properties.hpp`       | Live "what is the value now?" matrix. |
| `ng_game_events.hpp`      | FIFO of events with a single handler. |

### Hardware / VRAM management

| Header | Purpose |
|---|---|
| `ng_sprite_pool.hpp`      | Slot reservations (chars, FX, particles, BG). |
| `ng_sprite_window.hpp`    | Owned strip range with auto tail-clear. |
| `ng_vram.hpp`             | Slot ↔ SCB1 address conversions; bulk clears. |
| `ng_render_queue.hpp`     | Deferred VRAM/palette writes drained at VBlank. |

### Rendering

| Header | Purpose |
|---|---|
| `ng_fix.hpp`              | FIX layer text/cells with dirty-cell cache. |
| `ng_sprite_group.hpp`     | `NGSpriteGroup` — chained strip group + upload(). |
| `ng_chars.hpp`            | `CharManager` — depth-sorted character pool. |
| `ng_actions.hpp`          | Animation script DSL (`FRAME / WAIT / LOOP / ...`). |
| `ng_npcs.hpp`             | AI layer on top of `NGCharacter`. |
| `ng_art_asset.hpp`        | Per-asset metadata (tile_base, strips, rows). |
| `ng_palette_assets.hpp`   | Palette asset lookup. |
| `ng_bg.hpp`               | Two background layers using slots 300+. |
| `ng_camera.hpp`           | Smooth follow, look-ahead, dead zone, shake. |
| `ng_palette_fx.hpp`       | Fade / flash / pulse / cycle palette FX. |
| `ng_particles.hpp`        | Particle pool with priority-based drop. |
| `ng_feedback.hpp`         | Hitstop, slow-motion, `ng_impact_event()`. |
| `ng_depthfx.hpp`          | 2.5D depth projection (z → scale + sort). |

### Gameplay glue

| Header | Purpose |
|---|---|
| `ng_physics.hpp`          | Gravity, drag, solid AABB resolution. |
| `ng_border_constraints.hpp` | Invisible rects that fire game events. |
| `ng_level.hpp`             | Current level state struct (scroll, world bounds). |
| `ng_scene.hpp`             | Scene reset / push / pop helpers. |
| `ng_game_interupt.hpp`     | VBlank hook table (before_logic / after_draw / ...). |
| `ng_joystick.hpp`          | Pads, edge events, buffered specials (QCF, DP). |

### Tooling

| Header | Purpose |
|---|---|
| `ng_debug.hpp`            | FIX-layer perf overlay (sprites used, particle count, ...). |
| `ng_demo_advanced.hpp`    | Self-contained demo scenes (sticky-char, palette cycle, ...). |

---

## Lifecycle

A frame in a typical game loop:

```cpp
ng_game_engine_init();     // one-shot, at boot
ng_game_runtime_init();    // one-shot, after assets are loaded

// per scene
ng_scene_reset(NG_SCENE_CLEAN_DEFAULT);

// per frame
waitVbl();                 // hardware VBlank
ng_render_queue_flush();   // drain queued VRAM/palette writes
ng_palette_fx_update();    // step fades/flashes
ng_particles_update();     // step particle pool
ng_feedback_update();      // step hitstop/slow-mo
// ...game logic...
ng_chars_update();         // run per-kind interrupts
ng_physics_update_pre();   // apply gravity/drag
ng_physics_resolve();      // resolve against solids
ng_chars_draw();           // depth sort + upload SCB1/2/3/4
```

`ng_game_engine_set_hooks(before_logic, collision, after_events,
before_draw, after_draw)` lets a game register its per-frame entry
points without taking over the main loop.

---

## Minimal scene

```cpp
#include "ng_engine_plus.hpp"

static void enter()
{
    ng_scene_reset(NG_SCENE_CLEAN_DEFAULT);

    NGCharacter* hero = CharManager::instance().add(0, 80, 160);
    hero->setSprite(0, /*strips*/6, /*rowsTiles*/6,
                    /*tileBase*/19108, /*palette*/90);
    hero->setBody(-12, -32, 24, 32);
    ng_physics_attach(hero, NG_PHYSICS_GRAVITY | NG_PHYSICS_SOLIDS);
    ng_physics_set_gravity(hero, NG_FP_FROM_FRAC(1, 4), NG_TO_FP(4));
    ng_physics_add_solid(0, 200, 320, 8, 0);
}
```

`CharManager::instance()` is the singleton. It owns the character
pool, runs depth sort by Y, reassigns hardware sprite slots, and
calls `upload()` only when `sprite_dirty` is set.

---

## C++ build specifics

Things this directory adds on top of the C build:

* **Singletons** for the systems that own a global pool —
  `CharManager`, `FeedbackSystem`, `ParticleSystem`, `PalFxSystem`.
  Each exposes `instance()` and works under `-fno-threadsafe-statics`
  because the NeoGeo is single-core with no threads.
* **Member methods** on the public POD-like structs (`NGCharacter`,
  `NGSpriteGroup`, `NGCamera`) for ergonomic call sites. The fields
  remain public so C code can read them directly.
* **`ng_scene.hpp` helpers** for push / pop / replace style transitions.

Everything else — including all `ng_*` extern-C entry points — is
binary-compatible with the C build.

---

## Where to go next

* The unified demo at `games/demo` exercises every public subsystem
  end-to-end. Start at `games/demo/scenes/demo.c`.
* The parallel C engine docs are in `sdk/2d_engine/README.md`.
* SDK API conventions, hardware mapping, and the ROM build pipeline
  are documented at the project root (`README.md`, `SDK_API_GUIDE.md`).
