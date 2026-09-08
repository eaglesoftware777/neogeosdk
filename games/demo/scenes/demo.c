/*
 * demo.c — top-level demo flow: attract loop, full showcase, MVS/AES branching.
 *
 * https://eaglesoftware.biz
 * https://github.com/eaglesoftware777/neogeosdk
 */

#include "demo.h"
#include "demo_intro.h"
#include "demo_sprites.h"
#include "demo_camera.h"
#include "demo_palette.h"
#include "demo_particles.h"
#include "demo_depth.h"
#include "demo_sound.h"
#include "demo_fix.h"
#include "demo_combat.h"
#include "demo_stress.h"
#include "demo_title.h"
#include "demo_render.h"
#include "demo_unified.h"

#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/ng_fix/ng_fix.h"
#include "sdk/2d_engine/ng_engine.h"
#include "sdk/2d_engine/ng_render_queue.h"
#include "sdk/2d_engine/ng_scene.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "sdk/2d_engine/ng_particles.h"
#include "sdk/2d_engine/ng_camera.h"
#include "sdk/2d_engine/ng_feedback.h"
#include "sdk/2d_engine/ng_sprite_pool.h"
#include "sdk/2d_engine/ng_sprite_hw.h"
#include "sprite_meta.h"
#include <stdint.h>
#ifdef __cplusplus
/* A USE_2D_PLUS build compiles this file as C++.  Everything here is
 * reached from inline asm, the cart entry vectors or the BIOS by its
 * plain symbol name, so it must keep C linkage and not be mangled. */
