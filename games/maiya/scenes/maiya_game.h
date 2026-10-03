#ifndef MAIYA_GAME_H
#define MAIYA_GAME_H

#include "sdk/neogeo.h"

extern volatile uint8_t maiya_console_start;   /* Start pressed: see user.c */

void NEOGEO_USER maiya_boot(void);
void NEOGEO_USER maiya_frame(void);
/* Wait for the vertical blank and put the frame's colour changes on screen
 * in it: every frame of hers, the attract's and title's included. */
void NEOGEO_USER maiya_vblank(void);
void NEOGEO_USER maiya_title(void);
void NEOGEO_USER maiya_title_frame(void);
void NEOGEO_USER maiya_hero_select(void);
uint8_t NEOGEO_USER maiya_hero_choice(void);
void NEOGEO_USER maiya_eyecatcher(void);

/* Attract mode: the cabinet plays the game to itself between title cards. */
void NEOGEO_USER maiya_demo_begin(void);
void NEOGEO_USER maiya_demo_end(void);
uint8_t NEOGEO_USER maiya_demo_spent(void);

/* The saved data -- the score table and a few totals -- in the header's
 * save block. maiya_save_check() keeps what is there if it is Maiya's and
 * whole, and starts it afresh otherwise; maiya_save_reset() always starts
 * it afresh (the system's first-power-on request). */
void NEOGEO_USER maiya_save_check(void);
void NEOGEO_USER maiya_save_reset(void);

/* 1 when a Start (or, on an arcade board, a credit) is waiting: the
 * attract gives way to the title. Defined in user.c. */
uint8_t NEOGEO_USER maiya_start_pending(void);

/* The operator's DEMO SOUND setting: 1 = the attract plays with sound. */
uint8_t NEOGEO_USER maiya_dip_demo_sound(void);

#endif
