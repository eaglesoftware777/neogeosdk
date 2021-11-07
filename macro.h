#ifndef MACRO_H
#define MACRO_H
#define LN10 2.3025850929940456840179914546844
#define lsize sizeof(uint32_t)
#define lmask (lsize - 1)
#define MESSPOINTZERO 0x0000
#define COMMAND0 0x0000
#define COMMAND1BF 0x0001
#define COMMAND1BL 0x0101
#define COMMAND1WF 0x0201
#define COMMAND1WL 0x0301
#define ZENDCODEFF 0x00FF
#define COMMAND2INC20 0x2002 
#define COMMAND2INC1 0x0102 
#define COMMAND3 0x0003
#define COMMAND4 0x0004
#define COMMAND5 0x0005
#define COMMAND6 0x0006
#define COMMAND7 0x0007
#define COMMAND8T1 0x0108
#define COMMAND8T2 0x0208
#define COMMAND8T3 0x0308
#define COMMAND8T4 0x0408
#define COMMAND8T5 0x0508
#define COMMAND8T6 0x0608
#define COMMAND8T7 0x0708
#define COMMAND8T8 0x0808
#define COMMAND9T1 0x0109
#define COMMAND9T2 0x0209
#define COMMAND9T3 0x0309
#define COMMAND9T4 0x0409
#define COMMAND9T5 0x0509
#define COMMAND9T6 0x0609
#define COMMAND9T7 0x0709
#define COMMAND9T8 0x0809 
#define COMMANDA 0x000A
#define COMMANDB 0x000B
#define COMMANDC1 0x010C
#define COMMANDC2 0x020C
#define COMMANDC3 0x030C
#define COMMANDC4 0x040C
#define COMMANDC5 0x050C
#define COMMANDC6 0x060C
#define COMMANDC7 0x070C
#define COMMANDC8 0x080C
#define COMMANDC9 0x090C
#define COMMANDCA 0x0A0C
#define COMMANDD1 0x010D
#define COMMANDD2 0x020D
#define COMMANDD3 0x030D
#define COMMANDD4 0x040D
#define COMMANDD5 0x050D
#define COMMANDD6 0x060D
#define COMMANDD7 0x070D
#define COMMANDD8 0x080D
#define COMMANDD9 0x090D
#define COMMANDDA 0x0A0D




 
//Hard DIPs:
#define DIPSW_SETTINGS   0
#define DIPSW_CHUTES     1
#define DIPSW_CTRL       2
#define DIPSW_ID0        3
#define DIPSW_ID1        4
#define DIPSW_MULTI      5
#define DIPSW_FREEPLAY   6
#define DIPSW_FREEZE     7
//


//VRAM zones:
#define SCB1_ADDR        0x0000   //Sprite tilemaps
#define FIXMAP           0x7000
#define SCB2_ADDR        0x8000   //Sprite shrink values
#define SCB3_ADDR        0x8200   //Sprite Y positions, eigts and flags
#define SCB4_ADDR        0x8400   //Sprite X positions
 
//Basic colors:
#define BLACK            0x8000
#define MIDRED           0x4700
#define RED              0x4F00
#define MIDGREEN         0x2070
#define GREEN            0x20F0
#define MIDBLUE          0x1007
#define BLUE             0x100F
#define MIDYELLOW        0x6770
#define YELLOW           0x6FF0
#define MIDMAGENTA       0x5707
#define MAGENTA          0x5F0F
#define MIDCYAN          0x3077
#define CYAN             0x30FF
#define ORANGE           0x6F70
#define MIDGREY          0x7777
#define WHITE            0x7FFF
 
//
#define SR        		 0x2700   //
//Zones:
#define RAMSTART         0x100000   //68k work RAM
#define PALETTES         0x400000   //Palette RAM
#define BACKDROP         PALETTES+(16*2*256)
#define PALOFFSET		 0x20
#define MEMCARD          0x800000   //Memory card
#define SYSROM           0xC00000   //System ROM
 
//Registers:
#define REG_P1CNT        0x300000
#define REG_DIPSW        0x300001   //#define DIPSWitces/Watcdog
#define REG_SOUND        0x320000   //Z80 I/O
#define REG_STATUS_A     0x320001
#define REG_P2CNT        0x340000
#define REG_STATUS_B     0x380000
#define REG_POUTPUT      0x380001   //Joypad port outputs
#define REG_SLOT         0x380021   //Slot select
 
