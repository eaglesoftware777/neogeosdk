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
/* Second copy of the pair, stacked above the first so the backdrop can scroll
 * and wrap.  Each page is 16 strips, so the four occupy slots 1..64, still
 * clear of the player at 80. */
#define NGG_BG2_SLOT      33u
#define NGG_BG2_SIDE_SLOT 49u
/* Vertical shrink the backdrop is drawn with, and the on-screen height that
 * gives: 16 tile rows * 16 px * (0xAF + 1) / 256 = 176.  The scroll wraps on
 * that height so the two stacked copies meet without a gap. */
#define NGG_BG_YSCALE     0xAFu
#define NGG_BG_PAGE_H     176
#define NGG_PLAYER_SLOT   80u
#define NGG_ENEMY_SLOT    96u
#define NGG_PBULLET_SLOT  232u
#define NGG_EBULLET_SLOT  240u
#define NGG_BOOM_SLOT     252u

/*
 * Hardware sprites reserved per entity.  A sprite costs one hardware sprite
 * per 16-pixel column of tile data - its "strips" in sprite_meta.h - and that
 * count comes from how large the artwork was imported, not from how small it
 * is drawn on screen.  Reserve fewer than an asset needs and its strips run
 * into the next entity's slots, which shows up as sprites flickering, drawing
 * the wrong art, or vanishing.  The checks below fail the build rather than
 * let that happen silently.
 */
#define NGG_ENEMY_STRIDE   4u
#define NGG_PBULLET_STRIDE 2u
#define NGG_EBULLET_STRIDE 2u

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
                      (uint16_t)m->tile_row_start * m->tile_stride +
                      m->tile_col_start);
}

uint8_t NEOGEO_USER ng_load_screen_palette(uint16_t screen_id);
const NGArtAsset * NEOGEO_USER ng_screen_art_asset(uint16_t screen_id);

static void NEOGEO_USER ngg_load_asset_palette(uint8_t asset_id)
{
    ng_load_screen_palette(asset_id);
}

