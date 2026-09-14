# NeoGeoSDK v1.7.0 - The 2D Engine Release

Release publication: **September 14, 2026**.

NeoGeoSDK v1.7.0 brings together the C and C++ 2D engines, the 25-chapter
showcase, project-aware Artbox and Sound studios, and the expanded multi-game
build environment. The `v1.7.0` tag identifies the release publication commit
on `neo_universal_2d`. GitHub source downloads follow that tag.

- [Release page](https://github.com/eaglesoftware777/neogeosdk/releases/tag/v1.7.0)
- [Complete release notes](../CHANGELOG.md)
- [Demo chapter reference](DEMO_CHAPTERS.md)
- [Desktop studios manual](DESKTOP_STUDIOS.md)
- [Game projects and shared assets](GAMES.md)

## Downloads

| Asset | Contents |
|---|---|
| [neogeosdk_v1.7.0_demo_777.zip](https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.0/neogeosdk_v1.7.0_demo_777.zip) | Six demo ROMs, matching MAME software list, installation notes, and per-file checksums |
| [neogeosdk_v1.7.0_demo_777.sha256](https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.0/neogeosdk_v1.7.0_demo_777.sha256) | SHA-256 of the demo ZIP |
| [neogeosdk_v1.7.0_manual.pdf](https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.0/neogeosdk_v1.7.0_manual.pdf) | Printable programming manual |
| [neogeosdk_v1.7.0_overview.pdf](https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.0/neogeosdk_v1.7.0_overview.pdf) | Illustrated release overview |
| [x-tools-v3.tar.xz](https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.0/x-tools-v3.tar.xz) | Linux 68000 cross-toolchain |
| [x-tools-v3-win.zip](https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.0/x-tools-v3-win.zip) | Windows 68000 cross-toolchain |
| [SHA256SUMS.txt](https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.0/SHA256SUMS.txt) | Existing toolchain checksums, not the demo ZIP checksum |

The toolchains and previously attached PDFs are unchanged by this publication
update. GitHub's automatic source ZIP and tar.gz downloads are separate from
the prebuilt demo package.

## Run the Prebuilt Demo

No compiler or Python installation is required to use the ROM package. Install
MAME separately and supply your own compatible Neo Geo BIOS. The package does
not include an emulator, BIOS, or commercial game ROM set.

1. Download the demo ZIP and its `.sha256` file into the same directory.
2. Verify the archive checksum before extraction.
3. Extract into a new folder and keep the directory structure below.
4. Place your Neo Geo BIOS archive at `roms/neogeo.zip` in that folder.
5. Open a terminal in the extracted folder and launch MAME as shown below.

```text
README.txt
BUILD.txt
LICENSE
SHA256SUMS.txt
docs/RELEASE_v1.7.0.md
hash_eagle/demo/neogeo.xml
roms/demo/777-p1.p1
roms/demo/777-m1.m1
roms/demo/777-s1.s1
roms/demo/777-v1.v1
roms/demo/777-c1.c1
roms/demo/777-c2.c2
```

`BUILD.txt` records the exact source commit and tag. The internal
`SHA256SUMS.txt` covers the package payload, including the software list. It is
different from the toolchain checksum file attached separately to the release.

Linux/WSL, before extraction:

```sh
sha256sum -c neogeosdk_v1.7.0_demo_777.sha256
unzip neogeosdk_v1.7.0_demo_777.zip -d neogeosdk-demo-777
cd neogeosdk-demo-777
sha256sum -c SHA256SUMS.txt
```

Windows PowerShell, before extraction:

```powershell
Get-FileHash .\neogeosdk_v1.7.0_demo_777.zip -Algorithm SHA256
Get-Content .\neogeosdk_v1.7.0_demo_777.sha256
Expand-Archive .\neogeosdk_v1.7.0_demo_777.zip -DestinationPath .\neogeosdk-demo-777
```

Compare the two checksum values. After adding the BIOS, run from the extracted
folder on Linux/WSL:

```sh
mame neogeo -rompath roms -hashpath hash_eagle/demo -bios euro -cart1 demo -window -nofilter -waitvsync
```

Or from Windows CMD:

```bat
mame.exe neogeo -rompath roms -hashpath hash_eagle\demo -bios euro -cart1 demo -window -nofilter -waitvsync
```

MAME must be on `PATH`; otherwise use the quoted full path to its executable.
The custom `neogeo.xml` software list is required for the `demo` cartridge name.
A missing or incompatible BIOS is not supplied by the SDK; review MAME's
reported missing files. If your BIOS set differs, select an available BIOS
instead of `euro`.

Use MAME's input menu to check your coin/start and Neo Geo button mappings.
In the demo, **A advances** and **C restarts** the chapter except where C is
reserved for gameplay. Sky Lance uses **B to fire** and **D for a super missile**.

## Building From Source

Use the [README setup instructions](../README.md) and
[dependency checklist](DEPENDENCIES.md). Neither the toolchain location nor
the checkout must be on a particular Windows drive. `SDKHOME` points to the
parent containing the checkout and toolchain bundles unless explicitly overridden.

Linux/WSL:

```sh
make GAME=demo GAME_CFG_FILE=games/demo/game.cfg all
make GAME=demo GAME_CFG_FILE=games/demo/game.cfg test
```

Windows CMD:

```bat
make -f MakefileWin32.mak GAME=demo GAME_CFG_FILE=games/demo/game.cfg all
make -f MakefileWin32.mak GAME=demo GAME_CFG_FILE=games/demo/game.cfg test
```

`all` rebuilds graphics, FIX, sound, and P1. The downloadable ZIP is instead
packaged from the committed `roms/demo/777-*` bytes without regenerating them.
Packaging checks cover file sizes, CRC-32 and SHA-1 against the software list,
SHA-256 payload checksums, and ZIP integrity. This publication is not a new
MAME playthrough or hardware-validation claim; earlier dated test results in
the changelog describe their own captures.

## PDF Addendum

The attached PDFs are the September 13 documentation snapshots. Keep their
programming/reference structure, but use the current Markdown for these later
publication details:

- The active demo has **25 chapters**, ending with Sky Lance and credits.
- `demo_plus` shares the demo's scenes, art, and sound through `GAME_SCENES_FROM`,
  `GAME_ART_FROM`, and `GAME_SOUND_FROM`. Its old three-scene smoke test is optional.
- Sky Lance's standalone project has seven stages; the demo chapter is a
  two-stage slice. They are not the same ROM or campaign.
- The desktop studios now include project navigation and expanded asset/audio
  workbenches. Use the corrected paths, dependencies, hardware data layouts,
  and preview limitations in `DESKTOP_STUDIOS.md`.
- The prebuilt demo and checksum filenames are the ones listed above.

Eagle Software

https://eaglesoftware.biz
