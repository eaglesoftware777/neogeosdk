# Final Acceptance: v1.7.7 quality pass (2026-10-28)

What this pass changed, with the evidence, and what is still open. The
work went into the v1.7.7 pre-release on 2026-10-29 (the testers' fixes of
that day are listed under each game); the candidates' release files are in
`dist/release`, not yet uploaded as GitHub release assets.

## Delivered

| Part | Deliverable | Where |
|---|---|---|
| I | Baseline audit: repository, games, builds, tests, problems | `docs/qa/BASELINE_AUDIT.md` |
| II | ROM registry of every build in `dist/release` and its git history (432 contents), checksums, GitHub asset matching, MAME definition checks, HBMAME and FinalBurn Neo comparison, next version per game | `tools/rom_registry.py`, `releases/`, `docs/qa/ROM_INTEGRITY_REPORT.md` |
| II | Versioning policy; frozen versions in `make_release.py --replace` | `docs/releases/VERSIONING_POLICY.md`, `tools/make_release.py` |
| II | Database compatibility and updated entries for 7 sets | `docs/releases/EXTERNAL_DATABASE_COMPATIBILITY.md`, `releases/db-submissions/` |
| II | Reproducibility: every game rebuilt from HEAD; 6 of 7 released games byte-identical to their current release | `docs/qa/BASELINE_AUDIT.md` |
| VI | Sky Lance v2.1.0 candidate: shots, clouds, sprite order, difficulty | `games/skylance/`, `DIFFICULTY.md`, `CHANGELOG.md` |
| VI | Automated shot and cloud probe | `games/skylance/tools/shot_probe.py`, `docs/qa/skylance-probe/` |
| VIII | Every registered version in `C:\mame\neogeosdk` with its own launcher, a menu, verification, boot tests and a report | `tools/install_rom_versions.py`, `C:\mame\neogeosdk\README_TESTING.txt` |
| IX | Boot checks of every version (Linux MAME), animation checks with contact sheets, visual comparison | `tools/boot_versions.py`, `tools/animation_qa.py`, `docs/qa/ANIMATION_QA.md`, `docs/qa/VISUAL_REGRESSION.md` |
| X | Hardware matrix with the testers' reports | `docs/qa/HARDWARE_COMPATIBILITY.md` |
| XII | Release pipeline | `tools/release_all_games.sh`, `tools/release_all_games.bat` |
| Testers | Neo Geo CD discs as one BIN/CUE each (SD loaders, burning): No Signal v3, Maiya & Luna v2, Maiya v4-r2 and Sky Lance v2-r2 (stock and EagleBIOS CDZ) | `tools/neocd/bincue.py`, `dist/release/*-NeoGeoCD-*BinCue*.zip` |

## Per game

### Sky Lance

- **Wrong**: aimed shots fired within 16 px of the target got velocity (0,0)
  and hung at the muzzle (the probe held one still for 2999 frames on the
  v2 release); the overhead cloud kept a screen x while the camera panned
  (1 position in 648 frames) and the low cloud bank drifted with the plane;
  enemy shots held the sprite slots the LSPC drops first on a full line;
  difficulty flat across sectors.
- **Changed**: `scenes/sky_stage.c` (fixed-point aiming, `sky_spawn_bolt`,
  stale-shot removal, `k_difficulty`, sector patterns, fire hold),
  `scenes/sky_stage.h` (12 shot slots), `scenes/sky_draw.c` (cloud parallax
  and world anchoring, line peak), `scenes/sky_draw.h` (slot order),
  `CHANGELOG.md`, `DIFFICULTY.md`; QA tools `tools/shot_probe.py`,
  `tools/shot_probe_layout.c`.
