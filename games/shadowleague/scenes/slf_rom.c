#include "slf_rom.h"
#ifndef SLF_ROM_HOST
#include "sdk/neogeo.h"
#endif
uint16_t slf_rom_copies, slf_rom_errors;
uint8_t slf_rom_banks_seen;
#ifdef SLF_ROM_HOST
extern void slf_rom_test_select(uint8_t bank);
extern uint8_t slf_rom_test_byte(uint32_t mapped_address);
#define SELECT_BANK(b) slf_rom_test_select(b)
#define READ_BYTE(a) slf_rom_test_byte(a)
#else
#define SELECT_BANK(b) (NEO_REGISTER(0x2FFFF0u) = (uint16_t)(b))
#define READ_BYTE(a) (*(volatile const uint8_t *)(a))
#endif

uint8_t NEOGEO_USER slf_rom_read(void *destination, uint32_t offset, uint16_t count)
{
    uint8_t *out = (uint8_t *)destination;
    uint8_t switched = 0u;
    if ((!out && count) || offset > SLF_ROM_BYTES || count > SLF_ROM_BYTES - offset) {
        slf_rom_errors++; return 0u;
    }
    while (count) {
        uint32_t in_bank = offset & 0xFFFFFu;
        uint32_t remaining = SLF_ROM_BANK_BYTES - in_bank;
        uint16_t part = remaining < count ? (uint16_t)remaining : count;
        uint32_t mapped = offset;
        uint16_t i;
        if (offset >= SLF_ROM_FIXED_BYTES) {
            uint8_t bank = (uint8_t)((offset >> 20) - 1u);
            SELECT_BANK(bank); switched = 1u;
            slf_rom_banks_seen |= (uint8_t)(1u << bank);
            mapped = 0x200000u + in_bank;
        }
        for (i = 0u; i < part; i++) out[i] = READ_BYTE(mapped + i);
        out += part; offset += part; count = (uint16_t)(count - part);
    }
    if (switched) SELECT_BANK(0u);
    slf_rom_copies++;
    return 1u;
}