#define REG_NOSHADOW     0x3A0001   //Video output normal/dark
#define REG_SHADOW       0x3A0011
#define REG_BRDFIX       0x3A000B   //Use embedded fix tileset
#define REG_CRTFIX       0x3A001B   //Use game fix tileset
#define REG_PALBANK1     0x3A000F   //Use palette bank 1
#define REG_PALBANK0     0x3A001F   //Use palette bank 0 (default);
 
#define VRAM_ADDR        0x3C0000
#define VRAM_RW          0x3C0002
#define VRAM_INC         0x3C0004
#define REG_LSPCMODE     0x3C0006
#define REG_TIMERHIGH    0x3C0008
#define REG_TIMERLOW     0x3C000A
#define REG_IRQACK       0x3C000C
#define REG_TIMERSTOP    0x3C000E
 
//System ROM calls:
#define SYS_INT1           0xC00438
#define SYS_RETURN         0xC00444
#define SYS_IO             0xC0044A
#define SYS_CREDIT_CHECK   0xC00450
#define SYS_CREDIT_DOWN    0xC00456
#define SYS_READ_CALENDAR  0xC0045C   //MVS only
#define SYS_CARD           0xC00468
#define SYS_CARD_ERROR     0xC0046E
#define SYS_HOWTOPLAY      0xC00474   //MVS only
#define SYS_FIX_CLEAR      0xC004C2
#define SYS_LSP_1ST        0xC004C8   //Clear sprites
#define SYS_MESS_OUT       0xC004CE
 
//RAM locations:
#define BIOS_SYSTEM_MODE   0x10FD80
#define BIOS_MVS_FLAG      0x10FD82
#define BIOS_COUNTRY_CODE  0x10FD83
#define BIOS_GAME_DIP      0x10FD84   //Start of soft DIPs settings (up to 10FD93);

//Set by SYS_IO:
#define BIOS_P1STATUS    0x10FD94
#define BIOS_P1PREVIOUS  0x10FD95
#define BIOS_P1CURRENT   0x10FD96
#define BIOS_P1CHANGE    0x10FD97
#define BIOS_P1REPEAT    0x10FD98
#define BIOS_P1TIMER     0x10FD99
 
#define BIOS_P2STATUS    0x10FD9A
#define BIOS_P2PREVIOUS  0x10FD9B
#define BIOS_P2CURRENT   0x10FD9C
#define BIOS_P2CHANGE    0x10FD9D
#define BIOS_P2REPEAT    0x10FD9E
#define BIOS_P2TIMER     0x10FD99
 
#define BIOS_STATCURNT     0x10FDAC
#define BIOS_STATCHANGE    0x10FDAD
#define BIOS_USER_REQUEST     0x10FDAE
#define BIOS_USER_MODE     0x10FDAF
#define BIOS_START_FLAG    0x10FDB4
#define BIOS_PLAYER1_MODE	0x10FDB6
#define BIOS_PLAYER2_MODE	0x10FDB7
#define BIOS_PLAYER3_MODE	0x10FDB8
#define BIOS_PLAYER4_MODE	0x10FDB6
#define BIOS_MESS_POINT    0x10FDBE
#define BIOS_MESS_BUSY     0x10FDC2
#define BIOS_MESS_BUFFER   0x10FF00
 
//Memory card:
#define BIOS_CRDF        0x10FDC4   //Byte: function to perform wen calling BIOSF_CRDACCESS
#define BIOS_CRDRESULT   0x10FDC6   //Byte: 00 on success, else 80+ and encodes te error
#define BIOS_CRDPTR      0x10FDC8   //Longword: pointer to read from/write to
#define BIOS_CRDSIZE     0x10FDCC   //Word: ow muc data to read/write from/to card
#define BIOS_CRDNGH      0x10FDCE   //Word: usually game NGH. Unique identifier for te game tat owns te save file
#define BIOS_CRDFILE     0x10FDD0   //Word: eac NGH as up to 16 save files associated wit
 
//Calendar, MVS only (in BCD);:
#define BIOS_YEAR        0x10FDD2   //Last 2 digits of year
#define BIOS_MONTH       0x10FDD3
#define BIOS_DAY         0x10FDD4
#define BIOS_WEEKDAY     0x10FDD5   //Sunday = 0, Monday = 1 ... Saturday = 6
#define BIOS_HOUR        0x10FDD6  //24 our time
#define BIOS_MINUTE      0x10FDD7
#define BIOS_SECOND      0x10FDD8
 