- **2026-10-29, the red spot on the wing**: not a shot. The damaged frames
  (the player's jet below 35% hull, enemies on their last third of armour)
  had a small fire painted on the right wing, an enemy orb's size and
  colour. `tools/paint_jets.py`, `tools/paint_sprites.py` and
  `tools/unit_frames.py` now draw a charred, smoking core instead.
- **ROM**: v2.1.0 release candidate in `dist/release/SkyLance-*_v2.1.0.*`
  (build `72ea5e0d13e6`, rebuilt 2026-10-29 with the wing repaint;
  `tools/audit_release.py` PASS, 0 failures). MVS MAME set
  `SkyLance-WIP-EagleBIOS_v2.1.0.zip` CRC32 `eaaf3c60`, SHA-256
  `95c1d95baff6b5926373490552bbd7a1425e2205a564a61e9dc214d813a04997`; NeoSD
  MVS `.neo` SHA-256 `d614fb9fef2f268f17b17549bd98159afaece8c9f436e3b938478e3898ecd7a5`;
  every file in `SkyLance-MANIFEST_v2.1.0.txt` and `releases/SHA256SUMS.txt`.
  Database entries: `releases/db-submissions/skylance_v2.1.0.txt`. Launch:
  `C:\mame\neogeosdk\launchers\skylance_v2_1_0_mvs.bat` (or `_aes`).
- **Passed**: build (0 warnings, P 58% C 41% V 54%); shot probe (0 hung
  shots with the plane held under each enemy; 22 cloud positions; busiest
  line 65 of 96; enemy shots in slots 194-201, player 186); boot check.
- **Failed / open**: playtesting of the sector 3-7 difficulty; NeoSD Pro
  full-screen report needs a capture on the cartridge; real-hardware
  flicker check.

### No Signal Z Alpha

- **Wrong** (measured, not yet changed): enemies and bosses walk on 3
  distinct frames; the hero run has 4; Zerox drifts 5 px in its walk;
  Overmind's height swings 131-184 px.
- **Changed**: nothing in the game; contact sheets and metrics in
  `docs/qa/animation/nosignal/`.
- **ROM**: v3 (current), reproduced byte for byte; next release v3.0.1
  (the name went out as 2 builds).
- **Passed**: build, reproducibility, boot checks v1-v3 MVS/AES.

### Shadow League Fighting

- **Wrong** (testers, 2026-10-29): blurry select thumbnails, pink pixels on
  the fighters' outlines, faces without eyes or mouths in the fight.
- **Changed**: `tools/select_wall.py` (head crops, area-averaged and
  sharpened, 32 palettes for the wall), `tools/clean_fringe.py` (the magenta
  key painted out of nine fighters), `tools/painted_import/face_detail.py`
  (eyes, brows, noses and mouths from the source sheets painted into 41
  fighters' head pixels with each tile's own colours; silhouettes, boxes
  and animation unchanged), `CHANGELOG.md`, `README.md`.
- **ROM**: v3.1.0 candidate in `dist/release/ShadowLeague-*_v3.1.0.*`
  (build `5a598ffe6f33`; `tools/audit_release.py` PASS, 0 failures). MVS
  MAME set `ShadowLeague-WIP-EagleBIOS_v3.1.0.zip` CRC32 `4dd4cc4e`,
  SHA-256 `b8be45b7ad68615d2cec865cc1bab9246ecd90724d698e61b4eb893cac4da642`;
  NeoSD MVS `.neo` SHA-256
  `1493262bac639ce0278cd79d88ccd8cbcb803d41c087dfd45bb8d0ba70401be6`. v3
  stays as released; its re-release would be v3.0.1 (went out as 4 builds;
  HBMAME and FBNeo list different ones). V ROM 95% full.
- **Passed**: build (0 warnings); MAME duels MVS (Maia) and AES (Lucas),
  100 s each; boot checks v2-v3 and v3.1.0 MVS/AES.
- **Open**: the eyes still read as one dark band on most fighters, as in
  the approved frames; real eyes need redrawn heads.

### Maiya: Super Nature Girl

- **Found**: today's tree builds a different S1 from the v4 release (the
  fix layer cleaned of SNK tiles on 2026-10-27; P1, M1, V1, C1, C2 equal).
  That build is v4.0.1. The published v1.7.1-v1.7.7 downloads still carry
  the old S1.
- **Passed**: build; boot checks v2-v4 MVS/AES.

### Maiya & Luna

- **Changed**: nothing; scale and perspective work (Part V) needs captures
  and art.
- **ROM**: v2 reproduced; next v2.0.1 (HBMAME lists a v2 whose P1 is not
  ours).
- **Passed**: build, reproducibility, boot checks v1-v2.

### Seven Wonders Puzzles, Mahjong Demo

- **ROM**: v1 reproduced; next v1.0.1 each (went out as 3 builds).
- **Passed**: build, reproducibility, boot checks.

### SDK Demo

- **ROM**: v1.7.0 asset matches HBMAME `sdkdemo`; today's tree differs in
  P1 and M1 (newer SDK). Boot check passed.

## Tests

| Test | Result |
|---|---|
| Builds of all 8 games (`make neo`) | 8 passed, 0 warnings |
| Reproducibility against current releases | 6 identical; Maiya differs in S1 only (explained); SDK Demo has no release file |
| `tools/rom_registry.py --verify` | 212 of 212 registered files match (2026-10-29) |
| `tools/boot_versions.py` (31 MVS/AES sets incl. the v2.1.0 candidate) | 31 passed: both CPUs running, video written, MAME exit 0 |
| `tools/boot_versions.py`, 2026-10-29: Sky Lance v2.1.0 rebuild, Shadow League v3.1.0 (MVS and AES each) | 4 passed |
| Shadow League duels (`games/shadowleague/tools/qa.py`, MVS and AES) | 2 passed |
| Neo Geo CD BIN/CUE discs in MAME `neocdz` (EagleBIOS-CD) | 6 boot, screens identical to the CHD/ISO originals |
| Sky Lance shot probe | v2: 1 hung shot (2999 frames still); v2.1.0: 0 |
| Host unit tests `make -C tests test` | **fails at HEAD**: `test_move_touches_driver_only` (see BASELINE_AUDIT.md); not caused by this pass |
| Hardware (AES, MVS, NeoSD, BackBit, Darksoft, AES+) | NOT TESTED in this pass |

## Not done, and why

- **Pixel art** (Parts III-V): No Signal's enemy and boss walks (3
  frames), Maiya & Luna's scale and parallax and redrawn Shadow League heads
  still need new drawings (the Shadow League faces were recovered from the
  source sheets, not redrawn). The measurements and contact sheets that the art work
  starts from are in `docs/qa/animation/`.
- **Historical builds in the MAME folder**: the 46 builds replaced under the
  same name come out of git with `tools/install_rom_versions.py --history`
  (310 MB) once C: has room.
- **Release publication**: Sky Lance v2.1.0 and Shadow League v3.1.0 are
  in `dist/release` and the v*.0.1 re-releases are proposed; none is a
  GitHub release asset yet. Database submissions go out only after the
  matching GitHub release.
- **Owner actions**: GitHub Support for the pull-request refs and forks that
  hold the pre-cleanup history; re-uploads of the v1.7.1-v1.7.7 Maiya and
  Sky Lance v1 downloads.

## Where to launch

`C:\mame\neogeosdk\launchers\all_games_menu.bat` (every version),
`launchers\<set>.bat` (newest release), `run_all_tests.bat`,
`verify_all_roms.bat`, `generate_qa_report.bat`.
