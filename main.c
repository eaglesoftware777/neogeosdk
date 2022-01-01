#include <stdint.h>
#include "sdk/macro.h"


//main demo game
void NEOGEO_USER maingame(void) {

  clearFix();
  clearSprs();
  fixtext_out(15, 10, "NEO GEO STARTED  ...", 0);
  playSoundtest(0x22);
  cyclexs(2);
  clearFix();
  clearSprs();
  playgame();
  cyclexs(10);
}


//play game
void NEOGEO_USER playgame() {
  // showScreen1(16,480,0xF,0xCF,16,0xFFF);
  int i = 0;
  for (i = 1; i <= 100; i++) {
    fixtext_out(15, 7, "FRAME 1", 0);

    waitVbl();
    showScreen2(16, 520, 0xF, 0xAF, 9, 0xFFF);
    cyclexms(30);
    fixtext_out(15, 7, "FRAME 2", 0);

    waitVbl();
    showScreen3(16, 520, 0xF, 0xAF, 9, 0xFFF);
    cyclexms(30);
    fixtext_out(15, 7, "FRAME 3", 0);
    waitVbl();
    showScreen4(16, 520, 0xF, 0xAF, 9, 0xFFF);
    cyclexms(30);
    fixtext_out(15, 7, "FRAME 4", 0);

    waitVbl();
    showScreen5(16, 520, 0xF, 0xAF, 9, 0xFFF);
    cyclexms(30);
    fixtext_out(15, 7, "FRAME 5", 0);

    waitVbl();
    showScreen6(16, 520, 0xF, 0xAF, 9, 0xFFF);
    cyclexms(30);
    waitVbl();
    fixtext_out(15, 7, "FRAME 6", 0);

    showScreen7(16, 520, 0xF, 0xAF, 9, 0xFFF);
    cyclexms(30);
    fixtext_out(15, 7, "FRAME 7", 0);

    waitVbl();

    showScreen8(16, 520, 0xF, 0xAF, 9, 0xFFF);
    cyclexms(30);
    waitVbl();
  }
  cyclexms(10);
}

