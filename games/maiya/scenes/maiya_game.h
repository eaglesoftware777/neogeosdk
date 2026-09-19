#ifndef MAIYA_GAME_H
#define MAIYA_GAME_H

#include "sdk/neogeo.h"

void NEOGEO_USER maiya_boot(void);
void NEOGEO_USER maiya_frame(void);
void NEOGEO_USER maiya_title(void);

/* Attract mode: the cabinet plays the game to itself between title cards. */
void NEOGEO_USER maiya_demo_begin(void);
void NEOGEO_USER maiya_demo_end(void);
uint8_t NEOGEO_USER maiya_demo_spent(void);

#endif
