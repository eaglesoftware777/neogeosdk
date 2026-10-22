# Sky Lance

Current source: **V3 CANDIDATE**. Local rendering/performance work is in progress;
released packages remain separate. The latest stage0 MVS stress measurement is
48.7 updates/s versus41.8 at baseline. Full-rate gameplay and physical NeoSD,
BackBit and fullscreen acceptance remain pending. See CHANGELOG.md for scope.

A native Neo Geo vertical shooter with three pilots, seven long routes and
one main boss per mission. First public set: `_v1`, with the NeoGeoSDK
v1.7.3 pre-release (see [CHANGELOG.md](CHANGELOG.md)).

The [2026-10-14 hardware candidate](https://github.com/eaglesoftware777/neogeosdk/releases/tag/skylance-hardware-rc1-20261014)
corrects BIOS-dependent black graphics while retaining the v1 art and audio.
Use its complete matched set for BackBit MVS or NeoSD testing. Real cartridge
confirmation is still pending. [Diagnosis and emulator results](assets/review/hardware-20261014/REPORT.md).

Rook flies a silver-blue F-16, Kira an olive F-35 and Blaze a crimson F-15.
Aircraft proportions use the official type dimensions as an arcade reference.
Each has different acceleration, top speed, primary fire and bomb damage.
Held lateral input reaches a 45-degree bank; releasing the stick levels the
whole airframe. B with a direction performs a barrel roll, climb or Split-S.
C spends a counted super bomb; B+D spends a counted chaff release.
See [CONTROLS.txt](CONTROLS.txt) for the exact controls.

Fuel is a 240-second mission countdown, including time spent in target battles.
It never regenerates while coasting or after a lost life. F pickups add 30
seconds, capped at 240. Running out of fuel fails the mission.

At route distances 512, 1024 and 1536, the scenery stops at an empty military
pad. Defeat the escorts and a dedicated mini boss before damaging the command
target. Destroying the exposed target resumes scrolling. Destroy all three
for the 2500-point objective bonus; submarines do not count as command targets.
Entering the visible radar circle calls six extra defenders and doubles the
site's missile cadence. Avoiding coverage preserves the quieter encounter.
Chaff briefly spoofs coverage and can divert guided missiles to a visible decoy:
each release rolls every missile in the air, and the chance falls with the
route and the operator's DIFFICULTY setting (EASY to EXPERT, NORMAL by default;
a console plays NORMAL). Bosses' missiles see through most of it except on EASY.

Every boss is met on its route's last page, an open arena themed to it: deep
sea for the warship, empty desert for the tank fortress, a forest clearing, a
lava plain. Once the waves are spent the flight cruises there, and the boss
appears when the arena fills the screen.

The routes are painted panels (`assets/painted/sources`) that
`tools/paint_terrain.py` scales to the route's 352-pixel width, cross-fades,
fits to sixteen palette banks per route (one per 16x16 tile, refined and
despeckled) and lays out in streaming order; it paints the valley road, the
target pads and the boss arena, and checks the sea lanes are open water. Roads,
water lanes and encounter pads own their associated object positions. Villages, farms, docks, forests, rail
freight, industry, runways and hangars vary along eight streamed pages per route.
Clouds use a separate rear layer. The hardware output is 320x224; enlarged
review sheets use nearest-neighbour pixels.

The flight field is 288 pixels wide. A 32-pixel cockpit contains a stable pilot
portrait, a moving attitude indicator and working G/engine gauges. F-35 uses a
panoramic panel; F-16/F-15 use separate instrument displays. These are compact
arcade interpretations of their layouts. Briefing text wraps inside its assigned
rectangle. Ordinary aircraft/vehicles use compact 32-pixel canvases; mini and
main bosses have larger dedicated art. Hostile weapons have eight distinct
projectile designs. C strikes vary by pilot and grow in damage by mission.

## Build and inspect

```sh
python3 games/skylance/tools/art.py
python3 games/skylance/tools/contact_sheet.py
python3 games/skylance/tools/test_production.py
python3 -m unittest discover -s tests -p 'test_skylance*.py'
python3 tools/build_skylance_mame.py /mnt/c/mame/neogeosdk --skip-art
python3 games/skylance/tools/qa.py --platform mvs --bios stock --stage 0 --encounters --seconds 260
python3 games/skylance/tools/qa.py --platform aes --bios eagle --stage 1 --encounters --radar-test --seconds 300
```

Native previews, metadata and diagnostic sheets are in `artbox/generated`.
Accepted source masters remain in `artbox/redrawn`; the new terrain and compact
consoles come from the deterministic pixel authoring script. Original art is
retained. All asset IDs stay stable; new assets are appended.

C1/C2 are 4 MiB each. Boot tiles 0x4900..0x49FF, the transparent utility tile,
FIX transparency, cartridge backup RAM and the 104-bank atomic fade budget are
reserved. Higher effect banks use the queued palette path. Exact boss health is
owned by Sky Lance as 16-bit data; the shared character ABI remains unchanged.
Deferred groups preserve pending membership during rebinds and aged uploads;
queue entries publish their pointer before increasing the visible count.

## Installation and packages

`tools/build_skylance_mame.py <mame-folder>` builds, verifies and installs
the MVS and AES test sets. Run `run_skylance_mvs.bat` or `run_skylance_aes.bat`
in that folder.
Normal launchers use fullscreen display settings; scanline variants select
MAME's BGFX crt-geom shader. Each platform has isolated ROMs, hashes and saves.
Existing Sky Lance files are backed up; other game installations are preserved.

```sh
python3 tools/build_skylance_candidate.py --output <candidate-folder>
make neo GAME=skylance
python3 tools/make_release.py --game skylance --game-id 779 --prefix SkyLance \
    --name "Sky Lance" --version v1 --changelog games/skylance/CHANGELOG.md
```

Candidates contain separate AES/MVS NeoSD, Darksoft, BackBit and MAME packages.
ROM hashes and format round trips are checked. NeoSD/Darksoft sample regions
receive FF padding to 8 MiB; BackBit retains the original six ROM payloads.
Hardware packages contain no BIOS and never overwrite an earlier candidate.
The standalone Windows setup wizard includes EagleBIOS for emulator use,
normal/scanline launchers, previews, controls and the cartridge packages. It
needs Windows PowerShell and an installed MAME; Python is not required.

The v1.7.3 pre-release carries `SkyLance_Setup.zip` (`tools/make_setup_zips.py`,
sources in `tools/installer/skylance`): `install_skylance.bat` (Windows
PowerShell) or `install_skylance.sh` (Linux/macOS) downloads the v1 MVS and
AES MAME sets from the release (or takes them from beside the script), checks
them against `SkyLance-MANIFEST_v1.txt` and installs them side by side with
EagleBIOS and the launchers (`tests/skylance/{mvs,aes}`, the layout above);
replaced files go to `backups/skylance-setup-<time>`, settings and saves stay.
`tools/audit_release.py --prefix SkyLance --game skylance --game-id 779
--version v1` checks the release files for every hardware fix.

MAME captures, audio and state logs are saved under
`C:/mame/neogeosdk/tests/skylance/qa/<build-id>`. The harness seeds resources and
invulnerability only in explicit inspection modes. Physical AES/MVS boot,
cartridge audio, slot switching and return shortcuts still need real-board
verification. See [PRODUCTION_STATUS.md](PRODUCTION_STATUS.md) for recorded
build evidence and remaining production limits.
