/*
 * sw_levels.c — the seven wonders and their levels.
 *
 * Seven chapters of four levels, two of each puzzle family. Each chapter
 * brings in one new rule and keeps the ones before it:
 *   I    the Sky Garden          collecting by colour, clearing a field
 *   II   the Crystal Canal       ice under the pieces, anchors in the field
 *   III  the Moon Lighthouse     special pieces (lines, bombs, prisms) as goals
 *   IV   the Clockwork Tower     crates to break, rows that come down
 *   V    the Coral Palace        locked pieces, kittens to rescue
 *   VI   the Sunstone Pyramid    the clock
 *   VII  the Starlight Dome      everything at once
 * The tables are written by tools/levels.py from levels/levels.json; every
 * level passes ng_puzzle_level_check() (tools/check_levels.py plays them on
 * the host with the engine's own hints).
 */
#include "sw.h"
#include "sw_levels_data.h"
