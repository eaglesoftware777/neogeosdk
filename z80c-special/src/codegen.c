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

static int has_explicit_org(Program *p) {
    for (Item *it = p->items; it; it = it->next) {
        if (it->kind == IT_ASM && it->text && strstr(it->text, ".org")) return 1;
    }
    return 0;
}

static void gen_expr(Program *prog, Expr *e);
static void gen_stmt(Program *prog, Stmt *s);

static void gen_call(Program *prog, Expr *e) {
    if (!strcmp(e->name, "out")) {
        Arg *port = e->args;
        Arg *value = port ? port->next : NULL;
        if (!port || !value) { fprintf(stderr, "out expects 2 args\n"); exit(1); }
        gen_expr(prog, value->expr); emit("    ld b,a");
        gen_expr(prog, port->expr); emit("    ld c,a"); emit("    out (c),b");
        return;
    }
    if (!strcmp(e->name, "in")) {
        if (!e->args) { fprintf(stderr, "in expects 1 arg\n"); exit(1); }
        gen_expr(prog, e->args->expr); emit("    ld c,a"); emit("    in a,(c)"); return;
    }
    if (!strcmp(e->name, "halt") || !strcmp(e->name, "di") || !strcmp(e->name, "ei")) {
        printf("    %s\n", e->name); return;
    }
    Item *fn = find_func(prog, e->name);
    if (!fn) { fprintf(stderr, "unknown function %s\n", e->name); exit(1); }
    Arg *a = e->args;
    Param *p = fn->params;
    while (a && p) {
        gen_expr(prog, a->expr);
        if (p->type && (p->type->kind == TY_PTR || p->type->kind == TY_ARRAY)) {
            printf("    ld (%s),hl\n", p->name);
        } else {
            printf("    ld (%s),a\n", p->name);
        }
        a = a->next; p = p->next;
    }
    printf("    call %s\n", e->name);
}

static void gen_expr(Program *prog, Expr *e) {
    if (!e) return;
    switch (e->kind) {
    case EX_NUM: printf("    ld a,$%02X\n", e->value & 255); break;
    case EX_VAR:
        if (e->type && e->type->kind == TY_ARRAY) {
            printf("    ld hl,%s\n", e->name);
        } else if (e->type && e->type->kind == TY_PTR) {
            printf("    ld hl,(%s)\n", e->name);
        } else {
            printf("    ld a,(%s)\n", e->name);
        }
        break;
    case EX_DEREF:
        gen_expr(prog, e->left); // This will put the pointer addr in HL
        printf("    ld a,(hl)\n");
        break;
    case EX_ADDR:
        printf("    ld hl,%s\n", e->left->name);
        break;
    case EX_INDEX:
        // Optimize for literal base (array index)
        if (e->left->kind == EX_VAR && e->left->type && e->left->type->kind == TY_ARRAY) {
            printf("    ld hl,%s\n", e->left->name);
        } else {
            gen_expr(prog, e->left); // HL = base
        }
        printf("    push hl\n");
        gen_expr(prog, e->right); // A = index
        printf("    ld e,a\n    ld d,0\n    pop hl\n    add hl,de\n    ld a,(hl)\n");
        break;
    case EX_CALL: gen_call(prog, e); break;
    case EX_BIN: {
        Type *lt = e->left->type;
        if (lt && (lt->kind == TY_PTR || lt->kind == TY_ARRAY)) {
            gen_expr(prog, e->left); // Result in HL
            printf("    push hl\n");
            gen_expr(prog, e->right); // Result in A
            printf("    ld e,a\n    ld d,0\n    pop hl\n");
            if (e->op == '+') printf("    add hl,de\n");
            else if (e->op == '-') printf("    or a\n    sbc hl,de\n");
            // Result is now in HL
            break;
        }
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
        else if (e->op == TOK_SHL) {
            if (e->right->kind == EX_NUM) {
                int n = e->right->value & 7;
                while (n--) emit("    add a,a");
            }
        }
        else {
            int a = ++label_id, b = ++label_id;
            emit("    cp b");
            if (e->op == TOK_EQ) printf("    jr z,.cmptrue%d\n", a);
            else if (e->op == TOK_NE) printf("    jr nz,.cmptrue%d\n", a);
            else if (e->op == '<') printf("    jr c,.cmptrue%d\n", a);
            else if (e->op == '>') {
                printf("    jp z,.cmpend%d\n", b);
                printf("    jr nc,.cmptrue%d\n", a);
            }
            else if (e->op == TOK_LE) {
                printf("    jr c,.cmptrue%d\n", a);
                printf("    jr z,.cmptrue%d\n", a);
            }
            else if (e->op == TOK_GE) printf("    jr nc,.cmptrue%d\n", a);
            emit("    xor a"); printf("    jp .cmpend%d\n", b);
            printf(".cmptrue%d:\n", a); emit("    ld a,1"); printf(".cmpend%d:\n", b);
        }
        break;
    }}
}

