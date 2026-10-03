#include "bios.h"

/*
 * ============================================================================
 *  EagleBIOS Memory Card Sub-system
 * ============================================================================
 *
 * SYS_CARD keeps a card in the layout the console system ROMs use, so a card
 * moves between them and this firmware with its saves intact (neogeodev
 * wiki: "Memory card", "CARD"). Offsets are card bytes; an 8-bit card, the
 * SNK one included, puts byte n in the low byte of the word at
 * $800000 + 2n, and is only ever accessed in words.
 *
 *   block 0       header: card size (in address space) at $0A, the two FAT
 *                 checksums at $0D/$0E, the user name flag at $0F and name
 *                 at $10, "NEO-GEO" $80 on the even bytes from $20, and the
 *                 formatting system's region at $30
 *   directory     one 4-byte entry a block: sub-number ($FF: free), NGH
 *                 (big-endian), first block
 *   FAT 1, FAT 2  one byte a block (at least 64): 0 free, 1 a file's last
 *                 block, 2 the system's, else the next block of the file;
 *                 each checksum is the low byte of its sum
 *   data          64-byte blocks
 *
 * A 2 KiB card: header, directory in blocks 1-2, FATs in 3 and 4, files
 * from block 5. Commands and answers are the system ROM's (BIOS_CRDF,
 * BIOS_CRDRESULT): 0 format, 1 search, 2 load, 3 save, 4 delete, 5 a
 * file's title, 6/7 the user name. Only 8-bit cards are handled.
 */

#define CARD_OK          0x00u
#define CARD_NONE        0x80u
#define CARD_UNFORMATTED 0x81u
#define CARD_NO_DATA     0x82u
#define CARD_FAT_ERROR   0x83u
#define CARD_FULL        0x84u
#define CARD_PROTECTED   0x85u

#define CARD_WORD(n)     (((volatile uint16_t *)ADDR_MEMCARD)[n])
#define CARD_BLOCK       64u
#define CARD_MIN         0x0800u   /* bytes: 2 KiB */
#define CARD_MAX         0x4000u   /* bytes: 16 KiB */
#define CARD_SUB         (*(volatile uint8_t *)0x10FDD0u)

static uint16_t card_blocks;       /* blocks on the card                  */
static uint16_t card_dir;          /* offset of the directory             */
static uint16_t card_fat[2];       /* offsets of the two FATs             */
static uint16_t card_fat_size;     /* bytes in each FAT                   */
static uint16_t card_data;         /* first data block                    */

static uint8_t rd(uint16_t n)
{
    return (uint8_t)CARD_WORD(n);
}

static void wr(uint16_t n, uint8_t value)
{
    CARD_WORD(n) = value;
}

static void geometry(uint16_t bytes)
{
    uint16_t dir_blocks, fat_blocks;
    card_blocks = (uint16_t)(bytes / CARD_BLOCK);
    dir_blocks = (uint16_t)(card_blocks / 16u);          /* 4 bytes an entry */
    card_fat_size = card_blocks < 64u ? 64u : card_blocks;
    fat_blocks = (uint16_t)(card_fat_size / CARD_BLOCK);
    card_dir = CARD_BLOCK;
    card_fat[0] = (uint16_t)((1u + dir_blocks) * CARD_BLOCK);
    card_fat[1] = (uint16_t)(card_fat[0] + card_fat_size);
    card_data = (uint16_t)(1u + dir_blocks + 2u * fat_blocks);
}

static uint8_t fat_sum(uint8_t which)
{
    uint8_t sum = 0;
    for (uint16_t i = 0; i < card_fat_size; i++) sum = (uint8_t)(sum + rd((uint16_t)(card_fat[which] + i)));
    return sum;
}

static uint8_t fat(uint16_t block)
{
    return rd((uint16_t)(card_fat[0] + block));
}

static void set_fat(uint16_t block, uint8_t value)
{
    wr((uint16_t)(card_fat[0] + block), value);
}

/* FAT 2 becomes FAT 1 again, and both checksums are written. */
static void fat_seal(void)
{
    for (uint16_t i = 0; i < card_fat_size; i++)
        wr((uint16_t)(card_fat[1] + i), rd((uint16_t)(card_fat[0] + i)));
    uint8_t sum = fat_sum(0);
    wr(0x0Du, sum);
    wr(0x0Eu, sum);
}

/* The card's memory, not its attribute registers, answers reads only once
 * the system has chosen it (REG_CRDNORMAL); writes need both unlocks. */
static void unlock(void)
{
    REG_CRDNORMAL = 0;
    REG_CRDUNLOCK1 = 0;
    REG_CRDUNLOCK2 = 0;
}

static void lock(void)
{
    REG_CRDLOCK1 = 0;
    REG_CRDLOCK2 = 0;
}

static uint8_t is_formatted(void)
{
    static const uint8_t magic[8] = { 'N', 'E', 'O', '-', 'G', 'E', 'O', 0x80 };
    for (uint8_t i = 0; i < 8u; i++)
        if (rd((uint16_t)(0x20u + 2u * i)) != magic[i]) return 0;
    return 1;
}

