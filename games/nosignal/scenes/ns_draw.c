/*
 * ns_draw.c -- No Signal Z Alpha presentation.
 *
 * The frame pump and input (player 1 only), palettes and fades, the street
 * (a painting wider than the screen, streamed into a 512-pixel wrapping
 * chain of 32 strips), the fighters drawn far to near from the engine's
 * draw list, props, thrown phones, hit sparks, the status line (health is
 * the phone's battery, lives its signal bars), barks, the dialogue box and
 * sound. The engine decides; this file shows it.
 *
 * Sprites (hardware slots, lower behind): 1-32 the street, 37-60 props,
 * 61-76 shots, 77-236 fighters (two exclusive 80-strip depth-layout banks), 237-252 sparks, 253-272 the dialogue plate, 273-276
 * its portrait, 277-294 the card, 295-310 the logo, 311-334 hero pictures
 * on cards and the select screen, 335-338 the hero's portrait on the status
 * line (hardware-shrunk to 32 x 32).
 *
 * The hardware shows at most 96 sprites on a line, counting every sprite
 * whose height covers it, on the screen or not, and drops the highest
 * numbered (here the nearest fighters) past that. So the street's strips are
 * placed one by one and only those on the screen are shown (21, not 32).
 * Two exclusive 80-strip banks hold complete depth layouts, including expert
 * crowds. Changed maps are staged in parked slots across bounded blanks;
 * a prepared control snapshot switches banks without exposing partial poses.
 *
 * Palettes: the heroes' and the screen furniture's stay loaded; a stage
 * loads its painting and its crew's and boss's colours (they share slots,
 * tools/art.py), the dialogue box the speaker's portrait colours.
 */
#include "ns.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "sdk/2d_engine/ng_fix.h"
#include "sdk/2d_engine/ng_art_asset.h"
#include "sdk/2d_engine/ng_render_queue.h"
#include "sdk/ng_perf.h"
#include "games/nosignal/artbox/generated/ns_assets.h"
#include "games/nosignal/artbox/generated/ns_faces.h"
#ifdef NG_CD_NOSIGNAL
#include "games/nosignal/cd/platform.h"
#endif

void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playSFXB(uint8_t n);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundCancelFade(void);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER soundSetSSGVolume(uint8_t v);
void NEOGEO_USER soundSetFMVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBLoop(uint8_t v);

#define SPR_BG      1u
#define SPR_PROP    37u
#define SPR_SHOT    61u
#define SPR_FIGHTER 77u
#define FIGHTER_BANK_STRIPS 80u
#define SPR_FIGHTER_END (SPR_FIGHTER + 2u * FIGHTER_BANK_STRIPS) /* 237 */
#define SPR_FX      237u
#define FXN         8u
#define SPR_DIALOG  253u
#define SPR_FACE    273u
#define SPR_CARD    277u
#define SPR_LOGO    295u
#define SPR_HERO    311u
#define SPR_HUD     335u
#define PAL_BANKS   112u

/* ======================================================================
 * frame pump, input, palettes
 * ==================================================================== */
static uint16_t s_pal_base[PAL_BANKS * 16u];
static uint16_t s_pal_out[PAL_BANKS * 16u];
static volatile uint8_t s_publish;
static uint16_t s_joy, s_joy_prev, s_pressed, s_frames, s_rng;
static void NEOGEO_USER fighters_vblank(uint8_t upload);

void NEOGEO_USER ns_vblank(void)
{
    if (!s_publish || ng_vram_busy) return;
    ng_palfx_vblank();
    ng_vram_busy++;
    fighters_vblank(0u);
    ng_vram_busy--;
    ng_vram_commit();
    ng_vram_busy++;
    fighters_vblank(1u);
    ng_vram_busy--;
    __asm__ volatile ("" ::: "memory");
    s_publish = 0;
}

static const NGArtAsset *NEOGEO_USER art(uint8_t id)
{
    return &ns_art[id - 1u];
}

static void NEOGEO_USER load_art_palettes(uint8_t id)
{
    const NGArtAsset *a = art(id);
    uint8_t i;
    for (i = 0; i < ns_art_banks[id - 1u]; i++)
        if ((uint8_t)(a->palette_bank + i) < PAL_BANKS)
            ng_palfx_screen_load((uint8_t)(a->palette_bank + i), ns_art_pal[id - 1u] + 16u * i);
}

static void NEOGEO_USER fix_pal(uint8_t bank, uint16_t ink, uint16_t edge, uint16_t third)
{
    uint16_t p[16];
    uint8_t i;
    for (i = 0; i < 16u; i++) p[i] = 0;
    p[1] = ink; p[2] = edge; p[3] = third;
    ng_palfx_screen_load(bank, p);
}

/* A health bar's colours (FIX glyphs NS_FIX_ICONS + 4..15): 1 the fill,
 * 4 its light top, 5 its shaded bottom, 2 the dark edge, 3 the empty part. */
static void NEOGEO_USER bar_pal(uint8_t bank, uint16_t fill, uint16_t light, uint16_t shade)
{
    uint16_t p[16];
    uint8_t i;
    for (i = 0; i < 16u; i++) p[i] = 0;
    p[1] = fill; p[2] = 0x3101u; p[3] = 0x7223u; p[4] = light; p[5] = shade;
    ng_palfx_screen_load(bank, p);
}

void NEOGEO_USER ns_fix_palettes(void)
{
    fix_pal(NS_TXT_WHITE, 0x7FFFu, 0x8000u, 0x8000u);
    fix_pal(NS_TXT_GOLD, 0x6FC3u, 0x8000u, 0x8000u);
    fix_pal(NS_TXT_PINK, 0x6F3Cu, 0x8000u, 0x8000u);
    fix_pal(NS_TXT_SKY, 0x16DFu, 0x8000u, 0x8000u);
    fix_pal(NS_TXT_MINT, 0x07FAu, 0x2A0Au, 0x8000u);     /* battery: green cells */
    fix_pal(NS_TXT_RED, 0x4F33u, 0x8000u, 0x8000u);
    fix_pal(NS_TXT_GREY, 0x3888u, 0x8000u, 0x8000u);
    bar_pal(NS_BAR_FULL, 0x34D6u, 0x7BFCu, 0x6184u);    /* green */
    bar_pal(NS_BAR_HALF, 0x3FC2u, 0x4FFAu, 0x4A81u);    /* amber */
    bar_pal(NS_BAR_LOW, 0x2F33u, 0x6FA9u, 0x3911u);     /* red */
    bar_pal(NS_BAR_ENEMY, 0x7F82u, 0x4FD9u, 0x4A51u);   /* orange */
    bar_pal(NS_BAR_BOSS, 0x0E36u, 0x5FABu, 0x5813u);    /* crimson */
}

void NEOGEO_USER ns_presentation_init(void)
{
    static const uint8_t keep[8] = { NS_ART_F_JAY, NS_ART_F_ZOE, NS_ART_F_KAI, NS_ART_PROPS, NS_ART_SPARKS,
                                     NS_ART_DIALOG, NS_ART_CARD, NS_ART_LOGO };
    uint8_t i;
    ng_palfx_screen_init(s_pal_base, s_pal_out, PAL_BANKS);
    ng_fix_init();
    ng_fix_set_ascii_base(0xD00u);
    for (i = 0; i < 8u; i++) load_art_palettes(keep[i]);
    s_rng = 0x51A7u;
}

static void NEOGEO_USER bark_tick_internal(void);

