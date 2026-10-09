/*
 * sky_draw.c — Sky Lance presentation layer.
 *
 * Everything that talks to the hardware or to the artbox tables lives
 * here: asset binding, the two-page scrolling backdrop, the FIX HUD and
 * the per-frame pump.  sky_stage.c and sky_sortie.c above it only deal
 * in game state.
 *
 * https://eaglesoftware.biz
 */

#include "sky_draw.h"
#include "sky_stage.h"
#include "sdk/neogeo.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include "sdk/2d_engine/ng_sprite_window.h"
#include "sdk/2d_engine/ng_art_asset.h"
#include "sdk/2d_engine/ng_sprite_hw.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "sdk/2d_engine/ng_fix.h"
#include "sdk/2d_engine/ng_shooter.h"
#include "sprite_meta.h"
#include "infix_palettes.h"

const uint16_t * NEOGEO_USER ng_get_screen_palette(uint16_t screen_id);
static uint8_t s_palette_loaded[256];

void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER clearSprs(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);
uint16_t NEOGEO_USER poll_joystick(void);
void NEOGEO_USER mess_out_clipped(uint16_t x, uint16_t y, const char *text,
                                  short pal, uint16_t max_chars);
void NEOGEO_USER ngfix_write_tile(uint8_t x, uint8_t y, uint16_t tile, uint8_t pal);
uint8_t NEOGEO_USER ng_load_screen_palette(uint16_t screen_id);
const NGArtAsset * NEOGEO_USER ng_screen_art_asset(uint16_t screen_id);
void NEOGEO_USER load_palettes(uint16_t *pal, uintptr_t dest);

/* ------------------------------------------------------------------ */
/*  Asset metrics                                                       */
/* ------------------------------------------------------------------ */
/*
 * Asset ids are 1-based everywhere in this SDK - showScreenN(),
 * ng_load_screen_palette() and the SKY_* constants all count from 1 -
 * while g_ng_asset_meta[] is a plain 0-based array.  This is the single
 * place the two conventions meet.
 */
static const NGSpriteAssetMeta * NEOGEO_USER sky_meta(uint8_t id)
{
    if (id == 0u || id > NG_ASSET_META_COUNT) return 0;
    return &g_ng_asset_meta[id - 1u];
}

int16_t NEOGEO_USER sky_width(uint8_t id, uint8_t scale)
{
    const NGSpriteAssetMeta *m = sky_meta(id);
    if (!m) return 0;
    return (int16_t)ng_sprite_scaled_x(m->content_width, scale);
}

int16_t NEOGEO_USER sky_height(uint8_t id, uint8_t scale)
{
    const NGSpriteAssetMeta *m = sky_meta(id);
    if (!m) return 0;
    return (int16_t)ng_sprite_scaled_y(m->content_height, scale);
}

