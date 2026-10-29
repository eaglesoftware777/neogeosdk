# Baseline Audit (2026-10-28)

The state of the repository before the v1.7.7 quality pass changed anything,
with the evidence behind each line. Later reports in this folder build on it.

## Repository

| Item | State |
|---|---|
| Branch | `fix/maiya-arenas-eagle-bios`, HEAD `c7bf0b88` ("GitHub cleanup"), the same commit as the remote branch and open PR #9 |
| Remote | 3 branches (`main`, `fix/maiya-arenas-eagle-bios`, `neo_universal_2d`), 13 tags (v1.0 ... v1.7.7, framework-v1.0, skylance-hardware-rc1-20261014), 11 GitHub releases |
| History | rewritten 2026-10-27 to purge SNK BIOS files and fix tiles; GitHub's pull-request refs #2 to #8 and the forks still hold the old history (only GitHub Support can purge them) |
| Uncommitted work found | `sound/driver/sample_table.inc` (kept), build outputs under `out/`, two No Signal CD v3 text files in `dist/release` (kept); nothing was reverted |
| Toolchain | `x-tools-v3` (GCC 16, m68k-unknown-elf) at `C:\neogeo\x-tools-v3`; Windows builds `x-tools-v3-win` |
| Emulators | Windows MAME 0.283 (`C:\mame\mame.exe`), Linux MAME 0.289 (snap, needs `DISPLAY=:0`) |

## Games

| Game | ID | Engine | Source | Current release | Targets |
|---|---|---|---|---|---|
| SDK Demo | 777 | shared 2D engine (scenes) | 23 files, 23.9k lines | v1.7.0 asset only | MAME |
| SDK Demo PLUS | 778 | demo scenes reused | 7 files | none | MAME |
| Sky Lance | 779 | own shooter code on the 2D engine (`ng_shooter`, depth module) | 13 files, 9.8k lines | v1, v2 | MVS, AES, NeoSD, Darksoft, BackBit, CD |
| Maiya: Super Nature Girl | 780 | own platformer on the 2D engine | 204 files, 124.8k lines | v2, v3, v4 (local files only) | MVS, AES, NeoSD, Darksoft, BackBit, CD |
| Shadow League Fighting | 781 | `ng_fight` (fighting genre engine) | 34 files, 15.3k lines | v2, v3 | MVS, AES, NeoSD, Darksoft, BackBit |
| Seven Wonders Puzzles | 782 | genre engine `puzzle` | 9 files | v1 | MVS, AES, NeoSD, Darksoft, BackBit |
| Maiya & Luna | 783 | genre engine `rpg` | 11 files, 11.8k lines | v1, v2 | MVS, AES, NeoSD, Darksoft, BackBit |
| No Signal Z Alpha | 784 | genre engine `beatemup` | 11 files, 5.2k lines | v1, v2, v3 | MVS, AES, NeoSD, Darksoft, BackBit, CD |
| Mahjong Demo | 785 | genre engine `mahjong` | 9 files | v1 | MVS, AES, NeoSD, Darksoft, BackBit |
| Hello World, Tutorial, My NeoGeo Game | 772, 555, 775 | examples | small | none | MAME |

Build entry points: `make neo GAME=<game>` (AES then MVS sets and NeoSD
images in `dist/neo/<game>/`), `tools/make_release.py` (versioned release
files in `dist/release/`), `tools/audit_release.py` (hardware fixes in a
release), `tools/mame_bats.py` (the launchers in `C:\mame\neogeosdk`).

## Baseline builds and reproducibility

Each game was built with `make neo` from HEAD, one at a time (`out/` is
shared), and its output compared with the previous build left in
`dist/neo/` and with the registered release files.

| Game | Result | Time | Warnings | Against the earlier dist/neo build | Reproduces release build |
|---|---|---|---|---|---|
| Sky Lance | exit 0 | 404 s | 0 | identical | v2 (current) MVS `aeea2ed21c52`, AES `e7a7cf41727a` |
| No Signal | exit 0 | 308 s | 0 | identical | v3 (current) MVS `15b597e00a46`, AES `328e4b0f9ec5` |
| Shadow League | exit 0 | 1040 s | 0 | identical | v3 (current) MVS `20243d02ff60`, AES `d3625c5bf27c` |
| Maiya & Luna | exit 0 | 315 s | 0 | identical | v2 (current) MVS `7f446d36b854`, AES `3682640e1299` |
| Seven Wonders | exit 0 | 149 s | 0 | identical | v1 (current) MVS `282b83a2daf1`, AES `fc9fa1aff4d0` |
| Mahjong Demo | exit 0 | 103 s | 0 | identical | v1 (current) MVS `8b12abace5c6`, AES `180110aadf4e` |
| Maiya | exit 0 | 156 s | 0 | S1 differs (7 files) | none: P1, M1, V1, C1, C2 equal v4, **S1 differs**. The 2026-10-27 cleanup removed SNK tiles from the fix-layer sources after v4 was packed; the tree now builds the cleaned S1, which must ship as v4.0.1 |
| SDK Demo | exit 0 | 596 s | 0 | no earlier build | no dist/release file (v1.7.0 asset differs in P1 and M1: newer SDK code) |