uint16_t NEOGEO_USER ns_frame(void)
{
    NG_PERF_FRAME_END();
    __asm__ volatile ("" ::: "memory");
    s_publish = 1;
    do { waitVbl(); } while (s_publish);
    NG_PERF_FRAME_BEGIN();
    ng_palette_fx_update();
    s_joy_prev = s_joy;
    s_joy = (uint16_t)(poll_joystick() & 0x01FFu);    /* player 1 and its start: nobody else plays */
    s_pressed = (uint16_t)(s_joy & (uint16_t)~s_joy_prev);
    s_frames++;
    s_rng ^= (uint16_t)(s_joy + s_frames);
    return s_frames;
}

uint16_t NEOGEO_USER ns_held(void) { return s_joy; }
uint16_t NEOGEO_USER ns_pressed(void) { return s_pressed; }

uint16_t NEOGEO_USER ns_rand(void)
{
    s_rng = (uint16_t)(s_rng * 2053u + 13849u);
    return s_rng;
}

void NEOGEO_USER ns_wait(uint16_t frames)
{
    while (frames--) {
        ns_frame();
        if (ns_start_pending() && !NEO_REGISTER8(NS_START_FLAG)) return;
    }
}

uint8_t NEOGEO_USER ns_wait_button(uint16_t frames)
{
    uint16_t t;
#ifdef NS_AUTOPLAY
    if (frames > 90u) frames = 90u;
#endif
    for (t = 0; t < frames; t++) {
        ns_frame();
        if (t > 12u && (ns_pressed() & (NS_BUTTONS | NS_START))) return 1;
        if (ns_start_pending() && !NEO_REGISTER8(NS_START_FLAG)) return 0;
    }
    return 0;
}

/* ======================================================================
 * FIX text
 * ==================================================================== */
void NEOGEO_USER ns_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    char line[41];
    uint8_t i;
    if (!text || x >= 40u || y >= 32u) return;
    for (i = 0; i < (uint8_t)(40u - x) && text[i] && text[i] != '\n'; i++) line[i] = text[i];
    line[i] = 0;
    ng_fix_puts(x, y, line, pal);
}

void NEOGEO_USER ns_puts_c(uint8_t y, const char *text, uint8_t pal)
{
    uint8_t n = 0;
    while (text[n] && text[n] != '\n' && n < 40u) n++;
    ns_puts((uint8_t)((40u - n) >> 1), y, text, pal);
}

void NEOGEO_USER ns_putn(uint8_t x, uint8_t y, uint32_t value, uint8_t digits, uint8_t pal)
{
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
    ns_puts(x, y, buf, pal);
}

/* Text in lines of at most `width`, words kept whole. */
static uint8_t NEOGEO_USER wrap_block(uint8_t x, uint8_t y, const char *text, uint8_t width, uint8_t pal)
{
    char line[41];
    uint8_t rows = 0;
    while (*text && rows < 6u) {
        uint8_t n = 0, cut = 0, i;
        while (text[n] && text[n] != '\n' && n < width) { if (text[n] == ' ') cut = n; n++; }
        if (text[n] && text[n] != '\n' && text[n] != ' ' && cut) n = cut;
        for (i = 0; i < n; i++) line[i] = text[i];
        line[n] = 0;
        ns_puts(x, (uint8_t)(y + rows), line, pal);
        rows++;
        text += n;
        while (*text == ' ' || *text == '\n') text++;
    }
    return rows;
}

void NEOGEO_USER ns_text_block(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    wrap_block(x, y, text, (uint8_t)(38u - x), pal);
}

void NEOGEO_USER ns_clear_text(uint8_t y0, uint8_t y1)
{
    uint8_t y;
    for (y = y0; y <= y1 && y < 32u; y++) ng_fix_clear_rect(0, y, 40, 1, 0);
}

/* ======================================================================
 * sprites
 * ==================================================================== */
static NGSpriteGroup s_bg, s_logo, s_card, s_dialog, s_face, s_cards[3], s_hud_face;
static NGSpriteGroup s_fighter_banks[2][NGB_FIGHTERS];
static NGSpriteGroup *s_fighter;
static NGSpriteGroup s_prop[NGB_MAX_PROPS], s_shot[NGB_MAX_SHOTS], s_fx[FXN];
static uint8_t s_rank[NGB_FIGHTERS];
static uint8_t s_order[2][NGB_FIGHTERS], s_count[2];
static uint8_t s_art_of[2][NGB_FIGHTERS], s_pose_of[2][NGB_FIGHTERS];
static uint8_t s_live_bank, s_reorder, s_upload_actor, s_upload_column, s_cutover_ready;
static uint16_t s_cutover_scb[3][FIGHTER_BANK_STRIPS];
static uint16_t s_layout_capacity_faults, s_layout_commits, s_layout_deadline_misses;
static uint8_t s_respawn_t;                      /* frames left of the blink after a respawn */
/* Keep animation uploads small while a new depth layout is prepared in the
 * other, hidden bank. The current layout stays visible until the cutover. */
#define NEW_PICTURES 1u
static uint8_t s_street, s_street_rows;          /* the street is up: its strips are placed one by one */
static uint8_t s_fx_t[FXN];
/* "!" over an enemy about to strike: a FIX cell, rubbed out again after a moment */
#define MARKS 4u
static uint8_t s_mark_col[MARKS], s_mark_row[MARKS], s_mark_t[MARKS];
static uint8_t s_dark;      /* the screen is faded out: set a scene up at once, not over the next blanks */

static void NEOGEO_USER put(NGSpriteGroup *g)
{
    if (s_dark) ng_sprite_group_upload(g);
    else ng_sprite_group_flush(g);
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
    put(g);
}

static void NEOGEO_USER set_frame(NGSpriteGroup *g, uint8_t id, uint8_t frame)
{
    const NGArtAsset *a = art(id);
    uint16_t per = (uint16_t)((uint16_t)a->strips * a->rows);
    ng_sprite_group_set_tile_base(g, (uint16_t)(a->tile_base + per * frame));
    ng_sprite_group_set_palette_map(g, a->tile_palettes + per * frame);
}

static void NEOGEO_USER hide(NGSpriteGroup *g)
{
    if (!g->visible) return;
    ng_sprite_group_set_visible(g, 0);
    ng_sprite_group_flush(g);
}

void NEOGEO_USER ns_fade_out(void)
{
    ng_palfx_screen_fade_out(NG_PALFX_BLACK, 10u);
    while (ng_palfx_screen_fading()) ns_frame();
    ns_frame();
}

static void NEOGEO_USER forget_all(void)
{
    uint8_t i;
    s_bg.visible = s_logo.visible = s_card.visible = s_dialog.visible = s_face.visible = s_hud_face.visible = 0;
    for (i = 0; i < 3u; i++) s_cards[i].visible = 0;
    s_live_bank = s_reorder = s_upload_actor = s_upload_column = s_cutover_ready = 0u;
    s_count[0] = s_count[1] = 0u;
    s_fighter = s_fighter_banks[0];
    for (i = 0; i < NGB_FIGHTERS; i++) {
        s_fighter_banks[0][i].visible = s_fighter_banks[1][i].visible = 0u;
        s_rank[i] = 0xFFu;
        s_art_of[0][i] = s_art_of[1][i] = 0xFFu;
        s_pose_of[0][i] = s_pose_of[1][i] = 0xFFu;
    }
    s_street = 0;
    for (i = 0; i < NGB_MAX_PROPS; i++) s_prop[i].visible = 0;
    for (i = 0; i < NGB_MAX_SHOTS; i++) s_shot[i].visible = 0;
    for (i = 0; i < FXN; i++) { s_fx[i].visible = 0; s_fx_t[i] = 0; }
    for (i = 0; i < MARKS; i++) s_mark_t[i] = 0;
}

