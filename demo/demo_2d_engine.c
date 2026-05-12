#include "demo_2d_engine.h"
#include "demo_screen.h"
#include "sdk/neogeo.h"
#include "sdk/sound_ids.h"
#include "sdk/2d_engine/ng_engine.h"
#include "artbox/sprite_meta.h"
#include <stdint.h>

#if defined(__GNUC__)
#define DEMO_MAYBE_UNUSED __attribute__((unused))
#else
#define DEMO_MAYBE_UNUSED
#endif

void NEOGEO_USER soundPlayGameLoop(uint8_t music_track);
void NEOGEO_USER soundSceneReset(void);
void NEOGEO_USER soundStopAll(void);
void NEOGEO_USER soundSetADPCMAVolume(uint8_t v);
void NEOGEO_USER soundSetADPCMBVolume(uint8_t v);
void NEOGEO_USER soundSetSSGVolume(uint8_t v);
void NEOGEO_USER soundSetFMVolume(uint8_t v);
void NEOGEO_USER playVoiceCue(uint8_t n);
void NEOGEO_USER playSFX(uint8_t n);
void NEOGEO_USER playSFXB(uint8_t n);
void NEOGEO_USER playMusic(uint8_t n);
void NEOGEO_USER setpal(uint16_t *pal_tile, uint16_t t0, uint16_t t1, uint16_t t2, uint16_t t3, uint16_t t4, uint16_t t5, uint16_t t6, uint16_t t7, uint16_t t8, uint16_t t9, uint16_t t10, uint16_t t11, uint16_t t12, uint16_t t13, uint16_t t14, uint16_t t15);
void NEOGEO_USER load_palettes(uint16_t *p_palette, uintptr_t palette_offset);
void NEOGEO_USER clearFix(void);
void NEOGEO_USER clearSprs(void);
void NEOGEO_USER waitVbl(void);
void NEOGEO_USER cyclexms(int cycxms);
void NEOGEO_USER fixtext_out(uint16_t x, uint16_t y, char *mess, short pal);
void NEOGEO_USER setBACKDROP(uint16_t backdrop_color);

typedef void (*SceneScreenFn)(int, int, int, int, int, uint16_t, uint16_t);

#define DEMO_SCREEN_TILE(screen_id) ((uint16_t)(((screen_id) - 1u) * 256u))
#define DEMO_SCREEN_PALETTE(screen_id) ((uint8_t)(0x10u + ((screen_id) - 1u)))
#define DEMO_FRAME(screen_id, ticks) PALETTE(DEMO_SCREEN_PALETTE(screen_id)), FRAME(DEMO_SCREEN_TILE(screen_id), ticks)
#define DEMO_SPRITE_SCREEN(sprite_id) ((uint8_t)((sprite_id) + 10u))
#define DEMO_SPRITE_TILE(sprite_id) DEMO_SCREEN_TILE(DEMO_SPRITE_SCREEN(sprite_id))
#define DEMO_SPRITE_PALETTE(sprite_id) DEMO_SCREEN_PALETTE(DEMO_SPRITE_SCREEN(sprite_id))
#define DEMO_SPRITE_FRAME(sprite_id, ticks) PALETTE(DEMO_SPRITE_PALETTE(sprite_id)), FRAME(DEMO_SPRITE_TILE(sprite_id), ticks)

/*
 * Runtime palette stabilization.
 * Each sprite_*.png was generated with its own palette bank, so changing
 * action frames can make the same character change color.  Until Artbox
 * exports one shared character palette, draw the main character using
 * sprite_001's palette and NPCs using z_npc_84's palette.
 */
#define DEMO_MAIN_MASTER_PALETTE DEMO_SPRITE_PALETTE(1)
#define DEMO_NPC_MASTER_PALETTE  DEMO_SCREEN_PALETTE(94)


enum {
    DEMO_KIND_WARRIOR = 1,
    DEMO_KIND_MASCOT,
    DEMO_KIND_FX,
    DEMO_KIND_GALLERY,
    DEMO_KIND_CAST_LEFT,
    DEMO_KIND_CAST_RIGHT,
    DEMO_KIND_PORTRAIT,
    DEMO_KIND_NPC_CENTER,
    DEMO_KIND_NPC_LEFT,
    DEMO_KIND_NPC_RIGHT
};

enum {
    DEMO_NPC_KIND_CENTER = 1,
    DEMO_NPC_KIND_LEFT,
    DEMO_NPC_KIND_RIGHT
};

enum {
    DEMO_ACT_WARRIOR_WALK = 1,
    DEMO_ACT_WARRIOR_ATTACK_A,
    DEMO_ACT_WARRIOR_ATTACK_B,
    DEMO_ACT_WARRIOR_IDLE,
    DEMO_ACT_WARRIOR_RUN,
    DEMO_ACT_WARRIOR_JUMP,
    DEMO_ACT_WARRIOR_HIT,
    DEMO_ACT_MASCOT_FLOAT,
    DEMO_ACT_FX_SLASH,
    DEMO_ACT_PORTRAIT_END,
    DEMO_ACT_WARRIOR_FIRE,
    DEMO_ACT_WARRIOR_FIRE2,
    DEMO_ACT_WARRIOR_CLOSE,
    DEMO_ACT_WARRIOR_VICTORY,
    DEMO_ACT_WARRIOR_SLIDE,
    DEMO_ACT_WARRIOR_RESPECT,
    DEMO_ACT_WARRIOR_SUPER_KICK,
    DEMO_ACT_ITEM_COLLECTED
};

enum {
    DEMO_EVT_GATE_CENTER = 1,
    DEMO_EVT_ATTACK_IMPACT,
    DEMO_EVT_GALLERY_DONE,
    DEMO_EVT_PLAYER_HURT
};

enum {
    DEMO_TIMER_WALK = 0,
    DEMO_TIMER_DUEL,
    DEMO_TIMER_HIT_LOCK,
    DEMO_TIMER_GALLERY_STEP,
    DEMO_TIMER_END_HOLD,
    DEMO_TIMER_PLAYER_HURT
};

enum {
    DEMO_PROGRESS_PHASE = 0,
    DEMO_PROGRESS_GALLERY
};

enum {
    DEMO_STATUS_GATE_CROSSED = 0,
    DEMO_STATUS_DUEL_ACTIVE,
    DEMO_STATUS_GALLERY_ACTIVE,
    DEMO_STATUS_DONE,
    DEMO_STATUS_FLASH,
    DEMO_STATUS_PLAYER_INPUT
};

static const uint8_t DEMO_MAYBE_UNUSED g_demo_gallery_frames[] = {
    74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87
};

static const uint8_t DEMO_MAYBE_UNUSED g_demo_cast_left_frames[] = {
    87, 88, 89, 90, 91, 92, 93
};

static const uint8_t DEMO_MAYBE_UNUSED g_demo_cast_right_frames[] = {
    84, 83, 82, 81, 80, 79
};

static const uint8_t g_demo_npc_front_frames[] = {
    94, 95, 96, 97, 96, 95
};

static const uint8_t g_demo_npc_walk_frames[] = {
    98, 99, 100, 101, 102, 103, 104, 105
};

#define DEMO_GALLERY_BASELINE DEMO_REEL_BASELINE
#define DEMO_PORTRAIT_BASELINE DEMO_REEL_BASELINE
#define DEMO_LATE_SCENE_BG 106

/*
 * Keep the gameplay actors in the lower display area so the FIX text overlay
 * at the top never covers heads or upper bodies.
 */
#define DEMO_WALK_BASELINE 88
#define DEMO_DUEL_BASELINE 96
#define DEMO_REEL_BASELINE 88
#define DEMO_NPC_BASELINE 132
#define DEMO_INTRO_WALK_REPEATS 2
#define DEMO_INTRO_WALK_HOLD 18

static const SceneScreenFn g_demo_duel_palette_loaders[] = {
    showScreen20, showScreen21, showScreen22, showScreen23, showScreen24,
    showScreen25, showScreen26, showScreen27, showScreen28, showScreen29,
    showScreen30, showScreen31, showScreen32, showScreen33, showScreen34,
    showScreen35, showScreen36, showScreen37, showScreen38, showScreen39,
    showScreen40, showScreen41, showScreen42, showScreen43, showScreen44,
    showScreen45, showScreen46, showScreen47, showScreen48, showScreen49,
    showScreen50, showScreen51, showScreen52, showScreen53, showScreen54,
    showScreen55, showScreen56, showScreen57, showScreen58, showScreen59,
    showScreen60, showScreen61, showScreen62
};

static const SceneScreenFn DEMO_MAYBE_UNUSED g_demo_gallery_palette_loaders[] = {
    showScreen13, showScreen14, showScreen15, showScreen16, showScreen17,
    showScreen18, showScreen19, showScreen20, showScreen21, showScreen23,
    showScreen24, showScreen25, showScreen26, showScreen27, showScreen28,
    showScreen29, showScreen30, showScreen31, showScreen34, showScreen35,
    showScreen36, showScreen37, showScreen38, showScreen39
};

static const SceneScreenFn DEMO_MAYBE_UNUSED g_demo_npc_palette_loaders[] = {
    showScreen94, showScreen95, showScreen96, showScreen97,
    showScreen98, showScreen99, showScreen100, showScreen101,
    showScreen102, showScreen103, showScreen104, showScreen105
};

