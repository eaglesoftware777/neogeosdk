#include <stdint.h>
#include "macro.h"
#include "sound_ids.h"

uint16_t  setSCB2(uint16_t,uint16_t);
uint16_t  setSCB3(uint16_t,uint16_t,uint16_t);
uint16_t  setFIXDATA(uint16_t, uint16_t);
uint16_t  setSCB4(uint16_t);
uint16_t  setSCB1_2(uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t);
void  setBACKDROP(uint16_t);
void  load_palettes(uint16_t*, uintptr_t);
void  vram_init(uint16_t,uint16_t);
void  setpal(uint16_t *,uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t);
void  vram_SCB1(uint16_t *,uint16_t *,uint8_t);
void  vram_sfix(uint16_t,uint16_t,uint16_t);
void  vram_sfix1(uint16_t);
void  vram_SCB234(uint16_t,uint16_t);
void  vram_sprite (uint16_t ,uint16_t  ,uint16_t ,uint16_t*, uint16_t*, uint16_t, uint16_t, uint16_t, uint16_t);
void  vram_sprite_mvx(uint16_t,uint16_t);
void  vram_sprite_mvy(uint16_t,uint16_t);
void  fixtext_out(uint16_t, uint16_t,char *,short);
void  fixtext_out1(uint16_t, uint16_t,uint16_t *,short,int);
void  fixtext_out2(uint16_t, uint16_t,uint16_t, uint16_t, uint16_t,uint16_t,uint16_t *,short,int);
void  mess_outtest(void);
void setBIOSMESSBusy(void);
void setBIOSMESSReady(void);
void clearRAM(void);
void clearSprs(void);
void clearFix(void);
void waitVbl(void);
void cycle10ms(void);
void cycle1s(void);
void cyclexms1(int);
void cyclexs(int);
void cyclexms(int);
uint16_t poll_joystick(void);
void fix_svalue1(uint16_t, uint16_t,uint16_t,short,uint16_t);
void fix_svalue(uint16_t, uint16_t,uint16_t,short);
void setsfix(void);
void  display_digit(uint16_t, uint16_t,uint32_t,short,uint16_t);
int   read_p1credit(void);
void  playSoundtest(uint16_t);
void  isZ80Ready(void);
void  soundInit(void);
void soundCommand(uint8_t);
void  soundReset(void);
void  soundStopAll(void);
void  playMusic(uint8_t);
void  playSFX(uint8_t);
void  playSFXB(uint8_t);
void  playFMDebug(void);
void soundSetFMVolume(uint8_t);
void  playFMTrack(uint8_t);
void  playVoiceCue(uint8_t);
void  soundFadeOut(void);
void  soundFadeIn(void);
void  soundFadeOutSpeed(uint8_t);
void  soundFadeInSpeed(uint8_t);
void  soundStopMusic(void);
void  soundCancelFade(void);
void  soundSetTempo(uint8_t);
void  soundSetADPCMAVolume(uint8_t);
void  soundSetADPCMBVolume(uint8_t);
void  soundSetSSGVolume(uint8_t);
void playSSGTrack(uint8_t);
void soundSetSSGPreset(uint8_t);
void soundSceneReset(void);
void playInsertCoinSSG(void);
void playGetReadyVoice(void);
void playAttackVoice(void);
void playCoinThenReady(void);
void soundApplyMix(uint8_t,uint8_t,uint8_t,uint8_t);
void soundPlayDemoFM(uint8_t);
void soundPlayTitleMusic(uint8_t);
void soundPlayGameLoop(uint8_t);
void  kickWatchDog(void);
void  sleep1FFF(void);
void  displayCreditP1(void);
void  *memcpy(void *, const void *, int);
void  displayCreditP2(void);

