#ifndef EAGLE_BIOS_H
#define EAGLE_BIOS_H

#include <stdint.h>
#include <stddef.h>

/*
 * ============================================================================
 *  EagleBIOS - Open Source Neo Geo Compatible System BIOS
 *  Designed for Neo Geo MVS (Arcade) and AES (Home Console) Hardware
 * ============================================================================
 */

/* Memory Addresses */
#define ADDR_SYSROM         0xC00000u
#define ADDR_USER_RAM       0x100000u
#define ADDR_BIOS_RAM       0x10F300u
#define ADDR_PALETTES       0x400000u
#define ADDR_BACKDROP       0x401FFEu
#define ADDR_MEMCARD        0x800000u

/* Hardware I/O Registers */
#define REG_P1CNT           (*(volatile uint8_t  *)0x300000u)
#define REG_DIPSW           (*(volatile uint8_t  *)0x300001u)  /* Also watchdog kick */
#define REG_SYSTYPE         (*(volatile uint8_t  *)0x300081u)
#define REG_SOUND           (*(volatile uint8_t  *)0x320000u)  /* Sound code to Z80 */
#define REG_STATUS_A        (*(volatile uint8_t  *)0x320001u)  /* Coins, Service, Test */
#define REG_P2CNT           (*(volatile uint8_t  *)0x340000u)
#define REG_STATUS_B        (*(volatile uint8_t  *)0x380000u)  /* Start 1/2, Select 1/2 */
#define REG_POUTPUT         (*(volatile uint8_t  *)0x380001u)
#define REG_SLOT            (*(volatile uint8_t  *)0x380021u)

/* Video Control Registers (Strobe registers: write any value to trigger) */
#define REG_NOSHADOW        (*(volatile uint8_t  *)0x3A0001u)  /* Normal video output */
#define REG_SHADOW          (*(volatile uint8_t  *)0x3A0011u)  /* Darkened video output */
#define REG_SWPBIOS         (*(volatile uint8_t  *)0x3A0003u)  /* BIOS vector table at 0x00 */
#define REG_SWPROM          (*(volatile uint8_t  *)0x3A0013u)  /* Cartridge vector table at 0x00 */
#define REG_BRDFIX          (*(volatile uint8_t  *)0x3A000Bu)  /* System board S-ROM (SFIX) & SM1 */
#define REG_CRTFIX          (*(volatile uint8_t  *)0x3A001Bu)  /* Cartridge S-ROM & M1 */
#define REG_SRAMLOCK        (*(volatile uint8_t  *)0x3A000Du)  /* Lock backup RAM */
#define REG_SRAMUNLOCK      (*(volatile uint8_t  *)0x3A001Du)  /* Unlock backup RAM */
#define REG_PALBANK0        (*(volatile uint8_t  *)0x3A000Fu)  /* Palette bank 0 */
#define REG_PALBANK1        (*(volatile uint8_t  *)0x3A001Fu)  /* Palette bank 1 */

/* VRAM Registers */
#define REG_VRAM_ADDR       (*(volatile uint16_t *)0x3C0000u)
#define REG_VRAM_RW         (*(volatile uint16_t *)0x3C0002u)
#define REG_VRAM_INC        (*(volatile uint16_t *)0x3C0004u)
#define REG_LSPCMODE        (*(volatile uint16_t *)0x3C0006u)
#define REG_TIMERHIGH       (*(volatile uint16_t *)0x3C0008u)
#define REG_TIMERLOW        (*(volatile uint16_t *)0x3C000Au)
#define REG_IRQACK          (*(volatile uint16_t *)0x3C000Cu)
#define REG_TIMERSTOP       (*(volatile uint16_t *)0x3C000Eu)

/* BIOS Work RAM Locations (0x10FD80 .. 0x10FFFF) */
#define BIOS_SYSTEM_MODE    (*(volatile uint8_t  *)0x10FD80u)
#define BIOS_MVS_FLAG       (*(volatile uint8_t  *)0x10FD82u)  /* 1 = MVS Arcade, 0 = AES Home */
#define BIOS_COUNTRY_CODE   (*(volatile uint8_t  *)0x10FD83u)  /* 0 = Japan, 1 = USA, 2 = Europe */
#define BIOS_GAME_DIP       ((volatile uint8_t   *)0x10FD84u)  /* 16 soft dip bytes (10FD84..10FD93) */

/* Controller status updated by SYS_IO */
#define BIOS_P1STATUS       (*(volatile uint8_t  *)0x10FD94u)
#define BIOS_P1PREVIOUS     (*(volatile uint8_t  *)0x10FD95u)
#define BIOS_P1CURRENT      (*(volatile uint8_t  *)0x10FD96u)
#define BIOS_P1CHANGE       (*(volatile uint8_t  *)0x10FD97u)
#define BIOS_P1REPEAT       (*(volatile uint8_t  *)0x10FD98u)
#define BIOS_P1TIMER        (*(volatile uint8_t  *)0x10FD99u)

