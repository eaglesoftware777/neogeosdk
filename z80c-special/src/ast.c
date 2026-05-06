#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *xcalloc(size_t n, size_t s) {
    void *p = calloc(n, s);
    if (!p) { perror("calloc"); exit(1); }
    return p;
}

char *xstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = xcalloc(n, 1);
    memcpy(p, s, n);
    return p;
}

char *xstrndup(const char *s, size_t n) {
    char *p = xcalloc(n + 1, 1);
    memcpy(p, s, n);
    p[n] = 0;
    return p;
}

void program_add(Program *p, Item *it) { *p->tail = it; p->tail = &it->next; }
Item *item_new(ItemKind k) { Item *i = xcalloc(1, sizeof(*i)); i->kind = k; return i; }
Param *param_new(char *name) { Param *p = xcalloc(1, sizeof(*p)); p->name = name; return p; }
Expr *expr_new(ExprKind k) { Expr *e = xcalloc(1, sizeof(*e)); e->kind = k; return e; }
Arg *arg_new(Expr *e) { Arg *a = xcalloc(1, sizeof(*a)); a->expr = e; return a; }
Stmt *stmt_new(StmtKind k) { Stmt *s = xcalloc(1, sizeof(*s)); s->kind = k; return s; }

void stmt_append(Stmt **head, Stmt *s) {
    while (*head) head = &(*head)->next;
    *head = s;
}
