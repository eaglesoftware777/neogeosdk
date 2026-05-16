: "${GAME_ID:=777}"
: "${GAME:=demo}"
python3 - <<'PY'
import os
import shutil
import sys

game_id = os.environ.get("GAME_ID", "777")
target_size = 8192 * 1024

def pad_copy(src: str, dst: str) -> None:
    if not os.path.exists(src):
        raise FileNotFoundError(src)
    with open(src, "rb") as f:
        data = f.read()
    if len(data) < target_size:
        data = data + (b"\x00" * (target_size - len(data)))
        action = "padded"
    else:
        action = "kept"
        if len(data) > target_size:
            over = len(data) - target_size
            tiles_over = over // 64
            print(
                f"WARNING: {src} exceeds 8MB target by {over} bytes "
                f"(~{tiles_over} Cx tile rows). Keeping full size."
            )
    with open(dst, "wb") as f:
        f.write(data)
    print(f"{action}: {src} -> {dst} ({len(data)} bytes)")

pad_copy("1c.c1", f"{game_id}-c1.c1")
pad_copy("2c.c2", f"{game_id}-c2.c2")
PY
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$REPO_ROOT/roms/$GAME"
cp ${GAME_ID}-c1.c1 ${GAME_ID}-c2.c2 "$REPO_ROOT/roms/$GAME"
