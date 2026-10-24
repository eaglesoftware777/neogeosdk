# Sky Lance inspection candidate — 2026-10-10

Installed AES/MVS build: **20261010-203856-530629**, at `C:\mame\neogeosdk`.
Run `run_skylance_mvs.bat` or `run_skylance_aes.bat`; matching scanline launchers
are also installed. This is an inspection candidate, not a hardware-certified release.

## Implemented behavior and files

- `scenes/sky_combat.c/.h`, `sky_sortie.c`: progressive whole-airframe banking
  reaches 45 degrees on a sustained turn. F-16, F-35 and F-15 inspired aircraft
  have distinct arcade acceleration, size and braking. B plus a direction starts
  a barrel roll, climb or Split-S. C launches the pilot-specific six-pulse strike;
  its icon and remaining stock appear in FIX. Stage progression increases damage.
- `scenes/sky_stage.c/.h`: smaller enemy classes have individual projectile art
  and attack patterns. SAM sites guide missiles. Three command installations per
  sector stop the scrolling scene, deploy escorts and a miniboss, and become
  vulnerable only after their defenders die. Destroying the exposed installation
  resumes travel. Radar coverage is drawn: avoiding it preserves the quiet route;
  entering it brings six reinforcements and faster antiaircraft fire. B+D expends
  one of six chaff charges and draws a decoy that attracts guided missiles.
- `scenes/sky_sortie.c`: fuel is a 240-second mission deadline; boost does not
  consume it separately. Fuel pickups add 30 seconds, capped at 240. Death does
  not refill it. Zero displays 000 and fails the mission. Briefing text wraps
  within its panel. The pilot face stays at (304,44).
- `tools/pixel_scenery.py`, `tools/art.py`: seven native 352x2048 pixel-art routes
  each contain eight different pages, with villages, farmland, ports, islands,
  airports, rail freight, industry and fortress terrain. They use explicit integer
  pixel drawing and exact palette indexing, without painting downsampling.
  Ships occupy reserved water lanes; traffic follows the road; command sites
  occupy clear pads. Clouds are native pixel clusters. Compact 32x64 consoles
  have live G/engine indicators and a separate 16x16 attitude instrument.
  Cockpits and handling are model-inspired arcade interpretations.
- `tools/redrawn.py`, `tools/contact_sheet.py`: bank poses, aerobatics, distinct
  weapon sprites, strike art and title portraits, plus native-size review sheets.
  Asset IDs remain stable; new entries are appended. Source/provenance and
  cockpit references are recorded in `artbox/redrawn/BUILD_NOTES.md`.
- `sdk/2d_engine/ng_sprite_group.c`: deferred group initialization and aged writes
  retain pending-list membership. Queue entries are populated before the count
  is published, so consumers cannot see an unfinished entry. The real-renderer
  regression fixture is `tests/sprite_deferred_test.c`.
- `tools/qa.py/.lua`, `tools/test_production.py`, `tests/test_skylance_*.py`:
  validate assets, placement, resources, collision recovery, radar, encounters,
  chaff, banking, special moves and native rendered captures.
- `tools/build_skylance_candidate.py`, `build_skylance_mame.py`,
  `tools/mame_tests/*skylance*.bat`: separate AES/MVS cartridge packages, a
  checksum-verified MAME install, full-screen launchers, optional scanlines and
  backups of replaced game files.

## Validation

Both AES and MVS compiled and installed successfully. Host suites passed:
18 Sky Lance asset/scenery/music tests, 13 production tests, two deferred-sprite
regressions and 12 existing Maiya tests. The atomic fade remains at 104 palette
banks; higher banks use the independent queue.

Ten native Windows MAME checks passed for this exact build. Their results,
CSV traces, PNG captures and WAV recordings are under:
`C:\mame\neogeosdk\tests\skylance\qa\20261010-203856-530629`.

| Inspection | Result |
| --- | --- |
| Stock MVS sector 1, quiet route, 270 seconds | Three targets; frozen defense/exposed phases; no radar alerts; one diverted missile |
| EagleBIOS AES sector 2, radar route, 310 seconds | Three targets; reinforcements; three diverted missiles |
| Stock AES sector 3, pilot 0 | 45° bank, aerobatics, bomb, streamed terrain |
| EagleBIOS MVS sector 5, pilot 1 | 45° bank, aerobatics, bomb, streamed terrain |
| Stock AES sector 7, pilot 2 | 45° bank, aerobatics, bomb, exact 450-HP boss |
| Stock MVS sector 4, pilot 2 | 45° bank, aerobatics, bomb, streamed terrain |
| EagleBIOS MVS sector 6, pilot 0 | 45° bank, aerobatics, bomb, streamed terrain |
| EagleBIOS MVS fuel exhaustion | Zero timer, 000 HUD, mission failure |
| EagleBIOS AES pilot 1 damage | Three injected hits and pixel-verified aircraft recovery |
| Stock MVS attract | Title matches encoded artwork |

All checks retained strict duplicate-upload detection, fixed portrait position,
active sound protocol and non-silent audio. Inspection routes seed power and
invulnerability to exercise features; they do not prove normal campaign balance.

Current ROM budget: P 152,290/393,216 bytes; C 2,760,192/8,388,608 (21,564 tiles);
S 14,560/131,072; M 8,778/32,768; V 2,553,600/8,388,608.

## Deliverables

Candidate directory:
`dist/prerelease-v1.7.3/SkyLance-20261010-204311`.
It contains eight verified AES/MVS MAME, NeoSD, Darksoft and BackBit packages,
plus their size/SHA256 manifest and test instructions. NeoSD/Darksoft V regions
are FF-padded to 8 MiB; BackBit preserves the six original payload lengths.
Hardware packages contain no BIOS. The Windows bundle includes EagleBIOS,
controls, both editions, full-screen/scanline launchers, art previews and these
hardware packages. No emulator is bundled.

Final setup archive: `SkyLance-Setup-20261010-203856-530629.zip` in that directory.
Its payload is round-trip verified during packaging. Windows payload verification and fresh-folder CLI installation both passed.
All 267 installed payload files match their expected size and SHA256. The
WinForms wizard was rendered and inspected. Results are in `INSTALLER_CHECK.json`
next to the archive, with `setup-preview.png` and `gameplay-review.png`.

Review images are under `games/skylance/artbox/generated/diagnostic_*.png`,
including seven full-route nearest-neighbor previews and banking/maneuver sheets.
Final gameplay captures are available in the MAME QA directory above.

## Boundaries

No commits or pushes were made. The 3,828-file Maiya pre-Sky snapshot matched exactly before beginning its
separately requested animation work. Its gait progress is in `../maiya/GAIT_STATUS.md`.
No shared sound-driver instruction or BIOS sound protocol was changed.

Physical AES/MVS cartridge audio, slot switching, controller shortcuts, complete
campaign play and frame-rate certification still require direct validation.
The independent programmed YM2610 soundtrack, continuous engine treatment and
bonus interludes from the earlier handover remain unfinished; the present tracks
are sampled compositions. Z80Ex-only handshake tests were skipped because that
host library is absent. These results do not certify exact aircraft instruments
or flight-simulator physics.
