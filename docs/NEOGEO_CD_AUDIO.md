# CD audio — authored media versus proven playback

| Feature | Cartridge path | CD development status |
|---|---|---|
| CDDA | unavailable | valid original track authored/round-tripped; runtime controls unsupported |
| ADPCM-A | existing ROM driver | original RAM Z80 driver and 4 KiB sample; cue captured in HLE only |
| ADPCM-B | existing game usage | unavailable in CD backend; never silently substituted |
| FM / SSG | existing driver support | not implemented or recorded by CD fixture |
| Concurrent CDDA + SFX | not applicable | untested |
| Scene-load audio restart | cartridge behavior unchanged | unsupported |

The hello-world disc contains an original 4-second diagnostic chord, stereo
16-bit signed PCM at 44,100 Hz, and a separate original decaying ADPCM-A cue.
The Z80 code is source-original WLA assembly linked into a full 64 KiB RAM
image. It does not assume a cartridge M1 or copy a stock sound driver. Sample
addresses are in the RAM driver's own 256-byte-unit table. `playSFX(0)` queues
its own command; an accepted command is not a hardware audibility guarantee.

The CUE puts MODE1/2048 data first, then track 02 from WAV with a two-second
pregap and index 01 at zero in its own file. Audio must contain whole 588-sample
sectors. Validation rejects mono, non-PCM, incorrect rate/width, incomplete or
truncated sectors and malformed/escaping CUE paths. chdman independently
compressed, verified and recovered the exact data and PCM bytes.

In the recorded NeoCD HLE diagnostic, A produces the cue (peak 5,580 signed
16-bit units), with zero pre-button audio. This demonstrates a specific emulator
RAM-driver path, **not** CDDA, stock BIOS behavior or physical sound quality.
The WAV capture and exact external-core hash are in the validation report.

CDDA track arguments are validated as tracks 2..99 and encoded in BCD. Play,
loop, pause, resume, stop, read readiness and restart-after-load remain pending
firmware probes. They return `NG_CD_UNSUPPORTED` by default. No gapless-loop or
music-plus-effect claim is made. CDDA polling is disabled in the native header;
future explicit BIOS services need SYSTEM_IO every VBlank and real return/error
evidence. [Public CDDA notes](https://wiki.neogeodev.org/index.php/CDDA).
