/*
 * ng_fixscript.c — Text screens as short scripts (see ng_fixscript.h)
 */

#include "ng_fixscript.h"
#include "ng_fix.h"
#include "ng_task.h"

#define SCREEN_W 40u

static uint16_t NEOGEO_USER number(const char **pp)
{
    const char *p = *pp;
    uint16_t v = 0;
    while (*p >= '0' && *p <= '9') { v = (uint16_t)(v * 10u + (uint16_t)(*p - '0')); p++; }
    *pp = p;
    return v;
}

/* the printable width of the rest of the line, braces left out */
static uint8_t NEOGEO_USER line_width(const char *p)
{
    uint8_t n = 0;
    while (*p && *p != '\n') {
        if (*p == '{') { while (*p && *p != '}') p++; if (*p) p++; continue; }
        n++;
        p++;
    }
    return n;
}

/* one command, p just past its '{'; returns 1 when it makes the script wait */
static uint8_t NEOGEO_USER command(NGFixScript *s)
{
    const char *p = s->p;
    uint8_t wait = 0;
    char c = *p;
    if (c >= '0' && c <= '9') {
        uint16_t x = number(&p), y = 0;
        if (*p == ',') { p++; y = number(&p); }
        s->x = s->x0 = (uint8_t)x;
        s->y = (uint8_t)y;
    } else if (c == 'p') { p++; s->pal = (uint8_t)number(&p); }
    else if (c == 't') { p++; s->speed = (uint8_t)number(&p); s->tick = 0; }
    else if (c == 'w') {
        p++;
        if (*p >= '0' && *p <= '9') { s->wait = number(&p); wait = (uint8_t)(s->wait != 0u); }
        else { s->button = 1; wait = 1; }
    } else if (c == '^') {
        uint8_t w;
        p++;
        if (*p == '}') p++;
        w = line_width(p);
        s->x = (uint8_t)(w < SCREEN_W ? (SCREEN_W - w) / 2u : 0u);
        s->centre = 1;
        s->p = p;
        return 0;
    } else if (c == 'b' || c == 'f') {
        uint16_t w, h, t = 0;
        p++;
        w = number(&p);
        if (*p == ',') p++;
        h = number(&p);
        if (c == 'f' && *p == ',') { p++; t = number(&p); }
        if (c == 'b') ng_fix_clear_rect(s->x, s->y, (uint8_t)w, (uint8_t)h, s->pal);
        else {
            uint8_t i, j;
            for (j = 0; j < h; j++) for (i = 0; i < w; i++) ng_fix_put_tile((uint8_t)(s->x + i), (uint8_t)(s->y + j), t, s->pal);
        }
    } else if (c == 'c') { p++; ng_fix_clear(); }
    while (*p && *p != '}') p++;
    if (*p) p++;
    s->p = p;
    return wait;
}

void NEOGEO_USER ng_fixscript_start(NGFixScript *s, const char *script)
{
    s->p = script ? script : "";
    s->wait = 0;
    s->x = s->x0 = 0;
    s->y = 0;
    s->pal = 0;
    s->speed = 0;
    s->tick = 0;
    s->button = 0;
    s->centre = 0;
    s->done = 0;
}

uint8_t NEOGEO_USER ng_fixscript_step(NGFixScript *s, uint16_t pressed)
{
    uint8_t hurry = 0;
    if (s->done) return 1;
    if (s->button) {
        if (!pressed) return 0;
        s->button = 0;
        pressed = 0;                      /* that press is spent on the wait */
    }
    if (s->wait) { s->wait--; if (s->wait) return 0; }
    if (pressed && s->speed) hurry = 1;   /* the rest of the typed line at once */
    for (;;) {
        char c = *s->p;
        if (!c) { s->done = 1; return 1; }
        if (c == '{') {
            s->p++;
            if (command(s)) return 0;
            continue;
        }
        s->p++;
        if (c == '\n') {
            s->y++;
            s->x = s->x0;
            if (s->centre) { s->centre = 0; }
            hurry = 0;
            continue;
        }
        ng_fix_putc(s->x, s->y, c, s->pal);
        s->x++;
        if (s->speed && !hurry && c != ' ') {
            s->wait = s->speed;
            return 0;
        }
    }
}

void NEOGEO_USER ng_fixscript_play(const char *script, uint16_t (*read_pressed)(void))
{
    NGFixScript s;
    ng_fixscript_start(&s, script);
    while (!ng_fixscript_step(&s, read_pressed ? read_pressed() : 0u)) ng_task_sleep(1);
}