static const SceneScreenFn g_demo_all_actor_palette_loaders[] = {
    showScreen11, showScreen12, showScreen13, showScreen14, showScreen15,
    showScreen16, showScreen17, showScreen18, showScreen19, showScreen20,
    showScreen21, showScreen22, showScreen23, showScreen24, showScreen25,
    showScreen26, showScreen27, showScreen28, showScreen29, showScreen30,
    showScreen31, showScreen32, showScreen33, showScreen34, showScreen35,
    showScreen36, showScreen37, showScreen38, showScreen39, showScreen40,
    showScreen41, showScreen42, showScreen43, showScreen44, showScreen45,
    showScreen46, showScreen47, showScreen48, showScreen49, showScreen50,
    showScreen51, showScreen52, showScreen53, showScreen54, showScreen55,
    showScreen56, showScreen57, showScreen58, showScreen59, showScreen60,
    showScreen61, showScreen62, showScreen63, showScreen64, showScreen65,
    showScreen66, showScreen67, showScreen68, showScreen69, showScreen70,
    showScreen71, showScreen72, showScreen73, showScreen74, showScreen75,
    showScreen76, showScreen77, showScreen78, showScreen79, showScreen80,
    showScreen81, showScreen82, showScreen83, showScreen84, showScreen85,
    showScreen86, showScreen87, showScreen88, showScreen89, showScreen90,
    showScreen91, showScreen92, showScreen93
};

static const SceneScreenFn g_demo_warrior_palette_loaders[] = {
    showScreen11, showScreen12, showScreen13, showScreen14, showScreen15,
    showScreen16, showScreen17, showScreen18, showScreen19
};

static const SceneScreenFn g_demo_mascot_palette_loaders[] = {
    showScreen79, showScreen80, showScreen81, showScreen82, showScreen83,
    showScreen84, showScreen85, showScreen86, showScreen87, showScreen88,
    showScreen89, showScreen90, showScreen91, showScreen92
};

static const NGActionCmd g_demo_warrior_walk[] = {
    SPEED(0, 0),
    DEMO_SPRITE_FRAME(1, 5), SFX(SOUND_SFX_FOOTSTEP),
    DEMO_SPRITE_FRAME(2, 5),
    DEMO_SPRITE_FRAME(3, 5),
    DEMO_SPRITE_FRAME(4, 5),
    DEMO_SPRITE_FRAME(5, 5), SFX(SOUND_SFX_FOOTSTEP),
    DEMO_SPRITE_FRAME(6, 5),
    DEMO_SPRITE_FRAME(7, 5),
    DEMO_SPRITE_FRAME(8, 5),
    DEMO_SPRITE_FRAME(9, 5), SFX(SOUND_SFX_FOOTSTEP),
    DEMO_SPRITE_FRAME(10, 5),
    DEMO_SPRITE_FRAME(11, 5),
    LOOP()
};

static const NGActionCmd g_demo_warrior_attack_a[] = {
    SPEED(0, 0),
    DEMO_SPRITE_FRAME(12, 2),
    DEMO_SPRITE_FRAME(13, 2),
    DEMO_SPRITE_FRAME(14, 2),
    DEMO_SPRITE_FRAME(15, 2),
    DEMO_SPRITE_FRAME(16, 2),
    DEMO_SPRITE_FRAME(17, 2), SFX(SOUND_SFX_BLADE_WHOOSH),
    HITBOX(92, 72, 68, 58),
    DEMO_SPRITE_FRAME(18, 2),
    DEMO_SPRITE_FRAME(19, 2),
    DEMO_SPRITE_FRAME(20, 2),
    DEMO_SPRITE_FRAME(21, 2),
    DEMO_SPRITE_FRAME(22, 2),
    DEMO_SPRITE_FRAME(23, 2),
    DEMO_SPRITE_FRAME(24, 3), SFX(SOUND_SFX_IMPACT_HIT),
    CLEAR_HITBOX(),
    GOTO(DEMO_ACT_WARRIOR_IDLE)
};

static const NGActionCmd g_demo_warrior_attack_b[] = {
    SPEED(0, 0),
    DEMO_SPRITE_FRAME(25, 3),
    DEMO_SPRITE_FRAME(26, 3), SFX(SOUND_SFX_BLADE_WHOOSH),
    DEMO_SPRITE_FRAME(27, 3),
    DEMO_SPRITE_FRAME(28, 3),
    DEMO_SPRITE_FRAME(29, 3),
    DEMO_SPRITE_FRAME(30, 5), SFX(SOUND_SFX_IMPACT_HIT),
    GOTO(DEMO_ACT_WARRIOR_IDLE)
};

static const NGActionCmd g_demo_warrior_idle[] = {
    SPEED(0, 0),
    DEMO_SPRITE_FRAME(1, 12),
    DEMO_SPRITE_FRAME(2, 12),
    LOOP()
};

static const NGActionCmd g_demo_warrior_run[] = {
    SPEED(0, 0),
    DEMO_SPRITE_FRAME(1, 3), SFX(SOUND_SFX_FOOTSTEP),
    DEMO_SPRITE_FRAME(2, 3),
    DEMO_SPRITE_FRAME(3, 3),
    DEMO_SPRITE_FRAME(4, 3),
    DEMO_SPRITE_FRAME(5, 3), SFX(SOUND_SFX_FOOTSTEP),
    DEMO_SPRITE_FRAME(6, 3),
    DEMO_SPRITE_FRAME(7, 3),
    DEMO_SPRITE_FRAME(8, 3),
    DEMO_SPRITE_FRAME(9, 3),
    DEMO_SPRITE_FRAME(10, 3),
    DEMO_SPRITE_FRAME(11, 3),
    LOOP()
};

static const NGActionCmd g_demo_warrior_jump[] = {
    SPEED(0, 0),
    DEMO_SPRITE_FRAME(31, 4), SFX(SOUND_SFX_START_SLASH),
    DEMO_SPRITE_FRAME(32, 4),
    DEMO_SPRITE_FRAME(33, 6), SFX(SOUND_SFX_IMPACT_HIT),
    GOTO(DEMO_ACT_WARRIOR_IDLE)
};

static const NGActionCmd g_demo_warrior_hit[] = {
    SPEED(0, 0),
    DEMO_SPRITE_FRAME(57, 5),
    DEMO_SPRITE_FRAME(58, 5),
    DEMO_SPRITE_FRAME(59, 5),
    DEMO_SPRITE_FRAME(60, 5),
    DEMO_SPRITE_FRAME(61, 8),
    GOTO(DEMO_ACT_WARRIOR_IDLE)
};

static const NGActionCmd g_demo_mascot_float[] = {
    SPEED(-1, 0),
    DEMO_FRAME(79, 3),
    DEMO_FRAME(80, 3),
    DEMO_FRAME(81, 3),
    DEMO_FRAME(82, 3),
    DEMO_FRAME(83, 3),
    DEMO_FRAME(84, 3),
    DEMO_FRAME(85, 3),
    DEMO_FRAME(86, 3),
    DEMO_FRAME(87, 3),
    DEMO_FRAME(88, 3),
    DEMO_FRAME(89, 3),
    DEMO_FRAME(90, 3),
    DEMO_FRAME(91, 3),
    DEMO_FRAME(92, 3),
    LOOP()
};

static const NGActionCmd g_demo_fx_slash[] = {
    SCALE(0xA0, 0xA0),
    DEMO_SPRITE_FRAME(38, 2),
    DEMO_SPRITE_FRAME(39, 2),
    DEMO_SPRITE_FRAME(40, 2),
    DEMO_SPRITE_FRAME(41, 2),
    DEMO_SPRITE_FRAME(42, 2),
    DEMO_SPRITE_FRAME(43, 2),
    DEMO_SPRITE_FRAME(44, 2),
    DEMO_SPRITE_FRAME(45, 2),
    DEMO_SPRITE_FRAME(46, 2),
    DEMO_SPRITE_FRAME(47, 2),
    DEMO_SPRITE_FRAME(48, 2),
    END()
};

static const NGActionCmd g_demo_portrait_end[] = {
    MUSIC(SOUND_MUSIC_SAMURAI_ENDING_SCENE),
    SFX(SOUND_SFX_STRING_PHRASE),
    WAIT(150),
    END()
};


static const NGActionCmd g_demo_warrior_fire[] = {
    SPEED(0, 0),
    DEMO_SPRITE_FRAME(38, 3), SFX(SOUND_SFX_START_SLASH),
    DEMO_SPRITE_FRAME(39, 3),
    DEMO_SPRITE_FRAME(40, 3),
    DEMO_SPRITE_FRAME(41, 5), SFX(SOUND_SFX_IMPACT_HIT),
    GOTO(DEMO_ACT_WARRIOR_IDLE)
};

static const NGActionCmd g_demo_warrior_fire2[] = {
    SPEED(0, 0),
    DEMO_SPRITE_FRAME(42, 3), SFX(SOUND_SFX_START_SLASH),
    DEMO_SPRITE_FRAME(43, 3),
    DEMO_SPRITE_FRAME(44, 3),
    DEMO_SPRITE_FRAME(45, 3),
    DEMO_SPRITE_FRAME(46, 3),
    DEMO_SPRITE_FRAME(47, 3),
    DEMO_SPRITE_FRAME(48, 5), SFX(SOUND_SFX_IMPACT_HIT),
    GOTO(DEMO_ACT_WARRIOR_IDLE)
};

