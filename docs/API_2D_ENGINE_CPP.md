# 2D Engine — C++ API Reference

**Eagle Software · Neo Geo SDK v1.7.0 · `sdk/2d_engine_plus/`**

The C++14 build of the 2D engine. This is not a different engine — it is
the same design with the same public ABI, so a C game can link against
this build and a C++ game can link against the C one.

```sh
make p1 USE_2D_PLUS=1     # link this engine
make p1                   # link the C engine
```

```cpp
#include "sdk/2d_engine_plus/ng_engine_plus.hpp"
```

## What this build adds

Every function in [`API_2D_ENGINE_C.md`](./API_2D_ENGINE_C.md) exists
here unchanged, declared `extern "C"`. On top of them this build adds
member methods and singletons, so the same work reads as
`cam.follow(...)` and `CharManager::instance()` instead of a free
function plus an explicit pointer. The wrappers are `inline` and compile
to the same hardware writes — there is no runtime cost and no vtable.

Compiled under `-std=c++14 -fno-exceptions -fno-rtti
-fno-threadsafe-statics -ffreestanding -march=68000`. No standard
library, no `malloc`, no floating point, and no global constructors that
depend on initialisation order.

## Choosing between the builds

| Build | Choose it when |
|---|---|
| `sdk/2d_engine` | You are writing C. Smallest code-size footprint and the simplest call chain. |
| `sdk/2d_engine_plus` | You are writing C++. You want member methods, singletons, and the scene-stack helpers. |

One engine per ROM. `games/demo_plus` (id 778) is the smoke-test target
that links only against this build.

---

## Classes and methods

Extracted from the headers by `tools/gen_api_reference.py`. Types with no
methods are plain data structures shared with the C build.

### ng_actions.hpp

Table-driven action scripts with sound and FX hooks.

**`ActionRegistry`**

```cpp
static ActionRegistry& instance();
void NEOGEO_USER init();
void NEOGEO_USER registerScript(uint16_t action_id, const NGActionCmd *script);
const NGActionCmd* NEOGEO_USER get(uint16_t action_id);
void NEOGEO_USER setSoundHooks(NGActionSfxHook sfx, NGActionMusicHook music);
void NEOGEO_USER setFxHook(NGActionFxHook fx);
void NEOGEO_USER update(struct NGCharacter *c);
NGActionSfxHook sfxHook();
NGActionMusicHook musicHook();
NGActionFxHook fxHook();
```

### ng_camera.hpp

Follow, dead zone, look-ahead, shake, cinematic pan, bounds clamp.

**`NGCamera`**

```cpp
void NEOGEO_USER init();
void NEOGEO_USER setBounds(int16_t left, int16_t top, int16_t right, int16_t bottom);
void NEOGEO_USER setFollowSpeed(uint8_t speed);
void NEOGEO_USER setDeadZone(uint8_t half_w, uint8_t half_h);
void NEOGEO_USER setLookAhead(int16_t max_x, int16_t max_y, uint8_t rate);
void NEOGEO_USER snap(int16_t x, int16_t y);
void NEOGEO_USER shake(uint8_t amp, uint8_t frames);
void NEOGEO_USER panTo(int16_t dest_x, int16_t dest_y, uint8_t speed);
void NEOGEO_USER update(int16_t target_x, int16_t target_y, int16_t target_vx);
void NEOGEO_USER apply(int16_t target_x, int16_t target_y, int16_t target_vx);
int16_t NEOGEO_USER worldToScreenX(int16_t world_x);
int16_t NEOGEO_USER worldToScreenY(int16_t world_y);
```

### ng_chars.hpp

The character pool: 64 objects, movement, animation, hitboxes, damage.

**`NGCharacter`**

```cpp
void NEOGEO_USER setPos(int16_t x, int16_t y);
void NEOGEO_USER setSpeed(int16_t vx_px, int16_t vy_px);
void NEOGEO_USER setSpeedFp(int32_t vx_fp, int32_t vy_fp);
void NEOGEO_USER addSpeedFp(int32_t ax_fp, int32_t ay_fp);
void NEOGEO_USER setSprite(uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette);
void NEOGEO_USER setAssetBounds(uint16_t tileStart, uint16_t tileEnd);
uint8_t NEOGEO_USER bindAsset(const NGSpriteAssetView *asset);
void NEOGEO_USER setCullMargin(int16_t l, int16_t r, int16_t t, int16_t b);
void NEOGEO_USER setAnimClip(const NGAnimClip *clip);
void NEOGEO_USER animUpdate();
void NEOGEO_USER setTileStride(uint16_t stride);
void NEOGEO_USER setBody(int16_t x, int16_t y, int16_t w, int16_t h);
void NEOGEO_USER setPriority(uint8_t band, int16_t offset);
void NEOGEO_USER damage(uint8_t amount);
void NEOGEO_USER heal(uint8_t amount);
NGRect NEOGEO_USER bodyRect();
NGRect NEOGEO_USER hitRect();
```

