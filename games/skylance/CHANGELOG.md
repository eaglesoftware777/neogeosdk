# Sky Lance — changelog

Sky Lance's ROM sets ship with the NeoGeoSDK pre-releases and are numbered
on their own: `_v1` with v1.7.3, `_v2` with v1.7.7.

## v2.1.0 candidate: shots, clouds, flicker, difficulty (2026-10-28)

Versions are now `MAJOR.MINOR.PATCH` and every changed binary gets a new
one (docs/releases/VERSIONING_POLICY.md). The title-safe rebuild below goes
out as v2.0.1; this pass changes gameplay and is v2.1.0.

- **Shots stuck on wings.** Aimed shots worked their velocity out in whole
  pixels, so a target within 16 pixels of the muzzle gave (0,0): the orb hung
  at the gun (the Crimson Keep's wing guns, a hovering heli) for the rest of
  the fight and held a place in the shot pool. Aiming is now 8.8 fixed point,
  no shot starts still, and a non-seeker shot that stops or outlives any
  crossing of the field is removed (`s_stale_bolts` counts them for QA).
- **The red spot on the wing.** The damaged frames had a small fire painted
  on the right wing: the player's jet below 35% hull, and a drone, fighter,
  interceptor, bomber, tank, SAM or radar site on its last third of armour.
  It was an enemy orb's size and colour and stayed put on the wing, so it
  read as a shot stuck there. Those frames now show charred plating instead
  (paint_jets.py, paint_sprites.py, unit_frames.py); the smoke trail and the
  red tint still mark a hurt jet. The minibosses, warships and the Keep keep
  their fires, which are spread over hulls far larger than an orb.
- **The white cloud followed the plane.** The overhead cloud kept a fixed
  screen x while the camera panned with the pilot; it now keeps its place on
  the field and pans 1.5 times as far as the ground. The low cloud bank was
  projected as distant terrain (a quarter of the ground's scroll and pan) and
  drifted along with the plane; it now scrolls a quarter faster than the
  ground, pans with it and crosses once every 1024 pixels at a new spot.
- **Bullets lost in crowds.** On a line with more than 96 sprites the LSPC
  drops the highest sprite numbers, and enemy shots had the highest fixed
  slots. The pilot portrait and the player's shots now go first; enemy shots
  still draw over every aircraft. Fire is held while the busiest line is
  within two sprites of the limit, and the low cloud bank stands aside when
  a line passes 88.
- **Difficulty.** A per-sector table (`k_difficulty`) tightens fire cycles
  to 60% by sector 7, adds lane-keeping patterns (paired fighter tracers,
  aimed drone darts, wide interceptor fans, heli and boat follow-ups, bomber
  side bombs), raises the live enemy-shot cap from 8 to 12 and aimed shots
  from 3.0 to 3.75 px per update. Sector 1 is unchanged. Rates and rules:
  DIFFICULTY.md.

## Title-safe HUD (2026-10-26)

- The flight HUD stays inside columns 3-36 of the 40 (x 24-295), the part
  of the picture every CRT shows: SCORE and HI with the callsign under
  SCORE (the aircraft model is on the briefing), the status line, the
  equipment row, the hull and the CHAFF count on the bottom row. The
  instruments and the portrait moved 8 pixels in, and the jet keeps clear
  of them.
- The status line is drawn again after the cloud warning and the super
  bomb's name, which used to leave it partly blank.
- The title's version and copyright are on rows of their own; the briefing
  boxes are 24 letters wide.

## v3 candidate (2026-10-20, local validation)

- Publish queued video earlier in the game-owned VBlank interrupt.
- Preload terrain in hidden whole columns rather than uploading a large recycled
  page at once; keep completed pages visible while the next page is prepared.
- Reserve independent sprite slots for the sortie player, portrait, enemies,
  boss and projectiles. Keep optional scenery behind these essential objects.
- Avoid repeated unchanged HUD writes and fixed-art projectile model checks.
- Four 80-second MVS stress captures cover stage indices0,1,3,6 and all pilots.
  They report no terrain preload/identity/partial-visibility or slot-owner faults,
  and no observed active-display gameplay video writes. Stage0 cadence improves
  from41.8 to48.7 updates/s; sustained60FPS is not achieved.
- Existing C/S/M/V payloads are unchanged. NeoSD fullscreen behavior, complete
  campaign/difficulty, remaining stages, AES and real hardware remain unverified.
  This is a local candidate; no released or installed cartridge was replaced.

## v2 (2026-10-18, NeoGeoSDK v1.7.7 pre-release) — the hardware fix

The Hardware RC1 correction below, now a regular set in every format (NeoSD,
Darksoft, BackBit and MAME, MVS and AES), built with SDK 1.7.7. Gameplay and
difficulty are unchanged from v1.

## Hardware RC1 (2026-10-14)

- Fix BIOS-dependent black terrain and aircraft by uploading a completed
  palette buffer at the start of the cartridge's VBlank interrupt.
- Keep the overhead cloud on a straight course when the player banks.
- Inset critical HUD information for CRT overscan; prioritize hostile bullet
  uploads and include cockpit instruments in the scanline budget.
- Preserve the v1 graphics and sound ROM payloads. Provide matched MVS/AES
  NeoSD images and a BackBit MVS package with checksums.
- Validate four 90-second MAME runs with EagleBIOS and Universe BIOS 4.0.
  Physical BackBit/NeoSD confirmation remains pending; difficulty is unchanged.

Downloads and test details: [Hardware RC1](https://github.com/eaglesoftware777/neogeosdk/releases/tag/skylance-hardware-rc1-20261014).

## v1 (2026-10-12, NeoGeoSDK v1.7.3 pre-release) — the first public set

### The game

- Three pilots: Rook in a silver-blue F-16, Kira in an olive F-35, Blaze in
  a crimson F-15, each with its own handling, primary fire and super bomb.
  Held lateral input banks to 45 degrees; B with a direction rolls, climbs
  or turns a Split-S; C drops a counted super bomb; B+D releases chaff.
- Seven long routes over painted terrain -- mountains, coast, a valley
  convoy, the open sea, a forest outpost, an armoured column and the final
  citadel -- with ground traffic on the roads, gunboats and submarines on
  open water, and three command targets per route, each guarded by escorts,
  a radar site and a mini-boss.
- A peacekeeping story: defeat the rogue faction, restore peace and save the
  population. Seven bosses, ending with a castle.
- Every boss is met in an open arena on its route's last page: deep sea for
  the warship, an empty desert for the tank fortress, a forest clearing, a
  lava plain. When the waves are spent the flight cruises there.
- Chaff can pull a guided missile onto a decoy, but each release rolls every
  missile: the odds fall with the route and the difficulty, newer seekers
  appear from route 4, and a boss's missiles see through most of it unless
  the operator chose EASY. A missile still locked on turns the radar scope
  red and the HUD reads LOCKED.
- Arcade settings (MVS): LIVES, HOW TO PLAY and DIFFICULTY (EASY to EXPERT,
  NORMAL by default). A console plays NORMAL.

### Presentation

- A painted title with the three pilots' faces under their jets.
- HUD: score and hi-score counters, route and target line, radio chatter,
  segmented hull and boss bars (a beaten mini-boss leaves no empty bar), and
  three cockpit instruments under the pilot's portrait: an attitude
  indicator that banks with the jet, a sweeping radar scope and a G meter
  with an afterburner lamp.
- Eight-frame explosions with a shock ring and drifting smoke; bosses,
  mini-bosses and command targets go up in a chain across the hull.
- Smooth play at 60 Hz: 91-98% of frames update the game on the busiest
  routes and 99.6% or more in boss fights (static HUD art in sprite groups, a
  per-frame scanline budget for shadows and clouds).

### Hardware

- Separate MVS and AES sets, EagleBIOS for emulator use; NeoSD, Darksoft
  and BackBit cartridge packages carry no BIOS. MVS sound start-up and slot
  switching keep the SDK's recovery paths.
- Physical MVS/AES boards are still to be verified.
