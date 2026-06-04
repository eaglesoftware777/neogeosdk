#!/usr/bin/env bash
# NeoGeo SDK — generic Linux installer.
#
# Auto-detects the host package manager (apt / dnf-yum / pacman / zypper) and
# installs the same dependency set the README documents.  Builds WLA-DX from
# source, fetches the m68k cross toolchain release tarball, and writes a
# `setenv.sh` helper so subsequent shells can pick up SDKHOME.
#
# Idempotent — running it twice is safe; already-installed components are
# detected and skipped.
#
# Usage:
#   install-linux.sh [--prefix DIR] [--no-toolchain] [--no-wladx] [--yes]
#
#   --prefix DIR     Root that holds neogeosdk/ + x-tools-v2/ (default: $HOME/neogeo)
#   --no-toolchain   Skip the m68k cross toolchain download
#   --no-wladx       Skip the WLA-DX build
#   --yes            Pass -y to the package manager (non-interactive)
#
# Exit codes: 0 success, 1 user error, 2 download/build failure.

set -euo pipefail

PREFIX="${SDKHOME:-$HOME/neogeo}"
DO_TOOLCHAIN=1
DO_WLADX=1
NONINTERACTIVE=0

while [ $# -gt 0 ]; do
    case "$1" in
        --prefix)         PREFIX="$2"; shift 2 ;;
        --no-toolchain)   DO_TOOLCHAIN=0; shift ;;
        --no-wladx)       DO_WLADX=0; shift ;;
        --yes|-y)         NONINTERACTIVE=1; shift ;;
        -h|--help)
            sed -n '2,18p' "$0" | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *)  echo "unknown argument: $1" >&2; exit 1 ;;
    esac
done

log()  { printf '\n[install] %s\n' "$*"; }
warn() { printf '[install] WARN: %s\n' "$*" >&2; }
die()  { printf '[install] ERROR: %s\n' "$*" >&2; exit 2; }

# ---------------------------------------------------------------------------
# 1. Detect package manager
# ---------------------------------------------------------------------------
PKG_MGR=""
if   command -v apt-get >/dev/null 2>&1; then PKG_MGR=apt
elif command -v dnf     >/dev/null 2>&1; then PKG_MGR=dnf
elif command -v yum     >/dev/null 2>&1; then PKG_MGR=yum
elif command -v pacman  >/dev/null 2>&1; then PKG_MGR=pacman
elif command -v zypper  >/dev/null 2>&1; then PKG_MGR=zypper
else die "no supported package manager found (apt/dnf/yum/pacman/zypper)"
fi
log "detected package manager: $PKG_MGR"

YES_FLAG=""
[ "$NONINTERACTIVE" -eq 1 ] && YES_FLAG="-y"

# Run apt/dnf/pacman with sudo only when not root.
SUDO=""
if [ "$(id -u)" -ne 0 ]; then
    command -v sudo >/dev/null 2>&1 || die "sudo not found and not running as root"
    SUDO="sudo"
fi

# ---------------------------------------------------------------------------
# 2. Install base packages
# ---------------------------------------------------------------------------
log "installing base packages..."
case "$PKG_MGR" in
    apt)
        $SUDO apt-get update
        $SUDO apt-get install $YES_FLAG \
            git curl ca-certificates \
            build-essential cmake make srecord \
            python3 python3-pip python3-numpy python3-pil \
            python3-venv sqlite3 sox mame
        ;;
    dnf|yum)
        $SUDO "$PKG_MGR" install $YES_FLAG \
            git curl ca-certificates \
            @development-tools cmake make srecord \
            python3 python3-pip python3-numpy python3-pillow \
            sqlite sox mame
        ;;
    pacman)
        $SUDO pacman -Sy $YES_FLAG --needed \
            git curl ca-certificates \
            base-devel cmake make srecord \
            python python-pip python-numpy python-pillow \
            sqlite sox mame
        ;;
    zypper)
        $SUDO zypper $YES_FLAG install \
            git curl ca-certificates \
            -t pattern devel_basis \
            cmake make srecord \
            python3 python3-pip python3-numpy python3-Pillow \
            sqlite3 sox mame
        ;;
esac

# pypng is not always packaged distro-side — pip-install per-user.
log "installing pypng via pip..."
python3 -m pip install --user --upgrade pypng

