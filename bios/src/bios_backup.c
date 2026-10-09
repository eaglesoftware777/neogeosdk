#include "bios.h"

/*
 * The game's save block.
 *
 * The cartridge header names a block of work RAM -- its start at 0x10E, its
 * size at 0x112, 4 KB at most -- that the system keeps for the game. On an
 * arcade board it outlives the power: it is loaded from backup RAM before
 * the game is entered, and written back each time the game returns to the
 * system. That is where a game keeps its score table. A console has no
 * backup RAM; there the block simply stays in work RAM for the session.
 *
 * In backup RAM it lives at D01000: the tag 'E','G','S','V', the game's
 * NGH number, the block size, then the block. A different game, or the
 * same game with a different size, is not this game's data and is not
 * loaded -- the game is then asked to start its block afresh.
 */
#define BACKUP_STORE   ((volatile uint8_t *)0xD01000u)
#define BACKUP_HEAD    8u
#define WORK_RAM_LO    0x100000u
#define WORK_RAM_HI    0x10F300u   /* the system's own work area starts here */

/*
 * Byte copy, kept in assembly on purpose: GCC folds a C copy loop into a
 * post-increment form that a 68000 evaluates one entry late (see
 * bios_set_palette). `count` is at least 1.
 */
__attribute__((noinline))
static void copy_bytes(volatile uint8_t *dst, const volatile uint8_t *src, uint16_t count)
{
    count = (uint16_t)(count - 1u);
    asm volatile (
        "1:\n\t"
        "move.b (%0)+,(%1)+\n\t"
        "dbf %2,1b"
        : "+a" (src), "+a" (dst), "+d" (count)
        :
        : "memory");
}

static uint8_t block_bounds(uint32_t *start, uint16_t *size)
{
    uint32_t s = CART_HEADER->pbcks;
    uint16_t n = CART_HEADER->dipsize;
    if (n == 0 || n > 0x1000u) return 0;
    if (s < WORK_RAM_LO || s >= WORK_RAM_HI || n > WORK_RAM_HI - s) return 0;
    *start = s;
    *size = n;
    return 1;
}

/* Loads the game's block from backup RAM. Returns 1 if this game's data
 * was there and is now back in work RAM. */
uint8_t bios_backup_load(void)
{
    const volatile uint8_t *store = BACKUP_STORE;
    uint32_t start;
    uint16_t size;
    if (!BIOS_MVS_FLAG || !block_bounds(&start, &size)) return 0;
    if (store[0] != 'E' || store[1] != 'G' || store[2] != 'S' || store[3] != 'V') return 0;
    if ((uint16_t)((store[4] << 8) | store[5]) != CART_HEADER->ngh_id) return 0;
    if ((uint16_t)((store[6] << 8) | store[7]) != size) return 0;
    copy_bytes((volatile uint8_t *)(uintptr_t)start, store + BACKUP_HEAD, size);
    return 1;
}

/* Writes the game's block back to backup RAM (arcade boards only). */
void bios_backup_save(void)
{
    volatile uint8_t *store = BACKUP_STORE;
    uint32_t start;
    uint16_t size;
    if (!BIOS_MVS_FLAG || !block_bounds(&start, &size)) return;
    REG_SRAMUNLOCK = 0;
    store[0] = 'E'; store[1] = 'G'; store[2] = 'S'; store[3] = 'V';
    store[4] = (uint8_t)(CART_HEADER->ngh_id >> 8);
    store[5] = (uint8_t)CART_HEADER->ngh_id;
    store[6] = (uint8_t)(size >> 8);
    store[7] = (uint8_t)size;
    copy_bytes(store + BACKUP_HEAD, (const volatile uint8_t *)(uintptr_t)start, size);
    REG_SRAMLOCK = 0;
}
