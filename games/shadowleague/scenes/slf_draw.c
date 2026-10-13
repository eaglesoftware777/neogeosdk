/*
 * Shadow League Fighting: hardware side.
 *
 * Sprite slots: stage 1-32 (32 strips x 14 tiles), shadows 33-40, fighters
 * 41-47 and 48-54 (7 strips x 8 tiles; the attacking fighter takes the
 * front slot), hit effects 55-62. Palette banks: FIX 0-7, fighters 16-17,
 * effects 18-20, shadow 21, stage 32-41. Every VRAM write goes through the
 * SDK's deferred sprite groups and FIX queue, committed in the blank.
 */
#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include "sdk/2d_engine/ng_fix.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "games/shadowleague/scenes/slf.h"
#include "games/shadowleague/artbox/generated/slf_assets.h"

void NEOGEO_USER playSFX(uint8_t id);
void NEOGEO_USER playSFXB(uint8_t id);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundCancelFade(void);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER soundSetSSGVolume(uint8_t v);
void NEOGEO_USER soundSetFMVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBLoop(uint8_t enable);

#define SPR_STAGE_FAR  1u
#define SPR_STAGE     33u
#define SPR_SHADOW    65u
#define SPR_FIGHTER   73u
#define SPR_PROJ      87u
#define SPR_STRIKER   93u
#define SPR_FX       107u
#define SPR_PORTRAITS 115u
#define SPR_CURSOR_1P 129u
#define SPR_CURSOR_2P 131u
#define PAL_FIGHTER   16u
#define PAL_FX        18u
#define PAL_SHADOW    21u
#define PAL_PORTRAITS 22u
#define PAL_CURSOR_1P 29u
#define PAL_CURSOR_2P 30u
#define FX_SLOTS       4u
#define FX_TICKS       3u

static uint16_t s_palette_base[104u * 16u];
static uint16_t s_palette_out[104u * 16u];
static NGSpriteGroup s_stage_far, s_stage, s_fighter[2], s_shadow[2], s_fx[FX_SLOTS];
static NGSpriteGroup s_proj[NGF_PROJECTILES], s_striker[2];
static NGSpriteGroup s_portraits[7], s_cursor[2];
static uint8_t s_stage_id;
static uint8_t s_stage_live, s_stage_far_live;
static uint8_t s_fighter_live[2], s_fighter_pal[2], s_shadow_live[2];
static uint8_t s_proj_live[NGF_PROJECTILES], s_striker_live[2];
static uint8_t s_portraits_live, s_cursor_live[2], s_port_last_win;
static uint8_t s_fx_kind[FX_SLOTS], s_fx_timer[FX_SLOTS], s_fx_live[FX_SLOTS];
static int16_t s_fx_x[FX_SLOTS], s_fx_y[FX_SLOTS];
static uint8_t s_joy[2], s_joy_prev[2], s_start, s_start_prev;
static uint16_t s_frames, s_rng;
/* HUD cache: what the FIX layer shows now. */
static uint8_t s_hud_valid, s_hud_life[2], s_hud_power[2], s_hud_stock[2], s_hud_wins[2], s_hud_secs;
static uint8_t s_hud_combo[2], s_hud_combo_timer[2];

/* ------------------------------------------------------------ palettes */
void NEOGEO_USER slf_presentation_init(void)
{
    ng_palfx_screen_init(s_palette_base, s_palette_out, 104u);
    ng_fix_init();
    ng_fix_set_ascii_base(0xD00u);
    s_stage_id = 0u;
    s_port_last_win = 0xFFu;
    s_rng = 0x5A17u;
}

void NEOGEO_USER slf_fix_palettes_init(void)
{
    uint8_t i;
    for (i = 0u; i < 8u; i++) ng_palfx_screen_load(i, slf_fix_pals[i]);
    for (i = 0u; i < 3u; i++) ng_palfx_screen_load((uint8_t)(PAL_FX + i), slf_fx_pals[i]);
    ng_palfx_screen_load(PAL_SHADOW, slf_shadow_pal);
    for (i = 0u; i < SLF_STAGE_BANKS; i++) ng_palfx_screen_load((uint8_t)(SLF_PAL_STAGE + i), slf_stage_pals[s_stage_id][i]);
}

void NEOGEO_USER slf_fighter_palettes(uint8_t def0, uint8_t alt0, uint8_t def1, uint8_t alt1)
{
    ng_palfx_screen_load(PAL_FIGHTER, slf_fighter_pals[def0][alt0 ? 1 : 0]);
    ng_palfx_screen_load((uint8_t)(PAL_FIGHTER + 1u), slf_fighter_pals[def1][alt1 ? 1 : 0]);
}