static void store(Expr *lhs) {
    if (lhs->kind == EX_VAR) {
        if (lhs->type && (lhs->type->kind == TY_PTR || lhs->type->kind == TY_ARRAY)) {
            printf("    ld (%s),hl\n", lhs->name);
        } else {
            printf("    ld (%s),a\n", lhs->name);
        }
    } else if (lhs->kind == EX_DEREF) {
        printf("    push af\n    ld hl,(%s)\n    pop af\n    ld (hl),a\n", lhs->left->name);
    } else if (lhs->kind == EX_INDEX) {
        printf("    push af\n    ld hl,%s\n", lhs->left->name);
        // This is complex because index might be in A.
        // We'll skip complex LHS for now.
        printf("    pop af\n    ld (hl),a\n"); 
    }
}

static void gen_stmt(Program *prog, Stmt *s) {
    for (; s; s = s->next) {
        switch (s->kind) {
        case ST_ASM: raw_asm(s->text); break;
        case ST_BLOCK: gen_stmt(prog, s->body); break;
        case ST_ASSIGN: gen_expr(prog, s->b); store(s->a); break;
        case ST_EXPR: gen_expr(prog, s->a); break;
        case ST_RETURN: gen_expr(prog, s->a); emit("    ret"); break;
        case ST_WHILE: {
            int a = ++label_id, b = ++label_id;
            printf(".while%d:\n", a);
            if (!(s->a && s->a->kind == EX_NUM && s->a->value != 0)) {
                gen_expr(prog, s->a); emit("    or a"); printf("    jp z,.wend%d\n", b);
            }
            gen_stmt(prog, s->body);
            printf("    jp .while%d\n", a);
            printf(".wend%d:\n", b);
            break;
        }
        case ST_IF: {
            int a = ++label_id, b = ++label_id;
            gen_expr(prog, s->a); emit("    or a"); printf("    jp z,.else%d\n", a);
            gen_stmt(prog, s->body); printf("    jp .ifend%d\n", b); printf(".else%d:\n", a);
            gen_stmt(prog, s->else_body); printf(".ifend%d:\n", b); break;
        }
        default: break;
        }
    }
}

void emit_program(Program *p) {
    emit(".memorymap"); emit("defaultslot 0"); emit("slotsize $10000"); emit("slot 0 $0000"); emit(".endme\n");
    emit(".rombankmap"); emit("bankstotal 1"); emit("banksize $10000"); emit("banks 1"); emit(".endro\n");
    emit(".bank 0 slot 0");
    int explicit_org = has_explicit_org(p);
    int first_func = 1;
    for (Item *it = p->items; it; it = it->next) {
        if (it->kind == IT_ASM) raw_asm(it->text);
        else if (it->kind == IT_FUNC) {
            if (first_func && !explicit_org) emit(".org $0100");
            first_func = 0;
            printf("\n%s:\n", it->name);
            gen_stmt(p, it->body);
            emit("    ret");
        }
    }
    emit("\n.ramsection \"globals\" slot 0 OFFSET $FE00");
    char *names[1024];
    int name_count = 0;
    for (Symbol *s = p->symbols; s; s = s->next) {
        if (s->is_extern) continue;
        if (!emitted_name(names, name_count, s->name)) {
            int sz = 1;
            if (s->type) {
                if (s->type->kind == TY_ARRAY) sz = s->type->size;
                else if (s->type->kind == TY_PTR) sz = 2;
            }
            printf("%s: dsb %d\n", s->name, sz);
            name_count++;
        }
    }
    emit(".ends");
}
