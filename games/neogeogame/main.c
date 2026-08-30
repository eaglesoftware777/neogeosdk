/*
 * Eagle Software Neo Geo mini shooter.
 *
 * A compact sprite arcade game for the neogeogame ROM.
 */

#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include "sdk/2d_engine/ng_art_asset.h"
#include "sdk/2d_engine/ng_bg.h"
#include "sdk/2d_engine/ng_chars.h"
#include "sdk/2d_engine/ng_palette_assets.h"
#include "sdk/2d_engine/ng_vram.h"
#include "sprite_meta.h"

#define NGG_ROWS          3u
#define NGG_COLS          6u
#define NGG_ENEMIES       (NGG_ROWS * NGG_COLS)
#define NGG_PBULLETS      3u
#define NGG_EBULLETS      5u
#define NGG_DIVE_NONE     0xFFu
#define NGG_BG_SLOT       1u
#define NGG_BG_SIDE_SLOT  17u
#define NGG_PLAYER_SLOT   80u
#define NGG_ENEMY_SLOT    96u
#define NGG_PBULLET_SLOT  232u
#define NGG_EBULLET_SLOT  240u
#define NGG_BOOM_SLOT     252u

#define NGG_ASSET_BG_STAR        1u
#define NGG_ASSET_BG_SPACE       2u
#define NGG_ASSET_SHIP           3u
#define NGG_ASSET_SHIP_ALT       4u
#define NGG_ASSET_PLAYER_BULLET  6u
#define NGG_ASSET_ENEMY_BULLET   7u
#define NGG_ASSET_EXPLOSION      8u
#define NGG_ASSET_ENEMY          9u
#define NGG_ASSET_LIFE           10u

void NEOGEO_USER showScreen1(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);

static void NEOGEO_USER ngg_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    fixtext_out(x, y, (char *)text, pal);
}

static void NEOGEO_USER ngg_put_u16(uint8_t x, uint8_t y, uint16_t v, uint8_t pal);
static void NEOGEO_USER ngg_put_2(uint8_t x, uint8_t y, uint8_t v, uint8_t pal);

typedef struct {
    uint8_t alive;
    uint8_t kind;
    int16_t x;
    int16_t y;
    int16_t vx;
    int16_t vy;
    uint8_t diving;
} NGGEnemy;

typedef struct {
    uint8_t active;
    int16_t x;
    int16_t y;
    int16_t vy;
} NGGBullet;

static uint16_t NEOGEO_USER ngg_tile_base(uint8_t asset_id)
{
    const NGSpriteAssetMeta *m = &g_ng_asset_meta[(uint8_t)(asset_id - 1u)];

    return (uint16_t)(m->tile_base +
                      (uint16_t)m->tile_row_start * 16u +
                      m->tile_col_start);
}

static void NEOGEO_USER ngg_load_asset_palette(uint8_t asset_id)
{
    if (asset_id <= ng_screen_count && ng_screen_table[asset_id]) {
        ng_screen_table[asset_id](0, 0, 0, 0, 16, 0u, 0u);
    }
}

static void NEOGEO_USER ngg_bind_sprite(NGCharacter *c,
                                        uint8_t asset_id,
                                        uint16_t first_sprite,
                                        uint8_t scale)
{
    const NGSpriteAssetMeta *m;
    uint16_t anchor_x;
    uint16_t anchor_y;

    if (!c || asset_id == 0u || asset_id > NG_ASSET_META_COUNT) return;

    m = &g_ng_asset_meta[(uint8_t)(asset_id - 1u)];
    ngg_load_asset_palette(asset_id);
    ng_char_set_sprite(c, first_sprite, m->strips, m->active_rows,
                       ngg_tile_base(asset_id), m->palette_bank);
    ng_char_set_tile_stride(c, 16u);

    c->scale_x = scale;
    c->scale_y = scale;
    anchor_x = (uint16_t)((uint16_t)m->x_pad + (m->content_width >> 1));
    anchor_y = (uint16_t)((uint16_t)m->y_pad + m->content_height);
    c->sprite_offset_x = -(int16_t)((anchor_x * scale) >> 8);
    c->sprite_offset_y = -(int16_t)((anchor_y * scale) >> 8);
    c->sprite_dirty = 1u;
}

