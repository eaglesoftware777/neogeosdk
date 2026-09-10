#ifndef ARTBOX_SPRITE_META_H
#define ARTBOX_SPRITE_META_H

#include <stdint.h>

typedef struct {
    uint16_t tile_base;
    uint8_t palette_bank;
    uint8_t strips;
    uint8_t active_rows;
    uint8_t tile_col_start;
    uint8_t tile_row_start;
    uint8_t x_pad;
    uint8_t y_pad;
    uint16_t content_width;
    uint16_t content_height;
    uint8_t mode;
    uint8_t category;
    /* Tiles per row of this asset's canvas.  Row n of the
     * artwork starts tile_stride tiles after row n-1, so a
     * sprite group or character bound to this asset has to be
     * given this as its tile stride.  It is not always 16: it
     * is the canvas width in tiles, and an asset imported onto
     * a narrower canvas has a narrower stride.  Assuming 16
     * reads each row from 16 tiles on instead of this many,
     * which draws whatever is at that address - the right
     * palette over the wrong art. */
    uint8_t tile_stride;
} NGSpriteAssetMeta;

#define NG_ASSET_MODE_SCREEN 0
#define NG_ASSET_MODE_SPRITE 1

#define NG_ASSET_CATEGORY_BACKGROUND 0
#define NG_ASSET_CATEGORY_MAIN_CHARACTER 1
#define NG_ASSET_CATEGORY_OPPONENT 2
#define NG_ASSET_CATEGORY_NPC 3

#define NG_ASSET_META_COUNT 173

