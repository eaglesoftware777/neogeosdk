#include <stdint.h>
#include "macro.h"
#include "sound_ids.h"

uint16_t  setSCB2(uint16_t,uint16_t);
uint16_t  setSCB3(uint16_t,uint16_t,uint16_t);
uint16_t  setFIXDATA(uint16_t, uint16_t);
uint16_t  setSCB4(uint16_t);
uint16_t  setSCB1_2(uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t);
void  setBACKDROP(uint16_t);
void  load_palettes(uint16_t*, uint16_t*);
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



uint16_t NEOGEO_USER setSCB2(uint16_t Xshrink , uint16_t Yshrink) {
	uint16_t SCB2 = (Xshrink << 8) | Yshrink;
	return SCB2;
}


uint16_t NEOGEO_USER setSCB3(/*uint16_t Y_offset,*/ uint16_t Ypos , uint16_t sticky_flag , uint16_t height_factor) {
	uint16_t SCB3 = ((/*Y_offset-*/	Ypos) << 7)  |  (sticky_flag << 6) | height_factor;
	//uint16_t SCB3 = ((Y_offset-Ypos) << 7) /*+ (sticky_flag << 6) */ + height_factor; /*((496-36)<<7)  + 10;*/
	/*if (sticky_flag == 1) 
SCB3 |= (1 << 6);
else
SCB3 &= ~(1 < 6);*/
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

	//tile_offset = /*0;*/ tile_offset >> 16;
	uint16_t Pal = (pal_offset << 8) | (tile_offset << 4) | (bit3_autoanim << 3) | (bit2_autoanim << 2) | (vflip << 1) | hflip ;
	return Pal;
	
}

void NEOGEO_USER setBACKDROP(uint16_t backdrop_color) {

	NEO_REGISTER(BACKDROP) = backdrop_color;
	/*ASM_START
ASM_MVW(%0,BACKDROP)
: "=r" (backdrop_color)
:
: "memory"
ASM_END*/

}

void NEOGEO_USER load_palettes (uint16_t *p_palette , uint16_t *palette_offset) {

	uint16_t i = 0;
	for (i = 0 ; i < 16 ; i++) {
		*(palette_offset + i) = *(p_palette + i);
	}
	//memcpy(palette_offset,p_palette,16*16);
	//ASM_START
	//ASM_LEA(%0,%%a0)
	//ASM_LEA(%1,%%a1)
	//ASM_CLRL(%%d3)
	//ASM_MVQL(#15,%%d3)
	//ASM_L(loadpal)
	//ASM_MVL((%%a0)+,(%%a1)+)
	//ASM_SUBW(#1,%%d3)
	//ASM_CMPW(#-1,%%d3)
	//ASM_BNE(loadpal)
	//: "=o" (p_palette), "=o" (palette_offset)
	//: 
	//: //"d0","d1"
	// ASM_END
}

