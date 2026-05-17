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

    /* Optimize for 16-bit divisors */
    if (b <= 0xFFFF && a < 0x80000000UL && (a >> 16) < b) {
        asm volatile (
        "move.l %[a], %%d0\n\t"
        "move.w %[b], %%d1\n\t"
        "divu %%d1,%%d0\n\t"
        "andi.l #0xFFFF,%%d0\n\t"
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

    /* Optimize for 16-bit divisors */
    if (b <= 0xFFFF && a < 0x80000000UL && (a >> 16) < b) {
        asm volatile (
        "move.l %[a], %%d0\n\t"
        "move.w %[b], %%d1\n\t"
        "divu %%d1,%%d0\n\t"
        "swap %%d0\n\t"
        "andi.l #0xFFFF,%%d0\n\t"
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

/* 32-bit signed multiply — not built-in on 68000; provided here for -nostdlib builds.
 * Uses two 16×16→32 MULU operations.  Only the low 32 bits of the 64-bit result
 * are returned, which is correct for all uses in this codebase. */
int32_t NEOGEO_USER __mulsi3(int32_t a, int32_t b)
{
    uint16_t a_lo = (uint16_t)a;
    uint16_t a_hi = (uint16_t)((uint32_t)a >> 16);
    uint16_t b_lo = (uint16_t)b;
    uint16_t b_hi = (uint16_t)((uint32_t)b >> 16);
    uint32_t lo;

    asm volatile (
        "mulu.w %[b],%[a]"
        : [a] "=d" (lo)
        : "0" ((uint32_t)a_lo), [b] "d" (b_lo)
    );
    /* Add cross terms (only affect upper 16 bits of the 32-bit result) */
    lo += (uint32_t)((uint16_t)((uint16_t)(a_hi * b_lo) + (uint16_t)(a_lo * b_hi))) << 16;
    return (int32_t)lo;
}

/* 32-bit signed divide — wrap the unsigned implementation. */
int32_t NEOGEO_USER __divsi3(int32_t a, int32_t b)
{
    uint32_t ua = (uint32_t)(a < 0 ? -a : a);
    uint32_t ub = (uint32_t)(b < 0 ? -b : b);
    uint32_t r  = __udivsi3(ua, ub);
    return ((a < 0) != (b < 0)) ? -(int32_t)r : (int32_t)r;
}

/* 32-bit signed modulo. */
int32_t NEOGEO_USER __modsi3(int32_t a, int32_t b)
{
    uint32_t ua  = (uint32_t)(a < 0 ? -a : a);
    uint32_t ub  = (uint32_t)(b < 0 ? -b : b);
    uint32_t rem = __umodsi3(ua, ub);
    return (a < 0) ? -(int32_t)rem : (int32_t)rem;
}

uint16_t NEOGEO_USER setSCB2(uint16_t Xshrink , uint16_t Yshrink) {
	return (uint16_t)((Xshrink << 8) | Yshrink);
}

uint16_t NEOGEO_USER setSCB3(uint16_t Ypos , uint16_t sticky_flag , uint16_t height_factor) {
	return (uint16_t)((Ypos << 7)  |  (sticky_flag << 6) | height_factor);
}

uint16_t NEOGEO_USER setFIXDATA(uint16_t palette_index, uint16_t tilenumber) {
	return (uint16_t)((palette_index << 12) |  tilenumber);
}

uint16_t NEOGEO_USER setSCB4(uint16_t Xpos) {
	return (uint16_t)(Xpos << 7) ;
}

uint16_t NEOGEO_USER setSCB1_2(uint16_t pal_offset , uint16_t tile_offset , uint16_t bit3_autoanim , uint16_t bit2_autoanim , uint16_t vflip , uint16_t hflip ) {
	return (uint16_t)((pal_offset << 8) | (tile_offset << 4) | (bit3_autoanim << 3) | (bit2_autoanim << 2) | (vflip << 1) | hflip) ;
}

void NEOGEO_USER setBACKDROP(uint16_t backdrop_color) {
	NEO_REGISTER(BACKDROP) = backdrop_color;
}

void NEOGEO_USER load_palettes(uint16_t *p_palette, uintptr_t palette_offset) {
	uint16_t *dst = (uint16_t *)palette_offset;
	for (int i = 0; i < 16; i++) {
		dst[i] = p_palette[i];
	}
}

void NEOGEO_USER vram_init(uint16_t start,uint16_t vram_inc) {
	NEO_REGISTER(VRAM_ADDR) = start;
	NEO_REGISTER(VRAM_INC) = vram_inc;
}

void NEOGEO_USER setpal(uint16_t *pal_tile,uint16_t t0, uint16_t t1, uint16_t t2, uint16_t t3, uint16_t t4, uint16_t t5,uint16_t t6,uint16_t t7,uint16_t t8,uint16_t t9,uint16_t t10,uint16_t t11,uint16_t t12,uint16_t t13,uint16_t t14,uint16_t t15) {
	pal_tile[0] = t0; pal_tile[1] = t1; pal_tile[2] = t2; pal_tile[3] = t3;
	pal_tile[4] = t4; pal_tile[5] = t5; pal_tile[6] = t6; pal_tile[7] = t7;
	pal_tile[8] = t8; pal_tile[9] = t9; pal_tile[10] = t10; pal_tile[11] = t11;
	pal_tile[12] = t12; pal_tile[13] = t13; pal_tile[14] = t14; pal_tile[15] = t15;
}

void NEOGEO_USER vram_SCB1(uint16_t *SCB1_1 , uint16_t *SCB1_2 ,uint8_t tiles_number) {
	for (int i = 0 ; i <  tiles_number  ; i++) {
		NEO_REGISTER(VRAM_RW) = SCB1_1[i];
		NEO_REGISTER(VRAM_RW) = SCB1_2[i];
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
	vram_init(vram_start,vram_inc);
	vram_SCB1(SCB1_1,SCB1_2 ,tiles_number);
	vram_SCB234(SCB2_ADDR+vram_offset,SCB2);
	vram_SCB234(SCB3_ADDR+vram_offset,SCB3);
	vram_SCB234(SCB4_ADDR+vram_offset,SCB4);
}

void NEOGEO_USER vram_sprite_mvx(uint16_t vram_offset, uint16_t SCB4) {
	vram_SCB234(SCB4_ADDR+vram_offset,SCB4);
}

void NEOGEO_USER vram_sprite_mvy(uint16_t vram_offset, uint16_t SCB3) {
	vram_SCB234(SCB3_ADDR+vram_offset,SCB3);
}

int NEOGEO_USER strlen(const char *s) {
	const char *p = s;
	while(*s) s++;
	return (s - p);
}

void NEOGEO_USER fixtext_out(uint16_t x, uint16_t y,char *mess, short pal) {
	int len = strlen(mess);
	NEO_REGISTER(VRAM_ADDR) = FIXMAP+y+x*32;
	NEO_REGISTER(VRAM_INC) = 0x20;
	for (int i=0; i<len; i++) NEO_REGISTER(VRAM_RW) = (uint16_t)((pal << 12) | mess[i]);
}

void NEOGEO_USER fixtext_out1(uint16_t x, uint16_t y,uint16_t *mess,short pal,int objsz) {
	NEO_REGISTER(VRAM_ADDR) = FIXMAP+y+x*32;
	NEO_REGISTER(VRAM_INC) = 0x20;
	for (int i=0; i<objsz; i++) NEO_REGISTER(VRAM_RW) = (uint16_t)((pal << 12) | mess[i]);
}

void NEOGEO_USER fixtext_out2(uint16_t x, uint16_t y,uint16_t a, uint16_t b, uint16_t c,uint16_t mod,uint16_t *mess,short pal,int objsz) {
	NEO_REGISTER(VRAM_ADDR) = FIXMAP+a*x+b*y+c;
	NEO_REGISTER(VRAM_INC) = mod;
	for (int i=0; i<objsz; i++) NEO_REGISTER(VRAM_RW) = (uint16_t)((pal << 12) | mess[i]);
}

void NEOGEO_USER mess_outtest(void) {
	setBIOSMESSBusy();
	uint16_t *pmessp = (uint16_t *)NEO_REGISTER32(BIOS_MESS_POINT);
	uint16_t *ptr = (uint16_t *)RAMSTART;
	*ptr++ =0x434F; *ptr++ =0x4D4D; *ptr++ =0x414E; *ptr++ =0x4420; *ptr++ =0x34FF;
	*ptr++ =0x434F; *ptr++ =0x4D4D; *ptr++ =0x414E; *ptr++ =0x4420; *ptr++ =0x36FF;
	*ptr++ =0x0;
	NEO_REGISTER32(BIOS_MESS_POINT) = (uint32_t)pmessp;
	setBIOSMESSReady();
	CALLNEOGEOF(SYS_MESS_OUT);
}

void NEOGEO_USER setBIOSMESSBusy(void) { NEO_REGISTER8(BIOS_MESS_BUSY) += 1; }
void NEOGEO_USER setBIOSMESSReady(void) { NEO_REGISTER8(BIOS_MESS_BUSY) -= 1; }

void NEOGEO_USER clearRAM() {
	ASM_START
	ASM_MVL(#0x3CC-1, %%d7)
	ASM_LEA(RAMSTART, %%a0)
	ASM_MVQ(#0, %%d0)
	ASM_L(.clram)
	ASM_MVL(%%d0, (%%a0)+)
	ASM_MVL(%%d0, (%%a0)+)
	ASM_MVL(%%d0, (%%a0)+)
	ASM_MVL(%%d0, (%%a0)+)
	ASM_MVL(%%d0, (%%a0)+)
	ASM_MVL(%%d0, (%%a0)+)
	ASM_MVL(%%d0, (%%a0)+)
	ASM_MVL(%%d0, (%%a0)+)
	ASM_DBRA(%%d7, .clram)
	: : : "d0", "d7", "a0", "memory"
	ASM_END
}

void NEOGEO_USER clearSprs() {
	ASM_START
	ASM_MVW(#SCB3_ADDR, VRAM_ADDR)
	ASM_CLRW(%%d0)
	ASM_MVW(#1, VRAM_INC)
	ASM_MVL(#512-1, %%d7)
	ASM_L(.clspr)
	ASM_MVW(%%d0, VRAM_RW)
	ASM_DBRA(%%d7, .clspr)
	: : : "d0", "d7", "memory"
	ASM_END
}

void NEOGEO_USER clearFix() {
	ASM_START
	ASM_JSR(SYS_FIX_CLEAR)
	
	ASM_MVL(#1280-1, %%d7)
	ASM_MVW(#FIXMAP, VRAM_ADDR)
	ASM_MVW(#0xFF, %%d0)
	ASM_L(.clfix)
	ASM_MVW(%%d0, VRAM_RW)
	ASM_DBRA(%%d7, .clfix)
	: : : "d0", "d7", "memory"
	ASM_END
}

void NEOGEO_USER waitVbl() {
	ASM_START
	ASM_L(.waitv)
	ASM_TSTW(USER_WORKRAM)
	ASM_JEQ(.waitv)
	ASM_CLRW(USER_WORKRAM)
	ASM_ADDQL(#1, USER_WORKRAM+32)
	ASM_END
}

void NEOGEO_USER cycle10ms() {
	ASM_START
	ASM_MVW(#2400, %%d0)
	ASM_L(.d10)
	ASM_DBF(%%d0, .d10)
	: : : "d0"
	ASM_END
}

void NEOGEO_USER cycle1s() {
	ASM_START
	ASM_MVW(#5000, %%d1)
	ASM_L(.c1s)
	ASM_MVW(#240, %%d0)
	ASM_L(.d1s)
	ASM_DBF(%%d0, .d1s)
	ASM_SUBQ(#1, %%d1)
	ASM_BNE(.c1s)
	: : : "d0", "d1"
	ASM_END
}

void NEOGEO_USER cyclexms1(int cyc1) {
	ASM_START
	ASM_MVL(%[cyc1], %%d1)
	ASM_MVML(%%d0, -(%%sp))
	ASM_L(.cxms1)
	ASM_MVW(#240, %%d0)
	ASM_L(.dxms1)
	ASM_DBF(%%d0, .dxms1)
	ASM_SUBQ(#1, %%d1)
	ASM_BNE(.cxms1)
	ASM_MVML((%%sp)+, %%d0)
	: : [cyc1] "g" (cyc1) : "cc"
	ASM_END
}

void NEOGEO_USER cyclexs(int cyc1xs) {
	ASM_START
	ASM_MVL(%[cyc1xs], %%d1)
	ASM_MVML(%%d0/%%d2, -(%%sp))
	ASM_L(.cxs)
	ASM_MVW(#5000, %%d2)
	ASM_L(.cxs_sub)
	ASM_MVW(#240, %%d0)
	ASM_L(.dxs)
	ASM_DBF(%%d0, .dxs)
	ASM_SUBQ(#1, %%d2)
	ASM_BNE(.cxs_sub)
	ASM_SUBQ(#1, %%d1)
	ASM_BNE(.cxs)
	ASM_MVML((%%sp)+, %%d0/%%d2)
	: : [cyc1xs] "g" (cyc1xs) : "cc"
	ASM_END
}

  void NEOGEO_USER cyclexms(int cycxms) {
      ASM_START
      ASM_MVL(%[cycxms], %%d1)
      ASM_MVML(%%d0/%%d2, -(%%sp))
      ASM_L(.cxms)
      ASM_MVW(#50, %%d2)
      ASM_L(.cxms_sub)
      ASM_MVW(#240, %%d0)
      ASM_L(.dxms)
      ASM_DBF(%%d0, .dxms)
      ASM_SUBQ(#1, %%d2)
      ASM_BNE(.cxms_sub)
      ASM_SUBQ(#1, %%d1)
      ASM_BNE(.cxms)
      ASM_MVML((%%sp)+, %%d0/%%d2)
      : : [cycxms] "g" (cycxms) : "cc"
      ASM_END
  }

uint16_t NEOGEO_USER poll_joystick() {
	uint8_t d1 = *(volatile uint8_t *)BIOS_P1CURRENT;
	uint8_t d2 = *(volatile uint8_t *)BIOS_STATCURNT;
	return (uint16_t)(((uint16_t)d2 << 8) | d1);
}

uint16_t NEOGEO_USER poll_joystick_edge() {
	uint8_t d1 = *(volatile uint8_t *)BIOS_P1CHANGE;
	uint8_t d2 = *(volatile uint8_t *)BIOS_STATCHANGE;
	return (uint16_t)(((uint16_t)d2 << 8) | d1);
}

uint16_t NEOGEO_USER read_joy1(void) {
	return poll_joystick();
}

void NEOGEO_USER fix_svalue1(uint16_t X, uint16_t Y,uint16_t v,short pal,uint16_t offset) {
	vram_sfix(0x20, FIXMAP+(Y+2+((X+1)*32)), (uint16_t)((pal << 12) | (v + offset)));
}

void NEOGEO_USER fix_svalue(uint16_t X, uint16_t Y,uint16_t v,short pal) {
	vram_sfix(0x20, FIXMAP+(Y+2+((X+1)*32)), (uint16_t)((pal << 12) | v));
}

void NEOGEO_USER setsfix() { ASM_START ASM_BSETB(#0, REG_BRDFIX) ASM_END }

void NEOGEO_USER display_digit(uint16_t X, uint16_t Y,uint32_t value,short pal,uint16_t offset) {
	uint16_t s[10]; int objsz = 0; uint32_t power = 1UL;
	if (X < 2) X = 2;
	if (X > 37) X = 37;
	if (Y < 1) Y = 1;
	if (Y > 26) Y = 26;
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

int NEOGEO_USER read_p1credit(void) { return *(volatile uint8_t *)P1_CREDITS; }

void NEOGEO_USER playSoundtest(uint16_t index) { isZ80Ready(); soundCommand((uint8_t)(index & 0xFF)); }
void NEOGEO_USER soundCommand(uint8_t command) { isZ80Ready(); NEO_REGISTER8(REG_SOUND) = command; isZ80Ready(); }
void NEOGEO_USER soundInit(void) { soundCommand(0x01); }
void NEOGEO_USER soundReset(void) { soundCommand(0x03); }
void NEOGEO_USER soundStopAll(void) { soundCommand(0x04); }
void NEOGEO_USER soundStopMusic(void) { soundCommand(0x0F); }
void NEOGEO_USER soundCancelFade(void) { soundCommand(0x11); }
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
	ASM_START
	ASM_L(.isready)
	ASM_MVB(#0,0x300001)
	ASM_MVB(0x320000,%%d0)
	ASM_CMPB(#0x01,%%d0)
	ASM_BNE(.isready)
	: : : "d0"
	ASM_END
}

void NEOGEO_USER kickWatchDog(void) { ASM_START ASM_MVB(%%d0, REG_DIPSW) : : : "d0" ASM_END }
void NEOGEO_USER sleep1FFF(void) { ASM_START ASM_MVW(#0x1FFF,%%d0) ASM_L(.slp) ASM_NOP ASM_DBRA(%%d0,.slp) : : : "d0" ASM_END }

void NEOGEO_USER displayCreditP1(void) {
	ASM_START
	ASM_MVQ(#0,%%d0)
	ASM_MVB(P1_CREDITS,%%d0)
	ASM_MVW(#0x7088,VRAM_ADDR)
	ASM_MVW(%%d0,%%d1)
	ASM_MVW(#0x20,VRAM_INC)
	ASM_LSRB(#4,%%d1)
	ASM_ANDIW(#0x000F,%%d1)
	ASM_ORIW(#0x0030,%%d1)
	ASM_MVW(%%d1,VRAM_RW)
	ASM_ANDIW(#0x000F,%%d0)
	ASM_ORIW(#0x0030,%%d0)
	ASM_MVW(%%d0,VRAM_RW)
	: : : "d0", "d1", "memory"
	ASM_END
}

void NEOGEO_USER displayCreditP2(void) {
	ASM_START
	ASM_MVQ(#0,%%d0)
	ASM_MVB(P2_CREDITS,%%d0)
	ASM_MVW(#0x7288,VRAM_ADDR)
	ASM_MVW(%%d0,%%d1)
	ASM_MVW(#0x20,VRAM_INC)
	ASM_LSRB(#4,%%d1)
	ASM_ANDIW(#0x000F,%%d1)
	ASM_ORIW(#0x0030,%%d1)
	ASM_MVW(%%d1,VRAM_RW)
	ASM_ANDIW(#0x000F,%%d0)
	ASM_ORIW(#0x0030,%%d0)
	ASM_MVW(%%d0,VRAM_RW)
	: : : "d0", "d1", "memory"
	ASM_END
}

void *memcpy(void *dest, const void *src, int count) {
    char *d = (char *)dest; const char *s = (const char *)src;
    while (count--) *d++ = *s++;
    return dest;
}
