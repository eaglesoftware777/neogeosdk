# Changelog

## v1.7.5 - Budgeted palette banks

Release date: 2026-09-08

### One palette per image was the ceiling, and it has been lifted

Artwork was analysed per tile and then collapsed back to a single
15-colour palette for the whole image, so most of the analysis was
discarded on the way to the ROM.  Assets now carry a budget of palette
banks: bank zero is fitted to the whole image and kept as a fallback,
and each additional bank is fitted to the tiles carrying the most
remaining error.  Tile boundaries only ever change which bank a tile
selects; they never change the source colours.

Measured against the source pixels, decoded back out of the C-ROMs
rather than from a preview, over the demo's 173 assets:

| | before | after |
|---|---|---|
| mean dE | 16.22 | 8.00 |
| median dE | 15.95 | 9.35 |
| worst dE | 51.99 | 25.46 |

Every asset improved and none regressed.  Backgrounds gain most - the
mountain page goes 8.96 to 3.11 and the forest 12.60 to 3.83 - because
their colour variety is spread across tiles, which is exactly what extra
banks address.

- `artbox/palette_banks.py` fits palettes to original pixel frequencies
  and refines them on the hardware lattice.
- `artbox/tile_codec.py` encodes and decodes C-ROM planar data, so the
  build can verify that what the ROM holds is what the pipeline meant.
- Both sprite engines take an optional per-tile palette map, so multiple
  banks reach the hardware instead of being flattened on the way.
- `tools/artbox_quality_report.py` decodes both C-ROMs, checks every
  asset against the indices it was built from, and reports colour error
  against the source.

### Where the budget stops

Extra banks are spent where they pay.  Backgrounds saturate at eight -
sixteen banks moves the mountain page from 3.18 to 3.16 - so the budget
stays there.  Sprites stay on one bank: a character's colour variety is
*within* each tile rather than spread across tiles, so eight banks move
a portrait only from 11.4 to 10.2, which does not justify the palette
RAM.  Dithering them is worse still, on both raw and blur-averaged
error, because these sources are pixel art with flat regions rather than
photographic gradients.  The engine support exists either way.

### Fixed

- The generated screen code is spliced into a game's `main.c` at a
  marker now, not at the first `showScreen1`.  The generated region grew
  file-scope palette tables that sit ahead of that function, so they
  landed on the keep side of the cut and every pipeline run left another
  copy behind - four runs in, `main.c` had four sets of every table and
  would not compile.  Repeated syncs are idempotent again.

## v1.7.4 - Tile stride, and what the palette is actually spending

Release date: 2026-09-07

### Every asset publishes its tile stride

An asset's rows are `tile_stride` tiles apart, and that stride is the
canvas width in tiles - 16 only for a 256 px import.  Six places in the
tree hardcoded 16, which was true until v1.7.3 gave Sky Lance's craft a
32 px canvas, its bosses 112, and the `neogeogame` opponent 48.  From
that point those assets were drawn by reading each row from sixteen
tiles on instead of two, seven or three: the right palette over whatever
else lives at that address.  Nineteen assets across two games.

- `tile_stride` is now a field in `NGSpriteAssetMeta`, emitted from the
  canvas width, and every bind takes it from there.
- Both engines validate an asset window against the stride the character
  will be drawn with, not against its strip count.  The two are equal
  only for an asset that fills its canvas, so the old check under-measured
  the window and passed exactly the binds it exists to catch.
- `demo_asset_scale()` converts a `U_SCALE_*` fraction - written against
  a 256 px import - into the hardware scale that asset needs from its own
  canvas, so a call site can say how big a figure should look without
  knowing what it was imported at.

### Palette

- Pure black and white are reserved only when the asset has a real
  population at that end, currently 0.4% of its opaque pixels.  A single
  dark pixel used to be enough, and an anti-aliased contour bled inward
  almost always leaves one, so two of fifteen slots went on colours
  nothing wanted.  Character art earns both; backgrounds mostly do not.

### Not done

Importing the demo's own characters at the size they are drawn - the
change that would halve what the sprite chip decimates - was attempted
twice and reverted twice.  The tile stride was one blocker and is fixed;
something else in the demo's binding still assumes a 16-tile canvas, and
the char-select portraits and the NPC chapter render wrongly without it.
`cat_sky_planes` shows the same idea working where the binding is
simpler.

## v1.7.3 - Sprites at the size they are drawn

Release date: 2026-09-06

### Import at display size, do not shrink in hardware

The sprite chip does not resample when it shrinks.  It drops whole rows and
columns, and what it drops is a dither the quantiser laid down for pixels it
expected to survive - so a sprite imported at twice its on-screen size and
halved by hardware arrives as a smear, however good the source was.  Every
asset that was being reduced at draw time is now imported at the size it is
actually drawn.

- Sky Lance draws its playfield art at full size now.  Its craft were
  imported at up to 240 px and shrunk to a quarter; new artbox rules import
  the player craft and standard opponents at 40 px and bosses at 112.  The
  player also comes down from 35x50 to 28x40, which is what it should have
  been for the playfield.
- The `neogeogame` opponent imports at 48 px instead of 32.  A 1238x800 source
  reduced by 38x had nothing left of itself; the formation opens from 34 px
  spacing to 50 to carry it, which the enemy pool's existing four-strip stride
  already had room for.
- The same treatment for the demo's own characters is **not** in this release.
  Importing them at 128 with the `U_SCALE_*` fractions doubled is measurably
  sharper, but it moves every character's strip count and tile stride, and
  `ng_char_set_sprite()` drops a bind whose asset window fails validation
  without saying so - four chapters came up with stale or missing art.  The
  ceiling stays at 256 until that path reports a rejection instead of
  swallowing it.

### Fixed in the art

- Sky Lance's three pilot portraits had a chroma-key magenta background baked
  in with no alpha channel, so the select and roster screens drew a solid pink
  slab where each pilot should be.  `artbox/fix_sprite_alpha.py` keys it out:
  it flood-fills only background reachable from the image border, so the
  figures are untouched.

### Fixed

- **A character that blinked off never came back.**  Hiding a char released
  its VRAM slots but only marked it for re-upload when it was active AND
  visible - which is the one case where nothing was hidden.  So a char hidden
  *because* it turned invisible returned through the transform-only path with
  no tilemap and no chain bits, and stayed gone.  Sky Lance's invulnerability
  flash after a hit is what this looked like in practice: get hit, lose the
  plane.
- **The `neogeogame` player alternated between two different ships.**  It
  swapped `010_ship` and `011_ship_alt` every sixteen frames as if they were
  two frames of a thrust animation.  They are two different craft, so the
  player read as morphing.
