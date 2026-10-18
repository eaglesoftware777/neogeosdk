#ifndef SLF_ROM_H
#define SLF_ROM_H
#include <stdint.h>
#include "sdk/macro.h"
#define SLF_ROM_FIXED_BYTES 0x100000u
#define SLF_ROM_BANK_BYTES  0x100000u
#define SLF_ROM_BANK_COUNT 2u
#define SLF_ROM_BYTES      0x300000u
/* Physical cartridge offsets, never bank-window pointers. Runtime code,
 * interrupt handlers and deferred VRAM jobs stay in the fixed P region.
 * Copies may span banks. The window is returned to bank zero afterwards. */
uint8_t NEOGEO_USER slf_rom_read(void *destination, uint32_t offset, uint16_t count);
extern uint16_t slf_rom_copies, slf_rom_errors;
extern uint8_t slf_rom_banks_seen;
#endif
