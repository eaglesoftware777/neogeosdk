#ifndef DEMO_INTRO_H
#define DEMO_INTRO_H

#include "sdk/macro.h"

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

/* Scene 1 — Eagle Software cinematic intro */
void NEOGEO_USER demo_intro_eagle(void);

/* Scene 2 — NEO GEO SDK title card */
void NEOGEO_USER demo_intro_sdk_title(void);

/* Scene 0.2 — System banner (MVS/AES, ROM ID, build date) */
void NEOGEO_USER demo_intro_system_banner(void);

/* Scene 7.0 — Loading scene with progress bar */
void NEOGEO_USER demo_intro_loading(void);

#endif /* DEMO_INTRO_H */