- **NPC patrol lanes overlapped.**  Four NPCs 64 px apart each patrolled 44 px
  either side, so neighbours walked through each other and the middle of the
  line read as one smeared sprite.  80 px apart, 22 either side.
- Six chapters printed their whole header onto a full-bleed backdrop.  They
  letterbox now - the FIX layer draws in front of every sprite, so a band
  under the header costs the artwork nothing that was not already covered in
  text.

### Art pipeline

- The HD conditioning passes are **on by default**.  The source art in this
  tree is HD, so conditioning is the normal case here rather than the
  exception; `ARTBOX_ENHANCE=0` or `--no-enhance` turns it off for art drawn
  at the target size that does not want its tone touched.
- `artbox/gen_starfield.py` synthesises the shooter's backdrop: a tiling
  nebula and three layers of stars, with the noise lattice wrapping so the
  page joins itself exactly rather than approximately.  Pass several outputs
  and it generates one wide page and cuts it into columns, so backdrops laid
  side by side join without a line down the middle.  The two black pages with
  a scatter of dots that `neogeogame` was using are replaced by it.

## v1.7.2 - Active characters, art fidelity, and a testable build

Release date: 2026-09-06

### Sprite height on screen

SCB3 carries the number of ACTIVE CHARACTERS, and that field is the sprite's
height on screen: the hardware covers exactly `rows * 16` scanlines with it and
does not consult the shrink register.  Vertical shrink only decides which
source row of the SCB1 map each of those scanlines reads.  A shrunk sprite that
keeps its full-size count therefore repeats itself inside a window too tall for
it, and the row lookup can reach past the last row of real art into whatever the
previous frame left there.

- Both engines now derive the active-character count from the source rows and
  the vertical shrink, and blank every map row the lookup can reach.
- That reach is bounded.  The lookup reads the first sixteen rows and only gets
  to rows 16..31 through the mirror it applies once a sprite spans more than 256
  scanlines, so under seventeen active characters half the map is unreachable.
  Uploads now stop at sixteen rows there, which is half the VRAM traffic of
  writing all thirty-two.
- `ng_sprite_group_flush()` refreshes SCB3 when the shrink or the row count
  changes - the count is derived from both, and writing SCB2 alone left it
  stale.
- A move now writes the driver strip only.  The hardware reads no position from
  a chained strip, so re-stamping every strip was most of what a scrolling
  background cost per frame.
- `tests/sprite_render_test.c` covers all of the above against a stand-in VRAM
  array, and builds both engines from the same source file so they cannot drift.

### Sky Lance chapter

The mini shooter cropped a scrolling sky into a boxed playfield.  A sprite is a
whole number of characters tall, so a window whose edge is not on a character
boundary always overhangs it by up to 15 px; the overhang is not optional, only
its destination is.  The chapter now runs the sky edge to edge, where the
overhang lands off-screen, and letterboxes the HUD onto opaque FIX bands
instead of drawing a dotted outline over the artwork.

### Art pipeline

- **Resampling in linear light.**  Every resize converted gamma-encoded sRGB
  with an arithmetic mean, which is not an average of light.  Measured across
  the demo's character set, downscaling was losing 8-22% of each sprite's
  luminance.  `resize_rgba_linear()` converts to linear light, premultiplies by
  alpha, resizes, and converts back; the same sprites now land within 2%.
- **Alpha bled before the resize, not after.**  A sprite cut out against white
  has white sitting under its transparent pixels, and a filter run before the
  bleed mixes that white into the contour - baking in the halo `alpha_bleed()`
  exists to remove.  The bleed now runs at source resolution.
- **Palette banks compared as sets.**  Bank clustering compared slot i of one
  palette against slot i of another.  These palettes come out of k-means, so
  slot order is whatever the seeding produced: two tiles holding the same
  colours in a different order scored as maximally different, took a bank each,
  exhausted the budget, and the tolerance widened until palettes that really
  were different got merged.  Clustering now uses a symmetric mean-nearest-
  neighbour distance in Lab, which reordering cannot change, and re-fits each
  bank to every colour its members hold instead of keeping whichever palette
  arrived first.  Mean dE against the pre-clustering reference, over three
  reference images: 11.4 -> 1.9, 11.7 -> 2.2, 28.2 -> 5.4.
- **Optional HD conditioning.**  `ARTBOX_ENHANCE=1` (or `--enhance`) runs an
  edge-preserving bilateral smooth and a lightness CLAHE before quantisation.
  Off by default: these are for photographic or rendered source, and running
  them over art drawn at the target size rewrites the artist's tone choices for
  no gain.
- `artbox/fix_sprite_alpha.py` accepts paths, so art under a game's own artbox
  can be repaired.  The demo's Sky Lance enemy shipped with an opaque white
  matte and drew as a white box; it now has an alpha channel like its siblings.

### Build and test

- The host-side renderer tests are no longer a prerequisite of `make test`.
  Launching a ROM in MAME should not require a host C++ toolchain, and on
  Windows it was failing the whole run.  `make unit-tests` runs them, `make
  check` gates on tests plus a complete ROM set.
- `tests/Makefile` suffixes its binaries with the platform's executable
  extension, so a tree shared between WSL and Windows cannot run the other
  side's build.
- `sound-clean` deletes encoded samples only where the WAVs that produce them
  are present.  `make samples` skips an encode whose `in_wav_*` directory is
  missing, so for the four games that ship ADPCM without source in the tree
  the old rule was not a clean but an unrecoverable loss - and `clean-all`
  reaches it, which is the first thing `test.bat` runs.  Verified: a
  `sound-clean` on Sky Lance now leaves all 110 sample files in place.

## v1.7.1 - Rendering and platform fixes

Release date: 2026-09-06

A correctness pass over sprite scaling, per-frame VRAM traffic, the AES build
and the per-game build plumbing.  No API was removed; one macro was added.

### Sprite scaling

The hardware does not shrink the two axes alike: X is taken from the top nibble
as `((value >> 4) + 1) / 16`, Y from the whole byte as `(value + 1) / 256`.
Those agree only when the low nibble is `F`, so scale bytes written as `0xN0`
drew sprites up to 12% shorter than they were wide.  Every affected constant and
call site has been corrected, and `NG_SCALE(sixteenths)` in `ng_defs.h` now
builds a byte whose axes match.  The rule is documented in the C engine API
reference.

- Corrected 29 scale literals across the demo scenes and `neogeogame`.
- Corrected the demo's named `U_SCALE_*` presets, which had been tuned as
  percentages and so were right on Y and quantised too wide on X.
