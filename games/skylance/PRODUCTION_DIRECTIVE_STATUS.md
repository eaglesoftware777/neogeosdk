# Sky Lance production directive — working tree

Directive: `../sky_pro.txt`, applied after the completed Maiya gait repair.
Source baseline is the current dirty `fix/maiya-arenas-eagle-bios` checkout,
not the older remote PR #9. No commits, pushes, remotes or Git identity changes.

The preceding Sky Lance inspection candidate remains backed up and packaged.
This follow-on production pass is not yet a finished release.

## Verified baseline and production gaps

- Seven native 352x2048 routes, streamed as eight pages; coherent water lanes,
  roads and encounter pads. Current playfield is 288 pixels plus a 32-pixel
  instrumentation column. The directive's 320-pixel field description is older.
- Aircraft, normal enemies and bosses still use reduced concept masters in
  `tools/redrawn.py`. Native indexed Aseprite replacements are required.
- The three existing minibosses are shared-hull recolours; seven original
  mechanical designs are required.
- Camera lateral following is disabled. Terrain/cloud groups occupy physical
  slots 1–88; actor sorting begins at 96. Depth effects must fit that allocation.
- Existing airborne controls, radar/target defenses, mission deadline, pilot
  identity, cartridge support and queue fixes are preserved.

## Execution order

1. Native player model/animation sources, exact Aseprite export/import and pivots.
2. Stage 1 ordinary enemies, Ridge Warden, Crimson Keep, explosion art and terrain.
3. Extend existing shooter/depth helpers, sparse clouds and aircraft shadows;
   verify physical priority, scanline load and bounded camera response.
4. Stage 1 native AES/MVS showcase and measured frame/deferred-render evidence.
5. Stage 2 harbor/naval showcase, then the remaining five stage-specific designs.
6. Full art/engine/cart QA, reconstructed hardware previews and final packages.

Stage 1 is in progress. Stage 2 and seven-stage acceptance are not yet complete.
Maiya's completed source/art snapshot is `/tmp/sky-production-maiya-finished.json`.

## Stage 1 units, wrecks and the Crimson Keep's guns — 2026-10-11

Installed in `C:\mame\neogeosdk` (previous install backed up in
`backups\skylance\20261011-114446-490281`); every AES and MVS ROM is
byte-identical to the build that passed the checks below.

