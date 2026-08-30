

void NEOGEO_USER showScreen1(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 1 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal1[16];
setpal(pal1,0x0,0x6158,0x28a3,0x4998,0x49a,0x1653,0x105a,0x3222,0x4cb8,0x1ddc,0x6456,0x8ce,0x217d,0x4363,0x249d,0x4b63);
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
setBACKDROP(backdrop);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS1_1,spal1_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS1_2,spal1_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS1_3,spal1_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS1_4,spal1_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS1_5,spal1_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS1_6,spal1_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS1_7,spal1_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS1_8,spal1_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS1_9,spal1_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS1_10,spal1_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS1_11,spal1_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS1_12,spal1_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS1_13,spal1_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS1_14,spal1_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS1_15,spal1_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS1_16,spal1_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen2(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 2 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal2[16];
setpal(pal2,0x0,0x4eff,0x305c,0x4158,0x1cde,0x37df,0x58bd,0x23af,0x64bf,0x318e,0x5159,0x3bef,0x28aa,0x548c,0x3478,0x79ce);
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
setBACKDROP(backdrop);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS2_1,spal2_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS2_2,spal2_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS2_3,spal2_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS2_4,spal2_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS2_5,spal2_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS2_6,spal2_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS2_7,spal2_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS2_8,spal2_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS2_9,spal2_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS2_10,spal2_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS2_11,spal2_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS2_12,spal2_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS2_13,spal2_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS2_14,spal2_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS2_15,spal2_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS2_16,spal2_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen3(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 3 ******************************************/
uint16_t  pal3[16];
setpal(pal3,0x0,0x6112,0x3d00,0x7fc3,0x6fb7,0x4951,0x5800,0x4f09,0x3138,0x5492,0x452,0x2e83,0x4bcc,0x6411,0x0,0x7fff);
load_palettes(pal3,PALETTES+PALOFFSET*18);
}


void NEOGEO_USER showScreen4(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 4 ******************************************/
uint16_t  pal4[16];
setpal(pal4,0x0,0x6112,0x3d00,0x7fc3,0x6fb7,0x4951,0x5800,0x4f09,0x3138,0x5492,0x452,0x2e83,0x4bcc,0x6411,0x0,0x7fff);
load_palettes(pal4,PALETTES+PALOFFSET*19);
}


void NEOGEO_USER showScreen5(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 5 ******************************************/
uint16_t  pal5[16];
setpal(pal5,0x0,0x6112,0x3d00,0x7fc3,0x6fb7,0x4951,0x5800,0x4f09,0x3138,0x5492,0x452,0x2e83,0x4bcc,0x6411,0x0,0x7fff);
load_palettes(pal5,PALETTES+PALOFFSET*20);
}


void NEOGEO_USER showScreen6(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 6 ******************************************/
uint16_t  pal6[16];
setpal(pal6,0x0,0x6112,0x3d00,0x7fc3,0x6fb7,0x4951,0x5800,0x4f09,0x3138,0x5492,0x452,0x2e83,0x4bcc,0x6411,0x0,0x7fff);
load_palettes(pal6,PALETTES+PALOFFSET*21);
}


void NEOGEO_USER showScreen7(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 7 ******************************************/
uint16_t  pal7[16];
setpal(pal7,0x0,0x6112,0x3d00,0x7fc3,0x6fb7,0x4951,0x5800,0x4f09,0x3138,0x5492,0x452,0x2e83,0x4bcc,0x6411,0x0,0x7fff);
load_palettes(pal7,PALETTES+PALOFFSET*22);
}


void NEOGEO_USER showScreen8(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 8 ******************************************/
uint16_t  pal8[16];
setpal(pal8,0x0,0x6112,0x3d00,0x7fc3,0x6fb7,0x4951,0x5800,0x4f09,0x3138,0x5492,0x452,0x2e83,0x4bcc,0x6411,0x0,0x7fff);
load_palettes(pal8,PALETTES+PALOFFSET*23);
}


void NEOGEO_USER showScreen9(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 9 ******************************************/
uint16_t  pal9[16];
setpal(pal9,0x0,0x6112,0x3d00,0x7fc3,0x6fb7,0x4951,0x5800,0x4f09,0x3138,0x5492,0x452,0x2e83,0x4bcc,0x6411,0x0,0x7fff);
load_palettes(pal9,PALETTES+PALOFFSET*24);
}


void NEOGEO_USER showScreen10(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 10 ******************************************/
uint16_t  pal10[16];
setpal(pal10,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal10,PALETTES+PALOFFSET*25);
}


void NEOGEO_USER showScreen11(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 11 ******************************************/
uint16_t  pal11[16];
setpal(pal11,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal11,PALETTES+PALOFFSET*26);
}


void NEOGEO_USER showScreen12(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 12 ******************************************/
uint16_t  pal12[16];
setpal(pal12,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal12,PALETTES+PALOFFSET*27);
}


void NEOGEO_USER showScreen13(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 13 ******************************************/
uint16_t  pal13[16];
setpal(pal13,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal13,PALETTES+PALOFFSET*28);
}


void NEOGEO_USER showScreen14(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 14 ******************************************/
uint16_t  pal14[16];
setpal(pal14,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal14,PALETTES+PALOFFSET*29);
}


void NEOGEO_USER showScreen15(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 15 ******************************************/
uint16_t  pal15[16];
setpal(pal15,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal15,PALETTES+PALOFFSET*30);
}


void NEOGEO_USER showScreen16(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 16 ******************************************/
uint16_t  pal16[16];
setpal(pal16,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal16,PALETTES+PALOFFSET*31);
}


void NEOGEO_USER showScreen17(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 17 ******************************************/
uint16_t  pal17[16];
setpal(pal17,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal17,PALETTES+PALOFFSET*32);
}


void NEOGEO_USER showScreen18(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 18 ******************************************/
uint16_t  pal18[16];
setpal(pal18,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal18,PALETTES+PALOFFSET*33);
}


void NEOGEO_USER showScreen19(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 19 ******************************************/
uint16_t  pal19[16];
setpal(pal19,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal19,PALETTES+PALOFFSET*34);
}


void NEOGEO_USER showScreen20(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 20 ******************************************/
uint16_t  pal20[16];
setpal(pal20,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal20,PALETTES+PALOFFSET*35);
}


void NEOGEO_USER showScreen21(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 21 ******************************************/
uint16_t  pal21[16];
setpal(pal21,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal21,PALETTES+PALOFFSET*36);
}


void NEOGEO_USER showScreen22(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 22 ******************************************/
uint16_t  pal22[16];
setpal(pal22,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal22,PALETTES+PALOFFSET*37);
}


void NEOGEO_USER showScreen23(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 23 ******************************************/
uint16_t  pal23[16];
setpal(pal23,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal23,PALETTES+PALOFFSET*38);
}


void NEOGEO_USER showScreen24(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 24 ******************************************/
uint16_t  pal24[16];
setpal(pal24,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal24,PALETTES+PALOFFSET*39);
}


void NEOGEO_USER showScreen25(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 25 ******************************************/
uint16_t  pal25[16];
setpal(pal25,0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff);
load_palettes(pal25,PALETTES+PALOFFSET*40);
}

static const NGPaletteAsset ng_screen_palette_assets[] = {
    {1,16,{0x0,0x6158,0x28a3,0x4998,0x49a,0x1653,0x105a,0x3222,0x4cb8,0x1ddc,0x6456,0x8ce,0x217d,0x4363,0x249d,0x4b63}},
    {2,17,{0x0,0x4eff,0x305c,0x4158,0x1cde,0x37df,0x58bd,0x23af,0x64bf,0x318e,0x5159,0x3bef,0x28aa,0x548c,0x3478,0x79ce}},
    {3,18,{0x0,0x6112,0x3d00,0x7fc3,0x6fb7,0x4951,0x5800,0x4f09,0x3138,0x5492,0x452,0x2e83,0x4bcc,0x6411,0x0,0x7fff}},
    {4,19,{0x0,0x6112,0x3d00,0x7fc3,0x6fb7,0x4951,0x5800,0x4f09,0x3138,0x5492,0x452,0x2e83,0x4bcc,0x6411,0x0,0x7fff}},
    {5,20,{0x0,0x6112,0x3d00,0x7fc3,0x6fb7,0x4951,0x5800,0x4f09,0x3138,0x5492,0x452,0x2e83,0x4bcc,0x6411,0x0,0x7fff}},
    {6,21,{0x0,0x6112,0x3d00,0x7fc3,0x6fb7,0x4951,0x5800,0x4f09,0x3138,0x5492,0x452,0x2e83,0x4bcc,0x6411,0x0,0x7fff}},
    {7,22,{0x0,0x6112,0x3d00,0x7fc3,0x6fb7,0x4951,0x5800,0x4f09,0x3138,0x5492,0x452,0x2e83,0x4bcc,0x6411,0x0,0x7fff}},
    {8,23,{0x0,0x6112,0x3d00,0x7fc3,0x6fb7,0x4951,0x5800,0x4f09,0x3138,0x5492,0x452,0x2e83,0x4bcc,0x6411,0x0,0x7fff}},
    {9,24,{0x0,0x6112,0x3d00,0x7fc3,0x6fb7,0x4951,0x5800,0x4f09,0x3138,0x5492,0x452,0x2e83,0x4bcc,0x6411,0x0,0x7fff}},
    {10,25,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
    {11,26,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
    {12,27,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
    {13,28,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
    {14,29,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
    {15,30,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
    {16,31,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
    {17,32,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
    {18,33,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
    {19,34,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
    {20,35,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
    {21,36,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
    {22,37,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
    {23,38,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
    {24,39,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
    {25,40,{0x0,0x111,0x1ea3,0xd22,0x511,0x2653,0x445,0x5811,0x1347,0x4123,0x6952,0x1888,0x3984,0x3431,0x0,0x7fff}},
};
const uint16_t ng_screen_palette_count = 25;

uint8_t NEOGEO_USER ng_load_screen_palette(uint16_t screen_id) {
    return ng_palette_load_asset(ng_screen_palette_assets, ng_screen_palette_count, screen_id);
}

const NGArtAsset ng_screen_art_assets[] = {
    {1,NG_ART_TYPE_BACKGROUND,16,0,255,16,16,16,16,0,0,256,256},
    {2,NG_ART_TYPE_BACKGROUND,17,256,511,16,16,16,16,0,0,256,256},
    {3,NG_ART_TYPE_SPRITE,18,531,767,10,15,15,16,48,16,160,240},
    {4,NG_ART_TYPE_SPRITE,19,868,1023,8,10,10,16,64,96,112,160},
    {5,NG_ART_TYPE_SPRITE,20,1043,1279,10,15,15,16,48,16,160,240},
    {6,NG_ART_TYPE_SPRITE,21,1396,1535,8,9,9,16,64,112,112,144},
    {7,NG_ART_TYPE_SPRITE,22,1555,1791,10,15,15,16,48,16,160,240},
    {8,NG_ART_TYPE_SPRITE,23,1908,2047,8,9,9,16,64,112,112,144},
    {9,NG_ART_TYPE_SPRITE,24,2263,2303,2,3,3,16,112,208,8,40},
    {10,NG_ART_TYPE_SPRITE,25,2403,2559,10,10,10,16,48,96,144,160},
    {11,NG_ART_TYPE_SPRITE,26,2563,2815,10,16,16,16,48,0,146,256},
    {12,NG_ART_TYPE_SPRITE,27,2832,3071,16,15,15,16,0,16,240,240},
    {13,NG_ART_TYPE_SPRITE,28,3073,3327,14,16,16,16,16,0,197,256},
    {14,NG_ART_TYPE_SPRITE,29,3332,3583,8,16,16,16,64,0,124,256},
    {15,NG_ART_TYPE_SPRITE,30,3586,3839,12,16,16,16,32,0,190,256},
    {16,NG_ART_TYPE_SPRITE,31,3841,4095,14,16,16,16,16,0,205,256},
    {17,NG_ART_TYPE_SPRITE,32,4098,4351,11,16,16,16,32,0,161,256},
    {18,NG_ART_TYPE_SPRITE,33,4550,4607,4,4,4,16,96,192,56,56},
    {19,NG_ART_TYPE_SPRITE,34,4724,4863,8,9,9,16,64,112,128,144},
    {20,NG_ART_TYPE_SPRITE,35,4980,5119,8,9,9,16,64,112,112,144},
    {21,NG_ART_TYPE_SPRITE,36,5236,5375,8,9,9,16,64,112,104,144},
    {22,NG_ART_TYPE_SPRITE,37,5477,5631,6,10,10,16,80,96,72,160},
    {23,NG_ART_TYPE_SPRITE,38,5863,5887,2,2,2,16,112,224,32,32},
    {24,NG_ART_TYPE_SPRITE,39,6102,6143,4,3,3,16,96,208,40,40},
    {25,NG_ART_TYPE_SPRITE,40,6277,6399,6,8,8,16,80,128,80,120},
};
const uint16_t ng_screen_art_asset_count = 25;

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
};
const uint16_t ng_screen_count = 25;
