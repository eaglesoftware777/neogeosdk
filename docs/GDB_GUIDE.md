# GDB Guide for NeoGeoSDK

> **v1.7.0 note**
>
> Debug builds work the same way for both engine variants.  With
> `USE_2D_PLUS=1`, the engine source files are `.cpp` and you can step
> through C++ methods directly; GDB will demangle the symbols for you.
>
> Set a breakpoint in a demo chapter with:
>
> ```
> break demo_unified.c:chap_mini_game
> break demo_plus_main.c:demo_plus_main_run
> ```

This guide covers how to use the cross-GDB (`m68k-unknown-elf-gdb` on Linux, `m68k-elf-gdb.exe` on Windows) with NeoGeoSDK builds.

---

## Overview

The NeoGeoSDK makefile produces a standard ELF object (`out/game`) from the 68000 source.  When built with `DEBUG=1`, it carries full DWARF-2 debug information including source line numbers, function names, and variable locations.

GDB can:

- read symbols from `out/game` without a connected target
- connect to MAME's built-in GDB stub and step live 68000 code
- run batch scripts to dump symbol tables, section layout, and function lists

---

## Installing GDB

### Linux / WSL

The cross-GDB is included in the `x-tools-v2` release asset alongside the compiler:

```text
$SDKHOME/x-tools-v2/m68k-unknown-elf/bin/m68k-unknown-elf-gdb
```

If the bundled GDB cannot start because of missing host libraries (common on newer Ubuntu), install a system GDB that understands the m68k ELF format:

```bash
sudo apt-get install gdb-multiarch
```

Then override the GDB path in the makefile:

```bash
make gdb-trace GDB=gdb-multiarch
make gdb GDB=gdb-multiarch
make gdb-remote GDB=gdb-multiarch GDB_REMOTE=localhost:1234
```

### Windows

On Windows, the makefile resolves `GDB` from the selected compiler prefix.
With `x-tools-v2-win`, it is typically one of:

```text
<sdk root>\x-tools-v2-win\m68k-unknown-elf\bin\m68k-unknown-elf-gdb.exe
<sdk root>\x-tools-v2-win\m68k-elf\bin\m68k-elf-gdb.exe
```

SysGCC fallback:

```text
C:\SysGCC\m68k-elf\bin\m68k-elf-gdb.exe
```

The Win32 makefile sets `GDB` automatically from `M68K_ELF_ROOT`.

---

## Debug build

Always build with `DEBUG=1` before using GDB:

```bash
make debug-build
```

```bat
make -f MakefileWin32.mak debug-build
```

This runs two steps:

1. `make DEBUG=1 p1` — compiles with `-g3 -gdwarf-2 -DNG_DEBUG=1` and links with a map file
2. `make DEBUG=1 debug-artifacts` — writes the following files to `dump/`:

| File                    | Contents                              |
|-------------------------|---------------------------------------|
| `dump/game.size.txt`    | section sizes from `size`             |
| `dump/game.sym`         | symbol table from `nm -n`             |
| `dump/game.readelf`     | full ELF metadata from `readelf -a`   |
| `dump/game.debug.dump`  | full disassembly with source from `objdump -DhtS` |
| `dump/game.map`         | linker map (sections and symbol addresses) |

---

## make gdb

Opens an interactive GDB session on `out/game` with no target connected.  Useful for inspecting symbols and disassembly offline:

```bash
make gdb
```

Inside GDB:

```gdb
(gdb) info functions
(gdb) info variables
(gdb) disassemble playgame
(gdb) x/32xw 0x100200
(gdb) quit
```

---

## make gdb-remote

Connects to a running target (typically MAME) that exposes a GDB stub.  MAME's GDB stub listens on `localhost:1234` by default:

```bash
make gdb-remote
```

Or with an explicit address:

```bash
make gdb-remote GDB_REMOTE=localhost:1234
```

Steps to use with MAME:

1. Start MAME with the debug flag:

```bash
mame -rompath roms -debug -window ssideki
```

2. In the MAME debugger window, type:

```
gdbstub
```

MAME now listens on `localhost:1234`.

3. In a second terminal, run:

```bash
make gdb-remote GDB_REMOTE=localhost:1234
```

4. GDB connects and you can set breakpoints, step, and inspect registers:

```gdb
(gdb) break playgame
(gdb) continue
(gdb) info registers
(gdb) stepi
(gdb) next
(gdb) print ng_level_state.scroll_x
```

---

## make gdb-trace

Runs GDB in batch mode and writes a full trace dump to `dump/gdb_trace.txt`:

```bash
make gdb-trace
```

The generated GDB script (`dump/gdb_trace.gdb`) runs:

```gdb
set pagination off
set confirm off
file out/game
info files
info functions
info variables
maintenance info sections
quit
```

Output goes to `dump/gdb_trace.txt`.  Any GDB startup errors go to `dump/gdb_trace.err`.

If GDB is unavailable, the target file contains a plain note instead of crashing the build.

---

## Useful GDB commands for 68000 NeoGeo targets

```gdb
info registers           # d0-d7, a0-a7, pc, sr
info registers d0        # single register
set $d0 = 0x1234         # write register
x/1xw 0x401000           # read VRAM register word
x/8xb 0x100200           # read 8 bytes from RAM
disassemble 0x1000, +64  # disassemble 16 instructions
break *0x1234            # breakpoint at address
watch *0x401800          # watchpoint on VRAM write
```

---

## Linker map

The linker map at `dump/game.map` (produced by `debug-build`) shows the exact address of every function and data object.  Use it alongside the symbol table:

```bash
grep playgame dump/game.map
grep ng_level_state dump/game.sym
```

---

## Overriding GDB path

Both makefiles accept a `GDB` variable:

```bash
make gdb GDB=/usr/bin/gdb-multiarch
make gdb-trace GDB=/usr/bin/gdb-multiarch
make gdb-remote GDB=gdb-multiarch GDB_REMOTE=localhost:1234
```

```bat
make -f MakefileWin32.mak gdb GDB=D:\tools\m68k-gdb.exe
```

---

## Notes

- The 68000 is big-endian; byte and word reads in GDB reflect that.
- MAME's GDB stub does not support all GDB features.  Single-step (`stepi`) and register reads work reliably.  Memory watchpoints may not.
- `out/game` is the ELF target used by GDB.  `out/game.rom` and `out/777-p1.p1` are the final ROM images for MAME; GDB does not load those directly.
