/* Host-only QA: obtain a move from a read-only snapshot of the first board.
 * Output is used to press the game's hint/pick/direction buttons. */
#include <stdio.h>
#include <string.h>
#include "ng_puzzle.h"
int main(int argc, char **argv)
{
    static NGPuzzle p;
    NGPuzzleLevel lv;
    uint8_t header[4], c, d;
    uint16_t n;
    FILE *file;
    if (argc != 2 || !(file = fopen(argv[1], "rb"))) return 1;
    if (fread(header, 1, 4, file) != 4 || header[0] != NGP_MODE_MATCH ||
        header[1] != 7 || header[2] != 7 || header[3] > 8 || header[3] < 2) return 2;
    memset(&lv, 0, sizeof(lv));
    lv.mode = header[0]; lv.w = header[1]; lv.h = header[2]; lv.colors = header[3];
    p.level = &lv; p.mode = lv.mode; p.w = lv.w; p.h = lv.h; p.colors = lv.colors;
    n = (uint16_t)(p.w * p.h);
    if (fread(p.color, 1, n, file) != n || fread(p.kind, 1, n, file) != n ||
        fread(p.cover, 1, n, file) != n) return 3;
    fclose(file);
    if (!ng_puzzle_hint(&p, &c, &d)) return 4;
    printf("%u %u\n", c, d);
    return 0;
}
