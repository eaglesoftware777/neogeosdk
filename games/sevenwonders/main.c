/* Seven Wonders Puzzles: the game starts from user.c (the BIOS's USER
 * requests) and lives in scenes/; generated art is in artbox/generated. */
#include <stdint.h>
#include "sdk/neogeo.h"
#include "games/sevenwonders/scenes/sw.h"

void NEOGEO_USER maingame(void) { sw_run(); }
int NEOGEO_USER playgame(void) { maingame(); return 0; }
