# Neo Geo CD support audit — 2026-10-21

Development target: SDK 1.7.7. This document records the inspected baseline;
it does not certify CD support, replacement firmware, or physical hardware.

## Source and isolation

Local branch: `fix/maiya-arenas-eagle-bios`.
Local HEAD, tracked upstream, remote branch and PR 9 head were independently
read as `8cb21a9b23252112a8a2c0a1f8a56f30a3638aef`.
Remote verification used `git ls-remote`; no fetch, checkout, reset, stash,
commit, identity change or publication was performed.

The checkout contains newer uncommitted Maiya, Sky Lance, No Signal, puzzle,
rendering and tests work. It is part of the source baseline and must survive.
CD implementation uses separate source directories and output directories.
The installed cartridge test folder and its launchers are outside this task.
Temporary references, builds and baseline hashes are kept outside the checkout
under `/home/kepler/neogeosdk-cd-20261021/` on this development machine; that
path is not an SDK build dependency or a distributable asset.

## Inspected boundaries

| Existing files | Cartridge assumption / reuse decision | CD adaptation and risk | Verification | Confidence |
|---|---|---|---|---|
| `Makefile`, `MakefileWin32.mak`, game configs | Every non-MVS platform currently selects AES; shared `out/`, ROM byte swapping and cartridge firmware dependencies | Dispatch CD builds before cartridge recipes; independent outputs, reject unsupported selections | Target selection and unchanged AES/MVS commands | Source verified |
| `games/*/neogeo*.c`, `sdk/neogeo.ld`, per-game link scripts | Cartridge vectors/header, callbacks and custom code sections; main RAM begins at `$100000` | Native unswapped PRG and CD startup/linker; reserve BIOS buffers and work RAM | ELF/map/entry audits, actual stock firmware boot | Source verified; CD map pending |
| `games/helloworld/user.c` and `main.c` | USER request dispatcher, credit callbacks, stack at BIOS work RAM; game logic calls common FIX/audio/input APIs | Separate CD startup; preserve game logic where possible; do not copy cartridge boot into CD | Boot, moving object/input proof, error return | Source verified |
| `sdk/neogeolib.c`, `sdk/macro.h`, `sdk/cabinet/ng_sys.c` | Absolute cartridge BIOS addresses, MVS flag, ROM sound events; ADPCM-B callers | Narrow CD service wrappers and capability query; no coin/AES inference for CD | Register-preservation and capability tests | Source verified; CD ABI pending |
| `sdk/2d_engine/`, `sdk/2d_engine_plus/` | Shared simulation can remain; art indexes assume resident ROM contents | Scene residency/generation checks; retain palette/VRAM deadlines and C/C++14 ABI | Existing host tests, bank reload tests, write taps | Source verified |
| `artbox/`, generated C/S graphics | Interleaved cartridge sprite lanes and FIX tile encoding | Native SPR/FIX conversion with independent decoded-pixel round trips | Boundary fixtures and actual CD DRAM captures | Conversion research pending |
| `sound/`, per-game M/V assets | Z80 driver and immutable sample ROM; cartridge ADPCM-B music/voices | Finite Z80/PCM DRAM, explicit unsupported features and original CDDA tracks | File-layout tests, audible capture and concurrent SFX/CDDA | Source verified; CD audio unverified |
| `bios/src/bios_entry.s`, `bios/eagle_bios.ld`, `bios/Makefile` | 128 KiB system ROM; cartridge service table; private RAM `$10F400..$10FBFF` | New independent `bios/cd/`; keep existing ROMs/headers/builds unchanged | Existing cartridge BIOS contract probes; separate 512 KiB CD audits | Source verified |
| `bios/src/bios_card.c`, `bios/src/bios_backup.c` | Cartridge card and MVS backup interfaces | Separate CD virtual-card implementation; no real saves formatted or erased | Corruption, interrupted writes, cold restart | Source verified; CD storage unverified |
| Existing MAME and game QA | Cartridge machines, software lists, isolated BIOS paths | Disc-based launch, model/firmware identification, actual program and asset evidence | Independent parser plus emulation, separate hardware matrix | Source verified |

## Tools and dependencies