extern "C" {
#endif


const NGArtAsset * NEOGEO_USER ng_screen_art_asset(uint16_t screen_id);

/* ------------------------------------------------------------------ */
/*  External hardware / BIOS functions                                  */
/* ------------------------------------------------------------------ */
void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER clearSprs(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);
void NEOGEO_USER mess_out_clipped(uint16_t x, uint16_t y, const char *text,
                                  short pal, uint16_t max_chars);
void NEOGEO_USER vram_SCB234(uint16_t SCBADDR, uint16_t SCB234);
void NEOGEO_USER load_palettes(uint16_t *p_palette, uintptr_t palette_offset);
void NEOGEO_USER setpal(uint16_t *pal_tile,
    uint16_t t0, uint16_t t1, uint16_t t2, uint16_t t3,
    uint16_t t4, uint16_t t5, uint16_t t6, uint16_t t7,
    uint16_t t8, uint16_t t9, uint16_t t10, uint16_t t11,
    uint16_t t12, uint16_t t13, uint16_t t14, uint16_t t15);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundCancelFade(void);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER soundSetSSGVolume(uint8_t v);
void NEOGEO_USER soundSetFMVolume(uint8_t v);
void NEOGEO_USER soundFadeOutSpeed(uint8_t speed);
void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playSFXB(uint8_t n);
void NEOGEO_USER playVoiceCue(uint8_t n);
void NEOGEO_USER showEyeCatcherMVS(void);

/* ------------------------------------------------------------------ */
/*  demo_frame: per-frame pump — call from any scene loop               */
/* ------------------------------------------------------------------ */
uint8_t NEOGEO_USER demo_frame(void)
{
    waitVbl();
    ng_render_queue_flush();
    ng_palette_fx_update();
    ng_particles_update();
    ng_feedback_update();
    return demo_advance_requested();
}

/* ------------------------------------------------------------------ */
/*  Input helpers                                                        */
/* ------------------------------------------------------------------ */
uint8_t NEOGEO_USER demo_advance_requested(void)
{
    static uint8_t input_ready;
    static uint16_t prev_joy;
    uint16_t joy = poll_joystick();
    uint16_t edge;
    uint8_t bios_edge;

    if (!input_ready) {
        prev_joy = joy;
        input_ready = 1u;
        return 0u;
    }

    edge = (uint16_t)(joy & (uint16_t)(~prev_joy));
    prev_joy = joy;
    bios_edge = NEO_REGISTER8(BIOS_P1CHANGE);

    /*
     * Two distinct requests share this one edge detector, because it is
     * the only place that owns prev_joy - a second detector elsewhere
     * would race this one for the same button edge and one of them
     * would miss presses.
     *
     *   2 = C, restart the current chapter
     *   1 = A, advance to the next chapter
     *
     * Callers that only care "should this scene stop" keep working
     * unchanged: both codes are simply non-zero.
     */
    if ((edge & BUTTON_C) || (bios_edge & (uint8_t)(1u << CNT_C))) {
        NEO_REGISTER8(BIOS_P1CHANGE) &= (uint8_t)~(1u << CNT_C);
        return 2u;
    }
    if ((edge & BUTTON_A) || (bios_edge & (uint8_t)(1u << CNT_A))) {
        NEO_REGISTER8(BIOS_P1CHANGE) &= (uint8_t)~(1u << CNT_A);
        return 1u;
    }
    return 0u;
}

uint8_t NEOGEO_USER demo_wait(uint16_t frames)
{
    uint16_t i;
    for (i = 0; i < frames; i++) {
        if (demo_frame()) return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  FIX text helpers                                                     */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_fix_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal)
{
    uint16_t max_chars;

    if (!text || x >= 40u || y >= 28u) return;

    max_chars = (uint16_t)(40u - x);
    mess_out_clipped(x, y, text, (short)pal, max_chars);
}

void NEOGEO_USER demo_caption(const char *line1, const char *line2, const char *line3)
{
    clearFix();
    if (line1) demo_fix_puts(2, 1,  line1, 2);
    if (line2) demo_fix_puts(2, 3,  line2, 1);
    if (line3) demo_fix_puts(2, 5,  line3, 0);
    demo_fix_puts(2, 27, "A: NEXT", 1);
}

/* ------------------------------------------------------------------ */
/*  Scene clear                                                          */
/* ------------------------------------------------------------------ */
/* One window per distinct first_sprite slot drawn this scene, used to
 * track each sprite group's previous tile footprint so a shrinking
 * frame correctly clears its old tail (see ng_sprite_window_clear_tail
 * below).  Sized for the busiest chapter's real distinct-slot count,
 * same reasoning as DEMO_SPRITE_QUEUE_MAX above: a 24-enemy formation
 * plus player/bullet/explosion pools uses up to 35 distinct slots in
 * one scene.  When this overflows, demo_sprite_window_find() below
 * forcibly evicts slot 0 for every slot beyond the cap, so multiple
 * unrelated sprites thrash the same cached window within a single
 * frame - corrupting their tail-clear state and producing exactly the
 * "sprites missing / garbled" symptom this cache exists to prevent. */
#define DEMO_SPRITE_WINDOWS 48u

static NGSpriteWindow demo_sprite_windows[DEMO_SPRITE_WINDOWS];
static NGSpriteGroup demo_sprite_groups[DEMO_SPRITE_WINDOWS];
static uint8_t demo_sprite_group_ready[DEMO_SPRITE_WINDOWS];
static uint8_t demo_palette_loaded[256];
static uint8_t demo_sprite_queue_count;

static void NEOGEO_USER demo_reset_sprite_window_cache(void)
{
    uint8_t i;

    for (i = 0u; i < DEMO_SPRITE_WINDOWS; i++) {
        ng_sprite_window_init(&demo_sprite_windows[i], 0u, 0xffffu, 16u);
        demo_sprite_group_ready[i] = 0u;
    }
    for (i = 0u; i < NG_ASSET_META_COUNT; i++) demo_palette_loaded[i + 1u] = 0u;
    demo_sprite_queue_count = 0u;
}

/* Public entry point for chap_header() (demo_unified.c) to call at the
 * start of every chapter.  This cache is otherwise never reset by the
 * normal per-chapter clear path (ng_clear_screen_full() only clears
 * sprite VRAM/SCB state, not this tracking table) - it's global and
 * cumulative across the whole ROM run, only reset via
 * demo_clear_all_sprites()/demo_safe_show(), which chapters don't
 * call.  Every distinct first_sprite value any chapter has ever used
 * this run permanently occupies a slot until evicted, so a run with
 * enough distinct one-off slot numbers across many chapters can starve
 * a later chapter (like the 24-enemy shooter, which alone needs ~35)
 * of free slots, forcing thrashing/corruption before it even gets
 * going.  A plain VRAM clear doesn't touch this table at all, so this
 * has to be called explicitly. */
void NEOGEO_USER demo_sprite_window_cache_reset(void)
{
    demo_reset_sprite_window_cache();
}

static NGSpriteWindow * NEOGEO_USER demo_sprite_window_find(uint16_t first_sprite)
{
    uint8_t i;
    uint8_t free_slot = 0xffu;

    for (i = 0u; i < DEMO_SPRITE_WINDOWS; i++) {
        if (demo_sprite_windows[i].first_slot == first_sprite) return &demo_sprite_windows[i];
        if (free_slot == 0xffu &&
            (demo_sprite_windows[i].first_slot == 0xffffu ||
             (demo_sprite_windows[i].first_slot == 0u &&
              demo_sprite_windows[i].current_strips == 0u))) {
            free_slot = i;
        }
    }

    if (free_slot != 0xffu) {
        ng_sprite_window_init(&demo_sprite_windows[free_slot],
                              1u,
                              first_sprite,
                              16u);
        return &demo_sprite_windows[free_slot];
    }

    return 0;
}

void NEOGEO_USER demo_clear_all_sprites(void)
{
    ng_sprite_hide_all();
    demo_reset_sprite_window_cache();
}

void NEOGEO_USER demo_clear_scene(void)
{
    soundCancelFade();
    soundStopAll();
    soundSceneReset();
    /* ng_scene_begin(..., 0u) below does NOT wait for vblank itself - it
     * clears sprite/FIX VRAM immediately, then returns.  Without this
     * sync first, that clear can straddle active scanout and produce
     * exactly the same "black box flash" the START_GAME fix eliminated
     * for the boot path - demo_clear_scene() is called 60+ times across
     * every scene file (including the very first thing demo_intro_loading()
     * does right after START), so this was the far more common source
     * of it. */
    waitVbl();
    demo_sprite_window_cache_reset();
    ng_render_queue_init();
    /* Wipe on a black page: ng_scene_begin() rewrites FIX and sprite VRAM
     * in bulk, and a light backdrop would let every half-written cell show
     * up as a bright block for the frames the wipe takes.  The real page
     * colour goes back on the next vblank, once the screen is quiet. */
    setBACKDROP(DEMO_BG_CLEAR);
    ng_scene_begin(NG_SCENE_CLEAN_DEFAULT, 0u);
    waitVbl();
    setBACKDROP(DEMO_BG);
}

/* ------------------------------------------------------------------ */
/*  Screen helpers                                                       */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_safe_show(DemoShowScreenFn fn,
                                int x0, int y0,
                                int xr, int yr,
                                int min_crt_sz,
                                uint16_t backdrop,
                                uint16_t sprite_base)
{
    if (!fn) return;
    if (sprite_base == 0u) sprite_base = DEMO_SHOWSCREEN_BASE;
    ng_sprite_hide_vram_base(sprite_base, 32u);
    demo_reset_sprite_window_cache();
    fn(x0, y0, xr, yr, min_crt_sz, backdrop, sprite_base);
}

static const NGSpriteAssetMeta * NEOGEO_USER demo_screen_meta(uint8_t screen_id)
{
    uint16_t idx;
    if (screen_id == 0u) return 0;
    idx = (uint16_t)(screen_id - 1u);
    if (idx >= NG_ASSET_META_COUNT) return 0;
    return &g_ng_asset_meta[idx];
}

static const NGArtAsset * NEOGEO_USER demo_screen_asset(uint8_t screen_id)
{
    if (screen_id == 0u) return 0;
    return ng_screen_art_asset(screen_id);
}

uint16_t NEOGEO_USER demo_screen_tile(uint8_t screen_id)
{
    const NGArtAsset *asset = demo_screen_asset(screen_id);
    const NGSpriteAssetMeta *meta = demo_screen_meta(screen_id);
    if (asset) return asset->tile_base;
    if (screen_id == 0u) return 0u;
    if (!meta) return (uint16_t)(((uint16_t)(screen_id - 1u)) * 256u);
    return (uint16_t)(meta->tile_base +
                      ((uint16_t)meta->tile_row_start * meta->tile_stride) +
                      meta->tile_col_start);
}

uint8_t NEOGEO_USER demo_screen_palette(uint8_t screen_id)
{
    const NGArtAsset *asset = demo_screen_asset(screen_id);
    const NGSpriteAssetMeta *meta = demo_screen_meta(screen_id);
    if (asset) return asset->palette_bank;
    if (screen_id == 0u) return 0u;
    if (!meta) return (uint8_t)(0x10u + (screen_id - 1u));
    return meta->palette_bank;
}

uint8_t NEOGEO_USER demo_screen_strips(uint8_t screen_id)
{
    const NGArtAsset *asset = demo_screen_asset(screen_id);
    const NGSpriteAssetMeta *meta = demo_screen_meta(screen_id);
    if (asset && asset->strips != 0u) return asset->strips;
    if (!meta || meta->strips == 0u) return 1u;
    return meta->strips;
}

/*
 * Tiles per row of the asset's canvas.
 *
 * Row n of the artwork sits this many tiles after row n-1, so anything
 * bound to the asset has to use it as the tile stride.  It is not always
 * 16 - that is just the stride of a 256 px canvas.  Assuming 16 for an
 * asset imported narrower reads each row from further along the C ROM
 * than the artwork actually is, which draws the right palette over
 * whatever happens to live there.
 */
uint8_t NEOGEO_USER demo_screen_tile_stride(uint8_t screen_id)
{
    const NGSpriteAssetMeta *meta = demo_screen_meta(screen_id);
    if (!meta || meta->tile_stride == 0u) return 16u;
    return meta->tile_stride;
}

/*
 * Convert a "fraction of a 256 px import" scale into the hardware scale
 * this particular asset needs to reach that size on screen.
 *
 * Assets are imported at the size they are drawn, so they do not all
 * arrive on the same canvas: a character imported at 128 needs twice the
 * hardware scale of one imported at 256 to cover the same pixels.  The
 * canvas width in tiles is the asset's tile stride, so that is what the
 * conversion divides by.
 *
 * Scale bytes count sixteenths in their top nibble and must keep the low
 * nibble at F, or the two axes disagree - see NG_SCALE().
 */
uint8_t NEOGEO_USER demo_asset_scale(uint8_t screen_id, uint8_t scale_256)
{
    uint16_t stride = demo_screen_tile_stride(screen_id);
    uint16_t sixteenths;

    if (stride == 0u || stride == 16u) return scale_256;

    sixteenths = (uint16_t)(((uint16_t)(scale_256 >> 4) + 1u) * 16u / stride);
    if (sixteenths < 1u)  sixteenths = 1u;
    if (sixteenths > 16u) sixteenths = 16u;
    return (uint8_t)(((sixteenths - 1u) << 4) | 0x0Fu);
}

uint8_t NEOGEO_USER demo_screen_rows(uint8_t screen_id)
{
    const NGArtAsset *asset = demo_screen_asset(screen_id);
    const NGSpriteAssetMeta *meta = demo_screen_meta(screen_id);
    if (asset && asset->active_rows != 0u) return asset->active_rows;
    if (!meta || meta->active_rows == 0u) return 1u;
    return meta->active_rows;
}

int16_t NEOGEO_USER demo_screen_x_offset(uint8_t screen_id)
{
    const NGArtAsset *asset = demo_screen_asset(screen_id);
    const NGSpriteAssetMeta *meta = demo_screen_meta(screen_id);
    if (asset) return asset->offset_x;
    if (!meta) return 0;
    return (int16_t)((uint16_t)meta->tile_col_start * 16u);
}

int16_t NEOGEO_USER demo_screen_y_offset(uint8_t screen_id)
{
    const NGArtAsset *asset = demo_screen_asset(screen_id);
    const NGSpriteAssetMeta *meta = demo_screen_meta(screen_id);
    if (asset) return asset->offset_y;
    if (!meta) return 0;
    return (int16_t)((uint16_t)meta->tile_row_start * 16u);
}

uint8_t NEOGEO_USER demo_screen_x_pad(uint8_t screen_id)
{
    const NGSpriteAssetMeta *meta = demo_screen_meta(screen_id);
    if (!meta) return 0u;
    return meta->x_pad;
}

uint8_t NEOGEO_USER demo_screen_y_pad(uint8_t screen_id)
{
    const NGSpriteAssetMeta *meta = demo_screen_meta(screen_id);
    if (!meta) return 0u;
    return meta->y_pad;
}

uint16_t NEOGEO_USER demo_screen_content_width(uint8_t screen_id)
{
    const NGSpriteAssetMeta *meta = demo_screen_meta(screen_id);
    if (!meta) return 0u;
    return meta->content_width;
}

uint16_t NEOGEO_USER demo_screen_content_height(uint8_t screen_id)
{
    const NGSpriteAssetMeta *meta = demo_screen_meta(screen_id);
    if (!meta) return 0u;
    return meta->content_height;
}

static int16_t NEOGEO_USER demo_scale_px_u16(uint16_t px, uint8_t scale)
{
    if (scale >= 0xFFu) return (int16_t)px;
    return (int16_t)(((uint32_t)px * (uint32_t)scale + 127u) >> 8);
}

void NEOGEO_USER demo_anchor_bottom_center(uint8_t screen_id,
                                           uint8_t scale_x,
                                           uint8_t scale_y,
                                           int16_t cx,
                                           int16_t cy,
                                           int16_t *out_x,
                                           int16_t *out_y)
{
    const NGSpriteAssetMeta *meta = demo_screen_meta(screen_id);
    uint16_t pad_x;
    uint16_t pad_y;
    int16_t  off_x;
    int16_t  off_y;

    if (!meta) {
        if (out_x) *out_x = cx;
        if (out_y) *out_y = cy;
        return;
    }

    /* x_pad + content_width/2 is the per-frame offset from the
     * top-left of the artwork's used tile region to the centre of
     * the actual painted artwork.  y_pad + content_height is the
     * offset to the artwork's bottom edge. Both vary frame-to-frame
     * across an animation cycle, but together they describe the
     * STABLE position of the art inside whatever tile bounding box
     * the artist drew this frame in, so anchoring on them removes
     * the per-frame jitter the demo was exhibiting. */
    pad_x = (uint16_t)meta->x_pad + (meta->content_width >> 1);
    pad_y = (uint16_t)meta->y_pad + meta->content_height;

    /* The tile-grid origin offset (tile_col_start * 16) is added
     * UNSCALED by demo_draw_sprite_screen when computing the SCB4
     * X position, so we subtract it unscaled here too.  Only the
     * in-tile component lives inside the shrinking sprite, so only
     * that part is multiplied by the SCB2 scale factor. */
    off_x = demo_scale_px_u16(pad_x, scale_x);
    off_y = demo_scale_px_u16(pad_y, scale_y);

    if (out_x) *out_x = (int16_t)(cx
                                  - (int16_t)((uint16_t)meta->tile_col_start * 16u)
                                  - off_x);
    if (out_y) *out_y = (int16_t)(cy
                                  - (int16_t)((uint16_t)meta->tile_row_start * 16u)
                                  - off_y);
}

/* Forward-declare all showScreenN functions needed for palette preload */
void NEOGEO_USER showScreen1(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen2(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen3(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen4(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen5(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen6(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen7(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen8(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen9(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen10(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen11(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen12(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen13(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen14(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen15(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen16(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen17(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen18(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen19(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen20(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen21(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen22(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen23(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen24(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen25(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen26(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen27(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen28(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen29(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen30(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen31(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen32(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen33(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen34(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen35(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen36(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen37(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen38(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen39(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen40(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen41(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen42(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen43(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen44(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen45(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen46(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen47(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen48(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen49(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen50(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen51(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen52(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen53(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen54(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen55(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen56(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen57(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen58(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen59(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen60(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen61(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen62(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen63(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen64(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen65(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen66(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen67(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen68(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen69(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen70(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen71(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen72(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen73(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen74(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen75(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen76(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen77(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen78(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen79(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen80(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen81(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen82(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen83(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen84(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen85(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen86(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen87(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen88(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen89(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen90(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen91(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen92(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen107(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen108(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen109(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);

/* ------------------------------------------------------------------ */
/*  Palette preload helper (off-screen load for palette-only priming)    */
/* ------------------------------------------------------------------ */
extern const DemoShowScreenFn ng_screen_table[];
extern const uint16_t ng_screen_count;
uint8_t NEOGEO_USER ng_load_screen_palette(uint16_t screen_id);

void NEOGEO_USER demo_load_screen_palette(uint8_t screen_id)
{
    if (screen_id == 0u || screen_id > ng_screen_count) return;
    ng_load_screen_palette(screen_id);
    demo_palette_loaded[screen_id] = 1u;
}

/* ------------------------------------------------------------------ */
/*  Sprite group draw helper                                             */
/* ------------------------------------------------------------------ */
static uint8_t NEOGEO_USER demo_normalize_x_scale(uint8_t scale_x)
{
    /*
     * Generated showScreen() calls use 0x0F nibble for full-width.
     * NGSpriteGroup uses 0xFF.  Remap 4-bit → 8-bit so groups render correctly.
     */
    if (scale_x <= 0x0Fu) {
        return (uint8_t)((scale_x << 4) | scale_x);
    }
    return scale_x;
}

void NEOGEO_USER demo_draw_sprite_screen(uint8_t screen_id,
                                         uint16_t first_sprite,
                                         int16_t x, int16_t y,
                                         uint8_t strips, uint8_t rows,
                                         uint8_t scale_x, uint8_t scale_y)
{
    demo_draw_sprite_screen_flip(screen_id, first_sprite, x, y,
                                 strips, rows, scale_x, scale_y, 0u);
}

/*
 * Deferred sprite write queue.
 *
 * NeoGeo sprite tearing comes from writing SCB1/2/3/4 while the LSPC
 * is reading them mid-frame.  The chapter loops in demo_unified.c
 * call demo_draw_sprite_screen during ACTIVE VIDEO (before uframe's
 * waitVbl), so every issued upload had a chance to be partly read by
 * the sprite chip — that's the "char splits / old sprite still
 * shows when char walks" the demo has been exhibiting in every
 * chapter with a moving main character.
 *
 * Fix: demo_draw_sprite_screen{,_flip} now only RECORDS the request
 * here.  demo_flush_sprite_queue() drains the queue inside the
 * vblank window (called by uframe in demo_unified.c right after
 * waitVbl, before ng_render_queue_flush), so every SCB write
 * completes while the screen is blanked.
 *
 * Sized for the busiest chapter's worst-case per-frame draw count, not
 * just "the common case" - a full 24-enemy Galaxian-style formation
 * plus player/bullet/explosion pools needs up to 35 queued draws in a
 * single frame.  Whenever the queue is full, callers silently fall
 * back to an immediate, unsynced write (see the overflow branch below)
 * which reintroduces the exact tearing this queue exists to prevent -
 * so this bound must stay above any real chapter's per-frame draw
 * count, not just comfortably above "1..4 sprite groups".
 */
#define DEMO_SPRITE_QUEUE_MAX 48u

typedef struct {
    uint8_t  active;
    uint8_t  screen_id;
    uint16_t first_sprite;
    int16_t  x;
    int16_t  y;
    uint8_t  strips;
    uint8_t  rows;
    uint8_t  scale_x;
    uint8_t  scale_y;
    uint8_t  hflip;
    uint8_t  tile_y;
} DemoSpriteDraw;

static DemoSpriteDraw demo_sprite_queue[DEMO_SPRITE_QUEUE_MAX];

static void NEOGEO_USER demo_perform_sprite_draw(const DemoSpriteDraw *cmd)
{
    NGSpriteGroup *g;
    uint8_t meta_strips;
    uint8_t meta_rows;
    uint8_t strips;
    uint8_t rows;
    NGSpriteWindow *window;
    uint8_t index;
    uint16_t tile_base;

    if (!cmd || cmd->first_sprite == 0u || cmd->first_sprite >= NG_SPR_TOTAL) return;
    if (cmd->screen_id == 0u) {
        uint16_t end = (uint16_t)(cmd->first_sprite + cmd->strips);
        ng_sprite_hide_range(cmd->first_sprite, cmd->strips);
        for (index = 0u; index < DEMO_SPRITE_WINDOWS; index++) {
            window = &demo_sprite_windows[index];
            if (window->first_slot < end &&
                window->first_slot + window->current_strips > cmd->first_sprite) {
                demo_sprite_group_ready[index] = 0u;
            }
        }
        return;
    }
    if (cmd->screen_id > ng_screen_count) return;

    strips = cmd->strips ? cmd->strips : 1u;
    rows   = cmd->rows   ? cmd->rows   : 1u;
    if (strips > 16u) strips = 16u;
    if (rows   > 16u) rows   = 16u;

    meta_strips = demo_screen_strips(cmd->screen_id);
    meta_rows   = demo_screen_rows(cmd->screen_id);
    if (strips > meta_strips) strips = meta_strips;

    /* An asset owns meta_rows of tiles and no more.  A scrolled draw
     * starts tile_y rows into it, so what is left from there is the
     * ceiling for the map as well as for the row count - reaching past
     * it addresses the next asset's tiles. */
    if (cmd->tile_y >= meta_rows) return;
    meta_rows = (uint8_t)(meta_rows - cmd->tile_y);
    if (rows > meta_rows) rows = meta_rows;

    window = demo_sprite_window_find(cmd->first_sprite);
    if (!window || cmd->first_sprite + strips > NG_SPR_TOTAL) return;
    index = (uint8_t)(window - demo_sprite_windows);
    g = &demo_sprite_groups[index];
    ng_sprite_window_set_shape(window, strips, rows);
    ng_sprite_window_clear_tail(window);

    if (!demo_palette_loaded[cmd->screen_id]) demo_load_screen_palette(cmd->screen_id);

    tile_base = ng_sprite_row_tile(DEMO_SCREEN_TILE(cmd->screen_id),
                                   demo_screen_tile_stride(cmd->screen_id), cmd->tile_y);

    if (!demo_sprite_group_ready[index] || g->strips != strips || g->heightTiles != meta_rows) {
        ng_sprite_group_init(g, cmd->first_sprite, strips, meta_rows,
                             tile_base,
                             DEMO_SCREEN_PALETTE(cmd->screen_id));
        demo_sprite_group_ready[index] = 1u;
    } else {
        ng_sprite_group_set_tile_base(g, tile_base);
        ng_sprite_group_set_palette(g, DEMO_SCREEN_PALETTE(cmd->screen_id));
    }
    ng_sprite_group_set_tile_stride(g, demo_screen_tile_stride(cmd->screen_id));
    ng_sprite_group_set_active_rows(g, rows);
    ng_sprite_group_set_pos(g,
                            (int16_t)(cmd->x + demo_screen_x_offset(cmd->screen_id)),
                            (int16_t)(cmd->y + demo_screen_y_offset(cmd->screen_id)));
    ng_sprite_group_set_scale(g,
                              demo_normalize_x_scale(
                                  demo_asset_scale(cmd->screen_id, cmd->scale_x)),
                              demo_asset_scale(cmd->screen_id, cmd->scale_y));
    ng_sprite_group_set_flip(g, cmd->hflip, 0u);
    ng_sprite_group_flush(g);
}

void NEOGEO_USER demo_flush_sprite_queue(void)
{
    uint8_t i;
    for (i = 0u; i < demo_sprite_queue_count; i++) {
        if (demo_sprite_queue[i].active) {
            demo_perform_sprite_draw(&demo_sprite_queue[i]);
            demo_sprite_queue[i].active = 0u;
        }
    }
    demo_sprite_queue_count = 0u;
}

void NEOGEO_USER demo_draw_sprite_screen_flip(uint8_t screen_id,
                                              uint16_t first_sprite,
                                              int16_t x, int16_t y,
                                              uint8_t strips, uint8_t rows,
                                              uint8_t scale_x, uint8_t scale_y,
                                              uint8_t hflip)
{
    demo_draw_sprite_screen_scroll(screen_id, first_sprite, x, y, strips, rows, scale_x, scale_y, hflip, 0u);
}

void NEOGEO_USER demo_draw_sprite_screen_scroll(uint8_t screen_id,
                                                uint16_t first_sprite,
                                                int16_t x, int16_t y,
                                                uint8_t strips, uint8_t rows,
                                                uint8_t scale_x, uint8_t scale_y,
                                                uint8_t hflip, uint8_t tile_y)
{
    DemoSpriteDraw *slot;

    if (first_sprite == 0u || first_sprite >= NG_SPR_TOTAL) return;

    /* Coalesce: if the queue already has an entry for this slot this
     * frame, overwrite it.  Chapters that call draw_asset_bottom_center
     * twice (rare but possible) take the last one, which matches the
     * old direct-write behaviour. */
    {
        uint8_t i;
        for (i = 0u; i < demo_sprite_queue_count; i++) {
            if (demo_sprite_queue[i].active &&
                demo_sprite_queue[i].first_sprite == first_sprite) {
                slot = &demo_sprite_queue[i];
                goto fill;
            }
        }
    }

    if (demo_sprite_queue_count >= DEMO_SPRITE_QUEUE_MAX) {
        /* Keep the last complete frame if the caller exceeds its budget. */
        return;
    }

    slot = &demo_sprite_queue[demo_sprite_queue_count++];

fill:
    slot->active       = 1u;
    slot->screen_id    = screen_id;
    slot->first_sprite = first_sprite;
    slot->x            = x;
    slot->y            = y;
    slot->strips       = strips;
    slot->rows         = rows;
    slot->scale_x      = scale_x;
    slot->scale_y      = scale_y;
    slot->hflip        = hflip;
    slot->tile_y       = tile_y;
}

void NEOGEO_USER demo_hide_sprite_range(uint16_t first, uint8_t strips)
{
    demo_draw_sprite_screen_flip(0u, first, 0, 0, strips, 0u, 0u, 0u, 0u);
}

/* ------------------------------------------------------------------ */
/*  Legacy entry points (user.c / main.c compatibility)                 */
/* ------------------------------------------------------------------ */
void NEOGEO_USER showEagleIntro(void)
{
    demo_intro_eagle();
}

void NEOGEO_USER showGameOver(void)
{
    demo_title_end_card();
}

void NEOGEO_USER showTitleScreen(void)
{
    demo_title_screen();
}

void NEOGEO_USER showCharacterParade(void)
{
    demo_sprites_parade();
}

void NEOGEO_USER showWalkDemo(int loops, int delay_frames)
{
    demo_sprites_walk(loops, delay_frames);
}

/* ------------------------------------------------------------------ */
/*  Attract mode (short, loops)                                          */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_run_attract(void)
{
    soundSceneReset();
    soundSetADPCMAVolume(0x3Cu);
    soundSetADPCMBVolume(0xBCu);

    demo_intro_sdk_title();
    demo_intro_system_banner();
    demo_title_attract_reel();
}

/* ------------------------------------------------------------------ */
/*  Full showcase (after START_GAME or auto-advance)                    */
/* ------------------------------------------------------------------ */
void NEOGEO_USER demo_run_full_flow(void)
{
    ng_render_queue_init();
    ng_palette_fx_init();
    ng_particles_init();
    ng_feedback_init();
    ng_debug_init();

    soundSceneReset();
    soundSetADPCMAVolume(0x3Cu);
    soundSetADPCMBVolume(0xB8u);
    soundSetSSGVolume(0x08u);
    soundSetFMVolume(0x08u);

    playSFX(SOUND_SFX_3);
    demo_wait(20u);
    demo_intro_loading();
    demo_unified_run();
}

#ifdef __cplusplus
}  /* extern "C" */
#endif