static void NEOGEO_USER ngg_bind_sprite(NGCharacter *c,
                                        uint8_t asset_id,
                                        uint16_t first_sprite,
                                        uint8_t scale)
{
    const NGSpriteAssetMeta *m;
    const NGArtAsset *art;
    uint16_t anchor_x;
    uint16_t anchor_y;

    if (!c || asset_id == 0u || asset_id > NG_ASSET_META_COUNT) return;

    m = &g_ng_asset_meta[(uint8_t)(asset_id - 1u)];
    art = ng_screen_art_asset(asset_id);
    ngg_load_asset_palette(asset_id);
    ng_char_set_sprite(c, first_sprite, m->strips, m->active_rows,
                       ngg_tile_base(asset_id), m->palette_bank);
    ng_char_set_tile_stride(c, m->tile_stride);
    ng_char_set_palette_map(c, art ? art->tile_palettes : 0);

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

/*
 * Scrolling backdrop.
 *
 * Two 256-wide pages side by side cover the 320-px screen, and a second copy
 * of that pair sits one page-height above it so the field can slide down and
 * wrap.  The four pages are built once and only their Y is written per frame:
 * ng_sprite_group_flush() writes SCB2/3/4 alone unless the tiles are marked
 * dirty, where re-running the generated showScreenN would rewrite the whole
 * tilemap - far more than a vblank has room for.
 *
 * The four pages live on the caller's stack, not at file scope: the ROM link
 * strips .text/.data/.bss from this translation unit and keeps only
 * neogeo_user, so a `static` here would be dropped and the link would fail.
 */
static void NEOGEO_USER ngg_bg_page_init(NGSpriteGroup *g, uint16_t slot,
                                         uint8_t asset_id, int16_t x)
{
    const NGSpriteAssetMeta *m;
    const NGArtAsset *art;

    if (asset_id == 0u || asset_id > NG_ASSET_META_COUNT) return;
    m = &g_ng_asset_meta[(uint8_t)(asset_id - 1u)];
    art = ng_screen_art_asset(asset_id);
    ngg_load_asset_palette(asset_id);
    ng_sprite_group_init(g, slot, 16u, 16u, m->tile_base, m->palette_bank);
    ng_sprite_group_set_tile_stride(g, m->tile_stride);
    ng_sprite_group_set_palette_map(g, art ? art->tile_palettes : 0);
    ng_sprite_group_set_active_rows(g, 16u);
    ng_sprite_group_set_scale(g, 0xFFu, NGG_BG_YSCALE);
    ng_sprite_group_set_pos(g, x, 0);
    ng_sprite_group_upload(g);
}

static void NEOGEO_USER ngg_draw_background(NGSpriteGroup *page)
{
    ngg_bg_page_init(&page[0], NGG_BG_SLOT,       NGG_ASSET_BG_SPACE, 0);
    ngg_bg_page_init(&page[1], NGG_BG_SIDE_SLOT,  NGG_ASSET_BG_STAR,  256);
    ngg_bg_page_init(&page[2], NGG_BG2_SLOT,      NGG_ASSET_BG_SPACE, 0);
    ngg_bg_page_init(&page[3], NGG_BG2_SIDE_SLOT, NGG_ASSET_BG_STAR,  256);
}

static void NEOGEO_USER ngg_scroll_background(NGSpriteGroup *page,
                                              uint16_t *scroll,
                                              uint8_t pixels)
{
    int16_t upper;
    int16_t lower;

    *scroll = (uint16_t)((*scroll + pixels) % NGG_BG_PAGE_H);
    upper = (int16_t)((int16_t)*scroll - NGG_BG_PAGE_H);
    lower = (int16_t)*scroll;

    ng_sprite_group_set_pos(&page[0], 0,   upper);
    ng_sprite_group_set_pos(&page[1], 256, upper);
    ng_sprite_group_set_pos(&page[2], 0,   lower);
    ng_sprite_group_set_pos(&page[3], 256, lower);
    ng_sprite_group_flush(&page[0]);
    ng_sprite_group_flush(&page[1]);
    ng_sprite_group_flush(&page[2]);
    ng_sprite_group_flush(&page[3]);
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

/* Each entity's reserved stride must cover the strips its asset really uses,
 * and no block may run into the one above it. */
typedef char ngg_slot_check_enemy[
    (NG_ASSET_STRIPS_9 <= NGG_ENEMY_STRIDE) ? 1 : -1];
typedef char ngg_slot_check_pbullet[
    (NG_ASSET_STRIPS_6 <= NGG_PBULLET_STRIDE) ? 1 : -1];
typedef char ngg_slot_check_ebullet[
    (NG_ASSET_STRIPS_7 <= NGG_EBULLET_STRIDE) ? 1 : -1];
typedef char ngg_slot_check_player[
    (NGG_PLAYER_SLOT + NG_ASSET_STRIPS_3 <= NGG_ENEMY_SLOT) ? 1 : -1];
typedef char ngg_slot_check_enemy_block[
    (NGG_ENEMY_SLOT + NGG_ENEMIES * NGG_ENEMY_STRIDE <= NGG_PBULLET_SLOT) ? 1 : -1];
typedef char ngg_slot_check_pbullet_block[
    (NGG_PBULLET_SLOT + NGG_PBULLETS * NGG_PBULLET_STRIDE <= NGG_EBULLET_SLOT) ? 1 : -1];
typedef char ngg_slot_check_ebullet_block[
    (NGG_EBULLET_SLOT + NGG_EBULLETS * NGG_EBULLET_STRIDE <= NGG_BOOM_SLOT) ? 1 : -1];

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
        ngg_bind_sprite(*player, NGG_ASSET_SHIP, NGG_PLAYER_SLOT, 0xCFu);
        (*player)->priority_band = NG_RENDER_BAND_PLAYER;
    }

    for (i = 0u; i < NGG_ENEMIES; i++) {
        enemy_sprite[i] = chars_add(2u, 0, 0);
        if (enemy_sprite[i]) {
            enemy_sprite[i]->sprite_first = (uint16_t)(NGG_ENEMY_SLOT + i * NGG_ENEMY_STRIDE);
            /* Full scale: the artwork is imported at the size it is drawn,
             * so no hardware shrink is needed and none of its detail is
             * resampled away. */
            ngg_bind_sprite(enemy_sprite[i], NGG_ASSET_ENEMY,
                            enemy_sprite[i]->sprite_first, 0xFFu);
            enemy_sprite[i]->priority_band = NG_RENDER_BAND_ENEMY;
        }
    }

    for (i = 0u; i < NGG_PBULLETS; i++) {
        player_bullet_sprite[i] = chars_add(3u, 0, 0);
        if (player_bullet_sprite[i]) {
            player_bullet_sprite[i]->sprite_first = (uint16_t)(NGG_PBULLET_SLOT + i * NGG_PBULLET_STRIDE);
            ngg_bind_sprite(player_bullet_sprite[i], NGG_ASSET_PLAYER_BULLET,
                            player_bullet_sprite[i]->sprite_first, 0x8Fu);
            player_bullet_sprite[i]->priority_band = NG_RENDER_BAND_FX;
        }
    }

    for (i = 0u; i < NGG_EBULLETS; i++) {
        enemy_bullet_sprite[i] = chars_add(4u, 0, 0);
        if (enemy_bullet_sprite[i]) {
            enemy_bullet_sprite[i]->sprite_first = (uint16_t)(NGG_EBULLET_SLOT + i * NGG_EBULLET_STRIDE);
            ngg_bind_sprite(enemy_bullet_sprite[i], NGG_ASSET_ENEMY_BULLET,
                            enemy_bullet_sprite[i]->sprite_first, 0x8Fu);
            enemy_bullet_sprite[i]->priority_band = NG_RENDER_BAND_FX;
        }
    }

    *boom = chars_add(5u, 0, 0);
    if (*boom) {
        (*boom)->sprite_first = NGG_BOOM_SLOT;
        ngg_bind_sprite(*boom, NGG_ASSET_EXPLOSION, NGG_BOOM_SLOT, 0xBFu);
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
            enemy[i].x = (int16_t)(34 + col * 50);
            enemy[i].y = (int16_t)(50 + row * 34);
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
    NGSpriteGroup bg_page[4];
    uint16_t bg_scroll = 0u;

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
    ngg_draw_background(bg_page);
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
                        enemy[i].x = (int16_t)(34 + (i % NGG_COLS) * 50);
                        enemy[i].y = (int16_t)(54 + (i / NGG_COLS) * 22);
                        if (dive_idx == i) dive_idx = NGG_DIVE_NONE;
                    }
                } else {
                    enemy[i].x = (int16_t)(34 + (i % NGG_COLS) * 50 + drift);
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
            /*
             * One ship, not two.  This alternated between 010_ship and
             * 011_ship_alt every sixteen frames as if they were two frames
             * of a thrust animation, but they are two different craft - a
             * teal jet and a dark blue spiked hull - so the player read as
             * morphing rather than animating.  011 stays in the ROM as a
             * second design; nothing draws it as the player.
             */
            ngg_place_sprite(player, NGG_ASSET_SHIP,
                             (int16_t)(ship_x * 8 + 8), 206, 1u);
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

        /* Wait for vblank first, then write the sprite VRAM inside it.  The
         * other way round - drawing and then waiting - puts every sprite
         * write into active display, which is what makes the ships and
         * enemies tear and flicker. */
        waitVbl();
        ngg_scroll_background(bg_page, &bg_scroll, 1u);
        ng_chars_draw();
        frame++;
    }

    soundStopAll();
    ng_vram_clear_all_sprites();
}

