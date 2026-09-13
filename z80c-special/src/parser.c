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

static Type *type_new(TypeKind k) { Type *t = xcalloc(1, sizeof(Type)); t->kind = k; return t; }
static Type *type_char(void) { return type_new(TY_CHAR); }
static Type *type_ptr(Type *base) { Type *t = type_new(TY_PTR); t->base = base; return t; }

static Type *parse_type(Parser *p) {
    Type *t = NULL;
    if (peek(p, TOK_VOID)) { take(p); t = type_new(TY_VOID); }
    else {
        if (peek(p, TOK_UNSIGNED)) take(p);
        expect(p, TOK_CHAR);
        t = type_char();
    }
    while (peek(p, '*')) { take(p); t = type_ptr(t); }
    return t;
}

static Expr *expr(Parser *p, Program *prog);
static Stmt *statement(Parser *p, Program *prog);

static void reg_symbol(Program *prog, char *name, Type *t, int is_ext) {
    Symbol *s = xcalloc(1, sizeof(Symbol));
    s->name = name; s->type = t; s->is_extern = is_ext; s->next = prog->symbols; prog->symbols = s;
}

static Symbol *find_symbol(Program *prog, const char *name) {
    for (Symbol *s = prog->symbols; s; s = s->next) if (!strcmp(s->name, name)) return s;
    return NULL;
}

static Expr *unary(Parser *p, Program *prog) {
    if (peek(p, '*')) {
        take(p);
        Expr *e = expr_new(EX_DEREF);
        e->left = unary(p, prog);
        if (e->left->type && e->left->type->kind == TY_PTR) e->type = e->left->type->base;
        else e->type = type_char();
        return e;
    }
    if (peek(p, '&')) {
        take(p);
        Expr *e = expr_new(EX_ADDR);
        e->left = unary(p, prog);
        e->type = type_ptr(e->left->type ? e->left->type : type_char());
        return e;
    }
    if (peek(p, '(')) { take(p); Expr *e = expr(p, prog); expect(p, ')'); return e; }
    if (peek(p, TOK_NUM)) { Expr *e = expr_new(EX_NUM); e->value = take(p).value; e->type = type_char(); return e; }
    Token id = expect(p, TOK_ID);
    if (peek(p, '(')) {
        take(p); Expr *e = expr_new(EX_CALL); e->name = id.text;
        Arg **tail = &e->args;
        if (!peek(p, ')')) {
            for (;;) {
                *tail = arg_new(expr(p, prog));
                tail = &(*tail)->next;
                if (peek(p, ')')) break;
                expect(p, ',');
            }
        }
        expect(p, ')'); e->type = type_char(); return e;
    }
    Expr *e = expr_new(EX_VAR); e->name = id.text;
    Symbol *sym = find_symbol(prog, e->name);
    if (sym) { e->name = sym->name; e->type = sym->type; }
    return e;
}

static Expr *postfix(Parser *p, Program *prog) {
    Expr *e = unary(p, prog);
    while (peek(p, '[')) {
        take(p);
        Expr *idx = expr_new(EX_INDEX);
        idx->left = e;
        idx->right = expr(p, prog);
        expect(p, ']');
        if (e->type && (e->type->kind == TY_ARRAY || e->type->kind == TY_PTR)) idx->type = e->type->base;
        else idx->type = type_char();
        e = idx;
    }
    return e;
}

static int prec(int k) {
    if (k == '*' || k == '/' || k == '%') return 10;
    if (k == '+' || k == '-') return 9;
    if (k == TOK_SHL || k == TOK_SHR) return 8;
    if (k == '<' || k == '>' || k == TOK_LE || k == TOK_GE) return 7;
    if (k == TOK_EQ || k == TOK_NE) return 6;
    if (k == '&') return 5;
    if (k == '^') return 4;
    if (k == '|') return 3;
    if (k == TOK_LAND) return 2;
    if (k == TOK_LOR) return 1;
    return 0;
}

static Expr *expr_prec(Parser *p, Program *prog, int min_prec) {
    Expr *left = postfix(p, prog);
    for (;;) {
        int op = p->lx.tok.kind;
        int p_op = prec(op);
        if (p_op < min_prec) break;
        take(p);
        Expr *e = expr_new(EX_BIN);
        e->op = op;
        e->left = left;
        e->right = expr_prec(p, prog, p_op + 1);
        e->type = left->type;
        left = e;
    }
    return left;
}

static Expr *expr(Parser *p, Program *prog) {
    return expr_prec(p, prog, 1);
}

static Stmt *block(Parser *p, Program *prog) {
    expect(p, '{');
    Stmt *head = NULL;
    while (!peek(p, '}')) stmt_append(&head, statement(p, prog));
    expect(p, '}');
    Stmt *s = stmt_new(ST_BLOCK);
    s->body = head;
    return s;
}