- Corrected Sky Lance's four scale presets and re-sized them against the
  playfield: the player ship took 35% of the screen height and bosses 66-71%.

### Per-frame VRAM traffic

`ng_sprite_group_upload()` rewrites the tilemap; `ng_sprite_group_flush()`
writes only what changed.  Several places called `upload()` every frame for
artwork that had only moved, which is enough traffic to overrun vblank and
corrupt the sprite writes that follow it.

- Sky Lance's scrolling backdrop now uploads once and flushes position only.
- The demo's starfield chapter builds its groups once and flushes per frame,
  re-uploading the tilemap only when a depth band actually changes the palette.
- The demo's sprite-screen chapter no longer re-uploads a static image 180
  times.
- `neogeogame` wrote its sprite VRAM *before* waiting for vblank, putting every
  write into active display.  The order is now wait, then draw.

### Sprite budgets

- `neogeogame`'s enemy artwork needed 16 hardware sprite strips while the slot
  map reserved 4, so 18 enemies demanded 288 sprites from a 136-slot range and
  overran the bullet and explosion pools.  The art is now imported at the size
  it is drawn, and the strides are checked against the asset table at compile
  time - `gen_sprite_meta.py` emits `NG_ASSET_STRIPS_n` for that purpose.

### AES

- `neogeo_aes.c` was missing the pointer casts its MVS counterpart had, so
  `PLATFORM=aes` did not compile for any game.  Fixed for all six.
- `helloworld` never called the `game_boot()` / `game_frame()` in its own
  `main.c`, and its attract loop waited on a BIOS flag it never returned to let
  the BIOS set, so START did nothing.  Both fixed, on MVS and AES.

### Build system

- `GAME_SCENES_FROM`, `GAME_ART_FROM` and `GAME_SOUND_FROM` let a game reuse
  another game's scenes, artwork or audio without a second copy.
- `USE_2D_PLUS=1` now builds the game's own sources as C++ too, not just the
  engine.  The SDK headers carry `extern "C"` guards for this.
- A game with no fix layer, no screens, no art or no sound assets now builds
  instead of failing: the generated `.inc` files the Z80 driver includes
  unconditionally are always written, empty if there is nothing to put in them.

### Gameplay

- Sky Lance stages could hang before the boss: enemies on a holding pattern
  never left the field, and the boss waits for the field to clear.  Holders now
  break station after a bounded time, and the director sends the boss anyway if
  stragglers remain.
- Sky Lance and the demo's shooter chapter now aim their shots at the player and
  drift toward the player's column rather than flying fixed lanes.

## v1.7.0 - The 2D Engine Release

Release date: 2026-08-30

This entry covers the whole `neo_universal_2d` branch - 164 commits since the
last mainline release.  It is the largest release the SDK has had: a complete
2D game engine in two languages, a rebuilt art pipeline, a rebuilt audio
stack, a multi-game build system, native Windows support, desktop authoring
tools, one-shot installers, three new games, and the documentation set that
explains all of it.  The v1.7.0-pre entry below remains as the record of the
mid-branch pre-release.

### Headline features

- **A complete 2D game engine**, in plain C (`sdk/2d_engine/`) and C++14
  (`sdk/2d_engine_plus/`) with an identical public ABI - 35 modules covering
  characters, actions, physics, NPCs, camera, level, particles, feedback,
  depth effects, palette effects, sprite groups, the render queue, the FIX
  layer, input, timers, events and fixed-point math.
- **A multi-game build system** - one repository, any number of independent
  games, each with its own id, ROM folder, artbox, sound tree and MAME hash
  set.  Six games ship with it.
- **A rebuilt art pipeline** - a perceptual quantiser that works in CIE-Lab
  on the Neo Geo's actual 5-bit colour lattice, with blue-noise dithering,
  per-tile palettes, master sprite palettes, sprite halo removal and a
  content-preserving screen fit.
- **A rebuilt audio stack** - nine ADPCM-B beds, eight FM tracks, nine SSG
  tracks, a recorded voice bank with `speakText()`, live FM tempo, LFO, pan,
  noise and CSM control, and a fade engine that actually fades.
- **Sky Lance** - a complete vertical arcade shooter, the SDK's worked
  example of a finished game.
- **A 25-chapter demo reel** exercising every public subsystem in order.
- **Native Windows builds**, one-shot installers for four host setups, and a
  documented WSL audio path.
- **Two PyQt6 desktop tools** - Artbox Studio and Sound Studio.
- **A documentation set written for the end user**, including generated API
  references and a printable manual.

---

### The 2D engine

Added on this branch, from nothing, in thirteen staged passes.

**Core** - `ng_game_interupt` runs the whole frame tick in a fixed order
(timers, characters, actions, physics, NPCs, camera, particles, palette
effects, events) and flushes the render queue inside VBlank, with five hook
points for game code.  `ng_scene`, `ng_game_time`, `ng_timers`,
`ng_progress`, `ng_status`, `ng_properties` and `ng_game_events` carry the
state primitives.

**Characters** - `ng_chars` is a 64-object pool with movement, animation
clips, asset binding, body and hit rectangles, cull margins, priority bands,
damage and healing, plus scene arenas and a fixed-step update mode.
`ng_actions` adds table-driven action scripts with sound and FX hooks;
`ng_npcs` adds patrol bounds, home positions and think callbacks;
`ng_physics` adds gravity, drag, solid rectangles and grounded tests;
`ng_border_constraints` keeps characters inside declared regions.

**Video** - `ng_render_queue` is a 128-slot deferred write queue drained in
VBlank, so no engine module writes VRAM from the logic path.
`ng_sprite_group` implements dirty-flag sticky-bit sprite chains that write
only what changed.  `ng_sprite_window` tracks the strips a variable-size
sprite occupies so shrinking one actively hides the strips it vacates.
`ng_bg` gives two scrolling sprite background layers with parallax factors.
`ng_fix` is the FIX text layer with a dirty-cell cache.  `ng_palette_fx`
runs fades, flashes, pulses and colour cycles, all queue-safe.
`ng_depthfx` projects `NGVec3` positions to screen with a Z-to-shrink table,
fog palette banding, and starfield advance.

**Effects** - `ng_particles` is a 32-slot fixed pool with typed spawns (hit
spark, dust, smoke, explosion, slash trail, magic spark) and three-tier
priority eviction, so a flood of dust cannot push out an explosion.
`ng_feedback` packages hitstop, screen shake, palette flash and a sound hook
into a single `ng_impact_event()` call, plus slow motion.

