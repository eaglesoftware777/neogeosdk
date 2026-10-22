#include "ng_cd.h"
#include "sdk/neogeo.h"
#include "sdk/macro.h"
#include "sdk/2d_engine/ng_fix.h"

volatile uint32_t ng_cd_frames;
volatile uint16_t ng_cd_input_seen;
static uint16_t object_x=32;
void game_boot(void);
void game_frame(void);

void NEOGEO_USER vram_sfix(uint16_t increment, uint16_t address, uint16_t value)
{
    NEO_REGISTER(VRAM_INC)=increment;
    NEO_REGISTER(VRAM_ADDR)=address;
    NEO_REGISTER(VRAM_RW)=value;
}

void NEOGEO_USER clearFix(void)
{
    uint16_t i;
    NEO_REGISTER(VRAM_INC)=1;
    NEO_REGISTER(VRAM_ADDR)=FIXMAP;
    for(i=0;i<1280u;i++) NEO_REGISTER(VRAM_RW)=0xFF;
}
void NEOGEO_USER waitVbl(void) { ng_cd_wait_vblank(); }

void ng_cd_irq(void)
{
    uint16_t current=(uint16_t)(NEO_REGISTER8(BIOS_P1CURRENT) |
                               ((uint16_t)NEO_REGISTER8(BIOS_STATCURNT)<<8));
    /* Firmware interrupt completion may poll again before returning to the
     * game. Preserve a frame's edges in game-owned RAM, not BIOS scratch. */
    ng_cd_input_sample(current);
    ++ng_cd_frames;
    ng_cd_input_seen |= current;
}

static void moving_sprite(void)
{
    uint16_t joy=poll_joystick();
    if((joy & JOY_RIGHT) && object_x<288) object_x+=2;
    if((joy & JOY_LEFT) && object_x>16) object_x-=2;
    NEO_REGISTER(VRAM_INC)=1;
    NEO_REGISTER(VRAM_ADDR)=64;
    NEO_REGISTER(VRAM_RW)=1;
    NEO_REGISTER(VRAM_RW)=0x0100;
    NEO_REGISTER(VRAM_ADDR)=0x8001;
    NEO_REGISTER(VRAM_RW)=0x0FFF;
    NEO_REGISTER(VRAM_ADDR)=0x8201;
    NEO_REGISTER(VRAM_RW)=(uint16_t)(((512u-120u)&511u)<<7)|1u;
    NEO_REGISTER(VRAM_ADDR)=0x8401;
    NEO_REGISTER(VRAM_RW)=(uint16_t)(object_x<<7);
}

void ng_cd_start(void)
{
    uint16_t i;
    *(volatile uint8_t *)0xFF0111ul=1;
    *(volatile uint8_t *)0xFF0115ul=1;
    *(volatile uint8_t *)0x3A0001ul=0; /* release the LSPC display blank */
    *(volatile uint8_t *)0x3A000Ful=0;
    NEO_REGISTER(PALETTES)=0x8000;
    NEO_REGISTER(PALETTES+2)=0x0FFF;
    NEO_REGISTER(PALETTES+32)=0x8000;
    for(i=1;i<16u;i++) NEO_REGISTER(PALETTES+32+i*2u)=(uint16_t)(0x0F00u+i);
    NEO_REGISTER(0x401FFE)=0x8006;
    NEO_REGISTER(VRAM_INC)=1;
    NEO_REGISTER(VRAM_ADDR)=0x8200;
    for(i=0;i<384u;i++) NEO_REGISTER(VRAM_RW)=0;
    game_boot();
    ng_fix_puts(3,20,"NEO GEO CD NATIVE",0);
    ng_fix_puts(3,22,"RIGHT/LEFT: MOVE THE SPRITE",0);
    *(volatile uint8_t *)0xFF0111ul=0;
    *(volatile uint8_t *)0xFF0115ul=0;
    *(volatile uint8_t *)0xFF0119ul=1;
    *(volatile uint16_t *)0xFF0004ul=0x0030;
    for(;;) {
        ng_cd_wait_vblank();
        game_frame();
        moving_sprite();
    }
}
