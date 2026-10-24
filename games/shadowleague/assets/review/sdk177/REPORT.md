# Shadow League V1 hardware-test candidate, 18 October 2026

This is an isolated game/fighting-component candidate for SDK 1.7.7. It is not
an SDK release or a claim of finished visual production. No commit, push or
publication is part of this work. Other games and genre engines are untouched.

## Changes delivered

- `sdk/2d_engine/ng_fight.c/.h`: calibrated seven-rank CPU policy, default 3 in
  the game, observed-attack reaction delays, projectile selection, preserved
  difficulty across rounds/teams, configurable round time, bounded training
  input recording/playback, nonlethal practice, throw alignment/lift/release,
  saturated combo counters and deterministic hash coverage.
- `scenes/slf_game.c`, `slf_practice.c`, `slf_settings.h` and platform headers:
  MVS software DIPs, AES home/options/training screens, command guides, long,
  short and reverse throws, original projectiles and special effects. AES
  defaults are initialized explicitly because writable data is NOLOAD on this
  cartridge. Practice input text is built in writable stack storage.
- `slf_draw.c`: hidden pose banks, separate build/upload/visible packet ownership,
  bounded early upload work, stable actor slots, disjoint projectile/striker
  reservations, per-pose roots and wider 192-pixel source canvases. Effects have
  separate palettes and cannot displace important hit sparks.
- Fighter definitions/cooker/validator: original energy/flame moves, per-pose
  collision metadata, long/short/lift throw data and command descriptions.
- Native art/import/packing: 44 complete fighter sets, 2,460 runtime frames,
  explicit detailed selection faces, four tile palette banks per actor, exact
  decoded ROM pixel checks. Missing poses and opaque cropping fail. Jack's new
  56-pose set uses boxing attacks and a tank top/gloves; Batu has a closed jacket
  and shorts. Malik/Viktor/Ryuhei use restored uncropped wider exports.
- Encoded title and skippable introduction have original tournament imagery.
  Four current arena backgrounds remain unchanged.
- Audio: 59 localized lines for 15 fighters; 209 stable voice entries plus 16
  effects. A game-only build dependency synchronizes approved named WAVs before
  ADPCM encoding, preventing stale numbered samples. Source licenses are in
  `assets/authored/voices/voice-sources-sdk177.md`.
- `install_milestone.py`: game-specific verified installs, no new backups.
  `package_v1.py`: private V1 NeoSD/Darksoft AES/MVS, BackBit MVS and MAME ZIPs;
  decoded payloads must match all six candidate ROMs exactly.

## Review evidence

`jack-correct-boxing-native.png`, `new-faces-review.png`, the roster review sheets
and `animation-previews/` show native source exports. `v1-gameplay-contact-native.png`
and its 2x nearest-neighbour version show actual cartridge frames. The final
MAME scenario directories contain native screenshots, ROM/ELF hashes, logs,
controller-driven selection and read-only hardware telemetry.

Flat fighter exports have no separate anatomy layers. Pixel-integrity checks
therefore do not certify anatomy, movement appeal or professional visual quality.
No independent artist/player approval is asserted. An identical-scenario V2
before/after acceptance comparison has not been completed.

## Failures caught without relaxing gates

The initial whole-actor upload exceeded its drawing deadline. Hidden bank uploads
and bounded column work replaced it; the same zero-deadline-miss gate is retained.
Training reset initially exposed a 60-second frame; reset now assigns 99 immediately.
A reverse-throw trajectory regression was caught by actual command tests and fixed
by respecting the victim's side at release. The audio build originally encoded
stale numbered WAVs; the game-specific synchronization dependency fixes that path.

Visual review of the first V1 AES capture exposed zero-valued rounds/time: the
linker places initialized writable globals in cleared NOLOAD RAM. Runtime defaults
now set level 3, two wins and 60 seconds. The emulator gate was extended to inspect
those initial values and every displayed rounds/time setting. Moving practice text
to a local initialized array exposed this freestanding target's absent `memcpy`;
explicit character stores avoid that dependency. The failed build log is retained.

