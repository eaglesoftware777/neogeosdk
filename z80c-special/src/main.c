#define _POSIX_C_SOURCE 200809L
#include "compiler.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct StrList { char **v; int n; int cap; } StrList;

typedef struct Options {
    const char *input;
    const char *output;
    const char *target;
    const char *assembler;
    int use_cpp;
    int emit_asm;
    int compile_object;
    int keep_asm;
    int opt_level;
    StrList defines;
    StrList includes;
} Options;

static void list_add(StrList *l, const char *s) {
    if (l->n == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 8;
        l->v = realloc(l->v, (size_t)l->cap * sizeof(l->v[0]));
        if (!l->v) { perror("realloc"); exit(1); }
    }
    l->v[l->n++] = xstrdup(s);
}

static char *read_stream(FILE *f) {
    size_t cap = 4096, len = 0;
    char *buf = xcalloc(cap, 1);
    for (;;) {
        if (len + 2048 > cap) {
            cap *= 2;
            buf = realloc(buf, cap);
            if (!buf) { perror("realloc"); exit(1); }
        }
        size_t n = fread(buf + len, 1, 2048, f);
        len += n;
        if (n < 2048) break;
    }
    buf[len] = 0;
    return buf;
}

static char *shell_quote(const char *s) {
    size_t cap = strlen(s) * 4 + 3, j = 0;
    char *out = xcalloc(cap, 1);
    out[j++] = '\'';
    for (size_t i = 0; s[i]; i++) {
        if (s[i] == '\'') { memcpy(out + j, "'\\''", 4); j += 4; }
        else out[j++] = s[i];
    }
    out[j++] = '\'';
    out[j] = 0;
    return out;
}

static char *path_without_ext(const char *path) {
    const char *base = strrchr(path, '/');
#ifdef _WIN32
    const char *b2 = strrchr(path, '\\');
    if (!base || (b2 && b2 > base)) base = b2;
#endif
    base = base ? base + 1 : path;
    const char *dot = strrchr(base, '.');
    size_t n = dot ? (size_t)(dot - base) : strlen(base);
    return xstrndup(base, n);
}

static char *replace_ext(const char *path, const char *ext) {
    char *stem = path_without_ext(path);
    size_t n = strlen(stem) + strlen(ext) + 1;
    char *out = xcalloc(n, 1);
    snprintf(out, n, "%s%s", stem, ext);
    free(stem);
    return out;
}

static void append_quoted(char **cmd, size_t *cap, size_t *len, const char *arg) {
    char *q = shell_quote(arg);
    size_t need = *len + strlen(q) + 2;
    if (need > *cap) {
        while (need > *cap) *cap *= 2;
        *cmd = realloc(*cmd, *cap);
        if (!*cmd) { perror("realloc"); exit(1); }
    }
    (*cmd)[(*len)++] = ' ';
    strcpy(*cmd + *len, q);
    *len += strlen(q);
    free(q);
}

static char *preprocess(const Options *opt) {
    if (!opt->use_cpp) {
        FILE *f = fopen(opt->input, "rb");
        if (!f) { perror(opt->input); exit(1); }
        char *s = read_stream(f);
        fclose(f);
        return s;
    }

    size_t cap = 4096, len = 0;
    char *cmd = xcalloc(cap, 1);
    strcpy(cmd, "cc -E -P -x c");
    len = strlen(cmd);

    if (opt->target && !strcmp(opt->target, "neogeo")) {
        append_quoted(&cmd, &cap, &len, "-D__NEOGEO__=1");
        append_quoted(&cmd, &cap, &len, "-Iinclude");
    }
    for (int i = 0; i < opt->includes.n; i++) {
        char *flag = xcalloc(strlen(opt->includes.v[i]) + 3, 1);
        sprintf(flag, "-I%s", opt->includes.v[i]);
        append_quoted(&cmd, &cap, &len, flag);
        free(flag);
    }
    for (int i = 0; i < opt->defines.n; i++) {
        char *flag = xcalloc(strlen(opt->defines.v[i]) + 3, 1);
        sprintf(flag, "-D%s", opt->defines.v[i]);
        append_quoted(&cmd, &cap, &len, flag);
        free(flag);
    }
    append_quoted(&cmd, &cap, &len, opt->input);

    FILE *pp = popen(cmd, "r");
    if (!pp) { perror("popen"); exit(1); }
    char *src = read_stream(pp);
    int rc = pclose(pp);
    if (rc != 0) { fprintf(stderr, "preprocessor failed for %s\n", opt->input); exit(1); }
    free(cmd);
    return src;
}