static void NEOGEO_USER ngg_place_sprite(NGCharacter *c,
                                         uint8_t asset_id,
                                         int16_t x,
                                         int16_t y,
                                         uint8_t visible)
{
    if (!c) return;

    c->visible = visible;
    if (!visible) {
        c->sprite_dirty = 1u;
        return;
    }
    if (asset_id) {
        ngg_bind_sprite(c, asset_id, c->sprite_first, c->scale_x);
    }
    ng_char_set_pos(c, x, y);
}

static void NEOGEO_USER ngg_draw_background(void)
{
    if (NGG_ASSET_BG_SPACE <= ng_screen_count &&
        ng_screen_table[NGG_ASSET_BG_SPACE]) {
        ng_screen_table[NGG_ASSET_BG_SPACE](0, 0, 0x0F, 0xAF, 16, BLACK,
                            (uint16_t)(NGG_BG_SLOT << 6));
    }
    if (NGG_ASSET_BG_STAR <= ng_screen_count &&
        ng_screen_table[NGG_ASSET_BG_STAR]) {
        ng_screen_table[NGG_ASSET_BG_STAR](256, 0, 0x0F, 0xAF, 16, BLACK,
                            (uint16_t)(NGG_BG_SIDE_SLOT << 6));
    }
}

static void NEOGEO_USER ngg_load_game_palettes(void)
{
    ngg_load_asset_palette(NGG_ASSET_SHIP);
    ngg_load_asset_palette(NGG_ASSET_SHIP_ALT);
    ngg_load_asset_palette(NGG_ASSET_PLAYER_BULLET);
    ngg_load_asset_palette(NGG_ASSET_ENEMY_BULLET);
    ngg_load_asset_palette(NGG_ASSET_EXPLOSION);
    ngg_load_asset_palette(NGG_ASSET_ENEMY);
    ngg_load_asset_palette(NGG_ASSET_LIFE);
}

static void NEOGEO_USER ngg_draw_hud_static(void)
{
    clearFix();
    ngg_puts(1u, 1u, "EAGLE SOFTWARE  STAR RAID", 1u);
    ngg_puts(1u, 2u, "SCORE", 0u);
    ngg_puts(17u, 2u, "WAVE", 0u);
    ngg_puts(29u, 2u, "LIVES", 0u);
    ngg_puts(1u, 3u, "PROGRESS", 0u);
    ngg_puts(10u, 3u, "------------------", 1u);
}

static void NEOGEO_USER ngg_draw_hud_numbers(uint16_t score,
                                             uint8_t wave,
                                             uint8_t lives,
                                             uint8_t alive)
{
    uint8_t done;
    uint8_t i;

    ngg_puts(7u, 2u, "     ", 1u);
    ngg_put_u16(7u, 2u, score, 1u);
    ngg_put_2(22u, 2u, wave, 1u);
    ngg_put_2(35u, 2u, lives, 1u);

    done = (uint8_t)(NGG_ENEMIES - alive);
    for (i = 0u; i < 18u; i++) {
        char cell[2];
        cell[0] = (i < (uint8_t)((done * 18u) / NGG_ENEMIES)) ? '#' : '-';
        cell[1] = '\0';
        ngg_puts((uint8_t)(10u + i), 3u, cell, (uint8_t)(cell[0] == '#' ? 2u : 1u));
    }
}