/* ------------------------------------------------------------ frames */
void NEOGEO_USER slf_fade_out(void)
{
    ng_palfx_screen_fade_out(16u, 8u);
    while (ng_palfx_screen_fading()) slf_frame();
    slf_frame();
}

void NEOGEO_USER slf_scene_begin(void)
{
    uint8_t i;
    slf_fade_out();
    waitVbl();
    setBACKDROP(BLACK);
    clearFix();
    clearSprs();
    ng_sprite_hide_all();
    s_stage_live = 0u;
    s_stage_far_live = 0u;
    for (i = 0u; i < 2u; i++) {
        s_fighter_live[i] = s_shadow_live[i] = 0u;
        s_striker_live[i] = 0u;
    }
    for (i = 0u; i < NGF_PROJECTILES; i++) s_proj_live[i] = 0u;
    for (i = 0u; i < FX_SLOTS; i++) s_fx_live[i] = s_fx_timer[i] = 0u;
    s_portraits_live = 0u;
    s_port_last_win = 0xFFu;
    s_cursor_live[0] = s_cursor_live[1] = 0u;
    s_hud_valid = 0u;
    ng_fix_invalidate_all();
    slf_fix_palettes_init();
    ng_palfx_screen_fade_in(0u, 12u);
    waitVbl();
}

uint16_t NEOGEO_USER slf_frame(void)
{
    waitVbl();
    if (!ng_vram_window_open()) waitVbl();
    ng_palfx_vblank();
    ng_vram_commit();
    ng_palette_fx_update();
    s_joy_prev[0] = s_joy[0];
    s_joy_prev[1] = s_joy[1];
    s_start_prev = s_start;
    s_joy[0] = NEO_REGISTER8(BIOS_P1CURRENT);
    s_joy[1] = NEO_REGISTER8(BIOS_P2CURRENT);
    s_start = NEO_REGISTER8(0x10FDACu);          /* BIOS_STATCURNT */
    s_frames++;
    s_rng ^= (uint16_t)(s_joy[0] + (s_joy[1] << 8) + s_frames);
    return s_frames;
}

uint8_t NEOGEO_USER slf_joy(uint8_t p) { return s_joy[p & 1u]; }
uint8_t NEOGEO_USER slf_pressed(uint8_t p) { return (uint8_t)(s_joy[p & 1u] & (uint8_t)~s_joy_prev[p & 1u]); }
uint8_t NEOGEO_USER slf_start_pressed(uint8_t p)
{
    uint8_t bit = p ? 4u : 1u;
    return (uint8_t)((s_start & bit) && !(s_start_prev & bit));
}

uint16_t NEOGEO_USER slf_rand(void)
{
    s_rng = (uint16_t)(s_rng * 2053u + 13849u);
    return s_rng;
}

/* ------------------------------------------------------------ FIX text */
void NEOGEO_USER slf_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    char line[41];
    uint8_t i;
    if (!text || x >= 40u || y >= 28u) return;
    for (i = 0u; i < (uint8_t)(40u - x) && text[i]; i++) line[i] = text[i];
    line[i] = '\0';
    ng_fix_puts(x, y, line, pal);
}

static uint8_t NEOGEO_USER slen(const char *s)
{
    uint8_t n = 0u;
    while (s[n] && n < 40u) n++;
    return n;
}

void NEOGEO_USER slf_puts_center(uint8_t y, const char *text, uint8_t pal)
{
    uint8_t n = slen(text);
    slf_puts((uint8_t)(n < 40u ? (40u - n) / 2u : 0u), y, text, pal);
}

void NEOGEO_USER slf_puts_right(uint8_t x_end, uint8_t y, const char *text, uint8_t pal)
{
    uint8_t n = slen(text);
    slf_puts((uint8_t)(x_end + 1u > n ? x_end + 1u - n : 0u), y, text, pal);
}

void NEOGEO_USER slf_big(uint8_t x, uint8_t y, uint16_t tile, uint8_t w, uint8_t h, uint8_t pal)
{
    uint8_t r, c;
    for (r = 0u; r < h; r++)
        for (c = 0u; c < w; c++)
            if ((uint8_t)(x + c) < 40u && (uint8_t)(y + r) < 28u)
                ng_fix_put_tile((uint8_t)(x + c), (uint8_t)(y + r), (uint16_t)(tile + r * w + c), pal);
}

void NEOGEO_USER slf_big_center(uint8_t y, uint16_t tile, uint8_t w, uint8_t h, uint8_t pal)
{
    slf_big((uint8_t)(w < 40u ? (40u - w) / 2u : 0u), y, tile, w, h, pal);
}

void NEOGEO_USER slf_clear_rows(uint8_t y0, uint8_t y1)
{
    uint8_t x, y;
    for (y = y0; y <= y1 && y < 28u; y++)
        for (x = 0u; x < 40u; x++) ng_fix_blank_cell(x, y);
}