**`CharManager`**

```cpp
static CharManager& instance();
void NEOGEO_USER init();
NGCharacter* NEOGEO_USER add(uint8_t kind, int16_t x, int16_t y);
void NEOGEO_USER remove(NGCharacter *c);
void NEOGEO_USER clearKind(uint8_t kind);
void NEOGEO_USER resetSlot(uint8_t index);
void NEOGEO_USER clearArena(uint8_t arena_id);
void NEOGEO_USER setDefaultArena(uint8_t arena_id);
void NEOGEO_USER setFixedStep(uint8_t updates_per_frame);
void NEOGEO_USER updateFixed();
void NEOGEO_USER defragSlots();
NGCharacter* NEOGEO_USER find(uint8_t kind);
NGCharacter* NEOGEO_USER at(uint8_t index);
uint8_t NEOGEO_USER count();
uint8_t NEOGEO_USER indexOf(const NGCharacter *c);
void NEOGEO_USER setInterrupt(uint8_t kind, NGCharInterupt fn);
void NEOGEO_USER update();
void NEOGEO_USER draw();
void NEOGEO_USER clearUploadSlot(uint8_t index);
```

### ng_feedback.hpp

Hitstop, shake, palette flash and a sound hook, in one call.

**`FeedbackSystem`**

```cpp
static FeedbackSystem& instance();
void NEOGEO_USER init();
void NEOGEO_USER setSfxHook(NGFeedbackSfxHook hook);
void NEOGEO_USER update();
void NEOGEO_USER hitstop(uint8_t frames);
uint8_t NEOGEO_USER isHitstop();
uint8_t NEOGEO_USER hitstopRemaining();
void NEOGEO_USER shake(NGCamera *cam, uint8_t amp, uint8_t frames);
void NEOGEO_USER flashWhite(uint8_t palette_slot, const uint16_t *base_pal, uint8_t frames);
void NEOGEO_USER flashRed(uint8_t palette_slot, const uint16_t *base_pal, uint8_t frames);
void NEOGEO_USER slowMotion(uint8_t frames);
uint8_t NEOGEO_USER isSlowMotion();
uint8_t NEOGEO_USER slowMotionRemaining();
void NEOGEO_USER impactEvent(uint8_t impact_kind, uint8_t palette_slot, const uint16_t *base_pal, NGCamera *cam, uint16_t sfx_id, int16_t spark_x, int16_t spark_y, uint16_t spark_tile, uint8_t spark_pal);
```

### ng_npcs.hpp

NPC wrappers: patrol bounds, home position, think callbacks.

**`NpcManager`**

```cpp
static NpcManager& instance();
void NEOGEO_USER init();
NGNpc* NEOGEO_USER spawn(uint8_t npc_kind, uint8_t char_kind, int16_t x, int16_t y);
void NEOGEO_USER remove(NGNpc *npc);
NGNpc* NEOGEO_USER at(uint8_t index);
NGNpc* NEOGEO_USER find(uint8_t npc_kind);
NGCharacter* NEOGEO_USER charOf(NGNpc *npc);
uint8_t NEOGEO_USER count();
void NEOGEO_USER setThink(NGNpc *npc, NGNpcThink think, uint16_t interval_frames);
void NEOGEO_USER setHome(NGNpc *npc, int16_t x, int16_t y);
void NEOGEO_USER setPatrolBounds(NGNpc *npc, int16_t min_x, int16_t max_x, int16_t min_y, int16_t max_y);
void NEOGEO_USER update();
static void NEOGEO_USER thinkPatrol(NGNpc *npc, NGCharacter *c);
static void NEOGEO_USER thinkHover(NGNpc *npc, NGCharacter *c);
```

### ng_palette_fx.hpp

Fade, flash, pulse and colour cycle on a palette bank.

**`PaletteFxSystem`**

```cpp
static PaletteFxSystem& instance();
void NEOGEO_USER init();
void NEOGEO_USER update();
void NEOGEO_USER uploadBase(uint8_t palette_slot, const uint16_t *pal);
void NEOGEO_USER fadeIn(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
void NEOGEO_USER fadeOut(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
void NEOGEO_USER flashWhite(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
void NEOGEO_USER flashRed(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
void NEOGEO_USER flashBlue(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
void NEOGEO_USER pulse(uint8_t palette_slot, const uint16_t *base_pal, uint8_t period);
void NEOGEO_USER cycle(uint8_t palette_slot, const uint16_t *base_pal, uint8_t start, uint8_t end);
void NEOGEO_USER stop(uint8_t palette_slot);
uint8_t NEOGEO_USER active(uint8_t palette_slot);
```

