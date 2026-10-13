#include "bios.h"

/*
 * The game's save block.
 *
 * In backup RAM it lives at D01000:
 * Format v2 ('E','G','S','2'):
 *   0..3: 'E','G','S','2'
 *   4..5: NGH id (big-endian)
 *   6..7: block size
 *   8..9: checksum (16-bit)
 *   10..: payload
 * Format v1 ('E','G','S','V'):
 *   0..3: 'E','G','S','V'
 *   4..5: NGH id
 *   6..7: block size
 *   8..:  payload (migrated to v2 on save)
 */
#define BACKUP_STORE   ((volatile uint8_t *)0xD01000u)
#define BACKUP_HEAD_V1 8u
#define BACKUP_HEAD_V2 10u
#define WORK_RAM_LO    0x100000u
#define WORK_RAM_HI    0x10F300u   /* the system's own work area starts here */

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

static uint16_t calc_checksum(const volatile uint8_t *data, uint16_t len)
{
    uint16_t sum = 0xAA55u;
    for (uint16_t i = 0u; i < len; i++) {
        sum = (uint16_t)(sum + data[i] + (i << 1));
    }
    return sum;
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
 * was there, passed checksum, and is now back in work RAM. */
uint8_t bios_backup_load(void)
{
    const volatile uint8_t *store = BACKUP_STORE;
    uint32_t start;
    uint16_t size;
    if (!BIOS_MVS_FLAG || !block_bounds(&start, &size)) return 0;

    /* Check Format v2 */
    if (store[0] == 'E' && store[1] == 'G' && store[2] == 'S' && store[3] == '2') {
        if ((uint16_t)((store[4] << 8) | store[5]) != CART_HEADER->ngh_id) return 0;
        if ((uint16_t)((store[6] << 8) | store[7]) != size) return 0;
        uint16_t expected_sum = (uint16_t)((store[8] << 8) | store[9]);
        uint16_t actual_sum = calc_checksum(store + BACKUP_HEAD_V2, size);
        if (expected_sum != actual_sum) return 0; /* Corrupted payload */
        copy_bytes((volatile uint8_t *)(uintptr_t)start, store + BACKUP_HEAD_V2, size);
        return 1;
    }

    /* Check Format v1 (legacy migration) */
    if (store[0] == 'E' && store[1] == 'G' && store[2] == 'S' && store[3] == 'V') {
        if ((uint16_t)((store[4] << 8) | store[5]) != CART_HEADER->ngh_id) return 0;
        if ((uint16_t)((store[6] << 8) | store[7]) != size) return 0;
        copy_bytes((volatile uint8_t *)(uintptr_t)start, store + BACKUP_HEAD_V1, size);
        return 1;
    }

    return 0;
}

/* Writes the game's block back to backup RAM with checksum (arcade boards only). */
void bios_backup_save(void)
{
    volatile uint8_t *store = BACKUP_STORE;
    uint32_t start;
    uint16_t size;
    if (!BIOS_MVS_FLAG || !block_bounds(&start, &size)) return;
    REG_SRAMUNLOCK = 0;
    store[0] = 'E'; store[1] = 'G'; store[2] = 'S'; store[3] = '2';
    store[4] = (uint8_t)(CART_HEADER->ngh_id >> 8);
    store[5] = (uint8_t)CART_HEADER->ngh_id;
    store[6] = (uint8_t)(size >> 8);
    store[7] = (uint8_t)size;
    copy_bytes(store + BACKUP_HEAD_V2, (const volatile uint8_t *)(uintptr_t)start, size);
    uint16_t sum = calc_checksum(store + BACKUP_HEAD_V2, size);
    store[8] = (uint8_t)(sum >> 8);
    store[9] = (uint8_t)sum;
    REG_SRAMLOCK = 0;
}

/* Clear backup RAM area */
void bios_backup_clear(void)
{
    if (!BIOS_MVS_FLAG) return;
    REG_SRAMUNLOCK = 0;
    volatile uint8_t *s = (volatile uint8_t *)0xD00000u;
    for (uint32_t i = 0; i < 0x10000u; i++) {
        s[i] = 0;
    }
    REG_SRAMLOCK = 0;
}
