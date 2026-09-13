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
#include "sdk/neogeo.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include "sdk/2d_engine/ng_sprite_window.h"
#include "sdk/2d_engine/ng_art_asset.h"
#include "sdk/2d_engine/ng_sprite_hw.h"
#include "sprite_meta.h"
#include "infix_palettes.h"

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
    ng_load_screen_palette(id);
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
/*
 * Two copies of one 256x256 page, stacked and slid downward.  The pages
 * were made vertically tileable when the art was imported, so page A at
 * (scroll - 256) and page B at (scroll) meet without a seam and the pair
 * wraps every 256 px.
 *
 * The negative Y on page A is deliberate and safe: SCB3 stores 496 - y
 * in a 9-bit field, so y and y+512 are the same position to the
 * hardware, and both readings of y = -256 leave the page off-screen.
 */
#define SKY_BG_PAGE_H  256

static uint8_t  s_bg_id     = 0u;
static uint16_t s_bg_scroll = 0u;

/*
 * The two pages are kept here rather than rebuilt each frame.  Scrolling only
 * changes their Y, and ng_sprite_group_flush() writes SCB2/3/4 alone unless
 * the tiles are marked dirty - where ng_sprite_group_upload() rewrites the
 * whole tilemap.  Two 16x16 pages is 512 tilemap words, far more than a vblank
 * has room for, so uploading them every frame both judders the scroll and eats
 * the budget the character sprites drawn afterwards need.
 */
static NGSpriteGroup s_bg_page[2];
static uint8_t       s_bg_ready = 0u;

void NEOGEO_USER sky_bg_select(uint8_t id)
{
    const NGSpriteAssetMeta *m;
    const NGArtAsset *art;
    uint8_t i;

    if (s_bg_id == id) return;
    s_bg_id = id;
    s_bg_scroll = 0u;
    s_bg_ready = 0u;
    if (!id) return;

    ng_load_screen_palette(id);

    m = sky_meta(id);
    if (!m) return;
    art = ng_screen_art_asset(id);

    /* Build both pages once and push the tile data now, while the scene is
     * still being set up and there is time for it. */
    for (i = 0u; i < 2u; i++) {
        NGSpriteGroup *g = &s_bg_page[i];
        ng_sprite_group_init(g, (i == 0u) ? NG_SPR_BG0_FIRST : NG_SPR_BG1_FIRST,
                             16u, 16u, m->tile_base, m->palette_bank);
        ng_sprite_group_set_tile_stride(g, m->tile_stride);
        ng_sprite_group_set_palette_map(g, art ? art->tile_palettes : 0);
        ng_sprite_group_set_active_rows(g, 16u);
        ng_sprite_group_set_scale(g, SKY_SCALE_FULL, SKY_SCALE_FULL);
        ng_sprite_group_set_pos(g, SKY_FIELD_X,
                                (i == 0u) ? -SKY_BG_PAGE_H : 0);
        ng_sprite_group_upload(g);
    }
    s_bg_ready = 1u;
}

void NEOGEO_USER sky_bg_advance(uint8_t pixels)
{
    s_bg_scroll = (uint16_t)((s_bg_scroll + pixels) % SKY_BG_PAGE_H);
}

void NEOGEO_USER sky_bg_draw(void)
{
    if (!s_bg_id || !s_bg_ready) return;

    /* Position only - the tiles are already in VRAM and are not marked
     * dirty, so each flush writes three words per strip and nothing more. */
    ng_sprite_group_set_pos(&s_bg_page[0], SKY_FIELD_X,
                            (int16_t)((int16_t)s_bg_scroll - SKY_BG_PAGE_H));
    ng_sprite_group_set_pos(&s_bg_page[1], SKY_FIELD_X, (int16_t)s_bg_scroll);
    ng_sprite_group_flush(&s_bg_page[0]);
    ng_sprite_group_flush(&s_bg_page[1]);
}

void NEOGEO_USER sky_bg_hide(void)
{
    ng_sprite_hide_vram_base(NG_SPR_VRAM_BASE(NG_SPR_BG0_FIRST), 32u);
    s_bg_id = 0u;
    s_bg_ready = 0u;
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
    waitVbl();
    setBACKDROP(SKY_BG_CLEAR);
    clearFix();
    clearSprs();
    ng_sprite_hide_all();
    ng_chars_init();
    sky_bg_hide();
    s_joy = 0u;
    s_joy_prev = 0u;
    s_pressed = 0u;
    waitVbl();
}

uint16_t NEOGEO_USER sky_frame(void)
{
    waitVbl();
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
    mess_out_clipped(x, y, text, (short)pal, (uint16_t)(40u - x));
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
        load_palettes(pal, PALETTES + PALOFFSET * s_infix_bank[i]);
    }
}

void NEOGEO_USER sky_fix_blank(uint8_t x, uint8_t y, uint8_t cells)
{
    uint8_t i;
    for (i = 0u; i < cells; i++) {
        uint8_t cx = (uint8_t)(x + i);
        if (cx >= 40u || y >= 28u) return;
        ngfix_write_tile(cx, y, 0xFFu, 0u);
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
            ngfix_write_tile(cx, cy,
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
