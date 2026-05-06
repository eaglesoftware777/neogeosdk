# z80c-special

A small experimental Z80 C compiler for compact sound-driver style code.

The implementation is now written in C and organized around a Dragon Book style front end: preprocessing, lexical analysis, parsing into an AST, semantic checks where the target needs them, then Z80 assembly emission. A Bison grammar lives in `src/z80cc.y`; the repository also carries a portable C parser so the compiler builds even on systems where `bison` or `yacc` is not installed.

The design stays close to old CP/M Small C practice: keep the language small, make generated assembly readable, and allow direct assembly where the target hardware needs exact instruction sequences.

## Build

```sh
make
```

This creates `build/z80cc`.

The command line is intentionally closer to a small `zcc`-style front end:

```sh
build/z80cc -S -o build/game.asm source.c
build/z80cc -c -o build/game.o source.c
build/z80cc --target neogeo -DDEBUG=1 -Iinclude -S -o build/game.asm source.c
```

`-S` emits assembly, `-c` assembles through `wla-z80`, `-o` selects the output path, `-D` and `-I` are passed to the C preprocessor, `-O0` through `-O3` select peephole optimization, and `--target neogeo` enables Neo Geo defaults and the bundled include path.

If Bison is installed, the grammar can be checked or regenerated with:

```sh
make grammar
```

## Supported C Subset

- `void` and `unsigned char`
- global byte variables and byte parameters
- functions, calls, assignment, `if`, `else`, `while`, `return`
- byte constants, identifiers, `+`, `-`, `&`, `|`, `^`, `==`, `!=`, `<`, `>`, `<=`, `>=`
- builtins: `out(port, value)`, `in(port)`, `halt()`, `di()`, `ei()`
- embedded Z80 assembly blocks with `asm { ... }` at file scope or inside functions
- C preprocessor macros through `cc -E -P -x c` before parsing
- z88dk-style front-end flow: preprocess, compile, optimize, optionally assemble, and target-specific defaults
- Neo Geo target helpers in `include/neogeo.h`

## Examples

```sh
build/z80cc -S -o build/tone.asm examples/tone.c
wla-z80 -o build/tone.o build/tone.asm

build/z80cc -S -o build/neo_sound_driver.asm examples/driver/neo_sound_driver.c
wla-z80 -o build/neo_sound_driver.o build/neo_sound_driver.asm

build/z80cc --target neogeo -S -o build/neogeo_demo.asm examples/neogeo/command_demo.c
wla-z80 -o build/neogeo_demo.o build/neogeo_demo.asm

build/z80cc --target neogeo -c --keep-asm -o build/neogeo_demo_c.o examples/neogeo/command_demo.c
```

`examples/driver/neo_sound_driver.c` is a hybrid Neo Geo sound-driver core. C handles command dispatch and sample selection, while embedded assembly owns reset/NMI vectors and precise YM2610 port writes. `examples/neogeo/command_demo.c` shows the target include and SDK-style macros for YM2610 access.

Preprocessor macros work for constants and SDK-style call wrappers:

```c
#define SAMPLE0 0x28
#define PLAY_B0() play(SAMPLE0)
```

For multi-instruction hardware sequences, prefer `asm { ... }` blocks so instruction boundaries remain explicit in the generated WLA-Z80 assembly.
