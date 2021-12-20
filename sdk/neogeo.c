#include <stdint.h>
#include "macro.h"
void ZD_ENTRY(void);
void CHK_ENTRY(void);
void TRAPV_ENTRY(void);
void VBlank(void);
void IRQ2(void);
void IRQ3(void);
void INT4(void);
void INT5(void);
void INT6(void);
void INT7(void);
void USER(void);
void PLAYER_START(void);
void DEMO_END(void);
void COIN_SOUND(void);
/*void USER(void);		
void PLAYER_START(void);
void DEMO_END(void);	
void COIN_SOUND(void);*/
//#pragma location=0x0
//#define INT_OP_GROUP = 3
//int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, intptr_t,  uintptr_t
ASM_START0
	"\t.cpu 68000\t\n"
	"\t.equ	DIPSW_SETTINGS,	0\t\n"
	"\t.equ	DIPSW_CHUTES,	0x1\t\n"
	"\t.equ	DIPSW_CTRL,	0x2\t\n"
	"\t.equ	DIPSW_ID0,	0x3\t\n"
	"\t.equ	DIPSW_ID1,	0x4\t\n"
	"\t.equ	DIPSW_MULTI,	0x5\t\n"
	"\t.equ	DIPSW_FREEPLAY,	0x6\t\n"
	"\t.equ	DIPSW_FREEZE,	0x7\t\n"
	"\t.equ	SCB1,	0x0000\t\n"
	"\t.equ	FIXMAP,	0x7000\t\n"
	"\t.equ	SCB2,	0x8000\t\n"
	"\t.equ	SCB3,	0x8200\t\n"
	"\t.equ	SCB4,	0x8400\t\n"
	"\t.equ	BLACK,	0x8000\t\n"
	"\t.equ	MIDRED,	0x4700\t\n"
	"\t.equ	RED,	0x4F00\t\n"
	"\t.equ	MIDGREEN,	0x2070\t\n"
	"\t.equ	GREEN,	0x20F0\t\n"
	"\t.equ	MIDBLUE,	0x1007\t\n"
	"\t.equ	BLUE,	0x100F\t\n"
	"\t.equ	MIDYELLOW,	0x6770\t\n"
	"\t.equ	YELLOW,	0x6FF0\t\n"
	"\t.equ	MIDMAGENTA,	0x5707\t\n"
	"\t.equ	MAGENTA,	0x5F0F\t\n"
	"\t.equ	MIDCYAN,	0x3077\t\n"
	"\t.equ	CYAN,	0x30FF\t\n"
	"\t.equ	ORANGE,	0x6F70\t\n"
	"\t.equ	MIDGREY,	0x7777\t\n"
	"\t.equ	WHITE,	0x7FFF\t\n"
	"\t.equ	RAMSTART,	0x100000\t\n"
	"\t.equ	PALETTES,	0x400000\t\n"
	"\t.equ	BACKDROP,	0x400000+(16*2*256)\t\n"
	"\t.equ	MEMCARD,	0x800000\t\n"
	"\t.equ	SYSROM,	0xC00000\t\n"
	"\t.equ	REG_P1CNT,	0x300000\t\n"
	"\t.equ	REG_DIPSW,	0x300001\t\n"
	"\t.equ	REG_SOUND,	0x320000\t\n"
	"\t.equ	REG_STATUS_A,	0x320001\t\n"
	"\t.equ	REG_P2CNT,	0x340000\t\n"
	"\t.equ	REG_STATUS_B,	0x380000\t\n"
	"\t.equ	REG_POUTPUT,	0x380001\t\n"
	"\t.equ	REG_SLOT,	0x380021\t\n"
	"\t.equ	REG_NOSHADOW,	0x3A0001\t\n"
	"\t.equ	REG_SHADOW,	0x3A0011\t\n"
	"\t.equ	REG_BRDFIX,	0x3A000B\t\n"
	"\t.equ	REG_CRTFIX,	0x3A001B\t\n"
	"\t.equ	REG_PALBANK1,	0x3A000F\t\n"
	"\t.equ	REG_PALBANK0,	0x3A001F\t\n"
	"\t.equ	VRAM_ADDR,	0x3C0000\t\n"
	"\t.equ	VRAM_RW,	0x3C0002\t\n"
	"\t.equ	VRAM_MOD,	0x3C0004\t\n"
	"\t.equ	REG_LSPCMODE,	0x3C0006\t\n"
	"\t.equ	REG_TIMERHIGH,	0x3C0008\t\n"
	"\t.equ	REG_TIMERLOW,	0x3C000A\t\n"
	"\t.equ	REG_IRQACK,	0x3C000C\t\n"
	"\t.equ	REG_TIMERSTOP,	0x3C000E\t\n"
	"\t.equ	SYS_INT1,	0xC00438\t\n"
	"\t.equ	SYS_RETURN,	0xC00444\t\n"
	"\t.equ	SYS_IO,	0xC0044A\t\n"
	"\t.equ	SYS_CREDIT_CHECK,	0xC00450\t\n"
	"\t.equ	SYS_CREDIT_DOWN,	0xC00456\t\n"
	"\t.equ	SYS_READ_CALENDAR,	0xC0045C\t\n"
	"\t.equ	SYS_CARD,	0xC00468\t\n"
	"\t.equ	SYS_CARD_ERROR,	0xC0046E\t\n"
	"\t.equ	SYS_HOWTOPLAY,	0xC00474\t\n"
	"\t.equ	SYS_FIX_CLEAR,	0xC004C2\t\n"
	"\t.equ	SYS_LSP_1ST,	0xC004C8\t\n"
	"\t.equ	SYS_MESS_OUT,	0xC004CE\t\n"
	"\t.equ	BIOS_SYSTEM_MODE,	0x10FD80\t\n"
	"\t.equ	BIOS_MVS_FLAG,	0x10FD82\t\n"
	"\t.equ	BIOS_COUNTRY_CODE,	0x10FD83\t\n"
	"\t.equ	BIOS_GAME_DIP,	0x10FD84\t\n"
	"\t.equ	BIOS_P1STATUS,	0x10FD94\t\n"
	"\t.equ	BIOS_P1PREVIOUS,	0x10FD95\t\n"
	"\t.equ	BIOS_P1CURRENT,	0x10FD96\t\n"
	"\t.equ	BIOS_P1CHANGE,	0x10FD97\t\n"
	"\t.equ	BIOS_P1REPEAT,	0x10FD98\t\n"
	"\t.equ	BIOS_P1TIMER,	0x10FD99\t\n"
	"\t.equ	BIOS_P2STATUS,	0x10FD9A\t\n"
	"\t.equ	BIOS_P2PREVIOUS,	0x10FD9B\t\n"
	"\t.equ	BIOS_P2CURRENT,	0x10FD9C\t\n"
	"\t.equ	BIOS_P2CHANGE,	0x10FD9D\t\n"
	"\t.equ	BIOS_P2REPEAT,	0x10FD9E\t\n"
	"\t.equ	BIOS_P2TIMER,	0x10FD99\t\n"
	"\t.equ	BIOS_STATCURNT,	0x10FDAC\t\n"
	"\t.equ	BIOS_STATCHANGE,	0x10FDAD\t\n"
	"\t.equ	BIOS_USER_REST,	0x10FDAE\t\n"
	"\t.equ	BIOS_USER_MODE,	0x10FDAF\t\n"
	"\t.equ	BIOS_START_FLAG,	0x10FDB4\t\n"
	"\t.equ	BIOS_MESS_POINT,	0x10FDBE\t\n"
	"\t.equ	BIOS_MESS_BUSY,	0x10FDC2\t\n"
	"\t.equ	BIOS_CRDF,	0x10FDC4\t\n"
	"\t.equ	BIOS_CRDRESULT,	0x10FDC6\t\n"
	"\t.equ	BIOS_CRDPTR,	0x10FDC8\t\n"
	"\t.equ	BIOS_CRDSIZE,	0x10FDCC\t\n"
	"\t.equ	BIOS_CRDNGH,	0x10FDCE\t\n"
	"\t.equ	BIOS_CRDFILE,	0x10FDD0\t\n"
	"\t.equ	BIOS_YEAR,	0x10FDD2\t\n"
	"\t.equ	BIOS_MONTH,	0x10FDD3\t\n"
	"\t.equ	BIOS_DAY,	0x10FDD4\t\n"
	"\t.equ	BIOS_WEEKDAY,	0x10FDD5\t\n"
	"\t.equ	BIOS_HOUR,	0x10FDD6\t\n"
	"\t.equ	BIOS_MINUTE,	0x10FDD7\t\n"
	"\t.equ	BIOS_SECOND,	0x10FDD8\t\n"
	"\t.equ	BIOS_SELECT_TIMER,	0x10FDDA\t\n"
	"\t.equ	BIOS_DEVMODE,	0x10FE80\t\n"
	"\t.equ	BIOS_UPDEST,	0x10FEF4\t\n"
	"\t.equ	BIOS_UPSRC,	0x10FEF8\t\n"
	"\t.equ	BIOS_UPSIZE,	0x10FEFC\t\n"
	"\t.equ	BIOS_UPZONE,	0x10FEDA\t\n"
	"\t.equ	BIOS_UPBANK,	0x10FEDB\t\n"
	"\t.equ	SOUND_STOP,	0xD00046\t\n"
	"\t.equ	CNT_UP,	0x0\t\n"
	"\t.equ	CNT_DOWN,	0x1\t\n"
	"\t.equ	CNT_LEFT,	0x2\t\n"
	"\t.equ	CNT_RIGHT,	0x3\t\n"
	"\t.equ	CNT_A,	0x4\t\n"
	"\t.equ	CNT_B,	0x5\t\n"
	"\t.equ	CNT_C,	0x6\t\n"
	"\t.equ	CNT_D,	0x7\t\n"
	"\t.equ	CNT_START1,	0x0\t\n"
	"\t.equ	CNT_SELECT1,	0x1\t\n"
	"\t.equ	CNT_START2,	0x2\t\n"
	"\t.equ	CNT_SELECT2,	0x3\t\n"
	"\t.equ	BIOS_WORKRAM,	0x10F300\t\n"
	"\t.equ	FLAG_DOTASKS,	0x0\t\n"
	"\t.equ	VertBlank,	0x100000\t\n"
	"\t.equ	ScrollPos,	0x100004\t\n"
