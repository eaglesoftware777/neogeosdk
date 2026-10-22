# Neo Geo CD — experimental 1.7.7 development backend

**Status: native image toolchain and startup diagnostics, not a completed CD platform.**
The stock-firmware boot gate is blocked by the absence of a user-supplied CD
BIOS. EagleBIOS-CD has independently passed its F1 menu/input gate only.
Neither a valid ISO nor HLE execution establishes stock or hardware compatibility.
See the [validation report](roadmap/NEOGEO_CD_VALIDATION_REPORT.md).

## Build an original native disc

From the SDK root on Linux:

```sh
make GAME=helloworld PLATFORM=cd CD_BIOS=stock cd-image
make GAME=helloworld PLATFORM=cd USE_2D_PLUS=1 cd-image
make cd-unit-tests
```

On Windows CMD, use the same arguments with `make -f MakefileWin32.mak`.
Native Windows and Linux C/C++14 builds were exercised. `USE_2D_PLUS=1`
currently checks the hello-world game/FIX C++14 linkage; it does **not** claim
that the complete C++ scene engine has been ported to CD.

Each build defaults to a fresh `out/neocd/helloworld/<UTC timestamp>/` folder.
Use `CD_BUILD_DIR=<new-or-empty-path>` for an exact destination. Existing builds
are preserved; use a new directory to rebuild. `CD_CC` selects an alternate
68000 GCC. The Python builder also accepts `--wlaz80` and `--wlalink`.
Unported games fail before creating output; cartridge ROMs are never renamed
or passed through the CD pipeline.

Outputs include `program.elf`, `program.map`, five native payload classes,
`source-manifest.json`, `disc/helloworld.iso`, `disc/helloworld.cue`, original
`disc/audio/track02.wav`, memory report, file hashes and independent audit.
`CD_CDDA=0` makes a data-only ISO/CUE. It does not change the Z80/PCM fixture.

```sh
make PLATFORM=cd cd-audit CD_IMAGE=/absolute/build/disc/helloworld.cue
xorriso -indev /absolute/build/disc/helloworld.iso -osirrox on -extract / /new/extract
chdman createcd -i /absolute/build/disc/helloworld.cue -o /new/helloworld.chd
chdman verify -i /new/helloworld.chd
```

The writer creates a flat ISO9660 Level 1 directory with versioned `;1`
identifiers and unswapped `.PRG`. Its separate parser checks the encoded image,
not a list returned by the writer. xorriso extraction and chdman round trips
also passed. Stock-loader acceptance of this directory profile remains unproven.

## Actual firmware tests

Supply your own legitimate CD system ROM. No stock firmware is supplied,
downloaded or embedded. The smoke probe recognizes the three official firmware
identities listed by the MAME 0.264 driver, accepts raw or word-reversed files,
and rejects cartridge-sized, unknown and diagnostic firmware as stock evidence.

```sh
make PLATFORM=cd GAME=helloworld cd-test \
  CD_BUILD_DIR=/absolute/native-build \
  CD_BIOS=stock CD_MODEL=cdz CD_BIOS_FILE=/your/firmware.bin \
  CD_QA_DIR=/new/isolated-stock-probe
```

`CD_MODEL=front|top|cdz` selects a test configuration, not different gameplay.
The probe copies firmware only into its new private test directory, uses the
SDK's original LO data, records native screenshots/audio/input, and compares
actual loaded firmware and program bytes. It neither modifies the MAME install
nor packages firmware in the disc. Model/BIOS hashes and commands go in JSON.

`CD_BIOS=eagle cd-test` explicitly refuses disc-boot testing at F1. Build/test
its independent menu using [the firmware guide](../bios/cd/README.md).

An optional secondary debugger is available:

```sh
python3 tools/neocd/qa_libretro.py --hle-debug \
  --core /your/built/neocd_libretro.so --build /absolute/native-build \
  --out /new/isolated-hle-probe
```

It requires an external NeoCD core with HLE fallback, an empty private system
directory and an explicit HLE warning. Its report always says
`stock_gate_passed=false` and `eagle_gate_passed=false`. It verifies native
hello-world video, moving-sprite input and the original ADPCM-A cue. It is not
a replacement firmware and is not used to pass the stock boot gate.

## Current limitations and next gate

Only `helloworld/main.c` is compiled, unchanged, with CD-specific startup,
input and the common FIX renderer. Runtime disc loading, executable overlays,
CDDA controls and battery saving are unavailable. File/CDDA operations return
`NG_CD_UNSUPPORTED`; constructing a file descriptor is not reported as a load.
The second sprite scene is authored on disc but has **not** been loaded by a
running game. No tutorial, demo or full game port is declared complete.

Next: run the native smoke probe with a recognized user-supplied stock BIOS;
inspect screenshots, native graphics and return/interrupt semantics. Only then
stabilize the HAL and implement/probe LOADFILE/UPLOAD, safe scene swaps, CDDA
and cold-restart saves in order. Firmware F2 drive work is an independent gate.

Cartridge defaults and original recipe bodies remain unchanged. CD outputs
never enter `roms/`, existing cartridge distributions or installed launchers.
No physical flashing or original-hardware test has been performed.
