#include <stdint.h>
#include "macro.h"
#include "sound_ids.h"
#ifdef __cplusplus
/* A USE_2D_PLUS build compiles this file as C++.  Everything here is
 * reached from inline asm, the cart entry vectors or the BIOS by its
 * plain symbol name, so it must keep C linkage and not be mangled. */
extern "C" {
#endif


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
void  mess_out(uint16_t, uint16_t,const char *,short);
void  mess_out_clipped(uint16_t, uint16_t,const char *,short,uint16_t);
void  mess_out_vram(uint16_t, uint16_t,const char *,short,uint16_t);
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
void playSSGVoiceGetReady(void);
void playSSGVoiceLetsGo(void);
void playSSGVoiceGameOver(void);
void playGetReadyVoice(void);
void playAttackVoice(void);
void playVoiceGetReady(void);
void playVoiceLetsGo(void);
void playVoiceGameOver(void);
void playVoiceSample(uint8_t sample_index);
void playVoiceWord(uint8_t word_sample);
void playVoiceNumber(uint16_t value);
void speakText(const char *text);
void soundSetADPCMBPan(uint8_t pan);
void soundFMSetLFO(uint8_t rate_enable);
void soundSetSSGNoise(uint8_t period);
void soundFMSetTempo(uint8_t period_frames);
void playVoiceLetter(uint8_t letter_index);
void speakWord(const char *text);
void soundFMCSMBegin(uint8_t period_hi);
void soundFMCSMEnd(void);
void soundFMCSMSweep(uint8_t hi_start, uint8_t hi_end, uint8_t step_ms);
void playFMSpeechRobot(void);
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
    if (b <= 0xFFFF && a < 0x7FFF0000UL && (a >> 16) < b) {
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
    if (b <= 0xFFFF && a < 0x7FFF0000UL && (a >> 16) < b) {
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

/*
 * The FIX map is 40x32 cells but the CRT only shows 28 of those rows:
 * map rows 0, 1, 30 and 31 sit in the vertical blanking area and never
 * reach the screen.  Every helper below addresses the *visible* grid -
 * row 0 is the top line you can actually see - so the map row is
 * y + FIX_ROW0.  fix_svalue()/fix_svalue1() further down always used
 * this convention; the text helpers did not, which pushed every line
 * two rows up the screen: text written to row 0 landed in the blanking
 * area and never appeared, and the bottom two visible rows could not be
 * addressed at all.
 */
#define FIX_ROW0    2u
#define FIX_ROWS    28u

void NEOGEO_USER fixtext_out(uint16_t x, uint16_t y,char *mess, short pal) {
	int len = strlen(mess);
	NEO_REGISTER(VRAM_ADDR) = FIXMAP+y+FIX_ROW0+x*32;
	NEO_REGISTER(VRAM_INC) = 0x20;
	for (int i=0; i<len; i++) NEO_REGISTER(VRAM_RW) = (uint16_t)((pal << 12) | mess[i]);
}

static uint16_t NEOGEO_USER mess_out_strlen_clipped(const char *text, uint16_t max_chars)
{
	uint16_t len = 0;

	if (!text) return 0;
	if (max_chars > 240u) max_chars = 240u;

	while (len < max_chars && text[len]) len++;
	return len;
}

void NEOGEO_USER mess_out_vram(uint16_t vram_addr, uint16_t vram_inc,
                               const char *text, short pal, uint16_t max_chars)
{
	uint16_t len = mess_out_strlen_clipped(text, max_chars);
	uint16_t word_buffer[240];
	uint16_t cell_pal;
	uint16_t inc_cmd;
	uint32_t data_addr;
	uint16_t i;

	if (!len) return;

	cell_pal = (uint16_t)(((uint16_t)pal & 0x000Fu) << 12);
	for (i = 0; i < len; i++) {
		uint8_t ch = (uint8_t)text[i];
		word_buffer[i] = (ch == ' ') ? 0x00FFu : (uint16_t)(cell_pal | ch);
	}

	inc_cmd = (uint16_t)(((vram_inc & 0x00FFu) << 8) | 0x0002u);
	data_addr = (uint32_t)(uintptr_t)word_buffer;

	asm volatile (
		"movem.l %%d0-%%d7/%%a0-%%a6,-(%%sp)\n\t"
		"addq.b #1,%c[busy]\n\t"
		"movea.l #%c[buffer],%%a0\n\t"
		"clr.l (%%a0)+\n\t"
		"move.w #%c[cmd1],(%%a0)+\n\t"
		"move.w %[len],(%%a0)+\n\t"
		"move.w %[inc_cmd],(%%a0)+\n\t"
		"move.w #%c[cmd3],(%%a0)+\n\t"
		"move.w %[vram_addr],(%%a0)+\n\t"
		"move.w #%c[cmd4],(%%a0)+\n\t"
		"move.l %[data_addr],(%%a0)+\n\t"
		"clr.w (%%a0)+\n\t"
		"move.l %%a0,%c[point]\n\t"
		"subq.b #1,%c[busy]\n\t"
		"jsr %c[sys_mess]\n\t"
		"movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a6\n\t"
		:
		: [len] "d" (len),
		  [inc_cmd] "d" (inc_cmd),
		  [vram_addr] "d" (vram_addr),
		  [data_addr] "d" (data_addr),
		  [busy] "i" (BIOS_MESS_BUSY),
		  [buffer] "i" (BIOS_MESS_BUFFER),
		  [point] "i" (BIOS_MESS_POINT),
		  [sys_mess] "i" (SYS_MESS_OUT),
		  [cmd1] "i" (COMMAND1WL),
		  [cmd3] "i" (COMMAND3),
		  [cmd4] "i" (COMMAND4)
		: "cc", "memory"
	);
}

void NEOGEO_USER mess_out_clipped(uint16_t x, uint16_t y, const char *text,
                                  short pal, uint16_t max_chars)
{
	if (!text || x >= 40u || y >= FIX_ROWS) return;
	if (max_chars > (uint16_t)(40u - x)) max_chars = (uint16_t)(40u - x);
	mess_out_vram((uint16_t)(FIXMAP + y + FIX_ROW0 + x * 32u), 0x20u, text, pal, max_chars);
}

void NEOGEO_USER mess_out(uint16_t x, uint16_t y, const char *text, short pal)
{
	if (x >= 40u) return;
	mess_out_clipped(x, y, text, pal, (uint16_t)(40u - x));
}

void NEOGEO_USER fixtext_out1(uint16_t x, uint16_t y,uint16_t *mess,short pal,int objsz) {
	NEO_REGISTER(VRAM_ADDR) = FIXMAP+y+FIX_ROW0+x*32;
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
void NEOGEO_USER soundCommand(uint8_t command) { isZ80Ready(); NEO_REGISTER8(REG_SOUND) = command; }
void NEOGEO_USER soundInit(void) { soundCommand(0x01); }
void NEOGEO_USER soundReset(void) { soundCommand(0x03); }
void NEOGEO_USER soundStopAll(void) { soundCommand(0x04); }
void NEOGEO_USER soundStopMusic(void) { soundCommand(0x0F); }
void NEOGEO_USER soundCancelFade(void) { soundCommand(0x11); }
void NEOGEO_USER soundSceneReset(void) { isZ80Ready(); soundStopAll(); cyclexms(4); isZ80Ready(); soundReset(); cyclexms(4); }
void NEOGEO_USER playMusic(uint8_t n) { isZ80Ready(); soundCommand(0x20 + n); }
void NEOGEO_USER playSFX(uint8_t n) {
	isZ80Ready();
	if (n < 0x40u) {
		soundCommand((uint8_t)(0x40u + n));
	} else {
		soundCommand(0x16u);
		isZ80Ready();
		soundCommand(n);
	}
}
void NEOGEO_USER playSFXB(uint8_t n) { isZ80Ready(); soundCommand(0x80 + n); }
void NEOGEO_USER playFMDebug(void) { isZ80Ready(); soundCommand(0x30); }
void NEOGEO_USER playFMTrack(uint8_t n) { isZ80Ready(); soundCommand(0x31); isZ80Ready(); soundCommand(n); }
void NEOGEO_USER soundSetFMVolume(uint8_t v) { isZ80Ready(); soundCommand(0x13); isZ80Ready(); soundCommand(v & 0x0F); }
void NEOGEO_USER playSSGTrack(uint8_t n) { isZ80Ready(); soundCommand(0x32); isZ80Ready(); soundCommand(n); }
void NEOGEO_USER soundSetSSGPreset(uint8_t preset) { isZ80Ready(); soundCommand(0x14); isZ80Ready(); soundCommand(preset & 0x0F); }
void NEOGEO_USER playInsertCoinSSG(void) { isZ80Ready(); soundSetSSGPreset(1u); isZ80Ready(); playSSGTrack(SOUND_SSG_C); }
void NEOGEO_USER playSSGVoiceGetReady(void) { isZ80Ready(); soundSetSSGPreset(4u); isZ80Ready(); playSSGTrack(SOUND_SSG_E); }
void NEOGEO_USER playSSGVoiceLetsGo(void) { isZ80Ready(); soundSetSSGPreset(3u); isZ80Ready(); playSSGTrack(SOUND_SSG_F); }
void NEOGEO_USER playSSGVoiceGameOver(void) { isZ80Ready(); soundSetSSGPreset(4u); isZ80Ready(); playSSGTrack(SOUND_SSG_G); }

void NEOGEO_USER playVoiceCue(uint8_t n) {
	isZ80Ready();
	switch (n) {
		case SOUND_VOICE_1: playVoiceGetReady(); break;
		case SOUND_VOICE_2: playAttackVoice(); break;
		default: playSFX(n); break;
	}
}

void NEOGEO_USER playVoiceSample(uint8_t sample_index) {
	playSFX(sample_index);
}

void NEOGEO_USER playGetReadyVoice(void) { playVoiceGetReady(); }
void NEOGEO_USER playAttackVoice(void) { isZ80Ready(); playSFX(SOUND_SFX_12); }

void NEOGEO_USER playCoinThenReady(void) {
	isZ80Ready(); playInsertCoinSSG(); cyclexms(250); isZ80Ready(); playGetReadyVoice();
}

static uint8_t NEOGEO_USER voice_letter_sample(char c) {
	switch (c) {
#ifdef SOUND_VOICE_LETTER_A
		case 'A': case 'a': return SOUND_VOICE_LETTER_A;
#endif
#ifdef SOUND_VOICE_LETTER_B
		case 'B': case 'b': return SOUND_VOICE_LETTER_B;
#endif
#ifdef SOUND_VOICE_LETTER_C
		case 'C': case 'c': return SOUND_VOICE_LETTER_C;
#endif
#ifdef SOUND_VOICE_LETTER_D
		case 'D': case 'd': return SOUND_VOICE_LETTER_D;
#endif
#ifdef SOUND_VOICE_LETTER_E
		case 'E': case 'e': return SOUND_VOICE_LETTER_E;
#endif
#ifdef SOUND_VOICE_LETTER_F
		case 'F': case 'f': return SOUND_VOICE_LETTER_F;
#endif
#ifdef SOUND_VOICE_LETTER_G
		case 'G': case 'g': return SOUND_VOICE_LETTER_G;
#endif
#ifdef SOUND_VOICE_LETTER_H
		case 'H': case 'h': return SOUND_VOICE_LETTER_H;
#endif
#ifdef SOUND_VOICE_LETTER_I
		case 'I': case 'i': return SOUND_VOICE_LETTER_I;
#endif
#ifdef SOUND_VOICE_LETTER_J
		case 'J': case 'j': return SOUND_VOICE_LETTER_J;
#endif
#ifdef SOUND_VOICE_LETTER_K
		case 'K': case 'k': return SOUND_VOICE_LETTER_K;
#endif
#ifdef SOUND_VOICE_LETTER_L
		case 'L': case 'l': return SOUND_VOICE_LETTER_L;
#endif
#ifdef SOUND_VOICE_LETTER_M
		case 'M': case 'm': return SOUND_VOICE_LETTER_M;
#endif
#ifdef SOUND_VOICE_LETTER_N
		case 'N': case 'n': return SOUND_VOICE_LETTER_N;
#endif
#ifdef SOUND_VOICE_LETTER_O
		case 'O': case 'o': return SOUND_VOICE_LETTER_O;
#endif
#ifdef SOUND_VOICE_LETTER_P
		case 'P': case 'p': return SOUND_VOICE_LETTER_P;
#endif
#ifdef SOUND_VOICE_LETTER_Q
		case 'Q': case 'q': return SOUND_VOICE_LETTER_Q;
#endif
#ifdef SOUND_VOICE_LETTER_R
		case 'R': case 'r': return SOUND_VOICE_LETTER_R;
#endif
#ifdef SOUND_VOICE_LETTER_S
		case 'S': case 's': return SOUND_VOICE_LETTER_S;
#endif
#ifdef SOUND_VOICE_LETTER_T
		case 'T': case 't': return SOUND_VOICE_LETTER_T;
#endif
#ifdef SOUND_VOICE_LETTER_U
		case 'U': case 'u': return SOUND_VOICE_LETTER_U;
#endif
#ifdef SOUND_VOICE_LETTER_V
		case 'V': case 'v': return SOUND_VOICE_LETTER_V;
#endif
#ifdef SOUND_VOICE_LETTER_W
		case 'W': case 'w': return SOUND_VOICE_LETTER_W;
#endif
#ifdef SOUND_VOICE_LETTER_X
		case 'X': case 'x': return SOUND_VOICE_LETTER_X;
#endif
#ifdef SOUND_VOICE_LETTER_Y
		case 'Y': case 'y': return SOUND_VOICE_LETTER_Y;
#endif
#ifdef SOUND_VOICE_LETTER_Z
		case 'Z': case 'z': return SOUND_VOICE_LETTER_Z;
#endif
		default: return 0xFFu;
	}
}

static char NEOGEO_USER voice_upper(char c) {
	if (c >= 'a' && c <= 'z') return (char)(c - ('a' - 'A'));
	return c;
}

/* Every caller sits behind a SOUND_VOICE_WORD_* guard, so a sound set that
 * defines none of them leaves this with no users.  That is a valid
 * configuration, not a mistake worth a warning. */
static uint8_t NEOGEO_USER voice_match_phrase(const char *text,
                                              const char *phrase) __attribute__((unused));
static uint8_t NEOGEO_USER voice_match_phrase(const char *text,
                                              const char *phrase) {
	uint8_t i = 0u;
	while (phrase[i]) {
		if (voice_upper(text[i]) != phrase[i]) return 0u;
		i++;
	}
	return i;
}

static uint8_t NEOGEO_USER voice_direct_phrase(const char *text,
                                               uint8_t *sample,
                                               uint8_t *advance) {
	/* Unused when the sound set defines no voice words at all. */
	uint8_t n __attribute__((unused));

#ifdef SOUND_VOICE_WORD_INSERT_COIN
	n = voice_match_phrase(text, "INSERT COIN");
	if (n) { *sample = SOUND_VOICE_WORD_INSERT_COIN; *advance = n; return 1u; }
#endif
#ifdef SOUND_VOICE_WORD_GAME_OVER
	n = voice_match_phrase(text, "GAME OVER");
	if (n) { *sample = SOUND_VOICE_WORD_GAME_OVER; *advance = n; return 1u; }
#endif
#ifdef SOUND_VOICE_WORD_CONTINUE
	n = voice_match_phrase(text, "CONTINUE");
	if (n) { *sample = SOUND_VOICE_WORD_CONTINUE; *advance = n; return 1u; }
#endif
#ifdef SOUND_VOICE_WORD_PLAYER
	n = voice_match_phrase(text, "PLAYER");
	if (n) { *sample = SOUND_VOICE_WORD_PLAYER; *advance = n; return 1u; }
#endif
#ifdef SOUND_VOICE_WORD_READY
	n = voice_match_phrase(text, "READY");
	if (n) { *sample = SOUND_VOICE_WORD_READY; *advance = n; return 1u; }
#endif
#ifdef SOUND_VOICE_WORD_START
	n = voice_match_phrase(text, "START");
	if (n) { *sample = SOUND_VOICE_WORD_START; *advance = n; return 1u; }
#endif
#ifdef SOUND_VOICE_WORD_STAGE
	n = voice_match_phrase(text, "STAGE");
	if (n) { *sample = SOUND_VOICE_WORD_STAGE; *advance = n; return 1u; }
#endif
#ifdef SOUND_VOICE_WORD_LEVEL
	n = voice_match_phrase(text, "LEVEL");
	if (n) { *sample = SOUND_VOICE_WORD_LEVEL; *advance = n; return 1u; }
#endif
#ifdef SOUND_VOICE_WORD_SCORE
	n = voice_match_phrase(text, "SCORE");
	if (n) { *sample = SOUND_VOICE_WORD_SCORE; *advance = n; return 1u; }
#endif
#ifdef SOUND_VOICE_WORD_GO
	n = voice_match_phrase(text, "GO");
	if (n) { *sample = SOUND_VOICE_WORD_GO; *advance = n; return 1u; }
#endif
	return 0u;
}

static uint8_t NEOGEO_USER voice_char_sample(char c) {
	switch (c) {
#ifdef SOUND_VOICE_NUM_0_ZERO
		case '0': return SOUND_VOICE_NUM_0_ZERO;
#endif
#ifdef SOUND_VOICE_NUM_1_ONE
		case '1': return SOUND_VOICE_NUM_1_ONE;
#endif
#ifdef SOUND_VOICE_NUM_2_TWO
		case '2': return SOUND_VOICE_NUM_2_TWO;
#endif
#ifdef SOUND_VOICE_NUM_3_THREE
		case '3': return SOUND_VOICE_NUM_3_THREE;
#endif
#ifdef SOUND_VOICE_NUM_4_FOUR
		case '4': return SOUND_VOICE_NUM_4_FOUR;
#endif
#ifdef SOUND_VOICE_NUM_5_FIVE
		case '5': return SOUND_VOICE_NUM_5_FIVE;
#endif
#ifdef SOUND_VOICE_NUM_6_SIX
		case '6': return SOUND_VOICE_NUM_6_SIX;
#endif
#ifdef SOUND_VOICE_NUM_7_SEVEN
		case '7': return SOUND_VOICE_NUM_7_SEVEN;
#endif
#ifdef SOUND_VOICE_NUM_8_EIGHT
		case '8': return SOUND_VOICE_NUM_8_EIGHT;
#endif
#ifdef SOUND_VOICE_NUM_9_NINE
		case '9': return SOUND_VOICE_NUM_9_NINE;
#endif
#ifdef SOUND_VOICE_CHAR_SPACE
		case ' ': return SOUND_VOICE_CHAR_SPACE;
#endif
#ifdef SOUND_VOICE_CHAR_EXCLAMATION_MARK
		case '!': return SOUND_VOICE_CHAR_EXCLAMATION_MARK;
#endif
#ifdef SOUND_VOICE_CHAR_QUESTION_MARK
		case '?': return SOUND_VOICE_CHAR_QUESTION_MARK;
#endif
#ifdef SOUND_VOICE_CHAR_COMMA
		case ',': return SOUND_VOICE_CHAR_COMMA;
#endif
#ifdef SOUND_VOICE_CHAR_COLON
		case ':': return SOUND_VOICE_CHAR_COLON;
#endif
#ifdef SOUND_VOICE_CHAR_SEMICOLON
		case ';': return SOUND_VOICE_CHAR_SEMICOLON;
#endif
#ifdef SOUND_VOICE_CHAR_DOT
		case '.': return SOUND_VOICE_CHAR_DOT;
#endif
#ifdef SOUND_VOICE_CHAR_DASH
		case '-': return SOUND_VOICE_CHAR_DASH;
#endif
#ifdef SOUND_VOICE_CHAR_SLASH
		case '/': return SOUND_VOICE_CHAR_SLASH;
#endif
#ifdef SOUND_VOICE_CHAR_BACKSLASH
		case '\\': return SOUND_VOICE_CHAR_BACKSLASH;
#endif
#ifdef SOUND_VOICE_CHAR_PLUS
		case '+': return SOUND_VOICE_CHAR_PLUS;
#endif
#ifdef SOUND_VOICE_CHAR_STAR
		case '*': return SOUND_VOICE_CHAR_STAR;
#endif
#ifdef SOUND_VOICE_CHAR_EQUAL
		case '=': return SOUND_VOICE_CHAR_EQUAL;
#endif
#ifdef SOUND_VOICE_CHAR_PERCENT
		case '%': return SOUND_VOICE_CHAR_PERCENT;
#endif
#ifdef SOUND_VOICE_CHAR_HASH
		case '#': return SOUND_VOICE_CHAR_HASH;
#endif
#ifdef SOUND_VOICE_CHAR_AT
		case '@': return SOUND_VOICE_CHAR_AT;
#endif
#ifdef SOUND_VOICE_CHAR_AMPERSAND
		case '&': return SOUND_VOICE_CHAR_AMPERSAND;
#endif
#ifdef SOUND_VOICE_CHAR_UNDERSCORE
		case '_': return SOUND_VOICE_CHAR_UNDERSCORE;
#endif
#ifdef SOUND_VOICE_CHAR_QUOTE
		case '"': case '\'': return SOUND_VOICE_CHAR_QUOTE;
#endif
#ifdef SOUND_VOICE_CHAR_OPEN_PARENTHESIS
		case '(': return SOUND_VOICE_CHAR_OPEN_PARENTHESIS;
#endif
#ifdef SOUND_VOICE_CHAR_CLOSE_PARENTHESIS
		case ')': return SOUND_VOICE_CHAR_CLOSE_PARENTHESIS;
#endif
		default: return 0xFFu;
	}
}

void NEOGEO_USER playVoiceWord(uint8_t word_sample) {
	playVoiceSample(word_sample);
}

void NEOGEO_USER playVoiceNumber(uint16_t value) {
	uint8_t sample = 0xFFu;

	switch (value) {
#ifdef SOUND_VOICE_NUM_0_ZERO
		case 0u: sample = SOUND_VOICE_NUM_0_ZERO; break;
#endif
#ifdef SOUND_VOICE_NUM_1_ONE
		case 1u: sample = SOUND_VOICE_NUM_1_ONE; break;
#endif
#ifdef SOUND_VOICE_NUM_2_TWO
		case 2u: sample = SOUND_VOICE_NUM_2_TWO; break;
#endif
#ifdef SOUND_VOICE_NUM_3_THREE
		case 3u: sample = SOUND_VOICE_NUM_3_THREE; break;
#endif
#ifdef SOUND_VOICE_NUM_4_FOUR
		case 4u: sample = SOUND_VOICE_NUM_4_FOUR; break;
#endif
#ifdef SOUND_VOICE_NUM_5_FIVE
		case 5u: sample = SOUND_VOICE_NUM_5_FIVE; break;
#endif
#ifdef SOUND_VOICE_NUM_6_SIX
		case 6u: sample = SOUND_VOICE_NUM_6_SIX; break;
#endif
#ifdef SOUND_VOICE_NUM_7_SEVEN
		case 7u: sample = SOUND_VOICE_NUM_7_SEVEN; break;
#endif
#ifdef SOUND_VOICE_NUM_8_EIGHT
		case 8u: sample = SOUND_VOICE_NUM_8_EIGHT; break;
#endif
#ifdef SOUND_VOICE_NUM_9_NINE
		case 9u: sample = SOUND_VOICE_NUM_9_NINE; break;
#endif
#ifdef SOUND_VOICE_NUM_10_TEN
		case 10u: sample = SOUND_VOICE_NUM_10_TEN; break;
#endif
#ifdef SOUND_VOICE_NUM_11_ELEVEN
		case 11u: sample = SOUND_VOICE_NUM_11_ELEVEN; break;
#endif
#ifdef SOUND_VOICE_NUM_12_TWELVE
		case 12u: sample = SOUND_VOICE_NUM_12_TWELVE; break;
#endif
#ifdef SOUND_VOICE_NUM_13_THIRTEEN
		case 13u: sample = SOUND_VOICE_NUM_13_THIRTEEN; break;
#endif
#ifdef SOUND_VOICE_NUM_14_FOURTEEN
		case 14u: sample = SOUND_VOICE_NUM_14_FOURTEEN; break;
#endif
#ifdef SOUND_VOICE_NUM_15_FIFTEEN
		case 15u: sample = SOUND_VOICE_NUM_15_FIFTEEN; break;
#endif
#ifdef SOUND_VOICE_NUM_16_SIXTEEN
		case 16u: sample = SOUND_VOICE_NUM_16_SIXTEEN; break;
#endif
#ifdef SOUND_VOICE_NUM_17_SEVENTEEN
		case 17u: sample = SOUND_VOICE_NUM_17_SEVENTEEN; break;
#endif
#ifdef SOUND_VOICE_NUM_18_EIGHTEEN
		case 18u: sample = SOUND_VOICE_NUM_18_EIGHTEEN; break;
#endif
#ifdef SOUND_VOICE_NUM_19_NINETEEN
		case 19u: sample = SOUND_VOICE_NUM_19_NINETEEN; break;
#endif
#ifdef SOUND_VOICE_NUM_20_TWENTY
		case 20u: sample = SOUND_VOICE_NUM_20_TWENTY; break;
#endif
#ifdef SOUND_VOICE_NUM_30_THIRTY
		case 30u: sample = SOUND_VOICE_NUM_30_THIRTY; break;
#endif
#ifdef SOUND_VOICE_NUM_40_FORTY
		case 40u: sample = SOUND_VOICE_NUM_40_FORTY; break;
#endif
#ifdef SOUND_VOICE_NUM_50_FIFTY
		case 50u: sample = SOUND_VOICE_NUM_50_FIFTY; break;
#endif
#ifdef SOUND_VOICE_NUM_60_SIXTY
		case 60u: sample = SOUND_VOICE_NUM_60_SIXTY; break;
#endif
#ifdef SOUND_VOICE_NUM_70_SEVENTY
		case 70u: sample = SOUND_VOICE_NUM_70_SEVENTY; break;
#endif
#ifdef SOUND_VOICE_NUM_80_EIGHTY
		case 80u: sample = SOUND_VOICE_NUM_80_EIGHTY; break;
#endif
#ifdef SOUND_VOICE_NUM_90_NINETY
		case 90u: sample = SOUND_VOICE_NUM_90_NINETY; break;
#endif
#ifdef SOUND_VOICE_NUM_100_HUNDRED
		case 100u: sample = SOUND_VOICE_NUM_100_HUNDRED; break;
#endif
#ifdef SOUND_VOICE_NUM_1000_THOUSAND
		case 1000u: sample = SOUND_VOICE_NUM_1000_THOUSAND; break;
#endif
		default: break;
	}

	if (sample != 0xFFu) {
		playVoiceSample(sample);
		return;
	}

	{
		char digits[6];
		uint8_t len = 0u;
		uint8_t i;
		uint16_t v = value;

		while (v > 0u && len < 5u) {
			digits[len++] = (char)('0' + (v % 10u));
			v /= 10u;
		}
		for (i = 0u; i < len; i++) {
			uint8_t d = (uint8_t)(digits[(uint8_t)(len - 1u - i)] - '0');
			sample = voice_char_sample((char)('0' + d));
			if (sample != 0xFFu) playVoiceSample(sample);
			cyclexms(45);
		}
	}
}

void NEOGEO_USER playVoiceGetReady(void) { speakText("READY"); }
void NEOGEO_USER playVoiceLetsGo(void)   { speakText("GO"); }
void NEOGEO_USER playVoiceGameOver(void) { speakText("GAME OVER"); }

/*
 * ADPCM-B L/R pan control (YM2610 register $11, active-high).
 *   pan = 0xC0 → stereo (L+R)
 *   pan = 0x80 → left only
 *   pan = 0x40 → right only
 *   pan = 0x00 → mute
 * Takes effect immediately on the currently-playing ADPCM-B sample.
 */
void NEOGEO_USER soundSetADPCMBPan(uint8_t pan) {
	isZ80Ready(); soundCommand(0x15); isZ80Ready(); soundCommand(pan & 0xC0);
}

/*
 * FM LFO control (YM2610 register $22).
 *   bit 3 = LFO enable
 *   bits 0..2 = rate (0 slowest .. 7 fastest)
 * FM patches with non-zero AMS/PMS (set via fm/patches.fm) only
 * modulate when the LFO is enabled here.  Common values:
 *   0x00 = off, 0x0A = enable+rate2, 0x0F = enable+max-rate
 */
void NEOGEO_USER soundFMSetLFO(uint8_t rate_enable) {
	isZ80Ready(); soundCommand(0x17); isZ80Ready(); soundCommand(rate_enable & 0x0F);
}

/*
 * SSG noise period (YM2610 register $06, 5 bits).
 * Higher value = lower noise frequency.  Range 1..31; 0 silences the
 * noise generator (per chip behaviour).  Useful for tuning consonant
 * "colour" in voice cues or for noise-only SFX.
 */
void NEOGEO_USER soundSetSSGNoise(uint8_t period) {
	isZ80Ready(); soundCommand(0x19); isZ80Ready(); soundCommand(period & 0x1F);
}

/*
 * FM tempo override (writes VAR_FM_TEMPO directly).
 *
 * period_frames = Timer-B IRQs per FM music step.  Timer B fires
 * at ~8.1 Hz on the YM2610, so:
 *
 *   period_frames=1 → step every ~123 ms (fastest, sub-jingle pace)
 *   period_frames=2 → step every ~246 ms
 *   period_frames=4 → step every ~493 ms (relaxed)
 *   period_frames=8 → step every ~986 ms (very slow)
 *
 * Takes effect immediately on the currently-playing FM track and
 * persists until either the next F0 directive in the MML or the
 * next call to this function.  Range 1..8.
 */
void NEOGEO_USER soundFMSetTempo(uint8_t period_frames) {
	if (period_frames == 0u) {
		period_frames = 1u;
	} else if (period_frames > 8u) {
		period_frames = 8u;
	}
	isZ80Ready(); soundCommand(0x1A); isZ80Ready(); soundCommand(period_frames);
}
/*
 * Voice playback. Each voice constant is an ADPCM-A sample in the bank
 * generated from in_wav_a_voice. speakText prefers complete word samples
 * when available, then falls back to letters, numbers, and punctuation.
 */
void NEOGEO_USER playVoiceLetter(uint8_t letter_index) {
	playVoiceSample(letter_index);
}

void NEOGEO_USER speakWord(const char *text) {
	speakText(text);
}

void NEOGEO_USER speakText(const char *text) {
	const char *p;
	if (!text) return;

	soundStopMusic();
	for (p = text; *p; p++) {
		uint8_t sample;
		uint8_t advance;
		char c = *p;

		if (voice_direct_phrase(p, &sample, &advance)) {
			playVoiceSample(sample);
			cyclexms(115);
			p += (advance - 1u);
			continue;
		}

		sample = voice_letter_sample(c);
		if (sample == 0xFFu) sample = voice_char_sample(c);
		if (sample != 0xFFu) {
			playVoiceSample(sample);
		}
		cyclexms(45);
	}
}

/*
 * CSM (Composite Sine Mode) on FM channel 3.
 *
 * CSM is the YM2610's hardware speech-synthesis path: writing Mode
 * register $27 = $C5 (MODE=11 CSM + Enable A + Load A) puts FM
 * channel 3 under Timer A control.  Each Timer A overflow auto-keys
 * ch3's operators, producing a formant burst at the Timer A rate.
 * Combined with whatever FM patch ch3 is currently playing, the
 * chip emits vowel-like buzz / voice colouring.
 *
 *   period_hi = Timer A high byte (0..255).
 *               Formant rate ≈ 4MHz / 18 / (1024 - period) Hz.
 *               Lower period_hi = shorter period = higher formant.
 *               Typical vowels land around 80..200.
 *
 * Recommended usage:
 *   playFMTrack(N);        // give ch3 something to be buzzed
 *   soundFMCSMBegin(160);  // start CSM at a mid formant rate
 *   ...                    // sweep / vary the period for vowel motion
 *   soundFMCSMEnd();       // back to normal FM playback
 *
 * Driver cmd $1B (1-byte param) writes regs $24 / $25 / $27 atomically;
 * cmd $1C writes $27 = $00 to leave CSM.  stop_all also clears $27 so
 * CSM state can't bleed across scenes.
 */
void NEOGEO_USER soundFMCSMBegin(uint8_t period_hi) {
	isZ80Ready(); soundCommand(0x1B);
	isZ80Ready(); soundCommand(period_hi);
}
void NEOGEO_USER soundFMCSMEnd(void) {
	isZ80Ready(); soundCommand(0x1C);
}

/*
 * Loads an FM patch directly onto FM channel 3 — the one channel CSM
 * actually auto-keys — and latches a base pitch on it.  playFMTrack()
 * loads patches onto channel 2 instead, which is why "playFMTrack()
 * then CSM" left CSM buzzing an unconfigured, silent channel.  Call
 * this instead of playFMTrack() before soundFMCSMBegin()/Sweep().
 * patch = index into the FM patch table (same indices as MML `I`).
 */
void NEOGEO_USER soundFMCSMLoadVoice(uint8_t patch) {
	isZ80Ready(); soundCommand(0x1D);
	isZ80Ready(); soundCommand(patch);
}

/*
 * CSM rate sweep helper — sweeps Timer A period from hi_start down
 * to hi_end in step_ms-millisecond intervals, producing a "vowel
 * slide" formant motion against whatever FM track is currently
 * playing on channel 2.  Useful for sound-system smoke tests and
 * for "talking robot" SFX where you don't need full phoneme
 * synthesis.
 */
void NEOGEO_USER soundFMCSMSweep(uint8_t hi_start, uint8_t hi_end,
                                  uint8_t step_ms) {
	uint8_t v = hi_start;
	soundFMCSMBegin(v);
	if (hi_start < hi_end) {
		while (v < hi_end) {
			cyclexms(step_ms);
			v++;
			soundFMCSMBegin(v);
		}
	} else {
		while (v > hi_end) {
			cyclexms(step_ms);
			v--;
			soundFMCSMBegin(v);
		}
	}
	cyclexms(step_ms);
	soundFMCSMEnd();
}

void NEOGEO_USER playFMSpeechRobot(void) {
	isZ80Ready(); soundStopAll();
	isZ80Ready(); soundSceneReset();
	isZ80Ready(); soundApplyMix(0x00u, 0x00u, 0x00u, 0x0Cu);
	/* Was playFMTrack(SOUND_FM_F) - that loads a patch onto FM channel
	 * 2, but CSM only ever auto-keys channel 3, so the track it started
	 * played on the wrong channel while CSM buzzed a silent, unloaded
	 * one.  soundFMCSMLoadVoice loads the same patch (index 5, the one
	 * SOUND_FM_F's MML selects) directly onto channel 3. */
	isZ80Ready(); soundFMCSMLoadVoice(5u);
	cyclexms(20);
	soundFMCSMSweep(150u, 92u, 7u);
	soundFMCSMSweep(92u, 168u, 7u);
	isZ80Ready(); soundFMCSMEnd();
	isZ80Ready(); soundSetFMVolume(0x00u);
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
	isZ80Ready(); playSFX(SOUND_SFX_3); cyclexms(10); isZ80Ready(); playSFXB(SOUND_TRACK_A);
}

/*
 * soundPlayGameLoop — main scene music.
 *
 * POLICY (v1.3.1 audio review):
 *   - Music is ALWAYS an ADPCM-B TRACK (FM is harsh on the current
 *     driver, so we never use it as continuous background).
 *   - The `music_track` argument is mapped to ADPCM-B TRACK (0..3) via
 *     `music_track % 4` so different scenes get different TRACKs.
 *   - SOUND_TRACK_E (TRACK 4) is RESERVED for the eyecatcher
 *     screen — it is never produced by this dispatcher.  Call
 *     playSFXB(SOUND_TRACK_E) directly if you need it.
 *
 * FM / SSG / ADPCM-A remain available as one-shot cues via
 * playFMTrack / playSSGTrack / playSFX.
 */
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track) {
	/* Pool of every ADPCM-B TRACK EXCEPT TRACK 4 (SOUND_TRACK_E),
	 * which is reserved for the eyecatcher screen.  Eight slots map
	 * to 1.wav..4.wav, 6.wav..9.wav — gives a wider variety of scene
	 * music than the old 4-slot rotation. */
	static const uint8_t TRACK_pool[8] = {
		SOUND_TRACK_A,   /* 1.wav */
		SOUND_TRACK_B,     /* 2.wav */
		SOUND_TRACK_C,     /* 3.wav */
		SOUND_TRACK_D,  /* 4.wav */
		SOUND_TRACK_F,       /* 6.wav */
		SOUND_TRACK_G,       /* 7.wav */
		SOUND_TRACK_H,       /* 8.wav */
		SOUND_TRACK_I        /* 9.wav */
	};
	uint8_t TRACK = TRACK_pool[music_track & 0x07u];
	isZ80Ready(); soundSceneReset();
	isZ80Ready(); soundApplyMix(0x30, 0xB8, 0x00, 0x00);  /* TRACK prominent, FM/SSG silent */
	isZ80Ready(); playSFXB(TRACK);
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

#ifdef __cplusplus
}  /* extern "C" */
#endif