uint32_t NEOGEO_USER __udivsi3(uint32_t a, uint32_t b)
{
    uint32_t res = 0;
    uint32_t rem = 0;
    int i;

    if (b == 0) return 0;

    if (b <= 0xFFFF && a < 0x80000000UL && (a >> 16) < b) {
        asm volatile (
            "move.l %[a], %%d0\n\t"
            "move.w %[b], %%d1\n\t"
            "divu %%d1,%%d0\n\t"
            "and.l #0xFFFF, %%d0\n\t"
            "move.l %%d0, %[res]"
            : [res] "=r" (res)
            : [a] "r" (a), [b] "r" (b)
            : "d0", "d1", "cc"
        );
        return res;
    }

    for (i = 31; i >= 0; i--) {
        rem <<= 1;
        rem |= (a >> i) & 1;
        if (rem >= b) {
            rem -= b;
            res |= (1UL << i);
        }
    }
    return res;
}

uint32_t NEOGEO_USER __umodsi3(uint32_t a, uint32_t b)
{
    uint32_t rem = 0;
    int i;

    if (b == 0) return 0;

    if (b <= 0xFFFF && a < 0x80000000UL && (a >> 16) < b) {
        asm volatile (
            "move.l %[a], %%d0\n\t"
            "move.w %[b], %%d1\n\t"
            "divu %%d1,%%d0\n\t"
            "swap %%d0\n\t"
            "and.l #0xFFFF, %%d0\n\t"
            "move.l %%d0, %[rem]"
            : [rem] "=r" (rem)
            : [a] "r" (a), [b] "r" (b)
            : "d0", "d1", "cc"
        );
        return rem;
    }

    for (i = 31; i >= 0; i--) {
        rem <<= 1;
        rem |= (a >> i) & 1;
        if (rem >= b) {
            rem -= b;
        }
    }
    return rem;
}

uint16_t NEOGEO_USER setSCB2(uint16_t Xshrink , uint16_t Yshrink) {
	uint16_t SCB2 = (Xshrink << 8) | Yshrink;
	return SCB2;
}

uint16_t NEOGEO_USER setSCB3(uint16_t Ypos , uint16_t sticky_flag , uint16_t height_factor) {
	uint16_t SCB3 = (Ypos << 7)  |  (sticky_flag << 6) | height_factor;
	return SCB3;
}

uint16_t NEOGEO_USER setFIXDATA(uint16_t palette_index, uint16_t tilenumber) {
	uint16_t FIXDATA = (palette_index << 11) |  tilenumber;
	return FIXDATA;
}

uint16_t NEOGEO_USER setSCB4(uint16_t Xpos) {
	uint16_t SCB4 = (Xpos << 7) ;
	return SCB4;
}

uint16_t NEOGEO_USER setSCB1_2(uint16_t pal_offset , uint16_t tile_offset , uint16_t bit3_autoanim , uint16_t bit2_autoanim , uint16_t vflip , uint16_t hflip ) {
	uint16_t Pal = (pal_offset << 8) | (tile_offset << 4) | (bit3_autoanim << 3) | (bit2_autoanim << 2) | (vflip << 1) | hflip ;
	return Pal;
}

void NEOGEO_USER setBACKDROP(uint16_t backdrop_color) {
	NEO_REGISTER(BACKDROP) = backdrop_color;
}

void NEOGEO_USER load_palettes(uint16_t *p_palette, uintptr_t palette_offset) {
	uint16_t *dst = (uint16_t *)palette_offset;
	uint16_t i = 0;
	for (i = 0; i < 16; i++) {
		dst[i] = p_palette[i];
	}
}

void NEOGEO_USER vram_init(uint16_t start,uint16_t vram_inc) {
	NEO_REGISTER(VRAM_ADDR) = start;
	NEO_REGISTER(VRAM_INC) = vram_inc;
}

