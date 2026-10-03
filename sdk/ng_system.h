/*
 * The machine, its operator settings and the game's save block.
 *
 * Everything here is static inline: a game that never calls one of these
 * pays nothing for it, since only what a game uses is compiled into it.
 * Included by neogeo.h.
 */
#ifndef NG_SYSTEM_H
#define NG_SYSTEM_H

#include <stdint.h>
#include "macro.h"

/* ------------------------------------------------------------------ */
/*  The machine: arcade board or console, region, system ROM          */
/* ------------------------------------------------------------------ */
/* ng_sys_is_mvs() and ng_sys_region() live in sdk/cabinet (ng_sys.h):
 * 1 on an arcade board, 0 on a console -- the UniBIOS can switch this on
 * the fly, so read it at run time rather than trusting the build -- and
 * the region, with any unknown code read as the USA. */
#include "cabinet/ng_sys.h"

#define NG_REGION_JAPAN   NG_REGION_JP
#define NG_REGION_USA     NG_REGION_US
#define NG_REGION_EUROPE  NG_REGION_EU

/*
 * 1 when the system ROM is the UniBIOS. It names itself in its ROM
 * ("UNIVERSE BIOS"), but not at the same place in every version, so this
 * scans the system ROM. It takes a moment: ask once, at start-up, and keep
 * the answer.
 */
static inline uint8_t NEOGEO_USER ng_sys_is_unibios(void)
{
    const volatile uint8_t *rom = (const volatile uint8_t *)0xC00000u;
    uint32_t i;
    for (i = 0; i < 0x20000u - 16u; i++) {
        if (rom[i] != 'U' || rom[i + 1] != 'N' || rom[i + 2] != 'I' || rom[i + 3] != 'V') continue;
        if (rom[i + 4] == 'E' && rom[i + 5] == 'R' && rom[i + 6] == 'S' && rom[i + 7] == 'E' &&
            rom[i + 8] == ' ' && rom[i + 9] == 'B' && rom[i + 10] == 'I' && rom[i + 11] == 'O' &&
            rom[i + 12] == 'S') return 1u;
    }
    return 0u;
}

/* ------------------------------------------------------------------ */
/*  Software DIPs: the operator's settings for this game              */
/* ------------------------------------------------------------------ */
/*
 * The system copies them into BIOS_GAME_DIP before it enters USER, laid
 * out as the header's settings table: two time settings, two count
 * settings, then up to ten options. A console has no settings menu: there
 * these are always the cartridge's defaults.
 */

/* Time setting n (0..1): 0xMMSS in BCD, 0xFFFF when unused. */
static inline uint16_t NEOGEO_USER ng_dip_time(uint8_t n)
{
    const volatile uint8_t *dip = (const volatile uint8_t *)BIOS_GAME_DIP;
    if (n > 1u) return 0xFFFFu;
    return (uint16_t)(((uint16_t)dip[n * 2u] << 8) | dip[n * 2u + 1u]);
}

/* Count setting n (0..1): 0xFF when unused. */
static inline uint8_t NEOGEO_USER ng_dip_count(uint8_t n)
{
    const volatile uint8_t *dip = (const volatile uint8_t *)BIOS_GAME_DIP;
    return n > 1u ? (uint8_t)0xFFu : dip[4u + n];
}

/* Option n (0..9): the chosen entry, counted from 0. */
static inline uint8_t NEOGEO_USER ng_dip_option(uint8_t n)
{
    const volatile uint8_t *dip = (const volatile uint8_t *)BIOS_GAME_DIP;
    return n > 9u ? (uint8_t)0u : dip[6u + n];
}

