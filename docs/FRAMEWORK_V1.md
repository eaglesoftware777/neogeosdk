# Framework v1

Framework v1 is the C 2D engine (`sdk/2d_engine`) at a fixed API. Its public calls were recorded when it was declared (v4 phases 0–2). From then on they change by **additions only**:
- a call is never removed, renamed or given a new signature;
- new calls are allowed.

```c
#include "ng_defs.h"
#if NG_FRAMEWORK_VERSION >= 0x0100      /* 1.0 */
...
#endif
```

| Macro | Value | Meaning |
|---|---|---|
| `NG_SDK_VERSION` | `0x010701` | the SDK release this tree becomes (`CHANGELOG.md`) |
| `NG_FRAMEWORK_VERSION` | `0x0101` | the engine API: 1.0 frozen, 1.1 adds raster bands |

The list is `docs/api/framework_v1.txt`: every `NEOGEO_USER` prototype in `sdk/2d_engine/*.h`.
- `make api-check` (also part of `make test`) runs `tools/api_freeze.py`. It fails when a listed prototype is missing or changed, and reports the additions.
- `python3 tools/api_freeze.py --write` records a new list. Run it only when a new framework version is declared.

The C++ engine (`sdk/2d_engine_plus`) carries the same version macros. It isn't frozen by this check.

## The frame

A game that wants every sprite, text and colour change to land in the vertical blank builds its frame in two halves:
- the logic updates only the engine's shadow state;
- a short routine right after the wait for the blank writes what changed.

Maiya's frame end, `games/maiya/scenes/maiya_game.c` (its perf counter marks left out):

```c
void maiya_vblank(void)
{
    mg_wait_vblank();       /* sleep until the blank (STOP, not a busy loop) */
    ng_vram_commit();       /* sprite groups, then FIX text cells            */
    ng_palfx_vblank();      /* the colours that changed                      */
    ng_sound_vblank();      /* the next queued byte for the Z80              */
}
```

Then the logic runs:
1. input;
2. the game's update;
3. `ng_game_engine_frame()`, whose draw hook places the scenery;
4. `ng_chars_draw()`.

None of these write video memory directly while `NG_VRAM_DEFER` is set.

## The switches

A game turns these on in its `game.mk`. Each is off by default, and a game that leaves one off builds exactly as before.

| Setting | What it builds in |
|---|---|
| `GAME_ENGINE_DEFINES += -DNG_VRAM_DEFER=1` | `ng_sprite_group_flush()` and the FIX text calls only list their writes; `ng_vram_commit()` writes them. Character hides are listed and written first. |
| `GAME_ENGINE_DEFINES += -DNG_SOUND_QUEUE=1` | Sound commands wait in a queue instead of the 68000 waiting for the Z80. A byte goes out when the driver shows ready, at least 16 lines after the one before. The delay routines empty the queue first. |
| `GAME_ENGINE_DEFINES += -DNG_PALFX_SCREEN=1` | Screen-wide fades over all of a game's palette banks (`ng_palfx_screen_*`). A middle level is blended over two frames and shown whole. |
| `GAME_OPTIMIZE = -O2` | The engine and the game's scenes at -O2. The start-up sources stay at -O0. |
| `make PERF=1` / `PERF=2` | The frame counters (`sdk/ng_perf.h`): all of them, or frame timing only. |

## Rules that keep a frame inside 264 lines

- **Write FIX text only through `ng_fix_*`.**
  - The layer remembers every cell's map word and writes only what changes. Changed cells queue as runs along a row.
  - After any direct write, such as `clearFix()` or `fixtext_out()`, call `ng_fix_clear()` or `ng_fix_invalidate_all()`.
  - `ng_scene_clean()` already does.
- **Place per-frame scenery with `ng_sprite_group_show_at()`.** It sets tile, palette and position, shows the group and flushes it, in one call.
- **Put away the rest of a pool with `ng_sprite_groups_hide_all()`.**
- **No 32-bit multiply or divide in frame work.**
  - On the 68000 they are library calls: `__mulsi3`, `__divsi3`, several hundred cycles each.
  - Multiply 16-bit values: `(uint16_t)a * (uint16_t)b`.
  - Divide with `divu.w` when the quotient fits a word (`ng_chars_index()` does).
- **Build a HUD line whole, then write it.** `ng_fix_puts()` of the finished line writes only the cells that differ. Clearing first and then writing rewrites every cell.
- **Spread rare, heavy work over frames.** Maiya's tray follows the rest of the HUD a frame later. The fade blends half its colours a frame.

## Measuring

```
python3 games/maiya/tools/build.py --perf-lite   # or --perf, every counter
python3 games/maiya/tools/perf_report.py --mame <mame>
```

The report gives game fps, the share of frames that overran, work in scanlines (average and peak), video words and how many landed on drawn lines, and the most sprites on any line. The method and results are in `docs/perf/`.

## v1.x

Additions since 1.0:
- **1.1: raster bands** (`ng_raster.h`, `docs/raster.md`, `-DNG_RASTER=1`).
- **The memory card**, in `sdk/ng_system.h`: `ng_card_save` and `ng_card_load` (`docs/saves.md`).
- **`make neo`** (`docs/MAKEFILE_INTEGRATION.md`).

Measured, not added: the strips-per-line check is `perf_report.py --all-stages --strip-limit 96`. A plane is a sprite group, and a band of one can scroll at its own pace through `ng_raster` (`docs/perf/maiya_v4_phase5.md`).