#define BIOS_P2STATUS       (*(volatile uint8_t  *)0x10FD9Au)
#define BIOS_P2PREVIOUS     (*(volatile uint8_t  *)0x10FD9Bu)
#define BIOS_P2CURRENT      (*(volatile uint8_t  *)0x10FD9Cu)
#define BIOS_P2CHANGE       (*(volatile uint8_t  *)0x10FD9Du)
#define BIOS_P2REPEAT       (*(volatile uint8_t  *)0x10FD9Eu)
#define BIOS_P2TIMER        (*(volatile uint8_t  *)0x10FD9Fu)
#define BIOS_CREDIT_DEC     ((volatile uint8_t *)0x10FDB0u)
#define BIOS_FRAME_COUNTER  (*(volatile uint32_t *)0x10FE88u)

#define BIOS_STATCURNT      (*(volatile uint8_t  *)0x10FDACu)
#define BIOS_STATCHANGE     (*(volatile uint8_t  *)0x10FDADu)

#define BIOS_USER_REQUEST   (*(volatile uint8_t  *)0x10FDAEu)  /* 0=POWER_ON, 1=EYE_CATCHER, 2=GAME, 3=TITLE */
#define BIOS_USER_MODE      (*(volatile uint8_t  *)0x10FDAFu)  /* 0=Init, 1=Attract, 2=Game */
#define BIOS_START_FLAG     (*(volatile uint8_t  *)0x10FDB4u)  /* Bit 0 = P1 started, Bit 1 = P2 started */
#define BIOS_PLAYER1_MODE   (*(volatile uint8_t  *)0x10FDB6u)
#define BIOS_PLAYER2_MODE   (*(volatile uint8_t  *)0x10FDB7u)

#define BIOS_MESS_POINT     (*(volatile uint32_t *)0x10FDBEu)
#define BIOS_MESS_BUSY      (*(volatile uint8_t  *)0x10FDC2u)
#define BIOS_MESS_BUFFER    ((volatile uint16_t  *)0x10FF00u)

/* Memory Card */
#define BIOS_CRDF           (*(volatile uint8_t  *)0x10FDC4u)
#define BIOS_CRDRESULT      (*(volatile uint8_t  *)0x10FDC6u)
#define BIOS_CRDPTR         (*(volatile uint32_t *)0x10FDC8u)
#define BIOS_CRDSIZE        (*(volatile uint16_t *)0x10FDCCu)
#define BIOS_CRDNGH         (*(volatile uint16_t *)0x10FDCEu)
#define BIOS_CRDFILE        (*(volatile uint16_t *)0x10FDD0u)

/* Calendar (BCD) */
#define BIOS_YEAR           (*(volatile uint8_t  *)0x10FDD2u)
#define BIOS_MONTH          (*(volatile uint8_t  *)0x10FDD3u)
#define BIOS_DAY            (*(volatile uint8_t  *)0x10FDD4u)
#define BIOS_WEEKDAY        (*(volatile uint8_t  *)0x10FDD5u)
#define BIOS_HOUR           (*(volatile uint8_t  *)0x10FDD6u)
#define BIOS_MINUTE         (*(volatile uint8_t  *)0x10FDD7u)
#define BIOS_SECOND         (*(volatile uint8_t  *)0x10FDD8u)
#define BIOS_SELECT_TIMER   (*(volatile uint8_t  *)0x10FDDAu)

#define BIOS_VBL_TICK       (*(volatile uint32_t *)0x10FD00u)

extern uint8_t bios_coin_state, bios_coin_change, bios_free_play_override;
extern uint8_t bios_cart_active;
uint8_t bios_free_play(void);
uint8_t bios_cart_valid(void);
uint8_t bios_backup_load(void);
void bios_backup_save(void);
void bios_cart_prepare(void);
void bios_controller_setup(void);

/* Credits in Backup RAM / System RAM */
#define P1_CREDITS          (*(volatile uint8_t  *)0xD00034u)
#define P2_CREDITS          (*(volatile uint8_t  *)0xD00035u)

/* Controller Bitmasks (Active High in BIOS_P1CURRENT / BIOS_P1CHANGE) */
#define JOY_UP              0x01u
#define JOY_DOWN            0x02u
#define JOY_LEFT            0x04u
#define JOY_RIGHT           0x08u
#define BTN_A               0x10u
#define BTN_B               0x20u
#define BTN_C               0x40u
#define BTN_D               0x80u

/* System Status Bits in BIOS_STATCURNT / BIOS_STATCHANGE */
#define STAT_START1         0x01u
#define STAT_SELECT1        0x02u
#define STAT_START2         0x04u
#define STAT_SELECT2        0x08u
#define STAT_COIN1          0x10u
#define STAT_COIN2          0x20u
#define STAT_SERVICE        0x40u
#define STAT_TEST           0x80u