void NEOGEO_USER setpal(uint16_t *pal_tile,uint16_t t0, uint16_t t1, uint16_t t2, uint16_t t3, uint16_t t4, uint16_t t5,uint16_t t6,uint16_t t7,uint16_t t8,uint16_t t9,uint16_t t10,uint16_t t11,uint16_t t12,uint16_t t13,uint16_t t14,uint16_t t15) {
	pal_tile[0] = t0;
	pal_tile[1] = t1;
	pal_tile[2] = t2;
	pal_tile[3] = t3;
	pal_tile[4] = t4;
	pal_tile[5] = t5;
	pal_tile[6] = t6;
	pal_tile[7] = t7;
	pal_tile[8] = t8;
	pal_tile[9] = t9;
	pal_tile[10] = t10;
	pal_tile[11] = t11;
	pal_tile[12] = t12;
	pal_tile[13] = t13;
	pal_tile[14] = t14;
	pal_tile[15] = t15;
}

void NEOGEO_USER vram_SCB1(uint16_t *SCB1_1 , uint16_t *SCB1_2 ,uint8_t tiles_number) {
	uint16_t i = 0;
	for (i = 0 ; i <  tiles_number  ; i++) {
		uint16_t SCB11 = *(SCB1_1+i);
		uint16_t SCB12 = *(SCB1_2+i);
		NEO_REGISTER(VRAM_RW) = SCB11;
		NEO_REGISTER(VRAM_RW) = SCB12;
	}
}

void NEOGEO_USER vram_sfix(uint16_t vram_inc,uint16_t FIXADDR,uint16_t FIXDATA) {
	NEO_REGISTER(VRAM_INC) = vram_inc;
	NEO_REGISTER(VRAM_ADDR) = FIXADDR;
	NEO_REGISTER(VRAM_RW)   = FIXDATA;
}

void NEOGEO_USER vram_sfix1(uint16_t FIXDATA) {
	NEO_REGISTER(VRAM_RW)   = FIXDATA;
}

void NEOGEO_USER vram_SCB234(uint16_t SCBADDR,uint16_t SCB234) {
	NEO_REGISTER(VRAM_ADDR) = SCBADDR;
	NEO_REGISTER(VRAM_RW)   = SCB234;
}

void NEOGEO_USER vram_sprite (uint16_t vram_start,uint16_t vram_inc,uint16_t vram_offset,uint16_t *SCB1_1 , uint16_t *SCB1_2 ,uint16_t tiles_number , uint16_t SCB2 , uint16_t SCB3 , uint16_t SCB4) {
	uint16_t SCB2ADDR = SCB2_ADDR+vram_offset;
	uint16_t SCB3ADDR = SCB3_ADDR+vram_offset;
	uint16_t SCB4ADDR = SCB4_ADDR+vram_offset;
	vram_init(vram_start,vram_inc);
	vram_SCB1(SCB1_1,SCB1_2 ,tiles_number);
	vram_SCB234(SCB2ADDR,SCB2);
	vram_SCB234(SCB3ADDR,SCB3);
	vram_SCB234(SCB4ADDR,SCB4);
}

void NEOGEO_USER vram_sprite_mvx(uint16_t vram_offset, uint16_t SCB4) {
	uint16_t SCB4ADDR = SCB4_ADDR+vram_offset;
	vram_SCB234(SCB4ADDR,SCB4);
}

void NEOGEO_USER vram_sprite_mvy(uint16_t vram_offset, uint16_t SCB3) {
	uint16_t SCB3ADDR = SCB3_ADDR+vram_offset;
	vram_SCB234(SCB3ADDR,SCB3);
}

int NEOGEO_USER strlen(const char *s) {
	const char *p = s;
	while(*s) s++;
	return (s - p);
}

void NEOGEO_USER fixtext_out(uint16_t x, uint16_t y,char *mess, short pal) {
	long textsz = strlen(mess);
	uint16_t addrfix =  FIXMAP+y+x*32;
	NEO_REGISTER(VRAM_ADDR)=addrfix;
	NEO_REGISTER(VRAM_INC) = 0x20;
	long i =0;
	for (i=0; i<textsz;i++) {
		uint16_t fixdata = (pal << 12) | *(mess+i);
		vram_sfix1(fixdata);
	}
}

void NEOGEO_USER fixtext_out1(uint16_t x, uint16_t y,uint16_t *mess,short pal,int objsz) {
	uint16_t addrfix =  FIXMAP+y+x*32;
	NEO_REGISTER(VRAM_ADDR)=addrfix;
	NEO_REGISTER(VRAM_INC) = 0x20;
	int i =0;
	for (i=0; i<objsz;i++) {
		uint16_t fixdata = (pal << 12) | *(mess+i);
		vram_sfix1(fixdata);
	}
}

void NEOGEO_USER fixtext_out2(uint16_t x, uint16_t y,uint16_t a, uint16_t b, uint16_t c,uint16_t mod,uint16_t *mess,short pal,int objsz) {
	uint16_t addrfix =  FIXMAP+a*x+b*y+c;
	NEO_REGISTER(VRAM_ADDR)=addrfix;
	NEO_REGISTER(VRAM_INC) = mod;
	int i = 0;
	for (i=0; i<objsz;i++) {
		uint16_t fixdata = (pal << 12) | *(mess+i);
		vram_sfix1(fixdata);
	}
}

void NEOGEO_USER mess_outtest(void) {
	setBIOSMESSBusy();
	uint16_t *pmessp = (uint16_t *)NEO_REGISTER32(BIOS_MESS_POINT);
	uint16_t *ptr = (uint16_t *)RAMSTART;
	*ptr++ =0x434F; *ptr++ =0x4D4D; *ptr++ =0x414E; *ptr++ =0x4420; *ptr++ =0x34FF;
	*ptr++ =0x434F; *ptr++ =0x4D4D; *ptr++ =0x414E; *ptr++ =0x4420; *ptr++ =0x36FF;
	*ptr++ =0x0;
	uint16_t *ptrsub = (uint16_t *)(RAMSTART + 100);
	*ptrsub++=COMMAND5; *ptrsub++=0x0001; *ptrsub++=COMMAND9T1; *ptrsub++=0x5C2F; *ptrsub++=0x3A2E; *ptrsub++=0xD7FF;
	*ptrsub++=COMMAND5; *ptrsub++=0x0001; *ptrsub++=COMMANDB; *ptrsub++ =0x0;
	*pmessp++=MESSPOINTZERO; *pmessp++=MESSPOINTZERO; *pmessp++=COMMAND1BF; *pmessp++=ZENDCODEFF;
	*pmessp++=COMMAND2INC20; *pmessp++=COMMAND3; *pmessp++=0x7252; *pmessp++=COMMAND7;
	*pmessp++=0x434F; *pmessp++=0x4D4D; *pmessp++=0x414E; *pmessp++=0x4420; *pmessp++=0x37FF;
	*pmessp++=COMMANDC2; *pmessp++=0x0021; *pmessp++=COMMAND5; *pmessp++=0x0001;
	*pmessp++=COMMAND4; *pmessp++=0x0010; *pmessp++=0x0000; *pmessp++=COMMANDC2;
	*pmessp++=0x0021; *pmessp++=COMMAND5; *pmessp++=0x0001; *pmessp++=COMMAND6;
	*pmessp++=COMMANDC2; *pmessp++=0x0021; *pmessp++=COMMAND5; *pmessp++=0x0001;
	*pmessp++=COMMANDDA; *pmessp++=0x0030; *pmessp++=COMMAND5; *pmessp++=0x0001;
	*pmessp++=COMMAND8T1; *pmessp++=0x4142; *pmessp++=0x4344; *pmessp++=0x4546; *pmessp++=0x47FF;
	*pmessp++=COMMAND5; *pmessp++=0x0001; *pmessp++=COMMAND9T1; *pmessp++=0xD1D2;
	*pmessp++=0xD3D4; *pmessp++=0xD5D6; *pmessp++=0xD7FF; *pmessp++=COMMANDA;
	*pmessp++=0x0010; *pmessp++=0x0100; *pmessp++=COMMAND7; *pmessp++=0x454E; *pmessp++=0x44FF;
	*pmessp++=COMMAND0;
	NEO_REGISTER32(BIOS_MESS_POINT) = (uint32_t)pmessp;
	setBIOSMESSReady();
	CALLNEOGEOF(SYS_MESS_OUT);
}

