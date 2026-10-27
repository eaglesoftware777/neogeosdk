/* No Signal native CD startup; cartridge startup is not linked. */
#include <stddef.h>
#include "sdk/neogeo.h"
#include "sdk/platform/cd/ng_cd.h"
#include "sdk/2d_engine/ng_fix.h"
#include "games/nosignal/scenes/ns.h"
#include "nosignal_cd_backgrounds.h"

volatile uint32_t ng_cd_frames;
volatile uint16_t ng_cd_input_seen;

void ns_cd_audio_tick(void);
void ng_fix_queue_drop(void);
extern int32_t ng_cd_bios_load(const uint8_t *descriptor);
volatile uint32_t ns_cd_background_loads, ns_cd_hero_loads, ns_cd_load_failures;
volatile uint16_t ns_cd_background, ns_cd_hero;
volatile int32_t ns_cd_load_status;

/* A slot's CRC in sprite RAM bank 3 (the scenery and the hero slot). */
static uint32_t slot_crc(uint32_t offset, uint32_t bytes)
{
    uint32_t crc = 0xFFFFFFFFul, i;
    uint16_t saved_sr;
    const volatile uint8_t *data = (const volatile uint8_t *)(0xE00000ul + offset);
    __asm__ volatile ("move.w %%sr,%0\n\tmove.w #0x2700,%%sr" : "=d" (saved_sr) : : "memory");
    NEO_REGISTER8(0xFF0121) = 0;
    NEO_REGISTER8(0xFF0105) = 0;
    NEO_REGISTER8(0xFF01A1) = 3;
    NEO_REGISTER8(0xFF016F) = 1;
    for (i=0; i<bytes; ++i) {
        crc = (crc >> 8) ^ ns_cd_crc_table[(crc ^ data[i]) & 255u];
        if (!(i & 255u)) kickWatchDog();
    }
    NEO_REGISTER8(0xFF0141) = 0;
    NEO_REGISTER8(0xFF016F) = 0;
    __asm__ volatile ("move.w %0,%%sr" : : "d" (saved_sr) : "memory");
    return crc ^ 0xFFFFFFFFul;
}

/* One file from the disc into its slot, read again until its CRC matches. */
static void load_checked(const NSCDBg *file, uint32_t offset)
{
    soundStopAll(); ns_cd_audio_tick();
    for (;;) {
        ns_cd_load_status = ng_cd_bios_load(file->descriptor);
        if (slot_crc(offset, file->bytes) == file->crc) break;
        ++ns_cd_load_failures;
        setBACKDROP(0x0F00u);
        /* Keep invalid scenery hidden. A retries the disc read. */
        do { waitVbl(); } while (!(poll_joystick() & NS_A));
    }
    NEO_REGISTER8(BIOS_SYSTEM_MODE) |= 0x80;
    NEO_REGISTER8(0xFF0111) = 0;
    NEO_REGISTER8(0xFF0115) = 0;
    NEO_REGISTER8(0xFF0119) = 1;
}

void ns_cd_load_background(uint8_t id)
{
    uint8_t i;
    if (ns_cd_background == id) return;
    for (i=0; i<sizeof(ns_cd_backgrounds)/sizeof(ns_cd_backgrounds[0]); ++i) {
        const NSCDBg *bg = &ns_cd_backgrounds[i];
        if (bg->id != id) continue;
        load_checked(bg, NS_CD_BG_OFFSET);
        ns_cd_background = id;
        ++ns_cd_background_loads;
        return;
    }
}

/* The hero's walk and run strides (the pictures past the resident ones)
 * into the hero slot; once a game unless the hero changes. */
void ns_cd_load_hero(uint8_t hero)
{
    if (hero >= sizeof(ns_cd_heroes)/sizeof(ns_cd_heroes[0]) || ns_cd_hero == hero + 1u) return;
    load_checked(&ns_cd_heroes[hero], NS_CD_HERO_OFFSET);
    ns_cd_hero = (uint16_t)(hero + 1u);
    ++ns_cd_hero_loads;
}

uint16_t NEOGEO_USER setSCB3(uint16_t y, uint16_t sticky, uint16_t rows)
{
    return (uint16_t)((y << 7) | (sticky << 6) | rows);
}

uint16_t NEOGEO_USER setSCB4(uint16_t x)
{
    return (uint16_t)((x & 511u) << 7);
}

void ng_cd_irq(void)
{
    uint16_t pad = (uint16_t)(NEO_REGISTER8(BIOS_P1CURRENT) |
                             ((uint16_t)NEO_REGISTER8(BIOS_STATCURNT) << 8));
    ng_cd_input_sample(pad);
    ng_cd_input_seen |= pad;
    *(volatile uint16_t *)USER_WORKRAM = 1;
    ++ng_cd_frames;
}

