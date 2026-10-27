/* Maiya & Luna: the game starts from user.c (the BIOS's USER
 * requests) and lives in scenes/; generated art is in artbox/generated. */
#include <stdint.h>
#include "sdk/neogeo.h"
#include "games/maiyaluna/scenes/ml.h"

void NEOGEO_USER maingame(void) { ml_run(); }
int NEOGEO_USER playgame(void) { maingame(); return 0; }
