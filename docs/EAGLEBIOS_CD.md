# EagleBIOS-CD — independent firmware architecture

The original CD firmware is a development deliverable alongside, not a
replacement for, the existing cartridge EagleBIOS. Current acceptance is F1
diagnostics only. The on-screen menu explicitly reports that disc boot is not
implemented. No BIOS-CD release or physical compatibility claim is made.

`bios/cd/src/cd_vectors.S` owns CD reset, exceptions and IRQ entry. The linker
fixes vectors at `$C00000`, service slots at `$C00400`, identity at `$C00700`
and original code at `$C01000`. Reset establishes its private stack, initializes
only its own data/BSS, holds the sound CPU and leaves optical commands disabled.
No heap is used. Fatal exceptions set a distinct backdrop and service watchdog;
automatic recovery or a detailed exception screen has not been implemented.

`cd_video.c` uploads the SDK's original font to CD FIX DRAM through the odd
byte lane, initializes palettes/maps and renders the original diagnostic menu.
`cd_boot.c` samples controller inputs and maintains a VBlank clock. `build.py`
compiles original source with the 68000 toolchain, asserts section boundaries
and writes exact 512 KiB images. Unused space is filled with `$FF`; no cartridge
firmware bytes are copied into that space.

F1 tests passed on MAME 0.264 front/top/CDZ configurations with exact mapped
ROM comparisons, native screenshots, A/Right inputs, monotonic VBlank count,
ROM PC and private stack bounds. The three images were reproduced identically
on native Windows. This covers menu execution only: the emulator's optical
model shortcomings do not disappear because a menu appears.

Next firmware gate F2 requires an independently implemented, bounded drive
state machine and valid/invalid synthetic media tests. Serial strobe, packet
checksums, CDC IRQ acknowledgement, TOC timing and model mechanics require
conformance probes before a sector reader or loader is advertised. Then add
ISO/IPL loading, memory upload, USER scheduling, runtime services, audio and
transactional virtual-card support in gate order.

No virtual-card bytes are formatted, cleared or written. No physical EPROM,
flash, motherboard or optical hardware operation was performed. An independent
backup/recovery plan and explicit authorization remain necessary for those
operations. See [the actual gate ledger](roadmap/EAGLEBIOS_CD_VALIDATION_REPORT.md).