void NEOGEO_USER setBIOSMESSBusy(void) {
	asm volatile ("addq.b #1, 0x10FDC2");
}

void NEOGEO_USER setBIOSMESSReady(void) {
	asm volatile ("subq.b #1, 0x10FDC2");
}

void NEOGEO_USER clearRAM() {
	asm volatile (
		"move.l #0x3CC-1, %%d7\n\t"
		"movea.l #0x100000, %%a0\n\t"
		"moveq #0, %%d0\n"
		".clram:\n\t"
		"move.l %%d0, (%%a0)+\n\t"
		"move.l %%d0, (%%a0)+\n\t"
		"move.l %%d0, (%%a0)+\n\t"
		"move.l %%d0, (%%a0)+\n\t"
		"move.l %%d0, (%%a0)+\n\t"
		"move.l %%d0, (%%a0)+\n\t"
		"move.l %%d0, (%%a0)+\n\t"
		"move.l %%d0, (%%a0)+\n\t"
		"dbra %%d7, .clram"
		: : : "d0", "d7", "a0", "memory"
	);
}

void NEOGEO_USER clearSprs() {
	asm volatile (
		"move.w #0x8200, 0x3C0000\n\t"
		"clr.w %%d0\n\t"
		"move.w #1, 0x3C0004\n\t"
		"move.l #512-1, %%d7\n"
		".clspr:\n\t"
		"move.w %%d0, 0x3C0002\n\t"
		"dbra %%d7, .clspr"
		: : : "d0", "d7", "memory"
	);
}

void NEOGEO_USER clearFix() {
	asm volatile (
		"jsr 0xC004C2\n\t"
		"move.l #1280-1, %%d7\n\t"
		"move.w #0x7000, 0x3C0000\n\t"
		"move.w #0xFF, %%d0\n"
		".clfix:\n\t"
		"move.w %%d0, 0x3C0002\n\t"
		"dbra %%d7, .clfix"
		: : : "d0", "d7", "memory"
	);
}

void NEOGEO_USER waitVbl() {
	asm volatile (
		".waitv:\n\t"
		"tst.w 0x100000\n\t"
		"jeq .waitv\n\t"
		"clr.w 0x100000\n\t"
		"addq.l #1, 0x100020"
	);
}

void NEOGEO_USER cycle10ms() {
	asm volatile ("move.w #2400, %%d0\n.d10:\ndbf %%d0, .d10" : : : "d0");
}

void NEOGEO_USER cycle1s() {
	asm volatile (
		"move.w #5000, %%d1\n"
		".c1s:\n\t"
		"move.w #240, %%d0\n"
		".d1s:\n\t"
		"dbf %%d0, .d1s\n\t"
		"subq #1, %%d1\n\t"
		"bne .c1s"
		: : : "d0", "d1"
	);
}

void NEOGEO_USER cyclexms1(int cyc1) {
	asm volatile (
		"movem.l %%d0-%%d1, -(%%sp)\n\t"
		"move.l %[cyc1], %%d1\n"
		".cxms1:\n\t"
		"move.w #240, %%d0\n"
		".dxms1:\n\t"
		"dbf %%d0, .dxms1\n\t"
		"subq #1, %%d1\n\t"
		"bne .cxms1\n\t"
		"movem.l (%%sp)+, %%d0-%%d1"
		: : [cyc1] "g" (cyc1) : "cc"
	);
}

void NEOGEO_USER cyclexs(int cyc1xs) {
	asm volatile (
		"movem.l %%d0-%%d2, -(%%sp)\n\t"
		"move.w %[cyc1xs], %%d1\n"
		".cxs:\n\t"
		"move.w #5000, %%d2\n"
		".cxs_sub:\n\t"
		"move.w #240, %%d0\n"
		".dxs:\n\t"
		"dbf %%d0, .dxs\n\t"
		"subq #1, %%d2\n\t"
		"bne .cxs_sub\n\t"
		"subq #1, %%d1\n\t"
		"bne .cxs\n\t"
		"movem.l (%%sp)+, %%d0-%%d2"
		: : [cyc1xs] "g" (cyc1xs) : "cc"
	);
}

