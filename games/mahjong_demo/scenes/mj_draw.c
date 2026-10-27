/*
 * mj_draw.c -- Mahjong Demo presentation.
 *
 * The frame pump and input (player 1 only, with held directions
 * repeating), palettes and fades, FIX text and the two-cell mini tiles,
 * the felt and the logo, the sprite tiles and the sounds.
 *
 * A sprite tile is one strip of two 16-pixel rows (a 16 x 32 frame of the
 * tile sheet). Its look is its palette: the sheet's own, or one derived
 * from it at start-up -- darker for a blocked tile, warm for the selected
 * one, cool for a hint -- so a tile's state reads by brightness and by
 * colour, not by colour alone.
 *
 * Sprites (hardware slots, lower behind): 1-20 the felt, 21-36 the logo,
 * 41-200 the tiles.
 */
#include "mj.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "sdk/2d_engine/ng_fix.h"
#include "sdk/2d_engine/ng_art_asset.h"
#include "games/mahjong_demo/artbox/generated/mj_assets.h"
#include "games/mahjong_demo/artbox/generated/mj_mini.h"

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
#define SPR_LOGO    21u
#define SPR_TILE    41u
#define PAL_BANKS   32u
#define LOOK_BANK   24u     /* the derived tile palettes: blocked, selected, hint */

/* ======================================================================
 * frame pump, input, palettes
 * ==================================================================== */
static uint16_t s_pal_base[PAL_BANKS * 16u];
static uint16_t s_pal_out[PAL_BANKS * 16u];
static volatile uint8_t s_publish;
static uint16_t s_joy, s_joy_prev, s_pressed, s_repeat, s_frames;
static uint8_t s_hold_t;

void NEOGEO_USER mj_vblank(void)
{
    if (!s_publish) return;
    s_publish = 0;
    ng_palfx_vblank();
}

static const NGArtAsset *NEOGEO_USER art(uint8_t id)
{
    return &mj_art[id - 1u];
}

static void NEOGEO_USER load_art_palettes(uint8_t id)
{
    const NGArtAsset *a = art(id);
    uint8_t i;
    for (i = 0; i < mj_art_banks[id - 1u]; i++)
        ng_palfx_screen_load((uint8_t)(a->palette_bank + i), mj_art_pal[id - 1u] + 16u * i);
}

static void NEOGEO_USER fix_pal(uint8_t bank, uint16_t ink, uint16_t edge)
{
    uint16_t p[16];
    uint8_t i;
    for (i = 0; i < 16u; i++) p[i] = 0;
    p[1] = ink; p[2] = edge; p[3] = edge;
    ng_palfx_screen_load(bank, p);
}

void NEOGEO_USER mj_fix_palettes(void)
{
    fix_pal(MJ_TXT_WHITE, 0x7FFFu, 0x8000u);
    fix_pal(MJ_TXT_GOLD, 0x6FC3u, 0x8000u);
    fix_pal(MJ_TXT_RED, 0x4F44u, 0x8000u);
    fix_pal(MJ_TXT_SKY, 0x16DFu, 0x8000u);
    fix_pal(MJ_TXT_MINT, 0x07FAu, 0x8000u);
    fix_pal(MJ_TXT_GREY, 0x3999u, 0x8000u);
    ng_palfx_screen_load(MJ_MINI_PAL, mj_mini_pal);
}

/* A Neo Geo colour word's 5-bit channels, and back (the dark bit dropped). */
static uint16_t NEOGEO_USER chan(uint16_t w, uint8_t c)
{
    if (c == 0) return (uint16_t)(((w >> 7) & 0x1Eu) | ((w >> 14) & 1u));
    if (c == 1) return (uint16_t)(((w >> 3) & 0x1Eu) | ((w >> 13) & 1u));
    return (uint16_t)(((w << 1) & 0x1Eu) | ((w >> 12) & 1u));
}

static uint16_t NEOGEO_USER word(uint16_t r, uint16_t g, uint16_t b)
{
    return (uint16_t)(((r & 1u) << 14) | ((g & 1u) << 13) | ((b & 1u) << 12) |
                      ((r >> 1) << 8) | ((g >> 1) << 4) | (b >> 1));
}

/* look 1 blocked: five eighths as bright; 2 selected: halfway to warm
 * yellow; 3 hint: halfway to cyan. Index 0 stays transparent. */