# ---------------------------------------------------------------------------
# 3. Build & install WLA-DX (wla-z80 + wlalink)
# ---------------------------------------------------------------------------
if [ "$DO_WLADX" -eq 1 ]; then
    if command -v wla-z80 >/dev/null 2>&1 && command -v wlalink >/dev/null 2>&1; then
        log "WLA-DX already installed ($(wla-z80 --version 2>&1 | head -1)); skipping"
    else
        log "building WLA-DX from source..."
        WLA_TMP="$(mktemp -d)"
        trap 'rm -rf "$WLA_TMP"' EXIT
        git clone --depth 1 https://github.com/vhelin/wla-dx "$WLA_TMP/wla-dx"
        cmake -S "$WLA_TMP/wla-dx" -B "$WLA_TMP/wla-dx/build"
        cmake --build "$WLA_TMP/wla-dx/build" -j "$(nproc 2>/dev/null || echo 2)"
        $SUDO cp "$WLA_TMP/wla-dx/build/binaries/wla-z80" \
                 "$WLA_TMP/wla-dx/build/binaries/wlalink" /usr/local/bin/
        trap - EXIT; rm -rf "$WLA_TMP"
        log "WLA-DX installed: $(command -v wla-z80)"
    fi
fi

# ---------------------------------------------------------------------------
# 4. m68k cross toolchain
# ---------------------------------------------------------------------------
mkdir -p "$PREFIX"
if [ "$DO_TOOLCHAIN" -eq 1 ]; then
    TC_DIR="$PREFIX/x-tools-v2"
    TC_GCC="$TC_DIR/m68k-unknown-elf/bin/m68k-unknown-elf-gcc"
    if [ -x "$TC_GCC" ]; then
        log "m68k toolchain already at $TC_DIR; skipping"
    else
        log "fetching m68k cross toolchain into $PREFIX..."
        cd "$PREFIX"
        # v1.3.0 release ships x-tools-v2.tar (Linux) and
        # x-tools-v2-win.tar (Windows).
        curl -fL -o x-tools-v2.tar \
            "https://github.com/eaglesoftware777/neogeosdk/releases/download/v1.3.0/x-tools-v2.tar" \
            || die "toolchain download failed"
        tar -xf x-tools-v2.tar
        rm -f x-tools-v2.tar
        [ -x "$TC_GCC" ] || die "toolchain GCC not found at $TC_GCC after extract"
        log "toolchain extracted: $($TC_GCC --version | head -1)"
    fi
fi

# ---------------------------------------------------------------------------
# 5. Clone the SDK if not already adjacent
# ---------------------------------------------------------------------------
SDK_DIR="$PREFIX/neogeosdk"
SDK_BRANCH="neo_universal_2d"
if [ ! -d "$SDK_DIR/.git" ]; then
    log "cloning neogeosdk ($SDK_BRANCH) into $SDK_DIR..."
    git clone -b "$SDK_BRANCH" https://github.com/eaglesoftware777/neogeosdk.git "$SDK_DIR"
else
    log "neogeosdk repo already at $SDK_DIR; skipping clone"
fi

# ---------------------------------------------------------------------------
# 6. Write a setenv.sh helper
# ---------------------------------------------------------------------------
ENV_FILE="$PREFIX/setenv.sh"
cat > "$ENV_FILE" <<EOF
# Source this file to set up the NeoGeo SDK build environment.
# Generated by install-linux.sh on $(date).
export SDKHOME="$PREFIX"
EOF
log "wrote $ENV_FILE — source it before building:"
echo "    . \"$ENV_FILE\""

# ---------------------------------------------------------------------------
# 7. Smoke test
# ---------------------------------------------------------------------------
log "verifying installation..."
ok=1
need() {
    if command -v "$1" >/dev/null 2>&1; then
        printf '  %-12s OK   (%s)\n' "$1" "$(command -v "$1")"
    else
        printf '  %-12s MISSING\n' "$1"; ok=0
    fi
}
need git
need cmake
need make
need python3
need wla-z80
need wlalink
need mame
need sox
python3 -c 'import numpy, PIL, png' 2>/dev/null \
    && echo "  python deps  OK   (numpy/PIL/png)" \
    || { echo "  python deps  MISSING"; ok=0; }

if [ -x "$PREFIX/x-tools-v2/m68k-unknown-elf/bin/m68k-unknown-elf-gcc" ]; then
    printf '  m68k-gcc     OK   (%s)\n' \
        "$($PREFIX/x-tools-v2/m68k-unknown-elf/bin/m68k-unknown-elf-gcc --version | head -1)"
else
    printf '  m68k-gcc     MISSING (expected at %s)\n' \
        "$PREFIX/x-tools-v2/m68k-unknown-elf/bin/m68k-unknown-elf-gcc"
    ok=0
fi

if [ "$ok" -eq 1 ]; then
    log "install complete.  Build with:"
    echo "    . \"$ENV_FILE\""
    echo "    cd \"$SDK_DIR\""
    echo "    make all"
    echo "    make test"
else
    warn "some components are missing; review the report above."
    exit 2
fi