static void NEOGEO_USER clear_screen(void)
{
    waitVbl();
    clearFix();
    ng_sprite_hide_all();
    ng_fix_invalidate_all();
    forget_all();
    s_dark = 1;
}

/* A full-screen picture, left dark: the caller adds its sprites and text,
 * then ns_reveal() fades the finished screen in. */
void NEOGEO_USER ns_picture(uint8_t id)
{
    ns_fade_out();
    clear_screen();
#ifdef NG_CD_NOSIGNAL
    ns_cd_load_background(id);
#endif
    load_art_palettes(id);
    ng_palfx_screen_backdrop(0x8000u);
    show_asset(&s_bg, SPR_BG, id, 0, 0, 0);
}

void NEOGEO_USER ns_reveal(void)
{
    ns_frame();                 /* the text written while dark */
    s_dark = 0;
    ng_palfx_screen_fade_in(NG_PALFX_BLACK, 12u);
    ns_frame();
}

void NEOGEO_USER ns_logo(uint8_t show, int16_t x, int16_t y)
{
    if (!show) { hide(&s_logo); return; }
    show_asset(&s_logo, SPR_LOGO, NS_ART_LOGO, 0, x, y);
}

void NEOGEO_USER ns_card(uint8_t show)
{
    if (!show) { hide(&s_card); return; }
    ng_sprite_group_set_priority(&s_card, NG_SG_PRIO_HIGH);
    show_asset(&s_card, SPR_CARD, NS_ART_CARD, 0, 16, 16);
}

static uint8_t NEOGEO_USER hero_art(uint8_t hero)
{
    return (uint8_t)(hero == NS_ZOE ? NS_ART_F_ZOE : hero == NS_KAI ? NS_ART_F_KAI : NS_ART_F_JAY);
}

void NEOGEO_USER ns_hero_card(uint8_t slot, uint8_t hero, uint8_t frame, int16_t x, int16_t y, uint8_t flip)
{
    if (slot >= 3u) return;
    if (hero >= NS_HEROES) { hide(&s_cards[slot]); return; }
    show_asset(&s_cards[slot], (uint16_t)(SPR_HERO + 8u * slot), hero_art(hero), frame, x, y);
    ng_sprite_group_set_flip(&s_cards[slot], flip, 0);
    put(&s_cards[slot]);
}

void NEOGEO_USER ns_cards_hide(void)
{
    uint8_t i;
    for (i = 0; i < 3u; i++) hide(&s_cards[i]);
}

/* Any fighter's picture in the first card slot (wide ones run on into the
 * others' sprites: the gallery shows one at a time), in its own colours. */
void NEOGEO_USER ns_show_fighter(uint8_t id, uint8_t frame, int16_t x, int16_t y)
{
    uint8_t i;
    for (i = 1; i < 3u; i++) hide(&s_cards[i]);
    load_art_palettes(id);
    show_asset(&s_cards[0], SPR_HERO, id, frame, x, y);
}

/* ======================================================================
 * the street
 * ==================================================================== */
#define STREET_STRIPS 21u                /* 336 pixels: the screen, and the strip leaving it */
#define STRIP_DRIVER(rows) ((uint16_t)((496u << 7) | (rows)))   /* SCB3: at the top, its own height */
#define STRIP_STICKY 0x0040u                                    /* SCB3: follows the strip before it */
static uint8_t s_stage_art, s_cols, s_shown[STREET_STRIPS], s_hero_id;
static uint8_t s_ring;                   /* the strip showing the column at the screen's left */
static uint16_t s_left = 0xFFFFu;        /* that column */
static int16_t s_chain_x[2];             /* where each chain's first strip was last put */

/*
 * The street: 21 strips as a ring, strip i showing a painting column c with
 * c = i (mod 21), the 21 columns from the one at the screen's left. In slot
 * order that is two chains: strips s_ring..20 (the left of the screen) and
 * 0..s_ring-1 (the right), each placed by its first strip, so scrolling is
 * two position writes a frame, and every 16 pixels one new column and two
 * sticky bits. (A 32-strip chain over the 512-pixel wrap would be one write,
 * but its 11 strips off the screen still count towards a line's 96 sprites.)
 * Columns past the painting's end show its last one.
 */
static void NEOGEO_USER street(uint16_t cam)
{
    uint16_t left = (uint16_t)(cam >> 4), c;
    uint8_t ring, k, i;
    int16_t x0 = (int16_t)(left * 16u) - (int16_t)cam, x1;
    if (left != s_left) {
        /* the left column's strip: left mod 21, by subtraction (no divide on the 68000) */
        c = left;
        while (c >= STREET_STRIPS) c = (uint16_t)(c - STREET_STRIPS);
        ring = (uint8_t)c;
        if (ring != s_ring) {
            if (s_ring) ng_rq_sprite_pos((uint16_t)(SPR_BG + s_ring), STRIP_STICKY, 0);   /* joins the left chain */
            s_ring = ring;
            s_chain_x[0] = s_chain_x[1] = 0x7FFF;                 /* both first strips put again below */
        }
        for (k = 0, i = ring; k < STREET_STRIPS; k++) {
            uint16_t col = (uint16_t)(left + k);
            if (col >= s_cols) col = (uint16_t)(s_cols - 1u);
            if (s_shown[i] != col) { s_shown[i] = (uint8_t)col; ng_sprite_group_set_strip_column(&s_bg, i, col); }
            if (++i == STREET_STRIPS) i = 0;
        }
        s_left = left;
    }
    if (x0 != s_chain_x[0]) {
        s_chain_x[0] = x0;
        ng_rq_sprite_pos((uint16_t)(SPR_BG + s_ring), STRIP_DRIVER(s_street_rows), (uint16_t)(((uint16_t)x0 & 0x1FFu) << 7));
    }
    x1 = (int16_t)(x0 + (int16_t)((STREET_STRIPS - s_ring) * 16u));
    if (s_ring && x1 != s_chain_x[1]) {
        s_chain_x[1] = x1;
        ng_rq_sprite_pos(SPR_BG, STRIP_DRIVER(s_street_rows), (uint16_t)(((uint16_t)x1 & 0x1FFu) << 7));
    }
}

void NEOGEO_USER ns_stage_show(const NGBeatEmUp *b, const NSStageInfo *si, uint8_t hero)
{
    const NGArtAsset *a;
    uint8_t i;
    ns_fade_out();
    clear_screen();
    s_stage_art = si->bg;
#ifdef NG_CD_NOSIGNAL
    ns_cd_load_background(s_stage_art);
#endif
    s_hero_id = hero;
    a = art(s_stage_art);
    s_cols = a->strips;
    load_art_palettes(s_stage_art);
    load_art_palettes(hero_art(hero));
    if (hero < NS_HEROES && ns_hud_art[hero]) load_art_palettes(ns_hud_art[hero]);
    for (i = 0; i < 6u && si->crew[i]; i++) load_art_palettes(si->crew[i]);
    ng_palfx_screen_backdrop(0x8000u);
    /* the 21-strip ring over the painting, first as one chain at the start of the street */
    ng_sprite_group_init(&s_bg, SPR_BG, STREET_STRIPS, a->rows, a->tile_base, a->palette_bank);
    ng_sprite_group_set_tile_stride(&s_bg, a->strips);
    ng_sprite_group_set_palette_map(&s_bg, a->tile_palettes);
    ng_sprite_group_set_pos(&s_bg, 0, 0);
    ng_sprite_group_set_visible(&s_bg, 1);
    ng_sprite_group_mark_dirty(&s_bg, NG_SGF_DIRTY_ALL);
    put(&s_bg);
    for (i = 0; i < STREET_STRIPS; i++) s_shown[i] = i;
    s_street = 1;
    s_street_rows = a->rows;
    s_ring = 0;
    s_left = 0xFFFFu;
    s_chain_x[0] = s_chain_x[1] = 0x7FFF;
    ns_stage_draw(b);
    ns_reveal();
}