static const NGActionCmd g_demo_warrior_close[] = {
    SPEED(0, 0),
    DEMO_SPRITE_FRAME(49, 4), SFX(SOUND_SFX_BLADE_WHOOSH),
    DEMO_SPRITE_FRAME(50, 4),
    DEMO_SPRITE_FRAME(51, 7), SFX(SOUND_SFX_IMPACT_HIT),
    GOTO(DEMO_ACT_WARRIOR_IDLE)
};

static const NGActionCmd g_demo_warrior_victory[] = {
    SPEED(0, 0),
    DEMO_SPRITE_FRAME(52, 7),
    DEMO_SPRITE_FRAME(53, 7),
    DEMO_SPRITE_FRAME(54, 7),
    DEMO_SPRITE_FRAME(55, 7),
    DEMO_SPRITE_FRAME(56, 12),
    LOOP()
};

static const NGActionCmd g_demo_warrior_slide[] = {
    SPEED(0, 0),
    DEMO_SPRITE_FRAME(52, 3), SFX(SOUND_SFX_FOOTSTEP),
    DEMO_SPRITE_FRAME(53, 3),
    DEMO_SPRITE_FRAME(54, 3), SFX(SOUND_SFX_BLADE_WHOOSH),
    DEMO_SPRITE_FRAME(55, 3),
    DEMO_SPRITE_FRAME(56, 5), SFX(SOUND_SFX_IMPACT_HIT),
    GOTO(DEMO_ACT_WARRIOR_IDLE)
};

static const NGActionCmd g_demo_warrior_respect[] = {
    SPEED(0, 0),
    DEMO_SPRITE_FRAME(57, 8),
    DEMO_SPRITE_FRAME(58, 8),
    DEMO_SPRITE_FRAME(59, 8),
    DEMO_SPRITE_FRAME(60, 8),
    DEMO_SPRITE_FRAME(61, 12),
    LOOP()
};

static const NGActionCmd g_demo_warrior_super_kick[] = {
    SPEED(0, 0),
    DEMO_SPRITE_FRAME(58, 3), SFX(SOUND_SFX_START_SLASH),
    DEMO_SPRITE_FRAME(59, 3),
    DEMO_SPRITE_FRAME(60, 3),
    DEMO_SPRITE_FRAME(61, 6), SFX(SOUND_SFX_IMPACT_HIT),
    GOTO(DEMO_ACT_WARRIOR_IDLE)
};

static const NGActionCmd g_demo_item_collected[] = {
    SPEED(0, 0),
    DEMO_SPRITE_FRAME(63, 5), SFX(SOUND_SFX_COIN_CHIME),
    DEMO_SPRITE_FRAME(63, 5),
    END()
};

static NGCharacter *NEOGEO_USER demo_char(uint8_t kind)
{
    return chars_find(kind);
}

static const NGSpriteAssetMeta *NEOGEO_USER demo_asset_meta(uint8_t screen_id)
{
    uint16_t index;

    if (screen_id == 0) return 0;
    index = (uint16_t)(screen_id - 1u);
    if (index >= NG_ASSET_META_COUNT) return 0;
    return &g_ng_asset_meta[index];
}

static uint8_t NEOGEO_USER demo_meta_visible_strips(const NGSpriteAssetMeta *meta)
{
    if (!meta) return 1;
    return meta->strips ? meta->strips : 1;
}

static uint8_t NEOGEO_USER demo_meta_visible_rows(const NGSpriteAssetMeta *meta)
{
    if (!meta) return 1;
    return meta->active_rows ? meta->active_rows : 1;
}


static uint8_t NEOGEO_USER demo_stable_palette_for(const NGCharacter *c, const NGSpriteAssetMeta *meta)
{
    if (!meta) return 0;

    if (c &&
        (c->kind == DEMO_KIND_NPC_CENTER ||
         c->kind == DEMO_KIND_NPC_LEFT ||
         c->kind == DEMO_KIND_NPC_RIGHT)) {
        return DEMO_NPC_MASTER_PALETTE;
    }

    /*
     * Keep the main character color stable across WALK / DUEL / REEL.
     * The special-effect overlay character can still use its own palette.
     */
    if (c && c->kind == DEMO_KIND_FX) {
        return meta->palette_bank;
    }

    if (meta->category == NG_ASSET_CATEGORY_MAIN_CHARACTER) {
        return DEMO_MAIN_MASTER_PALETTE;
    }

    return meta->palette_bank;
}

static void NEOGEO_USER demo_set_character_sprite_frame(NGCharacter *c, const NGSpriteAssetMeta *meta)
{
    uint16_t new_tile;
    uint8_t new_strips;
    uint8_t new_rows;
    uint8_t new_palette;

    if (!c || !meta) return;

    new_tile = (uint16_t)(
        meta->tile_base + ((uint16_t)meta->tile_row_start * 16u) + meta->tile_col_start
    );

    /*
     * Use the generated cropped sprite rectangle as the visible character.
     * Runtime x_pad/y_pad alignment caused special frames to move too high
     * or lose body parts.  Keep offsets zero and anchor the visible sprite
     * consistently in the engine field.
     */
    new_strips = demo_meta_visible_strips(meta);
    new_rows = demo_meta_visible_rows(meta);
    new_palette = demo_stable_palette_for(c, meta);

    if (c->sprite_tile != new_tile ||
        c->sprite_strips != new_strips ||
        c->sprite_stride != 16 ||
        c->sprite_height != new_rows ||
        c->sprite_active_rows != new_rows ||
        c->palette != new_palette ||
        c->sprite_offset_x != 0 ||
        c->sprite_offset_y != 0) {
        c->sprite_tile = new_tile;
        c->sprite_strips = new_strips;
        c->sprite_stride = 16;
        c->sprite_height = new_rows;
        c->sprite_active_rows = new_rows;
        c->palette = new_palette;
        c->sprite_offset_x = 0;
        c->sprite_offset_y = 0;
        c->sprite_dirty = 1;
    }
}


static uint8_t NEOGEO_USER DEMO_MAYBE_UNUSED demo_screen_id_from_palette(uint8_t palette_bank)
{
    uint16_t screen_id;

    if (palette_bank < 16) return 0;
    screen_id = (uint16_t)(palette_bank - 15u);
    if (screen_id == 0 || screen_id > NG_ASSET_META_COUNT) return 0;
    return (uint8_t)screen_id;
}

static uint8_t NEOGEO_USER demo_screen_id_from_tile(uint16_t tile)
{
    uint16_t screen_id;

    screen_id = (uint16_t)(tile / 256u) + 1u;

    if (screen_id == 0 || screen_id > NG_ASSET_META_COUNT) return 0;

    return (uint8_t)screen_id;
}

static void NEOGEO_USER demo_sync_action_sprite(NGCharacter *c)
{
    const NGSpriteAssetMeta *meta;
    uint8_t screen_id;

    if (!c || !c->visible) return;

    screen_id = demo_screen_id_from_tile(c->sprite_tile);
    if (screen_id == 0) return;

    meta = demo_asset_meta(screen_id);
    if (!meta || meta->mode != NG_ASSET_MODE_SPRITE) return;

    demo_set_character_sprite_frame(c, meta);
}

static void NEOGEO_USER demo_hide_all_hardware_sprites(void);

static void NEOGEO_USER demo_load_palette_group(const SceneScreenFn *loaders, uint16_t count)
{
    uint16_t i;
    uint8_t settle;

    /*
     * Generated showScreenXX() functions load palettes but also upload
     * sprite data.  Draw them in a reserved high sprite range, off-screen,
     * then clear sprite control for several frames so no preload sprite
     * remains visible when the engine starts.
     */
    for (i = 0; i < count; i++) {
        loaders[i](-320, 1024, 0, 0, 0, BLACK, 400);
    }

    for (settle = 0; settle < 2; settle++) {
        clearSprs();
        demo_hide_all_hardware_sprites();
        waitVbl();
    }
}

static void NEOGEO_USER demo_hide_all_hardware_sprites(void)
{
    uint16_t sprite;

    /*
     * Neo Geo has hundreds of hardware sprite slots.  clearSprs() may not
     * reliably hide every slot touched by generated showScreenXX code.
     * For scene boundaries, clear SCB3 height for the whole hardware range.
     */
    for (sprite = 0; sprite < 384; sprite++) {
        vram_SCB234((uint16_t)(SCB3_ADDR + sprite), 0);
    }
}


static void NEOGEO_USER demo_hard_clear_scene(void)
{
    /*
     * Scene-boundary clear.  Generated showScreen functions upload hardware
     * sprites.  Hide every sprite slot and allow VBlank to settle before
     * starting the engine scene.
     */
    clearSprs();
    demo_hide_all_hardware_sprites();
    clearFix();
    waitVbl();

    clearSprs();
    demo_hide_all_hardware_sprites();
    waitVbl();
}


static void NEOGEO_USER demo_load_master_sprite_palettes(void)
{
    /*
     * Load at least the master character palette and the master NPC palette.
     * This avoids random/previous Palette RAM colors when the runtime forces
     * stable palette banks.
     */
    showScreen11(-320, 1024, 0, 0, 0, BLACK, 400);
    showScreen94(-320, 1024, 0, 0, 0, BLACK, 400);
    clearSprs();
}