/* ------------------------------------------------------------------ */
/*  The save block                                                    */
/* ------------------------------------------------------------------ */
/*
 * The cartridge header names a block of work RAM -- its start at 0x10E,
 * its size at 0x112, 4 KB at most -- that the system keeps for the game:
 * on an arcade board it is loaded from backup RAM before USER and written
 * back when the game returns to the system. (A console keeps it only in
 * work RAM, for the session.) The first two bytes are the system's own
 * debug DIPs and are left alone. After them comes a small header -- a tag,
 * the game's number, a layout version, the data size and a checksum -- so
 * a game can tell its own saved data from an empty or foreign block, and
 * then the game's data.
 */
#define NG_SAVE_HEADER  12u   /* debug DIPs 2, tag 4, NGH 2, version 1, pad 1, size 2 */
#define NG_SAVE_SUM      2u   /* then the checksum */

/*
 * The cartridge header at 0x100. Its address comes out of a one-line lea:
 * written as a constant pointer this low, GCC's bounds checks at -O2 take
 * every read of it for a null-pointer access and warn.
 */
static inline const volatile uint8_t *NEOGEO_USER ng_cart_header(void)
{
    const volatile uint8_t *header;
    __asm__ ("lea 0x100,%0" : "=a" (header));
    return header;
}

#define NG_CART_NGH()        (*(const volatile uint16_t *)(ng_cart_header() + 0x08))
#define NG_CART_SAVE_START() (*(const volatile uint32_t *)(ng_cart_header() + 0x0E))
#define NG_CART_SAVE_SIZE()  (*(const volatile uint16_t *)(ng_cart_header() + 0x12))

static inline volatile uint8_t *NEOGEO_USER ng_save_block(void)
{
    return (volatile uint8_t *)(uintptr_t)NG_CART_SAVE_START();
}

/* The game's data: after the header, word aligned when the block is. */
static inline void *NEOGEO_USER ng_save_data(void)
{
    return (void *)(ng_save_block() + NG_SAVE_HEADER + NG_SAVE_SUM);
}

/* Bytes of the game's data the declared block has room for. */
static inline uint16_t NEOGEO_USER ng_save_capacity(void)
{
    uint16_t size = NG_CART_SAVE_SIZE();
    return size > NG_SAVE_HEADER + NG_SAVE_SUM ? (uint16_t)(size - NG_SAVE_HEADER - NG_SAVE_SUM) : (uint16_t)0u;
}

/* Fletcher-16 over the game's data: catches a flipped bit and a shuffled
 * byte alike, and needs no table. Runs when the data is checked or sealed,
 * never per frame. ng_save_checksum_at() reads any copy of a block (a
 * memory card's, say); ng_save_checksum() the system's. */
static inline uint16_t NEOGEO_USER ng_save_checksum_at(const volatile uint8_t *block, uint16_t size)
{
    const volatile uint8_t *data = block + NG_SAVE_HEADER + NG_SAVE_SUM;
    uint16_t a = 0x5Au, b = 0xA5u, i;
    for (i = 0; i < size; i++) {
        a = (uint16_t)(a + data[i]);
        if (a >= 255u) a = (uint16_t)(a - 255u);
        b = (uint16_t)(b + a);
        if (b >= 255u) b = (uint16_t)(b - 255u);
    }
    return (uint16_t)((b << 8) | a);
}

static inline uint16_t NEOGEO_USER ng_save_checksum(uint16_t size)
{
    return ng_save_checksum_at(ng_save_block(), size);
}

/* 1 when a block holds this game's data at this layout version and size,
 * whole: ng_save_valid_at() for any copy, ng_save_valid() for the
 * system's. */
static inline uint8_t NEOGEO_USER ng_save_valid_at(const volatile uint8_t *h, uint8_t version, uint16_t size)
{
    uint16_t ngh = NG_CART_NGH();
    if (size > ng_save_capacity()) return 0u;
    if (h[2] != 'S' || h[3] != 'A' || h[4] != 'V' || h[5] != 'E') return 0u;
    if (h[6] != (uint8_t)(ngh >> 8) || h[7] != (uint8_t)ngh) return 0u;
    if (h[8] != version) return 0u;
    if (h[10] != (uint8_t)(size >> 8) || h[11] != (uint8_t)size) return 0u;
    return (uint8_t)((((uint16_t)h[12] << 8) | h[13]) == ng_save_checksum_at(h, size));
}

