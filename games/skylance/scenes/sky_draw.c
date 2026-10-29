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
#include "sdk/2d_engine/ng_depthfx.h"
#include "games/skylance/artbox/generated/sky_native_assets.h"
#include "sdk/2d_engine/ng_palette_math.h"
#include "sprite_meta.h"
#include "infix_palettes.h"
#include "sky_video_sync.h"
#include "sdk/ng_perf.h"

static SkyVideoSync s_video_sync;
static volatile uint16_t s_palette_irq_count;
static volatile uint8_t s_palette_irq_lines;
static void NEOGEO_USER sky_ground_vblank(void);
static NGCharacter *s_shot_owner[SKY_MAX_PSHOTS + SKY_MAX_ESHOTS];
static volatile uint16_t s_shot_slot_faults;
static NGCharacter *s_enemy_owner[SKY_MAX_ENEMIES], *s_boss_owner;
static volatile uint16_t s_enemy_slot_faults;

void NEOGEO_USER sky_palette_vblank(void)
{
    if (ng_vram_busy) return;
    if (!sky_video_take(&s_video_sync)) return;
    s_palette_irq_lines = ng_vram_lines_left();
    ng_palfx_vblank();
    /* Consume the completed scene batch before BIOS I/O spends the blank. */
    ng_vram_commit();
    ng_vram_busy++;
    sky_ground_vblank();
    ng_vram_busy--;
    s_palette_irq_count++;
}

const uint16_t * NEOGEO_USER ng_get_screen_palette(uint16_t screen_id);
static uint8_t s_palette_loaded[NG_ASSET_META_COUNT + 1u];
static uint8_t s_damage_color = 0xffu;

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
static const NGSpriteAssetMeta * NEOGEO_USER sky_meta(uint16_t id)
{
    if (id == 0u || id > NG_ASSET_META_COUNT) return 0;
    return &g_ng_asset_meta[id - 1u];
}

int16_t NEOGEO_USER sky_width(uint16_t id, uint8_t scale)
{
    const NGSpriteAssetMeta *m = sky_meta(id);
    if (!m) return 0;
    return (int16_t)ng_sprite_scaled_x(m->content_width, scale);
}

int16_t NEOGEO_USER sky_height(uint16_t id, uint8_t scale)
{
    const NGSpriteAssetMeta *m = sky_meta(id);
    if (!m) return 0;
    return (int16_t)ng_sprite_scaled_y(m->content_height, scale);
}

void NEOGEO_USER sky_bind(NGCharacter *c, uint16_t id, uint8_t scale, uint8_t band)
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
    if (id == 4u || id == 6u || id == 8u ||
        (id >= 35u && id <= 40u) || (id >= 72u && id <= 83u)) {
        half_w = (int16_t)ng_sprite_scaled_x(24u, scale);
        half_h = (int16_t)ng_sprite_scaled_y(24u, scale);
    }
    if (id >= SKY_SPECIAL_FIRST && id < SKY_SPECIAL_FIRST + 12u) {
        half_w = (int16_t)ng_sprite_scaled_x(32u, scale);
        half_h = (int16_t)ng_sprite_scaled_y(32u, scale);
    }
    if (g_ng_asset_pivots[id-1u][0] >= 0) {
        half_w = (int16_t)ng_sprite_scaled_x((uint16_t)g_ng_asset_pivots[id-1u][0], scale);
        half_h = (int16_t)ng_sprite_scaled_y((uint16_t)g_ng_asset_pivots[id-1u][1], scale);
    }
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
        if (c->kind == SKY_KIND_PLAYER) {
            ng_char_set_body(c, -6, -6, 12, 12);
        } else if (c->kind == SKY_KIND_ENEMY || c->kind == SKY_KIND_BOSS) {
            int16_t bw = (w < 20) ? 20 : w;
            int16_t bh = (h < 20) ? 20 : h;
            ng_char_set_body(c, (int16_t)(-(bw >> 1)), (int16_t)(-(bh >> 1)), bw, bh);
        } else {
            ng_char_set_body(c, (int16_t)(-(w >> 2)), (int16_t)(-(h >> 2)),
                             (int16_t)(w >> 1), (int16_t)(h >> 1));
        }
    }

    ng_char_set_priority(c, band, 0);
    c->vram_prio = (c->kind == SKY_KIND_PLAYER || c->kind == SKY_KIND_ESHOT) ?
                  NG_SG_PRIO_HIGH : NG_SG_PRIO_NORMAL;
    if (!s_palette_loaded[id]) {
        ng_load_screen_palette(id);
        s_palette_loaded[id] = 1u;
        if (c->kind == SKY_KIND_PLAYER) s_damage_color = 0xffu;
    }
}

