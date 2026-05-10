#ifndef NG_BANKSWITCH_H
#define NG_BANKSWITCH_H

#include "ng_defs.h"

/*
 * NeoGeo P-ROM bank switching API.
 *
 * Standard NeoGeo cartridges map the entire P-ROM into the 68000 address space
 * at once.  Cartridges larger than 512 KB require bank switching: a region of
 * the address space is remapped to a different physical ROM bank by writing the
 * bank number to a specific address.
 *
 * Bank switch register: 0x2FFFFE  (word write, lower byte = bank number)
 * Banked window:        0x200000 - 0x2FFFFF  (1 MB window per bank)
 * Fixed window:         0x000000 - 0x0FFFFF  (first 1 MB, always bank 0)
 *
 * The BIOS provides a soft helper at $C004A2 (home) / $C00402 (arcade) but
 * direct register writes are equally safe and faster.
 *
 * This SDK does not use bank switching in the current demo.  The P1 ROM is
 * kept within the fixed 128 KB window.  These stubs are provided so that
 * future cartridge builds with more than 512 KB of program space can adopt the
 * API without changes to call sites.
 *
 * How to use when you need it:
 *   1. Keep all code that must run during a bank switch in the fixed window.
 *   2. Place extra data/code in bank 1, 2, ... in the linker script.
 *   3. Before reading from the banked window, call ng_bankswitch(bank).
 *   4. Perform the read(s) from 0x200000 - 0x2FFFFF.
 *   5. Switch back if the code running after this point expects a known bank.
 *
 * Do NOT bank switch from interrupt context unless the interrupt handler saves
 * and restores the current bank number itself.
 */

#define NG_BANKSWITCH_REG  (*((volatile uint16_t *)0x2FFFFE))
#define NG_BANKSWITCH_WINDOW_BASE  0x200000UL
#define NG_BANKSWITCH_WINDOW_SIZE  0x100000UL

void ng_bankswitch(uint8_t bank);
uint8_t ng_bankswitch_current(void);

#endif
