# Build Dependencies

> **v1.7.0 — extra dependencies**
>
> - The HD artbox alt scripts (`artbox/img2neo_hd.py`,
>   `artbox/fixtiles_hd.py`) require the same packages as the main
>   pipeline (`numpy`, `pypng`, `Pillow`).  No new dependencies, but
>   they're slower because of the bilateral filter + CLAHE numpy passes.
> - The C++ engine (`USE_2D_PLUS=1`) is built by the same
>   `m68k-unknown-elf-g++` shipped with `x-tools-v2`.  No extra toolchain.

This document lists every tool and library required to build the NeoGeoSDK on Linux, WSL, and Windows.

---

## Linux / WSL

### System packages

Install with `apt-get` (Ubuntu / Debian):

```bash
sudo apt-get update
sudo apt-get install \
    git \
    mame \
    srecord \
    cmake \
    build-essential \
    python3 \
    python3-pip \
    python3-numpy \
    python3-pil \
    sqlite3
```

`sox` is optional.  The build falls back to the bundled Python converter when it is not present:

```bash
sudo apt-get install sox   # optional
```

### Python packages

```bash
python3 -m pip install --user pypng
```

Core modules used by the build scripts:

| Module     | Used by                                   | Install via               |
|------------|-------------------------------------------|---------------------------|
| `PIL`      | `artbox/img2neo.py`, `artbox/romtiles.py` | `python3-pil` / `pillow`  |
| `numpy`    | `artbox/img2neo.py`                       | `python3-numpy`           |
| `pypng`    | `artbox/romdbimgimport.py`                | `pip install pypng`       |
| `sqlite3`  | `artbox/neorom.db` tooling                | stdlib (always available) |
| `wave`     | sound sample tools                        | stdlib                    |
| `struct`   | sound sample tools                        | stdlib                    |
| `argparse` | all CLI tools                             | stdlib                    |
| `pathlib`  | all CLI tools                             | stdlib                    |
| `json`     | `artbox/asset_rules.py`, manifest tools   | stdlib                    |

### WLA-DX (Z80 assembler)

Build from source:

```bash
git clone https://github.com/vhelin/wla-dx
cd wla-dx
cmake -S . -B build
cmake --build build -j
sudo cp build/binaries/wla-z80 build/binaries/wlalink /usr/local/bin/
```

Verify:

```bash
wla-z80 --version
wlalink --version
```

### m68k-unknown-elf toolchain (Linux)

Download the pre-built `x-tools-v2.tar` from the release page and place it next to the repository:

```bash
mkdir -p $HOME/neogeo
cd $HOME/neogeo
curl -L -o x-tools-v2.tar https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.7.0/x-tools-v2.tar
tar -xf x-tools-v2.tar
```

Expected layout:

```text
$HOME/neogeo/
  neogeosdk/
  x-tools-v2/
    m68k-unknown-elf/
      bin/
        m68k-unknown-elf-gcc
        m68k-unknown-elf-ld
        m68k-unknown-elf-objcopy
        m68k-unknown-elf-objdump
        m68k-unknown-elf-gdb
        m68k-unknown-elf-nm
        m68k-unknown-elf-readelf
        m68k-unknown-elf-addr2line
        m68k-unknown-elf-size
```

Verify:

```bash
export SDKHOME=$HOME/neogeo
$SDKHOME/x-tools-v2/m68k-unknown-elf/bin/m68k-unknown-elf-gcc --version

The Linux makefile fallback order is:
1. `$SDKHOME/x-tools-v2`
2. `$SDKHOME/x-tools` (legacy)
```

### MAME

```bash
sudo apt-get install mame
```

Or build from source: https://www.mamedev.org/

---

## Windows

### m68k toolchain (Windows)

The Win32 makefile (`MakefileWin32.mak`) auto-detects this default first:

```text
<sdk root>\x-tools-v2-win\m68k-unknown-elf\bin\m68k-unknown-elf-gcc.exe
```