NGCharacter * NEOGEO_USER sky_spawn(uint8_t kind, uint16_t id,
                                    int16_t cx, int16_t cy,
                                    uint8_t scale, uint8_t band)
{
    NGCharacter *c = chars_add(kind, cx, cy);
    if (!c) return 0;
    sky_bind(c, id, scale, band);
    if (kind == SKY_KIND_PSHOT || kind == SKY_KIND_ESHOT) {
        uint8_t i, begin = kind == SKY_KIND_PSHOT ? 0u : SKY_MAX_PSHOTS;
        uint8_t end = kind == SKY_KIND_PSHOT ? SKY_MAX_PSHOTS : SKY_MAX_PSHOTS + SKY_MAX_ESHOTS;
        /* Shot artwork is fixed after spawn; seeker updates only move it. */
        ng_char_set_render_mode(c, NG_CHAR_RENDER_MOVE_ONLY);
        for (i = 0u; i < SKY_MAX_PSHOTS + SKY_MAX_ESHOTS; i++) {
            NGCharacter *old = s_shot_owner[i];
            uint8_t expected = i < SKY_MAX_PSHOTS ? SKY_KIND_PSHOT : SKY_KIND_ESHOT;
            if (old && (!old->active || old->kind != expected || old == c)) {
                uint16_t first = (uint16_t)(i < SKY_MAX_PSHOTS ? SKY_FIXED_PSHOT_FIRST + i :
                    SKY_FIXED_ESHOT_FIRST + i - SKY_MAX_PSHOTS);
                if (old->active && old != c && old->kind != expected && old->sprite_first == first)
                    ng_char_pin_sprite(old, 0xFFFFu);
                s_shot_owner[i] = 0;
            }
        }
        for (i = begin; i < end && s_shot_owner[i]; i++) {}
        if (i < end && c->sprite_strips == 1u) {
            s_shot_owner[i] = c;
            ng_char_pin_sprite(c, (uint16_t)(kind == SKY_KIND_PSHOT ?
                SKY_FIXED_PSHOT_FIRST + i : SKY_FIXED_ESHOT_FIRST + i - SKY_MAX_PSHOTS));
        } else s_shot_slot_faults++;
    }
    if (kind == SKY_KIND_ENEMY) {
        uint8_t i;
        for (i = 0u; i < SKY_MAX_ENEMIES; i++) {
            NGCharacter *old = s_enemy_owner[i];
            if (old && (!old->active || old->kind != SKY_KIND_ENEMY || old == c)) {
                if (old->active && old != c && old->kind != SKY_KIND_ENEMY &&
                    old->sprite_first == SKY_FIXED_ENEMY_FIRST + i * 8u)
                    ng_char_pin_sprite(old, 0xFFFFu);
                s_enemy_owner[i] = 0;
            }
        }
        for (i = 0u; i < SKY_MAX_ENEMIES && s_enemy_owner[i]; i++) {}
        if (i < SKY_MAX_ENEMIES && c->sprite_strips <= 8u) {
            s_enemy_owner[i] = c;
            ng_char_pin_sprite(c, (uint16_t)(SKY_FIXED_ENEMY_FIRST + i * 8u));
        } else s_enemy_slot_faults++;
    } else if (kind == SKY_KIND_BOSS) {
        if (s_boss_owner && s_boss_owner->active && s_boss_owner != c && s_boss_owner->kind != SKY_KIND_BOSS &&
            s_boss_owner->sprite_first == SKY_FIXED_BOSS_FIRST)
            ng_char_pin_sprite(s_boss_owner, 0xFFFFu);
        if (c->sprite_strips <= 8u && (!s_boss_owner || !s_boss_owner->active ||
            s_boss_owner == c || s_boss_owner->kind != SKY_KIND_BOSS)) {
            s_boss_owner = c;
            ng_char_pin_sprite(c, SKY_FIXED_BOSS_FIRST);
        } else s_enemy_slot_faults++;
    }
    c->visible = 1u;
    c->life_state = NG_CHAR_LIFE_VISIBLE;
    return c;
}

void NEOGEO_USER sky_damage_color(const NGCharacter *c, uint8_t critical)
{
    uint8_t i;
    uint16_t colors[16];
    const uint16_t *base;
    uint8_t id;
    if (!c || c->kind != SKY_KIND_PLAYER || s_damage_color == critical) return;
    id = c->palette == 44u ? SKY_P1_PLANE : c->palette == 48u ? SKY_P2_PLANE : SKY_P3_PLANE;
    base = ng_get_screen_palette(id);
    if (!base) return;
    for (i = 0u; i < 4u; i++) {
        ng_palette_tint_colors(colors, base + (uint16_t)i * 16u,
                               255u, 72u, 48u, critical ? 32u : 0u);
        ng_palfx_screen_load((uint8_t)(c->palette + i), colors);
    }
    s_damage_color = critical;
}

static uint8_t s_damage_flash = 0u;