static const NSLook *NEOGEO_USER look_of(const NGBFighterDef *d)
{
    uint8_t i;
    for (i = 0; i < ns_nlooks; i++) if (ns_looks[i].def == d) return &ns_looks[i];
    return &ns_looks[0];
}

static uint8_t s_throw_t;
static uint8_t s_shake;          /* frames of screen shake left (heavy blows) */
static int16_t s_shake_dx;       /* this frame's sideways nudge of the street and everyone on it */

/* The move under way. Two numbers are not rows of the fighter's table: the
 * engine's swing of a held weapon (0xFE) and a throw's recovery (NGB_NONE,
 * an ATTACK state with no move): read as moves they are memory past the
 * table, a picture that doesn't exist and colours from anywhere. */
#define SWING_MOVE 0xFEu
static const NGBMove *NEOGEO_USER move_now(const NGBeatEmUp *b, const NGBFighter *f)
{
    if (f->move == SWING_MOVE) {
        if (f->weapon == NGB_NONE || !b->stage->nweapons) return 0;
        return b->stage->weapons[b->prop[f->weapon].contents].move;
    }
    return f->def && f->move < f->def->nmoves ? &f->def->moves[f->move] : 0;
}

/* Cycles (walking, running, a hurt shake, idling) run on the fighter's own
 * clock, offset by `ph` per fighter so that a crowd's pictures don't all
 * change on the same frame. */
static uint8_t NEOGEO_USER hero_frame(const NGBeatEmUp *b, const NGBFighter *f, uint8_t ph)
{
    static const uint8_t walk[4] = { HF_WALK1, HF_WALK2, HF_WALK3, HF_WALK4 };
    const NGBMove *mv;
    uint16_t t = (uint16_t)(f->timer + ph);
    switch (f->state) {
    case NGB_ST_WALK: return walk[(t >> 3) & 3u];
    case NGB_ST_RUN: return (uint8_t)(((t >> 2) & 1u) ? HF_RUN2 : HF_RUN1);
    case NGB_ST_JUMP: return (uint8_t)(f->vz > 0 ? HF_JUMP_UP : HF_JUMP_DOWN);
    case NGB_ST_ATTACK:
        mv = move_now(b, f);
        if (f->move == SWING_MOVE)          /* a bat or a pipe: raised over the head, then through */
            return (uint8_t)(f->move_frame < (mv ? mv->startup : 5u) ? HF_UPPER : HF_CROSS);
        if (!mv) return HF_THROW;           /* after a throw */
        if (mv->anim == HF_SPECIAL1 && f->move_frame >= (uint8_t)(mv->startup + (mv->active >> 1))) return HF_SPECIAL2;
        if (f->z > (8 << 8) && f->move == f->def->air) return HF_JUMPKICK;
        return mv->anim;
    case NGB_ST_GRAB: return s_throw_t ? HF_THROW : HF_GRAB;
    case NGB_ST_GRABBED: case NGB_ST_HITSTUN: return (uint8_t)((t & 8u) ? HF_HURT_LOW : HF_HURT);
    case NGB_ST_THROWN: case NGB_ST_FALL: return HF_KNOCKED;
    case NGB_ST_DOWN: case NGB_ST_DEAD: return HF_DOWN;
    case NGB_ST_GETUP: return HF_GETUP;
    case NGB_ST_RIDE: return HF_RIDE;
    default:
        if (s_throw_t) return HF_THROW;
        return (uint8_t)(((t >> 5) & 1u) ? HF_IDLE : HF_STANCE);
    }
}

static uint8_t NEOGEO_USER enemy_frame(const NGBeatEmUp *b, const NGBFighter *f, const NSLook *lk, uint8_t ph)
{
    static const uint8_t walk[4] = { EF_WALK1, EF_WALK2, EF_WALK3, EF_WALK2 };
    const NGBMove *mv;
    uint8_t ef;
    uint16_t t = (uint16_t)(f->timer + ph);
    switch (f->state) {
    case NGB_ST_WALK: case NGB_ST_RUN: ef = walk[(t >> 3) & 3u]; break;
    case NGB_ST_ATTACK:
        mv = move_now(b, f);
        if (!mv) { ef = EF_ATTACK; break; }  /* after throwing the hero */
        if (mv->anim >= EF_COUNT) {                       /* the boss's own pictures */
            if (mv->anim == BF_BIKE1 && lk->art == NS_ART_F_RICO) return (uint8_t)(((t >> 2) & 1u) ? BF_BIKE2 : BF_BIKE1);
            return mv->anim;
        }
        ef = (uint8_t)(f->move_frame < mv->startup ? EF_WINDUP : EF_ATTACK);
        break;
    case NGB_ST_JUMP: case NGB_ST_GRAB: ef = EF_ATTACK; break;
    case NGB_ST_GRABBED: case NGB_ST_HITSTUN: ef = EF_HURT; break;
    case NGB_ST_THROWN: case NGB_ST_FALL: ef = EF_KNOCKED; break;
    case NGB_ST_DOWN: case NGB_ST_DEAD: ef = EF_DOWN; break;
    case NGB_ST_GETUP: ef = EF_GETUP; break;
    default: ef = EF_STANCE; break;
    }
    if (lk->phase_frames) return lk->phase_frames[f->phase < 3u ? f->phase : 2u][ef];
    return ef;
}

static uint8_t NEOGEO_USER fighter_visible(const NGBeatEmUp *b, uint8_t id, uint8_t art_id)
{
    const NGBFighter *f = &b->f[id];
    return (uint8_t)(f->alive && look_of(f->def)->art == art_id);
}

static void NEOGEO_USER fighter_position(NGSpriteGroup *g, const NGBeatEmUp *b, uint8_t id, uint8_t art_id)
{
    const NGBFighter *f = &b->f[id];
    const NGArtAsset *a = art(art_id);
    int16_t x = (int16_t)(((f->x - b->camera_x) >> 8) - s_shake_dx);
    int16_t y = (int16_t)((f->y - f->z) >> 8);
    uint8_t blink = (uint8_t)((id == NGB_PLAYER && s_respawn_t && (s_frames & 4u)) ||
        (f->state == NGB_ST_DEAD && (f->timer & 4u)));
    /* A blink keeps its exclusive map resident. Parking the whole figure
     * below the field avoids invalidating/reloading it every four ticks. */
    ng_sprite_group_set_pos(g, (int16_t)(x - ((uint16_t)a->strips << 3)),
        blink ? 240 : (int16_t)(y - look_of(f->def)->lift - (((uint16_t)a->rows << 4) - 4u)));
    ng_sprite_group_set_visible(g, fighter_visible(b, id, art_id));
}

