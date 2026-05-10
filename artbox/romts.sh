./romtool /p 1c.c1 1p.c1 2048 0
./romtool /p 2c.c2 2p.c2 2048 0
./romtool /f 1p.c1 777-c1.c1
./romtool /f 2p.c2 777-c2.c2
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cp 777-c1.c1 777-c2.c2 "$REPO_ROOT/roms/neogeosdk"
