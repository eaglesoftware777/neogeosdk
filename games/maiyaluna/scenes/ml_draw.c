/*
 * ml_draw.c -- Maiya & Luna presentation.
 *
 * Everything that touches the hardware: the frame pump and input, palettes
 * and fades, the room picture, people, objects and water drawn from the RPG
 * engine's state, the dialogue box with portraits, the status line, menu
 * plates and icons, and sound. The rules live in the engine; this file only
 * shows what it decided, a frame after it decided it.
 *
 * Sprites (hardware slot numbers, lower ones behind):
 *   1-20 the room, 21-80 water in channels and light, 81-208 objects (four
 *   slots each), 209-218 the sisters (the first game's 80 x 64 frames),
 *   219-226 effects, 227-246 the dialogue plate, 247-250 its portrait,
 *   251-268 the menu card, 269-284 the logo, 285-300 menu icons, 301-308
 *   people on cards and the title.
 */
#include "ml.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "sdk/2d_engine/ng_fix.h"
#include "sdk/2d_engine/ng_art_asset.h"
#include "games/maiyaluna/artbox/generated/ml_assets.h"
#include "games/maiyaluna/artbox/generated/ml_looks.h"

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
#define SPR_WATER   21u
#define WATERS      60u
#define SPR_OBJ     81u
#define SPR_PARTY   209u
#define SPR_FX      219u
#define FXN         8u
#define SPR_DIALOG  227u
#define SPR_FACE    247u
#define SPR_CARD    251u
#define SPR_LOGO    269u
#define SPR_ICON    285u
#define ICONS       16u
#define SPR_PEOPLE  301u
#define PERSON_STRIPS 5u   /* the sisters' 80 px frames; 4 people end at 320 */
#define PAL_BANKS   112u

/* ======================================================================
 * frame pump, input, palettes
 * ==================================================================== */
static uint16_t s_pal_base[PAL_BANKS * 16u];
static uint16_t s_pal_out[PAL_BANKS * 16u];
static volatile uint8_t s_publish;
static uint16_t s_joy, s_joy_prev, s_pressed, s_released, s_hold_frames, s_frames, s_rng;

static void NEOGEO_USER dialog_tick(void);

void NEOGEO_USER ml_vblank(void)
{
    if (!s_publish) return;     /* only a frame the game has finished */
    s_publish = 0;
    ng_palfx_vblank();
}

static const NGArtAsset *NEOGEO_USER art(uint8_t id)
{
    return &ml_art[id - 1u];
}

static void NEOGEO_USER load_art_palettes(uint8_t id)
{
    const NGArtAsset *a = art(id);
    uint8_t i;
    for (i = 0; i < ml_art_banks[id - 1u]; i++)
        if ((uint8_t)(a->palette_bank + i) < PAL_BANKS)
            ng_palfx_screen_load((uint8_t)(a->palette_bank + i), ml_art_pal[id - 1u] + 16u * i);
}

static void NEOGEO_USER fix_pal(uint8_t bank, uint16_t ink, uint16_t edge, uint16_t third)
{
    uint16_t p[16];
    uint8_t i;
    for (i = 0; i < 16u; i++) p[i] = 0;
    p[1] = ink; p[2] = edge; p[3] = third;
    ng_palfx_screen_load(bank, p);
}

void NEOGEO_USER ml_fix_palettes(void)
{
    fix_pal(ML_TXT_WHITE, 0x7FFFu, 0x8000u, 0x8000u);
    fix_pal(ML_TXT_GOLD, 0x6FC3u, 0x8000u, 0x8000u);
    fix_pal(ML_TXT_PINK, 0x6F8Cu, 0x8000u, 0x8000u);
    fix_pal(ML_TXT_SKY, 0x16DFu, 0x8000u, 0x8000u);
    fix_pal(ML_TXT_MINT, 0x07FAu, 0x8000u, 0x8000u);
    fix_pal(ML_TXT_HEART, 0x4F33u, 0x8000u, 0x8000u);    /* red fill, dark rim */
    fix_pal(ML_TXT_GREY, 0x3888u, 0x8000u, 0x8000u);
}

void NEOGEO_USER ml_presentation_init(void)
{
    uint8_t id;
    ng_palfx_screen_init(s_pal_base, s_pal_out, PAL_BANKS);
    ng_fix_init();
    ng_fix_set_ascii_base(0xD00u);
    for (id = ML_ART_P_MAIYA; id <= ML_ART_COUNT; id++) load_art_palettes(id);
    s_rng = 0x3A71u;
}

uint16_t NEOGEO_USER ml_frame(void)
{
    s_publish = 1;
    do { waitVbl(); } while (s_publish);
    if (!ng_vram_window_open()) waitVbl();
    ng_vram_commit();
    ng_palette_fx_update();
    s_joy_prev = s_joy;
    s_joy = poll_joystick();
    s_pressed = (uint16_t)(s_joy & (uint16_t)~s_joy_prev);
    s_released = (uint16_t)(s_joy_prev & (uint16_t)~s_joy);
    if (s_joy & ML_DIRS) s_hold_frames++;
    else s_hold_frames = 0;
    s_frames++;
    s_rng ^= (uint16_t)(s_joy + s_frames);
    dialog_tick();
    return s_frames;
}

uint16_t NEOGEO_USER ml_held(void) { return s_joy; }
uint16_t NEOGEO_USER ml_pressed(void) { return s_pressed; }
uint16_t NEOGEO_USER ml_released(void) { return s_released; }