**World and input** - `ng_camera` provides follow, dead zone, look-ahead,
shake, cinematic pan and bounds clamping.  `ng_level` carries world bounds,
scroll and camera helpers.  `ng_joystick` provides edge detection, auto
repeat, held-frame counting, numpad-notation direction history, QCF and
dragon-punch motion recognition, and direct character control.

**Math** - `ng_fixed` supplies 16.16 fixed-point, `ng_sin_tab[]`,
`ng_cos_tab[]` and `ng_shrink_tab[]`.  No float, no division in the frame
loop.

**C++14 port** - `sdk/2d_engine_plus/` is the same engine with member
methods and singletons (`CharManager::instance()`, `cam.follow(...)`), built
under `-std=c++14 -fno-exceptions -fno-rtti -fno-threadsafe-statics
-ffreestanding`.  The public surface stays `extern "C"`, so a C game can
link the C++ build and vice versa.  Select with `USE_2D_PLUS=1`.

**Supporting SDK layers** - `sdk/ng_fix` (a standalone FIX SDK usable
without the engine), plus `sdk/ng_audio`, `sdk/ng_scene`, `sdk/ng_show`,
`sdk/ng_video` and `sdk/bsp`.

### Engine correctness work

A long arc of hardware-behaviour fixes, each found by observation rather
than from documentation.

- **Sprite teardown rewritten.**  `ng_sprite_disable_hw()` now performs a
  real hardware-level teardown: a full 64-word SCB1 wipe with blank tile
  `0xFFFF` (not tile 0, which may carry art), sprites parked off-screen,
  a two-tier teardown with a previous-strips tail clear, and SCB1 row 0
  blanked as height-wrap defence.  Ghost sprites persisting across scene
  transitions were the visible symptom.
- **Per-window VRAM footprint tracking** with stable per-frame anchoring, so
  a sprite that changes size between frames leaves nothing behind.
- **Demo sprite uploads queued to drain inside VBlank** instead of writing
  VRAM from the logic path.
- **Sprite slot priority reversed.**  The observed hardware direction is
  **HIGHER slot number = drawn IN FRONT**.  The earlier claim was the
  opposite, and the result was backgrounds parked at "behind" slots drawing
  over characters.  Backgrounds now belong at slots 1-32, characters at
  96-223, foreground effects at 288+.
- **`REG_PALBANK0` / `REG_PALBANK1` address swap fixed.**
- **The backdrop register corrected** to the last word of palette RAM,
  `$401FFE`.  `$402000` is a mirror that silently does nothing.
- **The FIX layer's visible row origin corrected** - 28 rows are visible and
  visible row *y* is map row *y + 2*.
- **Per-axis camera dead zone fixed**, along with vertical scroll in the
  camera chapter.
- **Sprite-group dirty bits and the render contract** documented and fixed.
- **Sub-tile offset included in `used_tile_cols` / `used_tile_rows`** so a
  painted bounding box actually fits its art.
- **`START_GAME` syncs to VBlank before clearing sprites**, eliminating the
  black-box flash on entry.
- **Particle sprite budget reporting corrected** and stale particle slots
  cleared on expiry.
- **Weak fallbacks for `ng_screen_table[]` / `ng_screen_count`** added to
  `ng_bg.c` and `ng_bg.cpp`.  The artbox emits the real table into each
  game's `main.c`; a game shipping no screen art had no table, and because
  `ng_bg` is always linked the reference was always present - so a minimal
  game could not link at all.
- **`ng_chars_count()` is an active count, not an array bound.**  Loops
  bounded by it silently skipped the tail once a slot freed.
- **`NG_CHAR_LIFE_DESTROY_PENDING` is not acted on by the engine.**  Objects
  marked with it stayed active until the pool filled permanently; use
  `ng_chars_remove()`.

### Build system

- **Multi-game builds.**  Each game lives in `games/<name>/` with its own
  `game.mk` (`GAME_NAME`, `GAME_ID`, `GAME_SCENES`, `GAME_EXTRA_INCLUDES`),
  `game.cfg`, linker script, BIOS hooks, artbox and sound tree.
  `GAME_ID` propagates through every stage and becomes the ROM prefix.
- **Per-game ROM folders** (`roms/<game>/`) and **per-game MAME hash sets**
  (`hash_eagle/<game>/neogeo.xml`), regenerated on every `p1` with correct
  CRC/SHA1 and `loadflag="load16_word_swap"`.
- **`game.cfg` cross-check** - the build stops if `GAME=` and `CURRENT_GAME`
  disagree, so one game's art cannot be silently built into another's ROM.
  In v1.7.0 all six games carry their own cfg, so the invocation is uniform.
- **`GAME_EXTRA_INCLUDES`** lets a game pull in another game's artbox or
  headers without duplicating data (used by `demo_plus` to share the demo's
  artbox).
- **`GAME_SCENES` is an explicit list**, not a wildcard, so helper files that
  are `#include`d elsewhere are not compiled twice - the cause of a run of
  multiple-definition link errors.
- **Native Windows builds** through `MakefileWin32.mak`: quoted `GAME_ID`
  filenames in `romts.bat` / `romfx.bat` so `romtool` stops parsing hyphens
  as flags, `SHELL=cmd.exe`, backslashed sound paths, correct source-list
  expansion for `fm` / `mml` / `ssg`, correct 128 KB S-ROM format from
  `sfix`, and catch-all data/bss patterns in every linker script for Windows
  `ld` compatibility.  v1.7.0 also builds the shipped ROMs from the Windows
  toolchain and stops the CRLF churn in generated files.
- **Toolchain default moved to x-tools-v2.**
- **P1 generation crops to the full 512 KB window before byte swap and
  padding** - the format both MAME and hardware require.
- **Debug and trace builds**: `debug-build`, `debug-artifacts` (size, nm,
  readelf, map, disassembly), `gdb`, `gdb-trace`, `gdb-remote`, plus MAME
  launch support and `bios-list` / `games-list` / `menu`.
- **`make dist`** packages `dist/roms/<game>.zip` with ROM files at the
  archive root.
- **One-shot installers** for Linux, Ubuntu, native Windows and WSL under
  `install/`.

### Art pipeline

The artbox went through a long series of quantiser revisions on this branch.
The end state is `artbox/img2neo_tile.py`, the default for every build via
`ARTBOX_TILE=1` in both makefiles.

**Asset routing and fitting**

- Unified asset pipeline with **background vs. sprite routing** and a
  **derived master palette** shared by the `characters` and `npcs` groups.
