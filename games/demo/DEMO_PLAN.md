# Unified Demo — Design Plan

> Status: **proposal, awaiting review**. No existing files have been
> removed or rewritten yet. Once approved, this plan replaces the
> 16-file `demo_*` collection with one linear flow in
> `games/demo/scenes/demo_unified.c`.

## Goal

A single linear demo that showcases every public capability of the C++
2D engine (`sdk/2d_engine_plus`). One play-through, no menu, no
sub-scene branching. The user presses **A** to advance through each
chapter.

The demo targets only the C++ engine. The C engine remains buildable
via `USE_2D_PLUS=0` against the *old* `demo_run_full_flow()` path so
nothing regresses there until we decide to retire the C demo too.

## Chapter list (14 chapters, ~3 minutes runtime)

Each chapter is a function with the signature:

```c
static uint8_t NEOGEO_USER chap_<name>(void);
```

Return non-zero if the user pressed **A to skip**; the parent
`demo_unified_run()` continues to the next chapter regardless. Inside
each chapter the loop calls `demo_frame()` and breaks on
`demo_advance_requested()` — same pattern the current demo uses.

| # | Chapter           | Demonstrates                                                                                                   |
|---|-------------------|----------------------------------------------------------------------------------------------------------------|
| 0 | `chap_boot`       | FIX text bring-up, BIOS copyright banner, palette zero.                                                        |
| 1 | `chap_title`      | Animated title screen, sprite + FIX overlay, fade-in via `ng_palette_fx`.                                      |
| 2 | `chap_fix`        | FIX layer: `ng_fix_puts`, `ng_fix_put_u32`, multi-palette text, dirty-cell cache demo (overwrite, see no glitch). |
| 3 | `chap_sprite`     | `NGSpriteGroup` chains: tile stride, sticky bit, hflip/vflip, hardware scale (`SCB2` shrink matrix).              |
| 4 | `chap_chars`      | `CharManager` + `NGActionCmd` script DSL: idle / walk / attack loop. Shows `FRAME / WAIT / LOOP / SFX`.        |
| 5 | `chap_physics`    | Gravity + drag + solid AABBs: hero jumps onto two platforms, one is `NG_SOLID_ONE_WAY` (jump-through).         |
| 6 | `chap_camera`     | Smooth follow, dead zone, look-ahead on input, shake (Z trigger), cinematic pan over a wide scroll.            |
| 7 | `chap_palette_fx` | All 4 palette FX in sequence: fade in/out, flash white, flash red, pulse, cycle 4-stop rainbow.                |
| 8 | `chap_particles`  | Sparks, dust, smoke, explosions; priority-based drop when budget exhausted (spawn 64 → engine keeps 32).      |
| 9 | `chap_feedback`   | `ng_impact_event()` composer: LIGHT / MEDIUM / HEAVY / BOSS each fire shake + flash + hitstop + spark.         |
|10 | `chap_depthfx`    | 2.5D depth projection: 8 sprites drift toward camera, shrink + fog palette swap with `z`.                      |
|11 | `chap_npcs`       | 6 NPCs with `NG_NPC_FLAG_PATROL_X` and `think` callbacks; depth-sorted by Y; crowd shows zero split artefacts. |
|12 | `chap_mini_game`  | 30-second playable segment: player walks/jumps, hits NPCs (which spawn particles + hitstop), camera scrolls.    |
|13 | `chap_credits`    | "Engine by Eagle Software" card, fade-out, return to attract.                                                  |

## File layout (proposed)

```
games/demo/scenes/
├── demo.c               (UNCHANGED — keep helpers; entry point updated to call unified runner)
├── demo.h               (UNCHANGED)
├── demo_unified.c       (NEW — all 14 chapters in one file, ~1200 lines)
├── demo_unified.h       (NEW — public entry: void demo_unified_run(void))
└── demo_*.c             (UNCHANGED for now — old scenes stay buildable so the C engine still runs.
                          We delete these in a follow-up commit once the unified flow is verified.)
```

`demo.c` exposes a new entry `demo_run_unified()` that supersedes
`demo_run_full_flow()`. Both can coexist during transition.

## Asset reuse

The unified demo uses **only** assets already in
`games/demo/artbox/sprite_meta.h` (~120 screens):

| Chapter        | Screens used         | Notes                                  |
|----------------|----------------------|----------------------------------------|
| title          | 79-92 (parade)       | Existing eyecatcher frames.            |
| chars/physics  | 73-78 (eagle)        | The frames we just fixed.              |
| chars (script) | 11-21 (warrior)      | Existing idle/walk/jump/attack.        |
| npcs/crowd     | 110-121 (cat NPCs)   | Existing.                              |
| particles      | tile_base for dust/spark via existing palette banks. | |
| depthfx        | 79-92 (parade frames at varying z). | |

No new art needed. No `artbox` regeneration required.

## Engine subsystems referenced

The unified demo touches every public module listed in
`sdk/2d_engine_plus/README.md`:

`ng_fix`, `ng_sprite_group`, `ng_chars`, `ng_actions`, `ng_npcs`,
`ng_physics`, `ng_camera`, `ng_palette_fx`, `ng_particles`,
`ng_feedback`, `ng_depthfx`, `ng_joystick`, `ng_game_events`,
`ng_render_queue`, `ng_debug` (perf overlay toggleable on Z).

Modules **not** demoed explicitly because they are passive
infrastructure (timers / progress / status / properties / border
constraints) are listed in a single "behind the scenes" line on the
credits card.

## What this plan does *not* do

* Doesn't change the SDK. Only `games/demo/` is rewritten.
* Doesn't add new assets, palettes, or audio.
* Doesn't delete the old `demo_*.c` files in the same commit — they
  remain compilable so the C engine demo keeps working until the
  next iteration.

## Open questions for you before I implement

1. **Skip behaviour.** Should **A** skip the *current chapter* (next
   one auto-starts) or skip the *whole demo* back to attract? Current
   demo uses skip-current; I propose the same.
2. **Audio.** Should each chapter set its own music track, or should
   one BGM play through the whole demo? Current demo switches per
   chapter.
3. **Perf overlay default.** Should `NG_DEBUG_PERF` be on by default
   in the unified demo build? I propose: off; toggled by holding
   **D** on the joystick during any chapter.
4. **Mini-game length.** 30 seconds was a placeholder — would 20 or
   45 sec be better?

Once you answer those (or say "your call"), I'll fill in the chapter
bodies and produce a working binary.
