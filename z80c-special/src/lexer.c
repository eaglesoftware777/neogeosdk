#include "compiler.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int isid0(int c) { return isalpha(c) || c == '_'; }
static int isid(int c) { return isalnum(c) || c == '_'; }

void lexer_init(Lexer *lx, const char *src) {
    lx->src = src;
    lx->pos = 0;
    lx->line = 1;
}

static void read_asm_block(Lexer *lx) {
    size_t start = lx->pos;
    int depth = 1;
    while (lx->src[lx->pos] && depth > 0) {
        if (lx->src[lx->pos] == '{') depth++;
        if (lx->src[lx->pos] == '}') depth--;
        if (lx->src[lx->pos] == '\n') lx->line++;
        if (depth > 0) lx->pos++;
    }
    lx->tok.kind = TOK_ASM;
    lx->tok.text = xstrndup(lx->src + start, lx->pos - start);
    if (lx->src[lx->pos] == '}') lx->pos++;
}

void lexer_next(Lexer *lx) {
    while (isspace(lx->src[lx->pos])) {
        if (lx->src[lx->pos] == '\n') lx->line++;
        lx->pos++;
    }

    if (!lx->src[lx->pos]) {
        lx->tok.kind = TOK_EOF;
        lx->tok.text = "";
        return;
    }

    lx->tok.line = lx->line;
    int c = (unsigned char)lx->src[lx->pos];

    if (isdigit(c)) {
        char *end;
        lx->tok.value = strtol(lx->src + lx->pos, &end, 0);
        lx->tok.text = xstrndup(lx->src + lx->pos, end - (lx->src + lx->pos));
        lx->tok.kind = TOK_NUM;
        lx->pos = end - lx->src;
        return;
    }

    if (isid0(c)) {
        size_t start = lx->pos;
        while (isid(lx->src[lx->pos])) lx->pos++;
        lx->tok.text = xstrndup(lx->src + start, lx->pos - start);
        lx->tok.kind = TOK_ID;
        if (!strcmp(lx->tok.text, "void")) lx->tok.kind = TOK_VOID;
        if (!strcmp(lx->tok.text, "unsigned")) lx->tok.kind = TOK_UNSIGNED;
        if (!strcmp(lx->tok.text, "char")) lx->tok.kind = TOK_CHAR;
        if (!strcmp(lx->tok.text, "if")) lx->tok.kind = TOK_IF;
        if (!strcmp(lx->tok.text, "else")) lx->tok.kind = TOK_ELSE;
        if (!strcmp(lx->tok.text, "while")) lx->tok.kind = TOK_WHILE;
        if (!strcmp(lx->tok.text, "return")) lx->tok.kind = TOK_RETURN;
        if (!strcmp(lx->tok.text, "asm")) {
            while (isspace(lx->src[lx->pos])) {
                if (lx->src[lx->pos] == '\n') lx->line++;
                lx->pos++;
            }
            if (lx->src[lx->pos] == '{') {
                lx->pos++;
                read_asm_block(lx);
            }
        }
        if (!strcmp(lx->tok.text, "extern")) lx->tok.kind = TOK_EXTERN;
        return;
    }

    if (c == '/' && lx->src[lx->pos + 1] == '/') {
        while (lx->src[lx->pos] && lx->src[lx->pos] != '\n') lx->pos++;
        lexer_next(lx);
        return;
    }

    if (c == '=' && lx->src[lx->pos + 1] == '=') { lx->pos += 2; lx->tok.kind = TOK_EQ; lx->tok.text = xstrdup("=="); return; }
    if (c == '!' && lx->src[lx->pos + 1] == '=') { lx->pos += 2; lx->tok.kind = TOK_NE; lx->tok.text = xstrdup("!="); return; }
    if (c == '<' && lx->src[lx->pos + 1] == '=') { lx->pos += 2; lx->tok.kind = TOK_LE; lx->tok.text = xstrdup("<="); return; }
    if (c == '<' && lx->src[lx->pos + 1] == '<') { lx->pos += 2; lx->tok.kind = TOK_SHL; lx->tok.text = xstrdup("<<"); return; }
    if (c == '>' && lx->src[lx->pos + 1] == '=') { lx->pos += 2; lx->tok.kind = TOK_GE; lx->tok.text = xstrdup(">="); return; }
    if (c == '>' && lx->src[lx->pos + 1] == '>') { lx->pos += 2; lx->tok.kind = TOK_SHR; lx->tok.text = xstrdup(">>"); return; }

    lx->pos++;
    lx->tok.kind = (unsigned char)c;
    lx->tok.text = xstrndup((char *)&c, 1);
}
