# Maiya v4 and NeoGeoSDK Framework v1: the plan

The v4 directive asks for an arcade-grade engine: VBlank-only video writes,
a sprite allocator, raster effects, palette cycling, sub-pixel motion,
hardware squash and stretch, multi-plane parallax, hitstop, aligned ROMs
and saves on every system. This document maps each point onto what the
engine already has and what the hardware allows, then splits the work
into phases.

Each phase is one commit. Facts are held to `docs/platforms/CLASSIC_BASELINE.md`.

## 1. Where the engine is today (v3)

| Directive point | Today | Gap |
|---|---|---|
| VRAM writes only in blanking | Palettes already go in VBlank (`ng_palfx_vblank`). Sprites don't: the frame waits for VBlank, runs the logic, *then* draws (`ng_game_engine_draw`). Every sprite write lands mid-frame, while the picture is being drawn | **The main cause of tearing.** Phase 1 |
| Shadow SCB buffers | `NGSpriteGroup` keeps each group's state with dirty flags; v3 made characters keep theirs between frames | The flush happens at draw time, not in VBlank |
| Sprite allocator, culling | Fixed slot ranges per pool (`ng_sprite_pool.h`); characters off screen are skipped (`ng_char_render_visible`) | No per-scanline budget check |
| Hardware scaling | `ng_shrink_tab`, `ng_depthfx` (depth to shrink), `ng_sprite_group_set_scale` | Not used for squash and stretch |
| Raster effects | None | Phase 3 |
| Palette cycling | `ng_palfx_cycle`, screen-wide fades, Maiya's water | Only on whole banks, never per line |
| Sub-pixel motion | 24.8 fixed point everywhere (`NG_FP_SHIFT` 8, `int32_t` positions); no floating point anywhere | Camera and sprites round differently, which reads as jitter |
| Collision | Box tests with an x-distance early-out; ≤ 8 creatures, 6 shots | Fine at this scale (§3) |
| Object pools | Fixed arrays everywhere; the SDK has no `malloc` | Nothing to do |
| Hitstop, flash | `ng_feedback_hitstop`, hit flash, screen shake | Tune per blow |
| Palettes | Bank 0: 82 of 256 palettes in use; bank 1 unused | Room to give the heroines two palettes each, and depth to the scenery |
| Parallax | Far plane, road plane, a front plane | Phase 5 |
| ROM alignment, `.neo` | 8 MB alignment and `.neo` packing exist, but as separate scripts | Phase 7: one build target |
| Saves | MVS backup RAM block (`ng_save_*`) | AES memory card (P1 step B) |

Also measured:
- The v3 MVS build boots and plays on both the original arcade BIOS and EagleBIOS.
- The `build.py` test workspace boots to a black screen on the original BIOS. It is the harness the regression scripts use, so it gets fixed in phase 0.
- The helloworld, demo and demo_plus ROMs no longer match the current SDK. They get rebuilt with the first SDK commit.

## 2. Where the directive needs correcting for real hardware

1. **No VRAM DMA on cartridge systems.** The 68000 writes VRAM through `REG_VRAMADDR` ($3C0000), `REG_VRAMRW` ($3C0002) and `REG_VRAMMOD` ($3C0004, the step added after each write).
   - Spacing: 12 cycles between data writes, 16 cycles from a data write to an address write [W: VRAM].
2. **A full shadow of SCB1 can't be flushed in a VBlank.**
   - SCB1 is 64 words per sprite, 24,384 words for 381 sprites: at least 290,000 cycles, more than a whole frame (202,752).
   - VBlank is about 30,720 cycles.
   - So:
     - SCB2, SCB3 and SCB4 are shadowed in full: 3 × 381 words, about 15,000–18,000 cycles with auto-increment.
     - SCB1 is written only for the strips whose tiles changed, against a budget; whatever doesn't fit waits for the next VBlank.
3. **Horizontal blanking is short.** 64 of 384 pixels is 128 CPU cycles a line: room for a few register writes, not a VRAM flush.
4. **There is no H-blank interrupt.** Raster effects use the LSPC timer interrupt (`REG_LSPCMODE` bits 4–7, `REG_TIMERHIGH/LOW` $3C0008/$3C000A, acknowledged at $3C000C).
   - **Every-line interrupts are not viable:** a line is 768 cycles, and each interrupt costs 44 cycles to enter and 20 to return before any work.
   - Effects use bands of 8–16 lines.
   - The reload value must stay above 4 [W: Timer interrupt].
   - A palette write during active display shows as "snow" [W: Palettes]. Each band changes a few colours, timed into horizontal blanking.
   - MAME is off by a couple of lines here [M], so raster effects must be checked on hardware.