/* ------------------------------------------------------------ sprites */
void NEOGEO_USER slf_stage_set(uint8_t stage_id)
{
    uint8_t b;
    if (stage_id >= SLF_STAGE_COUNT) stage_id = 0u;
    if (stage_id == s_stage_id && s_stage_live) return;
    s_stage_id = stage_id;
    for (b = 0u; b < SLF_STAGE_BANKS; b++) {
        ng_palfx_screen_load((uint8_t)(SLF_PAL_STAGE + b), slf_stage_pals[s_stage_id][b]);
    }
    if (s_stage_far_live) {
        ng_sprite_group_cancel(&s_stage_far);
        ng_sprite_group_hide(&s_stage_far);
        s_stage_far_live = 0u;
    }
    if (s_stage_live) {
        ng_sprite_group_cancel(&s_stage);
        ng_sprite_group_hide(&s_stage);
        s_stage_live = 0u;
    }
}

uint8_t NEOGEO_USER slf_stage_current(void)
{
    return s_stage_id;
}

void NEOGEO_USER slf_stage_show(int16_t camera_x)
{
    int16_t far_x = (int16_t)(camera_x >> 1);
    if (!s_stage_far_live) {
        ng_sprite_group_init(&s_stage_far, SPR_STAGE_FAR, SLF_STAGE_COLS, SLF_STAGE_FAR_ROWS, slf_stage_far_tiles[s_stage_id], SLF_PAL_STAGE);
        ng_sprite_group_set_tile_stride(&s_stage_far, SLF_STAGE_COLS);
        ng_sprite_group_set_palette_map(&s_stage_far, slf_stage_far_maps[s_stage_id]);
        ng_sprite_group_set_pos(&s_stage_far, (int16_t)-far_x, 0);
        ng_sprite_group_upload(&s_stage_far);
        s_stage_far_live = 1u;
    } else {
        ng_sprite_group_set_pos(&s_stage_far, (int16_t)-far_x, 0);
        ng_sprite_group_flush(&s_stage_far);
    }
    if (!s_stage_live) {
        ng_sprite_group_init(&s_stage, SPR_STAGE, SLF_STAGE_COLS, SLF_STAGE_ROWS, slf_stage_tiles[s_stage_id], SLF_PAL_STAGE);
        ng_sprite_group_set_tile_stride(&s_stage, SLF_STAGE_COLS);
        ng_sprite_group_set_palette_map(&s_stage, slf_stage_maps[s_stage_id]);
        ng_sprite_group_set_pos(&s_stage, (int16_t)-camera_x, 0);
        ng_sprite_group_upload(&s_stage);
        s_stage_live = 1u;
        return;
    }
    ng_sprite_group_set_pos(&s_stage, (int16_t)-camera_x, 0);
    ng_sprite_group_flush(&s_stage);
}

void NEOGEO_USER slf_fighter_pose(uint8_t slot, uint8_t def, uint8_t sprite, int16_t x, int16_t y, uint8_t flip)
{
    NGSpriteGroup *g = &s_fighter[slot];
    uint16_t tile = (uint16_t)(slf_fighter_tiles[def] + sprite * 56u);
    uint8_t pal = (uint8_t)(PAL_FIGHTER + (s_fighter_pal[slot] & 1u));
    if (!s_fighter_live[slot]) {
        ng_sprite_group_init(g, (uint16_t)(SPR_FIGHTER + slot * 7u), 7u, 8u, tile, pal);
        ng_sprite_group_set_tile_stride(g, 7u);
        ng_sprite_group_set_flip(g, flip, 0u);
        ng_sprite_group_set_pos(g, (int16_t)(x - 56), y);
        ng_sprite_group_upload(g);
        s_fighter_live[slot] = 1u;
        return;
    }
    ng_sprite_group_set_tile_base(g, tile);
    ng_sprite_group_set_palette(g, pal);
    ng_sprite_group_set_flip(g, flip, 0u);
    ng_sprite_group_set_pos(g, (int16_t)(x - 56), y);
    ng_sprite_group_flush(g);
}

void NEOGEO_USER slf_fighter_bank(uint8_t slot, uint8_t bank)
{
    s_fighter_pal[slot & 1u] = bank;
}

void NEOGEO_USER slf_fighter_hide(uint8_t slot)
{
    if (s_fighter_live[slot]) {
        ng_sprite_group_cancel(&s_fighter[slot]);
        ng_sprite_group_hide(&s_fighter[slot]);
        s_fighter_live[slot] = 0u;
    }
    if (s_shadow_live[slot]) {
        ng_sprite_group_cancel(&s_shadow[slot]);
        ng_sprite_group_hide(&s_shadow[slot]);
        s_shadow_live[slot] = 0u;
    }
}

