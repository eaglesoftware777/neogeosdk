/*
 * Shadow League Fighting: hardware side.
 * Sprite reservations: stage 33-64, shadows 65-72, fighters 73-146,
 * projectiles 173-184, strikers 195-268, effects 279-290. Each actor reserves
 * two banks of 12 columns so partially uploaded poses stay hidden.
 * Palettes: FIX 0-11, effects 18-25, stage 32-47, bodies/support 64-79,
 * cinematic cards 80-95, HUD 96, selection faces 97-100.
 * The early palette interrupt also streams hidden actor columns. Other
 * sprite and FIX writes use the deferred SDK queues during blanking.
 */
#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include "sdk/ng_system.h"
#include "sdk/ng_input_queue.h"
#include "sdk/ng_perf.h"
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
#define SPR_STRIKER  195u
#define SPR_FX       279u
#define SPR_CURSOR_1P 301u
#define SPR_CURSOR_2P 303u
#define PAL_FIGHTER   16u
#define PAL_BODY      64u
#define PAL_SUPPORT   72u
#define PAL_FX        18u
#define PAL_SHADOW    21u
#define PAL_PROJ      22u           /* 22-23 */
#define PAL_CURSOR_1P 29u
#define PAL_CURSOR_2P 30u
#define PAL_LARGE_PORTRAIT 97u    /* (80-95: the painted cards) */
#define FX_SLOTS       4u
#define FX_TICKS       3u

static uint16_t s_palette_base[104u * 16u];
static uint16_t s_palette_out[104u * 16u];
static volatile uint8_t s_palette_pending;
static uint8_t s_demo_silent;
volatile uint16_t slf_palette_irq_count;
volatile uint8_t slf_palette_irq_lines;
static void NEOGEO_USER actors_commit(void);
static uint16_t s_irq_begin_line;
volatile uint16_t slf_irq_lines_peak;

/* Only a completed main-loop buffer may be read by the early interrupt.
 * Upload before BIOS I/O can exhaust the full-screen palette window. */
void NEOGEO_USER slf_palette_vblank(void)
{
    s_irq_begin_line = NG_PERF_LINE();
    if (!s_palette_pending || ng_vram_busy) return;
    ng_vram_busy++;
    s_palette_pending = 0u;
    __asm__ volatile ("" ::: "memory");
    slf_palette_irq_lines = ng_vram_lines_left();
    ng_palfx_vblank();
    actors_commit();
    ng_vram_busy--;
    slf_palette_irq_count++;
}
static NGSpriteGroup s_stage_far, s_stage, s_shadow[2], s_fx[FX_SLOTS];
static NGSpriteGroup s_proj[NGF_PROJECTILES];
static NGSpriteGroup s_title_logo, s_title_sub, s_large_port[2], s_story_panel;
static NGSpriteGroup s_hud_top, s_hud_bottom;
static uint8_t s_hud_plate_live;
typedef struct {
    uint32_t tile;
    uint16_t duration;
    uint8_t bank_count, strips;
    uint16_t palettes[16][16];
    uint8_t map[320];
    char caption[2][40];
} SLFStoryCard;
typedef char SLFStoryCard_size_must_be_920[(sizeof(SLFStoryCard) == 920u) ? 1 : -1];
static SLFStoryCard s_story_card;
uint8_t slf_presentation_current;
static void NEOGEO_USER actors_reset(void);
static uint8_t s_large_port_live[2], s_large_port_def[2];
static uint8_t s_stage_id, s_stage_dim;
static uint8_t s_stage_live, s_stage_far_live;
static uint8_t s_fighter_live[2], s_fighter_pal[2], s_shadow_live[2];
static uint8_t s_proj_live[NGF_PROJECTILES], s_striker_live[2];
static NGSpriteGroup s_wall, s_face[2], s_wall_cursor[2];
static uint8_t s_face_live[2], s_face_def[2], s_wall_cursor_live[2];
volatile uint8_t slf_select_live;       /* (read by the capture scripts) */
volatile uint8_t slf_select_cursor[2];
static const uint8_t k_fx_pal[5] = {18u,19u,20u,24u,25u};
static uint8_t s_fx_kind[FX_SLOTS], s_fx_timer[FX_SLOTS], s_fx_live[FX_SLOTS];
static int16_t s_fx_x[FX_SLOTS], s_fx_y[FX_SLOTS];
static uint8_t s_joy[2], s_joy_prev[2], s_start, s_start_prev;
NGInputQueue slf_input_queue;
volatile uint32_t slf_input_sequence, slf_input_catchups, slf_commit_skipped;
volatile uint32_t slf_hardware_ticks, slf_input_dropped;
volatile uint32_t slf_scene_input_discarded;
static uint8_t s_input_live;
static uint8_t s_scene_setup;
volatile uint32_t slf_scene_input_ignored;
static uint16_t s_frames, s_rng;
/* HUD cache: what the FIX layer shows now. */
static uint8_t s_hud_valid, s_hud_life[2], s_hud_power[2], s_hud_stock[2], s_hud_wins[2], s_hud_secs;
static uint8_t s_hud_combo[2], s_hud_combo_timer[2];
static uint8_t s_hud_trail[2], s_hud_trail_hold[2], s_hud_guard[2], s_hud_spirit[2], s_hud_max[2];

