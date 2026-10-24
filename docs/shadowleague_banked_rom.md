# Shadow League cartridge layout

This candidate uses the standard, unprotected cartridge mapper. Code, interrupt
handlers, engine tables and all deferred-render callbacks remain in fixed P-ROM.
Banked data is copied to persistent work RAM before its palette map is queued.
The implementation does not execute banked code or retain window pointers.

| Physical P image | CPU mapping | Purpose |
| --- | --- | --- |
| 000000..0FFFFF | 000000..0FFFFF | Fixed code and engine data |
| 100000..1FFFFF | 200000..2FFFFF, selector 0 | Presentation descriptors |
| 200000..2FFFFF | 200000..2FFFFF, selector 1 | Presentation descriptors |

The cartridge build emits exactly 3 MiB. Each bank begins on a 1-MiB boundary;
unused space is FF padding. The P header declares the full 3-MiB physical size.
The current descriptors occupy only 4,648 bytes: this is capacity and a real
bank-window exercise, not a claim of three MiB of new authored content.

`slf_rom_read` accepts a physical offset and bounded 16-bit length. It splits
copies at each bank boundary, validates the entire range before changing the
destination, and returns the window to selector zero after a banked copy.
Normal main-thread scene loading owns this API. Interrupt handlers must stay
independent of the data window. The first 128 CPU bytes are the system's selected
vector table; scene data never uses that reserved range.

The standard selector is a word write to 2FFFF0. Selector N maps physical
(N+1)*100000 into the CPU's 200000..2FFFFF window. The software list selects
`rom`; the former `rom_fatfur2` protection handler intercepts this range and
cannot be used for the banked candidate. These mappings were verified against
MAME's Neo Geo driver
and then exercised by the candidate's actual scene loads in AES and MVS.

`banked_rom.py` rejects fixed code larger than 1 MiB and any overflowing data
bank. It assembles the physical image before swapping each 16-bit word for the
cartridge files. Host tests verify byte-exact payload offsets, padding, copies
across fixed/data and data/data boundaries, final-bank bounds and restoration.
The NeoSD packer preserves the resulting bytes without another swap or rotation.
It accepts complete 1-MiB banked images up to its declared 8-MiB P capacity.
Only this game's three-MiB layout has been exercised in the candidate emulator.

## Graphics addresses and alignment

C1 and C2 are equal 8-MiB lanes, each aligned to whole 64-byte planar tiles.
Together they form a 16-MiB graphics region. Legacy sprites and utility tiles
FFFD..FFFF remain in the low tile space. New presentation starts at tile 10000.
There is no 68000 P-ROM bank switch involved in accessing C-ROM graphics.

`ng_sprite_group_set_tile_address` accepts the full 20-bit tile address. Low 16
bits are written into SCB1's tile word; bits 16..19 go into attribute bits 4..7.
Address carry is computed separately for every row, including flipped strips.
Padding continues to use the bank-zero transparent tile. Existing `set_tile_base`
updates the low address within the current bank; initializing a group resets
its bank to zero. Use the full-address setter when selecting a different region.

The common low-address writer remains in use for legacy art. Host C/C++ tests
cover the 1FFFF/20000 and 7FFFF/80000 seams, palette maps, flips, the final FFFFF
tile, and invalid base addresses. The candidate itself allocates 16 MiB; larger
address bits are covered by the host VRAM tests, not by a 128-MiB cartridge claim.
Deferred tests verify that a queued upload uses the latest address exactly once.
MAME's Neo Geo sprite renderer
provides the hardware reference for address attributes and the 96-strips-per-line
limit. Actual scanline load is measured from sprite RAM in every captured frame.

Each fighter can use up to four palette banks. Palette assignment is per tile,
independent of strip sharing, and both mirror variants retain all authored colors.
The packer rejects sixteen opaque colors in a tile, excessive banks, partial
alpha, missing authored poses and any decoded-ROM pixel mismatch. Current
playable fighters still use their existing single-bank drawings.

## Candidate builds

```
python3 games/shadowleague/tools/build_candidate.py
python3 -m unittest discover -s tests -p 'test_shadowleague*.py'
python3 -m unittest discover -s tests -p 'test_sprite_deferred.py'
make GAME=shadowleague test-precheck
make GAME=shadowleague budget
```

The normal build runs all asset checks. `--reuse-assets` is a code-only rebuild
option after unchanged C/S/M/V assets have already passed a full build. Packages
go to `dist/candidates/shadowleague/rebirth-banked`, with matching AES/MVS ELF
files in `games/shadowleague/build/elf/rebirth-banked`. They do not replace
released packages or the installed MAME copy. Physical-cartridge and flashcart
execution still require a hardware test; emulator results do not establish it.