void NEOGEO_USER slf_portraits_show(uint8_t win_base, uint8_t p1_cur, uint8_t p2_cur, uint8_t p1_done, uint8_t p2_done, uint16_t tick)
{
    uint8_t i, p;
    if (win_base > (uint8_t)(SLF_FIGHTER_COUNT - 7u)) win_base = (uint8_t)(SLF_FIGHTER_COUNT - 7u);
    if (!s_portraits_live) {
        ng_palfx_screen_load(PAL_CURSOR_1P, slf_cursor_pals[0]);
        ng_palfx_screen_load(PAL_CURSOR_2P, slf_cursor_pals[1]);
        for (i = 0u; i < 7u; i++) {
            uint8_t f = (uint8_t)(win_base + i);
            int16_t px = (int16_t)(36 + i * 36);
            int16_t py = 36;
            uint16_t tile = (uint16_t)(SLF_PORTRAIT_TILE + f * 4u);
            ng_palfx_screen_load((uint8_t)(PAL_PORTRAITS + i), slf_fighter_pals[f][0]);
            ng_sprite_group_init(&s_portraits[i], (uint16_t)(SPR_PORTRAITS + i * 2u), 2u, 2u, tile, (uint8_t)(PAL_PORTRAITS + i));
            ng_sprite_group_set_pos(&s_portraits[i], px, py);
            ng_sprite_group_upload(&s_portraits[i]);
        }
        for (p = 0u; p < 2u; p++) {
            uint8_t cur = p ? p2_cur : p1_cur;
            int16_t cx = (int16_t)(cur >= win_base && cur < (uint8_t)(win_base + 7u) ? (36 + (cur - win_base) * 36) : -32);
            int16_t cy = 36;
            ng_sprite_group_init(&s_cursor[p], (uint16_t)(p ? SPR_CURSOR_2P : SPR_CURSOR_1P), 2u, 2u, SLF_CURSOR_TILE, (uint8_t)(p ? PAL_CURSOR_2P : PAL_CURSOR_1P));
            ng_sprite_group_set_pos(&s_cursor[p], cx, cy);
            ng_sprite_group_upload(&s_cursor[p]);
            s_cursor_live[p] = 1u;
        }
        s_port_last_win = win_base;
        s_portraits_live = 1u;
        return;
    }
    if (win_base != s_port_last_win) {
        for (i = 0u; i < 7u; i++) {
            uint8_t f = (uint8_t)(win_base + i);
            uint16_t tile = (uint16_t)(SLF_PORTRAIT_TILE + f * 4u);
            ng_palfx_screen_load((uint8_t)(PAL_PORTRAITS + i), slf_fighter_pals[f][0]);
            ng_sprite_group_set_tile_base(&s_portraits[i], tile);
            ng_sprite_group_flush(&s_portraits[i]);
        }
        s_port_last_win = win_base;
    }
    for (p = 0u; p < 2u; p++) {
        uint8_t cur = p ? p2_cur : p1_cur;
        int16_t cx = (int16_t)(cur >= win_base && cur < (uint8_t)(win_base + 7u) ? (36 + (cur - win_base) * 36) : -32);
        int16_t cy = 36;
        if (p1_cur == p2_cur && (tick & 4u) && p == 1u) {
            ng_sprite_group_set_pos(&s_cursor[1], -32, -32);
        } else {
            ng_sprite_group_set_pos(&s_cursor[p], cx, cy);
        }
        ng_sprite_group_flush(&s_cursor[p]);
    }
}

void NEOGEO_USER slf_portraits_hide(void)
{
    uint8_t i;
    if (s_portraits_live) {
        for (i = 0u; i < 7u; i++) {
            ng_sprite_group_cancel(&s_portraits[i]);
            ng_sprite_group_hide(&s_portraits[i]);
        }
        s_portraits_live = 0u;
        s_port_last_win = 0xFFu;
    }
    for (i = 0u; i < 2u; i++) {
        if (s_cursor_live[i]) {
            ng_sprite_group_cancel(&s_cursor[i]);
            ng_sprite_group_hide(&s_cursor[i]);
            s_cursor_live[i] = 0u;
        }
    }
}

static void NEOGEO_USER shadow_at(uint8_t slot, int16_t x)
{
    NGSpriteGroup *g = &s_shadow[slot];
    if (!s_shadow_live[slot]) {
        ng_sprite_group_init(g, (uint16_t)(SPR_SHADOW + slot * 4u), 4u, 1u, SLF_SHADOW_TILE, PAL_SHADOW);
        ng_sprite_group_set_tile_stride(g, 4u);
        ng_sprite_group_set_pos(g, (int16_t)(x - 32), SLF_FLOOR_Y - 10);
        ng_sprite_group_upload(g);
        s_shadow_live[slot] = 1u;
        return;
    }
    ng_sprite_group_set_pos(g, (int16_t)(x - 32), SLF_FLOOR_Y - 10);
    ng_sprite_group_flush(g);
}