static void NEOGEO_USER fighter_describe(NGSpriteGroup *g, const NGBeatEmUp *b,
    uint8_t id, uint16_t first, uint8_t pic, uint8_t art_id)
{
    const NGArtAsset *a = art(art_id);
    uint16_t per = (uint16_t)(a->strips * a->rows);
    uint16_t offset = (uint16_t)(per * (pic & 0x7Fu));
    ng_sprite_group_detach(g);
    ng_sprite_group_init(g, first, a->strips, a->rows, (uint16_t)(a->tile_base + offset), a->palette_bank);
    ng_sprite_group_set_priority(g, id == NGB_PLAYER ? NG_SG_PRIO_HIGH : NG_SG_PRIO_NORMAL);
    ng_sprite_group_set_palette_map(g, a->tile_palettes + offset);
    ng_sprite_group_set_flip(g, (uint8_t)(pic >> 7), 0u);
    fighter_position(g, b, id, art_id);
}

/* Map columns are prepared only in parked, exclusive slots. The old bank
 * remains live while this takes several blanks. Publish all fighters and
 * retire the old bank together, so depth crossings cannot erase a hero. */
static void NEOGEO_USER fighters_vblank(uint8_t upload)
{
    uint8_t bank, k;
    if (!s_reorder) return;
    bank = (uint8_t)(s_live_bank ^ 1u);
    if (upload) {
        while (s_upload_actor < s_count[bank]) {
            uint8_t id = s_order[bank][s_upload_actor];
            if (!ng_sprite_group_stage_map(&s_fighter_banks[bank][id], &s_upload_column)) return;
            s_upload_actor++;
            s_upload_column = 0u;
        }
        return;
    }
    if (!s_cutover_ready) return;
    if (!ng_sprite_stage_swap((uint16_t)(SPR_FIGHTER + s_live_bank * FIGHTER_BANK_STRIPS),
        FIGHTER_BANK_STRIPS, (uint16_t)(SPR_FIGHTER + bank * FIGHTER_BANK_STRIPS),
        FIGHTER_BANK_STRIPS, s_cutover_scb[0], s_cutover_scb[1], s_cutover_scb[2])) return;
    if (!ng_vram_window_open()) s_layout_deadline_misses++;
    for (k = 0u; k < s_count[bank]; k++) {
        NGSpriteGroup *g = &s_fighter_banks[bank][s_order[bank][k]];
        g->dirty = 0u;
        g->readyRows = g->visible ? g->activeRows : 0u;
    }
    s_live_bank = bank;
    s_fighter = s_fighter_banks[bank];
    s_reorder = s_cutover_ready = 0u;
    s_layout_commits++;
}

/* All pointer walking, palette validation and transform arithmetic happens
 * in the producer. The interrupt publishes just four bounded register runs. */
static void NEOGEO_USER fighters_prepare_cutover(void)
{
    uint8_t bank = (uint8_t)(s_live_bank ^ 1u), k;
    uint16_t col;
    s_cutover_ready = 0u;
    if (!s_reorder || s_upload_actor < s_count[bank]) return;
    for (k = 0u; k < s_count[bank]; k++) {
        uint8_t id = s_order[bank][k], art_id = s_art_of[bank][id];
        const NGArtAsset *a = art(art_id);
        if (!ng_palfx_screen_ready(a->palette_bank, ns_art_banks[art_id - 1u])) return;
    }
    for (col = 0u; col < FIGHTER_BANK_STRIPS; col++) {
        s_cutover_scb[0][col] = 0x0FFFu;
        s_cutover_scb[1][col] = NG_SPRITE_DISABLED_SCB3;
        s_cutover_scb[2][col] = 0u;
    }
    for (k = 0u; k < s_count[bank]; k++) {
        NGSpriteGroup *g = &s_fighter_banks[bank][s_order[bank][k]];
        uint16_t first = (uint16_t)(g->firstSprite - SPR_FIGHTER - bank * FIGHTER_BANK_STRIPS);
        if (!g->visible) continue;
        for (col = 0u; col < g->strips; col++)
            s_cutover_scb[1][first + col] = (uint16_t)(col ? 0x40u | g->activeRows :
                ((uint16_t)(496 - g->y) << 7) | g->activeRows);
        s_cutover_scb[2][first] = (uint16_t)((uint16_t)g->x << 7);
    }
    for (k = 0u; k < s_count[s_live_bank]; k++)
        ng_sprite_group_cancel(&s_fighter[s_order[s_live_bank][k]]);
    s_cutover_ready = 1u;
}

static uint8_t NEOGEO_USER prop_frame(const NGBProp *p)
{
    switch (p->kind) {
    case NGB_PROP_CRATE: return 0;
    case NGB_PROP_BARREL: return 1;
    case NGB_PROP_WEAPON: return (uint8_t)(p->contents ? 9u : 2u);
    case NGB_PROP_FOOD: return 3;
    case NGB_PROP_SCORE: return 4;
    case NGB_PROP_VEHICLE: return 5;
    case NGB_PROP_RAMP: return 6;
    case NGB_PROP_HAZARD: return 7;
    default: return 0;
    }
}

/* What can be picked up bobs a little on the ground (one position write
 * every eight frames), so it reads as something to walk over. */
static int16_t NEOGEO_USER prop_bob(const NGBProp *p, uint8_t i)
{
    static const uint8_t bob[4] = { 0, 1, 2, 1 };
    if (p->state != 0u || !(p->kind == NGB_PROP_FOOD || p->kind == NGB_PROP_SCORE || p->kind == NGB_PROP_WEAPON)) return 0;
    return bob[(uint8_t)((s_frames >> 3) + i) & 3u];
}

/* Where a hero's front fist is, from the feet, facing right (measured on
 * the heroes' 128-pixel frames, a few pixels in from the knuckles): a
 * weapon picked up is drawn with its handle there. */
enum { HAND_STANCE = 0, HAND_IDLE, HAND_WALK, HAND_RUN, HAND_UPPER, HAND_CROSS, HAND_THROW, HANDS };
static const int8_t k_hand[NS_HEROES][HANDS][2] = {
    { { 18, -64 }, { 18, -55 }, { 20, -50 }, { 29, -59 }, { 14, -111 }, { 46, -69 }, { -28, -72 } },   /* Jay */
    { { 15, -61 }, { 16, -57 }, { 17, -47 }, { 24, -60 }, { 19, -97 }, { 39, -70 }, { -13, -55 } },    /* Zoe */
    { { 23, -61 }, { 22, -55 }, { 19, -62 }, { 26, -66 }, { 14, -103 }, { 49, -73 }, { -28, -80 } },   /* Kai */
};

static void NEOGEO_USER hand_of(uint8_t pose, int16_t *x, int16_t *y)
{
    uint8_t k;
    switch (pose) {
    case HF_IDLE: k = HAND_IDLE; break;
    case HF_WALK1: case HF_WALK2: case HF_WALK3: case HF_WALK4: k = HAND_WALK; break;
    case HF_RUN1: case HF_RUN2: k = HAND_RUN; break;
    case HF_UPPER: k = HAND_UPPER; break;      /* raised for the swing */
    case HF_CROSS: k = HAND_CROSS; break;      /* swung through */
    case HF_THROW: k = HAND_THROW; break;
    default: k = HAND_STANCE; break;
    }
    *x = k_hand[s_hero_id < NS_HEROES ? s_hero_id : 0u][k][0];
    *y = k_hand[s_hero_id < NS_HEROES ? s_hero_id : 0u][k][1];
}

