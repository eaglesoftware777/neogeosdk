/*
 * Seven Wonders Puzzles: what the game's files share.
 *
 * sw_draw.c  the presentation: frame pump, palettes, backgrounds, the board
 *            (match pieces or bubbles) and its animations, cats, the FIX
 *            panel, sound.
 * sw_game.c  the screens: title, attract demo, chapter map, a level from
 *            start to its result, continue, game over, ending.
 * sw_levels.c the seven chapters' levels (ng_puzzle level tables).
 *
 * The rules themselves are the SDK's puzzle engine (ng_puzzle.h).
 */
#ifndef SW_H
#define SW_H

#include <stdint.h>
#include "sdk/neogeo.h"
#include "sdk/2d_engine/ng_puzzle.h"
#include "sw_limits.h"

#define SW_START_FLAG    0x100700u      /* set by PLAYER_START (user.c) */
#define SW_SAVE_BASE     0x100400u      /* the cartridge save block (512 bytes) */

/* ---- input --------------------------------------------------------------- */
#define SW_UP     0x0001u
#define SW_DOWN   0x0002u
#define SW_LEFT   0x0004u
#define SW_RIGHT  0x0008u
#define SW_A      0x0010u
#define SW_B      0x0020u
#define SW_C      0x0040u
#define SW_D      0x0080u
#define SW_START  0x0100u
#define SW_BUTTONS (SW_A | SW_B | SW_C | SW_D)

/* ---- FIX text palettes ----------------------------------------------------- */
#define SW_TXT_WHITE  0u
#define SW_TXT_GOLD   1u
#define SW_TXT_PINK   2u
#define SW_TXT_SKY    3u
#define SW_TXT_MINT   4u

/* ---- sound effects (ADPCM-A: sound/samples/in_wav_a/<n + 1>.wav) ------------ */
enum {
    SW_SFX_COIN = 0, SW_SFX_MOVE, SW_SFX_START, SW_SFX_PICK, SW_SFX_SWAP, SW_SFX_NOPE,
    SW_SFX_MATCH, SW_SFX_CASCADE, SW_SFX_SPECIAL, SW_SFX_BLAST, SW_SFX_CRATE, SW_SFX_ICE,
    SW_SFX_FIRE, SW_SFX_BOUNCE, SW_SFX_POP, SW_SFX_DROP, SW_SFX_WIN, SW_SFX_LOSE,
    SW_SFX_MEOW, SW_SFX_MEW, SW_SFX_SHUFFLE, SW_SFX_TICK, SW_SFX_STAR, SW_SFX_RESCUE,
    SW_SFX_COUNT
};
/* ---- music (ADPCM-B: sound/samples/in_wav_b/<n + 1>.wav) ------------------- */
enum {
    SW_MUS_TITLE = 0, SW_MUS_MATCH, SW_MUS_BUBBLE, SW_MUS_MAP, SW_MUS_RESERVED, SW_MUS_RESULT,
    SW_MUS_OVER, SW_MUS_FINALE, SW_MUS_COUNT
};

/* ---- cats ------------------------------------------------------------------ */
enum { SW_CAT_MIMI = 0, SW_CAT_KIKO };
enum { SW_MOOD_IDLE = 0, SW_MOOD_BLINK, SW_MOOD_HAPPY, SW_MOOD_WOW, SW_MOOD_SAD, SW_MOOD_CHEER, SW_MOODS };

/* ---- levels ---------------------------------------------------------------- */
#define SW_CHAPTERS      7u
#define SW_PER_CHAPTER   4u
#define SW_LEVELS        (SW_CHAPTERS * SW_PER_CHAPTER)
typedef struct {
    const char *name;        /* upper case, at most 22 characters */
    const char *line1, *line2;   /* the chapter card's two lines */
    uint8_t cat;             /* the guide for this wonder */
} SWChapter;
extern const SWChapter sw_chapters[SW_CHAPTERS];
extern const NGPuzzleLevel sw_levels[SW_LEVELS];
extern const char *const sw_level_tips[SW_LEVELS];   /* one line each, shown before the level */
extern const NGPuzzleLevel sw_demo_level;