void NEOGEO_USER slf_fx_reset(void)
{
    uint8_t i;
    for (i = 0u; i < FX_SLOTS; i++) {
        s_fx_timer[i] = 0u;
        if (s_fx_live[i]) { ng_sprite_group_cancel(&s_fx[i]); ng_sprite_group_hide(&s_fx[i]); s_fx_live[i] = 0u; }
    }
}

void NEOGEO_USER slf_fx_spawn(uint8_t kind, int16_t screen_x, int16_t screen_y)
{
    uint8_t i, pick = 0u, oldest = 255u;
    for (i = 0u; i < FX_SLOTS; i++) {
        if (!s_fx_timer[i]) { pick = i; break; }
        if (s_fx_timer[i] < oldest) { oldest = s_fx_timer[i]; pick = i; }
    }
    s_fx_kind[pick] = kind;
    s_fx_timer[pick] = (uint8_t)(4u * FX_TICKS);
    s_fx_x[pick] = (int16_t)(screen_x - 16);
    s_fx_y[pick] = (int16_t)(screen_y - 16);
}

static void NEOGEO_USER fx_draw(void)
{
    uint8_t i;
    for (i = 0u; i < FX_SLOTS; i++) {
        NGSpriteGroup *g = &s_fx[i];
        uint8_t frame;
        uint16_t tile;
        if (!s_fx_timer[i]) {
            if (s_fx_live[i]) { ng_sprite_group_cancel(g); ng_sprite_group_hide(g); s_fx_live[i] = 0u; }
            continue;
        }
        frame = (uint8_t)(3u - (s_fx_timer[i] - 1u) / FX_TICKS);
        tile = (uint16_t)(SLF_FX_TILE + (s_fx_kind[i] * 4u + frame) * 4u);
        s_fx_timer[i]--;
        if (!s_fx_live[i]) {
            ng_sprite_group_init(g, (uint16_t)(SPR_FX + i * 2u), 2u, 2u, tile, (uint8_t)(PAL_FX + s_fx_kind[i]));
            ng_sprite_group_set_tile_stride(g, 2u);
            ng_sprite_group_set_pos(g, s_fx_x[i], s_fx_y[i]);
            ng_sprite_group_upload(g);
            s_fx_live[i] = 1u;
            continue;
        }
        ng_sprite_group_set_tile_base(g, tile);
        ng_sprite_group_set_palette(g, (uint8_t)(PAL_FX + s_fx_kind[i]));
        ng_sprite_group_set_pos(g, s_fx_x[i], s_fx_y[i]);
        ng_sprite_group_flush(g);
    }
}