void NEOGEO_USER cyclexms(int cycxms) {
	asm volatile (
		"movem.l %%d0-%%d2, -(%%sp)\n\t"
		"move.w %[cycxms], %%d1\n"
		".cxms:\n\t"
		"move.w #50, %%d2\n"
		".cxms_sub:\n\t"
		"move.w #240, %%d0\n"
		".dxms:\n\t"
		"dbf %%d0, .dxms\n\t"
		"subq #1, %%d2\n\t"
		"bne .cxms_sub\n\t"
		"subq #1, %%d1\n\t"
		"bne .cxms\n\t"
		"movem.l (%%sp)+, %%d0-%%d2"
		: : [cycxms] "g" (cycxms) : "cc"
	);
}

uint16_t NEOGEO_USER poll_joystick() {
	uint8_t d1 = *(volatile uint8_t *)0x10FD95;
	uint8_t d2 = *(volatile uint8_t *)0x10FDAC;
	return (uint16_t)((d2 << 8) | d1);
}

void NEOGEO_USER fix_svalue1(uint16_t X, uint16_t Y,uint16_t v,short pal,uint16_t offset) {
	uint16_t addrfix =  FIXMAP+(Y+2+((X+1)*32));
	vram_sfix(0x20,addrfix,(uint16_t)((pal << 12) | (v + offset)));
}

void NEOGEO_USER fix_svalue(uint16_t X, uint16_t Y,uint16_t v,short pal) {
	uint16_t addrfix =  FIXMAP+(Y+2+((X+1)*32));
	vram_sfix(0x20,addrfix,(uint16_t)((pal << 12) | v));
}

void NEOGEO_USER setsfix() {
	asm volatile ("bset.b #0, 0x3A000B");
}

void NEOGEO_USER display_digit(uint16_t X, uint16_t Y,uint32_t value,short pal,uint16_t offset) {
	uint16_t s[10];
	int objsz = 0;
	uint32_t power = 1UL;
    if (X < 2) X = 2; if (X > 37) X = 37;
    if (Y < 1) Y = 1; if (Y > 26) Y = 26;
	if (value >= 1000000000UL) power = 1000000000UL;
	else if (value >= 100000000UL) power = 100000000UL;
	else if (value >= 10000000UL) power = 10000000UL;
	else if (value >= 1000000UL) power = 1000000UL;
	else if (value >= 100000UL) power = 100000UL;
	else if (value >= 10000UL) power = 10000UL;
	else if (value >= 1000UL) power = 1000UL;
	else if (value >= 100UL) power = 100UL;
	else if (value >= 10UL) power = 10UL;
	while (1) {
		uint16_t digit = 0;
		while (value >= power) { value -= power; digit++; }
		s[objsz++] = digit + offset;
		if (power == 1UL) break;
		if (power == 1000000000UL) power = 100000000UL;
		else if (power == 100000000UL) power = 10000000UL;
		else if (power == 10000000UL) power = 1000000UL;
		else if (power == 1000000UL) power = 100000UL;
		else if (power == 100000UL) power = 10000UL;
		else if (power == 10000UL) power = 1000UL;
		else if (power == 1000UL) power = 100UL;
		else if (power == 100UL) power = 10UL;
		else power = 1UL;
	}
	fixtext_out1(X,Y,s,pal,objsz);
}

int NEOGEO_USER read_p1credit(void) {
	return *(volatile uint8_t *)0xD00034;
}

void NEOGEO_USER playSoundtest(uint16_t index) {
	isZ80Ready();
	soundCommand((uint8_t)(index & 0xFF));
}

void NEOGEO_USER soundCommand(uint8_t command) {
	isZ80Ready();
	*(volatile uint8_t*)0x320000 = command;
	isZ80Ready();
}