static void NEOGEO_USER draw_props(const NGBeatEmUp *b, uint16_t cam)
{
    uint8_t i;
    for (i = 0; i < NGB_MAX_PROPS; i++) {
        const NGBProp *p = &b->prop[i];
        NGSpriteGroup *g = &s_prop[i];
        int16_t sx, sy, left, top;
        uint8_t flip = 0u;
        if (!p->kind || p->state == 3u || (p->state == 1u && p->owner != NGB_NONE && p->kind == NGB_PROP_VEHICLE)) { hide(g); continue; }
        sx = (int16_t)((p->x >> 8) - (int32_t)cam);
        sy = (int16_t)(((p->y - p->z) >> 8) - prop_bob(p, i));
        left = (int16_t)(sx - 16); top = (int16_t)(sy - 30);
        if (p->state == 1u && p->owner == NGB_PLAYER) {   /* in the hero's hand: the handle (6, 26) on it */
            const NGBFighter *h = &b->f[NGB_PLAYER];
            int16_t hx, hy;
            hand_of(hero_frame(b, h, 0u), &hx, &hy);
            flip = h->facing;
            sx = (int16_t)((h->x >> 8) - (int32_t)cam);
            sy = (int16_t)((h->y - h->z) >> 8);
            left = (int16_t)(flip ? sx - hx - 25 : sx + hx - 6);
            top = (int16_t)(sy + hy - 26);
        }
        if (sx < -40 || sx > 360) { hide(g); continue; }
        if (!g->visible) show_asset(g, (uint16_t)(SPR_PROP + 2u * i), NS_ART_PROPS, prop_frame(p), left, top);
        else {
            set_frame(g, NS_ART_PROPS, prop_frame(p));
            ng_sprite_group_set_pos(g, left, top);
        }
        ng_sprite_group_set_flip(g, flip, 0u);
        ng_sprite_group_flush(g);
    }
    for (i = 0; i < NGB_MAX_SHOTS; i++) {
        const NGBShot *s = &b->shot[i];
        NGSpriteGroup *g = &s_shot[i];
        int16_t sx, sy;
        if (!s->alive) { hide(g); continue; }
        sx = (int16_t)((s->x >> 8) - (int32_t)cam);
        sy = (int16_t)((s->y - s->z) >> 8);
        if (!g->visible) show_asset(g, (uint16_t)(SPR_SHOT + 2u * i), NS_ART_PROPS, 8, (int16_t)(sx - 16), (int16_t)(sy - 16));
        else { ng_sprite_group_set_pos(g, (int16_t)(sx - 16), (int16_t)(sy - 16)); ng_sprite_group_flush(g); }
    }
}

void NEOGEO_USER ns_effect(int16_t x, int16_t y, uint8_t kind)
{
    uint8_t i;
    for (i = 0; i < FXN; i++) {
        if (s_fx_t[i]) continue;
        s_fx_t[i] = 12;
        show_asset(&s_fx[i], (uint16_t)(SPR_FX + 2u * i), NS_ART_SPARKS, (uint8_t)(kind == NS_FX_DUST ? 3u : kind == NS_FX_BIG ? 1u : 0u),
                   (int16_t)(x - 16), (int16_t)(y - 16));
        return;
    }
}

static void NEOGEO_USER draw_fx(void)
{
    uint8_t i;
    for (i = 0; i < FXN; i++) {
        if (!s_fx_t[i]) continue;
        if (!--s_fx_t[i]) { hide(&s_fx[i]); continue; }
        if (s_fx_t[i] == 8u || s_fx_t[i] == 4u) {
            uint16_t f = (uint16_t)((s_fx[i].tileBase - art(NS_ART_SPARKS)->tile_base) >> 2);   /* 4 tiles a frame */
            if (f < 2u) { set_frame(&s_fx[i], NS_ART_SPARKS, (uint8_t)(f + 1u)); ng_sprite_group_flush(&s_fx[i]); }
        }
    }
}

/* Two fighters within this many pixels of depth keep last frame's order, so
 * walking side by side doesn't swap their sprites (and redraw both) every frame. */
#define DEPTH_SLACK (6 << 8)

void NEOGEO_USER ns_stage_draw(const NGBeatEmUp *b)
{
    uint8_t ids[NGB_FIGHTERS + NGB_MAX_PROPS + NGB_MAX_SHOTS];
    uint8_t order[NGB_FIGHTERS], want[NGB_FIGHTERS], n, k, i, m = 0u, changed, swapped;
    uint8_t bank = s_live_bank, left = NEW_PICTURES;
    uint16_t used = 0u, cam = (uint16_t)(b->camera_x >> 8);
    static const int8_t k_shake[8] = { 0, 1, -1, 1, -2, 2, -2, 2 };     /* by frames left: dies away */
    s_shake_dx = 0;
    if (s_shake && !s_dark) {
        s_shake--;
        s_shake_dx = k_shake[s_shake & 7u];
        if (s_shake_dx < 0 && cam < 2u) s_shake_dx = (int16_t)-s_shake_dx;   /* the street's left end */
    }
    cam = (uint16_t)(cam + s_shake_dx);
    street(cam);
    draw_props(b, cam);
    n = ng_beatemup_draw_list(b, ids, (uint8_t)sizeof(ids));
    for (k = 0u; k < n; k++)
        if (ids[k] < NGB_FIGHTERS && b->f[ids[k]].alive) order[m++] = ids[k];
    do {
        swapped = 0u;
        for (k = 1u; k < m; k++) {
            uint8_t a = order[k - 1u], c = order[k];
            if (b->f[c].y - b->f[a].y < DEPTH_SLACK && s_rank[c] < s_rank[a]) {
                order[k - 1u] = c; order[k] = a; swapped = 1u;
            }
        }
    } while (swapped);
    for (i = 0u; i < NGB_FIGHTERS; i++) s_rank[i] = 0xFFu;
    changed = (uint8_t)(m != s_count[bank]);
    for (k = 0u; k < m; k++) {
        uint8_t id = order[k];
        const NGBFighter *f = &b->f[id];
        uint8_t art_id = look_of(f->def)->art;
        uint8_t frame = id == NGB_PLAYER ? hero_frame(b, f, 0u) : enemy_frame(b, f, look_of(f->def), (uint8_t)(id * 5u));
        if (frame >= ns_art_frames[art_id - 1u]) frame = 0u;   /* never a picture past the sheet */
        s_rank[id] = k;
        want[id] = (uint8_t)(frame | (f->facing ? 0x80u : 0u));
        used = (uint16_t)(used + art(art_id)->strips);
        if (s_order[bank][k] != id || s_art_of[bank][id] != art_id) changed = 1u;
    }
    if (used > FIGHTER_BANK_STRIPS) {
        s_layout_capacity_faults++;
    } else if (s_dark || (changed && !s_reorder)) {
        uint16_t first;
        uint8_t target = s_dark ? bank : (uint8_t)(bank ^ 1u);
        first = (uint16_t)(SPR_FIGHTER + target * FIGHTER_BANK_STRIPS);
        s_count[target] = m;
        for (k = 0u; k < m; k++) {
            uint8_t id = order[k], art_id = look_of(b->f[id].def)->art;
            s_order[target][k] = id;
            s_art_of[target][id] = art_id;
            s_pose_of[target][id] = want[id];
            fighter_describe(&s_fighter_banks[target][id], b, id, first, want[id], art_id);
            if (s_dark) put(&s_fighter_banks[target][id]);
            first = (uint16_t)(first + art(art_id)->strips);
        }
        if (!s_dark) { s_upload_actor = s_upload_column = 0u; s_reorder = 1u; }
    }
    for (k = 0u; k < s_count[bank]; k++) {
        uint8_t id = s_order[bank][k], art_id = s_art_of[bank][id];
        NGSpriteGroup *g = &s_fighter[id];
        fighter_position(g, b, id, art_id);
        if (s_rank[id] != 0xFFu && look_of(b->f[id].def)->art == art_id &&
            (id == NGB_PLAYER || left || want[id] == s_pose_of[bank][id])) {
            if (id != NGB_PLAYER && want[id] != s_pose_of[bank][id]) left--;
            set_frame(g, art_id, (uint8_t)(want[id] & 0x7Fu));
            ng_sprite_group_set_flip(g, (uint8_t)(want[id] >> 7), 0u);
            s_pose_of[bank][id] = want[id];
        }
        put(g);
    }
    if (s_reorder) {
        uint8_t target = (uint8_t)(bank ^ 1u);
        for (k = 0u; k < s_count[target]; k++) {
            uint8_t id = s_order[target][k];
            fighter_position(&s_fighter_banks[target][id], b, id, s_art_of[target][id]);
        }
    }
    fighters_prepare_cutover();
    if (s_throw_t) s_throw_t--;
    if (s_respawn_t) s_respawn_t--;
    draw_fx();
    bark_tick_internal();
}