static void NEOGEO_USER demo_park_character(NGCharacter *c)
{
    if (!c) return;
    ng_char_set_pos(c, 496, 704);
    c->visible = 0;
}

static void NEOGEO_USER demo_apply_gallery_frame(NGCharacter *c, uint8_t screen_id)
{
    const NGSpriteAssetMeta *meta;

    if (!c) return;

    meta = demo_asset_meta(screen_id);
    if (!meta || meta->mode != NG_ASSET_MODE_SPRITE) return;

    demo_set_character_sprite_frame(c, meta);
}

static uint16_t NEOGEO_USER demo_wrap_index(uint16_t value, uint16_t count)
{
    if (count == 0) return 0;
    while (value >= count) {
        value = (uint16_t)(value - count);
    }
    return value;
}

static void NEOGEO_USER demo_place_sprite_frame_ex(
    NGCharacter *c,
    uint8_t screen_id,
    int16_t center_x,
    int16_t baseline_y,
    uint8_t scale_x,
    uint8_t scale_y,
    uint8_t flip_x
) {
    const NGSpriteAssetMeta *meta;
    int16_t left_x;

    if (!c) return;
    meta = demo_asset_meta(screen_id);
    if (!meta) return;

    demo_apply_gallery_frame(c, screen_id);
    left_x = (int16_t)(center_x - ((int16_t)meta->content_width / 2) - (int16_t)meta->x_pad);
    ng_char_set_pos(c, left_x, baseline_y);
    c->data0 = (uint16_t)baseline_y;
    c->scale_x = scale_x;
    c->scale_y = scale_y;
    c->flip_x = flip_x;
    c->flip_y = 0;
    c->visible = 1;
    c->sprite_dirty = 1;
}

static void NEOGEO_USER demo_place_sprite_frame(
    NGCharacter *c,
    uint8_t screen_id,
    int16_t center_x,
    int16_t baseline_y,
    uint8_t scale_x,
    uint8_t scale_y
) {
    demo_place_sprite_frame_ex(c, screen_id, center_x, baseline_y, scale_x, scale_y, 0);
}

static void NEOGEO_USER DEMO_MAYBE_UNUSED demo_draw_stage_background(uint16_t screen_id)
{
    if (screen_id == DEMO_LATE_SCENE_BG) {
        
    }
}

static void NEOGEO_USER DEMO_MAYBE_UNUSED demo_draw_walk_scene(void)
{
    ng_level_set_backdrop(BLACK);
    setBACKDROP(BLACK);
}

static void NEOGEO_USER DEMO_MAYBE_UNUSED demo_cycle_showcase(
    NGCharacter *c,
    const uint8_t *frames,
    uint16_t frame_count,
    int16_t center_x,
    int16_t baseline_y,
    uint8_t scale_x,
    uint8_t scale_y
) {
    uint16_t index;
    uint16_t next_index;

    if (!c || !frames || frame_count == 0) return;

    index = c->data1;
    if (index >= frame_count) {
        index = 0;
    }
    next_index = (uint16_t)(index + 1u);
    if (next_index >= frame_count) {
        next_index = 0;
    }

    demo_place_sprite_frame(
        c,
        frames[index],
        center_x,
        baseline_y,
        scale_x,
        scale_y
    );
    c->data1 = next_index;
}

static uint8_t NEOGEO_USER demo_input_down(uint16_t input, uint8_t bit)
{
    return (input & (uint16_t)(1u << bit)) ? 1 : 0;
}

static void NEOGEO_USER demo_set_main_action(NGCharacter *c, uint16_t action_id)
{
    if (!c || c->action == action_id) return;
    ng_char_action(c, action_id);
}

static void NEOGEO_USER demo_sfx_hook(uint16_t id)
{
    playSFX((uint8_t)id);
}

static void NEOGEO_USER demo_music_hook(uint16_t id)
{
    soundPlayGameLoop((uint8_t)id);
}

static void NEOGEO_USER demo_fx_hook(uint16_t kind, int16_t x, int16_t y)
{
    NGCharacter *fx = demo_char(DEMO_KIND_FX);

    NG_UNUSED(kind);
    if (!fx) return;

    ng_char_set_pos(fx, (int16_t)(x - 24), (int16_t)(y + 28));
    fx->visible = 1;
    fx->data0 = 28;
    ng_char_action(fx, DEMO_ACT_FX_SLASH);
}

static void NEOGEO_USER demo_start_duel_phase(void)
{
    NGCharacter *warrior = demo_char(DEMO_KIND_WARRIOR);
    NGCharacter *mascot = demo_char(DEMO_KIND_MASCOT);

    if (!warrior || !mascot) return;
    if (ng_status_has(DEMO_STATUS_DUEL_ACTIVE)) return;

    demo_load_palette_group(
        g_demo_all_actor_palette_loaders,
        (uint16_t)NG_ARRAY_SIZE(g_demo_all_actor_palette_loaders)
    );
    demo_load_palette_group(
        g_demo_mascot_palette_loaders,
        (uint16_t)NG_ARRAY_SIZE(g_demo_mascot_palette_loaders)
    );
    demo_load_palette_group(
        g_demo_duel_palette_loaders,
        (uint16_t)NG_ARRAY_SIZE(g_demo_duel_palette_loaders)
    );

    demo_load_master_sprite_palettes();
    ng_status_set(DEMO_STATUS_GATE_CROSSED);
    ng_status_set(DEMO_STATUS_DUEL_ACTIVE);
    ng_status_set(DEMO_STATUS_PLAYER_INPUT);
    ng_status_clear(DEMO_STATUS_GALLERY_ACTIVE);
    ng_progress_set(DEMO_PROGRESS_PHASE, 1);
    ng_prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_MODE, 1);
    ng_level_set_world_bounds(0, 0, 511, 255);
    ng_timer_start(DEMO_TIMER_DUEL, NG_MS_TO_FRAMES(14000));
    ng_timer_stop(DEMO_TIMER_HIT_LOCK);
    ng_timer_stop(DEMO_TIMER_PLAYER_HURT);

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);
    playVoiceCue(SOUND_VOICE_ATTACK);
    ng_level_set_backdrop(BLACK);

    warrior->visible = 1;
    ng_char_set_pos(warrior, 70, DEMO_DUEL_BASELINE);
    warrior->scale_x = 0xD8;
    warrior->scale_y = 0xD8;
    warrior->sprite_dirty = 1;
    ng_char_action(warrior, DEMO_ACT_WARRIOR_IDLE);

    mascot->visible = 0;
    ng_char_set_pos(mascot, 496, 704);
    mascot->scale_x = 0xD0;
    mascot->scale_y = 0xD0;
    mascot->sprite_dirty = 1;
    mascot->hp = 5;
    mascot->max_hp = 5;
}

static void NEOGEO_USER demo_start_gallery_phase(void)
{
    NGCharacter *gallery = demo_char(DEMO_KIND_GALLERY);
    NGCharacter *cast_left = demo_char(DEMO_KIND_CAST_LEFT);
    NGCharacter *cast_right = demo_char(DEMO_KIND_CAST_RIGHT);
    NGCharacter *portrait = demo_char(DEMO_KIND_PORTRAIT);
    NGCharacter *mascot = demo_char(DEMO_KIND_MASCOT);
    NGCharacter *warrior = demo_char(DEMO_KIND_WARRIOR);
    NGCharacter *fx = demo_char(DEMO_KIND_FX);

    if (!warrior) return;
    if (ng_status_has(DEMO_STATUS_GALLERY_ACTIVE)) return;

    demo_load_palette_group(
        g_demo_all_actor_palette_loaders,
        (uint16_t)NG_ARRAY_SIZE(g_demo_all_actor_palette_loaders)
    );

    ng_status_clear(DEMO_STATUS_DUEL_ACTIVE);
    ng_status_clear(DEMO_STATUS_PLAYER_INPUT);
    ng_status_set(DEMO_STATUS_GALLERY_ACTIVE);
    ng_progress_set(DEMO_PROGRESS_PHASE, 2);
    ng_progress_start(DEMO_PROGRESS_GALLERY, 96);
    ng_prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_MODE, 2);
    ng_prop_set(NG_PROP_GROUP_BOSS, NG_PROP_BOSS_PHASE, 2);
    ng_level_set_background(0);
    ng_level_set_backdrop(BLACK);
    ng_level_set_world_bounds(0, 0, 319, 255);

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);
    playVoiceCue(SOUND_VOICE_ATTACK);

    if (mascot) demo_park_character(mascot);
    if (gallery) demo_park_character(gallery);
    if (cast_left) demo_park_character(cast_left);
    if (cast_right) demo_park_character(cast_right);
    if (portrait) demo_park_character(portrait);
    if (fx) demo_park_character(fx);

    warrior->visible = 1;
    ng_char_set_pos(warrior, 70, DEMO_REEL_BASELINE);
    warrior->scale_x = 0xD8;
    warrior->scale_y = 0xD8;
    warrior->sprite_dirty = 1;
    warrior->data1 = 0xffff;
    ng_char_action(warrior, DEMO_ACT_WARRIOR_CLOSE);

    ng_timer_start(DEMO_TIMER_GALLERY_STEP, 1);
}