static void NEOGEO_USER ngg_init_sprites(NGCharacter **player,
                                         NGCharacter **enemy_sprite,
                                         NGCharacter **player_bullet_sprite,
                                         NGCharacter **enemy_bullet_sprite,
                                         NGCharacter **boom)
{
    uint8_t i;

    ng_chars_init();
    *player = chars_add(1u, 160, 204);
    if (*player) {
        (*player)->sprite_first = NGG_PLAYER_SLOT;
        ngg_bind_sprite(*player, NGG_ASSET_SHIP, NGG_PLAYER_SLOT, 0xC0u);
        (*player)->priority_band = NG_RENDER_BAND_PLAYER;
    }

    for (i = 0u; i < NGG_ENEMIES; i++) {
        enemy_sprite[i] = chars_add(2u, 0, 0);
        if (enemy_sprite[i]) {
            enemy_sprite[i]->sprite_first = (uint16_t)(NGG_ENEMY_SLOT + i * 4u);
            ngg_bind_sprite(enemy_sprite[i], NGG_ASSET_ENEMY,
                            enemy_sprite[i]->sprite_first, 0x90u);
            enemy_sprite[i]->priority_band = NG_RENDER_BAND_ENEMY;
        }
    }

    for (i = 0u; i < NGG_PBULLETS; i++) {
        player_bullet_sprite[i] = chars_add(3u, 0, 0);
        if (player_bullet_sprite[i]) {
            player_bullet_sprite[i]->sprite_first = (uint16_t)(NGG_PBULLET_SLOT + i * 2u);
            ngg_bind_sprite(player_bullet_sprite[i], NGG_ASSET_PLAYER_BULLET,
                            player_bullet_sprite[i]->sprite_first, 0x80u);
            player_bullet_sprite[i]->priority_band = NG_RENDER_BAND_FX;
        }
    }

    for (i = 0u; i < NGG_EBULLETS; i++) {
        enemy_bullet_sprite[i] = chars_add(4u, 0, 0);
        if (enemy_bullet_sprite[i]) {
            enemy_bullet_sprite[i]->sprite_first = (uint16_t)(NGG_EBULLET_SLOT + i * 2u);
            ngg_bind_sprite(enemy_bullet_sprite[i], NGG_ASSET_ENEMY_BULLET,
                            enemy_bullet_sprite[i]->sprite_first, 0x80u);
            enemy_bullet_sprite[i]->priority_band = NG_RENDER_BAND_FX;
        }
    }

    *boom = chars_add(5u, 0, 0);
    if (*boom) {
        (*boom)->sprite_first = NGG_BOOM_SLOT;
        ngg_bind_sprite(*boom, NGG_ASSET_EXPLOSION, NGG_BOOM_SLOT, 0xB0u);
        (*boom)->priority_band = NG_RENDER_BAND_FRONT;
    }
}

static void NEOGEO_USER ngg_put_u16(uint8_t x, uint8_t y, uint16_t v, uint8_t pal)
{
    char b[6];
    uint8_t i = 0u;
    uint16_t div = 10000u;
    uint8_t started = 0u;

    while (div > 0u) {
        uint8_t digit = (uint8_t)(v / div);
        if (digit || started || div == 1u) {
            b[i++] = (char)('0' + digit);
            started = 1u;
        }
        v = (uint16_t)(v % div);
        div = (uint16_t)(div / 10u);
    }
    b[i] = '\0';
    ngg_puts(x, y, b, pal);
}

static void NEOGEO_USER ngg_put_2(uint8_t x, uint8_t y, uint8_t v, uint8_t pal)
{
    char b[3];

    if (v > 99u) v = 99u;
    b[0] = (char)('0' + (uint8_t)(v / 10u));
    b[1] = (char)('0' + (uint8_t)(v % 10u));
    b[2] = '\0';
    ngg_puts(x, y, b, pal);
}

static uint8_t NEOGEO_USER ngg_hit(int16_t a, int16_t b, int16_t margin)
{
    int16_t d = (int16_t)(a - b);

    if (d < 0) d = (int16_t)-d;
    return (uint8_t)(d <= margin);
}

static uint8_t NEOGEO_USER ngg_alive_count(NGGEnemy *enemy)
{
    uint8_t i;
    uint8_t count = 0u;

    for (i = 0u; i < NGG_ENEMIES; i++) {
        if (enemy[i].alive) count++;
    }
    return count;
}

static void NEOGEO_USER ngg_reset_wave(NGGEnemy *enemy,
                                       NGGBullet *player_bullet,
                                       NGGBullet *enemy_bullet,
                                       uint8_t *dive_idx)
{
    uint8_t row;
    uint8_t col;
    uint8_t i = 0u;

    for (row = 0u; row < NGG_ROWS; row++) {
        for (col = 0u; col < NGG_COLS; col++) {
            enemy[i].alive = 1u;
            enemy[i].kind = (uint8_t)((row == 0u) ? 2u : ((row == 1u) ? 1u : 0u));
            enemy[i].x = (int16_t)(56 + col * 34);
            enemy[i].y = (int16_t)(54 + row * 22);
            enemy[i].vx = 0;
            enemy[i].vy = 0;
            enemy[i].diving = 0u;
            i++;
        }
    }

    for (i = 0u; i < NGG_PBULLETS; i++) player_bullet[i].active = 0u;
    for (i = 0u; i < NGG_EBULLETS; i++) enemy_bullet[i].active = 0u;
    *dive_idx = NGG_DIVE_NONE;
}

