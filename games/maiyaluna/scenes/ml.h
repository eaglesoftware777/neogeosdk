/*
 * ml.h -- Maiya & Luna: Wonderful Adventures in Nature Fantasy.
 *
 * A top-down adventure on the SDK's RPG engine (sdk/2d_engine/ng_rpg.c):
 * two sisters explore the Valley of Sacred Falls, solve its old engineering
 * puzzles and bring its seven wonders back to work. ml_game.c runs the game
 * (title, story, play, menus, saving); ml_draw.c draws it and plays its
 * sounds; ml_world.c holds the world tables written in the world files (world/).
 */
#ifndef ML_H
#define ML_H
#include <stdint.h>
#include "sdk/neogeo.h"
#include "sdk/ng_system.h"
#include "games/maiyaluna/scenes/ml_world.h"

#define ML_START_FLAG   0x100700u     /* game RAM latch for a console START */

/* inputs (poll_joystick: P1 in the low byte, starts in the high byte) */
#define ML_UP      0x0001u
#define ML_DOWN    0x0002u
#define ML_LEFT    0x0004u
#define ML_RIGHT   0x0008u
#define ML_A       0x0010u
#define ML_B       0x0020u
#define ML_C       0x0040u
#define ML_D       0x0080u
#define ML_START   0x0100u
#define ML_BUTTONS (ML_A | ML_B | ML_C | ML_D)
#define ML_DIRS    (ML_UP | ML_DOWN | ML_LEFT | ML_RIGHT)

/* FIX text palettes */
enum { ML_TXT_WHITE = 0, ML_TXT_GOLD, ML_TXT_PINK, ML_TXT_SKY, ML_TXT_MINT, ML_TXT_HEART, ML_TXT_GREY };

/* effects (ADPCM-A samples 1..n: sound/samples/in_wav_a) */
enum {
    ML_SFX_COIN = 0, ML_SFX_START, ML_SFX_TALK, ML_SFX_PAGE, ML_SFX_PICK, ML_SFX_COLLECT, ML_SFX_PUSH,
    ML_SFX_THUD, ML_SFX_CLICK, ML_SFX_LEVER, ML_SFX_GATE, ML_SFX_GEARS, ML_SFX_WATER, ML_SFX_CHIME0,
    ML_SFX_CHIME1, ML_SFX_CHIME2, ML_SFX_WRONG, ML_SFX_SOLVED, ML_SFX_HURT, ML_SFX_WHIP, ML_SFX_CALM,
    ML_SFX_SWITCH, ML_SFX_REST, ML_SFX_MENU, ML_SFX_SECRET, ML_SFX_STEP, ML_SFX_COUNT
};

/* draw.c ------------------------------------------------------------------- */
void     NEOGEO_USER ml_vblank(void);
void     NEOGEO_USER ml_presentation_init(void);
void     NEOGEO_USER ml_fix_palettes(void);
uint16_t NEOGEO_USER ml_frame(void);
uint16_t NEOGEO_USER ml_held(void);
uint16_t NEOGEO_USER ml_pressed(void);
uint16_t NEOGEO_USER ml_released(void);
uint16_t NEOGEO_USER ml_repeat(void);
uint16_t NEOGEO_USER ml_rand(void);
void     NEOGEO_USER ml_wait(uint16_t frames);
uint8_t  NEOGEO_USER ml_wait_button(uint16_t frames);
void     NEOGEO_USER ml_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal);
void     NEOGEO_USER ml_puts_c(uint8_t y, const char *text, uint8_t pal);
void     NEOGEO_USER ml_putn(uint8_t x, uint8_t y, uint32_t value, uint8_t digits, uint8_t pal);
void     NEOGEO_USER ml_text_block(uint8_t x, uint8_t y, const char *text, uint8_t pal);
void     NEOGEO_USER ml_clear_text(uint8_t y0, uint8_t y1);
void     NEOGEO_USER ml_fade_out(void);
void     NEOGEO_USER ml_picture(uint8_t art);         /* a full-screen picture: title, chapter */
void     NEOGEO_USER ml_chapter_picture(uint8_t chapter);   /* chapter (from 0)'s card picture */
void     NEOGEO_USER ml_logo(uint8_t show, int16_t x, int16_t y);
void     NEOGEO_USER ml_card(uint8_t show);
void     NEOGEO_USER ml_icon(uint8_t slot, uint8_t frame, int16_t x, int16_t y);   /* menu icons; frame 0xFF hides */
void     NEOGEO_USER ml_icons_hide(void);
void     NEOGEO_USER ml_person(uint8_t slot, uint8_t look, uint8_t frame, uint8_t flip, int16_t x, int16_t y);
void     NEOGEO_USER ml_room_show(NGRpg *g);          /* fade into the room the engine is in */
void     NEOGEO_USER ml_room_draw(NGRpg *g);          /* once a frame */
void     NEOGEO_USER ml_room_hide(void);
void     NEOGEO_USER ml_bell_rung(uint8_t obj);
void     NEOGEO_USER ml_flash_member(uint8_t m);
enum { ML_POSE_REACH = 0, ML_POSE_WHIP, ML_POSE_LOOK, ML_POSE_CHEER };
void     NEOGEO_USER ml_member_pose(uint8_t m, uint8_t pose);    /* a sister acts for a moment */
void     NEOGEO_USER ml_effect(uint8_t kind, int16_t x, int16_t y);
enum { ML_FX_SPARKLE = 0, ML_FX_HIT, ML_FX_SPLASH };
void     NEOGEO_USER ml_dialog(uint8_t speaker, const char *text, uint8_t typing);
void     NEOGEO_USER ml_dialog_choices(const NGRpgDialogueNode *n, uint8_t offered, uint8_t cursor);
void     NEOGEO_USER ml_dialog_hide(void);
uint8_t  NEOGEO_USER ml_dialog_typing(void);
void     NEOGEO_USER ml_dialog_finish(void);
void     NEOGEO_USER ml_hud(NGRpg *g, uint8_t tool, uint8_t full);
void     NEOGEO_USER ml_toast(const char *a, const char *b, uint8_t icon);
void     NEOGEO_USER ml_toast_tick(void);
void     NEOGEO_USER ml_music(uint8_t track);
uint8_t  NEOGEO_USER ml_music_playing(void);      /* the last track ml_music started */
void     NEOGEO_USER ml_sfx(uint8_t sfx);

/* game.c ------------------------------------------------------------------- */
extern uint8_t ml_journey;          /* ML_J_MAIYA or ML_J_LUNA: who set out */
void     NEOGEO_USER ml_title_screen(void);
void     NEOGEO_USER ml_run_attract(void);
void     NEOGEO_USER ml_run(void);

/* user.c ------------------------------------------------------------------- */
uint8_t  NEOGEO_USER ml_start_pending(void);
void     NEOGEO_USER ml_auto_start(void);
uint8_t  NEOGEO_USER ml_spend_credit(void);

#endif