static void NEOGEO_USER derive_looks(void)
{
    static const uint8_t tint[2][3] = { { 31, 27, 6 }, { 8, 27, 31 } };
    const uint16_t *src = mj_art_pal[MJ_ART_TILES - 1u];
    uint16_t p[16];
    uint8_t look, i, c;
    for (look = MJ_LOOK_BLOCKED; look < MJ_LOOKS; look++) {
        p[0] = 0;
        for (i = 1; i < 16u; i++) {
            uint16_t v[3];
            for (c = 0; c < 3u; c++) {
                uint16_t x = chan(src[i], c);
                if (look == MJ_LOOK_BLOCKED) x = (uint16_t)(x - (x >> 2) - (x >> 3));
                else {
                    uint16_t t = tint[look - MJ_LOOK_SELECTED][c];
                    x = (uint16_t)(x > t ? x - ((x - t) >> 1) : x + ((t - x) >> 1));
                }
                v[c] = x;
            }
            p[i] = word(v[0], v[1], v[2]);
        }
        ng_palfx_screen_load((uint8_t)(LOOK_BANK + look - 1u), p);
    }
}

void NEOGEO_USER mj_presentation_init(void)
{
    ng_palfx_screen_init(s_pal_base, s_pal_out, PAL_BANKS);
    ng_fix_init();
    ng_fix_set_ascii_base(0xD00u);
    load_art_palettes(MJ_ART_FELT);
    load_art_palettes(MJ_ART_TILES);
    load_art_palettes(MJ_ART_LOGO);
    derive_looks();
}

uint16_t NEOGEO_USER mj_frame(void)
{
    s_publish = 1;
    do { waitVbl(); } while (s_publish);
    if (!ng_vram_window_open()) waitVbl();
    ng_vram_commit();
    ng_palette_fx_update();
    s_joy_prev = s_joy;
    s_joy = (uint16_t)(poll_joystick() & 0x01FFu);    /* player 1 and its start */
    s_pressed = (uint16_t)(s_joy & (uint16_t)~s_joy_prev);
    s_repeat = s_pressed;
    if (s_joy & 0x000Fu) {
        if (++s_hold_t >= 18u) { s_hold_t = 13u; s_repeat |= (uint16_t)(s_joy & 0x000Fu); }
    } else s_hold_t = 0;
    s_frames++;
    return s_frames;
}

uint16_t NEOGEO_USER mj_held(void) { return s_joy; }
uint16_t NEOGEO_USER mj_pressed(void) { return s_pressed; }
uint16_t NEOGEO_USER mj_repeat(void) { return s_repeat; }

uint8_t NEOGEO_USER mj_wait_button(uint16_t frames)
{
    uint16_t t;
    for (t = 0; t < frames; t++) {
        mj_frame();
        if (t > 12u && (mj_pressed() & (MJ_BUTTONS | MJ_START))) return 1;
        if (mj_quit_requested()) return 0;
    }
    return 0;
}

/* ======================================================================
 * FIX text and mini tiles
 * ==================================================================== */
void NEOGEO_USER mj_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    char line[41];
    uint8_t i;
    if (!text || x >= 40u || y >= 32u) return;
    for (i = 0; i < (uint8_t)(40u - x) && text[i]; i++) line[i] = text[i];
    line[i] = 0;
    ng_fix_puts(x, y, line, pal);
}

void NEOGEO_USER mj_puts_c(uint8_t y, const char *text, uint8_t pal)
{
    uint8_t n = 0;
    while (text[n] && n < 40u) n++;
    mj_puts((uint8_t)((40u - n) >> 1), y, text, pal);
}

/* Right-aligned in `digits` cells, with a minus sign for a negative value. */
void NEOGEO_USER mj_putn(uint8_t x, uint8_t y, int32_t value, uint8_t digits, uint8_t pal)
{
    static const uint32_t pow10[10] = { 1000000000u, 100000000u, 10000000u, 1000000u, 100000u,
                                        10000u, 1000u, 100u, 10u, 1u };
    char buf[12];
    uint32_t v = (uint32_t)(value < 0 ? -value : value);
    uint8_t i, o = 0, lead = 1;
    if (digits > 10u) digits = 10u;
    for (i = (uint8_t)(10u - digits); i < 10u; i++) {
        char d = '0';
        while (v >= pow10[i]) { v -= pow10[i]; d++; }
        if (d != '0' || i == 9u) {
            if (lead && value < 0 && o) buf[o - 1u] = '-';
            lead = 0;
        }
        buf[o++] = lead ? ' ' : d;
    }
    buf[o] = 0;
    mj_puts(x, y, buf, pal);
}

