/* What the presentation modules share. */
#ifndef CX_RENDER_H
#define CX_RENDER_H

#include <stdint.h>
#include "sdk/macro.h"
#include "sdk/neogeo.h"
#include "sdk/ng_system.h"
#include "sdk/2d_engine/ng_fix.h"
#include "sdk/2d_engine/ng_sprite_group.h"
#include "sdk/2d_engine/ng_render_queue.h"
#include "games/centuryxi/artbox/generated/cx_ids.h"
#include "games/centuryxi/artbox/generated/cx_cast.h"
#include "cx_sim.h"

#define CX_PAL_WHITE  0u
#define CX_PAL_GOLD   1u
#define CX_PAL_CYAN   2u
#define CX_PAL_GREEN  3u
#define CX_PAL_RED    4u

/* Hardware sprites, lower behind. Men have 4 strips each and keep the depth
 * order; everything in front of them comes after. */
#define SPR_BG        1u             /* two rings of 21: the stands, then the pitch */
#define RING          21u
#define SPR_SHADOW    43u
#define SPR_MEN       45u            /* 23 slots of 5: 22 men and the referee, four strips and a shirt number */
#define SPR_BALL      160u
#define SPR_GOAL      161u           /* up to 8 strips, one goal at a time */
#define SPR_CURSOR    169u
#define SPR_CARD      171u
#define SPR_BANNER    172u
#define SPR_PLATE     182u
#define SPR_PORTRAIT  202u
#define SPR_TROPHY    206u
#define SPR_LOGO      210u
#define SPR_FLAG      226u           /* two flags of 2 strips */
#define SPR_HERO      230u           /* a big figure, 6 strips */
#define SPR_HERO2     236u
#define SPR_INSET     242u           /* the coach's television window, 5 strips */
#define SPR_COACH     247u
#define SPR_LINES     251u           /* the linesman */
#define SPR_BIGCARD   255u
#define SPR_BALLZ     257u           /* the ball flying away from the camera, 2 strips */
#define SPR_WORD      259u           /* GOAL! GOAL! GOAL!: three words of 16 strips */

/* Palette banks: 32-53 the men, one each; 54 the referee. */
#define BANK_MEN      32u
#define BANK_REF      54u
#define BANK_HERO     55u

#define BANNER_GOAL   0u
#define BANNER_HALF   1u
#define BANNER_CHAMP  2u
#define BANNER_FULL   3u
#define BANNER_PENS   4u
#define BANNER_REPLAY 5u

typedef struct {
    NGSpriteGroup g;
    uint16_t art, tile;
    int16_t x, y;
    uint8_t flip, vis, bound, bank;
} CxSpr;

/* Frame-time probe, QA builds only: the raster line at each mark of a frame. */
#ifdef CX_PERF
extern volatile uint16_t cx_perf[10];
extern volatile uint8_t cx_test_gag;
#define CX_PERF_MARK(n) (cx_perf[n] = (uint16_t)(*(volatile uint16_t *)0x3C0006 >> 7))
#else
#define CX_PERF_MARK(n) ((void)0)
#endif

/* The game's options; saved. */
typedef struct {
    uint8_t classic;              /* 1: three-button controls */
    uint8_t rules;                /* CX_RULES_* */
    uint8_t var;                  /* CX_VAR_* */
    uint8_t level;                /* 0..7: levels 1..8 (saves before the eight levels held 0..2) */
    uint8_t length;               /* an index into k_minutes: the match in real minutes */
    uint8_t extra;                /* CX_ET_ */
    uint8_t levels8;              /* CX_LEVELS8 once level counts eight levels */
} CxOptions;
#define CX_LEVELS8 0xA8u
extern CxOptions cx_options;
extern uint8_t cx_match_round;    /* cup rounds the player has already played: set before cx_match */
extern uint8_t cx_match_on;       /* a match under way: who plays it (bit 0 P1, bit 1 P2), or CX_MATCH_NO_JOIN */
#define CX_MATCH_NO_JOIN 0x80u    /* a match on that nobody may join */