void NEOGEO_USER sky_damage_flash(const NGCharacter *c, uint8_t flash)
{
    uint8_t i;
    uint16_t colors[16];
    const uint16_t *base;
    uint8_t id;
    if (!c || c->kind != SKY_KIND_PLAYER) return;
    if (s_damage_flash == flash && !flash) return;
    id = c->palette == 44u ? SKY_P1_PLANE : c->palette == 48u ? SKY_P2_PLANE : SKY_P3_PLANE;
    base = ng_get_screen_palette(id);
    if (!base) return;
    s_damage_flash = flash;
    for (i = 0u; i < 4u; i++) {
        if (flash) {
            uint8_t k;
            for (k = 0u; k < 16u; k++) {
                colors[k] = (k == 0u) ? 0x0000u : ((flash & 1u) ? 0x7FFFu : 0x7E00u);
            }
        } else {
            ng_palette_tint_colors(colors, base + (uint16_t)i * 16u,
                                   255u, 72u, 48u, s_damage_color ? 32u : 0u);
        }
        ng_palfx_screen_load((uint8_t)(c->palette + i), colors);
    }
}

void NEOGEO_USER sky_hud_pilot_palette(const NGCharacter *c, uint8_t pilot_idx, uint8_t situation)
{
    uint8_t i;
    uint16_t colors[16];
    const uint16_t *base;
    uint8_t id;
    if (!c) return;
    id = pilot_idx == 0u ? SKY_P1_PILOT : (pilot_idx == 1u ? SKY_P2_PILOT : SKY_P3_PILOT);
    base = ng_get_screen_palette(id);
    if (!base) return;
    for (i = 0u; i < 4u; i++) {
        if (situation == 0u) {
            /* Situation 0: CRUISE / PATROL - Natural skin with subtle cyan avionics clarity */
            ng_palette_tint_colors(colors, base + (uint16_t)i * 16u, 80u, 220u, 255u, 8u);
        } else if (situation == 1u) {
            /* Situation 1: ENGAGE / ATTACK - Amber gold targeting reticle visor illumination */
            ng_palette_tint_colors(colors, base + (uint16_t)i * 16u, 255u, 190u, 40u, 28u);
        } else if (situation == 2u) {
            /* Situation 2: DAMAGE / HIT - Impact shock white/red trauma strobe */
            ng_palette_tint_colors(colors, base + (uint16_t)i * 16u, 255u, 80u, 80u, 44u);
        } else {
            /* Situation 3: CRITICAL / MAYDAY - High-intensity emergency red alarm cockpit lighting */
            ng_palette_tint_colors(colors, base + (uint16_t)i * 16u, 255u, 20u, 20u, 52u);
        }
        ng_palfx_screen_load((uint8_t)(c->palette + i), colors);
    }
}

/* ------------------------------------------------------------------ */
/*  Scrolling background                                                */
/* ------------------------------------------------------------------ */
/* Terrain occupies 1..44, clouds 45..88; craft start at slot 96. */
static uint8_t s_bg_id;
static uint8_t s_bg_advance;
static NGVerticalLayer s_ground;
static NGSpriteGroup s_air_shadow[6], s_far_cloud;
static NGSpriteGroup s_instrument[3];
static uint16_t s_instrument_id[3];
static uint16_t s_shadow_asset[6];
static uint8_t s_shadow_visible[6];
static uint8_t s_far_cloud_ready;
static volatile uint16_t s_depth_strip_peak;
static NGCharacter *s_overhead_cloud;
static uint16_t s_cloud_tick;
static int16_t s_overhead_x;
static int32_t s_overhead_x_q8;     /* the overhead cloud's place on the field */
static uint16_t s_line_peak;        /* busiest line, last sky_depth_draw */
static volatile uint16_t s_depth_shadow_drops;
static NGShooterCamera s_camera;
static int16_t s_follow_x = 160, s_follow_y = 112;
static uint8_t s_bg_ready;
static uint8_t s_route_stage;
static uint8_t s_route_pages;
static uint8_t s_ground_page[2];
static uint8_t s_ground_target[2], s_ground_column[2], s_ground_pending[2];
static volatile uint16_t s_ground_preload_misses;

static void NEOGEO_USER sky_ground_request(uint8_t slot, uint8_t page)
{
    NGSpriteGroup *g = &s_ground.pages[slot];
    const NGArtAsset *a = ng_screen_art_asset(sky_route_asset_ids[s_route_stage][page >> 1]);
    ng_sprite_group_detach(g);
    ng_sprite_group_set_tile_base(g, (uint16_t)(a->tile_base + (page & 1u) * 352u));
    ng_sprite_group_set_palette_map(g, a->tile_palettes + (page & 1u) * 352u);
    s_ground_target[slot] = page;
    s_ground_column[slot] = 0u;
    s_ground_pending[slot] = 1u;
}