#define BIOS_SELECT_TIMER  0x10FDDA   //Byte: game start countdown
#define BIOS_DEVMODE       0x10FE80   //Byte: non-zero for developer mode
 
//Upload system ROM call:
#define BIOS_UPDEST      0x10FEF4   //Longword
#define BIOS_UPSRC       0x10FEF8   //Longword
#define BIOS_UPSIZE      0x10FEFC   //Longword
#define BIOS_UPZONE      0x10FEDA   //Byte: zone (0=PRG, 1=FIX, 2=SPR, 3=Z80, 4=PCM, 5=PAT);
#define BIOS_UPBANK      0x10FEDB   //Byte: bank
 
#define SOUND_STOP       0xD00046
#define P1_CREDITS		 0xD00034
#define P2_CREDITS		 0xD00035

//Button definitions:
#define JOY_UP		0x0001
#define JOY_DOWN	0x0002
#define JOY_LEFT	0x0004
#define JOY_RIGHT	0x0008
#define BUTTON_A	0x0010
#define BUTTON_B	0x0020
#define BUTTON_C	0x0040
#define BUTTON_D	0x0080
#define START1		0x0100
#define SELECT1		0x0200
#define START2		0x0400
#define SELECT2		0x0800

#define CNT_UP	         0
#define CNT_DOWN	 1
#define CNT_LEFT	 2
#define CNT_RIGHT	 3
#define CNT_A	         4
#define CNT_B	         5
#define CNT_C	         6
#define CNT_D	         7
#define CNT_START1       0
#define CNT_SELECT1      1
#define CNT_START2       2
#define CNT_SELECT2      3
//////////////
#define BIOS_WORKRAM  0x10F300
#define USER_WORKRAM  0x100000
#define PALETTE_REFERENCE	 0x400000		// Reference Color (must be 8000);
#define FLAG_DOTASKS          0

//USER RAM
#define VertBlank		0x100000		// Vertical blank flag
#define ScrollPos		0x100004		// Scroll position
#define VAL(sym) #sym 
#define ASM_START0    asm   ( 
#define ASM_START    asm volatile  ( 
#define ASM_START1    asm volatile goto ( 
#define ASM_START2	__asm__ __volatile__ (
//#define ASM_EQU(sym) "\t.equ\t" VAL(sym) "\t,\t" VAL(sym) "\n\t"
#define NEO_REGISTER(sym) *(volatile uint16_t*)(sym)
#define NEO_REGISTER8(sym) *(volatile uint8_t*)(sym)
#define NEO_REGISTER32(sym) *(volatile uint32_t*)(sym)

