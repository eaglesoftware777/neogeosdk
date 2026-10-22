/*
 * sw_draw.c — Seven Wonders presentation.
 *
 * Everything that touches the hardware: the frame pump, palettes and fades,
 * backgrounds, the board's sprites and animations, cats, effects, FIX text
 * and sound. The rules live in the SDK's puzzle engine; this file only shows
 * what it decided. A phase of the board (a swap, a clear, a fall, a bubble's
 * flight) is animated here, and sw_board_idle() tells sw_game.c when the
 * engine may take its next step, so the animation never changes a result.
 *
 * Sprites (hardware slot numbers, lower ones behind):
 *   1-20 background, 27-124 cell backs (match) or bubbles (bubble mode),
 *   125-222 pieces, 147-172 falling bubbles and the shot (bubble mode),
 *   223-254 locks, 255-270 effects, 271-272 cursor, 273-286 launcher and
 *   aim guide, 287-294 cats, 295-310 logo, 311-313 stars; 21-26 the
 *   panel in play. A card's plate (223-238) and the banners (223-240,
 *   241-258) borrow the locks' and effects' slots: none is shown with the board.
 */
#include "sw.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "sdk/2d_engine/ng_fix.h"
#include "sdk/2d_engine/ng_art_asset.h"
#include "sdk/ng_input_repeat.h"
#include "sdk/2d_engine/ng_frame_sync.h"
#include "games/sevenwonders/artbox/generated/sw_assets.h"

void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playSFXB(uint8_t n);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundCancelFade(void);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER soundSetSSGVolume(uint8_t v);
void NEOGEO_USER soundSetFMVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBLoop(uint8_t v);

#define SPR_BG        1u
#define SPR_UNDER     27u
#define SPR_PIECE     125u
#define SPR_FALL      147u
#define SPR_SHOT      171u
#define SPR_NEXT      172u
#define SPR_LOCK      223u
#define SPR_FX        255u
#define SPR_CURSOR    271u
#define SPR_LAUNCH    273u
#define SPR_DOT       275u
#define SPR_CAT       287u
#define SPR_LOGO      295u
#define SPR_STAR      311u
#define SPR_CARD      223u        /* the locks' slots */
#define SPR_BANNER    223u        /* and two of 18 strips */
#define SPR_PANEL     21u

#define PITCH         28           /* match cell spacing, pixels */
#define LOCKS         16u
#define FX            8u
#define FALLS         24u
#define DOTS          12u
#define PAL_BANKS     112u

/* ======================================================================
 * frame pump, input, palettes
 * ==================================================================== */
static uint16_t s_pal_base[PAL_BANKS * 16u];
static uint16_t s_pal_out[PAL_BANKS * 16u];
static NGFrameSync s_video_sync;
static uint16_t s_joy, s_joy_prev, s_pressed, s_repeat, s_frames, s_rng;
static NGInputRepeat s_repeat_x, s_repeat_y;
static void NEOGEO_USER sw_commit_ui(void);
static void NEOGEO_USER sw_unbind_board(void);

void NEOGEO_USER sw_vblank(void)
{
    if (ng_vram_busy || !ng_vram_window_open()) return;
    if (!ng_frame_sync_take(&s_video_sync)) return;
    /* Completed UI before broad fades; bulk transfers retain their deadlines.
     * An idle IRQ never consumes a producer still changing the board. */
    sw_commit_ui();
    ng_palfx_vblank();
    ng_vram_commit();
}

static const NGArtAsset *NEOGEO_USER art(uint8_t id)
{
    return &sw_art[id - 1u];
}

static void NEOGEO_USER load_art_palettes(uint8_t id)
{
    const NGArtAsset *a = art(id);
    uint8_t i;
    for (i = 0; i < sw_art_banks[id - 1u]; i++)
        if ((uint8_t)(a->palette_bank + i) < PAL_BANKS)
            ng_palfx_screen_load((uint8_t)(a->palette_bank + i), sw_art_pal[id - 1u] + 16u * i);
}

static void NEOGEO_USER fix_pal(uint8_t bank, uint16_t ink, uint16_t edge)
{
    uint16_t p[16];
    uint8_t i;
    for (i = 0; i < 16u; i++) p[i] = 0;
    p[1] = ink; p[2] = edge; p[3] = edge;
    ng_palfx_screen_load(bank, p);
}

void NEOGEO_USER sw_fix_palettes(void)
{
    fix_pal(SW_TXT_WHITE, 0x7FFFu, 0x8000u);
    fix_pal(SW_TXT_GOLD, 0x6FC3u, 0x8000u);
    fix_pal(SW_TXT_PINK, 0x6F8Cu, 0x8000u);
    fix_pal(SW_TXT_SKY, 0x16DFu, 0x8000u);
    fix_pal(SW_TXT_MINT, 0x07FAu, 0x8000u);
}

void NEOGEO_USER sw_presentation_init(void)
{
    uint8_t id;
    ng_palfx_screen_init(s_pal_base, s_pal_out, PAL_BANKS);
    ng_fix_init();
    ng_fix_set_ascii_base(0xD00u);
    for (id = SW_ART_PIECES; id <= SW_ART_COUNT; id++) load_art_palettes(id);
    s_rng = 0x5A17u;
}