A fresh build of HEAD reproduces the released bytes exactly: the builds
are deterministic for the same source (timestamps are not embedded). The
release builds were made 2026-10-26 and rebuilt today with the same result,
which is the "two clean builds" check of the directive. Sky Lance was also
built a third time from HEAD for the before/after probe, identical again.

ROM budgets (`tools/rom_budget.py`): Sky Lance P 58%, C 41%, V 54%;
No Signal P 19%, C 9%, V 48%; Shadow League P 67%, C 62%, **V 95%**.

## Tests found

| Suite | Result at HEAD |
|---|---|
| `tests/` host unit tests (`make -C tests test`) | **FAILS**: `test_c` stops at `sprite_render_test.c:202`, `test_move_touches_driver_only`. Rebuilt from current sources it fails the same way. Instrumented: the first move after the previous test's vertical shrink change writes 10 VRAM words, later moves the expected 2. The test aborts the suite, so the tests after it did not run at HEAD. |
| `tools/audit_release.py` | per-release hardware checks (P/S/M sizes, eye-catcher, NeoSD alignment, EagleBIOS only); used for every release so far |
| Sky Lance `tools/qa.py` + `qa.lua` | Windows MAME captures (features, damage, encounters, boss) recorded in its PRODUCTION_STATUS.md |
| Maiya `regression.py` | known failure `pickups` on the re-laid Emerald Forest (project notes) |
| `tools/genre_qa.py`, `tools/game_capture.py` | genre and capture harnesses |

## Problems found

| # | Problem | Evidence | Priority |
|---|---|---|---|
| 1 | Release names reused for different binaries: 84 release files had more than one content; HBMAME and FinalBurn Neo list different builds of Maiya v4 and Shadow League v3, neither the current one | `docs/qa/ROM_INTEGRITY_REPORT.md` sections 2 and 7 | P1 |
| 2 | `make_release.py --replace` rewrote released versions in place | the tool's own help | P1 (fixed: frozen versions) |
| 3 | GitHub release assets for v1.7.1 to v1.7.7 Maiya and Sky Lance v1 still carry the old fix ROMs with SNK tiles | 2026-10-27 cleanup notes; registry section 3 (59 assets not in the repository) | P1, owner action |
| 4 | Sky Lance: aimed shots could stop dead at the muzzle; the overhead cloud kept a screen x while the camera panned; enemy shots sat in the first sprites dropped on a full line; flat difficulty | code review, `games/skylance/DIFFICULTY.md`, shot probe | P3 |
| 5 | No Signal: enemies and bosses walk on 3 distinct frames; the hero run has 4 | `docs/qa/ANIMATION_QA.md` | P4 |
| 6 | Host unit test failure above | `tests/` | P2 |
| 7 | Tracked generated files drift: Sky Lance's `sound/samples/out_b/*.adpcmb` are 16 kHz in git, every build writes 32 kHz (the released V ROM holds the 32 kHz data) | `git diff --stat` after a build | P2 |
| 8 | C: drive nearly full (1.5 to 2.5 GB free; WSL's 131 GB disk, a 6 GB page file and the hibernation file live there); a full C: crashes WSL | `df`, page file growth to 6 GB during this pass | P0 for the workstation |

## WSL and disk notes for anyone running the pipeline

- Builds and MAME runs write to C:. Keep at least 3 GB free; the pipeline
  refuses to start with less.
- Windows MAME reserves a lot of memory per run; with the browser holding
  most of the RAM, a run grew the page file by 1 GB on C:. The unattended
  checks use the Linux MAME inside WSL, under a memory cap.
- After moving data out of WSL, `C:\neogeo\compact_wsl.bat` (run as
  administrator, with WSL closed) shrinks WSL's disk file and returns the
  space to C:.
