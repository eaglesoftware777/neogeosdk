# Maiya's walk, run and throws on the move — 2026-10-12

## Current version: walk and run redrawn

Player feedback on the restored version: her arms looked distorted, the
palms and fists weird; shadows of her legs and knees appeared when she ran;
the movement looked generated rather than drawn. Asked for a walk and run
that read as hand-animated, the legs alternating one up and one down, using
her v1-v3 drawings as the reference.

What changed (details in `assets/authored/POSES.md`):

- **Arms**: redrawn as ten pixel drawings -- sleeve, bare upper arm, bracer
  lit along one edge, a small closed fist joined at the wrist -- instead of
  straight forearm sticks with cream discs ringed by an outline. The walk
  swings with a relaxed elbow; the run pumps with the elbow near 90 degrees.
- **Legs**: her original WALK/RUN drawings, one pair per phase, both legs in
  the same colours (the darker far leg was the "shadow"). The run is now a
  real run: contact, down with the other heel kicking up, push-off with the
  knee driving through, and a flight with both feet off the ground; the old
  push frame folded both knees together.
- **Timing**: each pose is held for the road its planted foot covers, so
  the foot no longer skates (up to 8-10 px of slip a pose before; at most 3
  at a walk and 1 at a run now). Walk holds 8/10/14/8, run 10/14/8/8.
- **Gates**: `check_hands.py`, `check_stride_details.py`,
  `tools/check_hero_frames.py` and `cute-polish.json` updated to this design
  (fists joined at the wrist, alternation, no far-leg darkening, planted-foot
  slip, arm sides from the shoulders); all pass, as do the C-ROM gate, the
  full build, the unit tests and the native AES/MVS gait run.

Working tree on `fix/maiya-arenas-eagle-bios`, HEAD `0b8273c`. Nothing is
committed or pushed. Installed in `C:\mame\neogeosdk` (previous files in
`backups\20261012-023045-walk-run-redraw`).

## Earlier: previous walk/run restored (2026-10-11)

The latest quiet-arm redraw and gait transition changes were rejected in
player testing. On the user's request, restored the exact preceding artwork,
movement code and the 14 AES/MVS/default ROM, list and NeoSD files from the
pre-correction backups. The quiet-gait report is historical and superseded.
Current restoration evidence: `build/rollback-20261011-221508/result.json`.

## What the player asked for

- Thick legs and feet, her proper head, no strange fast arm movement.
- Short walking steps between shots when a direction and fire are held.
- Her head the same as in her other moves; no ghost knee or leg flashing
  when the frames go fast; a walk and run that look natural, smooth and
  cute, not boring.
- Arms that move, alternating with her legs; a quick, smooth return to
  idle when she stops.
- Luna identical to Maiya in shape, her hair smooth, full brown.
- The MAME install always the current build, with the art build passing.

## What was wrong, and what changed

- **Thin limbs, strange head, flicking arms (2026-10-10 frames).** The legs
  were drawn as thin polygons, the walk's head had a hanging-ponytail clump,
  and fists jumped up to 21 px a frame. Rebuilt from her own pixels (see
  `assets/authored/POSES.md`): her original stride figure's head and tunic in
  every frame, native WALK/RUN legs with solid boots, arms swinging between
  the stride's reach and her original passing figure's hanging arms.
- **Head unlike her other moves.** The first rebuild used the newer RUN
  sheet's head (smaller rose, darker tunic); every throw, jump or landing
  switched to her original sheet's head. Now the walk and run use the same
  sheet as those moves.
- **"Shadow of a knee".** Two causes: the far leg was drawn a shade darker
  (a dark bent leg flashed on every second step), and the push-off's far
  knee was half hidden behind the other leg. Both legs are now full
  brightness, and the gate fails any frame where a boot is mostly hidden.
- **Sprite lurch.** Planted-foot offsets slid the sprite back and jumped it
  ~21 px forward every step. Removed: every frame keeps one hip and the
  offsets stay -40/-62. The walk clock runs 5/4 of the road (a walking
  cycle covers 64 px, her drawn step), so her planted foot barely slides.