uint16_t NEOGEO_USER sw_frame(void)
{
    uint16_t x, y;
    if (*(volatile uint8_t *)BIOS_SYSTEM_MODE & 0x80u) {
        ng_frame_sync_publish(&s_video_sync);
        do { waitVbl(); } while (s_video_sync.pending);
    } else {
        /* Established BIOS-owned fallback, outside the game-owned IRQ. */
        waitVbl();
        if (!ng_vram_window_open()) waitVbl();
        sw_commit_ui();
        ng_palfx_vblank();
        ng_vram_commit();
    }
    ng_palette_fx_update();
    s_joy_prev = s_joy;
    s_joy = poll_joystick();
    s_pressed = (uint16_t)(s_joy & (uint16_t)~s_joy_prev);
    x = (uint16_t)(s_joy & (SW_LEFT | SW_RIGHT));
    y = (uint16_t)(s_joy & (SW_UP | SW_DOWN));
    if (x == (SW_LEFT | SW_RIGHT)) x = 0;
    if (y == (SW_UP | SW_DOWN)) y = 0;
    s_repeat = (uint16_t)(ng_input_repeat_step(&s_repeat_x, x, 15u, 4u) |
                          ng_input_repeat_step(&s_repeat_y, y, 15u, 4u));
    s_frames++;
    s_rng ^= (uint16_t)(s_joy + s_frames);
    return s_frames;
}

uint16_t NEOGEO_USER sw_held(void) { return s_joy; }
uint16_t NEOGEO_USER sw_pressed(void) { return s_pressed; }

uint16_t NEOGEO_USER sw_repeat(void)
{
    return (uint16_t)((s_pressed & (uint16_t)~(SW_UP | SW_DOWN | SW_LEFT | SW_RIGHT)) | s_repeat);
}

uint16_t NEOGEO_USER sw_rand(void)
{
    s_rng = (uint16_t)(s_rng * 2053u + 13849u);
    return s_rng;
}

void NEOGEO_USER sw_wait(uint16_t frames)
{
    while (frames--) {
        sw_frame();
        if (sw_start_pending() && !NEO_REGISTER8(SW_START_FLAG)) return;
    }
}

/* ======================================================================
 * FIX text
 * ==================================================================== */
void NEOGEO_USER sw_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    char line[41];
    uint8_t i;
    if (!text || x >= 40u || y >= 32u) return;
    for (i = 0; i < (uint8_t)(40u - x) && text[i]; i++) line[i] = text[i];
    line[i] = 0;
    ng_fix_puts(x, y, line, pal);
}

void NEOGEO_USER sw_puts_c(uint8_t y, const char *text, uint8_t pal)
{
    uint8_t n = 0;
    while (text[n] && n < 40u) n++;
    sw_puts((uint8_t)((40u - n) >> 1), y, text, pal);
}

void NEOGEO_USER sw_putn(uint8_t x, uint8_t y, uint32_t value, uint8_t digits, uint8_t pal)
{
    /* decimal by subtracting powers of ten: at most nine per digit, no divide */
    static const uint32_t pow10[10] = { 1000000000u, 100000000u, 10000000u, 1000000u, 100000u,
                                        10000u, 1000u, 100u, 10u, 1u };
    char buf[11];
    uint8_t i, o = 0, lead = 1;
    if (digits > 10u) digits = 10u;
    for (i = (uint8_t)(10u - digits); i < 10u; i++) {
        char d = '0';
        while (value >= pow10[i]) { value -= pow10[i]; d++; }
        if (d != '0' || i == 9u) lead = 0;
        buf[o++] = lead ? ' ' : d;
    }
    buf[o] = 0;
    sw_puts(x, y, buf, pal);
}

void NEOGEO_USER sw_clear_text(uint8_t y0, uint8_t y1)
{
    uint8_t y;
    for (y = y0; y <= y1 && y < 32u; y++) ng_fix_clear_rect(0, y, 40, 1, 0);
}

/* ======================================================================
 * backgrounds, logo, cats, stars
 * ==================================================================== */
static NGSpriteGroup s_bg, s_logo, s_cat[2], s_star[3], s_card, s_banner[2], s_panel;

static void NEOGEO_USER sw_commit_ui(void)
{
    ng_sprite_group_commit_small(&s_panel);
    ng_fix_commit();
}

static void NEOGEO_USER show_asset(NGSpriteGroup *g, uint16_t first_sprite, uint8_t id, uint8_t frame,
                                   int16_t x, int16_t y)
{
    const NGArtAsset *a = art(id);
    uint16_t per = (uint16_t)((uint16_t)a->strips * a->rows);
    uint8_t prio = g->prio;
    ng_sprite_group_init(g, first_sprite, a->strips, a->rows, (uint16_t)(a->tile_base + per * frame), a->palette_bank);
    if (prio != NG_SG_PRIO_NORMAL) ng_sprite_group_set_priority(g, prio);
    ng_sprite_group_set_palette_map(g, a->tile_palettes + per * frame);
    ng_sprite_group_set_pos(g, x, y);
    ng_sprite_group_set_visible(g, 1);
    ng_sprite_group_mark_dirty(g, NG_SGF_DIRTY_ALL);
    ng_sprite_group_flush(g);
}

