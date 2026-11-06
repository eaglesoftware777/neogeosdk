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
#define SW_DEMO_TURN     0x1005FFu      /* its last byte, past the checked data: which demonstration
                                           is next (work RAM is cleared on every attract) */

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
#define SW_TXT_CLUE   5u     /* picture clues: plum ink on the cream plate */
#define SW_TXT_CLUE_HI 6u    /* the row and column under the paw */
#define SW_TXT_CLUE_DONE 7u  /* a finished line */
#define SW_TXT_HEART  8u     /* the hearts (FIX tiles SW_HEART_TILE, + 1 empty) */
#define SW_TXT_STAR   9u     /* a star won (FIX tile SW_STAR_TILE) */
#define SW_TXT_STAR_OFF 10u  /* a star still to win */
#define SW_HEART_TILE 0xE00u
#define SW_STAR_TILE  0xE02u

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
#define SW_PER_CHAPTER   5u          /* four of matching and bubbles, then the picture */
#define SW_PICTURE_STAGE 4u
#define SW_LEVELS        (SW_CHAPTERS * SW_PER_CHAPTER)
typedef struct {
    const char *name;        /* upper case, at most 22 characters */
    const char *line1, *line2;   /* the chapter card's two lines */
    uint8_t cat;             /* the guide for this wonder */
} SWChapter;
extern const SWChapter sw_chapters[SW_CHAPTERS];
extern const NGPuzzleLevel sw_levels[SW_LEVELS];
extern const char *const sw_level_tips[SW_LEVELS];   /* one line each, shown before the level */
typedef struct {
    const char *name;        /* the finished picture's title ("" for other levels) */
    uint8_t bg;              /* the picture colour its empty cells take when done */
} SWPicture;
extern const SWPicture sw_pictures[SW_LEVELS];
extern const NGPuzzleLevel sw_demo_level;
extern const NGPuzzleLevel sw_demo_logic;    /* the attract's picture */
extern const SWPicture sw_demo_picture;

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
void     NEOGEO_USER sw_cats_small(uint8_t small);   /* 1: sw_cat draws the 48-pixel cats until the next scene */
/* The title's big cats (96 x 96): frame 0 happy, 1 blinking, 2 cheering. */
void     NEOGEO_USER sw_title_cat(uint8_t slot, uint8_t cat, uint8_t frame, int16_t x, int16_t y);
/* The lucky cat (96 x 96, frame 0 paw up, 1 paw down) in cat slot 0. */
void     NEOGEO_USER sw_lucky_cat(uint8_t frame, int16_t x, int16_t y);
/* The lucky paw's prize icons (32 x 32): slot 0-3 a prize, slot 4 the selector; SW_PRIZE_* kinds. */
enum { SW_PRIZE_COIN = 0, SW_PRIZE_COINS, SW_PRIZE_BAG, SW_PRIZE_PAW, SW_PRIZE_SELECT, SW_PRIZE_KINDS };
void     NEOGEO_USER sw_prize(uint8_t slot, uint8_t kind, int16_t x, int16_t y);
/* The album: picture i (0-6) as a 48 x 48 polaroid, its picture or a question mark. */
void     NEOGEO_USER sw_album(uint8_t i, uint8_t solved, int16_t x, int16_t y);
/* A cheering word popping up, centred at (cx, cy); it rises and goes by itself. */
enum { SW_WORD_NICE = 0, SW_WORD_GREAT, SW_WORD_LOVELY, SW_WORD_AMAZING, SW_WORD_DONE, SW_WORD_LUCKY };
void     NEOGEO_USER sw_word(uint8_t word, int16_t cx, int16_t cy);
void     NEOGEO_USER sw_stars(uint8_t count, int16_t x, int16_t y);
void     NEOGEO_USER sw_card(uint8_t show);          /* the dark plate behind a card's text */
void     NEOGEO_USER sw_banner(uint8_t which, int16_t y);   /* a strip plate (0 or 1) behind a few lines */
void     NEOGEO_USER sw_panel(uint8_t show);         /* the plate behind the play panel (2: the narrow one) */

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
/* Picture logic: the clues (the cursor's row and column highlighted), the
 * hearts, the paw on cell `cursor`, and the finished picture's reveal:
 * sw_logic_reveal(p, bg, step) shows every cell whose x + y < step in its
 * picture colour (bg for the empty ones). */
void     NEOGEO_USER sw_logic_clues(NGPuzzle *p, uint8_t cursor);
void     NEOGEO_USER sw_logic_hearts(uint16_t left, uint16_t total);
void     NEOGEO_USER sw_logic_cursor(uint8_t show, uint8_t cursor);
void     NEOGEO_USER sw_logic_reveal(NGPuzzle *p, uint8_t bg, uint8_t step);

/* Sound. */
void     NEOGEO_USER sw_music(uint8_t track);
void     NEOGEO_USER sw_sfx(uint8_t sfx);
void     NEOGEO_USER sw_hush(void);

/* ---- sw_game.c ------------------------------------------------------------------ */
void     NEOGEO_USER sw_title_screen(void);
void     NEOGEO_USER sw_run_attract(void);
void     NEOGEO_USER sw_run(void);

#endif