void NEOGEO_USER vram_init(uint16_t start,uint16_t vram_inc) {

	NEO_REGISTER(VRAM_ADDR) = start;
	NEO_REGISTER(VRAM_INC) = vram_inc;
	/*	ASM_START
ASM_MVW(%0,VRAM_ADDR)  
ASM_MVW(%1,VRAM_INC ) 
: "=r" (start) , "=r" (vram_inc) 
:
: 	"memory"	
ASM_END */
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

//void NEOGEO_USER setpal_tile(uint16_t *pal_tile,uint16_t t0, uint16_t t1, uint16_t t2, uint16_t t3, uint16_t t4, uint16_t t5,uint16_t t6,uint16_t t7,uint16_t t8,uint16_t t9,uint16_t t10,uint16_t t11,uint16_t t12,uint16_t t13,uint16_t t14,uint16_t t15) {
//pal_tile[0] = t0;
//pal_tile[1] = t1;
//pal_tile[2] = t2;
//pal_tile[3] = t3;
//pal_tile[4] = t4;
//pal_tile[5] = t5;
//pal_tile[6] = t6;
//pal_tile[7] = t7;
//pal_tile[8] = t8;
//pal_tile[9] = t9;
//pal_tile[10] = t10;
//pal_tile[11] = t11;
//pal_tile[12] = t12;
//pal_tile[13] = t13;
//pal_tile[14] = t14;
//pal_tile[15] = t15;
//}

void NEOGEO_USER vram_SCB1(uint16_t *SCB1_1 , uint16_t *SCB1_2 ,uint8_t tiles_number) {

	uint16_t i = 0;
	
	for (i = 0 ; i <  tiles_number  ; i++) {
		uint16_t SCB11 = *(SCB1_1+i);
		uint16_t SCB12 = *(SCB1_2+i);
		NEO_REGISTER(VRAM_RW) = SCB11;
		NEO_REGISTER(VRAM_RW) = SCB12;
	}
	/*ASM_START
ASM_MVAL(%0,%%a0)
ASM_MVAL(%1,%%a1)
//ASM_MVW(%2,%%d2)
ASM_L(tiles)
ASM_MVW((%%a0)+,VRAM_RW)     	   //SCB1 //Even Word tile index
ASM_MVW((%%a1)+,VRAM_RW)     //Odd Word Pallete  + attributes
ASM_SUBW(#1,%%d2)
ASM_CMPW(#-1,%%d2)
ASM_BNE(tiles)	
: "=o" (SCB1_1) , "=o" (SCB1_2), "=r" (tiles_number)
:
: "cc","memory"
ASM_END */
}

void NEOGEO_USER vram_sfix(uint16_t vram_inc,uint16_t FIXADDR,uint16_t FIXDATA) {

	register int j =0;
	NEO_REGISTER(VRAM_INC) = vram_inc;
	j++;
	NEO_REGISTER(VRAM_ADDR) = FIXADDR;
	j++;
	NEO_REGISTER(VRAM_RW)   = FIXDATA;
	j++;
	j++;
	
}
void NEOGEO_USER vram_sfix1(uint16_t FIXDATA) {

	register int j =0;
	NEO_REGISTER(VRAM_RW)   = FIXDATA;
	j++;
	j++;
	
}
void NEOGEO_USER vram_SCB234(uint16_t SCBADDR,uint16_t SCB234) {

	register int j =0;
	NEO_REGISTER(VRAM_ADDR) = SCBADDR;
	NEO_REGISTER(VRAM_RW)   = SCB234;
	j++;
	j++;
	/*ASM_START
ASM_MVW(%0,VRAM_ADDR) 
ASM_MVW(%1,VRAM_RW) 
: "=r" (SCBADDR) , "=r" (SCB234)
:
: "memory"
ASM_END */

}


/*void NEOGEO_USER vram_sprite (uint16_t *p_tilemapindex,uint16_t *p_paletteindex,uint16_t *vram_addr,uint8_t * increment, uint8_t Xpos,uint8_t Ypos,uint8_t sticky_flag,uint8_t Xshrink ,uint8_t Yshrink,uint8_t height_factor, uint8_t tiles_number) {*/
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
	while(*s)
	s++;
	return (s - p);
	
}

void NEOGEO_USER  fixtext_out(uint16_t x, uint16_t y,char *mess, short pal) {

	long textsz = 0;
	textsz = strlen(mess);
	uint16_t addrfix =  FIXMAP+y+x*32;
	uint16_t fixdata = 0;
	NEO_REGISTER(VRAM_ADDR)=addrfix;
	NEO_REGISTER(VRAM_INC) = 0x20;
	long i =0;
	for (i=0; i<textsz;i++) {
		fixdata = (pal << 12) | *(mess+i);
		vram_sfix1(fixdata);
	}
	
}

void NEOGEO_USER  fixtext_out1(uint16_t x, uint16_t y,uint16_t *mess,short pal,int objsz) {

	uint16_t addrfix =  FIXMAP+y+x*32;
	uint16_t fixdata = 0;
	NEO_REGISTER(VRAM_ADDR)=addrfix;
	NEO_REGISTER(VRAM_INC) = 0x20;
	int i =0;
	for (i=0; i<objsz;i++) {
		fixdata = (pal << 12) | *(mess+i);
		vram_sfix1(fixdata);
	}
	
}

void NEOGEO_USER  fixtext_out2(uint16_t x, uint16_t y,uint16_t a, uint16_t b, uint16_t c,uint16_t mod,uint16_t *mess,short pal,int objsz) {

	uint16_t addrfix =  FIXMAP+a*x+b*y+c;
	uint16_t fixdata = 0;
	NEO_REGISTER(VRAM_ADDR)=addrfix;
	NEO_REGISTER(VRAM_INC) = mod;
	int i = 0;
	for (i=0; i<objsz;i++) {
		fixdata = (pal << 12) | *(mess+i);
		vram_sfix1(fixdata);
	}
	
}

