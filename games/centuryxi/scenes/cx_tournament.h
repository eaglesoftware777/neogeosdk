#ifndef CX_TOURNAMENT_H
#define CX_TOURNAMENT_H
#include "cx_sim.h"
#define CX_TOURNAMENT_VERSION 3      /* 3: scorers, the player's bookings and suspensions */
#define CX_NO_TEAM 255u
/* The championship is ng_soccer's NGSCup under the game's names: the layout (the save
 * format) is the one version 3 wrote. */
typedef NGSStanding CxStanding;
typedef NGSFixture CxFixture;
typedef NGSScorer CxScorer;
#define CX_SCORERS NGS_CUP_SCORERS
typedef NGSCup CxTournament;
/* A goal to a man's name. */
void CX_CODE cx_tournament_scorer(CxTournament *t, uint8_t team, uint8_t squad);
/* The leading scorers, n of them at most, most goals first: answers how many. */
uint8_t CX_CODE cx_tournament_top(const CxTournament *t, CxScorer *out, uint8_t n);
void CX_CODE cx_tournament_init(CxTournament *t, uint8_t human, uint32_t seed);
/* Returns next human fixture, or -1 after all 64 results are resolved. */
int16_t CX_CODE cx_tournament_next(CxTournament *t);
/* Reject duplicate/invalid results; winner is a team ID, or 255 for group draws. */
uint8_t CX_CODE cx_tournament_result(CxTournament *t, uint8_t index, uint8_t home_goals, uint8_t away_goals, uint8_t winner);
void CX_CODE cx_tournament_ranking(const CxTournament *t, uint8_t group, uint8_t out[4]);
#ifdef CX_RENDER_H
void NEOGEO_USER cx_road_store(const CxTournament *t);
void NEOGEO_USER cx_road_fetch(CxTournament *t);
void NEOGEO_USER cx_road_finish(uint8_t team, uint8_t won);
#endif
#endif
