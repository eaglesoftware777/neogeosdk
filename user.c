#include <stdint.h>
#include "macro.h"
#pragma GCC push_options
#pragma GCC optimize ("O0")

NEOGEO_INTERRUPT
void NEOGEO_USER ZD_ENTRY (void) {

}

NEOGEO_INTERRUPT
void NEOGEO_USER CHK_ENTRY (void) {

}

NEOGEO_INTERRUPT
void NEOGEO_USER TRAPV_ENTRY (void) {

}

NEOGEO_INTERRUPT
void NEOGEO_USER 
VBlank (void) {
	
ASM_START
ASM_L(VBLANK)                  //; Label defined in header.asm
ASM_MVW(#1, USER_WORKRAM)       // a word in USER_WORKRAM : vblank flag
ASM_BTST(#7,BIOS_SYSTEM_MODE)  //; Check if the system ROM wants to take care of the interrupt
ASM_BNE(.getvbl)               //; No: jump to .getvbl
ASM_JMP(SYS_INT1)              //; Yes: jump to system ROM
ASM_L(.getvbl)
ASM_MVEML(%%d0-%%d7/%%a0-%%a6,-(%%sp))
ASM_MVW(#4,REG_IRQACK) //; acknowledge the VBlank interrupt
ASM_MVB(%%d0,REG_DIPSW)        //; Kick watchdog
ASM_JSR(SYS_IO)
ASM_MVEML((%%sp)+, %%d0-%%d7/%%a0-%%a6)
: 
:
:
ASM_END
}

NEOGEO_INTERRUPT
void  NEOGEO_USER IRQ2 (void) {
	
ASM_START
ASM_MVW(#2,REG_IRQACK)				//;IRQ2
: 
:
: 
ASM_END
}

NEOGEO_INTERRUPT
void  NEOGEO_USER IRQ3 (void) {
	
ASM_START
ASM_MVW(#1,REG_IRQACK) 
ASM_MVB(%%d0,REG_DIPSW) 
: 
:
: 
ASM_END
}

NEOGEO_INTERRUPT
void  NEOGEO_USER INT4 (void)  {

}
NEOGEO_INTERRUPT
void  NEOGEO_USER INT5 (void) {

}
NEOGEO_INTERRUPT
void  NEOGEO_USER INT6 (void)  {

}
NEOGEO_INTERRUPT
void  NEOGEO_USER INT7 (void)  {

}

void  NEOGEO_USER USER(void) {
	
ASM_START
ASM_L(REQUEST_VECTOR)
ASM_DL(POWER_ON)
ASM_DL(EYE_CATCHER)
ASM_DL(GAME)
ASM_DL(TITLE)
ASM_MVB(BIOS_USER_REQUEST,%%d0)
ASM_LSLB(#2,%%d0)
ASM_LEA(REQUEST_VECTOR,%%a0)
ASM_MVAL((%%a0,%%d0),%%a0)
ASM_JSR((%%a0))
ASM_JMP(SYS_RETURN)
:
:
:
ASM_END
}

void NEOGEO_USER PLAYER_START (void) {
	
uint16_t start_flag,country_code = 0;
register short P1 = 0;
register short P2 = 0;
register short P3 = 0;
register short P4 = 0;
start_flag = NEO_REGISTER8(BIOS_START_FLAG) ;
country_code = NEO_REGISTER8(BIOS_COUNTRY_CODE);
playSoundtest(0x28);
CALLNEOGEOF(SYS_CREDIT_CHECK);
P1=(start_flag >> 0) & 1;
P2=(start_flag >> 1) & 1;
P3=(start_flag >> 2) & 1;
P4=(start_flag >> 3) & 1;
if (P1==1) {  
start_flag |= 1 << 0;
NEO_REGISTER8(BIOS_PLAYER1_MODE) |= 1 << 0;
NEO_REGISTER8(BIOS_PLAYER1_MODE) &= ~(1 << 1); 
NEO_REGISTER8(BIOS_PLAYER1_MODE) &= ~(1 << 2);
NEO_REGISTER8(BIOS_PLAYER1_MODE) &= ~(1 << 3);
}
if (P2==1) {  
start_flag |= 1 << 1;
if(country_code == 0) {
NEO_REGISTER8(BIOS_PLAYER2_MODE) |= 1 << 0;
NEO_REGISTER8(BIOS_PLAYER2_MODE) |= 1 << 1;
NEO_REGISTER8(BIOS_PLAYER2_MODE) &= ~(1 << 2);
NEO_REGISTER8(BIOS_PLAYER2_MODE) &= ~(1 << 3);
}
if(country_code == 1) {
NEO_REGISTER8(BIOS_PLAYER2_MODE) &= ~(1 << 0);
NEO_REGISTER8(BIOS_PLAYER2_MODE) |= 1 << 1;
NEO_REGISTER8(BIOS_PLAYER2_MODE) &= ~(1 << 2);
NEO_REGISTER8(BIOS_PLAYER2_MODE) &= ~(1 << 3);
}
if(country_code == 2) {
NEO_REGISTER8(BIOS_PLAYER2_MODE) |= 1 << 0;
NEO_REGISTER8(BIOS_PLAYER2_MODE) |= 1 << 1;
NEO_REGISTER8(BIOS_PLAYER2_MODE) &= ~(1 << 2);
NEO_REGISTER8(BIOS_PLAYER2_MODE) &= ~(1 << 3);
}
}
if (P3==1) {  
start_flag |= 1 << 2;
}
if (P4==1) {  
start_flag |= 1 << 3;
}
NEO_REGISTER8(BIOS_START_FLAG) = start_flag;
//NEO_REGISTER8(BIOS_USER_MODE) = 0x2; //keep user mode game
CALLNEOGEOF(SYS_CREDIT_CHECK);
CALLNEOGEOF(SYS_CREDIT_DOWN);
CALLNEOGEOF(SYS_RETURN);
}

void NEOGEO_USER DEMO_END (void) { //only MVS

int i =0;
i++;
i++;

}
void NEOGEO_USER COIN_SOUND (void) {

playSoundtest(0x29);
int i =0;
i++;
i++;
}


void  NEOGEO_USER POWER_ON (void) {  //MVS only

ASM_START
ASM_MVB(#0x00,BIOS_USER_MODE) //user_request = 0
ASM_LEA(USER_WORKRAM+64,%%a0)	//Start of game save block
ASM_MV(#0x001F,%%d1)		//32*4 longword writes = 512 bytes
ASM_MVQ(#0,%%d0)
ASM_L(.cl)                    //init game save block
ASM_MVL(%%d0,(%%a0)+)
ASM_MVL(%%d0,(%%a0)+)
ASM_MVL(%%d0,(%%a0)+)
ASM_MVL(%%d0,(%%a0)+)
ASM_DBF(%%d1,.cl)
ASM_MVB(%%d0,REG_DIPSW)   // Kick watchdog
ASM_JMP(SYS_RETURN)
:
:
:
ASM_END

}

void  NEOGEO_USER EYE_CATCHER (void) {  //AES only ; May be used in MVS : 
// MVS : Eye catcher call within command 2 (attract mode) advised 
}




void  NEOGEO_USER GAME (void) {

ASM_START
ASM_LEA(BIOS_WORKRAM,%%sp)              // A7 (SSP) = 10F300H Init stack pointer
ASM_MVB(%%d0,REG_DIPSW)           // Kick watchdog
ASM_MVW(#0x0000,REG_LSPCMODE)    // pixel timer is disabled (Timer Interrupt: Not allowed)
ASM_MVW(#7,REG_IRQACK)           //; Clear all interrupts ; Other I/O: Undefined
ASM_ADDQB(#1,BIOS_MESS_BUSY) //mess out disable
ASM_BCLRB(#7,BIOS_SYSTEM_MODE) //  system mode
ASM_MVW(#0x2000,%%sr)              //; Enable interrupts SR = 2700H supervisor mode
ASM_MVB(#0x02,BIOS_USER_MODE) //user_request = 2
ASM_SUBQB(#1,BIOS_MESS_BUSY) //mess out enable
ASM_BSETB(#7,BIOS_SYSTEM_MODE) //  game mode
ASM_JSR(INIT_GAME)
ASM_JSR(DEMO_GAME)
ASM_JMP(SYS_RETURN)
:
:
:
ASM_END
}

void NEOGEO_USER TITLE(void) {   // MVS systems only 

ASM_START
ASM_LEA(BIOS_WORKRAM,%%sp)              // A7 (SSP) = 10F300H Init stack pointer
ASM_MVB(%%d0,REG_DIPSW)           // Kick watchdog
ASM_MVW(#0x0000,REG_LSPCMODE)    // pixel timer is disabled (Timer Interrupt: Not allowed)
ASM_MVW(#7,REG_IRQACK)           //; Clear all interrupts ; Other I/O: Undefined
ASM_ADDQB(#1,BIOS_MESS_BUSY) //mess out disable
ASM_BCLRB(#7,BIOS_SYSTEM_MODE) //  system mode
ASM_MVW(#0x2000,%%sr)              //; Enable interrupts SR = 2700H supervisor mode
ASM_MVB(#0x03,BIOS_USER_MODE) //user_request = 2
ASM_SUBQB(#1,BIOS_MESS_BUSY) //mess out enable
ASM_BSETB(#7,BIOS_SYSTEM_MODE) //  game mode
ASM_JSR(INIT_GAME)
ASM_JSR(showTitleMVS)
ASM_MVB(#0x02,BIOS_USER_MODE) //user_request = 2
ASM_JMP(START_GAME)
:
:
:
ASM_END			
}


void  NEOGEO_USER eye_cactherAES (void) {   //AES ONLY
ASM_START
ASM_JMP(SYS_RETURN)
:
:
:
ASM_END		
}

void  NEOGEO_USER showTitleMVS(void) {

uint16_t  pal_tile0[16];
setpal(pal_tile0,BLACK,BLACK,0xFFF,RED,BLUE,MIDGREEN,CYAN,ORANGE,MAGENTA,RED,WHITE,BLUE,RED,BLUE,CYAN,RED);
load_palettes(pal_tile0,PALETTES);
waitVbl();
fixtext_out(15,10,"TITLE MODE MVS",0);
playSoundtest(0x24);
int i =0;
for(i=0;i<3;i++) {
fix_svalue1(13,15,i,0,48);
cycle1s();	
}
int p1c=0;
p1c = read_p1credit();
if(p1c==0) { CALLNEOGEOF(GAME);}
}

void  NEOGEO_USER showTitleAES(void) { //AES System call from GAME
uint16_t  pal_tile0[16];
setpal(pal_tile0,BLACK,BLACK,0xFFF,RED,BLUE,MIDGREEN,CYAN,ORANGE,MAGENTA,RED,WHITE,BLUE,RED,BLUE,CYAN,RED);
load_palettes(pal_tile0,PALETTES);
waitVbl();
fixtext_out(15,10,"TITLE MODE AES",0);
playSoundtest(0x24);
int i =0;
for(i=0;i<5;i++) {
fix_svalue1(13,15,i,0,48);
cycle1s();	
}
}

void NEOGEO_USER WORK_INIT(void) {
uint32_t *p1 = RAMSTART;
int i = 0;
for (i=1;i<=32768;i++)
*p1++=0;
}

void NEOGEO_USER DISPLAY_INIT(void) {

ASM_START	
ASM_MVW(#0x8000,PALETTES)
ASM_MVW(#0xFFF,PALETTES+8190) 
ASM_JSR(SYS_FIX_CLEAR) //; jump to the FIX_CLEAR subroutine
ASM_JSR(SYS_LSP_1ST) //; jump to the LSP_1st subroutine 
ASM_JSR(clearSprs)
ASM_JSR(clearFix)
:
:
:
ASM_END	
}

void NEOGEO_USER INIT_GAME(void) {
ASM_START
ASM_JSR(WORK_INIT)
ASM_JSR(DISPLAY_INIT)
:
:
:
ASM_END	
}

void NEOGEO_USER DEMO_GAME(void) {

uint16_t  pal_tile0[16];
uint16_t  pal_tile1[16];
uint16_t  pal_tile2[16];
int   p1c = 0;
float d=10;
float dd=2;
d=dd/d;
float p = 87;
d = ln2(p);
setpal(pal_tile0,BLACK,BLACK,0xFFF,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE,BLUE);
load_palettes(pal_tile0,PALETTES);
setpal(pal_tile1,BLACK,BLACK,RED,RED,RED,RED,RED,RED,RED,RED,RED,RED,RED,RED,RED,RED);
load_palettes(pal_tile1,PALETTES+PALOFFSET);
setpal(pal_tile2,BLACK,BLACK,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN,GREEN);
load_palettes(pal_tile2,PALETTES+PALOFFSET*2);
//waitVbl()	;
//showScreen9();
//cyclexs(3);
clearSprs();
clearFix();
waitVbl();

fixtext_out(15,10,"DEMO MODE",0);
fixtext_out(15,11,"ABCDEFGHIJKLMNOP",0);
fixtext_out(15,12,"ABCDEFGHIJKLMNOP",0x1);
fixtext_out(15,13,"ABCDEFGHIJKLMNOP",0x2);
mess_outtest();
playSoundtest(0x21);//(0x25);
p1c = read_p1credit();
display_digit(15,14,123456789,0,48);
fixtext_out(15,15,"P1C: ",0);
display_digit(20,15,p1c,0,48);
int i = 0;
for(i=0;i<10;i++) {
fix_svalue1(27,8,i,0,48);
p1c = read_p1credit();
display_digit(15,14,123456789,0,48);
fixtext_out(15,15,"P1C: ",0);
display_digit(20,15,p1c,0,48);
cycle1s();	
}}

void NEOGEO_USER START_GAME(void) {
uint16_t  pal_tile0[16];
uint16_t  pal_tile1[16];
setpal(pal_tile0,BLACK,BLACK,0xFFF,BLUE,BLUE,BLUE,BLACK,BLUE,BLUE,BLUE,BLUE,BLUE,BLACK,BLUE,BLACK,BLUE);
load_palettes(pal_tile0,PALETTES);
setpal(pal_tile1,BLACK,BLACK,0xFFF,RED,RED,RED,BLACK,RED,RED,RED,RED,RED,RED,RED,BLACK,RED);
load_palettes(pal_tile1,PALETTES+PALOFFSET*2);
clearFix();
clearSprs();
fixtext_out(15,10,"STARTING GAME",0);
cyclexs(1);
fixtext_out(15,10,"LOADING   ...",0);
cyclexs(2);
maingame();
CALLNEOGEOF(GAME);
}
#pragma GCC pop_options