static void NEOGEO_USER demo_apply_optical_depth(NGCharacter *c)
{
    uint8_t target_scale;

    if (!c || !c->visible) return;

    /*
     * Pseudo-3D inside the 2D engine: the character is still a normal
     * 2D sprite chain, but Y position changes scale and depth priority.
     * This gives a brawler/adventure optical depth illusion without
     * a real 3D renderer.
     */
    if (c->y < 96) target_scale = 0xB8;
    else if (c->y < 124) target_scale = 0xD0;
    else target_scale = 0xEC;

    if (c->scale_x != target_scale || c->scale_y != target_scale) {
        c->scale_x = target_scale;
        c->scale_y = target_scale;
        c->sprite_dirty = 1;
    }

    ng_char_set_priority(c, c->priority_band, c->y);
}

static void NEOGEO_USER demo_before_logic(void)
{
    NGCharacter *player = demo_char(DEMO_KIND_WARRIOR);
    NGRect body;

    if (player) {
        body = ng_char_body_rect(player);
        ng_prop_set(NG_PROP_GROUP_PLAYER, NG_PROP_PLAYER_X, body.x);
        ng_prop_set(NG_PROP_GROUP_PLAYER, NG_PROP_PLAYER_Y, body.y);
        ng_prop_set(NG_PROP_GROUP_PLAYER, NG_PROP_PLAYER_W, body.w);
        ng_prop_set(NG_PROP_GROUP_PLAYER, NG_PROP_PLAYER_H, body.h);
        ng_prop_set(NG_PROP_GROUP_PLAYER, NG_PROP_PLAYER_HP, player->hp);
        ng_level_set_camera((int16_t)(body.x + body.w / 2 - 160), 0, 320, 224);
    }

    demo_apply_optical_depth(demo_char(DEMO_KIND_WARRIOR));
    demo_apply_optical_depth(demo_char(DEMO_KIND_MASCOT));
    demo_apply_optical_depth(demo_char(DEMO_KIND_NPC_CENTER));
    demo_apply_optical_depth(demo_char(DEMO_KIND_NPC_LEFT));
    demo_apply_optical_depth(demo_char(DEMO_KIND_NPC_RIGHT));

    ng_prop_set(NG_PROP_GROUP_DEBUG, 0, ng_progress_value(DEMO_PROGRESS_PHASE));
    ng_prop_set(NG_PROP_GROUP_DEBUG, 1, ng_chars_count());
    ng_prop_set(NG_PROP_GROUP_DEBUG, 2, ng_timer_value(DEMO_TIMER_DUEL));

    if (demo_advance_requested()) {
        uint16_t phase = ng_progress_value(DEMO_PROGRESS_PHASE);
        if (phase == 0) {
            demo_start_duel_phase();
        } else if (phase == 1) {
            demo_start_gallery_phase();
        } else {
            ng_status_set(DEMO_STATUS_DONE);
        }
    }

    if (ng_progress_value(DEMO_PROGRESS_PHASE) == 0 && ng_timer_done(DEMO_TIMER_WALK)) {
        demo_start_duel_phase();
    }

    if (ng_status_has(DEMO_STATUS_DUEL_ACTIVE) && ng_timer_done(DEMO_TIMER_DUEL)) {
        demo_start_gallery_phase();
    }

    if (ng_progress_value(DEMO_PROGRESS_PHASE) == 3 && ng_timer_done(DEMO_TIMER_END_HOLD)) {
        ng_status_set(DEMO_STATUS_DONE);
    }
}

static void NEOGEO_USER demo_collision_logic(void)
{
    NGCharacter *warrior = demo_char(DEMO_KIND_WARRIOR);
    NGCharacter *mascot = demo_char(DEMO_KIND_MASCOT);
    NGRect hit;
    NGRect target;
    NGRect body;
    const NGLevelState *level;

    if (!ng_status_has(DEMO_STATUS_DUEL_ACTIVE)) return;
    if (!warrior || !mascot || !mascot->visible) return;

    if (ng_timer_done(DEMO_TIMER_PLAYER_HURT)) {
        body = ng_char_body_rect(warrior);
        target = ng_char_body_rect(mascot);
        level = level_state();
        if (ng_rect_hit(body, target) ||
            body.x <= level->world_left ||
            (int16_t)(body.x + body.w) >= level->world_right) {
            ng_game_events_send(DEMO_EVT_PLAYER_HURT, 0, 0, 0);
        }
    }

    if (mascot->hp == 0) return;
    if (!ng_timer_done(DEMO_TIMER_HIT_LOCK)) return;
    if (warrior->hit_w == 0 || warrior->hit_h == 0) return;

    hit = ng_char_hit_rect(warrior);
    target = ng_char_body_rect(mascot);
    if (ng_rect_hit(hit, target)) {
        ng_game_events_send(DEMO_EVT_ATTACK_IMPACT, 0, 0, 0);
    }
}

static void NEOGEO_USER demo_after_events(void)
{
    NGCharacter *gallery = demo_char(DEMO_KIND_GALLERY);
    NGCharacter *cast_left = demo_char(DEMO_KIND_CAST_LEFT);
    NGCharacter *cast_right = demo_char(DEMO_KIND_CAST_RIGHT);

    if (ng_status_has(DEMO_STATUS_GALLERY_ACTIVE) && gallery && cast_left && cast_right) {
        if (ng_timer_done(DEMO_TIMER_GALLERY_STEP)) {
            uint16_t step = ng_progress_value(DEMO_PROGRESS_GALLERY);

            if (step < 96) {
                demo_park_character(gallery);
                demo_park_character(cast_left);
                demo_park_character(cast_right);
                ng_progress_set(DEMO_PROGRESS_GALLERY, (uint16_t)(step + 1u));
                ng_timer_start(DEMO_TIMER_GALLERY_STEP, 10);
                /* stable text: no phase flashing */
            } else {
                ng_game_events_send(DEMO_EVT_GALLERY_DONE, 0, 0, 0);
            }
        }
    }
}

static void NEOGEO_USER DEMO_MAYBE_UNUSED demo_force_reel_npcs(void)
{
}

static void NEOGEO_USER demo_before_draw(void)
{
    const char *phase = "WALK";
    uint16_t phase_step = ng_progress_value(DEMO_PROGRESS_PHASE);
    NGCharacter *warrior = demo_char(DEMO_KIND_WARRIOR);

    if (phase_step == 1) phase = "DUEL";
    if (phase_step == 2) phase = "REEL";
    if (phase_step == 3) phase = "FINAL";

    /*
     * Engine pass: no generated showScreenXX calls here.
     * Only update the active character SCB data.
     */
    ng_level_set_backdrop(BLACK);
    setBACKDROP(BLACK);

    demo_sync_action_sprite(warrior);

    ng_fix_puts(2, 1, "2D ENGINE DEMO", 1);
    ng_fix_puts(2, 3, "PHASE", 1);
    ng_fix_puts(9, 3, phase, 1);

    ng_fix_puts(2, 5, "TIME", 1);
    ng_fix_put_u16(7, 5, ng_game_time_second(), 1, 48);

    ng_fix_puts(13, 5, "HP", 1);
    ng_fix_put_u16(16, 5, warrior ? warrior->hp : 0, 1, 48);

    ng_fix_puts(20, 5, "CAST", 1);
    ng_fix_put_u16(25, 5, 1, 1, 48);

    ng_fix_puts(2, 7, "DUEL", 1);
    ng_fix_put_u16(7, 7, ng_timer_percent_left(DEMO_TIMER_DUEL), 1, 48);

    ng_fix_puts(13, 7, "REEL", 1);
    ng_fix_put_u16(18, 7, ng_progress_percent(DEMO_PROGRESS_GALLERY), 1, 48);

    ng_fix_puts(2, 9, "OPTICAL 3D SCALE BY Y", 2);
    ng_fix_puts(2, 11, "A ADVANCES PHASE", 1);
}

static void NEOGEO_USER demo_scene_event_handler(const NGGameEvent *e)
{
    if (!e) return;

    switch (e->id) {
        case DEMO_EVT_GATE_CENTER:
            demo_start_duel_phase();
            break;

        case DEMO_EVT_ATTACK_IMPACT:
            ng_timer_start(DEMO_TIMER_HIT_LOCK, 14);
            playSFX(SOUND_SFX_IMPACT_HIT);
            ng_prop_add(NG_PROP_GROUP_PLAYER, NG_PROP_PLAYER_SCORE, 100);
            ng_prop_add(NG_PROP_GROUP_PROGRESS, 0, 1);
            if (demo_char(DEMO_KIND_MASCOT)) {
                NGCharacter *mascot = demo_char(DEMO_KIND_MASCOT);
                ng_char_damage(mascot, 1);
                if (mascot->hp == 0) {
                    demo_start_gallery_phase();
                }
            }
            break;

        case DEMO_EVT_PLAYER_HURT:
            if (demo_char(DEMO_KIND_WARRIOR)) {
                NGCharacter *warrior = demo_char(DEMO_KIND_WARRIOR);
                ng_timer_start(DEMO_TIMER_PLAYER_HURT, 80);
                playSFX(SOUND_SFX_SHORT_SHOUT);
                ng_char_damage(warrior, 1);
                demo_set_main_action(warrior, DEMO_ACT_WARRIOR_HIT);
                if (warrior->hp == 0) {
                    ng_status_set(DEMO_STATUS_DONE);
                }
            }
            break;

        case DEMO_EVT_GALLERY_DONE:
            ng_status_clear(DEMO_STATUS_GALLERY_ACTIVE);
            ng_status_clear(DEMO_STATUS_PLAYER_INPUT);
            ng_progress_set(DEMO_PROGRESS_PHASE, 3);
            ng_prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_MODE, 3);
            ng_level_set_background(0);
            ng_level_set_backdrop(BLACK);
            if (demo_char(DEMO_KIND_GALLERY)) demo_park_character(demo_char(DEMO_KIND_GALLERY));
            if (demo_char(DEMO_KIND_CAST_LEFT)) demo_park_character(demo_char(DEMO_KIND_CAST_LEFT));
            if (demo_char(DEMO_KIND_CAST_RIGHT)) demo_park_character(demo_char(DEMO_KIND_CAST_RIGHT));
            if (demo_char(DEMO_KIND_PORTRAIT)) {
                NGCharacter *portrait = demo_char(DEMO_KIND_PORTRAIT);
                demo_park_character(portrait);
                portrait->data1 = 0;
                portrait->state = 0;
            }
            playVoiceCue(SOUND_VOICE_GET_READY);
            ng_timer_start(DEMO_TIMER_END_HOLD, 600);
            break;

        default:
            break;
    }
}