/* Reads the layout from the header, and mends a FAT whose checksum is wrong
 * from the other one (when the card may be written). */
static uint8_t mount(uint8_t writable)
{
    uint16_t size;
    uint8_t good0, good1;
    if (!is_formatted()) return CARD_UNFORMATTED;
    size = (uint16_t)(((uint16_t)rd(0x0Au) << 8) | rd(0x0Bu));
    size >>= 1;                                    /* 8-bit: half the space */
    if (size < CARD_MIN || size > CARD_MAX || (size & (CARD_MIN - 1u))) return CARD_FAT_ERROR;
    geometry(size);
    good0 = (uint8_t)(fat_sum(0) == rd(0x0Du));
    good1 = (uint8_t)(fat_sum(1) == rd(0x0Eu));
    if (!good0 && !good1) return CARD_FAT_ERROR;
    if (!good0 || !good1) {
        if (!writable) {
            if (!good0) card_fat[0] = card_fat[1];  /* read from the good one */
            return CARD_OK;
        }
        if (!good0)
            for (uint16_t i = 0; i < card_fat_size; i++)
                wr((uint16_t)(card_fat[0] + i), rd((uint16_t)(card_fat[1] + i)));
        fat_seal();
    }
    return CARD_OK;
}

/* The directory entry of BIOS_CRDNGH / sub, or 0xFFFF. */
static uint16_t find(uint8_t sub)
{
    uint16_t ngh = BIOS_CRDNGH;
    for (uint16_t e = 0; e < card_blocks; e++) {
        uint16_t at = (uint16_t)(card_dir + 4u * e);
        if (rd(at) == sub && rd((uint16_t)(at + 1u)) == (uint8_t)(ngh >> 8) && rd((uint16_t)(at + 2u)) == (uint8_t)ngh)
            return at;
    }
    return 0xFFFFu;
}

static uint8_t valid_block(uint8_t block)
{
    return (uint8_t)(block >= card_data && block < card_blocks);
}

/* Frees a file's chain; returns how many blocks it held (0: a broken one). */
static uint16_t free_chain(uint8_t block)
{
    uint16_t count = 0;
    while (valid_block(block) && count < card_blocks) {
        uint8_t next = fat(block);
        set_fat(block, 0);
        count++;
        if (next == 1u) break;
        block = next;
    }
    return count;
}

static uint16_t chain_length(uint8_t block)
{
    uint16_t count = 0;
    while (valid_block(block) && count < card_blocks) {
        uint8_t next = fat(block);
        count++;
        if (next == 1u) return count;
        block = next;
    }
    return 0;
}

static uint8_t card_format(void)
{
    uint16_t size = CARD_MIN, i;
    /* The size: a write past the end either reads back wrong or lands on
     * the start again. */
    while (size < CARD_MAX) {
        wr(0, 0x00);
        wr(size, 0x5A);
        if (rd(size) != 0x5A || rd(0) == 0x5A) break;
        wr(size, 0xA5);
        if (rd(size) != 0xA5 || rd(0) == 0xA5) break;
        size = (uint16_t)(size + CARD_MIN);
    }
    geometry(size);
    for (i = 0; i < CARD_BLOCK; i++) wr(i, 0);
    wr(0x0Au, (uint8_t)((size << 1) >> 8));
    wr(0x0Bu, (uint8_t)(size << 1));
    for (i = 0; i < 16u; i++) wr((uint16_t)(0x10u + i), ' ');
    {
        static const uint8_t magic[8] = { 'N', 'E', 'O', '-', 'G', 'E', 'O', 0x80 };
        for (i = 0; i < 8u; i++) wr((uint16_t)(0x20u + 2u * i), magic[i]);
    }
    wr(0x30u, BIOS_COUNTRY_CODE);
    for (i = 0; i < 4u * card_blocks; i++) wr((uint16_t)(card_dir + i), 0xFF);
    for (i = 0; i < card_fat_size; i++) set_fat(i, (uint8_t)(i < card_data ? 2u : 0u));
    fat_seal();
    return CARD_OK;
}

static uint8_t card_search(void)
{
    uint16_t mask = 0, ngh = BIOS_CRDNGH;
    for (uint16_t e = 0; e < card_blocks; e++) {
        uint16_t at = (uint16_t)(card_dir + 4u * e);
        uint8_t sub = rd(at);
        if (sub < 16u && rd((uint16_t)(at + 1u)) == (uint8_t)(ngh >> 8) && rd((uint16_t)(at + 2u)) == (uint8_t)ngh)
            mask |= (uint16_t)(1u << sub);
    }
    BIOS_CRDFILE = mask;
    return CARD_OK;
}

/* Copies up to `size` bytes of a file into RAM, block by block. */
static uint8_t card_read_file(uint8_t block, volatile uint8_t *dest, uint16_t size)
{
    uint16_t done = 0, hops = 0;
    while (done < size) {
        uint16_t base, n;
        if (!valid_block(block) || hops++ >= card_blocks) return CARD_FAT_ERROR;
        base = (uint16_t)(block * CARD_BLOCK);
        n = (uint16_t)(size - done);
        if (n > CARD_BLOCK) n = CARD_BLOCK;
        for (uint16_t i = 0; i < n; i++) dest[done + i] = rd((uint16_t)(base + i));
        done = (uint16_t)(done + n);
        if (fat(block) == 1u) break;
        block = fat(block);
    }
    return CARD_OK;
}

