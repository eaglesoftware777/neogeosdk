#!/usr/bin/env python3
"""
Play every Seven Wonders level on the host with the puzzle engine.

    python3 games/sevenwonders/tools/check_levels.py [--seeds 40]

Each level is checked by ng_puzzle_level_check(), then played over many
seeds by a simple bot (the engine's own hints: the best swap, the best
aim, or a random aim when no shot helps; on a picture, the solution in a
shuffled order with a cross and a mistake or two on the way). It reports the bot's win rate
per level and fails if a level is rejected, ever stays busy after settling
(a softlock), or is never won. The bot is far weaker than a player, so its
win rates are a floor for tuning, not the difficulty a player feels.
"""

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]

HARNESS = r"""
#include <stdio.h>
#include <string.h>
#include "ng_puzzle.h"
#define SW_CHAPTERS 7u
#define SW_PER_CHAPTER 5u
#define SW_LEVELS 35u
enum { SW_CAT_MIMI = 0, SW_CAT_KIKO };
typedef struct { const char *name, *line1, *line2; uint8_t cat; } SWChapter;
typedef struct { const char *name; uint8_t bg; } SWPicture;
#include "sw_levels_data.h"
static NGPuzzle P;
static uint32_t rng = 1;
static uint16_t rnd(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (uint16_t)(rng >> 8); }
int main(int argc, char **argv)
{
    int seeds = argc > 1 ? atoi(argv[1]) : 40, bad = 0;
    unsigned li;
    for (li = 0; li < SW_LEVELS; li++) {
        const NGPuzzleLevel *lv = &sw_levels[li];
        uint16_t where = 0;
        int s, wins = 0, stuck = 0;
        long moves = 0;
        if (ng_puzzle_level_check(lv, &where) != NGP_OK) { printf("level %u: REJECTED at %u\n", li + 1, where); bad++; continue; }
        for (s = 1; s <= seeds; s++) {
            int guard = 0;
            if (ng_puzzle_init(&P, lv, (uint32_t)s * 7919u) != NGP_OK) { bad++; break; }
            rng = (uint32_t)s * 2654435761u;
            while (ng_puzzle_status(&P) == NGP_STATUS_READY && guard++ < 400) {
                if (lv->mode == NGP_MODE_LOGIC) {
                    /* a cell at random: fill it if it is in the picture, now and then
                       cross out or fill an empty one by mistake */
                    uint8_t c = (uint8_t)(rnd() % (lv->w * lv->h));
                    if (P.kind[c] == NGP_MARK_NONE) {
                        if (P.color[c]) ng_puzzle_action(&P, NGP_CMD_MARK, c, NGP_MARK_FILL);
                        else ng_puzzle_action(&P, NGP_CMD_MARK, c, (rnd() & 15u) ? NGP_MARK_CROSS : NGP_MARK_FILL);
                    }
                    guard--;
                    if (++moves > 100000L) break;
                } else if (lv->mode == NGP_MODE_MATCH) {
                    uint8_t c, d;
                    if (!ng_puzzle_hint(&P, &c, &d)) break;
                    ng_puzzle_action(&P, NGP_CMD_SWAP, c, d);
                } else {
                    uint16_t a;
                    if (!ng_puzzle_bubble_hint(&P, &a)) a = (uint16_t)(560u + (rnd() % 420u));
                    ng_puzzle_action(&P, NGP_CMD_AIM, (int16_t)a, 0);
                    ng_puzzle_action(&P, NGP_CMD_FIRE, 0, 0);
                }
                ng_puzzle_settle(&P);
                { NGPuzzleEvent e; while (ng_puzzle_event(&P, &e)) {} }
                /* a timed level's clock runs while the player thinks: a second a move */
                if (lv->time_frames) { int t; for (t = 0; t < 60 && ng_puzzle_status(&P) == NGP_STATUS_READY; t++) ng_puzzle_update(&P); }
            }
            if (ng_puzzle_status(&P) == NGP_STATUS_BUSY) stuck++;
            if (ng_puzzle_status(&P) == NGP_STATUS_WON) wins++;
            moves += P.moves_used;
        }
        printf("level %2u %-6s: bot won %3d%% (%d/%d), %ld moves a game%s\n", li + 1,
               lv->mode == NGP_MODE_MATCH ? "match" : lv->mode == NGP_MODE_LOGIC ? "logic" : "bubble", 100 * wins / seeds, wins, seeds,
               moves / seeds, stuck ? "  STUCK" : "");
        if (stuck || !wins) bad++;
    }
    return bad ? 1 : 0;
}
"""


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--seeds", type=int, default=40)
    args = ap.parse_args()
    subprocess.run([sys.executable, str(GAME / "tools/levels.py")], check=True)
    with tempfile.TemporaryDirectory(prefix="sw-levels-") as d:
        src = Path(d) / "harness.c"
        src.write_text("#include <stdlib.h>\n" + HARNESS, encoding="ascii")
        exe = Path(d) / "harness"
        subprocess.run(["cc", "-O2", "-std=c99", "-w", "-I" + str(ROOT / "sdk"), "-I" + str(ROOT / "sdk/2d_engine"),
                        "-I" + str(GAME / "scenes"), str(src), str(ROOT / "sdk/2d_engine/ng_puzzle.c"),
                        str(ROOT / "sdk/2d_engine/ng_trig.c"), str(ROOT / "sdk/2d_engine/ng_trig_table.c"),
                        "-o", str(exe)], check=True)
        r = subprocess.run([str(exe), str(args.seeds)])
        return r.returncode


if __name__ == "__main__":
    sys.exit(main())