/* Generated by the art pipeline - do not edit below this line. */
static const uint16_t ng_extra_pal_1_0[16] = {0x0000,0xc013,0xb778,0x6002,0x5012,0x3334,0x8000,0xc013,0x4013,0xd012,0x8223,0x4013,0x8001,0x7002,0xf112,0xd013};
static const uint16_t ng_extra_pal_1_1[16] = {0x0000,0xc002,0x3edc,0xd013,0xd665,0xf223,0xfbba,0xe002,0x4013,0x3556,0x8000,0x9998,0x6333,0x7001,0x7112,0xc002};
static const uint16_t ng_extra_pal_1_2[16] = {0x0000,0x0103,0x8ddc,0x8567,0x8000,0x5002,0x1bdf,0xf223,0x4003,0x9998,0x9001,0x289b,0x5002,0x8002,0x1103,0xa103};
static const uint16_t ng_extra_pal_1_3[16] = {0x0000,0x4002,0xdffd,0x6666,0x8001,0x6222,0x9001,0x8000,0x4002,0xf445,0x8002,0xcccb,0x5998,0xb778,0x3212,0xc555};
static const uint16_t ng_extra_pal_1_4[16] = {0x0000,0x4002,0x5ace,0x0001,0x2446,0x7002,0xc002,0x8000,0x5002,0x2223,0xc678,0xe113,0x7002,0x6334,0x5002,0xb102};
static const uint16_t ng_extra_pal_1_5[16] = {0x0000,0xa103,0x4eed,0x5665,0x8000,0xeaaa,0x4234,0xb102,0xb556,0x8223,0xa988,0xb102,0xf222,0x8002,0x9111,0x2103};
static const uint16_t ng_extra_pal_1_6[16] = {0x0000,0xe104,0x2dcc,0x8000,0x0667,0x7223,0xe104,0xb103,0xf103,0x8002,0xf113,0x5bba,0x3103,0xf445,0x8777,0x8224};
static const uint16_t ng_extra_pal_1_7[16] = {0x0000,0x8113,0x0002,0x3778,0x3223,0x9000,0x5013,0x8223,0x0113,0x6003,0x9113,0x6113,0x0445,0x3667,0x6003,0x6003};
static const uint16_t ng_extra_pal_1_8[16] = {0x0000,0x2002,0xfbba,0x8666,0x8000,0x4013,0x7002,0x7223,0x9887,0x7445,0xe112,0xd012,0x8224,0x4013,0x9001,0x6002};
static const uint16_t ng_extra_pal_1_9[16] = {0x0000,0x0001,0xbfed,0x4678,0x8000,0xc002,0xf333,0x8001,0x5aa9,0x9776,0x9001,0x9222,0x0556,0x9001,0x0001,0xf334};
static const uint16_t ng_extra_pal_1_10[16] = {0x0000,0x5002,0x3edc,0x8000,0xc456,0x0103,0x2abe,0xf888,0x4002,0xa103,0x6113,0xb778,0x6aaa,0x8113,0x0334,0x8777};
static const uint16_t ng_extra_pal_1_11[16] = {0x0000,0xc003,0x0334,0x9000,0xe113,0x8103,0xa223,0x5002,0x1223,0x1112,0x4003,0xc003,0xb103,0x5002,0xc113,0x4003};
static const uint16_t ng_extra_pal_1_12[16] = {0x0000,0xf002,0x8eed,0x8000,0x7555,0x9aa9,0x6223,0x8ccb,0xe445,0xf112,0x9001,0x6112,0x6002,0x6888,0x8223,0x6aaa};
static const uint16_t ng_extra_pal_1_13[16] = {0x0000,0x4003,0x9001,0xeacf,0x3556,0x8000,0x7223,0x5002,0x8002,0x1001,0x0103,0x3213,0xb778,0xe112,0x8103,0x0445};
static const uint16_t ng_extra_pal_1_14[16] = {0x0000,0xc002,0x8000,0x4ddc,0xf001,0xb667,0x9aa9,0x9333,0x1001,0x9001,0xc555,0x8223,0xe112,0x9001,0x9001,0x8445};
static const uint8_t ng_tile_pals_1[] = {29,26,26,29,16,29,35,16,31,16,29,35,26,25,26,32,30,16,29,35,29,29,29,36,36,36,16,16,29,29,32,32,29,35,35,35,35,30,16,16,31,27,16,16,32,37,29,32,30,27,38,30,27,16,16,36,38,16,16,30,30,32,29,29,28,16,35,35,27,16,16,36,31,16,29,38,39,26,26,26,29,16,38,36,38,35,35,31,31,27,29,39,34,26,26,38,39,29,16,36,27,35,35,31,31,30,16,39,39,39,39,34,26,38,16,16,16,27,31,31,31,30,16,39,39,38,39,16,28,28,16,16,16,38,31,31,31,30,29,39,39,39,39,34,16,16,16,36,36,38,38,31,31,35,29,39,39,39,39,34,28,16,16,36,38,27,27,31,31,35,29,26,26,39,34,39,38,16,16,16,36,16,27,27,31,35,29,37,37,26,39,39,26,16,16,29,16,29,16,27,27,35,29,29,37,37,26,39,39,16,16,39,26,29,32,16,36,36,35,35,25,37,25,33,34,16,29,16,29,39,16,38,38,36,32,32,32,33,25,37,34,39,39,39,29,39,16,35,35,32,26,32,32,25,25,25};
static const uint16_t ng_extra_pal_2_0[16] = {0x0000,0xa114,0x2dcc,0x9000,0xb103,0x5002,0x8667,0x0334,0x9113,0xc114,0x0113,0xa114,0x6999,0xa114,0x3114,0x1223};
static const uint16_t ng_extra_pal_2_1[16] = {0x0000,0x2115,0xe999,0x8125,0xe126,0x0005,0x8556,0x0125,0x5125,0x8015,0xc678,0x4235,0x9665,0x3114,0x8003,0x2446};
static const uint16_t ng_extra_pal_2_2[16] = {0x0000,0x1001,0xafed,0x7002,0x8000,0x5665,0xe002,0x6223,0x6002,0x2baa,0x0001,0x1001,0x8667,0xf002,0xf001,0x1001};
static const uint16_t ng_extra_pal_2_3[16] = {0x0000,0x4013,0xbfed,0x8000,0x6666,0x8001,0x9333,0xb113,0xbcba,0x6002,0xb778,0xe445,0xd998,0x7002,0x8223,0xa003};
static const uint16_t ng_extra_pal_2_4[16] = {0x0000,0x0114,0x9ccb,0xc567,0x9003,0x8001,0x1114,0x9334,0xc114,0x8224,0x1113,0xe114,0x4678,0x8003,0x1aa9,0x7445};
static const uint16_t ng_extra_pal_2_5[16] = {0x0000,0x9237,0x3edc,0xc666,0xa226,0x3446,0xe016,0x3226,0x5125,0xf444,0x8237,0x4236,0x9446,0x8556,0x7126,0xf126};
static const uint16_t ng_extra_pal_2_6[16] = {0x0000,0x7014,0x9aa9,0x3445,0x4234,0x2114,0x8004,0x7667,0x3114,0xb114,0x3114,0x7888,0xb224,0x8125,0x3114,0x0125};
static const uint16_t ng_extra_pal_2_7[16] = {0x0000,0x4002,0x8000,0xdaa9,0xb102,0x4567,0xd233,0x4002,0x9001,0x5ddc,0x8002,0x5002,0x4002,0x7112,0x7002,0xb102};
static const uint16_t ng_extra_pal_2_8[16] = {0x0000,0x7114,0x4678,0xc005,0xf335,0x9225,0xf114,0x0556,0x8004,0x1224,0x6115,0x6115,0x0115,0xc567,0xa215,0xe446};
static const uint16_t ng_extra_pal_2_9[16] = {0x0000,0xf114,0x6bbb,0x7667,0x2115,0x9000,0x8005,0xe446,0x3114,0x9003,0x9002,0xe225,0x2115,0x9004,0x9225,0xf888};
static const uint16_t ng_extra_pal_2_10[16] = {0x0000,0xd002,0x3667,0x8002,0x7223,0x5002,0x1222,0xc002,0x8001,0xe224,0xf445,0xe113,0x4003,0x8112,0x5002,0x5002};
static const uint16_t ng_extra_pal_2_11[16] = {0x0000,0xb113,0x3556,0x3223,0xa114,0x4013,0x0003,0xc345,0xa114,0x6014,0xd234,0x8014,0x0223,0xa114,0xa114,0x9124};
static const uint16_t ng_extra_pal_2_12[16] = {0x0000,0x9001,0x9113,0x1bdf,0xb556,0x9000,0x8001,0xa103,0xf223,0xe113,0x8113,0xc789,0x9113,0x9113,0x9113,0x6003};
static const uint16_t ng_extra_pal_2_13[16] = {0x0000,0x7002,0xfbba,0x8000,0x1555,0x5013,0x7223,0x4013,0x3778,0x1998,0xd012,0xd123,0xc013,0x0445,0x8888,0xd012};
static const uint16_t ng_extra_pal_2_14[16] = {0x0000,0x9113,0xdffd,0x9776,0x8000,0x7223,0x5998,0x7002,0xf445,0x0114,0x1113,0x8003,0x9001,0x7113,0x4567,0x1113};
static const uint8_t ng_tile_pals_2[] = {40,52,53,40,49,41,41,41,48,48,48,54,47,47,47,47,52,52,53,40,46,41,45,45,45,41,49,54,40,47,42,47,52,52,53,51,46,41,45,45,45,41,48,44,52,50,47,47,47,47,53,51,51,41,45,45,45,41,48,44,40,47,47,47,42,42,42,43,40,41,41,45,41,41,48,44,54,47,47,47,42,54,42,43,51,40,49,49,48,41,48,44,54,52,47,47,52,42,42,43,40,44,48,48,48,48,49,40,52,47,47,47,52,42,42,43,40,40,54,44,48,49,44,40,40,50,47,47,52,42,42,43,40,52,54,44,46,49,44,40,40,47,47,47,17,42,42,42,52,40,44,46,49,48,44,40,50,50,42,42,42,47,42,42,17,40,40,49,41,48,54,52,50,50,52,52,42,47,42,43,52,40,46,41,41,49,54,52,47,47,47,47,42,42,42,42,52,40,41,41,41,46,40,47,50,47,47,47,42,42,42,42,52,44,49,41,46,46,40,52,47,47,47,47,53,42,42,42,17,44,49,41,41,41,44,47,47,47,47,47,53,53,53,52,40,44,49,41,41,46,54,47,17,42,42,52};
static const uint16_t ng_extra_pal_5_0[16] = {0x0000,0x103b,0x105f,0x3223,0xa446,0x4568,0x288a,0x3bbc,0x1eee,0x7fff,0x7fff,0x7fff,0x7fff,0x7fff,0x7fff,0x7fff};
static const uint16_t ng_extra_pal_5_1[16] = {0x0000,0x2211,0x0543,0x2865,0x9ba7,0x4dda,0x6ffc,0x6ffc,0x6ffc,0x6ffc,0x6ffc,0x6ffc,0x6ffc,0x6ffc,0x6ffc,0x6ffc};
static const uint16_t ng_extra_pal_5_2[16] = {0x0000,0x3021,0x103b,0x1050,0x105f,0x2070,0x728f,0xc3a0,0x76cf,0x07c0,0x2ad0,0xcf80,0xcf80,0xcf80,0xcf80,0xcf80};
static const uint8_t ng_tile_pals_5[] = {19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,55,55,56,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,55,57,19,19,19,19,19,19,19,19};
static const uint16_t ng_extra_pal_9_0[16] = {0x0000,0xb212,0x8fc0,0xe02c,0x2a20,0xa989,0x624a,0x7621,0x2e60,0x6018,0xc556,0xf39e,0x5a60,0xa115,0xe15d,0x4247};
static const uint16_t ng_extra_pal_9_1[16] = {0x0000,0x212a,0x0e90,0xc5bd,0x5431,0x037e,0xb22f,0x6014,0xc940,0xb456,0x8ccd,0xb25e,0xc359,0x824a,0x738d,0x9435};
static const uint16_t ng_extra_pal_9_2[16] = {0x0000,0x5016,0xa960,0x959a,0xc13f,0x1311,0x316e,0xc247,0xb14e,0x6532,0xb14c,0x949e,0xc02b,0x1114,0x325a,0xc123};
static const uint16_t ng_extra_pal_9_3[16] = {0x0000,0x1002,0xb34f,0x1998,0xf04a,0x0008,0xe557,0xd027,0x9666,0xe246,0x099a,0x0445,0xc25a,0x804e,0x8018,0xd028};
static const uint16_t ng_extra_pal_9_4[16] = {0x0000,0x901a,0x801b,0xa01b,0x1014,0x103c,0xd233,0x625a,0x1444,0xf444,0x9777,0x799a,0x799a,0x799a,0x799a,0x799a};
static const uint8_t ng_tile_pals_9[] = {23,23,23,61,59,62,60,58,60};


