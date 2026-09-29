# Eagle Software - NeoSD ROM Packager & Converter (`eagle-neosd`)

High-performance, standalone toolchain specifically designed for Eagle Software homebrews and Neo Geo ROM sets. Converts loose ROM chips (`.p1`, `.s1`, `.m1`, `.v1`, `.v2`, `.c1`, `.c2`) into official NeoSD (`.neo`) flashcart images, and vice versa.

---

## Key Capabilities & Features

1. **Bidirectional Conversion**:
   - **Pack**: Loose ROMs -> `.neo` (with automated C-ROM byte interleaving).
   - **Extract / Unpack**: `.neo` -> Loose ROMs (with automated C-ROM de-interleaving).
   - **Inspect**: Detailed breakdown of `.neo` headers, checksums, region sizes, and metadata.
2. **Eagle Software Integration**:
   - Automatic game profile detection from P-ROM headers.
   - Built-in recognition of Eagle Software NGH IDs (e.g., Maiya `0x8007`, Demo `0x8006`).
   - Supports both Eagle file naming (`<id>-p1.p1`) and standard MAME naming (`*.p1`, `*.rom`).
3. **Dual Interface**:
   - Ultra-fast native C CLI (`eagle_neosd`).
   - Responsive Local GUI (`eagle_neosd_gui.py`) accessible via any browser with zero external dependencies, or native desktop Tkinter.
4. **Standalone Automation**:
   - `build_and_run.sh` builds the C engine and executes automated byte-level verification tests.

---

## Directory Structure

```
tools/eagle_neosd/
├── eagle_neosd.h       # NeoSD header definitions and data structures
├── eagle_neosd.c       # Fast C11 engine with CRC-32 and interleaver
├── eagle_neosd         # Compiled Linux executable
├── eagle_neosd_gui.py  # Zero-dependency local Web & Tkinter GUI
├── build_and_run.sh    # Standalone build, test, and runner script
└── README.md           # Documentation
```

---

## Command Line Usage (CLI)

### Quick Build & Self-Test
Run the standalone script to build the tool and verify integrity:
```bash
./tools/eagle_neosd/build_and_run.sh
```

### Pack Directory to `.neo`
```bash
# Basic packaging (auto-detects title, NGH ID, and ROM chips):
./tools/eagle_neosd/eagle_neosd -i roms/maiya -o roms/maiya.neo

# Explicit metadata options:
./tools/eagle_neosd/eagle_neosd -i roms/maiya -o roms/maiya.neo \
    -n "Maiya: Super Nature Girl" \
    -m "Eagle Software" \
    -y 2026 \
    -g Platformer \
    --ngh 0x8007
```

### Inspect `.neo` File
```bash
./tools/eagle_neosd/eagle_neosd --info roms/maiya.neo
```

### Extract / Unpack `.neo` to Loose ROMs
```bash
./tools/eagle_neosd/eagle_neosd -x roms/maiya.neo -o unpacked_roms/
```

---

## Graphical User Interface (GUI)

The GUI requires **no pip packages** and runs on any standard Python 3 installation.

### Launch GUI
```bash
# Using the build script:
./tools/eagle_neosd/build_and_run.sh --gui

# Or directly:
python3 tools/eagle_neosd/eagle_neosd_gui.py
```

Open `http://localhost:7800` in your web browser. You can:
- Select quick presets for Eagle games (**Maiya**, **Demo**).
- Point to any ROM directory and scan detected files.
- Package directly into `.neo` with live console feedback.
- Inspect and extract `.neo` files with a single click.

---

## Windows CMD / PowerShell Usage

If building or running natively on Windows:

### Compile with GCC (MinGW / MSYS2 / Clang)
```cmd
gcc -O3 -Wall -Wextra tools\eagle_neosd\eagle_neosd.c -o tools\eagle_neosd\eagle_neosd.exe
```

### Run Pack from CMD
```cmd
tools\eagle_neosd\eagle_neosd.exe -i roms\maiya -o roms\maiya.neo
```

### Run GUI from CMD
```cmd
python tools\eagle_neosd\eagle_neosd_gui.py
```
