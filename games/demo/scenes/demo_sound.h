#ifndef DEMO_SOUND_H
#define DEMO_SOUND_H

#include "sdk/macro.h"

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

/* Scene 10 — YM2610 audio engine showcase */
void NEOGEO_USER demo_sound_run(void);

/* Legacy compatibility entry point */
void NEOGEO_USER showSoundDemo(void);

#endif /* DEMO_SOUND_H */