void NEOGEO_USER showScreen1(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 1 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal1[16];
setpal(pal1,0x0,0xc002,0x9000,0xe104,0x5002,0x1bba,0x1,0xf002,0x7556,0x1001,0xb103,0xf223,0xd002,0x9001,0x8000,0xa103);
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
ng_palette_load_bank(25, ng_extra_pal_1_0);
ng_palette_load_bank(26, ng_extra_pal_1_1);
ng_palette_load_bank(27, ng_extra_pal_1_2);
ng_palette_load_bank(28, ng_extra_pal_1_3);
ng_palette_load_bank(29, ng_extra_pal_1_4);
ng_palette_load_bank(30, ng_extra_pal_1_5);
ng_palette_load_bank(31, ng_extra_pal_1_6);
ng_palette_load_bank(32, ng_extra_pal_1_7);
ng_palette_load_bank(33, ng_extra_pal_1_8);
ng_palette_load_bank(34, ng_extra_pal_1_9);
ng_palette_load_bank(35, ng_extra_pal_1_10);
ng_palette_load_bank(36, ng_extra_pal_1_11);
ng_palette_load_bank(37, ng_extra_pal_1_12);
ng_palette_load_bank(38, ng_extra_pal_1_13);
ng_palette_load_bank(39, ng_extra_pal_1_14);
uint16_t spal1_1[16]={0x1d00,0x1e00,0x1d00,0x1e00,0x1c00,0x1d00,0x2700,0x1a00,0x1c00,0x1000,0x1c00,0x2600,0x1a00,0x2700,0x2200,0x2200};
uint16_t spal1_2[16]={0x1a00,0x1000,0x2300,0x1b00,0x1000,0x1000,0x1d00,0x2600,0x1c00,0x1000,0x1000,0x1000,0x1000,0x1000,0x1000,0x2700};
uint16_t spal1_3[16]={0x1a00,0x1d00,0x2300,0x2600,0x2300,0x2600,0x1000,0x1000,0x1000,0x1000,0x1000,0x1000,0x1000,0x1000,0x1d00,0x2700};
uint16_t spal1_4[16]={0x1d00,0x2300,0x2300,0x1e00,0x2300,0x2400,0x2400,0x1000,0x1000,0x2400,0x2400,0x1000,0x1d00,0x2700,0x1000,0x2700};
uint16_t spal1_5[16]={0x1000,0x1d00,0x2300,0x1b00,0x1b00,0x2600,0x1b00,0x1000,0x1000,0x2400,0x2600,0x2400,0x1000,0x1a00,0x1d00,0x1d00};
uint16_t spal1_6[16]={0x1d00,0x1d00,0x1e00,0x1000,0x1000,0x2300,0x2300,0x1b00,0x2600,0x2600,0x1b00,0x1000,0x1d00,0x1d00,0x2700,0x2700};
uint16_t spal1_7[16]={0x2300,0x1d00,0x1000,0x1000,0x1000,0x2300,0x2300,0x1f00,0x1f00,0x2600,0x1b00,0x1b00,0x1000,0x2000,0x1000,0x1000};
uint16_t spal1_8[16]={0x1000,0x2400,0x1000,0x2400,0x2400,0x1f00,0x1f00,0x1f00,0x1f00,0x1f00,0x1f00,0x1b00,0x1b00,0x1000,0x2600,0x2300};
uint16_t spal1_9[16]={0x1f00,0x2400,0x1f00,0x2600,0x1f00,0x1f00,0x1f00,0x1f00,0x1f00,0x1f00,0x1f00,0x1f00,0x1b00,0x2400,0x2600,0x2300};
uint16_t spal1_10[16]={0x1000,0x2400,0x1b00,0x1000,0x1000,0x1b00,0x1e00,0x1e00,0x1e00,0x2300,0x2300,0x2300,0x2300,0x2400,0x2400,0x2000};
uint16_t spal1_11[16]={0x1d00,0x1000,0x1000,0x1000,0x1d00,0x1d00,0x1000,0x1000,0x1d00,0x1d00,0x1d00,0x1d00,0x1d00,0x2300,0x2000,0x1a00};
uint16_t spal1_12[16]={0x2300,0x1000,0x1000,0x1e00,0x2600,0x2700,0x2700,0x2700,0x2700,0x2700,0x1a00,0x2500,0x1d00,0x2300,0x2000,0x2000};
uint16_t spal1_13[16]={0x1a00,0x1d00,0x2000,0x1e00,0x2700,0x2200,0x2700,0x2700,0x2700,0x2700,0x1a00,0x2500,0x2500,0x1900,0x2000,0x2000};
uint16_t spal1_14[16]={0x1900,0x1d00,0x2500,0x2000,0x1a00,0x1a00,0x2700,0x2600,0x2700,0x2700,0x2700,0x1a00,0x2500,0x2500,0x2100,0x1900};
uint16_t spal1_15[16]={0x1a00,0x2000,0x1d00,0x1d00,0x1a00,0x1a00,0x2700,0x2700,0x2700,0x2700,0x2200,0x2700,0x1a00,0x1900,0x1900,0x1900};
uint16_t spal1_16[16]={0x2000,0x2000,0x2000,0x1d00,0x1a00,0x2600,0x2200,0x1000,0x2200,0x2200,0x2700,0x2700,0x2700,0x2100,0x2500,0x1900};
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
setpal(pal2,0x0,0x4002,0x8000,0x7126,0xf114,0x5bba,0x1001,0x7556,0x8113,0xd125,0x8001,0x1,0xf223,0xc9bd,0xa005,0x9113);
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
ng_palette_load_bank(40, ng_extra_pal_2_0);
ng_palette_load_bank(41, ng_extra_pal_2_1);
ng_palette_load_bank(42, ng_extra_pal_2_2);
ng_palette_load_bank(43, ng_extra_pal_2_3);
ng_palette_load_bank(44, ng_extra_pal_2_4);
ng_palette_load_bank(45, ng_extra_pal_2_5);
ng_palette_load_bank(46, ng_extra_pal_2_6);
ng_palette_load_bank(47, ng_extra_pal_2_7);
ng_palette_load_bank(48, ng_extra_pal_2_8);
ng_palette_load_bank(49, ng_extra_pal_2_9);
ng_palette_load_bank(50, ng_extra_pal_2_10);
ng_palette_load_bank(51, ng_extra_pal_2_11);
ng_palette_load_bank(52, ng_extra_pal_2_12);
ng_palette_load_bank(53, ng_extra_pal_2_13);
ng_palette_load_bank(54, ng_extra_pal_2_14);
uint16_t spal2_1[16]={0x2800,0x3400,0x3400,0x2f00,0x2a00,0x2a00,0x3400,0x3400,0x3400,0x1100,0x2a00,0x2a00,0x2a00,0x2a00,0x3500,0x3500};
uint16_t spal2_2[16]={0x3400,0x3400,0x3400,0x2f00,0x2a00,0x3600,0x2a00,0x2a00,0x2a00,0x2a00,0x2f00,0x2f00,0x2a00,0x2a00,0x2a00,0x3500};
uint16_t spal2_3[16]={0x3500,0x3500,0x3500,0x3500,0x2a00,0x2a00,0x2a00,0x2a00,0x2a00,0x2a00,0x2a00,0x2a00,0x2a00,0x2a00,0x2a00,0x3500};
uint16_t spal2_4[16]={0x2800,0x2800,0x3300,0x3300,0x2b00,0x2b00,0x2b00,0x2b00,0x2b00,0x2a00,0x2a00,0x2b00,0x2a00,0x2a00,0x2a00,0x3400};
uint16_t spal2_5[16]={0x3100,0x2e00,0x2e00,0x3300,0x2800,0x3300,0x2800,0x2800,0x2800,0x3400,0x1100,0x3400,0x3400,0x3400,0x1100,0x2800};
uint16_t spal2_6[16]={0x2900,0x2900,0x2900,0x2900,0x2900,0x2800,0x2c00,0x2800,0x3400,0x2800,0x2800,0x2800,0x2800,0x2c00,0x2c00,0x2c00};
uint16_t spal2_7[16]={0x2900,0x2d00,0x2d00,0x2d00,0x2900,0x3100,0x3000,0x3600,0x3600,0x2c00,0x2800,0x2e00,0x2900,0x3100,0x3100,0x3100};
uint16_t spal2_8[16]={0x2900,0x2d00,0x2d00,0x2d00,0x2d00,0x3100,0x3000,0x2c00,0x2c00,0x2e00,0x3100,0x2900,0x2900,0x2900,0x2900,0x2900};
uint16_t spal2_9[16]={0x3000,0x2d00,0x2d00,0x2d00,0x2900,0x3000,0x3000,0x3000,0x2e00,0x3100,0x2900,0x2900,0x2900,0x2e00,0x2900,0x2900};
uint16_t spal2_10[16]={0x3000,0x2900,0x2900,0x2900,0x2900,0x2900,0x3000,0x3100,0x3100,0x3000,0x3000,0x3100,0x2e00,0x2e00,0x2900,0x2e00};
uint16_t spal2_11[16]={0x3000,0x3100,0x3000,0x3000,0x3000,0x3000,0x3100,0x2c00,0x2c00,0x2c00,0x3600,0x3600,0x2800,0x2800,0x2c00,0x3600};
uint16_t spal2_12[16]={0x3600,0x3600,0x2c00,0x2c00,0x2c00,0x2c00,0x2800,0x2800,0x2800,0x2800,0x3400,0x3400,0x2f00,0x3400,0x2f00,0x2f00};
uint16_t spal2_13[16]={0x2f00,0x2800,0x3400,0x2800,0x3600,0x3600,0x3400,0x2800,0x2800,0x3200,0x3200,0x2f00,0x3200,0x2f00,0x2f00,0x1100};
uint16_t spal2_14[16]={0x2f00,0x2f00,0x3200,0x2f00,0x2f00,0x3400,0x2f00,0x3200,0x2f00,0x3200,0x3200,0x2f00,0x2f00,0x2f00,0x2f00,0x2a00};
uint16_t spal2_15[16]={0x2f00,0x2a00,0x2f00,0x2f00,0x2f00,0x2f00,0x2f00,0x2f00,0x2f00,0x2a00,0x3400,0x2f00,0x2f00,0x2f00,0x2f00,0x2a00};
uint16_t spal2_16[16]={0x2f00,0x2f00,0x2f00,0x2f00,0x2f00,0x2f00,0x2f00,0x2f00,0x2f00,0x2a00,0x3400,0x2f00,0x2f00,0x2f00,0x2f00,0x3400};
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
setpal(pal3,0x0,0x7245,0x28de,0x1124,0x5367,0x759a,0xc57a,0xb234,0x112,0x8346,0x1777,0x2113,0x6445,0xe357,0x4136,0x9333);
load_palettes(pal3,PALETTES+PALOFFSET*18);
}