static void NEOGEO_USER demo_warrior_tick(NGCharacter *c)
{
    uint16_t input;
    uint16_t phase;
    uint16_t slot;
    int16_t vx = 0;

    if (!c) return;

    phase = ng_progress_value(DEMO_PROGRESS_PHASE);

    if (phase == 0) {
        if (c->x < 160) {
            c->vx_fp = NG_TO_FP(1);
            c->flip_x = 0;
            demo_set_main_action(c, DEMO_ACT_WARRIOR_WALK);
        } else {
            c->vx_fp = 0;
            demo_set_main_action(c, DEMO_ACT_WARRIOR_IDLE);
        }
        c->y_fp = NG_TO_FP(DEMO_WALK_BASELINE);
        return;
    }

    if (phase == 1) {
        input = poll_joystick();
        slot = demo_wrap_index((uint16_t)(ng_game_time_frame() / 96u), 6);

        if (demo_input_down(input, CNT_LEFT)) {
            vx = -1;
            c->flip_x = 1;
        } else if (demo_input_down(input, CNT_RIGHT)) {
            vx = 1;
            c->flip_x = 0;
        } else {
            if (slot == 0 || slot == 2) vx = 1;
            if (slot == 4) vx = -1;
            if (c->x < 64) vx = 1;
            if (c->x > 185) vx = -1;
            c->flip_x = (vx < 0) ? 1 : 0;
        }

        if (c->data1 != slot) {
            c->data1 = slot;
            if (slot == 1) {
                playAttackVoice();
                playSFX(SOUND_SFX_BLADE_WHOOSH);
                demo_set_main_action(c, DEMO_ACT_WARRIOR_ATTACK_A);
            } else if (slot == 3) {
                playSFX(SOUND_SFX_START_SLASH);
                demo_set_main_action(c, DEMO_ACT_WARRIOR_ATTACK_B);
            } else if (slot == 5) {
                playSFX(SOUND_SFX_IMPACT_HIT);
                demo_set_main_action(c, DEMO_ACT_WARRIOR_FIRE);
            }
        }

        c->vx_fp = NG_TO_FP(vx);
        c->y_fp = NG_TO_FP(DEMO_DUEL_BASELINE);

        if (c->action_timer == 0) {
            if (slot == 0 || slot == 2 || slot == 4) {
                demo_set_main_action(c, DEMO_ACT_WARRIOR_RUN);
            } else {
                c->vx_fp = 0;
            }
        }
        return;
    }

    if (phase >= 2) {
        slot = demo_wrap_index((uint16_t)(ng_game_time_frame() / 120u), 5);
        c->vx_fp = 0;
        c->y_fp = NG_TO_FP(DEMO_REEL_BASELINE);
        c->flip_x = 0;

        if (c->data1 != (uint16_t)(100u + slot)) {
            c->data1 = (uint16_t)(100u + slot);
            if (slot == 0) demo_set_main_action(c, DEMO_ACT_WARRIOR_CLOSE);
            else if (slot == 1) demo_set_main_action(c, DEMO_ACT_WARRIOR_SLIDE);
            else if (slot == 2) demo_set_main_action(c, DEMO_ACT_WARRIOR_RESPECT);
            else if (slot == 3) demo_set_main_action(c, DEMO_ACT_WARRIOR_VICTORY);
            else demo_set_main_action(c, DEMO_ACT_WARRIOR_IDLE);
        }
    }
}

static void NEOGEO_USER demo_mascot_tick(NGCharacter *c)
{
    demo_park_character(c);
}

static void NEOGEO_USER demo_fx_tick(NGCharacter *c)
{
    demo_park_character(c);
}

static void NEOGEO_USER demo_gallery_tick(NGCharacter *c)
{
    int16_t base_y;

    if (!c) return;
    if (!ng_status_has(DEMO_STATUS_GALLERY_ACTIVE)) return;

    base_y = (int16_t)c->data0;
    if (base_y == 0) {
        c->data0 = (uint16_t)c->y;
        base_y = c->y;
    }

    c->y_fp = NG_TO_FP((int16_t)(base_y + (((ng_game_time_frame() >> 4) & 1) ? 1 : 0)));
}

static void NEOGEO_USER demo_cast_tick(NGCharacter *c)
{
    int16_t base_y;

    if (!c) return;
    if (!ng_status_has(DEMO_STATUS_GALLERY_ACTIVE)) return;

    base_y = (int16_t)c->data0;
    if (base_y == 0) {
        c->data0 = (uint16_t)c->y;
        base_y = c->y;
    }

    c->y_fp = NG_TO_FP((int16_t)(base_y + (((ng_game_time_frame() >> 3) & 1) ? 2 : 0)));
}

static void NEOGEO_USER demo_npc_center_think(NGNpc *npc, NGCharacter *c)
{
    uint16_t frame;
    int16_t baseline_y;

    if (!npc || !c) return;
    if (ng_progress_value(DEMO_PROGRESS_PHASE) < 2) {
        demo_park_character(c);
        return;
    }

    frame = demo_wrap_index((uint16_t)(ng_game_time_frame() >> 3), (uint16_t)NG_ARRAY_SIZE(g_demo_npc_front_frames));
    baseline_y = (int16_t)(npc->home_y + (((ng_game_time_frame() >> 4) & 1) ? 1 : 0));
    demo_place_sprite_frame_ex(
        c,
        g_demo_npc_front_frames[frame],
        npc->home_x,
        baseline_y,
        0xF0,
        0xF0,
        0
    );
}

static void NEOGEO_USER demo_npc_walker_think(NGNpc *npc, NGCharacter *c)
{
    uint16_t frame;
    int16_t center_x;
    int16_t baseline_y;
    uint8_t flip_x;

    if (!npc || !c) return;
    if (ng_progress_value(DEMO_PROGRESS_PHASE) < 2) {
        demo_park_character(c);
        return;
    }

    center_x = (int16_t)npc->data0;
    if (center_x == 0) {
        center_x = npc->home_x;
    }

    if (npc->state == 0) {
        center_x++;
        if (center_x >= npc->max_x) {
            center_x = npc->max_x;
            npc->state = 1;
        }
    } else {
        center_x--;
        if (center_x <= npc->min_x) {
            center_x = npc->min_x;
            npc->state = 0;
        }
    }

    npc->data0 = (uint16_t)center_x;
    frame = demo_wrap_index((uint16_t)(ng_game_time_frame() >> 2), (uint16_t)NG_ARRAY_SIZE(g_demo_npc_walk_frames));
    baseline_y = (int16_t)(npc->home_y + (((ng_game_time_frame() >> 3) & 1) ? 1 : 0));
    flip_x = (npc->state != 0) ? 1 : 0;
    demo_place_sprite_frame_ex(
        c,
        g_demo_npc_walk_frames[frame],
        center_x,
        baseline_y,
        0xF0,
        0xF0,
        flip_x
    );
}

static void NEOGEO_USER demo_portrait_tick(NGCharacter *c)
{
    demo_park_character(c);
}

