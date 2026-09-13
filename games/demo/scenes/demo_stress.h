#ifndef DEMO_STRESS_H
#define DEMO_STRESS_H

#include "sdk/macro.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

/* Scene 11 — hardware stress test with performance HUD */
void NEOGEO_USER demo_stress_run(void);


#ifdef __cplusplus
}
#endif
#endif /* DEMO_STRESS_H */