void NEOGEO_USER showScreen4(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 4 ******************************************/
uint16_t  pal4[16];
setpal(pal4,0x0,0x7245,0x28de,0x1124,0x5367,0x759a,0xc57a,0xb234,0x112,0x8346,0x1777,0x2113,0x6445,0xe357,0x4136,0x9333);
load_palettes(pal4,PALETTES+PALOFFSET*18);
}


void NEOGEO_USER showScreen5(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 5 ******************************************/
uint16_t  pal5[16];
setpal(pal5,0x0,0xcf20,0xc3a0,0x5567,0x6fc0,0xcf80,0xa04e,0x222,0xf875,0xccdd,0x3016,0x1050,0xc8d0,0x728f,0xe070,0x543);
load_palettes(pal5,PALETTES+PALOFFSET*19);
}


void NEOGEO_USER showScreen6(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 6 ******************************************/
uint16_t  pal6[16];
setpal(pal6,0x0,0x57d2,0x88d3,0xa9c3,0xb9c3,0xd9c3,0xbab4,0x1ba7,0x9cd7,0xfcd8,0x9de9,0x5dea,0x5dea,0x5dea,0x5dea,0x5dea);
load_palettes(pal6,PALETTES+PALOFFSET*20);
}


void NEOGEO_USER showScreen7(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 7 ******************************************/
uint16_t  pal7[16];
setpal(pal7,0x0,0xcf44,0x7fc2,0x7ffe,0xef90,0x4f10,0x4f60,0x6ffc,0x4f22,0x6f40,0x6f70,0x5fe5,0xdfc1,0x4f30,0x4f33,0xcf11);
load_palettes(pal7,PALETTES+PALOFFSET*21);
}


