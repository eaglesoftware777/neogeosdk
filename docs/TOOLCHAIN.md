# NeoGeo SDK toolchain 3.0

The `x-tools-v3` bundle is the compiler, binutils, C and C++ libraries and
debugger the SDK builds with. It targets the Motorola 68000 in the Neo Geo
and runs on any x86-64 Linux: every host program in it is statically
linked and needs no shared library, no interpreter and no package from the
machine it runs on.

## Contents

| Component | Version | Notes |
|---|---|---|
| GCC | 16.2.0 | C and C++ front ends |
| GNU binutils | 2.47 | `as`, `ld`, `objcopy`, `objdump`, `nm`, `size`, `readelf`, `strip`, `gprof`, `lto-dump` |
| GNU gdb | 17.2 | cross debugger with TUI, XML target descriptions, internal readline |
| newlib | 4.6.0 | C library, single-threaded, nano malloc, C99 and `long long` formatted I/O, floating-point formatted I/O |
| libstdc++ | 16.2.0 | full C++ standard library, quiet `terminate` (no demangler or stdio pulled in) |
| libgcc | 16.2.0 | software integer division, multiplication and floating point for the 68000 |

Target triplet: `m68k-unknown-elf`.

| Bundle | Host | Archive | Installed |
|---|---|---|---|
| `x-tools-v3` | x86-64 Linux, static binaries | `x-tools-v3.tar.xz`, 58 MB | 280 MB |
| `x-tools-v3-win` | 64-bit Windows, `.exe` files importing only system DLLs | `x-tools-v3-win.zip`, 106 MB | 250 MB |

Both bundles are built from the same sources with the same configuration
and produce byte-identical ROMs from the same input.

## Code generation

- Default CPU `-mcpu=68000`, so a build that names no CPU still produces
  code the Neo Geo can run. `-m68020` and later are accepted for other
  boards; the libraries link against those too, being 68000 code.
- Single library set built for the 68000 with `-Os`. No multilib.
- Link-time optimisation: `-flto` is available for compile and link. The
  linker has no plugin support (a consequence of static host binaries), so
  archives of LTO objects should be built with `-ffat-lto-objects`; objects
  named on the link line are optimised across translation units.
- Graphite loop optimisations are available.
- No TLS, no shared libraries, no thread model, no SSP, no quadmath.

## Language standards

| Language | Default | Supported through | Verified here |
|---|---|---|---|
| C | `gnu23` | C23 (`_BitInt` excepted: not available on this target) | `-std=gnu99`, `-std=c23` |
| C++ | `gnu++17` | C++26 | `-std=c++14`, `-std=c++23`, `-std=c++26` in freestanding mode with `-fno-exceptions -fno-rtti` |

The C++ headers (`<array>`, `<span>`, `<bit>`, `<algorithm>`, `<type_traits>`,
`<ranges>`, `<utility>` and the rest) are usable with `-ffreestanding` and
no runtime support beyond what the SDK already provides.

## Linking without an operating system

The SDK links with `-nostdlib` and supplies its own start-up code, so it
uses nothing below except `libgcc`. For programs that want the C library:
`-lc` provides `libc.a` (with `libg.a` and `libm.a` beside it); newlib's
own system-call stubs are not built in, so a program that reaches the OS
layer (`write`, `sbrk`, `exit`...) defines those functions itself, links
`-lnosys`, or passes `--specs=nosys.specs`. `crt0.o` and the `bcc`, `idp` and `idpgdb` board scripts from
libgloss are installed for reference.

## Layout

```
x-tools-v3/
  m68k-unknown-elf/
    bin/                        m68k-unknown-elf-gcc, -g++, -ld, -gdb, ...
    lib/gcc/m68k-unknown-elf/16.2.0/
                                libgcc.a, crtbegin.o, crtend.o, headers
    m68k-unknown-elf/include/   C library headers, c++/16.2.0/
    m68k-unknown-elf/lib/       libc.a, libg.a, libm.a, libstdc++.a,
                                libsupc++.a, libnosys.a, ldscripts/
    share/gdb/                  gdb data directory (relocatable)
    share/licenses/             licences of every component
```

The bundle is relocatable: unpack it anywhere and point `SDKHOME` (or
`XTOOLS_ROOT`) at its parent.

## Use with the SDK

`Makefile` looks for the toolchain in this order and takes the first found:

1. `$(SDKHOME)/x-tools-v3`
2. `$(SDKHOME)/x-tools-v2`
3. `$(SDKHOME)/x-tools`

or set `XTOOLS_ROOT=<directory containing m68k-unknown-elf/>` on the make
command line.

`MakefileWin32.mak` looks for `x-tools-v3-win` first, then `x-tools-v2-win`,
then `M68K_ELF_ROOT`. Its link step now hands the linker forward-slash object
paths, because the linker script places sections by object file name
(`out/ng_*0.o`) and backslash spellings never matched those patterns; with
that, a Windows build of any game is byte-identical to the Linux build.

## Verified

- All six games in the tree build with no warnings: `demo`, `demo_plus`
  (the C++ engine, `-std=c++14`), `skylance`, `neogeogame`, `helloworld`,
  `tutorial`. The demo's C build is 369,785 bytes of text against 377,973
  from the previous toolchain at the same `-O0`.
- The demo ROM set built with it passes the full 26-chapter capture tour
  with VRAM and palette RAM checked against the manifest, the controller
  run, and the live audio check.
- `-flto` links a multi-file program with cross-unit inlining.
- `gdb` loads the demo's ELF, resolves symbols and disassembles 68000 code.
- The Windows bundle builds all six games through `MakefileWin32.mak` with
  no warnings; every program ROM matches the Linux build byte for byte, and
  every `.exe` imports only `KERNEL32`, `msvcrt`, `ADVAPI32`, `USER32`,
  `WS2_32` and `bcrypt`.