void NEOGEO_USER soundInit(void) { isZ80Ready(); soundCommand(0x01); }
void NEOGEO_USER soundReset(void) { isZ80Ready(); soundCommand(0x03); }
void NEOGEO_USER soundStopAll(void) { isZ80Ready(); soundCommand(0x04); }
void NEOGEO_USER soundStopMusic(void) { isZ80Ready(); soundCommand(0x0F); }
void NEOGEO_USER soundCancelFade(void) { isZ80Ready(); soundCommand(0x11); }
void NEOGEO_USER soundSceneReset(void) { isZ80Ready(); soundStopAll(); cyclexms(4); isZ80Ready(); soundReset(); cyclexms(4); }
void NEOGEO_USER playMusic(uint8_t n) { isZ80Ready(); soundCommand(0x20 + n); }
void NEOGEO_USER playSFX(uint8_t n) { isZ80Ready(); soundCommand(0x40 + n); }
void NEOGEO_USER playSFXB(uint8_t n) { isZ80Ready(); soundCommand(0x80 + n); }
void NEOGEO_USER playFMDebug(void) { isZ80Ready(); soundCommand(0x30); }
void NEOGEO_USER playFMTrack(uint8_t n) { isZ80Ready(); soundCommand(0x31); isZ80Ready(); soundCommand(n); }
void NEOGEO_USER soundSetFMVolume(uint8_t v) { isZ80Ready(); soundCommand(0x13); isZ80Ready(); soundCommand(v & 0x0F); }
void NEOGEO_USER playSSGTrack(uint8_t n) { isZ80Ready(); soundCommand(0x32); isZ80Ready(); soundCommand(n); }
void NEOGEO_USER soundSetSSGPreset(uint8_t preset) { isZ80Ready(); soundCommand(0x14); isZ80Ready(); soundCommand(preset & 0x0F); }
void NEOGEO_USER playInsertCoinSSG(void) { isZ80Ready(); playSSGTrack(SOUND_SSG_INSERT_COIN); soundSetSSGPreset(1); }

void NEOGEO_USER playVoiceCue(uint8_t n) {
	isZ80Ready();
	switch (n) {
		case SOUND_VOICE_GET_READY: playSFX(SOUND_SFX_READY_VOICE); break;
		case SOUND_VOICE_ATTACK: playSFX(SOUND_SFX_ATTACK_VOICE); break;
		default: playSFX(n); break;
	}
}

void NEOGEO_USER playGetReadyVoice(void) { isZ80Ready(); playSFX(SOUND_SFX_READY_VOICE); }
void NEOGEO_USER playAttackVoice(void) { isZ80Ready(); playSFX(SOUND_SFX_ATTACK_VOICE); }

void NEOGEO_USER playCoinThenReady(void) {
	isZ80Ready(); playInsertCoinSSG(); cyclexms(250); isZ80Ready(); playGetReadyVoice();
}

void NEOGEO_USER soundFadeOut(void) { isZ80Ready(); soundFadeOutSpeed(0x20); }
void NEOGEO_USER soundFadeIn(void) { isZ80Ready(); soundFadeInSpeed(0x20); }
void NEOGEO_USER soundFadeOutSpeed(uint8_t speed) { isZ80Ready(); soundCommand(0x0A); isZ80Ready(); soundCommand(speed); }
void NEOGEO_USER soundFadeInSpeed(uint8_t speed) { isZ80Ready(); soundCommand(0x12); isZ80Ready(); soundCommand(speed); }
void NEOGEO_USER soundSetTempo(uint8_t t) { isZ80Ready(); soundCommand(0x0E); isZ80Ready(); soundCommand(t); }
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v) { isZ80Ready(); soundCommand(0x05); isZ80Ready(); soundCommand(v & 0x3F); }
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v) { isZ80Ready(); soundCommand(0x06); isZ80Ready(); soundCommand(v); }
void NEOGEO_USER soundSetSSGVolume(uint8_t v) { isZ80Ready(); soundCommand(0x07); isZ80Ready(); soundCommand(v & 0x0F); }