ASM_END
uint32_t * IVT[] __attribute__ ((section ("neogeo_ivt"))) = { 
					(uint32_t *)0X10F300,	
					(uint32_t *)0xC00402,	
					(uint32_t *)0xC00408,	
					(uint32_t *)0xC0040E,						
					(uint32_t *)0xC00414,	
					(uint32_t *)ZD_ENTRY,	
					(uint32_t *)CHK_ENTRY,	
					(uint32_t *)TRAPV_ENTRY,	
					(uint32_t *)0xC0041A,	
					(uint32_t *)0xC00420,	
					(uint32_t *)0xC00426,	
					(uint32_t *)0xC00426,	
					(uint32_t *)0xC00426,	
					(uint32_t *)0xC00426,	
					(uint32_t *)0xC00426,	
					(uint32_t *)0xC0042C,	
					(uint32_t *)0xC00426,	
					(uint32_t *)0xC00426,	
					(uint32_t *)0xC00426,	
					(uint32_t *)0xC00426,	
					(uint32_t *)0xC00426,	
					(uint32_t *)0xC00426,	
					(uint32_t *)0xC00426,	
					(uint32_t *)0xC00426,	
					(uint32_t *)0xC00432,	
					(uint32_t *)VBlank,		
					(uint32_t *)IRQ2,		
					(uint32_t *)IRQ3,		
					(uint32_t *)INT4,		
					(uint32_t *)INT5,		
					(uint32_t *)INT6,		
					(uint32_t *)INT7,		
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF,	
					(uint32_t *)0xFFFFFFFF	}; 
