# v1.7.1 NeoSD Test Images

These are separate prerelease hardware-test assets. They do not replace the
working `Maiya-WIP-NeoSD_AES_v1.neo` and `Maiya-WIP-NeoSD_MVS_v1.neo` images.
Hardware confirmation on NeoSD and NeoSD Pro is still required.

## MVS Sound Only

### Protocol revision (NGP2)

This is an unpublished hardware-test candidate, not a confirmed physical MVS
sound fix. Both P1 and M1 implement the paired protocol; changing only one
region is unsafe. See [MVS Sound Protocol](MVS_SOUND_PROTOCOL.md).

`dist/release/Maiya-WIP-NeoSD_MVS_SOUND_FIX_PROTOCOL_PRERELEASE_v1.neo`

This additional image retains all previous test images. Ordinary ready is `$80`;
only BIOS slot switching replies `$01`. Raw `$01/$02/$03` bypass parameters and
the FIFO. `$02` restarts and starts the existing eyecatcher music directly;
`$03` performs a full reset. Game scene reset is `$08`, initialization `$09`.
Reserved argument values are escaped by the 68000 sender and decoded by NMI.
The public API values and sample/music data are unchanged.

Before interpreting slot-wait or parameter flags, NMI checks a four-byte
work-RAM ownership signature. A newly selected cartridge can inherit a different
driver's RAM layout without executing its own reset vector first. Unowned RAM
therefore takes the clean restart path instead of treating foreign bytes as
protocol state. A prepare-switch request still silences the chip and enters
the RAM wait, invalidating a foreign register cache before doing so.

Only verified sound functions and erased P1 padding, plus M1, differ from the
original MVS v1 image. Header, S1, C and V regions remain byte-identical. V1
remains 8 MiB. No graphics,
animation, sample content, FM/SSG playback or AES assets are rebuilt.

Build with WLA-DX and 68000 binutils installed (the output must not exist):

```sh
python3 tools/build_mvs_sound_prerelease.py \
  --output dist/release/Maiya-WIP-NeoSD_MVS_SOUND_FIX_PROTOCOL_PRERELEASE_v1.neo
```

`tests/mvs_sound_boot.lua` exercises warm startup with an invalidated cache,
bank restoration, readiness ordering, RAM slot wait, BIOS restart and ADPCM
commands under MAME. It also models a ROM handoff from foreign work RAM; optional
`MVS_FOREIGN_RAM` selects a captured 2048-byte RAM snapshot, otherwise deliberately
invalid RAM is used. Set `MVS_BOOT_REPORT` to its output text path and use it as
the MAME autoboot script with a checksum-matched cartridge set. A WAV capture
confirmed nonzero ADPCM-A and ADPCM-B output after restart. Z80Ex tests require
the optional library; a skipped suite is not playback validation.

`tests/mvs_sound_protocol.lua` additionally interrupts incomplete transfers with
each BIOS command and checks all 256 arguments through the real patched P1.
The assembler-backed P1 adapter is release-specific and uses scratch RAM at
`$10EF00`. Normal SDK builds allocate their protocol state through the linker.

Physical MVS/NeoSD testing is still pending. This revision addresses startup
weaknesses; emulator checks alone do not establish the cause of hardware silence
or guarantee flashcart compatibility. Check cold/warm boots and slot changes,
then coin/start, music and effects before treating it as a verified hardware fix.

### Earlier isolated image (historical)

`dist/release/Maiya-WIP-NeoSD_MVS_SOUND_FIX_PRERELEASE.neo`

This earlier image did not resolve the reported physical sound silence. Its
behavior below describes that retained artifact, not the current NGP2 driver.

Based on the exact MVS v1 image. Its header, S1, C, V1 and V2 are unchanged.
P1 changes by exactly one byte: `soundInit()` sends `$09`, rather than using the
BIOS-reserved `$01`. M1 implements the BIOS slot-switch handshake:

- `$01` silences the chip and disables/acknowledges YM timer interrupts.
- The ready reply is written from a RAM-resident routine at `$FF80`.
- The Z80 waits in RAM with interrupts enabled while M1 can be replaced.
- `$03` received during that wait restarts the selected driver.
- Ordinary game `$03` remains the existing soft reset.
- `$02` retains the existing boot-music handler.
- Incoming parameter state is tracked before FIFO dispatch. Parameter `$01`
  must never be mistaken for a BIOS switch request.

The normal playback routines, patches, tracks, samples and timer settings are
not retuned. The experimental C driver accepts game-init `$09`, but does not
implement this slot-switch fix; use the authoritative ASM path for MVS hardware.
Old binaries that send game-init `$01` must be rebuilt with the updated SDK.
Do not mix a newly compiled P1 with an old M1.

