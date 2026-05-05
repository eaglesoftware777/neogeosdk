#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int label_id;

static void emit(const char *s) { puts(s); }
static int count_args(Arg *a) { int n = 0; for (; a; a = a->next) n++; return n; }
static int count_params(Param *p) { int n = 0; for (; p; p = p->next) n++; return n; }
static Item *find_func(Program *prog, const char *name) {
    for (Item *it = prog->items; it; it = it->next) if (it->kind == IT_FUNC && !strcmp(it->name, name)) return it;
    return NULL;
}
static int emitted_name(char **names, int count, const char *name) {
    for (int i = 0; i < count; i++) if (!strcmp(names[i], name)) return 1;
    names[count] = (char *)name;
    return 0;
}

static void raw_asm(const char *text) {
    const char *p = text;
    while (*p) {
        const char *e = strchr(p, '\n');
        if (!e) { printf("%s\n", p); break; }
        printf("%.*s\n", (int)(e - p), p);
        p = e + 1;
    }
}

static void gen_expr(Program *prog, Expr *e);
static void gen_stmt(Program *prog, Stmt *s);

static void gen_call(Program *prog, Expr *e) {
    if (!strcmp(e->name, "out")) {
        Arg *port = e->args;
        Arg *value = port ? port->next : NULL;
        if (!port || !value) { fprintf(stderr, "out expects 2 arguments\n"); exit(1); }
        gen_expr(prog, value->expr); emit("    ld b,a");
        gen_expr(prog, port->expr); emit("    ld c,a"); emit("    out (c),b");
        return;
    }
    if (!strcmp(e->name, "in")) {
        if (!e->args) { fprintf(stderr, "in expects 1 argument\n"); exit(1); }
        gen_expr(prog, e->args->expr); emit("    ld c,a"); emit("    in a,(c)"); return;
    }
    if (!strcmp(e->name, "halt") || !strcmp(e->name, "di") || !strcmp(e->name, "ei")) {
        printf("    %s\n", e->name); return;
    }
    Item *fn = find_func(prog, e->name);
    if (!fn) { fprintf(stderr, "unknown function %s\n", e->name); exit(1); }
    if (count_args(e->args) != count_params(fn->params)) { fprintf(stderr, "%s argument count mismatch\n", e->name); exit(1); }
    Arg *a = e->args;
    Param *p = fn->params;
    while (a && p) {
        gen_expr(prog, a->expr);
        printf("    ld (%s),a\n", p->name);
        a = a->next; p = p->next;
    }
    printf("    call %s\n", e->name);
}

static void gen_expr(Program *prog, Expr *e) {
    if (!e) return;
    switch (e->kind) {
    case EX_NUM: printf("    ld a,$%02X\n", e->value & 255); break;
    case EX_VAR: printf("    ld a,(%s)\n", e->name); break;
    case EX_CALL: gen_call(prog, e); break;
    case EX_BIN: {
        gen_expr(prog, e->left); emit("    push af");
        gen_expr(prog, e->right); emit("    ld b,a"); emit("    pop af");
        if (e->op == '+') emit("    add a,b");
        else if (e->op == '-') emit("    sub b");
        else if (e->op == '&') emit("    and b");
        else if (e->op == '|') emit("    or b");
        else if (e->op == '^') emit("    xor b");
        else if (e->op == TOK_SHR) {
            if (e->right->kind == EX_NUM) {
                int n = e->right->value & 7;
                while (n--) emit("    srl a");
            }
        }
        else {
            int a = ++label_id, b = ++label_id;
            emit("    cp b");
            if (e->op == TOK_EQ) printf("    jr z,.cmptrue%d\n", a);
            else if (e->op == TOK_NE) printf("    jr nz,.cmptrue%d\n", a);
            else if (e->op == '<') printf("    jr c,.cmptrue%d\n", a);
            else if (e->op == '>') {
                printf("    jr z,.cmpend%d\n", b);
                printf("    jr nc,.cmptrue%d\n", a);
            }
            else if (e->op == TOK_LE) {
                printf("    jr c,.cmptrue%d\n", a);
                printf("    jr z,.cmptrue%d\n", a);
            }
            else if (e->op == TOK_GE) printf("    jr nc,.cmptrue%d\n", a);
            emit("    xor a"); printf("    jr .cmpend%d\n", b);
            printf(".cmptrue%d:\n", a); emit("    ld a,1"); printf(".cmpend%d:\n", b);
        }
        break;
    }}
}

static void store(Expr *lhs) {
    if (!lhs || lhs->kind != EX_VAR) { fprintf(stderr, "assignment target must be a variable\n"); exit(1); }
    printf("    ld (%s),a\n", lhs->name);
}

static void gen_stmt(Program *prog, Stmt *s) {
    for (; s; s = s->next) {
        switch (s->kind) {
        case ST_ASM: raw_asm(s->text); break;
        case ST_BLOCK: gen_stmt(prog, s->body); break;
        case ST_LOCAL: break;
        case ST_ASSIGN: gen_expr(prog, s->b); store(s->a); break;
        case ST_EXPR: gen_expr(prog, s->a); break;
        case ST_RETURN: gen_expr(prog, s->a); emit("    ret"); break;
        case ST_WHILE: {
            int a = ++label_id, b = ++label_id;
            printf(".while%d:\n", a); gen_expr(prog, s->a); emit("    or a"); printf("    jr z,.wend%d\n", b);
            gen_stmt(prog, s->body); printf("    jr .while%d\n", a); printf(".wend%d:\n", b); break;
        }
        case ST_IF: {
            int a = ++label_id, b = ++label_id;
            gen_expr(prog, s->a); emit("    or a"); printf("    jr z,.else%d\n", a);
            gen_stmt(prog, s->body); printf("    jr .ifend%d\n", b); printf(".else%d:\n", a);
            gen_stmt(prog, s->else_body); printf(".ifend%d:\n", b); break;
        }}
    }
}

void emit_program(Program *p) {
    emit(".memorymap"); emit("defaultslot 0"); emit("slotsize $10000"); emit("slot 0 $0000"); emit(".endme\n");
    emit(".rombankmap"); emit("bankstotal 1"); emit("banksize $10000"); emit("banks 1"); emit(".endro\n");
    emit(".bank 0 slot 0");
    int first_func = 1;
    for (Item *it = p->items; it; it = it->next) {
        if (it->kind == IT_ASM) {
            raw_asm(it->text);
        } else if (it->kind == IT_FUNC) {
            if (first_func) { emit(".org $0100"); first_func = 0; }
            printf("\n%s:\n", it->name);
            gen_stmt(p, it->body);
            emit("    ret");
        }
    }
    emit("\n.ramsection \"globals\" slot 0 OFFSET $FE00");
    char *names[1024];
    int name_count = 0;
    for (Item *it = p->items; it; it = it->next) {
        if (it->kind == IT_GLOBAL && !emitted_name(names, name_count, it->name)) {
            names[name_count++] = it->name;
            printf("%s: dsb 1\n", it->name);
        }
        if (it->kind == IT_FUNC) for (Param *pa = it->params; pa; pa = pa->next) {
            if (!emitted_name(names, name_count, pa->name)) {
                names[name_count++] = pa->name;
                printf("%s: dsb 1\n", pa->name);
            }
        }
    }
    emit(".ends");
}