/* ------------------------------------------------------------ palettes */
void NEOGEO_USER slf_presentation_init(void)
{
    ng_palfx_screen_init(s_palette_base, s_palette_out, 104u);
    ng_fix_init();
    ng_fix_set_ascii_base(0xD00u);
    s_stage_id = 0u;
    s_rng = 0x5A17u;
    s_input_live = 1u;
}

/* SYS_IO has completed: snapshot both players and START on this tick,
 * including ticks when the main loop is still building presentation data. */
void NEOGEO_USER slf_input_vblank(void)
{
    if (s_input_live) {
        if (s_scene_setup) {
            slf_input_queue.ticks++;
            slf_scene_input_ignored++;
        } else ng_input_queue_push(&slf_input_queue, NEO_REGISTER8(BIOS_P1CURRENT),
                NEO_REGISTER8(BIOS_P2CURRENT), NEO_REGISTER8(0x10FDACu));
        slf_hardware_ticks = slf_input_queue.ticks;
        slf_input_dropped = slf_input_queue.dropped;
    }
#ifdef NG_DEBUG_PERF
    {
        uint16_t now = NG_PERF_LINE();
        uint16_t lines = now >= s_irq_begin_line ? now - s_irq_begin_line : now + 264u - s_irq_begin_line;
        if (lines > slf_irq_lines_peak) slf_irq_lines_peak = lines;
    }
#endif
}

void NEOGEO_USER slf_fix_palettes_init(void)
{
    uint8_t i;
    for (i = 0u; i < 12u; i++) ng_palfx_screen_load(i, slf_fix_pals[i]);
    for (i = 0u; i < 5u; i++) ng_palfx_screen_load(k_fx_pal[i], slf_fx_pals[i]);
    ng_palfx_screen_load(PAL_SHADOW, slf_shadow_pal);
    ng_palfx_screen_load(PAL_PROJ, slf_proj_pals[0]);
    ng_palfx_screen_load((uint8_t)(PAL_PROJ + 1u), slf_proj_pals[1]);
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
    s_scene_setup = 1u;
    waitVbl();
    setBACKDROP(BLACK);
    clearFix();
    clearSprs();
    ng_sprite_hide_all();
    actors_reset();
    s_stage_live = 0u;
    s_stage_far_live = 0u;
    s_large_port_live[0] = s_large_port_live[1] = 0u;
    for (i = 0u; i < 2u; i++) {
        s_fighter_live[i] = s_shadow_live[i] = 0u;
        s_striker_live[i] = 0u;
    }
    for (i = 0u; i < NGF_PROJECTILES; i++) s_proj_live[i] = 0u;
    for (i = 0u; i < FX_SLOTS; i++) s_fx_live[i] = s_fx_timer[i] = 0u;
    slf_select_live = 0u;
    s_face_live[0] = s_face_live[1] = 0u;
    s_wall_cursor_live[0] = s_wall_cursor_live[1] = 0u;
    s_hud_valid = 0u;
    s_hud_plate_live = 0u;
    ng_fix_invalidate_all();
    slf_fix_palettes_init();
    ng_palfx_screen_fade_in(0u, 12u);
    waitVbl();
    slf_scene_input_discarded += ng_input_queue_discard(&slf_input_queue);
    s_scene_setup = 0u;
}