uint16_t NEOGEO_USER ml_repeat(void)
{
    uint16_t r = s_pressed;
    if (s_hold_frames > 14u && !(s_hold_frames & 3u)) r = (uint16_t)(r | (s_joy & ML_DIRS));
    return r;
}

uint16_t NEOGEO_USER ml_rand(void)
{
    s_rng = (uint16_t)(s_rng * 2053u + 13849u);
    return s_rng;
}

void NEOGEO_USER ml_wait(uint16_t frames)
{
    while (frames--) {
        ml_frame();
        if (ml_start_pending() && !NEO_REGISTER8(ML_START_FLAG)) return;
    }
}

/* Waits for a button (or the time); 1 if a button ended it. */
uint8_t NEOGEO_USER ml_wait_button(uint16_t frames)
{
    uint16_t t;
#ifdef ML_AUTOPLAY
    if (frames > 120u) frames = 120u;     /* the test build reads a card in two seconds */
#endif
    for (t = 0; t < frames; t++) {
        ml_frame();
        if (t > 12u && (ml_pressed() & (ML_BUTTONS | ML_START))) return 1;
        if (ml_start_pending() && !NEO_REGISTER8(ML_START_FLAG)) return 0;
    }
    return 0;
}

/* ======================================================================
 * FIX text
 * ==================================================================== */
void NEOGEO_USER ml_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    char line[41];
    uint8_t i;
    if (!text || x >= 40u || y >= 32u) return;
    for (i = 0; i < (uint8_t)(40u - x) && text[i] && text[i] != '\n'; i++) line[i] = text[i];
    line[i] = 0;
    ng_fix_puts(x, y, line, pal);
}

void NEOGEO_USER ml_puts_c(uint8_t y, const char *text, uint8_t pal)
{
    uint8_t n = 0;
    while (text[n] && text[n] != '\n' && n < 40u) n++;
    ml_puts((uint8_t)((40u - n) >> 1), y, text, pal);
}

void NEOGEO_USER ml_putn(uint8_t x, uint8_t y, uint32_t value, uint8_t digits, uint8_t pal)
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
    ml_puts(x, y, buf, pal);
}

/* Lines separated by '\n', one row each. */
void NEOGEO_USER ml_text_block(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    while (text && *text && y < 32u) {
        ml_puts(x, y, text, pal);
        while (*text && *text != '\n') text++;
        if (*text == '\n') text++;
        y++;
    }
}

void NEOGEO_USER ml_clear_text(uint8_t y0, uint8_t y1)
{
    uint8_t y;
    for (y = y0; y <= y1 && y < 32u; y++) ng_fix_clear_rect(0, y, 40, 1, 0);
}

/* ======================================================================
 * sprites: helpers, pictures, plates, icons
 * ==================================================================== */
static NGSpriteGroup s_bg, s_logo, s_card, s_dialog, s_face, s_icon[ICONS], s_people[4], s_fx[FXN];
static uint8_t s_fx_t[FXN];

static uint8_t s_dark;      /* the screen is faded out: a scene is set up at once, not over the next blanks */

/* A group's changes: written at once while the screen is dark (a room's
 * whole set-up would otherwise take several blanks and pop in during the
 * fade), in the next blank otherwise. */
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

static void NEOGEO_USER show_asset_scaled(NGSpriteGroup *g, uint16_t first_sprite, uint8_t id, uint8_t frame,
                                          int16_t x, int16_t y, uint8_t scale_x, uint8_t scale_y)
{
    const NGArtAsset *a = art(id);
    uint16_t per = (uint16_t)((uint16_t)a->strips * a->rows);
    uint8_t prio = g->prio;
    ng_sprite_group_init(g, first_sprite, a->strips, a->rows, (uint16_t)(a->tile_base + per * frame), a->palette_bank);
    if (prio != NG_SG_PRIO_NORMAL) ng_sprite_group_set_priority(g, prio);
    ng_sprite_group_set_palette_map(g, a->tile_palettes + per * frame);
    ng_sprite_group_set_pos(g, x, y);
    ng_sprite_group_set_scale(g, scale_x, scale_y);
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
    put(g);
}

void NEOGEO_USER ml_fade_out(void)
{
    ng_palfx_screen_fade_out(NG_PALFX_BLACK, 10u);
    while (ng_palfx_screen_fading()) ml_frame();
    ml_frame();
}

static NGSpriteGroup s_obj[NGR_MAX_OBJECTS][4];
static NGSpriteGroup s_water[WATERS];
static NGSpriteGroup s_member[2];

