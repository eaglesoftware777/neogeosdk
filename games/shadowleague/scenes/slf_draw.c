/*
 * Shadow League Fighting: hardware side.
 *
 * Sprite slots: stage 33-64 (32 strips x 14 tiles), shadows 65-72, fighters
 * 73-172 (sparse shared 16x32 strips per fighter; the attacker takes the
 * front slot), projectiles 173-178, strikers 179-278, effects 279-286. Palette banks: FIX 0-7, fighters 16-17,
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
#include "games/shadowleague/scenes/slf_rom.h"
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
#define SPR_PROJ     173u
#define SPR_STRIKER  179u
#define SPR_FX       279u
#define SPR_PORTRAITS 287u
#define SPR_CURSOR_1P 301u
#define SPR_CURSOR_2P 303u
#define PAL_FIGHTER   16u
#define PAL_BODY      64u
#define PAL_SUPPORT   72u
#define PAL_FX        18u
#define PAL_SHADOW    21u
#define PAL_PORTRAITS 22u
#define PAL_CURSOR_1P 29u
#define PAL_CURSOR_2P 30u
#define FX_SLOTS       4u
#define FX_TICKS       3u

static uint16_t s_palette_base[104u * 16u];
static uint16_t s_palette_out[104u * 16u];
static volatile uint8_t s_palette_pending;
volatile uint16_t slf_palette_irq_count;
volatile uint8_t slf_palette_irq_lines;

/* Only a completed main-loop buffer may be read by the early interrupt.
 * Upload before BIOS I/O can exhaust the full-screen palette window. */
void NEOGEO_USER slf_palette_vblank(void)
{
    if (!s_palette_pending) return;
    s_palette_pending = 0u;
    __asm__ volatile ("" ::: "memory");
    slf_palette_irq_lines = ng_vram_lines_left();
    ng_palfx_vblank();
    slf_palette_irq_count++;
}
static NGSpriteGroup s_stage_far, s_stage, s_fighter[2][50], s_shadow[2], s_fx[FX_SLOTS];
static NGSpriteGroup s_proj[NGF_PROJECTILES], s_striker[2][50];
static NGSpriteGroup s_portraits[7], s_cursor[2];
static NGSpriteGroup s_title_logo, s_title_sub, s_large_port[2], s_story_panel;
static NGSpriteGroup s_hud_top, s_hud_bottom;
static uint8_t s_hud_plate_live;
typedef struct {
    uint32_t tile;
    uint16_t duration;
    uint8_t bank_count, strips;
    uint16_t palettes[8][16];
    uint8_t map[320];
    char caption[2][40];
} SLFStoryCard;
typedef char SLFStoryCard_size_must_be_664[(sizeof(SLFStoryCard) == 664u) ? 1 : -1];
static SLFStoryCard s_story_card;
uint8_t slf_presentation_current;
static void NEOGEO_USER mark_at(int16_t y);
static uint8_t s_large_port_live[2], s_large_port_def[2];
static uint8_t s_stage_id, s_stage_dim;
static uint8_t s_stage_live, s_stage_far_live;
static uint8_t s_actor_scale[2] = {255u,255u};
static uint8_t s_fighter_live[2], s_fighter_pal[2], s_shadow_live[2];
static uint8_t s_proj_live[NGF_PROJECTILES], s_striker_live[2];
static uint8_t s_portraits_live, s_cursor_live[2], s_port_last_win;
static uint8_t s_fx_kind[FX_SLOTS], s_fx_timer[FX_SLOTS], s_fx_live[FX_SLOTS];
static int16_t s_fx_x[FX_SLOTS], s_fx_y[FX_SLOTS];
typedef struct {uint8_t def, sprite, flip, pal, scale;} SLFPoseCache;
static SLFPoseCache s_pose_cache[4];
static uint8_t s_actor_palmap[4][50][2];
static uint8_t s_joy[2], s_joy_prev[2], s_start, s_start_prev;
static uint16_t s_frames, s_rng;
/* HUD cache: what the FIX layer shows now. */
static uint8_t s_hud_valid, s_hud_life[2], s_hud_power[2], s_hud_stock[2], s_hud_wins[2], s_hud_secs;
static uint8_t s_hud_combo[2], s_hud_combo_timer[2];
static uint8_t s_hud_trail[2], s_hud_trail_hold[2], s_hud_guard[2], s_hud_spirit[2];

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
    ng_palfx_screen_load(96u, slf_hud_plate_pal);
    for (i = 0u; i < SLF_STAGE_BANKS; i++) ng_palfx_screen_load((uint8_t)(SLF_PAL_STAGE + i), slf_stage_pals[s_stage_id][i]);
}