void NEOGEO_USER neogeogame_run(void)
{
    NGGEnemy enemy[NGG_ENEMIES];
    NGGBullet player_bullet[NGG_PBULLETS];
    NGGBullet enemy_bullet[NGG_EBULLETS];
    NGCharacter *player = 0;
    NGCharacter *enemy_sprite[NGG_ENEMIES];
    NGCharacter *player_bullet_sprite[NGG_PBULLETS];
    NGCharacter *enemy_bullet_sprite[NGG_EBULLETS];
    NGCharacter *boom = 0;
    uint8_t ship_x = 20u;
    uint8_t lives = 3u;
    uint8_t wave = 1u;
    uint8_t game_over = 0u;
    uint8_t dive_idx = NGG_DIVE_NONE;
    uint8_t boom_timer = 0u;
    int16_t boom_x = 0;
    int16_t boom_y = 0;
    uint16_t score = 0u;
    uint16_t frame = 0u;
    uint16_t prev_joy = 0u;
    uint16_t idle = 0u;
    uint8_t i;

    clearFix();
    clearSprs();
    ng_vram_clear_all_sprites();
    setBACKDROP(BLACK);
    soundSceneReset();
    soundSetADPCMAVolume(0x38u);
    soundSetSSGVolume(0x07u);
    playSSGTrack(SOUND_SSG_B);
    ngg_init_sprites(&player, enemy_sprite, player_bullet_sprite,
                     enemy_bullet_sprite, &boom);
    ngg_load_game_palettes();
    ngg_draw_background();
    ngg_draw_hud_static();
    ngg_reset_wave(enemy, player_bullet, enemy_bullet, &dive_idx);

    for (;;) {
        uint16_t joy = poll_joystick();
        uint16_t edge = (uint16_t)(joy & (uint16_t)(~prev_joy));
        int16_t drift = (int16_t)(((frame / 18u) & 7u) - 3);

        prev_joy = joy;

        if (game_over) {
            if ((edge & (START1 | START2)) || idle > 420u) break;
            idle++;
        } else {
            idle = 0u;

            if ((joy & JOY_LEFT) && ship_x > 2u) ship_x--;
            if ((joy & JOY_RIGHT) && ship_x < 36u) ship_x++;
            if (edge & BUTTON_B) {
                for (i = 0u; i < NGG_PBULLETS; i++) {
                    if (!player_bullet[i].active) {
                        player_bullet[i].active = 1u;
                        player_bullet[i].x = (int16_t)(ship_x * 8 + 8);
                        player_bullet[i].y = 188;
                        player_bullet[i].vy = -5;
                        playSFX(SOUND_SFX_1);
                        break;
                    }
                }
            }

            if (dive_idx == NGG_DIVE_NONE &&
                (frame % (uint16_t)(84u - wave * 5u)) == 0u) {
                uint8_t k;
                for (k = 0u; k < NGG_ENEMIES; k++) {
                    uint8_t idx = (uint8_t)((frame / 8u + k * 5u) % NGG_ENEMIES);
                    if (enemy[idx].alive) {
                        enemy[idx].diving = 1u;
                        enemy[idx].vx = (int16_t)((enemy[idx].x < (int16_t)(ship_x * 8)) ? 2 : -2);
                        enemy[idx].vy = 2;
                        dive_idx = idx;
                        playSFX(SOUND_SFX_1);
                        break;
                    }
                }
            }

            for (i = 0u; i < NGG_ENEMIES; i++) {
                if (!enemy[i].alive) continue;
                if (enemy[i].diving) {
                    enemy[i].x = (int16_t)(enemy[i].x + enemy[i].vx);
                    enemy[i].y = (int16_t)(enemy[i].y + enemy[i].vy);
                    if ((frame & 31u) == 12u) {
                        uint8_t b;
                        for (b = 0u; b < NGG_EBULLETS; b++) {
                            if (!enemy_bullet[b].active) {
                                enemy_bullet[b].active = 1u;
                                enemy_bullet[b].x = enemy[i].x;
                                enemy_bullet[b].y = enemy[i].y;
                                enemy_bullet[b].vy = 3;
                                break;
                            }
                        }
                    }
                    if (enemy[i].y > 210) {
                        enemy[i].diving = 0u;
                        enemy[i].x = (int16_t)(56 + (i % NGG_COLS) * 34);
                        enemy[i].y = (int16_t)(54 + (i / NGG_COLS) * 22);
                        if (dive_idx == i) dive_idx = NGG_DIVE_NONE;
                    }
                } else {
                    enemy[i].x = (int16_t)(56 + (i % NGG_COLS) * 34 + drift);
                    enemy[i].y = (int16_t)(54 + (i / NGG_COLS) * 22);
                }
            }

            for (i = 0u; i < NGG_PBULLETS; i++) {
                uint8_t j;

                if (!player_bullet[i].active) continue;
                player_bullet[i].y = (int16_t)(player_bullet[i].y + player_bullet[i].vy);
                if (player_bullet[i].y < 24) {
                    player_bullet[i].active = 0u;
                    continue;
                }

                for (j = 0u; j < NGG_ENEMIES; j++) {
                    if (!enemy[j].alive) continue;
                    if (ngg_hit(player_bullet[i].x, enemy[j].x, 9) &&
                        ngg_hit(player_bullet[i].y, enemy[j].y, 8)) {
                        uint16_t pts = (uint16_t)(enemy[j].kind == 2u ? 150u :
                                                  (enemy[j].kind ? 100u : 60u));
                        enemy[j].alive = 0u;
                        enemy[j].diving = 0u;
                        if (dive_idx == j) dive_idx = NGG_DIVE_NONE;
                        player_bullet[i].active = 0u;
                        boom_timer = 12u;
                        boom_x = enemy[j].x;
                        boom_y = enemy[j].y;
                        score = (uint16_t)((score + pts > 9999u) ? 9999u : score + pts);
                        playSFX(SOUND_SFX_2);
                        break;
                    }
                }
            }

            for (i = 0u; i < NGG_EBULLETS; i++) {
                if (!enemy_bullet[i].active) continue;
                enemy_bullet[i].y = (int16_t)(enemy_bullet[i].y + enemy_bullet[i].vy);
                if (enemy_bullet[i].y > 216) {
                    enemy_bullet[i].active = 0u;
                    continue;
                }
                if (ngg_hit(enemy_bullet[i].x, (int16_t)(ship_x * 8 + 8), 10) &&
                    ngg_hit(enemy_bullet[i].y, 196, 8)) {
                    enemy_bullet[i].active = 0u;
                    boom_timer = 16u;
                    boom_x = (int16_t)(ship_x * 8 + 8);
                    boom_y = 196;
                    if (lives > 0u) lives--;
                    playSFX(SOUND_SFX_3);
                    if (lives == 0u) game_over = 1u;
                }
            }

            if (ngg_alive_count(enemy) == 0u) {
                if (wave < 9u) wave++;
                playSFX(SOUND_SFX_4);
                ngg_reset_wave(enemy, player_bullet, enemy_bullet, &dive_idx);
            }
        }

        ngg_draw_hud_numbers(score, wave, lives, ngg_alive_count(enemy));

        for (i = 0u; i < NGG_ENEMIES; i++) {
            ngg_place_sprite(enemy_sprite[i], 0u, enemy[i].x, enemy[i].y,
                             enemy[i].alive ? 1u : 0u);
        }

        for (i = 0u; i < NGG_PBULLETS; i++) {
            ngg_place_sprite(player_bullet_sprite[i], 0u,
                             player_bullet[i].x, player_bullet[i].y,
                             player_bullet[i].active);
        }
        for (i = 0u; i < NGG_EBULLETS; i++) {
            ngg_place_sprite(enemy_bullet_sprite[i], 0u,
                             enemy_bullet[i].x, enemy_bullet[i].y,
                             enemy_bullet[i].active);
        }

        if (!game_over) {
            uint8_t ship_asset = ((frame >> 4) & 1u) ? NGG_ASSET_SHIP_ALT : NGG_ASSET_SHIP;
            ngg_place_sprite(player, ship_asset, (int16_t)(ship_x * 8 + 8), 206, 1u);
        } else {
            ngg_place_sprite(player, NGG_ASSET_EXPLOSION,
                             (int16_t)(ship_x * 8 + 8), 196, 1u);
            ngg_puts(12u, 13u, "GAME OVER", 2u);
            ngg_puts(9u, 15u, "START OR WAIT", 1u);
        }

        if (boom_timer) {
            ngg_place_sprite(boom, 0u, boom_x, boom_y, 1u);
            boom_timer--;
        } else {
            ngg_place_sprite(boom, 0u, 0, 0, 0u);
        }

        ng_chars_draw();
        waitVbl();
        frame++;
    }

    soundStopAll();
    ng_vram_clear_all_sprites();
}

