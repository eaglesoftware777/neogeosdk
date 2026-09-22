#!/usr/bin/env python3
"""Install packaged NeoGeo SDK games into a local MAME copy and run them.

Reads game roms + hash data out of the per-game dist zip built by
`make bios-package GAME=<game>` (or `make dist-all` for every game), copies
them into a self-contained `neogeosdk/` folder inside the user's MAME
installation, and can then launch MAME directly against that folder.

Nothing here touches MAME's own rom collection or ini files: everything
lives under `<mame_path>/neogeosdk/`, addressed with explicit -rompath and
-hashpath arguments on the command line.

Usage:
    python3 tools/mame_launcher.py list
    python3 tools/mame_launcher.py set-path [PATH]
    python3 tools/mame_launcher.py install --game maiya [--mame-path PATH]
    python3 tools/mame_launcher.py install --game all
    python3 tools/mame_launcher.py run --game maiya [--platform mvs|aes]
    python3 tools/mame_launcher.py install-run --game maiya
"""

import argparse
import json
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
CONFIG_DIR = Path.home() / ".neogeosdk"
CONFIG_FILE = CONFIG_DIR / "config.json"

BIOS_NAME = {"mvs": "euro", "aes": "asia"}
DRIVER_NAME = {"mvs": "neogeo", "aes": "aes"}
MAME_EXE_NAMES = ("mame", "mame64", "mame.exe", "mame64.exe")


def discover_games(root=REPO_ROOT):
    return sorted(p.parent.name for p in (Path(root) / "games").glob("*/game.mk"))


def load_config():
    if CONFIG_FILE.is_file():
        try:
            return json.loads(CONFIG_FILE.read_text(encoding="utf-8"))
        except (ValueError, OSError):
            return {}
    return {}


def save_config(config):
    CONFIG_DIR.mkdir(parents=True, exist_ok=True)
    CONFIG_FILE.write_text(json.dumps(config, indent=2) + "\n", encoding="utf-8")


def find_mame_executable(mame_path):
    """Look for a MAME binary inside mame_path, then fall back to PATH."""
    if mame_path:
        mame_path = Path(mame_path)
        for name in MAME_EXE_NAMES:
            candidate = mame_path / name
            if candidate.is_file():
                return str(candidate)
    for name in ("mame", "mame64"):
        found = shutil.which(name)
        if found:
            return found
    return None


def prompt_choose_game(games, allow_all=False):
    print("Games available:")
    for i, name in enumerate(games, 1):
        print(f"  {i}) {name}")
    if allow_all:
        print("  a) all")
    prompt = "Game [number, name, or 'a' for all]: " if allow_all else "Game [number or name]: "
    while True:
        choice = input(prompt).strip().lower()
        if allow_all and choice == "a":
            return "all"
        if choice.isdigit() and 1 <= int(choice) <= len(games):
            return games[int(choice) - 1]
        if choice in games:
            return choice
        print("Not a valid choice, try again.")


def prompt_for_mame_path(current=None):
    print()
    print("MAME installation folder needed (the directory containing the")
    print("mame / mame64 executable).")
    if current:
        print(f"Current saved path: {current}")
        prompt = "New path (blank to keep current): "
    else:
        prompt = "MAME path: "
    while True:
        answer = input(prompt).strip().strip('"')
        if not answer:
            if current:
                return Path(current)
            print("A path is required.")
            continue
        path = Path(answer).expanduser()
        if not path.is_dir():
            print(f"'{path}' is not a directory. Try again.")
            continue
        return path


def resolve_mame_path(cli_path=None, interactive=True):
    config = load_config()
    if cli_path:
        path = Path(cli_path).expanduser()
        config["mame_path"] = str(path)
        save_config(config)
        return path
    saved = config.get("mame_path")
    if saved and Path(saved).is_dir():
        return Path(saved)
    if not interactive:
        raise SystemExit(
            "No MAME path configured. Pass --mame-path or run:\n"
            "    python3 tools/mame_launcher.py set-path"
        )
    path = prompt_for_mame_path(saved)
    config["mame_path"] = str(path)
    save_config(config)
    return path


def dist_zip_path(game, dist_dir=None):
    dist_dir = Path(dist_dir) if dist_dir else (REPO_ROOT / "dist")
    return dist_dir / f"{game}-eagle-bios.zip"


