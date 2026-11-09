/* Not a pass/fail test: the shape of CPU football, averaged. cc ... tests/stats.c */
#include <stdio.h>
#include "cx_sim.h"
int main(void)
{
    int a, n = 0;
    long goals = 0, shots = 0, saves = 0, fouls = 0, cards = 0, reds = 0, offs = 0, pens = 0, feints = 0, kicks = 0, draws = 0, outs = 0, frames = 0, fk = 0;
    long claims = 0, punches = 0, corners = 0, corner_goals = 0, throws = 0;
    long g0 = 0, g1 = 0, p1 = 0, sh0 = 0, sh1 = 0;
    long strong_w = 0, strong_n = 0, strong_gd = 0, bad_touch = 0;
    long passes = 0, kept = 0, crowd = 0, samples = 0, near1 = 0, chains = 0, chain_len = 0;
#ifndef STATS_REPEAT
#define STATS_REPEAT 1
#endif
    int rep;
    for (rep = 0; rep < STATS_REPEAT; rep++)
    for (a = 0; a < 32; a++) {
        int b = (a * 7 + 5 + rep * 3) % 32, i, pending = -1, chain = 0, holder = -1, ck_t = 0;
        CxWorld w;
        if (a == b) continue;
        cx_init(&w, (uint8_t)a, (uint8_t)b, 0, 0);
        w.rng ^= (uint32_t)rep * 2654435761u;
#ifdef STATS_LEVEL
        w.level = STATS_LEVEL;
#endif
        for (i = 0; i < 40000 && w.phase != CX_P_OVER; i++) {
            cx_tick(&w, 0, 0, 0, 0);
            if (w.ev & CX_EV_SAVE) saves++;
            if (w.ev & CX_EV_FOUL) fouls++;
            if (w.ev & CX_EV_CLAIM) claims++;
            if (w.phase == CX_P_CORNER && w.phase_t == 1) { corners++; ck_t = 200; }
            if (w.phase == CX_P_THROW && w.phase_t == 1) throws++;
            if (ck_t) { ck_t--; if (w.ev & CX_EV_GOAL) { corner_goals++; ck_t = 0; } }
            if (w.ev & CX_EV_PUNCH) punches++;
            if (w.keeper_foul) bad_touch++;
            if (w.ev & CX_EV_CARD) cards++;
            if (w.ev & CX_EV_OFFSIDE) offs++;
            if (w.ev & CX_EV_FEINT) feints++;
            if (w.ev & CX_EV_KICK) kicks++;
            if (w.ev & CX_EV_OUT) outs++;
            if (w.phase == CX_P_PENALTY && w.phase_t == 1) pens++;
            if (w.phase == CX_P_FREEKICK && w.phase_t == 1) fk++;
            if (w.ev & CX_EV_PASS) { passes++; pending = w.ball.last >= 11; }
            if (pending >= 0 && w.ball.owner != 0xFF) {
                int t = w.ball.owner >= 11;
                if (t == pending) { kept++; chain++; }
                else { chains++; chain_len += chain; chain = 0; }
                pending = -1;
            }
            (void)holder;
            /* crowding: men of both sides within 70 units of the ball, in open play */
            if (w.phase == CX_P_PLAY && (i & 31) == 0) {
                int k, c = 0, d2 = 0;
                for (k = 0; k < CX_MEN; k++) {
                    int dx = (w.m[k].x - w.ball.x) / 16, dy = (w.m[k].y - w.ball.y) / 16;
                    if (dx < 0) dx = -dx;
                    if (dy < 0) dy = -dy;
                    if (dx < 70 && dy < 70) c++;
                    if (dx < 30 && dy < 30) d2++;
                }
                crowd += c; near1 += d2; samples++;
            }
        }
        frames += i;
        n++;
        {
            int pa = cx_squad_power((uint8_t)a), pb = cx_squad_power((uint8_t)b);
            if (pa != pb) {
                int sa = pa > pb ? w.score[0] : w.score[1], sb = pa > pb ? w.score[1] : w.score[0];
                strong_n++;
                strong_gd += sa - sb;
                if (sa > sb) strong_w += 2; else if (sa == sb) strong_w += 1;
            }
        }
        goals += w.score[0] + w.score[1];
        g0 += w.score[0]; g1 += w.score[1]; sh0 += w.shots[0]; sh1 += w.shots[1];
        p1 += w.score[1] > w.score[0] ? 2 : (w.score[1] == w.score[0] ? 1 : 0);
        shots += w.shots[0] + w.shots[1];
        reds += w.reds[0] + w.reds[1];
        draws += w.score[0] == w.score[1];
    }
    printf("%d matches a game: goals %.2f shots %.1f saves %.1f fouls %.1f cards %.1f reds %.2f off %.1f pens %.2f fk %.1f feints %.1f kicks %.0f outs %.1f draws %.0f%% frames %.0f\n",
           n, (double)goals / n, (double)shots / n, (double)saves / n, (double)fouls / n, (double)cards / n, (double)reds / n,
           (double)offs / n, (double)pens / n, (double)fk / n, (double)feints / n, (double)kicks / n, (double)outs / n, 100.0 * draws / n, (double)frames / n);
    printf("passes %.1f a game, completed %.0f%%, passes a move %.1f; men within 70 units of the ball %.1f, within 30 %.1f\n",
           (double)passes / n, 100.0 * kept / (passes ? passes : 1), (double)chain_len / (chains ? chains : 1), (double)crowd / (samples ? samples : 1), (double)near1 / (samples ? samples : 1));
    printf("the stronger side (by squad power): %.0f%% of points, goal difference %+.2f a game, over %ld games\n",
           50.0 * strong_w / (strong_n ? strong_n : 1), (double)strong_gd / (strong_n ? strong_n : 1), strong_n);
    printf("side 0: goals %.2f shots %.1f; side 1: goals %.2f shots %.1f, %.0f%% of points\n",
           (double)g0 / n, (double)sh0 / n, (double)g1 / n, (double)sh1 / n, 50.0 * p1 / n);
    printf("corners %.2f a game, goals within 200 frames of one %.2f (%.0f%%); throw-ins %.1f\n", (double)corners / n, (double)corner_goals / n, 100.0 * corner_goals / (corners ? corners : 1), (double)throws / n);
    printf("keepers: crosses caught %.2f, punched %.2f a game\n", (double)claims / n, (double)punches / n);
    printf("fouls on a keeper %.2f a game\n", (double)bad_touch / n);
    return 0;
}
