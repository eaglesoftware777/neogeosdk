#!/usr/bin/env bash
# NeoGeo SDK -- install built games into a local MAME copy.
#
# Thin wrapper around tools/mame_launcher.py.  Prompts for the MAME
# installation folder the first time it runs (or reads it from
# ~/.neogeosdk/config.json if already saved), then copies the requested
# game's dist zip into a self-contained neogeosdk/ folder inside that
# MAME install -- it never touches MAME's own rom collection or ini files.
#
# Usage:
#   install-mame.sh                 interactive: pick a game, install it
#   install-mame.sh --game maiya    install one game non-interactively-ish
#   install-mame.sh --game all      install every game already packaged
#   install-mame.sh --mame-path DIR set/override the saved MAME path
#
# Build + package a game first with:
#   make bios-package GAME=<name>   (or: make dist-all, for every game)

set -euo pipefail
cd "$(dirname "$0")/.."

PYTHON="${PYTHON:-python3}"
exec "$PYTHON" tools/mame_launcher.py install "$@"