void NEOGEO_USER sky_bind(NGCharacter *c, uint8_t id, uint8_t scale, uint8_t band)
{
    const NGSpriteAssetMeta *m = sky_meta(id);
    const NGArtAsset *art = ng_screen_art_asset(id);
    uint8_t  strips;
    uint8_t  rows;
    uint16_t tile;
    int16_t  half_w;
    int16_t  half_h;

    if (!c || !m) return;

    strips = m->strips ? m->strips : 1u;
    rows   = m->active_rows ? m->active_rows : 1u;
    if (strips > NG_SPRITE_MAX_STRIPS) strips = NG_SPRITE_MAX_STRIPS;
    if (rows > NG_SPRITE_MAX_HEIGHT_TILES) rows = NG_SPRITE_MAX_HEIGHT_TILES;

    /* First tile of the artwork inside its page.  The stride is the
     * canvas width in tiles, which is 16 only for a full-width import -
     * the playfield craft are imported far narrower than that. */
    tile = (uint16_t)(m->tile_base
                      + (uint16_t)m->tile_row_start * m->tile_stride
                      + (uint16_t)m->tile_col_start);

    ng_char_set_sprite(c, 0u, strips, rows, tile, m->palette_bank);
    ng_char_set_tile_stride(c, m->tile_stride);
    ng_char_set_palette_map(c, art ? art->tile_palettes : 0);
    c->scale_x = scale;
    c->scale_y = scale;

    /*
     * (x_pad + content_width/2) is the distance from the artwork's first
     * TILE to the centre of the painted pixels.  It lives inside the
     * sprite, so it follows the hardware's separate X and Y shrink ratios.
     */
    half_w = (int16_t)ng_sprite_scaled_x((uint16_t)(m->x_pad + (m->content_width >> 1)), scale);
    half_h = (int16_t)ng_sprite_scaled_y((uint16_t)(m->y_pad + (m->content_height >> 1)), scale);
    c->sprite_offset_x = (int16_t)(-half_w);
    c->sprite_offset_y = (int16_t)(-half_h);

    /*
     * Collision body: half the painted size, centred.  A shmup wants a
     * hitbox noticeably smaller than the art - the player in particular
     * should survive a wingtip graze - so this is deliberately generous
     * to the player rather than pixel-accurate.
     */
    {
        int16_t w = (int16_t)ng_sprite_scaled_x(m->content_width, scale);
        int16_t h = (int16_t)ng_sprite_scaled_y(m->content_height, scale);
        ng_char_set_body(c, (int16_t)(-(w >> 2)), (int16_t)(-(h >> 2)),
                         (int16_t)(w >> 1), (int16_t)(h >> 1));
    }

    ng_char_set_priority(c, band, 0);
    if (!s_palette_loaded[id] || id == SKY_P1_PLANE || id == SKY_P2_PLANE ||
        id == SKY_P3_PLANE || id >= 35u) {
        ng_load_screen_palette(id);
        s_palette_loaded[id] = 1u;
    }
}

NGCharacter * NEOGEO_USER sky_spawn(uint8_t kind, uint8_t id,
                                    int16_t cx, int16_t cy,
                                    uint8_t scale, uint8_t band)
{
    NGCharacter *c = chars_add(kind, cx, cy);
    if (!c) return 0;
    sky_bind(c, id, scale, band);
    c->visible = 1u;
    c->life_state = NG_CHAR_LIFE_VISIBLE;
    return c;
}

/* ------------------------------------------------------------------ */
/*  Scrolling background                                                */
/* ------------------------------------------------------------------ */
/* Terrain occupies 1..44, clouds 45..88; craft start at slot 96. */
static uint8_t s_bg_id;
static uint8_t s_bg_advance;
static NGVerticalLayer s_ground, s_clouds;
static NGShooterCamera s_camera;
static int16_t s_follow_x = 160, s_follow_y = 112;
static uint8_t s_bg_ready;
static uint16_t s_palette_base[104u * 16u];
static uint16_t s_palette_out[104u * 16u];

void NEOGEO_USER sky_bg_select(uint8_t id)
{
    const NGArtAsset *art = ng_screen_art_asset(id);
    const NGArtAsset *cloud = ng_screen_art_asset(SKY_CLOUD_ART);
    if (s_bg_id == id && s_bg_ready) return;
    if (!art || art->strips != 22u || art->active_rows != 32u) return;
    s_bg_id = id;
    s_bg_advance = 0u;
    ng_load_screen_palette(id);
    ng_shooter_camera_init(&s_camera);
    ng_shooter_camera_set_follow(&s_camera, 8u, 6u, 16u, 4u);
    s_follow_x = 160;
    s_follow_y = 112;
    s_bg_ready = ng_vertical_layer_init(&s_ground, 1u, 22u, 16u,
                                       art->tile_base, art->palette_bank, art->tile_palettes, -16);
    if (cloud) {
        ng_load_screen_palette(SKY_CLOUD_ART);
        ng_vertical_layer_init(&s_clouds, 45u, 22u, 16u, cloud->tile_base,
                               cloud->palette_bank, cloud->tile_palettes, -16);
    }
}

void NEOGEO_USER sky_bg_follow(int16_t x, int16_t y)
{
    s_follow_x = x;
    s_follow_y = y;
}

void NEOGEO_USER sky_bg_advance(uint8_t pixels)
{
    s_bg_advance = pixels;
}