def install_game(game, mame_path, dist_dir=None):
    zip_path = dist_zip_path(game, dist_dir)
    if not zip_path.is_file():
        raise SystemExit(
            f"Missing {zip_path}.\n"
            f"Build it first with:  make bios-package GAME={game}\n"
            f"(Windows:              make -f MakefileWin32.mak bios-package GAME={game})"
        )

    target_root = Path(mame_path) / "neogeosdk"
    roms_root = target_root / "roms"
    hash_root = target_root / "hash" / game
    roms_root.mkdir(parents=True, exist_ok=True)
    hash_root.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        with zipfile.ZipFile(zip_path) as archive:
            archive.extractall(tmp)
        shutil.copytree(tmp / "roms", roms_root, dirs_exist_ok=True)
        shutil.copyfile(tmp / "hash" / "neogeo.xml", hash_root / "neogeo.xml")

    write_launchers(target_root, game)
    print(f"Installed {game} into {target_root}")
    return target_root


def write_launchers(target_root, game):
    roms_root = target_root / "roms"
    hash_dir = target_root / "hash" / game
    sh_path = target_root / f"run_{game}.sh"
    bat_path = target_root / f"run_{game}.bat"

    # target_root is always a subfolder of the chosen MAME install (see
    # install_game), so the mame/mame64 binary one level up is preferred
    # over relying on PATH -- most Windows users never add MAME to PATH.
    with open(sh_path, "w", encoding="ascii", newline="\n") as f:
        f.write(
            "#!/bin/sh\n"
            "set -eu\n"
            'HERE="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"\n'
            'PLATFORM="${1:-mvs}"\n'
            'if [ "$PLATFORM" = "aes" ]; then DRIVER=aes; BIOS=asia; else DRIVER=neogeo; BIOS=euro; fi\n'
            'MAME_EXE="mame"\n'
            'if [ -x "$HERE/../mame" ]; then MAME_EXE="$HERE/../mame"; '
            'elif [ -x "$HERE/../mame64" ]; then MAME_EXE="$HERE/../mame64"; fi\n'
            f'exec "$MAME_EXE" "$DRIVER" -rompath "$HERE/roms" -hashpath "$HERE/hash/{game}" '
            f'-bios "$BIOS" -cart1 {game}\n'
        )
    sh_path.chmod(0o755)

    with open(bat_path, "w", encoding="ascii", newline="\r\n") as f:
        f.write(
            "@echo off\n"
            "setlocal\n"
            'set "HERE=%~dp0"\n'
            'set "PLATFORM=%~1"\n'
            'if "%PLATFORM%"=="" set "PLATFORM=mvs"\n'
            'if /I "%PLATFORM%"=="aes" (set DRIVER=aes& set BIOS=asia) else (set DRIVER=neogeo& set BIOS=euro)\n'
            'set "MAME_EXE=mame"\n'
            'if exist "%HERE%..\\mame.exe" set "MAME_EXE=%HERE%..\\mame.exe"\n'
            'if not exist "%HERE%..\\mame.exe" if exist "%HERE%..\\mame64.exe" set "MAME_EXE=%HERE%..\\mame64.exe"\n'
            f'"%MAME_EXE%" %DRIVER% -rompath "%HERE%roms" -hashpath "%HERE%hash\\{game}" '
            f'-bios %BIOS% -cart1 {game}\n'
        )
    (target_root / "README.txt").write_text(
        f"NeoGeo SDK games installed by mame_launcher.py.\n\n"
        f"Run '{game}' directly with:\n"
        f"    run_{game}.sh          (Linux/macOS, MVS)\n"
        f"    run_{game}.sh aes      (Linux/macOS, AES)\n"
        f"    run_{game}.bat         (Windows, MVS -- double-click it)\n"
        f"    run_{game}.bat aes     (Windows, AES)\n\n"
        "Those scripts find mame/mame.exe automatically in the folder one\n"
        "level up (where this neogeosdk/ folder was installed) -- MAME does\n"
        "not need to be on PATH. To call MAME yourself instead, from this\n"
        "MAME installation's own folder:\n"
        f"    mame neogeo -rompath neogeosdk/roms -hashpath neogeosdk/hash/{game} -bios euro -cart1 {game}\n"
        f"    mame aes    -rompath neogeosdk/roms -hashpath neogeosdk/hash/{game} -bios asia -cart1 {game}\n\n"
        "Emulator checksum warnings are expected for replacement system firmware.\n",
        encoding="utf-8",
    )


