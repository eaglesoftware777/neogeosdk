# NeoGeo P-ROM Bank Switching

> **v1.3.1 status**
>
> The unified 21-chapter demo (`games/demo`, ID 777) and the new
> `games/demo_plus` (ID 778) both fit comfortably in the 512 KB P1
> window without banking.  Bank switching remains an advisory feature
> for future games that need more program space.

This document describes the NeoGeo P-ROM banking mechanism and the `ng_bankswitch` API stub provided in `sdk/2d_engine/ng_bankswitch.h`.

**The current NeoGeoSDK demo does not use bank switching.**  The P1 ROM is kept within the fixed 128 KB window.  This document is advisory for future cartridge builds that need more than 512 KB of program space.

---

## Background

The standard NeoGeo memory map exposes the P-ROM (program ROM) at `0x000000 – 0x0FFFFF` (1 MB).  Cartridges that fit within 512 KB use only the lower half of that window and never need banking.

Large cartridges (1 MB, 2 MB, 4 MB) divide the P-ROM into banks of 1 MB each.  The first 1 MB (`0x000000 – 0x0FFFFF`) is always bank 0 and is always visible.  A second 1 MB window at `0x200000 – 0x2FFFFF` is the banked region; writing a bank number to the bank switch register remaps this window to any 1 MB block of the physical ROM.

```text
Address map (with banking active):

  0x000000 – 0x0FFFFF   fixed window — bank 0 always visible
  0x100000 – 0x1FFFFF   (not used for P-ROM on standard hardware)
  0x200000 – 0x2FFFFF   banked window — controlled by register at 0x2FFFFE
```

---

## Bank switch register

```text
Address:  0x2FFFFE  (word-wide)
Write:    lower byte = bank number (0, 1, 2, ...)
Effect:   maps that 1 MB bank of physical P-ROM into 0x200000 – 0x2FFFFF
```

Example: to access bank 2 of the physical ROM at `0x200000`:

```c
*((volatile uint16_t *)0x2FFFFE) = 2;
/* now 0x200000 maps to physical ROM offset 0x200000 * 2 = 2 MB */
```

After the bank switch, all reads from `0x200000 – 0x2FFFFF` come from the new bank until the register is written again.

---

## SDK API

`sdk/2d_engine/ng_bankswitch.h` / `sdk/2d_engine/ng_bankswitch.c` provide two functions:

```c
void    ng_bankswitch(uint8_t bank);
uint8_t ng_bankswitch_current(void);
```

`ng_bankswitch(bank)` writes `bank` to the hardware register and caches the value in a static variable.

`ng_bankswitch_current()` returns the cached bank number without reading the hardware register (the register is write-only on standard hardware).

---

## How to use it (advisory)

### 1. Linker script

Add a banked section to your linker script.  The fixed section stays at `0x000000`:

```ld
SECTIONS {
    .text   0x000000 : { ... }   /* fixed window */
    .bank1  0x200000 : AT(0x100000) { ... }   /* banked data at physical 1 MB */
}
```

### 2. Place large data in a banked section

```c
__attribute__((section(".bank1")))
static const uint8_t large_table[65536] = { ... };
```

### 3. Switch bank before accessing banked data

```c
ng_bankswitch(1);
value = large_table[index];   /* reads from 0x200000 + offset */
ng_bankswitch(0);             /* restore if callers depend on bank 0 */
```

### 4. Never bank switch from interrupt context

The VBL and IRQ2 handlers run from the fixed window.  If an interrupt fires during a bank switch, it will find the wrong bank in the `0x200000` window.  Either keep interrupt code entirely in the fixed window (recommended) or have the interrupt handler save and restore the bank:

```c
void NEOGEO_INT vbl_handler(void)
{
    uint8_t saved = ng_bankswitch_current();
    /* ... handler code ... */
    ng_bankswitch(saved);
}
```

### 5. Alignment

Each bank is a 1 MB block.  Physical ROM regions must be padded to 1 MB boundaries before flashing.

---

## Current demo limits

The current demo P1 is 128 KB.  The makefile crops and pads to exactly 128 KB:

```makefile
CROP=-crop 0x000000 0x020000
FILL=-fill 0xFF 0x000000 0x020000 -range-padding 4 -o
```

As long as the program fits in 128 KB, no bank switching is needed and `ng_bankswitch` is never called.

---

## References

- SNK hardware documentation and third-party reverse-engineering notes describe the bank register at `0x2FFFFE`.
- The BIOS provides a software helper (arcade: `$C00402`, home: `$C004A2`) that performs the same register write, but direct writes are equivalent and avoid BIOS calling convention overhead.
- WLA-DX linker documentation: https://github.com/vhelin/wla-dx
