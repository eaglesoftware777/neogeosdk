#ifndef Z80CC_COMPILER_H
#define Z80CC_COMPILER_H

#include <stddef.h>

typedef enum {
    TOK_EOF = 0, TOK_ID = 256, TOK_NUM, TOK_ASM,
    TOK_VOID, TOK_UNSIGNED, TOK_CHAR, TOK_IF, TOK_ELSE, TOK_WHILE, TOK_RETURN,
    TOK_EQ, TOK_NE, TOK_LE, TOK_GE, TOK_SHR, TOK_SHL, TOK_EXTERN,
    TOK_INC, TOK_DEC,
    TOK_LAND, TOK_LOR
} TokenKind;

typedef struct Token {
    int kind;
    char *text;
    int value;
    int line;
} Token;

typedef struct Lexer {
    const char *src;
    size_t pos;
    int line;
    Token tok;
} Lexer;

typedef enum { TY_VOID, TY_CHAR, TY_PTR, TY_ARRAY } TypeKind;
typedef struct Type Type;
struct Type { TypeKind kind; Type *base; int size; };

typedef enum { EX_NUM, EX_VAR, EX_CALL, EX_BIN, EX_DEREF, EX_ADDR, EX_INDEX } ExprKind;
typedef enum { ST_BLOCK, ST_ASM, ST_LOCAL, ST_ASSIGN, ST_EXPR, ST_RETURN, ST_WHILE, ST_IF } StmtKind;
typedef enum { IT_ASM, IT_GLOBAL, IT_FUNC } ItemKind;

typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Item Item;
typedef struct Param Param;
typedef struct Arg Arg;

struct Arg { Expr *expr; Arg *next; };
struct Param { char *name; Type *type; Param *next; };
struct Expr { ExprKind kind; int value; char *name; int op; Expr *left; Expr *right; Arg *args; Type *type; };
struct Stmt { StmtKind kind; char *text; char *name; Expr *a; Expr *b; Stmt *body; Stmt *else_body; Stmt *next; };
struct Item { ItemKind kind; char *name; char *text; Type *type; Param *params; Stmt *body; Item *next; };

typedef struct Symbol Symbol;
struct Symbol { char *name; Type *type; int is_extern; Symbol *next; };

typedef struct Program { Item *items; Item **tail; Symbol *symbols; } Program;

void lexer_init(Lexer *lx, const char *src);
void lexer_next(Lexer *lx);

Program *parse_program(const char *src);
void emit_program(Program *p);
void optimize_asm_file(const char *path, int level);

void *xcalloc(size_t n, size_t s);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t n);
void program_add(Program *p, Item *it);
Item *item_new(ItemKind k);
Param *param_new(char *name);
Expr *expr_new(ExprKind k);
Arg *arg_new(Expr *e);
Stmt *stmt_new(StmtKind k);
void stmt_append(Stmt **head, Stmt *s);

#endif