/* A whole row: the text centred, blanks around it (only changed cells are written). */
void NEOGEO_USER mj_line_c(uint8_t y, const char *text, uint8_t pal)
{
    char line[41];
    uint8_t n = 0, i, x;
    while (text && text[n] && n < 40u) n++;
    x = (uint8_t)((40u - n) >> 1);
    for (i = 0; i < 40u; i++) line[i] = (i >= x && i < (uint8_t)(x + n)) ? text[i - x] : ' ';
    line[40] = 0;
    ng_fix_puts(0, y, line, pal);
}

/* Blanks from column x to the end of row y. */
void NEOGEO_USER mj_clear_from(uint8_t x, uint8_t y)
{
    if (x < 40u) ng_fix_clear_rect(x, y, (uint8_t)(40u - x), 1, 0);
}

void NEOGEO_USER mj_clear_text(uint8_t y0, uint8_t y1)
{
    uint8_t y;
    for (y = y0; y <= y1 && y < 32u; y++) ng_fix_clear_rect(0, y, 40, 1, 0);
}

void NEOGEO_USER mj_mini(uint8_t x, uint8_t y, uint8_t kind)
{
    uint16_t l, r;
    if (kind == NGM_NO_TILE) {
        ng_fix_put_tile(x, y, 0x0FFu, 0);
        ng_fix_put_tile((uint8_t)(x + 1u), y, 0x0FFu, 0);
        return;
    }
    if (kind < 27u) {
        static const uint8_t icon[3] = { MJ_MINI_SEAL, MJ_MINI_DOT, MJ_MINI_STICK };
        uint8_t suit = (uint8_t)(kind < 9u ? 0u : kind < 18u ? 1u : 2u);
        l = (uint16_t)(kind - 9u * suit);
        r = icon[suit];
    } else if (kind < 31u) {
        l = (uint16_t)(MJ_MINI_LETTER + kind - 27u);
        r = MJ_MINI_WIND;                                         /* a plain cell */
    } else if (kind < 34u) {
        l = MJ_MINI_BLANK;
        r = (uint16_t)(MJ_MINI_RED + kind - 31u);
    } else {
        l = (uint16_t)((kind - 34u) & 3u);
        r = kind < 38u ? MJ_MINI_FLOWER : MJ_MINI_SEASON;
    }
    ng_fix_put_tile(x, y, (uint16_t)(MJ_FIX_MINI + l), MJ_MINI_PAL);
    ng_fix_put_tile((uint8_t)(x + 1u), y, (uint16_t)(MJ_FIX_MINI + r), MJ_MINI_PAL);
}

void NEOGEO_USER mj_mini_back(uint8_t x, uint8_t y)
{
    ng_fix_put_tile(x, y, (uint16_t)(MJ_FIX_MINI + MJ_MINI_BACK), MJ_MINI_PAL);
    ng_fix_put_tile((uint8_t)(x + 1u), y, (uint16_t)(MJ_FIX_MINI + MJ_MINI_BACK + 1u), MJ_MINI_PAL);
}

const char *NEOGEO_USER mj_tile_name(uint8_t kind)
{
    static const char *const suit[3] = { " CHARACTERS", " DOTS", " BAMBOO" };
    static const char *const honour[7] = { "EAST WIND", "SOUTH WIND", "WEST WIND", "NORTH WIND",
                                           "RED DRAGON", "GREEN DRAGON", "WHITE DRAGON" };
    static char buf[16];
    uint8_t i = 0;
    const char *s;
    if (kind >= NGM_KINDS_ALL) return "";
    if (kind >= 27u && kind < 34u) return honour[kind - 27u];
    if (kind < 27u) {
        uint8_t n = (uint8_t)(kind < 9u ? 0u : kind < 18u ? 1u : 2u);
        buf[i++] = (char)('1' + kind - 9u * n);
        s = suit[n];
    }
    else { s = kind < 38u ? "FLOWER " : "SEASON "; }
    while (*s) buf[i++] = *s++;
    if (kind >= 34u) buf[i++] = (char)('1' + ((kind - 34u) & 3u));
    buf[i] = 0;
    return buf;
}

/* ======================================================================
 * screens and sprites
 * ==================================================================== */
static NGSpriteGroup s_bg, s_logo, s_tile[MJ_SPRITE_TILES];
static uint8_t s_kind[MJ_SPRITE_TILES], s_look[MJ_SPRITE_TILES];
static int16_t s_x[MJ_SPRITE_TILES], s_y[MJ_SPRITE_TILES];
static uint8_t s_dark;      /* the screen is faded out: set it up at once */