## Remaining work

Player-reported occasional slowdown/flicker, more fluid movement, arena art/depth
and further fighter/title polish remain open. Zero late actor uploads in sampled
scenarios do not prove every possible sprite-load combination is safe. Worst-case
four-large-actor plus all-effect stress coverage remains incomplete. Full native-
language voices and human performance direction are unfinished. Physical NeoSD,
Darksoft, BackBit and original MVS/AES testing are pending; packaging integrity and
MAME are separate from cartridge compatibility. This candidate does not claim
1080p native hardware output or parity with a finished commercial fighter.

Final build, telemetry, installation and source integration records follow below.


## Final validation

- Art conversion: 44 fighters / 2,460 frames; zero pixel loss and exact C-ROM
  reconstruction. Four palette banks maximum per fighter. Flat anatomy layers: 0.
- Host: fighting-core test PASS; 25 game tests PASS; 9 sound-tool tests PASS.
  Actual inputs cover 347 specials/supers, all 44 throw sets and four close-range
  three-hit combos. API freeze: 315 retained prototypes / 65 additions.
- AES and MVS compile/link/package PASS. The final code build is
  `build-v1-settings-fixed.log`, using the validated unchanged C/S/M/V assets.
- `make GAME=shadowleague test-precheck budget` PASS. No level exporter applies
  to this arena fighter; level-check reports nothing to check. Build retains the
  existing unused `mark_at` helper warning; no link errors remain.
- MAME: final Jack duel 90 seconds, team 115 seconds, AES options/training 100
  seconds and attract 72 seconds PASS. See `validation-v1.json` for matching ELF,
  ROM hashes and measurements. Duel/team/training peak strips: 60/61/58; actor
  deadline misses: 0/0/0; skipped simulation frames: 5/13/28 respectively. Active
  observation counts: 3195/4368/4601. These runs do not close slowdown reports.
- Attract: seven presentation cards, both aligned physical program banks, eight
  data copies and no copy errors. Team: reserves enter and striker stock is used.
- AES: starts level 3 / two wins / 60 seconds; all seven settings exercised;
  practice stays nonlethal at 99 seconds.
- Actual cartridge audio: stereo 48 kHz capture, nonzero waveform. The 18-second
  `v1-jack-gameplay-1080p.mp4` has mean audio -17.0 dBFS. Its 320x224 source capture
  is archived losslessly; decoded pixel and PCM hashes match.
- V1 packaging: NeoSD/Darksoft AES/MVS, MAME AES/MVS and BackBit MVS PASS exact
  payload reconstruction. Hardware cartridges remain untested.
- ROM capacities: P 3 MiB (one fixed plus two aligned banks), C 16 MiB (equal
  lanes), V 7,979,264 bytes before power-of-two hardware padding. The budget tool
  reports P 66.8%, C 62.3%, S 41.4%, M 18.1%, V 95.1% used; emitted/reserved tile
  positions and allocated capacity are different measures.

Installer target: `C:\mame\neogeosdk`; V1 launchers route only to the private
SDK177 game directories. Hardware-test packages are in
`dist/candidates/shadowleague/sdk177/hardware-v1/` with checksums. The parent
`shadowleague-sdk177-coordination-2026-10-17.md` records ownership/recovery and
integration guidance. The parent portable source archive includes actual raw
source content rather than the live checkout's absolute Linux source symlink.

Installation verification: 23 installed payload/list files matched their SHA256
and sizes. An inventory of 14,224 other game/shared files retained the same paths,
sizes and modification times during installation. No new backups were created.
The installed MVS set also passed a direct 35-second Jack gameplay test, using
the installed ROM/hash directories rather than the build directories. Dedicated
`START_SHADOW_LEAGUE_V1.bat` opens AES; the MVS milestone BAT opens arcade mode.
See `install-v1-verification.json` and `qa-v1-installed-smoke.log`.

`git diff --check` passes; tracked diff details are in `diff-stat-v1.txt`. New
source/art/test files are additionally inventoried by the portable source archive,
since Git's tracked-only diff stat does not include untracked files.
