# NeoGeoSDK v1.7.7 - The Games Release

NeoGeoSDK v1.7.7 ships seven games built on the SDK, four genre engines, the
fighting core, EagleBIOS and an experimental Neo Geo CD backend. The `v1.7.7`
tag identifies the release commit; GitHub source downloads follow that tag.

- [Release page](https://github.com/eaglesoftware777/neogeosdk/releases/tag/v1.7.7)
- [Announcement: everything since v1.7.0](releases/ANNOUNCEMENT_v1.7.7.md)
- [Complete release notes](../CHANGELOG.md) and each game's `games/<name>/CHANGELOG.md`
- [Versioning policy](releases/VERSIONING_POLICY.md) and
  [release manifests](releases/RELEASE_MANIFESTS.md)
- [Genre engines](GENRE_ENGINES_V1.7.7.md), [fighting core](FIGHTING_V1_7_7.md),
  [Neo Geo CD guide](NEOGEO_CD_PIPELINE_AND_TESTING_GUIDE.md)

## Downloads

Every game is released in the same set of packages. `<Game>` is the package
name (`Maiya`, `SkyLance`, `ShadowLeague`, `NoSignal`, `MaiyaLuna`,
`SevenWonders`, `MahjongDemo`) and `<version>` the game's own version
(`MAJOR.MINOR.PATCH`, see the versioning policy):

| Asset | Contents |
|---|---|
| `<Game>-WIP-EagleBIOS_v<version>.zip` | MAME: the MVS set, its software list and EagleBIOS |
| `<Game>-WIP-AES-EagleBIOS_v<version>.zip` | MAME: the AES set, its software list, EagleBIOS and a blank memory card |
| `<Game>-WIP-NeoSD_MVS_v<version>.neo`, `<Game>-WIP-NeoSD_AES_v<version>.neo` | NeoSD images (cartridge data only; the system's own BIOS is used) |
| `<Game>-Darksoft-MVS_v<version>.zip`, `<Game>-Darksoft-AES_v<version>.zip` | Darksoft Neo Geo Multi folders |
| `<Game>-BackBit-MVS_v<version>.zip`, `<Game>-BackBit-AES_v<version>.zip` | BackBit Platinum folders |
| `<Game>-README_v<version>.txt`, `<Game>-CHANGELOG_v<version>.txt` | How to play, controls, what changed |
| `<Game>-MANIFEST_v<version>.txt` | Size, CRC-32 and SHA-256 of every file of that version |
| `neogeosdk_v1.7.7_manual.pdf` | Programmer's manual and library reference |
| `neogeosdk_v1.7.7_overview.pdf` | Technical overview of the release |

The packages contain EagleBIOS, the SDK's own open system ROM, and no other
BIOS. MAME warns that EagleBIOS's checksums differ from the original BIOS:
that is expected. Versions already released keep their files on the release
pages where they went out; a version name always means the same bytes
(`releases/rom-registry.json` records every one).

## Play a game in MAME

1. Download the game's `-WIP-EagleBIOS` (MVS) or `-WIP-AES-EagleBIOS` zip and
   its MANIFEST; check the SHA-256 against the MANIFEST.
2. Extract into a new folder, keeping its `roms/` and `hash/` folders.
3. From that folder:

```sh
mame neogeo -rompath roms -hashpath hash -bios euro -cart1 <set>
mame aes -rompath roms -hashpath hash -bios asia -cart1 <set> -memc card.bin
```

`<set>` is the software list name: `maiya`, `skylance`, `shadowleague`,
`nosignal`, `maiyaluna`, `sevenwonders` or `mahjong_demo`. Each zip's README
gives its exact command.

Linux/WSL checksum check:

```sh
sha256sum <file>     # compare with the line in <Game>-MANIFEST_v<version>.txt
```

Windows PowerShell:

```powershell
Get-FileHash .\<file> -Algorithm SHA256
```

## Real hardware

- NeoSD: copy the `.neo` image to the card.
- Darksoft Neo Geo Multi and BackBit Platinum: copy the folder inside the zip
  to the card.
- The games are tested in MAME (MVS and AES, EagleBIOS). Reports from real
  MVS, AES, NeoSD, Darksoft and BackBit hardware are welcome: board or
  console, BIOS, cartridge firmware, what you saw and heard.

## Building from source

Use the [README setup instructions](../README.md) and the
[dependency checklist](DEPENDENCIES.md). `SDKHOME` points to the parent
folder holding the checkout and the toolchain.

```sh
make neo GAME=shadowleague        # AES then MVS sets and NeoSD images in dist/neo/shadowleague
tools/release_all_games.sh        # every game, host tests, ROM registry checks, QA summary
```

Windows CMD: `make -f MakefileWin32.mak GAME=<game> all`.

`make all` rebuilds a game's graphics, FIX, sound and program from source.
Release packages are made with `tools/make_release.py` (an explicit version)
and recorded by `tools/rom_registry.py`; `tools/audit_release.py` checks
them.

## Upgrading a game from v1.7.0

The C engine's public calls have only grown since Framework 1.0: a v1.7.0
game builds unchanged. New features are opt-in through `GAME_ENGINE_DEFINES`
and `GAME_GENRE_ENGINE` in the game's `game.mk`; see the announcement for the
list.

Eagle Software

https://eaglesoftware.biz