/* ======================================================================
 * the status line, barks, banners
 * ==================================================================== */
static uint8_t s_shown_units = 0xFFu, s_shown_lives = 0xFFu, s_combo_t;
static uint32_t s_shown_score = 0xFFFFFFFFu;
static uint16_t s_shown_combo = 0xFFFFu;
static uint8_t s_focus = NGB_NONE, s_focus_t, s_focus_shown = 0xFFu;   /* the enemy last hit */
static const NGBFighterDef *s_focus_def;

#define HUD_COL    6u       /* the status line's text starts right of the hero's portrait */
#define BAR_TILES  12u      /* the hero's and an enemy's bar: 96 steps */
#define BOSS_TILES 20u      /* the boss's: 160 steps */
#define FIX_BAR    (NS_FIX_ICONS + 4u)       /* + 0..8: a cell filled that many pixels */
#define FIX_CAP_L  (NS_FIX_ICONS + 13u)
#define FIX_CAP_R  (NS_FIX_ICONS + 14u)
#define FIX_NUB    (NS_FIX_ICONS + 15u)      /* the battery's end */

/* hp of max in steps of `span`, without a divide (at least one while alive) */
static uint8_t NEOGEO_USER bar_units(uint8_t hp, uint8_t max, uint8_t span)
{
    uint16_t acc = 0, want = (uint16_t)((uint16_t)hp * span);
    uint8_t u = 0;
    if (!max) return 0;
    while (u < span && (uint16_t)(acc + max) <= want) { acc = (uint16_t)(acc + max); u++; }
    if (hp && !u) u = 1;
    return u;
}

static void NEOGEO_USER bar_draw(uint8_t col, uint8_t row, uint8_t tiles, uint8_t units, uint8_t pal, uint16_t end)
{
    uint8_t i;
    ng_fix_put_tile(col, row, FIX_CAP_L, pal);
    for (i = 0; i < tiles; i++) {
        uint8_t u = units >= 8u ? 8u : units;
        units = (uint8_t)(units - u);
        ng_fix_put_tile((uint8_t)(col + 1u + i), row, (uint16_t)(FIX_BAR + u), pal);
    }
    ng_fix_put_tile((uint8_t)(col + 1u + tiles), row, end, pal);
}

void NEOGEO_USER ns_focus(uint8_t fighter, const NGBFighterDef *def)
{
    if (s_focus != fighter || s_focus_def != def) s_focus_shown = 0xFFu;
    s_focus = fighter;
    s_focus_def = def;
    s_focus_t = 150;
}

static void NEOGEO_USER enemy_line(const NGBeatEmUp *b)
{
    const NGBFighter *f;
    uint8_t u;
    if (s_focus == NGB_NONE) return;
    f = &b->f[s_focus];
    if (!s_focus_t || f->def != s_focus_def) {          /* gone, or its slot holds someone new */
        ns_clear_text(3, 3);
        s_focus = NGB_NONE;
        return;
    }
    s_focus_t--;
    u = f->alive ? bar_units(f->hp, f->def->max_hp, BAR_TILES * 8u) : 0u;
    if (u == s_focus_shown) return;
    if (s_focus_shown == 0xFFu) {
        ns_clear_text(3, 3);
        ns_puts(HUD_COL + 15u, 3, look_of(f->def)->name, NS_TXT_WHITE);
    }
    bar_draw(HUD_COL, 3, BAR_TILES, u, NS_BAR_ENEMY, FIX_CAP_R);
    s_focus_shown = u;
}

/* The hero's portrait at the status line's left, its dialogue picture
 * shrunk by the hardware to 32 x 32 (SCB2), in banks of its own. */
static void NEOGEO_USER hud_face(uint8_t hero)
{
    const NGArtAsset *a;
    uint8_t id = hero < NS_HEROES ? ns_hud_art[hero] : 0u;
    if (!id) return;
    a = art(id);
    ng_sprite_group_init(&s_hud_face, SPR_HUD, a->strips, a->rows, a->tile_base, a->palette_bank);
    ng_sprite_group_set_priority(&s_hud_face, NG_SG_PRIO_HIGH);
    ng_sprite_group_set_palette_map(&s_hud_face, a->tile_palettes);
    ng_sprite_group_set_pos(&s_hud_face, 8, 8);
    ng_sprite_group_set_scale(&s_hud_face, 0x7Fu, 0x7Fu);
    ng_sprite_group_set_visible(&s_hud_face, 1);
    ng_sprite_group_mark_dirty(&s_hud_face, NG_SGF_DIRTY_ALL);
    put(&s_hud_face);
}

void NEOGEO_USER ns_hud(const NGBeatEmUp *b, uint8_t hero, uint8_t full)
{
    const NGBFighter *p = &b->f[NGB_PLAYER];
    uint8_t units, i;
    if (full) {
        s_shown_units = s_shown_lives = 0xFFu; s_shown_score = 0xFFFFFFFFu; s_shown_combo = 0xFFFFu;
        s_focus_shown = 0xFFu;
        hud_face(hero);
        ns_puts(HUD_COL, 1, ns_hero_info[hero].name, NS_TXT_GOLD);
    }
    /* the phone's battery: green, amber under half, red under a quarter */
    units = bar_units(p->hp, p->def->max_hp, BAR_TILES * 8u);
    if (units != s_shown_units) {
        bar_draw(HUD_COL, 2, BAR_TILES, units, units <= BAR_TILES * 2u ? NS_BAR_LOW : units <= BAR_TILES * 4u ? NS_BAR_HALF : NS_BAR_FULL,
                 FIX_NUB);
        s_shown_units = units;
    }
    if (b->lives != s_shown_lives) {        /* lives are the signal's bars */
        for (i = 0; i < 5u; i++) ng_fix_put_tile((uint8_t)(20u + i), 1, (uint16_t)(NS_FIX_ICONS + 3u), i < b->lives ? NS_TXT_WHITE : NS_TXT_GREY);
        s_shown_lives = b->lives;
    }
    if (b->score != s_shown_score) { ns_putn(29, 1, b->score, 9, NS_TXT_WHITE); s_shown_score = b->score; }
    enemy_line(b);
    if (b->combo != s_shown_combo) {
        if (b->combo >= 2u) {
            ns_puts(HUD_COL, 4, "COMBO", NS_TXT_PINK);
            ns_putn(HUD_COL + 6u, 4, b->combo, 2, NS_TXT_GOLD);
            ns_puts(HUD_COL + 9u, 4, b->combo >= 10u ? "SKILL ISSUE (THEIRS)" : b->combo >= 6u ? "LET HIM COOK      " : "                  ", NS_TXT_SKY);
            s_combo_t = 90;
        }
        s_shown_combo = b->combo;
    }
    if (s_combo_t && !--s_combo_t) ns_clear_text(4, 4);
}