static void NEOGEO_USER sky_ground_vblank(void)
{
    uint8_t slot;
    if (!s_bg_ready) return;
    for (slot = 0u; slot < 2u; slot++) {
        NGSpriteGroup *g = &s_ground.pages[slot];
        if (!s_ground_pending[slot]) continue;
        if (s_ground_pending[slot] == 1u) {
            if (!ng_sprite_stage_retire_range(g->firstSprite, g->strips)) return;
            s_ground_pending[slot] = 2u;
        }
        if (!ng_sprite_group_stage_map(g, &s_ground_column[slot])) return;
        s_ground_page[slot] = s_ground_target[slot];
        s_ground_pending[slot] = 0u;
        g->readyRows = 0u;
        /* Controls were parked; the next producer publishes a cheap full
         * transform over the already-complete map. */
        g->dirty = NG_SGF_DIRTY_POS | NG_SGF_DIRTY_SHRINK | NG_SGF_DIRTY_VIS;
    }
}
/* Keep the atomic fade upload within the available blanking window.
 * Higher effect banks use the normal per-bank palette queue. */
static uint16_t s_palette_base[104u * 16u];
static uint16_t s_palette_out[104u * 16u];

void NEOGEO_USER sky_bg_select(uint8_t id)
{
    const NGArtAsset *art = ng_screen_art_asset(id);
    uint8_t i;
    const NGArtAsset *cloud = ng_screen_art_asset(SKY_NATIVE_FAR_CLOUD);
    if (s_bg_id == id && s_bg_ready) return;
    if (!art || art->strips != 22u || art->active_rows != 32u) return;
    s_bg_id = id;
    switch (id) {
    case SKY_BG_COAST: s_route_stage = 1u; break;
    case SKY_BG_CONVOY: s_route_stage = 2u; break;
    case SKY_BG_OPEN_SEA: s_route_stage = 3u; break;
    case SKY_BG_OUTPOST: s_route_stage = 4u; break;
    case SKY_BG_ARMORED: s_route_stage = 5u; break;
    case SKY_BG_CITADEL: s_route_stage = 6u; break;
    default: s_route_stage = 0u; break;
    }
    s_route_pages=sky_route_page_counts[s_route_stage];
    s_ground_page[0] = 0u; s_ground_page[1] = 1u;
    s_ground_pending[0] = s_ground_pending[1] = 0u;
    s_bg_advance = 0u;
    ng_load_screen_palette(id);
    ng_shooter_camera_init(&s_camera);
    ng_shooter_camera_set_follow(&s_camera, 12u, 0u, 16u, 4u);
    s_follow_x = 144;
    s_follow_y = 112;
    s_bg_ready = ng_vertical_layer_init(&s_ground, 1u, 22u, 16u,
                                       art->tile_base, art->palette_bank, art->tile_palettes, -16);
    if (cloud) {
        ng_load_screen_palette(SKY_NATIVE_FAR_CLOUD);
        ng_sprite_group_init(&s_far_cloud,76u,6u,3u,cloud->tile_base,cloud->palette_bank);
        ng_sprite_group_set_tile_stride(&s_far_cloud,6u);
        ng_sprite_group_set_palette_map(&s_far_cloud,cloud->tile_palettes);
        ng_sprite_group_set_pos(&s_far_cloud,184,-48);
        ng_sprite_group_upload(&s_far_cloud);
        s_far_cloud_ready=1u;
    }
    for(i=0;i<6u;i++) { s_shadow_asset[i]=0u;s_shadow_visible[i]=0u; }
    s_overhead_cloud=0;s_cloud_tick=0u;s_line_peak=0u;
    /* Palette rotation cycling disabled to eliminate hardware color glitches. */
}

void NEOGEO_USER sky_bg_follow(int16_t x, int16_t y)
{
    s_follow_x = x;
    s_follow_y = y;
}
int16_t NEOGEO_USER sky_bg_camera_x(void)
{
    return (int16_t)(s_camera.x_q8 / 256);
}

void NEOGEO_USER sky_bg_advance(uint8_t pixels)
{
    s_bg_advance = pixels;
}

/* 1 when the 16x16 cell of a naval route under screen (x, y) is open water;
 * land routes have no water map and answer 0. Rows above the screen read the
 * pages still to come, as the ground layer streams them. */
uint8_t NEOGEO_USER sky_bg_water(int16_t x, int16_t y)
{
    const uint32_t *map;
    uint16_t travel, page;
    int16_t row, rx;
    if (s_route_stage != 1u && s_route_stage != 3u) return 0u;
    map = sky_route_water[s_route_stage == 1u ? 0u : 1u];
    travel = sky_bg_travel();
    page = (uint16_t)(travel / 256u);
    row = (int16_t)(y - (int16_t)(travel & 255u));
    while (row < 0) { row = (int16_t)(row + 256); page++; }
    while (row >= 256 && page) { row = (int16_t)(row - 256); page--; }
    if (page >= s_route_pages) page = (uint16_t)(s_route_pages - 1u);
    rx = (int16_t)(x + 16 + sky_bg_camera_x());
    if (rx < 0 || rx >= 352 || row >= 256) return 0u;
    return (uint8_t)((map[((page * 256u + (uint16_t)row) >> 4) & 127u] >> ((uint16_t)rx >> 4)) & 1u);
}

