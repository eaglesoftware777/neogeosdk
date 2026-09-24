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
#define NG_REGION_JAPAN   0
#define NG_REGION_USA     1
#define NG_REGION_EUROPE  2

/* 1 on an arcade board (MVS), 0 on a console. The UniBIOS can switch this
 * on the fly, so read it at run time rather than trusting the build. */
static inline uint8_t NEOGEO_USER ng_sys_is_mvs(void)
{
    return (*(volatile uint8_t *)BIOS_MVS_FLAG) ? 1u : 0u;
}

/* NG_REGION_JAPAN, NG_REGION_USA or NG_REGION_EUROPE. */
static inline uint8_t NEOGEO_USER ng_sys_region(void)
{
    uint8_t region = *(volatile uint8_t *)BIOS_COUNTRY_CODE;
    return region > NG_REGION_EUROPE ? (uint8_t)NG_REGION_EUROPE : region;
}

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
 * never per frame. */
static inline uint16_t NEOGEO_USER ng_save_checksum(uint16_t size)
{
    const volatile uint8_t *data = ng_save_block() + NG_SAVE_HEADER + NG_SAVE_SUM;
    uint16_t a = 0x5Au, b = 0xA5u, i;
    for (i = 0; i < size; i++) {
        a = (uint16_t)(a + data[i]);
        if (a >= 255u) a = (uint16_t)(a - 255u);
        b = (uint16_t)(b + a);
        if (b >= 255u) b = (uint16_t)(b - 255u);
    }
    return (uint16_t)((b << 8) | a);
}

/* 1 when the block holds this game's data at this layout version and size,
 * whole. */
static inline uint8_t NEOGEO_USER ng_save_valid(uint8_t version, uint16_t size)
{
    const volatile uint8_t *h = ng_save_block();
    uint16_t ngh = NG_CART_NGH();
    if (size > ng_save_capacity()) return 0u;
    if (h[2] != 'S' || h[3] != 'A' || h[4] != 'V' || h[5] != 'E') return 0u;
    if (h[6] != (uint8_t)(ngh >> 8) || h[7] != (uint8_t)ngh) return 0u;
    if (h[8] != version) return 0u;
    if (h[10] != (uint8_t)(size >> 8) || h[11] != (uint8_t)size) return 0u;
    return (uint8_t)((((uint16_t)h[12] << 8) | h[13]) == ng_save_checksum(size));
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

#endif
