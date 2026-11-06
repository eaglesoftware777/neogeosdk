# CD BIOS interoperability ledger

The native program targets documented CD entry arrangements. Its stock-firmware
interoperability gate has not passed: no recognized stock CD BIOS was supplied.
The secondary HLE run cannot establish exact vendor ABI behavior.

| Contract | Implemented boundary | Verified scope |
|---|---|---|
| Native PRG `$100` `NEO-GEO`, flag 2; USER jump `$122` | original assembly/header and independent parser | encoded bytes, native HLE execution |
| USER request and return `$C00444` | startup dispatcher | HLE init/game transition; stock pending |
| SYSTEM_IO `$C0044A`, interrupt completion `$C00438` | preserving registers, chaining completion | HLE frame clock and input; stock pending |
| UPLOAD `$C00546` | reserved original firmware entry; no runtime implementation | unsupported |
| LOADFILE `$C00552`, LOADFILEX `$C00564` | bounded descriptor/raw assembly wrapper | host bounds/format only; actual ABI/flags pending |
| CDDA `$C0056A` | bounded track/BCD wrapper | no firmware playback claimed |
| VIDEOEN `$C00570` | listed service address | not implemented/verified |
| CD virtual-card services | no speculative calls or raw writes | unsupported; storage untouched |

Source register/frame calls:
[BIOS calls](https://wiki.neogeodev.org/index.php/BIOS_calls),
[68k header](https://wiki.neogeodev.org/index.php/68k_program_header),
[loading descriptors](https://wiki.neogeodev.org/index.php/Loading_files),
[UPLOAD](https://wiki.neogeodev.org/index.php/BIOSF_UPLOAD),
[CDDA](https://wiki.neogeodev.org/index.php/CDDA).
Several public pages explicitly need further research. Do not infer success
from D0, a carry flag or a completed call until error/ABI probes establish it.

`NG_CD_ENABLE_EXPERIMENTAL_BIOS_IO` enables raw development calls only; results
still return unsupported. It is off in every validated build. Do not enable it
for a release or treat it as a working disc loader. No descriptor is generated
for program overlays. A manifest cannot substitute for checking actual file
size and checksum after a real load.

The stock QA tool recognizes public MAME 0.264 SHA1 metadata for front-loader,
top-loader and CDZ, and checks model agreement. Hashes identify a revision;
they do not authorize obtaining it. Firmware files are supplied by the user,
never bundled or downloaded by this SDK. MAME's file byte order differs from
the raw mapped 68000 image; QA normalizes explicitly and compares all 512 KiB
actually mapped in the emulator.

New system ABI: [EagleBIOS-CD](EAGLEBIOS_CD_ABI.md). Its identity and diagnostics
must never be confused with stock CD firmware or the existing 128 KiB cartridge
EagleBIOS. Cross-region behavior and START/SELECT scheduling require stock tests;
CD is not classified as AES merely because it has no coins.
