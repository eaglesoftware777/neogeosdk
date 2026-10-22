# EagleBIOS-CD hardware and interrupt ledger

Sources are public hardware notes and independently inspected emulator source,
not vendor firmware implementation. **Only rows marked F1 were exercised by
this firmware.** Addresses are 68000 byte addresses; byte-lane distinctions matter.

| Region/register | Intended CD use | Evidence/status |
|---|---|---|
| `$000000..$1FFFFF` | physical program DRAM, overlapping firmware/work areas | public map; private F1 RAM exercised |
| `$C00000..$C7FFFF` | 512 KiB system ROM | full mapped-byte comparison, F1 |
| `$300000`, `$380000` | P1 directions/buttons and START/SELECT inputs | A/Right F1; broader controls pending |
| `$300001` | watchdog service | F1 loop/IRQ; no reset seen in probe |
| `$3C0000/$02/$04` | LSPC VRAM address/data/increment | FIX menu and sprites disabled, F1 |
| `$3C000C` | LSPC interrupt acknowledgement | monotonic F1 clock |
| `$400000..$401FFF` | selected palette RAM | original text/backdrop, F1 |
| `$FF0004` | CD-side IRQ mask | `$0030` diagnostic VBlank configuration, F1 |
| `$FF000F` | CD IRQ acknowledgement | diagnostic acknowledgement only; no transfer proof |
| `$FF0101/$103` | CDC register selection/data | drive gate F2 pending |
| `$FF0105` | transfer region: SPR 0, PCM 1, Z80 4, FIX 5 | FIX upload only, F1 |
| `$FF0121/$123/$127/$129` | SPR/PCM/Z80/FIX bus request | FIX only, F1 |
| `$FF0141/$143/$147/$149` | matching bus release | FIX only, F1 |
| `$FF0111/$115/$119` | layer/video controls | original menu, F1; model semantics need further probes |
| `$FF0161/$163/$165` | response/command nibble/serial handshake | public emulator models differ; F2 pending |
| `$FF016D` | CD vector mapping | native HLE diagnostic only; stock test pending |
| `$FF0181` | communication reset/enable | held disabled at F1; timing unverified |
| `$FF0183` | sound CPU reset/release | firmware holds reset; native HLE RAM-driver cue captured separately |
| `$FF01A1/$1A3` | sprite/PCM upload bank selection | range policy tested; hardware uploads pending |
| `$E00000` | upload window, odd byte lanes for 8-bit regions | original FIX bytes, F1 |
| `$800001 + 2*n` | backup-memory byte lane | no firmware reads/writes or formatting; save gate pending |

Program vector table entries `$54/$58` represent CD controller paths in the
inspected map; they are not conventional autovectors. Do not infer drive ISR
correctness from the diagnostic handler acknowledging a bit. The native game
preserves firmware CD vectors and chains VBlank completion rather than owning
the whole optical subsystem.

MAME 0.264's CD driver itself documents original-model and timing limitations.
Front/top/CDZ F1 menus passed, but mechanical differences, tray/lid transitions,
media changes, timeouts, DMA/CDC behavior and IRQ storms remain untested. No
front/top/CDZ optical support is declared from these results.

References:
[register map](https://wiki.neogeodev.org/index.php/Memory_mapped_registers),
[MAME 0.264 CD driver](https://github.com/mamedev/mame/blob/mame0264/src/mame/neogeo/neogeocd.cpp),
[MAME 0.264 CDC model](https://github.com/mamedev/mame/blob/mame0264/src/mame/shared/megacdcd.cpp),
[NeoCD source](https://github.com/libretro/neocd_libretro/tree/b1e04c738cb48a1dae0574b8877f6a116d270ca1).