void NEOGEO_USER slf_fighter_palettes(uint8_t def0, uint8_t alt0, uint8_t def1, uint8_t alt1)
{
    uint8_t side, bank;
    uint8_t defs[2], alts[2];
    defs[0] = def0; defs[1] = def1;
    alts[0] = (uint8_t)(alt0 != 0u); alts[1] = (uint8_t)(alt1 != 0u);
    s_hud_valid = 0u;
    ng_palfx_screen_load(PAL_FIGHTER, slf_fighter_pals[def0][alt0 ? 1 : 0]);
    ng_palfx_screen_load((uint8_t)(PAL_FIGHTER + 1u), slf_fighter_pals[def1][alt1 ? 1 : 0]);
    for (side = 0u; side < 2u; side++)
        for (bank = 0u; bank < slf_fighter_bank_counts[defs[side]]; bank++)
            ng_palfx_screen_load((uint8_t)(PAL_BODY + side * 4u + bank),
                slf_fighter_bank_pals[slf_fighter_bank_offsets[defs[side]] + bank][alts[side]]);
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
    s_large_port_live[0] = s_large_port_live[1] = 0u;
    for (i = 0u; i < 2u; i++) {
        s_fighter_live[i] = s_shadow_live[i] = 0u;
        s_actor_scale[i] = 255u;
        s_striker_live[i] = 0u;
    }
    for (i = 0u; i < NGF_PROJECTILES; i++) s_proj_live[i] = 0u;
    for (i = 0u; i < FX_SLOTS; i++) s_fx_live[i] = s_fx_timer[i] = 0u;
    s_portraits_live = 0u;
    s_port_last_win = 0xFFu;
    s_cursor_live[0] = s_cursor_live[1] = 0u;
    s_hud_valid = 0u;
    s_hud_plate_live = 0u;
    ng_fix_invalidate_all();
    slf_fix_palettes_init();
    ng_palfx_screen_fade_in(0u, 12u);
    waitVbl();
}