void NEOGEO_USER sky_bg_draw(void)
{
    if (!s_bg_ready) return;
    ng_shooter_camera_set_speed(&s_camera, (int16_t)((uint16_t)s_bg_advance * 256u));
    ng_shooter_camera_step(&s_camera, s_follow_x, s_follow_y);
    ng_vertical_layer_draw(&s_ground, &s_camera, 256u);
    ng_vertical_layer_draw(&s_clouds, &s_camera, 384u);
    s_bg_advance = 0u;
}

void NEOGEO_USER sky_bg_hide(void)
{
    ng_vertical_layer_hide(&s_ground);
    ng_vertical_layer_hide(&s_clouds);
    /* The title borrows the first page without enabling a scrolling layer. */
    ng_sprite_group_cancel(&s_ground.pages[0]);
    ng_sprite_hide_vram_base(NG_SPR_VRAM_BASE(NG_SPR_BG0_FIRST), 20u);
    s_bg_id = 0u;
    s_bg_ready = 0u;
}

void NEOGEO_USER sky_title_draw(void)
{
    const NGArtAsset *art = ng_screen_art_asset(SKY_TITLE_ART);
    NGSpriteGroup *g = &s_ground.pages[0];
    sky_bg_hide();
    ng_load_screen_palette(SKY_TITLE_ART);
    ng_sprite_group_init(g, 1u, 20u, 14u, art->tile_base, art->palette_bank);
    ng_sprite_group_set_tile_stride(g, 20u);
    ng_sprite_group_set_palette_map(g, art->tile_palettes);
    ng_sprite_group_set_pos(g, 0, 0);
    ng_sprite_group_upload(g);
}

void NEOGEO_USER sky_presentation_init(void)
{
    ng_palfx_screen_init(s_palette_base, s_palette_out, 104u);
    ng_fix_init();
    ng_fix_set_ascii_base(0xD00u);
}

void NEOGEO_USER sky_fade_out(void)
{
    ng_palfx_screen_fade_out(16u, 8u);
    while (ng_palfx_screen_fading()) sky_frame();
    sky_frame();
}

/* ------------------------------------------------------------------ */
/*  Frame pump                                                          */
/* ------------------------------------------------------------------ */
static uint16_t s_joy      = 0u;
static uint16_t s_joy_prev = 0u;
static uint16_t s_pressed  = 0u;
static uint16_t s_frames   = 0u;
static uint16_t s_rng      = 0x1234u;

void NEOGEO_USER sky_scene_begin(void)
{
    uint16_t i;
    sky_fade_out();
    waitVbl();
    setBACKDROP(SKY_BG_CLEAR);
    clearFix();
    clearSprs();
    ng_sprite_hide_all();
    ng_chars_init();
    sky_bg_hide();
    for (i = 0u; i < 256u; i++) s_palette_loaded[i] = 0u;
    s_joy = 0u;
    s_joy_prev = 0u;
    s_pressed = 0u;
    ng_fix_invalidate_all();
    ng_palfx_screen_fade_in(0u, 12u);
    waitVbl();
}

uint16_t NEOGEO_USER sky_frame(void)
{
    waitVbl();
    if (!ng_vram_window_open()) waitVbl();
    ng_palfx_vblank();
    ng_vram_commit();
    ng_palette_fx_update();
    s_joy_prev = s_joy;
    s_joy = poll_joystick();
    s_pressed = (uint16_t)(s_joy & (uint16_t)~s_joy_prev);
    s_frames++;
    /* Stir the generator every frame so enemy spawns don't fall into a
     * visible pattern when the player holds a steady input. */
    s_rng ^= (uint16_t)(s_joy + s_frames);
    return s_frames;
}

uint16_t NEOGEO_USER sky_joy(void)         { return s_joy; }
uint16_t NEOGEO_USER sky_joy_pressed(void) { return s_pressed; }

uint16_t NEOGEO_USER sky_rand(void)
{
    s_rng = (uint16_t)(s_rng * 2053u + 13849u);
    return s_rng;
}

/* ------------------------------------------------------------------ */
/*  FIX layer                                                           */
/* ------------------------------------------------------------------ */
void NEOGEO_USER sky_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    if (!text || x >= 40u || y >= 28u) return;
    ng_fix_puts(x, y, text, pal);
}