static void NEOGEO_USER set_frame(NGSpriteGroup *g, uint8_t id, uint8_t frame)
{
    const NGArtAsset *a = art(id);
    uint16_t per = (uint16_t)((uint16_t)a->strips * a->rows);
    ng_sprite_group_set_tile_base(g, (uint16_t)(a->tile_base + per * frame));
    ng_sprite_group_set_palette_map(g, a->tile_palettes + per * frame);
}

static uint8_t NEOGEO_USER bg_asset(uint8_t which)
{
    if (which == SW_BG_TITLE) return SW_ART_TITLE;
    if (which == SW_BG_MAP) return SW_ART_MAP;
    return (uint8_t)(SW_ART_BG0 + (which < 7u ? which : 0u));
}

void NEOGEO_USER sw_fade_out(void)
{
    ng_palfx_screen_fade_out(NG_PALFX_BLACK, 10u);
    while (ng_palfx_screen_fading()) sw_frame();
    sw_frame();
}

void NEOGEO_USER sw_scene(uint8_t background)
{
    uint8_t id = bg_asset(background);
    sw_unbind_board(); /* old picture may fade, but no board owns this scene */
    sw_fade_out();
    waitVbl();
    clearFix();
    ng_sprite_hide_all();
    s_bg.visible = s_logo.visible = s_card.visible = s_panel.visible = 0;
    s_cat[0].visible = s_cat[1].visible = 0;    /* parked above: show them whole next time */
    s_banner[0].visible = s_banner[1].visible = 0;
    s_star[0].visible = s_star[1].visible = s_star[2].visible = 0;
    ng_fix_invalidate_all();
    load_art_palettes(id);
    ng_palfx_screen_backdrop(0x8000u);
    show_asset(&s_bg, SPR_BG, id, 0, 0, 0);
    ng_sprite_group_upload(&s_bg);              /* upload full background in the dark before fade-in */
    ng_palfx_screen_fade_in(NG_PALFX_BLACK, 12u);
    sw_frame();
}

void NEOGEO_USER sw_logo(uint8_t show, int16_t y)
{
    if (!show) { ng_sprite_group_set_visible(&s_logo, 0); ng_sprite_group_flush(&s_logo); return; }
    ng_sprite_group_set_priority(&s_logo, NG_SG_PRIO_HIGH);
    show_asset(&s_logo, SPR_LOGO, SW_ART_LOGO, 0, (int16_t)((320 - 256) >> 1), y);
}

void NEOGEO_USER sw_cat(uint8_t slot, uint8_t cat, uint8_t mood, int16_t x, int16_t y)
{
    NGSpriteGroup *g = &s_cat[slot & 1u];
    uint8_t id = cat == SW_CAT_KIKO ? SW_ART_KIKO : SW_ART_MIMI;
    if (mood >= SW_MOODS) mood = SW_MOOD_IDLE;
    if (!g->visible || g->firstSprite != (uint16_t)(SPR_CAT + 4u * (slot & 1u))) {
        show_asset(g, (uint16_t)(SPR_CAT + 4u * (slot & 1u)), id, mood, x, y);
        return;
    }
    set_frame(g, id, mood);
    ng_sprite_group_set_pos(g, x, y);
    ng_sprite_group_flush(g);
}

void NEOGEO_USER sw_cat_hide(uint8_t slot)
{
    ng_sprite_group_set_visible(&s_cat[slot & 1u], 0);
    ng_sprite_group_flush(&s_cat[slot & 1u]);
}

void NEOGEO_USER sw_card(uint8_t show)
{
    if (!show) { ng_sprite_group_set_visible(&s_card, 0); ng_sprite_group_flush(&s_card); return; }
    ng_sprite_group_set_priority(&s_card, NG_SG_PRIO_HIGH);
    show_asset(&s_card, SPR_CARD, SW_ART_CARD, 0, 32, 16);
}

void NEOGEO_USER sw_banner(uint8_t which, int16_t y)
{
    show_asset(&s_banner[which & 1u], (uint16_t)(SPR_BANNER + 18u * (which & 1u)), SW_ART_BANNER, 0, 16, y);
}

void NEOGEO_USER sw_panel(uint8_t show)
{
    if (!show) { ng_sprite_group_set_visible(&s_panel, 0); ng_sprite_group_flush(&s_panel); return; }
    ng_sprite_group_set_priority(&s_panel, NG_SG_PRIO_HIGH);
    show_asset(&s_panel, SPR_PANEL, SW_ART_PANEL, 0, 4, 4);
}

void NEOGEO_USER sw_stars(uint8_t count, int16_t x, int16_t y)
{
    uint8_t i;
    for (i = 0; i < 3u; i++)
        show_asset(&s_star[i], (uint16_t)(SPR_STAR + i), SW_ART_STAR, (uint8_t)(i < count ? 1u : 0u),
                   (int16_t)(x + 20 * i), y);
}

/* ======================================================================
 * sound
 * ==================================================================== */