/* Everything was parked: the groups must be shown whole again. */
static void NEOGEO_USER forget_all(void)
{
    uint8_t i, k;
    s_bg.visible = s_logo.visible = s_card.visible = s_dialog.visible = s_face.visible = 0;
    for (i = 0; i < ICONS; i++) s_icon[i].visible = 0;
    for (i = 0; i < 4u; i++) s_people[i].visible = 0;
    for (i = 0; i < FXN; i++) { s_fx[i].visible = 0; s_fx_t[i] = 0; }
    for (i = 0; i < NGR_MAX_OBJECTS; i++) for (k = 0; k < 4u; k++) s_obj[i][k].visible = 0;
    for (i = 0; i < WATERS; i++) s_water[i].visible = 0;
    s_member[0].visible = s_member[1].visible = 0;
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

void NEOGEO_USER ml_picture(uint8_t id)
{
    ml_fade_out();
    clear_screen();
    load_art_palettes(id);
    ng_palfx_screen_backdrop(0x8000u);
    show_asset(&s_bg, SPR_BG, id, 0, 0, 0);
    s_dark = 0;
    ng_palfx_screen_fade_in(NG_PALFX_BLACK, 12u);
    ml_frame();
}

void NEOGEO_USER ml_chapter_picture(uint8_t chapter)
{
    ml_picture(ml_card_art[chapter < ML_CHAPTERS ? chapter : 0u]);
}

void NEOGEO_USER ml_logo(uint8_t show, int16_t x, int16_t y)
{
    if (!show) { hide(&s_logo); return; }
    ng_sprite_group_set_priority(&s_logo, NG_SG_PRIO_HIGH);
    show_asset(&s_logo, SPR_LOGO, ML_ART_LOGO, 0, x, y);
}

void NEOGEO_USER ml_card(uint8_t show)
{
    if (!show) { hide(&s_card); return; }
    ng_sprite_group_set_priority(&s_card, NG_SG_PRIO_HIGH);
    show_asset(&s_card, SPR_CARD, ML_ART_CARD, 0, 16, 16);
}

void NEOGEO_USER ml_icon(uint8_t slot, uint8_t frame, int16_t x, int16_t y)
{
    NGSpriteGroup *g;
    if (slot >= ICONS) return;
    g = &s_icon[slot];
    if (frame == 0xFFu) { hide(g); return; }
    show_asset(g, (uint16_t)(SPR_ICON + slot), ML_ART_ICONS, frame, x, y);
}

void NEOGEO_USER ml_icons_hide(void)
{
    uint8_t i;
    for (i = 0; i < ICONS; i++) hide(&s_icon[i]);
}

void NEOGEO_USER ml_person(uint8_t slot, uint8_t look, uint8_t frame, uint8_t flip, int16_t x, int16_t y)
{
    NGSpriteGroup *g;
    if (slot >= 4u) return;
    g = &s_people[slot];
    if (look >= ML_LOOKS || ml_look_art[look].kind != ML_LK_PERSON) { hide(g); return; }
    show_asset(g, (uint16_t)(SPR_PEOPLE + PERSON_STRIPS * slot), ml_look_art[look].art, frame, x, y);
    ng_sprite_group_set_flip(g, flip, 0);
    put(g);
}

/* ======================================================================
 * the room
 * ==================================================================== */
static uint16_t s_water_tile[WATERS];      /* tile index (y * 20 + x) of each overlay */
static uint8_t s_water_on[WATERS];
static uint8_t s_nwater;
static uint8_t s_beam_shown;               /* light overlays after the water ones */
static uint8_t s_lit_seen[NGR_ROOM_BITS];
static uint8_t s_has_light;
static uint8_t s_obj_frame[NGR_MAX_OBJECTS], s_obj_shown[NGR_MAX_OBJECTS];
static int16_t s_obj_x[NGR_MAX_OBJECTS], s_obj_y[NGR_MAX_OBJECTS];
static uint8_t s_obj_anim[NGR_MAX_OBJECTS], s_obj_flip[NGR_MAX_OBJECTS];
static uint8_t s_ring[NGR_MAX_OBJECTS];
static uint8_t s_member_anim[2], s_member_act[2], s_member_pose[2], s_member_hflip[2];
static uint8_t s_front = 0xFFu;            /* which sister is drawn in front (0xFF: none yet) */
static uint8_t s_dlg_speaker = 0xFFu;      /* who is talking: their look is speaker + 1 */
static uint8_t s_dlg_on;                   /* (defined with the dialogue box below) */
static int32_t s_member_lx[2], s_member_ly[2];
static uint8_t s_room;

void NEOGEO_USER ml_bell_rung(uint8_t obj)
{
    if (obj < NGR_MAX_OBJECTS) s_ring[obj] = 24;
}

void NEOGEO_USER ml_member_pose(uint8_t m, uint8_t pose)
{
    static const uint8_t len[4] = { 14, 16, 24, 90 };   /* reach, whip, look, cheer */
    if (m >= 2u || pose >= 4u) return;
    s_member_pose[m] = pose;
    s_member_act[m] = len[pose];
}

void NEOGEO_USER ml_flash_member(uint8_t m)
{
    ml_member_pose(m, ML_POSE_REACH);
}

static uint8_t NEOGEO_USER bg_for(const NGRpg *g)
{
    const MLRoomMeta *meta = &ml_room_meta[g->room];
    uint8_t bg = (uint8_t)g->def->art;
    if ((g->restored >> g->def->region) & 1u) bg = meta->restored_bg;
    return bg < ML_BGS ? bg : 0u;
}

void NEOGEO_USER ml_room_show(NGRpg *g)
{
    uint8_t i;
    uint16_t t, n;
    uint8_t art_id = ml_bg_art[bg_for(g)];
    ml_fade_out();
    clear_screen();
    load_art_palettes(art_id);
    ng_palfx_screen_backdrop(0x8000u);
    show_asset(&s_bg, SPR_BG, art_id, 0, 0, 0);
    s_room = g->room;
    /* water overlays: one per solid channel tile */
    s_nwater = 0;
    if (g->def->channels) {
        n = (uint16_t)(g->def->w * g->def->h);
        for (t = 0; t < n && s_nwater < WATERS; t++) {
            if (!g->def->channels[t] || !(g->def->tiles[t] & NGR_T_SOLID)) continue;
            s_water_tile[s_nwater] = t;
            s_water_on[s_nwater] = 0;
            s_nwater++;
        }
    }
    /* light overlays take the slots the water leaves */
    s_beam_shown = 0;
    s_has_light = 0;
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++)
        if (g->def->objects[i].type == NGR_O_EMITTER) s_has_light = 1;
    for (t = 0; t < NGR_ROOM_BITS; t++) s_lit_seen[t] = 0;
    for (i = 0; i < NGR_MAX_OBJECTS; i++) { s_obj_frame[i] = 0xFFu; s_obj_shown[i] = 0; s_ring[i] = 0; s_obj_anim[i] = 0; }
    s_member_lx[0] = s_member_lx[1] = -1;
    s_front = 0xFFu;
    ml_room_draw(g);
    s_dark = 0;
    ng_palfx_screen_fade_in(NG_PALFX_BLACK, 10u);
    ml_frame();
}