Then this alternate default:

```text
<sdk root>\x-tools-v2-win\m68k-elf\bin\m68k-elf-gcc.exe
```

If neither exists, it falls back to a SysGCC-style `M68K_ELF_ROOT`:

```text
C:\SysGCC\m68k-elf\bin\m68k-elf-gcc.exe
C:\SysGCC\m68k-elf\bin\m68k-elf-ld.exe
C:\SysGCC\m68k-elf\bin\m68k-elf-objcopy.exe
C:\SysGCC\m68k-elf\bin\m68k-elf-objdump.exe
C:\SysGCC\m68k-elf\bin\m68k-elf-gdb.exe
C:\SysGCC\m68k-elf\bin\m68k-elf-nm.exe
C:\SysGCC\m68k-elf\bin\m68k-elf-readelf.exe
C:\SysGCC\m68k-elf\bin\m68k-elf-addr2line.exe
C:\SysGCC\m68k-elf\bin\m68k-elf-size.exe
```

If your toolchain root differs, pass it on the command line:

```bat
make -f MakefileWin32.mak M68K_ELF_ROOT=D:\toolchains\m68k-elf all
```

### Python

Install Python 3 from https://www.python.org/ or the Microsoft Store.  Make sure the `py` launcher is available:

```bat
py --version
```

Install required packages:

```bat
py -0p
py -m pip --version
py -m pip install --upgrade pip
py -m pip install numpy pillow pypng
```

The same module table applies as on Linux (see above).

Important: use `py -m pip` instead of `pip` or `pip3`. The Windows build
scripts call `py`, so using the same launcher avoids interpreter/package
mismatch errors.

Quick verification:

```bat
cd artbox
py -c "import sys; print(sys.executable); import PIL, numpy, png, img2neo; print('OK')"
```

### GNU Make

Use the `make.exe` that ships with your toolchain bundle, or install from:

- https://gnuwin32.sourceforge.net/packages/make.htm
- https://www.msys2.org/

Verify:

```bat
make --version
```

### WLA-DX (Windows)

Build from source with CMake and a MinGW or MSVC toolchain, or download a pre-built binary from:

- https://github.com/vhelin/wla-dx/releases

Place `wla-z80.exe` and `wlalink.exe` in a directory on `PATH`.

### srec_cat (Windows)

The Windows build includes a pre-built `win\srec_cat.exe` in the repository.  No separate installation is needed.

### xxd (Windows)

A pre-built `win\xxd.exe` is also included.

### MAME (Windows)

Download from https://www.mamedev.org/ and place `mame.exe` on `PATH`.

### sox (Windows)

Optional.  If not present, the bundled Python WAV converter is used automatically.  If you want SoX:

- https://sourceforge.net/projects/sox/files/sox/

Place `sox.exe` on `PATH` and pass it to make:

```bat
make -f MakefileWin32.mak SOX=sox samples
```

---

## Bundled tools (no install needed)

| File                          | Purpose                               |
|-------------------------------|---------------------------------------|
| `win/srec_cat.exe`            | ROM byte manipulation (Windows)       |
| `win/xxd.exe`                 | ROM hex dump (Windows)                |
| `sound/tools/wav_to_raw_pcm.py` | Python WAV converter fallback       |
| `sound/tools/*.py`            | ADPCM encoding, MML compile, FM tools |
| `artbox/*.py`                 | Graphics pipeline tools               |

---

## Version summary

| Dependency              | Minimum version tested |
|-------------------------|------------------------|
| GCC (m68k cross)        | 12.x                   |
| WLA-DX                  | 10.x                   |
| Python                  | 3.9                    |
| Pillow                  | 9.x                    |
| numpy                   | 1.21                   |
| MAME                    | 0.250                  |
| srec_cat                | 1.64                   |
| GNU Make                | 4.x                    |
