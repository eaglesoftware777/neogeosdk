/*
 * ng_join.h — A second player joining a game already under way
 *
 * While a game is played the system ROM watches START: when a player who is not
 * playing presses it (with a credit, on an arcade board), it calls the
 * cartridge's PLAYER_START entry with BIOS_START_FLAG naming him. A game that
 * takes newcomers calls ng_join_bios_start from its PLAYER_START while a match
 * is on; it accepts only players not already in, has the system check and take
 * their credit, and marks them as playing. The game loop then asks, once a frame:
 *
 *   uint8_t who = ng_join_poll();            // 1 or 2: he just came in; 0: nobody
 *   if (who == 2) { hand the CPU's side to player 2 and say so on screen }
 *
 * When the match he joined is over, ng_join_leave(2) marks him as not playing,
 * so his next START is a new join (and a new credit).
 */

#ifndef NG_JOIN_H
#define NG_JOIN_H

#include <stdint.h>
#include "macro.h"

#ifdef __cplusplus
extern "C" {
#endif

/* From the game's PLAYER_START while a match is on. playing: bit 0 player 1,
 * bit 1 player 2. Returns the players taken in (same bits). */
uint8_t NEOGEO_USER ng_join_bios_start(uint8_t playing);

/* Once a frame: the player (1 or 2) who joined since the last call, or 0. */
uint8_t NEOGEO_USER ng_join_poll(void);

/* The player (1 or 2) is out again: his next START is a new join. */
void NEOGEO_USER ng_join_leave(uint8_t p);

#ifdef __cplusplus
}
#endif

#endif