static void NEOGEO_USER demo_register_engine_scripts(void)
{
    ng_actions_set_sound_hooks(demo_sfx_hook, demo_music_hook);
    ng_actions_set_fx_hook(demo_fx_hook);

    ng_actions_register(DEMO_ACT_WARRIOR_WALK, g_demo_warrior_walk);
    ng_actions_register(DEMO_ACT_WARRIOR_ATTACK_A, g_demo_warrior_attack_a);
    ng_actions_register(DEMO_ACT_WARRIOR_ATTACK_B, g_demo_warrior_attack_b);
    ng_actions_register(DEMO_ACT_WARRIOR_IDLE, g_demo_warrior_idle);
    ng_actions_register(DEMO_ACT_WARRIOR_RUN, g_demo_warrior_run);
    ng_actions_register(DEMO_ACT_WARRIOR_JUMP, g_demo_warrior_jump);
    ng_actions_register(DEMO_ACT_WARRIOR_HIT, g_demo_warrior_hit);
    ng_actions_register(DEMO_ACT_MASCOT_FLOAT, g_demo_mascot_float);
    ng_actions_register(DEMO_ACT_FX_SLASH, g_demo_fx_slash);
    ng_actions_register(DEMO_ACT_PORTRAIT_END, g_demo_portrait_end);
    ng_actions_register(DEMO_ACT_WARRIOR_FIRE, g_demo_warrior_fire);
    ng_actions_register(DEMO_ACT_WARRIOR_FIRE2, g_demo_warrior_fire2);
    ng_actions_register(DEMO_ACT_WARRIOR_CLOSE, g_demo_warrior_close);
    ng_actions_register(DEMO_ACT_WARRIOR_VICTORY, g_demo_warrior_victory);
    ng_actions_register(DEMO_ACT_WARRIOR_SLIDE, g_demo_warrior_slide);
    ng_actions_register(DEMO_ACT_WARRIOR_RESPECT, g_demo_warrior_respect);
    ng_actions_register(DEMO_ACT_WARRIOR_SUPER_KICK, g_demo_warrior_super_kick);
    ng_actions_register(DEMO_ACT_ITEM_COLLECTED, g_demo_item_collected);
}

static void NEOGEO_USER demo_build_engine_cast(void)
{
    NGCharacter *warrior = chars_add(DEMO_KIND_WARRIOR, 32, DEMO_WALK_BASELINE);
    NGCharacter *mascot = chars_add(DEMO_KIND_MASCOT, 238, DEMO_DUEL_BASELINE);
    NGCharacter *fx = chars_add(DEMO_KIND_FX, 384, 520);
    NGCharacter *gallery = chars_add(DEMO_KIND_GALLERY, 384, 520);
    NGCharacter *cast_left = chars_add(DEMO_KIND_CAST_LEFT, 384, 520);
    NGCharacter *cast_right = chars_add(DEMO_KIND_CAST_RIGHT, 384, 520);
    NGCharacter *portrait = chars_add(DEMO_KIND_PORTRAIT, 384, 520);
    NGNpc *npc_center = npc_spawn(DEMO_NPC_KIND_CENTER, DEMO_KIND_NPC_CENTER, 172, DEMO_NPC_BASELINE);
    NGNpc *npc_left = npc_spawn(DEMO_NPC_KIND_LEFT, DEMO_KIND_NPC_LEFT, 72, DEMO_NPC_BASELINE);
    NGNpc *npc_right = npc_spawn(DEMO_NPC_KIND_RIGHT, DEMO_KIND_NPC_RIGHT, 278, DEMO_NPC_BASELINE);

    if (warrior) {
        ng_char_set_sprite(warrior, 160, 16, 16, DEMO_SCREEN_TILE(11), DEMO_SCREEN_PALETTE(11));
        warrior->sprite_stride = 16;
        warrior->sprite_active_rows = 9;
        warrior->scale_x = 0xD8;
        warrior->scale_y = 0xD8;
        warrior->max_hp = 5;
        warrior->hp = 5;
        ng_char_set_body(warrior, 76, 94, 58, 94);
        warrior->visible = 0;
        ng_char_action(warrior, DEMO_ACT_WARRIOR_WALK);
    }

    if (mascot) {
        ng_char_set_sprite(mascot, 64, 16, 16, DEMO_SCREEN_TILE(79), DEMO_SCREEN_PALETTE(79));
        mascot->sprite_stride = 16;
        mascot->sprite_active_rows = 9;
        mascot->scale_x = 0xFF;
        mascot->scale_y = 0xAF;
        mascot->max_hp = 5;
        mascot->hp = 5;
        ng_char_set_body(mascot, 88, 102, 56, 70);
        mascot->visible = 0;
        ng_char_action(mascot, DEMO_ACT_MASCOT_FLOAT);
    }

    if (fx) {
        ng_char_set_sprite(fx, 96, 16, 16, DEMO_SCREEN_TILE(50), DEMO_SCREEN_PALETTE(50));
        fx->sprite_stride = 16;
        fx->sprite_active_rows = 9;
        fx->scale_x = 0xFF;
        fx->scale_y = 0xAF;
        fx->visible = 0;
        demo_park_character(fx);
    }

    if (gallery) {
        ng_char_set_sprite(gallery, 128, 16, 16, DEMO_SCREEN_TILE(63), DEMO_SCREEN_PALETTE(63));
        gallery->sprite_stride = 16;
        gallery->sprite_active_rows = 16;
        gallery->scale_x = 0xFF;
        gallery->scale_y = 0xAF;
        gallery->visible = 0;
        demo_park_character(gallery);
    }

    if (cast_left) {
        ng_char_set_sprite(cast_left, 192, 16, 16, DEMO_SCREEN_TILE(63), DEMO_SCREEN_PALETTE(63));
        cast_left->sprite_stride = 16;
        cast_left->sprite_active_rows = 16;
        cast_left->scale_x = 0xFF;
        cast_left->scale_y = 0xAF;
        cast_left->visible = 0;
        demo_park_character(cast_left);
    }

    if (cast_right) {
        ng_char_set_sprite(cast_right, 224, 16, 16, DEMO_SCREEN_TILE(71), DEMO_SCREEN_PALETTE(71));
        cast_right->sprite_stride = 16;
        cast_right->sprite_active_rows = 16;
        cast_right->scale_x = 0xFF;
        cast_right->scale_y = 0xAF;
        cast_right->visible = 0;
        demo_park_character(cast_right);
    }

    if (portrait) {
        ng_char_set_sprite(portrait, 160, 16, 16, DEMO_SCREEN_TILE(93), DEMO_SCREEN_PALETTE(93));
        portrait->sprite_stride = 16;
        portrait->sprite_active_rows = 16;
        portrait->scale_x = 0xFF;
        portrait->scale_y = 0xAF;
        portrait->visible = 0;
        demo_park_character(portrait);
    }

    if (npc_center && npc_char(npc_center)) {
        NGCharacter *c = npc_char(npc_center);
        ng_char_set_sprite(c, 256, 6, 6, DEMO_SCREEN_TILE(94), DEMO_SCREEN_PALETTE(94));
        c->sprite_stride = 16;
        c->sprite_active_rows = 6;
        c->scale_x = 0xC2;
        c->scale_y = 0xC2;
        demo_park_character(c);
        ng_npc_set_home(npc_center, 172, DEMO_NPC_BASELINE);
        ng_npc_set_think(npc_center, demo_npc_center_think, 1);
    }

    if (npc_left && npc_char(npc_left)) {
        NGCharacter *c = npc_char(npc_left);
        ng_char_set_sprite(c, 272, 6, 6, DEMO_SCREEN_TILE(98), DEMO_SCREEN_PALETTE(98));
        c->sprite_stride = 16;
        c->sprite_active_rows = 6;
        c->scale_x = 0xC8;
        c->scale_y = 0xC8;
        demo_park_character(c);
        ng_npc_set_home(npc_left, 72, DEMO_NPC_BASELINE);
        ng_npc_set_patrol_bounds(npc_left, 44, 140, DEMO_NPC_BASELINE, DEMO_NPC_BASELINE);
        npc_left->data0 = 72;
        ng_npc_set_think(npc_left, demo_npc_walker_think, 1);
    }

    if (npc_right && npc_char(npc_right)) {
        NGCharacter *c = npc_char(npc_right);
        ng_char_set_sprite(c, 288, 6, 6, DEMO_SCREEN_TILE(98), DEMO_SCREEN_PALETTE(98));
        c->sprite_stride = 16;
        c->sprite_active_rows = 6;
        c->scale_x = 0xC8;
        c->scale_y = 0xC8;
        demo_park_character(c);
        ng_npc_set_home(npc_right, 278, DEMO_NPC_BASELINE);
        ng_npc_set_patrol_bounds(npc_right, 214, 304, DEMO_NPC_BASELINE, DEMO_NPC_BASELINE);
        npc_right->data0 = 278;
        npc_right->state = 1;
        ng_npc_set_think(npc_right, demo_npc_walker_think, 1);
    }

    if (warrior) ng_char_set_priority(warrior, NG_RENDER_BAND_PLAYER, 0);
    if (mascot) ng_char_set_priority(mascot, NG_RENDER_BAND_ENEMY, 0);
    if (fx) ng_char_set_priority(fx, NG_RENDER_BAND_FX, 0);
    if (gallery) ng_char_set_priority(gallery, NG_RENDER_BAND_FRONT, 0);
    if (cast_left) ng_char_set_priority(cast_left, NG_RENDER_BAND_NPC, 0);
    if (cast_right) ng_char_set_priority(cast_right, NG_RENDER_BAND_NPC, 0);
    if (portrait) ng_char_set_priority(portrait, NG_RENDER_BAND_FRONT, 0);
    if (npc_center && npc_char(npc_center)) ng_char_set_priority(npc_char(npc_center), NG_RENDER_BAND_NPC, 0);
    if (npc_left && npc_char(npc_left)) ng_char_set_priority(npc_char(npc_left), NG_RENDER_BAND_NPC, 0);
    if (npc_right && npc_char(npc_right)) ng_char_set_priority(npc_char(npc_right), NG_RENDER_BAND_NPC, 0);

    ng_chars_set_game_interupt(DEMO_KIND_WARRIOR, demo_warrior_tick);
    ng_chars_set_game_interupt(DEMO_KIND_MASCOT, demo_mascot_tick);
    ng_chars_set_game_interupt(DEMO_KIND_FX, demo_fx_tick);
    ng_chars_set_game_interupt(DEMO_KIND_GALLERY, demo_gallery_tick);
    ng_chars_set_game_interupt(DEMO_KIND_CAST_LEFT, demo_cast_tick);
    ng_chars_set_game_interupt(DEMO_KIND_CAST_RIGHT, demo_cast_tick);
    ng_chars_set_game_interupt(DEMO_KIND_PORTRAIT, demo_portrait_tick);
}
static void NEOGEO_USER demo_load_engine_fix_palettes(void)
{
    uint16_t fix_pal[16];

    /*
     * These values match the original intended FIX font mapping:
     * palette 0/1 -> readable white text on black background,
     * palette 2   -> cyan highlight.
     */
    setpal(fix_pal, 0, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE);
    load_palettes(fix_pal, PALETTES);

    setpal(fix_pal, 0, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE);
    load_palettes(fix_pal, PALETTES + PALOFFSET);

    setpal(fix_pal, 0, WHITE, CYAN, CYAN, CYAN, CYAN, CYAN, CYAN,
           WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE);
    load_palettes(fix_pal, PALETTES + PALOFFSET * 2);
}

