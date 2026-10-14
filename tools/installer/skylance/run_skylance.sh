#!/usr/bin/env bash
# Sky Lance in MAME: ./run_skylance.sh [mvs|aes] [eagle|stock|unibios40]
# Each platform keeps its own ROMs, software list, settings and saves under
# tests/skylance. MAME: $MAME, else mame-path.txt here, else 'mame' on PATH.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODE="${1:-mvs}"
FLAVOR="${2:-eagle}"
SYSTEM=neogeo; BIOS=euro
if [ "$MODE" = aes ]; then SYSTEM=aes; BIOS=asia; fi
[ "$FLAVOR" = unibios40 ] && BIOS=unibios40
MAME_EXE="${MAME:-mame}"
[ -z "${MAME:-}" ] && [ -f "$HERE/mame-path.txt" ] && MAME_EXE="$(head -n1 "$HERE/mame-path.txt")"
DATA="$HERE/tests/skylance/$MODE"
ROMS="$DATA/roms;$HERE/roms"
[ "$FLAVOR" = eagle ] && ROMS="$HERE/tests/skylance/bios;$ROMS"
exec "$MAME_EXE" "$SYSTEM" -noreadconfig -rompath "$ROMS" -hashpath "$DATA/hash" -bios "$BIOS" \
    -cart1 skylance -cfg_directory "$DATA/cfg-$FLAVOR" -nvram_directory "$DATA/nvram-$FLAVOR" \
    -skip_gameinfo -keepaspect