void NEOGEO_USER showScreen1(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 1 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal1[16];
setpal(pal1,0x0,0x5899,0x2bbc,0x5566,0x789,0x7cde,0x288a,0x1677,0x3fff,0x5789,0x3aab,0x4567,0x5788,0x4abc,0x1abc,0xbcd);
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
setpal(pal2,0x0,0x1000,0x333,0x222,0x4131,0x666,0x6674,0x7000,0x7255,0x3332,0x5632,0x7444,0x7100,0x111,0x311,0x5243);
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
setpal(pal3,0x0,0x6f10,0x223,0x6679,0x2346,0x3b0,0x4fe0,0x6f80,0x304e,0x6ddd,0x29e0,0x3050,0x57ee,0x80,0x0,0x7fff);
load_palettes(pal3,PALETTES+PALOFFSET*18);
}


void NEOGEO_USER showScreen4(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 4 ******************************************/
uint16_t  pal4[16];
setpal(pal4,0x0,0x6f10,0x223,0x6679,0x2346,0x3b0,0x4fe0,0x6f80,0x304e,0x6ddd,0x29e0,0x3050,0x57ee,0x80,0x0,0x7fff);
load_palettes(pal4,PALETTES+PALOFFSET*19);
}


void NEOGEO_USER showScreen5(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 5 ******************************************/
uint16_t  pal5[16];
setpal(pal5,0x0,0x6f10,0x223,0x6679,0x2346,0x3b0,0x4fe0,0x6f80,0x304e,0x6ddd,0x29e0,0x3050,0x57ee,0x80,0x0,0x7fff);
load_palettes(pal5,PALETTES+PALOFFSET*20);
}