//void NEOGEO_USER  fixtext_test() {
//NEO_REGISTER(VRAM_INC) = 0x20;
//NEO_REGISTER(VRAM_ADDR) =  FIXMAP+(5+((20)*32)); 
//int j =0;
//register uint16_t fixdata = 0;
//register uint16_t start = 0x41;
//fixdata = ((0x1 << 11 ) | (start));//0x144e;
//NEO_REGISTER(VRAM_RW) = fixdata;
//j++;
//start++;
//fixdata = (0x5 << 11 ) | start;//0x144e;
//NEO_REGISTER(VRAM_RW) = fixdata;
//j++;
//j++;
//start++;
//fixdata = (0x3 << 11 ) | start;//0x144e;
//NEO_REGISTER(VRAM_RW) = fixdata;
//j++;
//j++;
//start++;
//fixdata = (0x2 << 12 ) | start;//0x144e;
//NEO_REGISTER(VRAM_RW) = fixdata;
//j++;
//j++;
//start++;
//fixdata = (0x5 << 12 ) | start;//0x144e;
//NEO_REGISTER(VRAM_RW) = fixdata;
//j++;
//j++;
//start++;
//fixdata = (1 << 12 ) | start;//0x144e;
//NEO_REGISTER(VRAM_RW) = fixdata;
//j++;
//j++;
//start++;
//fixdata = (1 << 12 ) | start;//0x144e;
//NEO_REGISTER(VRAM_RW) = fixdata;
//j++;
//j++;
//start++;
//fixdata = (1 << 12 ) | start;//0x144e;
//NEO_REGISTER(VRAM_RW) = fixdata;
//j++;
//j++;
//start++;
//fixdata = (1 << 12 ) | start;//0x144e;
//NEO_REGISTER(VRAM_RW) = fixdata;
//j++;
//j++;
//start++;
//fixdata = (1 << 12 ) | start;//0x144e;
//NEO_REGISTER(VRAM_RW) = fixdata;
//j++;
//j++;
//start++;
//fixdata = (1 << 12 ) | start;//0x144e;
//NEO_REGISTER(VRAM_RW) = fixdata;
//j++;
//j++;
//start++;
////fixdata = (1 << 12 ) | 0x7b;//0x144e;
////NEO_REGISTER(VRAM_RW) = fixdata;
////j++;
////j++;
////j++;
//fixdata = (1 << 12 ) | start;//0x144e;
//NEO_REGISTER(VRAM_RW) = fixdata;
//j++;
//j++;
//start++;
////fixdata = (1 << 12 ) | 0x7d;//0x144e;
////NEO_REGISTER(VRAM_RW) = fixdata;
////j++;
////j++;
////j++;
//fixdata = (1 << 12 ) | start;//0x144e;
//NEO_REGISTER(VRAM_RW) = fixdata;
//j++;
//j++;
//start++;
////fixdata = (1 << 12 ) | 0x7f;//0x144e;
////NEO_REGISTER(VRAM_RW) = fixdata;
////j++;
////j++;
////j++;
//}
//void NEOGEO_USER  register_test() {
//ASM_START
//ASM_MVQ(#0,%%d0)
//ASM_MVW(#0x0100,%%d0)       
//ASM_MVB(#0x4b,%%d0)       
//: 
//:
//: 
//ASM_END
//}

void NEOGEO_USER mess_outtest(void) {

	setBIOSMESSBusy();
	uint16_t *pmessp= NEO_REGISTER32(BIOS_MESS_POINT);
	uint16_t *ptr = RAMSTART;
	*ptr++ =0x434F;
	*ptr++ =0x4D4D;
	*ptr++ =0x414E;
	*ptr++ =0x4420;
	*ptr++ =0x34FF;
	*ptr++ =0x434F;
	*ptr++ =0x4D4D;
	*ptr++ =0x414E;
	*ptr++ =0x4420;
	*ptr++ =0x36FF;
	*ptr++ =0x0;
	uint16_t *ptrsub = RAMSTART+100;
	*ptrsub++=COMMAND5;
	*ptrsub++=0x0001;
	*ptrsub++=COMMAND9T1;
	*ptrsub++=0x5C2F;
	*ptrsub++=0x3A2E;
	*ptrsub++=0xD7FF;
	*ptrsub++=COMMAND5;
	*ptrsub++=0x0001;
	*ptrsub++=COMMANDB;
	*ptrsub++ =0x0;
	*pmessp++=MESSPOINTZERO;
	*pmessp++=MESSPOINTZERO;
	*pmessp++=COMMAND1BF;
	*pmessp++=ZENDCODEFF;
	*pmessp++=COMMAND2INC20;
	*pmessp++=COMMAND3;
	*pmessp++=0x7252;
	*pmessp++=COMMAND7;
	*pmessp++=0x434F;
	*pmessp++=0x4D4D;
	*pmessp++=0x414E;
	*pmessp++=0x4420;
	*pmessp++=0x37FF;
	*pmessp++=COMMANDC2;
	*pmessp++=0x0021;
	*pmessp++=COMMAND5;
	*pmessp++=0x0001;
	*pmessp++=COMMAND4;
	*pmessp++=0x0010;
	*pmessp++=0x0000;
	*pmessp++=COMMANDC2;
	*pmessp++=0x0021;
	*pmessp++=COMMAND5;
	*pmessp++=0x0001;
	*pmessp++=COMMAND6;
	*pmessp++=COMMANDC2;
	*pmessp++=0x0021;
	*pmessp++=COMMAND5;
	*pmessp++=0x0001;
	*pmessp++=COMMANDDA;
	*pmessp++=0x0030;
	*pmessp++=COMMAND5;
	*pmessp++=0x0001;
	*pmessp++=COMMAND8T1;
	*pmessp++=0x4142;
	*pmessp++=0x4344;
	*pmessp++=0x4546;
	*pmessp++=0x47FF;
	*pmessp++=COMMAND5;
	*pmessp++=0x0001;
	*pmessp++=COMMAND9T1;
	*pmessp++=0xD1D2;
	*pmessp++=0xD3D4;
	*pmessp++=0xD5D6;
	*pmessp++=0xD7FF;
	*pmessp++=COMMANDA;
	*pmessp++=0x0010;
	*pmessp++=0x0100;
	*pmessp++=COMMAND7;
	*pmessp++=0x454E;
	*pmessp++=0x44FF;
	*pmessp++=COMMAND0;
	NEO_REGISTER32(BIOS_MESS_POINT) = pmessp;
	setBIOSMESSReady();
	CALLNEOGEOF(SYS_MESS_OUT);
	
}

