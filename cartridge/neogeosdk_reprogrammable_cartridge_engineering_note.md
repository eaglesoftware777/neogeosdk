Eagle Software Engineering Note
Reprogrammable Cartridge PCB Architecture for NeoGeoSDK Homebrew Development
Eagle Software is documenting a technical direction for a reprogrammable cartridge PCB intended for NeoGeoSDK homebrew development, hardware validation, diagnostics, and private engineering work.
The purpose of this board is to allow original NeoGeoSDK output to be programmed into cartridge ROM regions and tested on compatible hardware. It is not intended for unauthorized copying, reproduction, conversion, sale, or use of copyrighted commercial software.
This engineering note describes two possible hardware paths:
```text
1. Parallel NOR flash cartridge
2. SDRAM loader cartridge
```
The recommended first revision is the parallel NOR flash cartridge, because it behaves closest to a fixed ROM cartridge and is easier to validate.
---
Legal and Rights Notice
This project is not affiliated with, endorsed by, approved by, licensed by, or connected to SNK Corporation or any related rights holder.
All trademarks, names, historical references, hardware references, and platform references remain the property of their respective owners.
This work must be used only for:
```text
Original homebrew software
Private development
Technical research
Education
Lawful preservation-oriented study
Authorized consulting work
```
It must not be used for unauthorized copying, redistribution, reproduction, sale, conversion, or operation of copyrighted commercial software.
Any commercial, consulting, manufacturing, or distribution activity must be reviewed for legal compliance and, where required, performed only with proper authorization or acceptance from the relevant rights holders.
---
1. Cartridge ROM Model
The cartridge memory model must be respected as separate functional ROM regions:
```text
P1  - main program region
C1  - sprite graphics region
C2  - sprite graphics region
S1  - fix/text graphics region
M1  - sound program region
V1  - sample data region
```
The cartridge structure uses two boards:
```text
PROG board:
  P1  - main program region
  V1  - sample data region

CHA board:
  C1  - sprite graphics region
  C2  - sprite graphics region
  S1  - fix/text graphics region
  M1  - sound program region
```
For the home console cartridge format, the cartridge uses two boards with 50 pins per side, 100 pins per board, 200 pins per cartridge, 2.54 mm pitch, and 1.6 mm PCB thickness.
---
2. First Revision Target
The first Eagle Software engineering target should be:
```text
Platform:       home console cartridge format
Board style:    two-board cartridge set
Memory type:    parallel NOR flash
ROM layout:     fixed map
Banking:        none in revision 1
Programming:    external USB programmer or onboard programming controller
Purpose:        NeoGeoSDK homebrew validation
```
Recommended first capacity:
```text
P1  = 1 MB
C1  = 2 MB
C2  = 2 MB
S1  = 128 KB
M1  = 128 KB
V1  = 2 MB
```
This capacity is enough for early NeoGeoSDK demonstrations, engine tests, diagnostic ROMs, visual tests, sound tests, and small homebrew programs.
---
3. Architecture A — Parallel NOR Flash Cartridge
This is the preferred first design.
3.1 Principle
In PLAY mode, each flash device behaves like a normal ROM.
In PROGRAM mode, a programmer takes control of the flash address bus, data bus, and write strobes.
```text
                 +---------------------+
                 | USB programmer MCU  |
                 +----------+----------+
                            |
                            v
                 +---------------------+
                 | CPLD / bus control  |
                 +----------+----------+
                            |
     +----------------------+----------------------+
     |                      |                      |
     v                      v                      v
  P1 flash              C1/C2 flash           S1/M1/V1 flash
     |                      |                      |
     +----------------------+----------------------+
                            |
                            v
                  Cartridge edge connector
```
3.2 Advantages
```text
Closest behavior to fixed ROM hardware
No boot loader delay
No SDRAM refresh concern
No runtime image loading
Simpler failure analysis
Good for manufacturing prototype revision 1
```
3.3 Disadvantages
```text
More flash devices
More address/data routing
More level shifting
Less flexible than SDRAM
Large ROM expansion requires larger flash devices or banking
```
---
4. Architecture B — SDRAM Loader Cartridge
This is a more advanced design.
4.1 Principle
The cartridge contains SDRAM. On power-up or reset, a microcontroller loads ROM images from onboard flash, SD card, or USB into SDRAM. After loading, the SDRAM controller presents those memory regions to the cartridge bus as if they were ROM.
```text
              +-------------------+
              | SD / USB storage  |
              +---------+---------+
                        |
                        v
              +-------------------+
              | Loader MCU        |
              +---------+---------+
                        |
                        v
              +-------------------+
              | SDRAM controller  |
              +---------+---------+
                        |
     +------------------+------------------+
     |                  |                  |
     v                  v                  v
  P/V bus            C bus              S/M bus
     |                  |                  |
     +------------------+------------------+
                        |
                        v
             Cartridge edge connector
```
4.2 Advantages
```text
Flexible ROM sizes
Can support SD card loading
Can support multiple homebrew images
Can reduce number of large parallel flash chips
Easier to update content without erasing many NOR devices
```
4.3 Disadvantages
```text
Harder timing problem
Requires deterministic bus response
Requires SDRAM refresh management
Requires loader firmware
Requires robust reset behavior
More complex validation
Harder to manufacture correctly on first revision
```
For Eagle Software revision 1, SDRAM should be considered a later engineering path after the direct NOR flash cartridge is validated.
---
5. Board Division
5.1 PROG Board
The PROG board should implement:
```text
P1 flash region
V1 flash region
Bus transceivers
CPLD or control logic
Programming connector
3.3 V regulator
Mode select circuit
Test pads
Cartridge edge connector fingers
```
Functional role:
```text
P1 = main program read region
V1 = sample data read region
```
5.2 CHA Board
The CHA board should implement:
```text
C1 flash region
C2 flash region
S1 flash region
M1 flash region
Bus transceivers
CPLD or control logic
Programming connector
3.3 V regulator
Mode select circuit
Test pads
Cartridge edge connector fingers
```
Functional role:
```text
C1/C2 = sprite data regions
S1    = fix/text data region
M1    = sound program region
```
---
6. Electrical Requirements
The cartridge must be designed as a 5 V bus system connected to modern 3.3 V logic.
Minimum requirements:
```text
No 5 V signal shall be connected directly to a non-5 V-tolerant input.
All bidirectional data buses shall use controlled bus transceivers.
Flash write enable shall be disabled in PLAY mode.
All control inputs shall have defined pull-ups or pull-downs.
All unused flash control pins shall be tied to known states.
All voltage rails shall have local decoupling.
The programming interface shall never drive the console bus.
```
Recommended devices:
```text
74LVC245 / 74LVC16245 bus transceivers
74LVC573 / 74LVC16373 latches where needed
CPLD or small FPGA for mode and chip-select control
3.3 V parallel NOR flash
RP2040 / RP2350 / STM32-class programming MCU
```
---
7. PLAY / PROGRAM Mode
The board must have two electrically separate states.
PLAY Mode
```text
Console address bus -> flash address bus
Console read strobes -> flash /OE and /CE
Flash data bus -> console data bus
Flash /WE -> forced inactive
Programmer bus -> disconnected
```
In this mode, the board behaves as a read-only cartridge.
PROGRAM Mode
```text
Console cartridge bus -> disconnected
Programmer address bus -> flash address bus
Programmer data bus -> flash data bus
Programmer control -> flash /CE, /OE, /WE
Flash regions -> erase, write, read, verify
```
The safest first revision uses a physical switch or jumper:
```text
PLAY
PROGRAM
```
Default must be safe:
```text
/WE pulled high
/OE pulled high until valid
/CE pulled high until valid
programmer disabled unless PROGRAM mode is selected
```
---
8. NOR Flash Implementation Detail
8.1 Suggested Region Mapping
```text
P1 region:
  width: 16-bit preferred
  size: 1 MB minimum
  device: x16 parallel NOR flash

C1 region:
  width: according to CHA bus wiring
  size: 2 MB minimum
  device: parallel NOR flash

C2 region:
  width: according to CHA bus wiring
  size: 2 MB minimum
  device: parallel NOR flash

S1 region:
  width: 8-bit
  size: 128 KB
  device: small NOR flash or shared flash window

M1 region:
  width: 8-bit
  size: 128 KB
  device: small NOR flash or shared flash window

V1 region:
  width: 8-bit or 16-bit according to board design
  size: 2 MB minimum
  device: parallel NOR flash
```
8.2 Programming Process
```text
1. Put cartridge in PROGRAM mode.
2. Programmer reads board ID.
3. Programmer reads flash manufacturer/device ID.
4. Programmer erases target region.
5. Programmer writes data in flash program units.
6. Programmer reads back region.
7. Programmer verifies CRC32/SHA-1.
8. Programmer reports pass/fail.
9. Cartridge returns to PLAY mode.
```
8.3 File Padding
The SDK output must be padded to the exact hardware region size using `0xFF`.
```text
P1  -> 0x100000
C1  -> 0x200000
C2  -> 0x200000
S1  -> 0x020000
M1  -> 0x020000
V1  -> 0x200000
```
Example region table:
```text
REGION  FILE EXTENSION  SIZE       FUNCTION
P1      .p1             1 MB       main program
C1      .c1             2 MB       sprite data
C2      .c2             2 MB       sprite data
S1      .s1             128 KB     fix/text data
M1      .m1             128 KB     sound program
V1      .v1             2 MB       sample data
```
---
9. SDRAM Implementation Detail
The SDRAM design should be treated as a second-generation cartridge design.
9.1 Required Blocks
```text
Storage:
  SD card, onboard QSPI flash, or USB mass-storage interface

Loader:
  MCU loads ROM files into SDRAM

Memory:
  SDRAM or PSRAM large enough for all ROM regions

Controller:
  FPGA or fast MCU logic serving cartridge bus reads

Bus interface:
  Level shifting between cartridge bus and 3.3 V logic

Protection:
  State machine preventing invalid bus ownership
```
9.2 SDRAM Load Process
```text
1. Power on.
2. Hold cartridge output disabled until loader is ready.
3. Loader reads ROM manifest.
4. Loader copies P1, C1, C2, S1, M1, V1 into SDRAM regions.
5. Loader verifies CRC32/SHA-1.
6. Controller enables console read access.
7. Cartridge presents memory as ROM.
```
9.3 SDRAM Runtime Requirements
The SDRAM design must meet read timing for every cartridge bus access.
Required behavior:
```text
The bus controller must decode the requested region.
It must return stable data within the required access window.
It must handle refresh cycles without corrupting cartridge reads.
It must provide deterministic response.
It must recover cleanly from reset.
It must never expose half-loaded memory to the console.
```
9.4 SDRAM Risks
```text
Late data causes boot failure or corrupted graphics.
Refresh collision causes intermittent crash.
Incorrect bus isolation can damage logic.
Loader delay must be handled safely.
Reset sequencing must be deterministic.
```
For this reason, SDRAM should not be the first production path unless the engineering team has already validated bus timing with a prototype.
---
10. Programming Tool Requirements
The PC-side tool should handle ROM regions explicitly.
Required commands:
```text
identify-board
identify-flash
erase-region P1
write-region P1 file.p1
verify-region P1 file.p1
erase-region C1
write-region C1 file.c1
verify-region C1 file.c1
erase-region C2
write-region C2 file.c2
verify-region C2 file.c2
erase-region S1
write-region S1 file.s1
verify-region S1 file.s1
erase-region M1
write-region M1 file.m1
verify-region M1 file.m1
erase-region V1
write-region V1 file.v1
verify-region V1 file.v1
```
The tool must reject:
```text
Files larger than region
Unknown board revision
Unknown flash ID
Missing required region
CRC mismatch after readback
Programming while board is not in PROGRAM mode
```
---
11. Manufacturing Requirements
The PCB set should use:
```text
Two-board cartridge PCB set
1.6 mm PCB thickness
4-layer stackup
Hard-gold edge fingers
Beveled edge connector
2.54 mm pitch
Solid ground plane
Local decoupling for every IC
Clear silkscreen region labels
Test pads for major buses
Revision ID on both boards
```
Recommended stackup:
```text
Layer 1: components and high-priority signals
Layer 2: ground plane
Layer 3: power and secondary routing
Layer 4: signals
```
First production order:
```text
5 bare PCB sets
Assemble 1 set only
Bench-test before full assembly
Revise before larger run
```
---
12. Bring-Up Procedure
12.1 Bench Test
```text
1. Inspect soldering under microscope.
2. Check 5 V to GND resistance.
3. Check 3.3 V to GND resistance.
4. Power from bench supply with current limit.
5. Verify 3.3 V regulator output.
6. Verify CPLD/MCU clock.
7. Verify mode-select signal.
8. Verify flash ID read.
9. Verify erase/write/read/verify on each flash.
```
12.2 Cartridge Slot Test
```text
1. Insert into unpowered console slot.
2. Confirm mechanical fit.
3. Confirm no edge connector misalignment.
4. Confirm no shell interference.
5. Confirm no short on power rails after insertion.
```
12.3 Powered Console Test
```text
1. Use a known-good console unit.
2. Use current-monitored power.
3. Insert cartridge in PLAY mode only.
4. Power on.
5. Observe current behavior.
6. If current is abnormal, power off immediately.
7. Test P1 boot.
8. Test S1 text.
9. Test C1/C2 sprites.
10. Test M1 sound.
11. Test V1 sample playback.
```
---
13. Diagnostic ROM Validation Order
Do not validate the board with a full game first.
Use a staged NeoGeoSDK diagnostic sequence:
```text
Test 1:
  P1 only boot and watchdog stability

Test 2:
  P1 + S1 text output

Test 3:
  P1 + C1/C2 sprite pattern

Test 4:
  P1 + M1 sound command test

Test 5:
  P1 + M1 + V1 sample playback

Test 6:
  Full NeoGeoSDK demo
```
Failure isolation:
```text
No boot:
  P1 mapping, address bus, data bus, read strobes, reset, checksum

Wrong text:
  S1 mapping, S data lines, address lines

Bad sprites:
  C1/C2 swapped, C bus wiring, C address lines, data plane issue

No sound driver:
  M1 mapping, Z80 region issue, sound reset issue

No samples:
  V1 mapping, sample address issue, sample data bus issue

Random crash:
  Timing margin, bus contention, weak level shifting, power noise
```
---
14. Eagle Software Engineering Position
Eagle Software’s position is that NeoGeoSDK hardware support must remain technically faithful to the cartridge memory model while staying strictly focused on lawful homebrew development.
The first cartridge revision should not try to be universal. It should be a stable, conservative, fixed-layout development board.
Primary goals:
```text
Correct mechanical format
Correct two-board organization
Correct ROM region behavior
Safe PLAY / PROGRAM mode separation
Reliable flash programming
Reliable readback verification
Proper voltage translation
Stable operation on original compatible hardware
```
Secondary goals, only after validation:
```text
Larger ROM regions
Bank switching
SD loading
Multiple homebrew images
Shared arcade/home cartridge design
Advanced diagnostics
Manufacturing test fixture
```
---
15. Short Public Notice
```text
Eagle Software is evaluating a reprogrammable cartridge PCB direction for NeoGeoSDK homebrew development.

The proposed board is intended to support original homebrew software produced with NeoGeoSDK by allowing developers to program and test standard cartridge ROM regions on compatible hardware. The first engineering direction is a conservative flash-based development cartridge focused on reliable region mapping, safe bus control, proper voltage translation, and hardware validation.

Two technical paths are under study: a direct parallel NOR flash cartridge and a later SDRAM loader cartridge. The first revision is expected to use parallel NOR flash with a fixed ROM map, because this provides the most direct and reliable path to hardware validation.

This work is not affiliated with, endorsed by, approved by, licensed by, or connected to SNK Corporation or any related rights holder.

The project is intended only for lawful homebrew development, technical research, education, preservation-oriented study, or authorized consulting work. It must not be used for unauthorized copying, distribution, reproduction, sale, conversion, or operation of copyrighted commercial software.

All copyrights, trademarks, and intellectual property rights must be respected. Any commercial or consulting use should be performed only in accordance with applicable law and with proper authorization or acceptance from the relevant rights holders.
```