- **Non-destructive screen fitting** - `fit=contain` / `pad` / `letterbox`
  alongside the original `crop`, with padded area transparent, transparent
  pixels mapped to palette index 0 and visible pixels to 1..15.  Metadata
  now records the real content box for screen assets.  Default fit changed
  from `crop` to `contain`.
- Category rule mismatch fixed (`match_category = npc` -> `npcs`).

**Colour**

- **CIE-Lab clustering** replaced the sepia-collapsing RGB path.
- **Per-tile Lab k-means with per-tile palettes**, greedy MAE bank dedup, and
  a Lab-nearest remap into a representative palette derived from the
  weighted union of banks - preserving per-tile dither detail end to end.
- **Context windows plus global Floyd-Steinberg** killed tile-boundary seams.
- **Decoupled scanline dithering** removed Z-order traversal noise.
- **Alpha preservation, a SHA256 conversion cache, and the "luma trap" fix**,
  followed by the "teflon routing" fix for the shattered wavefront it caused.
- **Atkinson dithering** replaced Floyd-Steinberg for a period to kill
  speckle; the v1.7.0 quantiser replaces both with blue noise.
- **Photocopy variance gate** for flat-source protection.
- **Auto-scaling epsilon** in `cluster_and_remap` and pre-allocated output
  lists.

**v1.7.0 quantiser**

- **Lab throughout** - palette selection, refinement and the final per-pixel
  remap all run in CIE-Lab through a precomputed 32^3 lattice LUT.  An
  earlier attempt that matched in luma-weighted YCbCr while measuring in Lab
  made sprites measurably worse.
- **Lattice refitting** - `refine_palette_on_lattice()` runs Lloyd iterations
  with the hardware snap *inside* the update step, keeps a move only when
  total error drops, and re-seeds duplicate or unowned slots from the
  worst-served pixels.
- **Void-and-cluster blue noise** - a 32 x 32 mask (0.04 % low-frequency
  energy) orders the mix between the two nearest palette entries along a
  serpentine scan, with a dead band that skips the mix within 18 % of either
  endpoint.  This removed the speckle in flat regions.
- **Pre-cluster contrast and saturation push with a `tanh` soft knee**, tuned
  separately for sprites and backgrounds.  Clipping folds boosted highlights
  onto pure white, throwing colour detail away before k-means sees it.
- Measured across the reference assets: mean dE 9.10 -> 8.33, p95 18.19 ->
  15.66, dither speckle down a third.
- `_VIVID_CACHE_VERSION = "v11-lattice-refit-bluenoise"`.

**Alternative pipelines and sprite fixes**

- **CRT pipeline** (`img2neo_crt.py`) - Lab k-means, horizontal-biased
  dither, gamma 1.20 / contrast 1.10 pre-boost.  `make art-crt` or
  `ARTBOX_CRT=1`.
- **HD alt scripts** - `img2neo_hd.py` (bilateral filter, CLAHE on the Lab L
  channel, unsharp mask, blue-noise dither) and `fixtiles_hd.py` (per-tile
  palette FIX conversion, `--sharp-text` binarisation).
- **`ARTBOX_LEGACY=1`** keeps the original nearest-neighbour path for
  diffing.
- **Sprite halo fix** - `alpha_bleed()` runs after `fit_sprite_rgba` so
  anti-aliased contours stop baking the source PNG's hidden
  transparent-pixel RGB (usually near-white) into the indexed sprite.
- **Configurable halo strip on sprite contours** with per-category thresholds
  and a post-convert transparency audit.
- **GIMP plugins** for the art pipeline under `tools/gimp-plugins/`.

### Audio

**Driver and protocol**

- **Z80 polling race condition fixed.**  `soundCommand` called
  `isZ80Ready()` twice; the trailing call wrote 0 to `REG_DIPSW`, triggering
  an NMI whose handler clears `REG_SOUND`, so the 68000 polling loop read 0,
  re-triggered, and deadlocked permanently - a frozen white screen with no
  sound.  The trailing call was removed.
- **Driver restored to the verified-working assembly baseline** and the M1
  ROM rebuilt through the pure assembler path.  `make compare-driver` builds
  the ASM and experimental C drivers and diffs the ROM images.

**New chip-level control**

- **ADPCM-B L/R pan** (cmd `$15`, reg `$11`), and `play_adpcmb_index` now
  writes `$11 = $C0` explicitly so stereo is guaranteed after a reset.
- **FM LFO control** (cmd `$17`, reg `$22`), and `fm_apply_patch` no longer
  writes reg `$22` - the LFO is a global register and per-patch resets were
  clobbering every user setting.
- **SSG noise period** (cmd `$19`, reg `$06`).
- **Live FM tempo** (cmd `$1A`, `soundFMSetTempo()`).
- **FM CSM** - Composite Sine Mode on FM channel 2 (cmds `$1B` / `$1C`),
  with `soundFMCSMBegin()`, `soundFMCSMEnd()` and a `soundFMCSMSweep()`
  helper for formant slides.  `stop_all` ends CSM so the mode cannot bleed
  across scenes.

**Content and tracks**

- **Track ids realigned and names made generic** - `N_<kind>_example_<x>`
  filenames, letter-suffixed canonical ids (`SOUND_MUSIC_A`..`H`), and
  1-based `TRACK_N` aliases.  All legacy semantic names dropped.
- **Nine ADPCM-B beds**, eight FM tracks, nine SSG tracks.  TRACK 4 is
  reserved for the eyecatcher.
- **All FM and SSG tracks rewritten** in arcade style, with octave shifts
  balanced so no note clamps to MIDI 127 - the earlier octave drift produced
  identical stuck-high notes.
- **`soundPlayGameLoop` dispatcher** and the scene helpers
  `soundPlayDemoFM` / `soundPlayTitleMusic`.
- **Fade engine fixed.**  The driver formula is `COUNTER = $FF - speed`, so
  the old 2/8/20 speed values meant almost no fade; and the per-step
  decrement was 1, so a fade from `$B8` took 22.6 s.  Speeds corrected and
  the decrement raised, giving audible ~1.4 s fades.
- **SSG MML envelope directive renamed `E` -> `K`** - it collided with the
  musical note E, so every melodic SSG track containing an E note compiled
  as an envelope-mode flip instead of a note.

**Voice**

- **Voice alphabet pipeline** - `in_wav_a_voice/*.wav` -> 16-bit PCM ->
  ADPCM-A, bundled into the V-ROM right after the SFX bank by `vrom.py`.
  `sample_table.inc` exports `ADPCMA_VOICE_BASE` / `ADPCMA_VOICE_COUNT`, and
  `sdk/sound_voice_ids.h` is generated with `SOUND_VOICE_*` constants.
