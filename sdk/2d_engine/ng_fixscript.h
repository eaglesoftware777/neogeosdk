/*
 * ng_fixscript.h — Text screens as short scripts on the FIX layer
 *
 * A title card, a how-to-play page, a staff roll or a cup draw announcement is
 * mostly text placed on the screen, a colour change or two and some pauses.
 * Written as C it is a page of ng_fix_puts calls and a frame counter; as a
 * script it is one string:
 *
 *   static const char k_card[] =
 *       "{c}{8,4}{p3}THE ROAD TO 2030\n"
 *       "{6,10}{p0}{t2}EIGHT GROUNDS. THIRTY-TWO NATIONS.\n"
 *       "ONE CUP.{w60}\n"
 *       "{6,20}{p2}{t0}PRESS A{w}";
 *
 * Plain text is printed at the cursor; a newline goes down a row, back to the
 * column the line started at. Commands sit in braces:
 *
 *   {x,y}      move the cursor to column x, row y (and start lines there)
 *   {pN}       palette N for what follows
 *   {tN}       typewriter: one letter every N frames ({t0}: all at once)
 *   {wN}       wait N frames          {w}   wait for a button
 *   {^}        centre the rest of this line on the 40-column screen
 *   {bW,H}     blank a W x H box from the cursor
 *   {fW,H,T}   fill a W x H box from the cursor with FIX tile T (decimal)
 *   {c}        clear the whole layer
 *
 * Numbers are decimal. Anything unknown in braces is skipped. A script either
 * runs a step a frame from the game's loop (ng_fixscript_step), or from inside
 * an ng_task task with ng_fixscript_play, which returns when it has ended.
 * A button press during a typed line prints the rest of it at once.
 */

#ifndef NG_FIXSCRIPT_H
#define NG_FIXSCRIPT_H

#include <stdint.h>
#include "macro.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *p;            /* the next character to read */
    uint16_t wait;            /* frames still to wait */
    uint8_t x, y, x0;         /* the cursor and the column lines start at */
    uint8_t pal;
    uint8_t speed, tick;      /* typewriter: frames a letter, and the count */
    uint8_t button;           /* waiting for a button */
    uint8_t centre;           /* the rest of this line is being centred */
    uint8_t done;
} NGFixScript;

void NEOGEO_USER ng_fixscript_start(NGFixScript *s, const char *script);

/* Once a frame, with the buttons pressed this frame (any bit counts as "a
 * button"). Returns 1 once the script has ended. */
uint8_t NEOGEO_USER ng_fixscript_step(NGFixScript *s, uint16_t pressed);

/* From inside an ng_task task: the whole script, a step a frame. read_pressed
 * gives the buttons pressed this frame (NULL: none, so {w} never ends). */
void NEOGEO_USER ng_fixscript_play(const char *script, uint16_t (*read_pressed)(void));

#ifdef __cplusplus
}
#endif

#endif