void NEOGEO_USER showScreen8(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 8 ******************************************/
uint16_t  pal8[16];
setpal(pal8,0x0,0x7ffe,0xf10,0x7ff1,0x8600,0xef90,0x7ff9,0xca00,0xefc0,0x6ff7,0x4f50,0x6ffc,0x7ff4,0xcfb0,0x4fe0,0x6f70);
load_palettes(pal8,PALETTES+PALOFFSET*22);
}


void NEOGEO_USER showScreen9(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 9 ******************************************/
uint16_t  pal9[16];
setpal(pal9,0x0,0xa213,0xaeb0,0xc13f,0xcabc,0x7930,0xa25d,0x6a60,0x7017,0xc48a,0xcf90,0xe556,0x702b,0xe237,0x4532,0x237d);
load_palettes(pal9,PALETTES+PALOFFSET*23);
}


void NEOGEO_USER showScreen10(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 10 ******************************************/
uint16_t  pal10[16];
setpal(pal10,0x0,0xf112,0x9bbb,0xa458,0xa224,0xa667,0x3113,0xf445,0xc789,0xe334,0xc57a,0x6236,0xf457,0x235,0x489a,0xd678);
load_palettes(pal10,PALETTES+PALOFFSET*24);
}

static const NGPaletteAsset ng_screen_palette_assets[] = {
    {1,16,{0x0,0xc002,0x9000,0xe104,0x5002,0x1bba,0x1,0xf002,0x7556,0x1001,0xb103,0xf223,0xd002,0x9001,0x8000,0xa103}},
    {2,17,{0x0,0x4002,0x8000,0x7126,0xf114,0x5bba,0x1001,0x7556,0x8113,0xd125,0x8001,0x1,0xf223,0xc9bd,0xa005,0x9113}},
    {3,18,{0x0,0x7245,0x28de,0x1124,0x5367,0x759a,0xc57a,0xb234,0x112,0x8346,0x1777,0x2113,0x6445,0xe357,0x4136,0x9333}},
    {4,18,{0x0,0x7245,0x28de,0x1124,0x5367,0x759a,0xc57a,0xb234,0x112,0x8346,0x1777,0x2113,0x6445,0xe357,0x4136,0x9333}},
    {5,19,{0x0,0xcf20,0xc3a0,0x5567,0x6fc0,0xcf80,0xa04e,0x222,0xf875,0xccdd,0x3016,0x1050,0xc8d0,0x728f,0xe070,0x543}},
    {6,20,{0x0,0x57d2,0x88d3,0xa9c3,0xb9c3,0xd9c3,0xbab4,0x1ba7,0x9cd7,0xfcd8,0x9de9,0x5dea,0x5dea,0x5dea,0x5dea,0x5dea}},
    {7,21,{0x0,0xcf44,0x7fc2,0x7ffe,0xef90,0x4f10,0x4f60,0x6ffc,0x4f22,0x6f40,0x6f70,0x5fe5,0xdfc1,0x4f30,0x4f33,0xcf11}},
    {8,22,{0x0,0x7ffe,0xf10,0x7ff1,0x8600,0xef90,0x7ff9,0xca00,0xefc0,0x6ff7,0x4f50,0x6ffc,0x7ff4,0xcfb0,0x4fe0,0x6f70}},
    {9,23,{0x0,0xa213,0xaeb0,0xc13f,0xcabc,0x7930,0xa25d,0x6a60,0x7017,0xc48a,0xcf90,0xe556,0x702b,0xe237,0x4532,0x237d}},
    {10,24,{0x0,0xf112,0x9bbb,0xa458,0xa224,0xa667,0x3113,0xf445,0xc789,0xe334,0xc57a,0x6236,0xf457,0x235,0x489a,0xd678}},
};
const uint16_t ng_screen_palette_count = 10;

