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
    if (!strcmp(e->name, "write_a") || !strcmp(e->name, "write_b") || !strcmp(e->name, "force_a") || !strcmp(e->name, "force_b")) {
        Arg *r = e->args;
        Arg *v = r ? r->next : NULL;
        if (!r || !v) { fprintf(stderr, "%s expects 2 args\n", e->name); exit(1); }
        gen_expr(prog, r->expr); emit("    ld d,a");
        gen_expr(prog, v->expr); emit("    ld e,a");
        if (!strcmp(e->name, "write_a")) emit("    call shadowed_write_a");
        else if (!strcmp(e->name, "write_b")) emit("    call shadowed_write_b");
        else if (!strcmp(e->name, "force_a")) emit("    call force_write_a");
        else emit("    call force_write_b");
        return;
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
            break;
        }

        // Optimized literal arithmetic
        if (e->right->kind == EX_NUM) {
            unsigned char val = e->right->value & 255;
            gen_expr(prog, e->left);
            if (e->op == '+') {
                if (val == 1) emit("    inc a");
                else if (val != 0) printf("    add a,$%02X\n", val);
            }
            else if (e->op == '-') {
                if (val == 1) emit("    dec a");
                else if (val != 0) printf("    sub $%02X\n", val);
            }
            else if (e->op == '&') printf("    and $%02X\n", val);
            else if (e->op == '|') printf("    or $%02X\n", val);
            else if (e->op == '^') printf("    xor $%02X\n", val);
            else if (e->op == TOK_SHR) {
                int n = val & 7;
                while (n--) emit("    srl a");
            }
            else if (e->op == TOK_SHL) {
                int n = val & 7;
                while (n--) emit("    add a,a");
            }
            else {
                printf("    cp $%02X\n", val);
                int label_true = ++label_id, label_end = ++label_id;
                if (e->op == TOK_EQ) printf("    jr z,.cmptrue%d\n", label_true);
                else if (e->op == TOK_NE) printf("    jr nz,.cmptrue%d\n", label_true);
                else if (e->op == '<') printf("    jr c,.cmptrue%d\n", label_true);
                else if (e->op == '>') {
                    printf("    jr z,.cmpend%d\n", label_end);
                    printf("    jr nc,.cmptrue%d\n", label_true);
                }
                else if (e->op == TOK_LE) {
                    printf("    jr c,.cmptrue%d\n", label_true);
                    printf("    jr z,.cmptrue%d\n", label_true);
                }
                else if (e->op == TOK_GE) printf("    jr nc,.cmptrue%d\n", label_true);
                emit("    xor a"); printf("    jr .cmpend%d\n", label_end);
                printf(".cmptrue%d:\n", label_true); emit("    ld a,1"); printf(".cmpend%d:\n", label_end);
            }
            break;
        }

        gen_expr(prog, e->left); emit("    push af");
        gen_expr(prog, e->right); emit("    ld b,a"); emit("    pop af");
        if (e->op == '+') emit("    add a,b");
        else if (e->op == '-') emit("    sub b");
        else if (e->op == '&') emit("    and b");
        else if (e->op == '|') emit("    or b");
        else if (e->op == '^') emit("    xor b");
        else if (e->op == TOK_LAND) {
            int label_false = ++label_id, label_end = ++label_id;
            gen_expr(prog, e->left); emit("    or a"); printf("    jr z,.lbl%d\n", label_false);
            gen_expr(prog, e->right); emit("    or a"); printf("    jr z,.lbl%d\n", label_false);
            emit("    ld a,1"); printf("    jr .lbl%d\n", label_end);
            printf(".lbl%d:\n", label_false); emit("    xor a");
            printf(".lbl%d:\n", label_end);
        }
        else if (e->op == TOK_LOR) {
            int label_true = ++label_id, label_end = ++label_id;
            gen_expr(prog, e->left); emit("    or a"); printf("    jr nz,.lbl%d\n", label_true);
            gen_expr(prog, e->right); emit("    or a"); printf("    jr nz,.lbl%d\n", label_true);
            emit("    xor a"); printf("    jr .lbl%d\n", label_end);
            printf(".lbl%d:\n", label_true); emit("    ld a,1");
            printf(".lbl%d:\n", label_end);
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
            if (!it->body) continue; // Skip prototypes
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
