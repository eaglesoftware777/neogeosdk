# Neo Geo CD — experimental 1.7.7 development backend

**Status: native image toolchain and startup diagnostics, not a completed CD platform.**
Maiya and Sky Lance have booted into gameplay using supplied stock CDZ firmware
in MAME 0.283, and also using the independent EagleBIOS-CD native loader.
Both paths have gameplay, input, sound-effect and CDDA evidence. Emulator
execution does not establish hardware compatibility.
See the [validation report](roadmap/NEOGEO_CD_VALIDATION_REPORT.md).

## Build an original native disc

From the SDK root on Linux:

```sh
make GAME=helloworld PLATFORM=cd CD_BIOS=stock cd-image
make GAME=helloworld PLATFORM=cd USE_2D_PLUS=1 cd-image
make GAME=skylance PLATFORM=cd CD_BUILD_DIR=../cd-build/skylance cd-image
make GAME=maiya PLATFORM=cd CD_BUILD_DIR=../cd-build/maiya cd-image
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

The writer creates a flat ISO9660 Level 1 directory with unversioned 8.3
identifiers (the `mkisofs -N` profile), native recognition markers and unswapped
`.PRG` files. The marker strings are firmware format requirements, not ownership
metadata for the game. Its separate parser checks the encoded image rather than
a list returned by the writer. See the public
[disc recognition](https://wiki.neogeodev.org/index.php/TXT_file) and
[ISO authoring](https://wiki.neogeodev.org/index.php/Making_an_ISO_file) notes.

During IPL, the program must already delegate CD interrupt vectors to the
firmware, because further asset transfers can interrupt before USER begins.
USER itself is entered with a jump; startup requests return through the firmware
scheduler. Returning from USER with `RTS` consumes an absent return address.

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

`CD_BIOS=eagle cd-test` accepts a source-built EagleBIOS-CD ROM whose identity
advertises native boot, and checks the mapped firmware bytes separately from
stock firmware evidence. Build it using [the firmware guide](../bios/cd/README.md).
Neither test substitutes another BIOS or HLE when the selected firmware fails.

Stock and Eagle packages can be kept in separate directories with distinct
ISO volume labels and separate CUE/CHD files. Their native game payloads remain
identical. The launch profile selects the external system ROM; the game disc
does not contain or install a BIOS.

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

Hello-world uses unchanged game code with CD-specific startup, input and the
common FIX renderer. Maiya and Sky Lance have separate CD adapters and native
SPR/FIX/Z80/PCM payloads; their resident artwork fits CD memory. Their original
music uses CDDA and their effects use a RAM sound driver. These adapters do not
establish a complete generic CD backend. The Eagle direct file service has
loaded and read back a second sprite bank; executable overlays and battery
saving remain unavailable. Generic HAL file/CDDA operations return
`NG_CD_UNSUPPORTED`; constructing a file descriptor is not reported as a load.
The runtime sprite test uses the direct BIOS ABI rather than those HAL stubs.
Physical CD hardware and complete campaign compatibility remain unverified.

Next: broaden stock-firmware and physical-hardware coverage, stabilize the HAL
and broaden LOADFILE/CDDA conformance, standalone UPLOAD, safe scene swaps
and cold-restart saves in order. Cross-model drive behavior remains a separate gate.

Cartridge defaults and original recipe bodies remain unchanged. CD outputs
never enter `roms/`, existing cartridge distributions or installed launchers.
No physical flashing or original-hardware test has been performed.