void NEOGEO_USER setBIOSMESSBusy(void) {

	ASM_START
	//ASM_BSETB(#0,BIOS_MESS_BUSY)	
	ASM_ADDQB(#1,BIOS_MESS_BUSY) //mess out disable
	ASM_RTS
	: 
	:
	: 
	ASM_END
	
}

void NEOGEO_USER setBIOSMESSReady(void) {

	ASM_START
	//ASM_BCLRB(#0,BIOS_MESS_BUSY)   //Ready to go
	ASM_SUBQB(#1,BIOS_MESS_BUSY) //mess out enable
	ASM_RTS
	: 
	:
	: 
	ASM_END
	
}

void NEOGEO_USER  clearRAM() {

	ASM_START
	ASM_MVL(#(0xF300/32)-1,%%d7)       // We'll clear $F300 bytes of user RAM by writing 8 longwords (32 bytes) at a time
	ASM_LEA(RAMSTART,%%a0)             // Start at the beginning of user RAM
	ASM_MVQL(#0,%%d0)                  // Clear it with 0's
	ASM_L(.clear_ram)
	ASM_MVL(%%d0,(%%a0)+)                // Write the 8 longwords, incrementing A0 each time
	ASM_MVL(%%d0,(%%a0)+)
	ASM_MVL(%%d0,(%%a0)+)
	ASM_MVL(%%d0,(%%a0)+)
	ASM_MVL(%%d0,(%%a0)+)
	ASM_MVL(%%d0,(%%a0)+)
	ASM_MVL(%%d0,(%%a0)+)
	ASM_MVL(%%d0,(%%a0)+)
	ASM_DBRA(%%d7,.clear_ram)           // Are we done ? No: jump back to .clear_ram
	: 
	:
	: 
	ASM_END
	
}


void NEOGEO_USER clearSprs() {

	ASM_START
	ASM_MVW(#SCB3_ADDR,VRAM_ADDR)     // Height attributes are in VRAM at Sprite Control Bank 3
	ASM_CLRW(%%d0)
	ASM_MVW(#1,VRAM_INC)        // Set the VRAM address auto-increment value
	ASM_MVL(#512-1,%%d7)              // Clear all 512 sprites
	ASM_NOP
	ASM_L(.clearspr)
	ASM_MVW(%%d0,VRAM_RW)          // Write to VRAM
	ASM_NOP                             // Wait a bit...
	ASM_NOP
	ASM_DBRA(%%d7,.clearspr)           // Are we done ? No: jump back to .clearspr
	: 
	:
	: 
	ASM_END
	
}


void NEOGEO_USER clearFix() {

	ASM_START
	ASM_JSR(SYS_FIX_CLEAR) //; jump to the FIX_CLEAR subroutine
	ASM_MVL(#(40*32)-1,%%d7)         // ; Clear the whole map
	ASM_MVW(#FIXMAP,VRAM_ADDR)
	ASM_MVW(#0xFF,%%d0)              //; Use tile $FF
	ASM_L(.clearfix)
	ASM_MVW(%%d0,VRAM_RW)         // ; Write to VRAM
	ASM_NOP                             //; Wait a bit...
	ASM_NOP
	ASM_DBRA(%%d7,.clearfix)          ///; Are we done ? No: jump back to .clearfix
	: 
	:
	: 
	ASM_END
	
}

void NEOGEO_USER  waitVbl() {

	ASM_START
	ASM_L(.wait_vbl)
	ASM_TSTW(USER_WORKRAM)
	ASM_JEQ(.wait_vbl)
	ASM_CLRW(USER_WORKRAM)
	ASM_ADDQL(#1, USER_WORKRAM+32)
	ASM_RTS
	: 
	:
	: 
	ASM_END
	
}

void NEOGEO_USER  cycle10ms() {

	ASM_START
	ASM_L(CYC)
	ASM_MVW(#2400,%%d0) //0x960
	ASM_L(DELAY)
	ASM_DBF(%%d0,DELAY)
	: 
	:
	: 
	ASM_END
	
}

void NEOGEO_USER  cycle1s() {

	ASM_START
	ASM_MVW(#5000,%%d1)
	ASM_L(CYC1)
	ASM_MVW(#240,%%d0) //0x960
	ASM_L(DELAY1)
	ASM_DBF(%%d0,DELAY1)
	ASM_SUBQ(#1,%%d1)
	ASM_BNEB(CYC1)
	: 
	:
	: 
	ASM_END
	
}


void NEOGEO_USER  cyclexms1(int cyc1) {

	ASM_START
	ASM_MVEML(%%d0-%%d7/%%a0-%%a6,-(%%sp))
	ASM_MVL(%[cyc1],%%d1)
	ASM_L(CYC0)
	ASM_MVW(#240,%%d0) //0x960
	ASM_L(DELAY0)
	ASM_DBF(%%d0,DELAY0)
	ASM_SUBQ(#1,%%d1)
	ASM_BNEB(CYC0)
	ASM_MVEML((%%sp)+, %%d0-%%d7/%%a0-%%a6)
	: 
	:[cyc1] "r" (cyc1)
	: 
	ASM_END
	
}

void NEOGEO_USER  cyclexs(int cyc1xs) {

	ASM_START
	ASM_MVEML(%%d0-%%d7/%%a0-%%a6,-(%%sp))
	ASM_MVW(%[cyc1xs],%%d1)
	ASM_L(CYCXS)
	ASM_MVW(#5000,%%d2)
	ASM_L(CYC1S)
	ASM_MVW(#240,%%d0) //0x960
	ASM_L(DELAY0XS)
	ASM_DBF(%%d0,DELAY0XS)
	ASM_SUBQ(#1,%%d2)
	ASM_BNEB(CYC1S)
	ASM_SUBQ(#1,%%d1)
	ASM_BNEB(CYCXS)
	ASM_MVEML((%%sp)+, %%d0-%%d7/%%a0-%%a6)
	: 
	:[cyc1xs] "r" (cyc1xs)
	: 
	ASM_END
	
}

void NEOGEO_USER  cyclexms(int cycxms) {

	ASM_START
	ASM_MVEML(%%d0-%%d7/%%a0-%%a6,-(%%sp))
	ASM_MVW(%[cycxms],%%d1)
	ASM_L(CYCXMS)
	ASM_MVW(#50,%%d2)
	ASM_L(CYC1MS)
	ASM_MVW(#240,%%d0) //0x960
	ASM_L(DELAY0XMS)
	ASM_DBF(%%d0,DELAY0XMS)
	ASM_SUBQ(#1,%%d2)
	ASM_BNEB(CYC1MS)
	ASM_SUBQ(#1,%%d1)
	ASM_BNEB(CYCXMS)
	ASM_MVEML((%%sp)+, %%d0-%%d7/%%a0-%%a6)
	: 
	:[cycxms] "r" (cycxms)
	: 
	ASM_END
	
}

uint16_t	NEOGEO_USER poll_joystick(/*uint8_t port, uint8_t flags*/) {

	uint16_t*	pdata1 = 0, pdata2 = 0;
	uint16_t	data1 = 0, data2 = 0;
	/*switch(port)
{
case	1:
switch(flags)
{
case	1:
data1 = (NEO_REGISTER(REG_P1CNT))^0xFF;
data2 = (NEO_REGISTER(REG_STATUS_B))^0xFF;
break;
case	2:*/
	pdata1 = BIOS_P1PREVIOUS;
	pdata2 = BIOS_STATCURNT;
	/*break;
case	3:
data1 = NEO_REGISTER(BIOS_P1CHANGE);
data2 = NEO_REGISTER(BIOS_STATCHANGE);
break;
case	4:
data1 = NEO_REGISTER(BIOS_P1REPEAT);
data2 = NEO_REGISTER(BIOS_STATCHANGE);
break;
}	
break;
case	2:
switch(flags)
{
case	1:
data1 = (NEO_REGISTER(REG_P2CNT))^0xFF;
data2 = ((NEO_REGISTER(REG_STATUS_B))^0xFF)>>2;
break;
case	2:
data1 = NEO_REGISTER(BIOS_P2PREVIOUS);
data2 = (NEO_REGISTER(BIOS_STATCURNT))>>2;
break;
case	3:
data1 = NEO_REGISTER(BIOS_P2CHANGE);
data2 = (NEO_REGISTER(BIOS_STATCHANGE))>>2;
break;
case	4:
data1 = NEO_REGISTER(BIOS_P2REPEAT);
data2 = (NEO_REGISTER(BIOS_STATCHANGE))>>2;
break;
}	
break;
}
*/
	data1 = (uint16_t*)(pdata1);
	data2 = (uint16_t*)(pdata2);
	uint16_t ret = data2 <<8 | data1;
	return ret;
	
}



void NEOGEO_USER fix_svalue1(uint16_t X, uint16_t Y,uint16_t v,short pal,uint16_t offset) {

	uint16_t addrfix =  FIXMAP+(Y+2+((X+1)*32));
	uint16_t fixdata = 0;
	v = v + offset;
	fixdata = (pal << 12) | v;
	vram_sfix(0x20,addrfix,fixdata);
	
}

void NEOGEO_USER fix_svalue(uint16_t X, uint16_t Y,uint16_t v,short pal) {

	uint16_t addrfix =  FIXMAP+(Y+2+((X+1)*32));
	uint16_t fixdata = 0;
	fixdata = (pal << 12) | v;
	vram_sfix(0x20,addrfix,fixdata);
	
}

void NEOGEO_USER setsfix() {

	//	NEO_REGISTER(REG_BRDFIX) = 0x0001; 
	ASM_START
	ASM_BSETB(#0,REG_BRDFIX)	
	ASM_RTS
	: 
	:
	: 
	ASM_END
	
}

void NEOGEO_USER display_digit(uint16_t X, uint16_t Y,uint32_t value,short pal,uint16_t offset) {

	uint16_t s[10];
	int objsz = 0;
	uint32_t power = 1UL;

	if (value >= 1000000000UL) {
		power = 1000000000UL;
	} else if (value >= 100000000UL) {
		power = 100000000UL;
	} else if (value >= 10000000UL) {
		power = 10000000UL;
	} else if (value >= 1000000UL) {
		power = 1000000UL;
	} else if (value >= 100000UL) {
		power = 100000UL;
	} else if (value >= 10000UL) {
		power = 10000UL;
	} else if (value >= 1000UL) {
		power = 1000UL;
	} else if (value >= 100UL) {
		power = 100UL;
	} else if (value >= 10UL) {
		power = 10UL;
	}

	/*
	 * Build decimal digits without 32-bit division or modulo. That keeps the
	 * 68000 game build independent from the old floating-point helper objects.
	 */
	while (1) {
		uint16_t digit = 0;

		while (value >= power) {
			value -= power;
			digit++;
		}

		s[objsz++] = digit + offset;
		if (power == 1UL) {
			break;
		}

		if (power == 1000000000UL) {
			power = 100000000UL;
		} else if (power == 100000000UL) {
			power = 10000000UL;
		} else if (power == 10000000UL) {
			power = 1000000UL;
		} else if (power == 1000000UL) {
			power = 100000UL;
		} else if (power == 100000UL) {
			power = 10000UL;
		} else if (power == 10000UL) {
			power = 1000UL;
		} else if (power == 1000UL) {
			power = 100UL;
		} else if (power == 100UL) {
			power = 10UL;
		} else {
			power = 1UL;
		}
	}

	fixtext_out1(X,Y,s,pal,objsz);
	
}


int NEOGEO_USER read_p1credit(void) {

	int  p1cr =0;
	uint8_t  *p1credit =0x0;
	p1credit = P1_CREDITS;
	p1cr = *p1credit++;
	return p1cr;
}


int NEOGEO_USER read_joy1() {

	register unsigned int pad;
	register unsigned char *s;
	pad = 0x0000;
	s = (unsigned char *) 0x380000;
	pad = *s;
	pad = pad << 8;
	s = (unsigned char *) 0x300000;
	pad |= (*s);
	/*	pad |= temp; */
	/*pad = (~pad);*/
	return ( ~(pad) );
}


void NEOGEO_USER  playSoundtest(uint16_t index) {
	isZ80Ready();
	soundCommand((uint8_t)(index & 0xFF));
}

void NEOGEO_USER soundCommand(uint8_t command) {
	isZ80Ready();
	NEO_REGISTER8(REG_SOUND) = command;
	isZ80Ready();
}

/* ----------------------------------------------------------
   Core sound control
   ---------------------------------------------------------- */

void NEOGEO_USER soundInit(void) {
	isZ80Ready();
	soundCommand(0x01); // CMD_INIT
}

void NEOGEO_USER soundReset(void) {
	isZ80Ready();
	soundCommand(0x03); // CMD_RESET
}

void NEOGEO_USER soundStopAll(void) {
	isZ80Ready();
	soundCommand(0x04); // CMD_STOP_ALL
}

void NEOGEO_USER soundStopMusic(void) {
	isZ80Ready();
	soundCommand(0x0F); // Stop SSG/music/FM output
}

void NEOGEO_USER soundCancelFade(void) {
	isZ80Ready();
	soundCommand(0x11); // Cancel fade
}

void NEOGEO_USER soundSceneReset(void) {
	isZ80Ready();
	soundStopAll();
	cyclexms(4);
	isZ80Ready();
	soundReset();
	cyclexms(4);
}

/* ----------------------------------------------------------
   Music and sample playback
   ---------------------------------------------------------- */

void NEOGEO_USER playMusic(uint8_t n) {
	isZ80Ready();
	soundCommand(0x20 + n); // MUSIC_BASE + n
}

void NEOGEO_USER playSFX(uint8_t n) {
	isZ80Ready();
	soundCommand(0x40 + n); // SFX_A_BASE + n
}

void NEOGEO_USER playSFXB(uint8_t n) {
	isZ80Ready();
	soundCommand(0x80 + n); // SFX_B_BASE + n
}

/* ----------------------------------------------------------
   FM sequencer
   ---------------------------------------------------------- */

void NEOGEO_USER playFMDebug(void) {
	isZ80Ready();
	soundCommand(0x30); // FM track 0/debug
}

void NEOGEO_USER playFMTrack(uint8_t n) {
	isZ80Ready();
	soundCommand(0x31); // FM track select prefix
	isZ80Ready();
	soundCommand(n);
}

void NEOGEO_USER soundSetFMVolume(uint8_t v) {
	isZ80Ready();
	soundCommand(0x13); // FM volume prefix
	isZ80Ready();
	soundCommand(v & 0x0F);
}

/* ----------------------------------------------------------
   Standalone SSG sequencer
   ---------------------------------------------------------- */

void NEOGEO_USER playSSGTrack(uint8_t n) {
	isZ80Ready();
	soundCommand(0x32); // SSG track select prefix
	isZ80Ready();
	soundCommand(n);
}

void NEOGEO_USER soundSetSSGPreset(uint8_t preset) {
	isZ80Ready();
	soundCommand(0x14); // SSG preset prefix
	isZ80Ready();
	soundCommand(preset & 0x0F);
}

void NEOGEO_USER playInsertCoinSSG(void) {
	isZ80Ready();
	playSSGTrack(SOUND_SSG_INSERT_COIN);
	soundSetSSGPreset(1);
}

/* ----------------------------------------------------------
   Voice / cue helpers
   ---------------------------------------------------------- */

void NEOGEO_USER playVoiceCue(uint8_t n) {
	isZ80Ready();

	switch (n) {
		case SOUND_VOICE_GET_READY:
			playSFX(SOUND_SFX_READY_VOICE);
			break;

		case SOUND_VOICE_ATTACK:
			playSFX(SOUND_SFX_ATTACK_VOICE);
			break;

		default:
			playSFX(n);
			break;
	}
}

void NEOGEO_USER playGetReadyVoice(void) {
	isZ80Ready();
	playSFX(SOUND_SFX_READY_VOICE);
}

void NEOGEO_USER playAttackVoice(void) {
	isZ80Ready();
	playSFX(SOUND_SFX_ATTACK_VOICE);
}

void NEOGEO_USER playCoinThenReady(void) {
	isZ80Ready();
	playInsertCoinSSG();
	cyclexms(250);
	isZ80Ready();
	playGetReadyVoice();
}

/* ----------------------------------------------------------
   Fade control
   ---------------------------------------------------------- */

void NEOGEO_USER soundFadeOut(void) {
	isZ80Ready();
	soundFadeOutSpeed(0x20);
}

void NEOGEO_USER soundFadeIn(void) {
	isZ80Ready();
	soundFadeInSpeed(0x20);
}

void NEOGEO_USER soundFadeOutSpeed(uint8_t speed) {
	isZ80Ready();
	soundCommand(0x0A); // fade-out speed prefix
	isZ80Ready();
	soundCommand(speed);
}

void NEOGEO_USER soundFadeInSpeed(uint8_t speed) {
	isZ80Ready();
	soundCommand(0x12); // fade-in speed prefix
	isZ80Ready();
	soundCommand(speed);
}

/* ----------------------------------------------------------
   Runtime tempo / volume
   ---------------------------------------------------------- */

void NEOGEO_USER soundSetTempo(uint8_t t) {
	isZ80Ready();
	soundCommand(0x0E); // tempo prefix
	isZ80Ready();
	soundCommand(t);
}

void NEOGEO_USER soundSetADPCMAVolume(uint8_t v) {
	isZ80Ready();
	soundCommand(0x05); // ADPCM-A volume prefix
	isZ80Ready();
	soundCommand(v & 0x3F);
}

void NEOGEO_USER soundSetADPCMBVolume(uint8_t v) {
	isZ80Ready();
	soundCommand(0x06); // ADPCM-B volume prefix
	isZ80Ready();
	soundCommand(v);
}

void NEOGEO_USER soundSetSSGVolume(uint8_t v) {
	isZ80Ready();
	soundCommand(0x07); // SSG/MML volume prefix
	isZ80Ready();
	soundCommand(v & 0x0F);
}

/* ----------------------------------------------------------
   Higher-level scene helpers
   ---------------------------------------------------------- */

void NEOGEO_USER soundApplyMix(uint8_t adpcma_vol, uint8_t adpcmb_vol, uint8_t ssg_vol, uint8_t fm_vol) {
	isZ80Ready();
	soundSetADPCMAVolume(adpcma_vol);
	isZ80Ready();
	soundSetADPCMBVolume(adpcmb_vol);
	isZ80Ready();
	soundSetSSGVolume(ssg_vol);
	isZ80Ready();
	soundSetFMVolume(fm_vol);
}

void NEOGEO_USER soundPlayDemoFM(uint8_t fm_track) {
	isZ80Ready();
	soundSceneReset();
	isZ80Ready();
	soundSetFMVolume(0x0C);
	isZ80Ready();
	playFMTrack(fm_track);
}

void NEOGEO_USER soundPlayTitleMusic(uint8_t music_track) {
	isZ80Ready();
	soundSceneReset();
	isZ80Ready();
	soundApplyMix(0x34, 0x45, 0x08, 0x0C);
	isZ80Ready();
	playMusic(music_track);
}

void NEOGEO_USER soundPlayGameLoop(uint8_t music_track) {
	isZ80Ready();
	soundSceneReset();
	isZ80Ready();
	soundApplyMix(0x34, 0xB8, 0x0A, 0x0E);
	isZ80Ready();
	playMusic(music_track);
}

void NEOGEO_USER  isZ80Ready() {

	ASM_START
	ASM_L(.isready)
	ASM_MVB(#0,0x300001)             // Kick watchdog
	ASM_MVB(0x320000,%%d0)
	ASM_CMPB(#0x01,%%d0)
	ASM_BNE(.isready)
	: 
	:
	: "d0"
	ASM_END
	
}

void NEOGEO_USER kickWatchDog(void) {

	ASM_START
	ASM_MVB(%%d0,REG_DIPSW) 
	: 
	:
	: 
	ASM_END
	
}

void NEOGEO_USER  sleep1FFF(void) {

	ASM_START	
	ASM_MVW(#0x1FFF,%%d0) //loop 0x1FFF
	ASM_L(.loop)
	ASM_NOP
	ASM_DBRA(%%d0,.loop)
	: 
	:
	: 
	ASM_END
	
}


void NEOGEO_USER displayCreditP1(void) {

	ASM_START	
	ASM_MVQ(#0,%%d0)
	ASM_MVB(P1_CREDITS,%%d0)           // Read P1 credits 
	ASM_MVW(#0x7088,VRAM_ADDR)   // Set position in fix map, top left
	ASM_MVW(%%d0,%%d1)           // Make a copy of the BCD value
	ASM_MVW(#0x20,VRAM_INC )       //Auto-inc 
	ASM_LSRB(#4,%%d1)                // Get tens
	ASM_ANDIW(#0x000F,%%d1)              // Mask off 
	ASM_ORIW(#0x0030,%%d1)              //Set palette 5 and tile offset $30 (ASCII "0")
	ASM_MVW(%%d1,VRAM_RW)         //; Write tens to VRAM
	ASM_ANDIW(#0x000F,%%d0)              //; Get units
	ASM_ORIW(#0x0030,%%d1)              //; 
	ASM_MVW(%%d1,VRAM_RW )         //; Write units to VRAM	
	: 
	:
	: 
	ASM_END
	
}

//void NEOGEO_USER *memcpy(void *dest, const void *src, int count)
//{
//char *d = (char *)dest;
//const char *s = (const char *)src;
//int len;
//if(count == 0 || dest == src)
//return dest;
//if(((uint32_t)d | (uint32_t)s) & lmask) {
//// src and/or dest do not align on word boundary
//if((((uint32_t)d ^ (uint32_t)s) & lmask) || (count < lsize))
//len = count; // copy the rest of the buffer with the byte mover
//else
//len = lsize - ((uint32_t)d & lmask); // move the ptrs up to a word boundary
//count -= len;
//for(; len > 0; len--)
//*d++ = *s++;
//}
//for(len = count / lsize; len > 0; len--) {
//*(uint32_t *)d = *(uint32_t *)s;
//d += lsize;
//s += lsize;
//}
//for(len = count & lmask; len > 0; len--)
//*d++ = *s++;
//return dest;
//}

void NEOGEO_USER displayCreditP2(void) {

	ASM_START	
	ASM_MVQ(#0,%%d0)
	ASM_MVB(P2_CREDITS,%%d0)           // Read P1 credits 
	ASM_MVW(#0x7088,VRAM_ADDR)   // Set position in fix map, top left
	ASM_MVW(%%d0,%%d1)           // Make a copy of the BCD value
	ASM_MVW(#0x20,VRAM_INC )       //Auto-inc 
	ASM_LSRB(#4,%%d1)                // Get tens
	ASM_ANDIW(#0x000F,%%d1)              // Mask off 
	ASM_ORIW(#0x0030,%%d1)              //Set palette 5 and tile offset $30 (ASCII "0")
	ASM_MVW(%%d1,VRAM_RW)         //; Write tens to VRAM
	ASM_ANDIW(#0x000F,%%d0)              //; Get units
	ASM_ORIW(#0x0030,%%d1)              //; 
	ASM_MVW(%%d1,VRAM_RW )         //; Write units to VRAM	
	: 
	:
	: 
	ASM_END
	
}