static void NEOGEO_USER demo_init_engine_scene(void)
{
    clearFix();
    clearSprs();
    demo_load_engine_fix_palettes();
    soundSceneReset();
    soundSetADPCMAVolume(0x34);
    soundSetADPCMBVolume(0xBC);
    soundSetSSGVolume(0x00);
    soundSetFMVolume(0x00);

    ng_game_engine_init();
    demo_register_engine_scripts();
    ng_game_events_set_handler(demo_scene_event_handler);
    ng_game_engine_set_hooks(demo_before_logic, demo_collision_logic, demo_after_events, demo_before_draw, 0);

    ng_prop_clear_group(NG_PROP_GROUP_DEBUG);
    ng_prop_clear_group(NG_PROP_GROUP_PLAYER);
    ng_prop_clear_group(NG_PROP_GROUP_PROGRESS);
    ng_prop_set(NG_PROP_GROUP_LEVEL, NG_PROP_LEVEL_MODE, 0);
    ng_prop_set(NG_PROP_GROUP_BOSS, NG_PROP_BOSS_PHASE, 0);
    /* Do not draw generated showScreen backgrounds inside the engine pass.
     * Some full-screen exports contain opaque strips/black blocks and can hide
     * the live animation.  The 2D-engine scene uses clean backdrop + FIX text. */
    ng_bg_hide_all();
    ng_level_set_backdrop(BLACK);
    ng_level_set_world_bounds(0, 0, 639, 223);
    ng_level_set_fix_palette(0);
    ng_progress_start(DEMO_PROGRESS_PHASE, 4);
    ng_progress_set(DEMO_PROGRESS_PHASE, 0);

    demo_load_palette_group(
        g_demo_warrior_palette_loaders,
        (uint16_t)NG_ARRAY_SIZE(g_demo_warrior_palette_loaders)
    );
    demo_load_palette_group(
        g_demo_mascot_palette_loaders,
        (uint16_t)NG_ARRAY_SIZE(g_demo_mascot_palette_loaders)
    );

    demo_load_master_sprite_palettes();
    demo_build_engine_cast();

    {
        NGCharacter *warrior = demo_char(DEMO_KIND_WARRIOR);
        NGCharacter *mascot = demo_char(DEMO_KIND_MASCOT);

        if (warrior) {
            warrior->visible = 1;
            ng_char_set_pos(warrior, 70, DEMO_WALK_BASELINE);
            warrior->scale_x = 0xD8;
            warrior->scale_y = 0xD8;
            warrior->sprite_dirty = 1;
            ng_char_action(warrior, DEMO_ACT_WARRIOR_WALK);
        }

        if (mascot) {
            mascot->visible = 0;
            ng_char_set_pos(mascot, 496, 704);
            mascot->scale_x = 0xD0;
            mascot->scale_y = 0xD0;
            mascot->sprite_dirty = 1;
            ng_char_action(mascot, DEMO_ACT_MASCOT_FLOAT);
        }
    }

    ng_timer_start(DEMO_TIMER_WALK, NG_MS_TO_FRAMES(6800));

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);
    playVoiceCue(SOUND_VOICE_GET_READY);
}


static void NEOGEO_USER DEMO_MAYBE_UNUSED demo_show_pre_engine_screen(uint8_t screen_id, int x0, int y0)
{
    /*
     * Old direct picture walk:
     * 1.png..7.png => showScreen2..showScreen8.
     *
     * 0.png / showScreen1 is NOT drawn in this clean pass because the current
     * asset behaves as a large opaque black block.  Re-enable it only after
     * the artbox alpha/index-0 conversion is fixed.
     */
    switch (screen_id) {
        case 2: showScreen2(x0, y0, 0xF, 0xAF, 9, 0x0FFF, 0); break;
        case 3: showScreen3(x0, y0, 0xF, 0xAF, 9, 0x0FFF, 0); break;
        case 4: showScreen4(x0, y0, 0xF, 0xAF, 9, 0x0FFF, 0); break;
        case 5: showScreen5(x0, y0, 0xF, 0xAF, 9, 0x0FFF, 0); break;
        case 6: showScreen6(x0, y0, 0xF, 0xAF, 9, 0x0FFF, 0); break;
        case 7: showScreen7(x0, y0, 0xF, 0xAF, 9, 0x0FFF, 0); break;
        default: showScreen8(x0, y0, 0xF, 0xAF, 9, 0x0FFF, 0); break;
    }
}

static void NEOGEO_USER demo_run_pre_engine_showscreen_walk(void)
{
    uint8_t hold;

    demo_hard_clear_scene();
    demo_load_engine_fix_palettes();

    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_GAME_LOOP);
    demo_scene_caption("NEOGEO 2D ENGINE", "SPRITE CHAINS + STICKY BIT", "Y SORT + OPTICAL 3D SCALE");
    playVoiceCue(SOUND_VOICE_GET_READY);

    for (hold = 0; hold < 90; hold++) {
        waitVbl();
        if (demo_advance_requested()) break;
    }

    demo_hard_clear_scene();
}

void NEOGEO_USER demo_2d_engine_run(void)
{
    uint16_t frame = 0;

    demo_run_pre_engine_showscreen_walk();
    demo_hard_clear_scene();
    demo_init_engine_scene();
    while (!ng_status_has(DEMO_STATUS_DONE) && frame < (uint16_t)NG_MS_TO_FRAMES(42000)) {
        waitVbl();
        ng_game_engine_frame();
        frame++;
    }
    soundStopAll();
    demo_hard_clear_scene();
}

/*
 * Step through the seven primary character frames at a fixed delay so the
 * current scene music can be judged against predictable animation timing.
 */


void NEOGEO_USER demo_2d_engine_advanced_animation(void)
{
    uint8_t i;
    static const SceneScreenFn attack_frames[] = {
        showScreen20, showScreen21, showScreen22, showScreen23, showScreen24,
        showScreen25, showScreen26, showScreen27, showScreen28, showScreen29,
        showScreen30, showScreen31, showScreen32, showScreen33, showScreen34,
        showScreen35, showScreen36, showScreen37, showScreen38, showScreen39
    };
    static const SceneScreenFn fx_frames[] = {
        showScreen50, showScreen51, showScreen52, showScreen53, showScreen54,
        showScreen55, showScreen56, showScreen57, showScreen58, showScreen59,
        showScreen60, showScreen61, showScreen62
    };

    demo_hard_clear_scene();
    demo_load_engine_fix_palettes();
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_BATTLE_LOOP);
    demo_scene_caption("ADVANCED 2D ANIMATION", "ATTACK / FX FRAME TIMING", "RAW FRAMES HARD-CLEARED");
    playVoiceCue(SOUND_VOICE_ATTACK);

    for (i = 0; i < (uint8_t)NG_ARRAY_SIZE(attack_frames); i++) {
        demo_hide_all_hardware_sprites();
        clearSprs();
        demo_scene_caption("ADVANCED 2D ANIMATION", "WARRIOR ATTACK SEQUENCE", "A: NEXT SCENE");
        attack_frames[i](72, 66, 0xF, 0xAF, 9, 0x0000, 0);
        if (i == 5u) playSFX(SOUND_SFX_BLADE_WHOOSH);
        if (i == 12u) playSFX(SOUND_SFX_IMPACT_HIT);
        if (demo_wait_frames_or_a(10)) goto done;
    }

    for (i = 0; i < (uint8_t)NG_ARRAY_SIZE(fx_frames); i++) {
        demo_hide_all_hardware_sprites();
        clearSprs();
        demo_scene_caption("ADVANCED 2D ANIMATION", "ENERGY / BLADE EFFECTS", "A: NEXT SCENE");
        fx_frames[i](88, 78, 0xF, 0xAF, 9, 0x0000, 0);
        if ((i & 3u) == 0u) playSFX(SOUND_SFX_STRING_PHRASE);
        if (demo_wait_frames_or_a(8)) goto done;
    }

done:
    soundStopAll();
    demo_hard_clear_scene();
}
