# Neo Geo CD memory contracts — development status

Public capacities below are hardware/format limits, **not available runtime
services**. Runtime residency/eviction and executable overlays are not released.
Sources: [PRG](https://wiki.neogeodev.org/index.php/PRG_file),
[loading files](https://wiki.neogeodev.org/index.php/Loading_files),
[native sprite format](https://wiki.neogeodev.org/index.php/Sprite_graphics_format).

| Area | Physical capacity | Conservative authoring policy | Initial hello payload |
|---|---:|---|---:|
| Program | 2 MiB | PRG destinations below 1 MiB; resident linker reserves 512 KiB | 3,070 bytes |
| Sprite | 4 MiB | 4 banks × 1 MiB; 128-byte complete tiles | 256 bytes, bank 0 |
| FIX | 128 KiB | one bank; 32-byte complete tiles | 131,072 bytes |
| PCM | 1 MiB | 2 banks × 512 KiB; 256-byte aligned | 4,096 bytes |
| Z80 | 64 KiB | one bank; even byte count | 65,536 bytes |
| CD backup | 8 KiB physical | no format or write API implemented | untouched |

The exact current ELF is authoritative; the validation report records sizes.
Do not treat the entire physical 2 MiB program RAM as free code storage: work
RAM, firmware buffers, vectors and stacks overlap that address space.

`sdk/platform/cd/neocd.ld` places the header at zero, code at `$200`, initialized
data at `$100800`, and work BSS below `$10F000`. Data's load image remains in
resident PRG. The game stack starts at `$10F000`; BIOS work above it is not
cleared. Header `$13A` disables implicit Z80 CDDA polling. Interrupts chain to
firmware completion. All destination arithmetic is checked without unsigned
wraparound. Actual stock-firmware overlap safety remains a required boot probe.

All manifest/API offsets are **physical data bytes**. Only the BIOS descriptor
boundary doubles FIX/Z80/PCM offsets into the byte-lane bus units. Sprite banks
and PCM banks are checked separately; a transfer may end exactly at a bank
boundary but cannot cross it. Initial IPL has at most 32 unique entries; the
disc has at most 512 root files including IPL.

The manifest declares pinned boot resources and scene dependencies. Dependency
cycles, missing dependencies, size/hash mismatches and simultaneous resident
overlaps fail the build. Exclusive scenes may reuse memory only when their
dependency closures do not coexist. Currently **all extra PRGs are rejected**:
there is no executable overlay relocation, stack migration or live-code handoff.
Future residency needs generation handles and invalidation of sprite, palette,
FIX, render-queue and audio references before any bank is evicted.

Native SPR is not a renamed C-ROM. Conversion maps final cartridge plane lanes
to CD order 1/0/3/2, with right half before left half. Unit tests pin every plane
and half with independent one-pixel fixtures, plus lossless round trips. FIX
retains its 32-byte column swizzle. The compiler consumes indexed pixels without
resampling. Emulator DRAM/decoded-pixel comparison through stock firmware is
still outstanding.

The original firmware uses a separate, private `$1F0000..$1F3FFF` area and stack
at `$1FF000`. These are **Eagle diagnostic choices**, not a discovered stock
BIOS RAM map. Original firmware ROM remains `$C00000..$C7FFFF` (512 KiB).