void NEOGEO_USER slf_match_draw(const NGFMatch *m, const uint8_t *def_index)
{
    uint8_t front = 1u, i, p, s;
    const NGFFighter *a = &m->f[0], *b = &m->f[1];
    uint8_t zoom = m->zoom_scale ? m->zoom_scale : 255u;
    if (a->state == NGF_S_ATTACK && b->state != NGF_S_ATTACK) front = 0u;
    slf_stage_show(m->camera_x);

    /* Super freeze backdrop */
    if (m->super_freeze) {
        if (m->super_freeze & 2u) setBACKDROP(0x0000);
        else setBACKDROP(0x4000);
    } else {
        setBACKDROP(BLACK);
    }

    for (i = 0u; i < 2u; i++) {
        uint8_t who = (uint8_t)(i == 1u ? front : (front ^ 1u));
        const NGFFighter *f = &m->f[who];
        int16_t sx, sy;
        if (zoom < 255u) {
            sx = (int16_t)(160 + ((int32_t)(ngf_fighter_px(f) - m->camera_x - 160) * zoom) / 255);
            sy = (int16_t)(SLF_FLOOR_Y - 12 - ((int32_t)(113 + ngf_fighter_py(f)) * zoom) / 255);
        } else {
            sx = (int16_t)(ngf_fighter_px(f) - m->camera_x);
            sy = (int16_t)(SLF_FLOOR_Y - 125 - ngf_fighter_py(f));
        }
        if (m->hitstop && (f->state == NGF_S_HIT || f->state == NGF_S_FALL)) sx = (int16_t)(sx + ((m->frame & 2u) ? 2 : -2));
        s_fighter_pal[i] = who;
        ng_sprite_group_set_scale(&s_fighter[i], zoom, zoom);
        ng_sprite_group_set_scale(&s_shadow[i], zoom, zoom);
        shadow_at(i, sx);
        slf_fighter_pose(i, def_index[who], ngf_fighter_frame(f)->sprite, sx, sy, f->facing_left);
    }

    /* Projectile rendering */
    for (p = 0u; p < NGF_PROJECTILES; p++) {
        const NGFProjectile *pj = &m->proj[p];
        NGSpriteGroup *g = &s_proj[p];
        if (!pj->active) {
            if (s_proj_live[p]) {
                ng_sprite_group_cancel(g);
                ng_sprite_group_hide(g);
                s_proj_live[p] = 0u;
            }
            continue;
        }
        int16_t px, py;
        if (zoom < 255u) {
            px = (int16_t)(160 + ((int32_t)((pj->x >> 8) - m->camera_x - 160) * zoom) / 255);
            py = (int16_t)(SLF_FLOOR_Y - 12 - ((int32_t)(113 + (pj->h >> 8)) * zoom) / 255);
        } else {
            px = (int16_t)((pj->x >> 8) - m->camera_x);
            py = (int16_t)(SLF_FLOOR_Y - 125 - (pj->h >> 8));
        }
        const NGFFrame *fr = ngf_projectile_frame(m, pj);
        uint16_t tile = (uint16_t)(slf_fighter_tiles[m->f[pj->owner].palette] + fr->sprite * 56u);
        uint8_t pal = (uint8_t)(PAL_FIGHTER + (pj->owner & 1u));
        if (!s_proj_live[p]) {
            ng_sprite_group_init(g, (uint16_t)(SPR_PROJ + p * 3u), 3u, 3u, tile, pal);
            ng_sprite_group_set_tile_stride(g, 3u);
            ng_sprite_group_set_scale(g, zoom, zoom);
            ng_sprite_group_set_flip(g, pj->facing_left, 0u);
            ng_sprite_group_set_pos(g, (int16_t)(px - 24), py);
            ng_sprite_group_upload(g);
            s_proj_live[p] = 1u;
        } else {
            ng_sprite_group_set_tile_base(g, tile);
            ng_sprite_group_set_palette(g, pal);
            ng_sprite_group_set_scale(g, zoom, zoom);
            ng_sprite_group_set_flip(g, pj->facing_left, 0u);
            ng_sprite_group_set_pos(g, (int16_t)(px - 24), py);
            ng_sprite_group_flush(g);
        }
    }

    /* Striker rendering */
    for (s = 0u; s < 2u; s++) {
        const NGFStriker *st = &m->striker[s];
        NGSpriteGroup *g = &s_striker[s];
        if (!st->active || !st->def) {
            if (s_striker_live[s]) {
                ng_sprite_group_cancel(g);
                ng_sprite_group_hide(g);
                s_striker_live[s] = 0u;
            }
            continue;
        }
        int16_t kx, ky;
        if (zoom < 255u) {
            kx = (int16_t)(160 + ((int32_t)((st->x >> 8) - m->camera_x - 160) * zoom) / 255);
            ky = (int16_t)(SLF_FLOOR_Y - 12 - ((int32_t)(113 + (st->h >> 8)) * zoom) / 255);
        } else {
            kx = (int16_t)((st->x >> 8) - m->camera_x);
            ky = (int16_t)(SLF_FLOOR_Y - 125 - (st->h >> 8));
        }
        const NGFFighterDef *sd = st->def;
        const NGFFrame *fr = &sd->frames[sd->anims[st->anim].first + st->frame];
        uint8_t d_idx = 0u, k;
        for (k = 0u; k < SLF_FIGHTER_COUNT; k++) {
            if (slf_fighter_defs[k] == sd) { d_idx = k; break; }
        }
        uint16_t tile = (uint16_t)(slf_fighter_tiles[d_idx] + fr->sprite * 56u);
        uint8_t pal = (uint8_t)(PAL_FIGHTER + (s & 1u));
        if (!s_striker_live[s]) {
            ng_sprite_group_init(g, (uint16_t)(SPR_STRIKER + s * 7u), 7u, 8u, tile, pal);
            ng_sprite_group_set_tile_stride(g, 7u);
            ng_sprite_group_set_scale(g, zoom, zoom);
            ng_sprite_group_set_flip(g, st->facing_left, 0u);
            ng_sprite_group_set_pos(g, (int16_t)(kx - 56), ky);
            ng_sprite_group_upload(g);
            s_striker_live[s] = 1u;
        } else {
            ng_sprite_group_set_tile_base(g, tile);
            ng_sprite_group_set_palette(g, pal);
            ng_sprite_group_set_scale(g, zoom, zoom);
            ng_sprite_group_set_flip(g, st->facing_left, 0u);
            ng_sprite_group_set_pos(g, (int16_t)(kx - 56), ky);
            ng_sprite_group_flush(g);
        }
    }

    fx_draw();
}

