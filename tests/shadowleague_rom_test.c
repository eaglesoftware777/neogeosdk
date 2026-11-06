#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "games/shadowleague/scenes/slf_rom.h"
static uint8_t bank;
static unsigned selects;
static uint8_t source(uint32_t offset) { return (uint8_t)((offset >> 20) * 71u + (offset & 63u)); }
void slf_rom_test_select(uint8_t b) { assert(b < 2u); bank=b; selects++; }
uint8_t slf_rom_test_byte(uint32_t address)
{
    if (address < 0x100000u) return source(address);
    assert(address >= 0x200000u && address <= 0x2FFFFFu);
    return source((uint32_t)(bank+1u)*0x100000u+address-0x200000u);
}
int main(void)
{
    uint8_t bytes[1024]; uint16_t i;
    uint32_t starts[]={0u,0xFFFF0u,0x100000u,0x1FFFF0u,0x200000u,0x2FFC00u};
    for (i=0u;i<sizeof(starts)/sizeof(starts[0]);i++) {
        uint16_t n;
        assert(slf_rom_read(bytes,starts[i],sizeof(bytes)));
        for (n=0u;n<sizeof(bytes);n++) assert(bytes[n]==source(starts[i]+n));
        assert(bank==0u);
    }
    assert(slf_rom_banks_seen==3u && slf_rom_copies==6u && selects>=10u);
    memset(bytes,0xA5,sizeof(bytes));
    assert(!slf_rom_read(bytes,0x2FFFFFu,2u));
    assert(!slf_rom_read(bytes,0xFFFFFFFEu,4u));
    assert(!slf_rom_read(0,0u,1u));
    assert(slf_rom_read(0,SLF_ROM_BYTES,0u));
    assert(slf_rom_errors==3u);
    for (i=0u;i<sizeof(bytes);i++) assert(bytes[i]==0xA5u);
    puts("P-ROM fixed/bank reads, boundary copies, restore and bounds: PASS");
    return 0;
}
