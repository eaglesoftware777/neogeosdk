# NeoGeoSDK v1.7.7 — the games release

**Eagle Software · MIT licence · Neo Geo MVS, AES and (experimental) Neo Geo CD**

NeoGeoSDK v1.7.0 was the 2D engine release: one engine, plain C or C++14,
building real cartridges. v1.7.7 is what came of using it. Between the two
sit five pre-releases (v1.7.1, v1.7.2, v1.7.3, v1.7.5 and the v1.7.7
pre-release), seven playable games written on the SDK, a fighting core, four
genre engines, an open system ROM, a Neo Geo CD backend, and the release and
test tooling that keeps every ROM accountable. This page is the whole story
since v1.7.0; the line-by-line record is in [CHANGELOG.md](../../CHANGELOG.md)
and each game's own `games/<name>/CHANGELOG.md`.

## At a glance

- **Seven games** that build to real ROMs from this repository: Maiya: Super
  Nature Girl, Sky Lance, Shadow League Fighting, No Signal Z Alpha, Maiya &
  Luna, Seven Wonders Puzzles and Mahjong Demo, each with MVS and AES sets,
  NeoSD images, Darksoft and MAME packages.
- **Framework 1.0 to 1.5**: the C engine's public calls were frozen at
  v1.7.1 and have grown by additions only since; existing games build
  unchanged.
- **Frames that never tear**: video memory written only in the vertical
  blank, with a deadline, priorities and a queue that never writes on a
  drawn line (`NG_VRAM_DEFER`); sound commands queued (`NG_SOUND_QUEUE`).