void NEOGEO_USER showScreen6(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 6 ******************************************/
uint16_t  pal6[16];
setpal(pal6,0x0,0xde8,0x7bc6,0x1ef9,0x3782,0x2875,0x1653,0x1653,0x1653,0x1653,0x1653,0x1653,0x1653,0x1653,0x1653,0x1653);
load_palettes(pal6,PALETTES+PALOFFSET*21);
}


void NEOGEO_USER showScreen7(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 7 ******************************************/
uint16_t  pal7[16];
setpal(pal7,0x0,0xf70,0x6fff,0x6e30,0xf50,0x6fc1,0x6c00,0x7fe4,0x2f80,0x4d20,0x4c00,0x6fb0,0x4e33,0xe22,0x6ffc,0xd11);
load_palettes(pal7,PALETTES+PALOFFSET*22);
}


void NEOGEO_USER showScreen8(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 8 ******************************************/
uint16_t  pal8[16];
setpal(pal8,0x0,0x7ff7,0x6f10,0x6f90,0x400,0x4500,0x4700,0x6ffc,0x6fc0,0x6ff1,0x4d00,0x4fb0,0xa00,0x4f70,0x200,0x6fff);
load_palettes(pal8,PALETTES+PALOFFSET*23);
}


void NEOGEO_USER showScreen9(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 9 ******************************************/
uint16_t  pal9[16];
setpal(pal9,0x0,0x112f,0x5015,0x6f50,0x7ccd,0x779,0x3500,0x4fe0,0x19ff,0x1000,0x317f,0x32af,0x9,0x105f,0x0,0x7fff);
load_palettes(pal9,PALETTES+PALOFFSET*24);
}


