

void NEOGEO_USER showScreen1(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 1 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal1[16];
setpal(pal1,0x0,0x0,0x223,0x3423,0x2633,0x5853,0x4d72,0x4da6,0x5ec9,0x4ff3,0x49e5,0x66b3,0x1396,0x7462,0x2542,0x3333);
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
vram_sprite(sprite_base + 64*0,1,0,spriteMapS1_1,spal1_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,1,spriteMapS1_2,spal1_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,2,spriteMapS1_3,spal1_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,3,spriteMapS1_4,spal1_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,4,spriteMapS1_5,spal1_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,5,spriteMapS1_6,spal1_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,6,spriteMapS1_7,spal1_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,7,spriteMapS1_8,spal1_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,8,spriteMapS1_9,spal1_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,9,spriteMapS1_10,spal1_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,10,spriteMapS1_11,spal1_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,11,spriteMapS1_12,spal1_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,12,spriteMapS1_13,spal1_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,13,spriteMapS1_14,spal1_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,14,spriteMapS1_15,spal1_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,15,spriteMapS1_16,spal1_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen2(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 2 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal2[16];
setpal(pal2,0x0,0x7fff,0x2000,0x4407,0x7111,0xa00,0x7222,0x191c,0x444,0x7555,0xb52,0x2888,0x7d85,0x7aaa,0x7ccc,0x7ec9);
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
vram_sprite(sprite_base + 64*0,1,0,spriteMapS2_1,spal2_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,1,spriteMapS2_2,spal2_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,2,spriteMapS2_3,spal2_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,3,spriteMapS2_4,spal2_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,4,spriteMapS2_5,spal2_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,5,spriteMapS2_6,spal2_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,6,spriteMapS2_7,spal2_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,7,spriteMapS2_8,spal2_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,8,spriteMapS2_9,spal2_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,9,spriteMapS2_10,spal2_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,10,spriteMapS2_11,spal2_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,11,spriteMapS2_12,spal2_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,12,spriteMapS2_13,spal2_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,13,spriteMapS2_14,spal2_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,14,spriteMapS2_15,spal2_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,15,spriteMapS2_16,spal2_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen3(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 3 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal3[16];
setpal(pal3,0x0,0x7fff,0x2000,0x4407,0x7111,0x4900,0x7222,0x191c,0x5f0f,0x555,0xb52,0x5787,0x5d95,0x2aaa,0x5ec9,0xddd);
uint16_t spriteMapS3_1[16] = {0x200,0x210,0x220,0x230,0x240,0x250,0x260,0x270,0x280,0x290,0x2a0,0x2b0,0x2c0,0x2d0,0x2e0,0x2f0};
uint16_t spriteMapS3_2[16] = {0x201,0x211,0x221,0x231,0x241,0x251,0x261,0x271,0x281,0x291,0x2a1,0x2b1,0x2c1,0x2d1,0x2e1,0x2f1};
uint16_t spriteMapS3_3[16] = {0x202,0x212,0x222,0x232,0x242,0x252,0x262,0x272,0x282,0x292,0x2a2,0x2b2,0x2c2,0x2d2,0x2e2,0x2f2};
uint16_t spriteMapS3_4[16] = {0x203,0x213,0x223,0x233,0x243,0x253,0x263,0x273,0x283,0x293,0x2a3,0x2b3,0x2c3,0x2d3,0x2e3,0x2f3};
uint16_t spriteMapS3_5[16] = {0x204,0x214,0x224,0x234,0x244,0x254,0x264,0x274,0x284,0x294,0x2a4,0x2b4,0x2c4,0x2d4,0x2e4,0x2f4};
uint16_t spriteMapS3_6[16] = {0x205,0x215,0x225,0x235,0x245,0x255,0x265,0x275,0x285,0x295,0x2a5,0x2b5,0x2c5,0x2d5,0x2e5,0x2f5};
uint16_t spriteMapS3_7[16] = {0x206,0x216,0x226,0x236,0x246,0x256,0x266,0x276,0x286,0x296,0x2a6,0x2b6,0x2c6,0x2d6,0x2e6,0x2f6};
uint16_t spriteMapS3_8[16] = {0x207,0x217,0x227,0x237,0x247,0x257,0x267,0x277,0x287,0x297,0x2a7,0x2b7,0x2c7,0x2d7,0x2e7,0x2f7};
uint16_t spriteMapS3_9[16] = {0x208,0x218,0x228,0x238,0x248,0x258,0x268,0x278,0x288,0x298,0x2a8,0x2b8,0x2c8,0x2d8,0x2e8,0x2f8};
uint16_t spriteMapS3_10[16] = {0x209,0x219,0x229,0x239,0x249,0x259,0x269,0x279,0x289,0x299,0x2a9,0x2b9,0x2c9,0x2d9,0x2e9,0x2f9};
uint16_t spriteMapS3_11[16] = {0x20a,0x21a,0x22a,0x23a,0x24a,0x25a,0x26a,0x27a,0x28a,0x29a,0x2aa,0x2ba,0x2ca,0x2da,0x2ea,0x2fa};
uint16_t spriteMapS3_12[16] = {0x20b,0x21b,0x22b,0x23b,0x24b,0x25b,0x26b,0x27b,0x28b,0x29b,0x2ab,0x2bb,0x2cb,0x2db,0x2eb,0x2fb};
uint16_t spriteMapS3_13[16] = {0x20c,0x21c,0x22c,0x23c,0x24c,0x25c,0x26c,0x27c,0x28c,0x29c,0x2ac,0x2bc,0x2cc,0x2dc,0x2ec,0x2fc};
uint16_t spriteMapS3_14[16] = {0x20d,0x21d,0x22d,0x23d,0x24d,0x25d,0x26d,0x27d,0x28d,0x29d,0x2ad,0x2bd,0x2cd,0x2dd,0x2ed,0x2fd};
uint16_t spriteMapS3_15[16] = {0x20e,0x21e,0x22e,0x23e,0x24e,0x25e,0x26e,0x27e,0x28e,0x29e,0x2ae,0x2be,0x2ce,0x2de,0x2ee,0x2fe};
uint16_t spriteMapS3_16[16] = {0x20f,0x21f,0x22f,0x23f,0x24f,0x25f,0x26f,0x27f,0x28f,0x29f,0x2af,0x2bf,0x2cf,0x2df,0x2ef,0x2ff};
load_palettes(pal3,PALETTES+PALOFFSET*18);
uint16_t SCB1_2common = setSCB1_2(18,0,0,0,0,0);
uint16_t spal3_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal3_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal3_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal3_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal3_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal3_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal3_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal3_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal3_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal3_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal3_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal3_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal3_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal3_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal3_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal3_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,0,spriteMapS3_1,spal3_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,1,spriteMapS3_2,spal3_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,2,spriteMapS3_3,spal3_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,3,spriteMapS3_4,spal3_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,4,spriteMapS3_5,spal3_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,5,spriteMapS3_6,spal3_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,6,spriteMapS3_7,spal3_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,7,spriteMapS3_8,spal3_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,8,spriteMapS3_9,spal3_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,9,spriteMapS3_10,spal3_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,10,spriteMapS3_11,spal3_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,11,spriteMapS3_12,spal3_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,12,spriteMapS3_13,spal3_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,13,spriteMapS3_14,spal3_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,14,spriteMapS3_15,spal3_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,15,spriteMapS3_16,spal3_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen4(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 4 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal4[16];
setpal(pal4,0x0,0x7fff,0x2000,0x4407,0x7111,0x5222,0x333,0x191c,0x5555,0x3b41,0x888,0x7999,0x5d95,0x7ccc,0x7ec9,0x7eee);
uint16_t spriteMapS4_1[16] = {0x300,0x310,0x320,0x330,0x340,0x350,0x360,0x370,0x380,0x390,0x3a0,0x3b0,0x3c0,0x3d0,0x3e0,0x3f0};
uint16_t spriteMapS4_2[16] = {0x301,0x311,0x321,0x331,0x341,0x351,0x361,0x371,0x381,0x391,0x3a1,0x3b1,0x3c1,0x3d1,0x3e1,0x3f1};
uint16_t spriteMapS4_3[16] = {0x302,0x312,0x322,0x332,0x342,0x352,0x362,0x372,0x382,0x392,0x3a2,0x3b2,0x3c2,0x3d2,0x3e2,0x3f2};
uint16_t spriteMapS4_4[16] = {0x303,0x313,0x323,0x333,0x343,0x353,0x363,0x373,0x383,0x393,0x3a3,0x3b3,0x3c3,0x3d3,0x3e3,0x3f3};
uint16_t spriteMapS4_5[16] = {0x304,0x314,0x324,0x334,0x344,0x354,0x364,0x374,0x384,0x394,0x3a4,0x3b4,0x3c4,0x3d4,0x3e4,0x3f4};
uint16_t spriteMapS4_6[16] = {0x305,0x315,0x325,0x335,0x345,0x355,0x365,0x375,0x385,0x395,0x3a5,0x3b5,0x3c5,0x3d5,0x3e5,0x3f5};
uint16_t spriteMapS4_7[16] = {0x306,0x316,0x326,0x336,0x346,0x356,0x366,0x376,0x386,0x396,0x3a6,0x3b6,0x3c6,0x3d6,0x3e6,0x3f6};
uint16_t spriteMapS4_8[16] = {0x307,0x317,0x327,0x337,0x347,0x357,0x367,0x377,0x387,0x397,0x3a7,0x3b7,0x3c7,0x3d7,0x3e7,0x3f7};
uint16_t spriteMapS4_9[16] = {0x308,0x318,0x328,0x338,0x348,0x358,0x368,0x378,0x388,0x398,0x3a8,0x3b8,0x3c8,0x3d8,0x3e8,0x3f8};
uint16_t spriteMapS4_10[16] = {0x309,0x319,0x329,0x339,0x349,0x359,0x369,0x379,0x389,0x399,0x3a9,0x3b9,0x3c9,0x3d9,0x3e9,0x3f9};
uint16_t spriteMapS4_11[16] = {0x30a,0x31a,0x32a,0x33a,0x34a,0x35a,0x36a,0x37a,0x38a,0x39a,0x3aa,0x3ba,0x3ca,0x3da,0x3ea,0x3fa};
uint16_t spriteMapS4_12[16] = {0x30b,0x31b,0x32b,0x33b,0x34b,0x35b,0x36b,0x37b,0x38b,0x39b,0x3ab,0x3bb,0x3cb,0x3db,0x3eb,0x3fb};
uint16_t spriteMapS4_13[16] = {0x30c,0x31c,0x32c,0x33c,0x34c,0x35c,0x36c,0x37c,0x38c,0x39c,0x3ac,0x3bc,0x3cc,0x3dc,0x3ec,0x3fc};
uint16_t spriteMapS4_14[16] = {0x30d,0x31d,0x32d,0x33d,0x34d,0x35d,0x36d,0x37d,0x38d,0x39d,0x3ad,0x3bd,0x3cd,0x3dd,0x3ed,0x3fd};
uint16_t spriteMapS4_15[16] = {0x30e,0x31e,0x32e,0x33e,0x34e,0x35e,0x36e,0x37e,0x38e,0x39e,0x3ae,0x3be,0x3ce,0x3de,0x3ee,0x3fe};
uint16_t spriteMapS4_16[16] = {0x30f,0x31f,0x32f,0x33f,0x34f,0x35f,0x36f,0x37f,0x38f,0x39f,0x3af,0x3bf,0x3cf,0x3df,0x3ef,0x3ff};
load_palettes(pal4,PALETTES+PALOFFSET*19);
uint16_t SCB1_2common = setSCB1_2(19,0,0,0,0,0);
uint16_t spal4_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal4_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal4_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal4_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal4_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal4_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal4_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal4_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal4_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal4_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal4_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal4_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal4_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal4_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal4_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal4_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,0,spriteMapS4_1,spal4_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,1,spriteMapS4_2,spal4_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,2,spriteMapS4_3,spal4_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,3,spriteMapS4_4,spal4_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,4,spriteMapS4_5,spal4_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,5,spriteMapS4_6,spal4_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,6,spriteMapS4_7,spal4_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,7,spriteMapS4_8,spal4_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,8,spriteMapS4_9,spal4_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,9,spriteMapS4_10,spal4_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,10,spriteMapS4_11,spal4_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,11,spriteMapS4_12,spal4_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,12,spriteMapS4_13,spal4_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,13,spriteMapS4_14,spal4_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,14,spriteMapS4_15,spal4_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,15,spriteMapS4_16,spal4_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen5(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 5 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal5[16];
setpal(pal5,0x0,0x7fff,0x2000,0x406,0x7111,0x609,0x1222,0x333,0x191c,0x7555,0x3b41,0x999,0x5d95,0x5ccc,0x7ec9,0x7eee);
uint16_t spriteMapS5_1[16] = {0x400,0x410,0x420,0x430,0x440,0x450,0x460,0x470,0x480,0x490,0x4a0,0x4b0,0x4c0,0x4d0,0x4e0,0x4f0};
uint16_t spriteMapS5_2[16] = {0x401,0x411,0x421,0x431,0x441,0x451,0x461,0x471,0x481,0x491,0x4a1,0x4b1,0x4c1,0x4d1,0x4e1,0x4f1};
uint16_t spriteMapS5_3[16] = {0x402,0x412,0x422,0x432,0x442,0x452,0x462,0x472,0x482,0x492,0x4a2,0x4b2,0x4c2,0x4d2,0x4e2,0x4f2};
uint16_t spriteMapS5_4[16] = {0x403,0x413,0x423,0x433,0x443,0x453,0x463,0x473,0x483,0x493,0x4a3,0x4b3,0x4c3,0x4d3,0x4e3,0x4f3};
uint16_t spriteMapS5_5[16] = {0x404,0x414,0x424,0x434,0x444,0x454,0x464,0x474,0x484,0x494,0x4a4,0x4b4,0x4c4,0x4d4,0x4e4,0x4f4};
uint16_t spriteMapS5_6[16] = {0x405,0x415,0x425,0x435,0x445,0x455,0x465,0x475,0x485,0x495,0x4a5,0x4b5,0x4c5,0x4d5,0x4e5,0x4f5};
uint16_t spriteMapS5_7[16] = {0x406,0x416,0x426,0x436,0x446,0x456,0x466,0x476,0x486,0x496,0x4a6,0x4b6,0x4c6,0x4d6,0x4e6,0x4f6};
uint16_t spriteMapS5_8[16] = {0x407,0x417,0x427,0x437,0x447,0x457,0x467,0x477,0x487,0x497,0x4a7,0x4b7,0x4c7,0x4d7,0x4e7,0x4f7};
uint16_t spriteMapS5_9[16] = {0x408,0x418,0x428,0x438,0x448,0x458,0x468,0x478,0x488,0x498,0x4a8,0x4b8,0x4c8,0x4d8,0x4e8,0x4f8};
uint16_t spriteMapS5_10[16] = {0x409,0x419,0x429,0x439,0x449,0x459,0x469,0x479,0x489,0x499,0x4a9,0x4b9,0x4c9,0x4d9,0x4e9,0x4f9};
uint16_t spriteMapS5_11[16] = {0x40a,0x41a,0x42a,0x43a,0x44a,0x45a,0x46a,0x47a,0x48a,0x49a,0x4aa,0x4ba,0x4ca,0x4da,0x4ea,0x4fa};
uint16_t spriteMapS5_12[16] = {0x40b,0x41b,0x42b,0x43b,0x44b,0x45b,0x46b,0x47b,0x48b,0x49b,0x4ab,0x4bb,0x4cb,0x4db,0x4eb,0x4fb};
uint16_t spriteMapS5_13[16] = {0x40c,0x41c,0x42c,0x43c,0x44c,0x45c,0x46c,0x47c,0x48c,0x49c,0x4ac,0x4bc,0x4cc,0x4dc,0x4ec,0x4fc};
uint16_t spriteMapS5_14[16] = {0x40d,0x41d,0x42d,0x43d,0x44d,0x45d,0x46d,0x47d,0x48d,0x49d,0x4ad,0x4bd,0x4cd,0x4dd,0x4ed,0x4fd};
uint16_t spriteMapS5_15[16] = {0x40e,0x41e,0x42e,0x43e,0x44e,0x45e,0x46e,0x47e,0x48e,0x49e,0x4ae,0x4be,0x4ce,0x4de,0x4ee,0x4fe};
uint16_t spriteMapS5_16[16] = {0x40f,0x41f,0x42f,0x43f,0x44f,0x45f,0x46f,0x47f,0x48f,0x49f,0x4af,0x4bf,0x4cf,0x4df,0x4ef,0x4ff};
load_palettes(pal5,PALETTES+PALOFFSET*20);
uint16_t SCB1_2common = setSCB1_2(20,0,0,0,0,0);
uint16_t spal5_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal5_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal5_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal5_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal5_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal5_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal5_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal5_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal5_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal5_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal5_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal5_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal5_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal5_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal5_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal5_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,0,spriteMapS5_1,spal5_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,1,spriteMapS5_2,spal5_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,2,spriteMapS5_3,spal5_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,3,spriteMapS5_4,spal5_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,4,spriteMapS5_5,spal5_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,5,spriteMapS5_6,spal5_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,6,spriteMapS5_7,spal5_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,7,spriteMapS5_8,spal5_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,8,spriteMapS5_9,spal5_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,9,spriteMapS5_10,spal5_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,10,spriteMapS5_11,spal5_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,11,spriteMapS5_12,spal5_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,12,spriteMapS5_13,spal5_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,13,spriteMapS5_14,spal5_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,14,spriteMapS5_15,spal5_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,15,spriteMapS5_16,spal5_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen6(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 6 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal6[16];
setpal(pal6,0x0,0x7fff,0x2000,0x4407,0x7111,0x5222,0x333,0x191c,0x2434,0x6556,0x3b41,0x5999,0x5d95,0x7ccc,0x7ec9,0x6eee);
uint16_t spriteMapS6_1[16] = {0x500,0x510,0x520,0x530,0x540,0x550,0x560,0x570,0x580,0x590,0x5a0,0x5b0,0x5c0,0x5d0,0x5e0,0x5f0};
uint16_t spriteMapS6_2[16] = {0x501,0x511,0x521,0x531,0x541,0x551,0x561,0x571,0x581,0x591,0x5a1,0x5b1,0x5c1,0x5d1,0x5e1,0x5f1};
uint16_t spriteMapS6_3[16] = {0x502,0x512,0x522,0x532,0x542,0x552,0x562,0x572,0x582,0x592,0x5a2,0x5b2,0x5c2,0x5d2,0x5e2,0x5f2};
uint16_t spriteMapS6_4[16] = {0x503,0x513,0x523,0x533,0x543,0x553,0x563,0x573,0x583,0x593,0x5a3,0x5b3,0x5c3,0x5d3,0x5e3,0x5f3};
uint16_t spriteMapS6_5[16] = {0x504,0x514,0x524,0x534,0x544,0x554,0x564,0x574,0x584,0x594,0x5a4,0x5b4,0x5c4,0x5d4,0x5e4,0x5f4};
uint16_t spriteMapS6_6[16] = {0x505,0x515,0x525,0x535,0x545,0x555,0x565,0x575,0x585,0x595,0x5a5,0x5b5,0x5c5,0x5d5,0x5e5,0x5f5};
uint16_t spriteMapS6_7[16] = {0x506,0x516,0x526,0x536,0x546,0x556,0x566,0x576,0x586,0x596,0x5a6,0x5b6,0x5c6,0x5d6,0x5e6,0x5f6};
uint16_t spriteMapS6_8[16] = {0x507,0x517,0x527,0x537,0x547,0x557,0x567,0x577,0x587,0x597,0x5a7,0x5b7,0x5c7,0x5d7,0x5e7,0x5f7};
uint16_t spriteMapS6_9[16] = {0x508,0x518,0x528,0x538,0x548,0x558,0x568,0x578,0x588,0x598,0x5a8,0x5b8,0x5c8,0x5d8,0x5e8,0x5f8};
uint16_t spriteMapS6_10[16] = {0x509,0x519,0x529,0x539,0x549,0x559,0x569,0x579,0x589,0x599,0x5a9,0x5b9,0x5c9,0x5d9,0x5e9,0x5f9};
uint16_t spriteMapS6_11[16] = {0x50a,0x51a,0x52a,0x53a,0x54a,0x55a,0x56a,0x57a,0x58a,0x59a,0x5aa,0x5ba,0x5ca,0x5da,0x5ea,0x5fa};
uint16_t spriteMapS6_12[16] = {0x50b,0x51b,0x52b,0x53b,0x54b,0x55b,0x56b,0x57b,0x58b,0x59b,0x5ab,0x5bb,0x5cb,0x5db,0x5eb,0x5fb};
uint16_t spriteMapS6_13[16] = {0x50c,0x51c,0x52c,0x53c,0x54c,0x55c,0x56c,0x57c,0x58c,0x59c,0x5ac,0x5bc,0x5cc,0x5dc,0x5ec,0x5fc};
uint16_t spriteMapS6_14[16] = {0x50d,0x51d,0x52d,0x53d,0x54d,0x55d,0x56d,0x57d,0x58d,0x59d,0x5ad,0x5bd,0x5cd,0x5dd,0x5ed,0x5fd};
uint16_t spriteMapS6_15[16] = {0x50e,0x51e,0x52e,0x53e,0x54e,0x55e,0x56e,0x57e,0x58e,0x59e,0x5ae,0x5be,0x5ce,0x5de,0x5ee,0x5fe};
uint16_t spriteMapS6_16[16] = {0x50f,0x51f,0x52f,0x53f,0x54f,0x55f,0x56f,0x57f,0x58f,0x59f,0x5af,0x5bf,0x5cf,0x5df,0x5ef,0x5ff};
load_palettes(pal6,PALETTES+PALOFFSET*21);
uint16_t SCB1_2common = setSCB1_2(21,0,0,0,0,0);
uint16_t spal6_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal6_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal6_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal6_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal6_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal6_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal6_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal6_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal6_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal6_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal6_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal6_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal6_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal6_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal6_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal6_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,0,spriteMapS6_1,spal6_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,1,spriteMapS6_2,spal6_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,2,spriteMapS6_3,spal6_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,3,spriteMapS6_4,spal6_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,4,spriteMapS6_5,spal6_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,5,spriteMapS6_6,spal6_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,6,spriteMapS6_7,spal6_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,7,spriteMapS6_8,spal6_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,8,spriteMapS6_9,spal6_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,9,spriteMapS6_10,spal6_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,10,spriteMapS6_11,spal6_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,11,spriteMapS6_12,spal6_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,12,spriteMapS6_13,spal6_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,13,spriteMapS6_14,spal6_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,14,spriteMapS6_15,spal6_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,15,spriteMapS6_16,spal6_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen7(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 7 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal7[16];
setpal(pal7,0x0,0x7fff,0x2000,0x4407,0x7111,0x5222,0x333,0x191c,0x2434,0x2656,0x3b41,0x5d95,0x699a,0x7ccc,0x7ec9,0xeee);
uint16_t spriteMapS7_1[16] = {0x600,0x610,0x620,0x630,0x640,0x650,0x660,0x670,0x680,0x690,0x6a0,0x6b0,0x6c0,0x6d0,0x6e0,0x6f0};
uint16_t spriteMapS7_2[16] = {0x601,0x611,0x621,0x631,0x641,0x651,0x661,0x671,0x681,0x691,0x6a1,0x6b1,0x6c1,0x6d1,0x6e1,0x6f1};
uint16_t spriteMapS7_3[16] = {0x602,0x612,0x622,0x632,0x642,0x652,0x662,0x672,0x682,0x692,0x6a2,0x6b2,0x6c2,0x6d2,0x6e2,0x6f2};
uint16_t spriteMapS7_4[16] = {0x603,0x613,0x623,0x633,0x643,0x653,0x663,0x673,0x683,0x693,0x6a3,0x6b3,0x6c3,0x6d3,0x6e3,0x6f3};
uint16_t spriteMapS7_5[16] = {0x604,0x614,0x624,0x634,0x644,0x654,0x664,0x674,0x684,0x694,0x6a4,0x6b4,0x6c4,0x6d4,0x6e4,0x6f4};
uint16_t spriteMapS7_6[16] = {0x605,0x615,0x625,0x635,0x645,0x655,0x665,0x675,0x685,0x695,0x6a5,0x6b5,0x6c5,0x6d5,0x6e5,0x6f5};
uint16_t spriteMapS7_7[16] = {0x606,0x616,0x626,0x636,0x646,0x656,0x666,0x676,0x686,0x696,0x6a6,0x6b6,0x6c6,0x6d6,0x6e6,0x6f6};
uint16_t spriteMapS7_8[16] = {0x607,0x617,0x627,0x637,0x647,0x657,0x667,0x677,0x687,0x697,0x6a7,0x6b7,0x6c7,0x6d7,0x6e7,0x6f7};
uint16_t spriteMapS7_9[16] = {0x608,0x618,0x628,0x638,0x648,0x658,0x668,0x678,0x688,0x698,0x6a8,0x6b8,0x6c8,0x6d8,0x6e8,0x6f8};
uint16_t spriteMapS7_10[16] = {0x609,0x619,0x629,0x639,0x649,0x659,0x669,0x679,0x689,0x699,0x6a9,0x6b9,0x6c9,0x6d9,0x6e9,0x6f9};
uint16_t spriteMapS7_11[16] = {0x60a,0x61a,0x62a,0x63a,0x64a,0x65a,0x66a,0x67a,0x68a,0x69a,0x6aa,0x6ba,0x6ca,0x6da,0x6ea,0x6fa};
uint16_t spriteMapS7_12[16] = {0x60b,0x61b,0x62b,0x63b,0x64b,0x65b,0x66b,0x67b,0x68b,0x69b,0x6ab,0x6bb,0x6cb,0x6db,0x6eb,0x6fb};
uint16_t spriteMapS7_13[16] = {0x60c,0x61c,0x62c,0x63c,0x64c,0x65c,0x66c,0x67c,0x68c,0x69c,0x6ac,0x6bc,0x6cc,0x6dc,0x6ec,0x6fc};
uint16_t spriteMapS7_14[16] = {0x60d,0x61d,0x62d,0x63d,0x64d,0x65d,0x66d,0x67d,0x68d,0x69d,0x6ad,0x6bd,0x6cd,0x6dd,0x6ed,0x6fd};
uint16_t spriteMapS7_15[16] = {0x60e,0x61e,0x62e,0x63e,0x64e,0x65e,0x66e,0x67e,0x68e,0x69e,0x6ae,0x6be,0x6ce,0x6de,0x6ee,0x6fe};
uint16_t spriteMapS7_16[16] = {0x60f,0x61f,0x62f,0x63f,0x64f,0x65f,0x66f,0x67f,0x68f,0x69f,0x6af,0x6bf,0x6cf,0x6df,0x6ef,0x6ff};
load_palettes(pal7,PALETTES+PALOFFSET*22);
uint16_t SCB1_2common = setSCB1_2(22,0,0,0,0,0);
uint16_t spal7_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal7_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal7_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal7_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal7_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal7_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal7_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal7_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal7_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal7_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal7_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal7_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal7_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal7_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal7_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal7_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,0,spriteMapS7_1,spal7_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,1,spriteMapS7_2,spal7_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,2,spriteMapS7_3,spal7_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,3,spriteMapS7_4,spal7_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,4,spriteMapS7_5,spal7_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,5,spriteMapS7_6,spal7_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,6,spriteMapS7_7,spal7_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,7,spriteMapS7_8,spal7_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,8,spriteMapS7_9,spal7_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,9,spriteMapS7_10,spal7_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,10,spriteMapS7_11,spal7_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,11,spriteMapS7_12,spal7_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,12,spriteMapS7_13,spal7_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,13,spriteMapS7_14,spal7_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,14,spriteMapS7_15,spal7_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,15,spriteMapS7_16,spal7_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen8(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 8 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal8[16];
setpal(pal8,0x0,0x7fff,0x2000,0x111,0x4407,0x7111,0x7222,0x191c,0x6555,0x3b41,0x7777,0x6d85,0x7999,0x7ccc,0x5ec9,0x2eee);
uint16_t spriteMapS8_1[16] = {0x700,0x710,0x720,0x730,0x740,0x750,0x760,0x770,0x780,0x790,0x7a0,0x7b0,0x7c0,0x7d0,0x7e0,0x7f0};
uint16_t spriteMapS8_2[16] = {0x701,0x711,0x721,0x731,0x741,0x751,0x761,0x771,0x781,0x791,0x7a1,0x7b1,0x7c1,0x7d1,0x7e1,0x7f1};
uint16_t spriteMapS8_3[16] = {0x702,0x712,0x722,0x732,0x742,0x752,0x762,0x772,0x782,0x792,0x7a2,0x7b2,0x7c2,0x7d2,0x7e2,0x7f2};
uint16_t spriteMapS8_4[16] = {0x703,0x713,0x723,0x733,0x743,0x753,0x763,0x773,0x783,0x793,0x7a3,0x7b3,0x7c3,0x7d3,0x7e3,0x7f3};
uint16_t spriteMapS8_5[16] = {0x704,0x714,0x724,0x734,0x744,0x754,0x764,0x774,0x784,0x794,0x7a4,0x7b4,0x7c4,0x7d4,0x7e4,0x7f4};
uint16_t spriteMapS8_6[16] = {0x705,0x715,0x725,0x735,0x745,0x755,0x765,0x775,0x785,0x795,0x7a5,0x7b5,0x7c5,0x7d5,0x7e5,0x7f5};
uint16_t spriteMapS8_7[16] = {0x706,0x716,0x726,0x736,0x746,0x756,0x766,0x776,0x786,0x796,0x7a6,0x7b6,0x7c6,0x7d6,0x7e6,0x7f6};
uint16_t spriteMapS8_8[16] = {0x707,0x717,0x727,0x737,0x747,0x757,0x767,0x777,0x787,0x797,0x7a7,0x7b7,0x7c7,0x7d7,0x7e7,0x7f7};
uint16_t spriteMapS8_9[16] = {0x708,0x718,0x728,0x738,0x748,0x758,0x768,0x778,0x788,0x798,0x7a8,0x7b8,0x7c8,0x7d8,0x7e8,0x7f8};
uint16_t spriteMapS8_10[16] = {0x709,0x719,0x729,0x739,0x749,0x759,0x769,0x779,0x789,0x799,0x7a9,0x7b9,0x7c9,0x7d9,0x7e9,0x7f9};
uint16_t spriteMapS8_11[16] = {0x70a,0x71a,0x72a,0x73a,0x74a,0x75a,0x76a,0x77a,0x78a,0x79a,0x7aa,0x7ba,0x7ca,0x7da,0x7ea,0x7fa};
uint16_t spriteMapS8_12[16] = {0x70b,0x71b,0x72b,0x73b,0x74b,0x75b,0x76b,0x77b,0x78b,0x79b,0x7ab,0x7bb,0x7cb,0x7db,0x7eb,0x7fb};
uint16_t spriteMapS8_13[16] = {0x70c,0x71c,0x72c,0x73c,0x74c,0x75c,0x76c,0x77c,0x78c,0x79c,0x7ac,0x7bc,0x7cc,0x7dc,0x7ec,0x7fc};
uint16_t spriteMapS8_14[16] = {0x70d,0x71d,0x72d,0x73d,0x74d,0x75d,0x76d,0x77d,0x78d,0x79d,0x7ad,0x7bd,0x7cd,0x7dd,0x7ed,0x7fd};
uint16_t spriteMapS8_15[16] = {0x70e,0x71e,0x72e,0x73e,0x74e,0x75e,0x76e,0x77e,0x78e,0x79e,0x7ae,0x7be,0x7ce,0x7de,0x7ee,0x7fe};
uint16_t spriteMapS8_16[16] = {0x70f,0x71f,0x72f,0x73f,0x74f,0x75f,0x76f,0x77f,0x78f,0x79f,0x7af,0x7bf,0x7cf,0x7df,0x7ef,0x7ff};
load_palettes(pal8,PALETTES+PALOFFSET*23);
uint16_t SCB1_2common = setSCB1_2(23,0,0,0,0,0);
uint16_t spal8_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal8_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal8_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal8_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal8_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal8_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal8_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal8_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal8_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal8_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal8_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal8_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal8_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal8_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal8_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal8_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,0,spriteMapS8_1,spal8_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,1,spriteMapS8_2,spal8_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,2,spriteMapS8_3,spal8_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,3,spriteMapS8_4,spal8_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,4,spriteMapS8_5,spal8_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,5,spriteMapS8_6,spal8_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,6,spriteMapS8_7,spal8_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,7,spriteMapS8_8,spal8_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,8,spriteMapS8_9,spal8_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,9,spriteMapS8_10,spal8_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,10,spriteMapS8_11,spal8_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,11,spriteMapS8_12,spal8_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,12,spriteMapS8_13,spal8_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,13,spriteMapS8_14,spal8_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,14,spriteMapS8_15,spal8_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,15,spriteMapS8_16,spal8_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen9(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 9 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal9[16];
setpal(pal9,0x0,0x1015,0x7fff,0x48ce,0x5b,0x1e88,0x4d00,0x1000,0x168a,0x1003,0x34ae,0x108e,0x1cee,0x1038,0x2c24,0x447);
uint16_t spriteMapS9_1[16] = {0x800,0x810,0x820,0x830,0x840,0x850,0x860,0x870,0x880,0x890,0x8a0,0x8b0,0x8c0,0x8d0,0x8e0,0x8f0};
uint16_t spriteMapS9_2[16] = {0x801,0x811,0x821,0x831,0x841,0x851,0x861,0x871,0x881,0x891,0x8a1,0x8b1,0x8c1,0x8d1,0x8e1,0x8f1};
uint16_t spriteMapS9_3[16] = {0x802,0x812,0x822,0x832,0x842,0x852,0x862,0x872,0x882,0x892,0x8a2,0x8b2,0x8c2,0x8d2,0x8e2,0x8f2};
uint16_t spriteMapS9_4[16] = {0x803,0x813,0x823,0x833,0x843,0x853,0x863,0x873,0x883,0x893,0x8a3,0x8b3,0x8c3,0x8d3,0x8e3,0x8f3};
uint16_t spriteMapS9_5[16] = {0x804,0x814,0x824,0x834,0x844,0x854,0x864,0x874,0x884,0x894,0x8a4,0x8b4,0x8c4,0x8d4,0x8e4,0x8f4};
uint16_t spriteMapS9_6[16] = {0x805,0x815,0x825,0x835,0x845,0x855,0x865,0x875,0x885,0x895,0x8a5,0x8b5,0x8c5,0x8d5,0x8e5,0x8f5};
uint16_t spriteMapS9_7[16] = {0x806,0x816,0x826,0x836,0x846,0x856,0x866,0x876,0x886,0x896,0x8a6,0x8b6,0x8c6,0x8d6,0x8e6,0x8f6};
uint16_t spriteMapS9_8[16] = {0x807,0x817,0x827,0x837,0x847,0x857,0x867,0x877,0x887,0x897,0x8a7,0x8b7,0x8c7,0x8d7,0x8e7,0x8f7};
uint16_t spriteMapS9_9[16] = {0x808,0x818,0x828,0x838,0x848,0x858,0x868,0x878,0x888,0x898,0x8a8,0x8b8,0x8c8,0x8d8,0x8e8,0x8f8};
uint16_t spriteMapS9_10[16] = {0x809,0x819,0x829,0x839,0x849,0x859,0x869,0x879,0x889,0x899,0x8a9,0x8b9,0x8c9,0x8d9,0x8e9,0x8f9};
uint16_t spriteMapS9_11[16] = {0x80a,0x81a,0x82a,0x83a,0x84a,0x85a,0x86a,0x87a,0x88a,0x89a,0x8aa,0x8ba,0x8ca,0x8da,0x8ea,0x8fa};
uint16_t spriteMapS9_12[16] = {0x80b,0x81b,0x82b,0x83b,0x84b,0x85b,0x86b,0x87b,0x88b,0x89b,0x8ab,0x8bb,0x8cb,0x8db,0x8eb,0x8fb};
uint16_t spriteMapS9_13[16] = {0x80c,0x81c,0x82c,0x83c,0x84c,0x85c,0x86c,0x87c,0x88c,0x89c,0x8ac,0x8bc,0x8cc,0x8dc,0x8ec,0x8fc};
uint16_t spriteMapS9_14[16] = {0x80d,0x81d,0x82d,0x83d,0x84d,0x85d,0x86d,0x87d,0x88d,0x89d,0x8ad,0x8bd,0x8cd,0x8dd,0x8ed,0x8fd};
uint16_t spriteMapS9_15[16] = {0x80e,0x81e,0x82e,0x83e,0x84e,0x85e,0x86e,0x87e,0x88e,0x89e,0x8ae,0x8be,0x8ce,0x8de,0x8ee,0x8fe};
uint16_t spriteMapS9_16[16] = {0x80f,0x81f,0x82f,0x83f,0x84f,0x85f,0x86f,0x87f,0x88f,0x89f,0x8af,0x8bf,0x8cf,0x8df,0x8ef,0x8ff};
load_palettes(pal9,PALETTES+PALOFFSET*24);
uint16_t SCB1_2common = setSCB1_2(24,0,0,0,0,0);
uint16_t spal9_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal9_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal9_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal9_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal9_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal9_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal9_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal9_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal9_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal9_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal9_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal9_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal9_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal9_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal9_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal9_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,0,spriteMapS9_1,spal9_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,1,spriteMapS9_2,spal9_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,2,spriteMapS9_3,spal9_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,3,spriteMapS9_4,spal9_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,4,spriteMapS9_5,spal9_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,5,spriteMapS9_6,spal9_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,6,spriteMapS9_7,spal9_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,7,spriteMapS9_8,spal9_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,8,spriteMapS9_9,spal9_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,9,spriteMapS9_10,spal9_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,10,spriteMapS9_11,spal9_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,11,spriteMapS9_12,spal9_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,12,spriteMapS9_13,spal9_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,13,spriteMapS9_14,spal9_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,14,spriteMapS9_15,spal9_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,15,spriteMapS9_16,spal9_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen10(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 10 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal10[16];
setpal(pal10,0x0,0x1000,0x3fe7,0x7f10,0x7820,0x111d,0x4c18,0x4526,0x3104,0x1f80,0xeff,0x79ac,0x7b10,0x7400,0x347e,0x5fe0);
uint16_t spriteMapS10_1[16] = {0x900,0x910,0x920,0x930,0x940,0x950,0x960,0x970,0x980,0x990,0x9a0,0x9b0,0x9c0,0x9d0,0x9e0,0x9f0};
uint16_t spriteMapS10_2[16] = {0x901,0x911,0x921,0x931,0x941,0x951,0x961,0x971,0x981,0x991,0x9a1,0x9b1,0x9c1,0x9d1,0x9e1,0x9f1};
uint16_t spriteMapS10_3[16] = {0x902,0x912,0x922,0x932,0x942,0x952,0x962,0x972,0x982,0x992,0x9a2,0x9b2,0x9c2,0x9d2,0x9e2,0x9f2};
uint16_t spriteMapS10_4[16] = {0x903,0x913,0x923,0x933,0x943,0x953,0x963,0x973,0x983,0x993,0x9a3,0x9b3,0x9c3,0x9d3,0x9e3,0x9f3};
uint16_t spriteMapS10_5[16] = {0x904,0x914,0x924,0x934,0x944,0x954,0x964,0x974,0x984,0x994,0x9a4,0x9b4,0x9c4,0x9d4,0x9e4,0x9f4};
uint16_t spriteMapS10_6[16] = {0x905,0x915,0x925,0x935,0x945,0x955,0x965,0x975,0x985,0x995,0x9a5,0x9b5,0x9c5,0x9d5,0x9e5,0x9f5};
uint16_t spriteMapS10_7[16] = {0x906,0x916,0x926,0x936,0x946,0x956,0x966,0x976,0x986,0x996,0x9a6,0x9b6,0x9c6,0x9d6,0x9e6,0x9f6};
uint16_t spriteMapS10_8[16] = {0x907,0x917,0x927,0x937,0x947,0x957,0x967,0x977,0x987,0x997,0x9a7,0x9b7,0x9c7,0x9d7,0x9e7,0x9f7};
uint16_t spriteMapS10_9[16] = {0x908,0x918,0x928,0x938,0x948,0x958,0x968,0x978,0x988,0x998,0x9a8,0x9b8,0x9c8,0x9d8,0x9e8,0x9f8};
uint16_t spriteMapS10_10[16] = {0x909,0x919,0x929,0x939,0x949,0x959,0x969,0x979,0x989,0x999,0x9a9,0x9b9,0x9c9,0x9d9,0x9e9,0x9f9};
uint16_t spriteMapS10_11[16] = {0x90a,0x91a,0x92a,0x93a,0x94a,0x95a,0x96a,0x97a,0x98a,0x99a,0x9aa,0x9ba,0x9ca,0x9da,0x9ea,0x9fa};
uint16_t spriteMapS10_12[16] = {0x90b,0x91b,0x92b,0x93b,0x94b,0x95b,0x96b,0x97b,0x98b,0x99b,0x9ab,0x9bb,0x9cb,0x9db,0x9eb,0x9fb};
uint16_t spriteMapS10_13[16] = {0x90c,0x91c,0x92c,0x93c,0x94c,0x95c,0x96c,0x97c,0x98c,0x99c,0x9ac,0x9bc,0x9cc,0x9dc,0x9ec,0x9fc};
uint16_t spriteMapS10_14[16] = {0x90d,0x91d,0x92d,0x93d,0x94d,0x95d,0x96d,0x97d,0x98d,0x99d,0x9ad,0x9bd,0x9cd,0x9dd,0x9ed,0x9fd};
uint16_t spriteMapS10_15[16] = {0x90e,0x91e,0x92e,0x93e,0x94e,0x95e,0x96e,0x97e,0x98e,0x99e,0x9ae,0x9be,0x9ce,0x9de,0x9ee,0x9fe};
uint16_t spriteMapS10_16[16] = {0x90f,0x91f,0x92f,0x93f,0x94f,0x95f,0x96f,0x97f,0x98f,0x99f,0x9af,0x9bf,0x9cf,0x9df,0x9ef,0x9ff};
load_palettes(pal10,PALETTES+PALOFFSET*25);
uint16_t SCB1_2common = setSCB1_2(25,0,0,0,0,0);
uint16_t spal10_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal10_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal10_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal10_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal10_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal10_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal10_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal10_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal10_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal10_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal10_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal10_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal10_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal10_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal10_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal10_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,0,spriteMapS10_1,spal10_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,1,spriteMapS10_2,spal10_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,2,spriteMapS10_3,spal10_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,3,spriteMapS10_4,spal10_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,4,spriteMapS10_5,spal10_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,5,spriteMapS10_6,spal10_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,6,spriteMapS10_7,spal10_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,7,spriteMapS10_8,spal10_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,8,spriteMapS10_9,spal10_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,9,spriteMapS10_10,spal10_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,10,spriteMapS10_11,spal10_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,11,spriteMapS10_12,spal10_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,12,spriteMapS10_13,spal10_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,13,spriteMapS10_14,spal10_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,14,spriteMapS10_15,spal10_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,15,spriteMapS10_16,spal10_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen11(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 11 ******************************************/
uint16_t  pal11[16];
setpal(pal11,0x0,0x6831,0x7fe8,0x0,0x6fff,0x5012,0x1410,0x1d70,0x4434,0x7248,0x5ea5,0x3b54,0x6fc2,0x777,0x3125,0x3baa);
load_palettes(pal11,PALETTES+PALOFFSET*26);
}


void NEOGEO_USER showScreen12(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 12 ******************************************/
uint16_t  pal12[16];
setpal(pal12,0x0,0x6fff,0x0,0x2c61,0x536a,0x4ffa,0x7137,0x3c45,0x6411,0x5fe3,0x6731,0x5234,0x1877,0x5012,0xcbb,0x5ea2);
load_palettes(pal12,PALETTES+PALOFFSET*27);
}


void NEOGEO_USER showScreen13(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 13 ******************************************/
uint16_t  pal13[16];
setpal(pal13,0x0,0x0,0x6fb1,0x1247,0xfff,0x521,0x5ffa,0x2aab,0x766,0x136a,0x6821,0x5b60,0x6fd6,0x1124,0x4d75,0x3101);
load_palettes(pal13,PALETTES+PALOFFSET*28);
}


void NEOGEO_USER showScreen14(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 14 ******************************************/
uint16_t  pal14[16];
setpal(pal14,0x0,0x0,0x4ffa,0x1b74,0x1d60,0x6013,0x3889,0x7fe3,0x4148,0x7830,0x6fff,0x1fb6,0x6fa1,0x6545,0x1ccc,0x2511);
load_palettes(pal14,PALETTES+PALOFFSET*29);
}


void NEOGEO_USER showScreen15(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 15 ******************************************/
uint16_t  pal15[16];
setpal(pal15,0x0,0x0,0x4ffa,0x7931,0x1112,0x544,0x248,0x7e85,0x7c61,0xfff,0x6fb1,0x2511,0x3125,0x2bab,0x7fd5,0x5777);
load_palettes(pal15,PALETTES+PALOFFSET*30);
}


void NEOGEO_USER showScreen16(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 16 ******************************************/
uint16_t  pal16[16];
setpal(pal16,0x0,0x0,0xfb6,0x4731,0x3baa,0x6b51,0xfff,0x6e82,0x1247,0x6311,0x3124,0x136a,0x5012,0x5ff9,0x6766,0x4fe2);
load_palettes(pal16,PALETTES+PALOFFSET*31);
}


void NEOGEO_USER showScreen17(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 17 ******************************************/
uint16_t  pal17[16];
setpal(pal17,0x0,0x1830,0x7ffe,0x1410,0x5544,0x64ae,0xa76,0x0,0x2c51,0x2f92,0x4fe9,0x359,0x4013,0x5136,0x5fe3,0x6aaa);
load_palettes(pal17,PALETTES+PALOFFSET*32);
}


void NEOGEO_USER showScreen18(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 18 ******************************************/
uint16_t  pal18[16];
setpal(pal18,0x0,0x5b34,0x2fff,0x0,0x4fc2,0x1247,0x2114,0x7666,0x7aaa,0x2621,0x2e71,0x2feb,0xa51,0x311,0xe96,0x5fe6);
load_palettes(pal18,PALETTES+PALOFFSET*33);
}


void NEOGEO_USER showScreen19(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 19 ******************************************/
uint16_t  pal19[16];
setpal(pal19,0x0,0x6545,0x2821,0x5fe8,0x4fd2,0x3410,0x7e85,0x0,0x3cbb,0x7ffe,0x1136,0x5888,0x3b51,0x7013,0x6e81,0x1258);
load_palettes(pal19,PALETTES+PALOFFSET*34);
}


void NEOGEO_USER showScreen20(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 20 ******************************************/
uint16_t  pal20[16];
setpal(pal20,0x0,0x545,0x6fd2,0x0,0x6fff,0x4e91,0x5730,0x6014,0x5788,0x3b50,0x5147,0x5fd6,0x5ffa,0x6311,0x6bbc,0x5b75);
load_palettes(pal20,PALETTES+PALOFFSET*35);
}


void NEOGEO_USER showScreen21(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 21 ******************************************/
uint16_t  pal21[16];
setpal(pal21,0x0,0x3112,0x5ffa,0x7931,0x544,0x4136,0x3510,0x4e95,0x5fc1,0x5abb,0x0,0x4fe6,0xfff,0x7d61,0x4777,0x1359);
load_palettes(pal21,PALETTES+PALOFFSET*36);
}


void NEOGEO_USER showScreen22(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 22 ******************************************/
uint16_t  pal22[16];
setpal(pal22,0x0,0x6fff,0x0,0x2f91,0x2434,0x5410,0x6fe9,0x7755,0x4e95,0x2989,0x4fe3,0x4bcc,0xc61,0x5830,0x3248,0x3113);
load_palettes(pal22,PALETTES+PALOFFSET*37);
}


void NEOGEO_USER showScreen23(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 23 ******************************************/
uint16_t  pal23[16];
setpal(pal23,0x0,0x7544,0x6fff,0x6e81,0x6a41,0x0,0x4fe2,0x4889,0x7248,0x6fe8,0x112,0x3125,0x3d54,0x6bbc,0x6511,0x2fa5);
load_palettes(pal23,PALETTES+PALOFFSET*38);
}


void NEOGEO_USER showScreen24(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 24 ******************************************/
uint16_t  pal24[16];
setpal(pal24,0x0,0x0,0x99a,0x3b30,0x5963,0x4fe2,0x7fff,0x1eec,0x1247,0x3655,0x4ff8,0x6e81,0x2d37,0x621,0x6113,0x3fa6);
load_palettes(pal24,PALETTES+PALOFFSET*39);
}


void NEOGEO_USER showScreen25(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 25 ******************************************/
uint16_t  pal25[16];
setpal(pal25,0x0,0x7bbc,0x0,0x4ff8,0x2113,0x5554,0x7820,0x5148,0x3e31,0x6fff,0x7fe2,0x5410,0x5888,0x1f90,0x4b61,0x4ea5);
load_palettes(pal25,PALETTES+PALOFFSET*40);
}


void NEOGEO_USER showScreen26(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 26 ******************************************/
uint16_t  pal26[16];
setpal(pal26,0x0,0x1410,0x5012,0xbbb,0x6fe3,0xfff,0x1359,0x6831,0x0,0x4c61,0x6434,0x5777,0x5136,0x1d76,0x7fe8,0x1fa1);
load_palettes(pal26,PALETTES+PALOFFSET*41);
}


void NEOGEO_USER showScreen27(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 27 ******************************************/
uint16_t  pal27[16];
setpal(pal27,0x0,0x112,0xfff,0x0,0x4fe2,0x7249,0x7fe8,0x2821,0x2b51,0x6e49,0x1ea5,0x1410,0x2f81,0x3766,0x3225,0x3aab);
load_palettes(pal27,PALETTES+PALOFFSET*42);
}


void NEOGEO_USER showScreen28(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 28 ******************************************/
uint16_t  pal28[16];
setpal(pal28,0x0,0x4fd1,0x0,0x2511,0x248,0x3544,0x7fff,0x7fe8,0x4e81,0x2113,0x2a41,0x5ffb,0x1bbb,0x1888,0x7fd4,0x7c86);
load_palettes(pal28,PALETTES+PALOFFSET*43);
}


void NEOGEO_USER showScreen29(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 29 ******************************************/
uint16_t  pal29[16];
setpal(pal29,0x0,0x4112,0x1e81,0x6511,0x6fff,0x2249,0xa41,0x7855,0x889,0xfc6,0x2cbc,0x5fe1,0x1125,0x4ffa,0x0,0x3434);
load_palettes(pal29,PALETTES+PALOFFSET*44);
}


void NEOGEO_USER showScreen30(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 30 ******************************************/
uint16_t  pal30[16];
setpal(pal30,0x0,0x731,0x2fff,0x0,0x4311,0x2f91,0x5777,0x113,0x6b51,0x7236,0x4ff9,0x7fe2,0xfa6,0x135a,0x1b45,0x4bbc);
load_palettes(pal30,PALETTES+PALOFFSET*45);
}


void NEOGEO_USER showScreen31(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 31 ******************************************/
uint16_t  pal31[16];
setpal(pal31,0x0,0x7300,0x6fff,0x0,0x7fc1,0x636c,0x4c61,0x6138,0x6fe8,0x6bbc,0x2821,0x7013,0x4e94,0x5988,0x5333,0x2656);
load_palettes(pal31,PALETTES+PALOFFSET*46);
}


void NEOGEO_USER showScreen32(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 32 ******************************************/
uint16_t  pal32[16];
setpal(pal32,0x0,0x2434,0x5fe3,0x766,0x6fff,0x0,0x1999,0x4a51,0x4ff9,0x3610,0x4c84,0xf91,0x2137,0x1112,0x1ccc,0x137b);
load_palettes(pal32,PALETTES+PALOFFSET*47);
}


void NEOGEO_USER showScreen33(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 33 ******************************************/
uint16_t  pal33[16];
setpal(pal33,0x0,0x0,0x7fe8,0x2fff,0x669f,0xc62,0x4248,0x403f,0x6fe2,0x7300,0x7013,0x2434,0x2f92,0x2821,0x1caa,0x5767);
load_palettes(pal33,PALETTES+PALOFFSET*48);
}


void NEOGEO_USER showScreen34(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 34 ******************************************/
uint16_t  pal34[16];
setpal(pal34,0x0,0x7fff,0x0,0x1e81,0x1cbb,0x57bf,0x1510,0x14f,0x7fea,0x5fe1,0x6c39,0x1777,0x4013,0x4136,0x4941,0x4fe6);
load_palettes(pal34,PALETTES+PALOFFSET*49);
}


void NEOGEO_USER showScreen35(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 35 ******************************************/
uint16_t  pal35[16];
setpal(pal35,0x0,0x2a31,0x0,0x5ff1,0x2fff,0x7aab,0x4666,0x4d71,0x6248,0x6d77,0x112,0x3fa1,0x7fea,0x6125,0x4fe6,0x6511);
load_palettes(pal35,PALETTES+PALOFFSET*50);
}


void NEOGEO_USER showScreen36(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 36 ******************************************/
uint16_t  pal36[16];
setpal(pal36,0x0,0x4a41,0x0,0x4fe6,0x1510,0x6fff,0x2cbc,0x5988,0x346b,0x4ffb,0x655,0x1247,0x1124,0x3e71,0x112,0x5fd1);
load_palettes(pal36,PALETTES+PALOFFSET*51);
}


void NEOGEO_USER showScreen37(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 37 ******************************************/
uint16_t  pal37[16];
setpal(pal37,0x0,0x5766,0x0,0x2fff,0x1aab,0x6fd6,0x4b51,0x6249,0x1720,0x4fe1,0x1125,0xe68,0x4333,0x3fea,0x1f91,0x1111);
load_palettes(pal37,PALETTES+PALOFFSET*52);
}


void NEOGEO_USER showScreen38(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 38 ******************************************/
uint16_t  pal38[16];
setpal(pal38,0x0,0x5fd1,0xa89,0x0,0x6fff,0x2931,0x5333,0x536b,0x7d71,0x248,0x6002,0x5410,0x2756,0x4fe8,0x6015,0x7bbc);
load_palettes(pal38,PALETTES+PALOFFSET*53);
}


void NEOGEO_USER showScreen39(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 39 ******************************************/
uint16_t  pal39[16];
setpal(pal39,0x0,0x6821,0x6fff,0x0,0x2645,0x2113,0x7fe1,0x4137,0x6988,0x7c39,0x335a,0x2c61,0x6fe8,0x1fa1,0x3410,0xccd);
load_palettes(pal39,PALETTES+PALOFFSET*54);
}


void NEOGEO_USER showScreen40(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 40 ******************************************/
uint16_t  pal40[16];
setpal(pal40,0x0,0x2931,0x0,0x6fff,0x1fa1,0x36f,0x5259,0x3d61,0x6a78,0x2545,0x7fe8,0x7fe2,0x2113,0x7bbd,0x3510,0x3126);
load_palettes(pal40,PALETTES+PALOFFSET*55);
}


void NEOGEO_USER showScreen41(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 41 ******************************************/
uint16_t  pal41[16];
setpal(pal41,0x0,0x3113,0x0,0x3fff,0x6fd2,0x669f,0xbdf,0x7e70,0x4ff8,0x3d75,0x2a31,0x5666,0x6137,0x1aaa,0x3511,0x515e);
load_palettes(pal41,PALETTES+PALOFFSET*56);
}


void NEOGEO_USER showScreen42(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 42 ******************************************/
uint16_t  pal42[16];
setpal(pal42,0x0,0x0,0x5fff,0x664f,0x1e81,0x6fd1,0x7544,0x4fe6,0x1d33,0x4411,0x2238,0x3113,0x6941,0x2c9f,0x6989,0xfeb);
load_palettes(pal42,PALETTES+PALOFFSET*57);
}


void NEOGEO_USER showScreen43(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 43 ******************************************/
uint16_t  pal43[16];
setpal(pal43,0x0,0x0,0x5caf,0x2b88,0x7fff,0x4458,0x7fe4,0x1f91,0x127,0x6ecf,0x6a31,0x674f,0x3a7f,0x4013,0x621f,0x2511);
load_palettes(pal43,PALETTES+PALOFFSET*58);
}


void NEOGEO_USER showScreen44(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 44 ******************************************/
uint16_t  pal44[16];
setpal(pal44,0x0,0x0,0x6fff,0x6511,0x6e81,0x1235,0x5c95,0x4013,0x259,0x7e25,0x1999,0x1765,0x5eb8,0x2feb,0x6941,0x7fe2);
load_palettes(pal44,PALETTES+PALOFFSET*59);
}


void NEOGEO_USER showScreen45(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 45 ******************************************/
uint16_t  pal45[16];
setpal(pal45,0x0,0x0,0x2fff,0x6fe9,0x7510,0x988,0x3abd,0x5148,0x4fe2,0x7002,0x7e95,0xa41,0x7e80,0x7124,0x1655,0x425f);
load_palettes(pal45,PALETTES+PALOFFSET*60);
}


void NEOGEO_USER showScreen46(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 46 ******************************************/
uint16_t  pal46[16];
setpal(pal46,0x0,0x0,0x6fd2,0x4bcd,0x1e96,0x336e,0x4951,0x5c22,0x248,0x5ff8,0x3510,0xe81,0x3113,0x545,0x7878,0x6fff);
load_palettes(pal46,PALETTES+PALOFFSET*61);
}


void NEOGEO_USER showScreen47(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 47 ******************************************/
uint16_t  pal47[16];
setpal(pal47,0x0,0x4877,0x6fe9,0x0,0x702e,0x6e81,0x5fe2,0x6e96,0x3510,0x548e,0x2fff,0x2238,0x4a41,0x7bbc,0x545,0x3113);
load_palettes(pal47,PALETTES+PALOFFSET*62);
}


void NEOGEO_USER showScreen48(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 48 ******************************************/
uint16_t  pal48[16];
setpal(pal48,0x0,0x6fe9,0x0,0x1235,0x4941,0x7fe3,0xfff,0x6249,0x6d61,0x3fa1,0xbbb,0x4521,0x1112,0x3766,0x2d86,0x4c26);
load_palettes(pal48,PALETTES+PALOFFSET*63);
}


void NEOGEO_USER showScreen49(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 49 ******************************************/
uint16_t  pal49[16];
setpal(pal49,0x0,0x799a,0x0,0x3e86,0x6fff,0x1766,0x3248,0x6014,0xd61,0x4eeb,0x7433,0x5410,0x4fe2,0x3f91,0x7fe6,0x2931);
load_palettes(pal49,PALETTES+PALOFFSET*64);
}


void NEOGEO_USER showScreen50(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 50 ******************************************/
uint16_t  pal50[16];
setpal(pal50,0x0,0x0,0x38af,0x7fff,0x3acf,0x7555,0x314f,0x336f,0x5dff,0x7aab,0x889,0x548f,0x236,0x7222,0x5bef,0x169f);
load_palettes(pal50,PALETTES+PALOFFSET*65);
}


void NEOGEO_USER showScreen51(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 51 ******************************************/
uint16_t  pal51[16];
setpal(pal51,0x0,0x0,0x3fff,0x369f,0x1dff,0x756a,0x18bf,0x1aab,0x5233,0x7555,0x114f,0x347f,0x7bdf,0x59cf,0xddd,0x7888);
load_palettes(pal51,PALETTES+PALOFFSET*66);
}


void NEOGEO_USER showScreen52(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 52 ******************************************/
uint16_t  pal52[16];
setpal(pal52,0x0,0x3235,0x6fd2,0x0,0xc51,0x1865,0x4731,0x5fe6,0x4013,0x3f81,0x6359,0x4fff,0xe96,0x3fea,0x6a9c,0x2411);
load_palettes(pal52,PALETTES+PALOFFSET*67);
}


void NEOGEO_USER showScreen53(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 53 ******************************************/
uint16_t  pal53[16];
setpal(pal53,0x0,0x763f,0x0,0x7ecf,0x196f,0x185f,0x520f,0x351f,0x5caf,0x7fff,0x352e,0x41d,0x1333,0x7a7f,0x6667,0x1aaa);
load_palettes(pal53,PALETTES+PALOFFSET*68);
}


void NEOGEO_USER showScreen54(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 54 ******************************************/
uint16_t  pal54[16];
setpal(pal54,0x0,0x763f,0x0,0x7fff,0x340f,0x7c9f,0x1a7f,0x5889,0x395f,0x384f,0x452f,0x5fdf,0x1b8f,0x341d,0x2434,0x1ebf);
load_palettes(pal54,PALETTES+PALOFFSET*69);
}


void NEOGEO_USER showScreen55(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 55 ******************************************/
uint16_t  pal55[16];
setpal(pal55,0x0,0x7fff,0x0,0x752f,0x5b9f,0x6667,0x1fdf,0x1aaa,0x1a7f,0x7daf,0x7223,0x641e,0x373f,0x130f,0x196f,0x384f);
load_palettes(pal55,PALETTES+PALOFFSET*70);
}


void NEOGEO_USER showScreen56(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 56 ******************************************/
uint16_t  pal56[16];
setpal(pal56,0x0,0x163f,0x5caf,0x0,0x3a7f,0x7fff,0x196f,0x542d,0x4778,0x141f,0x3ebf,0x1fdf,0x445,0x7b8f,0x3bab,0x774f);
load_palettes(pal56,PALETTES+PALOFFSET*71);
}


void NEOGEO_USER showScreen57(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 57 ******************************************/
uint16_t  pal57[16];
setpal(pal57,0x0,0x3113,0x5fc2,0x7fff,0x596f,0x556,0x3daf,0x4c62,0x721,0x6fe9,0x0,0x340f,0x1fdf,0x373f,0x7b8f,0x7989);
load_palettes(pal57,PALETTES+PALOFFSET*72);
}


void NEOGEO_USER showScreen58(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 58 ******************************************/
uint16_t  pal58[16];
setpal(pal58,0x0,0x0,0x7fff,0x5daf,0x361f,0x7fcf,0x5888,0x3323,0x2658,0x595f,0x5fef,0x6555,0x3c9f,0x3ebf,0x2bab,0x7b7f);
load_palettes(pal58,PALETTES+PALOFFSET*73);
}


void NEOGEO_USER showScreen59(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 59 ******************************************/
uint16_t  pal59[16];
setpal(pal59,0x0,0x1410,0x6fff,0x0,0x2c98,0xc61,0x3434,0x546b,0x2f91,0x831,0x4ff9,0x7137,0x4fe3,0x4bbe,0x4777,0x4013);
load_palettes(pal59,PALETTES+PALOFFSET*74);
}


void NEOGEO_USER showScreen60(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 60 ******************************************/
uint16_t  pal60[16];
setpal(pal60,0x0,0x7fff,0x0,0x373f,0x1c9f,0x4334,0x499a,0x4112,0x3daf,0x395f,0x5a7f,0x340f,0x1ecf,0x7fdf,0x76a,0x1556);
load_palettes(pal60,PALETTES+PALOFFSET*75);
}


void NEOGEO_USER showScreen61(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 61 ******************************************/
uint16_t  pal61[16];
setpal(pal61,0x0,0x5fe3,0x0,0x6fff,0x7013,0x7147,0xe96,0x7941,0x2bab,0x6434,0x311,0x1d61,0x4621,0x3866,0x1fa1,0x5ff9);
load_palettes(pal61,PALETTES+PALOFFSET*76);
}


void NEOGEO_USER showScreen62(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 62 ******************************************/
uint16_t  pal62[16];
setpal(pal62,0x0,0x6013,0x4b51,0xfff,0x1888,0x3136,0x4bbb,0x865,0x7fe9,0x831,0x4e82,0x7fc1,0x2fc6,0x2411,0x6434,0x0);
load_palettes(pal62,PALETTES+PALOFFSET*77);
}


void NEOGEO_USER showScreen63(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 63 ******************************************/
uint16_t  pal63[16];
setpal(pal63,0x0,0x1543,0x6fff,0x0,0x6fd6,0x4d72,0x124,0x7410,0x4012,0x4941,0x7866,0x4baa,0x6feb,0x6159,0x5fb1,0x7136);
load_palettes(pal63,PALETTES+PALOFFSET*78);
}


void NEOGEO_USER showScreen64(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 64 ******************************************/
uint16_t  pal64[16];
setpal(pal64,0x0,0x3113,0x6fea,0x4148,0x5b50,0x1655,0x4e81,0x19aa,0x4731,0x7fe5,0x0,0x411,0xfff,0x7fc1,0x6e95,0x1b65);
load_palettes(pal64,PALETTES+PALOFFSET*79);
}


void NEOGEO_USER showScreen65(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 65 ******************************************/
uint16_t  pal65[16];
setpal(pal65,0x0,0x6fff,0x3d74,0x6014,0x4311,0x0,0xbbb,0x4fe2,0x4777,0x248,0x544,0x4ffa,0xb51,0x4fd6,0xf91,0x5720);
load_palettes(pal65,PALETTES+PALOFFSET*80);
}


void NEOGEO_USER showScreen66(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 66 ******************************************/
uint16_t  pal66[16];
setpal(pal66,0x0,0x0,0x7fff,0x5410,0x5fe1,0x2113,0x3bbc,0x7c51,0x234e,0x5fd6,0x7830,0x3877,0x5ffa,0x4444,0x6138,0x3f91);
load_palettes(pal66,PALETTES+PALOFFSET*81);
}


void NEOGEO_USER showScreen67(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 67 ******************************************/
uint16_t  pal67[16];
setpal(pal67,0x0,0x6fc1,0x0,0x6fe6,0x2125,0x699a,0x5ea5,0x5a74,0x2fea,0x1ffe,0x3d70,0x1112,0x2511,0x655,0x941,0x4248);
load_palettes(pal67,PALETTES+PALOFFSET*82);
}


void NEOGEO_USER showScreen68(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 68 ******************************************/
uint16_t  pal68[16];
setpal(pal68,0x0,0x2989,0x3cbc,0x0,0x4731,0x6fe9,0x6fff,0x7e80,0x6137,0x1433,0x3755,0x5ea5,0x1b61,0x5310,0x5012,0x4fd2);
load_palettes(pal68,PALETTES+PALOFFSET*83);
}


void NEOGEO_USER showScreen69(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 69 ******************************************/
uint16_t  pal69[16];
setpal(pal69,0x0,0x3113,0x6fff,0x2b51,0x7fd5,0x3cbb,0x6138,0x0,0x731,0x1f91,0x1c84,0x5ffa,0x4311,0x7fd1,0x4555,0x5888);
load_palettes(pal69,PALETTES+PALOFFSET*84);
}


void NEOGEO_USER showScreen70(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 70 ******************************************/
uint16_t  pal70[16];
setpal(pal70,0x0,0x6411,0xe81,0x7fff,0x6545,0x5b64,0x941,0x4fd1,0x0,0x5136,0xfa5,0x113,0xaaa,0x5eeb,0x1259,0x4fe7);
load_palettes(pal70,PALETTES+PALOFFSET*85);
}


void NEOGEO_USER showScreen71(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 71 ******************************************/
uint16_t  pal71[16];
setpal(pal71,0x0,0x7714,0x0,0x3fd1,0x6125,0x4fe8,0xfff,0x3e77,0x888,0x7248,0x2c61,0x555,0xc34,0x5bbb,0x4521,0x112);
load_palettes(pal71,PALETTES+PALOFFSET*86);
}


void NEOGEO_USER showScreen72(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 72 ******************************************/
uint16_t  pal72[16];
setpal(pal72,0x0,0x1daf,0x0,0x7fff,0x7ebf,0x563f,0x3323,0x384f,0x5fdf,0x5aab,0x7a7f,0x5fef,0x541f,0x5667,0x3c8f,0x596f);
load_palettes(pal72,PALETTES+PALOFFSET*87);
}


void NEOGEO_USER showScreen73(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 73 ******************************************/
uint16_t  pal73[16];
setpal(pal73,0x0,0x7002,0x1baa,0x6ffe,0x5fe3,0x3fa1,0x5c71,0x3856,0x6fea,0x1fa5,0x3225,0x5310,0x0,0x725b,0x6941,0x2621);
load_palettes(pal73,PALETTES+PALOFFSET*88);
}


void NEOGEO_USER showScreen74(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 74 ******************************************/
uint16_t  pal74[16];
setpal(pal74,0x0,0x5fe1,0x1c60,0x248,0x1410,0x6fff,0x0,0x4fb5,0x1830,0x6fe7,0x2f91,0x3114,0x5ffa,0x7d75,0x3aab,0x2856);
load_palettes(pal74,PALETTES+PALOFFSET*89);
}


void NEOGEO_USER showScreen75(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 75 ******************************************/
uint16_t  pal75[16];
setpal(pal75,0x0,0x2d71,0x0,0x5ffa,0x148c,0x3126,0x7fe1,0xfff,0x5730,0x1410,0x1fa5,0x4a51,0xb89,0x2fa2,0x2735,0x6fe6);
load_palettes(pal75,PALETTES+PALOFFSET*90);
}


void NEOGEO_USER showScreen76(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 76 ******************************************/
uint16_t  pal76[16];
setpal(pal76,0x0,0x1c87,0x5ffa,0x0,0x4fe2,0x6e81,0x6fff,0x5410,0x1fa4,0x28ac,0x7fe6,0x2238,0x1830,0x1756,0x2114,0x2b51);
load_palettes(pal76,PALETTES+PALOFFSET*91);
}


void NEOGEO_USER showScreen77(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 77 ******************************************/
uint16_t  pal77[16];
setpal(pal77,0x0,0x5ffa,0x0,0x4c36,0x3114,0x5fe1,0x6fff,0x1fa5,0x6aac,0x4fe6,0x248,0x6767,0xc61,0x5410,0x3830,0x3f91);
load_palettes(pal77,PALETTES+PALOFFSET*92);
}


void NEOGEO_USER showScreen78(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 78 ******************************************/
uint16_t  pal78[16];
setpal(pal78,0x0,0xd71,0x0,0x3b38,0x4ffd,0x5fe1,0x656,0xb99,0x6911,0xfa3,0x4fe7,0x4137,0x311,0x3940,0x258d,0x1620);
load_palettes(pal78,PALETTES+PALOFFSET*93);
}


void NEOGEO_USER showScreen79(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 79 ******************************************/
uint16_t  pal79[16];
setpal(pal79,0x0,0x2200,0x4420,0x7fff,0x2d71,0x5887,0x2baa,0x3a50,0x0,0x2655,0x630,0x840,0x6310,0x3edc,0x5fe0,0x333);
load_palettes(pal79,PALETTES+PALOFFSET*94);
}


void NEOGEO_USER showScreen80(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 80 ******************************************/
uint16_t  pal80[16];
setpal(pal80,0x0,0x310,0x840,0x7fff,0x0,0x333,0x7a99,0x6520,0x6d81,0x1ccc,0x3655,0x6a50,0x3fed,0x6fe0,0x6877,0x2ecb);
load_palettes(pal80,PALETTES+PALOFFSET*95);
}


void NEOGEO_USER showScreen81(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 81 ******************************************/
uint16_t  pal81[16];
setpal(pal81,0x0,0x3433,0x7fff,0x0,0x1b60,0x3766,0xccc,0x4fd0,0x6eed,0x5dca,0x2a99,0x6840,0x420,0x630,0xe92,0x210);
load_palettes(pal81,PALETTES+PALOFFSET*96);
}


void NEOGEO_USER showScreen82(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 82 ******************************************/
uint16_t  pal82[16];
setpal(pal82,0x0,0x7fff,0x3323,0x656,0xcbb,0x878,0x0,0x310,0x3fdc,0x4a99,0x5fc0,0x1eca,0x4630,0x5950,0x6ffe,0x2f80);
load_palettes(pal82,PALETTES+PALOFFSET*97);
}


void NEOGEO_USER showScreen83(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 83 ******************************************/
uint16_t  pal83[16];
setpal(pal83,0x0,0x6fff,0x1212,0x2840,0x2f80,0x656,0x3434,0x3caa,0x0,0x7c22,0x2410,0x6fed,0x5877,0x5fd0,0x3ecb,0x2a89);
load_palettes(pal83,PALETTES+PALOFFSET*98);
}


void NEOGEO_USER showScreen84(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 84 ******************************************/
uint16_t  pal84[16];
setpal(pal84,0x0,0x6fff,0x2410,0x5950,0x4630,0x767,0x2dbb,0x3fec,0x4100,0x3b99,0x1988,0x7fd1,0x0,0x4223,0xf90,0x1545);
load_palettes(pal84,PALETTES+PALOFFSET*99);
}


void NEOGEO_USER showScreen85(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 85 ******************************************/
uint16_t  pal85[16];
setpal(pal85,0x0,0x2fb5,0x0,0x4544,0x5d86,0x248,0x7fe2,0x2867,0x4e91,0x7ffe,0x7fe9,0x4b61,0x1410,0x1830,0x7013,0x2bab);
load_palettes(pal85,PALETTES+PALOFFSET*100);
}


void NEOGEO_USER showScreen86(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 86 ******************************************/
uint16_t  pal86[16];
setpal(pal86,0x0,0x2410,0x5aaa,0x5dbf,0x7d90,0x531f,0x4888,0x5fff,0x0,0x7a7f,0x5740,0x3655,0x574f,0x1ddc,0x1669,0x4333);
load_palettes(pal86,PALETTES+PALOFFSET*101);
}


void NEOGEO_USER showScreen87(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 87 ******************************************/
uint16_t  pal87[16];
setpal(pal87,0x0,0x5e91,0x77ac,0x7fd1,0x0,0x4ffb,0x3300,0x7237,0x4fe9,0x4a78,0x1940,0x1fa4,0xc61,0x1620,0x6fd6,0x6fff);
load_palettes(pal87,PALETTES+PALOFFSET*102);
}


void NEOGEO_USER showScreen88(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 88 ******************************************/
uint16_t  pal88[16];
setpal(pal88,0x0,0x6d71,0x1ffe,0x0,0x5fe5,0x2300,0x5840,0x6a9b,0x2448,0x2fa2,0x3b50,0x1620,0x6fea,0x6fe1,0x3fa5,0x7c64);
load_palettes(pal88,PALETTES+PALOFFSET*103);
}


void NEOGEO_USER showScreen89(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 89 ******************************************/
uint16_t  pal89[16];
setpal(pal89,0x0,0x3c60,0x4000,0x7fe5,0x6239,0x7830,0x7fc7,0xe74,0x5410,0x298a,0xfff,0x2f91,0x4fb4,0x5ffb,0x4fe1,0x6fe9);
load_palettes(pal89,PALETTES+PALOFFSET*104);
}


void NEOGEO_USER showScreen90(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 90 ******************************************/
uint16_t  pal90[16];
setpal(pal90,0x0,0x2fa2,0x0,0x7fe9,0x1a50,0x3b9b,0x146b,0xd71,0x3ffe,0x7225,0x6300,0x3720,0x7fe3,0x6e74,0x4766,0x6fb6);
load_palettes(pal90,PALETTES+PALOFFSET*105);
}


void NEOGEO_USER showScreen91(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 91 ******************************************/
uint16_t  pal91[16];
setpal(pal91,0x0,0x1a50,0x4347,0x410,0x5ffa,0x1fa4,0x0,0x1d23,0x698a,0x7fd8,0x5730,0x4fe1,0xfa1,0x5fe4,0xfff,0x1d71);
load_palettes(pal91,PALETTES+PALOFFSET*106);
}


void NEOGEO_USER showScreen92(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 92 ******************************************/
uint16_t  pal92[16];
setpal(pal92,0x0,0x2fa4,0x1225,0x4ffd,0x0,0x1b25,0x5ade,0x6300,0x7fe8,0x4c9a,0x3720,0x6fd2,0x5a50,0x158b,0x5765,0x1e81);
load_palettes(pal92,PALETTES+PALOFFSET*107);
}


void NEOGEO_USER showScreen93(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 93 ******************************************/
uint16_t  pal93[16];
setpal(pal93,0x0,0x5b60,0x0,0x4ffa,0xa64,0x4fd2,0x1830,0x5467,0x3a79,0x7fd6,0x3424,0x1f94,0xccc,0x4e91,0x7ffe,0x410);
load_palettes(pal93,PALETTES+PALOFFSET*108);
}
