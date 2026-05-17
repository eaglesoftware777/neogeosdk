/*
 * ng_demo_advanced.h — Prototypes for Stage 12 advanced demos.
 */

#ifndef NG_DEMO_ADVANCED_HPP
#define NG_DEMO_ADVANCED_HPP

#include "ng_defs.hpp"

#ifdef __cplusplus
extern "C" {
#endif


void NEOGEO_USER demo_sticky_character(void);
void NEOGEO_USER demo_smooth_camera(void);
void NEOGEO_USER demo_palette_flash_cycle(void);
void NEOGEO_USER demo_hitstop_and_shake(void);
void NEOGEO_USER demo_particle_stress(void);
void NEOGEO_USER demo_depth_starfield(void);
void NEOGEO_USER demo_boss_depth_shrink(void);

#ifdef NG_DEBUG_PERF
void NEOGEO_USER demo_perf_hud(void);
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif