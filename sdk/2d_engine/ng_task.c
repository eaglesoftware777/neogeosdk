/*
 * ng_task.c — Cooperative tasks (see ng_task.h)
 *
 * A task is suspended as a stack pointer: everything else it needs is on its own
 * stack. ng_task_switch_ pushes the registers C expects a call to keep, stores the
 * stack pointer, loads the other one and pops that side's registers; its rts then
 * returns into whichever side was switched to. A new task's stack is laid out as
 * if it had been switched away from just before entering task_entry.
 *
 * RAM is zeroed at boot and initialised data is not loaded, so every value here is
 * 0 until set: the running task is kept as its index plus one, 0 meaning none.
 */

#include "ng_task.h"

enum { ST_FREE = 0, ST_LIVE = 1, ST_DEAD = 2 };

#define FILL     0x5A5Au
#define SAVED    44u          /* d2-d7, a2-a6 */
#define MIN_STACK 128u

typedef struct {
    uint32_t sp;              /* the stack pointer while it is not running */
    NGTaskFn fn;
    void *arg;
    uint16_t *stack;          /* its lowest word */
    uint16_t bytes;
    uint16_t sleep;           /* runs to wait: run when it is 1 or 0 */
    uint8_t state;
    uint8_t paused;
} NGTask;

static NGTask s_task[NG_TASK_MAX];
static uint32_t s_sched_sp;
static uint8_t s_cur1;        /* running task + 1; 0: the scheduler */

/* void ng_task_switch_(uint32_t *save_sp, uint32_t to_sp) */
void ng_task_switch_(uint32_t *save_sp, uint32_t to_sp);
__asm__ (
    "\t.section neogeo_user,\"ax\",@progbits\n"
    "\t.even\n"
    "\t.globl ng_task_switch_\n"
    "ng_task_switch_:\n"
    "\tmove.l 4(%sp),%a0\n"
    "\tmove.l 8(%sp),%d0\n"
    "\tmovem.l %d2-%d7/%a2-%a6,-(%sp)\n"
    "\tmove.l %sp,(%a0)\n"
    "\tmove.l %d0,%sp\n"
    "\tmovem.l (%sp)+,%d2-%d7/%a2-%a6\n"
    "\trts\n"
    "\t.previous\n"
);

static void NEOGEO_USER task_entry(void)
{
    NGTask *t = &s_task[s_cur1 - 1u];
    t->fn(t->arg);
    ng_task_exit();
}

uint8_t NEOGEO_USER ng_task_spawn(NGTaskFn fn, void *arg, uint16_t *stack, uint16_t stack_bytes)
{
    uint8_t i;
    uint16_t k, words;
    uint32_t *sp;
    if (!fn || !stack || stack_bytes < MIN_STACK) return NG_TASK_NONE;
    for (i = 0; i < NG_TASK_MAX; i++) if (s_task[i].state == ST_FREE) break;
    if (i >= NG_TASK_MAX) return NG_TASK_NONE;
    words = (uint16_t)(stack_bytes >> 1);
    for (k = 0; k < words; k++) stack[k] = FILL;
    /* from the top down: a dummy return address for task_entry, task_entry itself
     * for the switch's rts, then the eleven registers it pops (zeros) */
    sp = (uint32_t *)(void *)(stack + words);
    *--sp = 0;
    *--sp = (uint32_t)(uintptr_t)task_entry;
    for (k = 0; k < SAVED / 4u; k++) *--sp = 0;
    s_task[i].sp = (uint32_t)(uintptr_t)sp;
    s_task[i].fn = fn;
    s_task[i].arg = arg;
    s_task[i].stack = stack;
    s_task[i].bytes = (uint16_t)(words << 1);
    s_task[i].sleep = 0;
    s_task[i].paused = 0;
    s_task[i].state = ST_LIVE;
    return i;
}

void NEOGEO_USER ng_task_run(void)
{
    uint8_t i;
    if (s_cur1) return;                       /* not from inside a task */
    for (i = 0; i < NG_TASK_MAX; i++) {
        NGTask *t = &s_task[i];
        if (t->state != ST_LIVE || t->paused) continue;
        if (t->sleep > 1u) { t->sleep--; continue; }
        t->sleep = 0;
        s_cur1 = (uint8_t)(i + 1u);
        ng_task_switch_(&s_sched_sp, t->sp);
        s_cur1 = 0;
        if (t->state == ST_DEAD) t->state = ST_FREE;
    }
}

void NEOGEO_USER ng_task_sleep(uint16_t frames)
{
    NGTask *t;
    if (!s_cur1) return;
    t = &s_task[s_cur1 - 1u];
    t->sleep = frames ? frames : 1u;
    ng_task_switch_(&t->sp, s_sched_sp);
}

void NEOGEO_USER ng_task_exit(void)
{
    NGTask *t;
    if (!s_cur1) return;
    t = &s_task[s_cur1 - 1u];
    t->state = ST_DEAD;
    for (;;) ng_task_switch_(&t->sp, s_sched_sp);   /* never resumed */
}

void NEOGEO_USER ng_task_kill(uint8_t id)
{
    if (id >= NG_TASK_MAX) return;
    if (s_cur1 == (uint8_t)(id + 1u)) { ng_task_exit(); return; }
    s_task[id].state = ST_FREE;
}

void NEOGEO_USER ng_task_pause(uint8_t id, uint8_t paused)
{
    if (id < NG_TASK_MAX) s_task[id].paused = (uint8_t)(paused != 0u);
}

uint8_t NEOGEO_USER ng_task_alive(uint8_t id)
{
    return (uint8_t)(id < NG_TASK_MAX && s_task[id].state == ST_LIVE);
}

uint8_t NEOGEO_USER ng_task_current(void)
{
    return s_cur1 ? (uint8_t)(s_cur1 - 1u) : NG_TASK_NONE;
}

uint16_t NEOGEO_USER ng_task_stack_left(uint8_t id)
{
    uint16_t k = 0, words;
    if (id >= NG_TASK_MAX || s_task[id].state == ST_FREE) return 0;
    words = (uint16_t)(s_task[id].bytes >> 1);
    while (k < words && s_task[id].stack[k] == FILL) k++;
    return (uint16_t)(k << 1);
}

void NEOGEO_USER ng_task_reset(void)
{
    uint8_t i;
    if (s_cur1) return;
    for (i = 0; i < NG_TASK_MAX; i++) s_task[i].state = ST_FREE;
}