void NEOGEO_USER ml_room_hide(void)
{
    ml_fade_out();
    clear_screen();
}

static void NEOGEO_USER draw_water(const NGRpg *g)
{
    uint8_t i, frame = (uint8_t)((s_frames >> 4) & 1u), anim = (uint8_t)((s_frames & 15u) == 0);
    for (i = 0; i < s_nwater; i++) {
        uint16_t t = s_water_tile[i];
        uint8_t wet = (uint8_t)((g->wet[t >> 3] >> (t & 7u)) & 1u);
        NGSpriteGroup *gr = &s_water[i];
        if (wet && !s_water_on[i]) {
            uint8_t x = 0, y = 0;
            while (t >= g->def->w) { t = (uint16_t)(t - g->def->w); y++; }
            x = (uint8_t)t;
            show_asset(gr, (uint16_t)(SPR_WATER + i), ML_ART_FX_WATER, frame, (int16_t)(x << 4), (int16_t)(y << 4));
            s_water_on[i] = 1;
        } else if (!wet && s_water_on[i]) {
            hide(gr);
            s_water_on[i] = 0;
        } else if (wet && anim) {
            set_frame(gr, ML_ART_FX_WATER, frame);
            put(gr);
        }
    }
}

static uint8_t NEOGEO_USER lit_at(const NGRpg *g, int16_t x, int16_t y)
{
    uint16_t t;
    uint8_t o;
    if (x < 0 || y < 0 || x >= (int16_t)g->def->w || y >= (int16_t)g->def->h) return 0;
    t = (uint16_t)((uint16_t)y * g->def->w + (uint16_t)x);
    if ((g->lit[t >> 3] >> (t & 7u)) & 1u) return 1;
    o = ng_rpg_object_at(g, (uint8_t)x, (uint8_t)y);
    return (uint8_t)(o != NGR_NONE && g->def->objects[o].type == NGR_O_EMITTER);
}

/* Light: one overlay per lit tile, its arms towards the lit tiles beside it
 * (frame bits 1 up, 2 right, 4 down, 8 left), redrawn when the light moves. */
static void NEOGEO_USER draw_light(const NGRpg *g)
{
    uint16_t t, n;
    uint8_t k = 0, changed = 0, x = 0, y = 0;
    if (!s_has_light) return;
    for (t = 0; t < NGR_ROOM_BITS; t++) if (s_lit_seen[t] != g->lit[t]) { s_lit_seen[t] = g->lit[t]; changed = 1; }
    if (!changed) return;
    n = (uint16_t)(g->def->w * g->def->h);
    for (t = 0; t < n; t++) {
        if ((g->lit[t >> 3] >> (t & 7u)) & 1u) {
            uint8_t frame = 0, slot = (uint8_t)(s_nwater + k);
            if (slot >= WATERS) break;
            if (lit_at(g, x, (int16_t)(y - 1))) frame |= 1u;
            if (lit_at(g, (int16_t)(x + 1), y)) frame |= 2u;
            if (lit_at(g, x, (int16_t)(y + 1))) frame |= 4u;
            if (lit_at(g, (int16_t)(x - 1), y)) frame |= 8u;
            if (k < s_beam_shown) {
                set_frame(&s_water[slot], ML_ART_FX_BEAM, frame);
                ng_sprite_group_set_pos(&s_water[slot], (int16_t)(x << 4), (int16_t)(y << 4));
                put(&s_water[slot]);
            } else {
                show_asset(&s_water[slot], (uint16_t)(SPR_WATER + slot), ML_ART_FX_BEAM, frame, (int16_t)(x << 4), (int16_t)(y << 4));
            }
            k++;
        }
        if (++x >= g->def->w) { x = 0; y++; }
    }
    while (s_beam_shown > k) { s_beam_shown--; hide(&s_water[s_nwater + s_beam_shown]); }
    s_beam_shown = k;
}

/* A person's frame: down 0-3, left 4-7 (right is it flipped), up 8-11. */
static uint8_t NEOGEO_USER person_frame(uint8_t facing, uint8_t walk_phase, uint8_t act, uint8_t *flip)
{
    static const uint8_t walk[4] = { 0, 1, 0, 2 };
    uint8_t base;
    *flip = 0;
    switch (facing) {
    case NGR_FACE_RIGHT: base = 4; *flip = 1; break;
    case NGR_FACE_LEFT: base = 4; break;
    case NGR_FACE_UP: base = 8; break;
    default: base = 0; break;
    }
    return (uint8_t)(base + (act ? 3u : walk[walk_phase & 3u]));
}

