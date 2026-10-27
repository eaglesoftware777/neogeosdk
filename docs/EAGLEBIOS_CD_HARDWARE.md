# EagleBIOS-CD hardware and interrupt ledger

Sources are public hardware notes and inspected emulator source. Addresses
are 68000 byte addresses; byte lanes matter. Evidence below is emulator-only:
CDZ loader/game tests used MAME 0.283; the earlier front/top tests covered F1
menus. Physical drive timing, lid/tray behavior and media changes remain open.

| Region/register | CD use | Evidence/status |
|---|---|---|
| `$000000..$1FFFFF` | program DRAM and firmware work areas | native PRG and isolated loader state |
| `$C00000..$C7FFFF` | 512 KiB system ROM | exact mapped-byte comparison |
| `$300000/$340000/$380000` | controllers and START/SELECT | game input sampled; complete two-player matrix pending |
| `$300001` | watchdog | bounded loader/error loops and gameplay |
| `$3C0000/$02/$04/$0C` | VRAM and LSPC IRQ acknowledgement | FIX loader text, gameplay and VBlank |
| `$400000..$401FFF` | palette RAM | native menu and game captures |
| `$FF0004/$FF000F` | CD-side mask/acknowledgement | TOC, sectors, runtime loads |
| `$FF0101/$103` | CDC register selection/data | sector notification, header, pointer and transfer |
| `$FF0064/$70/$7E..$8E/$61` | LC8953 destination/count/program/trigger | 2048-byte DMA sector matched ISO bytes |
| `$FF0105` | SPR 0, PCM 1, Z80 4, FIX 5 transfer areas | all five native payload classes loaded |
| `$FF0121/$123/$127/$129` | SPR/PCM/Z80/FIX bus request | initial upload and runtime SPR readback |
| `$FF0141/$143/$147/$149` | corresponding bus release | successful/error file-transfer paths |
| `$FF0111/$115/$119` | video/layer controls | original loading screen and native games |
| `$FF0161/$163/$165` | drive response/command/serial handshake | checksummed packets, TOC, reads and CDDA |
| `$FF016D` | ROM/RAM vector selection | IPL interrupt delegation and USER handoff |
| `$FF0181` | communication reset/enable | drive bring-up |
| `$FF0183` | sound CPU reset/release | native RAM driver and game sound effects |
| `$FF01A1/$1A3` | sprite/PCM upload banks | four resident SPR banks; second-bank runtime readback |
| `$E00000` | upload window, odd lanes for 8-bit regions | encoded native assets and exact sprite readback |
| `$800001 + 2*n` | backup-memory byte lane | no save reads/writes or formatting implemented |

Vectors `$54/$58/$5C` are CD controller paths, not conventional autovectors.
A native program must preserve/delegate them during IPL, before USER can repair
RAM vectors. USER is entered through a jump and returns initial requests through
its firmware scheduler. Transfer services must permit required interrupts and
restore the caller's interrupt mask afterward.

The transport has bounded serial waits and sector timeouts. It checks packet
checksums, sector headers and expected LBAs. The loader validates ISO endian
pairs, extents and directory padding, plus IPL terminators, alignment, overlap
and bank limits. Host malformed-media tests supplement native no-disc and
invalid-disc probes; neither proves resilience to every physical drive fault.

Front/top mechanics, hot media changes, extended audio behavior, DMA bus timing,
interrupt storms and saves still require broader tests. Do not infer hardware
certification or complete optical-model support from CDZ emulator results.

References:
[register map](https://wiki.neogeodev.org/index.php/Memory_mapped_registers),
[drive control](https://wiki.neogeodev.org/index.php/CD_drive_control),
[MAME CD driver](https://github.com/mamedev/mame/blob/mame0283/src/mame/neogeo/neogeocd.cpp),
[MAME CDC model](https://github.com/mamedev/mame/blob/mame0283/src/mame/shared/megacdcd.cpp).
