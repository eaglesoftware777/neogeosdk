/*******
https://eaglesoftware.biz
https://github.com/eaglesoftware777
https://github.com/eaglesoftware777/neogeosdk
******/

#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include "sdk/2d_engine/ng_chars.h"
#include "sdk/2d_engine/ng_engine.h"
#include "artbox/sprite_meta.h"
#include <stdint.h>

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-const-variable"
#endif
#define PAL_BLUE 1
#define PAL_RED  2
void NEOGEO_USER soundAutoDemo(void);
void NEOGEO_USER showEagleIntro(void);
void NEOGEO_USER showTitleScreen(void);
void NEOGEO_USER showGameOver(void);
void NEOGEO_USER showCharacterParade(void);
void NEOGEO_USER showPseudo3DLoop(void);
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
void NEOGEO_USER showScreen106(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
/* warrior walk/idle — sprite_001 to sprite_009 */
void NEOGEO_USER showScreen11(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen12(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen13(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen14(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen15(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen16(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen17(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen18(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen19(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
/* warrior attack — all 30 frames: sprite_010 to sprite_039 */
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
/* blade and energy effects — sprite_040 to sprite_052 (13 frames) */
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
/* eagle mascot — sprite_069 to sprite_082 (14 frames) */
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
/* portrait — sprite_083 */
void NEOGEO_USER showScreen93(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
/* late-scene NPC sprite frames — z_npc_84 to z_npc_95 */
void NEOGEO_USER showScreen94(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen95(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen96(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen97(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen98(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen99(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen100(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen101(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen102(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen103(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen104(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen105(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
/* late-scene backdrop */
void NEOGEO_USER showScreen106(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen107(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
/* title screens from artbox/in/titles/ */
void NEOGEO_USER showScreen108(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER showScreen109(int x0, int y0, int xr, int yr, int min_crt_sz, uint16_t backdrop, uint16_t sprite_base);
void NEOGEO_USER show3DRaycaster(void);
int NEOGEO_USER playgame(void);

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

static const uint8_t g_demo_gallery_frames[] = {
    74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87
};

static const uint8_t g_demo_cast_left_frames[] = {
    87, 88, 89, 90, 91, 92, 93
};

static const uint8_t g_demo_cast_right_frames[] = {
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
#define DEMO_WALK_BASELINE 552
#define DEMO_DUEL_BASELINE 552
#define DEMO_REEL_BASELINE 552
#define DEMO_NPC_BASELINE 548
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

static const SceneScreenFn g_demo_gallery_palette_loaders[] = {
    showScreen13, showScreen14, showScreen15, showScreen16, showScreen17,
    showScreen18, showScreen19, showScreen20, showScreen21, showScreen23,
    showScreen24, showScreen25, showScreen26, showScreen27, showScreen28,
    showScreen29, showScreen30, showScreen31, showScreen34, showScreen35,
    showScreen36, showScreen37, showScreen38, showScreen39
};

static const SceneScreenFn g_demo_npc_palette_loaders[] = {
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
    uint8_t screen_id;

    if (!c || !meta) return;

    new_tile = (uint16_t)(
        meta->tile_base + ((uint16_t)meta->tile_row_start * 16u) + meta->tile_col_start
    );

    screen_id = (uint8_t)((meta->tile_base / 256u) + 1u);

    /*
     * Use the generated cropped sprite rectangle as the visible character.
     * Runtime x_pad/y_pad alignment caused special frames to move too high
     * or lose body parts.  Keep offsets zero and anchor the visible sprite
     * consistently in the engine field.
     */
    new_strips = meta->strips ? meta->strips : 1;
    new_rows = meta->active_rows ? meta->active_rows : 1;
new_palette = meta->palette_bank;

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


static uint8_t NEOGEO_USER demo_screen_id_from_palette(uint8_t palette_bank)
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

static void NEOGEO_USER demo_draw_stage_background(uint16_t screen_id)
{
    if (screen_id == DEMO_LATE_SCENE_BG) {
        
    }
}

static void NEOGEO_USER demo_draw_walk_scene(void)
{
    ng_level_set_backdrop(0x0FFF);
    setBACKDROP(BLACK);
}

static void NEOGEO_USER demo_cycle_showcase(
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
    ng_level_set_backdrop(0x0FFF);

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
    ng_level_set_backdrop(0x0FFF);
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

    ng_prop_set(NG_PROP_GROUP_DEBUG, 0, ng_progress_value(DEMO_PROGRESS_PHASE));
    ng_prop_set(NG_PROP_GROUP_DEBUG, 1, ng_chars_count());
    ng_prop_set(NG_PROP_GROUP_DEBUG, 2, ng_timer_value(DEMO_TIMER_DUEL));

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

static void NEOGEO_USER demo_force_reel_npcs(void)
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
    ng_level_set_backdrop(0x0FFF);
    setBACKDROP(0x0FFF);

    demo_sync_action_sprite(warrior);

    ng_fix_puts(2, 1, "2D ENGINE CLEAN PASS", 1);
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
            ng_level_set_backdrop(0x0FFF);
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
    NGNpc *npc_center = 0; /* NPC disabled in clean engine pass */
    NGNpc *npc_left = 0; /* NPC disabled in clean engine pass */
    NGNpc *npc_right = 0; /* NPC disabled in clean engine pass */

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
    ng_level_set_background(1);  /* screen 1 = 0.png behind characters */
    ng_level_set_backdrop(0x0FFF);
    ng_level_set_world_bounds(0, 0, 319, 223);
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


static void NEOGEO_USER demo_show_pre_engine_screen(uint8_t screen_id, int x0, int y0)
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

    /* Show 0.png background while announcing the 2D engine transition */
    showScreen1(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
    fixtext_out(2, 1, "NEOGEO 2D ENGINE", 1);
    fixtext_out(2, 3, "HARDWARE SPRITE SCALING", 0);
    fixtext_out(2, 5, "68000 PHYSICS ENGINE", 1);
    fixtext_out(2, 7, "PREPARING ENGINE...", 2);
    playVoiceCue(SOUND_VOICE_GET_READY);

    for (hold = 0; hold < 120; hold++) {
        waitVbl();
    }

    demo_hard_clear_scene();
}

static void NEOGEO_USER demo_run_engine_scene(void)
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

    /* Pseudo-3D floor perspective demo */
    showPseudo3DLoop();

    /* Software 3D DDA raycaster demo */
    show3DRaycaster();
}

/*
 * Step through the seven primary character frames at a fixed delay so the
 * current scene music can be judged against predictable animation timing.
 */

void NEOGEO_USER showWalkDemo(int loops, int delay_ms) {
    int i = 0;
    int frame = 0;
	clearFix();
    clearSprs();
    /*showScreen1(16, 520, 0xF, 0xAF, 9, 0x0FFF, 0);*/
    for (i = 0; i < loops; i++) {
        waitVbl();
        if (frame == 0)
            showScreen2(16, 520, 0xF, 0xAF, 9, 0xFFF, 0);
        if (frame == 1)
            showScreen3(16, 520, 0xF, 0xAF, 9, 0xFFF, 0);
        if (frame == 2)
            showScreen4(16, 520, 0xF, 0xAF, 9, 0xFFF, 0);
        if (frame == 3)
            showScreen5(16, 520, 0xF, 0xAF, 9, 0xFFF, 0);
        if (frame == 4)
            showScreen6(16, 520, 0xF, 0xAF, 9, 0xFFF, 0);
        if (frame == 5)
            showScreen7(16, 520, 0xF, 0xAF, 9, 0xFFF, 0);
        if (frame == 6)
            showScreen8(16, 520, 0xF, 0xAF, 9, 0xFFF, 0);
        cyclexms(delay_ms);
        frame++;
        if (frame >= 7)
            frame = 0;
    }
}

/* Title screen — Eagle Soft logo with blinking INSERT COIN, auto-advances after 6 seconds. */
void NEOGEO_USER showTitleScreen(void) {
    int i;
    clearFix();
    clearSprs();
    soundSceneReset();
    soundStopAll();
    setBACKDROP(BLACK);

    /* Title image 1 — 8.png */
    showScreen108(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
    playVoiceCue(SOUND_SFX_STRING_PHRASE);


    /* Hold title image 1 for ~3 seconds then cross-fade to image 2 */
    cyclexs(3);

    clearSprs();
    showScreen109(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
    playSFX(SOUND_SFX_LOW_DRUM);

    /* Blink HIT START for 5 seconds */
    for (i = 0; i < 300; i++) {
        waitVbl();
        if (i % 60 < 30)
            fixtext_out(14, 26, "PRESS START", 0);
        else
            fixtext_out(14, 26, "           ", 0);
    }

    soundStopAll();
    clearFix();
    clearSprs();
}

/* Main demo game entry: reset the visible state, then hand off to playgame(). */
void NEOGEO_USER maingame(void) {
   
    clearFix();
    clearSprs();
    playgame();
}



/* The game loop uses the high-level music helper so restart/volume policy stays centralized. */
int NEOGEO_USER playgame(void) {
    clearFix();
    clearSprs();
    demo_run_engine_scene();
    showGameOver();
    return 0;
}

/* Full YM2610 sound driver showcase: FM, SSG, ADPCM-A, ADPCM-B, full mix. */
void NEOGEO_USER showSoundDemo(void) {
    clearFix();
    clearSprs();
    soundSceneReset();
    fixtext_out(2, 1, "YM2610 SOUND DRIVER", 0);

    /* FM channels — OPN2 4-op FM synthesis */
    fixtext_out(2, 3, "FM  ATTRACT FAST", 1);
    soundSetFMVolume(0x0C);
    soundSetSSGVolume(0x00);
    soundSetADPCMAVolume(0x00);
    soundSetADPCMBVolume(0x00);
    playFMTrack(SOUND_FM_ATTRACT_FAST);
    cyclexs(2);
    fixtext_out(2, 4, "FM  DUEL SUSPENSE", 2);
    soundStopMusic();
    playFMTrack(SOUND_FM_DUEL_SUSPENSE);
    cyclexs(2);
    fixtext_out(2, 5, "FM  VICTORY JINGLE", 0);
    soundStopMusic();
    playFMTrack(SOUND_FM_VICTORY_JINGLE);
    cyclexs(2);
    soundStopAll();

    /* SSG — AY-3-8910 3-voice PSG oscillators */
    fixtext_out(2, 7, "SSG ARCADE ALERT", 1);
    soundSetSSGVolume(0x0F);
    soundSetFMVolume(0x00);
    playSSGTrack(SOUND_SSG_ARCADE_ALERT);
    soundSetSSGPreset(1);
    cyclexs(2);
    fixtext_out(2, 8, "SSG MENU LOOP", 2);
    playSSGTrack(SOUND_SSG_MENU_LOOP);
    cyclexs(2);
    fixtext_out(2, 9, "SSG INSERT COIN", 0);
    playSSGTrack(SOUND_SSG_INSERT_COIN);
    soundSetSSGPreset(0);
    cyclexs(2);
    soundStopAll();

    /* ADPCM-A — 6-channel 4-bit PCM samples */
    clearFix();
    clearSprs();
    fixtext_out(2, 1, "YM2610 SOUND DRIVER", 0);
    fixtext_out(2, 3, "ADPCM-A 6CH SAMPLES", 1);
    soundSetADPCMAVolume(0x3F);
    playSFX(SOUND_SFX_INTRO_TAIKO);    cyclexs(1);
    playSFX(SOUND_SFX_TITLE_GONG);     cyclexms(600);
    playSFX(SOUND_SFX_BLADE_WHOOSH);   cyclexms(400);
    playSFX(SOUND_SFX_IMPACT_HIT);     cyclexms(400);
    playSFX(SOUND_SFX_STRING_PHRASE);  cyclexms(700);
    playSFX(SOUND_SFX_LOW_DRUM);       cyclexms(500);
    playSFX(SOUND_SFX_COIN_CHIME);     cyclexms(500);
    playSFX(SOUND_SFX_SHORT_SHOUT);    cyclexms(600);

    /* ADPCM-B — 1-channel high-quality PCM bed + voice over */
    fixtext_out(2, 5, "ADPCM-B MUSIC BED", 2);
    soundSetADPCMBVolume(0xBC);
    soundSetADPCMAVolume(0x3F);
    playSFXB(SOUND_BED_STAGE_TWO);
    cyclexms(500);
    fixtext_out(2, 7, "VOICE CUE  GET READY", 0);
    playVoiceCue(SOUND_VOICE_GET_READY);
    cyclexs(2);
    fixtext_out(2, 9, "VOICE CUE  ATTACK", 1);
    playVoiceCue(SOUND_VOICE_ATTACK);
    cyclexs(2);

    /* Full mix — all channels simultaneously */
    fixtext_out(2, 11, "FULL MIX ALL CHANNELS", 2);
    soundApplyMix(0x30, 0xB8, 0x08, 0x08);
    playFMTrack(SOUND_FM_BASS_MOTIF);
    playSSGTrack(SOUND_SSG_INSERT_COIN);
    soundSetSSGPreset(1);
    cyclexms(500);
    playSFX(SOUND_SFX_BLADE_WHOOSH);
    cyclexs(3);

    soundFadeOutSpeed(8);
    cyclexs(2);
    soundStopAll();
    clearFix();
    clearSprs();
}

/* ---- 3D Software Raycaster (DDA) -----------------------------------------
 * Camera navigates an 8x8 maze using DDA ray casting.
 * Wall textures use eyecatcher NPC sprite tiles (screens 93-99).
 * 20 columns × 16px = 320px, fisheye-corrected, full-screen.
 * --------------------------------------------------------------------------*/
#pragma GCC push_options
#pragma GCC optimize ("O0")

#define S3D_MAP_W    8
#define S3D_MAP_H    8
#define S3D_FP       6       /* fixed-point bits: 1.0 = 64 */
#define S3D_ONE      64
#define S3D_COLS     20      /* 20 strips × 16px = 320px full screen */
#define S3D_TILES    14      /* tiles per column sprite = 14×16 = 224px max */
#define S3D_MAXH     224
#define S3D_SCRCY    112     /* screen centre Y */
#define S3D_PROJ     2048    /* projection constant → wall_h = PROJ/dist */
#define S3D_FOV      43      /* ≈60° in 256-unit circle space */

static const int8_t s3d_sintab[256] = {
       0,   2,   3,   5,   6,   8,   9,  11,  12,  14,  15,  17,  18,  20,  21,  23,
      24,  26,  27,  28,  30,  31,  32,  34,  35,  36,  38,  39,  40,  41,  42,  43,
      45,  46,  47,  48,  49,  50,  51,  52,  52,  53,  54,  55,  56,  56,  57,  58,
      58,  59,  59,  60,  60,  61,  61,  61,  62,  62,  62,  63,  63,  63,  63,  63,
      63,  63,  63,  63,  63,  63,  62,  62,  62,  61,  61,  61,  60,  60,  59,  59,
      58,  58,  57,  56,  56,  55,  54,  53,  52,  52,  51,  50,  49,  48,  47,  46,
      45,  43,  42,  41,  40,  39,  38,  36,  35,  34,  32,  31,  30,  28,  27,  26,
      24,  23,  21,  20,  18,  17,  15,  14,  12,  11,   9,   8,   6,   5,   3,   2,
       0,  -2,  -3,  -5,  -6,  -8,  -9, -11, -12, -14, -15, -17, -18, -20, -21, -23,
     -24, -26, -27, -28, -30, -31, -32, -34, -35, -36, -38, -39, -40, -41, -42, -43,
     -45, -46, -47, -48, -49, -50, -51, -52, -52, -53, -54, -55, -56, -56, -57, -58,
     -58, -59, -59, -60, -60, -61, -61, -61, -62, -62, -62, -63, -63, -63, -63, -63,
     -63, -63, -63, -63, -63, -63, -62, -62, -62, -61, -61, -61, -60, -60, -59, -59,
     -58, -58, -57, -56, -56, -55, -54, -53, -52, -52, -51, -50, -49, -48, -47, -46,
     -45, -43, -42, -41, -40, -39, -38, -36, -35, -34, -32, -31, -30, -28, -27, -26,
     -24, -23, -21, -20, -18, -17, -15, -14, -12, -11,  -9,  -8,  -6,  -5,  -3,  -2
};
#define S3D_SIN(a)  s3d_sintab[(uint8_t)(a)]
#define S3D_COS(a)  s3d_sintab[(uint8_t)((uint8_t)(a) + 64u)]

static const uint8_t s3d_map[S3D_MAP_H][S3D_MAP_W] = {
    {1,1,1,1,1,1,1,1},
    {1,0,0,1,0,0,0,1},
    {1,0,1,1,0,1,0,1},
    {1,0,0,0,0,1,0,1},
    {1,0,1,0,0,0,0,1},
    {1,0,1,1,0,1,0,1},
    {1,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1},
};

/* DDA ray cast — returns approximate step count proportional to distance.
 * Dominant axis advances 8 FP units per step (1/8 cell); 72 steps max.
 * (cam_x, cam_y) in FP units where S3D_ONE = 1 map cell.
 * (rdx, rdy) from sin table: range -63..+63.  Returns 1..72. */
static uint8_t NEOGEO_USER s3d_cast(int16_t cam_x, int16_t cam_y,
                                     int8_t rdx, int8_t rdy)
{
    int8_t  ax = rdx, ay = rdy, sx, sy;
    int16_t rx = cam_x, ry = cam_y;
    uint8_t i;

    if (ax < 0) ax = -ax;
    if (ay < 0) ay = -ay;

    if (ax >= ay) {
        sx = (rdx >= 0) ? 8 : -8;
        sy = (rdx != 0) ? (int8_t)((int16_t)rdy * 8 / rdx) : 0;
    } else {
        sy = (rdy >= 0) ? 8 : -8;
        sx = (rdy != 0) ? (int8_t)((int16_t)rdx * 8 / rdy) : 0;
    }
    if (!sx && rdx) sx = (rdx > 0) ? 1 : -1;
    if (!sy && rdy) sy = (rdy > 0) ? 1 : -1;

    for (i = 1; i < 72; i++) {
        int8_t mx, my;
        rx += sx;
        ry += sy;
        mx = (int8_t)((uint16_t)rx >> S3D_FP);
        my = (int8_t)((uint16_t)ry >> S3D_FP);
        if ((uint8_t)mx >= S3D_MAP_W || (uint8_t)my >= S3D_MAP_H) return i;
        if (s3d_map[my][mx]) return i;
    }
    return 72;
}

void NEOGEO_USER show3DRaycaster(void)
{
    uint16_t s3d_tiles[S3D_TILES];
    uint16_t s3d_attrs[S3D_TILES];
    /* Camera starts in open cell (1.5, 1.5) facing right */
    int16_t  cam_x    = (int16_t)((1 << S3D_FP) + S3D_ONE / 2);
    int16_t  cam_y    = (int16_t)((1 << S3D_FP) + S3D_ONE / 2);
    uint8_t  cam_a    = 0;
    uint8_t  stuck    = 0;
    uint16_t frame;
    uint8_t  col, t;
    int8_t   fwd_dx, fwd_dy;
    uint8_t  fwd_dist;

    clearFix();
    clearSprs();

    /* Preload eyecatcher palette banks (screens 93-99) as wall textures */
    showScreen93(16, 24, 0xF, 0xAF, 16, 0x0000, 0); waitVbl();
    showScreen94(16, 24, 0xF, 0xAF, 16, 0x0000, 0); waitVbl();
    showScreen95(16, 24, 0xF, 0xAF, 16, 0x0000, 0); waitVbl();
    showScreen96(16, 24, 0xF, 0xAF, 16, 0x0000, 0); waitVbl();
    showScreen97(16, 24, 0xF, 0xAF, 16, 0x0000, 0); waitVbl();
    showScreen98(16, 24, 0xF, 0xAF, 16, 0x0000, 0); waitVbl();
    showScreen99(16, 24, 0xF, 0xAF, 16, 0x0000, 0); waitVbl();
    clearSprs();
    for (col = 0; col < S3D_COLS; col++)
        vram_SCB234((uint16_t)(SCB3_ADDR + col), 0u);

    fixtext_out(2, 1, "3D DDA RAYCASTER", 0);
    fixtext_out(2, 2, "EYECATCHER NPC SPRITES", 1);
    fixtext_out(2, 4, "EAGLE SOFTWARE  2026", 0);

    for (frame = 0; frame < (uint16_t)NG_MS_TO_FRAMES(12000); frame++) {
        /* Rotation: 1 angle unit per 2 frames */
        if (frame & 1u) cam_a++;

        /* Camera forward movement */
        fwd_dx = (int8_t)S3D_COS(cam_a);
        fwd_dy = (int8_t)S3D_SIN(cam_a);
        fwd_dist = s3d_cast(cam_x, cam_y, fwd_dx, fwd_dy);

        if (fwd_dist > 8u) {
            /* Path clear — advance faster than before */
            cam_x = (int16_t)(cam_x + (int16_t)((int16_t)fwd_dx >> 4));
            cam_y = (int16_t)(cam_y + (int16_t)((int16_t)fwd_dy >> 4));
            stuck = 0;
        } else {
            /* Near wall — turn right ~22.5° */
            cam_a = (uint8_t)(cam_a + 16u);
            if (++stuck > 40u) {
                cam_x = (int16_t)((3 << S3D_FP) + S3D_ONE / 2);
                cam_y = (int16_t)((3 << S3D_FP) + S3D_ONE / 2);
                stuck = 0;
            }
        }

        for (col = 0; col < S3D_COLS; col++) {
            int16_t  fov_off = (int16_t)col * (int16_t)S3D_FOV / (int16_t)S3D_COLS
                               - (int16_t)(S3D_FOV / 2);
            uint8_t  ray_a   = (uint8_t)((int16_t)cam_a + fov_off);
            int8_t   rdx     = (int8_t)S3D_COS(ray_a);
            int8_t   rdy     = (int8_t)S3D_SIN(ray_a);
            uint8_t  dist    = s3d_cast(cam_x, cam_y, rdx, rdy);

            /* Fisheye correction */
            uint8_t  abs_fov  = (fov_off < 0) ? (uint8_t)(-(int16_t)fov_off) : (uint8_t)fov_off;
            uint8_t  cos_fov  = (uint8_t)S3D_COS(abs_fov);
            uint16_t perp_d   = (uint16_t)((uint16_t)dist * cos_fov / 63u);
            if (perp_d < 1u) perp_d = 1u;

            uint16_t wall_h   = (uint16_t)(S3D_PROJ / perp_d);
            if (wall_h > S3D_MAXH) wall_h = S3D_MAXH;
            if (wall_h < 4u)       wall_h = 4u;

            uint8_t  yscale   = (uint8_t)((wall_h * 255u) / S3D_MAXH);
            uint16_t vram_y   = (uint16_t)(384u + wall_h / 2u);

            /* Each column maps to one of 7 eyecatcher NPC sprite screens (93-99) */
            uint8_t  ec_idx      = (uint8_t)((uint16_t)col * 7u / (uint16_t)S3D_COLS);
            uint16_t ec_tile_base = DEMO_SCREEN_TILE((uint16_t)(93u + ec_idx));
            uint8_t  ec_pal      = (uint8_t)DEMO_SCREEN_PALETTE((uint16_t)(93u + ec_idx));

            for (t = 0; t < S3D_TILES; t++) {
                s3d_tiles[t] = ec_tile_base + (uint16_t)t;
                s3d_attrs[t] = setSCB1_2(ec_pal, 0, 0, 0, 0, 0);
            }

            vram_sprite(
                (uint16_t)(col * 64u), 1u, (uint16_t)col,
                s3d_tiles, s3d_attrs, (uint8_t)S3D_TILES,
                setSCB2(0xFFu, yscale),
                setSCB3(vram_y, 0u, (uint16_t)S3D_TILES),
                setSCB4((uint16_t)(col * 16u))
            );
        }
        waitVbl();
    }

    for (col = 0; col < S3D_COLS; col++)
        vram_SCB234((uint16_t)(SCB3_ADDR + col), 0u);
    clearFix();
    clearSprs();
}

#pragma GCC pop_options

void NEOGEO_USER showGameOver(void) {
    soundStopAll();
    clearFix();
    clearSprs();
    soundPlayGameLoop(SOUND_MUSIC_SAMURAI_ENDING_SCENE);
    fixtext_out(15, 10, "GAME OVER", 0);
    cyclexs(4);
    soundStopAll();
}

/* Boot intro: typewriter "EAGLE SOFTWARE" with voice cues, then Eagle Soft logo. */
void NEOGEO_USER showEagleIntro(void) {
    static const char eagle_word[] = "EAGLE";
    static const char soft_word[] = "SOFTWARE";
    uint16_t fix_pal[16];
    char ch[2];
    int i;

    clearFix();
    clearSprs();
    soundSceneReset();
    soundSetADPCMAVolume(0x3F);
    soundSetADPCMBVolume(0xB8);
    soundSetSSGVolume(0x00);

    /* FIX palettes: white/cyan/yellow text on black for typewriter and shimmer. */
    setpal(fix_pal, 0x8000, WHITE, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES);
    setpal(fix_pal, 0x8000, CYAN, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES + PALOFFSET);
    setpal(fix_pal, 0x8000, YELLOW, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
           BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK);
    load_palettes(fix_pal, PALETTES + PALOFFSET * 2);

    /* Eagle fanfare boot melody */
   /* soundPlayTitleMusic(0);*/

    ch[1] = '\0';

    /* "Get Ready" voice + typewriter "EAGLE" — centered at tile col 17 row 13 */
    playVoiceCue(SOUND_VOICE_GET_READY);
    for (i = 0; i < 5; i++) {
        ch[0] = eagle_word[i];
        fixtext_out(17 + i, 13, ch, 0);
        cyclexms(70);
    }

    cyclexms(20);

    /* "Attack" voice + typewriter "SOFTWARE" — centered at tile col 16 row 15 */
    playVoiceCue(SOUND_VOICE_ATTACK);
    for (i = 0; i < 8; i++) {
        ch[0] = soft_word[i];
        fixtext_out(16 + i, 15, ch, 0);
        cyclexms(60);
    }

for (i = 0; i < 18; i++) {
    short eaglePal    = (short)((i & 1) ? PAL_RED  : PAL_BLUE);
    short softwarePal = (short)((i & 1) ? PAL_BLUE : PAL_RED);

    fixtext_out(17, 13, "EAGLE", eaglePal);
    fixtext_out(16, 15, "SOFTWARE", softwarePal);

    if ((i & 3) == 0) {
        playSFX(SOUND_SFX_STRING_PHRASE);
    }

    cyclexms(35);
}

    /* Eagle Soft title reveal */
    clearFix();
    clearSprs();
    playSFX(SOUND_SFX_TITLE_GONG);
    showScreen107(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
    cycle1s();
    playSFX(SOUND_SFX_LOW_DRUM);
    cycle1s();

    soundFadeOutSpeed(4);
    cyclexms(170);
    soundStopAll();
    clearFix();
    clearSprs();
}

/* Slide through sprite asset screens 2–8 as a character parade. */
void NEOGEO_USER showCharacterParade(void) {
    uint8_t hold;
    clearFix();
    clearSprs();

    /* Show 0.png background with SDK feature labels */
    showScreen1(16, 24, 0xF, 0xAF, 16, 0x0000, 0);
    fixtext_out(2,  1, "NEOGEO SDK FEATURES", 0);
    fixtext_out(2,  3, "HARDWARE SPRITES", 1);
    fixtext_out(2,  5, "PALETTE ANIMATION", 2);
    fixtext_out(2,  7, "SPRITE SCALING SCB2", 0);
    fixtext_out(2,  9, "2D ENGINE PHYSICS", 1);
    fixtext_out(2, 11, "PSEUDO-3D FLOOR", 2);
    fixtext_out(2, 13, "DDA RAYCASTER 68000", 0);
    fixtext_out(2, 15, "ADPCM SOUND ENGINE", 1);

    for (hold = 0; hold < 240; hold++) waitVbl();

    clearFix();
    clearSprs();
}

/* Pseudo-3D floor effect — Mode-7 style using sprite Y-scale perspective.
 *
 * NG_P3D_BANDS horizontal scan bands are stacked from near (screen bottom)
 * to the horizon (screen centre).  Each band is 1 tile tall; its vertical
 * shrink register (yr) increases with distance so near bands appear taller
 * and far bands appear very thin — creating foreshortening.  Horizontal
 * width is also proportional to depth so the floor tapers toward the horizon.
 * A scroll counter increments each frame to fake forward motion.
 *
 * Uses sprite slots 0..NG_P3D_BANDS*NG_P3D_STRIPS-1.  Call only when there
 * are no engine characters active (standalone demo scene).
 */
#define NG_P3D_BANDS    10
#define NG_P3D_STRIPS   16
#define NG_P3D_SLOT0    0
#define NG_P3D_NEAR_Y   220
#define NG_P3D_FAR_Y    112

void NEOGEO_USER showPseudo3DLoop(void) {
    uint16_t frame;
    uint16_t duration = (uint16_t)NG_MS_TO_FRAMES(8000);
    uint8_t  band;
    uint8_t  strip;
    uint16_t scb2, scb3, scb4;
    uint16_t tiles[1];
    uint16_t attrs[1];

    clearFix();
    clearSprs();
    fixtext_out(2,  2, "PSEUDO-3D FLOOR", 0);
    fixtext_out(2,  4, "SPRITE Y-SCALE PERSPECTIVE", 0);
    fixtext_out(2,  6, "NG HARDWARE SCALING TRICK", 0);

    for (frame = 0; frame < duration; frame++) {
        uint8_t scroll = (uint8_t)(frame >> 1);

        for (band = 0; band < NG_P3D_BANDS; band++) {
            /* depth: 0 = far (horizon), NG_P3D_BANDS-1 = near (bottom) */
            uint8_t depth = (uint8_t)(NG_P3D_BANDS - 1u - band);

            /* Y-scale grows with depth: far=0x08 (very thin), near=0xFF (full) */
            uint8_t yscale = (uint8_t)(0x08u + (uint16_t)depth * (0xFF - 0x08) / (NG_P3D_BANDS - 1));

            /* Screen Y: far bands cluster near horizon, near at bottom */
            int16_t screen_y = (int16_t)(NG_P3D_FAR_Y
                + (int32_t)(NG_P3D_NEAR_Y - NG_P3D_FAR_Y) * band / (NG_P3D_BANDS - 1));

            /* Width: tapers toward horizon */
            int16_t band_w = (int16_t)(16
                + (int32_t)(NG_P3D_STRIPS * 16 - 16) * depth / (NG_P3D_BANDS - 1));
            int16_t start_x = (int16_t)((320 - band_w) / 2);
            uint8_t n_strips = (uint8_t)((band_w + 15) / 16);
            if (n_strips > NG_P3D_STRIPS) n_strips = NG_P3D_STRIPS;

            /* Palette: cycle through stage palettes for depth colour bands */
            uint8_t pal = (uint8_t)(NG_PAL_STAGE_BASE + (depth & 3u));

            for (strip = 0; strip < n_strips; strip++) {
                uint16_t slot = (uint16_t)(NG_P3D_SLOT0 + band * NG_P3D_STRIPS + strip);

                /* Tile index scrolls to simulate forward motion; wrap in 256 */
                tiles[0] = (uint16_t)((scroll + strip + band * 2u) & 0xFFu);
                attrs[0] = setSCB1_2(pal, 0, 0, 0, 0, 0);

                scb2 = setSCB2(0xFu, yscale);
                if (strip == 0u) {
                    scb3 = setSCB3((uint16_t)(496 - (int)screen_y), 0, 1);
                    scb4 = setSCB4((uint16_t)start_x);
                } else {
                    scb3 = 0x0041u; /* sticky chain bit + height=1 */
                    scb4 = setSCB4((uint16_t)(start_x + strip * 16));
                }
                vram_sprite((uint16_t)((uint32_t)slot * 64u), 1u, slot,
                            tiles, attrs, 1u, scb2, scb3, scb4);
            }

            /* Hide unused strips in this band */
            for (strip = n_strips; strip < NG_P3D_STRIPS; strip++) {
                uint16_t slot = (uint16_t)(NG_P3D_SLOT0 + band * NG_P3D_STRIPS + strip);
                vram_SCB234((uint16_t)(SCB3_ADDR + slot), 0u);
            }
        }

        waitVbl();
    }

    /* Clear all 3D band sprites */
    for (band = 0; band < NG_P3D_BANDS; band++) {
        for (strip = 0; strip < NG_P3D_STRIPS; strip++) {
            uint16_t slot = (uint16_t)(NG_P3D_SLOT0 + band * NG_P3D_STRIPS + strip);
            vram_SCB234((uint16_t)(SCB3_ADDR + slot), 0u);
        }
    }
    clearFix();
    clearSprs();
}

void NEOGEO_USER showScreen1(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 1 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal1[16];
setpal(pal1,0x0,0x7113,0x7fff,0x5451,0x4231,0x7001,0x1871,0x0,0x1310,0x7747,0x753,0x5448,0x1121,0x433,0x4b53,0x7d92);
uint16_t spriteMapS1_1[16] = {0x0,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0};
uint16_t spriteMapS1_2[16] = {0x1,0x11,0x21,0x31,0x41,0x51,0x61,0x71,0x81,0x91,0xa1,0xb1,0xc1,0xd1,0xe1,0xf1};
uint16_t spriteMapS1_3[16] = {0x2,0x12,0x22,0x32,0x42,0x52,0x62,0x72,0x82,0x92,0xa2,0xb2,0xc2,0xd2,0xe2,0xf2};
uint16_t spriteMapS1_4[16] = {0x3,0x13,0x23,0x33,0x43,0x53,0x63,0x73,0x83,0x93,0xa3,0xb3,0xc3,0xd3,0xe3,0xf3};
uint16_t spriteMapS1_5[16] = {0x4,0x14,0x24,0x34,0x44,0x54,0x64,0x74,0x84,0x94,0xa4,0xb4,0xc4,0xd4,0xe4,0xf4};
uint16_t spriteMapS1_6[16] = {0x5,0x15,0x25,0x35,0x45,0x55,0x65,0x75,0x85,0x95,0xa5,0xb5,0xc5,0xd5,0xe5,0xf5};
uint16_t spriteMapS1_7[16] = {0x6,0x16,0x26,0x36,0x46,0x56,0x66,0x76,0x86,0x96,0xa6,0xb6,0xc6,0xd6,0xe6,0xf6};
uint16_t spriteMapS1_8[16] = {0x7,0x17,0x27,0x37,0x47,0x57,0x67,0x77,0x87,0x97,0xa7,0xb7,0xc7,0xd7,0xe7,0xf7};
uint16_t spriteMapS1_9[16] = {0x8,0x18,0x28,0x38,0x48,0x58,0x68,0x78,0x88,0x98,0xa8,0xb8,0xc8,0xd8,0xe8,0xf8};
uint16_t spriteMapS1_10[16] = {0x9,0x19,0x29,0x39,0x49,0x59,0x69,0x79,0x89,0x99,0xa9,0xb9,0xc9,0xd9,0xe9,0xf9};
uint16_t spriteMapS1_11[16] = {0xa,0x1a,0x2a,0x3a,0x4a,0x5a,0x6a,0x7a,0x8a,0x9a,0xaa,0xba,0xca,0xda,0xea,0xfa};
uint16_t spriteMapS1_12[16] = {0xb,0x1b,0x2b,0x3b,0x4b,0x5b,0x6b,0x7b,0x8b,0x9b,0xab,0xbb,0xcb,0xdb,0xeb,0xfb};
uint16_t spriteMapS1_13[16] = {0xc,0x1c,0x2c,0x3c,0x4c,0x5c,0x6c,0x7c,0x8c,0x9c,0xac,0xbc,0xcc,0xdc,0xec,0xfc};
uint16_t spriteMapS1_14[16] = {0xd,0x1d,0x2d,0x3d,0x4d,0x5d,0x6d,0x7d,0x8d,0x9d,0xad,0xbd,0xcd,0xdd,0xed,0xfd};
uint16_t spriteMapS1_15[16] = {0xe,0x1e,0x2e,0x3e,0x4e,0x5e,0x6e,0x7e,0x8e,0x9e,0xae,0xbe,0xce,0xde,0xee,0xfe};
uint16_t spriteMapS1_16[16] = {0xf,0x1f,0x2f,0x3f,0x4f,0x5f,0x6f,0x7f,0x8f,0x9f,0xaf,0xbf,0xcf,0xdf,0xef,0xff};
load_palettes(pal1,PALETTES+PALOFFSET*16);
uint16_t SCB1_2common = setSCB1_2(16,0,0,0,0,0);
uint16_t spal1_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal1_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS1_1,spal1_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS1_2,spal1_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS1_3,spal1_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS1_4,spal1_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS1_5,spal1_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS1_6,spal1_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS1_7,spal1_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS1_8,spal1_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS1_9,spal1_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS1_10,spal1_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS1_11,spal1_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS1_12,spal1_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS1_13,spal1_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS1_14,spal1_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS1_15,spal1_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS1_16,spal1_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen2(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 2 ******************************************/
uint16_t  pal2[16];
setpal(pal2,0x0,0x666,0x1111,0x7b72,0x7835,0x6ec9,0x2422,0x2fc5,0x7235,0x5da4,0x3edd,0x6632,0x3952,0x3a99,0x5a85,0x223);
load_palettes(pal2,PALETTES+PALOFFSET*17);
}


void NEOGEO_USER showScreen3(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 3 ******************************************/
uint16_t  pal3[16];
setpal(pal3,0x0,0xd94,0x5111,0x999,0x632,0x6ddd,0x2b86,0x6445,0x7113,0x6946,0x5321,0x1fd9,0x6ec5,0x7642,0x1346,0x5962);
load_palettes(pal3,PALETTES+PALOFFSET*18);
}


void NEOGEO_USER showScreen4(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 4 ******************************************/
uint16_t  pal4[16];
setpal(pal4,0x0,0x444,0x4642,0x2fdb,0x5246,0x2c83,0x7735,0x963,0x5b96,0x1111,0x3421,0x6113,0xfd8,0x7eb4,0xddd,0x1887);
load_palettes(pal4,PALETTES+PALOFFSET*19);
}


void NEOGEO_USER showScreen5(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 5 ******************************************/
uint16_t  pal5[16];
setpal(pal5,0x0,0x7444,0x7ddd,0x2d94,0x3842,0x1111,0x4a86,0x6113,0x4ec5,0x5ed9,0x3988,0x422,0x7835,0x3a62,0x1642,0x6236);
load_palettes(pal5,PALETTES+PALOFFSET*20);
}


void NEOGEO_USER showScreen6(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 6 ******************************************/
uint16_t  pal6[16];
setpal(pal6,0x0,0x7753,0x7952,0x1111,0x1fd9,0x5235,0x6312,0x6eb5,0x7834,0x1666,0x532,0x3a99,0x333,0x1b95,0x3c83,0x3edd);
load_palettes(pal6,PALETTES+PALOFFSET*21);
}


void NEOGEO_USER showScreen7(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 7 ******************************************/
uint16_t  pal7[16];
setpal(pal7,0x0,0x1962,0x1346,0x1fd9,0x642,0x1111,0x5444,0x5845,0x5ec5,0x4322,0x7888,0x5a53,0x6ddd,0x3a75,0xd94,0x4123);
load_palettes(pal7,PALETTES+PALOFFSET*22);
}


void NEOGEO_USER showScreen8(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 8 ******************************************/
uint16_t  pal8[16];
setpal(pal8,0x0,0x1421,0x2a63,0x2da7,0xeee,0x6a99,0x2a45,0x6667,0x3fda,0x1443,0x5111,0x1235,0x3742,0x1523,0x5c93,0x4ec5);
load_palettes(pal8,PALETTES+PALOFFSET*23);
}


void NEOGEO_USER showScreen9(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 9 ******************************************/
uint16_t  pal9[16];
setpal(pal9,0x0,0x111,0x5665,0x1852,0x1fd9,0x6113,0x5ec5,0x1aa9,0x3c83,0x4532,0x5224,0x7322,0x3edd,0x6b86,0x7944,0x4246);
load_palettes(pal9,PALETTES+PALOFFSET*24);
}


void NEOGEO_USER showScreen10(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 10 ******************************************/
uint16_t  pal10[16];
setpal(pal10,0x0,0x1eed,0x5c93,0x5123,0x1421,0x3b85,0x2336,0x2545,0x3742,0x5ec7,0x4999,0x5962,0x4845,0x1111,0x7eda,0x5ec4);
load_palettes(pal10,PALETTES+PALOFFSET*25);
}


void NEOGEO_USER showScreen11(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 11 ******************************************/
uint16_t  pal11[16];
setpal(pal11,0x0,0x666,0x6b84,0x6853,0x6532,0x4423,0x6322,0x2a99,0x7ec9,0x4a65,0x2eb5,0x4123,0x1eed,0x6236,0x835,0x1111);
load_palettes(pal11,PALETTES+PALOFFSET*26);
}


void NEOGEO_USER showScreen12(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 12 ******************************************/
uint16_t  pal12[16];
setpal(pal12,0x0,0x3742,0x2d94,0x7a98,0x6667,0x6432,0x1111,0x333,0xa45,0x6124,0x6ec9,0x6ddd,0x7236,0x7411,0x7a63,0x6ec5);
load_palettes(pal12,PALETTES+PALOFFSET*27);
}


void NEOGEO_USER showScreen13(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 13 ******************************************/
uint16_t  pal13[16];
setpal(pal13,0x0,0x1346,0x3b73,0x5bba,0x1852,0x2412,0xfd7,0x7eee,0x7da4,0x3433,0xec9,0x1a54,0x1111,0x5777,0x4123,0x1531);
load_palettes(pal13,PALETTES+PALOFFSET*28);
}


void NEOGEO_USER showScreen14(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 14 ******************************************/
uint16_t  pal14[16];
setpal(pal14,0x0,0x5111,0x1eed,0x6a86,0xaaa,0x1c83,0x1458,0x4432,0x2eb5,0x7732,0x1523,0x4234,0x4666,0x7852,0x5ed9,0x7235);
load_palettes(pal14,PALETTES+PALOFFSET*29);
}


void NEOGEO_USER showScreen15(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 15 ******************************************/
uint16_t  pal15[16];
setpal(pal15,0x0,0x4123,0x6c96,0x422,0x1999,0x1346,0x5ec5,0x642,0x2c83,0x7ddd,0x1975,0x1111,0x555,0x1fda,0x7945,0x1962);
load_palettes(pal15,PALETTES+PALOFFSET*30);
}


void NEOGEO_USER showScreen16(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 16 ******************************************/
uint16_t  pal16[16];
setpal(pal16,0x0,0x223,0x6ddd,0x4ec5,0x7555,0x1642,0x7ec9,0xb86,0x5842,0x5c93,0x1111,0x5999,0x7236,0x3835,0x1962,0x4322);
load_palettes(pal16,PALETTES+PALOFFSET*31);
}


void NEOGEO_USER showScreen17(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 17 ******************************************/
uint16_t  pal17[16];
setpal(pal17,0x0,0x7ec5,0x5321,0x2c96,0x5235,0x1777,0x1eed,0x5962,0x1111,0x642,0x7864,0x946,0x2434,0x7ec9,0x1c93,0x3baa);
load_palettes(pal17,PALETTES+PALOFFSET*32);
}


void NEOGEO_USER showScreen18(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 18 ******************************************/
uint16_t  pal18[16];
setpal(pal18,0x0,0x4999,0x5246,0x3b72,0x7111,0x7ddd,0x4123,0x2422,0x6846,0x7eda,0x7942,0x1da6,0x2eb5,0x7642,0x7444,0x7975);
load_palettes(pal18,PALETTES+PALOFFSET*33);
}


void NEOGEO_USER showScreen19(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 19 ******************************************/
uint16_t  pal19[16];
setpal(pal19,0x0,0x4a85,0x6113,0x422,0x777,0x2842,0x6ec9,0x7aaa,0x2336,0x7945,0x5111,0x4b83,0x2643,0x7333,0x1eed,0x5ec4);
load_palettes(pal19,PALETTES+PALOFFSET*34);
}


void NEOGEO_USER showScreen20(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 20 ******************************************/
uint16_t  pal20[16];
setpal(pal20,0x0,0x5834,0x2eb5,0x1223,0x6314,0x2b73,0x7864,0x5111,0x7ddd,0x2989,0x6236,0x6422,0x6742,0x3fda,0x5ca6,0x1555);
load_palettes(pal20,PALETTES+PALOFFSET*35);
}


void NEOGEO_USER showScreen21(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 21 ******************************************/
uint16_t  pal21[16];
setpal(pal21,0x0,0x6236,0xda5,0x4735,0x5985,0x6ddd,0x7111,0x2a43,0x6422,0x2b73,0x6889,0x5ed9,0x5752,0x7444,0x4ec5,0x4123);
load_palettes(pal21,PALETTES+PALOFFSET*36);
}


void NEOGEO_USER showScreen22(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 22 ******************************************/
uint16_t  pal22[16];
setpal(pal22,0x0,0x3742,0x5111,0x7aaa,0x2d94,0x6a63,0x5235,0xa86,0x1ec7,0x4733,0x7eda,0xeee,0x2422,0x7ec4,0x7333,0x2767);
load_palettes(pal22,PALETTES+PALOFFSET*37);
}


void NEOGEO_USER showScreen23(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 23 ******************************************/
uint16_t  pal23[16];
setpal(pal23,0x0,0x1a62,0x5666,0x6422,0x5ed9,0x5111,0x2236,0x3edd,0x1742,0x7975,0x7333,0x1eb4,0x7213,0x2fd7,0xaaa,0x2c84);
load_palettes(pal23,PALETTES+PALOFFSET*38);
}


void NEOGEO_USER showScreen24(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 24 ******************************************/
uint16_t  pal24[16];
setpal(pal24,0x0,0x2ec9,0x6532,0x4a43,0x5999,0x6112,0x4322,0x3b73,0x7864,0x4235,0x6445,0x6ddd,0x4852,0x846,0x6ec5,0x3d93);
load_palettes(pal24,PALETTES+PALOFFSET*39);
}


void NEOGEO_USER showScreen25(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 25 ******************************************/
uint16_t  pal25[16];
setpal(pal25,0x0,0x4ec5,0x223,0x7999,0x5985,0x3c83,0x1554,0x2632,0x5111,0x2835,0x422,0x4943,0x1962,0x7ddd,0x6ec9,0x2336);
load_palettes(pal25,PALETTES+PALOFFSET*40);
}


void NEOGEO_USER showScreen26(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 26 ******************************************/
uint16_t  pal26[16];
setpal(pal26,0x0,0xddd,0x1346,0x5111,0x2963,0x5ec4,0x7835,0x5325,0x7444,0x7ec9,0x1421,0x642,0x3988,0x4123,0x3c83,0x4953);
load_palettes(pal26,PALETTES+PALOFFSET*41);
}


void NEOGEO_USER showScreen27(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 27 ******************************************/
uint16_t  pal27[16];
setpal(pal27,0x0,0x7852,0x3edc,0x6112,0x2434,0x4ec5,0x347,0x4422,0x4777,0x3baa,0x3a46,0x3c84,0x6532,0x4111,0x4124,0x4426);
load_palettes(pal27,PALETTES+PALOFFSET*42);
}


void NEOGEO_USER showScreen28(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 28 ******************************************/
uint16_t  pal28[16];
setpal(pal28,0x0,0xeee,0x6112,0x963,0x4642,0x5ed9,0x546e,0x7bcf,0x4ec5,0x4846,0x3c83,0x3544,0x5321,0x1998,0x479f,0x7235);
load_palettes(pal28,PALETTES+PALOFFSET*43);
}


void NEOGEO_USER showScreen29(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 29 ******************************************/
uint16_t  pal29[16];
setpal(pal29,0x0,0x1fff,0x6112,0x3baa,0x6b73,0x479f,0x745e,0x5874,0x2656,0x3a46,0x2eb5,0x6225,0x6422,0x3bcf,0x6742,0x5ed9);
load_palettes(pal29,PALETTES+PALOFFSET*44);
}


void NEOGEO_USER showScreen30(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 30 ******************************************/
uint16_t  pal30[16];
setpal(pal30,0x0,0x6236,0x7223,0xa86,0x852,0x1111,0x7421,0x7777,0xeb7,0x2b73,0x7ccc,0x2eb4,0x6edb,0x3643,0x6736,0x1a44);
load_palettes(pal30,PALETTES+PALOFFSET*45);
}


void NEOGEO_USER showScreen31(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 31 ******************************************/
uint16_t  pal31[16];
setpal(pal31,0x0,0x946,0x3baa,0x7742,0x6ec7,0x1111,0x3ea3,0x2336,0x2422,0x3b73,0x4666,0x468f,0x4123,0x3ec9,0x1bcf,0x4eff);
load_palettes(pal31,PALETTES+PALOFFSET*46);
}


void NEOGEO_USER showScreen32(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 32 ******************************************/
uint16_t  pal32[16];
setpal(pal32,0x0,0x6eef,0x6556,0x735,0x532,0x189f,0x3fd6,0x3bbf,0x6da4,0x2b73,0x5235,0x2a9a,0x5ed9,0x1952,0x6864,0x6112);
load_palettes(pal32,PALETTES+PALOFFSET*47);
}


void NEOGEO_USER showScreen33(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 33 ******************************************/
uint16_t  pal33[16];
setpal(pal33,0x0,0x697f,0x2fef,0x6523,0x5326,0x4eda,0x5b83,0x6eb5,0x1852,0x5123,0x7cbf,0x7111,0x1a97,0x2879,0x5346,0x543);
load_palettes(pal33,PALETTES+PALOFFSET*48);
}


void NEOGEO_USER showScreen34(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 34 ******************************************/
uint16_t  pal34[16];
setpal(pal34,0x0,0x97f,0x6dde,0x4a56,0x7642,0x1223,0x2d94,0x988,0x6ec5,0x7444,0x2422,0x7336,0x5111,0x2caf,0x6eda,0x1a73);
load_palettes(pal34,PALETTES+PALOFFSET*49);
}


void NEOGEO_USER showScreen35(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 35 ******************************************/
uint16_t  pal35[16];
setpal(pal35,0x0,0x7963,0x7111,0x3ccf,0x289f,0x4434,0x1432,0x7c83,0x745e,0x944,0x3877,0x5235,0x4ddd,0x4642,0x6eb5,0x3ec9);
load_palettes(pal35,PALETTES+PALOFFSET*50);
}


void NEOGEO_USER showScreen36(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 36 ******************************************/
uint16_t  pal36[16];
setpal(pal36,0x0,0x4846,0x5eda,0x2eb5,0x6113,0x7ddd,0x4888,0x5111,0x479f,0x1752,0x4b96,0x5952,0x7421,0x7444,0x4b83,0x6236);
load_palettes(pal36,PALETTES+PALOFFSET*51);
}


void NEOGEO_USER showScreen37(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 37 ******************************************/
uint16_t  pal37[16];
setpal(pal37,0x0,0x1b83,0x1223,0x5dee,0x6422,0x1eda,0x2eb5,0x1752,0x457f,0x3655,0x2a99,0x59bf,0x1a53,0x1845,0x3336,0x5111);
load_palettes(pal37,PALETTES+PALOFFSET*52);
}


void NEOGEO_USER showScreen38(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 38 ******************************************/
uint16_t  pal38[16];
setpal(pal38,0x0,0x2eb5,0x4a56,0x4444,0x6dde,0x668f,0x7864,0x742,0x3b73,0x4999,0x1524,0x5123,0x422,0x6236,0x2ec9,0x5111);
load_palettes(pal38,PALETTES+PALOFFSET*53);
}


void NEOGEO_USER showScreen39(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 39 ******************************************/
uint16_t  pal39[16];
setpal(pal39,0x0,0x1cdf,0x6743,0x5acf,0x5def,0x4125,0x1fff,0x168f,0x7544,0x2212,0x724f,0x179f,0x746f,0x3ea6,0x6999,0x39af);
load_palettes(pal39,PALETTES+PALOFFSET*54);
}


void NEOGEO_USER showScreen40(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 40 ******************************************/
uint16_t  pal40[16];
setpal(pal40,0x0,0x5652,0x5235,0x7ddd,0x779f,0x2d94,0x7421,0x3988,0x4a73,0x1845,0x4b97,0x5ed9,0x7444,0x3fc5,0x6112,0x1952);
load_palettes(pal40,PALETTES+PALOFFSET*55);
}


void NEOGEO_USER showScreen41(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 41 ******************************************/
uint16_t  pal41[16];
setpal(pal41,0x0,0x7bcf,0x7fff,0x4753,0x59bf,0x779f,0x2212,0x4999,0x5346,0x168f,0x1def,0x3d83,0x3eef,0x3ea7,0x346f,0x7544);
load_palettes(pal41,PALETTES+PALOFFSET*56);
}


void NEOGEO_USER showScreen42(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 42 ******************************************/
uint16_t  pal42[16];
setpal(pal42,0x0,0xca8,0x1852,0x4432,0x7a45,0x7111,0x7524,0x7ddd,0x2eb4,0x3eca,0x4875,0x7444,0x3235,0x2a99,0x5b83,0x4ed8);
load_palettes(pal42,PALETTES+PALOFFSET*57);
}


void NEOGEO_USER showScreen43(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 43 ******************************************/
uint16_t  pal43[16];
setpal(pal43,0x0,0x6112,0xc83,0x6235,0xa8f,0x2964,0x4777,0x5ec5,0x4ddd,0x6523,0x3ec9,0x3edf,0x2532,0x2943,0x6caf,0x1eb6);
load_palettes(pal43,PALETTES+PALOFFSET*58);
}


void NEOGEO_USER showScreen44(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 44 ******************************************/
uint16_t  pal44[16];
setpal(pal44,0x0,0x5852,0x4aaa,0x6112,0x4eda,0x3a75,0x3dbf,0x7945,0x3655,0x4432,0x7a8f,0x6eb5,0x2c84,0x7235,0x5eee,0x5123);
load_palettes(pal44,PALETTES+PALOFFSET*59);
}


void NEOGEO_USER showScreen45(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 45 ******************************************/
uint16_t  pal45[16];
setpal(pal45,0x0,0x1fef,0x5baf,0x2cbf,0x7edf,0x664f,0x1a8f,0x3dbf,0x1cbf,0x3ecf,0x1edf,0x5edf,0x97f,0x4a9f,0x1fff,0x1dcf);
load_palettes(pal45,PALETTES+PALOFFSET*60);
}


void NEOGEO_USER showScreen46(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 46 ******************************************/
uint16_t  pal46[16];
setpal(pal46,0x0,0x3433,0x3fc5,0x2734,0x1346,0x3b98,0x5111,0x7a73,0x1421,0x2edd,0x6778,0x1952,0x1d93,0x6ec9,0x7642,0x1223);
load_palettes(pal46,PALETTES+PALOFFSET*61);
}


void NEOGEO_USER showScreen47(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 47 ******************************************/
uint16_t  pal47[16];
setpal(pal47,0x0,0x7a9f,0x7fff,0x3edf,0x1a8f,0x3dbf,0x3caf,0x1fef,0x385f,0x597f,0x7b9f,0x1fdf,0x7dcf,0x1b9f,0x5fff,0x5baf);
load_palettes(pal47,PALETTES+PALOFFSET*62);
}


void NEOGEO_USER showScreen48(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 48 ******************************************/
uint16_t  pal48[16];
setpal(pal48,0x0,0x5a9f,0x697f,0x1dcf,0x3fef,0x464e,0x7fff,0x253b,0x5eef,0x64c,0x175d,0x3edf,0x3caf,0x385e,0x565c,0x186e);
load_palettes(pal48,PALETTES+PALOFFSET*63);
}


void NEOGEO_USER showScreen49(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 49 ******************************************/
uint16_t  pal49[16];
setpal(pal49,0x0,0x64c,0x5cbf,0x3a8f,0x5dcf,0x97f,0x275d,0x385e,0x5fff,0x7edf,0x5baf,0x253b,0x4a9f,0x3b9f,0x397e,0x564d);
load_palettes(pal49,PALETTES+PALOFFSET*64);
}


void NEOGEO_USER showScreen50(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 50 ******************************************/
uint16_t  pal50[16];
setpal(pal50,0x0,0x1a8f,0x3edf,0x3cae,0x7fff,0x7dcf,0x3fef,0x374f,0x5baf,0x197f,0x5ddf,0x285f,0x5a9f,0x7caf,0x1edf,0x1dcf);
load_palettes(pal50,PALETTES+PALOFFSET*65);
}


void NEOGEO_USER showScreen51(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 51 ******************************************/
uint16_t  pal51[16];
setpal(pal51,0x0,0x97f,0x3ecf,0x464f,0x465d,0x1b9f,0x753b,0xa8f,0x286e,0x7fff,0x675e,0x1caf,0x274e,0x5cbf,0x3dbf,0x5eef);
load_palettes(pal51,PALETTES+PALOFFSET*66);
}


void NEOGEO_USER showScreen52(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 52 ******************************************/
uint16_t  pal52[16];
setpal(pal52,0x0,0x7dcf,0x7b9f,0x775d,0x753b,0x75d,0x543,0x5cbf,0x554b,0x2b83,0x686f,0x1a8f,0x575e,0x3fef,0x97f,0x4a9f);
load_palettes(pal52,PALETTES+PALOFFSET*67);
}


void NEOGEO_USER showScreen53(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 53 ******************************************/
uint16_t  pal53[16];
setpal(pal53,0x0,0x785f,0x7fff,0x3caf,0x1edf,0x7a63,0x3dbf,0x3a7f,0x1baf,0x7b9f,0x7a8f,0x1fef,0x5cbf,0x7dcf,0x4669,0x4323);
load_palettes(pal53,PALETTES+PALOFFSET*68);
}


void NEOGEO_USER showScreen54(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 54 ******************************************/
uint16_t  pal54[16];
setpal(pal54,0x0,0x1222,0x3edf,0x385f,0x3a63,0xec9,0x5542,0x3b9f,0x1dcf,0x7325,0x6878,0x6834,0x7da4,0x3caf,0x697f,0x5fff);
load_palettes(pal54,PALETTES+PALOFFSET*69);
}


void NEOGEO_USER showScreen55(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 55 ******************************************/
uint16_t  pal55[16];
setpal(pal55,0x0,0x111,0x4eda,0x6557,0x7ddd,0x7b97,0x6865,0x225,0x1421,0x5b83,0x7999,0x3724,0x1223,0x6eb5,0x6742,0x6433);
load_palettes(pal55,PALETTES+PALOFFSET*70);
}


void NEOGEO_USER showScreen56(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 56 ******************************************/
uint16_t  pal56[16];
setpal(pal56,0x0,0x5523,0x5ec4,0x6eee,0x5111,0x4986,0xa73,0x7a44,0x2225,0xd94,0x7ec8,0x4999,0x2422,0x4dca,0x7445,0x2743);
load_palettes(pal56,PALETTES+PALOFFSET*71);
}


void NEOGEO_USER showScreen57(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 57 ******************************************/
uint16_t  pal57[16];
setpal(pal57,0x0,0x7432,0xddd,0x5111,0x4d93,0x4123,0x2412,0x166c,0x3742,0x1ec9,0x3b84,0x4833,0x7236,0x7a62,0x2878,0x5ec5);
load_palettes(pal57,PALETTES+PALOFFSET*72);
}


void NEOGEO_USER showScreen58(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 58 ******************************************/
uint16_t  pal58[16];
setpal(pal58,0x0,0x1642,0x5ddc,0x7852,0x7888,0x4246,0x1a85,0x1842,0x945,0x6112,0x2eb5,0x6ec9,0x2225,0x1332,0x1c83,0x422);
load_palettes(pal58,PALETTES+PALOFFSET*73);
}


void NEOGEO_USER showScreen59(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 59 ******************************************/
uint16_t  pal59[16];
setpal(pal59,0x0,0x2865,0x5111,0x4da4,0x944,0x6113,0x6742,0x5444,0x1eec,0x5a73,0x7421,0x3fd7,0x4423,0xaaa,0x4db8,0x5246);
load_palettes(pal59,PALETTES+PALOFFSET*74);
}


void NEOGEO_USER showScreen60(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 60 ******************************************/
uint16_t  pal60[16];
setpal(pal60,0x0,0x6eb5,0x422,0x4ed9,0x1952,0x974,0x3c83,0x111,0x6113,0x7eee,0x4888,0x2632,0x6bbb,0x3336,0x3322,0x4555);
load_palettes(pal60,PALETTES+PALOFFSET*75);
}


void NEOGEO_USER showScreen61(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 61 ******************************************/
uint16_t  pal61[16];
setpal(pal61,0x0,0x1111,0xec5,0x7ddd,0x347,0x2315,0x5555,0x7222,0x2a53,0x6835,0x5123,0x5763,0x7312,0x3521,0x3b83,0x999);
load_palettes(pal61,PALETTES+PALOFFSET*76);
}


void NEOGEO_USER showScreen62(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 62 ******************************************/
uint16_t  pal62[16];
setpal(pal62,0x0,0x7235,0x2d94,0x3211,0x7742,0x6ddd,0x5344,0x2a75,0x6a63,0x4888,0x4422,0x5542,0x2fc6,0x3112,0x2936,0x5ec8);
load_palettes(pal62,PALETTES+PALOFFSET*77);
}


void NEOGEO_USER showScreen63(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 63 ******************************************/
uint16_t  pal63[16];
setpal(pal63,0x0,0x4c94,0x2422,0x1444,0x4ddd,0x3235,0x2732,0x1fd9,0x2754,0xb63,0x6112,0x888,0x5ec5,0x2c97,0x7a45,0x3963);
load_palettes(pal63,PALETTES+PALOFFSET*78);
}


void NEOGEO_USER showScreen64(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 64 ******************************************/
uint16_t  pal64[16];
setpal(pal64,0x0,0x2953,0x7111,0x945,0x6ddd,0x4ec5,0xeb7,0x1642,0x5246,0x6888,0x1b86,0x3444,0x7c83,0x2224,0x5321,0x3fda);
load_palettes(pal64,PALETTES+PALOFFSET*79);
}


void NEOGEO_USER showScreen65(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 65 ******************************************/
uint16_t  pal65[16];
setpal(pal65,0x0,0x223,0x5c93,0x5962,0x7a85,0x3512,0xfd9,0x1346,0x1224,0x4955,0x2422,0x7766,0x111,0x4ec5,0x2dcc,0x4642);
load_palettes(pal65,PALETTES+PALOFFSET*80);
}


void NEOGEO_USER showScreen66(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 66 ******************************************/
uint16_t  pal66[16];
setpal(pal66,0x0,0x7fef,0x475f,0x1caf,0x296f,0x3ecf,0x7dbf,0x1b9f,0x5cbf,0x486f,0x5eef,0x187d,0x7ebf,0x1a8f,0x63f,0x7fff);
load_palettes(pal66,PALETTES+PALOFFSET*81);
}


void NEOGEO_USER showScreen67(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 67 ******************************************/
uint16_t  pal67[16];
setpal(pal67,0x0,0x7ddd,0x2211,0x1da4,0x2c85,0x3426,0x1346,0x3643,0x2879,0x2b98,0x2fda,0x223,0xfd6,0x5531,0x7852,0x7945);
load_palettes(pal67,PALETTES+PALOFFSET*82);
}


void NEOGEO_USER showScreen68(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 68 ******************************************/
uint16_t  pal68[16];
setpal(pal68,0x0,0x2732,0x7ec7,0x2989,0x2edd,0x7962,0x3421,0x2d94,0x5a86,0x1fd5,0x5111,0x7642,0x5346,0x6645,0x7fda,0x5c95);
load_palettes(pal68,PALETTES+PALOFFSET*83);
}


void NEOGEO_USER showScreen69(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 69 ******************************************/
uint16_t  pal69[16];
setpal(pal69,0x0,0x3a87,0x5752,0x7111,0x1fd6,0x3d96,0x2fda,0x3ea5,0x7336,0x5b83,0x7732,0x2532,0x3767,0x5321,0x2a63,0x4ddd);
load_palettes(pal69,PALETTES+PALOFFSET*84);
}


void NEOGEO_USER showScreen70(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 70 ******************************************/
uint16_t  pal70[16];
setpal(pal70,0x0,0x3b73,0x6347,0x3a87,0x3421,0x7fda,0x2d94,0x767,0x4642,0xddd,0x3fc6,0x2ea7,0x2a45,0x7852,0x211,0x1323);
load_palettes(pal70,PALETTES+PALOFFSET*85);
}


void NEOGEO_USER showScreen71(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 71 ******************************************/
uint16_t  pal71[16];
setpal(pal71,0x0,0x963,0x1223,0x3a87,0x6946,0x3c83,0x6ddd,0x7555,0x5321,0x1111,0x1eb7,0x4ec5,0x3fda,0x2d86,0x4642,0x5347);
load_palettes(pal71,PALETTES+PALOFFSET*86);
}


void NEOGEO_USER showScreen72(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 72 ******************************************/
uint16_t  pal72[16];
setpal(pal72,0x0,0x4b83,0x347,0x1852,0x5a55,0x3fda,0x4534,0x3a99,0x6655,0x211,0x1eb6,0x2224,0x6b86,0x5431,0x1eed,0x7eb4);
load_palettes(pal72,PALETTES+PALOFFSET*87);
}


void NEOGEO_USER showScreen73(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 73 ******************************************/
uint16_t  pal73[16];
setpal(pal73,0x0,0x6ddd,0x1223,0x4c94,0x3211,0x3734,0x2c67,0x7ec5,0x1346,0x2b98,0x2422,0x4963,0x4777,0x1c95,0xfda,0x3742);
load_palettes(pal73,PALETTES+PALOFFSET*88);
}


void NEOGEO_USER showScreen74(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 74 ******************************************/
uint16_t  pal74[16];
setpal(pal74,0x0,0x6cbb,0x7ffe,0x321,0x4555,0x7b71,0x3323,0x4111,0x6878,0x7a99,0x4ea2,0x2657,0x2852,0x7531,0x1edc,0x2fed);
load_palettes(pal74,PALETTES+PALOFFSET*89);
}


void NEOGEO_USER showScreen75(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 75 ******************************************/
uint16_t  pal75[16];
setpal(pal75,0x0,0x4752,0x4bbb,0x1edc,0x3323,0x3d91,0x2421,0xa99,0x656,0x1933,0x4111,0x5ffe,0x1877,0x3544,0x1cba,0x4ffd);
load_palettes(pal75,PALETTES+PALOFFSET*90);
}


void NEOGEO_USER showScreen76(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 76 ******************************************/
uint16_t  pal76[16];
setpal(pal76,0x0,0x4fff,0x4445,0x4752,0x7988,0xea2,0x1dcb,0x7531,0x6fd9,0x7111,0x2767,0x4ffe,0xbaa,0x4321,0x3b71,0x5eec);
load_palettes(pal76,PALETTES+PALOFFSET*91);
}


void NEOGEO_USER showScreen77(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 77 ******************************************/
uint16_t  pal77[16];
setpal(pal77,0x0,0x321,0x1851,0xfff,0x5110,0x2a99,0x4ccc,0x3ea0,0x6a72,0x3433,0x2631,0x6421,0x1eed,0x3766,0x3cba,0x3642);
load_palettes(pal77,PALETTES+PALOFFSET*92);
}


void NEOGEO_USER showScreen78(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 78 ******************************************/
uint16_t  pal78[16];
setpal(pal78,0x0,0x4221,0x4bbb,0x1851,0xfd1,0x6631,0x5c91,0x4666,0x6eee,0x7310,0x2988,0x5110,0x6421,0x3433,0x6531,0x1210);
load_palettes(pal78,PALETTES+PALOFFSET*93);
}


void NEOGEO_USER showScreen79(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 79 ******************************************/
uint16_t  pal79[16];
setpal(pal79,0x0,0x5887,0x6631,0x2fee,0x3100,0x3310,0x3fd2,0x531,0x5ccb,0x1210,0x421,0x5aa9,0x3655,0x2c93,0x3433,0x1851);
load_palettes(pal79,PALETTES+PALOFFSET*94);
}


void NEOGEO_USER showScreen80(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 80 ******************************************/
uint16_t  pal80[16];
setpal(pal80,0x0,0x7dcf,0x7a8f,0x5641,0x653f,0x5443,0x3caf,0x2a72,0x1bba,0x7eee,0x4777,0x75f,0x1210,0x1fef,0x421,0x487f);
load_palettes(pal80,PALETTES+PALOFFSET*95);
}


void NEOGEO_USER showScreen81(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 81 ******************************************/
uint16_t  pal81[16];
setpal(pal81,0x0,0x211,0x1fc8,0x5752,0x7ddd,0x3d94,0x2fd4,0x7421,0x1a7a,0x6a44,0x4346,0x3fc6,0x7b98,0x2b74,0x4feb,0x2766);
load_palettes(pal81,PALETTES+PALOFFSET*96);
}


void NEOGEO_USER showScreen82(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 82 ******************************************/
uint16_t  pal82[16];
setpal(pal82,0x0,0x6a73,0x4ddd,0xfd4,0x3c75,0x5321,0x7a68,0x7fda,0x1fd7,0x5da4,0x3ea6,0x4457,0x2632,0x987,0x4111,0x1852);
load_palettes(pal82,PALETTES+PALOFFSET*97);
}


void NEOGEO_USER showScreen83(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 83 ******************************************/
uint16_t  pal83[16];
setpal(pal83,0x0,0x6ddd,0x6742,0x211,0xfd6,0x3e96,0x1667,0x2b85,0x6fda,0x3a62,0x3834,0x3976,0x7421,0x549,0x4da5,0x2d89);
load_palettes(pal83,PALETTES+PALOFFSET*98);
}


void NEOGEO_USER showScreen84(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 84 ******************************************/
uint16_t  pal84[16];
setpal(pal84,0x0,0xa73,0x1eed,0x211,0x3fd4,0x7fda,0x5c75,0x6da5,0x3d95,0x858,0x7421,0x2742,0x2fd8,0x3a75,0x3556,0x4aaa);
load_palettes(pal84,PALETTES+PALOFFSET*99);
}


void NEOGEO_USER showScreen85(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 85 ******************************************/
uint16_t  pal85[16];
setpal(pal85,0x0,0x742,0x7fda,0x5567,0x4ddd,0x1fd5,0x1753,0x5da4,0x1421,0x4a97,0x1c84,0x958,0x5962,0x7a43,0x1fc8,0x5111);
load_palettes(pal85,PALETTES+PALOFFSET*100);
}


void NEOGEO_USER showScreen86(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 86 ******************************************/
uint16_t  pal86[16];
setpal(pal86,0x0,0x5da5,0x6fda,0x2878,0x211,0x4336,0x6ddd,0x7852,0x5642,0x6a73,0x1fd4,0x3fc6,0x3a57,0x3c86,0x5da3,0x1421);
load_palettes(pal86,PALETTES+PALOFFSET*101);
}


void NEOGEO_USER showScreen87(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 87 ******************************************/
uint16_t  pal87[16];
setpal(pal87,0x0,0x6963,0x5321,0x2edd,0x5752,0x2fc4,0xeb5,0x2c84,0x5b75,0x7fda,0x2747,0x4111,0x4887,0x4532,0x1eb7,0xfd8);
load_palettes(pal87,PALETTES+PALOFFSET*102);
}


void NEOGEO_USER showScreen88(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 88 ******************************************/
uint16_t  pal88[16];
setpal(pal88,0x0,0x3eef,0x346f,0x5acf,0x58af,0x7cdf,0x168f,0x579f,0x1def,0x1cdf,0x59bf,0x3abf,0x3bcf,0x1eff,0x39af,0x5eff);
load_palettes(pal88,PALETTES+PALOFFSET*103);
}


void NEOGEO_USER showScreen89(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 89 ******************************************/
uint16_t  pal89[16];
setpal(pal89,0x0,0x3abf,0x746f,0x335f,0x1eff,0x558f,0x768f,0x3ddf,0x5acf,0x1cdf,0x59bf,0x779f,0x18af,0x3bcf,0x39af,0x414f);
load_palettes(pal89,PALETTES+PALOFFSET*104);
}


void NEOGEO_USER showScreen90(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 90 ******************************************/
uint16_t  pal90[16];
setpal(pal90,0x0,0x3ecf,0x797f,0x1b9f,0x7a8f,0x5cbf,0xbaf,0x5eef,0x1caf,0x197f,0x5dcf,0x674f,0x1a8f,0x7caf,0x3dbf,0x586f);
load_palettes(pal90,PALETTES+PALOFFSET*105);
}


void NEOGEO_USER showScreen91(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 91 ******************************************/
uint16_t  pal91[16];
setpal(pal91,0x0,0x7edf,0x5baf,0x797f,0x1cbf,0x1b9f,0x1a8f,0x3ecf,0x1edf,0x785f,0x1fef,0x197f,0x7b9f,0x3caf,0x5caf,0x763f);
load_palettes(pal91,PALETTES+PALOFFSET*106);
}


void NEOGEO_USER showScreen92(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 92 ******************************************/
uint16_t  pal92[16];
setpal(pal92,0x0,0x1caf,0x1b9f,0x675f,0x1fef,0x174f,0x3ecf,0x1a8f,0x753d,0x1dbf,0x687e,0x686f,0x652f,0x553e,0x396f,0x575f);
load_palettes(pal92,PALETTES+PALOFFSET*107);
}


void NEOGEO_USER showScreen93(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 93 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal93[16];
setpal(pal93,0x0,0x0,0x5ff4,0x2fff,0x68be,0x2f10,0x5a84,0x5610,0x513b,0x7fb2,0x2fea,0x2f61,0x648e,0x7778,0x1124,0x1aff);
uint16_t spriteMapS93_1[16] = {0x5c00,0x5c10,0x5c20,0x5c30,0x5c40,0x5c50,0x5c60,0x5c70,0x5c80,0x5c90,0x5ca0,0x5cb0,0x5cc0,0x5cd0,0x5ce0,0x5cf0};
uint16_t spriteMapS93_2[16] = {0x5c01,0x5c11,0x5c21,0x5c31,0x5c41,0x5c51,0x5c61,0x5c71,0x5c81,0x5c91,0x5ca1,0x5cb1,0x5cc1,0x5cd1,0x5ce1,0x5cf1};
uint16_t spriteMapS93_3[16] = {0x5c02,0x5c12,0x5c22,0x5c32,0x5c42,0x5c52,0x5c62,0x5c72,0x5c82,0x5c92,0x5ca2,0x5cb2,0x5cc2,0x5cd2,0x5ce2,0x5cf2};
uint16_t spriteMapS93_4[16] = {0x5c03,0x5c13,0x5c23,0x5c33,0x5c43,0x5c53,0x5c63,0x5c73,0x5c83,0x5c93,0x5ca3,0x5cb3,0x5cc3,0x5cd3,0x5ce3,0x5cf3};
uint16_t spriteMapS93_5[16] = {0x5c04,0x5c14,0x5c24,0x5c34,0x5c44,0x5c54,0x5c64,0x5c74,0x5c84,0x5c94,0x5ca4,0x5cb4,0x5cc4,0x5cd4,0x5ce4,0x5cf4};
uint16_t spriteMapS93_6[16] = {0x5c05,0x5c15,0x5c25,0x5c35,0x5c45,0x5c55,0x5c65,0x5c75,0x5c85,0x5c95,0x5ca5,0x5cb5,0x5cc5,0x5cd5,0x5ce5,0x5cf5};
uint16_t spriteMapS93_7[16] = {0x5c06,0x5c16,0x5c26,0x5c36,0x5c46,0x5c56,0x5c66,0x5c76,0x5c86,0x5c96,0x5ca6,0x5cb6,0x5cc6,0x5cd6,0x5ce6,0x5cf6};
uint16_t spriteMapS93_8[16] = {0x5c07,0x5c17,0x5c27,0x5c37,0x5c47,0x5c57,0x5c67,0x5c77,0x5c87,0x5c97,0x5ca7,0x5cb7,0x5cc7,0x5cd7,0x5ce7,0x5cf7};
uint16_t spriteMapS93_9[16] = {0x5c08,0x5c18,0x5c28,0x5c38,0x5c48,0x5c58,0x5c68,0x5c78,0x5c88,0x5c98,0x5ca8,0x5cb8,0x5cc8,0x5cd8,0x5ce8,0x5cf8};
uint16_t spriteMapS93_10[16] = {0x5c09,0x5c19,0x5c29,0x5c39,0x5c49,0x5c59,0x5c69,0x5c79,0x5c89,0x5c99,0x5ca9,0x5cb9,0x5cc9,0x5cd9,0x5ce9,0x5cf9};
uint16_t spriteMapS93_11[16] = {0x5c0a,0x5c1a,0x5c2a,0x5c3a,0x5c4a,0x5c5a,0x5c6a,0x5c7a,0x5c8a,0x5c9a,0x5caa,0x5cba,0x5cca,0x5cda,0x5cea,0x5cfa};
uint16_t spriteMapS93_12[16] = {0x5c0b,0x5c1b,0x5c2b,0x5c3b,0x5c4b,0x5c5b,0x5c6b,0x5c7b,0x5c8b,0x5c9b,0x5cab,0x5cbb,0x5ccb,0x5cdb,0x5ceb,0x5cfb};
uint16_t spriteMapS93_13[16] = {0x5c0c,0x5c1c,0x5c2c,0x5c3c,0x5c4c,0x5c5c,0x5c6c,0x5c7c,0x5c8c,0x5c9c,0x5cac,0x5cbc,0x5ccc,0x5cdc,0x5cec,0x5cfc};
uint16_t spriteMapS93_14[16] = {0x5c0d,0x5c1d,0x5c2d,0x5c3d,0x5c4d,0x5c5d,0x5c6d,0x5c7d,0x5c8d,0x5c9d,0x5cad,0x5cbd,0x5ccd,0x5cdd,0x5ced,0x5cfd};
uint16_t spriteMapS93_15[16] = {0x5c0e,0x5c1e,0x5c2e,0x5c3e,0x5c4e,0x5c5e,0x5c6e,0x5c7e,0x5c8e,0x5c9e,0x5cae,0x5cbe,0x5cce,0x5cde,0x5cee,0x5cfe};
uint16_t spriteMapS93_16[16] = {0x5c0f,0x5c1f,0x5c2f,0x5c3f,0x5c4f,0x5c5f,0x5c6f,0x5c7f,0x5c8f,0x5c9f,0x5caf,0x5cbf,0x5ccf,0x5cdf,0x5cef,0x5cff};
load_palettes(pal93,PALETTES+PALOFFSET*108);
uint16_t SCB1_2common = setSCB1_2(108,0,0,0,0,0);
uint16_t spal93_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal93_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal93_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal93_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal93_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal93_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal93_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal93_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal93_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal93_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal93_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal93_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal93_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal93_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal93_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal93_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS93_1,spal93_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS93_2,spal93_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS93_3,spal93_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS93_4,spal93_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS93_5,spal93_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS93_6,spal93_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS93_7,spal93_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS93_8,spal93_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS93_9,spal93_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS93_10,spal93_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS93_11,spal93_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS93_12,spal93_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS93_13,spal93_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS93_14,spal93_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS93_15,spal93_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS93_16,spal93_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen94(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 94 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal94[16];
setpal(pal94,0x0,0x523a,0x6f10,0x1567,0x4975,0x0,0x16ae,0x3fff,0x3910,0x5ff7,0xfeb,0x69df,0x5410,0x1f81,0x6fb2,0x4ff3);
uint16_t spriteMapS94_1[16] = {0x5d00,0x5d10,0x5d20,0x5d30,0x5d40,0x5d50,0x5d60,0x5d70,0x5d80,0x5d90,0x5da0,0x5db0,0x5dc0,0x5dd0,0x5de0,0x5df0};
uint16_t spriteMapS94_2[16] = {0x5d01,0x5d11,0x5d21,0x5d31,0x5d41,0x5d51,0x5d61,0x5d71,0x5d81,0x5d91,0x5da1,0x5db1,0x5dc1,0x5dd1,0x5de1,0x5df1};
uint16_t spriteMapS94_3[16] = {0x5d02,0x5d12,0x5d22,0x5d32,0x5d42,0x5d52,0x5d62,0x5d72,0x5d82,0x5d92,0x5da2,0x5db2,0x5dc2,0x5dd2,0x5de2,0x5df2};
uint16_t spriteMapS94_4[16] = {0x5d03,0x5d13,0x5d23,0x5d33,0x5d43,0x5d53,0x5d63,0x5d73,0x5d83,0x5d93,0x5da3,0x5db3,0x5dc3,0x5dd3,0x5de3,0x5df3};
uint16_t spriteMapS94_5[16] = {0x5d04,0x5d14,0x5d24,0x5d34,0x5d44,0x5d54,0x5d64,0x5d74,0x5d84,0x5d94,0x5da4,0x5db4,0x5dc4,0x5dd4,0x5de4,0x5df4};
uint16_t spriteMapS94_6[16] = {0x5d05,0x5d15,0x5d25,0x5d35,0x5d45,0x5d55,0x5d65,0x5d75,0x5d85,0x5d95,0x5da5,0x5db5,0x5dc5,0x5dd5,0x5de5,0x5df5};
uint16_t spriteMapS94_7[16] = {0x5d06,0x5d16,0x5d26,0x5d36,0x5d46,0x5d56,0x5d66,0x5d76,0x5d86,0x5d96,0x5da6,0x5db6,0x5dc6,0x5dd6,0x5de6,0x5df6};
uint16_t spriteMapS94_8[16] = {0x5d07,0x5d17,0x5d27,0x5d37,0x5d47,0x5d57,0x5d67,0x5d77,0x5d87,0x5d97,0x5da7,0x5db7,0x5dc7,0x5dd7,0x5de7,0x5df7};
uint16_t spriteMapS94_9[16] = {0x5d08,0x5d18,0x5d28,0x5d38,0x5d48,0x5d58,0x5d68,0x5d78,0x5d88,0x5d98,0x5da8,0x5db8,0x5dc8,0x5dd8,0x5de8,0x5df8};
uint16_t spriteMapS94_10[16] = {0x5d09,0x5d19,0x5d29,0x5d39,0x5d49,0x5d59,0x5d69,0x5d79,0x5d89,0x5d99,0x5da9,0x5db9,0x5dc9,0x5dd9,0x5de9,0x5df9};
uint16_t spriteMapS94_11[16] = {0x5d0a,0x5d1a,0x5d2a,0x5d3a,0x5d4a,0x5d5a,0x5d6a,0x5d7a,0x5d8a,0x5d9a,0x5daa,0x5dba,0x5dca,0x5dda,0x5dea,0x5dfa};
uint16_t spriteMapS94_12[16] = {0x5d0b,0x5d1b,0x5d2b,0x5d3b,0x5d4b,0x5d5b,0x5d6b,0x5d7b,0x5d8b,0x5d9b,0x5dab,0x5dbb,0x5dcb,0x5ddb,0x5deb,0x5dfb};
uint16_t spriteMapS94_13[16] = {0x5d0c,0x5d1c,0x5d2c,0x5d3c,0x5d4c,0x5d5c,0x5d6c,0x5d7c,0x5d8c,0x5d9c,0x5dac,0x5dbc,0x5dcc,0x5ddc,0x5dec,0x5dfc};
uint16_t spriteMapS94_14[16] = {0x5d0d,0x5d1d,0x5d2d,0x5d3d,0x5d4d,0x5d5d,0x5d6d,0x5d7d,0x5d8d,0x5d9d,0x5dad,0x5dbd,0x5dcd,0x5ddd,0x5ded,0x5dfd};
uint16_t spriteMapS94_15[16] = {0x5d0e,0x5d1e,0x5d2e,0x5d3e,0x5d4e,0x5d5e,0x5d6e,0x5d7e,0x5d8e,0x5d9e,0x5dae,0x5dbe,0x5dce,0x5dde,0x5dee,0x5dfe};
uint16_t spriteMapS94_16[16] = {0x5d0f,0x5d1f,0x5d2f,0x5d3f,0x5d4f,0x5d5f,0x5d6f,0x5d7f,0x5d8f,0x5d9f,0x5daf,0x5dbf,0x5dcf,0x5ddf,0x5def,0x5dff};
load_palettes(pal94,PALETTES+PALOFFSET*109);
uint16_t SCB1_2common = setSCB1_2(109,0,0,0,0,0);
uint16_t spal94_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal94_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS94_1,spal94_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS94_2,spal94_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS94_3,spal94_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS94_4,spal94_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS94_5,spal94_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS94_6,spal94_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS94_7,spal94_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS94_8,spal94_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS94_9,spal94_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS94_10,spal94_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS94_11,spal94_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS94_12,spal94_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS94_13,spal94_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS94_14,spal94_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS94_15,spal94_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS94_16,spal94_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen95(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 95 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal95[16];
setpal(pal95,0x0,0x6e71,0x648e,0x0,0x2fff,0x2fd9,0x6fe4,0x5920,0x5aff,0x223c,0x2115,0x5410,0x4777,0x4fb2,0xf10,0x8be);
uint16_t spriteMapS95_1[16] = {0x5e00,0x5e10,0x5e20,0x5e30,0x5e40,0x5e50,0x5e60,0x5e70,0x5e80,0x5e90,0x5ea0,0x5eb0,0x5ec0,0x5ed0,0x5ee0,0x5ef0};
uint16_t spriteMapS95_2[16] = {0x5e01,0x5e11,0x5e21,0x5e31,0x5e41,0x5e51,0x5e61,0x5e71,0x5e81,0x5e91,0x5ea1,0x5eb1,0x5ec1,0x5ed1,0x5ee1,0x5ef1};
uint16_t spriteMapS95_3[16] = {0x5e02,0x5e12,0x5e22,0x5e32,0x5e42,0x5e52,0x5e62,0x5e72,0x5e82,0x5e92,0x5ea2,0x5eb2,0x5ec2,0x5ed2,0x5ee2,0x5ef2};
uint16_t spriteMapS95_4[16] = {0x5e03,0x5e13,0x5e23,0x5e33,0x5e43,0x5e53,0x5e63,0x5e73,0x5e83,0x5e93,0x5ea3,0x5eb3,0x5ec3,0x5ed3,0x5ee3,0x5ef3};
uint16_t spriteMapS95_5[16] = {0x5e04,0x5e14,0x5e24,0x5e34,0x5e44,0x5e54,0x5e64,0x5e74,0x5e84,0x5e94,0x5ea4,0x5eb4,0x5ec4,0x5ed4,0x5ee4,0x5ef4};
uint16_t spriteMapS95_6[16] = {0x5e05,0x5e15,0x5e25,0x5e35,0x5e45,0x5e55,0x5e65,0x5e75,0x5e85,0x5e95,0x5ea5,0x5eb5,0x5ec5,0x5ed5,0x5ee5,0x5ef5};
uint16_t spriteMapS95_7[16] = {0x5e06,0x5e16,0x5e26,0x5e36,0x5e46,0x5e56,0x5e66,0x5e76,0x5e86,0x5e96,0x5ea6,0x5eb6,0x5ec6,0x5ed6,0x5ee6,0x5ef6};
uint16_t spriteMapS95_8[16] = {0x5e07,0x5e17,0x5e27,0x5e37,0x5e47,0x5e57,0x5e67,0x5e77,0x5e87,0x5e97,0x5ea7,0x5eb7,0x5ec7,0x5ed7,0x5ee7,0x5ef7};
uint16_t spriteMapS95_9[16] = {0x5e08,0x5e18,0x5e28,0x5e38,0x5e48,0x5e58,0x5e68,0x5e78,0x5e88,0x5e98,0x5ea8,0x5eb8,0x5ec8,0x5ed8,0x5ee8,0x5ef8};
uint16_t spriteMapS95_10[16] = {0x5e09,0x5e19,0x5e29,0x5e39,0x5e49,0x5e59,0x5e69,0x5e79,0x5e89,0x5e99,0x5ea9,0x5eb9,0x5ec9,0x5ed9,0x5ee9,0x5ef9};
uint16_t spriteMapS95_11[16] = {0x5e0a,0x5e1a,0x5e2a,0x5e3a,0x5e4a,0x5e5a,0x5e6a,0x5e7a,0x5e8a,0x5e9a,0x5eaa,0x5eba,0x5eca,0x5eda,0x5eea,0x5efa};
uint16_t spriteMapS95_12[16] = {0x5e0b,0x5e1b,0x5e2b,0x5e3b,0x5e4b,0x5e5b,0x5e6b,0x5e7b,0x5e8b,0x5e9b,0x5eab,0x5ebb,0x5ecb,0x5edb,0x5eeb,0x5efb};
uint16_t spriteMapS95_13[16] = {0x5e0c,0x5e1c,0x5e2c,0x5e3c,0x5e4c,0x5e5c,0x5e6c,0x5e7c,0x5e8c,0x5e9c,0x5eac,0x5ebc,0x5ecc,0x5edc,0x5eec,0x5efc};
uint16_t spriteMapS95_14[16] = {0x5e0d,0x5e1d,0x5e2d,0x5e3d,0x5e4d,0x5e5d,0x5e6d,0x5e7d,0x5e8d,0x5e9d,0x5ead,0x5ebd,0x5ecd,0x5edd,0x5eed,0x5efd};
uint16_t spriteMapS95_15[16] = {0x5e0e,0x5e1e,0x5e2e,0x5e3e,0x5e4e,0x5e5e,0x5e6e,0x5e7e,0x5e8e,0x5e9e,0x5eae,0x5ebe,0x5ece,0x5ede,0x5eee,0x5efe};
uint16_t spriteMapS95_16[16] = {0x5e0f,0x5e1f,0x5e2f,0x5e3f,0x5e4f,0x5e5f,0x5e6f,0x5e7f,0x5e8f,0x5e9f,0x5eaf,0x5ebf,0x5ecf,0x5edf,0x5eef,0x5eff};
load_palettes(pal95,PALETTES+PALOFFSET*110);
uint16_t SCB1_2common = setSCB1_2(110,0,0,0,0,0);
uint16_t spal95_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal95_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS95_1,spal95_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS95_2,spal95_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS95_3,spal95_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS95_4,spal95_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS95_5,spal95_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS95_6,spal95_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS95_7,spal95_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS95_8,spal95_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS95_9,spal95_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS95_10,spal95_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS95_11,spal95_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS95_12,spal95_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS95_13,spal95_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS95_14,spal95_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS95_15,spal95_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS95_16,spal95_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen96(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 96 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal96[16];
setpal(pal96,0x0,0x0,0x6f00,0x4ff9,0xfa1,0x68df,0x3720,0x648d,0x4ff3,0x413a,0xfff,0x4e71,0x6a10,0x6300,0x6fc1,0x667);
uint16_t spriteMapS96_1[16] = {0x5f00,0x5f10,0x5f20,0x5f30,0x5f40,0x5f50,0x5f60,0x5f70,0x5f80,0x5f90,0x5fa0,0x5fb0,0x5fc0,0x5fd0,0x5fe0,0x5ff0};
uint16_t spriteMapS96_2[16] = {0x5f01,0x5f11,0x5f21,0x5f31,0x5f41,0x5f51,0x5f61,0x5f71,0x5f81,0x5f91,0x5fa1,0x5fb1,0x5fc1,0x5fd1,0x5fe1,0x5ff1};
uint16_t spriteMapS96_3[16] = {0x5f02,0x5f12,0x5f22,0x5f32,0x5f42,0x5f52,0x5f62,0x5f72,0x5f82,0x5f92,0x5fa2,0x5fb2,0x5fc2,0x5fd2,0x5fe2,0x5ff2};
uint16_t spriteMapS96_4[16] = {0x5f03,0x5f13,0x5f23,0x5f33,0x5f43,0x5f53,0x5f63,0x5f73,0x5f83,0x5f93,0x5fa3,0x5fb3,0x5fc3,0x5fd3,0x5fe3,0x5ff3};
uint16_t spriteMapS96_5[16] = {0x5f04,0x5f14,0x5f24,0x5f34,0x5f44,0x5f54,0x5f64,0x5f74,0x5f84,0x5f94,0x5fa4,0x5fb4,0x5fc4,0x5fd4,0x5fe4,0x5ff4};
uint16_t spriteMapS96_6[16] = {0x5f05,0x5f15,0x5f25,0x5f35,0x5f45,0x5f55,0x5f65,0x5f75,0x5f85,0x5f95,0x5fa5,0x5fb5,0x5fc5,0x5fd5,0x5fe5,0x5ff5};
uint16_t spriteMapS96_7[16] = {0x5f06,0x5f16,0x5f26,0x5f36,0x5f46,0x5f56,0x5f66,0x5f76,0x5f86,0x5f96,0x5fa6,0x5fb6,0x5fc6,0x5fd6,0x5fe6,0x5ff6};
uint16_t spriteMapS96_8[16] = {0x5f07,0x5f17,0x5f27,0x5f37,0x5f47,0x5f57,0x5f67,0x5f77,0x5f87,0x5f97,0x5fa7,0x5fb7,0x5fc7,0x5fd7,0x5fe7,0x5ff7};
uint16_t spriteMapS96_9[16] = {0x5f08,0x5f18,0x5f28,0x5f38,0x5f48,0x5f58,0x5f68,0x5f78,0x5f88,0x5f98,0x5fa8,0x5fb8,0x5fc8,0x5fd8,0x5fe8,0x5ff8};
uint16_t spriteMapS96_10[16] = {0x5f09,0x5f19,0x5f29,0x5f39,0x5f49,0x5f59,0x5f69,0x5f79,0x5f89,0x5f99,0x5fa9,0x5fb9,0x5fc9,0x5fd9,0x5fe9,0x5ff9};
uint16_t spriteMapS96_11[16] = {0x5f0a,0x5f1a,0x5f2a,0x5f3a,0x5f4a,0x5f5a,0x5f6a,0x5f7a,0x5f8a,0x5f9a,0x5faa,0x5fba,0x5fca,0x5fda,0x5fea,0x5ffa};
uint16_t spriteMapS96_12[16] = {0x5f0b,0x5f1b,0x5f2b,0x5f3b,0x5f4b,0x5f5b,0x5f6b,0x5f7b,0x5f8b,0x5f9b,0x5fab,0x5fbb,0x5fcb,0x5fdb,0x5feb,0x5ffb};
uint16_t spriteMapS96_13[16] = {0x5f0c,0x5f1c,0x5f2c,0x5f3c,0x5f4c,0x5f5c,0x5f6c,0x5f7c,0x5f8c,0x5f9c,0x5fac,0x5fbc,0x5fcc,0x5fdc,0x5fec,0x5ffc};
uint16_t spriteMapS96_14[16] = {0x5f0d,0x5f1d,0x5f2d,0x5f3d,0x5f4d,0x5f5d,0x5f6d,0x5f7d,0x5f8d,0x5f9d,0x5fad,0x5fbd,0x5fcd,0x5fdd,0x5fed,0x5ffd};
uint16_t spriteMapS96_15[16] = {0x5f0e,0x5f1e,0x5f2e,0x5f3e,0x5f4e,0x5f5e,0x5f6e,0x5f7e,0x5f8e,0x5f9e,0x5fae,0x5fbe,0x5fce,0x5fde,0x5fee,0x5ffe};
uint16_t spriteMapS96_16[16] = {0x5f0f,0x5f1f,0x5f2f,0x5f3f,0x5f4f,0x5f5f,0x5f6f,0x5f7f,0x5f8f,0x5f9f,0x5faf,0x5fbf,0x5fcf,0x5fdf,0x5fef,0x5fff};
load_palettes(pal96,PALETTES+PALOFFSET*111);
uint16_t SCB1_2common = setSCB1_2(111,0,0,0,0,0);
uint16_t spal96_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal96_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS96_1,spal96_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS96_2,spal96_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS96_3,spal96_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS96_4,spal96_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS96_5,spal96_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS96_6,spal96_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS96_7,spal96_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS96_8,spal96_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS96_9,spal96_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS96_10,spal96_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS96_11,spal96_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS96_12,spal96_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS96_13,spal96_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS96_14,spal96_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS96_15,spal96_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS96_16,spal96_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen97(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 97 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal97[16];
setpal(pal97,0x0,0x0,0x1aff,0x4fb1,0x3f70,0x128,0x6f00,0x27be,0x248e,0x2fff,0x34e,0x5920,0x6fe5,0x6125,0x5410,0x3a85);
uint16_t spriteMapS97_1[16] = {0x6000,0x6010,0x6020,0x6030,0x6040,0x6050,0x6060,0x6070,0x6080,0x6090,0x60a0,0x60b0,0x60c0,0x60d0,0x60e0,0x60f0};
uint16_t spriteMapS97_2[16] = {0x6001,0x6011,0x6021,0x6031,0x6041,0x6051,0x6061,0x6071,0x6081,0x6091,0x60a1,0x60b1,0x60c1,0x60d1,0x60e1,0x60f1};
uint16_t spriteMapS97_3[16] = {0x6002,0x6012,0x6022,0x6032,0x6042,0x6052,0x6062,0x6072,0x6082,0x6092,0x60a2,0x60b2,0x60c2,0x60d2,0x60e2,0x60f2};
uint16_t spriteMapS97_4[16] = {0x6003,0x6013,0x6023,0x6033,0x6043,0x6053,0x6063,0x6073,0x6083,0x6093,0x60a3,0x60b3,0x60c3,0x60d3,0x60e3,0x60f3};
uint16_t spriteMapS97_5[16] = {0x6004,0x6014,0x6024,0x6034,0x6044,0x6054,0x6064,0x6074,0x6084,0x6094,0x60a4,0x60b4,0x60c4,0x60d4,0x60e4,0x60f4};
uint16_t spriteMapS97_6[16] = {0x6005,0x6015,0x6025,0x6035,0x6045,0x6055,0x6065,0x6075,0x6085,0x6095,0x60a5,0x60b5,0x60c5,0x60d5,0x60e5,0x60f5};
uint16_t spriteMapS97_7[16] = {0x6006,0x6016,0x6026,0x6036,0x6046,0x6056,0x6066,0x6076,0x6086,0x6096,0x60a6,0x60b6,0x60c6,0x60d6,0x60e6,0x60f6};
uint16_t spriteMapS97_8[16] = {0x6007,0x6017,0x6027,0x6037,0x6047,0x6057,0x6067,0x6077,0x6087,0x6097,0x60a7,0x60b7,0x60c7,0x60d7,0x60e7,0x60f7};
uint16_t spriteMapS97_9[16] = {0x6008,0x6018,0x6028,0x6038,0x6048,0x6058,0x6068,0x6078,0x6088,0x6098,0x60a8,0x60b8,0x60c8,0x60d8,0x60e8,0x60f8};
uint16_t spriteMapS97_10[16] = {0x6009,0x6019,0x6029,0x6039,0x6049,0x6059,0x6069,0x6079,0x6089,0x6099,0x60a9,0x60b9,0x60c9,0x60d9,0x60e9,0x60f9};
uint16_t spriteMapS97_11[16] = {0x600a,0x601a,0x602a,0x603a,0x604a,0x605a,0x606a,0x607a,0x608a,0x609a,0x60aa,0x60ba,0x60ca,0x60da,0x60ea,0x60fa};
uint16_t spriteMapS97_12[16] = {0x600b,0x601b,0x602b,0x603b,0x604b,0x605b,0x606b,0x607b,0x608b,0x609b,0x60ab,0x60bb,0x60cb,0x60db,0x60eb,0x60fb};
uint16_t spriteMapS97_13[16] = {0x600c,0x601c,0x602c,0x603c,0x604c,0x605c,0x606c,0x607c,0x608c,0x609c,0x60ac,0x60bc,0x60cc,0x60dc,0x60ec,0x60fc};
uint16_t spriteMapS97_14[16] = {0x600d,0x601d,0x602d,0x603d,0x604d,0x605d,0x606d,0x607d,0x608d,0x609d,0x60ad,0x60bd,0x60cd,0x60dd,0x60ed,0x60fd};
uint16_t spriteMapS97_15[16] = {0x600e,0x601e,0x602e,0x603e,0x604e,0x605e,0x606e,0x607e,0x608e,0x609e,0x60ae,0x60be,0x60ce,0x60de,0x60ee,0x60fe};
uint16_t spriteMapS97_16[16] = {0x600f,0x601f,0x602f,0x603f,0x604f,0x605f,0x606f,0x607f,0x608f,0x609f,0x60af,0x60bf,0x60cf,0x60df,0x60ef,0x60ff};
load_palettes(pal97,PALETTES+PALOFFSET*112);
uint16_t SCB1_2common = setSCB1_2(112,0,0,0,0,0);
uint16_t spal97_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal97_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS97_1,spal97_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS97_2,spal97_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS97_3,spal97_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS97_4,spal97_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS97_5,spal97_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS97_6,spal97_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS97_7,spal97_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS97_8,spal97_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS97_9,spal97_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS97_10,spal97_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS97_11,spal97_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS97_12,spal97_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS97_13,spal97_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS97_14,spal97_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS97_15,spal97_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS97_16,spal97_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen98(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 98 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal98[16];
setpal(pal98,0x0,0x0,0x537e,0x3f81,0x2fff,0x4ff5,0x986,0x123e,0x6fb2,0xaef,0x4f10,0x5410,0x7124,0x2118,0x7ae,0x7920);
uint16_t spriteMapS98_1[16] = {0x6100,0x6110,0x6120,0x6130,0x6140,0x6150,0x6160,0x6170,0x6180,0x6190,0x61a0,0x61b0,0x61c0,0x61d0,0x61e0,0x61f0};
uint16_t spriteMapS98_2[16] = {0x6101,0x6111,0x6121,0x6131,0x6141,0x6151,0x6161,0x6171,0x6181,0x6191,0x61a1,0x61b1,0x61c1,0x61d1,0x61e1,0x61f1};
uint16_t spriteMapS98_3[16] = {0x6102,0x6112,0x6122,0x6132,0x6142,0x6152,0x6162,0x6172,0x6182,0x6192,0x61a2,0x61b2,0x61c2,0x61d2,0x61e2,0x61f2};
uint16_t spriteMapS98_4[16] = {0x6103,0x6113,0x6123,0x6133,0x6143,0x6153,0x6163,0x6173,0x6183,0x6193,0x61a3,0x61b3,0x61c3,0x61d3,0x61e3,0x61f3};
uint16_t spriteMapS98_5[16] = {0x6104,0x6114,0x6124,0x6134,0x6144,0x6154,0x6164,0x6174,0x6184,0x6194,0x61a4,0x61b4,0x61c4,0x61d4,0x61e4,0x61f4};
uint16_t spriteMapS98_6[16] = {0x6105,0x6115,0x6125,0x6135,0x6145,0x6155,0x6165,0x6175,0x6185,0x6195,0x61a5,0x61b5,0x61c5,0x61d5,0x61e5,0x61f5};
uint16_t spriteMapS98_7[16] = {0x6106,0x6116,0x6126,0x6136,0x6146,0x6156,0x6166,0x6176,0x6186,0x6196,0x61a6,0x61b6,0x61c6,0x61d6,0x61e6,0x61f6};
uint16_t spriteMapS98_8[16] = {0x6107,0x6117,0x6127,0x6137,0x6147,0x6157,0x6167,0x6177,0x6187,0x6197,0x61a7,0x61b7,0x61c7,0x61d7,0x61e7,0x61f7};
uint16_t spriteMapS98_9[16] = {0x6108,0x6118,0x6128,0x6138,0x6148,0x6158,0x6168,0x6178,0x6188,0x6198,0x61a8,0x61b8,0x61c8,0x61d8,0x61e8,0x61f8};
uint16_t spriteMapS98_10[16] = {0x6109,0x6119,0x6129,0x6139,0x6149,0x6159,0x6169,0x6179,0x6189,0x6199,0x61a9,0x61b9,0x61c9,0x61d9,0x61e9,0x61f9};
uint16_t spriteMapS98_11[16] = {0x610a,0x611a,0x612a,0x613a,0x614a,0x615a,0x616a,0x617a,0x618a,0x619a,0x61aa,0x61ba,0x61ca,0x61da,0x61ea,0x61fa};
uint16_t spriteMapS98_12[16] = {0x610b,0x611b,0x612b,0x613b,0x614b,0x615b,0x616b,0x617b,0x618b,0x619b,0x61ab,0x61bb,0x61cb,0x61db,0x61eb,0x61fb};
uint16_t spriteMapS98_13[16] = {0x610c,0x611c,0x612c,0x613c,0x614c,0x615c,0x616c,0x617c,0x618c,0x619c,0x61ac,0x61bc,0x61cc,0x61dc,0x61ec,0x61fc};
uint16_t spriteMapS98_14[16] = {0x610d,0x611d,0x612d,0x613d,0x614d,0x615d,0x616d,0x617d,0x618d,0x619d,0x61ad,0x61bd,0x61cd,0x61dd,0x61ed,0x61fd};
uint16_t spriteMapS98_15[16] = {0x610e,0x611e,0x612e,0x613e,0x614e,0x615e,0x616e,0x617e,0x618e,0x619e,0x61ae,0x61be,0x61ce,0x61de,0x61ee,0x61fe};
uint16_t spriteMapS98_16[16] = {0x610f,0x611f,0x612f,0x613f,0x614f,0x615f,0x616f,0x617f,0x618f,0x619f,0x61af,0x61bf,0x61cf,0x61df,0x61ef,0x61ff};
load_palettes(pal98,PALETTES+PALOFFSET*113);
uint16_t SCB1_2common = setSCB1_2(113,0,0,0,0,0);
uint16_t spal98_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal98_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS98_1,spal98_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS98_2,spal98_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS98_3,spal98_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS98_4,spal98_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS98_5,spal98_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS98_6,spal98_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS98_7,spal98_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS98_8,spal98_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS98_9,spal98_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS98_10,spal98_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS98_11,spal98_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS98_12,spal98_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS98_13,spal98_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS98_14,spal98_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS98_15,spal98_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS98_16,spal98_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen99(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 99 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal99[16];
setpal(pal99,0x0,0x0,0x4fe3,0x4f10,0x8df,0x124d,0x7eff,0x5b84,0x3457,0x1a20,0x1510,0x4999,0xf91,0x159e,0x6016,0x3fe9);
uint16_t spriteMapS99_1[16] = {0x6200,0x6210,0x6220,0x6230,0x6240,0x6250,0x6260,0x6270,0x6280,0x6290,0x62a0,0x62b0,0x62c0,0x62d0,0x62e0,0x62f0};
uint16_t spriteMapS99_2[16] = {0x6201,0x6211,0x6221,0x6231,0x6241,0x6251,0x6261,0x6271,0x6281,0x6291,0x62a1,0x62b1,0x62c1,0x62d1,0x62e1,0x62f1};
uint16_t spriteMapS99_3[16] = {0x6202,0x6212,0x6222,0x6232,0x6242,0x6252,0x6262,0x6272,0x6282,0x6292,0x62a2,0x62b2,0x62c2,0x62d2,0x62e2,0x62f2};
uint16_t spriteMapS99_4[16] = {0x6203,0x6213,0x6223,0x6233,0x6243,0x6253,0x6263,0x6273,0x6283,0x6293,0x62a3,0x62b3,0x62c3,0x62d3,0x62e3,0x62f3};
uint16_t spriteMapS99_5[16] = {0x6204,0x6214,0x6224,0x6234,0x6244,0x6254,0x6264,0x6274,0x6284,0x6294,0x62a4,0x62b4,0x62c4,0x62d4,0x62e4,0x62f4};
uint16_t spriteMapS99_6[16] = {0x6205,0x6215,0x6225,0x6235,0x6245,0x6255,0x6265,0x6275,0x6285,0x6295,0x62a5,0x62b5,0x62c5,0x62d5,0x62e5,0x62f5};
uint16_t spriteMapS99_7[16] = {0x6206,0x6216,0x6226,0x6236,0x6246,0x6256,0x6266,0x6276,0x6286,0x6296,0x62a6,0x62b6,0x62c6,0x62d6,0x62e6,0x62f6};
uint16_t spriteMapS99_8[16] = {0x6207,0x6217,0x6227,0x6237,0x6247,0x6257,0x6267,0x6277,0x6287,0x6297,0x62a7,0x62b7,0x62c7,0x62d7,0x62e7,0x62f7};
uint16_t spriteMapS99_9[16] = {0x6208,0x6218,0x6228,0x6238,0x6248,0x6258,0x6268,0x6278,0x6288,0x6298,0x62a8,0x62b8,0x62c8,0x62d8,0x62e8,0x62f8};
uint16_t spriteMapS99_10[16] = {0x6209,0x6219,0x6229,0x6239,0x6249,0x6259,0x6269,0x6279,0x6289,0x6299,0x62a9,0x62b9,0x62c9,0x62d9,0x62e9,0x62f9};
uint16_t spriteMapS99_11[16] = {0x620a,0x621a,0x622a,0x623a,0x624a,0x625a,0x626a,0x627a,0x628a,0x629a,0x62aa,0x62ba,0x62ca,0x62da,0x62ea,0x62fa};
uint16_t spriteMapS99_12[16] = {0x620b,0x621b,0x622b,0x623b,0x624b,0x625b,0x626b,0x627b,0x628b,0x629b,0x62ab,0x62bb,0x62cb,0x62db,0x62eb,0x62fb};
uint16_t spriteMapS99_13[16] = {0x620c,0x621c,0x622c,0x623c,0x624c,0x625c,0x626c,0x627c,0x628c,0x629c,0x62ac,0x62bc,0x62cc,0x62dc,0x62ec,0x62fc};
uint16_t spriteMapS99_14[16] = {0x620d,0x621d,0x622d,0x623d,0x624d,0x625d,0x626d,0x627d,0x628d,0x629d,0x62ad,0x62bd,0x62cd,0x62dd,0x62ed,0x62fd};
uint16_t spriteMapS99_15[16] = {0x620e,0x621e,0x622e,0x623e,0x624e,0x625e,0x626e,0x627e,0x628e,0x629e,0x62ae,0x62be,0x62ce,0x62de,0x62ee,0x62fe};
uint16_t spriteMapS99_16[16] = {0x620f,0x621f,0x622f,0x623f,0x624f,0x625f,0x626f,0x627f,0x628f,0x629f,0x62af,0x62bf,0x62cf,0x62df,0x62ef,0x62ff};
load_palettes(pal99,PALETTES+PALOFFSET*114);
uint16_t SCB1_2common = setSCB1_2(114,0,0,0,0,0);
uint16_t spal99_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal99_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS99_1,spal99_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS99_2,spal99_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS99_3,spal99_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS99_4,spal99_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS99_5,spal99_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS99_6,spal99_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS99_7,spal99_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS99_8,spal99_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS99_9,spal99_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS99_10,spal99_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS99_11,spal99_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS99_12,spal99_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS99_13,spal99_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS99_14,spal99_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS99_15,spal99_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS99_16,spal99_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen100(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 100 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal100[16];
setpal(pal100,0x0,0x7fff,0x2000,0x4407,0x7111,0xa00,0x7222,0x191c,0x444,0x7555,0xb52,0x2888,0x7d85,0x7aaa,0x7ccc,0x7ec9);
uint16_t spriteMapS100_1[16] = {0x6300,0x6310,0x6320,0x6330,0x6340,0x6350,0x6360,0x6370,0x6380,0x6390,0x63a0,0x63b0,0x63c0,0x63d0,0x63e0,0x63f0};
uint16_t spriteMapS100_2[16] = {0x6301,0x6311,0x6321,0x6331,0x6341,0x6351,0x6361,0x6371,0x6381,0x6391,0x63a1,0x63b1,0x63c1,0x63d1,0x63e1,0x63f1};
uint16_t spriteMapS100_3[16] = {0x6302,0x6312,0x6322,0x6332,0x6342,0x6352,0x6362,0x6372,0x6382,0x6392,0x63a2,0x63b2,0x63c2,0x63d2,0x63e2,0x63f2};
uint16_t spriteMapS100_4[16] = {0x6303,0x6313,0x6323,0x6333,0x6343,0x6353,0x6363,0x6373,0x6383,0x6393,0x63a3,0x63b3,0x63c3,0x63d3,0x63e3,0x63f3};
uint16_t spriteMapS100_5[16] = {0x6304,0x6314,0x6324,0x6334,0x6344,0x6354,0x6364,0x6374,0x6384,0x6394,0x63a4,0x63b4,0x63c4,0x63d4,0x63e4,0x63f4};
uint16_t spriteMapS100_6[16] = {0x6305,0x6315,0x6325,0x6335,0x6345,0x6355,0x6365,0x6375,0x6385,0x6395,0x63a5,0x63b5,0x63c5,0x63d5,0x63e5,0x63f5};
uint16_t spriteMapS100_7[16] = {0x6306,0x6316,0x6326,0x6336,0x6346,0x6356,0x6366,0x6376,0x6386,0x6396,0x63a6,0x63b6,0x63c6,0x63d6,0x63e6,0x63f6};
uint16_t spriteMapS100_8[16] = {0x6307,0x6317,0x6327,0x6337,0x6347,0x6357,0x6367,0x6377,0x6387,0x6397,0x63a7,0x63b7,0x63c7,0x63d7,0x63e7,0x63f7};
uint16_t spriteMapS100_9[16] = {0x6308,0x6318,0x6328,0x6338,0x6348,0x6358,0x6368,0x6378,0x6388,0x6398,0x63a8,0x63b8,0x63c8,0x63d8,0x63e8,0x63f8};
uint16_t spriteMapS100_10[16] = {0x6309,0x6319,0x6329,0x6339,0x6349,0x6359,0x6369,0x6379,0x6389,0x6399,0x63a9,0x63b9,0x63c9,0x63d9,0x63e9,0x63f9};
uint16_t spriteMapS100_11[16] = {0x630a,0x631a,0x632a,0x633a,0x634a,0x635a,0x636a,0x637a,0x638a,0x639a,0x63aa,0x63ba,0x63ca,0x63da,0x63ea,0x63fa};
uint16_t spriteMapS100_12[16] = {0x630b,0x631b,0x632b,0x633b,0x634b,0x635b,0x636b,0x637b,0x638b,0x639b,0x63ab,0x63bb,0x63cb,0x63db,0x63eb,0x63fb};
uint16_t spriteMapS100_13[16] = {0x630c,0x631c,0x632c,0x633c,0x634c,0x635c,0x636c,0x637c,0x638c,0x639c,0x63ac,0x63bc,0x63cc,0x63dc,0x63ec,0x63fc};
uint16_t spriteMapS100_14[16] = {0x630d,0x631d,0x632d,0x633d,0x634d,0x635d,0x636d,0x637d,0x638d,0x639d,0x63ad,0x63bd,0x63cd,0x63dd,0x63ed,0x63fd};
uint16_t spriteMapS100_15[16] = {0x630e,0x631e,0x632e,0x633e,0x634e,0x635e,0x636e,0x637e,0x638e,0x639e,0x63ae,0x63be,0x63ce,0x63de,0x63ee,0x63fe};
uint16_t spriteMapS100_16[16] = {0x630f,0x631f,0x632f,0x633f,0x634f,0x635f,0x636f,0x637f,0x638f,0x639f,0x63af,0x63bf,0x63cf,0x63df,0x63ef,0x63ff};
load_palettes(pal100,PALETTES+PALOFFSET*115);
uint16_t SCB1_2common = setSCB1_2(115,0,0,0,0,0);
uint16_t spal100_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal100_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS100_1,spal100_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS100_2,spal100_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS100_3,spal100_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS100_4,spal100_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS100_5,spal100_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS100_6,spal100_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS100_7,spal100_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS100_8,spal100_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS100_9,spal100_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS100_10,spal100_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS100_11,spal100_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS100_12,spal100_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS100_13,spal100_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS100_14,spal100_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS100_15,spal100_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS100_16,spal100_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen101(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 101 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal101[16];
setpal(pal101,0x0,0x7fff,0x2000,0x4407,0x7111,0x4900,0x7222,0x191c,0x5f0f,0x555,0xb52,0x5787,0x5d95,0x2aaa,0x5ec9,0xddd);
uint16_t spriteMapS101_1[16] = {0x6400,0x6410,0x6420,0x6430,0x6440,0x6450,0x6460,0x6470,0x6480,0x6490,0x64a0,0x64b0,0x64c0,0x64d0,0x64e0,0x64f0};
uint16_t spriteMapS101_2[16] = {0x6401,0x6411,0x6421,0x6431,0x6441,0x6451,0x6461,0x6471,0x6481,0x6491,0x64a1,0x64b1,0x64c1,0x64d1,0x64e1,0x64f1};
uint16_t spriteMapS101_3[16] = {0x6402,0x6412,0x6422,0x6432,0x6442,0x6452,0x6462,0x6472,0x6482,0x6492,0x64a2,0x64b2,0x64c2,0x64d2,0x64e2,0x64f2};
uint16_t spriteMapS101_4[16] = {0x6403,0x6413,0x6423,0x6433,0x6443,0x6453,0x6463,0x6473,0x6483,0x6493,0x64a3,0x64b3,0x64c3,0x64d3,0x64e3,0x64f3};
uint16_t spriteMapS101_5[16] = {0x6404,0x6414,0x6424,0x6434,0x6444,0x6454,0x6464,0x6474,0x6484,0x6494,0x64a4,0x64b4,0x64c4,0x64d4,0x64e4,0x64f4};
uint16_t spriteMapS101_6[16] = {0x6405,0x6415,0x6425,0x6435,0x6445,0x6455,0x6465,0x6475,0x6485,0x6495,0x64a5,0x64b5,0x64c5,0x64d5,0x64e5,0x64f5};
uint16_t spriteMapS101_7[16] = {0x6406,0x6416,0x6426,0x6436,0x6446,0x6456,0x6466,0x6476,0x6486,0x6496,0x64a6,0x64b6,0x64c6,0x64d6,0x64e6,0x64f6};
uint16_t spriteMapS101_8[16] = {0x6407,0x6417,0x6427,0x6437,0x6447,0x6457,0x6467,0x6477,0x6487,0x6497,0x64a7,0x64b7,0x64c7,0x64d7,0x64e7,0x64f7};
uint16_t spriteMapS101_9[16] = {0x6408,0x6418,0x6428,0x6438,0x6448,0x6458,0x6468,0x6478,0x6488,0x6498,0x64a8,0x64b8,0x64c8,0x64d8,0x64e8,0x64f8};
uint16_t spriteMapS101_10[16] = {0x6409,0x6419,0x6429,0x6439,0x6449,0x6459,0x6469,0x6479,0x6489,0x6499,0x64a9,0x64b9,0x64c9,0x64d9,0x64e9,0x64f9};
uint16_t spriteMapS101_11[16] = {0x640a,0x641a,0x642a,0x643a,0x644a,0x645a,0x646a,0x647a,0x648a,0x649a,0x64aa,0x64ba,0x64ca,0x64da,0x64ea,0x64fa};
uint16_t spriteMapS101_12[16] = {0x640b,0x641b,0x642b,0x643b,0x644b,0x645b,0x646b,0x647b,0x648b,0x649b,0x64ab,0x64bb,0x64cb,0x64db,0x64eb,0x64fb};
uint16_t spriteMapS101_13[16] = {0x640c,0x641c,0x642c,0x643c,0x644c,0x645c,0x646c,0x647c,0x648c,0x649c,0x64ac,0x64bc,0x64cc,0x64dc,0x64ec,0x64fc};
uint16_t spriteMapS101_14[16] = {0x640d,0x641d,0x642d,0x643d,0x644d,0x645d,0x646d,0x647d,0x648d,0x649d,0x64ad,0x64bd,0x64cd,0x64dd,0x64ed,0x64fd};
uint16_t spriteMapS101_15[16] = {0x640e,0x641e,0x642e,0x643e,0x644e,0x645e,0x646e,0x647e,0x648e,0x649e,0x64ae,0x64be,0x64ce,0x64de,0x64ee,0x64fe};
uint16_t spriteMapS101_16[16] = {0x640f,0x641f,0x642f,0x643f,0x644f,0x645f,0x646f,0x647f,0x648f,0x649f,0x64af,0x64bf,0x64cf,0x64df,0x64ef,0x64ff};
load_palettes(pal101,PALETTES+PALOFFSET*116);
uint16_t SCB1_2common = setSCB1_2(116,0,0,0,0,0);
uint16_t spal101_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal101_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS101_1,spal101_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS101_2,spal101_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS101_3,spal101_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS101_4,spal101_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS101_5,spal101_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS101_6,spal101_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS101_7,spal101_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS101_8,spal101_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS101_9,spal101_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS101_10,spal101_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS101_11,spal101_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS101_12,spal101_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS101_13,spal101_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS101_14,spal101_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS101_15,spal101_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS101_16,spal101_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen102(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 102 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal102[16];
setpal(pal102,0x0,0x7fff,0x2000,0x4407,0x7111,0x5222,0x333,0x191c,0x5555,0x3b41,0x888,0x7999,0x5d95,0x7ccc,0x7ec9,0x7eee);
uint16_t spriteMapS102_1[16] = {0x6500,0x6510,0x6520,0x6530,0x6540,0x6550,0x6560,0x6570,0x6580,0x6590,0x65a0,0x65b0,0x65c0,0x65d0,0x65e0,0x65f0};
uint16_t spriteMapS102_2[16] = {0x6501,0x6511,0x6521,0x6531,0x6541,0x6551,0x6561,0x6571,0x6581,0x6591,0x65a1,0x65b1,0x65c1,0x65d1,0x65e1,0x65f1};
uint16_t spriteMapS102_3[16] = {0x6502,0x6512,0x6522,0x6532,0x6542,0x6552,0x6562,0x6572,0x6582,0x6592,0x65a2,0x65b2,0x65c2,0x65d2,0x65e2,0x65f2};
uint16_t spriteMapS102_4[16] = {0x6503,0x6513,0x6523,0x6533,0x6543,0x6553,0x6563,0x6573,0x6583,0x6593,0x65a3,0x65b3,0x65c3,0x65d3,0x65e3,0x65f3};
uint16_t spriteMapS102_5[16] = {0x6504,0x6514,0x6524,0x6534,0x6544,0x6554,0x6564,0x6574,0x6584,0x6594,0x65a4,0x65b4,0x65c4,0x65d4,0x65e4,0x65f4};
uint16_t spriteMapS102_6[16] = {0x6505,0x6515,0x6525,0x6535,0x6545,0x6555,0x6565,0x6575,0x6585,0x6595,0x65a5,0x65b5,0x65c5,0x65d5,0x65e5,0x65f5};
uint16_t spriteMapS102_7[16] = {0x6506,0x6516,0x6526,0x6536,0x6546,0x6556,0x6566,0x6576,0x6586,0x6596,0x65a6,0x65b6,0x65c6,0x65d6,0x65e6,0x65f6};
uint16_t spriteMapS102_8[16] = {0x6507,0x6517,0x6527,0x6537,0x6547,0x6557,0x6567,0x6577,0x6587,0x6597,0x65a7,0x65b7,0x65c7,0x65d7,0x65e7,0x65f7};
uint16_t spriteMapS102_9[16] = {0x6508,0x6518,0x6528,0x6538,0x6548,0x6558,0x6568,0x6578,0x6588,0x6598,0x65a8,0x65b8,0x65c8,0x65d8,0x65e8,0x65f8};
uint16_t spriteMapS102_10[16] = {0x6509,0x6519,0x6529,0x6539,0x6549,0x6559,0x6569,0x6579,0x6589,0x6599,0x65a9,0x65b9,0x65c9,0x65d9,0x65e9,0x65f9};
uint16_t spriteMapS102_11[16] = {0x650a,0x651a,0x652a,0x653a,0x654a,0x655a,0x656a,0x657a,0x658a,0x659a,0x65aa,0x65ba,0x65ca,0x65da,0x65ea,0x65fa};
uint16_t spriteMapS102_12[16] = {0x650b,0x651b,0x652b,0x653b,0x654b,0x655b,0x656b,0x657b,0x658b,0x659b,0x65ab,0x65bb,0x65cb,0x65db,0x65eb,0x65fb};
uint16_t spriteMapS102_13[16] = {0x650c,0x651c,0x652c,0x653c,0x654c,0x655c,0x656c,0x657c,0x658c,0x659c,0x65ac,0x65bc,0x65cc,0x65dc,0x65ec,0x65fc};
uint16_t spriteMapS102_14[16] = {0x650d,0x651d,0x652d,0x653d,0x654d,0x655d,0x656d,0x657d,0x658d,0x659d,0x65ad,0x65bd,0x65cd,0x65dd,0x65ed,0x65fd};
uint16_t spriteMapS102_15[16] = {0x650e,0x651e,0x652e,0x653e,0x654e,0x655e,0x656e,0x657e,0x658e,0x659e,0x65ae,0x65be,0x65ce,0x65de,0x65ee,0x65fe};
uint16_t spriteMapS102_16[16] = {0x650f,0x651f,0x652f,0x653f,0x654f,0x655f,0x656f,0x657f,0x658f,0x659f,0x65af,0x65bf,0x65cf,0x65df,0x65ef,0x65ff};
load_palettes(pal102,PALETTES+PALOFFSET*117);
uint16_t SCB1_2common = setSCB1_2(117,0,0,0,0,0);
uint16_t spal102_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal102_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS102_1,spal102_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS102_2,spal102_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS102_3,spal102_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS102_4,spal102_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS102_5,spal102_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS102_6,spal102_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS102_7,spal102_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS102_8,spal102_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS102_9,spal102_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS102_10,spal102_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS102_11,spal102_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS102_12,spal102_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS102_13,spal102_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS102_14,spal102_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS102_15,spal102_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS102_16,spal102_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen103(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 103 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal103[16];
setpal(pal103,0x0,0x7fff,0x2000,0x406,0x7111,0x609,0x1222,0x333,0x191c,0x7555,0x3b41,0x999,0x5d95,0x5ccc,0x7ec9,0x7eee);
uint16_t spriteMapS103_1[16] = {0x6600,0x6610,0x6620,0x6630,0x6640,0x6650,0x6660,0x6670,0x6680,0x6690,0x66a0,0x66b0,0x66c0,0x66d0,0x66e0,0x66f0};
uint16_t spriteMapS103_2[16] = {0x6601,0x6611,0x6621,0x6631,0x6641,0x6651,0x6661,0x6671,0x6681,0x6691,0x66a1,0x66b1,0x66c1,0x66d1,0x66e1,0x66f1};
uint16_t spriteMapS103_3[16] = {0x6602,0x6612,0x6622,0x6632,0x6642,0x6652,0x6662,0x6672,0x6682,0x6692,0x66a2,0x66b2,0x66c2,0x66d2,0x66e2,0x66f2};
uint16_t spriteMapS103_4[16] = {0x6603,0x6613,0x6623,0x6633,0x6643,0x6653,0x6663,0x6673,0x6683,0x6693,0x66a3,0x66b3,0x66c3,0x66d3,0x66e3,0x66f3};
uint16_t spriteMapS103_5[16] = {0x6604,0x6614,0x6624,0x6634,0x6644,0x6654,0x6664,0x6674,0x6684,0x6694,0x66a4,0x66b4,0x66c4,0x66d4,0x66e4,0x66f4};
uint16_t spriteMapS103_6[16] = {0x6605,0x6615,0x6625,0x6635,0x6645,0x6655,0x6665,0x6675,0x6685,0x6695,0x66a5,0x66b5,0x66c5,0x66d5,0x66e5,0x66f5};
uint16_t spriteMapS103_7[16] = {0x6606,0x6616,0x6626,0x6636,0x6646,0x6656,0x6666,0x6676,0x6686,0x6696,0x66a6,0x66b6,0x66c6,0x66d6,0x66e6,0x66f6};
uint16_t spriteMapS103_8[16] = {0x6607,0x6617,0x6627,0x6637,0x6647,0x6657,0x6667,0x6677,0x6687,0x6697,0x66a7,0x66b7,0x66c7,0x66d7,0x66e7,0x66f7};
uint16_t spriteMapS103_9[16] = {0x6608,0x6618,0x6628,0x6638,0x6648,0x6658,0x6668,0x6678,0x6688,0x6698,0x66a8,0x66b8,0x66c8,0x66d8,0x66e8,0x66f8};
uint16_t spriteMapS103_10[16] = {0x6609,0x6619,0x6629,0x6639,0x6649,0x6659,0x6669,0x6679,0x6689,0x6699,0x66a9,0x66b9,0x66c9,0x66d9,0x66e9,0x66f9};
uint16_t spriteMapS103_11[16] = {0x660a,0x661a,0x662a,0x663a,0x664a,0x665a,0x666a,0x667a,0x668a,0x669a,0x66aa,0x66ba,0x66ca,0x66da,0x66ea,0x66fa};
uint16_t spriteMapS103_12[16] = {0x660b,0x661b,0x662b,0x663b,0x664b,0x665b,0x666b,0x667b,0x668b,0x669b,0x66ab,0x66bb,0x66cb,0x66db,0x66eb,0x66fb};
uint16_t spriteMapS103_13[16] = {0x660c,0x661c,0x662c,0x663c,0x664c,0x665c,0x666c,0x667c,0x668c,0x669c,0x66ac,0x66bc,0x66cc,0x66dc,0x66ec,0x66fc};
uint16_t spriteMapS103_14[16] = {0x660d,0x661d,0x662d,0x663d,0x664d,0x665d,0x666d,0x667d,0x668d,0x669d,0x66ad,0x66bd,0x66cd,0x66dd,0x66ed,0x66fd};
uint16_t spriteMapS103_15[16] = {0x660e,0x661e,0x662e,0x663e,0x664e,0x665e,0x666e,0x667e,0x668e,0x669e,0x66ae,0x66be,0x66ce,0x66de,0x66ee,0x66fe};
uint16_t spriteMapS103_16[16] = {0x660f,0x661f,0x662f,0x663f,0x664f,0x665f,0x666f,0x667f,0x668f,0x669f,0x66af,0x66bf,0x66cf,0x66df,0x66ef,0x66ff};
load_palettes(pal103,PALETTES+PALOFFSET*118);
uint16_t SCB1_2common = setSCB1_2(118,0,0,0,0,0);
uint16_t spal103_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal103_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS103_1,spal103_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS103_2,spal103_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS103_3,spal103_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS103_4,spal103_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS103_5,spal103_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS103_6,spal103_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS103_7,spal103_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS103_8,spal103_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS103_9,spal103_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS103_10,spal103_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS103_11,spal103_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS103_12,spal103_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS103_13,spal103_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS103_14,spal103_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS103_15,spal103_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS103_16,spal103_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen104(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 104 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal104[16];
setpal(pal104,0x0,0x7fff,0x2000,0x4407,0x7111,0x5222,0x333,0x191c,0x2434,0x6556,0x3b41,0x5999,0x5d95,0x7ccc,0x7ec9,0x6eee);
uint16_t spriteMapS104_1[16] = {0x6700,0x6710,0x6720,0x6730,0x6740,0x6750,0x6760,0x6770,0x6780,0x6790,0x67a0,0x67b0,0x67c0,0x67d0,0x67e0,0x67f0};
uint16_t spriteMapS104_2[16] = {0x6701,0x6711,0x6721,0x6731,0x6741,0x6751,0x6761,0x6771,0x6781,0x6791,0x67a1,0x67b1,0x67c1,0x67d1,0x67e1,0x67f1};
uint16_t spriteMapS104_3[16] = {0x6702,0x6712,0x6722,0x6732,0x6742,0x6752,0x6762,0x6772,0x6782,0x6792,0x67a2,0x67b2,0x67c2,0x67d2,0x67e2,0x67f2};
uint16_t spriteMapS104_4[16] = {0x6703,0x6713,0x6723,0x6733,0x6743,0x6753,0x6763,0x6773,0x6783,0x6793,0x67a3,0x67b3,0x67c3,0x67d3,0x67e3,0x67f3};
uint16_t spriteMapS104_5[16] = {0x6704,0x6714,0x6724,0x6734,0x6744,0x6754,0x6764,0x6774,0x6784,0x6794,0x67a4,0x67b4,0x67c4,0x67d4,0x67e4,0x67f4};
uint16_t spriteMapS104_6[16] = {0x6705,0x6715,0x6725,0x6735,0x6745,0x6755,0x6765,0x6775,0x6785,0x6795,0x67a5,0x67b5,0x67c5,0x67d5,0x67e5,0x67f5};
uint16_t spriteMapS104_7[16] = {0x6706,0x6716,0x6726,0x6736,0x6746,0x6756,0x6766,0x6776,0x6786,0x6796,0x67a6,0x67b6,0x67c6,0x67d6,0x67e6,0x67f6};
uint16_t spriteMapS104_8[16] = {0x6707,0x6717,0x6727,0x6737,0x6747,0x6757,0x6767,0x6777,0x6787,0x6797,0x67a7,0x67b7,0x67c7,0x67d7,0x67e7,0x67f7};
uint16_t spriteMapS104_9[16] = {0x6708,0x6718,0x6728,0x6738,0x6748,0x6758,0x6768,0x6778,0x6788,0x6798,0x67a8,0x67b8,0x67c8,0x67d8,0x67e8,0x67f8};
uint16_t spriteMapS104_10[16] = {0x6709,0x6719,0x6729,0x6739,0x6749,0x6759,0x6769,0x6779,0x6789,0x6799,0x67a9,0x67b9,0x67c9,0x67d9,0x67e9,0x67f9};
uint16_t spriteMapS104_11[16] = {0x670a,0x671a,0x672a,0x673a,0x674a,0x675a,0x676a,0x677a,0x678a,0x679a,0x67aa,0x67ba,0x67ca,0x67da,0x67ea,0x67fa};
uint16_t spriteMapS104_12[16] = {0x670b,0x671b,0x672b,0x673b,0x674b,0x675b,0x676b,0x677b,0x678b,0x679b,0x67ab,0x67bb,0x67cb,0x67db,0x67eb,0x67fb};
uint16_t spriteMapS104_13[16] = {0x670c,0x671c,0x672c,0x673c,0x674c,0x675c,0x676c,0x677c,0x678c,0x679c,0x67ac,0x67bc,0x67cc,0x67dc,0x67ec,0x67fc};
uint16_t spriteMapS104_14[16] = {0x670d,0x671d,0x672d,0x673d,0x674d,0x675d,0x676d,0x677d,0x678d,0x679d,0x67ad,0x67bd,0x67cd,0x67dd,0x67ed,0x67fd};
uint16_t spriteMapS104_15[16] = {0x670e,0x671e,0x672e,0x673e,0x674e,0x675e,0x676e,0x677e,0x678e,0x679e,0x67ae,0x67be,0x67ce,0x67de,0x67ee,0x67fe};
uint16_t spriteMapS104_16[16] = {0x670f,0x671f,0x672f,0x673f,0x674f,0x675f,0x676f,0x677f,0x678f,0x679f,0x67af,0x67bf,0x67cf,0x67df,0x67ef,0x67ff};
load_palettes(pal104,PALETTES+PALOFFSET*119);
uint16_t SCB1_2common = setSCB1_2(119,0,0,0,0,0);
uint16_t spal104_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal104_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS104_1,spal104_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS104_2,spal104_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS104_3,spal104_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS104_4,spal104_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS104_5,spal104_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS104_6,spal104_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS104_7,spal104_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS104_8,spal104_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS104_9,spal104_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS104_10,spal104_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS104_11,spal104_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS104_12,spal104_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS104_13,spal104_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS104_14,spal104_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS104_15,spal104_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS104_16,spal104_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen105(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 105 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal105[16];
setpal(pal105,0x0,0x7fff,0x2000,0x4407,0x7111,0x5222,0x333,0x191c,0x2434,0x2656,0x3b41,0x5d95,0x699a,0x7ccc,0x7ec9,0xeee);
uint16_t spriteMapS105_1[16] = {0x6800,0x6810,0x6820,0x6830,0x6840,0x6850,0x6860,0x6870,0x6880,0x6890,0x68a0,0x68b0,0x68c0,0x68d0,0x68e0,0x68f0};
uint16_t spriteMapS105_2[16] = {0x6801,0x6811,0x6821,0x6831,0x6841,0x6851,0x6861,0x6871,0x6881,0x6891,0x68a1,0x68b1,0x68c1,0x68d1,0x68e1,0x68f1};
uint16_t spriteMapS105_3[16] = {0x6802,0x6812,0x6822,0x6832,0x6842,0x6852,0x6862,0x6872,0x6882,0x6892,0x68a2,0x68b2,0x68c2,0x68d2,0x68e2,0x68f2};
uint16_t spriteMapS105_4[16] = {0x6803,0x6813,0x6823,0x6833,0x6843,0x6853,0x6863,0x6873,0x6883,0x6893,0x68a3,0x68b3,0x68c3,0x68d3,0x68e3,0x68f3};
uint16_t spriteMapS105_5[16] = {0x6804,0x6814,0x6824,0x6834,0x6844,0x6854,0x6864,0x6874,0x6884,0x6894,0x68a4,0x68b4,0x68c4,0x68d4,0x68e4,0x68f4};
uint16_t spriteMapS105_6[16] = {0x6805,0x6815,0x6825,0x6835,0x6845,0x6855,0x6865,0x6875,0x6885,0x6895,0x68a5,0x68b5,0x68c5,0x68d5,0x68e5,0x68f5};
uint16_t spriteMapS105_7[16] = {0x6806,0x6816,0x6826,0x6836,0x6846,0x6856,0x6866,0x6876,0x6886,0x6896,0x68a6,0x68b6,0x68c6,0x68d6,0x68e6,0x68f6};
uint16_t spriteMapS105_8[16] = {0x6807,0x6817,0x6827,0x6837,0x6847,0x6857,0x6867,0x6877,0x6887,0x6897,0x68a7,0x68b7,0x68c7,0x68d7,0x68e7,0x68f7};
uint16_t spriteMapS105_9[16] = {0x6808,0x6818,0x6828,0x6838,0x6848,0x6858,0x6868,0x6878,0x6888,0x6898,0x68a8,0x68b8,0x68c8,0x68d8,0x68e8,0x68f8};
uint16_t spriteMapS105_10[16] = {0x6809,0x6819,0x6829,0x6839,0x6849,0x6859,0x6869,0x6879,0x6889,0x6899,0x68a9,0x68b9,0x68c9,0x68d9,0x68e9,0x68f9};
uint16_t spriteMapS105_11[16] = {0x680a,0x681a,0x682a,0x683a,0x684a,0x685a,0x686a,0x687a,0x688a,0x689a,0x68aa,0x68ba,0x68ca,0x68da,0x68ea,0x68fa};
uint16_t spriteMapS105_12[16] = {0x680b,0x681b,0x682b,0x683b,0x684b,0x685b,0x686b,0x687b,0x688b,0x689b,0x68ab,0x68bb,0x68cb,0x68db,0x68eb,0x68fb};
uint16_t spriteMapS105_13[16] = {0x680c,0x681c,0x682c,0x683c,0x684c,0x685c,0x686c,0x687c,0x688c,0x689c,0x68ac,0x68bc,0x68cc,0x68dc,0x68ec,0x68fc};
uint16_t spriteMapS105_14[16] = {0x680d,0x681d,0x682d,0x683d,0x684d,0x685d,0x686d,0x687d,0x688d,0x689d,0x68ad,0x68bd,0x68cd,0x68dd,0x68ed,0x68fd};
uint16_t spriteMapS105_15[16] = {0x680e,0x681e,0x682e,0x683e,0x684e,0x685e,0x686e,0x687e,0x688e,0x689e,0x68ae,0x68be,0x68ce,0x68de,0x68ee,0x68fe};
uint16_t spriteMapS105_16[16] = {0x680f,0x681f,0x682f,0x683f,0x684f,0x685f,0x686f,0x687f,0x688f,0x689f,0x68af,0x68bf,0x68cf,0x68df,0x68ef,0x68ff};
load_palettes(pal105,PALETTES+PALOFFSET*120);
uint16_t SCB1_2common = setSCB1_2(120,0,0,0,0,0);
uint16_t spal105_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal105_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS105_1,spal105_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS105_2,spal105_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS105_3,spal105_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS105_4,spal105_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS105_5,spal105_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS105_6,spal105_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS105_7,spal105_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS105_8,spal105_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS105_9,spal105_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS105_10,spal105_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS105_11,spal105_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS105_12,spal105_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS105_13,spal105_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS105_14,spal105_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS105_15,spal105_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS105_16,spal105_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen106(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 106 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal106[16];
setpal(pal106,0x0,0x7fff,0x2000,0x111,0x4407,0x7111,0x7222,0x191c,0x6555,0x3b41,0x7777,0x6d85,0x7999,0x7ccc,0x5ec9,0x2eee);
uint16_t spriteMapS106_1[16] = {0x6900,0x6910,0x6920,0x6930,0x6940,0x6950,0x6960,0x6970,0x6980,0x6990,0x69a0,0x69b0,0x69c0,0x69d0,0x69e0,0x69f0};
uint16_t spriteMapS106_2[16] = {0x6901,0x6911,0x6921,0x6931,0x6941,0x6951,0x6961,0x6971,0x6981,0x6991,0x69a1,0x69b1,0x69c1,0x69d1,0x69e1,0x69f1};
uint16_t spriteMapS106_3[16] = {0x6902,0x6912,0x6922,0x6932,0x6942,0x6952,0x6962,0x6972,0x6982,0x6992,0x69a2,0x69b2,0x69c2,0x69d2,0x69e2,0x69f2};
uint16_t spriteMapS106_4[16] = {0x6903,0x6913,0x6923,0x6933,0x6943,0x6953,0x6963,0x6973,0x6983,0x6993,0x69a3,0x69b3,0x69c3,0x69d3,0x69e3,0x69f3};
uint16_t spriteMapS106_5[16] = {0x6904,0x6914,0x6924,0x6934,0x6944,0x6954,0x6964,0x6974,0x6984,0x6994,0x69a4,0x69b4,0x69c4,0x69d4,0x69e4,0x69f4};
uint16_t spriteMapS106_6[16] = {0x6905,0x6915,0x6925,0x6935,0x6945,0x6955,0x6965,0x6975,0x6985,0x6995,0x69a5,0x69b5,0x69c5,0x69d5,0x69e5,0x69f5};
uint16_t spriteMapS106_7[16] = {0x6906,0x6916,0x6926,0x6936,0x6946,0x6956,0x6966,0x6976,0x6986,0x6996,0x69a6,0x69b6,0x69c6,0x69d6,0x69e6,0x69f6};
uint16_t spriteMapS106_8[16] = {0x6907,0x6917,0x6927,0x6937,0x6947,0x6957,0x6967,0x6977,0x6987,0x6997,0x69a7,0x69b7,0x69c7,0x69d7,0x69e7,0x69f7};
uint16_t spriteMapS106_9[16] = {0x6908,0x6918,0x6928,0x6938,0x6948,0x6958,0x6968,0x6978,0x6988,0x6998,0x69a8,0x69b8,0x69c8,0x69d8,0x69e8,0x69f8};
uint16_t spriteMapS106_10[16] = {0x6909,0x6919,0x6929,0x6939,0x6949,0x6959,0x6969,0x6979,0x6989,0x6999,0x69a9,0x69b9,0x69c9,0x69d9,0x69e9,0x69f9};
uint16_t spriteMapS106_11[16] = {0x690a,0x691a,0x692a,0x693a,0x694a,0x695a,0x696a,0x697a,0x698a,0x699a,0x69aa,0x69ba,0x69ca,0x69da,0x69ea,0x69fa};
uint16_t spriteMapS106_12[16] = {0x690b,0x691b,0x692b,0x693b,0x694b,0x695b,0x696b,0x697b,0x698b,0x699b,0x69ab,0x69bb,0x69cb,0x69db,0x69eb,0x69fb};
uint16_t spriteMapS106_13[16] = {0x690c,0x691c,0x692c,0x693c,0x694c,0x695c,0x696c,0x697c,0x698c,0x699c,0x69ac,0x69bc,0x69cc,0x69dc,0x69ec,0x69fc};
uint16_t spriteMapS106_14[16] = {0x690d,0x691d,0x692d,0x693d,0x694d,0x695d,0x696d,0x697d,0x698d,0x699d,0x69ad,0x69bd,0x69cd,0x69dd,0x69ed,0x69fd};
uint16_t spriteMapS106_15[16] = {0x690e,0x691e,0x692e,0x693e,0x694e,0x695e,0x696e,0x697e,0x698e,0x699e,0x69ae,0x69be,0x69ce,0x69de,0x69ee,0x69fe};
uint16_t spriteMapS106_16[16] = {0x690f,0x691f,0x692f,0x693f,0x694f,0x695f,0x696f,0x697f,0x698f,0x699f,0x69af,0x69bf,0x69cf,0x69df,0x69ef,0x69ff};
load_palettes(pal106,PALETTES+PALOFFSET*121);
uint16_t SCB1_2common = setSCB1_2(121,0,0,0,0,0);
uint16_t spal106_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal106_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS106_1,spal106_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS106_2,spal106_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS106_3,spal106_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS106_4,spal106_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS106_5,spal106_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS106_6,spal106_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS106_7,spal106_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS106_8,spal106_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS106_9,spal106_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS106_10,spal106_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS106_11,spal106_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS106_12,spal106_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS106_13,spal106_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS106_14,spal106_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS106_15,spal106_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS106_16,spal106_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen107(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 107 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal107[16];
setpal(pal107,0x0,0x7fff,0x428d,0x6f01,0x1,0x204a,0x2026,0x3003,0x5b11,0x2569,0xb67,0x5d11,0x1911,0x2016,0x6049,0x2abd);
uint16_t spriteMapS107_1[16] = {0x6a00,0x6a10,0x6a20,0x6a30,0x6a40,0x6a50,0x6a60,0x6a70,0x6a80,0x6a90,0x6aa0,0x6ab0,0x6ac0,0x6ad0,0x6ae0,0x6af0};
uint16_t spriteMapS107_2[16] = {0x6a01,0x6a11,0x6a21,0x6a31,0x6a41,0x6a51,0x6a61,0x6a71,0x6a81,0x6a91,0x6aa1,0x6ab1,0x6ac1,0x6ad1,0x6ae1,0x6af1};
uint16_t spriteMapS107_3[16] = {0x6a02,0x6a12,0x6a22,0x6a32,0x6a42,0x6a52,0x6a62,0x6a72,0x6a82,0x6a92,0x6aa2,0x6ab2,0x6ac2,0x6ad2,0x6ae2,0x6af2};
uint16_t spriteMapS107_4[16] = {0x6a03,0x6a13,0x6a23,0x6a33,0x6a43,0x6a53,0x6a63,0x6a73,0x6a83,0x6a93,0x6aa3,0x6ab3,0x6ac3,0x6ad3,0x6ae3,0x6af3};
uint16_t spriteMapS107_5[16] = {0x6a04,0x6a14,0x6a24,0x6a34,0x6a44,0x6a54,0x6a64,0x6a74,0x6a84,0x6a94,0x6aa4,0x6ab4,0x6ac4,0x6ad4,0x6ae4,0x6af4};
uint16_t spriteMapS107_6[16] = {0x6a05,0x6a15,0x6a25,0x6a35,0x6a45,0x6a55,0x6a65,0x6a75,0x6a85,0x6a95,0x6aa5,0x6ab5,0x6ac5,0x6ad5,0x6ae5,0x6af5};
uint16_t spriteMapS107_7[16] = {0x6a06,0x6a16,0x6a26,0x6a36,0x6a46,0x6a56,0x6a66,0x6a76,0x6a86,0x6a96,0x6aa6,0x6ab6,0x6ac6,0x6ad6,0x6ae6,0x6af6};
uint16_t spriteMapS107_8[16] = {0x6a07,0x6a17,0x6a27,0x6a37,0x6a47,0x6a57,0x6a67,0x6a77,0x6a87,0x6a97,0x6aa7,0x6ab7,0x6ac7,0x6ad7,0x6ae7,0x6af7};
uint16_t spriteMapS107_9[16] = {0x6a08,0x6a18,0x6a28,0x6a38,0x6a48,0x6a58,0x6a68,0x6a78,0x6a88,0x6a98,0x6aa8,0x6ab8,0x6ac8,0x6ad8,0x6ae8,0x6af8};
uint16_t spriteMapS107_10[16] = {0x6a09,0x6a19,0x6a29,0x6a39,0x6a49,0x6a59,0x6a69,0x6a79,0x6a89,0x6a99,0x6aa9,0x6ab9,0x6ac9,0x6ad9,0x6ae9,0x6af9};
uint16_t spriteMapS107_11[16] = {0x6a0a,0x6a1a,0x6a2a,0x6a3a,0x6a4a,0x6a5a,0x6a6a,0x6a7a,0x6a8a,0x6a9a,0x6aaa,0x6aba,0x6aca,0x6ada,0x6aea,0x6afa};
uint16_t spriteMapS107_12[16] = {0x6a0b,0x6a1b,0x6a2b,0x6a3b,0x6a4b,0x6a5b,0x6a6b,0x6a7b,0x6a8b,0x6a9b,0x6aab,0x6abb,0x6acb,0x6adb,0x6aeb,0x6afb};
uint16_t spriteMapS107_13[16] = {0x6a0c,0x6a1c,0x6a2c,0x6a3c,0x6a4c,0x6a5c,0x6a6c,0x6a7c,0x6a8c,0x6a9c,0x6aac,0x6abc,0x6acc,0x6adc,0x6aec,0x6afc};
uint16_t spriteMapS107_14[16] = {0x6a0d,0x6a1d,0x6a2d,0x6a3d,0x6a4d,0x6a5d,0x6a6d,0x6a7d,0x6a8d,0x6a9d,0x6aad,0x6abd,0x6acd,0x6add,0x6aed,0x6afd};
uint16_t spriteMapS107_15[16] = {0x6a0e,0x6a1e,0x6a2e,0x6a3e,0x6a4e,0x6a5e,0x6a6e,0x6a7e,0x6a8e,0x6a9e,0x6aae,0x6abe,0x6ace,0x6ade,0x6aee,0x6afe};
uint16_t spriteMapS107_16[16] = {0x6a0f,0x6a1f,0x6a2f,0x6a3f,0x6a4f,0x6a5f,0x6a6f,0x6a7f,0x6a8f,0x6a9f,0x6aaf,0x6abf,0x6acf,0x6adf,0x6aef,0x6aff};
load_palettes(pal107,PALETTES+PALOFFSET*122);
uint16_t SCB1_2common = setSCB1_2(122,0,0,0,0,0);
uint16_t spal107_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal107_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS107_1,spal107_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS107_2,spal107_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS107_3,spal107_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS107_4,spal107_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS107_5,spal107_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS107_6,spal107_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS107_7,spal107_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS107_8,spal107_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS107_9,spal107_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS107_10,spal107_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS107_11,spal107_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS107_12,spal107_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS107_13,spal107_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS107_14,spal107_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS107_15,spal107_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS107_16,spal107_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen108(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 108 ******************************************/
uint16_t  SCB2    = 0x0;
uint16_t  SCB3    = 0x0;
uint16_t  SCB4    = 0x0;
uint16_t  pal108[16];
setpal(pal108,0x0,0x5000,0x6e81,0x5e20,0x2204,0x2eef,0x7820,0x132c,0x6411,0x999,0x1f60,0x7915,0x656e,0x4ff3,0x2fc3,0x9bf);
uint16_t spriteMapS108_1[16] = {0x6b00,0x6b10,0x6b20,0x6b30,0x6b40,0x6b50,0x6b60,0x6b70,0x6b80,0x6b90,0x6ba0,0x6bb0,0x6bc0,0x6bd0,0x6be0,0x6bf0};
uint16_t spriteMapS108_2[16] = {0x6b01,0x6b11,0x6b21,0x6b31,0x6b41,0x6b51,0x6b61,0x6b71,0x6b81,0x6b91,0x6ba1,0x6bb1,0x6bc1,0x6bd1,0x6be1,0x6bf1};
uint16_t spriteMapS108_3[16] = {0x6b02,0x6b12,0x6b22,0x6b32,0x6b42,0x6b52,0x6b62,0x6b72,0x6b82,0x6b92,0x6ba2,0x6bb2,0x6bc2,0x6bd2,0x6be2,0x6bf2};
uint16_t spriteMapS108_4[16] = {0x6b03,0x6b13,0x6b23,0x6b33,0x6b43,0x6b53,0x6b63,0x6b73,0x6b83,0x6b93,0x6ba3,0x6bb3,0x6bc3,0x6bd3,0x6be3,0x6bf3};
uint16_t spriteMapS108_5[16] = {0x6b04,0x6b14,0x6b24,0x6b34,0x6b44,0x6b54,0x6b64,0x6b74,0x6b84,0x6b94,0x6ba4,0x6bb4,0x6bc4,0x6bd4,0x6be4,0x6bf4};
uint16_t spriteMapS108_6[16] = {0x6b05,0x6b15,0x6b25,0x6b35,0x6b45,0x6b55,0x6b65,0x6b75,0x6b85,0x6b95,0x6ba5,0x6bb5,0x6bc5,0x6bd5,0x6be5,0x6bf5};
uint16_t spriteMapS108_7[16] = {0x6b06,0x6b16,0x6b26,0x6b36,0x6b46,0x6b56,0x6b66,0x6b76,0x6b86,0x6b96,0x6ba6,0x6bb6,0x6bc6,0x6bd6,0x6be6,0x6bf6};
uint16_t spriteMapS108_8[16] = {0x6b07,0x6b17,0x6b27,0x6b37,0x6b47,0x6b57,0x6b67,0x6b77,0x6b87,0x6b97,0x6ba7,0x6bb7,0x6bc7,0x6bd7,0x6be7,0x6bf7};
uint16_t spriteMapS108_9[16] = {0x6b08,0x6b18,0x6b28,0x6b38,0x6b48,0x6b58,0x6b68,0x6b78,0x6b88,0x6b98,0x6ba8,0x6bb8,0x6bc8,0x6bd8,0x6be8,0x6bf8};
uint16_t spriteMapS108_10[16] = {0x6b09,0x6b19,0x6b29,0x6b39,0x6b49,0x6b59,0x6b69,0x6b79,0x6b89,0x6b99,0x6ba9,0x6bb9,0x6bc9,0x6bd9,0x6be9,0x6bf9};
uint16_t spriteMapS108_11[16] = {0x6b0a,0x6b1a,0x6b2a,0x6b3a,0x6b4a,0x6b5a,0x6b6a,0x6b7a,0x6b8a,0x6b9a,0x6baa,0x6bba,0x6bca,0x6bda,0x6bea,0x6bfa};
uint16_t spriteMapS108_12[16] = {0x6b0b,0x6b1b,0x6b2b,0x6b3b,0x6b4b,0x6b5b,0x6b6b,0x6b7b,0x6b8b,0x6b9b,0x6bab,0x6bbb,0x6bcb,0x6bdb,0x6beb,0x6bfb};
uint16_t spriteMapS108_13[16] = {0x6b0c,0x6b1c,0x6b2c,0x6b3c,0x6b4c,0x6b5c,0x6b6c,0x6b7c,0x6b8c,0x6b9c,0x6bac,0x6bbc,0x6bcc,0x6bdc,0x6bec,0x6bfc};
uint16_t spriteMapS108_14[16] = {0x6b0d,0x6b1d,0x6b2d,0x6b3d,0x6b4d,0x6b5d,0x6b6d,0x6b7d,0x6b8d,0x6b9d,0x6bad,0x6bbd,0x6bcd,0x6bdd,0x6bed,0x6bfd};
uint16_t spriteMapS108_15[16] = {0x6b0e,0x6b1e,0x6b2e,0x6b3e,0x6b4e,0x6b5e,0x6b6e,0x6b7e,0x6b8e,0x6b9e,0x6bae,0x6bbe,0x6bce,0x6bde,0x6bee,0x6bfe};
uint16_t spriteMapS108_16[16] = {0x6b0f,0x6b1f,0x6b2f,0x6b3f,0x6b4f,0x6b5f,0x6b6f,0x6b7f,0x6b8f,0x6b9f,0x6baf,0x6bbf,0x6bcf,0x6bdf,0x6bef,0x6bff};
load_palettes(pal108,PALETTES+PALOFFSET*123);
uint16_t SCB1_2common = setSCB1_2(123,0,0,0,0,0);
uint16_t spal108_1[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_2[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_3[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_4[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_5[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_6[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_7[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_8[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_9[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_10[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_11[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_12[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_13[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_14[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_15[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
uint16_t spal108_16[16]={SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common,SCB1_2common};
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,0,min_crt_sz);
SCB4    = setSCB4(x0);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*0,1,(sprite_base>>6)+0,spriteMapS108_1,spal108_1,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*1);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*1,1,(sprite_base>>6)+1,spriteMapS108_2,spal108_2,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*2);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*2,1,(sprite_base>>6)+2,spriteMapS108_3,spal108_3,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*3);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*3,1,(sprite_base>>6)+3,spriteMapS108_4,spal108_4,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*4);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*4,1,(sprite_base>>6)+4,spriteMapS108_5,spal108_5,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*5);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*5,1,(sprite_base>>6)+5,spriteMapS108_6,spal108_6,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*6);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*6,1,(sprite_base>>6)+6,spriteMapS108_7,spal108_7,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*7);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*7,1,(sprite_base>>6)+7,spriteMapS108_8,spal108_8,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*8);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*8,1,(sprite_base>>6)+8,spriteMapS108_9,spal108_9,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*9);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*9,1,(sprite_base>>6)+9,spriteMapS108_10,spal108_10,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*10);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*10,1,(sprite_base>>6)+10,spriteMapS108_11,spal108_11,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*11);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*11,1,(sprite_base>>6)+11,spriteMapS108_12,spal108_12,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*12);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*12,1,(sprite_base>>6)+12,spriteMapS108_13,spal108_13,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*13);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*13,1,(sprite_base>>6)+13,spriteMapS108_14,spal108_14,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*14);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*14,1,(sprite_base>>6)+14,spriteMapS108_15,spal108_15,16,SCB2,SCB3,SCB4);
SCB2    = setSCB2(xr,yr);
SCB3    = setSCB3(496-y0,1,min_crt_sz);
SCB4    = setSCB4(x0+16*15);
setBACKDROP(backdrop);
vram_sprite(sprite_base + 64*15,1,(sprite_base>>6)+15,spriteMapS108_16,spal108_16,16,SCB2,SCB3,SCB4);
}


void NEOGEO_USER showScreen109(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 109 ******************************************/
uint16_t  pal109[16];
setpal(pal109,0x0,0x7331,0x7445,0x510,0x4744,0x4774,0x1942,0x3ccd,0x5cb6,0x6d97,0xe75,0x1fc9,0x7fdb,0x0,0x0,0x0);
load_palettes(pal109,PALETTES+PALOFFSET*124);
}


void NEOGEO_USER showScreen110(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 110 ******************************************/
uint16_t  pal110[16];
setpal(pal110,0x0,0x7331,0x7445,0x510,0x4744,0x4774,0x1942,0x3ccd,0x5cb6,0x6d97,0xe75,0x1fc9,0x7fdb,0x0,0x0,0x0);
load_palettes(pal110,PALETTES+PALOFFSET*125);
}


void NEOGEO_USER showScreen111(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 111 ******************************************/
uint16_t  pal111[16];
setpal(pal111,0x0,0x7331,0x7445,0x510,0x4744,0x4774,0x1942,0x3ccd,0x5cb6,0x6d97,0xe75,0x1fc9,0x7fdb,0x0,0x0,0x0);
load_palettes(pal111,PALETTES+PALOFFSET*126);
}


void NEOGEO_USER showScreen112(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 112 ******************************************/
uint16_t  pal112[16];
setpal(pal112,0x0,0x7331,0x7445,0x510,0x4744,0x4774,0x1942,0x3ccd,0x5cb6,0x6d97,0xe75,0x1fc9,0x7fdb,0x0,0x0,0x0);
load_palettes(pal112,PALETTES+PALOFFSET*127);
}


void NEOGEO_USER showScreen113(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 113 ******************************************/
uint16_t  pal113[16];
setpal(pal113,0x0,0x7331,0x7445,0x510,0x4744,0x4774,0x4899,0x1942,0x3ccd,0x5cb6,0x6d97,0xe75,0x1fc9,0x7fdb,0x0,0x0);
load_palettes(pal113,PALETTES+PALOFFSET*128);
}


void NEOGEO_USER showScreen114(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 114 ******************************************/
uint16_t  pal114[16];
setpal(pal114,0x0,0x7331,0x7445,0x510,0x4744,0x4774,0x1942,0x3ccd,0x5cb6,0x6d97,0xe75,0x1fc9,0x7fdb,0x0,0x0,0x0);
load_palettes(pal114,PALETTES+PALOFFSET*129);
}


void NEOGEO_USER showScreen115(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 115 ******************************************/
uint16_t  pal115[16];
setpal(pal115,0x0,0x7331,0x7445,0x510,0x4744,0x4774,0x4899,0x1942,0x3ccd,0x5cb6,0x6d97,0xe75,0x1fc9,0x7fdb,0x0,0x0);
load_palettes(pal115,PALETTES+PALOFFSET*130);
}


void NEOGEO_USER showScreen116(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 116 ******************************************/
uint16_t  pal116[16];
setpal(pal116,0x0,0x7331,0x7445,0x510,0x4744,0x4774,0x1942,0x3ccd,0x5cb6,0x6d97,0xe75,0x1fc9,0x7fdb,0x0,0x0,0x0);
load_palettes(pal116,PALETTES+PALOFFSET*131);
}


void NEOGEO_USER showScreen117(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 117 ******************************************/
uint16_t  pal117[16];
setpal(pal117,0x0,0x7331,0x7445,0x510,0x4744,0x4774,0x4899,0x1942,0x3ccd,0x5cb6,0x6d97,0xe75,0x1fc9,0x7fdb,0x0,0x0);
load_palettes(pal117,PALETTES+PALOFFSET*132);
}


void NEOGEO_USER showScreen118(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 118 ******************************************/
uint16_t  pal118[16];
setpal(pal118,0x0,0x7331,0x7445,0x510,0x4744,0x4774,0x1942,0x3ccd,0x5cb6,0x6d97,0xe75,0x1fc9,0x7fdb,0x0,0x0,0x0);
load_palettes(pal118,PALETTES+PALOFFSET*133);
}


void NEOGEO_USER showScreen119(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 119 ******************************************/
uint16_t  pal119[16];
setpal(pal119,0x0,0x7331,0x7445,0x510,0x4744,0x4774,0x4899,0x1942,0x3ccd,0x5cb6,0x6d97,0xe75,0x1fc9,0x7fdb,0x0,0x0);
load_palettes(pal119,PALETTES+PALOFFSET*134);
}


void NEOGEO_USER showScreen120(int x0,int y0,int xr,int yr,int min_crt_sz,uint16_t backdrop,uint16_t sprite_base) {
/****************************************** screen 120 ******************************************/
uint16_t  pal120[16];
setpal(pal120,0x0,0x7331,0x7445,0x510,0x4744,0x4774,0x1942,0x3ccd,0x5cb6,0x6d97,0xe75,0x1fc9,0x7fdb,0x0,0x0,0x0);
load_palettes(pal120,PALETTES+PALOFFSET*135);
}

const NGShowScreenFn ng_screen_table[NG_SCREEN_TABLE_MAX] = {
    0, /* index 0 unused */
    showScreen1,
    showScreen2,
    showScreen3,
    showScreen4,
    showScreen5,
    showScreen6,
    showScreen7,
    showScreen8,
    showScreen9,
    showScreen10,
    showScreen11,
    showScreen12,
    showScreen13,
    showScreen14,
    showScreen15,
    showScreen16,
    showScreen17,
    showScreen18,
    showScreen19,
    showScreen20,
    showScreen21,
    showScreen22,
    showScreen23,
    showScreen24,
    showScreen25,
    showScreen26,
    showScreen27,
    showScreen28,
    showScreen29,
    showScreen30,
    showScreen31,
    showScreen32,
    showScreen33,
    showScreen34,
    showScreen35,
    showScreen36,
    showScreen37,
    showScreen38,
    showScreen39,
    showScreen40,
    showScreen41,
    showScreen42,
    showScreen43,
    showScreen44,
    showScreen45,
    showScreen46,
    showScreen47,
    showScreen48,
    showScreen49,
    showScreen50,
    showScreen51,
    showScreen52,
    showScreen53,
    showScreen54,
    showScreen55,
    showScreen56,
    showScreen57,
    showScreen58,
    showScreen59,
    showScreen60,
    showScreen61,
    showScreen62,
    showScreen63,
    showScreen64,
    showScreen65,
    showScreen66,
    showScreen67,
    showScreen68,
    showScreen69,
    showScreen70,
    showScreen71,
    showScreen72,
    showScreen73,
    showScreen74,
    showScreen75,
    showScreen76,
    showScreen77,
    showScreen78,
    showScreen79,
    showScreen80,
    showScreen81,
    showScreen82,
    showScreen83,
    showScreen84,
    showScreen85,
    showScreen86,
    showScreen87,
    showScreen88,
    showScreen89,
    showScreen90,
    showScreen91,
    showScreen92,
    showScreen93,
    showScreen94,
    showScreen95,
    showScreen96,
    showScreen97,
    showScreen98,
    showScreen99,
    showScreen100,
    showScreen101,
    showScreen102,
    showScreen103,
    showScreen104,
    showScreen105,
    showScreen106,
    showScreen107,
    showScreen108,
    showScreen109,
    showScreen110,
    showScreen111,
    showScreen112,
    showScreen113,
    showScreen114,
    showScreen115,
    showScreen116,
    showScreen117,
    showScreen118,
    showScreen119,
    showScreen120,
};
const uint16_t ng_screen_count = 120;
