# EagleBIOS

An original system ROM set for Neo Geo hardware, built from source in this
directory.  It boots the SDK's homebrew cartridges on an MVS arcade board
and on an AES console without any vendor system ROM: the 68000 firmware,
the Z80 system sound program, the system text font and the sprite shrink
table are all generated here.

| Output          | What it is                                            | Where a machine expects it |
|-----------------|-------------------------------------------------------|----------------------------|
| `sp-s2.sp1`     | 68000 firmware, byte-swapped, board id `8002` (MVS)   | MVS system ROM slot        |
| `neo-epo.bin`   | The same firmware assembled with id `0002` (AES)      | AES system ROM             |
| `eagle_bios.rom`| The firmware in plain big-endian order (for tools)    | -                          |
| `sm1.sm1`       | Z80 system sound program                              | MVS motherboard M1         |
| `sfix.sfix`     | System text font, 5x7 single ink glyphs               | MVS motherboard S-ROM      |
| `000-lo.lo`     | LSPC vertical shrink lookup table                     | Both boards                |

Everything is written from the documented programming interface that a
cartridge relies on: the service jump table at `C00400`, the work RAM
variables from `10FD80`, the USER request sequence, the message stream
format, the credit rules.  No vendor code, data, font or artwork is copied.
Where the original behaviour is a choice rather than a contract, this
firmware makes its own choice and says so below.

## Building

Requirements: the SDK's `m68k-unknown-elf` toolchain (`TOOLCHAIN=` points at
its `bin`), `wla-z80` and `wlalink` for the sound program, `srec_cat` for
the ROM images, Python 3 for the generators.

```
cd bios
make            # all six outputs
make install    # copy them into test_roms/{neogeo,aes} for the launch scripts
make test       # firmware contract on both boards (needs MAME on the PATH)
make test-games # every SDK cartridge in ../roms on the arcade firmware
make clean
```

From the repository root, `make test USE_EAGLE_BIOS=1 GAME=<game>` and
`./test_demo_bios.sh` launch MAME with these ROMs instead of a vendor set.

## What happens at power-on

1. **Reset** (`bios_reset`).  Interrupts masked, watchdog kicked, the system
   vectors mapped at `000000`, the LSPC quiet.  Work RAM `100000-10EFFF` and
   the private area `10F400-10FFFF` are cleared; the supervisor stack at
   `10F300` is left alone.  `.data` is copied out of ROM.
2. **Board probe.**  Bit 7 of the status port at `380000` tells an arcade
   board from a console; `10FD82` is set to `80` or `00` accordingly.  The
   region byte comes from the firmware's own id word (`02`, Europe).  On an
   MVS the motherboard font and sound program are selected, slot 0 chosen,
   coin lockouts released and backup RAM credits zeroed.  A console has no
   motherboard font or sound program, so the cartridge's are selected.
3. **Service menu** when the arcade test switch or the test DIP is set.
4. **Cartridge check.**  The `NEO-GEO` signature at `000100` must be present;
   otherwise the screen says so and waits.  The cartridge's regional soft
   DIP defaults are copied to `10FD84`.
5. **Title screen** with the Eagle fanfare (below), and on the arcade board
   the eye-catcher straight after it.  Then the sound handoff: the system
   sound program parks itself in Z80 RAM and answers, the cartridge sound
   program is switched in and sent the reset code.
6. **USER request 0** (power-on initialisation) is issued to the cartridge.

## The USER contract

A cartridge is entered by a jump to `000122` with:

* `SR = 2700`, interrupts masked; the game raises the mask itself when its
  VBlank handler is ready.
* `A7 = 10F300`, all other registers zero, `D0` holding the request.
* Cartridge vectors mapped, backup RAM write-protected, sound reset sent.
* `10FDAE` USER_REQUEST, `10FDAF` USER_MODE, `10FD80` SYSTEM_MODE = 0.
* FIX map filled with tile `0020`, all sprites off screen at full size,
  both palette banks black, auto-animation speed `4000` in the LSPC mode
  register, all interrupts acknowledged.

Requests, in the order a machine issues them:

| Request | Meaning        | MVS                    | AES                                         |
|---------|----------------|------------------------|---------------------------------------------|
| 0       | Power-on init  | once                   | once                                        |
| 1       | Eye-catcher    | never                  | only when cartridge byte `114` is 1         |
| 2       | Demo / game    | after 0 and after 3    | after 0 (or 1) and after 3                  |
| 3       | Title          | on a coin during demo  | on a coin during demo                       |

The cartridge returns through `SYS_RETURN` (`C00444`), which restores the
system vectors and stack, decides the next request and enters USER again.
Cartridge byte `114` selects the eye-catcher: `0` the system draws its own,
`1` the cartridge draws it on request 1, `2` none at all.

## Title screen, eye-catcher and fanfare

The **title screen** is the firmware banner: name, the board it found, the
cartridge id it is about to start, and the Eagle fanfare.  It holds for
about a second; any button, START or a coin moves on.

The **eye-catcher** is deliberately plain, the way an old home computer
announced itself: no sprites, no tile art, no palette ROM.  The wordmark is
five block letters laid out on the FIX layer, every "pixel" a cell of the
ordinary text font, so it draws identically from the system font on an MVS
and from whatever font the cartridge carries on an AES.  Letters land one at
a time to a tick, the wordmark holds for a moment, then the ink cools to
black.  Any button skips it.

The **fanfare** lives in the system sound program: three rising notes
(C5, E5, G5), the top C held, a breath, and a short answer, on SSG channel
A with a root under it on channel B, each note fading in three steps.  It
plays on an arcade board, where the system sound program is in charge at
power-on; a console has no system sound program, so its cartridge driver
is already live and the presentation there is silent.

On an arcade board the title screen and the eye-catcher run back to back at
power-on.  On a console the title screen runs at power-on and the
eye-catcher between request 0 and request 2 when the cartridge asks for the
system eye-catcher, the order a home cartridge is written for.

### System sound program codes

| Code | Effect                                                              |
|------|---------------------------------------------------------------------|
| 1    | Park: move to Z80 RAM, mute, answer `01`, then wait for the reset code and restart from the cartridge program |
| 2    | The Eagle fanfare                                                   |
| 3    | Mute (the reset code)                                               |
| 4    | One short tick                                                      |

A new code interrupts a tune within 10 ms.

## Services (jump table at `C00400`)

| Address  | Name              | Behaviour                                                                  |
|----------|-------------------|----------------------------------------------------------------------------|
| `C00400` | id word           | `8002` MVS / `0002` AES; low byte is the region                            |
| `C00402` | reset             | cold and warm boot                                                         |
| `C00408`-`C00432` | exceptions | red diagnostic screen with the fault PC, address, opcode and stack        |
| `C00438` | SYS_INT1          | VBlank housekeeping: acknowledge, watchdog, tick, SYS_IO, then MESS_OUT when the system owns the screen |
| `C0043E` | SYS_INT2          | timer interrupt acknowledge                                                |
| `C00444` | SYS_RETURN        | end of a USER request                                                      |
| `C0044A` | SYS_IO            | pads, start/select, coins, credits, START and coin callbacks into the cartridge |
| `C00450` | SYS_CREDIT_CHECK  | can the requested credits (`10FDB0`, `10FDB1`) be spent; carry set if not  |
| `C00456` | SYS_CREDIT_DOWN   | spend them                                                                 |
| `C0045C` | SYS_READ_CALENDAR | BCD date and time into `10FDD2`                                            |
| `C00462` | SYS_SET_CALENDAR  | accepted, nothing to write to                                              |
| `C00468` | SYS_CARD          | memory card: answers "not inserted" (`80`)                                 |
| `C0046E` | SYS_CARD_ERROR    | no-op                                                                      |
| `C00474` | SYS_HOWTOPLAY     | no-op                                                                      |
| `C004C2` | SYS_FIX_CLEAR     | FIX map to `0020`                                                          |
| `C004C8` | SYS_LSP_1ST       | every sprite height 0, full size, parked at x = 496 off the right edge     |
| `C004CE` | SYS_MESS_OUT      | run the message queue ending at `10FDBE`                                   |
| `C004D4` | controller setup  | reset pad status bytes                                                     |
| `C004DA` | timer entry       | same as SYS_INT2                                                           |

