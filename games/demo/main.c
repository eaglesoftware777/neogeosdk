/*******
https://eaglesoftware.biz
https://github.com/eaglesoftware777
https://github.com/eaglesoftware777/neogeosdk
******/

#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include "sdk/2d_engine/ng_chars.h"
#include "sdk/2d_engine/ng_engine.h"
#include "sprite_meta.h"
#include <stdint.h>

#include "games/demo/scenes/demo.h"
#include "games/demo/scenes/demo_sound.h"
#include "games/demo/scenes/demo_fix.h"

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-const-variable"
#endif
#define PAL_BLUE 1
#define PAL_RED  2
void NEOGEO_USER soundAutoDemo(void);
void NEOGEO_USER showEagleIntro(void);
void NEOGEO_USER showTitleScreen(void);
void NEOGEO_USER showGameOver(void);
void NEOGEO_USER showCharacterParade(void);
void NEOGEO_USER showPseudo3DLoop(void);
void NEOGEO_USER showScreen1(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen2(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen3(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen4(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen5(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen6(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen7(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen8(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen9(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen10(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen106(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
/* warrior walk/idle — sprite_001 to sprite_009 */
void NEOGEO_USER showScreen11(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen12(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen13(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen14(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen15(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen16(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen17(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen18(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen19(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
/* warrior attack — all 30 frames: sprite_010 to sprite_039 */
void NEOGEO_USER showScreen20(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen21(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen22(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen23(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen24(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen25(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen26(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen27(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen28(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen29(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen30(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen31(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen32(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen33(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen34(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen35(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen36(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen37(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen38(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen39(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen40(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen41(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen42(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen43(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen44(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen45(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen46(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen47(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen48(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen49(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
/* blade and energy effects — sprite_040 to sprite_052 (13 frames) */
void NEOGEO_USER showScreen50(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen51(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen52(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen53(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen54(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen55(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen56(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen57(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen58(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen59(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen60(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen61(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen62(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen63(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen64(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen65(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen66(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen67(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen68(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen69(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen70(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen71(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen72(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen73(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen74(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen75(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen76(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen77(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen78(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
/* eagle mascot — sprite_069 to sprite_082 (14 frames) */
void NEOGEO_USER showScreen79(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen80(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen81(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen82(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen83(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen84(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen85(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen86(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen87(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen88(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen89(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen90(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen91(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen92(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
/* portrait — sprite_083 */
void NEOGEO_USER showScreen93(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
/* late-scene NPC sprite frames: cat_01 to cat_12 */
void NEOGEO_USER showScreen94(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen95(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen96(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen97(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen98(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen99(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen100(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen101(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen102(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen103(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen104(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen105(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
/* late-scene backdrop */
void NEOGEO_USER showScreen106(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen107(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
/* title screens from artbox/in/titles/ */
void NEOGEO_USER showScreen108(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen109(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER show3DRaycaster(void);
int NEOGEO_USER playgame(void);


/*
 * Demo logic is centralized under games/demo/scenes/.  main.c keeps the generated
 * showScreenN() asset functions and only exposes the game entry wrappers.
 */
void NEOGEO_USER maingame(void)
{
    clearFix();
    clearSprs();
    demo_run_full_flow();
}

int NEOGEO_USER playgame(void)
{
    maingame();
    return 0;
}

void NEOGEO_USER showScreen1(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 1 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal1[16];
setpal(pal1,0x0,0x2545,0x4583,0x17ff,0x6864,0x3a78,0x7435,0x58a2,0x7367,0x5a73,0x5efe,0x3754,0x2464,0x17aa,0x1e93,0x2546);
uint16_t spriteMapS1_1[16] = {0x0,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0};
uint16_t spriteMapS1_2[16] = {0x1,0x11,0x21,0x31,0x41,0x51,0x61,0x71,0x81,0x91,0xa1,0xb1,0xc1,0xd1,0xe1,0xf1};
uint16_t spriteMapS1_3[16] = {0x2,0x12,0x22,0x32,0x42,0x52,0x62,0x72,0x82,0x92,0xa2,0xb2,0xc2,0xd2,0xe2,0xf2};
uint16_t spriteMapS1_4[16] = {0x3,0x13,0x23,0x33,0x43,0x53,0x63,0x73,0x83,0x93,0xa3,0xb3,0xc3,0xd3,0xe3,0xf3};
uint16_t spriteMapS1_5[16] = {0x4,0x14,0x24,0x34,0x44,0x54,0x64,0x74,0x84,0x94,0xa4,0xb4,0xc4,0xd4,0xe4,0xf4};
uint16_t spriteMapS1_6[16] = {0x5,0x15,0x25,0x35,0x45,0x55,0x65,0x75,0x85,0x95,0xa5,0xb5,0xc5,0xd5,0xe5,0xf5};
uint16_t spriteMapS1_7[16] = {0x6,0x16,0x26,0x36,0x46,0x56,0x66,0x76,0x86,0x96,0xa6,0xb6,0xc6,0xd6,0xe6,0xf6};
uint16_t spriteMapS1_8[16] = {0x7,0x17,0x27,0x37,0x47,0x57,0x67,0x77,0x87,0x97,0xa7,0xb7,0xc7,0xd7,0xe7,0xf7};
uint16_t spriteMapS1_9[16] = {0x8,0x18,0x28,0x38,0x48,0x58,0x68,0x78,0x88,0x98,0xa8,0xb8,0xc8,0xd8,0xe8,0xf8};
uint16_t spriteMapS1_10[16] = {0x9,0x19,0x29,0x39,0x49,0x59,0x69,0x79,0x89,0x99,0xa9,0xb9,0xc9,0xd9,0xe9,0xf9};
uint16_t spriteMapS1_11[16] = {0xa,0x1a,0x2a,0x3a,0x4a,0x5a,0x6a,0x7a,0x8a,0x9a,0xaa,0xba,0xca,0xda,0xea,0xfa};
uint16_t spriteMapS1_12[16] = {0xb,0x1b,0x2b,0x3b,0x4b,0x5b,0x6b,0x7b,0x8b,0x9b,0xab,0xbb,0xcb,0xdb,0xeb,0xfb};
uint16_t spriteMapS1_13[16] = {0xc,0x1c,0x2c,0x3c,0x4c,0x5c,0x6c,0x7c,0x8c,0x9c,0xac,0xbc,0xcc,0xdc,0xec,0xfc};
uint16_t spriteMapS1_14[16] = {0xd,0x1d,0x2d,0x3d,0x4d,0x5d,0x6d,0x7d,0x8d,0x9d,0xad,0xbd,0xcd,0xdd,0xed,0xfd};
uint16_t spriteMapS1_15[16] = {0xe,0x1e,0x2e,0x3e,0x4e,0x5e,0x6e,0x7e,0x8e,0x9e,0xae,0xbe,0xce,0xde,0xee,0xfe};
uint16_t spriteMapS1_16[16] = {0xf,0x1f,0x2f,0x3f,0x4f,0x5f,0x6f,0x7f,0x8f,0x9f,0xaf,0xbf,0xcf,0xdf,0xef,0xff};
load_palettes(pal1,PALETTES+PALOFFSET*16);
uint16_t SCB1_2common = setSCB1_2(16,0,0,0,0,0);
uint16_t spal1_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS1_1,spal1_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS1_2,spal1_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS1_3,spal1_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS1_4,spal1_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS1_5,spal1_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS1_6,spal1_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS1_7,spal1_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS1_8,spal1_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS1_9,spal1_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS1_10,spal1_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS1_11,spal1_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS1_12,spal1_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS1_13,spal1_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS1_14,spal1_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS1_15,spal1_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS1_16,spal1_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen2(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 2 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal2[16];
setpal(pal2,0x0,0x6462,0x4023,0x3921,0x6133,0x2ce0,0x1521,0x5151,0x2014,0x47c0,0x3005,0x2581,0x7e51,0x638a,0x1352,0x3355);
uint16_t spriteMapS2_1[16] = {0x100,0x110,0x120,0x130,0x140,0x150,0x160,0x170,0x180,0x190,0x1a0,0x1b0,0x1c0,0x1d0,0x1e0,0x1f0};
uint16_t spriteMapS2_2[16] = {0x101,0x111,0x121,0x131,0x141,0x151,0x161,0x171,0x181,0x191,0x1a1,0x1b1,0x1c1,0x1d1,0x1e1,0x1f1};
uint16_t spriteMapS2_3[16] = {0x102,0x112,0x122,0x132,0x142,0x152,0x162,0x172,0x182,0x192,0x1a2,0x1b2,0x1c2,0x1d2,0x1e2,0x1f2};
uint16_t spriteMapS2_4[16] = {0x103,0x113,0x123,0x133,0x143,0x153,0x163,0x173,0x183,0x193,0x1a3,0x1b3,0x1c3,0x1d3,0x1e3,0x1f3};
uint16_t spriteMapS2_5[16] = {0x104,0x114,0x124,0x134,0x144,0x154,0x164,0x174,0x184,0x194,0x1a4,0x1b4,0x1c4,0x1d4,0x1e4,0x1f4};
uint16_t spriteMapS2_6[16] = {0x105,0x115,0x125,0x135,0x145,0x155,0x165,0x175,0x185,0x195,0x1a5,0x1b5,0x1c5,0x1d5,0x1e5,0x1f5};
uint16_t spriteMapS2_7[16] = {0x106,0x116,0x126,0x136,0x146,0x156,0x166,0x176,0x186,0x196,0x1a6,0x1b6,0x1c6,0x1d6,0x1e6,0x1f6};
uint16_t spriteMapS2_8[16] = {0x107,0x117,0x127,0x137,0x147,0x157,0x167,0x177,0x187,0x197,0x1a7,0x1b7,0x1c7,0x1d7,0x1e7,0x1f7};
uint16_t spriteMapS2_9[16] = {0x108,0x118,0x128,0x138,0x148,0x158,0x168,0x178,0x188,0x198,0x1a8,0x1b8,0x1c8,0x1d8,0x1e8,0x1f8};
uint16_t spriteMapS2_10[16] = {0x109,0x119,0x129,0x139,0x149,0x159,0x169,0x179,0x189,0x199,0x1a9,0x1b9,0x1c9,0x1d9,0x1e9,0x1f9};
uint16_t spriteMapS2_11[16] = {0x10a,0x11a,0x12a,0x13a,0x14a,0x15a,0x16a,0x17a,0x18a,0x19a,0x1aa,0x1ba,0x1ca,0x1da,0x1ea,0x1fa};
uint16_t spriteMapS2_12[16] = {0x10b,0x11b,0x12b,0x13b,0x14b,0x15b,0x16b,0x17b,0x18b,0x19b,0x1ab,0x1bb,0x1cb,0x1db,0x1eb,0x1fb};
uint16_t spriteMapS2_13[16] = {0x10c,0x11c,0x12c,0x13c,0x14c,0x15c,0x16c,0x17c,0x18c,0x19c,0x1ac,0x1bc,0x1cc,0x1dc,0x1ec,0x1fc};
uint16_t spriteMapS2_14[16] = {0x10d,0x11d,0x12d,0x13d,0x14d,0x15d,0x16d,0x17d,0x18d,0x19d,0x1ad,0x1bd,0x1cd,0x1dd,0x1ed,0x1fd};
uint16_t spriteMapS2_15[16] = {0x10e,0x11e,0x12e,0x13e,0x14e,0x15e,0x16e,0x17e,0x18e,0x19e,0x1ae,0x1be,0x1ce,0x1de,0x1ee,0x1fe};
uint16_t spriteMapS2_16[16] = {0x10f,0x11f,0x12f,0x13f,0x14f,0x15f,0x16f,0x17f,0x18f,0x19f,0x1af,0x1bf,0x1cf,0x1df,0x1ef,0x1ff};
load_palettes(pal2,PALETTES+PALOFFSET*17);
uint16_t SCB1_2common = setSCB1_2(17,0,0,0,0,0);
uint16_t spal2_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal2_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal2_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal2_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal2_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal2_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal2_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal2_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal2_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal2_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal2_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal2_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal2_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal2_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal2_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal2_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS2_1,spal2_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS2_2,spal2_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS2_3,spal2_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS2_4,spal2_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS2_5,spal2_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS2_6,spal2_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS2_7,spal2_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS2_8,spal2_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS2_9,spal2_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS2_10,spal2_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS2_11,spal2_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS2_12,spal2_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS2_13,spal2_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS2_14,spal2_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS2_15,spal2_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS2_16,spal2_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen3(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 3 ******************************************/
uint16_t  pal3[16];
setpal(pal3,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal3,PALETTES+PALOFFSET*18);
}


void NEOGEO_USER showScreen4(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 4 ******************************************/
uint16_t  pal4[16];
setpal(pal4,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal4,PALETTES+PALOFFSET*19);
}


void NEOGEO_USER showScreen5(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 5 ******************************************/
uint16_t  pal5[16];
setpal(pal5,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal5,PALETTES+PALOFFSET*20);
}


void NEOGEO_USER showScreen6(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 6 ******************************************/
uint16_t  pal6[16];
setpal(pal6,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal6,PALETTES+PALOFFSET*21);
}


void NEOGEO_USER showScreen7(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 7 ******************************************/
uint16_t  pal7[16];
setpal(pal7,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal7,PALETTES+PALOFFSET*22);
}


void NEOGEO_USER showScreen8(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 8 ******************************************/
uint16_t  pal8[16];
setpal(pal8,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal8,PALETTES+PALOFFSET*23);
}


void NEOGEO_USER showScreen9(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 9 ******************************************/
uint16_t  pal9[16];
setpal(pal9,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal9,PALETTES+PALOFFSET*24);
}


void NEOGEO_USER showScreen10(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 10 ******************************************/
uint16_t  pal10[16];
setpal(pal10,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal10,PALETTES+PALOFFSET*25);
}


void NEOGEO_USER showScreen11(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 11 ******************************************/
uint16_t  pal11[16];
setpal(pal11,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal11,PALETTES+PALOFFSET*26);
}


void NEOGEO_USER showScreen12(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 12 ******************************************/
uint16_t  pal12[16];
setpal(pal12,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal12,PALETTES+PALOFFSET*27);
}


void NEOGEO_USER showScreen13(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 13 ******************************************/
uint16_t  pal13[16];
setpal(pal13,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal13,PALETTES+PALOFFSET*28);
}


void NEOGEO_USER showScreen14(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 14 ******************************************/
uint16_t  pal14[16];
setpal(pal14,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal14,PALETTES+PALOFFSET*29);
}


void NEOGEO_USER showScreen15(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 15 ******************************************/
uint16_t  pal15[16];
setpal(pal15,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal15,PALETTES+PALOFFSET*30);
}


void NEOGEO_USER showScreen16(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 16 ******************************************/
uint16_t  pal16[16];
setpal(pal16,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal16,PALETTES+PALOFFSET*31);
}


void NEOGEO_USER showScreen17(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 17 ******************************************/
uint16_t  pal17[16];
setpal(pal17,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal17,PALETTES+PALOFFSET*32);
}


void NEOGEO_USER showScreen18(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 18 ******************************************/
uint16_t  pal18[16];
setpal(pal18,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal18,PALETTES+PALOFFSET*33);
}


void NEOGEO_USER showScreen19(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 19 ******************************************/
uint16_t  pal19[16];
setpal(pal19,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal19,PALETTES+PALOFFSET*34);
}


void NEOGEO_USER showScreen20(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 20 ******************************************/
uint16_t  pal20[16];
setpal(pal20,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal20,PALETTES+PALOFFSET*35);
}


void NEOGEO_USER showScreen21(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 21 ******************************************/
uint16_t  pal21[16];
setpal(pal21,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal21,PALETTES+PALOFFSET*36);
}


void NEOGEO_USER showScreen22(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 22 ******************************************/
uint16_t  pal22[16];
setpal(pal22,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal22,PALETTES+PALOFFSET*37);
}


void NEOGEO_USER showScreen23(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 23 ******************************************/
uint16_t  pal23[16];
setpal(pal23,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal23,PALETTES+PALOFFSET*38);
}


void NEOGEO_USER showScreen24(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 24 ******************************************/
uint16_t  pal24[16];
setpal(pal24,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal24,PALETTES+PALOFFSET*39);
}


void NEOGEO_USER showScreen25(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 25 ******************************************/
uint16_t  pal25[16];
setpal(pal25,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal25,PALETTES+PALOFFSET*40);
}


void NEOGEO_USER showScreen26(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 26 ******************************************/
uint16_t  pal26[16];
setpal(pal26,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal26,PALETTES+PALOFFSET*41);
}


void NEOGEO_USER showScreen27(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 27 ******************************************/
uint16_t  pal27[16];
setpal(pal27,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal27,PALETTES+PALOFFSET*42);
}


void NEOGEO_USER showScreen28(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 28 ******************************************/
uint16_t  pal28[16];
setpal(pal28,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal28,PALETTES+PALOFFSET*43);
}


void NEOGEO_USER showScreen29(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 29 ******************************************/
uint16_t  pal29[16];
setpal(pal29,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal29,PALETTES+PALOFFSET*44);
}


void NEOGEO_USER showScreen30(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 30 ******************************************/
uint16_t  pal30[16];
setpal(pal30,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal30,PALETTES+PALOFFSET*45);
}


void NEOGEO_USER showScreen31(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 31 ******************************************/
uint16_t  pal31[16];
setpal(pal31,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal31,PALETTES+PALOFFSET*46);
}


void NEOGEO_USER showScreen32(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 32 ******************************************/
uint16_t  pal32[16];
setpal(pal32,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal32,PALETTES+PALOFFSET*47);
}


void NEOGEO_USER showScreen33(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 33 ******************************************/
uint16_t  pal33[16];
setpal(pal33,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal33,PALETTES+PALOFFSET*48);
}


void NEOGEO_USER showScreen34(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 34 ******************************************/
uint16_t  pal34[16];
setpal(pal34,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal34,PALETTES+PALOFFSET*49);
}


void NEOGEO_USER showScreen35(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 35 ******************************************/
uint16_t  pal35[16];
setpal(pal35,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal35,PALETTES+PALOFFSET*50);
}


void NEOGEO_USER showScreen36(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 36 ******************************************/
uint16_t  pal36[16];
setpal(pal36,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal36,PALETTES+PALOFFSET*51);
}


void NEOGEO_USER showScreen37(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 37 ******************************************/
uint16_t  pal37[16];
setpal(pal37,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal37,PALETTES+PALOFFSET*52);
}


void NEOGEO_USER showScreen38(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 38 ******************************************/
uint16_t  pal38[16];
setpal(pal38,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal38,PALETTES+PALOFFSET*53);
}


void NEOGEO_USER showScreen39(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 39 ******************************************/
uint16_t  pal39[16];
setpal(pal39,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal39,PALETTES+PALOFFSET*54);
}


void NEOGEO_USER showScreen40(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 40 ******************************************/
uint16_t  pal40[16];
setpal(pal40,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal40,PALETTES+PALOFFSET*55);
}


void NEOGEO_USER showScreen41(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 41 ******************************************/
uint16_t  pal41[16];
setpal(pal41,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal41,PALETTES+PALOFFSET*56);
}


void NEOGEO_USER showScreen42(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 42 ******************************************/
uint16_t  pal42[16];
setpal(pal42,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal42,PALETTES+PALOFFSET*57);
}


void NEOGEO_USER showScreen43(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 43 ******************************************/
uint16_t  pal43[16];
setpal(pal43,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal43,PALETTES+PALOFFSET*58);
}


void NEOGEO_USER showScreen44(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 44 ******************************************/
uint16_t  pal44[16];
setpal(pal44,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal44,PALETTES+PALOFFSET*59);
}


void NEOGEO_USER showScreen45(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 45 ******************************************/
uint16_t  pal45[16];
setpal(pal45,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal45,PALETTES+PALOFFSET*60);
}


void NEOGEO_USER showScreen46(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 46 ******************************************/
uint16_t  pal46[16];
setpal(pal46,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal46,PALETTES+PALOFFSET*61);
}


void NEOGEO_USER showScreen47(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 47 ******************************************/
uint16_t  pal47[16];
setpal(pal47,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal47,PALETTES+PALOFFSET*62);
}


void NEOGEO_USER showScreen48(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 48 ******************************************/
uint16_t  pal48[16];
setpal(pal48,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal48,PALETTES+PALOFFSET*63);
}


void NEOGEO_USER showScreen49(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 49 ******************************************/
uint16_t  pal49[16];
setpal(pal49,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal49,PALETTES+PALOFFSET*64);
}


void NEOGEO_USER showScreen50(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 50 ******************************************/
uint16_t  pal50[16];
setpal(pal50,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal50,PALETTES+PALOFFSET*65);
}


void NEOGEO_USER showScreen51(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 51 ******************************************/
uint16_t  pal51[16];
setpal(pal51,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal51,PALETTES+PALOFFSET*66);
}


void NEOGEO_USER showScreen52(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 52 ******************************************/
uint16_t  pal52[16];
setpal(pal52,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal52,PALETTES+PALOFFSET*67);
}


void NEOGEO_USER showScreen53(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 53 ******************************************/
uint16_t  pal53[16];
setpal(pal53,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal53,PALETTES+PALOFFSET*68);
}


void NEOGEO_USER showScreen54(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 54 ******************************************/
uint16_t  pal54[16];
setpal(pal54,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal54,PALETTES+PALOFFSET*69);
}


void NEOGEO_USER showScreen55(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 55 ******************************************/
uint16_t  pal55[16];
setpal(pal55,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal55,PALETTES+PALOFFSET*70);
}


void NEOGEO_USER showScreen56(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 56 ******************************************/
uint16_t  pal56[16];
setpal(pal56,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal56,PALETTES+PALOFFSET*71);
}


void NEOGEO_USER showScreen57(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 57 ******************************************/
uint16_t  pal57[16];
setpal(pal57,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal57,PALETTES+PALOFFSET*72);
}


void NEOGEO_USER showScreen58(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 58 ******************************************/
uint16_t  pal58[16];
setpal(pal58,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal58,PALETTES+PALOFFSET*73);
}


void NEOGEO_USER showScreen59(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 59 ******************************************/
uint16_t  pal59[16];
setpal(pal59,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal59,PALETTES+PALOFFSET*74);
}


void NEOGEO_USER showScreen60(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 60 ******************************************/
uint16_t  pal60[16];
setpal(pal60,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal60,PALETTES+PALOFFSET*75);
}


void NEOGEO_USER showScreen61(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 61 ******************************************/
uint16_t  pal61[16];
setpal(pal61,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal61,PALETTES+PALOFFSET*76);
}


void NEOGEO_USER showScreen62(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 62 ******************************************/
uint16_t  pal62[16];
setpal(pal62,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal62,PALETTES+PALOFFSET*77);
}


void NEOGEO_USER showScreen63(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 63 ******************************************/
uint16_t  pal63[16];
setpal(pal63,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal63,PALETTES+PALOFFSET*78);
}


void NEOGEO_USER showScreen64(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 64 ******************************************/
uint16_t  pal64[16];
setpal(pal64,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal64,PALETTES+PALOFFSET*79);
}


void NEOGEO_USER showScreen65(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 65 ******************************************/
uint16_t  pal65[16];
setpal(pal65,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal65,PALETTES+PALOFFSET*80);
}


void NEOGEO_USER showScreen66(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 66 ******************************************/
uint16_t  pal66[16];
setpal(pal66,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal66,PALETTES+PALOFFSET*81);
}


void NEOGEO_USER showScreen67(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 67 ******************************************/
uint16_t  pal67[16];
setpal(pal67,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal67,PALETTES+PALOFFSET*82);
}


void NEOGEO_USER showScreen68(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 68 ******************************************/
uint16_t  pal68[16];
setpal(pal68,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal68,PALETTES+PALOFFSET*83);
}


void NEOGEO_USER showScreen69(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 69 ******************************************/
uint16_t  pal69[16];
setpal(pal69,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal69,PALETTES+PALOFFSET*84);
}


void NEOGEO_USER showScreen70(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 70 ******************************************/
uint16_t  pal70[16];
setpal(pal70,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal70,PALETTES+PALOFFSET*85);
}


void NEOGEO_USER showScreen71(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 71 ******************************************/
uint16_t  pal71[16];
setpal(pal71,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal71,PALETTES+PALOFFSET*86);
}


void NEOGEO_USER showScreen72(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 72 ******************************************/
uint16_t  pal72[16];
setpal(pal72,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal72,PALETTES+PALOFFSET*87);
}


void NEOGEO_USER showScreen73(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 73 ******************************************/
uint16_t  pal73[16];
setpal(pal73,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal73,PALETTES+PALOFFSET*88);
}


void NEOGEO_USER showScreen74(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 74 ******************************************/
uint16_t  pal74[16];
setpal(pal74,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal74,PALETTES+PALOFFSET*89);
}


void NEOGEO_USER showScreen75(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 75 ******************************************/
uint16_t  pal75[16];
setpal(pal75,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal75,PALETTES+PALOFFSET*90);
}


void NEOGEO_USER showScreen76(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 76 ******************************************/
uint16_t  pal76[16];
setpal(pal76,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal76,PALETTES+PALOFFSET*91);
}


void NEOGEO_USER showScreen77(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 77 ******************************************/
uint16_t  pal77[16];
setpal(pal77,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal77,PALETTES+PALOFFSET*92);
}


void NEOGEO_USER showScreen78(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 78 ******************************************/
uint16_t  pal78[16];
setpal(pal78,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal78,PALETTES+PALOFFSET*93);
}


void NEOGEO_USER showScreen79(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 79 ******************************************/
uint16_t  pal79[16];
setpal(pal79,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal79,PALETTES+PALOFFSET*94);
}


void NEOGEO_USER showScreen80(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 80 ******************************************/
uint16_t  pal80[16];
setpal(pal80,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal80,PALETTES+PALOFFSET*95);
}


void NEOGEO_USER showScreen81(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 81 ******************************************/
uint16_t  pal81[16];
setpal(pal81,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal81,PALETTES+PALOFFSET*96);
}


void NEOGEO_USER showScreen82(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 82 ******************************************/
uint16_t  pal82[16];
setpal(pal82,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal82,PALETTES+PALOFFSET*97);
}


void NEOGEO_USER showScreen83(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 83 ******************************************/
uint16_t  pal83[16];
setpal(pal83,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal83,PALETTES+PALOFFSET*98);
}


void NEOGEO_USER showScreen84(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 84 ******************************************/
uint16_t  pal84[16];
setpal(pal84,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal84,PALETTES+PALOFFSET*99);
}


void NEOGEO_USER showScreen85(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 85 ******************************************/
uint16_t  pal85[16];
setpal(pal85,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal85,PALETTES+PALOFFSET*100);
}


void NEOGEO_USER showScreen86(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 86 ******************************************/
uint16_t  pal86[16];
setpal(pal86,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal86,PALETTES+PALOFFSET*101);
}


void NEOGEO_USER showScreen87(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 87 ******************************************/
uint16_t  pal87[16];
setpal(pal87,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal87,PALETTES+PALOFFSET*102);
}


void NEOGEO_USER showScreen88(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 88 ******************************************/
uint16_t  pal88[16];
setpal(pal88,0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321);
load_palettes(pal88,PALETTES+PALOFFSET*103);
}


void NEOGEO_USER showScreen89(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 89 ******************************************/
uint16_t  pal89[16];
setpal(pal89,0x0,0x3eef,0x346f,0x5acf,0x58af,0x7cdf,0x168f,0x579f,0x1def,0x1cdf,0x59bf,0x3abf,0x3bcf,0x1eff,0x39af,0x5eff);
load_palettes(pal89,PALETTES+PALOFFSET*104);
}


void NEOGEO_USER showScreen90(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 90 ******************************************/
uint16_t  pal90[16];
setpal(pal90,0x0,0x3abf,0x746f,0x335f,0x1eff,0x558f,0x768f,0x3ddf,0x5acf,0x1cdf,0x59bf,0x779f,0x18af,0x3bcf,0x39af,0x414f);
load_palettes(pal90,PALETTES+PALOFFSET*105);
}


void NEOGEO_USER showScreen91(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 91 ******************************************/
uint16_t  pal91[16];
setpal(pal91,0x0,0x3ecf,0x797f,0x1b9f,0x7a8f,0x5cbf,0xbaf,0x5eef,0x1caf,0x197f,0x5dcf,0x674f,0x1a8f,0x7caf,0x3dbf,0x586f);
load_palettes(pal91,PALETTES+PALOFFSET*106);
}


void NEOGEO_USER showScreen92(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 92 ******************************************/
uint16_t  pal92[16];
setpal(pal92,0x0,0x7edf,0x5baf,0x797f,0x1cbf,0x1b9f,0x1a8f,0x3ecf,0x1edf,0x785f,0x1fef,0x197f,0x7b9f,0x3caf,0x5caf,0x763f);
load_palettes(pal92,PALETTES+PALOFFSET*107);
}


void NEOGEO_USER showScreen93(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 93 ******************************************/
uint16_t  pal93[16];
setpal(pal93,0x0,0x1caf,0x1b9f,0x675f,0x1fef,0x174f,0x3ecf,0x1a8f,0x753d,0x1dbf,0x687e,0x686f,0x652f,0x553e,0x396f,0x575f);
load_palettes(pal93,PALETTES+PALOFFSET*108);
}


void NEOGEO_USER showScreen94(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 94 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal94[16];
setpal(pal94,0x0,0x3456,0x4ff2,0x6f10,0x3fe9,0x1115,0x6fa1,0xfff,0x48be,0x7910,0x18ff,0x32e,0x6964,0x448e,0x7400,0x0);
uint16_t spriteMapS94_1[16] = {0x5d00,0x5d10,0x5d20,0x5d30,0x5d40,0x5d50,0x5d60,0x5d70,0x5d80,0x5d90,0x5da0,0x5db0,0x5dc0,0x5dd0,0x5de0,0x5df0};
uint16_t spriteMapS94_2[16] = {0x5d01,0x5d11,0x5d21,0x5d31,0x5d41,0x5d51,0x5d61,0x5d71,0x5d81,0x5d91,0x5da1,0x5db1,0x5dc1,0x5dd1,0x5de1,0x5df1};
uint16_t spriteMapS94_3[16] = {0x5d02,0x5d12,0x5d22,0x5d32,0x5d42,0x5d52,0x5d62,0x5d72,0x5d82,0x5d92,0x5da2,0x5db2,0x5dc2,0x5dd2,0x5de2,0x5df2};
uint16_t spriteMapS94_4[16] = {0x5d03,0x5d13,0x5d23,0x5d33,0x5d43,0x5d53,0x5d63,0x5d73,0x5d83,0x5d93,0x5da3,0x5db3,0x5dc3,0x5dd3,0x5de3,0x5df3};
uint16_t spriteMapS94_5[16] = {0x5d04,0x5d14,0x5d24,0x5d34,0x5d44,0x5d54,0x5d64,0x5d74,0x5d84,0x5d94,0x5da4,0x5db4,0x5dc4,0x5dd4,0x5de4,0x5df4};
uint16_t spriteMapS94_6[16] = {0x5d05,0x5d15,0x5d25,0x5d35,0x5d45,0x5d55,0x5d65,0x5d75,0x5d85,0x5d95,0x5da5,0x5db5,0x5dc5,0x5dd5,0x5de5,0x5df5};
uint16_t spriteMapS94_7[16] = {0x5d06,0x5d16,0x5d26,0x5d36,0x5d46,0x5d56,0x5d66,0x5d76,0x5d86,0x5d96,0x5da6,0x5db6,0x5dc6,0x5dd6,0x5de6,0x5df6};
uint16_t spriteMapS94_8[16] = {0x5d07,0x5d17,0x5d27,0x5d37,0x5d47,0x5d57,0x5d67,0x5d77,0x5d87,0x5d97,0x5da7,0x5db7,0x5dc7,0x5dd7,0x5de7,0x5df7};
uint16_t spriteMapS94_9[16] = {0x5d08,0x5d18,0x5d28,0x5d38,0x5d48,0x5d58,0x5d68,0x5d78,0x5d88,0x5d98,0x5da8,0x5db8,0x5dc8,0x5dd8,0x5de8,0x5df8};
uint16_t spriteMapS94_10[16] = {0x5d09,0x5d19,0x5d29,0x5d39,0x5d49,0x5d59,0x5d69,0x5d79,0x5d89,0x5d99,0x5da9,0x5db9,0x5dc9,0x5dd9,0x5de9,0x5df9};
uint16_t spriteMapS94_11[16] = {0x5d0a,0x5d1a,0x5d2a,0x5d3a,0x5d4a,0x5d5a,0x5d6a,0x5d7a,0x5d8a,0x5d9a,0x5daa,0x5dba,0x5dca,0x5dda,0x5dea,0x5dfa};
uint16_t spriteMapS94_12[16] = {0x5d0b,0x5d1b,0x5d2b,0x5d3b,0x5d4b,0x5d5b,0x5d6b,0x5d7b,0x5d8b,0x5d9b,0x5dab,0x5dbb,0x5dcb,0x5ddb,0x5deb,0x5dfb};
uint16_t spriteMapS94_13[16] = {0x5d0c,0x5d1c,0x5d2c,0x5d3c,0x5d4c,0x5d5c,0x5d6c,0x5d7c,0x5d8c,0x5d9c,0x5dac,0x5dbc,0x5dcc,0x5ddc,0x5dec,0x5dfc};
uint16_t spriteMapS94_14[16] = {0x5d0d,0x5d1d,0x5d2d,0x5d3d,0x5d4d,0x5d5d,0x5d6d,0x5d7d,0x5d8d,0x5d9d,0x5dad,0x5dbd,0x5dcd,0x5ddd,0x5ded,0x5dfd};
uint16_t spriteMapS94_15[16] = {0x5d0e,0x5d1e,0x5d2e,0x5d3e,0x5d4e,0x5d5e,0x5d6e,0x5d7e,0x5d8e,0x5d9e,0x5dae,0x5dbe,0x5dce,0x5dde,0x5dee,0x5dfe};
uint16_t spriteMapS94_16[16] = {0x5d0f,0x5d1f,0x5d2f,0x5d3f,0x5d4f,0x5d5f,0x5d6f,0x5d7f,0x5d8f,0x5d9f,0x5daf,0x5dbf,0x5dcf,0x5ddf,0x5def,0x5dff};
load_palettes(pal94,PALETTES+PALOFFSET*109);
uint16_t SCB1_2common = setSCB1_2(109,0,0,0,0,0);
uint16_t spal94_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS94_1,spal94_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS94_2,spal94_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS94_3,spal94_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS94_4,spal94_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS94_5,spal94_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS94_6,spal94_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS94_7,spal94_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS94_8,spal94_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS94_9,spal94_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS94_10,spal94_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS94_11,spal94_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS94_12,spal94_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS94_13,spal94_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS94_14,spal94_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS94_15,spal94_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS94_16,spal94_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen95(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 95 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal95[16];
setpal(pal95,0x0,0x0,0x5ff2,0x132e,0xb21,0x57ff,0x2fb2,0xfff,0x4f10,0xf82,0x16ae,0x6611,0x5015,0x55d,0x1777,0x3300);
uint16_t spriteMapS95_1[16] = {0x5e00,0x5e10,0x5e20,0x5e30,0x5e40,0x5e50,0x5e60,0x5e70,0x5e80,0x5e90,0x5ea0,0x5eb0,0x5ec0,0x5ed0,0x5ee0,0x5ef0};
uint16_t spriteMapS95_2[16] = {0x5e01,0x5e11,0x5e21,0x5e31,0x5e41,0x5e51,0x5e61,0x5e71,0x5e81,0x5e91,0x5ea1,0x5eb1,0x5ec1,0x5ed1,0x5ee1,0x5ef1};
uint16_t spriteMapS95_3[16] = {0x5e02,0x5e12,0x5e22,0x5e32,0x5e42,0x5e52,0x5e62,0x5e72,0x5e82,0x5e92,0x5ea2,0x5eb2,0x5ec2,0x5ed2,0x5ee2,0x5ef2};
uint16_t spriteMapS95_4[16] = {0x5e03,0x5e13,0x5e23,0x5e33,0x5e43,0x5e53,0x5e63,0x5e73,0x5e83,0x5e93,0x5ea3,0x5eb3,0x5ec3,0x5ed3,0x5ee3,0x5ef3};
uint16_t spriteMapS95_5[16] = {0x5e04,0x5e14,0x5e24,0x5e34,0x5e44,0x5e54,0x5e64,0x5e74,0x5e84,0x5e94,0x5ea4,0x5eb4,0x5ec4,0x5ed4,0x5ee4,0x5ef4};
uint16_t spriteMapS95_6[16] = {0x5e05,0x5e15,0x5e25,0x5e35,0x5e45,0x5e55,0x5e65,0x5e75,0x5e85,0x5e95,0x5ea5,0x5eb5,0x5ec5,0x5ed5,0x5ee5,0x5ef5};
uint16_t spriteMapS95_7[16] = {0x5e06,0x5e16,0x5e26,0x5e36,0x5e46,0x5e56,0x5e66,0x5e76,0x5e86,0x5e96,0x5ea6,0x5eb6,0x5ec6,0x5ed6,0x5ee6,0x5ef6};
uint16_t spriteMapS95_8[16] = {0x5e07,0x5e17,0x5e27,0x5e37,0x5e47,0x5e57,0x5e67,0x5e77,0x5e87,0x5e97,0x5ea7,0x5eb7,0x5ec7,0x5ed7,0x5ee7,0x5ef7};
uint16_t spriteMapS95_9[16] = {0x5e08,0x5e18,0x5e28,0x5e38,0x5e48,0x5e58,0x5e68,0x5e78,0x5e88,0x5e98,0x5ea8,0x5eb8,0x5ec8,0x5ed8,0x5ee8,0x5ef8};
uint16_t spriteMapS95_10[16] = {0x5e09,0x5e19,0x5e29,0x5e39,0x5e49,0x5e59,0x5e69,0x5e79,0x5e89,0x5e99,0x5ea9,0x5eb9,0x5ec9,0x5ed9,0x5ee9,0x5ef9};
uint16_t spriteMapS95_11[16] = {0x5e0a,0x5e1a,0x5e2a,0x5e3a,0x5e4a,0x5e5a,0x5e6a,0x5e7a,0x5e8a,0x5e9a,0x5eaa,0x5eba,0x5eca,0x5eda,0x5eea,0x5efa};
uint16_t spriteMapS95_12[16] = {0x5e0b,0x5e1b,0x5e2b,0x5e3b,0x5e4b,0x5e5b,0x5e6b,0x5e7b,0x5e8b,0x5e9b,0x5eab,0x5ebb,0x5ecb,0x5edb,0x5eeb,0x5efb};
uint16_t spriteMapS95_13[16] = {0x5e0c,0x5e1c,0x5e2c,0x5e3c,0x5e4c,0x5e5c,0x5e6c,0x5e7c,0x5e8c,0x5e9c,0x5eac,0x5ebc,0x5ecc,0x5edc,0x5eec,0x5efc};
uint16_t spriteMapS95_14[16] = {0x5e0d,0x5e1d,0x5e2d,0x5e3d,0x5e4d,0x5e5d,0x5e6d,0x5e7d,0x5e8d,0x5e9d,0x5ead,0x5ebd,0x5ecd,0x5edd,0x5eed,0x5efd};
uint16_t spriteMapS95_15[16] = {0x5e0e,0x5e1e,0x5e2e,0x5e3e,0x5e4e,0x5e5e,0x5e6e,0x5e7e,0x5e8e,0x5e9e,0x5eae,0x5ebe,0x5ece,0x5ede,0x5eee,0x5efe};
uint16_t spriteMapS95_16[16] = {0x5e0f,0x5e1f,0x5e2f,0x5e3f,0x5e4f,0x5e5f,0x5e6f,0x5e7f,0x5e8f,0x5e9f,0x5eaf,0x5ebf,0x5ecf,0x5edf,0x5eef,0x5eff};
load_palettes(pal95,PALETTES+PALOFFSET*110);
uint16_t SCB1_2common = setSCB1_2(110,0,0,0,0,0);
uint16_t spal95_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS95_1,spal95_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS95_2,spal95_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS95_3,spal95_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS95_4,spal95_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS95_5,spal95_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS95_6,spal95_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS95_7,spal95_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS95_8,spal95_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS95_9,spal95_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS95_10,spal95_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS95_11,spal95_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS95_12,spal95_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS95_13,spal95_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS95_14,spal95_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS95_15,spal95_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS95_16,spal95_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen96(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 96 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal96[16];
setpal(pal96,0x0,0xfff,0x0,0x2f91,0x7fc0,0x2f61,0x6128,0x18ff,0x3556,0x4921,0xed8,0x6f00,0x1410,0x622e,0x77ae,0x4ff2);
uint16_t spriteMapS96_1[16] = {0x5f00,0x5f10,0x5f20,0x5f30,0x5f40,0x5f50,0x5f60,0x5f70,0x5f80,0x5f90,0x5fa0,0x5fb0,0x5fc0,0x5fd0,0x5fe0,0x5ff0};
uint16_t spriteMapS96_2[16] = {0x5f01,0x5f11,0x5f21,0x5f31,0x5f41,0x5f51,0x5f61,0x5f71,0x5f81,0x5f91,0x5fa1,0x5fb1,0x5fc1,0x5fd1,0x5fe1,0x5ff1};
uint16_t spriteMapS96_3[16] = {0x5f02,0x5f12,0x5f22,0x5f32,0x5f42,0x5f52,0x5f62,0x5f72,0x5f82,0x5f92,0x5fa2,0x5fb2,0x5fc2,0x5fd2,0x5fe2,0x5ff2};
uint16_t spriteMapS96_4[16] = {0x5f03,0x5f13,0x5f23,0x5f33,0x5f43,0x5f53,0x5f63,0x5f73,0x5f83,0x5f93,0x5fa3,0x5fb3,0x5fc3,0x5fd3,0x5fe3,0x5ff3};
uint16_t spriteMapS96_5[16] = {0x5f04,0x5f14,0x5f24,0x5f34,0x5f44,0x5f54,0x5f64,0x5f74,0x5f84,0x5f94,0x5fa4,0x5fb4,0x5fc4,0x5fd4,0x5fe4,0x5ff4};
uint16_t spriteMapS96_6[16] = {0x5f05,0x5f15,0x5f25,0x5f35,0x5f45,0x5f55,0x5f65,0x5f75,0x5f85,0x5f95,0x5fa5,0x5fb5,0x5fc5,0x5fd5,0x5fe5,0x5ff5};
uint16_t spriteMapS96_7[16] = {0x5f06,0x5f16,0x5f26,0x5f36,0x5f46,0x5f56,0x5f66,0x5f76,0x5f86,0x5f96,0x5fa6,0x5fb6,0x5fc6,0x5fd6,0x5fe6,0x5ff6};
uint16_t spriteMapS96_8[16] = {0x5f07,0x5f17,0x5f27,0x5f37,0x5f47,0x5f57,0x5f67,0x5f77,0x5f87,0x5f97,0x5fa7,0x5fb7,0x5fc7,0x5fd7,0x5fe7,0x5ff7};
uint16_t spriteMapS96_9[16] = {0x5f08,0x5f18,0x5f28,0x5f38,0x5f48,0x5f58,0x5f68,0x5f78,0x5f88,0x5f98,0x5fa8,0x5fb8,0x5fc8,0x5fd8,0x5fe8,0x5ff8};
uint16_t spriteMapS96_10[16] = {0x5f09,0x5f19,0x5f29,0x5f39,0x5f49,0x5f59,0x5f69,0x5f79,0x5f89,0x5f99,0x5fa9,0x5fb9,0x5fc9,0x5fd9,0x5fe9,0x5ff9};
uint16_t spriteMapS96_11[16] = {0x5f0a,0x5f1a,0x5f2a,0x5f3a,0x5f4a,0x5f5a,0x5f6a,0x5f7a,0x5f8a,0x5f9a,0x5faa,0x5fba,0x5fca,0x5fda,0x5fea,0x5ffa};
uint16_t spriteMapS96_12[16] = {0x5f0b,0x5f1b,0x5f2b,0x5f3b,0x5f4b,0x5f5b,0x5f6b,0x5f7b,0x5f8b,0x5f9b,0x5fab,0x5fbb,0x5fcb,0x5fdb,0x5feb,0x5ffb};
uint16_t spriteMapS96_13[16] = {0x5f0c,0x5f1c,0x5f2c,0x5f3c,0x5f4c,0x5f5c,0x5f6c,0x5f7c,0x5f8c,0x5f9c,0x5fac,0x5fbc,0x5fcc,0x5fdc,0x5fec,0x5ffc};
uint16_t spriteMapS96_14[16] = {0x5f0d,0x5f1d,0x5f2d,0x5f3d,0x5f4d,0x5f5d,0x5f6d,0x5f7d,0x5f8d,0x5f9d,0x5fad,0x5fbd,0x5fcd,0x5fdd,0x5fed,0x5ffd};
uint16_t spriteMapS96_15[16] = {0x5f0e,0x5f1e,0x5f2e,0x5f3e,0x5f4e,0x5f5e,0x5f6e,0x5f7e,0x5f8e,0x5f9e,0x5fae,0x5fbe,0x5fce,0x5fde,0x5fee,0x5ffe};
uint16_t spriteMapS96_16[16] = {0x5f0f,0x5f1f,0x5f2f,0x5f3f,0x5f4f,0x5f5f,0x5f6f,0x5f7f,0x5f8f,0x5f9f,0x5faf,0x5fbf,0x5fcf,0x5fdf,0x5fef,0x5fff};
load_palettes(pal96,PALETTES+PALOFFSET*111);
uint16_t SCB1_2common = setSCB1_2(111,0,0,0,0,0);
uint16_t spal96_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS96_1,spal96_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS96_2,spal96_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS96_3,spal96_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS96_4,spal96_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS96_5,spal96_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS96_6,spal96_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS96_7,spal96_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS96_8,spal96_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS96_9,spal96_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS96_10,spal96_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS96_11,spal96_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS96_12,spal96_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS96_13,spal96_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS96_14,spal96_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS96_15,spal96_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS96_16,spal96_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen97(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 97 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal97[16];
setpal(pal97,0x0,0x4f00,0x0,0x6ff0,0x7317,0x4fa0,0x3610,0x1fe9,0x5fe1,0x5eff,0x633e,0x369d,0x6910,0x1f70,0x4fc0,0x7300);
uint16_t spriteMapS97_1[16] = {0x6000,0x6010,0x6020,0x6030,0x6040,0x6050,0x6060,0x6070,0x6080,0x6090,0x60a0,0x60b0,0x60c0,0x60d0,0x60e0,0x60f0};
uint16_t spriteMapS97_2[16] = {0x6001,0x6011,0x6021,0x6031,0x6041,0x6051,0x6061,0x6071,0x6081,0x6091,0x60a1,0x60b1,0x60c1,0x60d1,0x60e1,0x60f1};
uint16_t spriteMapS97_3[16] = {0x6002,0x6012,0x6022,0x6032,0x6042,0x6052,0x6062,0x6072,0x6082,0x6092,0x60a2,0x60b2,0x60c2,0x60d2,0x60e2,0x60f2};
uint16_t spriteMapS97_4[16] = {0x6003,0x6013,0x6023,0x6033,0x6043,0x6053,0x6063,0x6073,0x6083,0x6093,0x60a3,0x60b3,0x60c3,0x60d3,0x60e3,0x60f3};
uint16_t spriteMapS97_5[16] = {0x6004,0x6014,0x6024,0x6034,0x6044,0x6054,0x6064,0x6074,0x6084,0x6094,0x60a4,0x60b4,0x60c4,0x60d4,0x60e4,0x60f4};
uint16_t spriteMapS97_6[16] = {0x6005,0x6015,0x6025,0x6035,0x6045,0x6055,0x6065,0x6075,0x6085,0x6095,0x60a5,0x60b5,0x60c5,0x60d5,0x60e5,0x60f5};
uint16_t spriteMapS97_7[16] = {0x6006,0x6016,0x6026,0x6036,0x6046,0x6056,0x6066,0x6076,0x6086,0x6096,0x60a6,0x60b6,0x60c6,0x60d6,0x60e6,0x60f6};
uint16_t spriteMapS97_8[16] = {0x6007,0x6017,0x6027,0x6037,0x6047,0x6057,0x6067,0x6077,0x6087,0x6097,0x60a7,0x60b7,0x60c7,0x60d7,0x60e7,0x60f7};
uint16_t spriteMapS97_9[16] = {0x6008,0x6018,0x6028,0x6038,0x6048,0x6058,0x6068,0x6078,0x6088,0x6098,0x60a8,0x60b8,0x60c8,0x60d8,0x60e8,0x60f8};
uint16_t spriteMapS97_10[16] = {0x6009,0x6019,0x6029,0x6039,0x6049,0x6059,0x6069,0x6079,0x6089,0x6099,0x60a9,0x60b9,0x60c9,0x60d9,0x60e9,0x60f9};
uint16_t spriteMapS97_11[16] = {0x600a,0x601a,0x602a,0x603a,0x604a,0x605a,0x606a,0x607a,0x608a,0x609a,0x60aa,0x60ba,0x60ca,0x60da,0x60ea,0x60fa};
uint16_t spriteMapS97_12[16] = {0x600b,0x601b,0x602b,0x603b,0x604b,0x605b,0x606b,0x607b,0x608b,0x609b,0x60ab,0x60bb,0x60cb,0x60db,0x60eb,0x60fb};
uint16_t spriteMapS97_13[16] = {0x600c,0x601c,0x602c,0x603c,0x604c,0x605c,0x606c,0x607c,0x608c,0x609c,0x60ac,0x60bc,0x60cc,0x60dc,0x60ec,0x60fc};
uint16_t spriteMapS97_14[16] = {0x600d,0x601d,0x602d,0x603d,0x604d,0x605d,0x606d,0x607d,0x608d,0x609d,0x60ad,0x60bd,0x60cd,0x60dd,0x60ed,0x60fd};
uint16_t spriteMapS97_15[16] = {0x600e,0x601e,0x602e,0x603e,0x604e,0x605e,0x606e,0x607e,0x608e,0x609e,0x60ae,0x60be,0x60ce,0x60de,0x60ee,0x60fe};
uint16_t spriteMapS97_16[16] = {0x600f,0x601f,0x602f,0x603f,0x604f,0x605f,0x606f,0x607f,0x608f,0x609f,0x60af,0x60bf,0x60cf,0x60df,0x60ef,0x60ff};
load_palettes(pal97,PALETTES+PALOFFSET*112);
uint16_t SCB1_2common = setSCB1_2(112,0,0,0,0,0);
uint16_t spal97_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS97_1,spal97_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS97_2,spal97_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS97_3,spal97_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS97_4,spal97_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS97_5,spal97_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS97_6,spal97_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS97_7,spal97_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS97_8,spal97_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS97_9,spal97_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS97_10,spal97_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS97_11,spal97_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS97_12,spal97_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS97_13,spal97_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS97_14,spal97_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS97_15,spal97_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS97_16,spal97_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen98(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 98 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal98[16];
setpal(pal98,0x0,0x42e,0x6f00,0x0,0x17ff,0x4fc0,0x4dd8,0x3f80,0x3fff,0x75ae,0x5410,0x5015,0x5ff1,0x2557,0x7920,0x156d);
uint16_t spriteMapS98_1[16] = {0x6100,0x6110,0x6120,0x6130,0x6140,0x6150,0x6160,0x6170,0x6180,0x6190,0x61a0,0x61b0,0x61c0,0x61d0,0x61e0,0x61f0};
uint16_t spriteMapS98_2[16] = {0x6101,0x6111,0x6121,0x6131,0x6141,0x6151,0x6161,0x6171,0x6181,0x6191,0x61a1,0x61b1,0x61c1,0x61d1,0x61e1,0x61f1};
uint16_t spriteMapS98_3[16] = {0x6102,0x6112,0x6122,0x6132,0x6142,0x6152,0x6162,0x6172,0x6182,0x6192,0x61a2,0x61b2,0x61c2,0x61d2,0x61e2,0x61f2};
uint16_t spriteMapS98_4[16] = {0x6103,0x6113,0x6123,0x6133,0x6143,0x6153,0x6163,0x6173,0x6183,0x6193,0x61a3,0x61b3,0x61c3,0x61d3,0x61e3,0x61f3};
uint16_t spriteMapS98_5[16] = {0x6104,0x6114,0x6124,0x6134,0x6144,0x6154,0x6164,0x6174,0x6184,0x6194,0x61a4,0x61b4,0x61c4,0x61d4,0x61e4,0x61f4};
uint16_t spriteMapS98_6[16] = {0x6105,0x6115,0x6125,0x6135,0x6145,0x6155,0x6165,0x6175,0x6185,0x6195,0x61a5,0x61b5,0x61c5,0x61d5,0x61e5,0x61f5};
uint16_t spriteMapS98_7[16] = {0x6106,0x6116,0x6126,0x6136,0x6146,0x6156,0x6166,0x6176,0x6186,0x6196,0x61a6,0x61b6,0x61c6,0x61d6,0x61e6,0x61f6};
uint16_t spriteMapS98_8[16] = {0x6107,0x6117,0x6127,0x6137,0x6147,0x6157,0x6167,0x6177,0x6187,0x6197,0x61a7,0x61b7,0x61c7,0x61d7,0x61e7,0x61f7};
uint16_t spriteMapS98_9[16] = {0x6108,0x6118,0x6128,0x6138,0x6148,0x6158,0x6168,0x6178,0x6188,0x6198,0x61a8,0x61b8,0x61c8,0x61d8,0x61e8,0x61f8};
uint16_t spriteMapS98_10[16] = {0x6109,0x6119,0x6129,0x6139,0x6149,0x6159,0x6169,0x6179,0x6189,0x6199,0x61a9,0x61b9,0x61c9,0x61d9,0x61e9,0x61f9};
uint16_t spriteMapS98_11[16] = {0x610a,0x611a,0x612a,0x613a,0x614a,0x615a,0x616a,0x617a,0x618a,0x619a,0x61aa,0x61ba,0x61ca,0x61da,0x61ea,0x61fa};
uint16_t spriteMapS98_12[16] = {0x610b,0x611b,0x612b,0x613b,0x614b,0x615b,0x616b,0x617b,0x618b,0x619b,0x61ab,0x61bb,0x61cb,0x61db,0x61eb,0x61fb};
uint16_t spriteMapS98_13[16] = {0x610c,0x611c,0x612c,0x613c,0x614c,0x615c,0x616c,0x617c,0x618c,0x619c,0x61ac,0x61bc,0x61cc,0x61dc,0x61ec,0x61fc};
uint16_t spriteMapS98_14[16] = {0x610d,0x611d,0x612d,0x613d,0x614d,0x615d,0x616d,0x617d,0x618d,0x619d,0x61ad,0x61bd,0x61cd,0x61dd,0x61ed,0x61fd};
uint16_t spriteMapS98_15[16] = {0x610e,0x611e,0x612e,0x613e,0x614e,0x615e,0x616e,0x617e,0x618e,0x619e,0x61ae,0x61be,0x61ce,0x61de,0x61ee,0x61fe};
uint16_t spriteMapS98_16[16] = {0x610f,0x611f,0x612f,0x613f,0x614f,0x615f,0x616f,0x617f,0x618f,0x619f,0x61af,0x61bf,0x61cf,0x61df,0x61ef,0x61ff};
load_palettes(pal98,PALETTES+PALOFFSET*113);
uint16_t SCB1_2common = setSCB1_2(113,0,0,0,0,0);
uint16_t spal98_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS98_1,spal98_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS98_2,spal98_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS98_3,spal98_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS98_4,spal98_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS98_5,spal98_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS98_6,spal98_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS98_7,spal98_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS98_8,spal98_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS98_9,spal98_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS98_10,spal98_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS98_11,spal98_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS98_12,spal98_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS98_13,spal98_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS98_14,spal98_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS98_15,spal98_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS98_16,spal98_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen99(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 99 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal99[16];
setpal(pal99,0x0,0x422e,0x0,0x7fff,0x6f00,0x5fc0,0x7ad,0x6411,0x4921,0xbff,0x147e,0x54ff,0x7cb7,0x3115,0x2f81,0x4ff2);
uint16_t spriteMapS99_1[16] = {0x6200,0x6210,0x6220,0x6230,0x6240,0x6250,0x6260,0x6270,0x6280,0x6290,0x62a0,0x62b0,0x62c0,0x62d0,0x62e0,0x62f0};
uint16_t spriteMapS99_2[16] = {0x6201,0x6211,0x6221,0x6231,0x6241,0x6251,0x6261,0x6271,0x6281,0x6291,0x62a1,0x62b1,0x62c1,0x62d1,0x62e1,0x62f1};
uint16_t spriteMapS99_3[16] = {0x6202,0x6212,0x6222,0x6232,0x6242,0x6252,0x6262,0x6272,0x6282,0x6292,0x62a2,0x62b2,0x62c2,0x62d2,0x62e2,0x62f2};
uint16_t spriteMapS99_4[16] = {0x6203,0x6213,0x6223,0x6233,0x6243,0x6253,0x6263,0x6273,0x6283,0x6293,0x62a3,0x62b3,0x62c3,0x62d3,0x62e3,0x62f3};
uint16_t spriteMapS99_5[16] = {0x6204,0x6214,0x6224,0x6234,0x6244,0x6254,0x6264,0x6274,0x6284,0x6294,0x62a4,0x62b4,0x62c4,0x62d4,0x62e4,0x62f4};
uint16_t spriteMapS99_6[16] = {0x6205,0x6215,0x6225,0x6235,0x6245,0x6255,0x6265,0x6275,0x6285,0x6295,0x62a5,0x62b5,0x62c5,0x62d5,0x62e5,0x62f5};
uint16_t spriteMapS99_7[16] = {0x6206,0x6216,0x6226,0x6236,0x6246,0x6256,0x6266,0x6276,0x6286,0x6296,0x62a6,0x62b6,0x62c6,0x62d6,0x62e6,0x62f6};
uint16_t spriteMapS99_8[16] = {0x6207,0x6217,0x6227,0x6237,0x6247,0x6257,0x6267,0x6277,0x6287,0x6297,0x62a7,0x62b7,0x62c7,0x62d7,0x62e7,0x62f7};
uint16_t spriteMapS99_9[16] = {0x6208,0x6218,0x6228,0x6238,0x6248,0x6258,0x6268,0x6278,0x6288,0x6298,0x62a8,0x62b8,0x62c8,0x62d8,0x62e8,0x62f8};
uint16_t spriteMapS99_10[16] = {0x6209,0x6219,0x6229,0x6239,0x6249,0x6259,0x6269,0x6279,0x6289,0x6299,0x62a9,0x62b9,0x62c9,0x62d9,0x62e9,0x62f9};
uint16_t spriteMapS99_11[16] = {0x620a,0x621a,0x622a,0x623a,0x624a,0x625a,0x626a,0x627a,0x628a,0x629a,0x62aa,0x62ba,0x62ca,0x62da,0x62ea,0x62fa};
uint16_t spriteMapS99_12[16] = {0x620b,0x621b,0x622b,0x623b,0x624b,0x625b,0x626b,0x627b,0x628b,0x629b,0x62ab,0x62bb,0x62cb,0x62db,0x62eb,0x62fb};
uint16_t spriteMapS99_13[16] = {0x620c,0x621c,0x622c,0x623c,0x624c,0x625c,0x626c,0x627c,0x628c,0x629c,0x62ac,0x62bc,0x62cc,0x62dc,0x62ec,0x62fc};
uint16_t spriteMapS99_14[16] = {0x620d,0x621d,0x622d,0x623d,0x624d,0x625d,0x626d,0x627d,0x628d,0x629d,0x62ad,0x62bd,0x62cd,0x62dd,0x62ed,0x62fd};
uint16_t spriteMapS99_15[16] = {0x620e,0x621e,0x622e,0x623e,0x624e,0x625e,0x626e,0x627e,0x628e,0x629e,0x62ae,0x62be,0x62ce,0x62de,0x62ee,0x62fe};
uint16_t spriteMapS99_16[16] = {0x620f,0x621f,0x622f,0x623f,0x624f,0x625f,0x626f,0x627f,0x628f,0x629f,0x62af,0x62bf,0x62cf,0x62df,0x62ef,0x62ff};
load_palettes(pal99,PALETTES+PALOFFSET*114);
uint16_t SCB1_2common = setSCB1_2(114,0,0,0,0,0);
uint16_t spal99_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS99_1,spal99_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS99_2,spal99_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS99_3,spal99_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS99_4,spal99_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS99_5,spal99_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS99_6,spal99_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS99_7,spal99_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS99_8,spal99_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS99_9,spal99_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS99_10,spal99_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS99_11,spal99_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS99_12,spal99_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS99_13,spal99_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS99_14,spal99_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS99_15,spal99_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS99_16,spal99_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen100(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 100 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal100[16];
setpal(pal100,0x0,0x3fff,0x0,0x4f10,0x3910,0x7fe1,0x17ff,0x7f90,0x422e,0x3e89,0x5775,0x347e,0x47be,0x6116,0x3400,0x2fe9);
uint16_t spriteMapS100_1[16] = {0x6300,0x6310,0x6320,0x6330,0x6340,0x6350,0x6360,0x6370,0x6380,0x6390,0x63a0,0x63b0,0x63c0,0x63d0,0x63e0,0x63f0};
uint16_t spriteMapS100_2[16] = {0x6301,0x6311,0x6321,0x6331,0x6341,0x6351,0x6361,0x6371,0x6381,0x6391,0x63a1,0x63b1,0x63c1,0x63d1,0x63e1,0x63f1};
uint16_t spriteMapS100_3[16] = {0x6302,0x6312,0x6322,0x6332,0x6342,0x6352,0x6362,0x6372,0x6382,0x6392,0x63a2,0x63b2,0x63c2,0x63d2,0x63e2,0x63f2};
uint16_t spriteMapS100_4[16] = {0x6303,0x6313,0x6323,0x6333,0x6343,0x6353,0x6363,0x6373,0x6383,0x6393,0x63a3,0x63b3,0x63c3,0x63d3,0x63e3,0x63f3};
uint16_t spriteMapS100_5[16] = {0x6304,0x6314,0x6324,0x6334,0x6344,0x6354,0x6364,0x6374,0x6384,0x6394,0x63a4,0x63b4,0x63c4,0x63d4,0x63e4,0x63f4};
uint16_t spriteMapS100_6[16] = {0x6305,0x6315,0x6325,0x6335,0x6345,0x6355,0x6365,0x6375,0x6385,0x6395,0x63a5,0x63b5,0x63c5,0x63d5,0x63e5,0x63f5};
uint16_t spriteMapS100_7[16] = {0x6306,0x6316,0x6326,0x6336,0x6346,0x6356,0x6366,0x6376,0x6386,0x6396,0x63a6,0x63b6,0x63c6,0x63d6,0x63e6,0x63f6};
uint16_t spriteMapS100_8[16] = {0x6307,0x6317,0x6327,0x6337,0x6347,0x6357,0x6367,0x6377,0x6387,0x6397,0x63a7,0x63b7,0x63c7,0x63d7,0x63e7,0x63f7};
uint16_t spriteMapS100_9[16] = {0x6308,0x6318,0x6328,0x6338,0x6348,0x6358,0x6368,0x6378,0x6388,0x6398,0x63a8,0x63b8,0x63c8,0x63d8,0x63e8,0x63f8};
uint16_t spriteMapS100_10[16] = {0x6309,0x6319,0x6329,0x6339,0x6349,0x6359,0x6369,0x6379,0x6389,0x6399,0x63a9,0x63b9,0x63c9,0x63d9,0x63e9,0x63f9};
uint16_t spriteMapS100_11[16] = {0x630a,0x631a,0x632a,0x633a,0x634a,0x635a,0x636a,0x637a,0x638a,0x639a,0x63aa,0x63ba,0x63ca,0x63da,0x63ea,0x63fa};
uint16_t spriteMapS100_12[16] = {0x630b,0x631b,0x632b,0x633b,0x634b,0x635b,0x636b,0x637b,0x638b,0x639b,0x63ab,0x63bb,0x63cb,0x63db,0x63eb,0x63fb};
uint16_t spriteMapS100_13[16] = {0x630c,0x631c,0x632c,0x633c,0x634c,0x635c,0x636c,0x637c,0x638c,0x639c,0x63ac,0x63bc,0x63cc,0x63dc,0x63ec,0x63fc};
uint16_t spriteMapS100_14[16] = {0x630d,0x631d,0x632d,0x633d,0x634d,0x635d,0x636d,0x637d,0x638d,0x639d,0x63ad,0x63bd,0x63cd,0x63dd,0x63ed,0x63fd};
uint16_t spriteMapS100_15[16] = {0x630e,0x631e,0x632e,0x633e,0x634e,0x635e,0x636e,0x637e,0x638e,0x639e,0x63ae,0x63be,0x63ce,0x63de,0x63ee,0x63fe};
uint16_t spriteMapS100_16[16] = {0x630f,0x631f,0x632f,0x633f,0x634f,0x635f,0x636f,0x637f,0x638f,0x639f,0x63af,0x63bf,0x63cf,0x63df,0x63ef,0x63ff};
load_palettes(pal100,PALETTES+PALOFFSET*115);
uint16_t SCB1_2common = setSCB1_2(115,0,0,0,0,0);
uint16_t spal100_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS100_1,spal100_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS100_2,spal100_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS100_3,spal100_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS100_4,spal100_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS100_5,spal100_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS100_6,spal100_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS100_7,spal100_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS100_8,spal100_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS100_9,spal100_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS100_10,spal100_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS100_11,spal100_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS100_12,spal100_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS100_13,spal100_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS100_14,spal100_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS100_15,spal100_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS100_16,spal100_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen101(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 101 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal101[16];
setpal(pal101,0x0,0x2000,0x4407,0x7111,0xa00,0x7222,0x191c,0x444,0x7555,0xb52,0x2888,0x7d85,0x7aaa,0x7ccc,0x7ec9,0x0);
uint16_t spriteMapS101_1[16] = {0x6400,0x6410,0x6420,0x6430,0x6440,0x6450,0x6460,0x6470,0x6480,0x6490,0x64a0,0x64b0,0x64c0,0x64d0,0x64e0,0x64f0};
uint16_t spriteMapS101_2[16] = {0x6401,0x6411,0x6421,0x6431,0x6441,0x6451,0x6461,0x6471,0x6481,0x6491,0x64a1,0x64b1,0x64c1,0x64d1,0x64e1,0x64f1};
uint16_t spriteMapS101_3[16] = {0x6402,0x6412,0x6422,0x6432,0x6442,0x6452,0x6462,0x6472,0x6482,0x6492,0x64a2,0x64b2,0x64c2,0x64d2,0x64e2,0x64f2};
uint16_t spriteMapS101_4[16] = {0x6403,0x6413,0x6423,0x6433,0x6443,0x6453,0x6463,0x6473,0x6483,0x6493,0x64a3,0x64b3,0x64c3,0x64d3,0x64e3,0x64f3};
uint16_t spriteMapS101_5[16] = {0x6404,0x6414,0x6424,0x6434,0x6444,0x6454,0x6464,0x6474,0x6484,0x6494,0x64a4,0x64b4,0x64c4,0x64d4,0x64e4,0x64f4};
uint16_t spriteMapS101_6[16] = {0x6405,0x6415,0x6425,0x6435,0x6445,0x6455,0x6465,0x6475,0x6485,0x6495,0x64a5,0x64b5,0x64c5,0x64d5,0x64e5,0x64f5};
uint16_t spriteMapS101_7[16] = {0x6406,0x6416,0x6426,0x6436,0x6446,0x6456,0x6466,0x6476,0x6486,0x6496,0x64a6,0x64b6,0x64c6,0x64d6,0x64e6,0x64f6};
uint16_t spriteMapS101_8[16] = {0x6407,0x6417,0x6427,0x6437,0x6447,0x6457,0x6467,0x6477,0x6487,0x6497,0x64a7,0x64b7,0x64c7,0x64d7,0x64e7,0x64f7};
uint16_t spriteMapS101_9[16] = {0x6408,0x6418,0x6428,0x6438,0x6448,0x6458,0x6468,0x6478,0x6488,0x6498,0x64a8,0x64b8,0x64c8,0x64d8,0x64e8,0x64f8};
uint16_t spriteMapS101_10[16] = {0x6409,0x6419,0x6429,0x6439,0x6449,0x6459,0x6469,0x6479,0x6489,0x6499,0x64a9,0x64b9,0x64c9,0x64d9,0x64e9,0x64f9};
uint16_t spriteMapS101_11[16] = {0x640a,0x641a,0x642a,0x643a,0x644a,0x645a,0x646a,0x647a,0x648a,0x649a,0x64aa,0x64ba,0x64ca,0x64da,0x64ea,0x64fa};
uint16_t spriteMapS101_12[16] = {0x640b,0x641b,0x642b,0x643b,0x644b,0x645b,0x646b,0x647b,0x648b,0x649b,0x64ab,0x64bb,0x64cb,0x64db,0x64eb,0x64fb};
uint16_t spriteMapS101_13[16] = {0x640c,0x641c,0x642c,0x643c,0x644c,0x645c,0x646c,0x647c,0x648c,0x649c,0x64ac,0x64bc,0x64cc,0x64dc,0x64ec,0x64fc};
uint16_t spriteMapS101_14[16] = {0x640d,0x641d,0x642d,0x643d,0x644d,0x645d,0x646d,0x647d,0x648d,0x649d,0x64ad,0x64bd,0x64cd,0x64dd,0x64ed,0x64fd};
uint16_t spriteMapS101_15[16] = {0x640e,0x641e,0x642e,0x643e,0x644e,0x645e,0x646e,0x647e,0x648e,0x649e,0x64ae,0x64be,0x64ce,0x64de,0x64ee,0x64fe};
uint16_t spriteMapS101_16[16] = {0x640f,0x641f,0x642f,0x643f,0x644f,0x645f,0x646f,0x647f,0x648f,0x649f,0x64af,0x64bf,0x64cf,0x64df,0x64ef,0x64ff};
load_palettes(pal101,PALETTES+PALOFFSET*116);
uint16_t SCB1_2common = setSCB1_2(116,0,0,0,0,0);
uint16_t spal101_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS101_1,spal101_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS101_2,spal101_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS101_3,spal101_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS101_4,spal101_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS101_5,spal101_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS101_6,spal101_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS101_7,spal101_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS101_8,spal101_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS101_9,spal101_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS101_10,spal101_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS101_11,spal101_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS101_12,spal101_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS101_13,spal101_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS101_14,spal101_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS101_15,spal101_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS101_16,spal101_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen102(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 102 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal102[16];
setpal(pal102,0x0,0x2000,0x4407,0x7111,0x4900,0x7222,0x191c,0x5f0f,0x555,0xb52,0x5787,0x5d95,0x2aaa,0x5ec9,0xddd,0x0);
uint16_t spriteMapS102_1[16] = {0x6500,0x6510,0x6520,0x6530,0x6540,0x6550,0x6560,0x6570,0x6580,0x6590,0x65a0,0x65b0,0x65c0,0x65d0,0x65e0,0x65f0};
uint16_t spriteMapS102_2[16] = {0x6501,0x6511,0x6521,0x6531,0x6541,0x6551,0x6561,0x6571,0x6581,0x6591,0x65a1,0x65b1,0x65c1,0x65d1,0x65e1,0x65f1};
uint16_t spriteMapS102_3[16] = {0x6502,0x6512,0x6522,0x6532,0x6542,0x6552,0x6562,0x6572,0x6582,0x6592,0x65a2,0x65b2,0x65c2,0x65d2,0x65e2,0x65f2};
uint16_t spriteMapS102_4[16] = {0x6503,0x6513,0x6523,0x6533,0x6543,0x6553,0x6563,0x6573,0x6583,0x6593,0x65a3,0x65b3,0x65c3,0x65d3,0x65e3,0x65f3};
uint16_t spriteMapS102_5[16] = {0x6504,0x6514,0x6524,0x6534,0x6544,0x6554,0x6564,0x6574,0x6584,0x6594,0x65a4,0x65b4,0x65c4,0x65d4,0x65e4,0x65f4};
uint16_t spriteMapS102_6[16] = {0x6505,0x6515,0x6525,0x6535,0x6545,0x6555,0x6565,0x6575,0x6585,0x6595,0x65a5,0x65b5,0x65c5,0x65d5,0x65e5,0x65f5};
uint16_t spriteMapS102_7[16] = {0x6506,0x6516,0x6526,0x6536,0x6546,0x6556,0x6566,0x6576,0x6586,0x6596,0x65a6,0x65b6,0x65c6,0x65d6,0x65e6,0x65f6};
uint16_t spriteMapS102_8[16] = {0x6507,0x6517,0x6527,0x6537,0x6547,0x6557,0x6567,0x6577,0x6587,0x6597,0x65a7,0x65b7,0x65c7,0x65d7,0x65e7,0x65f7};
uint16_t spriteMapS102_9[16] = {0x6508,0x6518,0x6528,0x6538,0x6548,0x6558,0x6568,0x6578,0x6588,0x6598,0x65a8,0x65b8,0x65c8,0x65d8,0x65e8,0x65f8};
uint16_t spriteMapS102_10[16] = {0x6509,0x6519,0x6529,0x6539,0x6549,0x6559,0x6569,0x6579,0x6589,0x6599,0x65a9,0x65b9,0x65c9,0x65d9,0x65e9,0x65f9};
uint16_t spriteMapS102_11[16] = {0x650a,0x651a,0x652a,0x653a,0x654a,0x655a,0x656a,0x657a,0x658a,0x659a,0x65aa,0x65ba,0x65ca,0x65da,0x65ea,0x65fa};
uint16_t spriteMapS102_12[16] = {0x650b,0x651b,0x652b,0x653b,0x654b,0x655b,0x656b,0x657b,0x658b,0x659b,0x65ab,0x65bb,0x65cb,0x65db,0x65eb,0x65fb};
uint16_t spriteMapS102_13[16] = {0x650c,0x651c,0x652c,0x653c,0x654c,0x655c,0x656c,0x657c,0x658c,0x659c,0x65ac,0x65bc,0x65cc,0x65dc,0x65ec,0x65fc};
uint16_t spriteMapS102_14[16] = {0x650d,0x651d,0x652d,0x653d,0x654d,0x655d,0x656d,0x657d,0x658d,0x659d,0x65ad,0x65bd,0x65cd,0x65dd,0x65ed,0x65fd};
uint16_t spriteMapS102_15[16] = {0x650e,0x651e,0x652e,0x653e,0x654e,0x655e,0x656e,0x657e,0x658e,0x659e,0x65ae,0x65be,0x65ce,0x65de,0x65ee,0x65fe};
uint16_t spriteMapS102_16[16] = {0x650f,0x651f,0x652f,0x653f,0x654f,0x655f,0x656f,0x657f,0x658f,0x659f,0x65af,0x65bf,0x65cf,0x65df,0x65ef,0x65ff};
load_palettes(pal102,PALETTES+PALOFFSET*117);
uint16_t SCB1_2common = setSCB1_2(117,0,0,0,0,0);
uint16_t spal102_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS102_1,spal102_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS102_2,spal102_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS102_3,spal102_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS102_4,spal102_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS102_5,spal102_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS102_6,spal102_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS102_7,spal102_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS102_8,spal102_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS102_9,spal102_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS102_10,spal102_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS102_11,spal102_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS102_12,spal102_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS102_13,spal102_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS102_14,spal102_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS102_15,spal102_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS102_16,spal102_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen103(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 103 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal103[16];
setpal(pal103,0x0,0x2000,0x4407,0x7111,0x5222,0x333,0x191c,0x5555,0x3b41,0x888,0x7999,0x5d95,0x7ccc,0x7ec9,0x7eee,0x0);
uint16_t spriteMapS103_1[16] = {0x6600,0x6610,0x6620,0x6630,0x6640,0x6650,0x6660,0x6670,0x6680,0x6690,0x66a0,0x66b0,0x66c0,0x66d0,0x66e0,0x66f0};
uint16_t spriteMapS103_2[16] = {0x6601,0x6611,0x6621,0x6631,0x6641,0x6651,0x6661,0x6671,0x6681,0x6691,0x66a1,0x66b1,0x66c1,0x66d1,0x66e1,0x66f1};
uint16_t spriteMapS103_3[16] = {0x6602,0x6612,0x6622,0x6632,0x6642,0x6652,0x6662,0x6672,0x6682,0x6692,0x66a2,0x66b2,0x66c2,0x66d2,0x66e2,0x66f2};
uint16_t spriteMapS103_4[16] = {0x6603,0x6613,0x6623,0x6633,0x6643,0x6653,0x6663,0x6673,0x6683,0x6693,0x66a3,0x66b3,0x66c3,0x66d3,0x66e3,0x66f3};
uint16_t spriteMapS103_5[16] = {0x6604,0x6614,0x6624,0x6634,0x6644,0x6654,0x6664,0x6674,0x6684,0x6694,0x66a4,0x66b4,0x66c4,0x66d4,0x66e4,0x66f4};
uint16_t spriteMapS103_6[16] = {0x6605,0x6615,0x6625,0x6635,0x6645,0x6655,0x6665,0x6675,0x6685,0x6695,0x66a5,0x66b5,0x66c5,0x66d5,0x66e5,0x66f5};
uint16_t spriteMapS103_7[16] = {0x6606,0x6616,0x6626,0x6636,0x6646,0x6656,0x6666,0x6676,0x6686,0x6696,0x66a6,0x66b6,0x66c6,0x66d6,0x66e6,0x66f6};
uint16_t spriteMapS103_8[16] = {0x6607,0x6617,0x6627,0x6637,0x6647,0x6657,0x6667,0x6677,0x6687,0x6697,0x66a7,0x66b7,0x66c7,0x66d7,0x66e7,0x66f7};
uint16_t spriteMapS103_9[16] = {0x6608,0x6618,0x6628,0x6638,0x6648,0x6658,0x6668,0x6678,0x6688,0x6698,0x66a8,0x66b8,0x66c8,0x66d8,0x66e8,0x66f8};
uint16_t spriteMapS103_10[16] = {0x6609,0x6619,0x6629,0x6639,0x6649,0x6659,0x6669,0x6679,0x6689,0x6699,0x66a9,0x66b9,0x66c9,0x66d9,0x66e9,0x66f9};
uint16_t spriteMapS103_11[16] = {0x660a,0x661a,0x662a,0x663a,0x664a,0x665a,0x666a,0x667a,0x668a,0x669a,0x66aa,0x66ba,0x66ca,0x66da,0x66ea,0x66fa};
uint16_t spriteMapS103_12[16] = {0x660b,0x661b,0x662b,0x663b,0x664b,0x665b,0x666b,0x667b,0x668b,0x669b,0x66ab,0x66bb,0x66cb,0x66db,0x66eb,0x66fb};
uint16_t spriteMapS103_13[16] = {0x660c,0x661c,0x662c,0x663c,0x664c,0x665c,0x666c,0x667c,0x668c,0x669c,0x66ac,0x66bc,0x66cc,0x66dc,0x66ec,0x66fc};
uint16_t spriteMapS103_14[16] = {0x660d,0x661d,0x662d,0x663d,0x664d,0x665d,0x666d,0x667d,0x668d,0x669d,0x66ad,0x66bd,0x66cd,0x66dd,0x66ed,0x66fd};
uint16_t spriteMapS103_15[16] = {0x660e,0x661e,0x662e,0x663e,0x664e,0x665e,0x666e,0x667e,0x668e,0x669e,0x66ae,0x66be,0x66ce,0x66de,0x66ee,0x66fe};
uint16_t spriteMapS103_16[16] = {0x660f,0x661f,0x662f,0x663f,0x664f,0x665f,0x666f,0x667f,0x668f,0x669f,0x66af,0x66bf,0x66cf,0x66df,0x66ef,0x66ff};
load_palettes(pal103,PALETTES+PALOFFSET*118);
uint16_t SCB1_2common = setSCB1_2(118,0,0,0,0,0);
uint16_t spal103_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS103_1,spal103_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS103_2,spal103_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS103_3,spal103_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS103_4,spal103_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS103_5,spal103_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS103_6,spal103_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS103_7,spal103_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS103_8,spal103_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS103_9,spal103_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS103_10,spal103_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS103_11,spal103_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS103_12,spal103_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS103_13,spal103_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS103_14,spal103_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS103_15,spal103_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS103_16,spal103_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen104(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 104 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal104[16];
setpal(pal104,0x0,0x2000,0x406,0x7111,0x609,0x1222,0x333,0x191c,0x7555,0x3b41,0x999,0x5d95,0x5ccc,0x7ec9,0x7eee,0x0);
uint16_t spriteMapS104_1[16] = {0x6700,0x6710,0x6720,0x6730,0x6740,0x6750,0x6760,0x6770,0x6780,0x6790,0x67a0,0x67b0,0x67c0,0x67d0,0x67e0,0x67f0};
uint16_t spriteMapS104_2[16] = {0x6701,0x6711,0x6721,0x6731,0x6741,0x6751,0x6761,0x6771,0x6781,0x6791,0x67a1,0x67b1,0x67c1,0x67d1,0x67e1,0x67f1};
uint16_t spriteMapS104_3[16] = {0x6702,0x6712,0x6722,0x6732,0x6742,0x6752,0x6762,0x6772,0x6782,0x6792,0x67a2,0x67b2,0x67c2,0x67d2,0x67e2,0x67f2};
uint16_t spriteMapS104_4[16] = {0x6703,0x6713,0x6723,0x6733,0x6743,0x6753,0x6763,0x6773,0x6783,0x6793,0x67a3,0x67b3,0x67c3,0x67d3,0x67e3,0x67f3};
uint16_t spriteMapS104_5[16] = {0x6704,0x6714,0x6724,0x6734,0x6744,0x6754,0x6764,0x6774,0x6784,0x6794,0x67a4,0x67b4,0x67c4,0x67d4,0x67e4,0x67f4};
uint16_t spriteMapS104_6[16] = {0x6705,0x6715,0x6725,0x6735,0x6745,0x6755,0x6765,0x6775,0x6785,0x6795,0x67a5,0x67b5,0x67c5,0x67d5,0x67e5,0x67f5};
uint16_t spriteMapS104_7[16] = {0x6706,0x6716,0x6726,0x6736,0x6746,0x6756,0x6766,0x6776,0x6786,0x6796,0x67a6,0x67b6,0x67c6,0x67d6,0x67e6,0x67f6};
uint16_t spriteMapS104_8[16] = {0x6707,0x6717,0x6727,0x6737,0x6747,0x6757,0x6767,0x6777,0x6787,0x6797,0x67a7,0x67b7,0x67c7,0x67d7,0x67e7,0x67f7};
uint16_t spriteMapS104_9[16] = {0x6708,0x6718,0x6728,0x6738,0x6748,0x6758,0x6768,0x6778,0x6788,0x6798,0x67a8,0x67b8,0x67c8,0x67d8,0x67e8,0x67f8};
uint16_t spriteMapS104_10[16] = {0x6709,0x6719,0x6729,0x6739,0x6749,0x6759,0x6769,0x6779,0x6789,0x6799,0x67a9,0x67b9,0x67c9,0x67d9,0x67e9,0x67f9};
uint16_t spriteMapS104_11[16] = {0x670a,0x671a,0x672a,0x673a,0x674a,0x675a,0x676a,0x677a,0x678a,0x679a,0x67aa,0x67ba,0x67ca,0x67da,0x67ea,0x67fa};
uint16_t spriteMapS104_12[16] = {0x670b,0x671b,0x672b,0x673b,0x674b,0x675b,0x676b,0x677b,0x678b,0x679b,0x67ab,0x67bb,0x67cb,0x67db,0x67eb,0x67fb};
uint16_t spriteMapS104_13[16] = {0x670c,0x671c,0x672c,0x673c,0x674c,0x675c,0x676c,0x677c,0x678c,0x679c,0x67ac,0x67bc,0x67cc,0x67dc,0x67ec,0x67fc};
uint16_t spriteMapS104_14[16] = {0x670d,0x671d,0x672d,0x673d,0x674d,0x675d,0x676d,0x677d,0x678d,0x679d,0x67ad,0x67bd,0x67cd,0x67dd,0x67ed,0x67fd};
uint16_t spriteMapS104_15[16] = {0x670e,0x671e,0x672e,0x673e,0x674e,0x675e,0x676e,0x677e,0x678e,0x679e,0x67ae,0x67be,0x67ce,0x67de,0x67ee,0x67fe};
uint16_t spriteMapS104_16[16] = {0x670f,0x671f,0x672f,0x673f,0x674f,0x675f,0x676f,0x677f,0x678f,0x679f,0x67af,0x67bf,0x67cf,0x67df,0x67ef,0x67ff};
load_palettes(pal104,PALETTES+PALOFFSET*119);
uint16_t SCB1_2common = setSCB1_2(119,0,0,0,0,0);
uint16_t spal104_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS104_1,spal104_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS104_2,spal104_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS104_3,spal104_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS104_4,spal104_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS104_5,spal104_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS104_6,spal104_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS104_7,spal104_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS104_8,spal104_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS104_9,spal104_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS104_10,spal104_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS104_11,spal104_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS104_12,spal104_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS104_13,spal104_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS104_14,spal104_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS104_15,spal104_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS104_16,spal104_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen105(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 105 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal105[16];
setpal(pal105,0x0,0x2000,0x4407,0x7111,0x5222,0x333,0x191c,0x2434,0x6556,0x3b41,0x5999,0x5d95,0x7ccc,0x7ec9,0x6eee,0x0);
uint16_t spriteMapS105_1[16] = {0x6800,0x6810,0x6820,0x6830,0x6840,0x6850,0x6860,0x6870,0x6880,0x6890,0x68a0,0x68b0,0x68c0,0x68d0,0x68e0,0x68f0};
uint16_t spriteMapS105_2[16] = {0x6801,0x6811,0x6821,0x6831,0x6841,0x6851,0x6861,0x6871,0x6881,0x6891,0x68a1,0x68b1,0x68c1,0x68d1,0x68e1,0x68f1};
uint16_t spriteMapS105_3[16] = {0x6802,0x6812,0x6822,0x6832,0x6842,0x6852,0x6862,0x6872,0x6882,0x6892,0x68a2,0x68b2,0x68c2,0x68d2,0x68e2,0x68f2};
uint16_t spriteMapS105_4[16] = {0x6803,0x6813,0x6823,0x6833,0x6843,0x6853,0x6863,0x6873,0x6883,0x6893,0x68a3,0x68b3,0x68c3,0x68d3,0x68e3,0x68f3};
uint16_t spriteMapS105_5[16] = {0x6804,0x6814,0x6824,0x6834,0x6844,0x6854,0x6864,0x6874,0x6884,0x6894,0x68a4,0x68b4,0x68c4,0x68d4,0x68e4,0x68f4};
uint16_t spriteMapS105_6[16] = {0x6805,0x6815,0x6825,0x6835,0x6845,0x6855,0x6865,0x6875,0x6885,0x6895,0x68a5,0x68b5,0x68c5,0x68d5,0x68e5,0x68f5};
uint16_t spriteMapS105_7[16] = {0x6806,0x6816,0x6826,0x6836,0x6846,0x6856,0x6866,0x6876,0x6886,0x6896,0x68a6,0x68b6,0x68c6,0x68d6,0x68e6,0x68f6};
uint16_t spriteMapS105_8[16] = {0x6807,0x6817,0x6827,0x6837,0x6847,0x6857,0x6867,0x6877,0x6887,0x6897,0x68a7,0x68b7,0x68c7,0x68d7,0x68e7,0x68f7};
uint16_t spriteMapS105_9[16] = {0x6808,0x6818,0x6828,0x6838,0x6848,0x6858,0x6868,0x6878,0x6888,0x6898,0x68a8,0x68b8,0x68c8,0x68d8,0x68e8,0x68f8};
uint16_t spriteMapS105_10[16] = {0x6809,0x6819,0x6829,0x6839,0x6849,0x6859,0x6869,0x6879,0x6889,0x6899,0x68a9,0x68b9,0x68c9,0x68d9,0x68e9,0x68f9};
uint16_t spriteMapS105_11[16] = {0x680a,0x681a,0x682a,0x683a,0x684a,0x685a,0x686a,0x687a,0x688a,0x689a,0x68aa,0x68ba,0x68ca,0x68da,0x68ea,0x68fa};
uint16_t spriteMapS105_12[16] = {0x680b,0x681b,0x682b,0x683b,0x684b,0x685b,0x686b,0x687b,0x688b,0x689b,0x68ab,0x68bb,0x68cb,0x68db,0x68eb,0x68fb};
uint16_t spriteMapS105_13[16] = {0x680c,0x681c,0x682c,0x683c,0x684c,0x685c,0x686c,0x687c,0x688c,0x689c,0x68ac,0x68bc,0x68cc,0x68dc,0x68ec,0x68fc};
uint16_t spriteMapS105_14[16] = {0x680d,0x681d,0x682d,0x683d,0x684d,0x685d,0x686d,0x687d,0x688d,0x689d,0x68ad,0x68bd,0x68cd,0x68dd,0x68ed,0x68fd};
uint16_t spriteMapS105_15[16] = {0x680e,0x681e,0x682e,0x683e,0x684e,0x685e,0x686e,0x687e,0x688e,0x689e,0x68ae,0x68be,0x68ce,0x68de,0x68ee,0x68fe};
uint16_t spriteMapS105_16[16] = {0x680f,0x681f,0x682f,0x683f,0x684f,0x685f,0x686f,0x687f,0x688f,0x689f,0x68af,0x68bf,0x68cf,0x68df,0x68ef,0x68ff};
load_palettes(pal105,PALETTES+PALOFFSET*120);
uint16_t SCB1_2common = setSCB1_2(120,0,0,0,0,0);
uint16_t spal105_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS105_1,spal105_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS105_2,spal105_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS105_3,spal105_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS105_4,spal105_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS105_5,spal105_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS105_6,spal105_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS105_7,spal105_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS105_8,spal105_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS105_9,spal105_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS105_10,spal105_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS105_11,spal105_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS105_12,spal105_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS105_13,spal105_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS105_14,spal105_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS105_15,spal105_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS105_16,spal105_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen106(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 106 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal106[16];
setpal(pal106,0x0,0x2000,0x4407,0x7111,0x5222,0x333,0x191c,0x2434,0x2656,0x3b41,0x5d95,0x699a,0x7ccc,0x7ec9,0xeee,0x0);
uint16_t spriteMapS106_1[16] = {0x6900,0x6910,0x6920,0x6930,0x6940,0x6950,0x6960,0x6970,0x6980,0x6990,0x69a0,0x69b0,0x69c0,0x69d0,0x69e0,0x69f0};
uint16_t spriteMapS106_2[16] = {0x6901,0x6911,0x6921,0x6931,0x6941,0x6951,0x6961,0x6971,0x6981,0x6991,0x69a1,0x69b1,0x69c1,0x69d1,0x69e1,0x69f1};
uint16_t spriteMapS106_3[16] = {0x6902,0x6912,0x6922,0x6932,0x6942,0x6952,0x6962,0x6972,0x6982,0x6992,0x69a2,0x69b2,0x69c2,0x69d2,0x69e2,0x69f2};
uint16_t spriteMapS106_4[16] = {0x6903,0x6913,0x6923,0x6933,0x6943,0x6953,0x6963,0x6973,0x6983,0x6993,0x69a3,0x69b3,0x69c3,0x69d3,0x69e3,0x69f3};
uint16_t spriteMapS106_5[16] = {0x6904,0x6914,0x6924,0x6934,0x6944,0x6954,0x6964,0x6974,0x6984,0x6994,0x69a4,0x69b4,0x69c4,0x69d4,0x69e4,0x69f4};
uint16_t spriteMapS106_6[16] = {0x6905,0x6915,0x6925,0x6935,0x6945,0x6955,0x6965,0x6975,0x6985,0x6995,0x69a5,0x69b5,0x69c5,0x69d5,0x69e5,0x69f5};
uint16_t spriteMapS106_7[16] = {0x6906,0x6916,0x6926,0x6936,0x6946,0x6956,0x6966,0x6976,0x6986,0x6996,0x69a6,0x69b6,0x69c6,0x69d6,0x69e6,0x69f6};
uint16_t spriteMapS106_8[16] = {0x6907,0x6917,0x6927,0x6937,0x6947,0x6957,0x6967,0x6977,0x6987,0x6997,0x69a7,0x69b7,0x69c7,0x69d7,0x69e7,0x69f7};
uint16_t spriteMapS106_9[16] = {0x6908,0x6918,0x6928,0x6938,0x6948,0x6958,0x6968,0x6978,0x6988,0x6998,0x69a8,0x69b8,0x69c8,0x69d8,0x69e8,0x69f8};
uint16_t spriteMapS106_10[16] = {0x6909,0x6919,0x6929,0x6939,0x6949,0x6959,0x6969,0x6979,0x6989,0x6999,0x69a9,0x69b9,0x69c9,0x69d9,0x69e9,0x69f9};
uint16_t spriteMapS106_11[16] = {0x690a,0x691a,0x692a,0x693a,0x694a,0x695a,0x696a,0x697a,0x698a,0x699a,0x69aa,0x69ba,0x69ca,0x69da,0x69ea,0x69fa};
uint16_t spriteMapS106_12[16] = {0x690b,0x691b,0x692b,0x693b,0x694b,0x695b,0x696b,0x697b,0x698b,0x699b,0x69ab,0x69bb,0x69cb,0x69db,0x69eb,0x69fb};
uint16_t spriteMapS106_13[16] = {0x690c,0x691c,0x692c,0x693c,0x694c,0x695c,0x696c,0x697c,0x698c,0x699c,0x69ac,0x69bc,0x69cc,0x69dc,0x69ec,0x69fc};
uint16_t spriteMapS106_14[16] = {0x690d,0x691d,0x692d,0x693d,0x694d,0x695d,0x696d,0x697d,0x698d,0x699d,0x69ad,0x69bd,0x69cd,0x69dd,0x69ed,0x69fd};
uint16_t spriteMapS106_15[16] = {0x690e,0x691e,0x692e,0x693e,0x694e,0x695e,0x696e,0x697e,0x698e,0x699e,0x69ae,0x69be,0x69ce,0x69de,0x69ee,0x69fe};
uint16_t spriteMapS106_16[16] = {0x690f,0x691f,0x692f,0x693f,0x694f,0x695f,0x696f,0x697f,0x698f,0x699f,0x69af,0x69bf,0x69cf,0x69df,0x69ef,0x69ff};
load_palettes(pal106,PALETTES+PALOFFSET*121);
uint16_t SCB1_2common = setSCB1_2(121,0,0,0,0,0);
uint16_t spal106_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS106_1,spal106_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS106_2,spal106_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS106_3,spal106_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS106_4,spal106_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS106_5,spal106_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS106_6,spal106_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS106_7,spal106_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS106_8,spal106_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS106_9,spal106_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS106_10,spal106_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS106_11,spal106_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS106_12,spal106_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS106_13,spal106_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS106_14,spal106_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS106_15,spal106_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS106_16,spal106_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen107(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 107 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal107[16];
setpal(pal107,0x0,0x2000,0x111,0x4407,0x7111,0x7222,0x191c,0x6555,0x3b41,0x7777,0x6d85,0x7999,0x7ccc,0x5ec9,0x2eee,0x0);
uint16_t spriteMapS107_1[16] = {0x6a00,0x6a10,0x6a20,0x6a30,0x6a40,0x6a50,0x6a60,0x6a70,0x6a80,0x6a90,0x6aa0,0x6ab0,0x6ac0,0x6ad0,0x6ae0,0x6af0};
uint16_t spriteMapS107_2[16] = {0x6a01,0x6a11,0x6a21,0x6a31,0x6a41,0x6a51,0x6a61,0x6a71,0x6a81,0x6a91,0x6aa1,0x6ab1,0x6ac1,0x6ad1,0x6ae1,0x6af1};
uint16_t spriteMapS107_3[16] = {0x6a02,0x6a12,0x6a22,0x6a32,0x6a42,0x6a52,0x6a62,0x6a72,0x6a82,0x6a92,0x6aa2,0x6ab2,0x6ac2,0x6ad2,0x6ae2,0x6af2};
uint16_t spriteMapS107_4[16] = {0x6a03,0x6a13,0x6a23,0x6a33,0x6a43,0x6a53,0x6a63,0x6a73,0x6a83,0x6a93,0x6aa3,0x6ab3,0x6ac3,0x6ad3,0x6ae3,0x6af3};
uint16_t spriteMapS107_5[16] = {0x6a04,0x6a14,0x6a24,0x6a34,0x6a44,0x6a54,0x6a64,0x6a74,0x6a84,0x6a94,0x6aa4,0x6ab4,0x6ac4,0x6ad4,0x6ae4,0x6af4};
uint16_t spriteMapS107_6[16] = {0x6a05,0x6a15,0x6a25,0x6a35,0x6a45,0x6a55,0x6a65,0x6a75,0x6a85,0x6a95,0x6aa5,0x6ab5,0x6ac5,0x6ad5,0x6ae5,0x6af5};
uint16_t spriteMapS107_7[16] = {0x6a06,0x6a16,0x6a26,0x6a36,0x6a46,0x6a56,0x6a66,0x6a76,0x6a86,0x6a96,0x6aa6,0x6ab6,0x6ac6,0x6ad6,0x6ae6,0x6af6};
uint16_t spriteMapS107_8[16] = {0x6a07,0x6a17,0x6a27,0x6a37,0x6a47,0x6a57,0x6a67,0x6a77,0x6a87,0x6a97,0x6aa7,0x6ab7,0x6ac7,0x6ad7,0x6ae7,0x6af7};
uint16_t spriteMapS107_9[16] = {0x6a08,0x6a18,0x6a28,0x6a38,0x6a48,0x6a58,0x6a68,0x6a78,0x6a88,0x6a98,0x6aa8,0x6ab8,0x6ac8,0x6ad8,0x6ae8,0x6af8};
uint16_t spriteMapS107_10[16] = {0x6a09,0x6a19,0x6a29,0x6a39,0x6a49,0x6a59,0x6a69,0x6a79,0x6a89,0x6a99,0x6aa9,0x6ab9,0x6ac9,0x6ad9,0x6ae9,0x6af9};
uint16_t spriteMapS107_11[16] = {0x6a0a,0x6a1a,0x6a2a,0x6a3a,0x6a4a,0x6a5a,0x6a6a,0x6a7a,0x6a8a,0x6a9a,0x6aaa,0x6aba,0x6aca,0x6ada,0x6aea,0x6afa};
uint16_t spriteMapS107_12[16] = {0x6a0b,0x6a1b,0x6a2b,0x6a3b,0x6a4b,0x6a5b,0x6a6b,0x6a7b,0x6a8b,0x6a9b,0x6aab,0x6abb,0x6acb,0x6adb,0x6aeb,0x6afb};
uint16_t spriteMapS107_13[16] = {0x6a0c,0x6a1c,0x6a2c,0x6a3c,0x6a4c,0x6a5c,0x6a6c,0x6a7c,0x6a8c,0x6a9c,0x6aac,0x6abc,0x6acc,0x6adc,0x6aec,0x6afc};
uint16_t spriteMapS107_14[16] = {0x6a0d,0x6a1d,0x6a2d,0x6a3d,0x6a4d,0x6a5d,0x6a6d,0x6a7d,0x6a8d,0x6a9d,0x6aad,0x6abd,0x6acd,0x6add,0x6aed,0x6afd};
uint16_t spriteMapS107_15[16] = {0x6a0e,0x6a1e,0x6a2e,0x6a3e,0x6a4e,0x6a5e,0x6a6e,0x6a7e,0x6a8e,0x6a9e,0x6aae,0x6abe,0x6ace,0x6ade,0x6aee,0x6afe};
uint16_t spriteMapS107_16[16] = {0x6a0f,0x6a1f,0x6a2f,0x6a3f,0x6a4f,0x6a5f,0x6a6f,0x6a7f,0x6a8f,0x6a9f,0x6aaf,0x6abf,0x6acf,0x6adf,0x6aef,0x6aff};
load_palettes(pal107,PALETTES+PALOFFSET*122);
uint16_t SCB1_2common = setSCB1_2(122,0,0,0,0,0);
uint16_t spal107_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS107_1,spal107_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS107_2,spal107_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS107_3,spal107_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS107_4,spal107_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS107_5,spal107_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS107_6,spal107_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS107_7,spal107_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS107_8,spal107_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS107_9,spal107_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS107_10,spal107_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS107_11,spal107_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS107_12,spal107_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS107_13,spal107_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS107_14,spal107_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS107_15,spal107_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS107_16,spal107_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen108(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 108 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal108[16];
setpal(pal108,0x0,0x2015,0x2f01,0x7fff,0xc78,0x106b,0x49d,0x1000,0x2003,0x358,0x4c34,0x1027,0x2bce,0x579a,0x3038,0x7a00);
uint16_t spriteMapS108_1[16] = {0x6b00,0x6b10,0x6b20,0x6b30,0x6b40,0x6b50,0x6b60,0x6b70,0x6b80,0x6b90,0x6ba0,0x6bb0,0x6bc0,0x6bd0,0x6be0,0x6bf0};
uint16_t spriteMapS108_2[16] = {0x6b01,0x6b11,0x6b21,0x6b31,0x6b41,0x6b51,0x6b61,0x6b71,0x6b81,0x6b91,0x6ba1,0x6bb1,0x6bc1,0x6bd1,0x6be1,0x6bf1};
uint16_t spriteMapS108_3[16] = {0x6b02,0x6b12,0x6b22,0x6b32,0x6b42,0x6b52,0x6b62,0x6b72,0x6b82,0x6b92,0x6ba2,0x6bb2,0x6bc2,0x6bd2,0x6be2,0x6bf2};
uint16_t spriteMapS108_4[16] = {0x6b03,0x6b13,0x6b23,0x6b33,0x6b43,0x6b53,0x6b63,0x6b73,0x6b83,0x6b93,0x6ba3,0x6bb3,0x6bc3,0x6bd3,0x6be3,0x6bf3};
uint16_t spriteMapS108_5[16] = {0x6b04,0x6b14,0x6b24,0x6b34,0x6b44,0x6b54,0x6b64,0x6b74,0x6b84,0x6b94,0x6ba4,0x6bb4,0x6bc4,0x6bd4,0x6be4,0x6bf4};
uint16_t spriteMapS108_6[16] = {0x6b05,0x6b15,0x6b25,0x6b35,0x6b45,0x6b55,0x6b65,0x6b75,0x6b85,0x6b95,0x6ba5,0x6bb5,0x6bc5,0x6bd5,0x6be5,0x6bf5};
uint16_t spriteMapS108_7[16] = {0x6b06,0x6b16,0x6b26,0x6b36,0x6b46,0x6b56,0x6b66,0x6b76,0x6b86,0x6b96,0x6ba6,0x6bb6,0x6bc6,0x6bd6,0x6be6,0x6bf6};
uint16_t spriteMapS108_8[16] = {0x6b07,0x6b17,0x6b27,0x6b37,0x6b47,0x6b57,0x6b67,0x6b77,0x6b87,0x6b97,0x6ba7,0x6bb7,0x6bc7,0x6bd7,0x6be7,0x6bf7};
uint16_t spriteMapS108_9[16] = {0x6b08,0x6b18,0x6b28,0x6b38,0x6b48,0x6b58,0x6b68,0x6b78,0x6b88,0x6b98,0x6ba8,0x6bb8,0x6bc8,0x6bd8,0x6be8,0x6bf8};
uint16_t spriteMapS108_10[16] = {0x6b09,0x6b19,0x6b29,0x6b39,0x6b49,0x6b59,0x6b69,0x6b79,0x6b89,0x6b99,0x6ba9,0x6bb9,0x6bc9,0x6bd9,0x6be9,0x6bf9};
uint16_t spriteMapS108_11[16] = {0x6b0a,0x6b1a,0x6b2a,0x6b3a,0x6b4a,0x6b5a,0x6b6a,0x6b7a,0x6b8a,0x6b9a,0x6baa,0x6bba,0x6bca,0x6bda,0x6bea,0x6bfa};
uint16_t spriteMapS108_12[16] = {0x6b0b,0x6b1b,0x6b2b,0x6b3b,0x6b4b,0x6b5b,0x6b6b,0x6b7b,0x6b8b,0x6b9b,0x6bab,0x6bbb,0x6bcb,0x6bdb,0x6beb,0x6bfb};
uint16_t spriteMapS108_13[16] = {0x6b0c,0x6b1c,0x6b2c,0x6b3c,0x6b4c,0x6b5c,0x6b6c,0x6b7c,0x6b8c,0x6b9c,0x6bac,0x6bbc,0x6bcc,0x6bdc,0x6bec,0x6bfc};
uint16_t spriteMapS108_14[16] = {0x6b0d,0x6b1d,0x6b2d,0x6b3d,0x6b4d,0x6b5d,0x6b6d,0x6b7d,0x6b8d,0x6b9d,0x6bad,0x6bbd,0x6bcd,0x6bdd,0x6bed,0x6bfd};
uint16_t spriteMapS108_15[16] = {0x6b0e,0x6b1e,0x6b2e,0x6b3e,0x6b4e,0x6b5e,0x6b6e,0x6b7e,0x6b8e,0x6b9e,0x6bae,0x6bbe,0x6bce,0x6bde,0x6bee,0x6bfe};
uint16_t spriteMapS108_16[16] = {0x6b0f,0x6b1f,0x6b2f,0x6b3f,0x6b4f,0x6b5f,0x6b6f,0x6b7f,0x6b8f,0x6b9f,0x6baf,0x6bbf,0x6bcf,0x6bdf,0x6bef,0x6bff};
load_palettes(pal108,PALETTES+PALOFFSET*123);
uint16_t SCB1_2common = setSCB1_2(123,0,0,0,0,0);
uint16_t spal108_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS108_1,spal108_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS108_2,spal108_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS108_3,spal108_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS108_4,spal108_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS108_5,spal108_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS108_6,spal108_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS108_7,spal108_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS108_8,spal108_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS108_9,spal108_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS108_10,spal108_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS108_11,spal108_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS108_12,spal108_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS108_13,spal108_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS108_14,spal108_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS108_15,spal108_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS108_16,spal108_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen109(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 109 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal109[16];
setpal(pal109,0x0,0x1ec7,0xfb1,0x3102,0x1999,0x6305,0x18ae,0x342d,0x3100,0x2e21,0x1711,0x6e71,0x634c,0x6def,0x841,0x7fe2);
uint16_t spriteMapS109_1[16] = {0x6c00,0x6c10,0x6c20,0x6c30,0x6c40,0x6c50,0x6c60,0x6c70,0x6c80,0x6c90,0x6ca0,0x6cb0,0x6cc0,0x6cd0,0x6ce0,0x6cf0};
uint16_t spriteMapS109_2[16] = {0x6c01,0x6c11,0x6c21,0x6c31,0x6c41,0x6c51,0x6c61,0x6c71,0x6c81,0x6c91,0x6ca1,0x6cb1,0x6cc1,0x6cd1,0x6ce1,0x6cf1};
uint16_t spriteMapS109_3[16] = {0x6c02,0x6c12,0x6c22,0x6c32,0x6c42,0x6c52,0x6c62,0x6c72,0x6c82,0x6c92,0x6ca2,0x6cb2,0x6cc2,0x6cd2,0x6ce2,0x6cf2};
uint16_t spriteMapS109_4[16] = {0x6c03,0x6c13,0x6c23,0x6c33,0x6c43,0x6c53,0x6c63,0x6c73,0x6c83,0x6c93,0x6ca3,0x6cb3,0x6cc3,0x6cd3,0x6ce3,0x6cf3};
uint16_t spriteMapS109_5[16] = {0x6c04,0x6c14,0x6c24,0x6c34,0x6c44,0x6c54,0x6c64,0x6c74,0x6c84,0x6c94,0x6ca4,0x6cb4,0x6cc4,0x6cd4,0x6ce4,0x6cf4};
uint16_t spriteMapS109_6[16] = {0x6c05,0x6c15,0x6c25,0x6c35,0x6c45,0x6c55,0x6c65,0x6c75,0x6c85,0x6c95,0x6ca5,0x6cb5,0x6cc5,0x6cd5,0x6ce5,0x6cf5};
uint16_t spriteMapS109_7[16] = {0x6c06,0x6c16,0x6c26,0x6c36,0x6c46,0x6c56,0x6c66,0x6c76,0x6c86,0x6c96,0x6ca6,0x6cb6,0x6cc6,0x6cd6,0x6ce6,0x6cf6};
uint16_t spriteMapS109_8[16] = {0x6c07,0x6c17,0x6c27,0x6c37,0x6c47,0x6c57,0x6c67,0x6c77,0x6c87,0x6c97,0x6ca7,0x6cb7,0x6cc7,0x6cd7,0x6ce7,0x6cf7};
uint16_t spriteMapS109_9[16] = {0x6c08,0x6c18,0x6c28,0x6c38,0x6c48,0x6c58,0x6c68,0x6c78,0x6c88,0x6c98,0x6ca8,0x6cb8,0x6cc8,0x6cd8,0x6ce8,0x6cf8};
uint16_t spriteMapS109_10[16] = {0x6c09,0x6c19,0x6c29,0x6c39,0x6c49,0x6c59,0x6c69,0x6c79,0x6c89,0x6c99,0x6ca9,0x6cb9,0x6cc9,0x6cd9,0x6ce9,0x6cf9};
uint16_t spriteMapS109_11[16] = {0x6c0a,0x6c1a,0x6c2a,0x6c3a,0x6c4a,0x6c5a,0x6c6a,0x6c7a,0x6c8a,0x6c9a,0x6caa,0x6cba,0x6cca,0x6cda,0x6cea,0x6cfa};
uint16_t spriteMapS109_12[16] = {0x6c0b,0x6c1b,0x6c2b,0x6c3b,0x6c4b,0x6c5b,0x6c6b,0x6c7b,0x6c8b,0x6c9b,0x6cab,0x6cbb,0x6ccb,0x6cdb,0x6ceb,0x6cfb};
uint16_t spriteMapS109_13[16] = {0x6c0c,0x6c1c,0x6c2c,0x6c3c,0x6c4c,0x6c5c,0x6c6c,0x6c7c,0x6c8c,0x6c9c,0x6cac,0x6cbc,0x6ccc,0x6cdc,0x6cec,0x6cfc};
uint16_t spriteMapS109_14[16] = {0x6c0d,0x6c1d,0x6c2d,0x6c3d,0x6c4d,0x6c5d,0x6c6d,0x6c7d,0x6c8d,0x6c9d,0x6cad,0x6cbd,0x6ccd,0x6cdd,0x6ced,0x6cfd};
uint16_t spriteMapS109_15[16] = {0x6c0e,0x6c1e,0x6c2e,0x6c3e,0x6c4e,0x6c5e,0x6c6e,0x6c7e,0x6c8e,0x6c9e,0x6cae,0x6cbe,0x6cce,0x6cde,0x6cee,0x6cfe};
uint16_t spriteMapS109_16[16] = {0x6c0f,0x6c1f,0x6c2f,0x6c3f,0x6c4f,0x6c5f,0x6c6f,0x6c7f,0x6c8f,0x6c9f,0x6caf,0x6cbf,0x6ccf,0x6cdf,0x6cef,0x6cff};
load_palettes(pal109,PALETTES+PALOFFSET*124);
uint16_t SCB1_2common = setSCB1_2(124,0,0,0,0,0);
uint16_t spal109_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal109_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal109_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal109_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal109_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal109_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal109_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal109_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal109_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal109_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal109_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal109_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal109_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal109_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal109_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal109_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS109_1,spal109_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS109_2,spal109_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS109_3,spal109_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS109_4,spal109_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS109_5,spal109_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS109_6,spal109_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS109_7,spal109_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS109_8,spal109_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS109_9,spal109_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS109_10,spal109_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS109_11,spal109_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS109_12,spal109_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS109_13,spal109_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS109_14,spal109_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS109_15,spal109_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS109_16,spal109_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen110(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 110 ******************************************/
uint16_t  pal110[16];
setpal(pal110,0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9);
load_palettes(pal110,PALETTES+PALOFFSET*125);
}


void NEOGEO_USER showScreen111(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 111 ******************************************/
uint16_t  pal111[16];
setpal(pal111,0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9);
load_palettes(pal111,PALETTES+PALOFFSET*126);
}


void NEOGEO_USER showScreen112(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 112 ******************************************/
uint16_t  pal112[16];
setpal(pal112,0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9);
load_palettes(pal112,PALETTES+PALOFFSET*127);
}


void NEOGEO_USER showScreen113(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 113 ******************************************/
uint16_t  pal113[16];
setpal(pal113,0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9);
load_palettes(pal113,PALETTES+PALOFFSET*128);
}


void NEOGEO_USER showScreen114(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 114 ******************************************/
uint16_t  pal114[16];
setpal(pal114,0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9);
load_palettes(pal114,PALETTES+PALOFFSET*129);
}


void NEOGEO_USER showScreen115(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 115 ******************************************/
uint16_t  pal115[16];
setpal(pal115,0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9);
load_palettes(pal115,PALETTES+PALOFFSET*130);
}


void NEOGEO_USER showScreen116(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 116 ******************************************/
uint16_t  pal116[16];
setpal(pal116,0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9);
load_palettes(pal116,PALETTES+PALOFFSET*131);
}


void NEOGEO_USER showScreen117(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 117 ******************************************/
uint16_t  pal117[16];
setpal(pal117,0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9);
load_palettes(pal117,PALETTES+PALOFFSET*132);
}


void NEOGEO_USER showScreen118(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 118 ******************************************/
uint16_t  pal118[16];
setpal(pal118,0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9);
load_palettes(pal118,PALETTES+PALOFFSET*133);
}


void NEOGEO_USER showScreen119(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 119 ******************************************/
uint16_t  pal119[16];
setpal(pal119,0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9);
load_palettes(pal119,PALETTES+PALOFFSET*134);
}


void NEOGEO_USER showScreen120(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 120 ******************************************/
uint16_t  pal120[16];
setpal(pal120,0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9);
load_palettes(pal120,PALETTES+PALOFFSET*135);
}


void NEOGEO_USER showScreen121(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 121 ******************************************/
uint16_t  pal121[16];
setpal(pal121,0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9);
load_palettes(pal121,PALETTES+PALOFFSET*136);
}

static const NGPaletteAsset ng_screen_palette_assets[] = {
    {1,16,{0x0,0x2545,0x4583,0x17ff,0x6864,0x3a78,0x7435,0x58a2,0x7367,0x5a73,0x5efe,0x3754,0x2464,0x17aa,0x1e93,0x2546}},
    {2,17,{0x0,0x6462,0x4023,0x3921,0x6133,0x2ce0,0x1521,0x5151,0x2014,0x47c0,0x3005,0x2581,0x7e51,0x638a,0x1352,0x3355}},
    {3,18,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {4,19,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {5,20,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {6,21,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {7,22,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {8,23,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {9,24,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {10,25,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {11,26,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {12,27,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {13,28,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {14,29,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {15,30,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {16,31,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {17,32,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {18,33,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {19,34,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {20,35,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {21,36,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {22,37,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {23,38,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {24,39,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {25,40,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {26,41,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {27,42,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {28,43,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {29,44,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {30,45,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {31,46,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {32,47,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {33,48,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {34,49,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {35,50,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {36,51,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {37,52,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {38,53,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {39,54,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {40,55,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {41,56,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {42,57,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {43,58,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {44,59,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {45,60,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {46,61,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {47,62,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {48,63,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {49,64,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {50,65,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {51,66,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {52,67,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {53,68,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {54,69,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {55,70,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {56,71,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {57,72,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {58,73,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {59,74,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {60,75,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {61,76,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {62,77,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {63,78,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {64,79,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {65,80,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {66,81,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {67,82,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {68,83,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {69,84,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {70,85,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {71,86,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {72,87,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {73,88,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {74,89,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {75,90,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {76,91,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {77,92,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {78,93,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {79,94,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {80,95,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {81,96,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {82,97,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {83,98,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {84,99,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {85,100,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {86,101,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {87,102,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {88,103,{0x0,0x4aaa,0x3655,0x3942,0x7baf,0xfda,0x2b73,0x2835,0x7eee,0x6b86,0x6642,0x5235,0x5111,0x476e,0x6eb5,0x5321}},
    {89,104,{0x0,0x3eef,0x346f,0x5acf,0x58af,0x7cdf,0x168f,0x579f,0x1def,0x1cdf,0x59bf,0x3abf,0x3bcf,0x1eff,0x39af,0x5eff}},
    {90,105,{0x0,0x3abf,0x746f,0x335f,0x1eff,0x558f,0x768f,0x3ddf,0x5acf,0x1cdf,0x59bf,0x779f,0x18af,0x3bcf,0x39af,0x414f}},
    {91,106,{0x0,0x3ecf,0x797f,0x1b9f,0x7a8f,0x5cbf,0xbaf,0x5eef,0x1caf,0x197f,0x5dcf,0x674f,0x1a8f,0x7caf,0x3dbf,0x586f}},
    {92,107,{0x0,0x7edf,0x5baf,0x797f,0x1cbf,0x1b9f,0x1a8f,0x3ecf,0x1edf,0x785f,0x1fef,0x197f,0x7b9f,0x3caf,0x5caf,0x763f}},
    {93,108,{0x0,0x1caf,0x1b9f,0x675f,0x1fef,0x174f,0x3ecf,0x1a8f,0x753d,0x1dbf,0x687e,0x686f,0x652f,0x553e,0x396f,0x575f}},
    {94,109,{0x0,0x3456,0x4ff2,0x6f10,0x3fe9,0x1115,0x6fa1,0xfff,0x48be,0x7910,0x18ff,0x32e,0x6964,0x448e,0x7400,0x0}},
    {95,110,{0x0,0x0,0x5ff2,0x132e,0xb21,0x57ff,0x2fb2,0xfff,0x4f10,0xf82,0x16ae,0x6611,0x5015,0x55d,0x1777,0x3300}},
    {96,111,{0x0,0xfff,0x0,0x2f91,0x7fc0,0x2f61,0x6128,0x18ff,0x3556,0x4921,0xed8,0x6f00,0x1410,0x622e,0x77ae,0x4ff2}},
    {97,112,{0x0,0x4f00,0x0,0x6ff0,0x7317,0x4fa0,0x3610,0x1fe9,0x5fe1,0x5eff,0x633e,0x369d,0x6910,0x1f70,0x4fc0,0x7300}},
    {98,113,{0x0,0x42e,0x6f00,0x0,0x17ff,0x4fc0,0x4dd8,0x3f80,0x3fff,0x75ae,0x5410,0x5015,0x5ff1,0x2557,0x7920,0x156d}},
    {99,114,{0x0,0x422e,0x0,0x7fff,0x6f00,0x5fc0,0x7ad,0x6411,0x4921,0xbff,0x147e,0x54ff,0x7cb7,0x3115,0x2f81,0x4ff2}},
    {100,115,{0x0,0x3fff,0x0,0x4f10,0x3910,0x7fe1,0x17ff,0x7f90,0x422e,0x3e89,0x5775,0x347e,0x47be,0x6116,0x3400,0x2fe9}},
    {101,116,{0x0,0x2000,0x4407,0x7111,0xa00,0x7222,0x191c,0x444,0x7555,0xb52,0x2888,0x7d85,0x7aaa,0x7ccc,0x7ec9,0x0}},
    {102,117,{0x0,0x2000,0x4407,0x7111,0x4900,0x7222,0x191c,0x5f0f,0x555,0xb52,0x5787,0x5d95,0x2aaa,0x5ec9,0xddd,0x0}},
    {103,118,{0x0,0x2000,0x4407,0x7111,0x5222,0x333,0x191c,0x5555,0x3b41,0x888,0x7999,0x5d95,0x7ccc,0x7ec9,0x7eee,0x0}},
    {104,119,{0x0,0x2000,0x406,0x7111,0x609,0x1222,0x333,0x191c,0x7555,0x3b41,0x999,0x5d95,0x5ccc,0x7ec9,0x7eee,0x0}},
    {105,120,{0x0,0x2000,0x4407,0x7111,0x5222,0x333,0x191c,0x2434,0x6556,0x3b41,0x5999,0x5d95,0x7ccc,0x7ec9,0x6eee,0x0}},
    {106,121,{0x0,0x2000,0x4407,0x7111,0x5222,0x333,0x191c,0x2434,0x2656,0x3b41,0x5d95,0x699a,0x7ccc,0x7ec9,0xeee,0x0}},
    {107,122,{0x0,0x2000,0x111,0x4407,0x7111,0x7222,0x191c,0x6555,0x3b41,0x7777,0x6d85,0x7999,0x7ccc,0x5ec9,0x2eee,0x0}},
    {108,123,{0x0,0x2015,0x2f01,0x7fff,0xc78,0x106b,0x49d,0x1000,0x2003,0x358,0x4c34,0x1027,0x2bce,0x579a,0x3038,0x7a00}},
    {109,124,{0x0,0x1ec7,0xfb1,0x3102,0x1999,0x6305,0x18ae,0x342d,0x3100,0x2e21,0x1711,0x6e71,0x634c,0x6def,0x841,0x7fe2}},
    {110,125,{0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9}},
    {111,126,{0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9}},
    {112,127,{0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9}},
    {113,128,{0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9}},
    {114,129,{0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9}},
    {115,130,{0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9}},
    {116,131,{0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9}},
    {117,132,{0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9}},
    {118,133,{0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9}},
    {119,134,{0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9}},
    {120,135,{0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9}},
    {121,136,{0x0,0x4fec,0x2101,0x964,0x6422,0xd76,0x3a63,0x655,0xfff,0xaaa,0x7632,0x1210,0x337a,0x7a87,0x4146,0x5db9}},
};
const uint16_t ng_screen_palette_count = 121;

uint8_t NEOGEO_USER ng_load_screen_palette(uint16_t screen_id) {
    return ng_palette_load_asset(ng_screen_palette_assets, ng_screen_palette_count, screen_id);
}

const NGArtAsset ng_screen_art_assets[] = {
    {1,NG_ART_TYPE_BACKGROUND,16,48,255,16,10,10,16,0,48,256,155},
    {2,NG_ART_TYPE_BACKGROUND,17,288,511,16,11,11,16,0,32,256,167},
    {3,NG_ART_TYPE_SPRITE,18,613,767,6,10,10,16,80,96,90,145},
    {4,NG_ART_TYPE_SPRITE,19,869,1023,6,10,10,16,80,96,90,146},
    {5,NG_ART_TYPE_SPRITE,20,1125,1279,6,10,10,16,80,96,91,146},
    {6,NG_ART_TYPE_SPRITE,21,1381,1535,5,10,10,16,80,96,75,146},
    {7,NG_ART_TYPE_SPRITE,22,1637,1791,6,10,10,16,80,96,84,145},
    {8,NG_ART_TYPE_SPRITE,23,1893,2047,6,10,10,16,80,96,83,147},
    {9,NG_ART_TYPE_SPRITE,24,2149,2303,6,10,10,16,80,96,83,146},
    {10,NG_ART_TYPE_SPRITE,25,2405,2559,5,10,10,16,80,96,79,147},
    {11,NG_ART_TYPE_SPRITE,26,2661,2815,6,10,10,16,80,96,82,147},
    {12,NG_ART_TYPE_SPRITE,27,2917,3071,6,10,10,16,80,96,82,148},
    {13,NG_ART_TYPE_SPRITE,28,3173,3327,5,10,10,16,80,96,70,147},
    {14,NG_ART_TYPE_SPRITE,29,3492,3583,8,6,6,16,64,160,113,96},
    {15,NG_ART_TYPE_SPRITE,30,3716,3839,8,8,8,16,64,128,126,119},
    {16,NG_ART_TYPE_SPRITE,31,3971,4095,9,8,8,16,48,128,129,118},
    {17,NG_ART_TYPE_SPRITE,32,4243,4351,9,7,7,16,48,144,139,106},
    {18,NG_ART_TYPE_SPRITE,33,4483,4607,9,8,8,16,48,128,134,120},
    {19,NG_ART_TYPE_SPRITE,34,4724,4863,7,9,9,16,64,112,110,140},
    {20,NG_ART_TYPE_SPRITE,35,4964,5119,7,10,10,16,64,96,107,145},
    {21,NG_ART_TYPE_SPRITE,36,5236,5375,7,9,9,16,64,112,102,134},
    {22,NG_ART_TYPE_SPRITE,37,5492,5631,8,9,9,16,64,112,118,129},
    {23,NG_ART_TYPE_SPRITE,38,5780,5887,8,7,7,16,64,144,113,110},
    {24,NG_ART_TYPE_SPRITE,39,6036,6143,7,7,7,16,64,144,101,98},
    {25,NG_ART_TYPE_SPRITE,40,6260,6399,7,9,9,16,64,112,108,131},
    {26,NG_ART_TYPE_SPRITE,41,6515,6655,9,9,9,16,48,112,143,131},
    {27,NG_ART_TYPE_SPRITE,42,6770,6911,11,9,9,16,32,112,169,129},
    {28,NG_ART_TYPE_SPRITE,43,7042,7167,11,8,8,16,32,128,162,128},
    {29,NG_ART_TYPE_SPRITE,44,7297,7423,13,8,8,16,16,128,193,128},
    {30,NG_ART_TYPE_SPRITE,45,7537,7679,14,9,9,16,16,112,224,138},
    {31,NG_ART_TYPE_SPRITE,46,7810,7935,12,8,8,16,32,128,184,128},
    {32,NG_ART_TYPE_SPRITE,47,8050,8191,12,9,9,16,32,112,177,138},
    {33,NG_ART_TYPE_SPRITE,48,8308,8447,8,9,9,16,64,112,123,139},
    {34,NG_ART_TYPE_SPRITE,49,8563,8703,9,9,9,16,48,112,131,135},
    {35,NG_ART_TYPE_SPRITE,50,8820,8959,7,9,9,16,64,112,110,131},
    {36,NG_ART_TYPE_SPRITE,51,9091,9215,9,8,8,16,48,128,135,128},
    {37,NG_ART_TYPE_SPRITE,52,9347,9471,10,8,8,16,48,128,149,122},
    {38,NG_ART_TYPE_SPRITE,53,9601,9727,13,8,8,16,16,128,199,125},
    {39,NG_ART_TYPE_SPRITE,54,9860,9983,7,8,8,16,64,128,111,127},
    {40,NG_ART_TYPE_SPRITE,55,10197,10239,6,3,3,16,80,208,83,44},
    {41,NG_ART_TYPE_SPRITE,56,10372,10495,8,8,8,16,64,128,113,122},
    {42,NG_ART_TYPE_SPRITE,57,10693,10751,6,4,4,16,80,192,93,49},
    {43,NG_ART_TYPE_SPRITE,58,10867,11007,10,9,9,16,48,112,149,130},
    {44,NG_ART_TYPE_SPRITE,59,11141,11263,6,8,8,16,80,128,95,119},
    {45,NG_ART_TYPE_SPRITE,60,11395,11519,9,8,8,16,48,128,131,119},
    {46,NG_ART_TYPE_SPRITE,61,11735,11775,2,3,3,16,112,208,23,34},
    {47,NG_ART_TYPE_SPRITE,62,11941,12031,6,6,6,16,80,160,90,91},
    {48,NG_ART_TYPE_SPRITE,63,12262,12287,3,2,2,16,96,224,40,29},
    {49,NG_ART_TYPE_SPRITE,64,12435,12543,9,7,7,16,48,144,143,112},
    {50,NG_ART_TYPE_SPRITE,65,12692,12799,7,7,7,16,64,144,110,103},
    {51,NG_ART_TYPE_SPRITE,66,13030,13055,4,2,2,16,96,224,57,18},
    {52,NG_ART_TYPE_SPRITE,67,13200,13311,16,7,7,16,0,144,243,100},
    {53,NG_ART_TYPE_SPRITE,68,13442,13567,12,8,8,16,32,128,181,119},
    {54,NG_ART_TYPE_SPRITE,69,13747,13823,10,5,5,16,48,176,160,69},
    {55,NG_ART_TYPE_SPRITE,70,13954,14079,11,8,8,16,32,128,172,115},
    {56,NG_ART_TYPE_SPRITE,71,14210,14335,11,8,8,16,32,128,169,126},
    {57,NG_ART_TYPE_SPRITE,72,14466,14591,12,8,8,16,32,128,180,126},
    {58,NG_ART_TYPE_SPRITE,73,14723,14847,10,8,8,16,48,128,145,126},
    {59,NG_ART_TYPE_SPRITE,74,14980,15103,7,8,8,16,64,128,104,116},
    {60,NG_ART_TYPE_SPRITE,75,15236,15359,7,8,8,16,64,128,101,119},
    {61,NG_ART_TYPE_SPRITE,76,15540,15615,7,5,5,16,64,176,100,77},
    {62,NG_ART_TYPE_SPRITE,77,15813,15871,6,4,4,16,80,192,94,53},
    {63,NG_ART_TYPE_SPRITE,78,16005,16127,5,8,8,16,80,128,77,126},
    {64,NG_ART_TYPE_SPRITE,79,16245,16383,6,9,9,16,80,112,88,129},
    {65,NG_ART_TYPE_SPRITE,80,16517,16639,5,8,8,16,80,128,67,124},
    {66,NG_ART_TYPE_SPRITE,81,16789,16895,5,7,7,16,80,144,77,103},
    {67,NG_ART_TYPE_SPRITE,82,17127,17151,2,2,2,16,112,224,30,28},
    {68,NG_ART_TYPE_SPRITE,83,17316,17407,7,6,6,16,64,160,97,94},
    {69,NG_ART_TYPE_SPRITE,84,17572,17663,7,6,6,16,64,160,103,94},
    {70,NG_ART_TYPE_SPRITE,85,17828,17919,7,6,6,16,64,160,101,94},
    {71,NG_ART_TYPE_SPRITE,86,18084,18175,7,6,6,16,64,160,101,94},
    {72,NG_ART_TYPE_SPRITE,87,18341,18431,5,6,6,16,80,160,76,94},
    {73,NG_ART_TYPE_SPRITE,88,18597,18687,6,6,6,16,80,160,89,94},
    {74,NG_ART_TYPE_SPRITE,89,18853,18943,6,6,6,16,80,160,90,83},
    {75,NG_ART_TYPE_SPRITE,90,19109,19199,6,6,6,16,80,160,95,84},
    {76,NG_ART_TYPE_SPRITE,91,19365,19455,6,6,6,16,80,160,95,84},
    {77,NG_ART_TYPE_SPRITE,92,19621,19711,6,6,6,16,80,160,93,84},
    {78,NG_ART_TYPE_SPRITE,93,19876,19967,8,6,6,16,64,160,122,90},
    {79,NG_ART_TYPE_SPRITE,94,20132,20223,7,6,6,16,64,160,100,93},
    {80,NG_ART_TYPE_SPRITE,95,20388,20479,7,6,6,16,64,160,97,94},
    {81,NG_ART_TYPE_SPRITE,96,20678,20735,4,4,4,16,96,192,50,54},
    {82,NG_ART_TYPE_SPRITE,97,20900,20991,7,6,6,16,64,160,107,83},
    {83,NG_ART_TYPE_SPRITE,98,21156,21247,7,6,6,16,64,160,106,83},
    {84,NG_ART_TYPE_SPRITE,99,21412,21503,7,6,6,16,64,160,105,83},
    {85,NG_ART_TYPE_SPRITE,100,21668,21759,7,6,6,16,64,160,108,83},
    {86,NG_ART_TYPE_SPRITE,101,21924,22015,7,6,6,16,64,160,105,83},
    {87,NG_ART_TYPE_SPRITE,102,22180,22271,8,6,6,16,64,160,118,83},
    {88,NG_ART_TYPE_SPRITE,103,22436,22527,7,6,6,16,64,160,105,83},
    {89,NG_ART_TYPE_SPRITE,104,22629,22783,6,3,3,16,80,96,93,48},
    {90,NG_ART_TYPE_SPRITE,105,22885,23039,6,3,3,16,80,96,83,44},
    {91,NG_ART_TYPE_SPRITE,106,23123,23295,10,6,6,16,48,80,159,82},
    {92,NG_ART_TYPE_SPRITE,107,23379,23551,9,5,5,16,48,80,137,65},
    {93,NG_ART_TYPE_SPRITE,108,23654,23807,3,3,3,16,96,96,34,39},
    {94,NG_ART_TYPE_SCREEN,109,23840,24063,16,11,11,16,0,32,256,171},
    {95,NG_ART_TYPE_SCREEN,110,24096,24319,16,11,11,16,0,32,256,171},
    {96,NG_ART_TYPE_SCREEN,111,24352,24575,16,11,11,16,0,32,256,171},
    {97,NG_ART_TYPE_SCREEN,112,24608,24831,16,11,11,16,0,32,256,171},
    {98,NG_ART_TYPE_SCREEN,113,24864,25087,16,11,11,16,0,32,256,171},
    {99,NG_ART_TYPE_SCREEN,114,25120,25343,16,11,11,16,0,32,256,171},
    {100,NG_ART_TYPE_SCREEN,115,25376,25599,16,11,11,16,0,32,256,171},
    {101,NG_ART_TYPE_SCREEN,116,25600,25855,16,16,16,16,0,0,256,256},
    {102,NG_ART_TYPE_SCREEN,117,25856,26111,16,16,16,16,0,0,256,256},
    {103,NG_ART_TYPE_SCREEN,118,26112,26367,16,16,16,16,0,0,256,256},
    {104,NG_ART_TYPE_SCREEN,119,26368,26623,16,16,16,16,0,0,256,256},
    {105,NG_ART_TYPE_SCREEN,120,26624,26879,16,16,16,16,0,0,256,256},
    {106,NG_ART_TYPE_SCREEN,121,26880,27135,16,16,16,16,0,0,256,256},
    {107,NG_ART_TYPE_SCREEN,122,27136,27391,16,16,16,16,0,0,256,256},
    {108,NG_ART_TYPE_SCREEN,123,27392,27647,16,16,16,16,0,0,256,256},
    {109,NG_ART_TYPE_SCREEN,124,27680,27903,16,12,12,16,0,32,256,192},
    {110,NG_ART_TYPE_SPRITE,125,27922,28159,12,15,15,16,32,16,191,225},
    {111,NG_ART_TYPE_SPRITE,126,28178,28415,12,15,15,16,32,16,187,227},
    {112,NG_ART_TYPE_SPRITE,127,28434,28671,12,15,15,16,32,16,190,226},
    {113,NG_ART_TYPE_SPRITE,128,28690,28927,12,15,15,16,32,16,190,228},
    {114,NG_ART_TYPE_SPRITE,129,28946,29183,12,15,15,16,32,16,191,227},
    {115,NG_ART_TYPE_SPRITE,130,29202,29439,12,15,15,16,32,16,187,225},
    {116,NG_ART_TYPE_SPRITE,131,29458,29695,12,15,15,16,32,16,189,208},
    {117,NG_ART_TYPE_SPRITE,132,29714,29951,12,15,15,16,32,16,182,201},
    {118,NG_ART_TYPE_SPRITE,133,29970,30207,12,15,15,16,32,16,180,199},
    {119,NG_ART_TYPE_SPRITE,134,30226,30463,12,15,15,16,32,16,189,200},
    {120,NG_ART_TYPE_SPRITE,135,30482,30719,12,15,15,16,32,16,185,202},
    {121,NG_ART_TYPE_SPRITE,136,30738,30975,12,15,15,16,32,16,187,198},
};
const uint16_t ng_screen_art_asset_count = 121;

const NGArtAsset * NEOGEO_USER ng_screen_art_asset(uint16_t screen_id) {
    return ng_art_asset_find(ng_screen_art_assets, ng_screen_art_asset_count, screen_id);
}

const NGShowScreenFn ng_screen_table[NG_SCREEN_TABLE_MAX] = {
    0, /* index 0 unused */
    showScreen1,
    showScreen2,
    showScreen3,
    showScreen4,
    showScreen5,
    showScreen6,
    showScreen7,
    showScreen8,
    showScreen9,
    showScreen10,
    showScreen11,
    showScreen12,
    showScreen13,
    showScreen14,
    showScreen15,
    showScreen16,
    showScreen17,
    showScreen18,
    showScreen19,
    showScreen20,
    showScreen21,
    showScreen22,
    showScreen23,
    showScreen24,
    showScreen25,
    showScreen26,
    showScreen27,
    showScreen28,
    showScreen29,
    showScreen30,
    showScreen31,
    showScreen32,
    showScreen33,
    showScreen34,
    showScreen35,
    showScreen36,
    showScreen37,
    showScreen38,
    showScreen39,
    showScreen40,
    showScreen41,
    showScreen42,
    showScreen43,
    showScreen44,
    showScreen45,
    showScreen46,
    showScreen47,
    showScreen48,
    showScreen49,
    showScreen50,
    showScreen51,
    showScreen52,
    showScreen53,
    showScreen54,
    showScreen55,
    showScreen56,
    showScreen57,
    showScreen58,
    showScreen59,
    showScreen60,
    showScreen61,
    showScreen62,
    showScreen63,
    showScreen64,
    showScreen65,
    showScreen66,
    showScreen67,
    showScreen68,
    showScreen69,
    showScreen70,
    showScreen71,
    showScreen72,
    showScreen73,
    showScreen74,
    showScreen75,
    showScreen76,
    showScreen77,
    showScreen78,
    showScreen79,
    showScreen80,
    showScreen81,
    showScreen82,
    showScreen83,
    showScreen84,
    showScreen85,
    showScreen86,
    showScreen87,
    showScreen88,
    showScreen89,
    showScreen90,
    showScreen91,
    showScreen92,
    showScreen93,
    showScreen94,
    showScreen95,
    showScreen96,
    showScreen97,
    showScreen98,
    showScreen99,
    showScreen100,
    showScreen101,
    showScreen102,
    showScreen103,
    showScreen104,
    showScreen105,
    showScreen106,
    showScreen107,
    showScreen108,
    showScreen109,
    showScreen110,
    showScreen111,
    showScreen112,
    showScreen113,
    showScreen114,
    showScreen115,
    showScreen116,
    showScreen117,
    showScreen118,
    showScreen119,
    showScreen120,
    showScreen121,
};
const uint16_t ng_screen_count = 121;
