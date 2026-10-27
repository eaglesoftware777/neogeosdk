/* Native CD boundary; V2 game, renderer and RPG rules remain unchanged. */
#include <stddef.h>
#include "sdk/neogeo.h"
#include "sdk/platform/cd/ng_cd.h"
#include "sdk/2d_engine/ng_fix.h"
#include "games/maiyaluna/scenes/ml.h"

volatile uint32_t ng_cd_frames;
volatile uint16_t ng_cd_input_seen;

void ml_cd_audio_tick(void);
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
    ml_cd_audio_tick();
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

uint8_t NEOGEO_USER ml_start_pending(void)
{
    if (NEO_REGISTER8(ML_START_FLAG)) return 1u;
    if (poll_joystick() & START1) {
        NEO_REGISTER8(ML_START_FLAG) = 1u;
        return 1u;
    }
    return 0u;
}
void NEOGEO_USER ml_auto_start(void) { NEO_REGISTER8(ML_START_FLAG) = 1u; }
uint8_t NEOGEO_USER ml_spend_credit(void) { return 1u; }
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
void ng_cd_start(void)
{
    uint8_t i;
    NEO_REGISTER8(0xFF0111) = 1;
    NEO_REGISTER8(0xFF0115) = 1;
    NEO_REGISTER8(0x3A0001) = 0;
    NEO_REGISTER8(0x3A000F) = 0;
    NEO_REGISTER(0x3C0006) = 0;
    clearSprs(); clearFix(); setBACKDROP(BLACK);
    ml_presentation_init(); ml_fix_palettes(); soundInit();
    /* CD follows the V2 console defaults rather than unrelated BIOS DIPs. */
    for (i=0; i<10; ++i) NEO_REGISTER8(BIOS_GAME_DIP + 6u + i)=0;
    NEO_REGISTER8(BIOS_SYSTEM_MODE) |= 0x80;
    NEO_REGISTER8(0xFF0111) = 0;
    NEO_REGISTER8(0xFF0115) = 0;
    NEO_REGISTER8(0xFF0119) = 1;
    NEO_REGISTER(0xFF0004) = 0x0030;
    for (;;) {
        NEO_REGISTER8(ML_START_FLAG)=0;
        NEO_REGISTER8(BIOS_USER_MODE)=1;
        ml_run_attract(); ml_title_screen();
        NEO_REGISTER8(BIOS_USER_MODE)=2;
        NEO_REGISTER8(BIOS_PLAYER1_MODE)=1;
        soundSceneReset(); ml_run(); soundStopAll();
        NEO_REGISTER8(BIOS_PLAYER1_MODE)=3;
    }
}
