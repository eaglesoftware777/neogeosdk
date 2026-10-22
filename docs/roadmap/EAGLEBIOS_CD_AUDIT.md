# EagleBIOS-CD F0 audit — 2026-10-21

Source baseline: local/remote/PR 9 `8cb21a9b23252112a8a2c0a1f8a56f30a3638aef`,
branch `fix/maiya-arenas-eagle-bios`, plus preserved newer local work.
No Git identity, commit, branch, stash, remote or publication was changed.

| Boundary | Decision | Evidence / remaining risk |
|---|---|---|
| Existing 128 KiB EagleBIOS | retain source, linker, outputs and build paths | source hashes, AES/MVS contract probes passed |
| System ROM/vector map | independent 512 KiB CD linker and vectors | source-built F1 exact-byte probes; game compatibility pending |
| Font/LO | reuse existing source-original generators without refactoring cartridge code | original font visible in CD FIX; isolated original LO for MAME |
| CD UI | original diagnostic menu, distinct profile and explicit incomplete state | native front/top/CDZ screenshots |
| Controller/IRQ | original bounded F1 sampling and watchdog handling | A/Right, monotonic frames, PC/SP bounds; full CD game ABI pending |
| Drive controller | separate CD-only future state machine | TOC/sector/invalid-media F2 not implemented |
| File upload/boot | separate native zones and future ISO/IPL firmware parser | host media tools exist; firmware F3 has no disc parser yet |
| CDDA and Z80 | separate original RAM-driver fixture; no cartridge M1 assumption | HLE fixture effect captured; firmware audio/CDDA pending |
| Battery memory | preserve all bytes; no provisional formatting | no save implementation, persistence untested |
| Build/install | outputs isolated under CD-specific paths; explicit test ROM mapping | Windows/Linux ROM identity; no physical installation |

Public emulator hardware source was inspected as a behavioral reference outside
the SDK. No emulator implementation, vendor firmware code, sample or asset is
copied into the firmware. The SDK font/LO reuse is original project source.
External NeoCD core commit: `b1e04c738cb48a1dae0574b8877f6a116d270ca1`;
its LGPL source/binary remain external and are not bundled with these changes.

The current private allocation is `$1F0000..$1F3FFF`, stack `$1FF000`. It is
explicitly not a claim about stock BIOS work RAM. Original CPU reset signature
and exact 512 KiB mapped ROM are tested. Profile names do not prove mechanical
model support: front-loader tray and top-loader/CDZ lid behavior still need F2.

No stock CD BIOS was found in the scoped development inventory or supplied for
this task. None was downloaded. The required stock-compatible native boot test
remains blocked. No hardware writes or retail disc compatibility tests occurred.

See [hardware ledger](../EAGLEBIOS_CD_HARDWARE.md),
[ABI ledger](../EAGLEBIOS_CD_ABI.md) and
[actual firmware results](EAGLEBIOS_CD_VALIDATION_REPORT.md).