void NEOGEO_USER showScreen1(int x0, int y0, int xr, int yr, int min_crt_sz,
                             uint16_t backdrop) {
  /****************************************** screen 1
   * ******************************************/
  uint16_t SCB2 = 0x0;
  uint16_t SCB3 = 0x0;
  uint16_t SCB4 = 0x0;
  uint16_t pal1[16];
  setpal(pal1, 0xFFF, 0xFFF, 0x213, 0x1413, 0x2613, 0x7823, 0x6d32, 0x4d56,
         0x5e69, 0x6f73, 0x4975, 0x6653, 0x3346, 0x5432, 0x522, 0x0);
  uint16_t spriteMapS1_1[16] = {0x0,  0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
                                0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0};
  uint16_t spriteMapS1_2[16] = {0x1,  0x11, 0x21, 0x31, 0x41, 0x51, 0x61, 0x71,
                                0x81, 0x91, 0xa1, 0xb1, 0xc1, 0xd1, 0xe1, 0xf1};
  uint16_t spriteMapS1_3[16] = {0x2,  0x12, 0x22, 0x32, 0x42, 0x52, 0x62, 0x72,
                                0x82, 0x92, 0xa2, 0xb2, 0xc2, 0xd2, 0xe2, 0xf2};
  uint16_t spriteMapS1_4[16] = {0x3,  0x13, 0x23, 0x33, 0x43, 0x53, 0x63, 0x73,
                                0x83, 0x93, 0xa3, 0xb3, 0xc3, 0xd3, 0xe3, 0xf3};
  uint16_t spriteMapS1_5[16] = {0x4,  0x14, 0x24, 0x34, 0x44, 0x54, 0x64, 0x74,
                                0x84, 0x94, 0xa4, 0xb4, 0xc4, 0xd4, 0xe4, 0xf4};
  uint16_t spriteMapS1_6[16] = {0x5,  0x15, 0x25, 0x35, 0x45, 0x55, 0x65, 0x75,
                                0x85, 0x95, 0xa5, 0xb5, 0xc5, 0xd5, 0xe5, 0xf5};
  uint16_t spriteMapS1_7[16] = {0x6,  0x16, 0x26, 0x36, 0x46, 0x56, 0x66, 0x76,
                                0x86, 0x96, 0xa6, 0xb6, 0xc6, 0xd6, 0xe6, 0xf6};
  uint16_t spriteMapS1_8[16] = {0x7,  0x17, 0x27, 0x37, 0x47, 0x57, 0x67, 0x77,
                                0x87, 0x97, 0xa7, 0xb7, 0xc7, 0xd7, 0xe7, 0xf7};
  uint16_t spriteMapS1_9[16] = {0x8,  0x18, 0x28, 0x38, 0x48, 0x58, 0x68, 0x78,
                                0x88, 0x98, 0xa8, 0xb8, 0xc8, 0xd8, 0xe8, 0xf8};
  uint16_t spriteMapS1_10[16] = {0x9,  0x19, 0x29, 0x39, 0x49, 0x59,
                                 0x69, 0x79, 0x89, 0x99, 0xa9, 0xb9,
                                 0xc9, 0xd9, 0xe9, 0xf9};
  uint16_t spriteMapS1_11[16] = {0xa,  0x1a, 0x2a, 0x3a, 0x4a, 0x5a,
                                 0x6a, 0x7a, 0x8a, 0x9a, 0xaa, 0xba,
                                 0xca, 0xda, 0xea, 0xfa};
  uint16_t spriteMapS1_12[16] = {0xb,  0x1b, 0x2b, 0x3b, 0x4b, 0x5b,
                                 0x6b, 0x7b, 0x8b, 0x9b, 0xab, 0xbb,
                                 0xcb, 0xdb, 0xeb, 0xfb};
  uint16_t spriteMapS1_13[16] = {0xc,  0x1c, 0x2c, 0x3c, 0x4c, 0x5c,
                                 0x6c, 0x7c, 0x8c, 0x9c, 0xac, 0xbc,
                                 0xcc, 0xdc, 0xec, 0xfc};
  uint16_t spriteMapS1_14[16] = {0xd,  0x1d, 0x2d, 0x3d, 0x4d, 0x5d,
                                 0x6d, 0x7d, 0x8d, 0x9d, 0xad, 0xbd,
                                 0xcd, 0xdd, 0xed, 0xfd};
  uint16_t spriteMapS1_15[16] = {0xe,  0x1e, 0x2e, 0x3e, 0x4e, 0x5e,
                                 0x6e, 0x7e, 0x8e, 0x9e, 0xae, 0xbe,
                                 0xce, 0xde, 0xee, 0xfe};
  uint16_t spriteMapS1_16[16] = {0xf,  0x1f, 0x2f, 0x3f, 0x4f, 0x5f,
                                 0x6f, 0x7f, 0x8f, 0x9f, 0xaf, 0xbf,
                                 0xcf, 0xdf, 0xef, 0xff};
  load_palettes(pal1, PALETTES + PALOFFSET * 2);
  uint16_t SCB1_2common = setSCB1_2(2, 0, 0, 0, 0, 0);
  uint16_t spal1_1[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal1_2[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal1_3[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal1_4[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal1_5[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal1_6[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal1_7[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal1_8[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal1_9[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal1_10[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal1_11[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal1_12[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal1_13[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal1_14[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal1_15[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal1_16[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 0, min_crt_sz);
  SCB4 = setSCB4(x0);
  setBACKDROP(backdrop);
  vram_sprite(64 * 16, 1, 0, spriteMapS1_1, spal1_1, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 1);
  setBACKDROP(backdrop);
  vram_sprite(64 * 17, 1, 1, spriteMapS1_2, spal1_2, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 2);
  setBACKDROP(backdrop);
  vram_sprite(64 * 18, 1, 2, spriteMapS1_3, spal1_3, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 3);
  setBACKDROP(backdrop);
  vram_sprite(64 * 19, 1, 3, spriteMapS1_4, spal1_4, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 4);
  setBACKDROP(backdrop);
  vram_sprite(64 * 20, 1, 4, spriteMapS1_5, spal1_5, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 5);
  setBACKDROP(backdrop);
  vram_sprite(64 * 21, 1, 5, spriteMapS1_6, spal1_6, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 6);
  setBACKDROP(backdrop);
  vram_sprite(64 * 22, 1, 6, spriteMapS1_7, spal1_7, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 7);
  setBACKDROP(backdrop);
  vram_sprite(64 * 23, 1, 7, spriteMapS1_8, spal1_8, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 8);
  setBACKDROP(backdrop);
  vram_sprite(64 * 24, 1, 8, spriteMapS1_9, spal1_9, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 9);
  setBACKDROP(backdrop);
  vram_sprite(64 * 25, 1, 9, spriteMapS1_10, spal1_10, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 10);
  setBACKDROP(backdrop);
  vram_sprite(64 * 26, 1, 10, spriteMapS1_11, spal1_11, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 11);
  setBACKDROP(backdrop);
  vram_sprite(64 * 27, 1, 11, spriteMapS1_12, spal1_12, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 12);
  setBACKDROP(backdrop);
  vram_sprite(64 * 28, 1, 12, spriteMapS1_13, spal1_13, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 13);
  setBACKDROP(backdrop);
  vram_sprite(64 * 29, 1, 13, spriteMapS1_14, spal1_14, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 14);
  setBACKDROP(backdrop);
  vram_sprite(64 * 30, 1, 14, spriteMapS1_15, spal1_15, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 15);
  setBACKDROP(backdrop);
  vram_sprite(64 * 31, 1, 15, spriteMapS1_16, spal1_16, 16, SCB2, SCB3, SCB4);
}

void NEOGEO_USER showScreen2(int x0, int y0, int xr, int yr, int min_crt_sz,
                             uint16_t backdrop) {
  /****************************************** screen 2
   * ******************************************/
  uint16_t SCB2 = 0x0;
  uint16_t SCB3 = 0x0;
  uint16_t SCB4 = 0x0;
  uint16_t pal2[16];
  setpal(pal2, 0xFFF, 0xFFF, 0x0, 0x4407, 0x7101, 0xa00, 0x5212, 0x390c, 0x424,
         0x7525, 0x2b22, 0x848, 0x5d45, 0x5a5a, 0x5c6c, 0x0);
  uint16_t spriteMapS2_1[16] = {0x100, 0x110, 0x120, 0x130, 0x140, 0x150,
                                0x160, 0x170, 0x180, 0x190, 0x1a0, 0x1b0,
                                0x1c0, 0x1d0, 0x1e0, 0x1f0};
  uint16_t spriteMapS2_2[16] = {0x101, 0x111, 0x121, 0x131, 0x141, 0x151,
                                0x161, 0x171, 0x181, 0x191, 0x1a1, 0x1b1,
                                0x1c1, 0x1d1, 0x1e1, 0x1f1};
  uint16_t spriteMapS2_3[16] = {0x102, 0x112, 0x122, 0x132, 0x142, 0x152,
                                0x162, 0x172, 0x182, 0x192, 0x1a2, 0x1b2,
                                0x1c2, 0x1d2, 0x1e2, 0x1f2};
  uint16_t spriteMapS2_4[16] = {0x103, 0x113, 0x123, 0x133, 0x143, 0x153,
                                0x163, 0x173, 0x183, 0x193, 0x1a3, 0x1b3,
                                0x1c3, 0x1d3, 0x1e3, 0x1f3};
  uint16_t spriteMapS2_5[16] = {0x104, 0x114, 0x124, 0x134, 0x144, 0x154,
                                0x164, 0x174, 0x184, 0x194, 0x1a4, 0x1b4,
                                0x1c4, 0x1d4, 0x1e4, 0x1f4};
  uint16_t spriteMapS2_6[16] = {0x105, 0x115, 0x125, 0x135, 0x145, 0x155,
                                0x165, 0x175, 0x185, 0x195, 0x1a5, 0x1b5,
                                0x1c5, 0x1d5, 0x1e5, 0x1f5};
  uint16_t spriteMapS2_7[16] = {0x106, 0x116, 0x126, 0x136, 0x146, 0x156,
                                0x166, 0x176, 0x186, 0x196, 0x1a6, 0x1b6,
                                0x1c6, 0x1d6, 0x1e6, 0x1f6};
  uint16_t spriteMapS2_8[16] = {0x107, 0x117, 0x127, 0x137, 0x147, 0x157,
                                0x167, 0x177, 0x187, 0x197, 0x1a7, 0x1b7,
                                0x1c7, 0x1d7, 0x1e7, 0x1f7};
  uint16_t spriteMapS2_9[16] = {0x108, 0x118, 0x128, 0x138, 0x148, 0x158,
                                0x168, 0x178, 0x188, 0x198, 0x1a8, 0x1b8,
                                0x1c8, 0x1d8, 0x1e8, 0x1f8};
  uint16_t spriteMapS2_10[16] = {0x109, 0x119, 0x129, 0x139, 0x149, 0x159,
                                 0x169, 0x179, 0x189, 0x199, 0x1a9, 0x1b9,
                                 0x1c9, 0x1d9, 0x1e9, 0x1f9};
  uint16_t spriteMapS2_11[16] = {0x10a, 0x11a, 0x12a, 0x13a, 0x14a, 0x15a,
                                 0x16a, 0x17a, 0x18a, 0x19a, 0x1aa, 0x1ba,
                                 0x1ca, 0x1da, 0x1ea, 0x1fa};
  uint16_t spriteMapS2_12[16] = {0x10b, 0x11b, 0x12b, 0x13b, 0x14b, 0x15b,
                                 0x16b, 0x17b, 0x18b, 0x19b, 0x1ab, 0x1bb,
                                 0x1cb, 0x1db, 0x1eb, 0x1fb};
  uint16_t spriteMapS2_13[16] = {0x10c, 0x11c, 0x12c, 0x13c, 0x14c, 0x15c,
                                 0x16c, 0x17c, 0x18c, 0x19c, 0x1ac, 0x1bc,
                                 0x1cc, 0x1dc, 0x1ec, 0x1fc};
  uint16_t spriteMapS2_14[16] = {0x10d, 0x11d, 0x12d, 0x13d, 0x14d, 0x15d,
                                 0x16d, 0x17d, 0x18d, 0x19d, 0x1ad, 0x1bd,
                                 0x1cd, 0x1dd, 0x1ed, 0x1fd};
  uint16_t spriteMapS2_15[16] = {0x10e, 0x11e, 0x12e, 0x13e, 0x14e, 0x15e,
                                 0x16e, 0x17e, 0x18e, 0x19e, 0x1ae, 0x1be,
                                 0x1ce, 0x1de, 0x1ee, 0x1fe};
  uint16_t spriteMapS2_16[16] = {0x10f, 0x11f, 0x12f, 0x13f, 0x14f, 0x15f,
                                 0x16f, 0x17f, 0x18f, 0x19f, 0x1af, 0x1bf,
                                 0x1cf, 0x1df, 0x1ef, 0x1ff};
  load_palettes(pal2, PALETTES + PALOFFSET * 3);
  uint16_t SCB1_2common = setSCB1_2(3, 0, 0, 0, 0, 0);
  uint16_t spal2_1[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal2_2[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal2_3[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal2_4[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal2_5[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal2_6[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal2_7[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal2_8[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal2_9[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal2_10[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal2_11[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal2_12[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal2_13[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal2_14[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal2_15[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal2_16[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 0, min_crt_sz);
  SCB4 = setSCB4(x0);
  setBACKDROP(backdrop);
  vram_sprite(64 * 0, 1, 0, spriteMapS2_1, spal2_1, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 1);
  setBACKDROP(backdrop);
  vram_sprite(64 * 1, 1, 1, spriteMapS2_2, spal2_2, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 2);
  setBACKDROP(backdrop);
  vram_sprite(64 * 2, 1, 2, spriteMapS2_3, spal2_3, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 3);
  setBACKDROP(backdrop);
  vram_sprite(64 * 3, 1, 3, spriteMapS2_4, spal2_4, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 4);
  setBACKDROP(backdrop);
  vram_sprite(64 * 4, 1, 4, spriteMapS2_5, spal2_5, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 5);
  setBACKDROP(backdrop);
  vram_sprite(64 * 5, 1, 5, spriteMapS2_6, spal2_6, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 6);
  setBACKDROP(backdrop);
  vram_sprite(64 * 6, 1, 6, spriteMapS2_7, spal2_7, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 7);
  setBACKDROP(backdrop);
  vram_sprite(64 * 7, 1, 7, spriteMapS2_8, spal2_8, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 8);
  setBACKDROP(backdrop);
  vram_sprite(64 * 8, 1, 8, spriteMapS2_9, spal2_9, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 9);
  setBACKDROP(backdrop);
  vram_sprite(64 * 9, 1, 9, spriteMapS2_10, spal2_10, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 10);
  setBACKDROP(backdrop);
  vram_sprite(64 * 10, 1, 10, spriteMapS2_11, spal2_11, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 11);
  setBACKDROP(backdrop);
  vram_sprite(64 * 11, 1, 11, spriteMapS2_12, spal2_12, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 12);
  setBACKDROP(backdrop);
  vram_sprite(64 * 12, 1, 12, spriteMapS2_13, spal2_13, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 13);
  setBACKDROP(backdrop);
  vram_sprite(64 * 13, 1, 13, spriteMapS2_14, spal2_14, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 14);
  setBACKDROP(backdrop);
  vram_sprite(64 * 14, 1, 14, spriteMapS2_15, spal2_15, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 15);
  setBACKDROP(backdrop);
  vram_sprite(64 * 15, 1, 15, spriteMapS2_16, spal2_16, 16, SCB2, SCB3, SCB4);
}

void NEOGEO_USER showScreen3(int x0, int y0, int xr, int yr, int min_crt_sz,
                             uint16_t backdrop) {
  /****************************************** screen 3
   * ******************************************/
  uint16_t SCB2 = 0x0;
  uint16_t SCB3 = 0x0;
  uint16_t SCB4 = 0x0;
  uint16_t pal3[16];
  setpal(pal3, 0xFFF, 0xFFF, 0x0, 0x4407, 0x7101, 0x4900, 0x5212, 0x390c,
         0x5f0f, 0x2525, 0x2b22, 0x5747, 0x7d45, 0xa5a, 0x5e69, 0x0);
  uint16_t spriteMapS3_1[16] = {0x200, 0x210, 0x220, 0x230, 0x240, 0x250,
                                0x260, 0x270, 0x280, 0x290, 0x2a0, 0x2b0,
                                0x2c0, 0x2d0, 0x2e0, 0x2f0};
  uint16_t spriteMapS3_2[16] = {0x201, 0x211, 0x221, 0x231, 0x241, 0x251,
                                0x261, 0x271, 0x281, 0x291, 0x2a1, 0x2b1,
                                0x2c1, 0x2d1, 0x2e1, 0x2f1};
  uint16_t spriteMapS3_3[16] = {0x202, 0x212, 0x222, 0x232, 0x242, 0x252,
                                0x262, 0x272, 0x282, 0x292, 0x2a2, 0x2b2,
                                0x2c2, 0x2d2, 0x2e2, 0x2f2};
  uint16_t spriteMapS3_4[16] = {0x203, 0x213, 0x223, 0x233, 0x243, 0x253,
                                0x263, 0x273, 0x283, 0x293, 0x2a3, 0x2b3,
                                0x2c3, 0x2d3, 0x2e3, 0x2f3};
  uint16_t spriteMapS3_5[16] = {0x204, 0x214, 0x224, 0x234, 0x244, 0x254,
                                0x264, 0x274, 0x284, 0x294, 0x2a4, 0x2b4,
                                0x2c4, 0x2d4, 0x2e4, 0x2f4};
  uint16_t spriteMapS3_6[16] = {0x205, 0x215, 0x225, 0x235, 0x245, 0x255,
                                0x265, 0x275, 0x285, 0x295, 0x2a5, 0x2b5,
                                0x2c5, 0x2d5, 0x2e5, 0x2f5};
  uint16_t spriteMapS3_7[16] = {0x206, 0x216, 0x226, 0x236, 0x246, 0x256,
                                0x266, 0x276, 0x286, 0x296, 0x2a6, 0x2b6,
                                0x2c6, 0x2d6, 0x2e6, 0x2f6};
  uint16_t spriteMapS3_8[16] = {0x207, 0x217, 0x227, 0x237, 0x247, 0x257,
                                0x267, 0x277, 0x287, 0x297, 0x2a7, 0x2b7,
                                0x2c7, 0x2d7, 0x2e7, 0x2f7};
  uint16_t spriteMapS3_9[16] = {0x208, 0x218, 0x228, 0x238, 0x248, 0x258,
                                0x268, 0x278, 0x288, 0x298, 0x2a8, 0x2b8,
                                0x2c8, 0x2d8, 0x2e8, 0x2f8};
  uint16_t spriteMapS3_10[16] = {0x209, 0x219, 0x229, 0x239, 0x249, 0x259,
                                 0x269, 0x279, 0x289, 0x299, 0x2a9, 0x2b9,
                                 0x2c9, 0x2d9, 0x2e9, 0x2f9};
  uint16_t spriteMapS3_11[16] = {0x20a, 0x21a, 0x22a, 0x23a, 0x24a, 0x25a,
                                 0x26a, 0x27a, 0x28a, 0x29a, 0x2aa, 0x2ba,
                                 0x2ca, 0x2da, 0x2ea, 0x2fa};
  uint16_t spriteMapS3_12[16] = {0x20b, 0x21b, 0x22b, 0x23b, 0x24b, 0x25b,
                                 0x26b, 0x27b, 0x28b, 0x29b, 0x2ab, 0x2bb,
                                 0x2cb, 0x2db, 0x2eb, 0x2fb};
  uint16_t spriteMapS3_13[16] = {0x20c, 0x21c, 0x22c, 0x23c, 0x24c, 0x25c,
                                 0x26c, 0x27c, 0x28c, 0x29c, 0x2ac, 0x2bc,
                                 0x2cc, 0x2dc, 0x2ec, 0x2fc};
  uint16_t spriteMapS3_14[16] = {0x20d, 0x21d, 0x22d, 0x23d, 0x24d, 0x25d,
                                 0x26d, 0x27d, 0x28d, 0x29d, 0x2ad, 0x2bd,
                                 0x2cd, 0x2dd, 0x2ed, 0x2fd};
  uint16_t spriteMapS3_15[16] = {0x20e, 0x21e, 0x22e, 0x23e, 0x24e, 0x25e,
                                 0x26e, 0x27e, 0x28e, 0x29e, 0x2ae, 0x2be,
                                 0x2ce, 0x2de, 0x2ee, 0x2fe};
  uint16_t spriteMapS3_16[16] = {0x20f, 0x21f, 0x22f, 0x23f, 0x24f, 0x25f,
                                 0x26f, 0x27f, 0x28f, 0x29f, 0x2af, 0x2bf,
                                 0x2cf, 0x2df, 0x2ef, 0x2ff};
  load_palettes(pal3, PALETTES + PALOFFSET * 4);
  uint16_t SCB1_2common = setSCB1_2(4, 0, 0, 0, 0, 0);
  uint16_t spal3_1[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal3_2[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal3_3[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal3_4[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal3_5[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal3_6[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal3_7[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal3_8[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal3_9[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal3_10[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal3_11[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal3_12[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal3_13[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal3_14[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal3_15[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal3_16[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 0, min_crt_sz);
  SCB4 = setSCB4(x0);
  setBACKDROP(backdrop);
  vram_sprite(64 * 0, 1, 0, spriteMapS3_1, spal3_1, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 1);
  setBACKDROP(backdrop);
  vram_sprite(64 * 1, 1, 1, spriteMapS3_2, spal3_2, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 2);
  setBACKDROP(backdrop);
  vram_sprite(64 * 2, 1, 2, spriteMapS3_3, spal3_3, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 3);
  setBACKDROP(backdrop);
  vram_sprite(64 * 3, 1, 3, spriteMapS3_4, spal3_4, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 4);
  setBACKDROP(backdrop);
  vram_sprite(64 * 4, 1, 4, spriteMapS3_5, spal3_5, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 5);
  setBACKDROP(backdrop);
  vram_sprite(64 * 5, 1, 5, spriteMapS3_6, spal3_6, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 6);
  setBACKDROP(backdrop);
  vram_sprite(64 * 6, 1, 6, spriteMapS3_7, spal3_7, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 7);
  setBACKDROP(backdrop);
  vram_sprite(64 * 7, 1, 7, spriteMapS3_8, spal3_8, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 8);
  setBACKDROP(backdrop);
  vram_sprite(64 * 8, 1, 8, spriteMapS3_9, spal3_9, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 9);
  setBACKDROP(backdrop);
  vram_sprite(64 * 9, 1, 9, spriteMapS3_10, spal3_10, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 10);
  setBACKDROP(backdrop);
  vram_sprite(64 * 10, 1, 10, spriteMapS3_11, spal3_11, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 11);
  setBACKDROP(backdrop);
  vram_sprite(64 * 11, 1, 11, spriteMapS3_12, spal3_12, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 12);
  setBACKDROP(backdrop);
  vram_sprite(64 * 12, 1, 12, spriteMapS3_13, spal3_13, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 13);
  setBACKDROP(backdrop);
  vram_sprite(64 * 13, 1, 13, spriteMapS3_14, spal3_14, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 14);
  setBACKDROP(backdrop);
  vram_sprite(64 * 14, 1, 14, spriteMapS3_15, spal3_15, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 15);
  setBACKDROP(backdrop);
  vram_sprite(64 * 15, 1, 15, spriteMapS3_16, spal3_16, 16, SCB2, SCB3, SCB4);
}

void NEOGEO_USER showScreen4(int x0, int y0, int xr, int yr, int min_crt_sz,
                             uint16_t backdrop) {
  /****************************************** screen 4
   * ******************************************/
  uint16_t SCB2 = 0x0;
  uint16_t SCB3 = 0x0;
  uint16_t SCB4 = 0x0;
  uint16_t pal4[16];
  setpal(pal4, 0xFFF, 0xFFF, 0x0, 0x4407, 0x7101, 0x5212, 0x2313, 0x390c,
         0x7525, 0x1b21, 0x848, 0x7949, 0x7d45, 0x5c6c, 0x5e69, 0x0);
  uint16_t spriteMapS4_1[16] = {0x300, 0x310, 0x320, 0x330, 0x340, 0x350,
                                0x360, 0x370, 0x380, 0x390, 0x3a0, 0x3b0,
                                0x3c0, 0x3d0, 0x3e0, 0x3f0};
  uint16_t spriteMapS4_2[16] = {0x301, 0x311, 0x321, 0x331, 0x341, 0x351,
                                0x361, 0x371, 0x381, 0x391, 0x3a1, 0x3b1,
                                0x3c1, 0x3d1, 0x3e1, 0x3f1};
  uint16_t spriteMapS4_3[16] = {0x302, 0x312, 0x322, 0x332, 0x342, 0x352,
                                0x362, 0x372, 0x382, 0x392, 0x3a2, 0x3b2,
                                0x3c2, 0x3d2, 0x3e2, 0x3f2};
  uint16_t spriteMapS4_4[16] = {0x303, 0x313, 0x323, 0x333, 0x343, 0x353,
                                0x363, 0x373, 0x383, 0x393, 0x3a3, 0x3b3,
                                0x3c3, 0x3d3, 0x3e3, 0x3f3};
  uint16_t spriteMapS4_5[16] = {0x304, 0x314, 0x324, 0x334, 0x344, 0x354,
                                0x364, 0x374, 0x384, 0x394, 0x3a4, 0x3b4,
                                0x3c4, 0x3d4, 0x3e4, 0x3f4};
  uint16_t spriteMapS4_6[16] = {0x305, 0x315, 0x325, 0x335, 0x345, 0x355,
                                0x365, 0x375, 0x385, 0x395, 0x3a5, 0x3b5,
                                0x3c5, 0x3d5, 0x3e5, 0x3f5};
  uint16_t spriteMapS4_7[16] = {0x306, 0x316, 0x326, 0x336, 0x346, 0x356,
                                0x366, 0x376, 0x386, 0x396, 0x3a6, 0x3b6,
                                0x3c6, 0x3d6, 0x3e6, 0x3f6};
  uint16_t spriteMapS4_8[16] = {0x307, 0x317, 0x327, 0x337, 0x347, 0x357,
                                0x367, 0x377, 0x387, 0x397, 0x3a7, 0x3b7,
                                0x3c7, 0x3d7, 0x3e7, 0x3f7};
  uint16_t spriteMapS4_9[16] = {0x308, 0x318, 0x328, 0x338, 0x348, 0x358,
                                0x368, 0x378, 0x388, 0x398, 0x3a8, 0x3b8,
                                0x3c8, 0x3d8, 0x3e8, 0x3f8};
  uint16_t spriteMapS4_10[16] = {0x309, 0x319, 0x329, 0x339, 0x349, 0x359,
                                 0x369, 0x379, 0x389, 0x399, 0x3a9, 0x3b9,
                                 0x3c9, 0x3d9, 0x3e9, 0x3f9};
  uint16_t spriteMapS4_11[16] = {0x30a, 0x31a, 0x32a, 0x33a, 0x34a, 0x35a,
                                 0x36a, 0x37a, 0x38a, 0x39a, 0x3aa, 0x3ba,
                                 0x3ca, 0x3da, 0x3ea, 0x3fa};
  uint16_t spriteMapS4_12[16] = {0x30b, 0x31b, 0x32b, 0x33b, 0x34b, 0x35b,
                                 0x36b, 0x37b, 0x38b, 0x39b, 0x3ab, 0x3bb,
                                 0x3cb, 0x3db, 0x3eb, 0x3fb};
  uint16_t spriteMapS4_13[16] = {0x30c, 0x31c, 0x32c, 0x33c, 0x34c, 0x35c,
                                 0x36c, 0x37c, 0x38c, 0x39c, 0x3ac, 0x3bc,
                                 0x3cc, 0x3dc, 0x3ec, 0x3fc};
  uint16_t spriteMapS4_14[16] = {0x30d, 0x31d, 0x32d, 0x33d, 0x34d, 0x35d,
                                 0x36d, 0x37d, 0x38d, 0x39d, 0x3ad, 0x3bd,
                                 0x3cd, 0x3dd, 0x3ed, 0x3fd};
  uint16_t spriteMapS4_15[16] = {0x30e, 0x31e, 0x32e, 0x33e, 0x34e, 0x35e,
                                 0x36e, 0x37e, 0x38e, 0x39e, 0x3ae, 0x3be,
                                 0x3ce, 0x3de, 0x3ee, 0x3fe};
  uint16_t spriteMapS4_16[16] = {0x30f, 0x31f, 0x32f, 0x33f, 0x34f, 0x35f,
                                 0x36f, 0x37f, 0x38f, 0x39f, 0x3af, 0x3bf,
                                 0x3cf, 0x3df, 0x3ef, 0x3ff};
  load_palettes(pal4, PALETTES + PALOFFSET * 5);
  uint16_t SCB1_2common = setSCB1_2(5, 0, 0, 0, 0, 0);
  uint16_t spal4_1[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal4_2[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal4_3[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal4_4[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal4_5[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal4_6[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal4_7[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal4_8[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal4_9[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal4_10[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal4_11[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal4_12[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal4_13[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal4_14[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal4_15[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal4_16[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 0, min_crt_sz);
  SCB4 = setSCB4(x0);
  setBACKDROP(backdrop);
  vram_sprite(64 * 0, 1, 0, spriteMapS4_1, spal4_1, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 1);
  setBACKDROP(backdrop);
  vram_sprite(64 * 1, 1, 1, spriteMapS4_2, spal4_2, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 2);
  setBACKDROP(backdrop);
  vram_sprite(64 * 2, 1, 2, spriteMapS4_3, spal4_3, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 3);
  setBACKDROP(backdrop);
  vram_sprite(64 * 3, 1, 3, spriteMapS4_4, spal4_4, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 4);
  setBACKDROP(backdrop);
  vram_sprite(64 * 4, 1, 4, spriteMapS4_5, spal4_5, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 5);
  setBACKDROP(backdrop);
  vram_sprite(64 * 5, 1, 5, spriteMapS4_6, spal4_6, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 6);
  setBACKDROP(backdrop);
  vram_sprite(64 * 6, 1, 6, spriteMapS4_7, spal4_7, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 7);
  setBACKDROP(backdrop);
  vram_sprite(64 * 7, 1, 7, spriteMapS4_8, spal4_8, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 8);
  setBACKDROP(backdrop);
  vram_sprite(64 * 8, 1, 8, spriteMapS4_9, spal4_9, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 9);
  setBACKDROP(backdrop);
  vram_sprite(64 * 9, 1, 9, spriteMapS4_10, spal4_10, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 10);
  setBACKDROP(backdrop);
  vram_sprite(64 * 10, 1, 10, spriteMapS4_11, spal4_11, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 11);
  setBACKDROP(backdrop);
  vram_sprite(64 * 11, 1, 11, spriteMapS4_12, spal4_12, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 12);
  setBACKDROP(backdrop);
  vram_sprite(64 * 12, 1, 12, spriteMapS4_13, spal4_13, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 13);
  setBACKDROP(backdrop);
  vram_sprite(64 * 13, 1, 13, spriteMapS4_14, spal4_14, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 14);
  setBACKDROP(backdrop);
  vram_sprite(64 * 14, 1, 14, spriteMapS4_15, spal4_15, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 15);
  setBACKDROP(backdrop);
  vram_sprite(64 * 15, 1, 15, spriteMapS4_16, spal4_16, 16, SCB2, SCB3, SCB4);
}

void NEOGEO_USER showScreen5(int x0, int y0, int xr, int yr, int min_crt_sz,
                             uint16_t backdrop) {
  /****************************************** screen 5
   * ******************************************/
  uint16_t SCB2 = 0x0;
  uint16_t SCB3 = 0x0;
  uint16_t SCB4 = 0x0;
  uint16_t pal5[16];
  setpal(pal5, 0xFFF, 0xFFF, 0x0, 0x406, 0x7101, 0x609, 0x1212, 0x2313, 0x390c,
         0x7525, 0x1b21, 0x2949, 0x7d45, 0x5c6c, 0x5e69, 0x0);
  uint16_t spriteMapS5_1[16] = {0x400, 0x410, 0x420, 0x430, 0x440, 0x450,
                                0x460, 0x470, 0x480, 0x490, 0x4a0, 0x4b0,
                                0x4c0, 0x4d0, 0x4e0, 0x4f0};
  uint16_t spriteMapS5_2[16] = {0x401, 0x411, 0x421, 0x431, 0x441, 0x451,
                                0x461, 0x471, 0x481, 0x491, 0x4a1, 0x4b1,
                                0x4c1, 0x4d1, 0x4e1, 0x4f1};
  uint16_t spriteMapS5_3[16] = {0x402, 0x412, 0x422, 0x432, 0x442, 0x452,
                                0x462, 0x472, 0x482, 0x492, 0x4a2, 0x4b2,
                                0x4c2, 0x4d2, 0x4e2, 0x4f2};
  uint16_t spriteMapS5_4[16] = {0x403, 0x413, 0x423, 0x433, 0x443, 0x453,
                                0x463, 0x473, 0x483, 0x493, 0x4a3, 0x4b3,
                                0x4c3, 0x4d3, 0x4e3, 0x4f3};
  uint16_t spriteMapS5_5[16] = {0x404, 0x414, 0x424, 0x434, 0x444, 0x454,
                                0x464, 0x474, 0x484, 0x494, 0x4a4, 0x4b4,
                                0x4c4, 0x4d4, 0x4e4, 0x4f4};
  uint16_t spriteMapS5_6[16] = {0x405, 0x415, 0x425, 0x435, 0x445, 0x455,
                                0x465, 0x475, 0x485, 0x495, 0x4a5, 0x4b5,
                                0x4c5, 0x4d5, 0x4e5, 0x4f5};
  uint16_t spriteMapS5_7[16] = {0x406, 0x416, 0x426, 0x436, 0x446, 0x456,
                                0x466, 0x476, 0x486, 0x496, 0x4a6, 0x4b6,
                                0x4c6, 0x4d6, 0x4e6, 0x4f6};
  uint16_t spriteMapS5_8[16] = {0x407, 0x417, 0x427, 0x437, 0x447, 0x457,
                                0x467, 0x477, 0x487, 0x497, 0x4a7, 0x4b7,
                                0x4c7, 0x4d7, 0x4e7, 0x4f7};
  uint16_t spriteMapS5_9[16] = {0x408, 0x418, 0x428, 0x438, 0x448, 0x458,
                                0x468, 0x478, 0x488, 0x498, 0x4a8, 0x4b8,
                                0x4c8, 0x4d8, 0x4e8, 0x4f8};
  uint16_t spriteMapS5_10[16] = {0x409, 0x419, 0x429, 0x439, 0x449, 0x459,
                                 0x469, 0x479, 0x489, 0x499, 0x4a9, 0x4b9,
                                 0x4c9, 0x4d9, 0x4e9, 0x4f9};
  uint16_t spriteMapS5_11[16] = {0x40a, 0x41a, 0x42a, 0x43a, 0x44a, 0x45a,
                                 0x46a, 0x47a, 0x48a, 0x49a, 0x4aa, 0x4ba,
                                 0x4ca, 0x4da, 0x4ea, 0x4fa};
  uint16_t spriteMapS5_12[16] = {0x40b, 0x41b, 0x42b, 0x43b, 0x44b, 0x45b,
                                 0x46b, 0x47b, 0x48b, 0x49b, 0x4ab, 0x4bb,
                                 0x4cb, 0x4db, 0x4eb, 0x4fb};
  uint16_t spriteMapS5_13[16] = {0x40c, 0x41c, 0x42c, 0x43c, 0x44c, 0x45c,
                                 0x46c, 0x47c, 0x48c, 0x49c, 0x4ac, 0x4bc,
                                 0x4cc, 0x4dc, 0x4ec, 0x4fc};
  uint16_t spriteMapS5_14[16] = {0x40d, 0x41d, 0x42d, 0x43d, 0x44d, 0x45d,
                                 0x46d, 0x47d, 0x48d, 0x49d, 0x4ad, 0x4bd,
                                 0x4cd, 0x4dd, 0x4ed, 0x4fd};
  uint16_t spriteMapS5_15[16] = {0x40e, 0x41e, 0x42e, 0x43e, 0x44e, 0x45e,
                                 0x46e, 0x47e, 0x48e, 0x49e, 0x4ae, 0x4be,
                                 0x4ce, 0x4de, 0x4ee, 0x4fe};
  uint16_t spriteMapS5_16[16] = {0x40f, 0x41f, 0x42f, 0x43f, 0x44f, 0x45f,
                                 0x46f, 0x47f, 0x48f, 0x49f, 0x4af, 0x4bf,
                                 0x4cf, 0x4df, 0x4ef, 0x4ff};
  load_palettes(pal5, PALETTES + PALOFFSET * 6);
  uint16_t SCB1_2common = setSCB1_2(6, 0, 0, 0, 0, 0);
  uint16_t spal5_1[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal5_2[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal5_3[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal5_4[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal5_5[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal5_6[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal5_7[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal5_8[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal5_9[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal5_10[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal5_11[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal5_12[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal5_13[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal5_14[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal5_15[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal5_16[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 0, min_crt_sz);
  SCB4 = setSCB4(x0);
  setBACKDROP(backdrop);
  vram_sprite(64 * 0, 1, 0, spriteMapS5_1, spal5_1, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 1);
  setBACKDROP(backdrop);
  vram_sprite(64 * 1, 1, 1, spriteMapS5_2, spal5_2, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 2);
  setBACKDROP(backdrop);
  vram_sprite(64 * 2, 1, 2, spriteMapS5_3, spal5_3, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 3);
  setBACKDROP(backdrop);
  vram_sprite(64 * 3, 1, 3, spriteMapS5_4, spal5_4, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 4);
  setBACKDROP(backdrop);
  vram_sprite(64 * 4, 1, 4, spriteMapS5_5, spal5_5, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 5);
  setBACKDROP(backdrop);
  vram_sprite(64 * 5, 1, 5, spriteMapS5_6, spal5_6, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 6);
  setBACKDROP(backdrop);
  vram_sprite(64 * 6, 1, 6, spriteMapS5_7, spal5_7, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 7);
  setBACKDROP(backdrop);
  vram_sprite(64 * 7, 1, 7, spriteMapS5_8, spal5_8, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 8);
  setBACKDROP(backdrop);
  vram_sprite(64 * 8, 1, 8, spriteMapS5_9, spal5_9, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 9);
  setBACKDROP(backdrop);
  vram_sprite(64 * 9, 1, 9, spriteMapS5_10, spal5_10, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 10);
  setBACKDROP(backdrop);
  vram_sprite(64 * 10, 1, 10, spriteMapS5_11, spal5_11, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 11);
  setBACKDROP(backdrop);
  vram_sprite(64 * 11, 1, 11, spriteMapS5_12, spal5_12, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 12);
  setBACKDROP(backdrop);
  vram_sprite(64 * 12, 1, 12, spriteMapS5_13, spal5_13, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 13);
  setBACKDROP(backdrop);
  vram_sprite(64 * 13, 1, 13, spriteMapS5_14, spal5_14, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 14);
  setBACKDROP(backdrop);
  vram_sprite(64 * 14, 1, 14, spriteMapS5_15, spal5_15, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 15);
  setBACKDROP(backdrop);
  vram_sprite(64 * 15, 1, 15, spriteMapS5_16, spal5_16, 16, SCB2, SCB3, SCB4);
}

void NEOGEO_USER showScreen6(int x0, int y0, int xr, int yr, int min_crt_sz,
                             uint16_t backdrop) {
  /****************************************** screen 6
   * ******************************************/
  uint16_t SCB2 = 0x0;
  uint16_t SCB3 = 0x0;
  uint16_t SCB4 = 0x0;
  uint16_t pal6[16];
  setpal(pal6, 0xFFF, 0xFFF, 0x0, 0x4407, 0x7101, 0x5212, 0x2313, 0x390c,
         0x2414, 0x6526, 0x1b21, 0x7949, 0x7d45, 0x5c6c, 0x5e69, 0x0);
  uint16_t spriteMapS6_1[16] = {0x500, 0x510, 0x520, 0x530, 0x540, 0x550,
                                0x560, 0x570, 0x580, 0x590, 0x5a0, 0x5b0,
                                0x5c0, 0x5d0, 0x5e0, 0x5f0};
  uint16_t spriteMapS6_2[16] = {0x501, 0x511, 0x521, 0x531, 0x541, 0x551,
                                0x561, 0x571, 0x581, 0x591, 0x5a1, 0x5b1,
                                0x5c1, 0x5d1, 0x5e1, 0x5f1};
  uint16_t spriteMapS6_3[16] = {0x502, 0x512, 0x522, 0x532, 0x542, 0x552,
                                0x562, 0x572, 0x582, 0x592, 0x5a2, 0x5b2,
                                0x5c2, 0x5d2, 0x5e2, 0x5f2};
  uint16_t spriteMapS6_4[16] = {0x503, 0x513, 0x523, 0x533, 0x543, 0x553,
                                0x563, 0x573, 0x583, 0x593, 0x5a3, 0x5b3,
                                0x5c3, 0x5d3, 0x5e3, 0x5f3};
  uint16_t spriteMapS6_5[16] = {0x504, 0x514, 0x524, 0x534, 0x544, 0x554,
                                0x564, 0x574, 0x584, 0x594, 0x5a4, 0x5b4,
                                0x5c4, 0x5d4, 0x5e4, 0x5f4};
  uint16_t spriteMapS6_6[16] = {0x505, 0x515, 0x525, 0x535, 0x545, 0x555,
                                0x565, 0x575, 0x585, 0x595, 0x5a5, 0x5b5,
                                0x5c5, 0x5d5, 0x5e5, 0x5f5};
  uint16_t spriteMapS6_7[16] = {0x506, 0x516, 0x526, 0x536, 0x546, 0x556,
                                0x566, 0x576, 0x586, 0x596, 0x5a6, 0x5b6,
                                0x5c6, 0x5d6, 0x5e6, 0x5f6};
  uint16_t spriteMapS6_8[16] = {0x507, 0x517, 0x527, 0x537, 0x547, 0x557,
                                0x567, 0x577, 0x587, 0x597, 0x5a7, 0x5b7,
                                0x5c7, 0x5d7, 0x5e7, 0x5f7};
  uint16_t spriteMapS6_9[16] = {0x508, 0x518, 0x528, 0x538, 0x548, 0x558,
                                0x568, 0x578, 0x588, 0x598, 0x5a8, 0x5b8,
                                0x5c8, 0x5d8, 0x5e8, 0x5f8};
  uint16_t spriteMapS6_10[16] = {0x509, 0x519, 0x529, 0x539, 0x549, 0x559,
                                 0x569, 0x579, 0x589, 0x599, 0x5a9, 0x5b9,
                                 0x5c9, 0x5d9, 0x5e9, 0x5f9};
  uint16_t spriteMapS6_11[16] = {0x50a, 0x51a, 0x52a, 0x53a, 0x54a, 0x55a,
                                 0x56a, 0x57a, 0x58a, 0x59a, 0x5aa, 0x5ba,
                                 0x5ca, 0x5da, 0x5ea, 0x5fa};
  uint16_t spriteMapS6_12[16] = {0x50b, 0x51b, 0x52b, 0x53b, 0x54b, 0x55b,
                                 0x56b, 0x57b, 0x58b, 0x59b, 0x5ab, 0x5bb,
                                 0x5cb, 0x5db, 0x5eb, 0x5fb};
  uint16_t spriteMapS6_13[16] = {0x50c, 0x51c, 0x52c, 0x53c, 0x54c, 0x55c,
                                 0x56c, 0x57c, 0x58c, 0x59c, 0x5ac, 0x5bc,
                                 0x5cc, 0x5dc, 0x5ec, 0x5fc};
  uint16_t spriteMapS6_14[16] = {0x50d, 0x51d, 0x52d, 0x53d, 0x54d, 0x55d,
                                 0x56d, 0x57d, 0x58d, 0x59d, 0x5ad, 0x5bd,
                                 0x5cd, 0x5dd, 0x5ed, 0x5fd};
  uint16_t spriteMapS6_15[16] = {0x50e, 0x51e, 0x52e, 0x53e, 0x54e, 0x55e,
                                 0x56e, 0x57e, 0x58e, 0x59e, 0x5ae, 0x5be,
                                 0x5ce, 0x5de, 0x5ee, 0x5fe};
  uint16_t spriteMapS6_16[16] = {0x50f, 0x51f, 0x52f, 0x53f, 0x54f, 0x55f,
                                 0x56f, 0x57f, 0x58f, 0x59f, 0x5af, 0x5bf,
                                 0x5cf, 0x5df, 0x5ef, 0x5ff};
  load_palettes(pal6, PALETTES + PALOFFSET * 7);
  uint16_t SCB1_2common = setSCB1_2(7, 0, 0, 0, 0, 0);
  uint16_t spal6_1[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal6_2[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal6_3[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal6_4[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal6_5[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal6_6[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal6_7[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal6_8[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal6_9[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal6_10[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal6_11[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal6_12[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal6_13[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal6_14[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal6_15[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal6_16[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 0, min_crt_sz);
  SCB4 = setSCB4(x0);
  setBACKDROP(backdrop);
  vram_sprite(64 * 0, 1, 0, spriteMapS6_1, spal6_1, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 1);
  setBACKDROP(backdrop);
  vram_sprite(64 * 1, 1, 1, spriteMapS6_2, spal6_2, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 2);
  setBACKDROP(backdrop);
  vram_sprite(64 * 2, 1, 2, spriteMapS6_3, spal6_3, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 3);
  setBACKDROP(backdrop);
  vram_sprite(64 * 3, 1, 3, spriteMapS6_4, spal6_4, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 4);
  setBACKDROP(backdrop);
  vram_sprite(64 * 4, 1, 4, spriteMapS6_5, spal6_5, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 5);
  setBACKDROP(backdrop);
  vram_sprite(64 * 5, 1, 5, spriteMapS6_6, spal6_6, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 6);
  setBACKDROP(backdrop);
  vram_sprite(64 * 6, 1, 6, spriteMapS6_7, spal6_7, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 7);
  setBACKDROP(backdrop);
  vram_sprite(64 * 7, 1, 7, spriteMapS6_8, spal6_8, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 8);
  setBACKDROP(backdrop);
  vram_sprite(64 * 8, 1, 8, spriteMapS6_9, spal6_9, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 9);
  setBACKDROP(backdrop);
  vram_sprite(64 * 9, 1, 9, spriteMapS6_10, spal6_10, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 10);
  setBACKDROP(backdrop);
  vram_sprite(64 * 10, 1, 10, spriteMapS6_11, spal6_11, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 11);
  setBACKDROP(backdrop);
  vram_sprite(64 * 11, 1, 11, spriteMapS6_12, spal6_12, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 12);
  setBACKDROP(backdrop);
  vram_sprite(64 * 12, 1, 12, spriteMapS6_13, spal6_13, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 13);
  setBACKDROP(backdrop);
  vram_sprite(64 * 13, 1, 13, spriteMapS6_14, spal6_14, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 14);
  setBACKDROP(backdrop);
  vram_sprite(64 * 14, 1, 14, spriteMapS6_15, spal6_15, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 15);
  setBACKDROP(backdrop);
  vram_sprite(64 * 15, 1, 15, spriteMapS6_16, spal6_16, 16, SCB2, SCB3, SCB4);
}

void NEOGEO_USER showScreen7(int x0, int y0, int xr, int yr, int min_crt_sz,
                             uint16_t backdrop) {
  /****************************************** screen 7
   * ******************************************/
  uint16_t SCB2 = 0x0;
  uint16_t SCB3 = 0x0;
  uint16_t SCB4 = 0x0;
  uint16_t pal7[16];
  setpal(pal7, 0xFFF, 0xFFF, 0x0, 0x4407, 0x7101, 0x5212, 0x2313, 0x390c,
         0x2414, 0x2626, 0x1b21, 0x7d45, 0x694a, 0x5c6c, 0x5e69, 0x0);
  uint16_t spriteMapS7_1[16] = {0x600, 0x610, 0x620, 0x630, 0x640, 0x650,
                                0x660, 0x670, 0x680, 0x690, 0x6a0, 0x6b0,
                                0x6c0, 0x6d0, 0x6e0, 0x6f0};
  uint16_t spriteMapS7_2[16] = {0x601, 0x611, 0x621, 0x631, 0x641, 0x651,
                                0x661, 0x671, 0x681, 0x691, 0x6a1, 0x6b1,
                                0x6c1, 0x6d1, 0x6e1, 0x6f1};
  uint16_t spriteMapS7_3[16] = {0x602, 0x612, 0x622, 0x632, 0x642, 0x652,
                                0x662, 0x672, 0x682, 0x692, 0x6a2, 0x6b2,
                                0x6c2, 0x6d2, 0x6e2, 0x6f2};
  uint16_t spriteMapS7_4[16] = {0x603, 0x613, 0x623, 0x633, 0x643, 0x653,
                                0x663, 0x673, 0x683, 0x693, 0x6a3, 0x6b3,
                                0x6c3, 0x6d3, 0x6e3, 0x6f3};
  uint16_t spriteMapS7_5[16] = {0x604, 0x614, 0x624, 0x634, 0x644, 0x654,
                                0x664, 0x674, 0x684, 0x694, 0x6a4, 0x6b4,
                                0x6c4, 0x6d4, 0x6e4, 0x6f4};
  uint16_t spriteMapS7_6[16] = {0x605, 0x615, 0x625, 0x635, 0x645, 0x655,
                                0x665, 0x675, 0x685, 0x695, 0x6a5, 0x6b5,
                                0x6c5, 0x6d5, 0x6e5, 0x6f5};
  uint16_t spriteMapS7_7[16] = {0x606, 0x616, 0x626, 0x636, 0x646, 0x656,
                                0x666, 0x676, 0x686, 0x696, 0x6a6, 0x6b6,
                                0x6c6, 0x6d6, 0x6e6, 0x6f6};
  uint16_t spriteMapS7_8[16] = {0x607, 0x617, 0x627, 0x637, 0x647, 0x657,
                                0x667, 0x677, 0x687, 0x697, 0x6a7, 0x6b7,
                                0x6c7, 0x6d7, 0x6e7, 0x6f7};
  uint16_t spriteMapS7_9[16] = {0x608, 0x618, 0x628, 0x638, 0x648, 0x658,
                                0x668, 0x678, 0x688, 0x698, 0x6a8, 0x6b8,
                                0x6c8, 0x6d8, 0x6e8, 0x6f8};
  uint16_t spriteMapS7_10[16] = {0x609, 0x619, 0x629, 0x639, 0x649, 0x659,
                                 0x669, 0x679, 0x689, 0x699, 0x6a9, 0x6b9,
                                 0x6c9, 0x6d9, 0x6e9, 0x6f9};
  uint16_t spriteMapS7_11[16] = {0x60a, 0x61a, 0x62a, 0x63a, 0x64a, 0x65a,
                                 0x66a, 0x67a, 0x68a, 0x69a, 0x6aa, 0x6ba,
                                 0x6ca, 0x6da, 0x6ea, 0x6fa};
  uint16_t spriteMapS7_12[16] = {0x60b, 0x61b, 0x62b, 0x63b, 0x64b, 0x65b,
                                 0x66b, 0x67b, 0x68b, 0x69b, 0x6ab, 0x6bb,
                                 0x6cb, 0x6db, 0x6eb, 0x6fb};
  uint16_t spriteMapS7_13[16] = {0x60c, 0x61c, 0x62c, 0x63c, 0x64c, 0x65c,
                                 0x66c, 0x67c, 0x68c, 0x69c, 0x6ac, 0x6bc,
                                 0x6cc, 0x6dc, 0x6ec, 0x6fc};
  uint16_t spriteMapS7_14[16] = {0x60d, 0x61d, 0x62d, 0x63d, 0x64d, 0x65d,
                                 0x66d, 0x67d, 0x68d, 0x69d, 0x6ad, 0x6bd,
                                 0x6cd, 0x6dd, 0x6ed, 0x6fd};
  uint16_t spriteMapS7_15[16] = {0x60e, 0x61e, 0x62e, 0x63e, 0x64e, 0x65e,
                                 0x66e, 0x67e, 0x68e, 0x69e, 0x6ae, 0x6be,
                                 0x6ce, 0x6de, 0x6ee, 0x6fe};
  uint16_t spriteMapS7_16[16] = {0x60f, 0x61f, 0x62f, 0x63f, 0x64f, 0x65f,
                                 0x66f, 0x67f, 0x68f, 0x69f, 0x6af, 0x6bf,
                                 0x6cf, 0x6df, 0x6ef, 0x6ff};
  load_palettes(pal7, PALETTES + PALOFFSET * 8);
  uint16_t SCB1_2common = setSCB1_2(8, 0, 0, 0, 0, 0);
  uint16_t spal7_1[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal7_2[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal7_3[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal7_4[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal7_5[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal7_6[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal7_7[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal7_8[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal7_9[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal7_10[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal7_11[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal7_12[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal7_13[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal7_14[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal7_15[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal7_16[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 0, min_crt_sz);
  SCB4 = setSCB4(x0);
  setBACKDROP(backdrop);
  vram_sprite(64 * 0, 1, 0, spriteMapS7_1, spal7_1, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 1);
  setBACKDROP(backdrop);
  vram_sprite(64 * 1, 1, 1, spriteMapS7_2, spal7_2, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 2);
  setBACKDROP(backdrop);
  vram_sprite(64 * 2, 1, 2, spriteMapS7_3, spal7_3, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 3);
  setBACKDROP(backdrop);
  vram_sprite(64 * 3, 1, 3, spriteMapS7_4, spal7_4, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 4);
  setBACKDROP(backdrop);
  vram_sprite(64 * 4, 1, 4, spriteMapS7_5, spal7_5, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 5);
  setBACKDROP(backdrop);
  vram_sprite(64 * 5, 1, 5, spriteMapS7_6, spal7_6, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 6);
  setBACKDROP(backdrop);
  vram_sprite(64 * 6, 1, 6, spriteMapS7_7, spal7_7, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 7);
  setBACKDROP(backdrop);
  vram_sprite(64 * 7, 1, 7, spriteMapS7_8, spal7_8, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 8);
  setBACKDROP(backdrop);
  vram_sprite(64 * 8, 1, 8, spriteMapS7_9, spal7_9, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 9);
  setBACKDROP(backdrop);
  vram_sprite(64 * 9, 1, 9, spriteMapS7_10, spal7_10, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 10);
  setBACKDROP(backdrop);
  vram_sprite(64 * 10, 1, 10, spriteMapS7_11, spal7_11, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 11);
  setBACKDROP(backdrop);
  vram_sprite(64 * 11, 1, 11, spriteMapS7_12, spal7_12, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 12);
  setBACKDROP(backdrop);
  vram_sprite(64 * 12, 1, 12, spriteMapS7_13, spal7_13, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 13);
  setBACKDROP(backdrop);
  vram_sprite(64 * 13, 1, 13, spriteMapS7_14, spal7_14, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 14);
  setBACKDROP(backdrop);
  vram_sprite(64 * 14, 1, 14, spriteMapS7_15, spal7_15, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 15);
  setBACKDROP(backdrop);
  vram_sprite(64 * 15, 1, 15, spriteMapS7_16, spal7_16, 16, SCB2, SCB3, SCB4);
}

void NEOGEO_USER showScreen8(int x0, int y0, int xr, int yr, int min_crt_sz,
                             uint16_t backdrop) {
  /****************************************** screen 8
   * ******************************************/
  uint16_t SCB2 = 0x0;
  uint16_t SCB3 = 0x0;
  uint16_t SCB4 = 0x0;
  uint16_t pal8[16];
  setpal(pal8, 0xFFF, 0xFFF, 0x0, 0x2101, 0x4407, 0x7101, 0x5212, 0x390c,
         0x6525, 0x1b21, 0x7737, 0x4d45, 0x7949, 0x5c6c, 0x5e69, 0x0);
  uint16_t spriteMapS8_1[16] = {0x700, 0x710, 0x720, 0x730, 0x740, 0x750,
                                0x760, 0x770, 0x780, 0x790, 0x7a0, 0x7b0,
                                0x7c0, 0x7d0, 0x7e0, 0x7f0};
  uint16_t spriteMapS8_2[16] = {0x701, 0x711, 0x721, 0x731, 0x741, 0x751,
                                0x761, 0x771, 0x781, 0x791, 0x7a1, 0x7b1,
                                0x7c1, 0x7d1, 0x7e1, 0x7f1};
  uint16_t spriteMapS8_3[16] = {0x702, 0x712, 0x722, 0x732, 0x742, 0x752,
                                0x762, 0x772, 0x782, 0x792, 0x7a2, 0x7b2,
                                0x7c2, 0x7d2, 0x7e2, 0x7f2};
  uint16_t spriteMapS8_4[16] = {0x703, 0x713, 0x723, 0x733, 0x743, 0x753,
                                0x763, 0x773, 0x783, 0x793, 0x7a3, 0x7b3,
                                0x7c3, 0x7d3, 0x7e3, 0x7f3};
  uint16_t spriteMapS8_5[16] = {0x704, 0x714, 0x724, 0x734, 0x744, 0x754,
                                0x764, 0x774, 0x784, 0x794, 0x7a4, 0x7b4,
                                0x7c4, 0x7d4, 0x7e4, 0x7f4};
  uint16_t spriteMapS8_6[16] = {0x705, 0x715, 0x725, 0x735, 0x745, 0x755,
                                0x765, 0x775, 0x785, 0x795, 0x7a5, 0x7b5,
                                0x7c5, 0x7d5, 0x7e5, 0x7f5};
  uint16_t spriteMapS8_7[16] = {0x706, 0x716, 0x726, 0x736, 0x746, 0x756,
                                0x766, 0x776, 0x786, 0x796, 0x7a6, 0x7b6,
                                0x7c6, 0x7d6, 0x7e6, 0x7f6};
  uint16_t spriteMapS8_8[16] = {0x707, 0x717, 0x727, 0x737, 0x747, 0x757,
                                0x767, 0x777, 0x787, 0x797, 0x7a7, 0x7b7,
                                0x7c7, 0x7d7, 0x7e7, 0x7f7};
  uint16_t spriteMapS8_9[16] = {0x708, 0x718, 0x728, 0x738, 0x748, 0x758,
                                0x768, 0x778, 0x788, 0x798, 0x7a8, 0x7b8,
                                0x7c8, 0x7d8, 0x7e8, 0x7f8};
  uint16_t spriteMapS8_10[16] = {0x709, 0x719, 0x729, 0x739, 0x749, 0x759,
                                 0x769, 0x779, 0x789, 0x799, 0x7a9, 0x7b9,
                                 0x7c9, 0x7d9, 0x7e9, 0x7f9};
  uint16_t spriteMapS8_11[16] = {0x70a, 0x71a, 0x72a, 0x73a, 0x74a, 0x75a,
                                 0x76a, 0x77a, 0x78a, 0x79a, 0x7aa, 0x7ba,
                                 0x7ca, 0x7da, 0x7ea, 0x7fa};
  uint16_t spriteMapS8_12[16] = {0x70b, 0x71b, 0x72b, 0x73b, 0x74b, 0x75b,
                                 0x76b, 0x77b, 0x78b, 0x79b, 0x7ab, 0x7bb,
                                 0x7cb, 0x7db, 0x7eb, 0x7fb};
  uint16_t spriteMapS8_13[16] = {0x70c, 0x71c, 0x72c, 0x73c, 0x74c, 0x75c,
                                 0x76c, 0x77c, 0x78c, 0x79c, 0x7ac, 0x7bc,
                                 0x7cc, 0x7dc, 0x7ec, 0x7fc};
  uint16_t spriteMapS8_14[16] = {0x70d, 0x71d, 0x72d, 0x73d, 0x74d, 0x75d,
                                 0x76d, 0x77d, 0x78d, 0x79d, 0x7ad, 0x7bd,
                                 0x7cd, 0x7dd, 0x7ed, 0x7fd};
  uint16_t spriteMapS8_15[16] = {0x70e, 0x71e, 0x72e, 0x73e, 0x74e, 0x75e,
                                 0x76e, 0x77e, 0x78e, 0x79e, 0x7ae, 0x7be,
                                 0x7ce, 0x7de, 0x7ee, 0x7fe};
  uint16_t spriteMapS8_16[16] = {0x70f, 0x71f, 0x72f, 0x73f, 0x74f, 0x75f,
                                 0x76f, 0x77f, 0x78f, 0x79f, 0x7af, 0x7bf,
                                 0x7cf, 0x7df, 0x7ef, 0x7ff};
  load_palettes(pal8, PALETTES + PALOFFSET * 9);
  uint16_t SCB1_2common = setSCB1_2(9, 0, 0, 0, 0, 0);
  uint16_t spal8_1[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal8_2[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal8_3[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal8_4[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal8_5[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal8_6[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal8_7[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal8_8[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal8_9[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal8_10[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal8_11[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal8_12[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal8_13[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal8_14[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal8_15[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal8_16[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 0, min_crt_sz);
  SCB4 = setSCB4(x0);
  setBACKDROP(backdrop);
  vram_sprite(64 * 0, 1, 0, spriteMapS8_1, spal8_1, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 1);
  setBACKDROP(backdrop);
  vram_sprite(64 * 1, 1, 1, spriteMapS8_2, spal8_2, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 2);
  setBACKDROP(backdrop);
  vram_sprite(64 * 2, 1, 2, spriteMapS8_3, spal8_3, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 3);
  setBACKDROP(backdrop);
  vram_sprite(64 * 3, 1, 3, spriteMapS8_4, spal8_4, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 4);
  setBACKDROP(backdrop);
  vram_sprite(64 * 4, 1, 4, spriteMapS8_5, spal8_5, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 5);
  setBACKDROP(backdrop);
  vram_sprite(64 * 5, 1, 5, spriteMapS8_6, spal8_6, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 6);
  setBACKDROP(backdrop);
  vram_sprite(64 * 6, 1, 6, spriteMapS8_7, spal8_7, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 7);
  setBACKDROP(backdrop);
  vram_sprite(64 * 7, 1, 7, spriteMapS8_8, spal8_8, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 8);
  setBACKDROP(backdrop);
  vram_sprite(64 * 8, 1, 8, spriteMapS8_9, spal8_9, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 9);
  setBACKDROP(backdrop);
  vram_sprite(64 * 9, 1, 9, spriteMapS8_10, spal8_10, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 10);
  setBACKDROP(backdrop);
  vram_sprite(64 * 10, 1, 10, spriteMapS8_11, spal8_11, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 11);
  setBACKDROP(backdrop);
  vram_sprite(64 * 11, 1, 11, spriteMapS8_12, spal8_12, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 12);
  setBACKDROP(backdrop);
  vram_sprite(64 * 12, 1, 12, spriteMapS8_13, spal8_13, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 13);
  setBACKDROP(backdrop);
  vram_sprite(64 * 13, 1, 13, spriteMapS8_14, spal8_14, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 14);
  setBACKDROP(backdrop);
  vram_sprite(64 * 14, 1, 14, spriteMapS8_15, spal8_15, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 15);
  setBACKDROP(backdrop);
  vram_sprite(64 * 15, 1, 15, spriteMapS8_16, spal8_16, 16, SCB2, SCB3, SCB4);
}

void NEOGEO_USER showScreen9(int x0, int y0, int xr, int yr, int min_crt_sz,
                             uint16_t backdrop) {
  /****************************************** screen 9
   * ******************************************/
  uint16_t SCB2 = 0x0;
  uint16_t SCB3 = 0x0;
  uint16_t SCB4 = 0x0;
  uint16_t pal9[16];
  setpal(pal9, 0xFFF, 0xFFF, 0x0, 0x7101, 0x5212, 0x7313, 0x5424, 0x7525,
         0x5636, 0x7737, 0x5848, 0x7949, 0x7a5a, 0x1c6b, 0x2d6d, 0x0);
  uint16_t spriteMapS9_1[16] = {0x800, 0x810, 0x820, 0x830, 0x840, 0x850,
                                0x860, 0x870, 0x880, 0x890, 0x8a0, 0x8b0,
                                0x8c0, 0x8d0, 0x8e0, 0x8f0};
  uint16_t spriteMapS9_2[16] = {0x801, 0x811, 0x821, 0x831, 0x841, 0x851,
                                0x861, 0x871, 0x881, 0x891, 0x8a1, 0x8b1,
                                0x8c1, 0x8d1, 0x8e1, 0x8f1};
  uint16_t spriteMapS9_3[16] = {0x802, 0x812, 0x822, 0x832, 0x842, 0x852,
                                0x862, 0x872, 0x882, 0x892, 0x8a2, 0x8b2,
                                0x8c2, 0x8d2, 0x8e2, 0x8f2};
  uint16_t spriteMapS9_4[16] = {0x803, 0x813, 0x823, 0x833, 0x843, 0x853,
                                0x863, 0x873, 0x883, 0x893, 0x8a3, 0x8b3,
                                0x8c3, 0x8d3, 0x8e3, 0x8f3};
  uint16_t spriteMapS9_5[16] = {0x804, 0x814, 0x824, 0x834, 0x844, 0x854,
                                0x864, 0x874, 0x884, 0x894, 0x8a4, 0x8b4,
                                0x8c4, 0x8d4, 0x8e4, 0x8f4};
  uint16_t spriteMapS9_6[16] = {0x805, 0x815, 0x825, 0x835, 0x845, 0x855,
                                0x865, 0x875, 0x885, 0x895, 0x8a5, 0x8b5,
                                0x8c5, 0x8d5, 0x8e5, 0x8f5};
  uint16_t spriteMapS9_7[16] = {0x806, 0x816, 0x826, 0x836, 0x846, 0x856,
                                0x866, 0x876, 0x886, 0x896, 0x8a6, 0x8b6,
                                0x8c6, 0x8d6, 0x8e6, 0x8f6};
  uint16_t spriteMapS9_8[16] = {0x807, 0x817, 0x827, 0x837, 0x847, 0x857,
                                0x867, 0x877, 0x887, 0x897, 0x8a7, 0x8b7,
                                0x8c7, 0x8d7, 0x8e7, 0x8f7};
  uint16_t spriteMapS9_9[16] = {0x808, 0x818, 0x828, 0x838, 0x848, 0x858,
                                0x868, 0x878, 0x888, 0x898, 0x8a8, 0x8b8,
                                0x8c8, 0x8d8, 0x8e8, 0x8f8};
  uint16_t spriteMapS9_10[16] = {0x809, 0x819, 0x829, 0x839, 0x849, 0x859,
                                 0x869, 0x879, 0x889, 0x899, 0x8a9, 0x8b9,
                                 0x8c9, 0x8d9, 0x8e9, 0x8f9};
  uint16_t spriteMapS9_11[16] = {0x80a, 0x81a, 0x82a, 0x83a, 0x84a, 0x85a,
                                 0x86a, 0x87a, 0x88a, 0x89a, 0x8aa, 0x8ba,
                                 0x8ca, 0x8da, 0x8ea, 0x8fa};
  uint16_t spriteMapS9_12[16] = {0x80b, 0x81b, 0x82b, 0x83b, 0x84b, 0x85b,
                                 0x86b, 0x87b, 0x88b, 0x89b, 0x8ab, 0x8bb,
                                 0x8cb, 0x8db, 0x8eb, 0x8fb};
  uint16_t spriteMapS9_13[16] = {0x80c, 0x81c, 0x82c, 0x83c, 0x84c, 0x85c,
                                 0x86c, 0x87c, 0x88c, 0x89c, 0x8ac, 0x8bc,
                                 0x8cc, 0x8dc, 0x8ec, 0x8fc};
  uint16_t spriteMapS9_14[16] = {0x80d, 0x81d, 0x82d, 0x83d, 0x84d, 0x85d,
                                 0x86d, 0x87d, 0x88d, 0x89d, 0x8ad, 0x8bd,
                                 0x8cd, 0x8dd, 0x8ed, 0x8fd};
  uint16_t spriteMapS9_15[16] = {0x80e, 0x81e, 0x82e, 0x83e, 0x84e, 0x85e,
                                 0x86e, 0x87e, 0x88e, 0x89e, 0x8ae, 0x8be,
                                 0x8ce, 0x8de, 0x8ee, 0x8fe};
  uint16_t spriteMapS9_16[16] = {0x80f, 0x81f, 0x82f, 0x83f, 0x84f, 0x85f,
                                 0x86f, 0x87f, 0x88f, 0x89f, 0x8af, 0x8bf,
                                 0x8cf, 0x8df, 0x8ef, 0x8ff};
  load_palettes(pal9, PALETTES + PALOFFSET * 10);
  uint16_t SCB1_2common = setSCB1_2(10, 0, 0, 0, 0, 0);
  uint16_t spal9_1[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal9_2[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal9_3[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal9_4[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal9_5[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal9_6[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal9_7[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal9_8[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal9_9[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal9_10[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal9_11[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal9_12[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal9_13[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal9_14[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal9_15[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal9_16[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 0, min_crt_sz);
  SCB4 = setSCB4(x0);
  setBACKDROP(backdrop);
  vram_sprite(64 * 0, 1, 0, spriteMapS9_1, spal9_1, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 1);
  setBACKDROP(backdrop);
  vram_sprite(64 * 1, 1, 1, spriteMapS9_2, spal9_2, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 2);
  setBACKDROP(backdrop);
  vram_sprite(64 * 2, 1, 2, spriteMapS9_3, spal9_3, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 3);
  setBACKDROP(backdrop);
  vram_sprite(64 * 3, 1, 3, spriteMapS9_4, spal9_4, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 4);
  setBACKDROP(backdrop);
  vram_sprite(64 * 4, 1, 4, spriteMapS9_5, spal9_5, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 5);
  setBACKDROP(backdrop);
  vram_sprite(64 * 5, 1, 5, spriteMapS9_6, spal9_6, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 6);
  setBACKDROP(backdrop);
  vram_sprite(64 * 6, 1, 6, spriteMapS9_7, spal9_7, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 7);
  setBACKDROP(backdrop);
  vram_sprite(64 * 7, 1, 7, spriteMapS9_8, spal9_8, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 8);
  setBACKDROP(backdrop);
  vram_sprite(64 * 8, 1, 8, spriteMapS9_9, spal9_9, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 9);
  setBACKDROP(backdrop);
  vram_sprite(64 * 9, 1, 9, spriteMapS9_10, spal9_10, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 10);
  setBACKDROP(backdrop);
  vram_sprite(64 * 10, 1, 10, spriteMapS9_11, spal9_11, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 11);
  setBACKDROP(backdrop);
  vram_sprite(64 * 11, 1, 11, spriteMapS9_12, spal9_12, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 12);
  setBACKDROP(backdrop);
  vram_sprite(64 * 12, 1, 12, spriteMapS9_13, spal9_13, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 13);
  setBACKDROP(backdrop);
  vram_sprite(64 * 13, 1, 13, spriteMapS9_14, spal9_14, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 14);
  setBACKDROP(backdrop);
  vram_sprite(64 * 14, 1, 14, spriteMapS9_15, spal9_15, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 15);
  setBACKDROP(backdrop);
  vram_sprite(64 * 15, 1, 15, spriteMapS9_16, spal9_16, 16, SCB2, SCB3, SCB4);
}

void NEOGEO_USER showScreen10(int x0, int y0, int xr, int yr, int min_crt_sz,
                              uint16_t backdrop) {
  /****************************************** screen 10
   * ******************************************/
  uint16_t SCB2 = 0x0;
  uint16_t SCB3 = 0x0;
  uint16_t SCB4 = 0x0;
  uint16_t pal10[16];
  setpal(pal10, 0xFFF, 0xFFF, 0x213, 0x1413, 0x7823, 0x4d56, 0x5e69, 0x6f73,
         0x4975, 0x6653, 0x3346, 0x5432, 0x522, 0x0, 0x0, 0x0);
  uint16_t spriteMapS10_1[16] = {0x900, 0x910, 0x920, 0x930, 0x940, 0x950,
                                 0x960, 0x970, 0x980, 0x990, 0x9a0, 0x9b0,
                                 0x9c0, 0x9d0, 0x9e0, 0x9f0};
  uint16_t spriteMapS10_2[16] = {0x901, 0x911, 0x921, 0x931, 0x941, 0x951,
                                 0x961, 0x971, 0x981, 0x991, 0x9a1, 0x9b1,
                                 0x9c1, 0x9d1, 0x9e1, 0x9f1};
  uint16_t spriteMapS10_3[16] = {0x902, 0x912, 0x922, 0x932, 0x942, 0x952,
                                 0x962, 0x972, 0x982, 0x992, 0x9a2, 0x9b2,
                                 0x9c2, 0x9d2, 0x9e2, 0x9f2};
  uint16_t spriteMapS10_4[16] = {0x903, 0x913, 0x923, 0x933, 0x943, 0x953,
                                 0x963, 0x973, 0x983, 0x993, 0x9a3, 0x9b3,
                                 0x9c3, 0x9d3, 0x9e3, 0x9f3};
  uint16_t spriteMapS10_5[16] = {0x904, 0x914, 0x924, 0x934, 0x944, 0x954,
                                 0x964, 0x974, 0x984, 0x994, 0x9a4, 0x9b4,
                                 0x9c4, 0x9d4, 0x9e4, 0x9f4};
  uint16_t spriteMapS10_6[16] = {0x905, 0x915, 0x925, 0x935, 0x945, 0x955,
                                 0x965, 0x975, 0x985, 0x995, 0x9a5, 0x9b5,
                                 0x9c5, 0x9d5, 0x9e5, 0x9f5};
  uint16_t spriteMapS10_7[16] = {0x906, 0x916, 0x926, 0x936, 0x946, 0x956,
                                 0x966, 0x976, 0x986, 0x996, 0x9a6, 0x9b6,
                                 0x9c6, 0x9d6, 0x9e6, 0x9f6};
  uint16_t spriteMapS10_8[16] = {0x907, 0x917, 0x927, 0x937, 0x947, 0x957,
                                 0x967, 0x977, 0x987, 0x997, 0x9a7, 0x9b7,
                                 0x9c7, 0x9d7, 0x9e7, 0x9f7};
  uint16_t spriteMapS10_9[16] = {0x908, 0x918, 0x928, 0x938, 0x948, 0x958,
                                 0x968, 0x978, 0x988, 0x998, 0x9a8, 0x9b8,
                                 0x9c8, 0x9d8, 0x9e8, 0x9f8};
  uint16_t spriteMapS10_10[16] = {0x909, 0x919, 0x929, 0x939, 0x949, 0x959,
                                  0x969, 0x979, 0x989, 0x999, 0x9a9, 0x9b9,
                                  0x9c9, 0x9d9, 0x9e9, 0x9f9};
  uint16_t spriteMapS10_11[16] = {0x90a, 0x91a, 0x92a, 0x93a, 0x94a, 0x95a,
                                  0x96a, 0x97a, 0x98a, 0x99a, 0x9aa, 0x9ba,
                                  0x9ca, 0x9da, 0x9ea, 0x9fa};
  uint16_t spriteMapS10_12[16] = {0x90b, 0x91b, 0x92b, 0x93b, 0x94b, 0x95b,
                                  0x96b, 0x97b, 0x98b, 0x99b, 0x9ab, 0x9bb,
                                  0x9cb, 0x9db, 0x9eb, 0x9fb};
  uint16_t spriteMapS10_13[16] = {0x90c, 0x91c, 0x92c, 0x93c, 0x94c, 0x95c,
                                  0x96c, 0x97c, 0x98c, 0x99c, 0x9ac, 0x9bc,
                                  0x9cc, 0x9dc, 0x9ec, 0x9fc};
  uint16_t spriteMapS10_14[16] = {0x90d, 0x91d, 0x92d, 0x93d, 0x94d, 0x95d,
                                  0x96d, 0x97d, 0x98d, 0x99d, 0x9ad, 0x9bd,
                                  0x9cd, 0x9dd, 0x9ed, 0x9fd};
  uint16_t spriteMapS10_15[16] = {0x90e, 0x91e, 0x92e, 0x93e, 0x94e, 0x95e,
                                  0x96e, 0x97e, 0x98e, 0x99e, 0x9ae, 0x9be,
                                  0x9ce, 0x9de, 0x9ee, 0x9fe};
  uint16_t spriteMapS10_16[16] = {0x90f, 0x91f, 0x92f, 0x93f, 0x94f, 0x95f,
                                  0x96f, 0x97f, 0x98f, 0x99f, 0x9af, 0x9bf,
                                  0x9cf, 0x9df, 0x9ef, 0x9ff};
  load_palettes(pal10, PALETTES + PALOFFSET * 11);
  uint16_t SCB1_2common = setSCB1_2(11, 0, 0, 0, 0, 0);
  uint16_t spal10_1[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal10_2[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal10_3[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal10_4[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal10_5[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal10_6[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal10_7[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal10_8[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal10_9[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal10_10[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal10_11[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal10_12[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal10_13[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal10_14[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal10_15[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  uint16_t spal10_16[16] = {
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common,
      SCB1_2common, SCB1_2common, SCB1_2common, SCB1_2common};
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 0, min_crt_sz);
  SCB4 = setSCB4(x0);
  setBACKDROP(backdrop);
  vram_sprite(64 * 0, 1, 0, spriteMapS10_1, spal10_1, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 1);
  setBACKDROP(backdrop);
  vram_sprite(64 * 1, 1, 1, spriteMapS10_2, spal10_2, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 2);
  setBACKDROP(backdrop);
  vram_sprite(64 * 2, 1, 2, spriteMapS10_3, spal10_3, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 3);
  setBACKDROP(backdrop);
  vram_sprite(64 * 3, 1, 3, spriteMapS10_4, spal10_4, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 4);
  setBACKDROP(backdrop);
  vram_sprite(64 * 4, 1, 4, spriteMapS10_5, spal10_5, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 5);
  setBACKDROP(backdrop);
  vram_sprite(64 * 5, 1, 5, spriteMapS10_6, spal10_6, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 6);
  setBACKDROP(backdrop);
  vram_sprite(64 * 6, 1, 6, spriteMapS10_7, spal10_7, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 7);
  setBACKDROP(backdrop);
  vram_sprite(64 * 7, 1, 7, spriteMapS10_8, spal10_8, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 8);
  setBACKDROP(backdrop);
  vram_sprite(64 * 8, 1, 8, spriteMapS10_9, spal10_9, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 9);
  setBACKDROP(backdrop);
  vram_sprite(64 * 9, 1, 9, spriteMapS10_10, spal10_10, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 10);
  setBACKDROP(backdrop);
  vram_sprite(64 * 10, 1, 10, spriteMapS10_11, spal10_11, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 11);
  setBACKDROP(backdrop);
  vram_sprite(64 * 11, 1, 11, spriteMapS10_12, spal10_12, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 12);
  setBACKDROP(backdrop);
  vram_sprite(64 * 12, 1, 12, spriteMapS10_13, spal10_13, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 13);
  setBACKDROP(backdrop);
  vram_sprite(64 * 13, 1, 13, spriteMapS10_14, spal10_14, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 14);
  setBACKDROP(backdrop);
  vram_sprite(64 * 14, 1, 14, spriteMapS10_15, spal10_15, 16, SCB2, SCB3, SCB4);
  SCB2 = setSCB2(xr, yr);
  SCB3 = setSCB3(496 - y0, 1, min_crt_sz);
  SCB4 = setSCB4(x0 + 16 * 15);
  setBACKDROP(backdrop);
  vram_sprite(64 * 15, 1, 15, spriteMapS10_16, spal10_16, 16, SCB2, SCB3, SCB4);
}