static uint8_t card_load(void)
{
    uint16_t at = find(CARD_SUB);
    if (at == 0xFFFFu) return CARD_NO_DATA;
    return card_read_file(rd((uint16_t)(at + 3u)), (volatile uint8_t *)BIOS_CRDPTR, BIOS_CRDSIZE);
}

static uint8_t card_title(void)
{
    uint16_t at = find(CARD_SUB);
    if (at == 0xFFFFu) return CARD_NO_DATA;
    return card_read_file(rd((uint16_t)(at + 3u)), (volatile uint8_t *)BIOS_CRDPTR, 20u);
}

static uint8_t card_save(void)
{
    const volatile uint8_t *src = (const volatile uint8_t *)BIOS_CRDPTR;
    uint16_t size = BIOS_CRDSIZE, need, room = 0, at, done = 0, i;
    uint8_t sub = CARD_SUB, prev = 0, first = 0;
    need = (uint16_t)((size + CARD_BLOCK - 1u) / CARD_BLOCK);
    if (!need) need = 1;
    at = find(sub);
    if (at != 0xFFFFu) room = chain_length(rd((uint16_t)(at + 3u)));
    for (i = card_data; i < card_blocks; i++) if (!fat(i)) room++;
    if (need > room) return CARD_FULL;
    if (at == 0xFFFFu) {
        for (i = 0; i < card_blocks && at == 0xFFFFu; i++)
            if (rd((uint16_t)(card_dir + 4u * i)) == 0xFFu) at = (uint16_t)(card_dir + 4u * i);
        if (at == 0xFFFFu) return CARD_FULL;
    } else {
        free_chain(rd((uint16_t)(at + 3u)));
    }
    for (i = card_data; i < card_blocks && need; i++) {
        uint16_t base = (uint16_t)(i * CARD_BLOCK);
        if (fat(i)) continue;
        for (uint16_t k = 0; k < CARD_BLOCK; k++, done++)
            wr((uint16_t)(base + k), done < size ? src[done] : 0u);
        if (prev) set_fat(prev, (uint8_t)i); else first = (uint8_t)i;
        set_fat(i, 1u);
        prev = (uint8_t)i;
        need--;
    }
    fat_seal();
    wr(at, sub);
    wr((uint16_t)(at + 1u), (uint8_t)(BIOS_CRDNGH >> 8));
    wr((uint16_t)(at + 2u), (uint8_t)BIOS_CRDNGH);
    wr((uint16_t)(at + 3u), first);
    return CARD_OK;
}

static uint8_t card_delete(void)
{
    uint16_t at = find(CARD_SUB);
    if (at == 0xFFFFu) return CARD_NO_DATA;
    free_chain(rd((uint16_t)(at + 3u)));
    fat_seal();
    wr(at, 0xFF);
    return CARD_OK;
}

static uint8_t card_name_save(void)
{
    const volatile uint8_t *src = (const volatile uint8_t *)BIOS_CRDPTR;
    for (uint16_t i = 0; i < 16u; i++) wr((uint16_t)(0x10u + i), src[i]);
    wr(0x0Fu, 1);
    return CARD_OK;
}

static uint8_t card_name_load(void)
{
    volatile uint8_t *dest = (volatile uint8_t *)BIOS_CRDPTR;
    if (!rd(0x0Fu)) return CARD_NO_DATA;
    for (uint16_t i = 0; i < 16u; i++) dest[i] = rd((uint16_t)(0x10u + i));
    return CARD_OK;
}

/*
 * SYS_CARD: the command in BIOS_CRDF, the answer in BIOS_CRDRESULT.
 */
void sys_card_c(void)
{
    uint8_t command = BIOS_CRDF, writes, r;
    /* Both card-detect lines low: a card is in. */
    if (REG_STATUS_B & 0x30u) { BIOS_CRDRESULT = CARD_NONE; return; }
    writes = (uint8_t)(command == 0u || command == 3u || command == 4u || command == 6u);
    if (writes && (REG_STATUS_B & 0x40u)) { BIOS_CRDRESULT = CARD_PROTECTED; return; }
    unlock();
    if (command == 0u) {
        r = card_format();
    } else if ((r = mount(writes)) == CARD_OK) {
        switch (command) {
        case 1: r = card_search(); break;
        case 2: r = card_load(); break;
        case 3: r = card_save(); break;
        case 4: r = card_delete(); break;
        case 5: r = card_title(); break;
        case 6: r = card_name_save(); break;
        case 7: r = card_name_load(); break;
        default: r = CARD_NO_DATA; break;
        }
    }
    lock();
    BIOS_CRDRESULT = r;
}

/*
 * SYS_CARD_ERROR: Display memory card error message.
 */
void sys_card_error_c(void)
{
    /* Quick return */
}
