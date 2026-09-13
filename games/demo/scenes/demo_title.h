#ifndef DEMO_TITLE_H
#define DEMO_TITLE_H

#include "sdk/macro.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

/* Title screen (shown in attract + after full flow start) */
void NEOGEO_USER demo_title_screen(void);

/* End card / credits (shown at end of full flow) */
void NEOGEO_USER demo_title_end_card(void);

/* Game over scene: red flash + score drain */
void NEOGEO_USER demo_title_game_over(void);

/* Attract reel: title card + teaser screenshots + coin detect */
void NEOGEO_USER demo_title_attract_reel(void);


#ifdef __cplusplus
}
#endif
#endif /* DEMO_TITLE_H */
