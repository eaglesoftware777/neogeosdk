/* The championship: ng_soccer's cup with the 48 nations, their strengths and the
 * game's quick results. */
#include "cx_tournament.h"

void CX_CODE cx_tournament_init(CxTournament *t, uint8_t human, uint32_t seed)
{
    ngs_cup_init(t, human < CX_SQUADS ? human : 0, seed ? seed : 787u, CX_SQUADS, cx_squad_power);
}
void CX_CODE cx_tournament_ranking(const CxTournament *t, uint8_t group, uint8_t out[4]) { ngs_cup_ranking(t, group, out); }
void CX_CODE cx_tournament_scorer(CxTournament *t, uint8_t team, uint8_t squad) { ngs_cup_scorer(t, team, squad); }
uint8_t CX_CODE cx_tournament_top(const CxTournament *t, CxScorer *out, uint8_t n) { return ngs_cup_top(t, out, n); }
uint8_t CX_CODE cx_tournament_result(CxTournament *t, uint8_t n, uint8_t a, uint8_t b, uint8_t winner)
{
    return ngs_cup_result(t, n, a, b, winner);
}
/* Independent CPU fixtures get recorded scores from the sides' strengths: no finalist shortcuts. */
int16_t CX_CODE cx_tournament_next(CxTournament *t) { return ngs_cup_next(t, cx_quick_result); }