- **New engines**: a vertical-shooter core (`ng_shooter`), a fighting core
  (`ng_fight`) and four one-file genre engines (puzzle, mahjong, RPG,
  beat'em-up) chosen per game with `GAME_GENRE_ENGINE`.
- **EagleBIOS**: an open system ROM for MVS and AES, with memory cards, the
  clock, diagnostics and a game's own HOW TO PLAY page.
- **Neo Geo CD (experimental)**: native CD program building, disc authoring
  (ISO, BIN/CUE) and CD ports of four games.
- **Every ROM named once**: a ROM registry, a versioning policy and audits
  that refuse to reuse a released name for different bytes.

## The games

| Game | ID | Genre | What it is |
|---|---|---|---|
| Maiya: Super Nature Girl | 780 | action platformer | Twelve stages (ten valleys, the Sky Road and the Smog Citadel), guardians behind sealed gates, a hidden vault per valley; play as Maiya or her sister Luna |
| Sky Lance | 779 | vertical shooter | Seven painted routes to boss arenas, three jets with banking and rolls, sector difficulty |
| Shadow League Fighting | 781 | fighting game | 44 painted fighters, 12 arenas, single and three-on-three team battles with strikers, a referee's ceremony, voices and a score with bonus counts |
| No Signal Z Alpha | 784 | beat'em-up | Eight streets for one player, weighty blows, grabs and throws, a ridable scooter, living backgrounds |
| Maiya & Luna | 783 | adventure RPG | Two solo journeys through seven chapters of rooms, puzzles, people and saves |
| Seven Wonders Puzzles | 782 | puzzle | Match and bubble boards and picture-logic levels, an album and rewards, saves |
| Mahjong Demo | 785 | mahjong | Four-seat Hong Kong Old Style against three CPU players, and solitaire |

Release packages carry each game's version number (`MAJOR.MINOR.PATCH`) and
build identity; see "Versions in this release" below.

### What changed in the games for the release

- **Shadow League Fighting**: every round and every team bout now ends with
  the winner's pose over the loser; a beaten side kneels at the match's end,
  the whole team after a team battle. A score runs through the arcade game,
  with a bonus count after each round won (life, time, untouched) and two
  match finales: TRIPLE UNTOUCHED for three wins without a blow taken and
  CLEAN SWEEP when one fighter beats a whole team, each with its own fanfare
  and call. A new HUD: framed faces, short names, team markers, the
  striker's calls left and a guard gauge that only shows when worn down.
  Arenas use twice the palettes, so skies, light beams and wet floors keep
  their gradients. Normals that connect cancel into any special or super, and
  a special that hits cancels into a super. Jumps push off from a crouch and
  land with a short squat. Every fighter has a painted victory of their own,
  fourteen in-between frames that smooth specials, the super and heavy blows
  without changing their timing, and their own voice lines: twenty of them
  call their moves and win in their own language. Round starts and
  victories no longer freeze while the music changes.
- **Maiya**: the tree over each valley's last door is drawn with the door
  instead of appearing late; water, fire and spike tiles and the scenery no
  longer stall and jump; busy fights keep their frame rate (the worst valley
  went from 50 to about 57 frames a second); ledges and pick-ups near Maiya
  no longer vanish for a moment.
- **No Signal Z Alpha v3**: fifty pictures per hero, six-step walk and
  four-step run, hitstop and shudder per blow, fitted hit boxes, one light per
  street, moving street details and today's street crew; in v3.0.1 the
  food and points in crates and barrels lie open, and walking over a
  pick-up takes it, even while guarding or mid-combo.
- **Sky Lance v2.1**: aimed shots in fixed point (no shot hangs on a wing),
  clouds anchored to the field, sprite slots ordered so enemy shots are never
  the first dropped, a difficulty table per sector.
- **All games**: HUDs and screens inside the title-safe area of a CRT
  (columns 3 to 36), rebuilt for this release from source.

## The engine, v1.7.0 to v1.7.7

### Framework 1.0 (v1.7.1): frozen and versioned

- The public C calls are listed in `docs/api/framework_v1.txt`, checked by
  `make api-check` (part of `make test`), and named by `NG_SDK_VERSION` and
  `NG_FRAMEWORK_VERSION` in `ng_defs.h`. From here they change by additions
  only.
- `NG_VRAM_DEFER` writes sprite groups and FIX text in the vertical blank;
  `NG_SOUND_QUEUE` queues sound commands instead of holding the frame.
- The FIX layer writes only changed cells; screen fades blend from byte
  tables over two frames.

### Framework 1.2 (v1.7.2): frames in the blank

- A deadline in the blank (`NG_VRAM_DEADLINE`): no job starts after it,
  moves go first, then content by priority (HIGH, streaming strips, NORMAL,
  FIX text, LOW). `ng_vram_lines_left()` and `ng_vram_window_open()` say
  how much of the blank is left.
- A full list never writes outside the blank.

### Framework 1.3 (v1.7.3): shooters and pinned sprites

- `ng_shooter` (`-DNG_SHOOTER=1`): fractional and reverse vertical scroll,
  smooth camera follow, paced parallax layers, a per-scanline strip budget.
- `ng_char_pin_sprite()` keeps a character's hardware slots fixed.

### Framework 1.4 (v1.7.5): the fighting core

- `ng_fight` (`-DNG_FIGHT=1`): motion inputs (quarter and half circles,
  dragon punch, charge, 360, 720), negative edge, buffered presses, cancels,
  super freeze, projectiles, three-on-three teams, strikers, tag duos,
  deterministic match hashes; seven calibrated CPU ranks; training with
  recording and playback.

### Framework 1.5 (v1.7.7): genre engines

- `ng_puzzle`, `ng_mahjong`, `ng_rpg` and `ng_beatemup`, one file each,
  chosen with `GAME_GENRE_ENGINE` in `game.mk` (`make genre-info` shows what
  a game links). Guide: [docs/GENRE_ENGINES_V1.7.7.md](../GENRE_ENGINES_V1.7.7.md).
- `ng_sprite_group_set_tile_address()`: 20-bit C-ROM tile addresses for
  cartridges past 64K tiles.
- Deferred sprite groups: no starvation of low-priority groups, string
  bounds checks, frame sync between the main loop and the interrupt.

### Added for the release (additions only)

- `ng_fight`: a round's end can be held by the game (`NGFMatch.end_hold`)
  for a bonus count; team bouts end like rounds before the next member comes
  in; shared cancel routes for every fighter; a landing squat; motion
  inputs read through a direction table.
- FIX layer: `ng_fix_blank_rows()` blanks whole rows for a fraction of the
  cell-by-cell cost.
- Sound driver: command `$10` reaches ADPCM-A samples 256 to 511
  (`playSFXWide()`), for games with more voices and effects than 256.
- Sprite groups: `NG_SG_MOVE_FIRST`, so a group whose picture is already in
  place keeps moving with the scenery while its new picture waits; the
  character slot range can be moved (`NG_SPR_CHAR_FIRST`).
- Palette effects: colour mixing without the 68000's library divide, the
  same colours for a fraction of the time (a hit flash cost a tenth of a
  frame).
- The C++14 engine reports the same SDK version as the C engine.

## EagleBIOS

The open system ROM written for the SDK, for MVS and AES; packages ship only
EagleBIOS, never a commercial BIOS.

- v1.7.1: first release, booting every SDK game on MVS and AES.
- v1.7.4: HOW TO PLAY with joystick and button badges, memory cards
  (16-bit JEIDA/PCMCIA, directory and errors), a checksummed backup RAM
  format with migration, the real-time clock, and an eleven-entry menu
  (settings, cabinet and game DIPs, memory cards, diagnostics, sound and
  input tests, clock, cartridge information, backup RAM clear).
- v1.7.7: `SYS_HOWTOPLAY` shows a game's own `HTP1` text block
  (`ng_sys_howtoplay()`).

## Neo Geo CD (experimental)

- Native CD programs (PRG/IPL), ISO and BIN/CUE authoring, format and bank
  checks, a CD diagnostic firmware, and CD ports of Maiya, Sky Lance, Maiya &
  Luna and No Signal (streamed scenery, CD audio). One BIN/CUE per disc for
  SD loaders and burning.
- Status: experimental. All four discs boot in MAME with the stock CDZ
  firmware (your own) and with EagleBIOS-CD; burnt discs, SD loaders and real
  consoles are not yet verified. See `docs/NEOGEO_CD_PIPELINE_AND_TESTING_GUIDE.md`.

## Tools, tests and releases

- **ROM registry** (`tools/rom_registry.py`, `releases/rom-registry.json`):
  every released file with its hashes and build identity; a versioning
  policy where changed bytes always get a new version
  ([VERSIONING_POLICY.md](VERSIONING_POLICY.md)).
- **Release audits**: `tools/audit_release.py` checks packages (EagleBIOS
  only, software lists, banked program ROMs); `tools/make_release.py`
  refuses to replace a version that went out.
- **MAME test folder**: `tools/mame_bats.py` writes every launcher, the menu,
  level launchers (start at any stage) and video recording steps;
  `tools/install_wip_mame.py` installs a build for MVS and AES.
- **Pipeline**: `tools/release_all_games.sh` builds every game, runs the host
  tests and the registry checks, and writes `docs/qa/PIPELINE_RUN.md`.
- **Measurements**: frame and VRAM counters (`make PERF=1`/`PERF=2`) and
  per-game QA scripts in MAME.

## Upgrading from v1.7.0

- A game written for v1.7.0 builds unchanged: the C engine has only grown.
- New games pick their engine pieces in `game.mk`: `GAME_ENGINE_DEFINES`
  (for example `-DNG_VRAM_DEFER=1 -DNG_SOUND_QUEUE=1`) and
  `GAME_GENRE_ENGINE`.
- Toolchain: x-tools v3 (GCC for m68k-unknown-elf), as in v1.7.0.

## Versions in this release

Numbered by the registry's rule (changed bytes, new version); the ROMs were
frozen on 2026-11-02. Build identities are the MVS and AES sets.

| Game | Version | Why this number | Build (MVS / AES) |
|---|---|---|---|
| Maiya: Super Nature Girl | 4.1.0 | the last door, steady scenery, full-speed fights | `207dba42d9b6` / `3358b489e407` |
| Sky Lance | 2.1.1 | rebuilt on the release's engine | `ecde51e48488` / `765a48e4dd9f` |
| Shadow League Fighting | 3.3.0 | round ends, score, finales, HUD, arenas, combos, victories, voices | `d878d5a7eec4` / `4a8c7cf2cc3e` |
| No Signal Z Alpha | 3.0.1 | open crates, walk-over pick-ups | `cf196c6a57a7` / `dcbc015c8d24` |
| Maiya & Luna | 2.0.2 | rebuilt on the release's engine | `1646c3bd7dd8` / `5d908db472ea` |
| Seven Wonders Puzzles | 1.0.2 | rebuilt on the release's engine | `148e53f5450c` / `c01546099d8f` |
| Mahjong Demo | 1.0.1 | v1 as it stands, numbered | `844bcca541e5` / `f79cad2395e6` |

## Known limits

- The games are tested in MAME (MVS and AES, EagleBIOS); reports from real
  MVS, AES, NeoSD and Darksoft hardware are welcome and not yet in.
- Neo Geo CD support is experimental (see above).

## Credits

NeoGeoSDK and its games are by Eagle Software. Shadow League's voices use the
LibriTTS-R speech data (CC BY 4.0), the Kokoro-82M model (Apache 2.0) with
the Koniwa (CC BY 3.0) and SIWIS (CC BY 4.0) voices, and Piper voices
(Russian CC0, Ukrainian Apache 2.0, French CC BY-SA 4.0), all credited in the
game's notes.
