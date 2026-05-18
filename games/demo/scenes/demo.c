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
#include "sdk/2d_engine/ng_engine.h"
#include "sdk/2d_engine/ng_render_queue.h"
#include "sdk/2d_engine/ng_scene.h"
#include "sdk/2d_engine/ng_palette_fx.h"
#include "sdk/2d_engine/ng_particles.h"
#include "sdk/2d_engine/ng_camera.h"
#include "sdk/2d_engine/ng_feedback.h"
#include "sdk/2d_engine/ng_sprite_pool.h"
#include "sprite_meta.h"
#include <stdint.h>

const NGArtAsset * NEOGEO_USER ng_screen_art_asset(uint16_t screen_id);

/* ------------------------------------------------------------------ */
/*  External hardware / BIOS functions                                  */
/* ------------------------------------------------------------------ */
void NEOGEO_USER waitVbl(void);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER clearSprs(void);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);
void NEOGEO_USER fixtext_out(uint16_t x, uint16_t y, char *mess, short pal);
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
    bios_edge = (uint8_t)(NEO_REGISTER8(BIOS_P1CHANGE) & (uint8_t)(1u << CNT_A));

    return ((edge & BUTTON_A) || bios_edge) ? 1u : 0u;
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
    char buf[39];
    uint8_t i = 0;

    if (!text || x >= 40u || y >= 28u) return;

    while (text[i] && i < (uint8_t)(38u - x)) {
        buf[i] = text[i];
        i++;
    }
    buf[i] = '\0';
    fixtext_out(x, y, buf, (short)pal);
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
static NGSpriteWindow demo_sprite_windows[8];

static void NEOGEO_USER demo_reset_sprite_window_cache(void)
{
    uint8_t i;

    for (i = 0u; i < 8u; i++) {
        ng_sprite_window_init(&demo_sprite_windows[i], 0u, 0xffffu, NG_SPRITE_MAX_STRIPS);
    }
}

static NGSpriteWindow * NEOGEO_USER demo_sprite_window_find(uint16_t first_sprite)
{
    uint8_t i;
    uint8_t free_slot = 0xffu;

    for (i = 0u; i < 8u; i++) {
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
                              NG_SPRITE_MAX_STRIPS);
        return &demo_sprite_windows[free_slot];
    }

    ng_sprite_window_clear(&demo_sprite_windows[0]);
    ng_sprite_window_init(&demo_sprite_windows[0],
                          1u,
                          first_sprite,
                          NG_SPRITE_MAX_STRIPS);
    return &demo_sprite_windows[0];
}

void NEOGEO_USER demo_clear_all_sprites(void)
{
    clearSprs();
    ng_sprite_hide_all();
    demo_reset_sprite_window_cache();
}

void NEOGEO_USER demo_clear_scene(void)
{
    soundCancelFade();
    soundStopAll();
    soundSceneReset();
    ng_scene_begin(NG_SCENE_CLEAN_DEFAULT, 0u);
    setBACKDROP(BLACK);
    waitVbl();
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
                      ((uint16_t)meta->tile_row_start * 16u) +
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
    NGSpriteGroup g;
    uint8_t meta_strips;
    uint8_t meta_rows;
    NGSpriteWindow *window;

    if (screen_id == 0u) return;
    if (strips == 0u) strips = 1u;
    if (rows   == 0u) rows   = 1u;
    if (strips > 16u) strips = 16u;
    if (rows   > 16u) rows   = 16u;

    meta_strips = demo_screen_strips(screen_id);
    meta_rows = demo_screen_rows(screen_id);
    if (strips > meta_strips) strips = meta_strips;
    if (rows > meta_rows) rows = meta_rows;

    window = demo_sprite_window_find(first_sprite);
    ng_sprite_window_set_current(window, strips);
    ng_sprite_window_clear_tail(window);

    demo_load_screen_palette(screen_id);

    ng_sprite_group_init(&g, first_sprite, strips, meta_rows,
                         DEMO_SCREEN_TILE(screen_id),
                         DEMO_SCREEN_PALETTE(screen_id));
    ng_sprite_group_set_tile_stride(&g, 16u);
    ng_sprite_group_set_active_rows(&g, rows);
    ng_sprite_group_set_pos(&g,
                            (int16_t)(x + demo_screen_x_offset(screen_id)),
                            (int16_t)(y + demo_screen_y_offset(screen_id)));
    ng_sprite_group_set_scale(&g,
                              demo_normalize_x_scale(scale_x),
                              scale_y);
    ng_sprite_group_upload(&g);
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

    demo_unified_run();
}