void NEOGEO_USER sw_music(uint8_t track)
{
    soundStopAll();              waitVbl();
    soundCancelFade();           waitVbl();
    soundSetADPCMAVolume(0x3Au); waitVbl();
    soundSetADPCMBVolume(0xB8u); waitVbl();
    soundSetSSGVolume(0x00u);    waitVbl();
    soundSetFMVolume(0x00u);     waitVbl();
    soundSetADPCMBLoop(1u);      waitVbl();
    playSFXB(track);
    waitVbl();
}

void NEOGEO_USER sw_sfx(uint8_t sfx)
{
    if (sfx < SW_SFX_COUNT) playSFX(sfx);
}

/* ======================================================================
 * the board
 * ==================================================================== */
static NGSpriteGroup s_cell[SW_BUBBLE_CELLS]; /* match pieces or bubbles */
static NGSpriteGroup s_under[SW_MATCH_CELLS];
static NGSpriteGroup s_lock[LOCKS];
static NGSpriteGroup s_fx[FX];
static NGSpriteGroup s_fall[FALLS];
static NGSpriteGroup s_shot, s_next, s_cursor, s_launch, s_dot[DOTS];
static int16_t s_ox, s_oy;               /* the board's top-left on screen */
static int16_t s_off[SW_MATCH_CELLS]; /* fall offset, pixels (negative: above) */
static uint8_t s_vel[SW_MATCH_CELLS]; /* speed, pixels per frame * 4 */
static uint8_t s_drawn_color[SW_BUBBLE_CELLS], s_drawn_kind[SW_BUBBLE_CELLS], s_drawn_cover[SW_MATCH_CELLS];
static uint8_t s_swap_a, s_swap_b, s_swap_t, s_swap_back;
static uint8_t s_clear_t;
static uint8_t s_fx_t[FX];
static int16_t s_fall_x[FALLS], s_fall_y[FALLS], s_fall_v[FALLS];
static uint8_t s_fall_on[FALLS];
static uint8_t s_group[(SW_BUBBLE_CELLS + 7u) / 8u]; /* last joined group, bits */
static uint8_t s_mode, s_bound, s_cursor_t, s_drawn_parity;
static NGPuzzle *s_p;
static NGPuzzlePreview s_aim_preview;
static uint32_t s_aim_generation;
/* t/6 of the way, in 256ths: the swap's slide without a divide */
static const uint16_t frac6[13] = { 0, 43, 85, 128, 171, 213, 256, 213, 171, 128, 85, 43, 0 };

int16_t NEOGEO_USER sw_board_x(void) { return s_ox; }
int16_t NEOGEO_USER sw_board_y(void) { return s_oy; }

static uint8_t NEOGEO_USER piece_frame(uint8_t color, uint8_t kind)
{
    if (color == NGP_COLOR_PRISM) return 32u;
    if (!color || color > 8u) return 0;
    if (kind > NGP_KIND_BOMB) kind = NGP_KIND_PLAIN;
    return (uint8_t)(((color - 1u) << 2) + kind);
}

static uint8_t NEOGEO_USER bubble_frame(uint8_t color, uint8_t kind)
{
    if (kind == NGP_KIND_ANCHOR) return 9u;
    if (color == NGP_COLOR_STONE) return 8u;
    if (!color || color > 8u) return 0;
    if (kind == NGP_KIND_RESCUE) return (uint8_t)(10u + color - 1u);
    return (uint8_t)(color - 1u);
}

static void NEOGEO_USER sw_unbind_board(void)
{
    s_bound = 0;
    s_p = 0;
    s_aim_generation++;
}

void NEOGEO_USER sw_board_hide(void)
{
    sw_unbind_board();
    ng_sprite_hide_range(SPR_UNDER, (uint16_t)(SPR_CURSOR + 2u - SPR_UNDER));
    ng_sprite_hide_range(SPR_LAUNCH, (uint16_t)(SPR_DOT + DOTS - SPR_LAUNCH));
}