5. **Sprite limits:** 381 sprites a screen and 96 strips on any one line, not 384 [M: neogeo_spr].
6. **The hardware can only shrink, never enlarge.**
   - Squash and stretch: the art is drawn at full size, and the stretch squashes the other axis.
   - Horizontal shrink has 16 steps per strip; vertical has 256 steps (the L0 ROM table).
   - Chained ("sticky") strips are placed one after another by the hardware, so a multi-strip heroine shrinks without seams.
7. **16.16 fixed point instead of 24.8:** not needed for smoothness. 1/256 px is already finer than any visible step.
   - The jitter comes from the camera and the sprites rounding differently.
   - Fix: one rounding point, the camera's integer position, subtracted before rounding.
   - 16.16 is cheap on the 68000 (`swap`), so the camera and parallax may use it internally. The engine API stays 24.8 to avoid a rewrite.
8. **Spatial hash grid:** rejected. With at most 8 creatures and 6 shots, the existing x-sorted early-out costs fewer cycles than keeping a grid up to date.

## 3. The phases

Each phase builds with zero warnings and rebuilds and commits the other games' ROMs when the SDK changes. Each runs MAME regressions on MVS and AES, and reports frame time, overruns and peak strips per line.

| Phase | What | Done when |
|---|---|---|
| **0 Measure** (G1 step A) | Fix the black-screen test workspace. Engine perf counters behind `NG_DEBUG_PERF`: frame time in scanlines (from `REG_LSPCMODE`'s line counter), overruns, VRAM words written per frame, peak strips per line. A MAME script over stages 1, 2, 4, 6, a guardian, the Sky Road and the rush for 3,600 frames each | A table of today's numbers |
| **1 Two-phase frame** (G2) | The logic updates the groups' shadow state only. At the start of VBlank, `ng_vram_commit()` writes SCB2–4 changes and the dirty SCB1 strips, highest priority first (the heroine, then creatures, then scenery), within a cycle budget; the rest waits a frame. The VBlank interrupt enables the commit; the logic never touches VRAM | No VRAM writes on active lines (measured); no tearing in captures; frame time not worse |
| **2 Steady frame rate** (G1 step B) | Cut the logic below one frame on every stage (now 52–57 fps): the measured hot spots, the scan loops, divisions | 60 fps (59.19 Hz), zero overruns over 3,600 frames per stage |
| **3 Raster bands and palette work** (B2, C1, G5) | Timer interrupt bands for a sky gradient, a water line and heat haze. Colour cycling for water, lava, glow and stars. Palette banks 0/1 flipped for whole-screen changes. All off unless the stage asks | Stable on MVS and AES in MAME; listed "not verified on hardware" until tested there |
| **4 Feel** | Shrink-based squash and stretch: jump, landing, recoil, strikes. Hitstop of 1–4 frames by blow weight, with a one-frame flash. One rounding point for camera and sprites (§2.7) | No art added; motion captures compared before and after |
| **5 Parallax** | Up to three full planes. More planes are stacked in vertical bands so they never share a line. A build-time check of strips per line against 96 | Peak ≤ 96 on every stage (measured) |
| **6 Animation and art** | The run cycle (the reviewers' first complaint) and the other stiff moves: more frames from the art pipeline. Two palettes per heroine. The remaining placeholder art | Captures; C-ROM budget raised if needed (the chips hold 8 MB, the budget says 2 MB) |
| **7 Build pipeline** | `make neo GAME=maiya`: aligned C, V and P ROMs, MVS and AES `.neo` images and the MAME test sets in one step | One command builds everything a tester needs |
| **8 Saves** (P1 step B) | The platform layer's cartridge backends: MVS backup RAM as today, the AES memory card through the BIOS card routine ($C00468, its command bytes cited first) | Saves on MVS and AES in MAME |

## Phase 0 results

`docs/perf/maiya_v3_baseline.md`. Every stage overruns, on 24–89% of its frames (50–59 game fps), and 86–90% of video writes land on drawn lines. The average video traffic is small (51–99 words a frame), so the phase 1 commit fits well inside a blank. Sprites per line peak at 65–72 of 96.

## Phase 1 results

`docs/perf/maiya_v4_phase1.md`. Sprites and FIX text are written in the vertical blank (`NG_VRAM_DEFER`). The commit fits the blank in nine frames in ten (median 15,268 cycles, p90 24,290, against ~30,700). Writes still landing on drawn lines come from overrunning frames, so phase 2 finishes the job.

## Phase 2 results

`docs/perf/maiya_v4_phase2.md`. 59.0–59.2 game fps on every scenario, with work averaging 131–187 of 264 lines (29–50% headroom), against 50–59 fps before. Overruns are 0–1% of frames, almost all of them outside play: the stage start, the guardian's entrance, the healed valley's tour. In play, only the World Tree still has a few (13 of about 3,100 frames). The frame no longer waits for the Z80 (`NG_SOUND_QUEUE`). Text, fades and scenery write only what changed.

## Phase 3 results

`docs/raster.md`.
- **Raster bands** are a Framework v1.x addition, `ng_raster.h`, opt-in with `-DNG_RASTER=1`.
  - Bands are 8 or 16 lines on the LSPC timer, carrying colours and sprite-table words.
  - The timer runs only over the bands in use: one interrupt per band, about 470 cycles each in MAME.
- **Maiya:**
  - The Golden Savanna has heat haze over its horizon: 11 interrupts a frame, average 187 of 264 lines, no overruns over 1,200 frames.
  - The Sunken Reef's whole painting sways: 14 interrupts.
  - The reef is a heavy stage on its own (215 lines median, 5% of frames over budget). It is outside the seven measured scenarios.
- **Colour cycling** already exists: `ng_palfx_cycle()`, and Maiya's scenery swap.
- **Palette-bank flips are left out.** The bank select switches the CPU's access and the display together, and Maiya's whole-screen changes are fades.
- **Not verified on hardware.**

## Phase 4 results

**Squash and stretch, in Maiya's code** (`mg_squash_step`; the tables are in `games/maiya/scenes/maiya_feel.h`). It uses the hardware shrink, which only makes a sprite smaller:
- **Landing:** she is squashed to 80% of her height, back to full over six frames.
- **Springing up:** she is drawn 13/16 wide, back over four frames.
- **Struck:** both ways, over four frames.
- **Striking:** a little shorter, over three frames.

Her feet and her middle stay put: the offsets take in what the shrink removed. It holds through a hitstop. Checked frame by frame in MAME: the scale bytes and offsets, and captures of a landing and a jump.

**Hitstop by weight and the flash were already in place** (`maiya_feel.h`, `ng_impact_event`):
- light 3 frames, medium 5, heavy 8;
- a heavy blow on a creature that survives 4, Maiya struck 5, a guardian's last blow 14;
- a flash where the target has a palette bank of its own.

These are the values already played, so they are kept rather than moved to the plan's 1–4.

**One rounding point was already in place.** Maiya's camera follows in whole pixels (`MG_CAM_FOLLOW` 255), so positions are rounded once, in the physics, and the camera and sprites can't disagree.

Regressions pass as before.

## 4. Framework v1

When phases 0–2 are in, the SDK is tagged **v1.0**:
- `NG_SDK_VERSION` in `ng_defs.h`;
- the public API frozen and listed: changes after that are additions only;
- docs updated: sprite groups, the frame (logic, then commit), the perf counters, raster bands;
- every game rebuilt and committed against it.

Phases 3–8 then land as v1.x additions.

**Done (2026-10-03).** The SDK already has release v1.7.0, and this branch becomes v1.7.1 (`CHANGELOG.md`). So the version is two numbers:
- `NG_SDK_VERSION` 1.7.1;
- `NG_FRAMEWORK_VERSION` 1.0, the frozen engine API.

The API itself:
- 315 prototypes are listed in `docs/api/framework_v1.txt`.
- `make api-check`, now part of `make test`, fails on any listed call that is removed or changed.
- `docs/FRAMEWORK_V1.md` covers the frame, the switches and the rules.

The rebuild against it:
- Every game was rebuilt: P1, plus M1, whose committed copies predated the current sound protocol.
- Built from the current source with the old M1s, demo and demo_plus hung at boot waiting for the Z80. They boot again.
- helloworld and tutorial stay black in the headless test, the same as their committed ROMs.

## 5. Not verified, and kept so until checked

- Classic MVS and AES hardware: everything after phase 0, especially the raster bands (§2.4).
- The NeoSD cart: the A+D+Start and Start+Select hotkeys (an open reviewer report).
- AES+: the frame budget at its overclock, which is unknown.
- Neo Geo CD: out of scope for v4.