/* Where the route stops: its last page, the boss's arena, fills the screen. */
uint16_t NEOGEO_USER sky_bg_travel_end(void)
{
    return (uint16_t)((s_route_pages?s_route_pages-1u:7u)*256u);
}

uint16_t NEOGEO_USER sky_bg_travel(void)
{
    uint16_t end=sky_bg_travel_end();
    return s_camera.travel_q8 >= (int32_t)end * 256L ? end :
           (uint16_t)(s_camera.travel_q8 / 256);
}

void NEOGEO_USER sky_bg_draw(void)
{
    uint8_t i;
    uint16_t travel, phase;
    int16_t previous_x;
    if (!s_bg_ready) return;
    ng_shooter_camera_set_speed(&s_camera, (int16_t)((uint16_t)s_bg_advance * 64u));
    previous_x=sky_bg_camera_x();
    ng_shooter_camera_step(&s_camera, (int16_t)(s_follow_x + 16), s_follow_y);
    {
        int16_t delta=(int16_t)(sky_bg_camera_x()-previous_x);
        sky_stage_camera_shift(delta);
        /* The overhead cloud is weather between the camera and the field:
         * a pan moves it half again as far as the ground (the depth
         * module's foreground weight). Held at a screen x it slid along
         * with the plane whenever the pilot banked. */
        s_overhead_x_q8-=(int32_t)delta*384;
    }
    travel = sky_bg_travel();
    phase = (uint16_t)(travel & 255u);
    /* A departing page is wholly below the field at phase224. Refill it
     * there, 32 pixels before it returns above the screen. Even cruise
     * speed leaves32 updates for bounded column uploads. */
    for (i = 0u; i < 2u; i++) {
        uint8_t page = (uint8_t)((travel >> 8) + i);
        uint8_t slot = (uint8_t)(page & 1u);
        NGSpriteGroup *g = &s_ground.pages[slot];
        if (page >= s_route_pages) continue;
        if (s_ground_page[slot] != page && !s_ground_pending[slot] &&
            !(i == 0u && phase >= 224u && s_ground_page[slot] == page + 2u)) {
            s_ground_preload_misses++;
            sky_ground_request(slot, page);
        }
        ng_sprite_group_set_pos(g, (int16_t)(-16 - s_camera.x_q8 / 256),
                                 (int16_t)(phase - (i ? 256 : 0)));
        if (!s_ground_pending[slot]) ng_sprite_group_flush(g);
    }
    if (phase >= 224u && (travel >> 8) + 2u < s_route_pages) {
        uint8_t page = (uint8_t)((travel >> 8) + 2u), slot = (uint8_t)(page & 1u);
        if (!s_ground_pending[slot] && s_ground_page[slot] != page)
            sky_ground_request(slot, page);
    }
    if(s_far_cloud_ready) {
        /* A cloud bank below the plane but above the ground is nearer the
         * camera than the terrain: it scrolls a quarter faster than the
         * ground and pans with it, crossing once every 1024 pixels of
         * scroll at a new spot. As distant terrain (a quarter of the
         * ground's scroll and pan) it lingered on screen and drifted along
         * with the plane. It is decoration: parked below the screen when
         * the busiest line is near the LSPC's 96 sprites. */
        uint16_t pass=(uint16_t)(travel+travel/4u);
        NGShooterProjection p=ng_depthfx_shooter_project(&s_camera,
                    (int16_t)(32+(int16_t)(((pass>>10)*97u)%192u)),
                    (int16_t)((pass&1023u)-48u),NG_SHOOTER_TERRAIN);
        if(p.y>232 || s_line_peak>88u) p.y=232;
        ng_sprite_group_set_pos(&s_far_cloud,(int16_t)(p.x-48),p.y);
        ng_sprite_group_flush(&s_far_cloud);
    }
    s_bg_advance = 0u;
}