/* Hardware sprite strips each asset needs, by 1-based asset id. */
#define NG_ASSET_STRIPS_1 16  /* 0.png */
#define NG_ASSET_STRIPS_2 16  /* 1.png */
#define NG_ASSET_STRIPS_3 12  /* sprite_001_r01_c01.png */
#define NG_ASSET_STRIPS_4 12  /* sprite_002_r01_c02.png */
#define NG_ASSET_STRIPS_5 12  /* sprite_003_r01_c03.png */
#define NG_ASSET_STRIPS_6 12  /* sprite_004_r01_c04.png */
#define NG_ASSET_STRIPS_7 12  /* sprite_005_r01_c05.png */
#define NG_ASSET_STRIPS_8 12  /* sprite_006_r01_c06.png */
#define NG_ASSET_STRIPS_9 12  /* sprite_007_r01_c07.png */
#define NG_ASSET_STRIPS_10 12  /* sprite_008_r01_c08.png */
#define NG_ASSET_STRIPS_11 12  /* sprite_009_r01_c09.png */
#define NG_ASSET_STRIPS_12 12  /* sprite_010_r01_c10.png */
#define NG_ASSET_STRIPS_13 12  /* sprite_011_r01_c11.png */
#define NG_ASSET_STRIPS_14 12  /* sprite_012_r01_c12.png */
#define NG_ASSET_STRIPS_15 14  /* sprite_013_r02_c01.png */
#define NG_ASSET_STRIPS_16 16  /* sprite_014_r02_c02.png */
#define NG_ASSET_STRIPS_17 14  /* sprite_015_r02_c03.png */
#define NG_ASSET_STRIPS_18 14  /* sprite_016_r02_c04.png */
#define NG_ASSET_STRIPS_19 14  /* sprite_017_r02_c05.png */
#define NG_ASSET_STRIPS_20 14  /* sprite_018_r02_c06.png */
#define NG_ASSET_STRIPS_21 14  /* sprite_019_r02_c07.png */
#define NG_ASSET_STRIPS_22 14  /* sprite_020_r02_c08.png */
#define NG_ASSET_STRIPS_23 14  /* sprite_021_r02_c09.png */
#define NG_ASSET_STRIPS_24 14  /* sprite_022_r02_c10.png */
#define NG_ASSET_STRIPS_25 12  /* sprite_023_r03_c01.png */
#define NG_ASSET_STRIPS_26 16  /* sprite_024_r03_c02.png */
#define NG_ASSET_STRIPS_27 16  /* sprite_025_r03_c03.png */
#define NG_ASSET_STRIPS_28 16  /* sprite_026_r03_c04.png */
#define NG_ASSET_STRIPS_29 16  /* sprite_027_r03_c05.png */
#define NG_ASSET_STRIPS_30 16  /* sprite_028_r03_c06.png */
#define NG_ASSET_STRIPS_31 16  /* sprite_029_r03_c07.png */
#define NG_ASSET_STRIPS_32 16  /* sprite_030_r03_c08.png */
#define NG_ASSET_STRIPS_33 16  /* sprite_031_r04_c01.png */
#define NG_ASSET_STRIPS_34 16  /* sprite_032_r04_c02.png */
#define NG_ASSET_STRIPS_35 14  /* sprite_033_r04_c03.png */
#define NG_ASSET_STRIPS_36 16  /* sprite_034_r04_c04.png */
#define NG_ASSET_STRIPS_37 16  /* sprite_035_r04_c05.png */
#define NG_ASSET_STRIPS_38 16  /* sprite_036_r04_c06.png */
#define NG_ASSET_STRIPS_39 14  /* sprite_037_r04_c07.png */
#define NG_ASSET_STRIPS_40 12  /* sprite_038_r04_c08.png */
#define NG_ASSET_STRIPS_41 16  /* sprite_039_r04_c09.png */
#define NG_ASSET_STRIPS_42 12  /* sprite_040_r04_c10.png */
#define NG_ASSET_STRIPS_43 16  /* sprite_041_r04_c11.png */
#define NG_ASSET_STRIPS_44 12  /* sprite_042_r05_c01.png */
#define NG_ASSET_STRIPS_45 16  /* sprite_043_r05_c02.png */
#define NG_ASSET_STRIPS_46 4  /* sprite_044_r05_c03.png */
#define NG_ASSET_STRIPS_47 12  /* sprite_045_r05_c04.png */
#define NG_ASSET_STRIPS_48 6  /* sprite_046_r05_c05.png */
#define NG_ASSET_STRIPS_49 16  /* sprite_047_r05_c06.png */
#define NG_ASSET_STRIPS_50 14  /* sprite_048_r05_c07.png */
#define NG_ASSET_STRIPS_51 8  /* sprite_049_r05_c08.png */
#define NG_ASSET_STRIPS_52 16  /* sprite_050_r05_c09.png */
#define NG_ASSET_STRIPS_53 16  /* sprite_051_r05_c10.png */
#define NG_ASSET_STRIPS_54 16  /* sprite_052_r05_c11.png */
#define NG_ASSET_STRIPS_55 16  /* sprite_053_r05_c12.png */
#define NG_ASSET_STRIPS_56 16  /* sprite_054_r06_c01.png */
#define NG_ASSET_STRIPS_57 16  /* sprite_055_r06_c02.png */
#define NG_ASSET_STRIPS_58 16  /* sprite_056_r06_c03.png */
#define NG_ASSET_STRIPS_59 14  /* sprite_057_r06_c04.png */
#define NG_ASSET_STRIPS_60 14  /* sprite_058_r06_c05.png */
#define NG_ASSET_STRIPS_61 14  /* sprite_059_r06_c06.png */
#define NG_ASSET_STRIPS_62 6  /* sprite_060_r06_c07.png */
#define NG_ASSET_STRIPS_63 10  /* sprite_061_r06_c08.png */
#define NG_ASSET_STRIPS_64 12  /* sprite_062_r06_c09.png */
#define NG_ASSET_STRIPS_65 10  /* sprite_063_r06_c10.png */
#define NG_ASSET_STRIPS_66 10  /* sprite_064_r06_c11.png */
#define NG_ASSET_STRIPS_67 4  /* sprite_065_r06_c12.png */
#define NG_ASSET_STRIPS_68 14  /* sprite_066_r07_c01.png */
#define NG_ASSET_STRIPS_69 14  /* sprite_067_r07_c02.png */
#define NG_ASSET_STRIPS_70 14  /* sprite_068_r07_c03.png */
#define NG_ASSET_STRIPS_71 14  /* sprite_069_r07_c04.png */
#define NG_ASSET_STRIPS_72 10  /* sprite_070_r07_c05.png */
#define NG_ASSET_STRIPS_73 12  /* sprite_071_r07_c06.png */
#define NG_ASSET_STRIPS_74 12  /* sprite_072_r07_c07.png */
#define NG_ASSET_STRIPS_75 12  /* sprite_073_r07_c08.png */
#define NG_ASSET_STRIPS_76 12  /* sprite_074_r07_c09.png */
#define NG_ASSET_STRIPS_77 12  /* sprite_075_r07_c10.png */
#define NG_ASSET_STRIPS_78 16  /* sprite_076_r07_c11.png */
#define NG_ASSET_STRIPS_79 14  /* sprite_077_r07_c12.png */
#define NG_ASSET_STRIPS_80 14  /* sprite_078_r07_c13.png */
#define NG_ASSET_STRIPS_81 8  /* sprite_079_r07_c14.png */
#define NG_ASSET_STRIPS_82 14  /* sprite_080_r08_c01.png */
#define NG_ASSET_STRIPS_83 14  /* sprite_081_r08_c02.png */
#define NG_ASSET_STRIPS_84 14  /* sprite_082_r08_c03.png */
#define NG_ASSET_STRIPS_85 14  /* sprite_083_r08_c04.png */
#define NG_ASSET_STRIPS_86 14  /* sprite_084_r08_c05.png */
#define NG_ASSET_STRIPS_87 16  /* sprite_085_r08_c06.png */
#define NG_ASSET_STRIPS_88 14  /* sprite_086_r08_c07.png */
#define NG_ASSET_STRIPS_89 6  /* sprite_040.png */
#define NG_ASSET_STRIPS_90 6  /* sprite_041.png */
#define NG_ASSET_STRIPS_91 10  /* sprite_048.png */
#define NG_ASSET_STRIPS_92 10  /* sprite_050.png */
#define NG_ASSET_STRIPS_93 4  /* sprite_062.png */
#define NG_ASSET_STRIPS_94 16  /* 1.png */
#define NG_ASSET_STRIPS_95 16  /* 2.png */
#define NG_ASSET_STRIPS_96 16  /* 3.png */
#define NG_ASSET_STRIPS_97 16  /* 4.png */
#define NG_ASSET_STRIPS_98 16  /* 5.png */
#define NG_ASSET_STRIPS_99 16  /* 6.png */
#define NG_ASSET_STRIPS_100 16  /* 7.png */
#define NG_ASSET_STRIPS_101 16  /* 1.png */
#define NG_ASSET_STRIPS_102 16  /* 2.png */
#define NG_ASSET_STRIPS_103 16  /* 3.png */
#define NG_ASSET_STRIPS_104 16  /* 4.png */
#define NG_ASSET_STRIPS_105 16  /* 5.png */
#define NG_ASSET_STRIPS_106 16  /* 6.png */
#define NG_ASSET_STRIPS_107 16  /* 7.png */
#define NG_ASSET_STRIPS_108 12  /* 8.png */
#define NG_ASSET_STRIPS_109 10  /* 9.png */
#define NG_ASSET_STRIPS_110 12  /* cat_01.png */
#define NG_ASSET_STRIPS_111 12  /* cat_02.png */
#define NG_ASSET_STRIPS_112 12  /* cat_03.png */
#define NG_ASSET_STRIPS_113 12  /* cat_04.png */
#define NG_ASSET_STRIPS_114 12  /* cat_05.png */
#define NG_ASSET_STRIPS_115 12  /* cat_06.png */
#define NG_ASSET_STRIPS_116 12  /* cat_07.png */
#define NG_ASSET_STRIPS_117 12  /* cat_08.png */
#define NG_ASSET_STRIPS_118 12  /* cat_09.png */
#define NG_ASSET_STRIPS_119 12  /* cat_10.png */
#define NG_ASSET_STRIPS_120 12  /* cat_11.png */
#define NG_ASSET_STRIPS_121 12  /* cat_12.png */
#define NG_ASSET_STRIPS_122 4  /* zz_shooter_enemy.png */
#define NG_ASSET_STRIPS_123 2  /* zz_shooter_enemy_bullet.png */
#define NG_ASSET_STRIPS_124 4  /* zz_shooter_explosion.png */
#define NG_ASSET_STRIPS_125 2  /* zz_shooter_player_bullet.png */
#define NG_ASSET_STRIPS_126 4  /* zz_shooter_ship.png */
#define NG_ASSET_STRIPS_127 4  /* zz_shooter_ship_alt.png */
#define NG_ASSET_STRIPS_128 8  /* zzz_ufo_blue.png */
#define NG_ASSET_STRIPS_129 8  /* zzz_ufo_green.png */
#define NG_ASSET_STRIPS_130 8  /* zzz_ufo_pink.png */
#define NG_ASSET_STRIPS_131 8  /* zzz_ufo_yellow.png */
#define NG_ASSET_STRIPS_132 8  /* zzzz_duck_back.png */
#define NG_ASSET_STRIPS_133 8  /* zzzz_duck_target_brown.png */
#define NG_ASSET_STRIPS_134 8  /* zzzz_duck_target_white.png */
#define NG_ASSET_STRIPS_135 8  /* zzzz_duck_target_yellow.png */
#define NG_ASSET_STRIPS_136 2  /* zzzzz_hitbox.png */
#define NG_ASSET_STRIPS_137 1  /* zzzzzz_particle_dust.png */
#define NG_ASSET_STRIPS_138 1  /* zzzzzz_particle_explosion.png */
#define NG_ASSET_STRIPS_139 1  /* zzzzzz_particle_hitspark.png */
#define NG_ASSET_STRIPS_140 1  /* zzzzzz_particle_magic.png */
#define NG_ASSET_STRIPS_141 1  /* zzzzzz_particle_smoke.png */
#define NG_ASSET_STRIPS_142 16  /* zzzzzzz_ssg_starfield.png */
#define NG_ASSET_STRIPS_143 8  /* zzzzzzzz_enemyship_blue.png */
#define NG_ASSET_STRIPS_144 8  /* zzzzzzzz_enemyship_green.png */
#define NG_ASSET_STRIPS_145 8  /* zzzzzzzz_enemyship_pink.png */
#define NG_ASSET_STRIPS_146 6  /* zzzzzzzz_player_vessel.png */
#define NG_ASSET_STRIPS_147 4  /* zzzzzzzzz_balloon_blue.png */
#define NG_ASSET_STRIPS_148 4  /* zzzzzzzzz_balloon_green.png */
#define NG_ASSET_STRIPS_149 4  /* zzzzzzzzz_balloon_red.png */
#define NG_ASSET_STRIPS_150 4  /* zzzzzzzzz_balloon_yellow.png */
#define NG_ASSET_STRIPS_151 8  /* zzzzzzzzzz_sky_boss.png */
#define NG_ASSET_STRIPS_152 2  /* zzzzzzzzzz_sky_bullet.png */
#define NG_ASSET_STRIPS_153 2  /* zzzzzzzzzz_sky_enemy_a.png */
#define NG_ASSET_STRIPS_154 2  /* zzzzzzzzzz_sky_enemy_b.png */
#define NG_ASSET_STRIPS_155 2  /* zzzzzzzzzz_sky_enemy_c.png */
#define NG_ASSET_STRIPS_156 2  /* zzzzzzzzzz_sky_orb.png */
#define NG_ASSET_STRIPS_157 2  /* zzzzzzzzzz_sky_plane.png */
#define NG_ASSET_STRIPS_158 16  /* zzzzzzzzzzz_crate.png */
#define NG_ASSET_STRIPS_159 16  /* zzzzzzzzzzz_crate_broken.png */
#define NG_ASSET_STRIPS_160 16  /* 0.png */
#define NG_ASSET_STRIPS_161 16  /* 1.png */
#define NG_ASSET_STRIPS_162 6  /* z_npc_84.png */
#define NG_ASSET_STRIPS_163 6  /* z_npc_85.png */
#define NG_ASSET_STRIPS_164 6  /* z_npc_86.png */
#define NG_ASSET_STRIPS_165 6  /* z_npc_87.png */
#define NG_ASSET_STRIPS_166 6  /* z_npc_88.png */
#define NG_ASSET_STRIPS_167 6  /* z_npc_89.png */
#define NG_ASSET_STRIPS_168 6  /* z_npc_90.png */
#define NG_ASSET_STRIPS_169 6  /* z_npc_91.png */
#define NG_ASSET_STRIPS_170 6  /* z_npc_92.png */
#define NG_ASSET_STRIPS_171 6  /* z_npc_93.png */
#define NG_ASSET_STRIPS_172 6  /* z_npc_94.png */
#define NG_ASSET_STRIPS_173 6  /* z_npc_95.png */

