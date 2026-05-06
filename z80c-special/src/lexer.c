#include "compiler.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int isid0(int c) { return isalpha(c) || c == '_'; }
static int isid(int c) { return isalnum(c) || c == '_'; }

void lexer_init(Lexer *lx, const char *src) {
    memset(lx, 0, sizeof(*lx));
    lx->src = src;
    lx->line = 1;
    lexer_next(lx);
}

static void skip_ws(Lexer *lx) {
    for (;;) {
        char c = lx->src[lx->pos];
        if (c == '\n') { lx->line++; lx->pos++; continue; }
        if (isspace((unsigned char)c)) { lx->pos++; continue; }
        if (c == '/' && lx->src[lx->pos + 1] == '/') {
            lx->pos += 2;
            while (lx->src[lx->pos] && lx->src[lx->pos] != '\n') lx->pos++;
            continue;
        }
        if (c == '/' && lx->src[lx->pos + 1] == '*') {
            lx->pos += 2;
            while (lx->src[lx->pos] && !(lx->src[lx->pos] == '*' && lx->src[lx->pos + 1] == '/')) {
                if (lx->src[lx->pos] == '\n') lx->line++;
                lx->pos++;
            }
            if (lx->src[lx->pos]) lx->pos += 2;
            continue;
        }
        break;
    }
}

static int keyword(const char *s) {
    if (!strcmp(s, "void")) return TOK_VOID;
    if (!strcmp(s, "unsigned")) return TOK_UNSIGNED;
    if (!strcmp(s, "char")) return TOK_CHAR;
    if (!strcmp(s, "if")) return TOK_IF;
    if (!strcmp(s, "else")) return TOK_ELSE;
    if (!strcmp(s, "while")) return TOK_WHILE;
    if (!strcmp(s, "return")) return TOK_RETURN;
    return TOK_ID;
}

static char *read_asm_block(Lexer *lx) {
    skip_ws(lx);
    if (lx->src[lx->pos] != '{') {
        fprintf(stderr, "line %d: expected '{' after asm\n", lx->line);
        exit(1);
    }
    lx->pos++;
    size_t start = lx->pos;
    int depth = 1;
    while (lx->src[lx->pos] && depth) {
        char c = lx->src[lx->pos++];
        if (c == '\n') lx->line++;
        else if (c == '{') depth++;
        else if (c == '}') depth--;
    }
    if (depth) {
        fprintf(stderr, "line %d: unterminated asm block\n", lx->line);
        exit(1);
    }
    size_t end = lx->pos - 1;
    return xstrndup(lx->src + start, end - start);
}

void lexer_next(Lexer *lx) {
    skip_ws(lx);
    lx->tok.text = NULL;
    lx->tok.value = 0;
    lx->tok.line = lx->line;
    char c = lx->src[lx->pos];
    if (!c) { lx->tok.kind = TOK_EOF; lx->tok.text = xstrdup(""); return; }

    if (isid0((unsigned char)c)) {
        size_t start = lx->pos++;
        while (isid((unsigned char)lx->src[lx->pos])) lx->pos++;
        char *s = xstrndup(lx->src + start, lx->pos - start);
        if (!strcmp(s, "asm")) {
            free(s);
            lx->tok.kind = TOK_ASM;
            lx->tok.text = read_asm_block(lx);
            return;
        }
        lx->tok.kind = keyword(s);
        lx->tok.text = s;
        return;
    }

    if (isdigit((unsigned char)c)) {
        size_t start = lx->pos;
        if (c == '0' && (lx->src[lx->pos + 1] == 'x' || lx->src[lx->pos + 1] == 'X')) lx->pos += 2;
        else lx->pos++;
        while (isxdigit((unsigned char)lx->src[lx->pos])) lx->pos++;
        char *s = xstrndup(lx->src + start, lx->pos - start);
        lx->tok.kind = TOK_NUM;
        lx->tok.text = s;
        lx->tok.value = (int)strtol(s, NULL, 0) & 0xff;
        return;
    }

    if (c == '=' && lx->src[lx->pos + 1] == '=') { lx->pos += 2; lx->tok.kind = TOK_EQ; lx->tok.text = xstrdup("=="); return; }
    if (c == '!' && lx->src[lx->pos + 1] == '=') { lx->pos += 2; lx->tok.kind = TOK_NE; lx->tok.text = xstrdup("!="); return; }
    if (c == '<' && lx->src[lx->pos + 1] == '=') { lx->pos += 2; lx->tok.kind = TOK_LE; lx->tok.text = xstrdup("<="); return; }
    if (c == '>' && lx->src[lx->pos + 1] == '=') { lx->pos += 2; lx->tok.kind = TOK_GE; lx->tok.text = xstrdup(">="); return; }
    if (c == '>' && lx->src[lx->pos + 1] == '>') { lx->pos += 2; lx->tok.kind = TOK_SHR; lx->tok.text = xstrdup(">>"); return; }

    lx->pos++;
    lx->tok.kind = (unsigned char)c;
    lx->tok.text = xstrndup(&c, 1);
}