static inline uint8_t NEOGEO_USER ng_save_valid(uint8_t version, uint16_t size)
{
    return ng_save_valid_at(ng_save_block(), version, size);
}

/* Re-seals the block after the game's data has changed. */
static inline void NEOGEO_USER ng_save_commit(void)
{
    volatile uint8_t *h = ng_save_block();
    uint16_t size = (uint16_t)(((uint16_t)h[10] << 8) | h[11]);
    uint16_t sum;
    if (size > ng_save_capacity()) return;
    sum = ng_save_checksum(size);
    h[12] = (uint8_t)(sum >> 8);
    h[13] = (uint8_t)sum;
}

/* Starts the block afresh for this game: header written, data zeroed. */
static inline void NEOGEO_USER ng_save_format(uint8_t version, uint16_t size)
{
    volatile uint8_t *h = ng_save_block();
    uint16_t ngh = NG_CART_NGH();
    uint16_t i;
    if (size > ng_save_capacity()) size = ng_save_capacity();
    h[2] = 'S'; h[3] = 'A'; h[4] = 'V'; h[5] = 'E';
    h[6] = (uint8_t)(ngh >> 8); h[7] = (uint8_t)ngh;
    h[8] = version; h[9] = 0u;
    h[10] = (uint8_t)(size >> 8); h[11] = (uint8_t)size;
    for (i = 0; i < size; i++) h[NG_SAVE_HEADER + NG_SAVE_SUM + i] = 0u;
    ng_save_commit();
}

/* ------------------------------------------------------------------ */
/*  The memory card: a console's saves that outlast the session       */
/* ------------------------------------------------------------------ */
/*
 * A console keeps the save block only while it is on; a memory card keeps
 * it after. These go through the system ROM's CARD routine at $C00468
 * (neogeodev wiki: "CARD", "Memory card"). The game's NGH number and a
 * sub-number (0 here) name the card file. Its first 20 bytes are a title
 * that a console's card manager shows, and the card stores it in 64-byte
 * blocks.
 *
 * What goes on the card is the block's header and the game's data, as
 * ng_save_commit() sealed them, behind the title: for a 100-byte save, two
 * card blocks. The routine keeps no register, so the call saves them all.
 * An arcade board keeps the block in backup RAM instead: ask
 * ng_sys_is_mvs() first.
 */
#define NG_CARD_OK           0x00u
#define NG_CARD_NONE         0x80u   /* no card in the slot             */
#define NG_CARD_UNFORMATTED  0x81u
#define NG_CARD_NO_DATA      0x82u   /* no file of this game's          */
#define NG_CARD_FAT_ERROR    0x83u
#define NG_CARD_FULL         0x84u
#define NG_CARD_PROTECTED    0x85u   /* write-protected, or a ROM card  */
#define NG_CARD_TITLE        20u
#define NG_CARD_FORMAT       0u
#define NG_CARD_LOAD         2u
#define NG_CARD_SAVE         3u

/*
 * A byte copy kept in assembly: as a C loop between two buffers a fixed
 * distance apart, GCC at -O2 can fold it into "move.b (a0)+,(d,a0,d0)",
 * whose destination a 68000 works out with the already incremented a0 --
 * every byte lands one along (the load below did, in MAME).
 */
static inline void NEOGEO_USER ng_copy_bytes(volatile uint8_t *d, const volatile uint8_t *s, uint16_t n)
{
    if (!n) return;
    n = (uint16_t)(n - 1u);
    __asm__ volatile ("1:\n\t"
                      "move.b (%0)+,(%1)+\n\t"
                      "dbf %2,1b"
                      : "+a" (s), "+a" (d), "+d" (n)
                      :
                      : "memory");
}

