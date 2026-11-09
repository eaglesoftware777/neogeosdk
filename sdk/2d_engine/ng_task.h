/*
 * ng_task.h — Cooperative tasks: code that waits N frames and carries on
 *
 * A cut-scene, a referee's joke, a caption that zooms in, holds and leaves, a
 * menu that types itself out: as frame-by-frame state machines each is a switch
 * on a step counter. As a task it is straight-line code:
 *
 *   static uint16_t s_stack[192];               // words; see "stacks" below
 *   static void caption(void *arg)
 *   {
 *       uint8_t k;
 *       for (k = 0; k < 16; k++) { grow(k); ng_task_sleep(1); }
 *       ng_task_sleep(90);                       // hold for a second and a half
 *       hide();
 *   }                                            // returning ends the task
 *   ...
 *   ng_task_spawn(caption, 0, s_stack, sizeof s_stack);
 *
 * and the game's frame loop calls ng_task_run() once a frame. Each task with no
 * frames left to sleep runs until it sleeps again (or returns); the others are
 * skipped. Nothing is pre-empted: a task that never sleeps never gives the CPU
 * back. Tasks run in the order they were spawned.
 *
 * Calls that may only be made from inside a task: ng_task_sleep, ng_task_yield,
 * ng_task_exit. The rest may be made from anywhere.
 *
 * Stacks: each task runs on its own stack, given by the caller (a static array
 * of uint16_t, word-aligned). It must hold the task's own calls and locals, the
 * 44 bytes ng_task saves when it switches, and an interrupt frame, since the
 * vertical-blank handler runs on whatever stack is current: 256 bytes is a safe
 * floor for a task that calls the SDK's drawing functions, more if it keeps big
 * locals. ng_task_stack_left(id) reports how much a task never touched, so a
 * stack can be sized from a test run (the stack is filled with a pattern at
 * spawn).
 *
 * The switch itself is a few instructions of assembly (ng_task.c): it saves the
 * registers the C calling convention says a function must preserve (d2-d7,
 * a2-a6) and swaps the stack pointer. It runs in the 68000's supervisor mode,
 * the mode a Neo Geo game runs in.
 */

#ifndef NG_TASK_H
#define NG_TASK_H

#include <stdint.h>
#include "macro.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NG_TASK_MAX
#define NG_TASK_MAX 8
#endif

#define NG_TASK_NONE 0xFFu

typedef void (*NGTaskFn)(void *arg);

/* Starts fn(arg) on the given stack at the next ng_task_run. Returns the task's
 * id, or NG_TASK_NONE when every slot is taken or the stack is too small. */
uint8_t NEOGEO_USER ng_task_spawn(NGTaskFn fn, void *arg, uint16_t *stack, uint16_t stack_bytes);

/* Runs every task whose sleep is over, in turn. Once a frame, from the game's loop
 * (never from inside a task). */
void NEOGEO_USER ng_task_run(void);

/* Inside a task: give the CPU back and carry on after `frames` calls of
 * ng_task_run (0 or 1: the next one). */
void NEOGEO_USER ng_task_sleep(uint16_t frames);
#define ng_task_yield() ng_task_sleep(1)

/* Inside a task: end it now (returning from the task's function does the same). */
void NEOGEO_USER ng_task_exit(void);

/* From anywhere: end a task. From inside the task itself it is ng_task_exit. */
void NEOGEO_USER ng_task_kill(uint8_t id);

/* From anywhere: stop running a task (1) or let it run again (0). */
void NEOGEO_USER ng_task_pause(uint8_t id, uint8_t paused);

/* 1 while the task has not ended. */
uint8_t NEOGEO_USER ng_task_alive(uint8_t id);

/* The running task's id, or NG_TASK_NONE outside any task. */
uint8_t NEOGEO_USER ng_task_current(void);

/* Bytes of a task's stack it has never written: its spare room so far. */
uint16_t NEOGEO_USER ng_task_stack_left(uint8_t id);

/* Ends every task at once (a scene change). */
void NEOGEO_USER ng_task_reset(void);

#ifdef __cplusplus
}
#endif

#endif