uint8_t NEOGEO_USER sw_board_bind(NGPuzzle *p)
{
    uint8_t c, n;
    sw_board_hide();
    if (!p || !p->level || !sw_board_dimensions_valid(p->mode, p->w, p->h) ||
        p->mode != p->level->mode || p->w != p->level->w || p->h != p->level->h) return 0u;
    s_p = p;
    s_mode = p->mode;
    s_drawn_parity = 0xFFu;
    s_swap_t = 0; s_clear_t = 0;
    for (c = 0; c < FX; c++) s_fx_t[c] = 0;
    for (c = 0; c < FALLS; c++) s_fall_on[c] = 0;
    if (s_mode == NGP_MODE_MATCH) {
        s_ox = (int16_t)(104 + ((216 - PITCH * p->w) >> 1) - 2);
        s_oy = (int16_t)(12 + ((PITCH * 7 - PITCH * p->h) >> 1) - 2);
        n = (uint8_t)(p->w * p->h);
        for (c = 0; c < n; c++) {
            int16_t x, y;
            ng_puzzle_cell_px(p, c, PITCH, &x, &y);
            show_asset(&s_under[c], (uint16_t)(SPR_UNDER + 2u * c), SW_ART_UNDER, 6, (int16_t)(s_ox + x), (int16_t)(s_oy + y));
            show_asset(&s_cell[c], (uint16_t)(SPR_PIECE + 2u * c), SW_ART_PIECES, 0, (int16_t)(s_ox + x), (int16_t)(s_oy + y));
            s_drawn_color[c] = 0xFFu; s_drawn_kind[c] = 0xFFu; s_drawn_cover[c] = 0xFFu;
            s_off[c] = 0; s_vel[c] = 0;
        }
        for (c = 0; c < LOCKS; c++) show_asset(&s_lock[c], (uint16_t)(SPR_LOCK + 2u * c), SW_ART_LOCK, 0, 0, 240);
    } else {
        s_ox = (int16_t)(104 + ((216 - 16 * p->w) >> 1));
        s_oy = 6;
        n = (uint8_t)(p->w * p->h);
        for (c = 0; c < n; c++) {
            show_asset(&s_cell[c], (uint16_t)(SPR_UNDER + c), SW_ART_BUBBLES, 0, 0, 240);
            ng_sprite_group_set_visible(&s_cell[c], 0);
            ng_sprite_group_flush(&s_cell[c]);
            s_drawn_color[c] = 0xFFu; s_drawn_kind[c] = 0xFFu;
        }
        for (c = 0; c < FALLS; c++) {
            show_asset(&s_fall[c], (uint16_t)(SPR_FALL + c), SW_ART_BUBBLES, 0, 0, 240);
            ng_sprite_group_set_visible(&s_fall[c], 0);
            ng_sprite_group_flush(&s_fall[c]);
        }
        show_asset(&s_shot, SPR_SHOT, SW_ART_BUBBLES, 0, 0, 240);
        show_asset(&s_next, SPR_NEXT, SW_ART_BUBBLES, 0, 0, 240);
        show_asset(&s_launch, SPR_LAUNCH, SW_ART_LAUNCHER, 0, 0, 240);
        for (c = 0; c < DOTS; c++) {
            show_asset(&s_dot[c], (uint16_t)(SPR_DOT + c), SW_ART_DOT, 0, 0, 240);
            ng_sprite_group_set_visible(&s_dot[c], 0);
            ng_sprite_group_flush(&s_dot[c]);
        }
    }
    for (c = 0; c < FX; c++) {
        show_asset(&s_fx[c], (uint16_t)(SPR_FX + 2u * c), SW_ART_POP, 0, 0, 240);
        ng_sprite_group_set_visible(&s_fx[c], 0);
        ng_sprite_group_flush(&s_fx[c]);
    }
    show_asset(&s_cursor, SPR_CURSOR, SW_ART_CURSOR, 0, 0, 240);
    ng_sprite_group_set_visible(&s_cursor, 0);
    ng_sprite_group_flush(&s_cursor);
    s_bound = 1;
    return 1u;
}

static void NEOGEO_USER effect(int16_t x, int16_t y)
{
    uint8_t i;
    for (i = 0; i < FX; i++) {
        if (s_fx_t[i]) continue;
        s_fx_t[i] = 12;
        set_frame(&s_fx[i], SW_ART_POP, 0);
        ng_sprite_group_set_pos(&s_fx[i], x, y);
        ng_sprite_group_set_visible(&s_fx[i], 1);
        ng_sprite_group_flush(&s_fx[i]);
        return;
    }
}

static void NEOGEO_USER bubble_xy(const NGPuzzle *p, uint8_t c, int16_t *x, int16_t *y)
{
    ng_puzzle_cell_px(p, c, 0, x, y);
    *x = (int16_t)(*x + s_ox);
    *y = (int16_t)(*y + s_oy);
}

static void NEOGEO_USER drop_bubble(const NGPuzzle *p, uint8_t c, uint8_t color)
{
    uint8_t i;
    for (i = 0; i < FALLS; i++) {
        int16_t x, y;
        if (s_fall_on[i]) continue;
        bubble_xy(p, c, &x, &y);
        s_fall_on[i] = 1; s_fall_x[i] = x; s_fall_y[i] = y; s_fall_v[i] = (int16_t)(-8 - (sw_rand() & 7u));
        set_frame(&s_fall[i], SW_ART_BUBBLES, bubble_frame(color, 0));
        ng_sprite_group_set_pos(&s_fall[i], x, y);
        ng_sprite_group_set_visible(&s_fall[i], 1);
        ng_sprite_group_flush(&s_fall[i]);
        return;
    }
}

