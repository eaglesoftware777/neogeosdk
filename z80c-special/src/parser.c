#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Parser { Lexer lx; } Parser;

static int peek(Parser *p, int k) { return p->lx.tok.kind == k; }
static Token take(Parser *p) { Token t = p->lx.tok; lexer_next(&p->lx); return t; }
static Token expect(Parser *p, int k) {
    if (!peek(p, k)) {
        fprintf(stderr, "line %d: expected token %d, got '%s'\n", p->lx.tok.line, k, p->lx.tok.text);
        exit(1);
    }
    return take(p);
}

static void parse_type(Parser *p) {
    if (peek(p, TOK_VOID)) { take(p); return; }
    expect(p, TOK_UNSIGNED);
    expect(p, TOK_CHAR);
}

static Expr *expr(Parser *p);
static Stmt *statement(Parser *p);

static Expr *primary(Parser *p) {
    if (peek(p, '(')) { take(p); Expr *e = expr(p); expect(p, ')'); return e; }
    if (peek(p, TOK_NUM)) { Expr *e = expr_new(EX_NUM); e->value = take(p).value; return e; }
    Token id = expect(p, TOK_ID);
    if (peek(p, '(')) {
        take(p);
        Expr *e = expr_new(EX_CALL);
        e->name = id.text;
        Arg **tail = &e->args;
        if (!peek(p, ')')) {
            for (;;) {
                *tail = arg_new(expr(p));
                tail = &(*tail)->next;
                if (peek(p, ')')) break;
                expect(p, ',');
            }
        }
        expect(p, ')');
        return e;
    }
    Expr *e = expr_new(EX_VAR);
    e->name = id.text;
    return e;
}

static int binop(Parser *p) {
    int k = p->lx.tok.kind;
    if (k == '+' || k == '-' || k == '&' || k == '|' || k == '^' || k == '<' || k == '>' || k == TOK_EQ || k == TOK_NE || k == TOK_LE || k == TOK_GE || k == TOK_SHR) return k;
    return 0;
}

static Expr *expr(Parser *p) {
    Expr *left = primary(p);
    int op;
    while ((op = binop(p)) != 0) {
        take(p);
        Expr *e = expr_new(EX_BIN);
        e->op = op;
        e->left = left;
        e->right = primary(p);
        left = e;
    }
    return left;
}

static Stmt *block(Parser *p) {
    expect(p, '{');
    Stmt *head = NULL;
    while (!peek(p, '}')) stmt_append(&head, statement(p));
    expect(p, '}');
    Stmt *s = stmt_new(ST_BLOCK);
    s->body = head;
    return s;
}

static Stmt *statement(Parser *p) {
    if (peek(p, TOK_ASM)) { Stmt *s = stmt_new(ST_ASM); s->text = take(p).text; return s; }
    if (peek(p, '{')) return block(p);
    if (peek(p, TOK_WHILE)) {
        take(p); expect(p, '(');
        Stmt *s = stmt_new(ST_WHILE); s->a = expr(p); expect(p, ')'); s->body = statement(p); return s;
    }
    if (peek(p, TOK_IF)) {
        take(p); expect(p, '(');
        Stmt *s = stmt_new(ST_IF); s->a = expr(p); expect(p, ')'); s->body = statement(p);
        if (peek(p, TOK_ELSE)) { take(p); s->else_body = statement(p); }
        return s;
    }
    if (peek(p, TOK_RETURN)) {
        take(p); Stmt *s = stmt_new(ST_RETURN);
        if (!peek(p, ';')) s->a = expr(p);
        expect(p, ';'); return s;
    }
    if (peek(p, TOK_UNSIGNED)) {
        parse_type(p); Stmt *s = stmt_new(ST_LOCAL); s->name = expect(p, TOK_ID).text; expect(p, ';'); return s;
    }
    Expr *lhs = expr(p);
    if (peek(p, '=')) {
        take(p); Stmt *s = stmt_new(ST_ASSIGN); s->a = lhs; s->b = expr(p); expect(p, ';'); return s;
    }
    Stmt *s = stmt_new(ST_EXPR); s->a = lhs; expect(p, ';'); return s;
}

static Item *function(Parser *p, char *name) {
    Item *it = item_new(IT_FUNC); it->name = name;
    expect(p, '(');
    Param **tail = &it->params;
    if (peek(p, TOK_VOID)) {
        take(p);
    } else if (!peek(p, ')')) {
        for (;;) {
            parse_type(p);
            *tail = param_new(expect(p, TOK_ID).text);
            tail = &(*tail)->next;
            if (peek(p, ')')) break;
            expect(p, ',');
        }
    }
    expect(p, ')');
    Stmt *b = block(p);
    it->body = b->body;
    return it;
}

Program *parse_program(const char *src) {
    Parser p;
    lexer_init(&p.lx, src);
    Program *prog = xcalloc(1, sizeof(*prog));
    prog->tail = &prog->items;
    while (!peek(&p, TOK_EOF)) {
        if (peek(&p, TOK_ASM)) { Item *it = item_new(IT_ASM); it->text = take(&p).text; program_add(prog, it); continue; }
        parse_type(&p);
        char *name = expect(&p, TOK_ID).text;
        if (peek(&p, '(')) program_add(prog, function(&p, name));
        else { expect(&p, ';'); Item *it = item_new(IT_GLOBAL); it->name = name; program_add(prog, it); }
    }
    return prog;
}