static void NEOGEO_USER draw_object(const NGRpg *g, uint8_t i)
{
    const NGRpgObjectDef *d = &g->def->objects[i];
    const NGRpgObject *o = &g->obj[i];
    const uint8_t *looks = ml_room_looks[ml_journey][g->room];
    uint8_t look = looks ? looks[i] : 0, kind, artid, frame = 0, visible = 1, k, w, h, flip = 0, base;
    int16_t x, y;
    if (look >= ML_LOOKS) return;
    kind = ml_look_art[look].kind;
    artid = ml_look_art[look].art;
    base = ml_look_art[look].base;
    if (kind == ML_LK_NONE || !artid) return;
    w = d->w ? d->w : 1u; h = d->h ? d->h : 1u;
    x = (int16_t)(o->x << 4); y = (int16_t)(o->y << 4);
    switch (kind) {
    case ML_LK_PERSON:
    case ML_LK_FOLK: {
        /* drawn by the feet: the middle of the picture's width, 3 pixels up from its bottom */
        const NGArtAsset *a = art(artid);
        int16_t px = (int16_t)(o->px >> 8), py = (int16_t)(o->py >> 8);
        int16_t ox = (int16_t)((uint16_t)a->strips << 3), oy = (int16_t)(((uint16_t)a->rows << 4) - 3u);
        uint8_t moving = (uint8_t)(px != s_obj_x[i] || py != s_obj_y[i]);
        if (moving) s_obj_anim[i]++;
        if (kind == ML_LK_FOLK) {
            /* standing, a sway now and then, talking while they speak; facing the way they walk */
            if (s_dlg_on && s_dlg_speaker + 1u == look) frame = (uint8_t)(((s_frames >> 4) & 1u) ? 2u : 0u);
            else if (moving) frame = (uint8_t)((s_obj_anim[i] >> 3) & 1u);
            else frame = (uint8_t)((((uint16_t)(s_frames + (uint16_t)i * 37u)) & 127u) < 20u ? 1u : 0u);
            flip = moving ? (uint8_t)(px < s_obj_x[i]) : s_obj_flip[i];
        } else {
            frame = person_frame(o->facing, (uint8_t)(moving ? (s_obj_anim[i] >> 3) : 0u), 0, &flip);
        }
        visible = o->alive;
        if (!visible) break;
        if (frame != s_obj_frame[i] || px != s_obj_x[i] || py != s_obj_y[i] || flip != s_obj_flip[i] || !s_obj_shown[i]) {
            NGSpriteGroup *gr = &s_obj[i][0];
            if (!s_obj_shown[i]) show_asset(gr, (uint16_t)(SPR_OBJ + 4u * i), artid, frame, (int16_t)(px - ox), (int16_t)(py - oy));
            else { set_frame(gr, artid, frame); ng_sprite_group_set_pos(gr, (int16_t)(px - ox), (int16_t)(py - oy)); }
            ng_sprite_group_set_flip(gr, flip, 0);
            put(gr);
            s_obj_frame[i] = frame; s_obj_x[i] = px; s_obj_y[i] = py; s_obj_flip[i] = flip; s_obj_shown[i] = 1;
        }
        return;
    }
    case 10: /* carried along: a platform */
        x = (int16_t)((o->px >> 8) - 8); y = (int16_t)((o->py >> 8) - 8);
        break;
    case 7: /* creature */
        visible = o->alive;
        x = (int16_t)((o->px >> 8) - 8); y = (int16_t)((o->py >> 8) - 12);
        frame = o->state ? 2u : (uint8_t)((s_frames >> 3) & 1u);
        break;
    case 1: /* glint */
        visible = o->alive;
        frame = (uint8_t)(((s_frames + (uint16_t)i * 23u) & 63u) < 8u);
        break;
    case 2: /* state */
        frame = o->state ? 1u : 0u;
        if (d->type == NGR_O_BLOCK) frame = 0;
        break;
    case 3: /* gear socket */
        frame = o->state ? (uint8_t)(1u + (o->aux ? ((o->aux == 2u ? (uint8_t)(0u - (s_frames >> 2)) : (uint8_t)(s_frames >> 2)) & 3u) : 0u)) : 0u;
        break;
    case 4: /* fixed gear */
        frame = o->aux ? (uint8_t)((o->aux == 2u ? (uint8_t)(0u - (s_frames >> 2)) : (uint8_t)(s_frames >> 2)) & 3u) : 0u;
        break;
    case 5: /* chime */
        frame = (uint8_t)((d->p[0] < 3u ? d->p[0] : 0u) * 2u + (s_ring[i] ? (uint8_t)((s_ring[i] >> 2) & 1u) : 0u));
        if (s_ring[i]) s_ring[i]--;
        break;
    case 6: /* two halves, gone when open */
        visible = (uint8_t)!o->state;
        break;
    default:
        if (d->type == NGR_O_BLOCK) visible = (uint8_t)(o->alive || o->aux);
        break;
    }
    if (d->type == NGR_O_BLOCK) {
        visible = (uint8_t)(o->alive || o->aux);
        frame = 0;
    }
    if (d->type == NGR_O_SECRET) { visible = 1; frame = o->state ? 1u : 0u; }
    if (!visible) {
        if (s_obj_shown[i]) {
            for (k = 0; k < 4u; k++) hide(&s_obj[i][k]);
            s_obj_shown[i] = 0;
        }
        return;
    }
    if (s_obj_shown[i] && frame == s_obj_frame[i] && x == s_obj_x[i] && y == s_obj_y[i]) return;
    for (k = 0; k < 4u && k < (uint8_t)(w * h); k++) {
        uint8_t tx = k, ty = 0, f = frame;
        while (tx >= w) { tx = (uint8_t)(tx - w); ty++; }     /* no divide */
        NGSpriteGroup *gr = &s_obj[i][k];
        if (kind == 6) f = tx ? 1u : 0u;
        f = (uint8_t)(f + base);                  /* its frames in the shared object sheet */
        if (!s_obj_shown[i])
            show_asset(gr, (uint16_t)(SPR_OBJ + 4u * i + k), artid, f, (int16_t)(x + (tx << 4)), (int16_t)(y + (ty << 4)));
        else {
            set_frame(gr, artid, f);
            ng_sprite_group_set_pos(gr, (int16_t)(x + (tx << 4)), (int16_t)(y + (ty << 4)));
            put(gr);
        }
    }
    s_obj_frame[i] = frame; s_obj_x[i] = x; s_obj_y[i] = y; s_obj_shown[i] = 1;
}