void NEOGEO_USER sw_board_event(NGPuzzle *p, const NGPuzzleEvent *e)
{
    uint8_t c, n;
    switch (e->type) {
    case NGP_EV_SWAP:
        s_swap_a = e->cell; s_swap_b = e->a; s_swap_t = 6; s_swap_back = 0;
        sw_sfx(SW_SFX_SWAP);
        break;
    case NGP_EV_SWAP_BACK:
        s_swap_a = e->cell; s_swap_b = e->a; s_swap_t = 12; s_swap_back = 1;
        sw_sfx(SW_SFX_NOPE);
        break;
    case NGP_EV_MATCH:
        sw_sfx(p->cascade >= 2u ? SW_SFX_CASCADE : SW_SFX_MATCH);
        break;
    case NGP_EV_CLEARED:
        s_clear_t = 10;
        n = (uint8_t)(p->w * p->h);
        for (c = 0; c < n; c++)
            if ((p->cleared[c >> 3] >> (c & 7u)) & 1u) {
                int16_t x, y;
                ng_puzzle_cell_px(p, c, PITCH, &x, &y);
                effect((int16_t)(s_ox + x), (int16_t)(s_oy + y));
            }
        break;
    case NGP_EV_SPECIAL_MADE: sw_sfx(SW_SFX_SPECIAL); break;
    case NGP_EV_SPECIAL_FIRED: sw_sfx(SW_SFX_BLAST); break;
    case NGP_EV_CRATE_HIT: sw_sfx(SW_SFX_CRATE); break;
    case NGP_EV_ICE_HIT: sw_sfx(SW_SFX_ICE); break;
    case NGP_EV_SHUFFLE: sw_sfx(SW_SFX_SHUFFLE); break;
    case NGP_EV_FALL:
        n = (uint8_t)(p->w * p->h);
        for (c = 0; c < n && c < 49u; c++) {
            if (!p->fall[c]) continue;
            s_off[c] = (int16_t)(-(int16_t)p->fall[c] * PITCH);
            s_vel[c] = 4;
        }
        break;
    case NGP_EV_FIRE:
        sw_sfx(SW_SFX_FIRE);
        break;
    case NGP_EV_BOUNCE: sw_sfx(SW_SFX_BOUNCE); break;
    case NGP_EV_ATTACH: {
        /* the group it joined: those that go now are popped, the rest that
         * go are dropped */
        uint8_t q[120], head = 0, tail = 0, col = e->a, d;
        for (c = 0; c < 15u; c++) s_group[c] = 0;
        q[tail++] = e->cell;
        s_group[e->cell >> 3] = (uint8_t)(1u << (e->cell & 7u));
        while (head < tail) {
            uint8_t at = q[head++];
            for (d = 0; d < 6u; d++) {
                uint8_t nb = ng_puzzle_neighbor(p, at, d);
                if (nb == NGP_NO_CELL || p->color[nb] != col || ((s_group[nb >> 3] >> (nb & 7u)) & 1u)) continue;
                s_group[nb >> 3] = (uint8_t)(s_group[nb >> 3] | (1u << (nb & 7u)));
                q[tail++] = nb;
            }
        }
        break;
    }
    case NGP_EV_POP:
        sw_sfx(SW_SFX_POP);
        break;
    case NGP_EV_DROP:
        sw_sfx(SW_SFX_DROP);
        break;
    case NGP_EV_RESCUE: sw_sfx(SW_SFX_RESCUE); break;
    case NGP_EV_TIME_LOW: sw_sfx(SW_SFX_TICK); break;
    default: break;
    }
}

uint8_t NEOGEO_USER sw_board_idle(void)
{
    uint8_t c;
    if (s_swap_t || s_clear_t) return 0;
    if (s_mode == NGP_MODE_MATCH)
        for (c = 0; c < 49u; c++) if (s_off[c]) return 0;
    return 1;
}

static void NEOGEO_USER draw_match(NGPuzzle *p)
{
    uint8_t c, n = (uint8_t)(p->w * p->h), locks = 0;
    for (c = 0; c < n; c++) {
        NGSpriteGroup *g = &s_cell[c];
        int16_t x, y, dx = 0, dy = 0;
        uint8_t color = p->color[c], kind = p->kind[c], cover = p->cover[c];
        ng_puzzle_cell_px(p, c, PITCH, &x, &y);
        /* the cell back: wall, crate, ice, or the plain slot */
        if (cover != s_drawn_cover[c]) {
            uint8_t f = 6;
            if (cover & NGP_COVER_WALL) f = 5;
            else if (NGP_CRATE_HP(cover)) f = (uint8_t)(1u + NGP_CRATE_HP(cover));
            else if (NGP_ICE(cover)) f = (uint8_t)(NGP_ICE(cover) - 1u);
            set_frame(&s_under[c], SW_ART_UNDER, f);
            ng_sprite_group_flush(&s_under[c]);
            s_drawn_cover[c] = cover;
        }
        if ((cover & NGP_COVER_LOCK) && locks < LOCKS) {
            ng_sprite_group_set_pos(&s_lock[locks], (int16_t)(s_ox + x), (int16_t)(s_oy + y));
            ng_sprite_group_set_visible(&s_lock[locks], 1);
            ng_sprite_group_flush(&s_lock[locks]);
            locks++;
        }
        if (s_off[c]) {
            /* falling: speed up toward the slot */
            s_off[c] = (int16_t)(s_off[c] + (s_vel[c] >> 2));
            if (s_vel[c] < 40u) s_vel[c]++;
            if (s_off[c] >= 0) s_off[c] = 0;
            dy = s_off[c];
        }
        if (s_swap_t && (c == s_swap_a || c == s_swap_b)) {
            /* A made swap is already on the board: each piece slides in from
             * the other cell. A refused one goes over and comes back. */
            uint8_t other = c == s_swap_a ? s_swap_b : s_swap_a;
            int16_t ox, oy;
            uint16_t f = s_swap_back ? frac6[s_swap_t] : frac6[s_swap_t > 6u ? 6u : s_swap_t];
            ng_puzzle_cell_px(p, other, PITCH, &ox, &oy);
            dx = (int16_t)(((int16_t)(ox - x) * (int16_t)f) >> 8);
            dy = (int16_t)(dy + (((int16_t)(oy - y) * (int16_t)f) >> 8));
        }
        if (color != s_drawn_color[c] || kind != s_drawn_kind[c]) {
            if (color) set_frame(g, SW_ART_PIECES, piece_frame(color, kind));
            ng_sprite_group_set_visible(g, (uint8_t)(color != 0));
            s_drawn_color[c] = color; s_drawn_kind[c] = kind;
        }
        if (s_clear_t && ((p->cleared[c >> 3] >> (c & 7u)) & 1u)) ng_sprite_group_set_visible(g, 0);
        ng_sprite_group_set_pos(g, (int16_t)(s_ox + x + dx), (int16_t)(s_oy + y + dy));
        ng_sprite_group_flush(g);
    }
    for (c = locks; c < LOCKS; c++) {
        if (!s_lock[c].visible) continue;
        ng_sprite_group_set_visible(&s_lock[c], 0);
        ng_sprite_group_flush(&s_lock[c]);
    }
    if (s_swap_t) s_swap_t--;
    if (s_clear_t) s_clear_t--;
}