static uint16_t NEOGEO_USER sky_shadow_for(const NGCharacter *c)
{
    static const uint16_t planes[3][7] = {
        {35,72,73,4,74,75,36},{37,76,77,6,78,79,38},{39,80,81,8,82,83,40}};
    static const uint16_t shadow[3]={SKY_NATIVE_SHADOW_0_0,SKY_NATIVE_SHADOW_1_0,SKY_NATIVE_SHADOW_2_0};
    static const uint16_t damage[3]={SKY_NATIVE_DAMAGE_0_0,SKY_NATIVE_DAMAGE_1_0,SKY_NATIVE_DAMAGE_2_0};
    static const uint16_t unit[10]={SKY_NATIVE_UNIT_SHADOW_DRONE,SKY_NATIVE_UNIT_SHADOW_FIGHTER,
        0u,SKY_NATIVE_UNIT_SHADOW_HELI,0u,SKY_NATIVE_UNIT_SHADOW_TANK,
        SKY_NATIVE_UNIT_SHADOW_BOMBER,SKY_NATIVE_UNIT_SHADOW_SAM,SKY_NATIVE_UNIT_SHADOW_RADAR_SITE,0u};
    uint8_t pilot,bank;
    if(c->kind==SKY_KIND_PLAYER) {
        pilot=c->palette==44u?0u:c->palette==48u?1u:2u;
        for(bank=0;bank<7u;bank++)
            if(c->sprite_tile==g_ng_asset_meta[planes[pilot][bank]-1u].tile_base ||
               c->sprite_tile==g_ng_asset_meta[damage[pilot]+bank-1u].tile_base) return shadow[pilot]+bank;
        return shadow[pilot]+3u;
    }
    if(c->kind==SKY_KIND_BOSS) return s_route_stage==0u?SKY_NATIVE_UNIT_SHADOW_RED_FORTRESS:0u;
    if(c->kind==SKY_KIND_ENEMY) {
        if(c->data0==5u) return s_route_stage==0u?SKY_NATIVE_UNIT_SHADOW_MINIBOSS_0:0u;
        /* Terrain units cast their small drawn drop shadows, rather than
         * aircraft-height floating silhouettes. */
        if(c->data0==3u || c->data0==4u) return 0u;
        return unit[c->data2%10u];
    }
    return 0u;
}
static void NEOGEO_USER sky_actor_budget(NGShooterStripBudget *b)
{
    uint8_t i;
    ng_shooter_budget_init(b,22u);
    if(s_far_cloud_ready) ng_shooter_budget_add(b,s_far_cloud.y,3u,6u);
    for(i=0u;i<3u;i++) if(s_instrument_id[i])
        ng_shooter_budget_add(b,s_instrument[i].y,s_instrument[i].activeRows,s_instrument[i].strips);
    for (const NGCharacter *c = chars_at(0u), *end = c + ng_chars_slots_used(); c < end; c++) {
        if(c && c->active && c->visible)
            ng_shooter_budget_add(b,(int16_t)(c->y+c->sprite_offset_y),c->sprite_height,c->sprite_strips);
    }
}
uint16_t NEOGEO_USER sky_depth_line_peak(void)
{
    return s_line_peak;
}