uint8_t NEOGEO_USER ng_load_screen_palette(uint16_t screen_id) {
    if (!ng_palette_load_asset(ng_screen_palette_assets, ng_screen_palette_count, screen_id)) return 0;
    switch (screen_id) {
    case 1:
        ng_palette_load_bank(25, ng_extra_pal_1_0);
        ng_palette_load_bank(26, ng_extra_pal_1_1);
        ng_palette_load_bank(27, ng_extra_pal_1_2);
        ng_palette_load_bank(28, ng_extra_pal_1_3);
        ng_palette_load_bank(29, ng_extra_pal_1_4);
        ng_palette_load_bank(30, ng_extra_pal_1_5);
        ng_palette_load_bank(31, ng_extra_pal_1_6);
        ng_palette_load_bank(32, ng_extra_pal_1_7);
        ng_palette_load_bank(33, ng_extra_pal_1_8);
        ng_palette_load_bank(34, ng_extra_pal_1_9);
        ng_palette_load_bank(35, ng_extra_pal_1_10);
        ng_palette_load_bank(36, ng_extra_pal_1_11);
        ng_palette_load_bank(37, ng_extra_pal_1_12);
        ng_palette_load_bank(38, ng_extra_pal_1_13);
        ng_palette_load_bank(39, ng_extra_pal_1_14);
        break;
    case 2:
        ng_palette_load_bank(40, ng_extra_pal_2_0);
        ng_palette_load_bank(41, ng_extra_pal_2_1);
        ng_palette_load_bank(42, ng_extra_pal_2_2);
        ng_palette_load_bank(43, ng_extra_pal_2_3);
        ng_palette_load_bank(44, ng_extra_pal_2_4);
        ng_palette_load_bank(45, ng_extra_pal_2_5);
        ng_palette_load_bank(46, ng_extra_pal_2_6);
        ng_palette_load_bank(47, ng_extra_pal_2_7);
        ng_palette_load_bank(48, ng_extra_pal_2_8);
        ng_palette_load_bank(49, ng_extra_pal_2_9);
        ng_palette_load_bank(50, ng_extra_pal_2_10);
        ng_palette_load_bank(51, ng_extra_pal_2_11);
        ng_palette_load_bank(52, ng_extra_pal_2_12);
        ng_palette_load_bank(53, ng_extra_pal_2_13);
        ng_palette_load_bank(54, ng_extra_pal_2_14);
        break;
    case 5:
        ng_palette_load_bank(55, ng_extra_pal_5_0);
        ng_palette_load_bank(56, ng_extra_pal_5_1);
        ng_palette_load_bank(57, ng_extra_pal_5_2);
        break;
    case 9:
        ng_palette_load_bank(58, ng_extra_pal_9_0);
        ng_palette_load_bank(59, ng_extra_pal_9_1);
        ng_palette_load_bank(60, ng_extra_pal_9_2);
        ng_palette_load_bank(61, ng_extra_pal_9_3);
        ng_palette_load_bank(62, ng_extra_pal_9_4);
        break;
    default: break;
    }
    return 1;
}