void NEOGEO_USER soundApplyMix(uint8_t adpcma_vol, uint8_t adpcmb_vol, uint8_t ssg_vol, uint8_t fm_vol) {
	isZ80Ready(); soundSetADPCMAVolume(adpcma_vol); isZ80Ready(); soundSetADPCMBVolume(adpcmb_vol);
	isZ80Ready(); soundSetSSGVolume(ssg_vol); isZ80Ready(); soundSetFMVolume(fm_vol);
}

void NEOGEO_USER soundPlayDemoFM(uint8_t fm_track) {
	isZ80Ready(); soundSceneReset(); isZ80Ready(); soundSetFMVolume(0x0C); isZ80Ready(); playFMTrack(fm_track);
}

void NEOGEO_USER soundPlayTitleMusic(uint8_t music_track) {
	isZ80Ready(); soundSceneReset(); isZ80Ready(); soundApplyMix(0x34, 0xC8, 0x00, 0x00);
	isZ80Ready(); playSFX(SOUND_SFX_TITLE_GONG); cyclexms(10); isZ80Ready(); playSFXB(SOUND_BED_TITLE_THEME);
}

void NEOGEO_USER soundPlayGameLoop(uint8_t music_track) {
	isZ80Ready(); soundSceneReset(); isZ80Ready(); soundApplyMix(0x30, 0xB8, 0x00, 0x00);
	isZ80Ready();
	switch (music_track) {
		case SOUND_MUSIC_SAMURAI_GAME_LOOP: playSFXB(SOUND_BED_STAGE_ONE); break;
		case SOUND_MUSIC_SAMURAI_BATTLE_LOOP: playSFXB(SOUND_BED_STAGE_TWO); break;
		default: playSFXB(SOUND_BED_ENDING_THEME); break;
	}
}

void NEOGEO_USER  isZ80Ready() {
	asm volatile (".isr:\n\tmove.b #0,0x300001\n\tmove.b 0x320000,%%d0\n\tcmp.b #0x01,%%d0\n\tbne .isr" : : : "d0");
}

void NEOGEO_USER kickWatchDog(void) {
	asm volatile ("move.b %%d0, 0x300001" : : : "memory");
}

void NEOGEO_USER sleep1FFF(void) {
	asm volatile ("move.w #0x1FFF,%%d0\n.slp:\nnop\ndbra %%d0,.slp" : : : "d0");
}

void NEOGEO_USER displayCreditP1(void) {
	asm volatile (
		"moveq #0,%%d0\n\t"
		"move.b 0xD00034,%%d0\n\t"
		"move.w #0x7088,0x3C0000\n\t"
		"move.w %%d0,%%d1\n\t"
		"move.w #0x20,0x3C0004\n\t"
		"lsr.b #4,%%d1\n\t"
		"andi.w #0x000F,%%d1\n\t"
		"ori.w #0x0030,%%d1\n\t"
		"move.w %%d1,0x3C0002\n\t"
		"andi.w #0x000F,%%d0\n\t"
		"ori.w #0x0030,%%d1\n\t"
		"move.w %%d1,0x3C0002"
		: : : "d0", "d1", "memory"
	);
}

void NEOGEO_USER displayCreditP2(void) {
	asm volatile (
		"moveq #0,%%d0\n\t"
		"move.b 0xD00035,%%d0\n\t"
		"move.w #0x7088,0x3C0000\n\t"
		"move.w %%d0,%%d1\n\t"
		"move.w #0x20,0x3C0004\n\t"
		"lsr.b #4,%%d1\n\t"
		"andi.w #0x000F,%%d1\n\t"
		"ori.w #0x0030,%%d1\n\t"
		"move.w %%d1,0x3C0002\n\t"
		"andi.w #0x000F,%%d0\n\t"
		"ori.w #0x0030,%%d1\n\t"
		"move.w %%d1,0x3C0002"
		: : : "d0", "d1", "memory"
	);
}

void *memcpy(void *dest, const void *src, int count) {
    char *d = (char *)dest; const char *s = (const char *)src;
    while (count--) *d++ = *s++;
    return dest;
}