/* ---- user.c ------------------------------------------------------------------ */
uint8_t NEOGEO_USER sw_start_pending(void);
void    NEOGEO_USER sw_auto_start(void);
uint8_t NEOGEO_USER sw_spend_credit(void);

/* ---- sw_draw.c ---------------------------------------------------------------- */
void     NEOGEO_USER sw_vblank(void);
void     NEOGEO_USER sw_presentation_init(void);
void     NEOGEO_USER sw_fix_palettes(void);
uint16_t NEOGEO_USER sw_frame(void);
uint16_t NEOGEO_USER sw_held(void);
uint16_t NEOGEO_USER sw_pressed(void);
uint16_t NEOGEO_USER sw_repeat(void);         /* pressed, then repeating while held */
uint16_t NEOGEO_USER sw_rand(void);
void     NEOGEO_USER sw_scene(uint8_t background);   /* fade out, clear, show a background, fade in */
void     NEOGEO_USER sw_fade_out(void);
void     NEOGEO_USER sw_wait(uint16_t frames);   /* frames, cut short by a start (returns early) */
void     NEOGEO_USER sw_puts(uint8_t x, uint8_t y, const char *text, uint8_t pal);
void     NEOGEO_USER sw_puts_c(uint8_t y, const char *text, uint8_t pal);   /* centred */
void     NEOGEO_USER sw_putn(uint8_t x, uint8_t y, uint32_t value, uint8_t digits, uint8_t pal);
void     NEOGEO_USER sw_clear_text(uint8_t y0, uint8_t y1);
void     NEOGEO_USER sw_logo(uint8_t show, int16_t y);
void     NEOGEO_USER sw_cat(uint8_t slot, uint8_t cat, uint8_t mood, int16_t x, int16_t y);
void     NEOGEO_USER sw_cat_hide(uint8_t slot);
void     NEOGEO_USER sw_stars(uint8_t count, int16_t x, int16_t y);
void     NEOGEO_USER sw_card(uint8_t show);          /* the dark plate behind a card's text */
void     NEOGEO_USER sw_banner(uint8_t which, int16_t y);   /* a strip plate (0 or 1) behind a few lines */
void     NEOGEO_USER sw_panel(uint8_t show);         /* the plate behind the play panel */

#define SW_BG_TITLE   7u
#define SW_BG_MAP     8u

/* The board: bind a context, then each frame give it the engine's events
 * and draw; ng_puzzle_update() only when sw_board_idle() says the last
 * phase has been shown. */
uint8_t  NEOGEO_USER sw_board_bind(NGPuzzle *p); /* 0: invalid renderer dimensions */
void     NEOGEO_USER sw_board_event(NGPuzzle *p, const NGPuzzleEvent *e);
void     NEOGEO_USER sw_board_draw(NGPuzzle *p);
uint8_t  NEOGEO_USER sw_board_idle(void);
void     NEOGEO_USER sw_board_hide(void);
void     NEOGEO_USER sw_cursor(uint8_t show, uint8_t cell, uint8_t picked);
void     NEOGEO_USER sw_aim(NGPuzzle *p, uint8_t show, uint8_t guide);
int16_t  NEOGEO_USER sw_board_x(void);
int16_t  NEOGEO_USER sw_board_y(void);

/* Sound. */
void     NEOGEO_USER sw_music(uint8_t track);
void     NEOGEO_USER sw_sfx(uint8_t sfx);

/* ---- sw_game.c ------------------------------------------------------------------ */
void     NEOGEO_USER sw_title_screen(void);
void     NEOGEO_USER sw_run_attract(void);
void     NEOGEO_USER sw_run(void);

#endif