void NEOGEO_USER sky_depth_draw(void)
{
    static const uint16_t slot[6]={45u,48u,53u,58u,63u,68u};
    NGShooterStripBudget budget;
    NGCharacter *actor[6]={0,0,0,0,0,0};
    uint8_t i,enemies=0u;
    uint16_t peak;
    if(!s_bg_ready) return;
    for (NGCharacter *c = chars_at(0u), *end = c + ng_chars_slots_used(); c < end; c++) {
        if(!c || !c->active)continue;
        if(c->kind==SKY_KIND_PLAYER) actor[0]=c;
        else if(c->kind==SKY_KIND_BOSS) actor[5]=c;
        else if(c->kind==SKY_KIND_ENEMY && enemies<4u) actor[++enemies]=c;
    }
    if(actor[0]) {
        s_cloud_tick++;
        /* over the field, clear of the status line (row 3) it would blank */
        if(s_cloud_tick==200u) sky_puts(12u,6u,"CLOUD BANK AHEAD",SKY_PAL_TITLE);
        if(s_cloud_tick==240u) sky_fix_blank(12u,6u,16u);
        if(s_overhead_cloud && (!s_overhead_cloud->active || s_overhead_cloud->kind!=SKY_KIND_CANOPY))s_overhead_cloud=0;
        if(s_overhead_cloud && s_overhead_cloud->y>256) {ng_chars_remove(s_overhead_cloud);s_overhead_cloud=0;}
        /* The strip budget is counted once a frame below; only the frame
         * that may release the overhead cloud needs it before then. */
        if((s_cloud_tick%960u)==240u && !s_overhead_cloud &&
           (sky_actor_budget(&budget),ng_shooter_budget_peak(&budget)+6u<=88u)) {
            s_overhead_x=(s_route_stage&1u)?96:192;
            s_overhead_x_q8=(int32_t)s_overhead_x*256;
            s_overhead_cloud=sky_spawn(SKY_KIND_CANOPY,SKY_NATIVE_OVERHEAD_CLOUD,s_overhead_x,-48,
                                       SKY_SCALE_FULL,NG_RENDER_BAND_FX);
            if(s_overhead_cloud)ng_char_set_speed(s_overhead_cloud,0,2);
        }
        /* The cloud keeps its place on the field (see sky_bg_draw), not
         * its place on the screen; panned well off a side, it is gone. */
        if(s_overhead_cloud) {
            s_overhead_x=(int16_t)(s_overhead_x_q8>>8);
            if(s_overhead_x<-96 || s_overhead_x>400) {ng_chars_remove(s_overhead_cloud);s_overhead_cloud=0;}
            else ng_char_set_pos(s_overhead_cloud,s_overhead_x,s_overhead_cloud->y);
        }
    }
    sky_actor_budget(&budget);
    peak=ng_shooter_budget_peak(&budget);
    /* Optional particles yield before critical aircraft or dangerous bolts.
     * The later native VRAM audit checks the actual physical SCB occupancy. */
    for(i=0u;i<3u && peak>90u;i++) {
        uint8_t kind=i==0u?SKY_KIND_SMOKE:i==1u?SKY_KIND_CANOPY:SKY_KIND_BLAST;
        for (NGCharacter *c = chars_at(0u), *end = c + ng_chars_slots_used(); c < end && peak>90u; c++) {
            if(c && c->active && c->kind==kind) {
                ng_chars_remove(c);if(c==s_overhead_cloud)s_overhead_cloud=0;sky_actor_budget(&budget);peak=ng_shooter_budget_peak(&budget);
            }
        }
    }
    for(i=0;i<6u;i++) {
        NGCharacter *c=actor[i];uint16_t id=c && c->visible?sky_shadow_for(c):0u;
        const NGArtAsset *art=id?ng_screen_art_asset(id):0;
        NGSpriteGroup *g=&s_air_shadow[i];
        if(!art || peak+art->strips>90u) {
            if(art)s_depth_shadow_drops++;
            if(s_shadow_visible[i]) {ng_sprite_group_hide_queued(g);s_shadow_visible[i]=0u;}
            continue;
        }
        if(s_shadow_asset[i]!=id) {
            if(!s_palette_loaded[id]) { ng_load_screen_palette(id);s_palette_loaded[id]=1u; }
            ng_sprite_group_cancel(g);
            ng_sprite_group_init(g,slot[i],art->strips,art->active_rows,art->tile_base,art->palette_bank);
            ng_sprite_group_set_tile_stride(g,art->strips);
            ng_sprite_group_set_palette_map(g,art->tile_palettes);
            ng_sprite_group_set_scale(g,159u,159u);
            s_shadow_asset[i]=id;s_shadow_visible[i]=0u;
        }
        {
            NGShooterProjection p=ng_depthfx_shooter_project(&s_camera,(int16_t)(c->x+8),
                         (int16_t)(c->y+12),NG_SHOOTER_SHADOW);
            int16_t px=(int16_t)(p.x-ng_sprite_scaled_x((uint16_t)g_ng_asset_pivots[id-1u][0],159u));
            int16_t py=(int16_t)(p.y-ng_sprite_scaled_y((uint16_t)g_ng_asset_pivots[id-1u][1],159u));
            ng_sprite_group_set_pos(g,px,py);
            ng_sprite_group_set_visible(g,1u);
            ng_sprite_group_set_priority(g, NG_SG_PRIO_LOW);
            ng_sprite_group_flush(g);
            s_shadow_visible[i]=1u;
            ng_shooter_budget_add(&budget,py,art->active_rows,art->strips);
            peak+=art->strips; /* conservative admission; one final exact walk */
        }
    }
    s_line_peak=peak;
    /* The inspector's estimate: a full scan, so refreshed every eighth frame. */
    {static uint8_t estimate;if(!(++estimate&7u)) s_depth_strip_peak=ng_shooter_budget_peak(&budget);}
}

/*
 * The cockpit instruments: three fixed 32x32 sprite groups in the HUD
 * column, front-most (slots 360-365). Not characters: they never move, so
 * a new frame only rebinds their tiles, and they cost nothing in the
 * per-frame character pass.
 */
#define SKY_INSTRUMENT_SLOT 360u

void NEOGEO_USER sky_instrument_show(uint8_t k, uint16_t id, int16_t x, int16_t y)
{
    const NGArtAsset *art;
    NGSpriteGroup *g;
    if (k > 2u || s_instrument_id[k] == id) return;
    art = ng_screen_art_asset(id);
    if (!art) return;
    g = &s_instrument[k];
    if (!s_palette_loaded[id]) {
        ng_load_screen_palette(id);
        s_palette_loaded[id] = 1u;
    }
    if (!s_instrument_id[k]) {
        ng_sprite_group_init(g, (uint16_t)(SKY_INSTRUMENT_SLOT + k * 2u), art->strips,
                             art->active_rows, art->tile_base, art->palette_bank);
        ng_sprite_group_set_tile_stride(g, art->strips);
        ng_sprite_group_set_palette_map(g, art->tile_palettes);
        ng_sprite_group_set_pos(g, x, y);
        ng_sprite_group_set_priority(g, NG_SG_PRIO_HIGH);
        ng_sprite_group_flush(g);
    } else {
        ng_sprite_group_set_tile_base(g, art->tile_base);
        ng_sprite_group_set_palette_map(g, art->tile_palettes);
        ng_sprite_group_flush(g);
    }
    s_instrument_id[k] = id;
}

