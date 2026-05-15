#!/bin/bash
#######
#https://eaglesoftware.biz
#https://github.com/eaglesoftware777
#https://github.com/eaglesoftware777/neogeosdk
#######
# NeoGeo SDK - Interactive build menu (Linux)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Default game selection
CURRENT_GAME=demo

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
    echo "   Game: $CURRENT_GAME"
    echo "============================================="
    echo ""
    echo "  -- GAME SELECTION --"
    echo "   g) Select game  [current: $CURRENT_GAME]"
    echo "      Available: demo (777)  helloworld (772)  tutorial (555)  neogeogame (775)"
    echo ""
    echo "  -- FULL BUILD (start here) --"
    echo "   1) make clean-all && make all               Full clean rebuild MVS  [recommended]"
    echo "   2) make clean-all && make PLATFORM=aes all  Full clean rebuild AES"
    echo "   3) make all                                 Build everything MVS (no clean)"
    echo "   4) make PLATFORM=aes all                    Build everything AES (no clean)"
    echo ""
    echo "  -- PROGRAM ROM --"
    echo "   5) make p1                                  MVS P1 ROM only"
    echo "   6) make PLATFORM=aes p1                     AES P1 ROM only"
    echo ""
    echo "  -- GRAPHICS --"
    echo "   7) make sfix                                FIX / S ROM (tile graphics)"
    echo "   8) make art                                 Sprite C ROMs"
    echo ""
    echo "  -- SOUND --"
    echo "   9) make sound                               Full sound (samples + V ROM + M1)"
    echo "  10) make m1rom                               Z80 sound driver ROM only"
    echo "  11) make vrom                                V ROM (ADPCM sample pack)"
    echo "  12) make mml                                 Compile SSG/MML music data"
    echo "  13) make fm                                  Compile FM music data"
    echo "  14) make fmpatches                           Compile FM patch bank"
    echo "  15) make ssg                                 Compile standalone SSG data"
    echo ""
    echo "  -- TEST / RUN --"
    echo "  16) make test                                Launch MAME (MVS, US BIOS)  [build first]"
    echo "  17) make test-aes                            Launch MAME (AES)           [build first]"
    echo "  18) make test-mvs                            Launch MAME (MVS, explicit) [build first]"
    echo "  19) make test BIOS=<name>                    Launch MAME with specific BIOS (prompts)"
    echo "  20) make bios-list                           Print all supported BIOS names"
    echo ""
    echo "  -- PACKAGE --"
    echo "  21) make dist                                Build release package (dist/roms/neogeosdk.zip)"
    echo ""
    echo "  -- CLEAN --"
    echo "  22) make clean                               Remove P1 ROM outputs"
    echo "  23) make sound-clean                         Remove sound ROM outputs"
    echo "  24) make art-clean                           Remove art outputs"
    echo "  25) make clean-all                           Full clean (all generated files)"
    echo ""
    echo "  -- DEBUG / TRACE (advanced) --"
    echo "  26) make debug                               Launch MAME with CPU debugger (MVS)"
    echo "  27) make debug-aes                           Launch MAME with CPU debugger (AES)"
    echo "  28) make mame-trace                          Trace m68k execution to dump/m68k_trace.txt"
    echo "  29) make debug-build                         Debug build with map/sym artifacts"
    echo "  30) make gdb-trace                           GDB trace dump to dump/gdb_trace.txt"
    echo "  31) make gdb                                 Launch GDB interactive"
    echo "  32) make gdb-remote                          GDB remote attach (MAME -debug)"
    echo "  33) make dump                                Disassemble and hex dump"
    echo ""
    echo "   q) Quit"
    echo ""
    read -r -p "Enter choice: " choice

    case "$choice" in
        g|G)
            echo ""
            echo "Available games:"
            echo "  demo       - Full SDK showcase demo  (ID 777)"
            echo "  helloworld - Minimal FIX-text hello world (ID 772)"
            echo "  tutorial   - Tutorial template game  (ID 555)"
            echo "  neogeogame - Blank game template     (ID 775)"
            echo ""
            read -r -p "Enter game name [current: $CURRENT_GAME]: " gamename
            [ -n "$gamename" ] && CURRENT_GAME="$gamename"
            echo "Game set to: $CURRENT_GAME"
            read -r -p "Press Enter to return to menu..."
            ;;
        1)  run "make GAME=$CURRENT_GAME clean-all && make GAME=$CURRENT_GAME all" ;;
        2)  run "make GAME=$CURRENT_GAME clean-all && make GAME=$CURRENT_GAME PLATFORM=aes all" ;;
        3)  run "make GAME=$CURRENT_GAME all" ;;
        4)  run "make GAME=$CURRENT_GAME PLATFORM=aes all" ;;
        5)  run "make GAME=$CURRENT_GAME p1" ;;
        6)  run "make GAME=$CURRENT_GAME PLATFORM=aes p1" ;;
        7)  run "make GAME=$CURRENT_GAME sfix" ;;
        8)  run "make GAME=$CURRENT_GAME art" ;;
        9)  run "make GAME=$CURRENT_GAME sound" ;;
        10) run "make GAME=$CURRENT_GAME m1rom" ;;
        11) run "make GAME=$CURRENT_GAME vrom" ;;
        12) run "make GAME=$CURRENT_GAME mml" ;;
        13) run "make GAME=$CURRENT_GAME fm" ;;
        14) run "make GAME=$CURRENT_GAME fmpatches" ;;
        15) run "make GAME=$CURRENT_GAME ssg" ;;
        16) run "make GAME=$CURRENT_GAME test" ;;
        17) run "make GAME=$CURRENT_GAME test-aes" ;;
        18) run "make GAME=$CURRENT_GAME test-mvs" ;;
        19) run "make GAME=$CURRENT_GAME bios-list" ; read -r -p "Enter BIOS name: " biosname ; run "make GAME=$CURRENT_GAME test BIOS=$biosname" ;;
        20) run "make bios-list" ;;
        21) run "make GAME=$CURRENT_GAME dist" ;;
        22) run "make GAME=$CURRENT_GAME clean" ;;
        23) run "make GAME=$CURRENT_GAME sound-clean" ;;
        24) run "make GAME=$CURRENT_GAME art-clean" ;;
        25) run "make GAME=$CURRENT_GAME clean-all" ;;
        26) run "make GAME=$CURRENT_GAME debug" ;;
        27) run "make GAME=$CURRENT_GAME debug-aes" ;;
        28) run "make GAME=$CURRENT_GAME mame-trace" ;;
        29) run "make GAME=$CURRENT_GAME debug-build" ;;
        30) run "make GAME=$CURRENT_GAME gdb-trace" ;;
        31) run "make GAME=$CURRENT_GAME gdb" ;;
        32) run "make GAME=$CURRENT_GAME gdb-remote" ;;
        33) run "make GAME=$CURRENT_GAME dump" ;;
        q|Q) echo "Bye."; exit 0 ;;
        *) echo "Unknown choice: $choice" ;;
    esac
done