/* The sisters' frames (tools/art.py SISTER_FRAMES): the first game's own. */
enum { SF_IDLE = 0, SF_WALK = 3, SF_LOOK = 11, SF_WHIP = 12, SF_CHEER = 16, SF_HURT = 17, SF_REACH = 18 };

static uint8_t NEOGEO_USER sister_frame(uint8_t m, uint8_t moving)
{
    static const uint8_t idle[4] = { 0, 1, 2, 1 };
    uint8_t t = s_member_act[m];
    if (t) {
        switch (s_member_pose[m]) {
        case ML_POSE_WHIP: return (uint8_t)(SF_WHIP + ((uint8_t)(16u - (t > 16u ? 16u : t)) >> 2));
        case ML_POSE_LOOK: return SF_LOOK;
        case ML_POSE_CHEER: return SF_CHEER;
        default: return SF_REACH;
        }
    }
    if (moving) return (uint8_t)(SF_WALK + ((s_member_anim[m] >> 2) & 7u));
    return idle[(s_frames >> 4) & 3u];
}

static void NEOGEO_USER hide_slots(uint16_t first, uint16_t count)
{
#ifdef NG_VRAM_DEFER
    ng_sprite_hide_range_queued(first, count);
#else
    ng_sprite_hide_range(first, count);
#endif
}

static void NEOGEO_USER draw_party(const NGRpg *g)
{
    uint8_t m, front = 0;
    if (g->nparty > 1u && g->party[1].y > g->party[0].y) front = 1;
    for (m = g->nparty; m < 2u; m++)            /* a sister who went home is not left standing here */
        if (s_member[m].visible) hide(&s_member[m]);
    if (front != s_front) {
        /* the nearer sister is drawn over the other: both change slots, drawn whole there */
        for (m = 0; m < 2u; m++) s_member[m].visible = 0;
        if (s_front != 0xFFu) hide_slots(SPR_PARTY, 10u);
        s_front = front;
    }
    for (m = 0; m < g->nparty && m < 2u; m++) {
        const NGRpgMember *mm = &g->party[m];
        NGSpriteGroup *gr = &s_member[m];
        uint16_t slot = (uint16_t)(SPR_PARTY + (m == front ? 5u : 0u));
        uint8_t frame, look = (uint8_t)(mm->id == ML_LUNA ? ML_LOOK_LUNA : ML_LOOK_MAIYA);
        uint8_t moving = (uint8_t)(mm->x != s_member_lx[m] || mm->y != s_member_ly[m]);
        int16_t x = (int16_t)((mm->x >> 8) - 29), y = (int16_t)((mm->y >> 8) - 47);   /* her feet (scaled to NPC height ~42px) */
        if (moving) s_member_anim[m]++;
        else s_member_anim[m] = 0;
        /* the frames face right; going up or down she keeps the way she last faced */
        if (mm->facing == NGR_FACE_RIGHT) s_member_hflip[m] = 0;
        else if (mm->facing == NGR_FACE_LEFT) s_member_hflip[m] = 1;
        frame = sister_frame(m, moving);
        if (s_member_act[m]) s_member_act[m]--;
        s_member_lx[m] = mm->x; s_member_ly[m] = mm->y;
        if (mm->invuln && (s_frames & 8u)) { hide(gr); continue; }   /* hurt: a slow blink, not a flicker */
        if (!gr->visible || gr->firstSprite != slot) {
            show_asset_scaled(gr, slot, ml_look_art[look].art, frame, x, y, 0xCF, 0xC4);
        } else {
            set_frame(gr, ml_look_art[look].art, frame);
            ng_sprite_group_set_pos(gr, x, y);
            ng_sprite_group_set_scale(gr, 0xCF, 0xC4);
            gr->dirty |= NG_SGF_DIRTY_SHRINK;
        }
        ng_sprite_group_set_flip(gr, s_member_hflip[m], 0);
        put(gr);
    }
}