### ng_particles.hpp

A 32-slot fixed pool with typed spawns and priority eviction.

**`ParticleSystem`**

```cpp
static ParticleSystem& instance();
void NEOGEO_USER init();
void NEOGEO_USER update();
uint16_t NEOGEO_USER draw(uint16_t first_slot, uint16_t sprite_budget_used);
NGParticle* NEOGEO_USER spawn(uint8_t type, uint8_t priority, int16_t x, int16_t y, int32_t vx_fp, int32_t vy_fp, uint8_t lifetime, uint16_t tile_base, uint8_t palette, uint8_t frame_count, uint8_t anim_period);
NGParticle* NEOGEO_USER spawnHitSpark(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
NGParticle* NEOGEO_USER spawnDust(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
NGParticle* NEOGEO_USER spawnSlashTrail(int16_t x, int16_t y, int8_t dir, uint16_t tile_base, uint8_t palette);
NGParticle* NEOGEO_USER spawnExplosion(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
NGParticle* NEOGEO_USER spawnSmoke(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
NGParticle* NEOGEO_USER spawnMagicSpark(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
uint8_t NEOGEO_USER count();
uint8_t NEOGEO_USER countPriority(uint8_t priority);
```

### ng_physics.hpp

Gravity, drag, solid rectangles, and grounded tests.

**`PhysicsWorld`**

```cpp
static PhysicsWorld& instance();
void NEOGEO_USER init();
void NEOGEO_USER attach(NGCharacter *c, uint16_t flags);
void NEOGEO_USER detach(NGCharacter *c);
NGPhysicsBody* NEOGEO_USER body(NGCharacter *c);
void NEOGEO_USER setGravity(NGCharacter *c, int32_t gravity_fp, int32_t max_fall_fp);
void NEOGEO_USER setDrag(NGCharacter *c, int32_t drag_x_fp, int32_t drag_y_fp);
void NEOGEO_USER clearSolids();
uint8_t NEOGEO_USER addSolid(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t flags);
void NEOGEO_USER updatePre();
void NEOGEO_USER resolve();
uint8_t NEOGEO_USER isGrounded(NGCharacter *c);
```

### ng_sprite_group.hpp

Dirty-flag sprite chains — write only what changed.

**`NGSpriteGroup`**

```cpp
void NEOGEO_USER init(uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette);
void NEOGEO_USER markDirty(uint8_t flags);
void NEOGEO_USER flush();
void NEOGEO_USER setTileBase(uint16_t tileBase);
void NEOGEO_USER setTileStride(uint16_t tileStride);
void NEOGEO_USER setPalette(uint8_t palette);
void NEOGEO_USER setActiveRows(uint8_t rows);
void NEOGEO_USER setPos(int16_t x, int16_t y);
void NEOGEO_USER move(int16_t dx, int16_t dy);
void NEOGEO_USER setScale(uint8_t xScale, uint8_t yScale);
void NEOGEO_USER setFlip(uint8_t hflip, uint8_t vflip);
void NEOGEO_USER setAutoAnim(uint8_t aa4, uint8_t aa8);
void NEOGEO_USER setVisible(uint8_t v);
void NEOGEO_USER upload();
void NEOGEO_USER updateTransform();
void NEOGEO_USER hide();
static void NEOGEO_USER hideRange(uint16_t firstSprite, uint16_t count);
static void NEOGEO_USER hideVramBase(uint16_t spriteBase, uint16_t count);
static void NEOGEO_USER hideAll();
static void NEOGEO_USER initHardware(uint16_t transparentTile);
```

### ng_sprite_window.hpp

Tracks the strips a variable-size sprite currently occupies.

**`NGSpriteWindow`**

```cpp
void NEOGEO_USER init(uint8_t owner, uint16_t first_slot, uint8_t max_strips);
void NEOGEO_USER reset();
void NEOGEO_USER setCurrent(uint8_t current_strips);
void NEOGEO_USER setShape(uint8_t current_strips, uint8_t current_rows);
void NEOGEO_USER clear();
void NEOGEO_USER clearTail();
void NEOGEO_USER hide();
```

---

## Free functions

The `extern "C"` surface is identical to the C build. See
[`API_2D_ENGINE_C.md`](./API_2D_ENGINE_C.md) for the full list; the
module layout, constants, and semantics all carry over unchanged.