static void NEOGEO_USER draw_bubbles(NGPuzzle *p)
{
    uint8_t c, n = (uint8_t)(p->w * p->h);
    uint8_t shifted = (uint8_t)(p->parity != s_drawn_parity);
    uint8_t changed = shifted;
    for (c = 0; c < n; c++) {
        NGSpriteGroup *g = &s_cell[c];
        uint8_t color = p->color[c], kind = p->kind[c];
        /* New rows change the staggered geometry even if a cell keeps the
         * same color/kind. Its cached artwork alone cannot establish position. */
        if (color == s_drawn_color[c] && kind == s_drawn_kind[c] && (!shifted || !color)) continue;
        changed = 1;
        if (!color && s_drawn_color[c] && s_drawn_color[c] != 0xFFu) {
            /* gone: popped where the shot stuck, dropped elsewhere */
            int16_t x, y;
            bubble_xy(p, c, &x, &y);
            if ((s_group[c >> 3] >> (c & 7u)) & 1u) effect((int16_t)(x - 8), (int16_t)(y - 8));
            else drop_bubble(p, c, s_drawn_color[c]);
        }
        if (color) {
            int16_t x, y;
            bubble_xy(p, c, &x, &y);
            set_frame(g, SW_ART_BUBBLES, bubble_frame(color, kind));
            ng_sprite_group_set_pos(g, x, y);
        }
        ng_sprite_group_set_visible(g, (uint8_t)(color != 0));
        ng_sprite_group_flush(g);
        s_drawn_color[c] = color; s_drawn_kind[c] = kind;
    }
    s_drawn_parity = p->parity;
    if (changed) s_aim_generation++;
    for (c = 0; c < FALLS; c++) {
        if (!s_fall_on[c]) continue;
        s_fall_v[c] = (int16_t)(s_fall_v[c] + 1);
        s_fall_y[c] = (int16_t)(s_fall_y[c] + (s_fall_v[c] >> 2));
        if (s_fall_y[c] > 230) {
            s_fall_on[c] = 0;
            ng_sprite_group_set_visible(&s_fall[c], 0);
        } else {
            ng_sprite_group_set_pos(&s_fall[c], s_fall_x[c], s_fall_y[c]);
        }
        ng_sprite_group_flush(&s_fall[c]);
    }
    /* the bubble in flight and the next one */
    if (ng_puzzle_flying(p)) {
        set_frame(&s_shot, SW_ART_BUBBLES, bubble_frame(p->shot_color, 0));
        ng_sprite_group_set_pos(&s_shot, (int16_t)(s_ox + (p->shot_x_q8 >> 8) - 8), (int16_t)(s_oy + (p->shot_y_q8 >> 8) - 8));
        ng_sprite_group_set_visible(&s_shot, 1);
    } else if (p->shot_color && ng_puzzle_status(p) == NGP_STATUS_READY) {
        set_frame(&s_shot, SW_ART_BUBBLES, bubble_frame(p->shot_color, 0));
        ng_sprite_group_set_pos(&s_shot, (int16_t)(s_ox + p->level->launch_x - 8), (int16_t)(s_oy + p->level->launch_y - 8));
        ng_sprite_group_set_visible(&s_shot, 1);
    } else {
        ng_sprite_group_set_visible(&s_shot, 0);
    }
    ng_sprite_group_flush(&s_shot);
    if (p->next_color) {
        set_frame(&s_next, SW_ART_BUBBLES, bubble_frame(p->next_color, 0));
        ng_sprite_group_set_pos(&s_next, (int16_t)(s_ox + p->level->launch_x - 40), (int16_t)(s_oy + p->level->launch_y + 4));
        ng_sprite_group_set_visible(&s_next, 1);
    } else {
        ng_sprite_group_set_visible(&s_next, 0);
    }
    ng_sprite_group_flush(&s_next);
}