static void NEOGEO_USER put(NGSpriteGroup *g)
{
    if (s_dark) ng_sprite_group_upload(g);
    else ng_sprite_group_flush(g);
}

static void NEOGEO_USER show_asset(NGSpriteGroup *g, uint16_t first_sprite, uint8_t id, int16_t x, int16_t y)
{
    const NGArtAsset *a = art(id);
    ng_sprite_group_init(g, first_sprite, a->strips, a->rows, a->tile_base, a->palette_bank);
    ng_sprite_group_set_palette_map(g, a->tile_palettes);
    ng_sprite_group_set_pos(g, x, y);
    ng_sprite_group_set_visible(g, 1);
    ng_sprite_group_mark_dirty(g, NG_SGF_DIRTY_ALL);
    put(g);
}

void NEOGEO_USER mj_fade_out(void)
{
    ng_palfx_screen_fade_out(NG_PALFX_BLACK, 10u);
    while (ng_palfx_screen_fading()) mj_frame();
    mj_frame();
}

void NEOGEO_USER mj_screen(uint8_t logo)
{
    uint8_t i;
    mj_fade_out();
    waitVbl();
    clearFix();
    ng_sprite_hide_all();
    ng_fix_invalidate_all();
    s_bg.visible = s_logo.visible = 0;
    for (i = 0; i < MJ_SPRITE_TILES; i++) { s_tile[i].visible = 0; s_kind[i] = 0xFFu; }
    s_dark = 1;
    ng_palfx_screen_backdrop(0x8000u);
    show_asset(&s_bg, SPR_BG, MJ_ART_FELT, 0, 0);
    if (logo) show_asset(&s_logo, SPR_LOGO, MJ_ART_LOGO, 32, 20);
}

void NEOGEO_USER mj_reveal(void)
{
    mj_frame();
    s_dark = 0;
    ng_palfx_screen_fade_in(NG_PALFX_BLACK, 12u);
    mj_frame();
}

void NEOGEO_USER mj_tile(uint8_t slot, uint8_t kind, int16_t x, int16_t y, uint8_t look)
{
    NGSpriteGroup *g;
    const NGArtAsset *a = art(MJ_ART_TILES);
    uint8_t bank;
    if (slot >= MJ_SPRITE_TILES) return;
    g = &s_tile[slot];
    if (g->visible && s_kind[slot] == kind && s_look[slot] == look && s_x[slot] == x && s_y[slot] == y) return;
    bank = (uint8_t)(look == MJ_LOOK_NORMAL ? a->palette_bank : LOOK_BANK + look - 1u);
    if (!g->visible || s_kind[slot] != kind || s_look[slot] != look) {
        ng_sprite_group_init(g, (uint16_t)(SPR_TILE + slot), 1u, 2u, (uint16_t)(a->tile_base + 2u * kind), bank);
        ng_sprite_group_set_pos(g, x, y);
        ng_sprite_group_set_visible(g, 1);
        ng_sprite_group_mark_dirty(g, NG_SGF_DIRTY_ALL);
    } else {
        ng_sprite_group_set_pos(g, x, y);
    }
    put(g);
    s_kind[slot] = kind; s_look[slot] = look; s_x[slot] = x; s_y[slot] = y;
}

void NEOGEO_USER mj_tile_hide(uint8_t slot)
{
    NGSpriteGroup *g;
    if (slot >= MJ_SPRITE_TILES) return;
    g = &s_tile[slot];
    if (!g->visible) return;
    ng_sprite_group_set_visible(g, 0);
    put(g);
    s_kind[slot] = 0xFFu;
}

void NEOGEO_USER mj_tiles_hide(uint8_t from, uint8_t to)
{
    while (from < to && from < MJ_SPRITE_TILES) mj_tile_hide(from++);
}

/* ======================================================================
 * sound
 * ==================================================================== */
void NEOGEO_USER mj_music(uint8_t track)
{
    if (track >= 4u) track++;           /* ADPCM-B track 5 (5.wav) belongs to the eye-catcher */
    soundStopAll();              waitVbl();
    soundCancelFade();           waitVbl();
    soundSetADPCMAVolume(0x3Cu); waitVbl();
    soundSetADPCMBVolume(0xA0u); waitVbl();
    soundSetSSGVolume(0x00u);    waitVbl();
    soundSetFMVolume(0x00u);     waitVbl();
    soundSetADPCMBLoop(1u);      waitVbl();
    playSFXB(track);
    waitVbl();
}

void NEOGEO_USER mj_sfx(uint8_t sfx)
{
    playSFX(sfx);
}
