/* Sky Lance CD platform boundary. Cartridge startup/user.c is not linked. */
#include <stddef.h>
#include "sdk/neogeo.h"
#include "sdk/platform/cd/ng_cd.h"
#include "sdk/2d_engine/ng_fix.h"
#include "sdk/2d_engine/ng_game_interupt.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "games/skylance/scenes/sky.h"
#include "games/skylance/scenes/sky_draw.h"

volatile uint32_t ng_cd_frames;
volatile uint16_t ng_cd_input_seen;
volatile uint16_t sky_cd_phase;
void sky_cd_audio_tick(void);
void ng_fix_queue_drop(void);
uint16_t NEOGEO_USER setSCB3(uint16_t y,uint16_t sticky,uint16_t rows)
{ return (uint16_t)((y<<7)|(sticky<<6)|rows); }
uint16_t NEOGEO_USER setSCB4(uint16_t x) { return (uint16_t)((x&511u)<<7); }

void ng_cd_irq(void)
{
    uint16_t pad=(uint16_t)(NEO_REGISTER8(BIOS_P1CURRENT) |
                          ((uint16_t)NEO_REGISTER8(BIOS_STATCURNT)<<8));
    ng_cd_input_sample(pad);
    ng_cd_input_seen|=pad;
    ++ng_cd_frames;
}

void NEOGEO_USER waitVbl(void)
{
    uint32_t before=ng_cd_frames;
    do { ng_cd_wait_vblank(); } while (ng_cd_frames==before);
    sky_cd_audio_tick();
}

void NEOGEO_USER vram_init(uint16_t address,uint16_t increment)
{ NEO_REGISTER(VRAM_INC)=increment;NEO_REGISTER(VRAM_ADDR)=address; }
void NEOGEO_USER vram_sfix(uint16_t increment,uint16_t address,uint16_t value)
{ vram_init(address,increment);NEO_REGISTER(VRAM_RW)=value; }
void NEOGEO_USER vram_sfix1(uint16_t value) { NEO_REGISTER(VRAM_RW)=value; }
void NEOGEO_USER vram_SCB234(uint16_t address,uint16_t value)
{ NEO_REGISTER(VRAM_ADDR)=address;NEO_REGISTER(VRAM_RW)=value; }
void NEOGEO_USER load_palettes(uint16_t *colors,uintptr_t address)
{
    volatile uint16_t *dest=(volatile uint16_t *)address;
    uint16_t i;for(i=0;i<16u;i++) dest[i]=colors[i];
}
void NEOGEO_USER setBACKDROP(uint16_t color) { NEO_REGISTER(BACKDROP)=color; }
void NEOGEO_USER clearFix(void)
{
    uint16_t i;ng_fix_queue_drop();vram_init(FIXMAP,1);
    for(i=0;i<1280u;i++) NEO_REGISTER(VRAM_RW)=0xFF;
    ng_fix_invalidate_all();
}
void NEOGEO_USER clearSprs(void)
{
    uint16_t i;vram_init(0x8200,1);
    for(i=0;i<512u;i++) NEO_REGISTER(VRAM_RW)=0;
}
void NEOGEO_USER setsfix(void) { /* CD has game FIX RAM, not a cartridge SFIX switch. */ }
void NEOGEO_USER kickWatchDog(void) { NEO_REGISTER8(0x300001)=0; }
int NEOGEO_USER ng_sys_is_mvs(void) { return 0; }
int NEOGEO_USER ng_sys_region(void) { return NEO_REGISTER8(BIOS_COUNTRY_CODE); }
uint8_t NEOGEO_USER sky_start_pending(void)
{ return (uint8_t)((poll_joystick() & 0x100u)!=0u); }

void *memcpy(void *to,const void *from,size_t n)
{ unsigned char *d=to;const unsigned char *s=from;while(n--) *d++=*s++;return to; }
void *memset(void *to,int value,size_t n)
{ unsigned char *d=to;while(n--) *d++=(unsigned char)value;return to; }

static void fix_palettes(void)
{
    static const uint16_t inks[4][2]={{0xDEFF,0x39AC},{0x6FEA,0x6E92},
                                          {0x3AEF,0xC2AE},{0x6FBA,0x0E32}};
    uint8_t i,j;uint16_t palette[16];
    for(i=0;i<4u;i++) {
        for(j=0;j<16u;j++) palette[j]=BLACK;
        palette[0]=0x8000;palette[1]=inks[i][0];palette[2]=0xC012;palette[3]=inks[i][1];
        ng_palfx_screen_load(i,palette);
    }
    sky_fix_palettes_init();
}

void ng_cd_start(void)
{
    NEO_REGISTER8(0xFF0111)=1;NEO_REGISTER8(0xFF0115)=1;
    NEO_REGISTER8(0x3A0001)=0;NEO_REGISTER8(0x3A000F)=0;
    NEO_REGISTER(0x3C0006)=0;
    clearSprs();clearFix();setBACKDROP(BLACK);
    ng_game_runtime_init();
    sky_presentation_init();fix_palettes();
    NEO_REGISTER8(SKY_START_FLAG)=0;
    soundInit();
    NEO_REGISTER8(0xFF0111)=0;NEO_REGISTER8(0xFF0115)=0;NEO_REGISTER8(0xFF0119)=1;
    NEO_REGISTER(0xFF0004)=0x0030;
    sky_cd_phase=1;
    for(;;) { sky_run();sky_cd_phase++; }
}