uint8_t neogeo_str[] __attribute__ ((section ("neogeo_init"))) = { 0x4e, 0x45, 0x4f, 0x2d, 0x47, 0x45, 0x4f };
uint8_t zerobyte __attribute__ ((section ("neogeo_init"))) = 0x00;
/*asm ("\t    .org 0x108 \t\n");*/
uint16_t id __attribute__ ((section ("neogeo_init"))) = 				0x555;  //0x52
uint32_t psize  __attribute__ ((section ("neogeo_init"))) = 			0x80000;
uint32_t pbcks __attribute__ ((section ("neogeo_init"))) = 			0x108000;
uint16_t dipsize  __attribute__ ((section ("neogeo_init"))) = 			0x1000;
//uint8_t	 bioslogo  __attribute__ ((section ("neogeo_init"))) = 	{0x2,0};
uint8_t	 bioslogoflag  __attribute__ ((section ("neogeo_init"))) = 	0x2;//0x0;
uint8_t	 bioslogotile  __attribute__ ((section ("neogeo_init"))) = 	0x0;//0x1B;
/********************************************************************/
/* neogeo_entry */
uint32_t NEOGEO_ENT0[]  __attribute__ ((section ("neogeo_entry"))) = {0x00000200, 0x00000280, 0x00000300};
uint32_t NEOGEO_ENT1[]  __attribute__ ((section ("neogeo_entry"))) = {0x4EFA0000+(USER-0x124)};
uint16_t nop1  __attribute__ ((section ("neogeo_entry"))) = 0x4e71;
uint32_t NEOGEO_ENT2[]  __attribute__ ((section ("neogeo_entry"))) = {0x4EFA0000+(PLAYER_START-0x12a)};
uint16_t nop2  __attribute__ ((section ("neogeo_entry"))) = 0x4e71;
uint32_t NEOGEO_ENT3[]  __attribute__ ((section ("neogeo_entry"))) = { 0x4EFA0000+(DEMO_END-0x130)};
uint16_t nop3  __attribute__ ((section ("neogeo_entry"))) = 0x4e71;
uint32_t NEOGEO_ENT4[]  __attribute__ ((section ("neogeo_entry"))) = { 0x4EFA0000+(COIN_SOUND-0x136)};
/********************************************************************/
//#pragma location=0x13a
uint32_t padds[] __attribute__ ((section ("neogeo_padds"))) = { 
	                 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
				     0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
				     0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
					 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
					0xFFFFFFFF
				   };