- **New unit drawings.** Drone, fighter, helicopter, bomber, tank, SAM and
  radar site are new indexed drawings in `assets/authored/units/*.png`, each
  facing down the screen with its own 12 colours plus three fire colours.
  Canvases by class: drone 32x32, fighter and helicopter 48x48, bomber 64x64,
  ground vehicles 32x48. `tools/unit_frames.py` builds each unit's eight
  frames from its drawing: three idle (nozzle or exhaust glow, the drone's
  sensor eye, the helicopter's rotor turning, the radar dish sweeping, the
  tank's tracks rolling), one damaged (scorch and fire), four destruction
  (aircraft flash, tear apart along a ragged line and fall away burning;
  ground vehicles flash, burn and char to a hulk). `production_pipeline.py
  --author` runs it before `author_stage_one.lua`, which now builds those
  seven documents from the strips instead of drawing polygons.
- **Palette banks.** Each unit loads its own palette; the Ridge Warden moved
  from bank 75 (now the drone's alone) to 76. `art.py` takes a native
  drawing's bank from its source.
- **Destruction frames play.** A defeated enemy becomes a wreck
  (`SKY_KIND_WRECK`, `sky_stage_wreck()`) that shows frames 4-7, six game
  updates each, under the usual blast; aircraft drift back, ground units stay
  on the ground. Enemies without a strip (interceptor, missile boat,
  submarine) still vanish as before.
- **The Keep's turrets own its guns.** Each side turret fires its own pair
  of shots, leaving the middle lane; a destroyed turret falls silent; with
  both gone the core's gun fires alone. Enraged, the core adds the aimed
  shot and ring flak as before.

| Check | Result |
| --- | --- |
| Aseprite author, reopen, export (89 frames) | PASS, unit frames identical to their strips |
| `make GAME=skylance art`, native C-ROM audit | PASS, 339 assets, 191 native |
| `make GAME=skylance all` | PASS, no warnings; P 48%, C 39%, V 54% |
| `tests/test_skylance_assets.py` (unit canvases, one palette per strip, distinct Stage 1 banks) | PASS |
| Native AES and MVS, features and boss inspections | PASS, scanline peak 54/96, no duplicate VRAM groups |
| Native AES and MVS `--mode turrets` (new) | PASS: both turrets fire, then the right one only, then the core only |
| Wrecks on screen during the MVS/AES feature runs | 14 samples each |

Found, not changed:

- Game logic updates on about 49% of video frames (`sky_frame` waits a
  second vblank when the VRAM window has closed). The 10:46 build measured
  the same, so it is not new; it was not looked into further.
- The 10:46 staged build failed the duplicate-VRAM-group check (1); this
  build has none.
- `test_production.py` (sound-effect hashes) and `test_skylance_music.py`
  fail against manifests last written on 9-10 October; the encoded files
  matched neither before this work.

Still old art: the Ridge Warden, the Crimson Keep and its turrets, the
explosions, the interceptor, the player aircraft, terrain, clouds and title.
New drawings for those were being generated as images to reduce into native
pixel art; that step was stopped by the session's permission check and needs
the owner's decision before it continues.

## Modern look, smooth flight — 2026-10-11

The owner asked for a modern, high-end arcade look while keeping the
gameplay, the roll, the title, and the units already redrawn: player jets as
true miniatures of the F-16, F-35 and F-15, new bosses, land and sea, and
complete boats and submarines. Movement felt stiff.

**Smooth flight (60 Hz).** The 68000 needed about 1.7 frames per update, so
the game ran its logic at about 30 Hz. Measured in MAME traces
(`~/.cache/skylance-art/prof`), fixed without touching gameplay:

- Every per-frame slot walk steps a pointer from `chars_at(0)` up to
  `ng_chars_slots_used()` instead of calling `chars_at(i)` (a call and a
  multiply by the 108-byte record) for all 64 slots.
- The strip budget is counted once a frame; the inspector's estimate is
  refreshed every eighth frame; the SDK clears the budget two lines per
  store.
- SDK: `ng_chars_draw`'s depth sort reads each character's key once
  (same order, still stable; a new host test covers negative sort keys).
- SDK fix: `ng_sprite_group_upload()` cleared a still-listed group's
  queued mark, so a later flush listed it twice (the duplicate deferred
  VRAM groups the inspector sometimes reported). New host test.

| Run (MVS) | Logic updates per video frame, before | after |
| --- | --- | --- |
| Features (full power, specials, many objects) | 49% | 95% |
| Boss fight | 49% | 99.7% |

**Painted art.** Full-colour paintings, reduced to the native canvas and
fitted to several hardware palettes, one per 16x16 tile (`tools/painted.py`,
families in `assets/painted/painted.json`; each family is fitted as one
picture so changing frame never recolours a live sprite). Sources are in
`assets/painted/sources` (cut-outs, no metadata).

- Player jets (`tools/paint_jets.py`): F-16, F-35, F-15 from a top, side
  and underside painting each, on a 64x64 canvas with four palettes. Bank
  poses roll the top view about its long axis (wings shorten by
  cos(angle), the lower wing brightens toward the light); the roll's knife
  edge is the side painting, past it the underside; boost stretches the
  painted afterburner; damage and destruction are drawn over them. Ground
  shadows stay three strips wide. The hitbox is unchanged (12x12).
- Crimson Keep and its turrets (one 16-palette family), with the turrets
  seated in the hull's sockets (`SKY_KEEP_TURRET_X/Y` 27/13) and the core
  hitbox on the new reactor; the Keep now plays its destruction frames.
- Ridge Warden (banks 76-77), interceptor (now a full strip with
  destruction frames), explosions (48x48), missile boat and submarine
  (32x64, whole hull and wake), the two clouds (banks 100-101).
- Stage bosses: Iron Tide battleship, Sol Core, Night Razor, Rotor Nest,
  Earth Hammer and the Crimson Citadel, a flying castle (banks 56-71, one
  boss at a time). No religious buildings or signs anywhere: the final
  boss's asset is `castle` (`SKY_B_CASTLE`), its old source image renamed;
  `ART_BIBLE.md` is now `ART_GUIDE.md`; the FM patch "Temple Gong" is
  "Bronze Gong".

**Painted land and sea (`tools/paint_terrain.py`).** Every stage's map is
stacked from painted panels in flight order (Stage 1 seven, the others four),
cross-faded at the seams, the valley road painted at world X=92 and concrete
target pads on pages 2, 4 and 6 of land routes; the sea lanes at X=80 and
192 stay open water (85%, reefs and a pier end allowed). Each map is fitted
to its sixteen terrain banks, then refined (every tile takes its best
palette, each palette is refit to its tiles) so dark water no longer breaks
into flat squares; pages are laid out in streaming order.
`mountain_route.route()` and `pixel_scenery.route()` return the painted
maps; Stage 1's still round-trips through its Aseprite route document.

**Story.** Sky Lance is a peacekeeping mission: defeat the rogue faction,
restore peace, protect the population.

**Sources.** The full-colour paintings in `assets/painted/sources` are cut
out on transparency (no metadata), then reduced to native size and fitted to
hardware palettes by the tools above.

**Checks (final build, installed).** Installed in `C:\mame\neogeosdk`
(previous Sky Lance backed up in `backups\skylance\20261011-201840-477819`);
every AES and MVS ROM is byte-identical to the build inspected below.

| Check | Result |
| --- | --- |
| `make GAME=skylance art`, native C-ROM audit | PASS, 346 assets |
| `make GAME=skylance all` | PASS, no warnings; P 52%, C 40%, V 54% |
| Sky Lance asset, scenery, sprite and shooter host tests | PASS (painted families share palettes; maps keep pads, road and sea lanes) |
| MVS features, Stages 1-7 | PASS; logic updates on 89-97% of video frames; scanline peak 58-65 of 96; no duplicate VRAM groups |
| MVS and AES boss and turret runs (Stage 1) | PASS, 99.7-99.9% |
| MVS boss runs, Stage 2 (battleship) and Stage 7 (castle) | PASS, 99.8-99.9% |

Still failing from before this work: `test_skylance_music.py` and the
sound-effect hash test in `tools/test_production.py` (manifests written on
9-10 October that match neither the committed nor the current samples).
Not changed: the title, the HUD and the pilot portraits.

## Placement, title and HUD — 2026-10-11

**Everything where it belongs.**

- Boats and submarines spawn only in a lane that is open water along their
  hull and the whole column below them (craft ride the terrain, and are sent
  on down their column to clear the field before an encounter). The art
  build writes a water map for the two naval routes (`sky_route_water`, a
  bit per 16x16 cell); `sky_bg_water()` reads it under any screen point.
  With no open lane in reach, that hull is skipped.
- At sea the command targets are warships, an AA cruiser and a radar
  picket (`naval_aa`, `naval_radar`, full strips on banks 119 and 121, the
  banks of two legacy drawings the game never shows), anchored on open
  water; on land the SAM and radar trucks stand on the painted pads as
  before.
- A miniboss guarding a command target enters on the far side and patrols
  only there, at least 88 pixels from the target, so it never covers it.
- Tanks and traffic stay on the road, ground targets on their pads (both
  ride the terrain).

**Title.** A painted title (`assets/painted/sources/title_screen.png`):
chrome SKY LANCE logo, the F-16, F-35 and F-15 climbing over the clouds at
dawn, on the title's sixteen banks; the MVS title prints the company line.

**HUD.** `tools/hud_art.py`:

- One squared 5x7 face on the game's ASCII page (the BIOS page keeps its
  font): upper rows light ink, lower rows a darker shade, a one-pixel drop
  shadow, no opaque plate behind the letters. `user.c` sets the four inks.
- Counter digits, SCORE / HI / 1P-3P labels, energy cells and the life pip
  (the F-16 painting) redrawn to match; pickups and the six equipment
  badges are glossy two-tone badges with the same symbols.
- Decluttered: the cockpit console and attitude panels and the
  G/T CHAF B+D labels are gone; the chaff stock reads CHAFF n under the
  callsign. Their banks (124-126) now hold the two new minibosses.

**Minibosses.** All three are painted strips with destruction frames: the
Ridge Warden, a tilt-rotor gunship (stages 2 and 5) and a heavy
ground-attack jet (stages 3 and 6).

**Terrain pixels.** After the palette fit, single-pixel specks (a pixel
unlike all four neighbours, three of which agree) take their neighbours'
colour, so the painted maps read as clean pixel clusters.

**HUD layout.** Rows 0-1: SCORE label and counter (cols 0-16), HI (18-31),
aircraft and callsign (36-39); row 2: TGT, stage, radar, flight state,
CHAFF; row 3: radio chatter; row 4: boss bar. Nothing overlaps any more
(the counters' second row used to cover TGT and the radar line).

**Inspector.** The road-anchor check now compares each car with the ground
page under it. It used to read page 0 only, and when that page has just been
recycled above the field its x can trail the camera by a frame or two while
its new tiles upload; the car and the page under it always agree.

**Checks (this build, installed).** Installed in `C:\mame\neogeosdk`
(previous Sky Lance in `backups\skylance\20261011-212455-873139`); every
AES and MVS ROM is byte-identical to the build inspected here.

| Check | Result |
| --- | --- |
| `make GAME=skylance art`, native C-ROM audit | PASS, 376 assets |
| `make GAME=skylance all` | PASS, no warnings; P 54%, C 40%, V 54% |
| Sky Lance asset, scenery, sprite and shooter host tests | PASS |
| MVS features, Stages 1-7 | PASS; 93-98% of frames update the game; peak 56-66 of 96 |
| MVS and AES boss, turrets, AES features | PASS |
| MVS encounters at sea (Stages 2 and 4) | PASS, warships on open water |

## Arenas, chaff, instruments and explosions — 2026-10-12

**Boss arenas.** Every boss is now met on its route's last page, an open
arena painted for it and laid over that page by `tools/paint_terrain.py`
(`sources/arena_<theme>.png`, cross-faded into the route below; the road
stops at the seam and no traffic is sent onto it): open deep sea for the
warship (Stage 2), a bare valley floor for the Sol Core (3), a forest
clearing for the carrier (5), empty desert for the tank fortress (6), a
scorched lava plain for the citadel (7) and an alpine plateau for the
Crimson Keep (1); the open-sea route (4) already ends in open ocean. Once the
waves are spent the flight cruises to the arena at four times the speed
("PROCEED TO TARGET AREA"), still stopping for any command target ahead,
and the boss appears only when the arena fills the screen. The warship steams at
half the other bosses' pace, and its whole lane is open water.

**Chaff.** Each release now rolls every guided missile in the air on its
own, so some can stay locked ("STILL LOCKED! BREAK!", the radar scope turns
red, the flight state reads LOCKED). The chance falls with the route and the
difficulty, routes 4-7 field newer seekers, and a boss's own missiles see
through most of it except on EASY:

| Fooled by one release | EASY | NORMAL | HARD | EXPERT |
| --- | --- | --- | --- | --- |
| Route 1 missiles | 92% | 80% | 68% | 56% |
| Route 7 missiles | 75% | 47% | 24% | 15% |
| Boss missiles (route 1 / 7) | 85% / 79% | 45% / 39% | 35% / 29% | 25% / 19% |

The difficulty is a new operator setting (soft DIP DIFFICULTY, EASY to
EXPERT, NORMAL by default; a console plays NORMAL). The three region tables
that held a placeholder name became one shared table named SKY LANCE.

**HUD.** The mini-boss bar is cleared when it is beaten instead of staying
as an empty frame. Under the pilot's portrait, in the HUD column right of
the field, stand three instruments drawn by `tools/instruments.py`: an
attitude indicator that banks with the jet, a radar scope that sweeps (red
under a radar lock or a missile still on the jet) and a G meter with an
afterburner lamp. They are fixed sprite groups (slots 360-365), not
characters: in the busy profile the game updated 75 of 120 frames with
them as characters, 93 as groups.
Meters are segmented and shaded; the hull reads cyan, gold when damaged, red
when one more hit is fatal; RADAR:ALERT and LOCKED blink in the warning ink.

**Explosions.** A new eight-frame painting at 64x64 (flash, shock ring,
fireball, break-up, smoke) on two palettes, each frame held a little longer
than the last, the smoke falling behind with the ground. Up to four burn at
once, and big kills go up in a chain across the hull: ten blasts for a
boss, four for a mini-boss, three for a command target, two for a turret.

**Title.** The three pilots stand under their jets on cards drawn by
`tools/pilot_cards.py` (portrait reduced from its full drawing, callsign
plate); every title screen reads (C) 2026 EAGLE SOFTWARE.

**Palette banks.** The build keeps every bank below 128. The orb, ring and
three old item drawings, which the game never shows, share bank 94; the
instruments take 95-97 and the explosions 98-99; the title cards borrow the
bosses' 56-61 (never on screen together; each scene loads its own palettes).

**Checks (this build, installed).** Installed in `C:\mame\neogeosdk`
(previous Sky Lance in `backups\skylance\20261011-231349-665279`); all 12
AES and MVS ROMs are byte-identical to the build inspected here.

| Check | Result |
| --- | --- |
| `make GAME=skylance art`, native C-ROM audit | PASS, 412 assets |
| `make GAME=skylance all` | PASS, no warnings |
| Sky Lance asset, scenery, sprite and shooter host tests | PASS (explosion and road tests updated to 8 frames and the arena page) |
| MVS boss, Stages 1-7 (each in its arena) | PASS; 99.6-99.9% of frames update the game |
| MVS features, Stages 1-7 | PASS; 91-98% of frames update the game; peak 63-69 of 96 |
| MVS turrets, AES features and boss | PASS |
| MVS encounters at sea (Stages 2 and 4) | PASS |
| `qa.py --encounters --seconds 170` (installed) | PASS: 3 targets, 2 missiles decoyed, route end reached |

`qa.py --encounters` at its 45-second default fails on time alone: the third
command target is about 100 seconds into the route.