void NEOGEO_USER waitVbl(void)
{
    uint32_t before = ng_cd_frames;
    do { ng_cd_wait_vblank(); } while (ng_cd_frames == before);
    ns_cd_audio_tick();
}

void NEOGEO_USER vram_init(uint16_t address, uint16_t increment)
{
    NEO_REGISTER(VRAM_INC) = increment;
    NEO_REGISTER(VRAM_ADDR) = address;
}

void NEOGEO_USER vram_sfix(uint16_t increment, uint16_t address, uint16_t value)
{
    vram_init(address, increment);
    NEO_REGISTER(VRAM_RW) = value;
}

void NEOGEO_USER vram_sfix1(uint16_t value)
{
    NEO_REGISTER(VRAM_RW) = value;
}

void NEOGEO_USER vram_SCB234(uint16_t address, uint16_t value)
{
    NEO_REGISTER(VRAM_ADDR) = address;
    NEO_REGISTER(VRAM_RW) = value;
}

void NEOGEO_USER load_palettes(uint16_t *colors, uintptr_t address)
{
    volatile uint16_t *dest = (volatile uint16_t *)address;
    uint16_t i;
    for (i = 0; i < 16u; i++) dest[i] = colors[i];
}

void NEOGEO_USER setBACKDROP(uint16_t color)
{
    NEO_REGISTER(BACKDROP) = color;
}

void NEOGEO_USER clearFix(void)
{
    uint16_t i;
    ng_fix_queue_drop();
    vram_init(FIXMAP, 1);
    for (i = 0; i < 1280u; i++) NEO_REGISTER(VRAM_RW) = 0xFF;
    ng_fix_invalidate_all();
}

void NEOGEO_USER clearSprs(void)
{
    uint16_t i;
    vram_init(0x8200, 1);
    for (i = 0; i < 512u; i++) NEO_REGISTER(VRAM_RW) = 0;
}

void NEOGEO_USER setsfix(void) {}
void NEOGEO_USER kickWatchDog(void) { NEO_REGISTER8(0x300001) = 0; }
int NEOGEO_USER ng_sys_is_mvs(void) { return 0; }
int NEOGEO_USER ng_sys_region(void) { return NEO_REGISTER8(BIOS_COUNTRY_CODE); }
int NEOGEO_USER read_p1credit(void) { return 0; }

uint8_t NEOGEO_USER ns_start_pending(void)
{
    if (NEO_REGISTER8(NS_START_FLAG)) return 1u;
    if (poll_joystick() & START1) {
        NEO_REGISTER8(NS_START_FLAG) = 1u;
        return 1u;
    }
    return 0u;
}
void NEOGEO_USER ns_auto_start(void) { NEO_REGISTER8(NS_START_FLAG) = 1u; }
uint8_t NEOGEO_USER ns_spend_credit(void) { return 1u; }
void *NEOGEO_USER memcpy(void *d, const void *s, size_t n)
{
    volatile uint8_t *out=d; const volatile uint8_t *in=s;
    while (n--) *out++=*in++;
    return d;
}
void *NEOGEO_USER memset(void *d, int c, size_t n)
{
    volatile uint8_t *out=d;
    while (n--) *out++=(uint8_t)c;
    return d;
}
void ns_cd_setup(void)
{
    uint8_t i;
    NEO_REGISTER8(0xFF0111) = 1;
    NEO_REGISTER8(0xFF0115) = 1;
    NEO_REGISTER8(0x3A0001) = 0;
    NEO_REGISTER8(0x3A000F) = 0;
    NEO_REGISTER(0x3C0006) = 0;
    clearSprs(); clearFix(); setBACKDROP(BLACK);
    ns_presentation_init(); ns_fix_palettes(); soundInit();
    /* CD follows the V2 console defaults rather than unrelated BIOS DIPs. */
    for (i=0; i<10; ++i) NEO_REGISTER8(BIOS_GAME_DIP + 6u + i)=0;
    NEO_REGISTER8(BIOS_SYSTEM_MODE) |= 0x80;
    NEO_REGISTER8(0xFF0111) = 0;
    NEO_REGISTER8(0xFF0115) = 0;
    NEO_REGISTER8(0xFF0119) = 1;
    NEO_REGISTER(0xFF0004) = 0x0030;
    /* Stock LOADFILE needs live drive interrupts, including on first use. */
    waitVbl();
}

void ng_cd_start(void)
{
    ns_cd_setup();
    for (;;) {
        NEO_REGISTER8(NS_START_FLAG)=0;
        NEO_REGISTER8(BIOS_USER_MODE)=1;
        ns_run_attract(); ns_title_screen();
        NEO_REGISTER8(BIOS_USER_MODE)=2;
        NEO_REGISTER8(BIOS_PLAYER1_MODE)=1;
        soundSceneReset(); ns_run(); soundStopAll();
        NEO_REGISTER8(BIOS_PLAYER1_MODE)=3;
    }
}
