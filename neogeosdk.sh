#!/bin/bash
#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
# NeoGeo SDK - Interactive build menu (Linux)
# Covers all make targets from first-use to most specialised.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

run() {
    echo ""
    echo ">>> $*"
    eval "$@"
    local rc=$?
    echo ""
    [ $rc -eq 0 ] && echo "Done." || echo "Exit code: $rc"
    echo ""
    read -r -p "Press Enter to return to menu..."
}

while true; do
    clear
    echo "============================================="
    echo "   NeoGeo SDK - Build Menu"
    echo "============================================="
    echo ""
    echo "  -- FULL BUILD (start here) --"
    echo "   1) make clean-all && make all   Full clean rebuild  [recommended]"
    echo "   2) make all                     Build everything (no clean)"
    echo ""
    echo "  -- PROGRAM ROM --"
    echo "   3) make p1                      68000 program ROM only"
    echo ""
    echo "  -- GRAPHICS --"
    echo "   4) make sfix                    FIX / S ROM (tile graphics)"
    echo "   5) make art                     Sprite C ROMs"
    echo ""
    echo "  -- SOUND --"
    echo "   6) make sound                   Full sound (samples + V ROM + M1)"
    echo "   7) make m1rom                   Z80 sound driver ROM only"
    echo "   8) make vrom                    V ROM (ADPCM sample pack)"
    echo "   9) make mml                     Compile SSG/MML music data"
    echo "  10) make fm                      Compile FM music data"
    echo "  11) make fmpatches               Compile FM patch bank"
    echo "  12) make ssg                     Compile standalone SSG data"
    echo ""
    echo "  -- TEST / DEBUG --"
    echo "  13) make test                    Build P1 (MVS), update hash, launch MAME  [BIOS: us]"
    echo "  14) make test BIOS=<name>        Launch MAME with a specific BIOS"
    echo "  15) make bios-list               Print all supported BIOS names"
    echo "  16) make test-aes                Build P1 (AES), launch MAME"
    echo "  17) make debug                   Launch MAME with CPU debugger"
    echo "  18) make mame-trace              Trace m68k execution to dump/m68k_trace.txt"
    echo "  19) make debug-build             Debug build with map/sym artifacts"
    echo "  20) make gdb-trace               GDB trace dump"
    echo "  21) make gdb                     Launch GDB"
    echo "  22) make gdb-remote              GDB remote (MAME -debug)"
    echo "  23) make dump                    Disassemble and hex dump"
    echo ""
    echo "  -- PACKAGE --"
    echo "  24) make dist                    Build release package (dist/roms/neogeosdk.zip)"
    echo ""
    echo "  -- CLEAN --"
    echo "  25) make clean                   Remove program ROM outputs"
    echo "  26) make sound-clean             Remove sound ROM outputs"
    echo "  27) make art-clean               Remove art outputs"
    echo "  28) make clean-all               Full clean (all generated files)"
    echo ""
    echo "   q) Quit"
    echo ""
    read -r -p "Enter choice: " choice

    case "$choice" in
        1)  run "make clean-all && make all" ;;
        2)  run make all ;;
        3)  run make p1 ;;
        4)  run make sfix ;;
        5)  run make art ;;
        6)  run make sound ;;
        7)  run make m1rom ;;
        8)  run make vrom ;;
        9)  run make mml ;;
        10) run make fm ;;
        11) run make fmpatches ;;
        12) run make ssg ;;
        13) run make test ;;
        14) run make bios-list ; read -r -p "Enter BIOS name: " biosname ; run "make test BIOS=$biosname" ;;
        15) run make bios-list ;;
        16) run make test-aes ;;
        17) run make debug ;;
        18) run make mame-trace ;;
        19) run make debug-build ;;
        20) run make gdb-trace ;;
        21) run make gdb ;;
        22) run make gdb-remote ;;
        23) run make dump ;;
        24) run make dist ;;
        25) run make clean ;;
        26) run make sound-clean ;;
        27) run make art-clean ;;
        28) run make clean-all ;;
        q|Q) echo "Bye."; exit 0 ;;
        *) echo "Unknown choice: $choice" ;;
    esac
done