Unused slots hold `rts`.  The header text at `C004E0` names the firmware.

### Coins and credits

Credits live in backup RAM at `D00034` (P1) and `D00035` (P2) as BCD, as
cartridges expect.  A coin edge on the arcade status port adds one credit
and calls the cartridge's COIN_SOUND entry (`000134`).  During the demo, a
coin also calls DEMO_END (`00012E`) and re-enters USER with request 3.  A
START during attract or title asks the cartridge (`000128`) to accept it and
then spends the credit; free play (DIP bit 6 clear, or the console) never
charges.  SYS_CREDIT_CHECK and SYS_CREDIT_DOWN implement the shared and
separate coin-chute rules from DIP bit 1.

### The message stream

SYS_MESS_OUT walks the pointer queue that ends at `10FDBE`, running each
stream: format and parameter (1), increment (2), absolute and relative
addressing (3, 5), data blocks (4, 6, 7), text runs on the FIX map with the
next palette on the second row (8), subroutine call and return (10, 11) and
fills (12, 13).  A zero pointer means the stream itself continues inline in
the buffer, which is how the SDK's `mess_out` writes text, so the queue end
may sit on any word boundary.  Work per call is bounded so a bad pointer
from a cartridge cannot wedge the VBlank.  The Japanese common-FIX
translation (9) is not provided.

## Service menu

Reached with the test switch or test DIP at boot, or button A on the "no
cartridge" screen: hardware information, pad and system button test, colour
bars, a sound code sender for the cartridge driver, and settings (region,
free play).

## Memory

```
C00000-C003FF  exception vectors
C00400-C004DF  service jump table
C004E0-C0051F  identification text
C00520-        code, constants, initial data
10F300         supervisor stack top
10F400-10FBFF  firmware private RAM (.data, .bss)
10FD80-10FFFF  the documented work variables shared with the cartridge
10FF00-        message queue
```

## Testing

`test_bios.py` never touches an installed ROM set: it copies this
directory's outputs into a scratch MAME configuration and drives the
machine from a Lua script that inserts coins, presses START, takes a
screenshot every second and logs the firmware variables.

* `--game probe` boots a purpose-built cartridge (`tests/probe.s`) that
  records what it was handed and asserts the contract above on both boards:
  entry registers, request order, BCD credit carry across ten coins, one
  accepted START, pad status, MESS_OUT output, message queue reset.
* `--game probe0 --platform aes` is the same cartridge asking for the
  system eye-catcher: request 1 must be skipped and the demo must not start
  before the presentation has run.
* `--game <sdk game>` boots `../roms/<game>` and checks that it reaches
  attract, takes a coin into its title, spends exactly one credit on START,
  and, on the console, that the eye-catcher hands over to the game.  An AES
  build of a game (`make PLATFORM=aes p1` at the root) can be substituted
  with `--p1`.

Captures land in `out/tests/<game>-<board>/`.

## Notes for cartridge authors

* The USER entry state above is exactly what a program may assume.  In
  particular interrupts are masked: a routine that waits on a VBlank
  counter before enabling them will wait forever, on this firmware and on
  the original.  The SDK's `helloworld` eye-catcher does this, which is why
  its arcade build (byte `114` = 1) stalls on a console while its console
  build (byte `114` = 0) plays the system eye-catcher and runs.
* Palette copies in this firmware are written in assembly with two address
  registers.  The C loop compiled to `move.w (a0)+,(0,a0,d0.l)`, and a 68000
  computes that destination with the already incremented register, which
  shifted every colour by one entry.  Keep that in mind for any 68000 code
  that reads and writes through the same register.
* The display chip walks sprite strip 0 on every line even when no sprite
  is active, so a strip with height 0 but x = 0 still paints its first tile
  down the left edge.  Park unused strips off screen, as SYS_LSP_1ST does,
  rather than relying on height alone.

## Limitations

No memory card support, no clock (the calendar answers a fixed date), no
HOWTOPLAY screen, no Japanese common-FIX text, one cartridge slot, region
fixed by the firmware id word.