- **Blinking out for a frame.** The character engine handed out hardware
  sprite slots in depth order each frame; whenever a creature or villager
  ahead of her came or went, her slots moved and stayed unwritten until a
  later blank (6 of 841 frames in a test run). `ng_char_pin_sprite()`
  (new, `sdk/2d_engine/ng_chars.c`) keeps a character's slots fixed at the
  top of the cast's range; Maiya's player is pinned. 0 of 841 frames now.
  `tests/sprite_deferred_test.c` reproduces the blink unpinned and checks
  the pinned slots stay written. Other games are unchanged (nothing else is
  pinned).
- **Arms.** Each fist now travels 7-10 px over a step (forward at her
  chest, down by her hip, the other way), and the arm nearest the viewer
  changes every step, so the swing alternates with her legs.
- **Stopping.** She settles feet together, arms down, for 6 frames
  (`MG_GAIT_STOP`) and is straight back in her idle pose. (A 24-frame
  side-on hold was tried and dropped.)
- **Fire on the move.** B held throws again every 20 frames. Walking or
  running, a throw (or a whip up close) plants her feet for 8 frames (12
  for the whip), drops her to a walk, and the next throw waits 24 frames;
  a press meanwhile is kept. Standing, kneeling and air throws and the
  combos are unchanged. (`mg_fire_tick`, `maiya_feel.h`.) Attract mode no
  longer holds B.
- **Luna's hair dots.** Slots 6, 12 and 14 stay light in Luna's bank; stray
  specks of them in the hair showed as dots. `tools/hair_specks.py` (used
  by the authoring and by `build_commercial_assets.py` for every hero
  frame) recolours exactly those specks: 47 pixels in 26 of the 60 existing
  frames, verified to be the only change; `legacy_manifest.json` records
  them. Luna is still Maiya's exact shapes with her own bank.

## Results (this build, installed)

| Check | Result |
| --- | --- |
| Aseprite author, save, reopen, export (17 frames) | PASS |
| Hero art gate | PASS, 17 frames, 0 failures |
| `make GAME=maiya art` C-ROM gate | PASS: 60 legacy frames match the manifest, 17 new frames pixel-exact, palettes unchanged |
| `make GAME=maiya all` | PASS, no warnings; P 67%, C 33%, S 84%, M 50%, V 95% |
| `make GAME=maiya test` (Windows MAME, 18 s) | PASS |
| `make GAME=maiya level-check` | PASS, 12 levels, 0 errors |
| All host tests (`tests/`, 82) | PASS (10 skipped) |
| API freeze | PASS (additions only) |
| Native AES and MVS behaviour runs | PASS |
| Per-frame MVS run, Maiya and Luna | 0 frames with her missing |
| `regression.py`, 12 MVS scenarios (full workspace build) | 11 PASS; `pickups` fails (below) |

The gait checks the gate makes: head and tunic identical to her stride in
every frame; at least 45 visible boot pixels per leg; no boot moves more
than 16 px and no fist more than 7 px between frames; each fist travels at
least 6 px over a step; the near arm alternates every step; grounded soles
on Y=61, flight 3+ px up; no orphan pixels; one connected figure.

`pickups` checks two item slots by index at the committed Emerald Forest's
positions; the level file in the working tree was laid out again before
this work (items moved and renumbered), so the scenario needs new targets.
Unrelated to the gait.

Native evidence: `C:\mame\neogeosdk\tests\maiya\gait-native-20261011c`
(`behaviour\{aes,mvs}`, `visual\mvs`, `luna\mvs`, flipbooks in `review`).

## Installed