#define ASM_EQU(sym1,sym2) "\t.equ\t" VAL(sym1) "\t,\t"  VAL(sym2)  "\n\t"
#define ASM_ASCII(sym)  "\t.ascii\t" VAL(sym) "\n\t"
#define ASM_DL(sym)  "\t.dc.l\t" VAL(sym) "\n\t"
#define ASM_DW(sym)  "\t.dc.w\t" VAL(sym) "\n\t"
#define ASM_DW4(sym1,sym2,sym3,sym4) "\t.dc.w\t" VAL(sym1) "\t,\t"  VAL(sym2) "\t,\t"  VAL(sym3) "\t,\t"  VAL(sym4)  "\n\t"
#define ASM_DW2(sym1,sym2) "\t.dc.w\t" VAL(sym1) "\t,\t"  VAL(sym2)  "\n\t"
#define ASM_DB(sym)  "\t.dc.b\t" VAL(sym) "\n\t"
#define ASM_DB2(sym1,sym2) "\t.dc.b\t" VAL(sym1) "\t,\t"  VAL(sym2)  "\n\t"
#define ASM_DB6(sym1,sym2,sym3,sym4,sym5,sym6) "\t.dc.b\t" VAL(sym1) "\t,\t"  VAL(sym2) "\t,\t"  VAL(sym3) "\t,\t"  VAL(sym4)"\t,\t"  VAL(sym5) "\t,\t" VAL(sym6)"\t,\n" 
#define ASM_JMP(sym) "\tjmp\t" VAL(sym) "\n\t"
#define ASM_DBF(sym1,sym2) "\tdbf\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_RAW(sym) "\t" VAL(sym) "\n\t"
#define ASM_L(sym) "\t" VAL(sym) ":\n\t"
#define ASM_MV(sym1,sym2) "\tmove\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_MVB(sym1,sym2) "\tmove.b\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_MVW(sym1,sym2) "\tmove.w\t" VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_MVL(sym1,sym2) "\tmove.l\t" VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_MVQ(sym1,sym2) "\tmoveq\t" VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_MVQL(sym1,sym2) "\tmoveq.l\t" VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_MVAL(sym1,sym2) "\tmovea.l\t" VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_MVEML(sym1,sym2) "\tmovem.l\t" VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_MVAW(sym1,sym2) "\tmovea.w\t" VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_LEA(sym1,sym2) "\tlea\t" VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_JSR(sym) "\tjsr\t" VAL(sym) "\n\t"
#define ASM_LSLB(sym1,sym2) "\tlsl.b\t" VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_RTS  "\t rts \t \n\t"
#define ASM_RTE  "\t rte \t \n\t"
#define ASM_BTST(sym1,sym2) "\tbtst\t" VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_BSETB(sym1,sym2) "\tbset.b\t" VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_BNES(sym1) "\tbne.s\t" VAL(sym1)  "\n\t"
#define ASM_BNEB(sym1) "\tbne.b\t" VAL(sym1)  "\n\t"
#define ASM_BNE(sym1) "\tbne\t" VAL(sym1)  "\n\t"
#define ASM_BEQ(sym1) "\tbeq\t" VAL(sym1)  "\n\t"
#define ASM_BEQS(sym1) "\tbeq.s\t" VAL(sym1)  "\n\t"
#define ASM_BCLRB(sym1,sym2) "\tbclr.b\t" VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"

#define ASM_MVML(sym1,sym2) "\tmovem.l\t" VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_TSTB(sym1) "\ttst.b\t" VAL(sym1) "\n\t"
#define ASM_TSTW(sym1) "\ttst.w\t" VAL(sym1) "\n\t"
#define ASM_CLRW(sym1) "\tclr.w\t" VAL(sym1) "\n\t"
#define ASM_CLRL(sym1) "\tclr.l\t" VAL(sym1) "\n\t"
#define ASM_DBRA(sym1,sym2)  "\tdbra\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_CMPB(sym1,sym2)  "\tcmp.b\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_ANDB(sym1,sym2)  "\tand.b\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_CMPW(sym1,sym2)  "\tcmp.w\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_SUBW(sym1,sym2)  "\tsub.w\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_SUBQ(sym1,sym2)  "\tsub.q\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_SUBQB(sym1,sym2)  "\tsubq.b\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_SUBI(sym1,sym2)  "\tsub.i\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"

#define ASM_ADDW(sym1,sym2)  "\tadd.w\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_ADDQL(sym1,sym2)  "\taddq.l\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_ADDQW(sym1,sym2)  "\taddq.w\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_ADDQB(sym1,sym2)  "\taddq.b\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_ANDIB(sym1,sym2)  "\tandi.b\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_ANDIW(sym1,sym2)  "\tandi.w\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_ANDIL(sym1,sym2)  "\tandi.l\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_LSRB(sym1,sym2)  "\tlsr.b\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_LSRW(sym1,sym2)  "\tlsr.w\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_LSRL(sym1,sym2)  "\tlsr.l\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_ORIB(sym1,sym2)  "\tori.b\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_ORIW(sym1,sym2)  "\tori.w\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"
#define ASM_ORIL(sym1,sym2)  "\tori.l\t"  VAL(sym1) "\t,\t"  VAL(sym2) "\n\t"



#define ASM_BRAS(sym1) "\tbra.s\t" VAL(sym1)  "\n\t"
#define ASM_BRA(sym1) "\tbra\t" VAL(sym1)  "\n\t"
#define ASM_NOP "\tnop\t" "\n\t"
#define ASM_JEQ(sym1) "\tjeq\t" VAL(sym1) "\n\t"

#define ASM_END );
#define NEOGEO_USER __attribute__ ((section ("neogeo_user")))
#define NEOGEO_INTERRUPT __attribute__ ((interrupt))
#define CALLNEOGEOF(sym1)  ((void(*)(void))sym1)()
#endif
