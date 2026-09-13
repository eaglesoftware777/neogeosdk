/*
 * demo_plus_main.h — entry point for the C++-engine-only showcase.
 */

#ifndef DEMO_PLUS_MAIN_H
#define DEMO_PLUS_MAIN_H

#include "sdk/macro.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * demo_plus_main_run — runs the demo_plus showcase to completion.
 *
 * Exercises only public APIs from sdk/2d_engine_plus.  When the game is
 * built with USE_2D_PLUS=1 (the only supported build for demo_plus), the
 * C linker resolves every ng_* call against the C++ engine implementation.
 */
void NEOGEO_USER demo_plus_main_run(void);

#ifdef __cplusplus
}
#endif

#endif /* DEMO_PLUS_MAIN_H */
