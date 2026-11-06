# EagleBIOS-CD — independent firmware architecture

The original CD firmware is a development deliverable alongside, not a
replacement for, the existing cartridge EagleBIOS. Its independent native CDZ
loader has booted Maiya and Sky Lance in MAME 0.283, including gameplay input,
RAM-driver sound effects and CDDA. Runtime asset loading has also been checked.
This remains experimental firmware; no physical compatibility claim is made.

`bios/cd/src/cd_vectors.S` owns CD reset, exceptions and IRQ entry. The linker
fixes vectors at `$C00000`, service slots at `$C00400`, identity at `$C00700`
and original code at `$C01000`. Reset establishes its private stack, initializes
only its own data/BSS and holds the sound CPU until a game driver is installed.
No heap is used. Fatal exceptions set a distinct backdrop and service watchdog;
automatic recovery or a detailed exception screen has not been implemented.

`cd_video.c` uploads the SDK's original font to CD FIX DRAM through the odd
byte lane, initializes palettes/maps and renders the original diagnostic menu.
`cd_boot.c` samples controller inputs, maintains a VBlank clock and schedules
USER requests 0, 1 and 2 through the game-return service. `build.py`
compiles original source with the 68000 toolchain, asserts section boundaries
and writes exact 512 KiB images. Unused space is filled with `$FF`; no cartridge
firmware bytes are copied into that space.

F1 tests passed on MAME 0.264 front/top/CDZ configurations with exact mapped
ROM comparisons, native screenshots, A/Right inputs, monotonic VBlank count,
ROM PC and private stack bounds. The three images were reproduced identically
on native Windows. This covers menu execution only: the emulator's optical
model shortcomings do not disappear because a menu appears.

`cd_drive.c` implements the original bounded serial transport, TOC and sector
reader, LC8953 DMA setup and direct CDDA control. `cd_files.c` validates a flat
ISO9660 directory and the complete IPL before loading its native payloads.
Program memory, four sprite banks, FIX, sound RAM and two PCM banks have separate
address, alignment and capacity checks. FIX loads last to preserve readable
firmware text during the larger transfers. No stock firmware is embedded.

The loader rejects malformed directories/IPLs, overlapping initial resources,
out-of-range banks and invalid USER entry points. No-disc and invalid-disc
errors return to a readable retry screen. The runtime file service preserves
caller registers and SR while allowing the interrupts needed for disc I/O.
A second sprite-bank load was read back exactly in MAME. Missing files and
invalid banks return errors; executable overlays return unsupported.

Next work is broader service conformance, generic HAL integration, tray/lid
and media-change handling, cross-model drive timing, and transactional saving.
Native CDZ emulator results do not establish front/top optical compatibility
or complete-game hardware validation. The dated validation ledger records the
earlier F1 baseline; it is not a claim that these later services are complete.

No virtual-card bytes are formatted, cleared or written. No physical EPROM,
flash, motherboard or optical hardware operation was performed. An independent
backup/recovery plan and explicit authorization remain necessary for those
operations. See [the actual gate ledger](roadmap/EAGLEBIOS_CD_VALIDATION_REPORT.md).
