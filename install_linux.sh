#!/usr/bin/env bash
set -e
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=========================================================="
echo " Eagle Software - Maiya: Super Nature Girl Setup Wizard"
echo "=========================================================="
echo "Starting Graphical Installer..."

SCRIPT=""
if [ -f "$HERE/tools/installer/install_maiya.py" ]; then
    SCRIPT="$HERE/tools/installer/install_maiya.py"
elif [ -f "$HERE/install_maiya.py" ]; then
    SCRIPT="$HERE/install_maiya.py"
elif [ -f "$HERE/installer/install_maiya.py" ]; then
    SCRIPT="$HERE/installer/install_maiya.py"
fi

if [ -z "$SCRIPT" ]; then
    echo "[ERROR] Could not locate install_maiya.py."
    exit 1
fi

if command -v python3 &>/dev/null; then
    exec python3 "$SCRIPT" "$@"
elif command -v python &>/dev/null; then
    exec python "$SCRIPT" "$@"
else
    echo "[ERROR] Python 3 was not found on your system."
    echo "Please install Python 3 to run the installer."
    exit 1
fi