void NEOGEO_USER sky_instruments_hide(void)
{
    uint8_t k;
    for (k = 0u; k < 3u; k++) {
        if (!s_instrument_id[k]) continue;
        ng_sprite_group_cancel(&s_instrument[k]);
        ng_sprite_group_hide(&s_instrument[k]);
        s_instrument_id[k] = 0u;
    }
}

void NEOGEO_USER sky_bg_hide(void)
{
    ng_vertical_layer_hide(&s_ground);
    { uint8_t i;for(i=0;i<6u;i++) { ng_sprite_group_cancel(&s_air_shadow[i]);ng_sprite_group_hide(&s_air_shadow[i]);s_shadow_asset[i]=0u;s_shadow_visible[i]=0u; } }
    ng_sprite_group_cancel(&s_far_cloud);ng_sprite_group_hide(&s_far_cloud);
    s_far_cloud_ready=0u;
    /* The title borrows the first page without enabling a scrolling layer. */
    ng_sprite_group_cancel(&s_ground.pages[0]);
    ng_sprite_hide_vram_base(NG_SPR_VRAM_BASE(NG_SPR_BG0_FIRST), 20u);
    s_bg_id = 0u;
    s_bg_ready = 0u;
    s_ground_pending[0] = s_ground_pending[1] = 0u;
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
    /* The three pilots' cards stand under their jets. */
    {
        static const int16_t x[3] = {56, 160, 264};
        uint8_t i;
        for (i = 0u; i < 3u; i++)
            sky_spawn(SKY_KIND_FACE, (uint16_t)(SKY_TITLE_CARD_0 + i), x[i], 176,
                      SKY_SCALE_FULL, NG_RENDER_BAND_PLAYER);
        ng_chars_update();
        ng_chars_draw();
    }
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
    for (i = 0u; i < SKY_MAX_PSHOTS + SKY_MAX_ESHOTS; i++) s_shot_owner[i] = 0;
    for (i = 0u; i < SKY_MAX_ENEMIES; i++) s_enemy_owner[i] = 0;
    s_boss_owner = 0;
    /* Bands express flight-layer order; screen Y is not actor depth here.
     * Crossing shots should not reorder the entire same-band sprite pool. */
    ng_chars_set_depth_sort(0u);
    sky_bg_hide();
    for (i = 0u; i <= NG_ASSET_META_COUNT; i++) s_palette_loaded[i] = 0u;
    for (i = 0u; i < 3u; i++) s_instrument_id[i] = 0u;     /* hidden with the rest */
    s_damage_color = 0xffu;
    s_joy = 0u;
    s_joy_prev = 0u;
    s_pressed = 0u;
    ng_fix_invalidate_all();
    ng_palfx_screen_fade_in(0u, 12u);
    waitVbl();
}

uint16_t NEOGEO_USER sky_frame(void)
{
    NG_PERF_FRAME_END();
    sky_video_publish(&s_video_sync);
    /* A stale flag from an overrun is not an acknowledgement of this batch. */
    do { waitVbl(); } while (s_video_sync.pending);
    NG_PERF_FRAME_BEGIN();
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
    char line[41];
    uint8_t i;
    if (!text || x >= 40u || y >= 28u) return;
    for (i = 0u; i < (uint8_t)(40u-x) && text[i]; i++) line[i] = text[i];
    line[i] = '\0';
    ng_fix_puts(x, y, line, pal);
}

void NEOGEO_USER sky_text_box(uint8_t x, uint8_t y, uint8_t width,
                             uint8_t rows, const char *text, uint8_t pal)
{
    char line[41];
    uint8_t row, n, cut;
    if (!text || x >= 40u || y >= 28u || !width) return;
    if (width > 40u-x) width = (uint8_t)(40u-x);
    for (row = 0u; row < rows && y+row < 28u && *text; row++) {
        while (*text == ' ') text++;
        for (n = 0u; n < width && text[n]; n++) line[n] = text[n];
        cut = n;
        if (n == width && text[n] && text[n] != ' ') {
            while (cut && line[cut-1u] != ' ') cut--;
            if (!cut) cut = n;
        }
        line[cut] = '\0';
        sky_puts(x, (uint8_t)(y+row), line, pal);
        text += cut;
    }
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

void NEOGEO_USER sky_meter(uint8_t x, uint8_t y, uint8_t width,
                           uint8_t filled, uint8_t palette)
{
    uint8_t i;
    if (x >= 40u || y >= 28u) return;
    if (width > 40u - x) width = (uint8_t)(40u - x);
    if (filled > width) filled = width;
    for (i = 0u; i < width; i++)
        ng_fix_put_tile((uint8_t)(x + i), y,
                        (uint16_t)(0xce0u + (i < filled ? 1u : 0u)), palette);
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