`C:\mame\neogeosdk` `roms\maiya`, `tests\mvs` and `tests\aes` hold exactly
this build (every ROM file byte-identical to `roms/maiya` and the art
build). Backups of earlier installs are in `C:\mame\neogeosdk\backups\`
(`maiya-gait2-*`, `maiya-gait3-*`, `maiya-gait4-20261011-103043`).

Not done: real hardware, and a full campaign play-through.

## Small finishing pass and player feedback — 2026-10-11

The player now accepts the head, proportions and complete walk/run cycle.
This follow-up preserves them. `tools/aseprite/polish_skin.py` makes 520
hand-selected layer-map index changes from skin shade 9 to light skin 6,
softening brown hands/knees while keeping small warm shade edges. All 17
silhouettes, head/tunic pixels, boot colors, palette, pose order, root and
hold times are unchanged. Evidence: `assets/authored/skin-polish.json` and
`skin-preservation-check.json`. The reviewed maps contain hand edits newer
than the historical `author_maps.py` assembly; rebuild the Aseprite source
from those maps, not that older assembly script.

Two plausible player reports exposed local gameplay defects:

- A 48-pixel authored ledge is drawn as two 32-pixel blocks but only its
  original 48 pixels supported the player. `scenes/maiya_collision.h` now
  supplies the same block count to drawing and landing, with four pixels of
  boot-edge forgiveness on each end. No shared collision size or jump change.
- The whip checked enemies above her but had no lower height bound.
  Standing/kneeling whips now require overlap with the target's actual body.
  The Surge's direct boss hit also needs height overlap. Screen-reaching
  Secret Arts keep their intended behavior.

The existing Rising Bloom damage code was retained and verified in native
AES and MVS play: the proper directional input triggered the uppercut and
damaged/defeated a live encounter. `README.md` now lists moves, powerups and
the actual character differences: appearance, voices, projectiles and colors;
movement/damage/collision stats and inputs are shared.

`tools/build.py` also stages the existing `level_check.ignore` file so isolated
level checks use the same documented intentional placements as the main tree.
No ignore was added, edited or weakened. Initial isolated placement failures
were caused by that file being absent, not by changed levels.

| Follow-up validation | Result |
| --- | --- |
| Accepted art preservation | PASS; all 17 silhouettes and upper bodies unchanged |
| Aseprite source/export and existing strict hero gate | PASS; 17 frames, zero failures |
| Actual C-ROM gate | PASS; 17 new frames exact, 60 legacy frames and palettes unchanged |
| Gait/presentation host tests | PASS; four gait plus three presentation tests |
| `tests/test_feedback_collision.py` | PASS; real local C helpers and tier/edge boundaries |
| Isolated MVS quick build and AES snapshot build | PASS |
| Native gait inspections, AES and MVS | PASS; each 557 gait updates, all 16 travelling poses |
| Hardware player groups | PASS; 1,066 samples per edition, no missing or partial groups |
| Held fire | PASS; walking steps and attacks in both directions |
| Native combat/ledge inspections, AES and MVS | PASS; high-tier standing/kneeling attacks blocked, ground whip and uppercut damage, both visible edges and outside limits |
| Isolated level-check | PASS; 12 levels, zero errors, 16 existing ignores; two existing unused-ignore warnings |
| ROM budget | PASS; P 263,168/393,216; C 2,764,288/8,388,608; other banks unchanged |

The hardware inspection accounts for reversed tile columns when facing left
and the CPU's newly queued frame preceding its VBlank upload. All 20 hardware
tiles must form one complete current or immediately preceding queued frame;
it does not accept blank strips, unrelated frames or partial frame updates.
These seeded native tests are bounded regression evidence, not campaign balance
or real-board certification.

Source/build/test/evidence changes stay in Maiya. Builds reused the previously
staged Maiya SDK snapshot in `build/workspace`; AES compiled its separate copy.
The shared SDK, Sky Lance, BIOS, sound-driver sources and shared `out/` were
not rebuilt or edited by this follow-up. Native runners live in
`tools/feedback_qa.py`, `feedback_gait.lua`, `feedback_combat.lua`; test evidence
and logs are in `build/feedback-20261011/`.

The English player-comment draft is
`../maiya-player-feedback-reply-2026-10-11.txt` relative to the repository root
(absolute path
`/mnt/c/neogeo/maiya-player-feedback-reply-2026-10-11.txt`). It acknowledges the
difficulty report without claiming an untested rebalance. Current stage six
is Sacred World Tree; no level layouts or difficulty values were changed.

No commits or pushes. Updated Maiya-only installation/backup details are in
`build/feedback-20261011/install-result.json`; global MAME BUILD metadata,
launchers and Sky Lance installation are preserved.

## 2026-10-11 visible hands and alternating-leg readability

The next playtest correctly identified wrist-only arm ends: a connected arm
layer and a moving distal pixel were insufficient to establish that she had
a visible palm. `tools/aseprite/repair_hands.py` now supplies six hand-authored
indexed pixel patches, repeated at the existing integer bob for all 17 poses.
Each is a small skin-colored palm/fist connected to its existing wrist, with
the existing warm shadow and outline. There are still exactly two arm layers.
No image generation, transformations, palette additions or whole-frame flips.

The near hand shows at least 13 skin pixels in every frame. The far hand
shows at least four; the rear passing hand is partly behind her skirt, rather
than being detached or omitted. The far knee/thigh uses the existing warm
skin shade so the physical legs read apart when they exchange positions.
Actual boot coordinates confirm that contact A and contact B lead with opposite
physical legs, with the near arm leading opposite the near leg. No leg or boot
geometry changed. Her accepted head, torso, near-leg pixels, ground row, pose
order, palette, holds, speed and movement code remain unchanged.

The first candidate's hand movement exceeded the existing seven-pixel limit
and reduced the rear hand's swing. Its patch coordinates were corrected;
no art gate or tolerance was changed. `tools/check_hands.py` adds an explicit
visible-palm check after layer occlusion and checks actual opposite leading
boots. It rejects the pre-edit wrist-only frames with 19 failures. Both build
paths in `tools/build.py` now run the unchanged strict hero art and decoded
C-ROM checks plus this extra hand check, preventing an old C-ROM import from
silently bringing the missing hands back.

All C-ROM bytes outside the appended 17 gait/stop frame groups and the entire
generated runtime header are byte-identical to the pre-edit build. The existing
60 action/idle frames, background art, enemies, guardians, HUD and palettes
are unchanged. Isolated AES/MVS P1 recompilation also produced the same
program hashes as before. `tests/test_assets.py` had a stale 512-pixel far-layer
assertion: the accepted pre-edit renderer already uses MG_BG*_COLS for exact
1024--1168-pixel paintings. The test now pins all 12 current widths and checks
their map and tile allocations; the 512-pixel road assertion stays intact.

| Validation | Result |
| --- | --- |
| Aseprite indexed reopen/export and PNG round-trip | PASS |
| Existing hero gate | PASS; 17 frames, zero failures |
| Visible palms and actual opposite-leg contacts | PASS |
| Accepted head/torso/leg-shape/pose/palette preservation | PASS |
| Missing-hand regression using the pre-edit exports | PASS; old wrists rejected |
| Decoded C-ROM gate | PASS; 17 frames exact, 60 legacy frames and all palettes unchanged |
| All other C-ROM bytes and runtime header | PASS; byte-identical |
| Asset host tests | PASS; 3 tests |
| Gait/presentation/collision host tests | PASS; 4 + 3 + 1 tests |
| Isolated Maiya asset and AES/MVS program builds | PASS |
| Native gait, AES/MVS | PASS; each 557 gait updates and 1,066 complete hardware groups |
| Normal inputs from installed AES/MVS test folders | PASS; each 225 gameplay samples, all 8 walk and 8 run poses |
| Isolated level-check | PASS; 12 levels, zero errors, 16 existing ignores and two existing unused-ignore warnings |
| Isolated ROM budget and enforced capacity check | PASS; P 263,168, C 2,764,288, S 96,064, M 8,204, V 7,973,632 bytes |
| Maiya diff whitespace check | PASS |

The natural-input AES inspection initially reached gameplay too late for its
30-second harness deadline. The limit was extended to 45 seconds to let the
installed BIOS intro finish; the gameplay/pose assertions were unchanged.
No BIOS or product behavior was edited. Native tests are MAME evidence.

Source and previews:

- `assets/authored/maiya_walkrun.aseprite`
- `assets/authored/maiya_walk.png`, `maiya_run.png`, `maiya_stop.png`
- `assets/authored/review/contact-sheet-4x.png`, `walk.gif`, `run.gif`
- `assets/authored/hand-repair.json`, `hand-check-result.json`
- `build/hands-20261011/` contains the preserved candidate comparison,
  old-hand regression, unchanged-byte audit, fresh native inspections,
  installed-path captures, AES/MVS NeoSD images and installation record.

Updated `C:\mame\neogeosdk`: default MVS set, both AES/MVS test sets and
software lists, and both NeoSD images. Previous changed files are backed up
under `backups/maiya-hands-20261011-185245`; install metadata is
`tests/maiya/hands-20261011-185245/BUILD.json`. User launchers, configurations,
memory cards and saves are retained. Relaunch MAME to load the revised graphics.
Shared SDK/Sky Lance work was not rebuilt or edited. No commits or pushes.

## 2026-10-11: continuous recovery and idle outfit details

The recovery foot previously advanced less than one pixel from contact to
down, then jumped ahead at the next pose. The passing/push recovery shin was
nearly vertical. The run now has a tucked heel, a forward knee with the shin
folded back, then extension; the other physical leg repeats the full path.
Green sleeves, pink sleeve trim and brown wrist bands now remain visible
through walk/run/stop, matching idle's outfit. Short skin highlights retain
warm knee rims; boots carry small band highlights from the existing palette.
The accepted head, hair, torso, walking leg outlines, run contact/flight
outlines, physics, palette, frame IDs and hold distances are preserved.

Files changed in this pass:

- `tools/aseprite/refine_stride.py`: exact hand-specified indexed rows for
  six arm positions and two recovery legs, plus whole-pixel stance placements
  and existing palette material details. Reapplying it changes zero pixels.
- `assets/authored/maps/`, `maiya_walkrun.aseprite`, strips and per-layer
  exports: 1,769 native pixel writes, authored/reopened in Aseprite 1.3.18.6.
  Transparent index 0, normal layers, saved 16-slot palette, 80x64 cells.
- `assets/authored/poses.json`, `POSES.md`, `stride-details.json`: updated
  leg sources, measured bounds/feet/fists, all 17 pose specs and pixel edits.
- `tools/check_stride_details.py`: actual foot progression, folded shin,
  support stroke, sleeve/trim/wrist/boot details and preservation audit.
- `tools/art.py`, `tools/build.py`: run the extra hand/detail gates in the
  normal art and isolated build paths. Existing strict gates were retained.
- Generated game graphics and Maiya's own MAME software lists were updated.
  No shared SDK, Sky Lance, BIOS, level, enemy, HUD or action-pose source was
  edited by this pass. No commits or pushes.

Pose order and clock-distance holds:

| Mode | A half | B half | Holds |
| --- | --- | --- | --- |
| walk | contact, down, passing, up | opposite leg, same order | 12,8,10,10,12,8,10,10 |
| run | contact, down/recovery, passing/push, flight | opposite leg, same order | 8,10,8,14,8,10,8,14 |
| stop | existing 6-frame settle | idle afterwards | existing stop timer |

Each cycle totals 80 clock units. Walking covers 64 road pixels, running 80.
In each run half the recovering boot progresses X=25.9,31.3,42.6,53.3; the
support boot progresses X=51.8,44.0,34.0,22.0. The planted contact/down
strokes differ from the 8/10-pixel holds by 0.23/0.02 pixels between pose
samples. The recovery shin folds back 3.19 pixels. The new gate rejects
the previous nearly stationary recovery and straight-shin drawings.

Validation:

- Aseprite reopen, indexed PNG round-trip, strict 17-frame anatomy gate,
  visible palms and continuous recovery/outfit gate: PASS, zero failures.
- Decoded C-ROM: all 17 authored frames pixel-exact; 60 legacy frames and
  existing Maiya/Luna palettes unchanged. Header byte-identical. Only gait
  tile bytes changed (903 C1 bytes, 713 C2 bytes); all other graphics match.
- Isolated MVS/AES scene compilation and linking: PASS. Reused the frozen
  Maiya SDK objects. Shared-root `make all` and interactive `make test` were
  not invoked, to keep this work independent of the concurrent SDK session.
- Native MAME gait inspection, both platforms: PASS; 557 gait updates,
  1,066 complete 20-tile groups, all 16 walk/run poses, full hardware scale,
  zero blank/stale groups, held-fire steps/attacks in both directions.
- Installed-path normal-input boot, both platforms: PASS; 225 gameplay
  frames each, all 8 walk and 8 run poses observed in hardware and captured.
  These captures use normal inputs, without forcing invulnerability.
- Host gait tests: 4 PASS. Host asset tests: 3 PASS.
- `make GAME=maiya level-check` in the frozen workspace: PASS, 12 levels,
  zero errors, 16 existing intentional ignores, 2 existing unused-ignore
  warnings. No ignore rules changed.
- `make GAME=maiya budget` in the frozen workspace: all within limits.
  P 263,168/393,216; C 2,764,544/8,388,608; S 96,064/114,688;
  M 8,204/16,384; V 7,973,632/8,388,608 bytes.
- `git diff --check -- games/maiya`: PASS.

Installed in `C:\mame\neogeosdk`: default MVS ROMs/software list,
`tests/mvs`, `tests/aes`, and both NeoSD images. Eleven changed installed
files were replaced; program, FIX and audio ROM bytes are unchanged.
Existing launchers/configuration were retained. Relaunch MAME to load them.
The installer ZIP in dist was not rebuilt in this art-only pass.

Backup: `/home/kepler/maiya-backups/stride-details-20261011-193027`.
A byte-verified Windows copy is `build/stride-details-20261011/before-update-backup.zip`.
An archive move during the full-C-drive condition left incomplete retired
files. The retired packages/backups were restored from preserved, hash-checked
ROM sources; original package headers/program/audio were preserved. Retired
native inspections were replayed and labelled as recovery, with every copy
hash-verified. See `build/stride-details-20261011/archive-repair.json`.
The active installed game was not reverted during recovery.

Source/review paths:

- `assets/authored/maiya_walkrun.aseprite`
- `assets/authored/maiya_walk.png`, `maiya_run.png`, `maiya_stop.png`
- `assets/authored/review/contact-sheet-4x.png`, `walk.gif`, `run.gif`
- `assets/authored/review/detail-comparison-8x.png`
- `build/stride-details-20261011/native/{mvs,aes}/gait/`
- `build/stride-details-20261011/installed-boot/{mvs,aes}/`
- `build/stride-details-20261011/install-result.json`, `installed-boot.json`

Final visual acceptance remains with the player; contact sheets, GIFs and
normal installed gameplay captures are supplied. No failing asset gate remains.

## 2026-10-11: cute palm and limb finishing pass

Maiya and Luna now share the same small palette polish on the accepted
walk/run/stop drawings. Palms have a continuous cream top, a small warm lower
rim and a dark-brown contour. Shoulders/elbows have quieter skin highlights;
knees retain a connected warm rim, and boots have simpler internal highlights.
The far knee has a smaller highlight so the second leg remains solid and
readable. There are 679 exact native indexed colour writes, with no added,
removed or repositioned opaque pixel in any of the 102 visible layer drawings.
The accepted head, hair, rose, torso, clothing shapes, limb/joint positions,
root, bob, physics, palette, frame IDs and game timing are unchanged.
Luna uses the same sprite indices and her existing hardware palette.

The approved distance clock remains: walk contact/down/passing/up A then B,
holds 12,8,10,10,12,8,10,10; run contact/down/push/flight A then B, holds
8,10,8,14,8,10,8,14. Each cycle is 80 clock units. Walking covers 64 road
pixels (32 game updates at WALK_SPEED); running covers 80 (25.6 updates at
RUN_SPEED). Arms, knees and feet stay on the same selected pose and clock.
No interpolated, blended, ghost or additional intermediate limb is introduced.

The GIF previews now redraw a complete opaque review background for every
frame, with disposal 2 and cumulative 10ms deadlines. This preserves the
intended preview tempo instead of discarding each hold's fractional tail.
Maiya and Luna share exactly the same preview durations: walk 530ms, run
430ms, within 3.4ms of the ideal nominal 60Hz cycles. Every decoded GIF frame
is pixel-exact against its complete review drawing. Runtime holds were kept.

Files changed:

- `tools/aseprite/polish_cute.py`: exact palm rows and small existing-palette
  material patches; idempotent (679 writes, then zero on reapplication).
- `assets/authored/maps/`, `maiya_walkrun.aseprite`, indexed strips and layer
  exports: authored and reopened in Aseprite 1.3.18.6-x64. Saved indexed 80x64
  cells, transparent index 0, normal layers, unchanged saved 16-slot palette.
- `assets/authored/cute-polish.json`, `cute-check-result.json`,
  `preview-timing.json`, `POSES.md` and `review/`: patch evidence and review.
- `tools/check_cute_polish.py`: reconstructs every preceding layer from the
  exact writes; verifies hashes, silhouettes, upper body, motion metadata
  and reopened exports. Produces both characters' contact sheets and GIFs.
- `tools/art.py`, `tools/build.py`: include the new preservation check after
  the existing strict anatomy, hand and stride checks. No gate was weakened.
- `tools/review_gait.py`: normal-input installed review with fresh DWARF,
  actual character selection, 20-tile hardware inspection and palette reads.
- Generated Maiya graphics and its own software lists/test packages.

Verification results:

- Aseprite indexed author/reopen/export and PNG round-trip: PASS.
- Existing strict anatomy gate: all 17 frames, zero failures. Visible palms,
  opposing contacts, continuous knee/foot recovery and outfit gates: PASS.
- New preservation gate: all 102 layer silhouettes and accepted metadata
  exact. Maiya/Luna GIF complete-frame pixel/timing comparison: PASS.
- Decoded C-ROM gate: 17 authored frames pixel-exact; 60 legacy frames and
  both palettes unchanged. The full asset header is byte-identical. Only
  appended gait tile bytes changed: 505 C1 bytes and 514 C2 bytes; every
  other graphics byte is unchanged. See `preservation-result.json`.
- Isolated AES/MVS scene compilation/linking using frozen SDK objects: PASS.
  Both P1 programs are byte-identical to the preceding accepted installation.
  Shared-root `make all` and interactive `make test` were not invoked.
- Host gait tests: 4 PASS. Host asset tests: 3 PASS.
- Native stress inspection on each platform: 557 gait updates, all 16 poses,
  1,066 complete hardware body groups, full gait scale, zero blank/stale/mixed
  groups, held-fire movement/attacks in both directions: PASS.
- Installed normal-input review, Maiya and Luna on AES and MVS: 225 gameplay
  frames per combination (900 total), all 8 walk and 8 run hardware poses,
  900 complete body groups, stable expected ordinary palettes, full gait
  scale: PASS. No veil or game-memory writes were used. Luna's chooser test
  initially held Right before the chooser consumed opening inputs. The
  harness now releases/represses after the fade and checks the selected
  heroine explicitly; the original failed traces remain as diagnostics.
- `make GAME=maiya level-check` in the frozen workspace: 12 levels, zero
  errors, 16 existing intentional ignores and 2 existing unused-ignore
  warnings. Ignore rules and level source were not changed.
- `make GAME=maiya budget` in the frozen workspace: PASS. P 263,168/393,216;
  C 2,764,544/8,388,608 (21,598 tiles); S 96,064/114,688;
  M 8,204/16,384; V 7,973,632/8,388,608 bytes.
- `git diff --check -- games/maiya`: PASS.

Installed in `C:\mame\neogeosdk`: default MVS ROMs/list, `tests/mvs`,
`tests/aes`, and both NeoSD images. Eleven game/list/image files were replaced,
plus separate current review metadata/previews. The original program, FIX,
audio, BIOS, launchers, configurations and saves were retained. Packages
were checked against their six ROM regions and software-list hashes, then
both installed platforms were booted for both characters. Relaunch MAME to
load the new art. The installer ZIP in dist was not rebuilt in this small pass.

Sky Lance and shared SDK source/output objects were not edited by this pass.
No commits or pushes. No failing art or native check remains; aesthetic review
belongs to the player.

Source/review:

- `assets/authored/maiya_walkrun.aseprite`
- `assets/authored/maiya_walk.png`, `maiya_run.png`, `maiya_stop.png`
- `assets/authored/review/contact-sheet-4x.png`
- `assets/authored/review/maiya-contact-sheet-4x.png`, `luna-contact-sheet-4x.png`
- `assets/authored/review/maiya-walk.gif`, `maiya-run.gif`, `luna-walk.gif`, `luna-run.gif`
- `build/cute-polish-20261011/native/{mvs,aes}/gait/`
- `build/cute-polish-20261011/installed-review-result.json`
- `build/cute-polish-20261011/install-result.json`

Verified preceding installed files: `/home/kepler/maiya-backups/cute-polish-20261011-210437`.
Current installed reviews: `/mnt/c/mame/neogeosdk/tests/maiya/cute-polish-20261011-210437`.
Accepted source/C-ROM backup: `/home/kepler/maiya-work/cute-polish-20261011-204720`.