static void NEOGEO_USER draw_fx(void)
{
    uint8_t i;
    for (i = 0; i < FXN; i++) {
        if (!s_fx_t[i]) continue;
        if (!--s_fx_t[i]) { hide(&s_fx[i]); continue; }
        if (!(s_fx_t[i] & 3u)) {
            set_frame(&s_fx[i], ML_ART_FX_SPLASH, (uint8_t)((s_fx_t[i] >> 2) & 1u));
            ng_sprite_group_move(&s_fx[i], 0, -1);
            put(&s_fx[i]);
        }
    }
}

void NEOGEO_USER ml_effect(uint8_t kind, int16_t x, int16_t y)
{
    uint8_t i;
    (void)kind;
    for (i = 0; i < FXN; i++) {
        if (s_fx_t[i]) continue;
        s_fx_t[i] = 20;
        show_asset(&s_fx[i], (uint16_t)(SPR_FX + i), ML_ART_FX_SPLASH, 0, (int16_t)(x - 8), (int16_t)(y - 8));
        return;
    }
}

void NEOGEO_USER ml_room_draw(NGRpg *g)
{
    uint8_t i;
    if (!g->def) return;
    draw_water(g);
    draw_light(g);
    for (i = 0; i < g->def->nobjects && i < NGR_MAX_OBJECTS; i++) draw_object(g, i);
    draw_party(g);
    draw_fx();
}

/* ======================================================================
 * the dialogue box
 * ==================================================================== */
static const char *s_dlg_text;
static uint16_t s_dlg_len, s_dlg_shown;
static uint8_t s_dlg_x, s_dlg_on;

static void NEOGEO_USER dialog_put(uint16_t upto)
{
    /* print the text up to `upto` characters, line by line, rows 22-25 */
    uint8_t row = 22, col = 0;
    uint16_t i;
    char buf[2];
    buf[1] = 0;
    for (i = 0; i < upto && s_dlg_text[i]; i++) {
        char c = s_dlg_text[i];
        if (c == '\n') { row++; col = 0; continue; }
        if (i >= s_dlg_shown) { buf[0] = c; ng_fix_puts((uint8_t)(s_dlg_x + col), row, buf, ML_TXT_WHITE); }
        col++;
    }
    s_dlg_shown = upto;
}

static void NEOGEO_USER dialog_tick(void)
{
    uint16_t next;
    if (!s_dlg_on || s_dlg_shown >= s_dlg_len) return;
    next = (uint16_t)(s_dlg_shown + 2u);
    if (next > s_dlg_len) next = s_dlg_len;
    dialog_put(next);
    if (s_dlg_shown >= s_dlg_len) ng_fix_put_tile(36, 26, (uint16_t)(ML_FIX_HEART + 3u), ML_TXT_GOLD);
}

uint8_t NEOGEO_USER ml_dialog_typing(void)
{
    return (uint8_t)(s_dlg_on && s_dlg_shown < s_dlg_len);
}

void NEOGEO_USER ml_dialog_finish(void)
{
    if (!ml_dialog_typing()) return;
    dialog_put(s_dlg_len);
    ng_fix_put_tile(36, 26, (uint16_t)(ML_FIX_HEART + 3u), ML_TXT_GOLD);
}

void NEOGEO_USER ml_dialog(uint8_t speaker, const char *text, uint8_t typing)
{
    uint8_t face = speaker < ML_SPEAKERS ? ml_face_art[speaker] : 0u;
    uint16_t n = 0;
    s_dlg_speaker = speaker;
    if (!s_dialog.visible) {
        ng_sprite_group_set_priority(&s_dialog, NG_SG_PRIO_HIGH);
        show_asset(&s_dialog, SPR_DIALOG, ML_ART_DIALOG, 0, 0, 160);
    }
    if (face) {
        load_art_palettes(face);             /* every portrait shares one pair of banks */
        ng_sprite_group_set_priority(&s_face, NG_SG_PRIO_HIGH);
        show_asset(&s_face, SPR_FACE, face, 0, 24, 160);
    }
    else hide(&s_face);
    ml_clear_text(21, 27);
    s_dlg_x = (uint8_t)(face ? 12u : 3u);
    if (speaker < ML_SPEAKERS && ml_text[ml_speakers[speaker].name][0])
        ml_puts(s_dlg_x, 21, ml_text[ml_speakers[speaker].name], ML_TXT_GOLD);
    s_dlg_text = text;
    while (text[n]) n++;
    s_dlg_len = n;
    s_dlg_shown = 0;
    s_dlg_on = 1;
    if (!typing) ml_dialog_finish();
}

/* Side by side on row 26 when they fit, else one a row ending on row 26 (the
 * world tool leaves the question's last page room for that). */
void NEOGEO_USER ml_dialog_choices(const NGRpgDialogueNode *nd, uint8_t offered, uint8_t cursor)
{
    const char *t[3];
    uint8_t len[3], i, n = 0, w = 0, x = s_dlg_x, y, stacked;
    for (i = 0; i < nd->nchoices && i < 3u; i++) {
        if (!((offered >> i) & 1u)) continue;
        t[n] = ml_text[nd->choice_text[i]];
        for (len[n] = 0; t[n][len[n]]; len[n]++) {}
        w = (uint8_t)(w + len[n] + (n ? 2u : 1u));
        n++;
    }
    stacked = (uint8_t)(s_dlg_x + w > 37u);
    y = (uint8_t)(stacked ? 27u - n : 26u);
    ml_clear_text(y, 26);
    for (i = 0; i < n; i++) {
        if (i == cursor) ng_fix_put_tile(x, y, (uint16_t)(ML_FIX_HEART + 3u), ML_TXT_GOLD);
        ml_puts((uint8_t)(x + 1u), y, t[i], i == cursor ? ML_TXT_GOLD : ML_TXT_GREY);
        if (stacked) y++;
        else x = (uint8_t)(x + len[i] + 2u);
    }
}

