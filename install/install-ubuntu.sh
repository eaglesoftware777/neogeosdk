#!/usr/bin/env bash
# NeoGeo SDK — Ubuntu / Debian installer.
#
# Apt-targeted fast path matching the line in README.md.  Same end-state as
# install-linux.sh but skips the distro auto-detect and uses the exact
# package names the README documents (python3-pil, srecord, sox, mame).
# Tested on Ubuntu 22.04 + 24.04.
#
# Usage:
#   install-ubuntu.sh [--prefix DIR] [--no-toolchain] [--no-wladx] [--yes]
#
#   --prefix DIR     Root that holds neogeosdk/ + x-tools-v3/ (default: $HOME/neogeo)
#   --no-toolchain   Skip the m68k cross toolchain download
#   --no-wladx       Skip the WLA-DX build
#   --yes            Pass -y to apt (non-interactive)

set -euo pipefail

PREFIX="${SDKHOME:-$HOME/neogeo}"
DO_TOOLCHAIN=1
DO_WLADX=1
APT_FLAGS=""

while [ $# -gt 0 ]; do
    case "$1" in
        --prefix)         PREFIX="$2"; shift 2 ;;
        --no-toolchain)   DO_TOOLCHAIN=0; shift ;;
        --no-wladx)       DO_WLADX=0; shift ;;
        --yes|-y)         APT_FLAGS="-y"; shift ;;
        -h|--help)
            sed -n '2,15p' "$0" | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *)  echo "unknown argument: $1" >&2; exit 1 ;;
    esac
done

log() { printf '\n[install] %s\n' "$*"; }
die() { printf '[install] ERROR: %s\n' "$*" >&2; exit 2; }

if ! command -v apt-get >/dev/null 2>&1; then
    die "apt-get not found — this script is Ubuntu/Debian specific.  Use install-linux.sh for other distros."
fi

SUDO=""
if [ "$(id -u)" -ne 0 ]; then
    command -v sudo >/dev/null 2>&1 || die "sudo not found and not running as root"
    SUDO="sudo"
fi

# ---------------------------------------------------------------------------
# 1. apt packages — exact set the README documents
# ---------------------------------------------------------------------------
log "installing apt packages..."
$SUDO apt-get update
$SUDO apt-get install $APT_FLAGS \
    git curl ca-certificates \
    build-essential cmake make srecord \
    python3 python3-pip python3-numpy python3-pil python3-venv \
    sqlite3 sox mame

log "installing pypng via pip..."
python3 -m pip install --user --upgrade pypng

# ---------------------------------------------------------------------------
# 2. WLA-DX
# ---------------------------------------------------------------------------
if [ "$DO_WLADX" -eq 1 ]; then
    if command -v wla-z80 >/dev/null 2>&1 && command -v wlalink >/dev/null 2>&1; then
        log "WLA-DX already installed; skipping"
    else
        log "building WLA-DX from source..."
        WLA_TMP="$(mktemp -d)"
        trap 'rm -rf "$WLA_TMP"' EXIT
        git clone --depth 1 https://github.com/vhelin/wla-dx "$WLA_TMP/wla-dx"
        # Classic in-tree-build-dir invocation (not -S/-B, not --build -j):
        # some Ubuntu LTS images still ship a CMake that predates those
        # flags, and this form works on both old and new CMake.
        mkdir -p "$WLA_TMP/wla-dx/build"
        ( cd "$WLA_TMP/wla-dx/build" && cmake .. && make -j "$(nproc 2>/dev/null || echo 2)" )
        $SUDO cp "$WLA_TMP/wla-dx/build/binaries/wla-z80" \
                 "$WLA_TMP/wla-dx/build/binaries/wlalink" /usr/local/bin/
        trap - EXIT; rm -rf "$WLA_TMP"
    fi
fi

