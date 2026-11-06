# Versioning Policy

Every game ROM that leaves the workshop gets a version that names exactly one
set of bytes, forever. This page is the rule; `releases/rom-registry.json` is
the record, and `docs/qa/ROM_INTEGRITY_REPORT.md` is the current state.

## Why

Until v1.7.7 a release name such as `SkyLance-WIP-EagleBIOS_v2.zip` was
rebuilt in place whenever a fix landed (`make_release.py --replace`). The
registry found 84 release files that carried more than one binary under the
same name, and the emulator databases caught different ones: HBMAME lists
Maiya v4 build `f5f3959b9de2` while FinalBurn Neo lists v4 build
`7dddff770857`, and neither is the v4 file in `dist/release` today. A player
with "v4" cannot tell which game they have.

## Version numbers

Each game has its own `MAJOR.MINOR.PATCH`, independent of the SDK version:

| Part | Raise it for | Examples |
|---|---|---|
| MAJOR | new content: stages, characters, modes, a redesign | Maiya v5.0.0 |
| MINOR | gameplay, graphics, animation or feature improvements | Sky Lance v2.1.0 for the projectile, cloud, flicker and difficulty work |
| PATCH | bug fixes and compatibility corrections, no new content | Sky Lance v2.0.1 for the safe-area rebuild |

The existing labels read as `vN` = `N.0.0` (`v4` is 4.0.0). Test builds keep a
suffix: `v3_TEST` is 3.0.0-test and never ships as a release.

File names carry the full version from now on:
`SkyLance-WIP-EagleBIOS_v2.0.1.zip`. The ROM files inside keep their names
(`779-p1.p1` and the software list name `skylance`), because MAME, HBMAME and
FinalBurn Neo look them up by name, size and hash; only the bytes and the
version change.

## Build identity

`build_id` is the first 12 hex digits of the SHA-256 over the SHA-256 of each
ROM part (`p1 s1 m1 v1 c1 c2`), computed by `tools/rom_registry.py`. Two
packages with the same `build_id` hold the same game, whatever their names;
the MAME set, the BackBit zip and the Darksoft zip of one build share it.
`git_commit` and `sdk_version` in the registry record where and on which SDK
release the build was made.

## The rules

1. **A version that went out is frozen.** "Out" means any of its files is a
   GitHub release asset, sits in the public repository (pushed to any remote
   branch), or matches an emulator database entry. `make_release.py
   --replace` refuses to change a frozen version's bytes; rebuilding with the
   same bytes is still allowed.
2. **Changed bytes, new version.** Any rebuild whose ROM parts differ gets at
   least a new PATCH number, even when the change is "only" a recompile with
   a newer toolchain.
3. **Old binaries stay available.** Released files are never deleted from
   `dist/release` or from GitHub; historical builds remain recorded in the
   registry with their hashes even where the file itself was purged (the
   Maiya v1 to v4 packages that carried SNK fix tiles are local files only).
4. **No release without a registry entry.** Run `tools/rom_registry.py` after
   `make_release.py`; `tools/rom_registry.py --verify` fails when a file in
   `dist/release` is not registered or no longer matches its entry.
5. **Never chase a database hash.** A ROM is never altered to match an old
   CRC in HBMAME or FinalBurn Neo. The new version goes out with its own
   hashes and `releases/db-submissions/` holds the updated entry for the
   maintainers (see `EXTERNAL_DATABASE_COMPATIBILITY.md`).

## Versions to assign now

Section 8 of `docs/qa/ROM_INTEGRITY_REPORT.md` is generated from the registry
and lists, per game, whether the current build can keep its name. On
2026-10-28 it gives:

| Game | Current file | Release the current build as |
|---|---|---|
| Maiya: Super Nature Girl | v4 (build `758f28909141`, never out under v4) | v4.0.1 |
| Maiya & Luna | v2 (HBMAME lists a different v2) | v2.0.1 |
| No Signal Z Alpha | v3 (went out as 2 builds) | v3.0.1 |
| Shadow League Fighting | v3 (went out as 4 builds) | v3.0.1 |
| Seven Wonders Puzzles | v1 (went out as 3 builds) | v1.0.1 |
| Mahjong Demo | v1 (went out as 3 builds) | v1.0.1 |
| Sky Lance | v2 (went out as 3 builds) | v2.0.1 |

Older names that are just as ambiguous (Maiya & Luna v1, No Signal v1,
Shadow League v2) belong to superseded versions and need no re-release: the
registry keeps each of their builds apart by `build_id`. This table is a
snapshot; the report is regenerated on every run and wins when they differ.
Builds that add gameplay or art (the v1.7.7 quality pass) take the next MINOR
on top of these: Sky Lance v2.1.0, No Signal v3.1.0, Shadow League v3.1.0,
Maiya & Luna v2.1.0.

## Release checklist

1. `make neo GAME=<game>` (MVS and AES sets, NeoSD images).
2. `python3 tools/make_release.py --game <game> --game-id <id> --prefix <Prefix> --name "<Title>" --version vX.Y.Z`
3. `python3 tools/audit_release.py --prefix <Prefix> --game <game> --game-id <id> --version vX.Y.Z`
4. `python3 tools/rom_registry.py` and check sections 2, 7 and 8 of the report.
5. Attach the files to a GitHub release only after the owner approves; never
   replace an asset that is already attached.
