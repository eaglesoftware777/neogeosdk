/* No Signal Z Alpha: the game starts from user.c (the BIOS's USER
 * requests) and lives in scenes/; generated art is in artbox/generated. */
#include <stdint.h>
#include "sdk/neogeo.h"
#include "games/nosignal/scenes/ns.h"

void NEOGEO_USER maingame(void) { ns_run(); }
int NEOGEO_USER playgame(void) { maingame(); return 0; }
