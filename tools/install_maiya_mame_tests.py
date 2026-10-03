"""Install Maiya's MVS and AES test sets, built by `make neo GAME=maiya`, into a
MAME folder's neogeosdk/ (e.g. C:\\mame\\neogeosdk), with its launchers.

    tests/mvs/roms/maiya, tests/mvs/hash   the MVS build (run_maiya_mvs.bat)
    tests/aes/roms/maiya, tests/aes/hash   the AES build (run_maiya_aes.bat)
    roms/maiya, roms/maiya.zip, hash/maiya the MVS build, for MAME's own lists

The launchers boot EagleBIOS (euro on MVS, asia on AES), installed in
roms/neogeo and roms/aes from bios/ (`make eagle-bios`). The UniBIOS
launchers need uni-bios_4_0.rom placed there by hand: no BIOS but EagleBIOS
is ever copied.

Everything replaced is copied first to backups/<time>-<label>/. With --tidy,
launchers this tool does not install (older .bat and .ps1 files) and the old
test folders that hold earlier builds move there too, so every launcher left
runs the build just installed. --clear-nvram moves the test sets' saved
settings and scores aside as well.

    python3 tools/install_maiya_mame_tests.py /mnt/c/mame/neogeosdk --tidy
"""

import argparse
from pathlib import Path
import shutil
import time
import zipfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
PARTS = ("p1", "s1", "m1", "v1", "c1", "c2")
EAGLE = {"neogeo": ("sp-s2.sp1", "sm1.sm1", "sfix.sfix", "000-lo.lo"),
         "aes": ("neo-epo.bin", "000-lo.lo")}
LAUNCHERS = ("run_maiya.bat", "run_maiya_mvs.bat", "run_maiya_aes.bat",
             "run_maiya_mvs_unibios.bat", "run_maiya_aes_unibios.bat")
OLD_FOLDERS = ("maiya-video-tests", "polish-tests", "presentation-pack", "render-v2-tests",
               "render-v3-tests", "video", "videos", "snap", "inp")


def crc(path):
    return zlib.crc32(path.read_bytes()) & 0xffffffff


def install(target, label, tidy, clear_nvram):
    sets = ROOT / "dist/neo/maiya"
    for platform in ("mvs", "aes"):
        for part in PARTS:
            if not (sets / platform / "roms/maiya" / f"780-{part}.{part}").is_file():
                raise SystemExit(f"Run make neo GAME=maiya first ({platform} {part} missing)")
    backup = target / "backups" / f"{time.strftime('%Y%m%d-%H%M%S')}-{label}"
    backup.mkdir(parents=True)

    def keep(path):
        """Copy a file or folder about to be replaced into the backup."""
        if path.exists():
            dest = backup / path.relative_to(target)
            dest.parent.mkdir(parents=True, exist_ok=True)
            (shutil.copytree if path.is_dir() else shutil.copy2)(path, dest)

    def move(path):
        """Move a file or folder no longer used into the backup."""
        dest = backup / path.relative_to(target)
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(path), str(dest))

    for platform in ("mvs", "aes"):
        tests = target / "tests" / platform
        keep(tests / "roms"); keep(tests / "hash")
        shutil.rmtree(tests / "roms/maiya", ignore_errors=True)
        shutil.copytree(sets / platform / "roms/maiya", tests / "roms/maiya")
        (tests / "hash").mkdir(parents=True, exist_ok=True)
        shutil.copy2(sets / platform / "hash/neogeo.xml", tests / "hash/neogeo.xml")
        if clear_nvram:
            for nvram in (tests / "nvram", target / "tests" / f"{platform}-unibios" / "nvram"):
                if nvram.exists():
                    move(nvram)
        print(f"{platform}: P1 {crc(tests / 'roms/maiya/780-p1.p1'):08x}")

    keep(target / "roms/maiya"); keep(target / "roms/maiya.zip"); keep(target / "hash/maiya")
    shutil.rmtree(target / "roms/maiya", ignore_errors=True)
    shutil.copytree(sets / "mvs/roms/maiya", target / "roms/maiya")
    (target / "hash/maiya").mkdir(parents=True, exist_ok=True)
    shutil.copy2(sets / "mvs/hash/neogeo.xml", target / "hash/maiya/neogeo.xml")
    with zipfile.ZipFile(target / "roms/maiya.zip", "w", zipfile.ZIP_DEFLATED) as z:
        for part in PARTS:
            z.write(sets / "mvs/roms/maiya" / f"780-{part}.{part}", f"maiya/780-{part}.{part}")

    for board, names in EAGLE.items():
        for name in names:
            source, dest = ROOT / "bios" / name, target / "roms" / board / name
            if not source.is_file():
                raise SystemExit("EagleBIOS is not built: run make eagle-bios")
            if not dest.is_file() or crc(dest) != crc(source):
                keep(dest)
                dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, dest)

    templates = ROOT / "tools/mame_tests"
    for name in LAUNCHERS + ("README.txt",):
        keep(target / name)
        text = (templates / name).read_text().replace("\r\n", "\n")
        # CMD batch files need Windows line endings even when installed from WSL.
        (target / name).write_bytes(text.replace("\n", "\r\n").encode("ascii"))

    if tidy:
        for path in sorted(target.iterdir()):
            old_launcher = path.suffix.lower() in (".bat", ".ps1", ".sh") and path.name not in LAUNCHERS
            if old_launcher or (path.is_dir() and path.name in OLD_FOLDERS) or path.name == "POLISH_TESTS.txt":
                move(path)
    print(f"Replaced and retired files are in {backup}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("target", type=Path, help="the MAME folder's neogeosdk/ folder")
    parser.add_argument("--label", default="before-install", help="the backup folder's suffix")
    parser.add_argument("--tidy", action="store_true", help="retire older launchers and test folders")
    parser.add_argument("--clear-nvram", action="store_true", help="start the test sets without saved settings")
    args = parser.parse_args()
    install(args.target, args.label, args.tidy, args.clear_nvram)


if __name__ == "__main__":
    main()