static void write_asm_file(const Options *opt, const char *asm_path) {
    char *src = preprocess(opt);
    Program *p = parse_program(src);
    FILE *old = stdout;
    FILE *out = fopen(asm_path, "wb");
    if (!out) { perror(asm_path); exit(1); }
    stdout = out;
    emit_program(p);
    fflush(out);
    stdout = old;
    fclose(out);
    free(src);
}

static int run_assembler(const Options *opt, const char *asm_path, const char *obj_path) {
    size_t cap = 1024, len = 0;
    char *cmd = xcalloc(cap, 1);
    snprintf(cmd, cap, "%s", opt->assembler);
    len = strlen(cmd);
    for (int i = 0; i < opt->includes.n; i++) {
        append_quoted(&cmd, &cap, &len, "-I");
        append_quoted(&cmd, &cap, &len, opt->includes.v[i]);
    }
    append_quoted(&cmd, &cap, &len, "-o");
    append_quoted(&cmd, &cap, &len, obj_path);
    append_quoted(&cmd, &cap, &len, asm_path);
    int rc = system(cmd);
    free(cmd);
    return rc == -1 ? 1 : WEXITSTATUS(rc);
}

static void usage(void) {
    fprintf(stderr,
        "usage: z80cc [options] file.c\n"
        "  -S                 emit assembly\n"
        "  -c                 assemble object with wla-z80\n"
        "  -o FILE            output file\n"
        "  -DNAME[=VALUE]     define macro for C preprocessor\n"
        "  -IDIR              add include directory\n"
        "  --target neogeo    enable Neo Geo defaults\n"
        "  --asm TOOL         assembler command for -c\n"
        "  -O0..-O3           select peephole optimization level\n"
        "  --keep-asm         keep generated assembly for -c\n"
        "  --no-cpp           parse source without preprocessing\n");
}

static void parse_args(Options *opt, int argc, char **argv) {
    opt->use_cpp = 1;
    opt->assembler = "wla-z80";
    opt->target = "z80";
    opt->opt_level = 2;
    for (int i = 1; i < argc; i++) {
        char *a = argv[i];
        if (!strcmp(a, "--no-cpp")) opt->use_cpp = 0;
        else if (!strcmp(a, "-S")) opt->emit_asm = 1;
        else if (!strcmp(a, "-c")) opt->compile_object = 1;
        else if (!strcmp(a, "--keep-asm")) opt->keep_asm = 1;
        else if (!strncmp(a, "-O", 2) && a[2]) opt->opt_level = atoi(a + 2);
        else if (!strcmp(a, "-o") && i + 1 < argc) opt->output = argv[++i];
        else if (!strcmp(a, "--target") && i + 1 < argc) opt->target = argv[++i];
        else if (!strcmp(a, "--asm") && i + 1 < argc) opt->assembler = argv[++i];
        else if (!strncmp(a, "-D", 2) && a[2]) list_add(&opt->defines, a + 2);
        else if (!strcmp(a, "-D") && i + 1 < argc) list_add(&opt->defines, argv[++i]);
        else if (!strncmp(a, "-I", 2) && a[2]) list_add(&opt->includes, a + 2);
        else if (!strcmp(a, "-I") && i + 1 < argc) list_add(&opt->includes, argv[++i]);
        else if (a[0] == '-') { usage(); exit(2); }
        else opt->input = a;
    }
    if (!opt->input) { usage(); exit(2); }
    if (!strcmp(opt->target, "neogeo")) {
        list_add(&opt->defines, "NEOGEO=1");
    }
}

int main(int argc, char **argv) {
    Options opt = {0};
    parse_args(&opt, argc, argv);

    if (opt.compile_object) {
        char *asm_path = opt.keep_asm ? replace_ext(opt.input, ".asm") : replace_ext(opt.input, ".tmp.asm");
        char *obj_path = opt.output ? xstrdup(opt.output) : replace_ext(opt.input, ".o");
        write_asm_file(&opt, asm_path);
        optimize_asm_file(asm_path, opt.opt_level);
        int rc = run_assembler(&opt, asm_path, obj_path);
        if (!opt.keep_asm) remove(asm_path);
        free(asm_path);
        free(obj_path);
        return rc;
    }

    if (opt.emit_asm || opt.output) {
        char *asm_path = opt.output ? xstrdup(opt.output) : replace_ext(opt.input, ".asm");
        write_asm_file(&opt, asm_path);
        optimize_asm_file(asm_path, opt.opt_level);
        free(asm_path);
        return 0;
    }

    char *src = preprocess(&opt);
    Program *p = parse_program(src);
    emit_program(p);
    free(src);
    return 0;
}