# ---------------------------------------------------------------------------
# 3. m68k cross toolchain
# ---------------------------------------------------------------------------
mkdir -p "$PREFIX"
if [ "$DO_TOOLCHAIN" -eq 1 ]; then
    TC_GCC=""
    for tc in x-tools-v3 x-tools-v2; do
        if [ -x "$PREFIX/$tc/m68k-unknown-elf/bin/m68k-unknown-elf-gcc" ]; then
            TC_GCC="$PREFIX/$tc/m68k-unknown-elf/bin/m68k-unknown-elf-gcc"
            log "m68k toolchain already at $PREFIX/$tc; skipping"
            break
        fi
    done
    if [ -z "$TC_GCC" ]; then
        log "fetching m68k cross toolchain into $PREFIX..."
        cd "$PREFIX"
        # x-tools-v3.tar.xz is the current bundle (static binaries, any
        # x86-64 Linux); x-tools-v2.tar from v1.3.0 is the fallback.
        if curl -fL -o x-tools-v3.tar.xz \
            "https://github.com/eaglesoftware777/neogeosdk/releases/latest/download/x-tools-v3.tar.xz"; then
            tar -xf x-tools-v3.tar.xz
            rm -f x-tools-v3.tar.xz
            TC_GCC="$PREFIX/x-tools-v3/m68k-unknown-elf/bin/m68k-unknown-elf-gcc"
        else
            curl -fL -o x-tools-v2.tar \
                "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.3.0/x-tools-v2.tar" \
                || die "toolchain download failed"
            tar -xf x-tools-v2.tar
            rm -f x-tools-v2.tar
            TC_GCC="$PREFIX/x-tools-v2/m68k-unknown-elf/bin/m68k-unknown-elf-gcc"
        fi
        [ -x "$TC_GCC" ] || die "toolchain GCC not found at $TC_GCC after extract"
    fi
fi

# ---------------------------------------------------------------------------
# 4. Clone the SDK
# ---------------------------------------------------------------------------
SDK_DIR="$PREFIX/neogeosdk"
SDK_BRANCH="neo_universal_2d"
if [ ! -d "$SDK_DIR/.git" ]; then
    log "cloning neogeosdk ($SDK_BRANCH) into $SDK_DIR..."
    git clone -b "$SDK_BRANCH" https://github.com/eaglesoftware777/neogeosdk.git "$SDK_DIR"
else
    log "neogeosdk repo already at $SDK_DIR; skipping clone"
fi

# Belt-and-suspenders: a checkout from a Windows filesystem (WSL /mnt/c,
# a zip export, some git configs) can silently drop the executable bit
# git tracked for build-helper scripts, which then fail with "Permission
# denied" deep inside `make`.  Restore it unconditionally.
find "$SDK_DIR" -name '*.sh' -exec chmod +x {} +

# ---------------------------------------------------------------------------
# 5. setenv helper
# ---------------------------------------------------------------------------
ENV_FILE="$PREFIX/setenv.sh"
cat > "$ENV_FILE" <<EOF
# Source this file to set up the NeoGeo SDK build environment.
# Generated by install-ubuntu.sh on $(date).
export SDKHOME="$PREFIX"
EOF

# ---------------------------------------------------------------------------
# 6. Smoke test
# ---------------------------------------------------------------------------
log "verifying installation..."
ok=1
need() {
    if command -v "$1" >/dev/null 2>&1; then
        printf '  %-12s OK\n' "$1"
    else
        printf '  %-12s MISSING\n' "$1"; ok=0
    fi
}
need git; need cmake; need make; need python3
need wla-z80; need wlalink; need mame; need sox
python3 -c 'import numpy, PIL, png' 2>/dev/null \
    && echo '  python deps  OK' \
    || { echo '  python deps  MISSING'; ok=0; }
{ [ -x "$PREFIX/x-tools-v3/m68k-unknown-elf/bin/m68k-unknown-elf-gcc" ] \
    || [ -x "$PREFIX/x-tools-v2/m68k-unknown-elf/bin/m68k-unknown-elf-gcc" ]; } \
    && echo '  m68k-gcc     OK' \
    || { echo '  m68k-gcc     MISSING'; ok=0; }

if [ "$ok" -eq 1 ]; then
    log "install complete.  Build with:"
    echo "    . \"$ENV_FILE\""
    echo "    cd \"$SDK_DIR\""
    echo "    make all && make test"
else
    exit 2
fi
