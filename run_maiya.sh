#!/bin/bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mame neogeo -cart1 maiya \
    -rompath "$DIR/roms/maiya;$DIR/roms" \
    -hashpath "$DIR/hash_eagle/maiya;$DIR/hash_eagle;$DIR/hash" \
    -bios euro \
    -window \
    -waitvsync \
    -nofilter