static inline uint8_t NEOGEO_USER ng_card_call(uint8_t command, void *data, uint16_t size)
{
    *(volatile uint8_t *)BIOS_CRDF = command;
    *(volatile uint32_t *)BIOS_CRDPTR = (uint32_t)(uintptr_t)data;
    *(volatile uint16_t *)BIOS_CRDSIZE = size;
    *(volatile uint16_t *)BIOS_CRDNGH = NG_CART_NGH();
    *(volatile uint16_t *)BIOS_CRDFILE = 0u;
    __asm__ volatile ("movem.l %%d0-%%d7/%%a0-%%a6,-(%%sp)\n\t"
                      "jsr 0xC00468\n\t"
                      "movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a6"
                      : : : "cc", "memory");
    return *(volatile uint8_t *)BIOS_CRDRESULT;
}

/* The card file of a block holding `size` bytes of data, in whole 64-byte
 * card blocks: the system ROM keeps a file in blocks, and with a size that
 * isn't a whole number of them (MAME 0.264, the AES system ROM) it reads
 * back only the first. */
#define NG_CARD_FILE_SIZE(size) \
    ((NG_CARD_TITLE + NG_SAVE_HEADER + NG_SAVE_SUM + (size) + 63u) & ~63u)

/*
 * Writes the sealed block to the card behind `title` (padded with spaces
 * to 20). buf is the caller's room for the file, NG_CARD_FILE_SIZE(data
 * size) bytes at least. A card that says it is unformatted holds nothing,
 * and is formatted first. Returns the card's answer.
 */
static inline uint8_t NEOGEO_USER ng_card_save(const char *title, uint8_t *buf, uint16_t buf_size)
{
    const volatile uint8_t *h = ng_save_block();
    uint16_t size = (uint16_t)(((uint16_t)h[10] << 8) | h[11]);
    uint16_t n = (uint16_t)NG_CARD_FILE_SIZE(size), i;
    uint8_t r;
    uint16_t used = (uint16_t)(NG_SAVE_HEADER + NG_SAVE_SUM + size);
    if (size > ng_save_capacity() || n > buf_size) return NG_CARD_FULL;
    for (i = 0; i < NG_CARD_TITLE; i++) buf[i] = (uint8_t)((title && *title) ? *title++ : ' ');
    ng_copy_bytes(buf + NG_CARD_TITLE, h, used);
    for (i = (uint16_t)(NG_CARD_TITLE + used); i < n; i++) buf[i] = 0u;
    r = ng_card_call(NG_CARD_SAVE, buf, n);
    if (r == NG_CARD_UNFORMATTED && ng_card_call(NG_CARD_FORMAT, buf, 0u) == NG_CARD_OK)
        r = ng_card_call(NG_CARD_SAVE, buf, n);
    return r;
}

/*
 * Reads this game's file from the card into buf (NG_CARD_FILE_SIZE(size)
 * bytes at least) and, when it holds this game's data at this version and
 * size, whole, puts it in the save block (its first two bytes, the
 * system's debug switches, are left alone). Returns NG_CARD_OK when it
 * did; NG_CARD_NO_DATA when the file isn't usable; or the card's error.
 */
static inline uint8_t NEOGEO_USER ng_card_load(uint8_t version, uint16_t size, uint8_t *buf, uint16_t buf_size)
{
    volatile uint8_t *h = ng_save_block();
    uint16_t n = (uint16_t)NG_CARD_FILE_SIZE(size);
    uint8_t r;
    if (size > ng_save_capacity() || n > buf_size) return NG_CARD_NO_DATA;
    r = ng_card_call(NG_CARD_LOAD, buf, n);
    if (r != NG_CARD_OK) return r;
    if (!ng_save_valid_at(buf + NG_CARD_TITLE, version, size)) return NG_CARD_NO_DATA;
    ng_copy_bytes(h + 2, buf + NG_CARD_TITLE + 2, (uint16_t)(NG_SAVE_HEADER + NG_SAVE_SUM + size - 2u));
    return NG_CARD_OK;
}

#endif