- **`speakText()` / `speakWord()`** prefer whole-word samples, then fall back
  to letters, digits and punctuation.  `playVoiceLetter()`,
  `playVoiceWord()`, `playVoiceNumber()` and the prebuilt cues
  (`playGetReadyVoice`, `playAttackVoice`, `playCoinThenReady`) route through
  the same bank.
- The **Windows sample pipeline mirrors the shell scripts** and skips
  cleanly when the voice directories are absent.
- An **SSG envelope and three-formant phoneme speech engine** was built,
  evaluated on hardware, and **removed**: a pure YM2610 SSG path cannot
  synthesise intelligible speech regardless of driver tricks.  Recorded
  ADPCM-A samples are the shipping answer.  The driver was reverted to the
  confirmed-working FM baseline with only the three new register commands
  layered back.

### Games

- **`games/skylance`** (id 779) - Sky Lance, a complete vertical arcade
  shooter.  Three pilots (ROOK / BLUE LANCE, KIRA / GREEN WING, BLAZE / RED
  STRIKER) differentiated by fire rate, speed and single vs. twin bolt.
  Seven stages, each running 5-7 squadrons before a named boss: CRIMSON
  KEEP, IRON TIDE, SOL CORE, NIGHT RAZOR, ROTOR NEST, EARTH HAMMER, SPIRE
  GOD.  Attract reel, pilot select, scoring, lives, energy and a continue
  flow.  `sky_fix_palettes_init()` folds 18 INFIX palettes into FIX banks
  4..15, because the FIX map word carries only four palette bits and banks
  19-21 had been wrapping onto text inks.
- **`games/demo`** (id 777) - grown from a 13-scene showcase into the
  **25-chapter reel**, with the chapter number printed top-right on every
  scene, **A** to advance and **C** to restart.  Chapters added over the
  branch include the camera lab auto-tour, the FIX FX infix-photo animation,
  char select, particle load, depth parallax, target range, garden 3D
  (sprite-scaling pseudo-3D), char 2D and Sky Lance.  The mini-game was
  rewritten as a player-vs-clone fight; the shooter chapter went Galaxian ->
  Eagle Invaders -> Sky Lance.  `chap_header()` performs a full hardware and
  engine reset per chapter, including the sprite-window tracking cache,
  which is global across the ROM run and was starving later slot-heavy
  chapters.
- **`games/demo_plus`** (id 778) - links exclusively against the C++ engine
  and exists to keep the two builds ABI-compatible; three smoke-test
  sub-scenes driving the render queue, palette effects and the particle
  pool.
- **`games/neogeogame`** (id 775) - blank template, plus a sprite shooter
  demo added mid-branch.
- **`games/tutorial`** (id 555) - the minimal engine loop.  Fixed in v1.7.0:
  it still included the long-removed `sdk/2d_engine/ng_game_engine.h` and
  called `game_engine_init()` / `game_engine_frame()`.
- **`games/helloworld`** (id 772) - minimal FIX text and one sample.
- All six verified building to a ROM from a clean invocation.

### Desktop tools

- **Artbox Studio** (`artbox/artbox_studio.py`) - Pipeline runner with a
  status pill per step and a live log, Asset Browser, Hex Sprite Inspector
  (decoded tile grid, raw C1/C2 bytes, live palette swap), Movement Designer
  (frame sequences exported as C arrays), Level Designer (20 x 14 tilemap
  painter), HD Compare, ROM Inventory, and an Asset Rules editor for
  `assets.cfg`.
- **Sound Studio** (`sound/sound_studio.py`) - Track, Mix and ROM tabs, a
  live waveform view, and an MML designer.

### Documentation

New in v1.7.0:

- `docs/INTRODUCTION.md` - what the SDK is, what the machine is, what is in
  the box, and the reading order.
- `docs/PROGRAMMERS_MANUAL.md` - build system, project layout, the game
  contract, the engine tick, assets, the frame budget, register and
  interrupt appendices, and a symptom-to-cause table.
- `docs/HELLO_WORLD.md` - first ROM in ten minutes, then how to grow it into
  your own game.
- `docs/SOUND_DRIVER.md` - architecture, the 68000-side API, the raw command
  protocol, the content pipelines, and the practices that keep audio stable.
- `docs/GAMES.md` - what each of the six shipped games demonstrates.
- `docs/TOOLS.md` - every helper script in the tree: the artbox conversion
  cores and pipeline stages, `romtool`, the asset validators, the sound
  compilers and ADPCM encoders, the M1 driver build and compare scripts, the
  verification tools, the launchers and installers, the Windows helper
  binaries, the GIMP plug-ins, and the debug artefacts.
- `docs/DEMO_CHAPTERS.md` - the 25 chapters, and the per-chapter reset worth
  copying.
- `docs/API_2D_ENGINE_C.md` and `docs/API_2D_ENGINE_CPP.md` - generated from
  the headers by `tools/gen_api_reference.py`, so they cannot drift.
- `NEOGEOSDK_v1.7.0.md` - the single-file release overview.
- `docs/neogeosdk_v1.7.0_manual.pdf` - the printable manual, built by
  `tools/make_manual_pdf.py`.

Updated:

- `docs/ARTBOX_PIPELINE.md` rewritten around the quantiser, the colour
  target, asset ordering and the Artbox Studio tabs.
- `SDK_API_GUIDE.md` quick reference corrected - it still carried the
  inverted sprite-priority claim.
- `README.md` - v1.7.0 summary, games table, documentation index.
- Earlier on the branch: engine layer docs, `docs/ADDING_A_GAME.md`,
  `docs/GDB_GUIDE.md`, `docs/BANKSWITCH.md`, `docs/DEPENDENCIES.md`,
  `docs/MAKEFILE_INTEGRATION.md`, the per-module docs
  (`sprite_groups`, `render_queue`, `camera`, `palette_fx`, `particles`,
  `depthfx`, `performance_rules`), the sound readmes, and the WSL2 +
  Ubuntu 24.04 + PulseAudio audio walkthrough.

## v1.7.0-pre - Bug Fixes, Per-Game ROM Folders, Z80 Sound Fix, and Demo Overhaul

Release date: 2026-05-17 (pre-release; superseded by v1.7.0 above)

### Highlights