/* Palette Color Constants (Neo Geo 16-bit RGB: 1 dark bit, 5 red, 5 green, 5 blue) */
#define COLOR_BLACK         0x8000u
#define COLOR_WHITE         0x7FFFu
#define COLOR_RED           0x4F00u
#define COLOR_GREEN         0x20F0u
#define COLOR_BLUE          0x100Fu
#define COLOR_YELLOW        0x6FF0u
#define COLOR_CYAN          0x30FFu
#define COLOR_MAGENTA       0x5F0Fu
#define COLOR_GOLD          0x6FE0u
#define COLOR_DARKGRAY      0x7333u
#define COLOR_MIDGRAY       0x7777u

/* Blank FIX cell: the space glyph, present in every text font. */
#define FIX_BLANK           0x0020u

/* VRAM Layout */
#define VRAM_SCB1           0x0000u  /* Sprite tile indices (0x0000..0x6FFF) */
#define VRAM_FIXMAP         0x7000u  /* FIX layer map (0x7000..0x74FF) */
#define VRAM_SCB2           0x8000u  /* Sprite shrink values (0x8000..0x81FF) */
#define VRAM_SCB3           0x8200u  /* Sprite Y pos & height (0x8200..0x83FF) */
#define VRAM_SCB4           0x8400u  /* Sprite X pos (0x8400..0x85FF) */

/* Cartridge Header Structure at 0x000100 */
typedef struct {
    char     magic[8];       /* 0x100: "NEO-GEO\0" */
    uint16_t ngh_id;         /* 0x108: Game unique ID */
    uint32_t psize;          /* 0x10A: Program ROM size in bytes */
    uint32_t pbcks;          /* 0x10E: Backup RAM address */
    uint16_t dipsize;        /* 0x112: Soft DIP settings size */
    uint8_t  logoflag;       /* 0x114: 0 = BIOS animated logo, 1 = Cartridge eye-catcher */
    uint8_t  logotile;       /* 0x115: Reserved */
    uint32_t reserved0[3];   /* 0x116..0x121: Jump table headers */
    uint32_t entry_user;     /* 0x122: bra.w USER */
    uint16_t nop0;           /* 0x126: nop */
    uint32_t entry_start;    /* 0x128: bra.w PLAYER_START */
    uint16_t nop1;           /* 0x12C: nop */
    uint32_t entry_demoend;  /* 0x12E: bra.w DEMO_END */
    uint16_t nop2;           /* 0x132: nop */
    uint32_t entry_coinsnd;  /* 0x134: bra.w COIN_SOUND */
    uint16_t nop3;           /* 0x138: nop */
} __attribute__((packed)) CartHeader;

static inline const CartHeader *get_cart_header(void)
{
    const volatile CartHeader *hdr = (const volatile CartHeader *)(uintptr_t)0x000100u;
    return (const CartHeader *)hdr;
}
#define CART_HEADER          get_cart_header()

/* Function Prototypes */
void bios_reset(void);
void bios_bus_err(void);
void bios_addr_err(void);
void bios_bus_err_c(uint32_t pc, uint32_t addr, uint32_t ir, uint32_t old_sp);
void bios_addr_err_c(uint32_t pc, uint32_t addr, uint32_t ir, uint32_t old_sp);
void bios_illegal(void);
void bios_privilege(void);
void bios_trace(void);
void bios_trap(void);
void bios_uninit(void);
void bios_spurious(void);

/* BIOS System Calls */
void sys_int1_c(void);
void sys_int2_c(void);
void sys_return_c(void);
void sys_io_c(void);
uint32_t sys_credit_check_c(void);
void sys_credit_down_c(void);
void sys_read_calendar_c(void);
void sys_set_calendar_c(void);
void sys_card_c(void);
void sys_card_error_c(void);
void sys_howtoplay_c(void);
void sys_fix_clear_c(void);
void sys_lsp_1st_c(void);
void sys_mess_out_c(void);

/* Helper Functions */
void bios_wait_vbl(void);
void bios_watchdog(void);
void bios_delay_frames(uint16_t frames);
void bios_fix_putc(uint8_t x, uint8_t y, char ch, uint8_t pal);
void bios_fix_puts(uint8_t x, uint8_t y, const char *str, uint8_t pal);
void bios_fix_put_hex16(uint8_t x, uint8_t y, uint16_t val, uint8_t pal);
void bios_fix_put_dec2(uint8_t x, uint8_t y, uint8_t val, uint8_t pal);
void bios_fix_clear_area(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void bios_set_palette(uint8_t bank, const uint16_t *colors);
void bios_set_backdrop(uint16_t color);
void bios_init_palette_banks(void);
void bios_palettes_clear(void);

/* Sub-systems */
void bios_splash_show(void);
void bios_eyecatcher(void);
void bios_test_menu(void);

#endif /* EAGLE_BIOS_H */
