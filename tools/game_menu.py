#!/usr/bin/env python3
"""Simple numbered text menu to build, package, install, and test SDK games.

Wraps the same make targets and tools/mame_launcher.py helpers used on the
command line, so a new game only needs games/<name>/game.mk to show up here
automatically -- nothing in this menu is hard-coded to a specific game.

Usage:
    python3 tools/game_menu.py
"""

import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import mame_launcher  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parent.parent
IS_WINDOWS = os.name == "nt"


def make_argv(*targets_and_vars):
    if IS_WINDOWS:
        return ["make", "-f", "MakefileWin32.mak", *targets_and_vars]
    return ["make", *targets_and_vars]


def run(argv):
    print()
    print("$ " + " ".join(argv))
    result = subprocess.run(argv, cwd=REPO_ROOT)
    return result.returncode


def choose_game(games, prompt="Game"):
    for i, name in enumerate(games, 1):
        print(f"  {i}) {name}")
    while True:
        choice = input(f"{prompt} [1-{len(games)} or name]: ").strip()
        if not choice:
            continue
        if choice.isdigit() and 1 <= int(choice) <= len(games):
            return games[int(choice) - 1]
        if choice in games:
            return choice
        print("Not a valid choice, try again.")


def choose_platform():
    choice = input("Platform [mvs/aes] (default mvs): ").strip().lower()
    return "aes" if choice == "aes" else "mvs"


def action_build(games):
    game = choose_game(games)
    run(make_argv("all", f"GAME={game}"))


def action_build_all(games):
    run(make_argv("all-games"))


def action_package(games):
    game = choose_game(games)
    run(make_argv("bios-package", f"GAME={game}"))


def action_package_all(games):
    run(make_argv("dist-all"))


def action_install(games):
    game = choose_game(games)
    mame_launcher.install_game(game, mame_launcher.resolve_mame_path())


def action_run(games):
    game = choose_game(games)
    platform = choose_platform()
    mame_launcher.run_game(game, mame_launcher.resolve_mame_path(), platform)


def action_full_pipeline(games):
    game = choose_game(games)
    platform = choose_platform()
    if run(make_argv("all", f"GAME={game}")) != 0:
        print("Build failed; stopping.")
        return
    if run(make_argv("bios-package", f"GAME={game}")) != 0:
        print("Packaging failed; stopping.")
        return
    mame_path = mame_launcher.resolve_mame_path()
    mame_launcher.install_game(game, mame_path)
    mame_launcher.run_game(game, mame_path, platform)


def action_set_mame_path(games):
    path = mame_launcher.prompt_for_mame_path(mame_launcher.load_config().get("mame_path"))
    config = mame_launcher.load_config()
    config["mame_path"] = str(path)
    mame_launcher.save_config(config)
    print(f"Saved MAME path: {path}")


MENU = [
    ("Build a game", action_build),
    ("Build ALL games", action_build_all),
    ("Package a game for MAME (dist zip)", action_package),
    ("Package ALL games for MAME", action_package_all),
    ("Install a game into MAME", action_install),
    ("Run a game in MAME", action_run),
    ("Full pipeline: build + package + install + run", action_full_pipeline),
    ("Set MAME path", action_set_mame_path),
]


def main():
    print("NeoGeo SDK - Build & Test Menu")
    while True:
        games = mame_launcher.discover_games(REPO_ROOT)
        print()
        print(f"Games available: {', '.join(games)}")
        for i, (label, _) in enumerate(MENU, 1):
            print(f" {i}) {label}")
        print(" 0) Exit")
        choice = input("> ").strip()
        if choice == "0":
            return
        if choice.isdigit() and 1 <= int(choice) <= len(MENU):
            _, action = MENU[int(choice) - 1]
            try:
                action(games)
            except SystemExit as exc:
                print(exc)
            except KeyboardInterrupt:
                print("\nCancelled.")
        else:
            print("Not a valid choice.")


if __name__ == "__main__":
    main()
