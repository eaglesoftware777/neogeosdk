# Sky Lance hardware regression candidate - 2026-10-14

Base: `76088ef37bfeb9cfe115a979a77cb4453dc8ab35`. The correction was developed and tested independently from the ongoing Shadow League artwork. Existing release files and installed games are preserved.

## Diagnosis

The published v1.7.3 BackBit MVS ZIP was checked against GitHub's release API digest:
`07ba64760d9f9e7bfd99ea0091373f2d92c4907129eea8002518a562c392e104`.
The NeoSD MVS image also matches the published digest:
`c4c251cc226ec339e7d0e879411dd1504377cb43638ab9829068160c7c5a4e9e`.
Its C-ROM byte-lane split reconstructs the same C1/C2 as the BackBit ZIP. There is no observed C-ROM repacking defect in these files.

All six development-branch ROMs match that BackBit release. The current default GitHub `main` folder differs in all six ROMs: C1/C2 are each 8 MiB there, versus 4 MiB each in the release; the older M1 has a ready-01 handshake and lacks the paired NGP2 protocol in the release. Saved API metadata records that comparison. The default branch folder and release are different versions; mixed files are not a valid test set.

The exact release P-ROM was reproduced by building the base source. Its retained ELF was decoded and compared byte for byte against the program ROM before emulator inspection. In MAME 0.283 with Universe BIOS 4.0, the full palette upload remained pending for every one of 1,185 observed hardware frames after 15 seconds in the 35-second baseline run. Terrain/aircraft palettes remained black while FIX text and sound worked. EagleBIOS displayed the same cartridge correctly.

The upload requires `2 + 104/4 = 28` lines under the existing SDK deadline check. The old game frame pump called it after BIOS input handling and the queued sound pump. A longer BIOS path can leave too little time on every frame. The candidate takes completed palette data at the start of the cartridge-owned VBlank interrupt, before BIOS I/O. Runtime telemetry measures 32 lines remaining there. This reproduces and repairs a BIOS-sensitive software defect consistent with the report. Physical BackBit Platinum 1.1.1 / NeoSD Pro confirmation remains pending.

The Imgur album https://imgur.com/a/uHTLp6l currently exposes a single CRT photograph, not the referenced dark-screen video. It demonstrates edge cropping. Normal Neo Geo output remains native 320x224; moving the HUD inward cannot change a TV/scaler's geometry.

## Source changes

- `games/skylance/scenes/sky_video_sync.h`: volatile publish/consume handshake; only complete palette buffers are made available to the interrupt.
- `games/skylance/user.c`: perform the published upload before `SYS_IO` in the game-owned IRQ path.
- `games/skylance/scenes/sky_draw.[ch]`: publish immediately before waiting; reject stale VBlank flags as acknowledgement; early-upload telemetry; foreground cloud keeps its screen X while the camera banks; hostile shots receive HIGH upload priority. Count cockpit instruments in the scanline budget and shed optional blast effects when required.
- `games/skylance/scenes/sky_sortie.c`: score/route text, stocks, lives and hull display move inward; fixed portrait anchor is now (288,44), instruments at x272. Aircraft movement and collision bounds are preserved.
- `games/skylance/tools/qa.py`: updated portrait anchor expectation.
- `games/skylance/tools/hardware_qa.py`, `hardware_capture.lua`: ELF/P-ROM correspondence, matched ROM staging, repeated physical input simulation, live LSPC row occupancy, palette-starvation observation, cloud/camera checks, audio PCM and actual screenshot checks. `--stress` explicitly replenishes resources for load testing; this is not difficulty/balance evidence.
- `games/skylance/tools/package_hardware.py`: non-overwriting candidate packaging, protected release-art/audio checks, lossless NeoSD lane reconstruction, per-file hashes, BackBit MVS only.
- `tools/pack_backbit.py`: distinguish the AES program-header label from physical cartridge compatibility in future README files.
- `tests/skylance_video_sync_test.c`, `test_skylance_video_sync.py`: exercise idle interrupts, single consumption and stale wait flags.