- 15 SDK bug fixes across sprite, FIX, sound, and build subsystems
- Per-game ROM output folders (`roms/<game>/`) — all ROM files now isolated per game
- FIX layer hardware bug fix: `clearFix()` now restores BRDFIX so game S ROM is always selected after a clear
- Z80 communication race condition fixed: `soundCommand` no longer hangs the 68k in polling loop
- Sound driver restored to last known-good assembly version (1947-line working driver)
- Demo fully rewritten as a cinematic 13-scene SDK showcase
- Windows build path bugs corrected across all sound and FIX tools
- Multiple-definition linker errors from scene file additions resolved

### Bug Fixes


#### Z80 Sound — `soundCommand` Polling Race Condition

`soundCommand` called `isZ80Ready()` twice: once before writing the command and
once after.  The trailing call wrote 0 to `$300001` (REG_DIPSW), triggering a Z80
NMI.  The NMI handler clears `$320000` (REG_SOUND) to 0 while processing the
previous command.  The 68k polling loop then reads 0, re-triggers another NMI, and
the cycle deadlocks permanently — producing a frozen white screen with no sound.

Fix: the trailing `isZ80Ready()` has been removed from `soundCommand` in
`sdk/neogeolib.c`.

#### Z80 Driver — Restored Working Assembly Driver

The M1 ROM was being built from an updated 2301-line `driver.asm` that diverged
from the last verified-working version (1947 lines).  `sound/driver/driver.asm`
has been restored to the working version and the M1 ROM rebuilt via the pure
assembler path (`USE_Z80C=0`).

#### Particle System — Sprite Budget Reporting and Stale Slot Cleanup

- Particle sprite budget counter now correctly reports actual slots consumed.
- Stale particle sprite slots are cleared when particles expire, preventing
  ghost sprites from persisting across scene transitions.
- Demo sprite cleanup limited to valid slot range to avoid out-of-bounds writes.

#### Linker — Multiple-Definition Errors When Adding Scene Files

Adding `demo_screen.c` to the demo scene list caused multiple-definition linker
errors for symbols shared across scene files.  Fixed by correcting the `GAME_SCENES`
list to exclude files that are `#include`-d by other scenes.

### Per-Game ROM Folders

ROM output files are now written to `roms/<game>/` instead of a shared `roms/`
root.  The `make p1`, `make sound`, `make art`, and `make sfix` targets all sync
their outputs to the per-game folder.

```text
roms/demo/       777-p1.p1  777-m1.m1  777-s1.s1  777-v1.v1  777-c1.c1  777-c2.c2
roms/helloworld/ 772-p1.p1  772-m1.m1  ...
roms/tutorial/   555-p1.p1  555-m1.m1  ...
```

`hash_eagle/<game>/neogeo.xml` is regenerated automatically on every `make p1`.

### Windows Build Fixes

- `MakefileWin32.mak`: `SHELL=cmd.exe` forced to prevent sh-style path expansion.
- `romts.bat` / `romfx.bat`: `GAME_ID` filenames now quoted to prevent romtool from
  treating hyphens as flags.
- `GAME_SOUND` path backslash-corrected; trailing space in `sfix GAME_ID` removed.
- All game linker scripts gain catch-all `.data` / `.bss` patterns for Windows `ld`
  compatibility.
- Per-game `GAME_ID` and `GAME_SOUND` propagated to all artbox and sound tools.

### Demo Overhaul

The demo game (`games/demo`) has been rewritten as a cinematic 13-scene SDK
showcase covering: intro, sprites, camera, palette FX, particles, depth FX, FIX
layer, sound, combat, stress test, title, render queue, and a 3D starfield scene.
Each scene exercises a distinct engine subsystem with timed transitions.

### Documentation

- `BUGFIX_SESSION.md` added at repository root — full root-cause analysis and fix
  description for the frozen-screen regression.
- `docs/ADDING_A_GAME.md` updated with per-game ROM folder layout.

### Validation

- `make GAME=demo all` — zero errors, zero warnings
- `make GAME=helloworld all`
- `make GAME=tutorial game`
- `make GAME=neogeogame game`
- `make m1rom-asm` — assembler path, working driver
- `make -f MakefileWin32.mak sound`
- `make -f MakefileWin32.mak sfix`

---

## v1.2.0 - NeoGeo Deluxe 2D Engine, Multi-Game Build System, and Sound Pipeline

Release date: 2026-05-06

### Highlights

- Complete NeoGeo Deluxe 2D engine layer targeting huge animated characters, smooth camera, parallax depth, sprite-scaling, palette FX, particles, and stable 60 FPS — no float, no malloc during gameplay
- Multi-game build system: single repository, multiple independent games under `games/<name>/`
- Full per-game art, sound, and ROM pipeline on both Linux and Windows
- Refined YM2610 sound driver workflow on both the authoritative ASM path and the experimental C path
- Stable sound IDs for 68k-side code and improved demo/game loop mixing

### Multi-Game Build System

| Game | ID | Description |
|------|----|-------------|
| `games/demo` | 777 | Full SDK showcase (13 scenes, all engine features) |
| `games/helloworld` | 772 | Minimal FIX-text hello world |
| `games/tutorial` | 555 | Tutorial template with BIOS hooks |
| `games/neogeogame` | 775 | Blank template for new projects |

Build any game with `make GAME=<name>` (Linux) or `make -f MakefileWin32.mak GAME=<name>` (Windows).

- `Makefile` and `MakefileWin32.mak` accept `GAME=<name>` (default `demo`)
- `-include games/$(GAME)/game.mk` loads per-game `GAME_ID` and `GAME_SCENES`
- All ROM filenames derived from `$(GAME_ID)` — no hardcoded `777`
- Per-game source paths: `games/$(GAME)/user.c`, `main.c`, `eyecatcher.c`, `neogeo.ld`
- Per-game sound: `GAME_SOUND = games/$(GAME)/sound` — fm, mml, ssg, samples all per-game
- Per-game art: `artbox/makeartbox.sh GAME` / `artbox/makeartbox.bat GAME`
- `GAME_ID` and `GAME_SOUND` propagated to all artbox and sound tools (romts, romfx, fixtiles, vrom, m1rom)
- Games with no art receive a stub `showEyeCatcherMVS()` to avoid linker errors
- Empty sound/art folders handled gracefully — each target skips with a message
- `aes`, `mvs`, `test-aes`, `test-mvs`, `debug-aes`, `debug-build` targets forward `GAME=`

### Deluxe 2D Engine Layer

New engine modules under `sdk/2d_engine/`:

| Module | Files | What it does |
|--------|-------|-------------|
| Render queue | `ng_render_queue.h/.c` | 128-slot VBlank-safe deferred VRAM and palette write queue |
| Fixed-point math | `ng_fixed.h/.c` | 16.16 fixed-point, pre-baked sin/cos/shrink lookup tables |
| Camera | `ng_camera.h/.c` | Smooth follow, dead zone, look-ahead, shake, cinematic pan, border clamp |
| Palette FX | `ng_palette_fx.h/.c` | Fade, flash, pulse, color cycle — queue-safe |
| Particles | `ng_particles.h/.c` | 32-slot fixed pool, 8 particle types, priority-based eviction |
| Feedback | `ng_feedback.h/.c` | Hitstop + screen shake + palette flash + sound hook in one call |
| Depth FX | `ng_depthfx.h/.c` | NGVec3 perspective projection, Z→shrink lookup table, starfield |
| Debug HUD | `ng_debug.h/.c` | Fix-layer perf overlay (compile with `NG_DEBUG_PERF=1`) |

Updated engine modules:

- `ng_sprite_group`: dirty flags and `ng_sprite_group_flush()` — only changed SCB regions written per frame
- `ng_depthfx`: extended with `NGVec3`, full perspective projection, Z→fog palette, starfield advance
- `ng_engine.h`: aggregate include covers all subsystem headers

### Sound System

- Refined YM2610 sound driver workflow on ASM and experimental C-driver paths
- `sdk/sound_ids.h` — stable track / cue / bed / voice identifiers for 68k-side code
- `sound/driver/driver_defs.h` — named C-driver command and runtime constants
- Experimental C-driver: `driver.c`, `driver_prelude.asm`, `combine_split_driver.py`, `compare_m1.py`
- Compare flow: `make m1rom-asm`, `make m1rom-c`, `make compare-driver`
- Higher-level sound helpers and mix setup in `sdk/neogeolib.c`
- Improved demo/game loop mix layering: SSG, FM, ADPCM-B, and ADPCM-A accents
- `GAME_SOUND` env var passed through all sound tools for per-game audio isolation
- `enc_wave16le_a/b.sh` and `.bat` read WAV input from `$GAME_SOUND/samples/` instead of shared dir
- Linux and Windows makefiles support `SDKHOME`, WSL usage, SoX-optional sample conversion

### Artbox Pipeline

- Default fit mode changed from `crop` to `contain` — non-destructive asset scaling
- `gen_eyecatcher.py` updated with fast animation pacing (lead hold, 2× animation pass, final hold)
- Eyecatcher sprites positioned at hardware-accurate size and position
- `romdbimgimport.py` hardened with absolute paths and fail-fast error handling
- `makeartbox.sh` / `makeartbox.bat` use absolute-path resolution to avoid symlink nesting bugs
- Windows: `mklink /J` directory junctions — no administrator privileges required (Vista+)
- Windows path bug fixed: `REPO_DIR` now resolved with `for %%i ... %%~fi` instead of raw `..`

### Build System

- Makefile toolchain detection defaults to `x-tools-v2` with `x-tools` legacy fallback
- Linker flags corrected (`-nostdlib`, `-nostartfiles` removed)
- Win32 Makefile engine module list updated to match Linux (added `ng_render_queue`, `ng_fixed`, `ng_camera`, `ng_palette_fx`, `ng_particles`, `ng_feedback`, `ng_debug`, `ng_demo_advanced`)
- `MakefileWin32.mak`: `vrom` and `m1rom` targets now export `GAME_ID` and `GAME_SOUND` to environment
- FIX ROM path corrected — `sfix` target creates its own temporary `artbox/infix` symlink/junction
- Win32 sound/FIX path parity corrected

### Launcher Scripts

Both `neogeosdk.sh` (Linux) and `neogeosdk.bat` (Windows) gained a game-selection step (`g` key). The selected game is passed to every build command for the duration of the session.

### New Documentation

- `docs/sprite_groups.md`
- `docs/render_queue.md`
- `docs/camera.md`
- `docs/palette_fx.md`
- `docs/particles.md`
- `docs/depthfx.md`
- `docs/performance_rules.md`
- `docs/ARTBOX_PIPELINE.md`

### Validation

- `make all` (demo, default)
- `make GAME=helloworld all`
- `make GAME=tutorial game`
- `make GAME=neogeogame game`
- `make m1rom-c`
- `make -f MakefileWin32.mak sound`
- `make -f MakefileWin32.mak sfix`
- zero errors, zero warnings on all four games

---

## v1.1.0 - Sound System, Python 3 Migration & Build Tools

Release date: 2026-05-04

### Highlights

- added the custom Neo Geo YM2610 sound system
- migrated the artbox pipeline to Python 3
- replaced the legacy ADPCM encoder with a cross-platform Python implementation
- improved Windows parity across the build chain

### Included work

- Z80 YM2610 driver with FM, ADPCM-A, ADPCM-B, and SSG
- MML compilation pipeline for sound content
- Win32 sound build scripts
- updated installation and project documentation

## v1.0 - Initial Public Release

Release date: 2021-11-07

### Highlights

- initial Neo Geo SDK release
- toolchain asset publishing
- demo ROM and example project layout

## Recent Commit History Since 2025

- `bbf84b0` 2025-11-29 `Updated README.md to enhance clarity and structure`
- `d36fe14` 2026-04-26 `Migrate artbox pipeline from Python 2 to Python 3`
- `774171e` 2026-04-26 `Update README: install numpy via apt, pypng via pip3`
- `6c87ed2` 2026-04-26 `upgrade artbox to python3`
- `0ec1cd4` 2026-05-04 `Add complete NeoGeo sound system: Z80 driver, YM2610 FM/ADPCM/SSG, MML pipeline and ROM build chain`
- `79436bc` 2026-05-04 `Replace binary ADPCM encoder with cross-platform Python implementation and fix Win32 build scripts`
- `197b75b` 2026-05-04 `Update sound driver, SSG configuration, and project build artifacts`
- `e362f11` 2026-05-04 `Fine-tune sound initialization and track playback in main and demo loops`
- `fa8ca15` 2026-05-05 `feat: migrate Z80 sound driver to C and enhance z80cc compiler`
- `3b5c4d4` 2026-05-05 `feat: implement execute_command and fix compiler distance errors`
- `bfcaa20` 2026-05-06 `Refine sound driver workflow and demo mix`
- `7747dcd` 2026-05-07 `Remove softfloat runtime and refresh generated assets`
- `abaefbe` 2026-05-07 `Document SDK API and clean up source formatting`
- `HEAD` 2026-05-07 `Refresh install/build docs and fix Win32 sound/FIX parity`
