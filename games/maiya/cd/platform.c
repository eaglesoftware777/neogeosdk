/* Maiya CD platform boundary. Cartridge startup/user.c is not linked. */
#include <stddef.h>
#include "sdk/neogeo.h"
#include "sdk/platform/cd/ng_cd.h"
#include "sdk/2d_engine/ng_fix.h"
#include "games/maiya/scenes/maiya_game.h"

void NEOGEO_USER game_boot(void);
void NEOGEO_USER game_frame(void);
uint8_t NEOGEO_USER maiya_session_over(void);
void NEOGEO_USER maiya_fade_out(void);

volatile uint32_t ng_cd_frames;
volatile uint16_t ng_cd_input_seen;
extern volatile uint8_t maiya_console_start;

void maiya_cd_audio_tick(void);
void ng_fix_queue_drop(void);

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
    maiya_cd_audio_tick();
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

uint8_t NEOGEO_USER maiya_start_pending(void)
{
    if (maiya_console_start) return 1;
    if (poll_joystick() & (START1 | BUTTON_A)) {
        maiya_console_start = 1;
        return 1;
    }
    return 0;
}

void NEOGEO_USER setup_fix_palettes(void)
{
    uint16_t p[16];
    uint8_t i;

    /* Bank 0: White (PAL_TEXT) */
    p[0] = 0x8000;
    for (i = 1; i < 16; i++) p[i] = 0x7FFF;
    load_palettes(p, PALETTES);

    /* Bank 1: Gold / Yellow (PAL_GOLD) */
    p[0] = 0x8000;
    for (i = 1; i < 16; i++) p[i] = 0x6FE0;
    load_palettes(p, PALETTES + PALOFFSET);

    /* Bank 2: Red / Warn (PAL_WARN) */
    p[0] = 0x8000;
    for (i = 1; i < 16; i++) p[i] = 0x4F44;
    load_palettes(p, PALETTES + PALOFFSET * 2);

    /* Bank 3: Sky / Cyan (PAL_SKY) */
    p[0] = 0x8000;
    for (i = 1; i < 16; i++) p[i] = 0x39FF;
    load_palettes(p, PALETTES + PALOFFSET * 3);
}

static void maiya_cd_title_wait(void)
{
    int i;
    clearFix(); clearSprs(); setBACKDROP(BLACK);
    maiya_title();
    for (i = 0; ; i++) {
        if (maiya_console_start) break;
        ng_fix_puts(14, 25, "PUSH START", 1);
        if (maiya_start_pending()) break;
        maiya_vblank();
    }
    (void)i;
    maiya_hero_select();
}

static void maiya_cd_start_game(void)
{
    NEO_REGISTER8(BIOS_USER_MODE) = 2;
    NEO_REGISTER8(BIOS_PLAYER1_MODE) = 1;
    clearFix();
    clearSprs();
    maiya_boot();
    for (;;) {
        maiya_vblank();
        game_frame();
        if (maiya_session_over()) break;
    }
    soundStopAll();
    maiya_console_start = 0;
    NEO_REGISTER8(BIOS_PLAYER1_MODE) = 3;
    NEO_REGISTER8(BIOS_USER_MODE) = 1;
}

static void maiya_cd_game_attract(void)
{
    int i, round;
    for (round = 0; ; round++) {
        if (round == 0 && maiya_start_pending()) return;
        if (round == 0) {
            maiya_eyecatcher();
            if (maiya_start_pending()) return;
        }

        clearFix(); clearSprs(); setBACKDROP(BLACK);
        maiya_title();
        for (i = 0; i < 60 * 12; i++) {
            ng_fix_puts(14, 25, "PUSH START", 1);
            if (maiya_start_pending()) { maiya_fade_out(); return; }
            maiya_vblank();
        }

        maiya_fade_out();
        clearFix(); clearSprs(); setBACKDROP(BLACK);
        maiya_demo_begin();
        for (i = 0; i < 60 * 22; i++) {
            maiya_vblank();
            game_frame();
            if (maiya_start_pending()) { maiya_demo_end(); return; }
            if (maiya_demo_spent()) break;
        }
        maiya_demo_end();
    }
}

void ng_cd_start(void)
{
    NEO_REGISTER8(0xFF0111) = 1;
    NEO_REGISTER8(0xFF0115) = 1;
    NEO_REGISTER8(0x3A0001) = 0;
    NEO_REGISTER8(0x3A000F) = 0;
    NEO_REGISTER(0x3C0006) = 0;
    clearSprs();
    clearFix();
    setBACKDROP(BLACK);
    setup_fix_palettes();
    soundInit();
    maiya_save_check();
    NEO_REGISTER8(BIOS_SYSTEM_MODE) |= 0x80;
    NEO_REGISTER8(0xFF0111) = 0;
    NEO_REGISTER8(0xFF0115) = 0;
    NEO_REGISTER8(0xFF0119) = 1;
    NEO_REGISTER(0xFF0004) = 0x0030;
    maiya_console_start = 0;

    for (;;) {
        if (!maiya_console_start) {
            maiya_cd_game_attract();
        }
        maiya_cd_title_wait();
        maiya_cd_start_game();
    }
}
