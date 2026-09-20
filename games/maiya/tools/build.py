r"""Build and run Maiya: Super Nature Girl without changing shared SDK outputs.

    python3 games/maiya/tools/build.py
    python3 games/maiya/tools/build.py --run
    py games\maiya\tools\build.py --run
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
WORK = GAME / "build/workspace"
TEXT_SUFFIXES = {".c", ".h", ".hpp", ".cpp", ".ld", ".py", ".sh", ".mk", ".mak", ".cfg"}


def copy_source(source, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    data = source.read_bytes()
    if source.suffix in TEXT_SUFFIXES or source.name == "Makefile":
        data = data.replace(b"\r\n", b"\n")
    destination.write_bytes(data)
    if source.suffix == ".sh":
        destination.chmod(0o755)


def stage():
    for name in ("sdk", "tools", "hash_eagle"):
        for source in (ROOT / name).rglob("*"):
            if source.is_file() and (source.suffix in TEXT_SUFFIXES):
                copy_source(source, WORK / source.relative_to(ROOT))
    for name in ("Makefile", "MakefileWin32.mak"):
        copy_source(ROOT / name, WORK / name)
    # The Windows makefile runs the ROM tools from win\ next to it.
    for source in (ROOT / "win").glob("*"):
        if source.is_file():
            (WORK / "win").mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, WORK / "win" / source.name)
    for source in GAME.rglob("*"):
        relative = source.relative_to(GAME)
        if relative.parts[0] in {"build", "assets", "tests"} or "__pycache__" in relative.parts:
            continue
        if source.is_file() and (source.suffix in TEXT_SUFFIXES):
            copy_source(source, WORK / "games/maiya" / relative)
    roms = WORK / "roms/maiya"
    roms.mkdir(parents=True, exist_ok=True)
    (WORK / "out").mkdir(exist_ok=True)
    # The initial game uses the exact tested demo sound and font bank. No
    # assembler or shared sample table is rebuilt or modified by this tool.
    provenance = {}
    for suffix in ("m1.m1", "v1.v1"):
        source = ROOT / "roms/maiya" / f"780-{suffix}"
        if not source.is_file():
            source = ROOT / "out" / f"780-{suffix}"
        shutil.copy2(source, roms / f"780-{suffix}")
        provenance[str(source.relative_to(ROOT))] = hashlib.sha256(source.read_bytes()).hexdigest()
    source_s1 = GAME / "artbox/780-s1.s1"
    shutil.copy2(source_s1, roms / "780-s1.s1")
    provenance[str(source_s1.relative_to(ROOT))] = hashlib.sha256(source_s1.read_bytes()).hexdigest()
    for suffix in ("c1.c1", "c2.c2"):
        source_c = GAME / "artbox/generated" / f"780-{suffix}"
        shutil.copy2(source_c, roms / f"780-{suffix}")
        provenance[str(source_c.relative_to(ROOT))] = hashlib.sha256(source_c.read_bytes()).hexdigest()
    (GAME / "build/bank-provenance.json").write_text(json.dumps(provenance, indent=2) + "\n")


def run(mame):
    command = [mame, "neogeo", "-noreadconfig", "-rompath",
               f"{WORK / 'roms'};{ROOT / 'roms'}", "-hashpath",
               str(WORK / "hash_eagle/maiya"), "-cart1", "maiya",
               "-bios", "euro", "-window", "-nofilter", "-waitvsync",
               "-noautoframeskip", "-frameskip", "0", "-skip_gameinfo",
               "-cfg_directory", str(GAME / "build/cfg"),
               "-nvram_directory", str(GAME / "build/nvram")]
    subprocess.run(command, cwd=GAME / "build", check=True)


def quick_build(toolchain):
    """Recompile this game's scene against the previously staged SDK objects."""
    if not (WORK / "out/neogeolib.o").exists():
        raise SystemExit("Run a full build first.")
    copy_source(GAME / "scenes/maiya_game.c", WORK / "games/maiya/scenes/maiya_game.c")
    copy_source(GAME / "scenes/maiya_levels.h", WORK / "games/maiya/scenes/maiya_levels.h")
    copy_source(GAME / "artbox/generated/maiya_assets.h",
                WORK / "games/maiya/artbox/generated/maiya_assets.h")
    binary = toolchain / "m68k-unknown-elf/bin"
    suffix = ".exe" if os.name == "nt" else ""
    def command(name, *args):
        subprocess.run([str(binary / f"m68k-unknown-elf-{name}{suffix}"), *map(str, args)],
                       cwd=WORK, check=True)
    command("gcc", "-c", "-O2", "-g", "-m68000", "-ffreestanding", "-fomit-frame-pointer",
            "-std=gnu99", "-Wall", "-I.", "-Isdk", "-Isdk/2d_engine", "-Igames/maiya",
            "games/maiya/scenes/maiya_game.c", "-o", "out/maiya_game0.o")
    copy_source(GAME / "user.c", WORK / "games/maiya/user.c")
    command("gcc", "-c", "-O0", "-m68000", "-ffreestanding", "-fomit-frame-pointer",
            "-std=gnu99", "-I.", "-Isdk", "-Isdk/2d_engine", "-DNG_MVS=1",
            "games/maiya/user.c", "-o", "out/user0.o")
    command("objcopy", "-R", ".comment", "-R", ".text", "-R", ".data", "-R", ".bss",
            "out/user0.o", "out/user.o")
    objects = ["neogeo.o", "user.o", "main.o", "eyecatcher.o", "neogeolib.o"]
    objects += sorted(p.name for p in (WORK / "out").glob("ng_*0.o"))
    objects += ["maiya_game0.o"]
    command("ld", "-nostdlib", "-T", "games/maiya/neogeo.ld", "-o", "out/game",
            *(f"out/{name}" for name in objects))
    command("objcopy", "-O", "binary", "out/game", "out/game.raw")
    data = (WORK / "out/game.raw").read_bytes()
    if len(data) > 524288:
        raise SystemExit("P1 exceeds its ROM reservation")
    data = data.ljust(524288, b"\xff")
    swapped = bytearray(len(data))
    swapped[0::2], swapped[1::2] = data[1::2], data[0::2]
    (WORK / "roms/maiya/780-p1.p1").write_bytes(swapped)
    for part in ("c1", "c2"):
        name = f"780-{part}.{part}"
        shutil.copyfile(GAME / "artbox/generated" / name, WORK / "roms/maiya" / name)
    shutil.copyfile(GAME / "artbox/780-s1.s1", WORK / "roms/maiya/780-s1.s1")
    subprocess.run([sys.executable, "hash_eagle/gen_hash.py"], cwd=WORK,
                   env=dict(os.environ, GAME="maiya", GAME_ID="780"), check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--run-only", action="store_true")
    parser.add_argument("--rebuild-art", action="store_true")
    parser.add_argument("--quick", action="store_true", help="Rebuild scene code only, using the staged SDK")
    parser.add_argument("--mame", default="mame")
    parser.add_argument("--make", default="make")
    parser.add_argument("--toolchain", type=Path, help="Directory containing m68k-unknown-elf/")
    args = parser.parse_args()
    if not args.run_only:
        if args.rebuild_art or not (GAME / "artbox/generated/maiya_assets.h").is_file():
            subprocess.run([sys.executable, str(GAME / "tools/build_commercial_assets.py")], check=True)
        if args.quick:
            toolchain = args.toolchain
            if toolchain is None:
                toolchain = next((ROOT.parent / p for p in ("x-tools-v3", "x-tools-v2", "x-tools")
                                  if (ROOT.parent / p).is_dir()), None)
            if toolchain is None:
                raise SystemExit("Pass --toolchain with the cross compiler location")
            quick_build(toolchain)
            if args.run:
                run(args.mame)
            return
        stage()
        command = [args.make]
        if os.name == "nt":
            command += ["-f", "MakefileWin32.mak"]
        command += ["GAME=maiya", "GAME_CFG_FILE=games/maiya/game.cfg",
                    f"SDKHOME={ROOT.parent}", "p1"]
        if args.toolchain:
            command.append(f"XTOOLS_ROOT={args.toolchain.resolve()}")
        with (GAME / "build/build.log").open("w", encoding="utf-8") as log:
            result = subprocess.run(command, cwd=WORK, stdout=log, stderr=subprocess.STDOUT)
        if result.returncode:
            print((GAME / "build/build.log").read_text()[-12000:])
            raise SystemExit(result.returncode)
        print(f"Built {WORK / 'roms/maiya'}")
        print(f"Build log: {GAME / 'build/build.log'}")
    if args.run or args.run_only:
        run(args.mame)


if __name__ == "__main__":
    main()