void NEOGEO_USER sw_board_draw(NGPuzzle *p)
{
    uint8_t i;
    if (!s_bound || !p || p != s_p) return;
    if (s_mode == NGP_MODE_MATCH) draw_match(p);
    else draw_bubbles(p);
    for (i = 0; i < FX; i++) {
        if (!s_fx_t[i]) continue;
        s_fx_t[i]--;
        set_frame(&s_fx[i], SW_ART_POP, (uint8_t)(3u - (s_fx_t[i] >> 2) > 3u ? 3u : 3u - (s_fx_t[i] >> 2)));
        if (!s_fx_t[i]) ng_sprite_group_set_visible(&s_fx[i], 0);
        ng_sprite_group_flush(&s_fx[i]);
    }
}

void NEOGEO_USER sw_cursor(uint8_t show, uint8_t cell, uint8_t picked)
{
    if (!show || s_mode != NGP_MODE_MATCH) {
        if (s_cursor.visible) { ng_sprite_group_set_visible(&s_cursor, 0); ng_sprite_group_flush(&s_cursor); }
        return;
    }
    {
        int16_t x, y;
        ng_puzzle_cell_px(s_p, cell, PITCH, &x, &y);
        ng_sprite_group_set_pos(&s_cursor, (int16_t)(s_ox + x), (int16_t)(s_oy + y));
    }
    s_cursor_t++;
    set_frame(&s_cursor, SW_ART_CURSOR, (uint8_t)(picked ? 1u : ((s_cursor_t >> 4) & 1u)));
    ng_sprite_group_set_visible(&s_cursor, 1);
    ng_sprite_group_flush(&s_cursor);
}

void NEOGEO_USER sw_aim(NGPuzzle *p, uint8_t show, uint8_t guide)
{
    NGPuzzlePoint prefix[NGP_PREVIEW_POINTS + 1u];
    const NGPuzzlePoint *pts = s_aim_preview.points;
    uint8_t n = 0, i, d = 0;
    int16_t lx, ly;
    if (!p || !p->level || !s_bound || p != s_p) return;
    lx = (int16_t)(s_ox + p->level->launch_x);
    ly = (int16_t)(s_oy + p->level->launch_y);
    if (!show) {
        ng_sprite_group_set_visible(&s_launch, 0); ng_sprite_group_flush(&s_launch);
        for (i = 0; i < DOTS; i++) if (s_dot[i].visible) { ng_sprite_group_set_visible(&s_dot[i], 0); ng_sprite_group_flush(&s_dot[i]); }
        return;
    }
    ng_sprite_group_set_pos(&s_launch, (int16_t)(lx - 16), (int16_t)(ly - 8));
    ng_sprite_group_set_visible(&s_launch, 1);
    ng_sprite_group_flush(&s_launch);
    if (guide) {
        uint8_t state = ng_puzzle_preview_step(&s_aim_preview, p, s_aim_generation, 0);
        if ((state != NGP_PREVIEW_BUSY && state != NGP_PREVIEW_READY) ||
            s_aim_preview.requested_angle != p->angle) {
            ng_puzzle_preview_begin(&s_aim_preview, p, p->angle, s_aim_generation);
        }
        state = ng_puzzle_preview_step(&s_aim_preview, p, s_aim_generation, NGP_PREVIEW_MAX_STEPS);
        n = s_aim_preview.count;
        if (state == NGP_PREVIEW_BUSY) {
            /* The exact computed prefix doubles as a short aiming pointer
             * while turning; extend it only as more collision work finishes. */
            for (i = 0; i < n; i++) {
                prefix[i].x = pts[i].x; prefix[i].y = pts[i].y;
            }
            prefix[n].x = (int16_t)(s_aim_preview.x_q8 >> 8);
            prefix[n].y = (int16_t)(s_aim_preview.y_q8 >> 8);
            n++; pts = prefix;
        }
    }
    /* dots spaced along the predicted path's segments */
    for (i = 0; i + 1u < n && d < DOTS; i++) {
        int16_t x0 = pts[i].x, y0 = pts[i].y, x1 = pts[i + 1u].x, y1 = pts[i + 1u].y;
        uint8_t k, steps = 4;
        for (k = 1; k <= steps && d < DOTS; k++) {
            int16_t x = (int16_t)(x0 + (((x1 - x0) * k) >> 2)), y = (int16_t)(y0 + (((y1 - y0) * k) >> 2));
            ng_sprite_group_set_pos(&s_dot[d], (int16_t)(s_ox + x - 8), (int16_t)(s_oy + y - 8));
            ng_sprite_group_set_visible(&s_dot[d], 1);
            ng_sprite_group_flush(&s_dot[d]);
            d++;
        }
    }
    for (; d < DOTS; d++) if (s_dot[d].visible) { ng_sprite_group_set_visible(&s_dot[d], 0); ng_sprite_group_flush(&s_dot[d]); }
}