uint16_t NEOGEO_USER slf_frame(void)
{
    __asm__ volatile ("" ::: "memory");
    s_palette_pending = 1u;
    /* An old wait flag from an overrun cannot acknowledge this batch. */
    do { waitVbl(); } while (s_palette_pending);
    if (!ng_vram_window_open()) waitVbl();
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
    s_stage_dim = 0u;
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
    /* Opaque panoramas need one layer; spare the scanline sprite budget. */
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

static void NEOGEO_USER actor_hide(NGSpriteGroup *groups)
{
    uint8_t k;
    for (k = 0u; k < 50u; k++) {
        ng_sprite_group_cancel(&groups[k]);
        ng_sprite_group_hide(&groups[k]);
    }
}

static void NEOGEO_USER actor_draw(NGSpriteGroup *groups, uint8_t *live, uint16_t first,
    uint8_t def, uint8_t sprite, int16_t x, int16_t y, uint8_t flip, uint8_t pal, uint8_t scale)
{
    const SLFStripRef *map = slf_fighter_strip_maps[def] + slf_fighter_strip_offsets[def][sprite];
    uint16_t tiles[50];
    uint8_t bits[50];
    uint8_t row, col, k, width = (uint8_t)((scale >> 4) + 1u);
    int16_t left = (int16_t)(x - width * 5u + (flip ? 1u : 0u));
    int16_t top = (int16_t)(y - (((uint16_t)16u * ((uint16_t)scale + 1u)) >> 8));
    uint8_t actor = first == SPR_FIGHTER ? 0u : first == SPR_FIGHTER + 50u ? 1u :
        first == SPR_STRIKER ? 2u : 3u;
    SLFPoseCache *cache = &s_pose_cache[actor];
    if (*live && cache->def == def && cache->sprite == sprite && cache->flip == flip &&
        cache->pal == pal && cache->scale == scale) {
        /* Held drawings need position updates only. Descriptor changes still
         * take the full path; hidden/reused actors always reload their art. */
        k = 0u;
        for (row = 0u; row < 5u; row++) {
            int16_t py = (int16_t)(top + (((uint16_t)row * 32u * ((uint16_t)scale + 1u)) >> 8));
            for (col = 0u; col < 10u; col++, k++) if (groups[k].visible) {
                ng_sprite_group_set_pos(&groups[k], (int16_t)(left + col * width), py);
                ng_sprite_group_flush(&groups[k]);
            }
        }
        return;
    }
    cache->def = def; cache->sprite = sprite; cache->flip = flip;
    cache->pal = pal; cache->scale = scale;
    for (k = 0u; k < 50u; k++) tiles[k] = 0xFFFFu;
    while (map->slot != 255u) {
        tiles[map->slot] = map->tile; bits[map->slot] = map->pal_bits; map++;
    }
    k = 0u;
    for (row = 0u; row < 5u; row++) {
        int16_t py = (int16_t)(top + (((uint16_t)row * 32u * ((uint16_t)scale + 1u)) >> 8));
        for (col = 0u; col < 10u; col++, k++) {
            NGSpriteGroup *g = &groups[k];
            uint8_t source = (uint8_t)(row * 10u + (flip ? 9u - col : col));
            uint16_t tile = tiles[source];
            if (!*live) {
                ng_sprite_group_init(g, (uint16_t)(first + k), 1u, 2u, tile, pal);
                ng_sprite_group_set_visible(g, 0u);
            }
            if (tile == 0xFFFFu) {
                if (*live && g->visible) {
                    ng_sprite_group_set_visible(g, 0u);
                    ng_sprite_group_flush(g);
                }
                continue;
            }
            ng_sprite_group_set_visible(g, 1u);
            if ((bits[source] & 15u) == 0u) {
                /* Uniform groups keep the original fast attribute writer. */
                ng_sprite_group_set_palette_map(g, 0);
                ng_sprite_group_set_palette(g, pal);
            } else {
                uint8_t *pm = s_actor_palmap[actor][k];
                uint8_t top_bank = (uint8_t)(pal + (bits[source] & 3u));
                uint8_t bottom_bank = (uint8_t)(pal + ((bits[source] >> 2) & 3u));
                if (pm[0] != top_bank || pm[1] != bottom_bank) {
                    pm[0] = top_bank; pm[1] = bottom_bank;
                    ng_sprite_group_mark_dirty(g, NG_SGF_DIRTY_PALETTE);
                }
                ng_sprite_group_set_palette_map(g, pm);
            }
            ng_sprite_group_set_tile_address(g, (uint32_t)tile | ((uint32_t)(bits[source] >> 4) << 16));
            ng_sprite_group_set_tile_stride(g, 1u);
            ng_sprite_group_set_scale(g, scale, scale);
            ng_sprite_group_set_flip(g, flip, 0u);
            ng_sprite_group_set_pos(g, (int16_t)(left + col * width), py);
            ng_sprite_group_flush(g);
        }
    }
    *live = 1u;
}

void NEOGEO_USER slf_fighter_pose(uint8_t slot, uint8_t def, uint8_t sprite, int16_t x, int16_t y, uint8_t flip)
{
    uint8_t pal = (uint8_t)(PAL_BODY + (s_fighter_pal[slot] & 1u) * 4u);
    uint8_t scale = s_actor_scale[slot];
    actor_draw(s_fighter[slot], &s_fighter_live[slot], (uint16_t)(SPR_FIGHTER + slot * 50u),
        def, sprite, x, y, flip, pal, scale);
}

void NEOGEO_USER slf_fighter_hide(uint8_t slot)
{
    if (s_fighter_live[slot]) {
        actor_hide(s_fighter[slot]);
        s_fighter_live[slot] = 0u;
    }
    if (s_shadow_live[slot]) {
        ng_sprite_group_cancel(&s_shadow[slot]);
        ng_sprite_group_hide(&s_shadow[slot]);
        s_shadow_live[slot] = 0u;
    }
}

void NEOGEO_USER slf_fighter_bank(uint8_t slot, uint8_t bank)
{
    s_fighter_pal[slot & 1u] = bank;
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
    if ((uint8_t)(m->super_freeze != 0u) != s_stage_dim) {
        uint8_t bank;
        s_stage_dim = (uint8_t)(m->super_freeze != 0u);
        for (bank = 0u; bank < SLF_STAGE_BANKS; bank++)
            ng_palfx_screen_load((uint8_t)(SLF_PAL_STAGE + bank), s_stage_dim ?
                slf_stage_dark_pals[s_stage_id][bank] : slf_stage_pals[s_stage_id][bank]);
    }

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
        s_actor_scale[i] = zoom;
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
        uint16_t tile = (uint16_t)(SLF_PROJECTILE_TILE + ((m->frame >> 2) & 1u) * 9u);
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
        NGSpriteGroup *g = s_striker[s];
        if (!st->active || !st->def) {
            if (s_striker_live[s]) {
                actor_hide(g);
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
        uint8_t pal = (uint8_t)(PAL_SUPPORT + s * 4u);
        if (!s_striker_live[s]) {
            uint8_t bank;
            for (bank = 0u; bank < slf_fighter_bank_counts[d_idx]; bank++)
                ng_palfx_screen_load((uint8_t)(pal + bank),
                    slf_fighter_bank_pals[slf_fighter_bank_offsets[d_idx] + bank][0]);
        }
        actor_draw(g, &s_striker_live[s], (uint16_t)(SPR_STRIKER + s * 50u),
            d_idx, fr->sprite, kx, ky, st->facing_left, pal, zoom);
    }

    fx_draw();
}

/* ------------------------------------------------------------ HUD */
static void NEOGEO_USER life_bar(uint8_t side, uint8_t px, uint8_t trail)
{
    uint8_t c;
    for (c = 0u; c < 16u; c++) {
        uint8_t lv = px > c * 8u ? (uint8_t)(px - c * 8u) : 0u;
        uint8_t recent = trail > c * 8u ? (uint8_t)(trail - c * 8u) : 0u;
        if (lv > 8u) lv = 8u;
        if (recent > 8u) recent = 8u;
        if (recent < lv) recent = lv;
        if (side == 0u) ng_fix_put_tile((uint8_t)(2u + c), 2u, (uint16_t)(SLF_FIX_BAR_L + lv * 9u + recent), SLF_PAL_HUD);
        else ng_fix_put_tile((uint8_t)(37u - c), 2u, (uint16_t)(SLF_FIX_BAR_R + lv * 9u + recent), SLF_PAL_HUD);
    }
}

static void NEOGEO_USER hud_plates(void)
{
    if (s_hud_plate_live) return;
    ng_sprite_group_init(&s_hud_top, 315u, 20u, 2u, 0u, 96u);
    ng_sprite_group_set_tile_address(&s_hud_top, SLF_HUD_TOP_TILE);
    ng_sprite_group_set_tile_stride(&s_hud_top, 20u);
    ng_sprite_group_set_pos(&s_hud_top, 0, 0);
    ng_sprite_group_upload(&s_hud_top);
    ng_sprite_group_init(&s_hud_bottom, 335u, 20u, 1u, 0u, 96u);
    ng_sprite_group_set_tile_address(&s_hud_bottom, SLF_HUD_BOTTOM_TILE);
    ng_sprite_group_set_tile_stride(&s_hud_bottom, 20u);
    ng_sprite_group_set_pos(&s_hud_bottom, 0, 208);
    ng_sprite_group_upload(&s_hud_bottom);
    s_hud_plate_live = 1u;
}

static void NEOGEO_USER small_meter(uint8_t side, uint8_t spirit, uint8_t value, uint8_t alert)
{
    uint8_t c;
    uint16_t base = spirit ? (side ? SLF_FIX_SPIRIT_R : SLF_FIX_SPIRIT_L) :
        alert ? (side ? SLF_FIX_GUARD_ALERT_R : SLF_FIX_GUARD_ALERT_L) :
        (side ? SLF_FIX_GUARD_R : SLF_FIX_GUARD_L);
    for (c = 0u; c < 4u; c++) {
        uint8_t level = value > c * 8u ? (uint8_t)(value - c * 8u) : 0u;
        uint8_t x = side ? (uint8_t)((spirit ? 28u : 35u) - c) : (uint8_t)((spirit ? 11u : 4u) + c);
        if (level > 8u) level = 8u;
        ng_fix_put_tile(x, 4u, (uint16_t)(base + level), SLF_PAL_HUD);
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
    hud_plates();
    if (!s_hud_valid) {
        slf_clear_rows(1u, 4u);
        slf_clear_rows(24u, 26u);
        ng_fix_put_tile(1u, 2u, SLF_FIX_CAP_L, SLF_PAL_HUD);
        ng_fix_put_tile(38u, 2u, SLF_FIX_CAP_R, SLF_PAL_HUD);
        slf_puts(2u, 3u, m->f[0].def->name, SLF_PAL_TEXT);
        slf_puts_right(37u, 3u, m->f[1].def->name, SLF_PAL_TEXT);
        slf_puts(4u, 25u, "POWER", SLF_PAL_DIM);
        slf_puts(33u, 25u, "POWER", SLF_PAL_DIM);
        slf_puts(2u, 4u, "G", SLF_PAL_TEXT);slf_puts(9u, 4u, "S", SLF_PAL_TEXT);
        slf_puts(37u, 4u, "G", SLF_PAL_TEXT);slf_puts(30u, 4u, "S", SLF_PAL_TEXT);
        for (i = 0u; i < 2u; i++) s_hud_life[i] = s_hud_power[i] = s_hud_stock[i] = s_hud_wins[i] = 0xFFu;
        for (i = 0u; i < 2u; i++) {
            s_hud_trail[i] = 0xFFu;s_hud_trail_hold[i] = 0u;
            s_hud_guard[i] = s_hud_spirit[i] = 0xFFu;
        }
        s_hud_secs = 0xFFu;
        s_hud_valid = 1u;
    }
    for (i = 0u; i < 2u; i++) {
        const NGFFighter *f = &m->f[i];
        uint8_t px = (uint8_t)((uint32_t)f->life * 128u / f->def->life);
        uint8_t pw = (uint8_t)((f->power % NGF_POWER_STOCK) / 2u);
        uint8_t st = (uint8_t)(f->power / NGF_POWER_STOCK);
        if (f->life && !px) px = 1u;
        {
            uint8_t old_trail = s_hud_trail[i];
            if (old_trail == 0xFFu || px > old_trail) s_hud_trail[i] = px;
            if (s_hud_life[i] != 0xFFu && px < s_hud_life[i]) s_hud_trail_hold[i] = 24u;
            if (!m->hitstop && !m->super_freeze) {
                if (s_hud_trail_hold[i]) s_hud_trail_hold[i]--;
                else if (s_hud_trail[i] > px) s_hud_trail[i]--;
            }
            if (px != s_hud_life[i] || old_trail != s_hud_trail[i]) life_bar(i, px, s_hud_trail[i]);
            s_hud_life[i] = px;
        }
        if (pw != s_hud_power[i] || st != s_hud_stock[i]) { power_bar(i, f->power); s_hud_power[i] = pw; s_hud_stock[i] = st; }
        if (f->wins != s_hud_wins[i]) {
            uint8_t k;
            for (k = 0u; k < m->rounds_to_win && k < 3u; k++)
                ng_fix_put_tile((uint8_t)(i ? 22u + k : 17u - k), 4u,
                                k < f->wins ? SLF_FIX_MARK_ON : SLF_FIX_MARK_OFF, SLF_PAL_HUD);
            s_hud_wins[i] = f->wins;
        }

        /* Compact distinct guard and spirit meters; update only changed art. */
        {
            uint8_t guard = (uint8_t)((uint16_t)f->guard * 32u / f->def->guard_max);
            uint8_t spirit = (uint8_t)((uint16_t)f->spirit * 32u / NGF_SPIRIT_MAX);
            uint8_t alert = (uint8_t)(f->guard < 30u);
            uint8_t encoded = (uint8_t)(guard | (alert ? 128u : 0u));
            if (guard > 32u) guard = 32u;
            if (spirit > 32u) spirit = 32u;
            if (encoded != s_hud_guard[i]) { small_meter(i, 0u, guard, alert);s_hud_guard[i] = encoded; }
            if (spirit != s_hud_spirit[i]) { small_meter(i, 1u, spirit, 0u);s_hud_spirit[i] = spirit; }
        }

        {
            char badge[7] = {'A','S','S','T',' ', '0', '\0'};
            if (m->assist[i]) {
                badge[5] = (char)('0' + m->assist_uses[i]);
                slf_puts(i ? 32u : 2u, 24u, badge,
                    m->assist_uses[i] && !m->striker[i].cooldown ? SLF_PAL_GOLD : SLF_PAL_DIM);
            }
            if (m->match_mode != NGF_MODE_1V1) {
                char team[6] = {'T', ' ', '1', '/', '3', '\0'};
                team[2] = (char)('1' + m->team_active[i]);
                team[4] = (char)('0' + m->team_count[i]);
                slf_puts(i ? 23u : 12u, 24u, team, SLF_PAL_TEXT);
            }
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
static uint8_t NEOGEO_USER showcase_index(uint8_t def)
{
    return def == 0u ? 0u : def == 3u ? 1u : def == 2u ? 2u : 255u;
}

void NEOGEO_USER slf_large_portrait(uint8_t side, uint8_t def)
{
    uint8_t idx = showcase_index(def);
    NGSpriteGroup *g = &s_large_port[side];
    if (idx == 255u) {
        if (s_large_port_live[side]) ng_sprite_group_hide(g);
        s_large_port_live[side] = 0u;
        return;
    }
    if (s_large_port_live[side] && s_large_port_def[side] == def) return;
    ng_palfx_screen_load((uint8_t)(80u + side), slf_rebirth_portrait_pals[idx]);
    if (!s_large_port_live[side]) {
        ng_sprite_group_init(g, (uint16_t)(305u + side * 5u), 5u, 6u,
            (uint16_t)slf_rebirth_portrait_tiles[idx], (uint8_t)(80u + side));
    }
    ng_sprite_group_set_tile_address(g, slf_rebirth_portrait_tiles[idx]);
    ng_sprite_group_set_pos(g, side ? 240 : 0, 64);
    ng_sprite_group_upload(g);
    s_large_port_live[side] = 1u;
    s_large_port_def[side] = def;
}

uint16_t NEOGEO_USER slf_story_show(uint8_t number)
{
    uint8_t i;
    uint16_t cell;
    if (number >= SLF_PRESENTATION_CARDS ||
        !slf_rom_read(&s_story_card, slf_presentation_offsets[number], sizeof(s_story_card)) ||
        s_story_card.bank_count == 0u || s_story_card.bank_count > 8u ||
        s_story_card.strips != 22u || s_story_card.tile > 0x3FECCu) return 0u;
    for (i = 0u; i < s_story_card.bank_count; i++)
        ng_palfx_screen_load((uint8_t)(80u + i), s_story_card.palettes[i]);
    for (cell = 0u; cell < 308u; cell++) {
        if (s_story_card.map[cell] >= s_story_card.bank_count) return 0u;
        s_story_card.map[cell] = (uint8_t)(80u + s_story_card.map[cell]);
    }
    ng_sprite_group_init(&s_story_panel, 1u, 22u, 14u, (uint16_t)s_story_card.tile, 80u);
    ng_sprite_group_set_tile_address(&s_story_panel, s_story_card.tile);
    ng_sprite_group_set_palette_map(&s_story_panel, s_story_card.map);
    ng_sprite_group_set_pos(&s_story_panel, -16, 0);
    ng_sprite_group_upload(&s_story_panel);
    slf_presentation_current = number;
    s_story_card.caption[0][39] = s_story_card.caption[1][39] = '\0';
    slf_puts_center(24u, s_story_card.caption[0], SLF_PAL_GOLD);
    slf_puts_center(26u, s_story_card.caption[1], SLF_PAL_TEXT);
    if (number == SLF_PRESENTATION_CARDS - 1u) mark_at(42);
    return s_story_card.duration;
}

void NEOGEO_USER slf_story_pan(uint16_t tick, uint8_t reverse)
{
    int16_t travel = (int16_t)(tick >> 4);
    if (travel > 16) travel = 16;
    ng_sprite_group_set_pos(&s_story_panel, (int16_t)(reverse ? -8 - travel : -24 + travel), 0);
    ng_sprite_group_flush(&s_story_panel);
}

static void NEOGEO_USER mark_at(int16_t y)
{
    ng_palfx_screen_load(88u, slf_rebirth_identity_pal);
    ng_sprite_group_init(&s_title_logo, 73u, 18u, 6u, (uint16_t)SLF_REBIRTH_LOGO_TILE, 88u);
    ng_sprite_group_set_tile_address(&s_title_logo, SLF_REBIRTH_LOGO_TILE);
    ng_sprite_group_set_pos(&s_title_logo, 16, y);
    ng_sprite_group_upload(&s_title_logo);
    ng_sprite_group_init(&s_title_sub, 91u, 8u, 1u, (uint16_t)SLF_REBIRTH_SUBTITLE_TILE, 88u);
    ng_sprite_group_set_tile_address(&s_title_sub, SLF_REBIRTH_SUBTITLE_TILE);
    ng_sprite_group_set_pos(&s_title_sub, 96, (int16_t)(y + 96));
    ng_sprite_group_upload(&s_title_sub);
}

void NEOGEO_USER slf_title_draw(void)
{
    slf_story_show(0u);
    mark_at(142);
    slf_puts(8u, 27u, "(C) 2026 EAGLE SOFTWARE", SLF_PAL_TEXT);
}

void NEOGEO_USER slf_title_tick(uint16_t tick)
{
    int16_t remaining = tick < 24u ? (int16_t)(24u - tick) : 0;
    ng_sprite_group_set_pos(&s_title_logo, 16, (int16_t)(94 + remaining * 2));
    ng_sprite_group_set_pos(&s_title_sub, 96, (int16_t)(190 + remaining * 2));
    ng_sprite_group_flush(&s_title_logo);ng_sprite_group_flush(&s_title_sub);
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
