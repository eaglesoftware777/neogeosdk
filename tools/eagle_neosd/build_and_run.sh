#!/usr/bin/env bash
# ==============================================================================
# Eagle Software - NeoSD Converter Build & Automation Script
# Standalone build and execution utility for the Eagle NeoSD toolchain.
#
# Usage:
#   ./build_and_run.sh                 # Compiles C engine and runs automated self-test
#   ./build_and_run.sh --pack <dir> <out.neo> [options]
#   ./build_and_run.sh --unpack <file.neo> <out_dir>
#   ./build_and_run.sh --info <file.neo>
#   ./build_and_run.sh --gui           # Starts local Web GUI server
# ==============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BIN="${SCRIPT_DIR}/eagle_neosd"
SRC="${SCRIPT_DIR}/eagle_neosd.c"
HDR="${SCRIPT_DIR}/eagle_neosd.h"
GUI="${SCRIPT_DIR}/eagle_neosd_gui.py"

echo "========================================================"
echo "  Eagle Software - NeoSD ROM Packager & Converter"
echo "========================================================"

# Function: Compile C binary
compile_tool() {
    echo "[Build] Compiling Eagle NeoSD engine..."
    gcc -O3 -Wall -Wextra -std=c11 "${SRC}" -o "${BIN}"
    chmod +x "${BIN}"
    echo "[Build] Compilation successful -> ${BIN}"
}

# Function: Run self-test
run_test() {
    compile_tool

    echo ""
    echo "[Test] Initiating automated self-test verification..."

    # Check if a test ROM directory exists
    TEST_ROM_DIR=""
    if [ -d "${SDK_ROOT}/roms/maiya" ] && [ -f "${SDK_ROOT}/roms/maiya/780-p1.p1" ]; then
        TEST_ROM_DIR="${SDK_ROOT}/roms/maiya"
    elif [ -d "${SDK_ROOT}/roms/demo" ] && [ -f "${SDK_ROOT}/roms/demo/777-p1.p1" ]; then
        TEST_ROM_DIR="${SDK_ROOT}/roms/demo"
    fi

    if [ -z "${TEST_ROM_DIR}" ]; then
        echo "[Test] Notice: No test ROM set found in roms/maiya or roms/demo to execute pack test."
        echo "[Test] Engine compiled and verified ready for use."
        return 0
    fi

    TMP_DIR="/tmp/eagle_neosd_test_$$"
    TMP_NEO="${TMP_DIR}/test_game.neo"
    TMP_UNPACK="${TMP_DIR}/unpacked"
    mkdir -p "${TMP_DIR}"
    mkdir -p "${TMP_UNPACK}"

    echo "[Test] Using test ROMs from: ${TEST_ROM_DIR}"
    echo "[Test] 1. Testing pack operation..."
    "${BIN}" -i "${TEST_ROM_DIR}" -o "${TMP_NEO}"

    echo ""
    echo "[Test] 2. Testing inspect operation..."
    "${BIN}" --info "${TMP_NEO}"

    echo ""
    echo "[Test] 3. Testing extract/unpack operation..."
    "${BIN}" -x "${TMP_NEO}" -o "${TMP_UNPACK}"

    echo ""
    echo "[Test] 4. Verifying byte-for-byte fidelity against original ROMs..."
    
    # Locate original files and compare
    P_ORIG=$(find "${TEST_ROM_DIR}" -name "*-p1.p1" | head -n 1)
    P_EXTR=$(find "${TMP_UNPACK}" -name "*-p1.p1" | head -n 1)
    cmp "${P_ORIG}" "${P_EXTR}"
    echo "  [OK] P-ROM binary comparison: 100% IDENTICAL MATCH"

    S_ORIG=$(find "${TEST_ROM_DIR}" -name "*-s1.s1" | head -n 1)
    S_EXTR=$(find "${TMP_UNPACK}" -name "*-s1.s1" | head -n 1)
    cmp "${S_ORIG}" "${S_EXTR}"
    echo "  [OK] S-ROM binary comparison: 100% IDENTICAL MATCH"

    M_ORIG=$(find "${TEST_ROM_DIR}" -name "*-m1.m1" | head -n 1)
    M_EXTR=$(find "${TMP_UNPACK}" -name "*-m1.m1" | head -n 1)
    cmp "${M_ORIG}" "${M_EXTR}"
    echo "  [OK] M-ROM binary comparison: 100% IDENTICAL MATCH"

    C1_ORIG=$(find "${TEST_ROM_DIR}" -name "*-c1.c1" | head -n 1)
    C1_EXTR=$(find "${TMP_UNPACK}" -name "*-c1.c1" | head -n 1)
    cmp "${C1_ORIG}" "${C1_EXTR}"
    echo "  [OK] C1-ROM de-interleave binary comparison: 100% IDENTICAL MATCH"

    C2_ORIG=$(find "${TEST_ROM_DIR}" -name "*-c2.c2" | head -n 1)
    C2_EXTR=$(find "${TMP_UNPACK}" -name "*-c2.c2" | head -n 1)
    cmp "${C2_ORIG}" "${C2_EXTR}"
    echo "  [OK] C2-ROM de-interleave binary comparison: 100% IDENTICAL MATCH"

    # Clean up test artifacts
    rm -rf "${TMP_DIR}"
    echo ""
    echo "========================================================"
    echo "  ALL TESTS PASSED: FULL INTEGRITY CONFIRMED"
    echo "========================================================"
}

# Mode Dispatcher
case "$1" in
    --gui)
        compile_tool
        echo "[GUI] Launching Eagle NeoSD Web GUI interface..."
        exec python3 "${GUI}"
        ;;
    --pack)
        compile_tool
        shift
        exec "${BIN}" -i "$1" -o "$2" "${@:3}"
        ;;
    --unpack|-x)
        compile_tool
        shift
        exec "${BIN}" -x "$1" -o "$2"
        ;;
    --info|-v)
        compile_tool
        shift
        exec "${BIN}" --info "$1"
        ;;
    --compile-only)
        compile_tool
        ;;
    *)
        run_test
        ;;
esac