/* cx_render.c */
extern CxSpr cx_men[CX_MEN], cx_ref, cx_ball_s, cx_shadow, cx_cursor[2], cx_banner, cx_portrait, cx_trophy, cx_plate, cx_logo,
             cx_goal, cx_card, cx_flag[2], cx_hero, cx_hero2, cx_inset, cx_coach, cx_lines, cx_bigcard, cx_ballz, cx_word[3];
extern int16_t cx_camx, cx_camy;
void NEOGEO_USER cx_frame_wait(void);
void NEOGEO_USER cx_init_video(void);
void NEOGEO_USER cx_screen_clear(void);
void NEOGEO_USER cx_picture_show(uint16_t art);
void NEOGEO_USER cx_spr_hide(CxSpr *s);
void NEOGEO_USER cx_spr_show(CxSpr *s, uint16_t slot, uint16_t art, uint8_t frame, int16_t x, int16_t y, uint8_t flip);
void NEOGEO_USER cx_spr_show_bank(CxSpr *s, uint16_t slot, uint16_t art, uint8_t frame, int16_t x, int16_t y, uint8_t flip, uint8_t bank);
void NEOGEO_USER cx_spr_show_zoom(CxSpr *s, uint16_t slot, uint16_t art, uint8_t frame, int16_t x, int16_t y, uint8_t scale);
void NEOGEO_USER cx_figure_show(CxSpr *s, uint16_t slot, uint16_t art, uint8_t frame, int16_t x, int16_t y, uint8_t flip, uint8_t bank);
void NEOGEO_USER cx_man_palette(uint8_t bank, uint8_t nation, uint8_t kit, uint8_t skin, uint8_t hair, uint8_t keeper);
void NEOGEO_USER cx_ref_palette(uint8_t bank);
void NEOGEO_USER cx_project(int16_t ux, int16_t uy, int16_t *px, int16_t *py, uint16_t *s8);
void NEOGEO_USER cx_pitch_load(uint8_t stadium);
void NEOGEO_USER cx_crowd_kits(uint8_t home, uint8_t away);
void NEOGEO_USER cx_pitch_draw(int16_t camx, int16_t camy);
void NEOGEO_USER cx_camera(const CxWorld *w, uint8_t snap);
void NEOGEO_USER cx_side_palettes(const CxWorld *w);
void NEOGEO_USER cx_men_reset(void);
void NEOGEO_USER cx_men_draw(const CxWorld *w, uint8_t record);
void NEOGEO_USER cx_ball_draw(const CxWorld *w);
void NEOGEO_USER cx_ball_at(int16_t bx, int16_t by, int16_t h, uint8_t spin);
void NEOGEO_USER cx_ref_update(const CxWorld *w);
void NEOGEO_USER cx_goals_draw(uint8_t bulge);
void NEOGEO_USER cx_cursors_draw(const CxWorld *w);
void NEOGEO_USER cx_replay(void);
void NEOGEO_USER cx_replay_reset(void);
uint8_t NEOGEO_USER cx_replay_ready(void);
void NEOGEO_USER cx_rep_mark(void);
void NEOGEO_USER cx_hero_show(CxSpr *s, uint16_t slot, uint16_t art, uint8_t frame, int16_t x, int16_t y, uint8_t flip, uint8_t bank);
uint8_t NEOGEO_USER cx_man_art(uint8_t man);
void NEOGEO_USER cx_load_art_palette(uint16_t art);
void NEOGEO_USER cx_flag_show(uint8_t which, uint8_t nation, int16_t x, int16_t y);
void NEOGEO_USER cx_puts_center(uint8_t y, const char *s, uint8_t pal);
void NEOGEO_USER cx_clear_row(uint8_t y);
void NEOGEO_USER cx_put_2d(uint8_t x, uint8_t y, uint16_t value, uint8_t pal);
uint16_t NEOGEO_USER cx_input(uint16_t *pressed);
uint16_t NEOGEO_USER cx_input2(uint16_t *pressed);
void NEOGEO_USER cx_wait(uint16_t n);
uint16_t NEOGEO_USER cx_any_press(uint16_t frames);