No shared SDK source, Maiya source, art source, audio source, release package or installed game was edited. Generated build files in the isolated worktree changed normally; the generated Intel HEX was archived before its obsolete tracked copy was returned to its original bytes.

## Validation

AES and MVS program builds, hash generation, test-precheck and ROM budget checks pass. Full art/FIX/sound regeneration also completed. All five non-program ROMs remain byte-identical to the release. API check: 315 frozen prototypes present, 57 additions already in the base branch. Sky Lance has no level exporter; level-check reports no export to check.

18 host tests pass: 1 video handoff, 12 Sky Lance assets, 1 shooter-depth fixture, 4 NeoSD packing tests. `git diff --check` passes. The live cartridge regression was verified against both EagleBIOS and Universe BIOS 4.0 on MAME AES/MVS:

| Run | Hardware frames | Gameplay samples | Peak strips/line | Max pending palette frames | Audio RMS | Cloud samples with camera motion |
|---|---:|---:|---:|---:|---:|---:|
| final-aes-eagle | 5327 | 38 | 63 | 19 | 2423.4 | 736 |
| final-aes-unibios | 5327 | 38 | 66 | 19 | 2369.1 | 680 |
| final-mvs-eagle | 5327 | 38 | 69 | 2 | 3018.3 | 702 |
| final-mvs-unibios | 5327 | 38 | 64 | 2 | 2819.5 | 742 |

The AES pending maximum includes startup before the normal game IRQ ownership; it settles and renders correctly. Cloud X remains exactly 96 or 192 for every observed active cloud despite camera movement. All measured peaks remain below 96. Audio is non-silent signed 16-bit PCM. These runs do not establish physical scanout behavior, full-campaign balance, or zero frame misses. The directory labelled baseline-stock in the private exploratory evidence used a substituted BIOS and is excluded from the stock-BIOS claims; genuine stock BIOS was not validated here.

Budget high-water marks: P 218,772 / 393,216; C 3,406,592 / 8,388,608; S 14,720 / 131,072; M 8,778 / 32,768; V 4,546,304 / 8,388,608. Cartridge payload capacities are P512KiB, S128KiB, M128KiB, C1/C2 4MiB each; NeoSD V padding is 8MiB.

## Tester deliverables

`C:\neogeo\SkyLance-Hardware-RC1-20261014` contains the BackBit MVS ZIP, MVS/AES NeoSD images, MVS/AES MAME sets, checksums and the proposed reply. No BIOS is included. The BackBit folder is `skylance_hwrc1` so it can coexist with older tests. `SHA256SUMS.txt` and `MANIFEST.json` identify the complete matched sets.

See `palette-before-after-native.png`, `palette-before-after-2x.png`, `final-hud-2x.png`, `RESULTS.json` and the four run directories for actual cartridge evidence. The comparison uses actual MAME output of the old and corrected program with unchanged graphics. Attract timing differs because the old palettes stall; it is not a claim of newly drawn art.

The English/Spanish reply is also at `../SkyLance-hardware-tester-reply-2026-10-14.txt`. It has not been posted or sent.

Remaining hardware checks: BackBit MVS cold boot and game-to-game return with simultaneous graphics/music/effects; NeoSD Pro MVS display/overscan and crowded combat; firmware/BIOS/board identification. Difficulty tuning remains outstanding; this candidate changes no enemy HP, rate of fire, player damage, lives or mission duration.

The BackBit maker specifies MVS hardware and direct loading of unencrypted MAME-style ROMs: https://store.backbit.io/product/backbit-platinum-mvs/ . The previously distributed AES label described the program header; it does not establish AES hardware support. NeoDecode's published ParseNeoSD method uses the same lossless byte-lane split verified here: https://github.com/evietron/BackBit-Convert/blob/main/NeoGeo/Program.cs .