static Type *parse_decl(Parser *p, Type *base, char **name) {
    Type *t = base;
    *name = expect(p, TOK_ID).text;
    if (peek(p, '[')) {
        take(p);
        Type *arr = type_new(TY_ARRAY);
        arr->base = t;
        if (peek(p, TOK_NUM)) arr->size = take(p).value;
        else arr->size = 1;
        expect(p, ']');
        t = arr;
    }
    return t;
}

static Stmt *statement(Parser *p, Program *prog) {
    if (peek(p, ';')) { take(p); return stmt_new(ST_BLOCK); }
    if (peek(p, TOK_ASM)) { Stmt *s = stmt_new(ST_ASM); s->text = take(p).text; return s; }
    if (peek(p, '{')) return block(p, prog);
    if (peek(p, TOK_WHILE)) {
        take(p); expect(p, '(');
        Stmt *s = stmt_new(ST_WHILE); s->a = expr(p, prog); expect(p, ')'); s->body = statement(p, prog); return s;
    }
    if (peek(p, TOK_IF)) {
        take(p); expect(p, '(');
        Stmt *s = stmt_new(ST_IF); s->a = expr(p, prog); expect(p, ')'); s->body = statement(p, prog);
        if (peek(p, TOK_ELSE)) { take(p); s->else_body = statement(p, prog); }
        return s;
    }
    if (peek(p, TOK_RETURN)) {
        take(p); Stmt *s = stmt_new(ST_RETURN);
        if (!peek(p, ';')) s->a = expr(p, prog);
        expect(p, ';'); return s;
    }
    if (peek(p, TOK_UNSIGNED) || peek(p, TOK_VOID) || peek(p, TOK_CHAR)) {
        Type *base = parse_type(p);
        char *name;
        Type *t = parse_decl(p, base, &name);
        reg_symbol(prog, name, t, 0);
        Stmt *s = stmt_new(ST_LOCAL); s->name = name; expect(p, ';'); return s;
    }
    Expr *lhs = expr(p, prog);
    if (peek(p, '=')) {
        take(p); Stmt *s = stmt_new(ST_ASSIGN); s->a = lhs; s->b = expr(p, prog); expect(p, ';'); return s;
    }
    if (peek(p, TOK_INC)) {
        take(p); Stmt *s = stmt_new(ST_ASSIGN); s->a = lhs;
        s->b = expr_new(EX_BIN); s->b->op = '+'; s->b->left = lhs;
        s->b->right = expr_new(EX_NUM); s->b->right->value = 1; s->b->type = type_char();
        expect(p, ';'); return s;
    }
    if (peek(p, TOK_DEC)) {
        take(p); Stmt *s = stmt_new(ST_ASSIGN); s->a = lhs;
        s->b = expr_new(EX_BIN); s->b->op = '-'; s->b->left = lhs;
        s->b->right = expr_new(EX_NUM); s->b->right->value = 1; s->b->type = type_char();
        expect(p, ';'); return s;
    }
    Stmt *s = stmt_new(ST_EXPR); s->a = lhs; expect(p, ';'); return s;
}

static Item *function(Parser *p, Program *prog, Type *ret, char *name) {
    Item *it = item_new(IT_FUNC); it->name = name; it->type = ret;
    expect(p, '(');
    Param **tail = &it->params;
    if (peek(p, TOK_VOID) && p->lx.src[p->lx.pos] == ')') {
        take(p);
    } else if (!peek(p, ')')) {
        for (;;) {
            Type *base = parse_type(p);
            char *pname;
            Type *pt = parse_decl(p, base, &pname);
            Param *pa = param_new(pname); pa->type = pt;
            reg_symbol(prog, pname, pt, 0);
            *tail = pa; tail = &pa->next;
            if (peek(p, ')')) break;
            expect(p, ',');
        }
    }
    expect(p, ')');
    if (peek(p, ';')) {
        take(p); it->body = NULL;
    } else {
        it->body = block(p, prog)->body;
    }
    return it;
}

Program *parse_program(const char *src) {
    Parser p; lexer_init(&p.lx, src); lexer_next(&p.lx);
    Program *prog = xcalloc(1, sizeof(Program)); prog->tail = &prog->items;
    while (!peek(&p, TOK_EOF)) {
        if (peek(&p, TOK_ASM)) {
            Item *it = item_new(IT_ASM); it->text = take(&p).text; program_add(prog, it);
            continue;
        }
        int is_ext = 0;
        if (peek(&p, TOK_EXTERN)) { take(&p); is_ext = 1; }
        Type *base = parse_type(&p);
        char *name;
        Type *t = parse_decl(&p, base, &name);
        if (peek(&p, '(')) {
            program_add(prog, function(&p, prog, t, name));
        } else {
            expect(&p, ';');
            Item *it = item_new(IT_GLOBAL); it->name = name; it->type = t;
            reg_symbol(prog, name, t, is_ext);
            program_add(prog, it);
        }
    }
    return prog;
}
