#include <stdint.h>
#include "macro.h"

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
void cyclexms1(int cyc1);
void cyclexs(int cyc1xs);
void cyclexms(int cycxms);
uint16_t poll_joystick(void);
void fix_svalue1(uint16_t, uint16_t,uint16_t,short,uint16_t);
void fix_svalue(uint16_t, uint16_t,uint16_t,short);
void setsfix(void);
int  msb(unsigned int);
float ln2(float);
float log10(float);
void  display_digit(uint16_t, uint16_t,uint32_t,short,uint16_t);
int   read_p1credit(void);
void  playSoundtest(uint16_t);
void  isZ80Ready(void);
void  kickWatchDog(void);
void  sleep1FFF(void);
void  displayCreditP1(void);
double ln(double);
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

int NEOGEO_USER msb(unsigned int v) {

	//pos = {0, 1, 28, 2, 29, 14, 24, 3,
	//30, 22, 20, 15, 25, 17, 4, 8, 31, 27, 13, 23, 21, 19,
	//16, 7, 26, 12, 18, 6, 11, 5, 10, 9}; 
	int pos[32];
	pos[0] =0;
	pos[1] =1;
	pos[2] =28;
	pos[3] =2;
	pos[4] =29;
	pos[5] =14;
	pos[6] =24;
	pos[7] =3;
	pos[8] =30;
	pos[9] =22;
	pos[10] =20;
	pos[11] =15;
	pos[12] =25;
	pos[13] =17;
	pos[14] =4;
	pos[15] =8;
	pos[16] =31;
	pos[17] =27;
	pos[18] =13;
	pos[19] =23;
	pos[20] =21;
	pos[21] =19;
	pos[22] =16;
	pos[23] =7;
	pos[24] =26;
	pos[25] =12;
	pos[26] =18;
	pos[27] =6;
	pos[28] =11;
	pos[29] =5;
	pos[30] =10;
	pos[31] =9;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v = (v >> 1) + 1;
	return pos[(v * 0x077CB531UL) >> 27];
	
}

float NEOGEO_USER ln2(float y) {

	int log2;
	float divisor, x, result;
	log2 = msb((int)y);
	// See: https://stackoverflow.com/a/4970859/6630230
	divisor = (float)(1 << log2);
	x = y / divisor;
	// normalized value between [1.0, 2.0]
	result = -1.7417939 + (2.8212026 + (-1.4699568 + (0.44717955 - 0.056570851 * x) * x) * x) * x;
	result += ((float)log2) * 0.69314718;
	// ln(2) = 0.69314718
	return result;
	
}


float NEOGEO_USER log10( float x ) {

	return ln2(x) / LN10;
}

void NEOGEO_USER display_digit(uint16_t X, uint16_t Y,uint32_t value,short pal,uint16_t offset) {

	int counter = 0;
	int objsz = 0;
	uint32_t value_tmp2 = 0;
	value_tmp2 = value;
	objsz =   (int)(log10((float)value)) + 1;
	//fix_svalue1(X,Y+1,objsz,pal,offset);
	uint16_t s[objsz];
	int i = 0;
	for (i = objsz-1; i >= 0; i--) {
		s[i] = (value_tmp2 % 10)+offset;
		value_tmp2 /= 10;
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

	register unsigned int pad, temp;
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

	NEO_REGISTER8(REG_SOUND) =  0x01;
	isZ80Ready();
	NEO_REGISTER8(REG_SOUND) = 0x03;
	sleep1FFF();
	sleep1FFF();
	NEO_REGISTER8(REG_SOUND) = 0x07;
	sleep1FFF();
	sleep1FFF();
	NEO_REGISTER8(REG_SOUND) = index;
	
}
void NEOGEO_USER  isZ80Ready() {

	ASM_START
	ASM_L(.isready)
	ASM_MVB(REG_SOUND,%%d0)
	ASM_CMPB(#0x01,%%d0)
	ASM_BNE(.isready)
	: 
	:
	: 
	ASM_END
	
}

void kickWatchDog(void) {

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

double NEOGEO_USER ln(double x) {

	double old_sum = 0.0;
	double xmlxpl = (x - 1) / (x + 1);
	double xmlxpl_2 = xmlxpl * xmlxpl;
	double denom = 1.0;
	double frac = xmlxpl;
	double term = frac;
	// denom start from 1.0
	double sum = term;
	while ( sum != old_sum ) {
		old_sum = sum;
		denom += 2.0;
		frac *= xmlxpl_2;
		sum += frac / denom;
	}
	return 2.0 * sum;
	
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
