/*
 * mj.h -- Mahjong Demo: the SDK's Mahjong engine (sdk/2d_engine/ng_mahjong.c)
 * on real hardware: four-seat Mahjong under its Hong Kong Old Style rules,
 * one player against three CPU seats, and Mahjong solitaire on the turtle
 * layout. A technical demonstration: the art is plain test art.
 *
 * mj_game.c runs the menu, attract (four CPU seats playing) and how to
 * play; mj_table.c the four-seat game; mj_sol.c solitaire; mj_draw.c draws
 * tiles, text and the table and plays the sounds.
 */
#ifndef MJ_H
#define MJ_H
#include <stdint.h>
#include "sdk/neogeo.h"
#include "sdk/ng_system.h"
#include "sdk/2d_engine/ng_mahjong.h"

#define MJ_START_FLAG   0x100700u

#define MJ_UP      0x0001u
#define MJ_DOWN    0x0002u
#define MJ_LEFT    0x0004u
#define MJ_RIGHT   0x0008u
#define MJ_A       0x0010u
#define MJ_B       0x0020u
#define MJ_C       0x0040u
#define MJ_D       0x0080u
#define MJ_START   0x0100u
#define MJ_BUTTONS (MJ_A | MJ_B | MJ_C | MJ_D)

enum { MJ_TXT_WHITE = 0, MJ_TXT_GOLD, MJ_TXT_RED, MJ_TXT_SKY, MJ_TXT_MINT, MJ_TXT_GREY };

/* the tile sheet's frames: a kind (0-41), then the back */
#define MJ_TILE_BACK  42u
/* how a tile is drawn: its palette (mj_draw.c derives them from the sheet's) */
enum { MJ_LOOK_NORMAL = 0, MJ_LOOK_BLOCKED, MJ_LOOK_SELECTED, MJ_LOOK_HINT, MJ_LOOKS };

/* effects: ADPCM-A samples */
enum {
    MJ_SFX_COIN = 0, MJ_SFX_START, MJ_SFX_MOVE, MJ_SFX_DISCARD, MJ_SFX_DRAW, MJ_SFX_CLAIM, MJ_SFX_KONG,
    MJ_SFX_WIN, MJ_SFX_EXHAUSTED, MJ_SFX_NOPE, MJ_SFX_PAIR, MJ_SFX_SHUFFLE, MJ_SFX_HINT, MJ_SFX_CLEARED,
    MJ_SFX_UNDO, MJ_SFX_SELECT
};
/* music: ADPCM-B tracks (file n + 1; from MJ_MUS_RESULT on n + 2: 5.wav is the eye-catcher's) */
enum { MJ_MUS_MENU = 0, MJ_MUS_TABLE, MJ_MUS_SOLITAIRE, MJ_MUS_QUIET, MJ_MUS_RESULT };

/* mj_draw.c ------------------------------------------------------------------ */
void     NEOGEO_USER mj_vblank(void);
void     NEOGEO_USER mj_presentation_init(void);
void     NEOGEO_USER mj_fix_palettes(void);
uint16_t NEOGEO_USER mj_frame(void);
uint16_t NEOGEO_USER mj_held(void);
uint16_t NEOGEO_USER mj_pressed(void);
uint16_t NEOGEO_USER mj_repeat(void);      /* presses, and held directions repeating */
uint8_t  NEOGEO_USER mj_wait_button(uint16_t frames);
void     NEOGEO_USER mj_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal);
void     NEOGEO_USER mj_puts_c(uint8_t y, const char *text, uint8_t pal);
void     NEOGEO_USER mj_putn(uint8_t x, uint8_t y, int32_t value, uint8_t digits, uint8_t pal);
void     NEOGEO_USER mj_clear_text(uint8_t y0, uint8_t y1);
void     NEOGEO_USER mj_line_c(uint8_t y, const char *text, uint8_t pal);
void     NEOGEO_USER mj_clear_from(uint8_t x, uint8_t y);
void     NEOGEO_USER mj_mini(uint8_t x, uint8_t y, uint8_t kind);   /* a two-cell tile; NGM_NO_TILE: blank */
void     NEOGEO_USER mj_mini_back(uint8_t x, uint8_t y);
const char *NEOGEO_USER mj_tile_name(uint8_t kind);                  /* "5 DOTS", "RED DRAGON" ... */
void     NEOGEO_USER mj_screen(uint8_t logo);                        /* the felt, dark; mj_reveal() shows it */
void     NEOGEO_USER mj_reveal(void);
void     NEOGEO_USER mj_fade_out(void);
/* sprite tiles: slot 0..MJ_SPRITE_TILES-1 */
#define MJ_SPRITE_TILES 160u
void     NEOGEO_USER mj_tile(uint8_t slot, uint8_t kind, int16_t x, int16_t y, uint8_t look);
void     NEOGEO_USER mj_tile_hide(uint8_t slot);
void     NEOGEO_USER mj_tiles_hide(uint8_t from, uint8_t to);
void     NEOGEO_USER mj_music(uint8_t track);
void     NEOGEO_USER mj_sfx(uint8_t sfx);

/* mj_table.c ----------------------------------------------------------------- */
/* One four-seat game; demo: every seat a CPU (attract). 1 if it ran to the end. */
uint8_t  NEOGEO_USER mj_table_run(uint8_t demo);

/* mj_sol.c ------------------------------------------------------------------- */
void     NEOGEO_USER mj_sol_run(void);

/* mj_game.c ------------------------------------------------------------------ */
void     NEOGEO_USER mj_title_screen(void);
void     NEOGEO_USER mj_run_attract(void);
void     NEOGEO_USER mj_run(void);
uint8_t  NEOGEO_USER mj_dip(uint8_t n);
uint8_t  NEOGEO_USER mj_quit_requested(void);

/* user.c ---------------------------------------------------------------------- */
uint8_t  NEOGEO_USER mj_start_pending(void);
void     NEOGEO_USER mj_auto_start(void);
uint8_t  NEOGEO_USER mj_spend_credit(void);

#endif