uint16_t NEOGEO_USER slf_frame(void)
{
    NGInputSample sample;
    if (ng_input_queue_pop(&slf_input_queue, &sample)) {
        /* Catch up simulation with preserved input. Do not publish an IRQ
         * pose buffer until the main loop has finished its newest state. */
        slf_input_catchups++;
    } else {
        NG_PERF_FRAME_END();
        __asm__ volatile ("" ::: "memory");
        s_palette_pending = 1u;
        /* An old wait flag cannot acknowledge this completed pose batch. */
        do { waitVbl(); } while (s_palette_pending);
        NG_PERF_FRAME_BEGIN();
        if (ng_vram_window_open()) ng_vram_commit();
        else slf_commit_skipped++;
        while (!ng_input_queue_pop(&slf_input_queue, &sample)) waitVbl();
    }
    ng_palette_fx_update();
    s_joy_prev[0] = s_joy[0];
    s_joy_prev[1] = s_joy[1];
    s_start_prev = s_start;
    s_joy[0] = sample.joy[0];
    s_joy[1] = sample.joy[1];
    s_start = sample.start;
    slf_input_sequence = sample.tick;
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

/* Two FIX rows a character: the top halves at 0x800 + c, the bottoms at 0x880 + c. */
void NEOGEO_USER slf_puts_tall(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    if (!text || y >= 27u) return;
    for (; *text && x < 40u; text++, x++) {
        uint8_t c = (uint8_t)*text & 0x7Fu;
        ng_fix_put_tile(x, y, (uint16_t)(0x800u + c), pal);
        ng_fix_put_tile(x, (uint8_t)(y + 1u), (uint16_t)(0x880u + c), pal);
    }
}

void NEOGEO_USER slf_puts_tall_center(uint8_t y, const char *text, uint8_t pal)
{
    uint8_t n = slen(text);
    slf_puts_tall((uint8_t)(n < 40u ? (40u - n) / 2u : 0u), y, text, pal);
}

void NEOGEO_USER slf_puts_tall_right(uint8_t x_end, uint8_t y, const char *text, uint8_t pal)
{
    uint8_t n = slen(text);
    slf_puts_tall((uint8_t)(x_end + 1u > n ? x_end + 1u - n : 0u), y, text, pal);
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

/* ------------------------------------------------------------ actors */
/* Each actor keeps a displayed sprite bank and a hidden upload bank. A pose
 * becomes visible only after all its columns have arrived in VRAM. */
#define ACTOR_COLS 12u
#define ACTOR_ROWS 6u
#define ACTOR_NONE 255u
typedef struct {
    uint8_t def, sprite, flip, pal, anchor_x, anchor_y;
    uint8_t row0[ACTOR_COLS], rows[ACTOR_COLS];
    uint16_t scb1[ACTOR_COLS][ACTOR_ROWS * 4u];
} SLFPose;
typedef struct {
    SLFPose pose[3];
    uint16_t first;
    int16_t x, y, left, top;
    uint8_t build, upload, active, column, bank, ready, hide, moved;
} SLFActor;
static SLFActor s_actor[4];
volatile uint16_t slf_actor_commits, slf_actor_deferred, slf_actor_deadline_misses;
volatile uint32_t slf_actor_requested[4], slf_actor_uploading[4], slf_actor_displayed[4];
volatile uint32_t slf_actor_request_tick[4];
volatile uint16_t slf_actor_age[4], slf_actor_age_peak[4];

static uint32_t NEOGEO_USER pose_id(const SLFPose *p)
{
    return ((uint32_t)p->def << 16) | ((uint16_t)p->sprite << 8) | (p->flip ? 128u : 0u) | p->pal;
}

static inline __attribute__((always_inline)) void actor_put(uint16_t addr, const uint16_t *src, uint16_t n)
{
    volatile uint16_t *p = (volatile uint16_t *)VRAM_ADDR;
    NG_PERF_VRAM(n);
    __asm__ volatile (
        "move.w %[addr],(%[p])\n\t"
        "move.w #1,4(%[p])\n\t"
        "subq.w #1,%[n]\n"
        "1:\n\t"
        "move.w (%[s])+,2(%[p])\n\t"
        "dbf %[n],1b"
        : [s] "+&a" (src), [n] "+&d" (n)
        : [p] "a" (p), [addr] "d" (addr)
        : "cc", "memory");
}

static inline __attribute__((always_inline)) void actor_word(uint16_t addr, uint16_t v)
{
    volatile uint16_t *p = (volatile uint16_t *)VRAM_ADDR;
    NG_PERF_VRAM(1u);
    p[0] = addr; p[1] = v;
}

static void NEOGEO_USER actor_position(SLFActor *a, const SLFPose *p, uint16_t first, uint8_t scale)
{
    uint8_t col;
    int16_t left = (int16_t)(a->x - p->anchor_x + (p->flip ? 1 : 0));
    int16_t top = (int16_t)(a->y + 125 - p->anchor_y);
    for (col = 0u; col < ACTOR_COLS; col++) {
        uint16_t sprite = (uint16_t)(first + col);
        if (scale && p->rows[col]) actor_word((uint16_t)(0x8000u + sprite), 0x0FFFu);
        actor_word((uint16_t)(0x8200u + sprite), p->rows[col] ?
            (uint16_t)(((uint16_t)(496 - top - p->row0[col] * 32) & 511u) << 7 | p->rows[col] * 2u) : 0u);
        if (p->rows[col]) actor_word((uint16_t)(0x8400u + sprite), (uint16_t)(((uint16_t)(left + col * 16) & 511u) << 7));
    }
    a->left = left; a->top = top; a->moved = 0u;
}

/* Build, upload and displayed poses have separate RAM ownership. Texture data
 * goes to hidden sprite slots; only a completed pose becomes visible. */
static void NEOGEO_USER actors_commit(void)
{
    uint8_t i, col;
    for (i = 0u; i < 4u; i++) {
        SLFActor *a = &s_actor[i];
        uint16_t first;
        uint32_t age = slf_actor_requested[i] != slf_actor_displayed[i] ?
            slf_input_queue.ticks + 1u - slf_actor_request_tick[i] : 0u;
        slf_actor_age[i] = (uint16_t)(age > 65535u ? 65535u : age);
        if (slf_actor_age[i] > slf_actor_age_peak[i]) slf_actor_age_peak[i] = slf_actor_age[i];
        if (a->hide) {
            if (ng_vram_lines_left() < 8u) { slf_actor_deferred++; continue; }
            for (col = 0u; col < ACTOR_COLS * 2u; col++) actor_word((uint16_t)(0x8200u + a->first + col), 0u);
            a->active = a->upload = ACTOR_NONE;
            a->hide = a->ready = a->moved = 0u;
            slf_actor_requested[i] = slf_actor_uploading[i] = slf_actor_displayed[i] = 0u;
            if (!ng_vram_window_open()) slf_actor_deadline_misses++;
            continue;
        }
        if (a->upload == ACTOR_NONE && a->ready) {
            a->upload = a->build; a->ready = 0u; a->column = 0u;
            slf_actor_uploading[i] = pose_id(&a->pose[a->upload]);
            for (col = 0u; col < 3u; col++) if (col != a->active && col != a->upload) { a->build = col; break; }
        }
        if (a->upload != ACTOR_NONE) {
            const SLFPose *p = &a->pose[a->upload];
            uint8_t next_bank = (uint8_t)(a->active == ACTOR_NONE ? 0u : a->bank ^ 1u);
            first = (uint16_t)(a->first + next_bank * ACTOR_COLS);
            while (a->column < ACTOR_COLS) {
                col = a->column;
                if (p->rows[col]) {
                    if (ng_vram_lines_left() < 6u) break;
                    actor_put((uint16_t)((first + col) << 6), p->scb1[col], (uint16_t)(p->rows[col] * 4u));
                    if (!ng_vram_window_open()) slf_actor_deadline_misses++;
                }
                a->column++;
            }
            if (a->column == ACTOR_COLS && ng_vram_lines_left() >= 12u &&
                ng_palfx_screen_ready(p->pal, slf_fighter_bank_counts[p->def])) {
                if (a->active != ACTOR_NONE)
                    for (col = 0u; col < ACTOR_COLS; col++) actor_word((uint16_t)(0x8200u + a->first + a->bank * ACTOR_COLS + col), 0u);
                actor_position(a, p, first, 1u);
                a->active = a->upload; a->upload = ACTOR_NONE; a->bank = next_bank;
                slf_actor_displayed[i] = pose_id(p); slf_actor_uploading[i] = 0u;
                slf_actor_commits++;
                if (!ng_vram_window_open()) slf_actor_deadline_misses++;
            } else slf_actor_deferred++;
        }
        if (a->active != ACTOR_NONE && a->moved) {
            if (ng_vram_lines_left() < 8u) { slf_actor_deferred++; continue; }
            actor_position(a, &a->pose[a->active], (uint16_t)(a->first + a->bank * ACTOR_COLS), 0u);
            if (!ng_vram_window_open()) slf_actor_deadline_misses++;
        }
    }
}

static void NEOGEO_USER actors_reset(void)
{
    uint8_t i, k;
    for (i = 0u; i < 4u; i++) {
        SLFActor *a = &s_actor[i];
        a->build = a->bank = a->ready = a->hide = a->moved = 0u;
        a->active = a->upload = ACTOR_NONE;
        slf_actor_requested[i] = slf_actor_uploading[i] = slf_actor_displayed[i] = 0u;
        slf_actor_age[i] = 0u;
        for (k = 0u; k < 3u; k++) a->pose[k].def = 255u;
    }
}

static void NEOGEO_USER actor_hide(uint8_t actor)
{
    s_actor[actor].hide = 1u;
    s_actor[actor].ready = 0u;
}

static uint8_t NEOGEO_USER pose_matches(const SLFPose *p, uint8_t def, uint8_t sprite, uint8_t flip, uint8_t pal)
{
    return (uint8_t)(p->def == def && p->sprite == sprite && p->flip == flip && p->pal == pal);
}

static void NEOGEO_USER actor_draw(uint8_t actor, uint8_t *live, uint16_t first,
    uint8_t def, uint8_t sprite, int16_t x, int16_t y, uint8_t flip, uint8_t pal)
{
    SLFActor *a = &s_actor[actor];
    SLFPose *p = &a->pose[a->build];
    const SLFFrameAnchor *root = &slf_fighter_frame_anchors[def][sprite];
    uint8_t col, columns = slf_fighter_columns[def];
    uint32_t requested = ((uint32_t)def << 16) | ((uint16_t)sprite << 8) | (flip ? 128u : 0u) | pal;
    if (requested != slf_actor_requested[actor]) {
        slf_actor_requested[actor] = requested;
        slf_actor_request_tick[actor] = slf_input_queue.ticks;
    }
    a->hide = 0u; a->first = first;
    if (a->x != x || a->y != y) { a->x = x; a->y = y; a->moved = 1u; }
    *live = 1u;
    if (a->active != ACTOR_NONE && pose_matches(&a->pose[a->active],def,sprite,flip,pal)) {
        a->upload = ACTOR_NONE; a->ready = 0u; slf_actor_uploading[actor] = 0u; return;
    }
    if (a->upload != ACTOR_NONE && pose_matches(&a->pose[a->upload],def,sprite,flip,pal)) { a->ready = 0u; return; }
    if (!pose_matches(p,def,sprite,flip,pal)) {
        const SLFStripRef *m, *map = slf_fighter_strip_maps[def] + slf_fighter_strip_offsets[def][sprite];
        uint8_t lo[ACTOR_COLS], hi[ACTOR_COLS];
        uint16_t flipbit = flip ? 1u : 0u;
        for (col = 0u; col < ACTOR_COLS; col++) { lo[col] = 255u; hi[col] = 0u; }
        for (m = map; m->slot != 255u; m++) {
            uint8_t c = (uint8_t)(m->slot % columns), r = (uint8_t)(m->slot / columns);
            if (r < lo[c]) lo[c] = r;
            if (r > hi[c]) hi[c] = r;
        }
        for (col = 0u; col < ACTOR_COLS; col++) {
            uint8_t src, k;
            uint16_t *w = p->scb1[col];
            p->rows[col] = 0u;
            if (col >= columns) continue;
            src = (uint8_t)(flip ? columns - 1u - col : col);
            if (lo[src] == 255u) continue;
            p->row0[col] = lo[src]; p->rows[col] = (uint8_t)(hi[src] - lo[src] + 1u);
            for (k = 0u; k < p->rows[col] * 2u; k++) { w[k*2u] = NG_SPRITE_BLANK_TILE; w[k*2u+1u] = NG_SPRITE_BLANK_ATTR; }
        }
        for (m = map; m->slot != 255u; m++) {
            uint8_t c = (uint8_t)(m->slot % columns), r = (uint8_t)(m->slot / columns);
            uint8_t dest = (uint8_t)(flip ? columns - 1u - c : c);
            uint16_t *w = p->scb1[dest] + (uint8_t)(r - lo[c]) * 4u;
            uint16_t high = (uint16_t)((m->pal_bits >> 4) << 4) | flipbit;
            w[0] = m->tile; w[1] = (uint16_t)((uint16_t)(pal + (m->pal_bits & 3u)) << 8) | high;
            w[2] = (uint16_t)(m->tile + 1u); w[3] = (uint16_t)((uint16_t)(pal + ((m->pal_bits >> 2) & 3u)) << 8) | high;
        }
        p->def = def; p->sprite = sprite; p->flip = flip; p->pal = pal;
        p->anchor_x = flip ? (uint8_t)(columns * 16u - root->x) : root->x; p->anchor_y = root->y;
    }
    a->ready = 1u;
}

void NEOGEO_USER slf_fighter_pose(uint8_t slot, uint8_t def, uint8_t sprite, int16_t x, int16_t y, uint8_t flip)
{
    uint8_t pal = (uint8_t)(PAL_BODY + (s_fighter_pal[slot] & 1u) * 4u);
    actor_draw(slot, &s_fighter_live[slot], (uint16_t)(SPR_FIGHTER + slot * 50u),
        def, sprite, x, y, flip, pal);
}

void NEOGEO_USER slf_fighter_hide(uint8_t slot)
{
    if (s_fighter_live[slot]) {
        actor_hide(slot);
        s_fighter_live[slot] = 0u;
    }
    if (s_shadow_live[slot]) {
        ng_sprite_group_hide_queued(&s_shadow[slot]);
        s_shadow_live[slot] = 0u;
    }
}

void NEOGEO_USER slf_fighter_bank(uint8_t slot, uint8_t bank)
{
    s_fighter_pal[slot & 1u] = bank;
}

/* ------------------------------------------------------------ select wall */
/* Every fighter's face on one octagonal wall (rows of 4, 6, 8, 8, 8, 6, 4:
 * nothing scrolls), a 64x64 face for each player in the top corners, and
 * a cursor frame per player that moves cell to cell. */
#define SPR_WALL   287u          /* 13 strips, in front of the fighters */
#define SPR_FACE   305u          /* 4 strips a side */
#define WALL_X     56
#define WALL_Y     16
#define PAL_FACE   97u           /* 2 a side: 97-100 */
#define PAL_CURSOR_LOCK 31u
static uint8_t s_face_map[2][16];
static const uint16_t k_cursor_lock_pal[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7FFF, 0x7F80};

static uint8_t NEOGEO_USER wall_row(uint8_t def)
{
    uint8_t r = 0u;
    while (r + 1u < SLF_WALL_ROW_COUNT && def >= slf_wall_row_start[r + 1u]) r++;
    return r;
}

/* The fighter one cell over: 0 left, 1 right, 2 up, 3 down. Rows wrap;
 * up and down keep the column, clamped to the shorter rows. */
uint8_t NEOGEO_USER slf_wall_step(uint8_t def, uint8_t dir)
{
    uint8_t r = wall_row(def), k = (uint8_t)(def - slf_wall_row_start[r]), len = slf_wall_row_len[r];
    int8_t col;
    if (dir == 0u) k = (uint8_t)(k ? k - 1u : len - 1u);
    else if (dir == 1u) k = (uint8_t)(k + 1u < len ? k + 1u : 0u);
    else {
        col = (int8_t)(slf_wall_row_off[r] + k);
        r = (uint8_t)(dir == 2u ? (r ? r - 1u : SLF_WALL_ROW_COUNT - 1u) : (r + 1u < SLF_WALL_ROW_COUNT ? r + 1u : 0u));
        col = (int8_t)(col - (int8_t)slf_wall_row_off[r]);
        if (col < 0) col = 0;
        if (col >= (int8_t)slf_wall_row_len[r]) col = (int8_t)(slf_wall_row_len[r] - 1u);
        k = (uint8_t)col;
    }
    return (uint8_t)(slf_wall_row_start[r] + k);
}

void NEOGEO_USER slf_wall_show(void)
{
    uint8_t b;
    for (b = 0u; b < SLF_WALL_BANKS; b++) ng_palfx_screen_load((uint8_t)(SLF_WALL_PAL + b), slf_wall_pals[b]);
    ng_palfx_screen_load(PAL_CURSOR_1P, slf_cursor_pals[0]);
    ng_palfx_screen_load(PAL_CURSOR_2P, slf_cursor_pals[1]);
    ng_palfx_screen_load(PAL_CURSOR_LOCK, k_cursor_lock_pal);
    ng_sprite_group_init(&s_wall, SPR_WALL, SLF_WALL_STRIPS, SLF_WALL_ROWS, (uint16_t)SLF_WALL_TILE, SLF_WALL_PAL);
    ng_sprite_group_set_tile_address(&s_wall, SLF_WALL_TILE);
    ng_sprite_group_set_tile_stride(&s_wall, SLF_WALL_STRIPS);
    ng_sprite_group_set_palette_map(&s_wall, slf_wall_map);
    ng_sprite_group_set_pos(&s_wall, WALL_X, WALL_Y);
    ng_sprite_group_upload(&s_wall);
    slf_select_live = 1u;
}

void NEOGEO_USER slf_wall_face(uint8_t side, uint8_t def)
{
    NGSpriteGroup *g = &s_face[side];
    uint8_t i, pal = (uint8_t)(PAL_FACE + side * 2u);
    if (s_face_live[side] && s_face_def[side] == def) return;
    ng_palfx_screen_load(pal, slf_face_pals[def][0]);
    ng_palfx_screen_load((uint8_t)(pal + 1u), slf_face_pals[def][1]);
    for (i = 0u; i < 16u; i++) s_face_map[side][i] = (uint8_t)(pal + slf_face_maps[def * 16u + i]);
    if (!s_face_live[side]) {
        ng_sprite_group_init(g, (uint16_t)(SPR_FACE + side * 4u), 4u, 4u, (uint16_t)SLF_FACE_TILE, pal);
        ng_sprite_group_set_tile_stride(g, 4u);
        ng_sprite_group_set_pos(g, side ? 254 : 2, 2);
    }
    ng_sprite_group_set_tile_address(g, (uint32_t)SLF_FACE_TILE + (uint32_t)def * SLF_FACE_TILES);
    ng_sprite_group_set_palette_map(g, s_face_map[side]);
    ng_sprite_group_mark_dirty(g, NG_SGF_DIRTY_PALETTE);
    if (!s_face_live[side]) ng_sprite_group_upload(g);
    else ng_sprite_group_flush(g);
    s_face_live[side] = 1u;
    s_face_def[side] = def;
}

/* state: 0 moving, 1 chosen. A moving cursor flashes; a chosen one is gold. */
void NEOGEO_USER slf_wall_cursor(uint8_t p, uint8_t def, uint8_t state, uint16_t tick)
{
    slf_select_cursor[p] = def;
    NGSpriteGroup *g = &s_wall_cursor[p];
    uint8_t r = wall_row(def);
    int16_t x = (int16_t)(WALL_X - 3 + (slf_wall_row_off[r] + def - slf_wall_row_start[r]) * SLF_WALL_CELL);
    int16_t y = (int16_t)(WALL_Y - 3 + r * SLF_WALL_CELL);
    uint8_t pal = (uint8_t)(state || (tick & 24u) == 24u ? PAL_CURSOR_LOCK : (p ? PAL_CURSOR_2P : PAL_CURSOR_1P));
    if (!s_wall_cursor_live[p]) {
        ng_sprite_group_init(g, (uint16_t)(p ? SPR_CURSOR_2P : SPR_CURSOR_1P), 2u, 2u,
            (uint16_t)(SLF_WALL_CURSOR_TILE + p * 4u), pal);
        ng_sprite_group_set_tile_address(g, (uint32_t)SLF_WALL_CURSOR_TILE + p * 4u);
        ng_sprite_group_set_tile_stride(g, 2u);
        ng_sprite_group_set_pos(g, x, y);
        ng_sprite_group_upload(g);
        s_wall_cursor_live[p] = 1u;
        return;
    }
    ng_sprite_group_set_palette(g, pal);
    ng_sprite_group_set_pos(g, x, y);
    ng_sprite_group_flush(g);
}

static void NEOGEO_USER shadow_at(uint8_t slot, int16_t x)
{
    NGSpriteGroup *g = &s_shadow[slot];
    if (!s_shadow_live[slot]) {
        ng_sprite_group_init(g, (uint16_t)(SPR_SHADOW + slot * 4u), 4u, 1u, SLF_SHADOW_TILE, PAL_SHADOW);
        ng_sprite_group_set_tile_stride(g, 4u);
        ng_sprite_group_set_pos(g, (int16_t)(x - 32), SLF_FLOOR_Y - 10);
        ng_sprite_group_flush(g);
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
        if (s_fx_live[i]) { ng_sprite_group_hide_queued(&s_fx[i]); s_fx_live[i] = 0u; }
    }
}

void NEOGEO_USER slf_fx_spawn(uint8_t kind, int16_t screen_x, int16_t screen_y)
{
    uint8_t i, pick = 0u, oldest = 255u;
    for (i = 0u; i < FX_SLOTS; i++) {
        if (!s_fx_timer[i]) { pick = i; break; }
        if (s_fx_timer[i] < oldest) { oldest = s_fx_timer[i]; pick = i; }
    }
    if (kind >= 5u || (i == FX_SLOTS && kind >= 2u)) return;
    s_fx_kind[pick] = kind;
    s_fx_timer[pick] = (uint8_t)(4u * FX_TICKS);
    s_fx_x[pick] = (int16_t)(screen_x - 24);
    s_fx_y[pick] = (int16_t)(screen_y - 24);
}

static void NEOGEO_USER fx_draw(void)
{
    uint8_t i;
    for (i = 0u; i < FX_SLOTS; i++) {
        NGSpriteGroup *g = &s_fx[i];
        uint8_t frame;
        uint16_t tile;
        if (!s_fx_timer[i]) {
            if (s_fx_live[i]) { ng_sprite_group_hide_queued(g); s_fx_live[i] = 0u; }
            continue;
        }
        frame = (uint8_t)(3u - (s_fx_timer[i] - 1u) / FX_TICKS);
        tile = (uint16_t)(SLF_FX_TILE + (s_fx_kind[i] * 4u + frame) * 9u);
        s_fx_timer[i]--;
        if (!s_fx_live[i]) {
            ng_sprite_group_init(g, (uint16_t)(SPR_FX + i * 3u), 3u, 3u, tile, k_fx_pal[s_fx_kind[i]]);
            ng_sprite_group_set_tile_stride(g, 3u);
            ng_sprite_group_set_pos(g, s_fx_x[i], s_fx_y[i]);
            ng_sprite_group_set_priority(g, NG_SG_PRIO_LOW);
            ng_sprite_group_flush(g);
            s_fx_live[i] = 1u;
            continue;
        }
        ng_sprite_group_set_tile_base(g, tile);
        ng_sprite_group_set_palette(g, k_fx_pal[s_fx_kind[i]]);
        ng_sprite_group_set_pos(g, s_fx_x[i], s_fx_y[i]);
        ng_sprite_group_flush(g);
    }
}

void NEOGEO_USER slf_match_draw(const NGFMatch *m, const uint8_t *def_index)
{
    uint8_t i, p, s;
    /* Full scale always: the stage does not shrink, and a shrink that
     * followed the camera rewrote every strip of both fighters each frame. */
    const uint8_t zoom = 255u;
    /* Each fighter keeps its hardware slots while hidden poses are uploaded. */
    slf_stage_show(m->camera_x);
    if ((uint8_t)(m->super_freeze != 0u) != s_stage_dim) {
        uint8_t bank;
        s_stage_dim = (uint8_t)(m->super_freeze != 0u);
        for (bank = 0u; bank < SLF_STAGE_BANKS; bank++)
            ng_palfx_screen_load((uint8_t)(SLF_PAL_STAGE + bank), s_stage_dim ?
                slf_stage_dark_pals[s_stage_id][bank] : slf_stage_pals[s_stage_id][bank]);
    }

    setBACKDROP(BLACK);

    for (i = 0u; i < 2u; i++) {
        uint8_t who = i;
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
                ng_sprite_group_hide_queued(g);
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
            py = (int16_t)(SLF_FLOOR_Y - 24 - (pj->h >> 8));
        }
        uint16_t tile = (uint16_t)(SLF_PROJECTILE_TILE + ((m->frame >> 2) & 3u) * 9u);
        uint8_t fire = slf_fighter_fx[def_index[pj->owner]];
        uint8_t pal = fire ? k_fx_pal[3] : (uint8_t)(PAL_PROJ + (pj->owner & 1u));
        if (fire) tile = (uint16_t)(SLF_FX_TILE + (12u + ((m->frame >> 2) & 3u)) * 9u);
        if (!s_proj_live[p]) {
            ng_sprite_group_init(g, (uint16_t)(SPR_PROJ + p * 3u), 3u, 3u, tile, pal);
            ng_sprite_group_set_tile_stride(g, 3u);
            ng_sprite_group_set_scale(g, zoom, zoom);
            ng_sprite_group_set_flip(g, pj->facing_left, 0u);
            ng_sprite_group_set_pos(g, (int16_t)(px - 24), py);
            ng_sprite_group_set_priority(g, NG_SG_PRIO_HIGH);
            ng_sprite_group_flush(g);
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
        if (!st->active || !st->def) {
            if (s_striker_live[s]) {
                actor_hide((uint8_t)(2u + s));
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
        actor_draw((uint8_t)(2u + s), &s_striker_live[s], (uint16_t)(SPR_STRIKER + s * 50u),
            d_idx, fr->sprite, kx, ky, st->facing_left, pal);
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

/* The power gauge; in MAX mode it shows the time the mode has left. */
static void NEOGEO_USER power_bar(uint8_t side, uint16_t power, uint16_t max_timer)
{
    uint8_t stock = (uint8_t)(power / NGF_POWER_STOCK), c;
    uint8_t px = (uint8_t)((power % NGF_POWER_STOCK) / 2u);
    char digit[2];
    if (stock >= 3u) px = 64u;
    if (max_timer) px = (uint8_t)((uint32_t)max_timer * 64u / NGF_MAX_FRAMES);
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
        for (i = 0u; i < 2u; i++) s_hud_life[i] = s_hud_power[i] = s_hud_stock[i] = s_hud_wins[i] = s_hud_max[i] = 0xFFu;
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
        uint8_t mx = f->max_timer ? (uint8_t)(1u + ((m->frame >> 3) & 1u)) : 0u;
        if (f->max_timer) {
            pw = (uint8_t)((uint32_t)f->max_timer * 64u / NGF_MAX_FRAMES);
            st = (uint8_t)(st | 0x10u);
        }
        if (mx != s_hud_max[i]) {
            slf_puts(i ? 33u : 4u, 25u, mx ? "MAX!!" : "POWER", mx == 1u ? SLF_PAL_RED : mx ? SLF_PAL_GOLD : SLF_PAL_DIM);
            s_hud_max[i] = mx;
        }
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
        if (pw != s_hud_power[i] || st != s_hud_stock[i]) { power_bar(i, f->power, f->max_timer); s_hud_power[i] = pw; s_hud_stock[i] = st; }
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
                    char text[9];
                    text[0] = (char)('0' + s_hud_combo[i] / 10u);
                    text[1] = (char)('0' + s_hud_combo[i] % 10u);
                    text[2] = ' '; text[3] = 'H'; text[4] = 'I'; text[5] = 'T'; text[6] = 'S'; text[7] = '!'; text[8] = '\0';
                    if (text[0] == '0') text[0] = ' ';
                    slf_puts_tall(i ? 30u : 2u, 6u, text, SLF_TALL_RED);
                }
                if (!--s_hud_combo_timer[i]) slf_puts_tall(i ? 30u : 2u, 6u, "        ", SLF_TALL_RED);
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
    ng_palfx_screen_load((uint8_t)(PAL_LARGE_PORTRAIT + side), slf_rebirth_portrait_pals[idx]);
    if (!s_large_port_live[side]) {
        ng_sprite_group_init(g, (uint16_t)(305u + side * 5u), 5u, 6u,
            (uint16_t)slf_rebirth_portrait_tiles[idx], (uint8_t)(PAL_LARGE_PORTRAIT + side));
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
        s_story_card.bank_count == 0u || s_story_card.bank_count > 16u ||
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
    return s_story_card.duration;
}

void NEOGEO_USER slf_story_pan(uint16_t tick, uint8_t reverse)
{
    int16_t travel = (int16_t)(tick >> 4);
    if (travel > 16) travel = 16;
    ng_sprite_group_set_pos(&s_story_panel, (int16_t)(reverse ? -8 - travel : -24 + travel), 0);
    ng_sprite_group_flush(&s_story_panel);
}

static void __attribute__((unused)) NEOGEO_USER mark_at(int16_t y)
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
    slf_puts(8u, 27u, "(C) 2026 EAGLE SOFTWARE", SLF_PAL_TEXT);
}

void NEOGEO_USER slf_title_tick(uint16_t tick)
{
    (void)tick;
}

/* ------------------------------------------------------------ sound */
void NEOGEO_USER slf_music(uint8_t track)
{
    if (s_demo_silent) return;
    soundStopAll();              waitVbl();
    soundCancelFade();           waitVbl();
    soundSetADPCMAVolume(0x3Fu); waitVbl();
    /* In a fight the music sits under the hits and the voices. */
    soundSetADPCMBVolume(track >= SLF_TRACK_STAGE ? 0x70u : 0xA8u); waitVbl();
    soundSetSSGVolume(0x00u);    waitVbl();
    soundSetFMVolume(0x00u);     waitVbl();
    soundSetADPCMBLoop(track != SLF_TRACK_VICTORY); waitVbl();
    playSFXB((uint8_t)(SOUND_TRACK_A + track - 1u));
    waitVbl();
}

void NEOGEO_USER slf_sfx(uint8_t id)
{
    if (id && !s_demo_silent) playSFX((uint8_t)(SOUND_SFX_1 + id - 1u));
}

void NEOGEO_USER slf_audio_demo(uint8_t demo)
{
    s_demo_silent = (uint8_t)(demo && ng_sys_is_mvs() && ng_dip_option(3u) == 1u);
    if (s_demo_silent) { soundStopAll(); waitVbl(); }
}
