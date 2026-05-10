#!/bin/bash
#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
# NeoGeo SDK - Interactive build menu
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
    echo "  13) make test                    Build P1, update hash, launch MAME"
    echo "  14) make debug                   Launch MAME with debugger"
    echo "  15) make debug-build             Debug build with map/sym artifacts"
    echo "  16) make gdb-trace               GDB trace dump"
    echo "  17) make gdb                     Launch GDB"
    echo "  18) make gdb-remote              GDB remote (MAME -debug)"
    echo "  19) make dump                    Disassemble and hex dump"
    echo ""
    echo "  -- CLEAN --"
    echo "  20) make clean                   Remove program ROM outputs"
    echo "  21) make sound-clean             Remove sound ROM outputs"
    echo "  22) make art-clean               Remove art outputs"
    echo "  23) make clean-all               Full clean (all generated files)"
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
        14) run make debug ;;
        15) run make debug-build ;;
        16) run make gdb-trace ;;
        17) run make gdb ;;
        18) run make gdb-remote ;;
        19) run make dump ;;
        20) run make clean ;;
        21) run make sound-clean ;;
        22) run make art-clean ;;
        23) run make clean-all ;;
        q|Q) echo "Bye."; exit 0 ;;
        *) echo "Unknown choice: $choice" ;;
    esac
done