The current tools create NGP2 images, not this historical revision. Do not use
them to overwrite its published filename. Use the protocol builder above with
a new output name.

This patcher is intentionally release-specific. It rejects an unexpected P1
instruction signature, incorrect region sizes, and a base without 8 MiB V1.

## AES Boot Compatibility

`dist/release/Maiya-WIP-NeoSD_AES_UNIBIOS_FIX_PRERELEASE.neo`

Based on the exact AES v1 image. M1 and V1/V2 remain byte-identical to that image.
No gameplay C tile moves. A previously unused C bank `$49` holds an original
Maiya boot logo; its tile `$FF` is blank. The AES cartridge header selects the
standard BIOS animation and bank `$49`.

The FIX boot area provides the system animation's text, publisher mark and
copyright glyph. The original game FIX tiles `$000-$2FF`, including colored
HUD graphics and font pixels, are copied unchanged into private tiles
`$D00-$FFF`. Gameplay references use that private range; the BIOS blank tile
`$FF` remains transparent. Existing imported FIX art in other banks is retained.

`ng_fix_set_ascii_base()` selects a 256-tile character page and invalidates the
text cache. Default `$000` preserves other games' layout; Maiya uses `$D00`.
Full-width tile calls are used for relocated HUD symbols, avoiding char truncation.
The font selection persists across scene clears; a clear never changes fonts.

This retained AES artifact uses the older paired protocol. Its release-specific
patcher restores game-init `$01` to match its preserved M1 and rejects the new
NGP2 SDK function layout. Do not rebuild this historical AES image with a new
P1 and its old M1. This pass does not alter the AES image or its graphics.
Normal new builds must use a matching P1 and M1, with game initialization `$09`.

## Padding and Container Layout

The previous 8 MiB V1 padding script and aligned baseline images were committed
in `00c0e46`. Both Python and native C packagers now pad nonempty V sample regions
to the next power of two, with a 64 KiB minimum. Maiya's `$79AB00` sample payload
therefore occupies `$800000` bytes. Padding is `$FF`; already aligned regions
are not extended again. P/S/M use 64 KiB alignment; C uses 256 KiB alignment.

Header lengths describe the actual padded payloads. C1 occupies even C bytes,
C2 odd C bytes. Padding does not change sample addresses or C-ROM tile order.
The Python `--legacy-alignment` option is an explicit diagnostic opt-out, not
the recommended hardware release path. The native CLI and its GUI use the
hardware policy automatically; rebuild the native executable after updating.

## Verification

Every test asset has a JSON manifest containing region sizes, SHA-256 hashes,
and unchanged-region flags. The current MVS patcher restricts P1 changes to
verified sound functions and erased padding; it rejects incompatible M1 revisions.
The AES patcher asserts sample and driver preservation and exact game-font copying.

```sh
python3 -m unittest discover -s tests -p 'test_neosd*.py' -v
python3 -m unittest discover -s tests -p test_maiya_boot_assets.py -v
python3 -m unittest discover -s tests -p test_mvs_sound_handshake.py -v
```

The last suite requires WLA-DX and Z80Ex (`libz80ex1` on Ubuntu). A non-system
library can be selected with `Z80EX_LIB=/path/to/libz80ex.so.1`. It executes the
assembled driver and checks RAM acknowledgement, restart, parameter races,
timer continuity, and YM write parity against the release baseline. It does not
emulate YM audio synthesis or flashcart hardware.

MAME smoke testing additionally checks boot, title/game progression and captured
MVS audio. Hardware acceptance must check cold/warm boots, MVS slot switching,
coin/start, ADPCM-A/B, FM/SSG, and AES UniBIOS splash and menu readability.

## Windows MAME installation

Install the MVS and AES sets `make neo GAME=maiya` builds, with their
software lists and launchers:

```bat
py tools\install_maiya_mame_tests.py C:\mame\neogeosdk --tidy
```

The destination can be any existing MAME SDK installation. Replaced files are
backed up first; `--tidy` also retires older launchers and the test folders of
earlier builds into the backup, so every launcher left runs the new build.
`run_maiya_mvs.bat` and `run_maiya_aes.bat` boot EagleBIOS;
`run_maiya_mvs_unibios.bat` and `run_maiya_aes_unibios.bat` boot UniBIOS 4.0,
which must be placed in `roms\neogeo` and `roms\aes` by hand. Each platform has
separate ROM/hash paths, configuration and NVRAM.