static const NGSpriteAssetMeta g_ng_asset_meta[NG_ASSET_META_COUNT] = {
    { 0, 16, 16, 16, 0, 0, 0, 0, 256, 256, 0, 0, 16 }, /* 0.png */
    { 256, 17, 16, 16, 0, 0, 0, 0, 256, 256, 0, 0, 16 }, /* 1.png */
    { 512, 18, 12, 16, 2, 0, 12, 0, 167, 256, 1, 0, 16 }, /* sprite_001_r01_c01.png */
    { 768, 18, 12, 16, 2, 0, 12, 0, 167, 256, 1, 0, 16 }, /* sprite_002_r01_c02.png */
    { 1024, 18, 12, 16, 2, 0, 12, 0, 167, 256, 1, 0, 16 }, /* sprite_003_r01_c03.png */
    { 1280, 18, 12, 16, 2, 0, 12, 0, 167, 256, 1, 0, 16 }, /* sprite_004_r01_c04.png */
    { 1536, 18, 12, 16, 2, 0, 12, 0, 167, 256, 1, 0, 16 }, /* sprite_005_r01_c05.png */
    { 1792, 18, 12, 16, 2, 0, 12, 0, 167, 256, 1, 0, 16 }, /* sprite_006_r01_c06.png */
    { 2048, 18, 12, 16, 2, 0, 12, 0, 167, 256, 1, 0, 16 }, /* sprite_007_r01_c07.png */
    { 2304, 18, 12, 16, 2, 0, 12, 0, 167, 256, 1, 0, 16 }, /* sprite_008_r01_c08.png */
    { 2560, 18, 12, 16, 2, 0, 12, 0, 167, 256, 1, 0, 16 }, /* sprite_009_r01_c09.png */
    { 2816, 18, 12, 16, 2, 0, 12, 0, 167, 256, 1, 0, 16 }, /* sprite_010_r01_c10.png */
    { 3072, 18, 12, 16, 2, 0, 12, 0, 167, 256, 1, 0, 16 }, /* sprite_011_r01_c11.png */
    { 3328, 18, 12, 16, 2, 0, 12, 0, 167, 256, 1, 0, 16 }, /* sprite_012_r01_c12.png */
    { 3584, 18, 14, 16, 1, 0, 9, 0, 206, 256, 1, 0, 16 }, /* sprite_013_r02_c01.png */
    { 3840, 18, 16, 15, 0, 1, 0, 6, 256, 234, 1, 0, 16 }, /* sprite_014_r02_c02.png */
    { 4096, 18, 14, 16, 1, 0, 9, 0, 206, 256, 1, 0, 16 }, /* sprite_015_r02_c03.png */
    { 4352, 18, 14, 16, 1, 0, 9, 0, 206, 256, 1, 0, 16 }, /* sprite_016_r02_c04.png */
    { 4608, 18, 14, 16, 1, 0, 9, 0, 206, 256, 1, 0, 16 }, /* sprite_017_r02_c05.png */
    { 4864, 18, 14, 16, 1, 0, 9, 0, 206, 256, 1, 0, 16 }, /* sprite_018_r02_c06.png */
    { 5120, 18, 14, 16, 1, 0, 9, 0, 206, 256, 1, 0, 16 }, /* sprite_019_r02_c07.png */
    { 5376, 18, 14, 16, 1, 0, 9, 0, 206, 256, 1, 0, 16 }, /* sprite_020_r02_c08.png */
    { 5632, 18, 14, 16, 1, 0, 9, 0, 206, 256, 1, 0, 16 }, /* sprite_021_r02_c09.png */
    { 5888, 18, 14, 16, 1, 0, 9, 0, 206, 256, 1, 0, 16 }, /* sprite_022_r02_c10.png */
    { 6144, 18, 12, 16, 2, 0, 10, 0, 172, 256, 1, 0, 16 }, /* sprite_023_r03_c01.png */
    { 6400, 18, 16, 16, 0, 0, 13, 0, 230, 256, 1, 0, 16 }, /* sprite_024_r03_c02.png */
    { 6656, 18, 16, 16, 0, 0, 0, 1, 256, 255, 1, 0, 16 }, /* sprite_025_r03_c03.png */
    { 6912, 18, 16, 16, 0, 0, 0, 1, 256, 255, 1, 0, 16 }, /* sprite_026_r03_c04.png */
    { 7168, 18, 16, 15, 0, 1, 0, 10, 256, 230, 1, 0, 16 }, /* sprite_027_r03_c05.png */
    { 7424, 18, 16, 10, 0, 6, 0, 2, 256, 158, 1, 0, 16 }, /* sprite_028_r03_c06.png */
    { 7680, 18, 16, 15, 0, 1, 0, 10, 256, 230, 1, 0, 16 }, /* sprite_029_r03_c07.png */
    { 7936, 18, 16, 16, 0, 0, 0, 11, 256, 245, 1, 0, 16 }, /* sprite_030_r03_c08.png */
    { 8192, 18, 16, 16, 0, 0, 14, 0, 227, 256, 1, 0, 16 }, /* sprite_031_r04_c01.png */
    { 8448, 18, 16, 16, 0, 0, 4, 0, 248, 256, 1, 0, 16 }, /* sprite_032_r04_c02.png */
    { 8704, 18, 14, 16, 1, 0, 4, 0, 215, 256, 1, 0, 16 }, /* sprite_033_r04_c03.png */
    { 8960, 18, 16, 16, 0, 0, 0, 13, 256, 243, 1, 0, 16 }, /* sprite_034_r04_c04.png */
    { 9216, 18, 16, 14, 0, 2, 0, 14, 256, 210, 1, 0, 16 }, /* sprite_035_r04_c05.png */
    { 9472, 18, 16, 11, 0, 5, 0, 15, 256, 161, 1, 0, 16 }, /* sprite_036_r04_c06.png */
    { 9728, 18, 14, 16, 1, 0, 1, 2, 222, 254, 1, 0, 16 }, /* sprite_037_r04_c07.png */
    { 9984, 18, 12, 6, 2, 10, 13, 8, 166, 88, 1, 0, 16 }, /* sprite_038_r04_c08.png */
    { 10240, 18, 16, 16, 0, 0, 15, 12, 226, 244, 1, 0, 16 }, /* sprite_039_r04_c09.png */
    { 10496, 18, 12, 7, 2, 9, 3, 14, 186, 98, 1, 0, 16 }, /* sprite_040_r04_c10.png */
    { 10752, 18, 16, 14, 0, 2, 0, 1, 256, 223, 1, 0, 16 }, /* sprite_041_r04_c11.png */
    { 11008, 18, 12, 15, 2, 1, 1, 2, 190, 238, 1, 0, 16 }, /* sprite_042_r05_c01.png */
    { 11264, 18, 16, 15, 0, 1, 0, 7, 256, 233, 1, 0, 16 }, /* sprite_043_r05_c02.png */
    { 11520, 18, 4, 5, 6, 11, 9, 12, 46, 68, 1, 0, 16 }, /* sprite_044_r05_c03.png */
    { 11776, 19, 12, 12, 2, 4, 6, 10, 180, 182, 1, 0, 16 }, /* sprite_045_r05_c04.png */
    { 12032, 19, 6, 4, 5, 12, 8, 6, 80, 58, 1, 0, 16 }, /* sprite_046_r05_c05.png */
    { 12288, 19, 16, 13, 0, 3, 0, 7, 256, 201, 1, 0, 16 }, /* sprite_047_r05_c06.png */
    { 12544, 19, 14, 13, 1, 3, 2, 2, 220, 206, 1, 0, 16 }, /* sprite_048_r05_c07.png */
    { 12800, 19, 8, 3, 4, 13, 7, 12, 114, 36, 1, 0, 16 }, /* sprite_049_r05_c08.png */
    { 13056, 19, 16, 7, 0, 9, 0, 7, 256, 105, 1, 0, 16 }, /* sprite_050_r05_c09.png */
    { 13312, 19, 16, 11, 0, 5, 0, 8, 256, 168, 1, 0, 16 }, /* sprite_051_r05_c10.png */
    { 13568, 19, 16, 7, 0, 9, 0, 2, 256, 110, 1, 0, 16 }, /* sprite_052_r05_c11.png */
    { 13824, 19, 16, 11, 0, 5, 0, 5, 256, 171, 1, 0, 16 }, /* sprite_053_r05_c12.png */
    { 14080, 20, 16, 12, 0, 4, 0, 1, 256, 191, 1, 0, 16 }, /* sprite_054_r06_c01.png */
    { 14336, 20, 16, 12, 0, 4, 0, 13, 256, 179, 1, 0, 16 }, /* sprite_055_r06_c02.png */
    { 14592, 20, 16, 14, 0, 2, 0, 2, 256, 222, 1, 0, 16 }, /* sprite_056_r06_c03.png */
    { 14848, 20, 14, 15, 1, 1, 8, 8, 208, 232, 1, 0, 16 }, /* sprite_057_r06_c04.png */
    { 15104, 20, 14, 15, 1, 1, 11, 2, 202, 238, 1, 0, 16 }, /* sprite_058_r06_c05.png */
    { 15360, 20, 14, 10, 1, 6, 12, 6, 200, 154, 1, 0, 16 }, /* sprite_059_r06_c06.png */
    { 15616, 20, 6, 4, 5, 12, 1, 11, 94, 53, 1, 0, 16 }, /* sprite_060_r06_c07.png */
    { 15872, 20, 10, 16, 3, 0, 3, 4, 154, 252, 1, 0, 16 }, /* sprite_061_r06_c08.png */
    { 16128, 20, 12, 16, 2, 0, 8, 0, 175, 256, 1, 0, 16 }, /* sprite_062_r06_c09.png */
    { 16384, 20, 10, 16, 3, 0, 13, 8, 134, 248, 1, 0, 16 }, /* sprite_063_r06_c10.png */
    { 16640, 20, 10, 13, 3, 3, 3, 2, 154, 206, 1, 0, 16 }, /* sprite_064_r06_c11.png */
    { 16896, 20, 4, 4, 6, 12, 2, 8, 60, 56, 1, 0, 16 }, /* sprite_065_r06_c12.png */
    { 17152, 21, 14, 12, 1, 4, 15, 4, 194, 188, 1, 0, 16 }, /* sprite_066_r07_c01.png */
    { 17408, 21, 14, 12, 1, 4, 9, 4, 206, 188, 1, 0, 16 }, /* sprite_067_r07_c02.png */
    { 17664, 21, 14, 12, 1, 4, 11, 4, 202, 188, 1, 0, 16 }, /* sprite_068_r07_c03.png */
    { 17920, 21, 14, 12, 1, 4, 11, 4, 202, 188, 1, 0, 16 }, /* sprite_069_r07_c04.png */
    { 18176, 21, 10, 12, 3, 4, 4, 4, 152, 188, 1, 0, 16 }, /* sprite_070_r07_c05.png */
    { 18432, 21, 12, 12, 2, 4, 7, 4, 178, 188, 1, 0, 16 }, /* sprite_071_r07_c06.png */
    { 18688, 21, 12, 11, 2, 5, 6, 10, 180, 166, 1, 0, 16 }, /* sprite_072_r07_c07.png */
    { 18944, 22, 12, 11, 2, 5, 1, 8, 190, 168, 1, 0, 16 }, /* sprite_073_r07_c08.png */
    { 19200, 22, 12, 11, 2, 5, 1, 8, 190, 168, 1, 0, 16 }, /* sprite_074_r07_c09.png */
    { 19456, 22, 12, 11, 2, 5, 3, 8, 186, 168, 1, 0, 16 }, /* sprite_075_r07_c10.png */
    { 19712, 22, 16, 12, 0, 4, 6, 12, 244, 180, 1, 0, 16 }, /* sprite_076_r07_c11.png */
    { 19968, 22, 14, 12, 1, 4, 12, 6, 200, 186, 1, 0, 16 }, /* sprite_077_r07_c12.png */
    { 20224, 22, 14, 12, 1, 4, 15, 4, 194, 188, 1, 0, 16 }, /* sprite_078_r07_c13.png */
    { 20480, 23, 8, 7, 4, 9, 14, 4, 100, 108, 1, 0, 16 }, /* sprite_079_r07_c14.png */
    { 20736, 24, 14, 11, 1, 5, 5, 10, 214, 166, 1, 0, 16 }, /* sprite_080_r08_c01.png */
    { 20992, 24, 14, 11, 1, 5, 6, 10, 212, 166, 1, 0, 16 }, /* sprite_081_r08_c02.png */
    { 21248, 24, 14, 11, 1, 5, 7, 10, 210, 166, 1, 0, 16 }, /* sprite_082_r08_c03.png */
    { 21504, 24, 14, 11, 1, 5, 4, 10, 216, 166, 1, 0, 16 }, /* sprite_083_r08_c04.png */
    { 21760, 24, 14, 11, 1, 5, 7, 10, 210, 166, 1, 0, 16 }, /* sprite_084_r08_c05.png */
    { 22016, 24, 16, 11, 0, 5, 10, 10, 236, 166, 1, 0, 16 }, /* sprite_085_r08_c06.png */
    { 22272, 24, 14, 11, 1, 5, 7, 10, 210, 166, 1, 0, 16 }, /* sprite_086_r08_c07.png */
    { 22528, 25, 6, 4, 5, 6, 1, 8, 93, 48, 1, 0, 16 }, /* sprite_040.png */
    { 22784, 25, 6, 4, 5, 6, 6, 10, 83, 44, 1, 0, 16 }, /* sprite_041.png */
    { 23040, 25, 10, 6, 3, 5, 0, 7, 159, 82, 1, 0, 16 }, /* sprite_048.png */
    { 23296, 25, 10, 5, 3, 5, 11, 15, 137, 65, 1, 0, 16 }, /* sprite_050.png */
    { 23552, 25, 4, 4, 6, 6, 15, 12, 34, 39, 1, 0, 16 }, /* sprite_062.png */
    { 23808, 26, 16, 10, 0, 3, 0, 0, 256, 159, 0, 0, 16 }, /* 1.png */
    { 24064, 27, 16, 10, 0, 3, 0, 0, 256, 159, 0, 0, 16 }, /* 2.png */
    { 24320, 28, 16, 10, 0, 3, 0, 0, 256, 159, 0, 0, 16 }, /* 3.png */
    { 24576, 29, 16, 10, 0, 3, 0, 0, 256, 159, 0, 0, 16 }, /* 4.png */
    { 24832, 30, 16, 10, 0, 3, 0, 0, 256, 159, 0, 0, 16 }, /* 5.png */
    { 25088, 31, 16, 10, 0, 3, 0, 0, 256, 159, 0, 0, 16 }, /* 6.png */
    { 25344, 32, 16, 10, 0, 3, 0, 0, 256, 159, 0, 0, 16 }, /* 7.png */
    { 25600, 33, 16, 16, 0, 0, 0, 0, 256, 256, 0, 0, 16 }, /* 1.png */
    { 25856, 34, 16, 16, 0, 0, 0, 0, 256, 256, 0, 0, 16 }, /* 2.png */
    { 26112, 35, 16, 16, 0, 0, 0, 0, 256, 256, 0, 0, 16 }, /* 3.png */
    { 26368, 36, 16, 16, 0, 0, 0, 0, 256, 256, 0, 0, 16 }, /* 4.png */
    { 26624, 37, 16, 16, 0, 0, 0, 0, 256, 256, 0, 0, 16 }, /* 5.png */
    { 26880, 38, 16, 16, 0, 0, 0, 0, 256, 256, 0, 0, 16 }, /* 6.png */
    { 27136, 39, 16, 16, 0, 0, 0, 0, 256, 256, 0, 0, 16 }, /* 7.png */
    { 27392, 40, 12, 16, 2, 0, 1, 0, 189, 256, 0, 0, 16 }, /* 8.png */
    { 27648, 41, 10, 16, 3, 0, 10, 0, 140, 256, 0, 0, 16 }, /* 9.png */
    { 27904, 42, 12, 15, 2, 1, 0, 15, 191, 225, 1, 3, 16 }, /* cat_01.png */
    { 28160, 42, 12, 15, 2, 1, 2, 13, 187, 227, 1, 3, 16 }, /* cat_02.png */
    { 28416, 42, 12, 15, 2, 1, 1, 14, 190, 226, 1, 3, 16 }, /* cat_03.png */
    { 28672, 42, 12, 15, 2, 1, 1, 12, 190, 228, 1, 3, 16 }, /* cat_04.png */
    { 28928, 42, 12, 15, 2, 1, 0, 13, 191, 227, 1, 3, 16 }, /* cat_05.png */
    { 29184, 42, 12, 15, 2, 1, 2, 15, 187, 225, 1, 3, 16 }, /* cat_06.png */
    { 29440, 42, 12, 15, 2, 1, 1, 0, 189, 208, 1, 3, 16 }, /* cat_07.png */
    { 29696, 42, 12, 15, 2, 1, 5, 7, 182, 201, 1, 3, 16 }, /* cat_08.png */
    { 29952, 42, 12, 15, 2, 1, 6, 9, 180, 199, 1, 3, 16 }, /* cat_09.png */
    { 30208, 42, 12, 15, 2, 1, 1, 8, 189, 200, 1, 3, 16 }, /* cat_10.png */
    { 30464, 42, 12, 15, 2, 1, 3, 6, 185, 202, 1, 3, 16 }, /* cat_11.png */
    { 30720, 42, 12, 15, 2, 1, 2, 10, 187, 198, 1, 3, 16 }, /* cat_12.png */
    { 30976, 43, 4, 2, 6, 14, 13, 4, 38, 28, 1, 3, 16 }, /* zz_shooter_enemy.png */
    { 31232, 44, 2, 1, 7, 15, 13, 5, 5, 11, 1, 3, 16 }, /* zz_shooter_enemy_bullet.png */
    { 31488, 45, 4, 3, 6, 13, 13, 11, 37, 37, 1, 3, 16 }, /* zz_shooter_explosion.png */
    { 31744, 46, 2, 1, 7, 15, 13, 5, 5, 11, 1, 3, 16 }, /* zz_shooter_player_bullet.png */
    { 32000, 47, 4, 2, 6, 14, 12, 6, 40, 26, 1, 3, 16 }, /* zz_shooter_ship.png */
    { 32256, 47, 4, 2, 6, 14, 10, 6, 43, 26, 1, 3, 16 }, /* zz_shooter_ship_alt.png */
    { 32512, 48, 8, 6, 4, 10, 2, 6, 124, 90, 1, 3, 16 }, /* zzz_ufo_blue.png */
    { 32768, 49, 8, 5, 4, 11, 2, 12, 124, 68, 1, 3, 16 }, /* zzz_ufo_green.png */
    { 33024, 50, 8, 5, 4, 11, 2, 8, 124, 72, 1, 3, 16 }, /* zzz_ufo_pink.png */
    { 33280, 51, 8, 4, 4, 12, 2, 2, 124, 62, 1, 3, 16 }, /* zzz_ufo_yellow.png */
    { 33536, 52, 8, 6, 4, 10, 14, 1, 99, 95, 1, 3, 16 }, /* zzzz_duck_back.png */
    { 33792, 53, 8, 6, 4, 10, 14, 1, 99, 95, 1, 3, 16 }, /* zzzz_duck_target_brown.png */
    { 34048, 54, 8, 6, 4, 10, 14, 1, 99, 95, 1, 3, 16 }, /* zzzz_duck_target_white.png */
    { 34304, 55, 8, 6, 4, 10, 14, 1, 99, 95, 1, 3, 16 }, /* zzzz_duck_target_yellow.png */
    { 34560, 56, 2, 2, 7, 14, 0, 0, 32, 32, 1, 3, 16 }, /* zzzzz_hitbox.png */
    { 34816, 57, 1, 1, 0, 15, 0, 0, 16, 16, 1, 3, 16 }, /* zzzzzz_particle_dust.png */
    { 35072, 58, 1, 1, 0, 15, 0, 0, 16, 16, 1, 3, 16 }, /* zzzzzz_particle_explosion.png */
    { 35328, 59, 1, 1, 0, 15, 0, 0, 16, 16, 1, 3, 16 }, /* zzzzzz_particle_hitspark.png */
    { 35584, 60, 1, 1, 0, 15, 0, 0, 16, 16, 1, 3, 16 }, /* zzzzzz_particle_magic.png */
    { 35840, 61, 1, 1, 0, 15, 0, 0, 16, 16, 1, 3, 16 }, /* zzzzzz_particle_smoke.png */
    { 36096, 62, 16, 16, 0, 0, 0, 0, 256, 256, 1, 3, 16 }, /* zzzzzzz_ssg_starfield.png */
    { 36352, 63, 8, 7, 4, 9, 8, 0, 112, 112, 1, 3, 16 }, /* zzzzzzzz_enemyship_blue.png */
    { 36608, 64, 8, 7, 4, 9, 8, 0, 112, 112, 1, 3, 16 }, /* zzzzzzzz_enemyship_green.png */
    { 36864, 65, 8, 7, 4, 9, 8, 0, 112, 112, 1, 3, 16 }, /* zzzzzzzz_enemyship_pink.png */
    { 37120, 66, 6, 7, 5, 9, 10, 0, 75, 112, 1, 3, 16 }, /* zzzzzzzz_player_vessel.png */
    { 37376, 67, 4, 6, 6, 10, 0, 0, 64, 96, 1, 3, 16 }, /* zzzzzzzzz_balloon_blue.png */
    { 37632, 68, 4, 6, 6, 10, 0, 0, 64, 96, 1, 3, 16 }, /* zzzzzzzzz_balloon_green.png */
    { 37888, 69, 4, 6, 6, 10, 0, 0, 64, 96, 1, 3, 16 }, /* zzzzzzzzz_balloon_red.png */
    { 38144, 70, 4, 6, 6, 10, 0, 0, 64, 96, 1, 3, 16 }, /* zzzzzzzzz_balloon_yellow.png */
    { 38400, 71, 8, 8, 4, 8, 0, 0, 128, 128, 1, 3, 16 }, /* zzzzzzzzzz_sky_boss.png */
    { 38656, 72, 2, 1, 7, 15, 8, 0, 16, 16, 1, 3, 16 }, /* zzzzzzzzzz_sky_bullet.png */
    { 38912, 73, 2, 2, 7, 14, 0, 0, 32, 32, 1, 3, 16 }, /* zzzzzzzzzz_sky_enemy_a.png */
    { 39168, 74, 2, 2, 7, 14, 0, 0, 32, 32, 1, 3, 16 }, /* zzzzzzzzzz_sky_enemy_b.png */
    { 39424, 75, 2, 2, 7, 14, 0, 0, 32, 32, 1, 3, 16 }, /* zzzzzzzzzz_sky_enemy_c.png */
    { 39680, 76, 2, 1, 7, 15, 8, 0, 16, 16, 1, 3, 16 }, /* zzzzzzzzzz_sky_orb.png */
    { 39936, 77, 2, 1, 7, 15, 8, 0, 16, 16, 1, 3, 16 }, /* zzzzzzzzzz_sky_plane.png */
    { 40192, 78, 16, 16, 0, 0, 0, 0, 256, 256, 1, 3, 16 }, /* zzzzzzzzzzz_crate.png */
    { 40448, 79, 16, 16, 0, 0, 0, 0, 256, 256, 1, 3, 16 }, /* zzzzzzzzzzz_crate_broken.png */
    { 40704, 80, 16, 16, 0, 0, 0, 8, 256, 239, 0, 0, 16 }, /* 0.png */
    { 40960, 81, 16, 16, 0, 0, 0, 0, 256, 256, 1, 0, 16 }, /* 1.png */
    { 41216, 82, 6, 6, 5, 10, 0, 0, 96, 96, 1, 0, 16 }, /* z_npc_84.png */
    { 41472, 82, 6, 6, 5, 10, 0, 0, 96, 96, 1, 0, 16 }, /* z_npc_85.png */
    { 41728, 82, 6, 6, 5, 10, 0, 0, 96, 96, 1, 0, 16 }, /* z_npc_86.png */
    { 41984, 82, 6, 6, 5, 10, 0, 0, 96, 96, 1, 0, 16 }, /* z_npc_87.png */
    { 42240, 82, 6, 6, 5, 10, 0, 0, 96, 96, 1, 0, 16 }, /* z_npc_88.png */
    { 42496, 82, 6, 6, 5, 10, 0, 0, 96, 96, 1, 0, 16 }, /* z_npc_89.png */
    { 42752, 82, 6, 6, 5, 10, 0, 0, 96, 96, 1, 0, 16 }, /* z_npc_90.png */
    { 43008, 82, 6, 6, 5, 10, 0, 0, 96, 96, 1, 0, 16 }, /* z_npc_91.png */
    { 43264, 82, 6, 6, 5, 10, 0, 0, 96, 96, 1, 0, 16 }, /* z_npc_92.png */
    { 43520, 82, 6, 6, 5, 10, 0, 0, 96, 96, 1, 0, 16 }, /* z_npc_93.png */
    { 43776, 82, 6, 6, 5, 10, 0, 0, 96, 96, 1, 0, 16 }, /* z_npc_94.png */
    { 44032, 82, 6, 6, 5, 10, 0, 0, 96, 96, 1, 0, 16 }, /* z_npc_95.png */
};

#endif
