: "${GAME_ID:=777}"
: "${GAME:=demo}"
: "${CROM_PAD_KB:=8192}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
GAME_ARTBOX="$REPO_ROOT/games/$GAME/artbox"
python3 "$SCRIPT_DIR/romtool.py" --dir "$GAME_ARTBOX" /p 1c.c1 1p.c1 "$CROM_PAD_KB" 0
python3 "$SCRIPT_DIR/romtool.py" --dir "$GAME_ARTBOX" /p 2c.c2 2p.c2 "$CROM_PAD_KB" 0
python3 "$SCRIPT_DIR/romtool.py" --dir "$GAME_ARTBOX" /f 1p.c1 "${GAME_ID}-c1.c1"
python3 "$SCRIPT_DIR/romtool.py" --dir "$GAME_ARTBOX" /f 2p.c2 "${GAME_ID}-c2.c2"

mkdir -p "$REPO_ROOT/roms/$GAME"
cp "$GAME_ARTBOX/${GAME_ID}-c1.c1" "$GAME_ARTBOX/${GAME_ID}-c2.c2" "$REPO_ROOT/roms/$GAME"
