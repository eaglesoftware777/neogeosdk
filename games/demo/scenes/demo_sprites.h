#ifndef DEMO_SPRITES_H
#define DEMO_SPRITES_H

#include "sdk/macro.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

/* Scene 3 — sprite group showcase: sticky-bit chains, walk, parade */
void NEOGEO_USER demo_sprites_run(void);
void NEOGEO_USER demo_sprites_parade(void);
void NEOGEO_USER demo_sprites_walk(int loops, int delay_frames);


#ifdef __cplusplus
}
#endif
#endif /* DEMO_SPRITES_H */