/*asm ("\t    .org 0x182 \t\n"
	"\t 	.dc.l ID2 \t\n");*/
uint32_t ID __attribute__ ((section ("neogeo_id2"))) = 0x186;
uint32_t ID2[] __attribute__ ((section ("neogeo_id2"))) = { 
0x76004A6D,0x0A146600,0x003C206D,0x0A043E2D,
0x0A0813C0,0x00300001,0x32100C01,0x00FF671A,
0x30280002,0xB02D0ACE,0x66103028,0x0004B02D,
0x0ACF6606,0xB22D0AD0,0x67085088,0x51CFFFD4,
0x36074E75,0x206D0A04,0x3E2D0A08,0x3210E049,
0x0C0100FF,0x671A3010,0xB02D0ACE,0x66123028,
0x0002E048,0xB02D0ACF,0x6606B22D,0x0AD06708,
0x588851CF,0xFFD83607
 };
uint16_t ID2END __attribute__ ((section ("neogeo_id2"))) = 0x4e75;
	
/*0x76004A6D,0x0A146600,0x003C206D,0x0A043E2D,
0x0A0813C0,0x00300001,0x32100C01,0x00FF671A,
0x30280002,0xB02D0ACE,0x66103028,0x0004B02D,
0x0ACF6606,0xB22D0AD0,0x67085088,0x51CFFFD4,
0x36074E75,0x206D0A04,0x3E2D0A08,0x3210E049,
0x0C0100FF,0x671A3010,0xB02D0ACE,0x66123028,
0x0002E048,0xB02D0ACF,0x6606B22D,0x0AD06708, 
0x588851CF,0xFFD83607,
0x00004e75 };*/

 struct JAPAN_DATA__ { 
	uint8_t game_name[16]; 
	uint8_t special_list[6]; 
	uint8_t option_list[10];
	uint8_t lives[12];
	uint8_t one[12];
	uint8_t two[12];
	uint8_t three[12];
	uint8_t for4[12]; 
	uint8_t howto[12];
	uint8_t with[12];
    uint8_t without[12];
	};
  struct JAPAN_DATA__ JAPAN_DATA __attribute__ ((section ("neogeo_gamedata"))) = { 
  	{0x30 , 0x31 , 0x32 , 0x33 , 0x34 , 0x35 , 0x36 , 0x37 , 0x38 , 0x39 , 0x41 , 0x42 , 0x43 , 0x44 , 0x45 , 0x46},
    {0xFF , 0xFF , 0xFF , 0xFF , 0xFF , 0xFF},
	{0x24 , 0x02 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00},
	{0x4c , 0x49 , 0x56 , 0x45 , 0x53 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},
	{0x31 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},
	{0x32 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},
	{0x33 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},
	{0x34 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},
	{0x48 , 0x4f , 0x57 , 0x20 , 0x54 , 0x4f , 0x20 , 0x50 , 0x4c , 0x41 , 0x59 , 0x20}, 
	{0x57 , 0x49 , 0x54 , 0x48 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},  
	{0x57 , 0x49 , 0x54 , 0x48 , 0x4f , 0x55 , 0x54 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 }
	};
 
 struct USA_DATA__ { 
	uint8_t game_name[16]; 
	uint8_t special_list[6]; 
	uint8_t option_list[10];
	uint8_t lives[12];
	uint8_t one[12];
	uint8_t two[12];
	uint8_t three[12];
	uint8_t for4[12]; 
	uint8_t howto[12];
	uint8_t with[12];
    uint8_t without[12];
	};
  struct USA_DATA__ USA_DATA __attribute__ ((section ("neogeo_gamedata"))) = { 
  	{0x30 , 0x31 , 0x32 , 0x33 , 0x34 , 0x35 , 0x36 , 0x37 , 0x38 , 0x39 , 0x41 , 0x42 , 0x43 , 0x44 , 0x45 , 0x46},
    {0xFF , 0xFF , 0xFF , 0xFF , 0xFF , 0xFF},
	{0x24 , 0x02 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00},
	{0x4c , 0x49 , 0x56 , 0x45 , 0x53 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},
	{0x31 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},
	{0x32 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},
	{0x33 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},
	{0x34 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},
	{0x48 , 0x4f , 0x57 , 0x20 , 0x54 , 0x4f , 0x20 , 0x50 , 0x4c , 0x41 , 0x59 , 0x20}, 
	{0x57 , 0x49 , 0x54 , 0x48 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},  
	{0x57 , 0x49 , 0x54 , 0x48 , 0x4f , 0x55 , 0x54 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20}
	};

 struct EUROPE_DATA__ { 
	uint8_t game_name[16]; 
	uint8_t special_list[6]; 
	uint8_t option_list[10];
	uint8_t lives[12];
	uint8_t one[12];
	uint8_t two[12];
	uint8_t three[12];
	uint8_t for4[12]; 
	uint8_t howto[12];
	uint8_t with[12];
    uint8_t without[12];
	};
  struct EUROPE_DATA__ EUROPE_DATA __attribute__ ((section ("neogeo_gamedata"))) = { 
  	{0x30 , 0x31 , 0x32 , 0x33 , 0x34 , 0x35 , 0x36 , 0x37 , 0x38 , 0x39 , 0x41 , 0x42 , 0x43 , 0x44 , 0x45 , 0x46},
    {0xFF , 0xFF , 0xFF , 0xFF , 0xFF , 0xFF},
	{0x24 , 0x02 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00},
	{0x4c , 0x49 , 0x56 , 0x45 , 0x53 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},
	{0x31 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},
	{0x32 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},
	{0x33 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},
	{0x34 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},
	{0x48 , 0x4f , 0x57 , 0x20 , 0x54 , 0x4f , 0x20 , 0x50 , 0x4c , 0x41 , 0x59 , 0x20}, 
	{0x57 , 0x49 , 0x54 , 0x48 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20},  
	{0x57 , 0x49 , 0x54 , 0x48 , 0x4f , 0x55 , 0x54 , 0x20 , 0x20 , 0x20 , 0x20 , 0x20}
	};

