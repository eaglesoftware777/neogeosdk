"""Portable EagleBIOS build and isolated packaging for Windows and Linux."""

import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

BIOS = Path(__file__).resolve().parents[1]
ROOT = BIOS.parent
FIRMWARE = {
    "neogeo": ("sp-s2.sp1", "sm1.sm1", "sfix.sfix", "000-lo.lo"),
    "aes": ("neo-epo.bin", "000-lo.lo"),
}


def rom_image(data, swapped=False):
    if not data or len(data) > 0x20000:
        raise ValueError("Firmware must fit its 128 KiB ROM")
    data = data.ljust(0x20000, b"\xff")
    if swapped:
        result = bytearray(data)
        result[0::2], result[1::2] = data[1::2], data[0::2]
        return bytes(result)
    return data


def build(cc, wlaz80, wlalink):
    out = BIOS / "out"
    out.mkdir(exist_ok=True)
    cc = shutil.which(cc) or cc
    suffix = ".exe" if cc.lower().endswith(".exe") else ""
    prefix = cc[:-(len("gcc") + len(suffix))]
    if not cc.endswith("gcc" + suffix):
        raise ValueError("--cc must name the m68k GCC executable")

    def run(tool, *args):
        subprocess.run([prefix + tool + suffix, *args], cwd=BIOS, check=True)

    objects = []
    for source in sorted((BIOS / "src").glob("*.c")):
        obj = "out/" + source.stem + ".o"
        run("gcc", "-c", "-O2", "-fomit-frame-pointer", "-Wall", "-Wextra",
            "-Wno-array-bounds", "-m68000", "-ffreestanding", "-fno-builtin",
            "-Iinclude", str(source), "-o", obj)
        objects.append(obj)
    for board, ident, filename in (("mvs", "0x8002", "sp-s2.sp1"),
                                   ("aes", "0x0002", "neo-epo.bin")):
        entry = f"out/entry_{board}.o"
        elf = f"out/firmware_{board}.elf"
        raw = f"out/firmware_{board}.bin"
        run("as", "-m68000", "--defsym", "BIOS_ID=" + ident, "src/bios_entry.s", "-o", entry)
        run("ld", "-nostdlib", "-T", "eagle_bios.ld", entry, *objects, "-o", elf)
        run("objcopy", "-O", "binary", elf, raw)
        data = (BIOS / raw).read_bytes()
        (BIOS / filename).write_bytes(rom_image(data, swapped=True))
        if board == "mvs":
            (BIOS / "eagle_bios.rom").write_bytes(rom_image(data))
    subprocess.run([wlaz80, "-o", "out/bios_sound.o", "src/bios_sound.s"], cwd=BIOS, check=True)
    (out / "sm1.link").write_text("[objects]\nout/bios_sound.o\n", encoding="ascii")
    subprocess.run([wlalink, "-S", "-r", "out/sm1.link", "out/sm1_raw.bin"], cwd=BIOS, check=True)
    (BIOS / "sm1.sm1").write_bytes(rom_image((out / "sm1_raw.bin").read_bytes()))
    for generator, filename in (("gen_sfix.py", "sfix.sfix"), ("gen_lo.py", "000-lo.lo")):
        subprocess.run([sys.executable, str(BIOS / "tools" / generator), str(BIOS / filename)], check=True)


def install(destination):
    for board, files in FIRMWARE.items():
        target = destination / board
        target.mkdir(parents=True, exist_ok=True)
        for filename in files:
            source = BIOS / filename
            if source.stat().st_size != 0x20000:
                raise ValueError(f"Wrong firmware size: {source}")
            shutil.copyfile(source, target / filename)


def package(game, game_id, destination):
    cart = ROOT / "roms" / game
    parts = [cart / f"{game_id}-{part}.{part}" for part in ("p1", "s1", "m1", "v1", "c1", "c2")]
    listing = ROOT / "hash_eagle" / game / "neogeo.xml"
    for path in [*parts, listing]:
        if not path.is_file():
            raise FileNotFoundError(f"Build GAME={game} all and hash first: {path}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED) as archive:
        for path in parts:
            archive.write(path, f"roms/{game}/{path.name}")
        for board, files in FIRMWARE.items():
            for filename in files:
                archive.write(BIOS / filename, f"roms/{board}/{filename}")
        archive.write(listing, "hash/neogeo.xml")
        archive.write(BIOS / "README.md", "EagleBIOS.md")
        archive.write(ROOT / "LICENSE", "LICENSE")
        archive.writestr("RUN.txt", f"mame neogeo -rompath roms -hashpath hash -bios euro -cart1 {game}\n"
                         "For AES, compile the cartridge with PLATFORM=aes, package again, and use:\n"
                         f"mame aes -rompath roms -hashpath hash -bios asia -cart1 {game}\n"
                         "Emulator checksum warnings are expected for replacement system firmware.\n")
    print(f"Packaged {destination}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="m68k-unknown-elf-gcc")
    parser.add_argument("--wlaz80", default="wla-z80")
    parser.add_argument("--wlalink", default="wlalink")
    parser.add_argument("--no-build", action="store_true")
    parser.add_argument("--install", type=Path)
    parser.add_argument("--package", type=Path)
    parser.add_argument("--game")
    parser.add_argument("--game-id")
    args = parser.parse_args()
    if args.package and (not args.game or not args.game_id):
        parser.error("--package requires --game and --game-id")
    if not args.no_build:
        build(args.cc, args.wlaz80, args.wlalink)
    if args.install:
        install(args.install.resolve())
    if args.package:
        package(args.game, args.game_id, args.package.resolve())


if __name__ == "__main__":
    main()