const uint16_t * NEOGEO_USER ng_get_screen_palette(uint16_t screen_id) {
    uint16_t i;
    for (i = 0; i < ng_screen_palette_count; i++) {
        if (ng_screen_palette_assets[i].asset_id == screen_id)
            return ng_screen_palette_assets[i].colors;
    }
    return 0;
}

const NGArtAsset ng_screen_art_assets[] = {
    {1,NG_ART_TYPE_BACKGROUND,16,0,255,16,16,16,16,0,0,256,256,ng_tile_pals_1 + 0},
    {2,NG_ART_TYPE_BACKGROUND,17,256,511,16,16,16,16,0,0,256,256,ng_tile_pals_2 + 0},
    {3,NG_ART_TYPE_SPRITE,18,742,767,4,2,2,16,96,224,40,26,0},
    {4,NG_ART_TYPE_SPRITE,18,998,1023,4,2,2,16,96,224,43,26,0},
    {5,NG_ART_TYPE_SPRITE,19,1222,1279,4,4,4,16,96,192,64,64,ng_tile_pals_5 + 198},
    {6,NG_ART_TYPE_SPRITE,20,1399,1535,2,2,2,16,112,112,5,11,0},
    {7,NG_ART_TYPE_SPRITE,21,1655,1791,2,2,2,16,112,112,5,11,0},
    {8,NG_ART_TYPE_SPRITE,22,1894,2047,4,4,4,16,96,96,37,37,0},
    {9,NG_ART_TYPE_SPRITE,23,2051,2303,3,2,2,3,0,16,48,31,ng_tile_pals_9 + 3},
    {10,NG_ART_TYPE_SPRITE,24,2551,2559,2,1,1,16,112,240,22,13,0},
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
