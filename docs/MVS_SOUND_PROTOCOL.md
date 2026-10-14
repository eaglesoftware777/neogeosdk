# MVS Sound Protocol

The authoritative ASM driver identifies protocol revision `NGP2` at M1 offset
`$00C0`. Its matching 68000 SDK and M1 must be deployed together. The experimental
C driver does not implement this BIOS/parameter protocol; it is not an alternative
for this MVS hardware-test image.

## BIOS Requests

NMI dispatches these bytes before inspecting ownership, parameters or the FIFO:

| Byte | Action |
|---|---|
| `$01` | Silence FM, SSG and ADPCM; stop/acknowledge timers; transfer through RETN to a RAM routine, then reply `$01` and wait at `$FF85`. |
| `$02` | Restart the driver and start its existing eyecatcher music directly, without FIFO insertion. |
| `$03` | Full driver restart: reset stack, linear ROM banks, RAM, FIFO, parsers, register cache and chip configuration. |
| `$09` | Private game initialization; use `soundInit()` rather than BIOS `$01`. |

The `$01` response is written only from RAM. No foreground ROM fetch follows
it until another request restarts the driver. Timer IRQs are disabled and their
flags cleared before the RAM wait; NMI remains enabled. Ordinary readiness is
`$80`, never `$01`. BIOS `$02/$03` do not wait for an ordinary FIFO acknowledgement;
the driver publishes `$80` when initialization has completed.

`soundReset()` has no arguments and sends `$08`: the existing game scene reset.
`soundHardwareReset()` has no arguments and sends `$03`, discarding a pending
sender transaction. `soundInit()` has no arguments and sends `$09` without
first waiting for ordinary readiness, so it also works from a BIOS RAM wait.

## Parameters

Public API arguments and their ranges are unchanged. `soundCommand()` tracks
one-argument commands and performs this wire encoding automatically:

| Logical argument | Bytes transmitted |
|---|---|
| `$01` | `$FF $81` |
| `$02` | `$FF $82` |
| `$03` | `$FF $83` |
| `$09` | `$FF $89` |
| `$FF` | `$FF $7F` |
| Any other byte | The original byte |

The Z80 consumes `$FF` as a parameter escape, not a FIFO entry. The following
byte is XORed with `$80` and enqueued as the original argument. Raw BIOS bytes
still take precedence during an incomplete escape. A restart or slot request
clears both receive states. This removes the ambiguity without reserving any
argument value or changing music/sample data.

Commands with one argument are `$05/$06/$07/$0A/$0E/$12/$13/$14/$15/$16/$17/$18/
$19/$1A/$1B/$1D/$1E/$1F/$31/$32`. A raw port writer must encode the argument
itself; SDK callers must not pre-escape it. Do not interleave two callers' command
and parameter transactions. Use `soundHardwareReset()` to abort one deliberately.

## Release-Specific Adapter

The sound-only image builder starts from the original MVS v1 image, preserving
its header, S1, C and sample regions byte for byte. It patches only the verified
sound functions and erased P1 padding. The adapter uses one previously unused
RAM byte at `$10EF00`; it is release-specific, not a general ROM patch.

For ordinary source builds, protocol state is in `.bss.sound_protocol`, retained
when generic `.bss` is stripped and placed in game RAM by the linker.

Install WLA-DX and a 68000 binutils toolchain. If automatic discovery fails,
set `M68K_PREFIX` to the full prefix ending in `m68k-unknown-elf-` or `m68k-elf-`.
Both P1 initialization and command adapters are assembled as 68000 code.

```sh
python3 tools/build_mvs_sound_prerelease.py \
  --output dist/release/Maiya-WIP-NeoSD_MVS_SOUND_FIX_PROTOCOL_PRERELEASE_v1.neo
```

The builder rejects existing output files. Preserve old test images; do not
replace a published artifact silently. V1 remains exactly `$800000` bytes.

## Validation

`tests/mvs_sound_protocol.lua` checks BIOS priority during incomplete parameters,
RAM waiting and chip silence, direct eyecatcher startup, and all 256 parameter
values through the actual patched 68000 sender. Set `MVS_PROTOCOL_REPORT` to
the output report and run it as a MAME autoboot script with matching ROM hashes.
The script records failures explicitly; an emulator exit code alone is not a pass.

`tests/mvs_sound_boot.lua` additionally checks foreign RAM handoff, bank/cache
reconstruction, reply ordering and restart timing. Packing tests enforce protected
region equality. Z80Ex tests are optional and require that library.

Physical MVS/NeoSD validation remains necessary: cold/warm boots, slot changes,
coin/start, ADPCM-A/B, FM, SSG and repeated scene transitions. Successful MAME
playback does not prove that the reported hardware silence is resolved.
