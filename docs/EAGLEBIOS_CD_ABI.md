# EagleBIOS-CD ABI — experimental native loader

This is a documented subset of the CD BIOS ABI. Reserved jump slots are not
compatibility evidence. CDZ execution has been checked in MAME 0.283; physical
hardware and complete vendor service conformance remain unverified.

| Address | Current contents / behavior |
|---|---|
| `$C00000` | initial SP `$0010F300`; reset selects private SP `$001FF000` |
| `$C00004` | reset PC `$C01000` |
| `$C00054/$58/$5C` | CD controller IRQ acknowledgement handlers |
| `$C00064/$68/$6C` | register-preserving VBlank wrapper |
| `$C00402` | original reset jump |
| `$C00438/$43E` | preserving IRQ entries |
| `$C00444` | USER return scheduler; advances initial requests 0, 1, 2 |
| `$C0044A` | register-preserving input sample into CD BIOS work RAM |
| `$C00552/$564` | bounded native asset-file descriptor load, list in A0 |
| `$C0056A` | direct CDDA command, action/BCD track word in D0 |
| `$C00546/$570` | standalone upload/video entries return D0 = -95 |
| `$C00700` | `EAGLECD\0`, 16-bit major/minor/patch/model, then 32-bit capabilities |

Identity fields are big-endian. Version is 1.7.7; profile IDs are 0 front,
1 top, 2 CDZ. Capability mask is 15: bit 0 diagnostics, bit 1 native IPL boot,
bit 2 runtime asset-file loading, bit 3 direct CDDA. These bits do not advertise
saving, executable overlays, all optical models or full stock compatibility.
There is no callable capability-discovery extension yet.

File descriptors contain a NUL-terminated 8.3 filename, bank byte, padding to
an even absolute address, and a big-endian 32-bit BIOS destination. A zero
filename byte terminates the list. Lists must reside below `$100000` and contain
at most 32 records. SPR destinations count bytes; FIX/Z80/PCM BIOS destinations
count upload-window byte addresses, twice the physical payload offset. Native
bank capacities and alignment are enforced. Runtime PRG overlays are refused.

File and CDDA services preserve D1–D7, A0–A6 and the caller's interrupt mask,
returning status in D0. They temporarily permit interrupts required for disc
I/O. Zero means success; errors include -2 missing file/media, -5 I/O, -22
malformed data, -34 address/bank limits, -95 unsupported and -110 timeout.
The runtime probe verified sprite-bank bytes, preserved registers/SR and
missing-file, bank-limit and executable-overlay errors.

The CDDA word has an action byte above a BCD track byte. Actions 0/1 play,
2 pause, 3 resume are implemented. Play resolves the audio track through TOC
and rejects data tracks. Game adapters have exercised music changes and sound
effects; every action and failure path has not yet passed a conformance suite.

Reset clears only linker-owned private BSS. Successful boot initializes the
CD BIOS work area and jumps to native USER. Initial USER requests return via
`$C00444`, not RTS. Native program vectors must delegate drive interrupts to
the mapped firmware before IPL completes. Cartridge ABI and linker are unchanged.
No backup-memory formatting or save writes are performed.