/* ------------------------------------------------------------ HUD */
static void NEOGEO_USER life_bar(uint8_t side, uint8_t px)
{
    uint8_t c;
    for (c = 0u; c < 16u; c++) {
        uint8_t lv = px > c * 8u ? (uint8_t)(px - c * 8u) : 0u;
        if (lv > 8u) lv = 8u;
        if (side == 0u) ng_fix_put_tile((uint8_t)(2u + c), 2u, (uint16_t)(SLF_FIX_BAR_L + lv), SLF_PAL_HUD);
        else ng_fix_put_tile((uint8_t)(37u - c), 2u, (uint16_t)(SLF_FIX_BAR_R + lv), SLF_PAL_HUD);
    }
}

static void NEOGEO_USER power_bar(uint8_t side, uint16_t power)
{
    uint8_t stock = (uint8_t)(power / NGF_POWER_STOCK), c;
    uint8_t px = (uint8_t)((power % NGF_POWER_STOCK) / 2u);
    char digit[2];
    if (stock >= 3u) px = 64u;
    for (c = 0u; c < 8u; c++) {
        uint8_t lv = px > c * 8u ? (uint8_t)(px - c * 8u) : 0u;
        if (lv > 8u) lv = 8u;
        ng_fix_put_tile((uint8_t)((side ? 30u : 4u) + c), 26u, (uint16_t)(SLF_FIX_POWER + lv), SLF_PAL_HUD);
    }
    digit[0] = (char)('0' + stock);
    digit[1] = '\0';
    slf_puts(side ? 38u : 2u, 26u, digit, stock ? SLF_PAL_GOLD : SLF_PAL_DIM);
}

void NEOGEO_USER slf_hud_draw(const NGFMatch *m)
{
    uint8_t i;
    if (!s_hud_valid) {
        slf_clear_rows(1u, 4u);
        slf_clear_rows(24u, 26u);
        ng_fix_put_tile(1u, 2u, SLF_FIX_CAP_L, SLF_PAL_HUD);
        ng_fix_put_tile(38u, 2u, SLF_FIX_CAP_R, SLF_PAL_HUD);
        slf_puts(2u, 3u, m->f[0].def->name, SLF_PAL_TEXT);
        slf_puts_right(37u, 3u, m->f[1].def->name, SLF_PAL_TEXT);
        slf_puts(4u, 25u, "POWER", SLF_PAL_DIM);
        slf_puts(33u, 25u, "POWER", SLF_PAL_DIM);
        for (i = 0u; i < 2u; i++) s_hud_life[i] = s_hud_power[i] = s_hud_stock[i] = s_hud_wins[i] = 0xFFu;
        s_hud_secs = 0xFFu;
        s_hud_valid = 1u;
    }
    for (i = 0u; i < 2u; i++) {
        const NGFFighter *f = &m->f[i];
        uint8_t px = (uint8_t)((uint32_t)f->life * 128u / f->def->life);
        uint8_t pw = (uint8_t)((f->power % NGF_POWER_STOCK) / 2u);
        uint8_t st = (uint8_t)(f->power / NGF_POWER_STOCK);
        if (f->life && !px) px = 1u;
        if (px != s_hud_life[i]) { life_bar(i, px); s_hud_life[i] = px; }
        if (pw != s_hud_power[i] || st != s_hud_stock[i]) { power_bar(i, f->power); s_hud_power[i] = pw; s_hud_stock[i] = st; }
        if (f->wins != s_hud_wins[i]) {
            uint8_t k;
            for (k = 0u; k < m->rounds_to_win && k < 3u; k++)
                ng_fix_put_tile((uint8_t)(i ? 22u + k : 17u - k), 4u,
                                k < f->wins ? SLF_FIX_MARK_ON : SLF_FIX_MARK_OFF, SLF_PAL_HUD);
            s_hud_wins[i] = f->wins;
        }

        /* Guard gauge */
        {
            char g_bar[8];
            uint8_t g_val = (uint8_t)(f->guard / 20u);
            uint8_t k;
            g_bar[0] = 'G'; g_bar[1] = ':';
            for (k = 0u; k < 5u; k++) g_bar[2u + k] = (char)(k < g_val ? '=' : '-');
            g_bar[7] = '\0';
            slf_puts(i ? 32u : 2u, 4u, g_bar, f->guard < 30u ? SLF_PAL_RED : SLF_PAL_DIM);
        }

        /* Striker status */
        if (m->striker[i].cooldown == 0u) {
            slf_puts(i ? 33u : 2u, 24u, "STRK", SLF_PAL_GOLD);
        } else {
            slf_puts(i ? 33u : 2u, 24u, "WAIT", SLF_PAL_DIM);
        }

        /* Combo count, on the attacker's side. */
        {
            uint8_t hits = m->f[i ^ 1u].combo;
            if (hits >= 2u) { s_hud_combo[i] = hits; s_hud_combo_timer[i] = 60u; }
            if (s_hud_combo_timer[i]) {
                if (s_hud_combo_timer[i] == 60u) {
                    char text[8];
                    text[0] = (char)('0' + s_hud_combo[i] / 10u);
                    text[1] = (char)('0' + s_hud_combo[i] % 10u);
                    text[2] = ' '; text[3] = 'H'; text[4] = 'I'; text[5] = 'T'; text[6] = 'S'; text[7] = '\0';
                    if (text[0] == '0') text[0] = ' ';
                    slf_puts(i ? 30u : 3u, 7u, text, SLF_PAL_GOLD);
                }
                if (!--s_hud_combo_timer[i]) slf_puts(i ? 30u : 3u, 7u, "       ", SLF_PAL_GOLD);
            }
        }
    }
    if (m->seconds != s_hud_secs) {
        uint8_t tens = (uint8_t)(m->seconds / 10u), ones = (uint8_t)(m->seconds % 10u);
        slf_big(18u, 1u, (uint16_t)(SLF_BIG_DIGITS + tens * 2u), 2u, 1u, m->seconds < 10u ? SLF_PAL_RED : SLF_PAL_BIG);
        slf_big(18u, 2u, (uint16_t)(SLF_BIG_DIGITS + SLF_BIG_DIGITS_W + tens * 2u), 2u, 1u, m->seconds < 10u ? SLF_PAL_RED : SLF_PAL_BIG);
        slf_big(20u, 1u, (uint16_t)(SLF_BIG_DIGITS + ones * 2u), 2u, 1u, m->seconds < 10u ? SLF_PAL_RED : SLF_PAL_BIG);
        slf_big(20u, 2u, (uint16_t)(SLF_BIG_DIGITS + SLF_BIG_DIGITS_W + ones * 2u), 2u, 1u, m->seconds < 10u ? SLF_PAL_RED : SLF_PAL_BIG);
        s_hud_secs = m->seconds;
    }
}

