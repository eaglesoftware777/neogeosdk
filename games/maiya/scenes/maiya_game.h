#ifndef MAIYA_GAME_H
#define MAIYA_GAME_H

#include "sdk/neogeo.h"

#ifdef NG_AES
extern volatile uint8_t maiya_console_start;
#endif

void NEOGEO_USER maiya_boot(void);
void NEOGEO_USER maiya_frame(void);
void NEOGEO_USER maiya_title(void);
void NEOGEO_USER maiya_title_frame(void);
void NEOGEO_USER maiya_hero_select(void);
uint8_t NEOGEO_USER maiya_hero_choice(void);
void NEOGEO_USER maiya_eyecatcher(void);

/* Attract mode: the cabinet plays the game to itself between title cards. */
void NEOGEO_USER maiya_demo_begin(void);
void NEOGEO_USER maiya_demo_end(void);
uint8_t NEOGEO_USER maiya_demo_spent(void);

#endif
