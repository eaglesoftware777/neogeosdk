# EagleBIOS-CD ABI — F1 diagnostic definition

This is not a complete vendor CD BIOS ABI. Games must not rely on private
extensions or treat reserved jump slots as compatibility evidence.

| Address | Current contents / behavior |
|---|---|
| `$C00000` | initial SP `$0010F300`; reset immediately selects private SP `$001FF000` |
| `$C00004` | reset PC `$C01000` |
| `$C00054/$58` | diagnostic CD IRQ acknowledgement handlers; no sector loader |
| `$C00064/$68/$6C` | preserving diagnostic IRQ wrapper |
| `$C00402` | original reset jump |
| `$C00438/$43E` | diagnostic IRQ entries; stock USER scheduling not implemented |
| `$C00444` | reset jump; game-return scheduling not implemented |
| `$C0044A` | register-preserving diagnostic input sample |
| `$C00546/$552/$564/$56A/$570` | reserved upload/load/CDDA/video entries return D0 = -95 |
| `$C00700` | `EAGLECD\0`, 16-bit major/minor/patch/model, then 32-bit capabilities |

Identity fields are big-endian. Current version is 1.7.7; profile IDs are
0 front, 1 top, 2 CDZ. Capability bit 0 means independent diagnostics only.
No bits advertise disc boot, runtime loader, CDDA, saving or stock compatibility.
There is no callable capability discovery service yet; a future game must probe
a documented versioned service and gracefully fall back, not read this marker
as permission to call unimplemented APIs.

Reset leaves stock-style game work RAM alone and clears only linker-owned
private BSS. The input test currently records firmware-private state, not a
completed stock-compatible BIOS RAM contract. CD START/SELECT and USER requests
have not passed conformance tests. Other reserved/unused table positions must
not be called. No failure is converted into a false successful disc load.

The existing cartridge ABI, linker, sources, names and tests are unchanged.
All documented private addresses apply only to this original diagnostic ROM.