void NEOGEO_USER ml_dialog_hide(void)
{
    s_dlg_on = 0;
    s_dlg_speaker = 0xFFu;
    hide(&s_dialog);
    hide(&s_face);
    ml_clear_text(20, 27);
}

/* ======================================================================
 * the status line and toasts
 * ==================================================================== */
static uint16_t s_toast_t;
static uint8_t s_toast_rows;

void NEOGEO_USER ml_hud(NGRpg *g, uint8_t tool, uint8_t full)
{
    static uint8_t shown_hp = 0xFFu, shown_lead = 0xFFu, shown_tool = 0xFFu, shown_found = 0xFFu;
    const NGRpgMember *m = &g->party[g->leader];
    uint8_t i, found = 0;
    if (full) shown_hp = shown_lead = shown_tool = shown_found = 0xFFu;
    if (m->id != shown_lead) {
        ml_puts(3, 1, m->id == ML_LUNA ? "LUNA " : "MAIYA", m->id == ML_LUNA ? ML_TXT_SKY : ML_TXT_MINT);
        shown_lead = m->id;
    }
    if (m->hp != shown_hp) {
        for (i = 0; i < (uint8_t)((m->max_hp + 1u) >> 1); i++) {
            uint8_t left = (uint8_t)(m->hp > (uint8_t)(2u * i) ? m->hp - 2u * i : 0u);
            ng_fix_put_tile((uint8_t)(9u + i), 1, (uint16_t)(ML_FIX_HEART + (left >= 2u ? 0u : left == 1u ? 1u : 2u)), ML_TXT_HEART);
        }
        shown_hp = m->hp;
    }
    if (tool != shown_tool) {
        ml_puts(20, 1, "C:               ", ML_TXT_GREY);        /* tool titles: 15 at most (worldlib) */
        if (tool < ML_ITEMS) ml_puts(22, 1, ml_text[ml_item_text[ml_journey][tool].title], ML_TXT_WHITE);
        shown_tool = tool;
    }
    /* the collected bits a byte at a time (175 calls a frame cost a fifth of it) */
    for (i = 0; i < (uint8_t)((ML_COLLECT + 7u) >> 3); i++) {
        static const uint8_t bits4[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
        uint8_t b = g->collected[i];
        if (i == (uint8_t)(ML_COLLECT >> 3)) b = (uint8_t)(b & ((1u << (ML_COLLECT & 7u)) - 1u));
        found = (uint8_t)(found + bits4[b & 15u] + bits4[b >> 4]);
    }
    if (found != shown_found) {
        ng_fix_put_tile(15, 1, (uint16_t)(ML_FIX_HEART + 4u), ML_TXT_GOLD);
        ml_putn(16, 1, found, 3, ML_TXT_GOLD);
        shown_found = found;
    }
}

/* Row 3, and row 4 for what doesn't fit: 34 columns (3-36), 28 beside an icon. */
void NEOGEO_USER ml_toast(const char *a, const char *b, uint8_t icon)
{
    char line[72];
    uint8_t i = 0, k = 0, cut, room = (uint8_t)(icon != 0xFFu ? 28u : 34u);
    while (a && a[k] && i < 68u) line[i++] = a[k++];
    k = 0;
    if (b && i < 67u) line[i++] = ' ';
    while (b && b[k] && b[k] != '\n' && i < 68u) line[i++] = b[k++];
    line[i] = 0;
    cut = i;
    if (i > room) {                                  /* break at the last space that fits */
        for (cut = room; cut && line[cut] != ' '; cut--) {}
        if (!cut) cut = room;
    }
    ml_clear_text(3, (uint8_t)(s_toast_rows > 1u ? 4u : 3u));
    s_toast_rows = 1;
    if (cut < i) {
        uint8_t rest = (uint8_t)(cut + (line[cut] == ' ' ? 1u : 0u));
        if (i - rest > 34u) line[rest + 34u] = 0;
        line[cut] = 0;
        ml_puts_c(4, line + rest, ML_TXT_GOLD);
        s_toast_rows = 2;
        i = cut;
    }
    ml_puts_c(3, line, ML_TXT_GOLD);
    if (icon != 0xFFu) ml_icon(0, icon, (int16_t)(((40 - i) >> 1) * 8 - 20), 20);
    s_toast_t = 150;
}

void NEOGEO_USER ml_toast_tick(void)
{
    if (!s_toast_t) return;
    if (!--s_toast_t) { ml_clear_text(3, (uint8_t)(s_toast_rows > 1u ? 4u : 3u)); s_toast_rows = 0; ml_icon(0, 0xFFu, 0, 0); }
}

/* ======================================================================
 * sound
 * ==================================================================== */
static uint8_t s_tune = 0xFFu;

uint8_t NEOGEO_USER ml_music_playing(void) { return s_tune; }

void NEOGEO_USER ml_music(uint8_t track)
{
    s_tune = track;
    if (track >= 4u) track++;           /* ADPCM-B track 5 (5.wav) belongs to the eyecatcher */
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

void NEOGEO_USER ml_sfx(uint8_t sfx)
{
    if (sfx < ML_SFX_COUNT) playSFX(sfx);
}
