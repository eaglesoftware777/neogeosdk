#ifndef DEMO_FIX_H
#define DEMO_FIX_H

#include "sdk/macro.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NEOGEO_USER
#define NEOGEO_USER
#endif

/* Scene 9 — FIX layer showcase */
void NEOGEO_USER demo_fix_run(void);

/* Legacy compat */
void NEOGEO_USER demo_fix_showcase(void);


#ifdef __cplusplus
}
#endif
#endif /* DEMO_FIX_H */
