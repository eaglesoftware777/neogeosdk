# 2D Engine — C API Reference

**Eagle Software · Neo Geo SDK v1.7.7 (Framework 1.5) · `sdk/2d_engine/`**

Every public call in the plain-C 2D engine, grouped by module. This file
is generated from the headers by `tools/gen_api_reference.py` — the
prototypes here are the prototypes that exist.

The C++ build of the same engine is documented in
[`API_2D_ENGINE_CPP.md`](./API_2D_ENGINE_CPP.md); it exposes these same
`extern "C"` entry points plus method wrappers, so the two are
link-compatible.

```c
#include "sdk/2d_engine/ng_engine.h"   /* everything */
```

Every function carries the `NEOGEO_USER` linkage tag, which places it in
the cartridge's user code section. It is omitted from the signatures
below for readability.

---

## Contents

**Core** — [ng_defs](#ng_defs) · [ng_engine](#ng_engine) · [ng_frame_sync](#ng_frame_sync) · [ng_game_events](#ng_game_events) · [ng_game_interupt](#ng_game_interupt) · [ng_game_time](#ng_game_time) · [ng_pause](#ng_pause) · [ng_progress](#ng_progress) · [ng_properties](#ng_properties) · [ng_runtime](#ng_runtime) · [ng_scene](#ng_scene) · [ng_status](#ng_status) · [ng_timers](#ng_timers)

**Math** — [ng_fixed](#ng_fixed) · [ng_palette_math](#ng_palette_math) · [ng_rand](#ng_rand) · [ng_spatial_grid](#ng_spatial_grid) · [ng_trig](#ng_trig)

**Video** — [ng_art_asset](#ng_art_asset) · [ng_bg](#ng_bg) · [ng_depthfx](#ng_depthfx) · [ng_fix](#ng_fix) · [ng_palette_assets](#ng_palette_assets) · [ng_palette_fx](#ng_palette_fx) · [ng_raster](#ng_raster) · [ng_render_queue](#ng_render_queue) · [ng_sprite_group](#ng_sprite_group) · [ng_sprite_hw](#ng_sprite_hw) · [ng_sprite_pool](#ng_sprite_pool) · [ng_sprite_window](#ng_sprite_window) · [ng_vram](#ng_vram)

**Characters** — [ng_actions](#ng_actions) · [ng_border_constraints](#ng_border_constraints) · [ng_char_render](#ng_char_render) · [ng_chars](#ng_chars) · [ng_joystick](#ng_joystick) · [ng_move](#ng_move) · [ng_npcs](#ng_npcs) · [ng_physics](#ng_physics)

**World** — [ng_camera](#ng_camera) · [ng_level](#ng_level)

**Effects** — [ng_debug](#ng_debug) · [ng_demo_advanced](#ng_demo_advanced) · [ng_feedback](#ng_feedback) · [ng_particles](#ng_particles)

**Genre engines** — [ng_beatemup](#ng_beatemup) · [ng_fight](#ng_fight) · [ng_mahjong](#ng_mahjong) · [ng_puzzle](#ng_puzzle) · [ng_rpg](#ng_rpg) · [ng_shooter](#ng_shooter)

---

# Core

## ng_defs

`sdk/2d_engine/ng_defs.h` — Sizes, engine constants, and the NEOGEO_USER linkage tag.

```c
uint8_t ng_rect_hit(NGRect a, NGRect b);
uint16_t ng_abs16(int16_t v);
```

| Constant | Value |
|---|---|
| `NG_SDK_VERSION_MAJOR` | `1` |
| `NG_SDK_VERSION_MINOR` | `7` |
| `NG_SDK_VERSION_PATCH` | `7` |
| `NG_SDK_VERSION` | `((NG_SDK_VERSION_MAJOR << 16) | (NG_SDK_VERSION_MINOR << 8) | NG_SDK_VERSION_PATCH)` |
| `NG_FRAMEWORK_VERSION_MAJOR` | `1` |
| `NG_FRAMEWORK_VERSION_MINOR` | `5` |
| `NG_FRAMEWORK_VERSION` | `((NG_FRAMEWORK_VERSION_MAJOR << 8) | NG_FRAMEWORK_VERSION_MINOR)` |
| `NG_TRUE` | `1` |
| `NG_FALSE` | `0` |
| `NG_FRAME_RATE` | `60` |
| `NG_FRAMES_PER_SEC` | `NG_FRAME_RATE` |
| `NG_SCREEN_WIDTH` | `320` |
| `NG_SCREEN_HEIGHT` | `224` |
| `NG_SPRITE_CULL_LEFT` | `-64` |
| `NG_SPRITE_CULL_RIGHT` | `384` |
| `NG_SPRITE_CULL_TOP` | `-128` |
| `NG_SPRITE_CULL_BOTTOM` | `640` |
| `NG_RENDER_BAND_BACK` | `0` |
| `NG_RENDER_BAND_NPC` | `1` |
| `NG_RENDER_BAND_ENEMY` | `2` |
| `NG_RENDER_BAND_PLAYER` | `3` |
| `NG_RENDER_BAND_FX` | `4` |
| `NG_RENDER_BAND_FRONT` | `5` |
| `NG_FP_SHIFT` | `8` |
| `NG_FP_ONE` | `(1 << NG_FP_SHIFT)` |
| `NG_MAX_CHARS` | `64` |
| `NG_MAX_CHAR_KINDS` | `128` |
| `NG_MAX_ACTIONS` | `256` |
| `NG_MAX_GAME_EVENTS` | `32` |
| `NG_MAX_TIMERS` | `64` |
| `NG_MAX_PROGRESS` | `64` |
| `NG_MAX_STATUS` | `256` |
| `NG_PROP_GROUPS` | `16` |
| `NG_PROP_COUNT` | `32` |
| `NG_MAX_BORDER_CONSTRAINTS` | `128` |
| `NG_MAX_NPCS` | `32` |
| `NG_MAX_SOLIDS` | `64` |
| `NG_FIX_WIDTH` | `40` |
| `NG_FIX_HEIGHT` | `32` |
| `NG_SPRITE_MAX_STRIPS` | `32` |
| `NG_SPRITE_MAX_HEIGHT_TILES` | `32` |
| `NG_SPRITE_FULL_XSCALE` | `0xff` |
| `NG_SPRITE_FULL_YSCALE` | `0xff` |
| `NG_PAL_FIX_BASE` | `0x00` |
| `NG_PAL_UI_BASE` | `0x04` |
| `NG_PAL_STAGE_BASE` | `0x10` |
| `NG_PAL_PLAYER_BASE` | `0x20` |
| `NG_PAL_ENEMY_BASE` | `0x30` |
| `NG_PAL_FX_BASE` | `0x40` |
| `NG_PAL_BOSS_BASE` | `0x50` |
| `NG_PAL_CUTSCENE_BASE` | `0x60` |

## ng_engine

`sdk/2d_engine/ng_engine.h` — Umbrella include. One #include pulls in every public module.

*Types and declarations only — see the header.*

## ng_frame_sync

`sdk/2d_engine/ng_frame_sync.h` — One finished frame handed from the main loop to the vertical-blank interrupt.

Publish after every video change of the frame, then wait until the interrupt has taken it; the interrupt checks its write window and re-entry guard before taking.

| Constant | Value |
|---|---|
| `NG_FRAME_SYNC_INLINE` | `static inline __attribute__((__always_inline__))` |
| `NG_FRAME_SYNC_INLINE` | `static inline` |

## ng_game_events

`sdk/2d_engine/ng_game_events.h` — A small event queue with an optional handler callback.

```c
void ng_game_events_init(void);
void ng_game_events_set_handler(NGGameEventHandler handler);
uint8_t ng_game_events_send(uint16_t id, uint16_t a, uint16_t b, uint16_t c);
uint8_t ng_game_events_read(NGGameEvent *out);
void ng_game_events_update(void);
uint8_t ng_game_events_count(void);
```

## ng_game_interupt

`sdk/2d_engine/ng_game_interupt.h` — The frame tick and the five hooks you attach your game to.

ng_game_engine_frame() runs the whole engine in a fixed order — timers, characters, actions, physics, NPCs, camera, particles, palette effects, events — and flushes the render queue during VBlank. Register your own code at the five hook points rather than reimplementing the tick.

```c
void ng_game_engine_init(void);
void ng_game_runtime_init(void);
void ng_game_engine_set_hooks( NGInteruptHook before_logic, NGInteruptHook collision_logic, NGInteruptHook after_events, NGInteruptHook before_draw, NGInteruptHook after_draw );
void ng_game_interupt_set_hooks( NGInteruptHook before_logic, NGInteruptHook collision_logic, NGInteruptHook after_events, NGInteruptHook before_draw, NGInteruptHook after_draw );
void ng_game_engine_frame(void);
void ng_game_engine_set_hitstop_freeze(uint8_t on);
void ng_game_interupt(void);
```

## ng_game_time

`sdk/2d_engine/ng_game_time.h` — Frame and stage counters at 60 Hz.

```c
void ng_game_time_init(void);
void ng_game_time_tick(void);
void ng_game_time_reset_stage(void);
void ng_game_time_stage_run(uint8_t on);
uint32_t ng_game_time_frame(void);
uint16_t ng_game_time_second(void);
uint32_t ng_game_time_stage_frame(void);
```

## ng_pause

`sdk/2d_engine/ng_pause.h` — Pause, and the freeze under pause, hitstop and slow motion.

```c
void ng_pause_set(int on);
int ng_pause_is_on(void);
void ng_pause_set_music_levels(uint8_t adpcmb, uint8_t ssg, uint8_t fm);
```

## ng_progress

`sdk/2d_engine/ng_progress.h` — Bounded 0..max counters — charge meters, load bars, HP ramps.

```c
void ng_progress_init(void);
void ng_progress_start(uint8_t id, uint16_t max);
void ng_progress_set(uint8_t id, uint16_t value);
void ng_progress_add(uint8_t id, uint16_t amount);
uint16_t ng_progress_value(uint8_t id);
uint8_t ng_progress_percent(uint8_t id);
uint8_t ng_progress_done(uint8_t id);
void ng_progress_update(void);
```

## ng_properties

`sdk/2d_engine/ng_properties.h` — A grouped int32 property matrix for game-defined values.

```c
void ng_properties_init(void);
void ng_prop_set(uint8_t group, uint8_t id, int32_t value);
int32_t ng_prop_get(uint8_t group, uint8_t id);
void ng_prop_add(uint8_t group, uint8_t id, int32_t amount);
void ng_prop_clear_group(uint8_t group);
```

## ng_runtime

`sdk/2d_engine/ng_runtime.h` — Thin alias header — includes the whole engine.

*Types and declarations only — see the header.*

## ng_scene

`sdk/2d_engine/ng_scene.h` — Scene entry/teardown helpers.

ng_scene_begin() is the "start from a known state" call: it clears hardware and engine state so nothing leaks in from the previous scene.

```c
void ng_scene_clean(uint8_t flags);
void ng_scene_clean_default(void);
void ng_scene_begin(uint8_t flags, uint8_t wait_vblank);
```

| Constant | Value |
|---|---|
| `NG_SCENE_CLEAN_FIX` | `0x01u` |
| `NG_SCENE_CLEAN_SPRITES` | `0x02u` |
| `NG_SCENE_CLEAN_CHARS` | `0x04u` |
| `NG_SCENE_CLEAN_PHYSICS` | `0x08u` |
| `NG_SCENE_CLEAN_EVENTS` | `0x10u` |
| `NG_SCENE_CLEAN_TIMERS` | `0x20u` |
| `NG_SCENE_CLEAN_PROGRESS` | `0x40u` |
| `NG_SCENE_CLEAN_PALETTEFX` | `0x80u` |
| `NG_SCENE_CLEAN_DEFAULT` | `( \` |

## ng_status

`sdk/2d_engine/ng_status.h` — A bitset of "has this happened yet" flags.

```c
void ng_status_init(void);
void ng_status_set(uint16_t status_id);
void ng_status_clear(uint16_t status_id);
uint8_t ng_status_has(uint16_t status_id);
void ng_status_toggle(uint16_t status_id);
```

## ng_timers

`sdk/2d_engine/ng_timers.h` — One-shot countdown timers, addressed by id.

```c
void ng_timers_init(void);
void ng_timer_start(uint8_t id, uint16_t frames);
void ng_timer_stop(uint8_t id);
void ng_timers_update(void);
uint8_t ng_timer_active(uint8_t id);
uint8_t ng_timer_done(uint8_t id);
uint16_t ng_timer_value(uint8_t id);
uint8_t ng_timer_percent_left(uint8_t id);
```

---

# Math

## ng_fixed

`sdk/2d_engine/ng_fixed.h` — 16.16 fixed-point helpers, sin/cos and shrink lookup tables.

There is no floating point on this target. Everything that would have been a float is 16.16 fixed-point, and every trig or scale lookup is a table.

```c
uint16_t ng_isqrt(uint32_t n);
```

## ng_palette_math

`sdk/2d_engine/ng_palette_math.h` — Colour words: channels, packing, mixing, tint and scale of a 16-colour bank.

Neo Geo colour words are not RGB555 (a dark bit and each channel's low bit sit apart). Mixing divides by 255 with shifts and adds, exactly.

*Types and declarations only — see the header.*

## ng_rand

`sdk/2d_engine/ng_rand.h` — The engine's one deterministic random generator (32-bit xorshift).

One ng_rand_seed() replays every random choice of the game and the engine in order.

```c
void ng_rand_seed(uint32_t seed);
uint16_t ng_rand(void);
uint16_t ng_rand_range(uint16_t n);
```

## ng_spatial_grid

`sdk/2d_engine/ng_spatial_grid.h` — A screen-local broad phase: 64-pixel cells, up to 16 fixed IDs.

*Types and declarations only — see the header.*

## ng_trig

`sdk/2d_engine/ng_trig.h` — Sine, cosine and atan2 in fixed point, without division.

Angles are one byte: 256 steps to the full turn, 64 a quarter turn.

```c
int16_t ng_sin(uint8_t a);
int16_t ng_cos(uint8_t a);
uint8_t ng_atan2(int16_t dy, int16_t dx);
int16_t ng_trig_mul(int16_t v, int16_t t);
```

| Constant | Value |
|---|---|
| `NG_TRIG_ONE` | `16384` |
| `NG_TRIG_SHIFT` | `14` |

---

# Video

## ng_art_asset

`sdk/2d_engine/ng_art_asset.h` — Type queries on generated artbox asset records.

```c
const NGArtAsset * ng_art_asset_find(const NGArtAsset *assets, uint16_t count, uint16_t asset_id);
uint8_t ng_art_asset_is_type(const NGArtAsset *asset, uint8_t type);
```

| Constant | Value |
|---|---|
| `NG_ART_TYPE_SCREEN` | `0u` |
| `NG_ART_TYPE_SPRITE` | `1u` |
| `NG_ART_TYPE_BACKGROUND` | `2u` |
| `NG_ART_TYPE_FIX` | `3u` |
| `NG_ART_TYPE_EFFECT` | `4u` |

## ng_bg

`sdk/2d_engine/ng_bg.h` — Two scrolling sprite background layers with parallax factors.

```c
void ng_bg_init(void);
void ng_bg_set(uint8_t layer, uint16_t screen_id, NGShowScreenFn show_fn, int16_t x0, int16_t y0, uint8_t xr, uint8_t yr, uint8_t min_crt_sz, uint8_t strips, uint16_t backdrop, int16_t parallax_x_fp, int16_t parallax_y_fp);
void ng_bg_set_by_id(uint8_t layer, uint16_t screen_id, int16_t parallax_x_fp, int16_t parallax_y_fp);
void ng_bg_clear(uint8_t layer);
void ng_bg_hide_all(void);
void ng_bg_mark_dirty(uint8_t layer);
void ng_bg_draw(int16_t camera_x, int16_t camera_y);
```

| Constant | Value |
|---|---|
| `NG_BG_LAYERS` | `2` |
| `NG_BG_LAYER0` | `0` |
| `NG_BG_LAYER1` | `1` |
| `NG_SCREEN_TABLE_MAX` | `200` |

## ng_depthfx

`sdk/2d_engine/ng_depthfx.h` — Perspective projection, Z-to-shrink, fog palettes, starfields.

The Neo Geo can only shrink sprites, so depth is expressed as reduction plus a palette swap for fog banding.

```c
void ng_depthfx_init(void);
NGProjected ng_depthfx_project(NGVec3 p, uint8_t base_palette);
void ng_depthfx_draw_group(NGSpriteGroup *group, NGVec3 p, uint8_t base_palette);
void ng_depthfx_set_fog_palettes(uint8_t near_offset, uint8_t mid_offset, uint8_t far_offset, uint8_t very_far_offset);
void ng_depthfx_advance_star(NGVec3 *p, int16_t speed, int16_t z_max, int16_t spread_x, int16_t spread_y);
NGShooterProjection ng_depthfx_shooter_project(const NGShooterCamera *camera, int16_t x, int16_t y, uint8_t band);
```

| Constant | Value |
|---|---|
| `NG_DEPTH_CX` | `160` |
| `NG_DEPTH_CY` | `112` |
| `NG_DEPTH_PAL_NEAR` | `0` |
| `NG_DEPTH_PAL_MID` | `1` |
| `NG_DEPTH_PAL_FAR` | `2` |
| `NG_DEPTH_PAL_VERY_FAR` | `3` |

## ng_fix

`sdk/2d_engine/ng_fix.h` — The FIX text layer, with a dirty-cell cache.

The layer is 40 x 32 cells of which 28 rows are visible; these helpers apply the row offset for you. Use ng_fix_blank_cell() to empty a cell — the BIOS font draws its space glyph as an opaque colour-2 plate.

```c
void ng_fix_init(void);
void ng_fix_set_ascii_base(uint16_t tile_base);
void ng_fix_invalidate_all(void);
void ng_fix_clear(void);
void ng_fix_clear_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pal);
void ng_fix_putc(uint8_t x, uint8_t y, char ch, uint8_t pal);
void ng_fix_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal);
void ng_fix_put_tile(uint8_t x, uint8_t y, uint16_t tile, uint8_t pal);
void ng_fix_put_u16(uint8_t x, uint8_t y, uint16_t value, uint8_t pal, uint16_t tile_offset);
void ng_fix_put_u32(uint8_t x, uint8_t y, uint32_t value, uint8_t pal, uint16_t tile_offset);
void ng_fix_blank_cell(uint8_t x, uint8_t y);
void ng_fix_blank_rows(uint8_t y0, uint8_t y1);
void ng_fix_commit(void);
void ng_fix_queue_drop(void);
uint8_t ng_fix_last_commit_cells(void);
```

| Constant | Value |
|---|---|
| `NG_FIX_SAFE_X` | `1` |
| `NG_FIX_SAFE_Y` | `2` |
| `NG_FIX_SAFE_WIDTH` | `38` |
| `NG_FIX_SAFE_HEIGHT` | `28` |

## ng_palette_assets

`sdk/2d_engine/ng_palette_assets.h` — Generated palette asset table lookup.

```c
const NGPaletteAsset * ng_palette_asset_find(const NGPaletteAsset *assets, uint16_t count, uint16_t asset_id);
```

## ng_palette_fx

`sdk/2d_engine/ng_palette_fx.h` — Fade, flash, pulse and colour cycle on a palette bank.

Every effect is queue-safe: the palette upload goes through the render queue, so an effect started in game logic still lands inside VBlank.

```c
void ng_palette_fx_init(void);
void ng_palette_fx_update(void);
void ng_palette_load_bank(uint8_t palette_slot, const uint16_t *pal);
uint8_t ng_palette_load_asset(const NGPaletteAsset *assets, uint16_t count, uint16_t asset_id);
void ng_palfx_upload_base(uint8_t palette_slot, const uint16_t *pal);
void ng_palfx_fade_in(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
void ng_palfx_fade_out(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
void ng_palfx_flash_white(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
void ng_palfx_flash_red(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
void ng_palfx_flash_blue(uint8_t palette_slot, const uint16_t *base_pal, uint8_t duration);
void ng_palfx_pulse(uint8_t palette_slot, const uint16_t *base_pal, uint8_t period);
void ng_palfx_cycle(uint8_t palette_slot, const uint16_t *base_pal, uint8_t start, uint8_t end);
void ng_palfx_stop(uint8_t palette_slot);
uint8_t ng_palfx_active(uint8_t palette_slot);
void ng_palfx_screen_init(uint16_t *base, uint16_t *out, uint8_t count);
void ng_palfx_screen_load(uint8_t bank, const uint16_t *colors);
const uint16_t * ng_palfx_screen_colors(uint8_t bank);
uint8_t ng_palfx_screen_ready(uint8_t first, uint8_t count);
void ng_palfx_screen_backdrop(uint16_t color);
void ng_palfx_screen_fade_out(uint8_t target, uint8_t duration);
void ng_palfx_screen_fade_in(uint8_t target, uint8_t duration);
void ng_palfx_screen_stop(void);
uint8_t ng_palfx_screen_fading(void);
void ng_palfx_vblank(void);
```

| Constant | Value |
|---|---|
| `NG_PALFX_MAX_SLOTS` | `8` |
| `NG_PALFX_NONE` | `0` |
| `NG_PALFX_FADE_IN` | `1` |
| `NG_PALFX_FADE_OUT` | `2` |
| `NG_PALFX_FLASH_WHITE` | `3` |
| `NG_PALFX_FLASH_RED` | `4` |
| `NG_PALFX_FLASH_BLUE` | `5` |
| `NG_PALFX_PULSE` | `6` |
| `NG_PALFX_CYCLE` | `7` |
| `NG_PALFX_BLACK` | `0u` |
| `NG_PALFX_WHITE` | `1u` |

## ng_raster

`sdk/2d_engine/ng_raster.h` — Raster bands: changes part way down the screen on the line timer (-DNG_RASTER=1).

C engine only; see docs/raster.md.

```c
void ng_raster_start(uint8_t lines);
void ng_raster_stop(void);
void ng_raster_set_anim_speed(uint8_t speed);
uint8_t ng_raster_band_at(int16_t y);
int16_t ng_raster_band_line(uint8_t band);
uint8_t ng_raster_last_band(void);
void ng_raster_clear(void);
void ng_raster_color(uint8_t band, uint16_t color_index, uint16_t color);
void ng_raster_vram(uint8_t band, uint16_t vram_addr, uint16_t value);
void ng_raster_vram_bands(uint8_t first, uint8_t count, uint16_t vram_addr, const uint16_t *values);
void ng_raster_vblank(void);
void ng_raster_irq(void);
NEOGEO_INTERRUPT void ng_raster_irq_handler(void);
uint16_t ng_raster_irq_count(void);
uint16_t ng_raster_vram_skips(void);
```

| Constant | Value |
|---|---|
| `NG_RASTER_MAX_COLORS` | `4` |
| `NG_RASTER_MAX_WORDS` | `2` |
| `NG_RASTER_BANDS` | `33` |

## ng_render_queue

`sdk/2d_engine/ng_render_queue.h` — A 128-slot deferred write queue, flushed inside VBlank.

The engine never writes VRAM from the game logic path. Modules post commands here and ng_render_queue_flush() drains them during the ~3 ms VBlank window. ng_rq_free() tells you how much room is left.

```c
void ng_render_queue_init(void);
void ng_rq_sprite_pos(uint16_t slot, uint16_t scb3, uint16_t scb4);
void ng_rq_sprite_shrink(uint16_t slot, uint16_t scb2);
void ng_rq_sprite_hide(uint16_t slot);
void ng_rq_scb234(uint16_t addr, uint16_t value);
void ng_rq_palette_upload(uint16_t palette_slot, const uint16_t *data);
void ng_render_queue_flush(void);
uint8_t ng_rq_free(void);
uint8_t ng_rq_pal_free(void);
```

| Constant | Value |
|---|---|
| `NG_RQ_MAX_CMDS` | `128` |
| `NG_RQ_MAX_PAL_UPLOADS` | `8` |
| `NG_RQ_SPRITE_POS` | `0` |
| `NG_RQ_SPRITE_TILE` | `1` |
| `NG_RQ_SPRITE_PAL` | `2` |
| `NG_RQ_SPRITE_SHRINK` | `3` |
| `NG_RQ_SPRITE_HIDE` | `4` |
| `NG_RQ_SCB234` | `5` |

## ng_sprite_group

`sdk/2d_engine/ng_sprite_group.h` — Dirty-flag sprite chains — write only what changed.

A group is a strip-0 anchor plus N chained strips welded together by the SCB3 sticky bit. Setting a property marks a dirty flag; the flush writes only the words that actually changed.

```c
void ng_sprite_group_init(NGSpriteGroup *g, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette);
void ng_sprite_group_mark_dirty(NGSpriteGroup *g, uint8_t dirty_flags);
void ng_sprite_group_flush(NGSpriteGroup *g);
void ng_sprite_group_set_priority(NGSpriteGroup *g, uint8_t prio);
void ng_sprite_group_set_move_first(NGSpriteGroup *g, uint8_t on);
void ng_vram_commit(void);
uint8_t ng_sprite_group_commit_small(NGSpriteGroup *g);
uint8_t ng_vram_lines_left(void);
uint8_t ng_vram_window_open(void);
void ng_sprite_group_cancel(NGSpriteGroup *g);
void ng_sprite_hide_range_queued(uint16_t firstSprite, uint16_t count);
void ng_sprite_group_hide_queued(NGSpriteGroup *g);
uint8_t ng_sprite_group_stage_map(NGSpriteGroup *g, uint8_t *next_strip);
uint8_t ng_sprite_group_present_staged(NGSpriteGroup *g);
void ng_sprite_group_detach(NGSpriteGroup *g);
uint8_t ng_sprite_stage_retire_range(uint16_t first, uint16_t count);
uint8_t ng_sprite_stage_swap(uint16_t old_first, uint8_t old_count, uint16_t first, uint8_t count, const uint16_t *scb2, const uint16_t *scb3, const uint16_t *scb4);
void ng_sprite_group_set_tile_base(NGSpriteGroup *g, uint16_t tileBase);
void ng_sprite_group_set_tile_address(NGSpriteGroup *g, uint32_t tileAddress);
void ng_sprite_group_set_tile_stride(NGSpriteGroup *g, uint16_t tileStride);
void ng_sprite_group_set_strip_column(NGSpriteGroup *g, uint8_t strip, uint16_t column);
void ng_sprite_group_set_palette(NGSpriteGroup *g, uint8_t palette);
void ng_sprite_group_set_palette_map(NGSpriteGroup *g, const uint8_t *banks);
void ng_sprite_group_set_active_rows(NGSpriteGroup *g, uint8_t activeRows);
void ng_sprite_group_set_pos(NGSpriteGroup *g, int16_t x, int16_t y);
void ng_sprite_group_move(NGSpriteGroup *g, int16_t dx, int16_t dy);
void ng_sprite_group_set_scale(NGSpriteGroup *g, uint8_t xScale, uint8_t yScale);
void ng_sprite_group_set_flip(NGSpriteGroup *g, uint8_t hflip, uint8_t vflip);
void ng_sprite_group_set_auto_anim(NGSpriteGroup *g, uint8_t autoAnim4, uint8_t autoAnim8);
void ng_sprite_group_set_visible(NGSpriteGroup *g, uint8_t visible);
void ng_sprite_group_show_at(NGSpriteGroup *g, uint16_t tileBase, uint8_t palette, int16_t x, int16_t y);
void ng_sprite_groups_hide_all(NGSpriteGroup *g, uint8_t count);
void ng_sprite_group_upload(NGSpriteGroup *g);
void ng_sprite_group_update_transform(NGSpriteGroup *g);
void ng_sprite_group_hide(NGSpriteGroup *g);
void ng_engine_init_hardware(uint16_t transparentTile);
void ng_sprite_hide_range(uint16_t firstSprite, uint16_t count);
void ng_sprite_hide_vram_base(uint16_t spriteBase, uint16_t count);
void ng_sprite_hide_all(void);
void ng_sprite_disable_hw(uint16_t spr);
void ng_sprite_disable_hw_range(uint16_t first, uint16_t count);
void ng_sprite_park_off(uint16_t spr);
void ng_sprite_park_off_range(uint16_t first, uint16_t count);
```

| Constant | Value |
|---|---|
| `NG_SGF_DIRTY_POS` | `0x01` |
| `NG_SGF_DIRTY_TILE` | `0x02` |
| `NG_SGF_DIRTY_PALETTE` | `0x04` |
| `NG_SGF_DIRTY_SHRINK` | `0x08` |
| `NG_SGF_DIRTY_VIS` | `0x10` |
| `NG_SGF_DIRTY_ALL` | `0x1F` |
| `NG_SGF_QUEUED` | `0x80` |
| `NG_SG_PRIO_NORMAL` | `0` |
| `NG_SG_PRIO_HIGH` | `1` |
| `NG_SG_PRIO_LOW` | `2` |
| `NG_SG_MOVE_FIRST` | `0x40` |
| `NG_VRAM_QUEUE_GROUPS` | `256u` |
| `NG_VRAM_QUEUE_HIGH` | `32u` |
| `NG_VRAM_QUEUE_LOW` | `128u` |
| `NG_VRAM_QUEUE_STRIPS` | `64u` |
| `NG_VRAM_DEADLINE` | `8u` |
| `NG_SPRITE_DISABLED_YREG` | `256u` |
| `NG_SPRITE_DISABLED_X` | `496u` |
| `NG_SPRITE_DISABLED_SCB3` | `((uint16_t)(NG_SPRITE_DISABLED_YREG << 7))` |
| `NG_SPRITE_BLANK_TILE` | `0xFFFFu` |
| `NG_SPRITE_BLANK_ATTR` | `0x0000u` |

## ng_sprite_hw

`sdk/2d_engine/ng_sprite_hw.h` — Sizes of a sprite drawn at a hardware shrink.

*Types and declarations only — see the header.*

## ng_sprite_pool

`sdk/2d_engine/ng_sprite_pool.h` — The hardware slot map. Constants only, no functions.

HIGHER slot number is drawn IN FRONT. This is the observed hardware direction and it is the opposite of what earlier revisions of this header claimed. Backgrounds belong at low slots, foreground effects at high ones. The legacy names NG_SPR_BG0_FIRST / NG_SPR_BG1_FIRST predate the correction and are kept only for existing code.

| Constant | Value |
|---|---|
| `NG_SPR_TOTAL` | `380` |
| `NG_SPR_LAST` | `379` |
| `NG_SPR_TITLE_FIRST` | `33` |
| `NG_SPR_TITLE_LAST` | `95` |
| `NG_SPR_CHAR_FIRST` | `96` |
| `NG_SPR_CHAR_LAST` | `223` |
| `NG_SPR_FX_FIRST` | `224` |
| `NG_SPR_FX_LAST` | `255` |
| `NG_SPR_PART_FIRST` | `256` |
| `NG_SPR_PART_LAST` | `287` |
| `NG_SPR_TEMP_FIRST` | `288` |
| `NG_SPR_TEMP_LAST` | `379` |
| `NG_SPR_BG0_FIRST` | `1` |
| `NG_SPR_BG1_FIRST` | `17` |
| `NG_SPR_BG_STRIPS` | `16` |

## ng_sprite_window

`sdk/2d_engine/ng_sprite_window.h` — Tracks the strips a variable-size sprite currently occupies.

When a sprite shrinks between frames the strips it no longer uses have to be actively hidden, or the old art stays on screen. That bookkeeping is what a window does.

```c
void ng_sprite_window_init(NGSpriteWindow *window, uint8_t owner, uint16_t first_slot, uint8_t max_strips);
void ng_sprite_window_reset(NGSpriteWindow *window);
void ng_sprite_window_set_current(NGSpriteWindow *window, uint8_t current_strips);
void ng_sprite_window_set_shape(NGSpriteWindow *window, uint8_t current_strips, uint8_t current_rows);
void ng_sprite_window_clear(NGSpriteWindow *window);
void ng_sprite_window_clear_tail(NGSpriteWindow *window);
void ng_sprite_window_hide(NGSpriteWindow *window);
```

## ng_vram

`sdk/2d_engine/ng_vram.h` — Direct VRAM sprite-slot clearing helpers.

```c
uint16_t ng_vram_sprite_slot_to_scb1(uint16_t sprite_slot);
uint16_t ng_vram_scb1_to_sprite_slot(uint16_t scb1_base);
void ng_vram_clear_sprite_slot(uint16_t sprite_slot);
void ng_vram_clear_sprite_range(uint16_t first_sprite, uint16_t count);
void ng_vram_clear_sprite_vram_base(uint16_t sprite_base, uint16_t count);
void ng_vram_clear_all_sprites(void);
```

---

# Characters

## ng_actions

`sdk/2d_engine/ng_actions.h` — Table-driven action scripts with sound and FX hooks.

```c
void ng_actions_init(void);
void ng_actions_register(uint16_t action_id, const NGActionCmd *script);
void ng_actions_set_sound_hooks(NGActionSfxHook sfx_hook, NGActionMusicHook music_hook);
void ng_actions_set_fx_hook(NGActionFxHook fx_hook);
void ng_char_action(struct NGCharacter *c, uint16_t action_id);
void ng_actions_update(struct NGCharacter *c);
```

## ng_border_constraints

`sdk/2d_engine/ng_border_constraints.h` — Keep characters inside declared screen regions.

```c
void ng_border_constraints_init(void);
void ng_border_constraints_load(NGBorderConstraint *list, uint16_t count);
void ng_border_constraints_update(void);
void ng_border_constraints_reset_used(void);
```

## ng_char_render

`sdk/2d_engine/ng_char_render.h` — The character managers' shared drawing step.

*Types and declarations only — see the header.*

## ng_chars

`sdk/2d_engine/ng_chars.h` — The character pool: 64 objects, movement, animation, hitboxes, damage.

ng_chars_draw() hands out hardware strips from NG_SPR_CHAR_FIRST upward. Two things catch people: ng_chars_count() returns the number of ACTIVE objects, not a high-water index, so it is not a safe array bound — iterate NG_MAX_CHARS and skip inactive slots. And a character is removed with ng_chars_remove(); setting a "destroy pending" life state is not acted on by the engine.

```c
void ng_chars_init(void);
void ng_chars_set_depth_sort(uint8_t enabled);
NGCharacter* chars_add(uint8_t kind, int16_t x, int16_t y);
void ng_char_set_vram_priority(NGCharacter *c, uint8_t prio);
void ng_char_set_render_mode(NGCharacter *c, uint8_t mode);
uint8_t ng_char_commit_small(NGCharacter *c);
void ng_char_pin_sprite(NGCharacter *c, uint16_t firstSprite);
void ng_chars_remove(NGCharacter *c);
void ng_chars_clear_kind(uint8_t kind);
NGCharacter* chars_find(uint8_t kind);
NGCharacter* chars_at(uint8_t index);
uint8_t ng_chars_count(void);
uint8_t ng_chars_slots_used(void);
uint8_t ng_chars_index(NGCharacter *c);
void ng_chars_set_game_interupt(uint8_t kind, NGCharInterupt fn);
void ng_chars_reset_slot(uint8_t index);
void ng_chars_begin_scene_arena(uint8_t arena_id);
void ng_chars_clear_arena(uint8_t arena_id);
void ng_chars_set_default_arena(uint8_t arena_id);
void ng_chars_set_fixed_step(uint8_t updates_per_frame);
void ng_chars_update_fixed(void);
void ng_chars_defrag_slots(void);
void ng_chars_update(void);
void ng_chars_draw(void);
void ng_char_set_sprite(NGCharacter *c, uint16_t firstSprite, uint8_t strips, uint8_t heightTiles, uint16_t tileBase, uint8_t palette);
void ng_char_set_palette_map(NGCharacter *c, const uint8_t *banks);
void ng_char_set_asset_bounds(NGCharacter *c, uint16_t tileStart, uint16_t tileEnd);
uint8_t ng_char_bind_asset(NGCharacter *c, const NGSpriteAssetView *asset);
void ng_char_set_cull_margin(NGCharacter *c, int16_t l, int16_t r, int16_t t, int16_t b);
void ng_char_set_anim_clip(NGCharacter *c, const NGAnimClip *clip);
void ng_char_anim_update(NGCharacter *c);
uint8_t ng_char_validate_asset_window(uint16_t tileBase, uint8_t strips, uint8_t rows, uint16_t stride, uint16_t tileStart, uint16_t tileEnd);
uint8_t ng_palette_claim(uint8_t palette_slot, uint8_t owner_kind);
void ng_palette_release(uint8_t palette_slot, uint8_t owner_kind);
void ng_char_set_tile_stride(NGCharacter *c, uint16_t stride);
void ng_char_set_body(NGCharacter *c, int16_t x, int16_t y, int16_t w, int16_t h);
void ng_char_set_pos(NGCharacter *c, int16_t x, int16_t y);
void ng_char_set_speed(NGCharacter *c, int16_t vx_px, int16_t vy_px);
void ng_char_set_speed_fp(NGCharacter *c, int32_t vx_fp, int32_t vy_fp);
void ng_char_add_speed_fp(NGCharacter *c, int32_t ax_fp, int32_t ay_fp);
void ng_char_set_priority(NGCharacter *c, uint8_t priority_band, int16_t depth_offset);
void ng_char_damage(NGCharacter *c, uint8_t amount);
void ng_char_heal(NGCharacter *c, uint8_t amount);
NGRect ng_char_body_rect(NGCharacter *c);
NGRect ng_char_hit_rect(NGCharacter *c);
```

| Constant | Value |
|---|---|
| `NG_CHAR_LIFE_FREE` | `0u` |
| `NG_CHAR_LIFE_ALLOCATED` | `1u` |
| `NG_CHAR_LIFE_VISIBLE` | `2u` |
| `NG_CHAR_LIFE_HIDDEN` | `3u` |
| `NG_CHAR_LIFE_DESTROY_PENDING` | `4u` |
| `NG_STRICT_RENDER_VALIDATE` | `1` |
| `NG_CHAR_RENDER_FULL` | `0u` |
| `NG_CHAR_RENDER_MOVE_ONLY` | `1u` |

## ng_joystick

`sdk/2d_engine/ng_joystick.h` — Edge-detected input, repeat, motion specials, and character control.

Poll once per frame with ng_joystick_update(), then read edges with ng_joy_pressed(). Acting on the raw held state instead fires every frame the button is down.

```c
void ng_joystick_init(void);
void ng_joystick_update(void);
const NGJoystickState *ng_joystick_state(void);
uint16_t ng_joy_down(void);
uint16_t ng_joy_pressed(void);
uint16_t ng_joy_released(void);
uint16_t ng_joy_repeat(void);
uint8_t ng_joy_direction(void);
uint8_t ng_joy_held_frames(uint16_t mask);
uint8_t ng_joy_special_qcf(uint8_t facing_left, uint16_t attack_button_mask);
uint8_t ng_joy_special_dp(uint8_t facing_left, uint16_t attack_button_mask);
void ng_joy_control_character(NGCharacter *c, const NGJoystickCharConfig *cfg);
void ng_joy_apply_hitbox(NGCharacter *c, uint8_t active_frames, int16_t x, int16_t y, int16_t w, int16_t h);
const NGJoystickCharConfig *ng_joy_default_char_config(void);
void ng_joy_set_event_handler(NGJoyEventHandler fn);
uint8_t ng_joy_pop_command(NGJoyCommand *out);
```

| Constant | Value |
|---|---|
| `NG_EVENT_JOY_JUMP` | `0x5001` |
| `NG_EVENT_JOY_FIRE_LIGHT` | `0x5002` |
| `NG_EVENT_JOY_FIRE_HEAVY` | `0x5003` |
| `NG_EVENT_JOY_HIT` | `0x5004` |
| `NG_EVENT_JOY_SPECIAL_QCF` | `0x5005` |
| `NG_EVENT_JOY_SPECIAL_DP` | `0x5006` |
| `NG_EVENT_JOY_COMMAND` | `0x5007` |

## ng_move

`sdk/2d_engine/ng_move.h` — Movement modes: swimming and free flight, steered by the stick.

```c
uint8_t ng_move_steer(NGCharacter *c, int8_t dx, int8_t dy, const NGMoveParams *p);
```

| Constant | Value |
|---|---|
| `NG_MOVE_NO_TOP` | `(-32768)` |
| `NG_MOVE_AT_TOP` | `0x01u` |
| `NG_MOVE_SURFACED` | `0x02u` |

## ng_npcs

`sdk/2d_engine/ng_npcs.h` — NPC wrappers: patrol bounds, home position, think callbacks.

```c
void ng_npcs_init(void);
void ng_npc_remove(NGNpc *npc);
uint8_t ng_npc_count(void);
void ng_npc_set_think(NGNpc *npc, NGNpcThink think, uint16_t interval_frames);
void ng_npc_set_home(NGNpc *npc, int16_t x, int16_t y);
void ng_npc_set_patrol_bounds(NGNpc *npc, int16_t min_x, int16_t max_x, int16_t min_y, int16_t max_y);
void ng_npcs_update(void);
void ng_npc_think_patrol(NGNpc *npc, NGCharacter *c);
void ng_npc_think_hover(NGNpc *npc, NGCharacter *c);
```

| Constant | Value |
|---|---|
| `NG_NPC_FLAG_PATROL_X` | `0x0001` |
| `NG_NPC_FLAG_PATROL_Y` | `0x0002` |
| `NG_NPC_FLAG_FACE_MOTION` | `0x0004` |

## ng_physics

`sdk/2d_engine/ng_physics.h` — Gravity, drag, solid rectangles, and grounded tests.

```c
void ng_physics_init(void);
void ng_physics_attach(NGCharacter *c, uint16_t flags);
void ng_physics_detach(NGCharacter *c);
void ng_physics_set_gravity(NGCharacter *c, int32_t gravity_fp, int32_t max_fall_fp);
void ng_physics_set_drag(NGCharacter *c, int32_t drag_x_fp, int32_t drag_y_fp);
void ng_physics_clear_solids(void);
uint8_t ng_physics_add_solid(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t flags);
void ng_physics_update_pre(void);
void ng_physics_resolve(void);
uint8_t ng_physics_is_grounded(NGCharacter *c);
```

| Constant | Value |
|---|---|
| `NG_PHYSICS_GRAVITY` | `0x0001` |
| `NG_PHYSICS_WORLD` | `0x0002` |
| `NG_PHYSICS_SOLIDS` | `0x0004` |
| `NG_PHYSICS_DRAG_X` | `0x0008` |
| `NG_PHYSICS_DRAG_Y` | `0x0010` |

---

# World

## ng_camera

`sdk/2d_engine/ng_camera.h` — Follow, dead zone, look-ahead, shake, cinematic pan, bounds clamp.

```c
void ng_camera_init(NGCamera *cam);
void ng_camera_set_bounds(NGCamera *cam, int16_t left, int16_t top, int16_t right, int16_t bottom);
void ng_camera_set_follow_speed(NGCamera *cam, uint8_t speed);
void ng_camera_set_dead_zone(NGCamera *cam, uint8_t half_w, uint8_t half_h);
void ng_camera_set_look_ahead(NGCamera *cam, int16_t max_x, int16_t max_y, uint8_t rate);
void ng_camera_snap(NGCamera *cam, int16_t x, int16_t y);
void ng_camera_shake(NGCamera *cam, uint8_t amp, uint8_t frames);
void ng_camera_pan_to(NGCamera *cam, int16_t dest_x, int16_t dest_y, uint8_t speed);
void ng_camera_update(NGCamera *cam, int16_t target_x, int16_t target_y, int16_t target_vx);
void ng_camera_apply(NGCamera *cam, int16_t target_x, int16_t target_y, int16_t target_vx);
```

| Constant | Value |
|---|---|
| `NG_SCREEN_W` | `320` |
| `NG_CAM_FREE` | `0x00` |
| `NG_CAM_FOLLOW` | `0x01` |
| `NG_CAM_CINEMATIC` | `0x02` |

## ng_level

`sdk/2d_engine/ng_level.h` — Level state, world bounds, scroll and camera helpers.

```c
void ng_level_init(void);
void ng_level_set_id(uint8_t level_id);
void ng_level_set_mode(uint8_t mode);
void ng_level_set_background(uint16_t screen_id);
void ng_level_set_overlay(uint16_t screen_id);
void ng_level_set_backdrop(uint16_t color);
void ng_level_set_fix_palette(uint8_t palette);
void ng_level_set_scroll(int16_t x, int16_t y);
void ng_level_move_scroll(int16_t dx, int16_t dy);
void ng_level_set_world_bounds(int16_t left, int16_t top, int16_t right, int16_t bottom);
void ng_level_set_camera(int16_t x, int16_t y, int16_t screen_w, int16_t screen_h);
void ng_level_move_camera(int16_t dx, int16_t dy, int16_t screen_w, int16_t screen_h);
void ng_level_camera_follow(int16_t target_x, int16_t target_y, int16_t screen_w, int16_t screen_h);
void ng_level_camera_input(uint16_t joystick, int16_t speed, uint8_t axes, int16_t screen_w, int16_t screen_h);
void ng_level_camera_joystick(int16_t speed, uint8_t axes, int16_t screen_w, int16_t screen_h);
void ng_level_update(void);
```

| Constant | Value |
|---|---|
| `NG_CAMERA_AXIS_X` | `0x01` |
| `NG_CAMERA_AXIS_Y` | `0x02` |
| `NG_CAMERA_AXIS_BOTH` | `(NG_CAMERA_AXIS_X | NG_CAMERA_AXIS_Y)` |

---

# Effects

## ng_debug

`sdk/2d_engine/ng_debug.h` — A FIX-layer performance overlay.

Compile with -DNG_DEBUG_PERF=1 to enable.

```c
void ng_debug_init(void);
void ng_debug_draw(void);
void ng_debug_clear(void);
```

| Constant | Value |
|---|---|
| `NG_DBG_X` | `22` |
| `NG_DBG_Y_START` | `2` |
| `NG_DBG_PAL` | `4` |
| `NG_DBG_SUPPRESS_UNUSED` | `\` |

## ng_demo_advanced

`sdk/2d_engine/ng_demo_advanced.h` — Prebuilt showcase routines used by the demo ROM.

```c
void demo_sticky_character(void);
void demo_smooth_camera(void);
void demo_palette_flash_cycle(void);
void demo_hitstop_and_shake(void);
void demo_particle_stress(void);
void demo_depth_starfield(void);
void demo_boss_depth_shrink(void);
void demo_perf_hud(void);
```

## ng_feedback

`sdk/2d_engine/ng_feedback.h` — Hitstop, shake, palette flash and a sound hook, in one call.

ng_impact_event() is the whole game-feel package for a hit. Prefer it over assembling the four effects by hand so intensity stays consistent.

```c
void ng_feedback_init(void);
void ng_feedback_set_sfx_hook(NGFeedbackSfxHook hook);
void ng_feedback_update(void);
void ng_feedback_hitstop(uint8_t frames);
uint8_t ng_feedback_is_hitstop(void);
void ng_feedback_shake(NGCamera *cam, uint8_t amp, uint8_t frames);
void ng_feedback_flash_white(uint8_t palette_slot, const uint16_t *base_pal, uint8_t frames);
void ng_feedback_flash_red(uint8_t palette_slot, const uint16_t *base_pal, uint8_t frames);
void ng_feedback_slow_motion(uint8_t frames);
uint8_t ng_feedback_is_slow_motion(void);
void ng_impact_event(uint8_t impact_kind, uint8_t palette_slot, const uint16_t *base_pal, NGCamera *cam, uint16_t sfx_id, int16_t spark_x, int16_t spark_y, uint16_t spark_tile, uint8_t spark_pal);
uint8_t ng_feedback_hitstop_remaining(void);
uint8_t ng_feedback_slow_motion_remaining(void);
```

| Constant | Value |
|---|---|
| `NG_FEEDBACK_MAX_EVENTS` | `4` |
| `NG_IMPACT_LIGHT` | `0` |
| `NG_IMPACT_MEDIUM` | `1` |
| `NG_IMPACT_HEAVY` | `2` |
| `NG_IMPACT_BOSS` | `3` |

## ng_particles

`sdk/2d_engine/ng_particles.h` — A 32-slot fixed pool with typed spawns and priority eviction.

The pool never grows and never allocates. When it is full a new spawn evicts the lowest-priority live particle, so a flood of dust cannot push out an explosion.

```c
void ng_particles_init(void);
void ng_particles_update(void);
uint16_t ng_particles_draw(uint16_t first_slot, uint16_t sprite_budget_used);
NGParticle * ng_spawn_hit_spark(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
NGParticle * ng_spawn_dust(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
NGParticle * ng_spawn_slash_trail(int16_t x, int16_t y, int8_t dir, uint16_t tile_base, uint8_t palette);
NGParticle * ng_spawn_explosion(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
NGParticle * ng_spawn_smoke(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
NGParticle * ng_spawn_magic_spark(int16_t x, int16_t y, uint16_t tile_base, uint8_t palette);
NGParticle * ng_particle_spawn(uint8_t type, uint8_t priority, int16_t x, int16_t y, int32_t vx_fp, int32_t vy_fp, uint8_t lifetime, uint16_t tile_base, uint8_t palette, uint8_t frame_count, uint8_t anim_period);
uint8_t ng_particles_count(void);
uint8_t ng_particles_count_priority(uint8_t priority);
```

| Constant | Value |
|---|---|
| `NG_PART_MAX_PARTICLES` | `32` |
| `NG_PART_PRI_CRITICAL` | `0` |
| `NG_PART_PRI_NORMAL` | `1` |
| `NG_PART_PRI_OPTIONAL` | `2` |
| `NG_PART_DUST` | `0` |
| `NG_PART_HIT_SPARK` | `1` |
| `NG_PART_SLASH_TRAIL` | `2` |
| `NG_PART_EXPLOSION` | `3` |
| `NG_PART_MAGIC_SPARK` | `4` |
| `NG_PART_SMOKE` | `5` |
| `NG_PART_WATER_SPLASH` | `6` |
| `NG_PART_METAL_HIT` | `7` |
| `NG_PART_ANIM_FRAMES` | `4` |

---

# Genre engines

## ng_beatemup

`sdk/2d_engine/ng_beatemup.h` — Belt-scrolling brawler core for one player (GAME_GENRE_ENGINE = beatemup).

Waves, a crowd director, grabs, throws, pick-ups and ridable vehicles.

```c
int ng_beatemup_init(NGBeatEmUp *b, const NGBStage *stage, const NGBFighterDef *player, uint8_t lives, uint32_t seed);
void ng_beatemup_input(NGBeatEmUp *b, uint8_t dirs, uint8_t buttons);
void ng_beatemup_update(NGBeatEmUp *b);
int ng_beatemup_spawn(NGBeatEmUp *b, const NGBFighterDef *def, int16_t x, uint8_t y, uint8_t from);
int ng_beatemup_add_prop(NGBeatEmUp *b, uint8_t kind, int16_t x, uint8_t y, uint8_t contents, uint8_t hp);
void ng_beatemup_continue(NGBeatEmUp *b, uint8_t lives);
uint8_t ng_beatemup_status(const NGBeatEmUp *b);
uint8_t ng_beatemup_event(NGBeatEmUp *b, NGBEvent *out);
uint8_t ng_beatemup_draw_list(const NGBeatEmUp *b, uint8_t *ids, uint8_t max);
uint8_t ng_beatemup_enemies_alive(const NGBeatEmUp *b);
uint8_t ng_beatemup_move_phase(const NGBeatEmUp *b, uint8_t fighter, uint8_t *frame);
uint32_t ng_beatemup_hash(const NGBeatEmUp *b);
```

## ng_fight

`sdk/2d_engine/ng_fight.h` — One-on-one fighting core (-DNG_FIGHT=1).

Buffered two-player input with motion recognition, data-driven fighters, cancels, super freeze, projectiles, teams, strikers, CPU ranks, training. See docs/fighting.md and docs/FIGHTING_V1_7_7.md.

```c
void ngf_input_reset(NGFInput *in);
void ngf_input_push(NGFInput *in, uint8_t raw, uint8_t facing_left);
uint8_t ngf_input_dir(const NGFInput *in, uint8_t ago, uint8_t facing_left);
uint8_t ngf_input_motion(const NGFInput *in, uint8_t motion, uint8_t facing_left, uint8_t window);
uint8_t ngf_input_pressed(const NGFInput *in, uint8_t mask, uint8_t buffer);
uint8_t ngf_input_released(const NGFInput *in, uint8_t mask, uint8_t buffer);
void ngf_input_consume(NGFInput *in, uint8_t mask);
uint8_t ngf_raw_from_numpad(uint8_t numpad, uint8_t facing_left);
uint8_t ngf_cancel_check(const NGFFighter *f, uint8_t to, uint8_t *rule_index);
void ngf_match_init(NGFMatch *m, const NGFFighterDef *p1, const NGFFighterDef *p2, int16_t stage_w, uint16_t seed);
void ngf_round_start(NGFMatch *m);
uint8_t ngf_set_round_seconds(NGFMatch *m, uint8_t seconds);
void ngf_set_intro_frames(NGFMatch *m, uint8_t frames);
void ngf_set_start_distance(NGFMatch *m, uint8_t half);
void ngf_match_step(NGFMatch *m, uint8_t raw1, uint8_t raw2);
void ngf_set_cpu(NGFMatch *m, uint8_t who, uint8_t level);
uint8_t ngf_set_cpu_difficulty(NGFMatch *m, uint8_t who, uint8_t difficulty);
uint8_t ngf_cpu_reaction_frames(uint8_t difficulty);
void ngf_training_init(NGFTraining *t, NGFMatch *m);
void ngf_training_reset(NGFTraining *t, NGFMatch *m);
uint8_t ngf_training_set_dummy(NGFTraining *t, NGFMatch *m, uint8_t dummy);
void ngf_training_step(NGFTraining *t, NGFMatch *m, uint8_t raw1, uint8_t raw2);
uint8_t ngf_cpu_input(NGFMatch *m, uint8_t who);
uint8_t ngf_pop_event(NGFMatch *m, NGFEvent *out);
const NGFFrame *ngf_fighter_frame(const NGFFighter *f);
const NGFFrame *ngf_projectile_frame(const NGFMatch *m, const NGFProjectile *p);
int16_t ngf_fighter_px(const NGFFighter *f);
int16_t ngf_fighter_py(const NGFFighter *f);
uint16_t ngf_distance(const NGFMatch *m);
uint32_t ngf_match_hash(const NGFMatch *m);
void ngf_match_set_mode(NGFMatch *m, uint8_t mode);
void ngf_team_set(NGFMatch *m, uint8_t who, const NGFFighterDef *f1, const NGFFighterDef *f2, const NGFFighterDef *f3);
void ngf_striker_set(NGFMatch *m, uint8_t who, const NGFFighterDef *def);
uint8_t ngf_call_striker(NGFMatch *m, uint8_t who);
uint8_t ngf_tag_switch(NGFMatch *m, uint8_t who);
```

## ng_mahjong

`sdk/2d_engine/ng_mahjong.h` — Mahjong core: four-seat Hong Kong Old Style and solitaire (GAME_GENRE_ENGINE = mahjong).

```c
void ng_mahjong_rules_default(NGMahjongRules *rules);
int ng_mahjong_init(NGMahjong *m, const NGMahjongRules *rules, uint32_t seed);
void ng_mahjong_set_cpu(NGMahjong *m, uint8_t seat, uint8_t level);
void ng_mahjong_update(NGMahjong *m);
int ng_mahjong_command(NGMahjong *m, uint8_t seat, uint8_t action, uint8_t arg);
uint16_t ng_mahjong_wants(const NGMahjong *m, uint8_t seat);
int ng_mahjong_next_hand(NGMahjong *m);
uint8_t ng_mahjong_event(NGMahjong *m, NGMahjongEvent *out);
void ng_mahjong_view(const NGMahjong *m, uint8_t seat, NGMahjongView *view);
void ng_mahjong_cpu_choose(const NGMahjongView *view, uint8_t level, uint8_t *action, uint8_t *arg);
uint8_t ng_mahjong_seat_wind(const NGMahjong *m, uint8_t seat);
int8_t ng_mahjong_shanten(const uint8_t counts[NGM_KINDS], uint8_t melds);
uint8_t ng_mahjong_is_win(const uint8_t counts[NGM_KINDS], uint8_t melds);
uint8_t ng_mahjong_score(const NGMahjong *m, uint8_t seat, uint8_t tile, uint8_t self_drawn, NGMahjongResult *r);
uint8_t ng_mahjong_tiles_ok(const NGMahjong *m);
uint32_t ng_mahjong_hash(const NGMahjong *m);
uint16_t ng_mahjong_save(const NGMahjong *m, uint8_t *buf, uint16_t size);
int ng_mahjong_load(NGMahjong *m, const uint8_t *buf, uint16_t size);
uint8_t ng_mjsol_layout_turtle(NGMahjongPos *out);
int ng_mjsol_init(NGMahjongSol *s, const NGMahjongPos *pos, uint8_t n, uint32_t seed);
uint8_t ng_mjsol_is_free(const NGMahjongSol *s, uint8_t i);
void ng_mjsol_recount(NGMahjongSol *s);
uint8_t ng_mjsol_match(uint8_t a, uint8_t b);
int ng_mjsol_select(NGMahjongSol *s, uint8_t i);
int ng_mjsol_undo(NGMahjongSol *s);
uint8_t ng_mjsol_hint(const NGMahjongSol *s, uint8_t *a, uint8_t *b);
uint8_t ng_mjsol_free_pairs(const NGMahjongSol *s);
int ng_mjsol_shuffle(NGMahjongSol *s);
uint8_t ng_mjsol_event(NGMahjongSol *s, NGMahjongSolEvent *out);
uint32_t ng_mjsol_hash(const NGMahjongSol *s);
```

## ng_puzzle

`sdk/2d_engine/ng_puzzle.h` — Board puzzle core (GAME_GENRE_ENGINE = puzzle).

See docs/GENRE_ENGINES_V1.7.7.md.

```c
int ng_puzzle_level_check(const NGPuzzleLevel *level, uint16_t *bad_index);
int ng_puzzle_init(NGPuzzle *p, const NGPuzzleLevel *level, uint32_t seed);
void ng_puzzle_reset(NGPuzzle *p);
int ng_puzzle_action(NGPuzzle *p, uint8_t cmd, int16_t a, int16_t b);
void ng_puzzle_update(NGPuzzle *p);
void ng_puzzle_settle(NGPuzzle *p);
uint8_t ng_puzzle_status(const NGPuzzle *p);
uint8_t ng_puzzle_flying(const NGPuzzle *p);
uint8_t ng_puzzle_event(NGPuzzle *p, NGPuzzleEvent *out);
uint8_t ng_puzzle_cell(const NGPuzzle *p, uint8_t x, uint8_t y);
uint8_t ng_puzzle_neighbor(const NGPuzzle *p, uint8_t cell, uint8_t dir);
uint8_t ng_puzzle_row_shifted(const NGPuzzle *p, uint8_t row);
uint8_t ng_puzzle_row_cells(const NGPuzzle *p, uint8_t row);
void ng_puzzle_cell_px(const NGPuzzle *p, uint8_t cell, uint8_t cell_px, int16_t *x, int16_t *y);
uint8_t ng_puzzle_next_dirty(const NGPuzzle *p, uint8_t from);
void ng_puzzle_dirty_clear(NGPuzzle *p);
uint8_t ng_puzzle_logic_clue(const NGPuzzle *p, uint8_t column, uint8_t index, uint8_t *runs, uint8_t max);
uint8_t ng_puzzle_logic_line_done(const NGPuzzle *p, uint8_t column, uint8_t index);
uint8_t ng_puzzle_hint(const NGPuzzle *p, uint8_t *cell, uint8_t *dir);
uint8_t ng_puzzle_has_moves(const NGPuzzle *p);
uint8_t ng_puzzle_bubble_preview(const NGPuzzle *p, uint16_t angle, NGPuzzlePoint *pts, uint8_t max, uint8_t *cell);
uint8_t ng_puzzle_bubble_target(const NGPuzzle *p, uint16_t angle);
uint8_t ng_puzzle_preview_begin(NGPuzzlePreview *preview, const NGPuzzle *p, uint16_t angle, uint32_t generation);
uint8_t ng_puzzle_preview_step(NGPuzzlePreview *preview, const NGPuzzle *p, uint32_t generation, uint8_t budget);
uint8_t ng_puzzle_bubble_hint(const NGPuzzle *p, uint16_t *angle);
uint32_t ng_puzzle_hash(const NGPuzzle *p);
uint16_t ng_puzzle_save(const NGPuzzle *p, uint8_t *buf, uint16_t size);
int ng_puzzle_load(NGPuzzle *p, const NGPuzzleLevel *level, const uint8_t *buf, uint16_t size);
uint32_t ng_puzzle_replay(NGPuzzle *p, const NGPuzzleLevel *level, uint32_t seed, const NGPuzzleAction *actions, uint16_t count);
```

## ng_rpg

`sdk/2d_engine/ng_rpg.h` — Top-down adventure and RPG core (GAME_GENRE_ENGINE = rpg).

Rooms, exits, party, items, dialogue, environmental puzzles and saves from ROM tables.

```c
int ng_rpg_world_check(const NGRpgWorld *w, uint8_t *room, uint16_t *index);
int ng_rpg_init(NGRpg *g, const NGRpgWorld *w, uint32_t seed);
int ng_rpg_party_add(NGRpg *g, const NGRpgMemberDef *def);
int ng_rpg_party_remove(NGRpg *g, uint8_t member);
int ng_rpg_enter_room(NGRpg *g, uint8_t room, uint8_t tx, uint8_t ty, uint8_t facing);
int ng_rpg_take_exit(NGRpg *g);
void ng_rpg_reset_room(NGRpg *g);
void ng_rpg_input(NGRpg *g, uint8_t dirs);
void ng_rpg_update(NGRpg *g);
int ng_rpg_interact(NGRpg *g);
int ng_rpg_use_item(NGRpg *g, uint8_t item);
int ng_rpg_strike(NGRpg *g);
int ng_rpg_switch(NGRpg *g, uint8_t member);
int ng_rpg_order(NGRpg *g, uint8_t member, uint8_t mode, uint8_t tx, uint8_t ty);
void ng_rpg_respawn(NGRpg *g);
int ng_rpg_dialogue_start(NGRpg *g, uint16_t node);
int ng_rpg_dialogue_next(NGRpg *g);
int ng_rpg_dialogue_choose(NGRpg *g, uint8_t i);
uint8_t ng_rpg_dialogue_offered(const NGRpg *g);
uint8_t ng_rpg_flag(const NGRpg *g, uint16_t flag);
void ng_rpg_set_flag(NGRpg *g, uint16_t flag, uint8_t on);
uint8_t ng_rpg_check(const NGRpg *g, const NGRpgCond *c);
void ng_rpg_run(NGRpg *g, const NGRpgAction *a);
uint8_t ng_rpg_count(const NGRpg *g, uint8_t item);
uint8_t ng_rpg_give(NGRpg *g, uint8_t item, uint8_t count);
uint8_t ng_rpg_take(NGRpg *g, uint8_t item, uint8_t count);
int ng_rpg_drop(NGRpg *g, uint8_t item, uint8_t count);
int ng_rpg_craft(NGRpg *g, uint8_t recipe);
uint8_t ng_rpg_collected(const NGRpg *g, uint8_t id);
uint8_t ng_rpg_tile(const NGRpg *g, uint8_t tx, uint8_t ty);
uint8_t ng_rpg_walkable(const NGRpg *g, uint8_t member, uint8_t tx, uint8_t ty);
uint8_t ng_rpg_object_at(const NGRpg *g, uint8_t tx, uint8_t ty);
uint8_t ng_rpg_event(NGRpg *g, NGRpgEvent *out);
uint32_t ng_rpg_hash(const NGRpg *g);
uint16_t ng_rpg_save(const NGRpg *g, uint8_t *buf, uint16_t size);
int ng_rpg_load(NGRpg *g, const uint8_t *buf, uint16_t size);
```

## ng_shooter

`sdk/2d_engine/ng_shooter.h` — Vertical shooter core (-DNG_SHOOTER=1): scroll, camera, parallax, strip budget.

See docs/vertical_shooters.md.

```c
void ng_shooter_budget_init(NGShooterStripBudget *budget, uint16_t baseline);
void ng_shooter_budget_add(NGShooterStripBudget *budget, int16_t y, uint8_t rows, uint8_t strips);
uint16_t ng_shooter_budget_peak(const NGShooterStripBudget *budget);
void ng_shooter_camera_init(NGShooterCamera *camera);
void ng_shooter_camera_set_speed(NGShooterCamera *camera, int16_t speed_q8);
void ng_shooter_camera_set_follow(NGShooterCamera *camera, uint8_t limit_x, uint8_t limit_y, uint8_t dead_zone, uint8_t shift);
void ng_shooter_camera_step(NGShooterCamera *camera, int16_t target_x, int16_t target_y);
uint8_t ng_vertical_layer_init(NGVerticalLayer *layer, uint16_t first_slot, uint8_t strips, uint8_t rows, uint16_t tile_base, uint8_t palette, const uint8_t *bank_map, int16_t origin_x);
void ng_vertical_layer_draw(NGVerticalLayer *layer, const NGShooterCamera *camera, uint16_t ratio_q8);
void ng_vertical_layer_hide(NGVerticalLayer *layer);
```

---

