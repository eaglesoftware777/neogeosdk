/*
 * demo_unified.h — public entry for the unified showcase demo.
 *
 * One linear playthrough covering every public capability of the
 * sdk/2d_engine_plus engine. See games/demo/DEMO_PLAN.md.
 */

#ifndef DEMO_UNIFIED_H
#define DEMO_UNIFIED_H

#include <stdint.h>
#include "sdk/macro.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * demo_unified_run — entry called from demo_run_full_flow().
 *
 * Runs every chapter in order. Each chapter ends on A-press or its
 * own time-out. Returns when the credits chapter completes.
 */
void NEOGEO_USER demo_unified_run(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DEMO_UNIFIED_H */