def run_game(game, mame_path, platform="mvs", extra_args=None):
    target_root = Path(mame_path) / "neogeosdk"
    roms_root = target_root / "roms"
    hash_dir = target_root / "hash" / game
    if not hash_dir.is_dir():
        raise SystemExit(f"{game} is not installed under {target_root}. Run 'install' first.")

    mame_exe = find_mame_executable(mame_path)
    if not mame_exe:
        raise SystemExit(
            "Could not find a mame/mame64 executable in the configured MAME path "
            "or on PATH. Run 'set-path' to point at the right folder, or install "
            "MAME and make sure it's reachable."
        )

    driver = DRIVER_NAME.get(platform, "neogeo")
    bios = BIOS_NAME.get(platform, "euro")
    argv = [
        mame_exe, driver,
        "-rompath", str(roms_root),
        "-hashpath", str(hash_dir),
        "-bios", bios,
        "-cart1", game,
    ]
    if extra_args:
        argv.extend(extra_args)
    print("Launching:", " ".join(argv))
    return subprocess.run(argv).returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    p_list = sub.add_parser("list", help="List buildable games")

    p_setpath = sub.add_parser("set-path", help="Save the MAME installation path")
    p_setpath.add_argument("path", nargs="?")

    p_install = sub.add_parser("install", help="Install a built dist zip into MAME")
    p_install.add_argument("--game", help="Game name, or 'all'; prompted if omitted")
    p_install.add_argument("--mame-path")
    p_install.add_argument("--dist-dir")
    p_install.add_argument("--non-interactive", action="store_true")

    p_run = sub.add_parser("run", help="Launch an installed game in MAME")
    p_run.add_argument("--game", help="Game name; prompted if omitted")
    p_run.add_argument("--platform", choices=["mvs", "aes"], default="mvs")
    p_run.add_argument("--mame-path")
    p_run.add_argument("--non-interactive", action="store_true")

    p_both = sub.add_parser("install-run", help="Install then immediately launch a game")
    p_both.add_argument("--game", help="Game name; prompted if omitted")
    p_both.add_argument("--platform", choices=["mvs", "aes"], default="mvs")
    p_both.add_argument("--mame-path")
    p_both.add_argument("--dist-dir")
    p_both.add_argument("--non-interactive", action="store_true")

    args = parser.parse_args()

    if args.command == "list":
        for name in discover_games():
            print(name)
        return

    if args.command == "set-path":
        mame_path = resolve_mame_path(args.path, interactive=True)
        print(f"Saved MAME path: {mame_path}")
        return

    games = discover_games()
    interactive = not args.non_interactive

    def pick_game(requested, allow_all=False):
        if requested:
            return requested
        if not interactive:
            raise SystemExit("--game is required with --non-interactive")
        return prompt_choose_game(games, allow_all=allow_all)

    if args.command == "install":
        game_choice = pick_game(args.game, allow_all=True)
        if game_choice not in games and game_choice != "all":
            raise SystemExit(f"Unknown game '{game_choice}'. Known games: {', '.join(games)}")
        mame_path = resolve_mame_path(args.mame_path, interactive=interactive)
        targets = games if game_choice == "all" else [game_choice]
        for game in targets:
            install_game(game, mame_path, args.dist_dir)
        return

    if args.command == "run":
        game_choice = pick_game(args.game)
        if game_choice not in games:
            raise SystemExit(f"Unknown game '{game_choice}'. Known games: {', '.join(games)}")
        mame_path = resolve_mame_path(args.mame_path, interactive=interactive)
        sys.exit(run_game(game_choice, mame_path, args.platform))

    if args.command == "install-run":
        game_choice = pick_game(args.game)
        if game_choice not in games:
            raise SystemExit(f"Unknown game '{game_choice}'. Known games: {', '.join(games)}")
        mame_path = resolve_mame_path(args.mame_path, interactive=interactive)
        install_game(game_choice, mame_path, args.dist_dir)
        sys.exit(run_game(game_choice, mame_path, args.platform))


if __name__ == "__main__":
    main()