/* cx_audio.c */
void NEOGEO_USER cx_sound_events(const CxWorld *w);
void NEOGEO_USER cx_crowd_level(const CxWorld *w);
void NEOGEO_USER cx_crowd_step(uint8_t excited);
void NEOGEO_USER cx_sfx_menu(void);
void NEOGEO_USER cx_sfx_kick(void);
void NEOGEO_USER cx_sfx_goal(void);
void NEOGEO_USER cx_sfx_save(void);
void NEOGEO_USER cx_sfx_whistle(void);
void NEOGEO_USER cx_say_penalty(void);
void NEOGEO_USER cx_say_card(uint8_t red);
void NEOGEO_USER cx_say_man_later(const CxWorld *w, uint8_t man, uint8_t delay);
void NEOGEO_USER cx_voice_tick(void);
void NEOGEO_USER cx_chant(uint8_t nation);
void NEOGEO_USER cx_terraces(const CxWorld *w);
void NEOGEO_USER cx_say_review(void);
void NEOGEO_USER cx_music_title(void);
void NEOGEO_USER cx_music_menu(void);
void NEOGEO_USER cx_music_match(void);
void NEOGEO_USER cx_music_penalty(void);
void NEOGEO_USER cx_music_final(void);
void NEOGEO_USER cx_music_champion(void);

/* cx_var.c */
void NEOGEO_USER cx_var_show(const CxWorld *w);

/* cx_referee.c: the officials, the bench and the talk */
void NEOGEO_USER cx_talk_reset(void);
void NEOGEO_USER cx_ref_intro(const CxWorld *w);
void NEOGEO_USER cx_talk_events(const CxWorld *w);
void NEOGEO_USER cx_talk_step(void);
void NEOGEO_USER cx_coach_events(const CxWorld *w);
void NEOGEO_USER cx_coach_step(uint16_t frame, uint8_t hold);
void NEOGEO_USER cx_coach_pop(uint8_t frame, uint8_t frames);
void NEOGEO_USER cx_linesman_events(const CxWorld *w);
void NEOGEO_USER cx_linesman_step(uint16_t frame);
void NEOGEO_USER cx_card_scene(const CxWorld *w);
void NEOGEO_USER cx_foul_scene(const CxWorld *w);

/* cx_show.c */
/* A match: humans bit 0 the first side, bit 1 the second. stage: 0 friendly, 1 group,
 * 2 a tie that must be settled, 3 the final. Answers 1 first side won, 0 lost, 2 quit, 3 drew. */
uint8_t NEOGEO_USER cx_match(uint8_t team, uint8_t foe, uint8_t stadium, uint8_t stage, uint8_t humans);
uint8_t NEOGEO_USER cx_match_goal(uint8_t k, uint8_t *team, uint8_t *squad);
uint8_t NEOGEO_USER cx_match_card(uint8_t squad);
uint8_t NEOGEO_USER cx_penalties(uint8_t team, uint8_t foe, uint8_t humans);
void NEOGEO_USER cx_training(uint8_t team);
extern uint8_t cx_result[2];

/* cx_menu.c */
void NEOGEO_USER cx_attract_init(void);
void NEOGEO_USER cx_attract_step(uint16_t tick);
void NEOGEO_USER cx_run(void);

/* user.c */
uint8_t NEOGEO_USER cx_pay_player2(void);   /* 1 once he has paid (always on a console) */
uint8_t NEOGEO_USER cx_player2_can_pay(void);   /* his credit is there (always on a console) */

/* cx_save.c */
void NEOGEO_USER cx_save_load(void);
void NEOGEO_USER cx_save_write(void);
void NEOGEO_USER cx_save_erase(void);
uint8_t NEOGEO_USER cx_road_saved(void);

#endif
