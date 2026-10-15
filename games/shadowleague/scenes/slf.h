#ifndef SLF_H
#define SLF_H
/*
 * Shadow League Fighting: shared declarations.
 *
 * slf_draw.c owns the hardware side (sprite groups, palettes, FIX HUD,
 * frame pump, sound); slf_game.c owns the flow (title, select, matches,
 * attract). The fight itself runs in sdk/2d_engine/ng_fight.
 */
#include <stdint.h>
#include "sdk/neogeo.h"
#include "sdk/2d_engine/ng_fight.h"

#define SLF_START_FLAG   0x100700u   /* BIOS handshake: a credited start */
#define SLF_P2_FLAG      0x100701u   /* player 2 pressed start during play */

/* FIX palettes (artbox FIX_PALS). */
#define SLF_PAL_TEXT   0u
#define SLF_PAL_GOLD   1u
#define SLF_PAL_DIM    2u
#define SLF_PAL_HUD    3u
#define SLF_PAL_BIG    4u
#define SLF_PAL_RED    5u
#define SLF_PAL_BLUE   6u
#define SLF_PAL_WHITE  7u

/* Sound effects (tools/sound.py order, 1-based like SOUND_SFX_n + 1). */
enum { SLF_SFX_COIN = 1, SLF_SFX_MENU, SLF_SFX_START, SLF_SFX_HIT_LIGHT, SLF_SFX_HIT_HEAVY,
       SLF_SFX_BLOCK, SLF_SFX_WHOOSH, SLF_SFX_SPECIAL, SLF_SFX_SUPER, SLF_SFX_THROW, SLF_SFX_KO,
       SLF_SFX_LAND, SLF_SFX_BELL, SLF_SFX_COUNTER, SLF_SFX_SELECT };
enum { SLF_TRACK_TITLE = 1, SLF_TRACK_DOJO, SLF_TRACK_VICTORY };

#define SLF_FLOOR_Y 212

/* slf_draw.c */
void     NEOGEO_USER slf_presentation_init(void);
void     NEOGEO_USER slf_palette_vblank(void);
void     NEOGEO_USER slf_fix_palettes_init(void);
void     NEOGEO_USER slf_scene_begin(void);
void     NEOGEO_USER slf_fade_out(void);
uint16_t NEOGEO_USER slf_frame(void);
uint8_t  NEOGEO_USER slf_joy(uint8_t player);
uint8_t  NEOGEO_USER slf_pressed(uint8_t player);
uint8_t  NEOGEO_USER slf_start_pressed(uint8_t player);
uint16_t NEOGEO_USER slf_rand(void);
void     NEOGEO_USER slf_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal);
void     NEOGEO_USER slf_puts_center(uint8_t y, const char *text, uint8_t pal);
void     NEOGEO_USER slf_puts_right(uint8_t x_end, uint8_t y, const char *text, uint8_t pal);
void     NEOGEO_USER slf_big(uint8_t x, uint8_t y, uint16_t tile, uint8_t w, uint8_t h, uint8_t pal);
void     NEOGEO_USER slf_big_center(uint8_t y, uint16_t tile, uint8_t w, uint8_t h, uint8_t pal);
void     NEOGEO_USER slf_clear_rows(uint8_t y0, uint8_t y1);
void     NEOGEO_USER slf_stage_set(uint8_t stage_id);
uint8_t  NEOGEO_USER slf_stage_current(void);
void     NEOGEO_USER slf_stage_show(int16_t camera_x);
void     NEOGEO_USER slf_fighter_palettes(uint8_t def0, uint8_t alt0, uint8_t def1, uint8_t alt1);
void     NEOGEO_USER slf_fighter_pose(uint8_t slot, uint8_t def, uint8_t sprite, int16_t x, int16_t y, uint8_t flip);
void     NEOGEO_USER slf_fighter_hide(uint8_t slot);
void     NEOGEO_USER slf_fighter_bank(uint8_t slot, uint8_t bank);
void     NEOGEO_USER slf_portraits_show(uint8_t win_base, uint8_t p1_cur, uint8_t p2_cur, uint8_t p1_done, uint8_t p2_done, uint16_t tick);
void     NEOGEO_USER slf_portraits_hide(void);
void     NEOGEO_USER slf_match_draw(const NGFMatch *m, const uint8_t *def_index);
void     NEOGEO_USER slf_hud_draw(const NGFMatch *m);
void     NEOGEO_USER slf_fx_spawn(uint8_t kind, int16_t screen_x, int16_t screen_y);
void     NEOGEO_USER slf_fx_reset(void);
void     NEOGEO_USER slf_music(uint8_t track);
void     NEOGEO_USER slf_sfx(uint8_t id);
void     NEOGEO_USER slf_title_draw(void);
void     NEOGEO_USER slf_title_tick(uint16_t tick);
uint16_t NEOGEO_USER slf_story_show(uint8_t number);
void     NEOGEO_USER slf_story_pan(uint16_t tick, uint8_t reverse);
void     NEOGEO_USER slf_large_portrait(uint8_t side, uint8_t def);

/* slf_game.c */
void     NEOGEO_USER slf_run_attract(void);
void     NEOGEO_USER slf_run(void);
uint8_t  NEOGEO_USER slf_start_pending(void);

#endif
