#ifndef DEMO_3D_H
#define DEMO_3D_H

#include "sdk/macro.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

void NEOGEO_USER demo_3d_showcase(void);
void NEOGEO_USER showPseudo3DLoop(void);
void NEOGEO_USER show3DRaycaster(void);


#ifdef __cplusplus
}
#endif
#endif