void NEOGEO_USER ns_boss_bar(const NGBeatEmUp *b, uint8_t boss, const char *name)
{
    static uint8_t shown = 0xFFu;
    const NGBFighter *f;
    uint8_t units;
    if (boss == NGB_NONE || boss >= NGB_FIGHTERS) { if (shown != 0xFFu) { ns_clear_text(26, 26); shown = 0xFFu; } return; }
    f = &b->f[boss];
    units = bar_units(f->hp, f->def->max_hp, BOSS_TILES * 8u);
    if (units == shown) return;
    if (shown == 0xFFu) ns_puts(1, 26, name, NS_TXT_RED);
    bar_draw(16, 26, BOSS_TILES, units, NS_BAR_BOSS, FIX_CAP_R);
    shown = units;
}

static uint16_t s_bark_t, s_banner_t;
static uint8_t s_bark_row;
void NEOGEO_USER ns_mark(int16_t x, int16_t y)
{
    uint8_t i, k = 0;
    if (x <= 8 || x >= 312 || y < 40 || y >= 216) return;     /* never in the status line (rows 1-4) */
    for (i = 1; i < MARKS; i++) if (s_mark_t[i] < s_mark_t[k]) k = i;     /* a free one, or the oldest */
    if (s_mark_t[k]) ng_fix_clear_rect(s_mark_col[k], s_mark_row[k], 1, 1, 0);
    s_mark_col[k] = (uint8_t)(x >> 3);
    s_mark_row[k] = (uint8_t)(y >> 3);
    s_mark_t[k] = 30;
    ns_puts(s_mark_col[k], s_mark_row[k], "!", NS_TXT_RED);
}

void NEOGEO_USER ns_bark(const char *text, int16_t x, int16_t y)
{
    uint8_t n = 0, col, row;
    while (text[n] && n < 30u) n++;
    if (s_bark_t) ns_clear_text(s_bark_row, s_bark_row);
    col = (uint8_t)(x < 8 ? 1 : (x >> 3) > (int16_t)(39 - n) ? 39u - n : (uint8_t)(x >> 3));
    row = (uint8_t)(y < 48 ? 5 : y > 200 ? 24 : (uint8_t)((y >> 3)));
    ns_puts(col, row, text, NS_TXT_GOLD);
    s_bark_row = row;
    s_bark_t = 70;
}

static void NEOGEO_USER bark_tick_internal(void)
{
    uint8_t i;
    for (i = 0; i < MARKS; i++)
        if (s_mark_t[i] && !--s_mark_t[i]) ng_fix_clear_rect(s_mark_col[i], s_mark_row[i], 1, 1, 0);
    if (s_bark_t && !--s_bark_t) ns_clear_text(s_bark_row, s_bark_row);
    if (s_banner_t && !--s_banner_t) ns_clear_text(12, 13);
}

void NEOGEO_USER ns_bark_tick(void) { }

void NEOGEO_USER ns_banner(const char *text, uint8_t pal, uint16_t frames)
{
    ns_clear_text(12, 13);
    ns_puts_c(12, text, pal);
    s_banner_t = frames;
}

/* ======================================================================
 * the dialogue box
 * ==================================================================== */
static const char *const k_names[NS_SPEAKERS] = {
    "JAY", "ZOE", "KAI", "RICO 'SWIPE'", "SNATCH", "DIESEL", "KICKFLIP",
    "BIG FLEX", "PATIENT ZERO-X", "PROTOTYPE M-08", "IRON BEHEMOTH", "GENERAL PROXY", "OVERMIND PRIME", "THE ARCHIVIST", "" };

void NEOGEO_USER ns_dialog(uint8_t speaker, const char *text)
{
    uint8_t face = speaker < NS_SPK_NARRATOR ? ns_face_art[speaker] : 0u, x;
    if (!s_dialog.visible) {
        ng_sprite_group_set_priority(&s_dialog, NG_SG_PRIO_HIGH);
        show_asset(&s_dialog, SPR_DIALOG, NS_ART_DIALOG, 0, 0, 160);
    }
    if (face) {
        load_art_palettes(face);                  /* every portrait shares one palette slot */
        ng_sprite_group_set_priority(&s_face, NG_SG_PRIO_HIGH);
        show_asset(&s_face, SPR_FACE, face, 0, 8, 160);
    } else hide(&s_face);
    ns_clear_text(21, 27);
    x = (uint8_t)(face ? 10u : 3u);
    if (speaker < NS_SPEAKERS && k_names[speaker][0]) ns_puts(x, 21, k_names[speaker], speaker <= NS_SPK_KAI ? NS_TXT_SKY : NS_TXT_PINK);
    wrap_block(x, (uint8_t)(speaker < NS_SPEAKERS && k_names[speaker][0] ? 22u : 22u), text, (uint8_t)(38u - x), NS_TXT_WHITE);
    ns_sfx(NS_SFX_TEXT);
}

/* The dialogue box's plate alone, behind text on a busy picture (hero select). */
void NEOGEO_USER ns_plate(uint8_t show)
{
    if (!show) { hide(&s_dialog); return; }
    ng_sprite_group_set_priority(&s_dialog, NG_SG_PRIO_HIGH);
    show_asset(&s_dialog, SPR_DIALOG, NS_ART_DIALOG, 0, 0, 160);
}

void NEOGEO_USER ns_dialog_hide(void)
{
    hide(&s_dialog);
    hide(&s_face);
    ns_clear_text(20, 27);
}

/* ======================================================================
 * sound
 * ==================================================================== */
void NEOGEO_USER ns_music(uint8_t track)
{
    if (track >= 4u) track++;           /* ADPCM-B track 5 (5.wav) belongs to the eyecatcher */
    soundStopAll();              waitVbl();
    soundCancelFade();           waitVbl();
    soundSetADPCMAVolume(0x3Cu); waitVbl();
    soundSetADPCMBVolume(0xB0u); waitVbl();
    soundSetSSGVolume(0x00u);    waitVbl();
    soundSetFMVolume(0x00u);     waitVbl();
    soundSetADPCMBLoop(1u);      waitVbl();
    playSFXB(track);
    waitVbl();
}

void NEOGEO_USER ns_sfx(uint8_t sfx)
{
    if (sfx < NS_SFX_COUNT) playSFX(sfx);
}

/* the game asks for the throw picture for a moment after a throw */
void NEOGEO_USER ns_throw_pose(void) { s_throw_t = 16; }
void NEOGEO_USER ns_shake(uint8_t frames) { if (frames > 8u) frames = 8u; if (frames > s_shake) s_shake = frames; }
void NEOGEO_USER ns_respawn_blink(void) { s_respawn_t = 90; }
