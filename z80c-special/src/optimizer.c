#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_all(FILE *f) {
    size_t cap = 8192, len = 0;
    char *buf = xcalloc(cap, 1);
    for (;;) {
        if (len + 4096 > cap) {
            cap *= 2;
            buf = realloc(buf, cap);
            if (!buf) { perror("realloc"); exit(1); }
        }
        size_t n = fread(buf + len, 1, 4096, f);
        len += n;
        if (n < 4096) break;
    }
    buf[len] = 0;
    return buf;
}

static void write_text(const char *path, const char *s) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fputs(s, f);
    fclose(f);
}

static char *replace_all(char *src, const char *from, const char *to) {
    size_t from_len = strlen(from), to_len = strlen(to);
    size_t count = 0;
    for (char *p = strstr(src, from); p; p = strstr(p + from_len, from)) count++;
    if (!count) return src;
    size_t out_len = strlen(src) + count * (to_len - from_len) + 1;
    char *out = xcalloc(out_len, 1);
    char *w = out;
    char *p = src;
    char *m;
    while ((m = strstr(p, from)) != NULL) {
        size_t n = (size_t)(m - p);
        memcpy(w, p, n); w += n;
        memcpy(w, to, to_len); w += to_len;
        p = m + from_len;
    }
    strcpy(w, p);
    free(src);
    return out;
}

void optimize_asm_file(const char *path, int level) {
    if (level <= 0) return;
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    char *s = read_all(f);
    fclose(f);

    s = replace_all(s, "    ld a,$00\n    or a\n", "    xor a\n    or a\n");
    s = replace_all(s, "    ld a,$00\n", "    xor a\n");
    s = replace_all(s, "    ld a,$01\n", "    ld a,1\n");
    s = replace_all(s, "    jr .ifend", "    jp .ifend");

    if (level >= 2) {
        s = replace_all(s, "    push af\n    ld a,$00\n    ld b,a\n    pop af\n", "    ld b,$00\n");
        s = replace_all(s, "    push af\n    ld a,$01\n    ld b,a\n    pop af\n", "    ld b,$01\n");
        s = replace_all(s, "    push af\n    ld a,$80\n    ld b,a\n    pop af\n", "    ld b,$80\n");
    }

    write_text(path, s);
    free(s);
}