/*
 * Infix palette banks.
 *
 * A FIX map word is (palette << 12) | tile, so the FIX layer has exactly
 * SIXTEEN palette banks - and 0..3 are already spoken for by the text
 * inks.  fixtiles.py hands out one bank per infix image and Sky Lance
 * has 18 of them, so taking its pal_bank at face value runs off the end
 * and wraps straight back over the text inks: with the raw banks, the
 * energy cells came out drawn in the red warning ink (bank 19 & 15 = 3).
 *
 * Nearly all of those 18 palettes are duplicates - the ten counter
 * digits are one palette, so are SCORE and HI, so are the 1P marker and
 * the life pip - so identical ones are folded onto a single bank here
 * and the fold is recorded for sky_infix().
 */
#define SKY_FIX_INFIX_BANK0  4u
#define SKY_FIX_INFIX_BANKS  12u

static uint8_t s_infix_bank[INFIX_IMAGE_COUNT];

static uint8_t NEOGEO_USER sky_infix_pal_same(uint8_t a, uint8_t b)
{
    uint8_t k;
    for (k = 0u; k < 16u; k++) {
        if (INFIX_PALETTES[a][k] != INFIX_PALETTES[b][k]) return 0u;
    }
    return 1u;
}

void NEOGEO_USER sky_fix_palettes_init(void)
{
    uint8_t i, j, k;
    uint8_t used = 0u;
    uint16_t pal[16];

    for (i = 0u; i < INFIX_IMAGE_COUNT; i++) {
        uint8_t bank = 0xFFu;

        for (j = 0u; j < i; j++) {
            if (sky_infix_pal_same(i, j)) { bank = s_infix_bank[j]; break; }
        }
        if (bank != 0xFFu) { s_infix_bank[i] = bank; continue; }

        if (used >= SKY_FIX_INFIX_BANKS) {
            /* Out of banks.  Fall back to the body-text ink: wrong
             * colour, but it cannot corrupt another image's palette. */
            s_infix_bank[i] = SKY_PAL_BODY;
            continue;
        }

        s_infix_bank[i] = (uint8_t)(SKY_FIX_INFIX_BANK0 + used);
        used++;
        for (k = 0u; k < 16u; k++) pal[k] = INFIX_PALETTES[i][k];
        ng_palfx_screen_load(s_infix_bank[i], pal);
    }
}

void NEOGEO_USER sky_fix_blank(uint8_t x, uint8_t y, uint8_t cells)
{
    uint8_t i;
    for (i = 0u; i < cells; i++) {
        uint8_t cx = (uint8_t)(x + i);
        if (cx >= 40u || y >= 28u) return;
        ng_fix_blank_cell(cx, y);
    }
}

/*
 * Draw one artbox infix image at FIX cell (x, y).
 *
 * fixtiles.py slices each PNG row-major into 8x8 tiles starting at
 * tile_base, and gives every image its own FIX palette bank, so a whole
 * image is one nested loop over cols x rows.
 */
void NEOGEO_USER sky_infix(uint8_t x, uint8_t y, uint8_t infix_index)
{
    const InfixImage *img;
    uint8_t col, row;

    if (infix_index >= INFIX_IMAGE_COUNT) return;
    img = &INFIX_IMAGES[infix_index];

    for (row = 0u; row < img->rows; row++) {
        for (col = 0u; col < img->cols; col++) {
            uint8_t cx = (uint8_t)(x + col);
            uint8_t cy = (uint8_t)(y + row);
            if (cx >= 40u || cy >= 28u) continue;
            ng_fix_put_tile(cx, cy,
                             (uint16_t)(img->tile_base + (uint16_t)row * img->cols + col),
                             s_infix_bank[infix_index]);
        }
    }
}

void NEOGEO_USER sky_infix_number(uint8_t x, uint8_t y, uint32_t value, uint8_t digits)
{
    uint8_t i;

    /* Right-aligned: walk the cells backwards, two FIX columns per digit
     * because the counter art is 16x16. */
    for (i = 0u; i < digits; i++) {
        uint8_t d = (uint8_t)(value % 10u);
        uint8_t cx = (uint8_t)(x + (uint8_t)((digits - 1u - i) * 2u));
        sky_infix(cx, y, (uint8_t)(SKY_INFIX_DIGIT0 + d));
        value /= 10u;
    }
}