/* ------------------------------------------------------------ title */
static uint8_t NEOGEO_USER anim_sprite(uint8_t def, uint8_t anim, uint8_t frame)
{
    const NGFFighterDef *d = slf_fighter_defs[def];
    return d->frames[d->anims[anim].first + frame].sprite;
}

void NEOGEO_USER slf_title_draw(void)
{
    slf_fighter_palettes(0u, 0u, 1u % SLF_FIGHTER_COUNT, 0u);
    slf_stage_show(96);
    s_fighter_pal[0] = 0u;
    s_fighter_pal[1] = 1u;
    slf_fighter_pose(0u, 0u, anim_sprite(0u, NGF_A_WIN, 1u), 72, SLF_FLOOR_Y - 125, 0u);
    slf_fighter_pose(1u, (uint8_t)(1u % SLF_FIGHTER_COUNT), anim_sprite((uint8_t)(1u % SLF_FIGHTER_COUNT), NGF_A_WIN, 1u),
                     248, SLF_FLOOR_Y - 125, 1u);
    shadow_at(0u, 72);
    shadow_at(1u, 248);
    slf_big_center(2u, SLF_BIG_TITLE1, SLF_BIG_TITLE1_W, SLF_BIG_TITLE1_H, SLF_PAL_BIG);
    slf_big_center(5u, SLF_BIG_TITLE1B, SLF_BIG_TITLE1B_W, SLF_BIG_TITLE1B_H, SLF_PAL_BIG);
    slf_big_center(9u, SLF_BIG_TITLE2, SLF_BIG_TITLE2_W, SLF_BIG_TITLE2_H, SLF_PAL_RED);
    slf_puts(8u, 27u, "(C) 2026 EAGLE SOFTWARE", SLF_PAL_TEXT);
}

/* ------------------------------------------------------------ sound */
void NEOGEO_USER slf_music(uint8_t track)
{
    soundStopAll();              waitVbl();
    soundCancelFade();           waitVbl();
    soundSetADPCMAVolume(0x30u); waitVbl();
    soundSetADPCMBVolume(0xD0u); waitVbl();
    soundSetSSGVolume(0x00u);    waitVbl();
    soundSetFMVolume(0x00u);     waitVbl();
    soundSetADPCMBLoop(track != SLF_TRACK_VICTORY); waitVbl();
    playSFXB((uint8_t)(SOUND_TRACK_A + track - 1u));
    waitVbl();
}

void NEOGEO_USER slf_sfx(uint8_t id)
{
    if (id) playSFX((uint8_t)(SOUND_SFX_1 + id - 1u));
}