void NEOGEO_USER showScreen10(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 10 ******************************************/
uint16_t  pal10[16];
setpal(pal10,0x0,0x112f,0x5015,0x6f50,0x7ccd,0x779,0x3500,0x4fe0,0x19ff,0x1000,0x317f,0x32af,0x9,0x105f,0x0,0x7fff);
load_palettes(pal10,PALETTES+PALOFFSET*25);
}

static const NGPaletteAsset ng_screen_palette_assets[] = {
    {1,16,{0x0,0x5899,0x2bbc,0x5566,0x789,0x7cde,0x288a,0x1677,0x3fff,0x5789,0x3aab,0x4567,0x5788,0x4abc,0x1abc,0xbcd}},
    {2,17,{0x0,0x1000,0x333,0x222,0x4131,0x666,0x6674,0x7000,0x7255,0x3332,0x5632,0x7444,0x7100,0x111,0x311,0x5243}},
    {3,18,{0x0,0x6f10,0x223,0x6679,0x2346,0x3b0,0x4fe0,0x6f80,0x304e,0x6ddd,0x29e0,0x3050,0x57ee,0x80,0x0,0x7fff}},
    {4,19,{0x0,0x6f10,0x223,0x6679,0x2346,0x3b0,0x4fe0,0x6f80,0x304e,0x6ddd,0x29e0,0x3050,0x57ee,0x80,0x0,0x7fff}},
    {5,20,{0x0,0x6f10,0x223,0x6679,0x2346,0x3b0,0x4fe0,0x6f80,0x304e,0x6ddd,0x29e0,0x3050,0x57ee,0x80,0x0,0x7fff}},
    {6,21,{0x0,0xde8,0x7bc6,0x1ef9,0x3782,0x2875,0x1653,0x1653,0x1653,0x1653,0x1653,0x1653,0x1653,0x1653,0x1653,0x1653}},
    {7,22,{0x0,0xf70,0x6fff,0x6e30,0xf50,0x6fc1,0x6c00,0x7fe4,0x2f80,0x4d20,0x4c00,0x6fb0,0x4e33,0xe22,0x6ffc,0xd11}},
    {8,23,{0x0,0x7ff7,0x6f10,0x6f90,0x400,0x4500,0x4700,0x6ffc,0x6fc0,0x6ff1,0x4d00,0x4fb0,0xa00,0x4f70,0x200,0x6fff}},
    {9,24,{0x0,0x112f,0x5015,0x6f50,0x7ccd,0x779,0x3500,0x4fe0,0x19ff,0x1000,0x317f,0x32af,0x9,0x105f,0x0,0x7fff}},
    {10,25,{0x0,0x112f,0x5015,0x6f50,0x7ccd,0x779,0x3500,0x4fe0,0x19ff,0x1000,0x317f,0x32af,0x9,0x105f,0x0,0x7fff}},
};
const uint16_t ng_screen_palette_count = 10;

uint8_t NEOGEO_USER ng_load_screen_palette(uint16_t screen_id) {
    return ng_palette_load_asset(ng_screen_palette_assets, ng_screen_palette_count, screen_id);
}

const NGArtAsset ng_screen_art_assets[] = {
    {1,NG_ART_TYPE_BACKGROUND,16,0,255,16,16,16,16,0,0,256,256},
    {2,NG_ART_TYPE_BACKGROUND,17,256,511,16,16,16,16,0,0,256,256},
    {3,NG_ART_TYPE_SPRITE,18,742,767,4,2,2,16,96,224,40,26},
    {4,NG_ART_TYPE_SPRITE,19,998,1023,4,2,2,16,96,224,43,26},
    {5,NG_ART_TYPE_SPRITE,20,1222,1279,4,4,4,16,96,192,64,64},
    {6,NG_ART_TYPE_SPRITE,21,1399,1535,2,2,2,16,112,112,5,11},
    {7,NG_ART_TYPE_SPRITE,22,1655,1791,2,2,2,16,112,112,5,11},
    {8,NG_ART_TYPE_SPRITE,23,1894,2047,4,4,4,16,96,96,37,37},
    {9,NG_ART_TYPE_SPRITE,24,2128,2303,16,11,11,16,0,80,256,165},
    {10,NG_ART_TYPE_SPRITE,25,2551,2559,2,1,1,16,112,240,22,13},
};
const uint16_t ng_screen_art_asset_count = 10;

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
};
const uint16_t ng_screen_count = 10;