The installed Linux MAME identifies itself as **0.264** and lists `neocd`,
`neocdz` and `neocdzj`. These names alone do not establish model functionality.
The existing cartridge checks also use Windows MAME 0.283; its CD drivers
must be inventoried separately. Headless probes must explicitly disable the
inherited X11/PulseAudio connection to avoid a host initialization hang.
The existing GCC 16 m68k-unknown-elf toolchain is available outside the repo.
Host C compiler and FFmpeg are installed. ISO authoring/CHD tools were not
found on PATH during the initial check. No supplied stock CD BIOS has yet
been verified. No physical CD hardware is attached or certified.

## Hardware/reference ledger

Public research sources to cross-check against executable behavior:

- [Native PRG and overlapping work RAM](https://wiki.neogeodev.org/index.php/PRG_file): PRG is not cartridge-byte-swapped. The physical 2 MiB is not all free executable storage.
- [Loader descriptors](https://wiki.neogeodev.org/index.php/Loading_files): filename/bank/aligned destination list; enabled interrupts; different FIX/PCM address units.
- [Initial disc classes](https://wiki.neogeodev.org/index.php/Game_CD_structure): PRG/FIX/SPR/Z80/PCM and conservative directory limits. The page explicitly expresses uncertainty; requirements need boot tests.
- [NeoCD emulator](https://github.com/libretro/neocd_libretro): independent hardware/format cross-check and optional second emulator. Its built-in high-level BIOS is neither stock firmware nor EagleBIOS-CD evidence.
- [MAME hardware implementation](https://github.com/mamedev/mame): independent memory/register/interrupt and model inventory cross-check.

Planning limits, pending per-model validation: program DRAM 2 MiB physical,
sprite DRAM 4 MiB in four 1 MiB upload banks, FIX 128 KiB, PCM 1 MiB in two
512 KiB banks, Z80 RAM 64 KiB. Save capacity and accessible format must be
distinguished. CDDA and ADPCM-A/FM/SSG require their own tests; cartridge-style
ADPCM-B support must not be assumed.

## Gates and current status

| Gate | Initial status | Evidence required |
|---|---|---|
| Phase 0 / F0 source inventory | In progress | Cartridge baseline tests and exact hardware/ABI map |
| Native stock-BIOS disc | Not implemented | Independent ISO/IPL audit plus real disc boot and inputs |
| EagleBIOS-CD F1 | Not implemented | Original 512 KiB ROM, menu/input capture, vectors and SHA |
| EagleBIOS-CD F2–F6 | Not implemented | TOC/data, IPL boot, reload/audio/save, recovery, model tests |
| Front/top/CDZ physical tests | Not tested | Individually authorized boards, media and recorded results |

No new game campaigns, art redesigns, installed cartridge updates or physical
firmware operations are authorized by this CD implementation assignment.
Source-original firmware and source-built disc results must be reported
independently; neither substitutes for the other.

## Implementation update — same source baseline

The table above records the initial inventory, not the final result. The
additive implementation now has native hello-world CD startup, lossless native
graphics conversion, manifest/IPL/ISO/CUE authoring, independent auditing,
bounded memory policy and original standalone diagnostic firmware. See
[the measured report](NEOGEO_CD_VALIDATION_REPORT.md) and
[firmware report](EAGLEBIOS_CD_VALIDATION_REPORT.md).

xorriso 1.5.6 and MAME tools/chdman 0.264 were subsequently installed; independent
extract/compress/verify/extract comparisons passed. Native Windows Python 3.11,
GCC, WLA-DX and GNU Make were exercised as well as Linux tools. All four
hello-world compiler configurations produce identical native payloads/ISO;
all three firmware images reproduce identically between Windows and Linux.

Original cartridge host tests and AES/MVS BIOS contract probes passed in the
private baseline. Seventeen protected files match their baseline hashes; the
two Makefile bodies match after removing only the additive CD selector. AES/MVS
dry-run recipe output also matches. Existing game sources and installed games
were not modified by the CD work.

F1 firmware diagnostics passed in three MAME model configurations. The native
hello-world has video/input/ADPCM-A evidence only through an explicitly labeled
external NeoCD **HLE** diagnostic. The required stock BIOS boot gate is still
blocked: no recognized user-supplied CD BIOS was available. No subsequent runtime
or firmware gate is promoted to complete on the strength of that HLE result.
