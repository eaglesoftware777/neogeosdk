#!/usr/bin/env python3
"""Isolated MVS/AES firmware tests; never searches installed system ROM sets.

    python3 bios/test_bios.py                      # firmware contract probe, both boards
    python3 bios/test_bios.py --game helloworld    # an SDK cartridge from roms/helloworld
    python3 bios/test_bios.py --game probe0 --platform aes   # system eye-catcher path
    python3 bios/test_bios.py --game helloworld --platform aes --p1 <console build of 772-p1.p1>

The probe is a purpose-built cartridge that records what the firmware handed
it (registers, request order, coin and start bookkeeping, message output).
Any SDK game in roms/<name>/ can be booted the same way to check that it
reaches its attract mode, takes a coin and a start, and returns to attract.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import wave
import xml.etree.ElementTree as ET
import zipfile
import zlib

BIOS = Path(__file__).resolve().parent
ROOT = BIOS.parent
PARTS = ("p1", "s1", "m1", "v1", "c1", "c2")


def probe_rom(out, toolchain, logoflag):
    prefix = str(Path(toolchain) / "m68k-unknown-elf-")
    subprocess.run([prefix + "as", "-m68000", f"--defsym=LOGOFLAG={logoflag}",
                    str(BIOS / "tests/probe.s"), "-o", str(out / "probe.o")], check=True)
    subprocess.run([prefix + "ld", "-Ttext=0", "-e", "0x122", str(out / "probe.o"),
                    "-o", str(out / "probe.elf")], check=True)
    subprocess.run([prefix + "objcopy", "-O", "binary", str(out / "probe.elf"),
                    str(out / "probe.bin")], check=True)
    data = (out / "probe.bin").read_bytes().ljust(0x100000, b"\xff")
    swapped = bytearray(data)
    swapped[0::2], swapped[1::2] = data[1::2], data[0::2]
    return bytes(swapped)


def sdk_rom_set(name):
    """The six ROM files of an SDK game built into roms/<name>/."""
    directory = ROOT / "roms" / name
    p1 = sorted(directory.glob("*-p1.p1"))
    if not p1:
        raise SystemExit(f"No <id>-p1.p1 in {directory}; build the game first")
    game_id = p1[0].name.split("-")[0]
    data = {}
    for part in PARTS:
        path = directory / f"{game_id}-{part}.{part}"
        if not path.exists():
            raise SystemExit(f"Missing {path}; build the whole ROM set first")
        data[part] = path.read_bytes()
    return data


def prepare_retail(args, out, cart):
    """Use the emulator's verified board layout, including protection and gaps."""
    if not args.cartridge:
        raise SystemExit("--game ssideki requires --cartridge PATH to your cartridge ZIP")
    if not args.software_list.is_file():
        raise SystemExit("Provide MAME's hash/neogeo.xml with --software-list PATH")
    reference = ET.parse(args.software_list).getroot().find("software[@name='ssideki']")
    if reference is None:
        raise SystemExit("The software list has no ssideki entry")
    with zipfile.ZipFile(args.cartridge) as archive:
        for rom in reference.findall("./part/dataarea/rom[@crc]"):
            candidates = [entry for entry in archive.infolist()
                          if entry.CRC == int(rom.get("crc"), 16)]
            if len(candidates) != 1:
                raise SystemExit(f"Not the verified retail cartridge: {rom.get('name')} CRC mismatch. "
                                 "A replacement homebrew set is not a retail compatibility test.")
            data = archive.read(candidates[0])
            if hashlib.sha1(data).hexdigest() != rom.get("sha1"):
                raise SystemExit(f"Cartridge SHA1 mismatch: {rom.get('name')}")
            (cart / Path(rom.get("name")).name).write_bytes(data)
    listing = ET.Element("softwarelist", name="neogeo", description="EagleBIOS validation")
    listing.append(reference)
    (out / "hash").mkdir(exist_ok=True)
    ET.ElementTree(listing).write(out / "hash/neogeo.xml", encoding="utf-8", xml_declaration=True)


def prepare(args, out):
    roms = out / "roms"
    for machine in ("neogeo", "aes"):
        directory = roms / machine
        directory.mkdir(parents=True, exist_ok=True)
        for filename in ("sp-s2.sp1", "neo-epo.bin", "sfix.sfix", "sm1.sm1", "000-lo.lo"):
            shutil.copyfile(BIOS / filename, directory / filename)
    cart = roms / args.game
    cart.mkdir(exist_ok=True)
    if args.game.startswith("probe"):
        logoflag = 0 if args.game == "probe0" else 1
        data = {"p1": probe_rom(out, args.toolchain, logoflag), "s1": (BIOS / "sfix.sfix").read_bytes(),
                "m1": (BIOS / "sm1.sm1").read_bytes(), "v1": bytes(0x20000),
                "c1": bytes(0x20000), "c2": bytes(0x20000)}
    elif args.game == "ssideki":
        prepare_retail(args, out, cart)
        return
    else:
        data = sdk_rom_set(args.game)
        if args.p1:
            # A console build of the same game (make PLATFORM=aes p1) shares
            # every ROM but the program; swap only that one in.
            data["p1"] = Path(args.p1).read_bytes()
    listing = ET.Element("softwarelist", name="neogeo", description="EagleBIOS validation")
    software = ET.SubElement(listing, "software", name=args.game)
    for tag, text in (("description", "EagleBIOS test cartridge"), ("year", "2026"),
                      ("publisher", "Eagle Software")):
        ET.SubElement(software, tag).text = text
    ET.SubElement(software, "sharedfeat", name="compatibility", value="MVS,AES")
    part = ET.SubElement(software, "part", name="cart", interface="neo_cart")
    ET.SubElement(part, "feature", name="slot", value="rom")
    for area_name, members, flags in (
        ("maincpu", ["p1"], {"width": "16", "endianness": "big"}),
        ("fixed", ["s1"], {}), ("audiocpu", ["m1"], {}),
        ("ymsnd:adpcma", ["v1"], {}), ("sprites", ["c1", "c2"], {}),
    ):
        area = ET.SubElement(part, "dataarea", name=area_name,
                             size=hex(sum(len(data[m]) for m in members)), **flags)
        for i, member in enumerate(members):
            payload = data[member]
            filename = f"cart-{member}.bin"
            (cart / filename).write_bytes(payload)
            options = {"loadflag": "load16_byte"} if member.startswith("c") else {}
            if member == "p1":
                options["loadflag"] = "load16_word_swap"
            ET.SubElement(area, "rom", name=filename, offset=hex(i), size=hex(len(payload)),
                          crc=f"{zlib.crc32(payload):08x}", sha1=hashlib.sha1(payload).hexdigest(), **options)
    (out / "hash").mkdir(exist_ok=True)
    ET.ElementTree(listing).write(out / "hash/neogeo.xml", encoding="utf-8", xml_declaration=True)


def check_probe(samples, platform, logoflag):
    last = samples[-1]
    assert last["sentinel"] == 0xDEADBEEF, "BIOS wrote into cartridge work RAM"
    assert last["entry_sr"] == 0x2700, "Incorrect USER entry interrupt mask"
    assert last["mode"] == 2 and last["starts"] == 1, "START was not accepted exactly once"
    assert last["mvs"] == (128 if platform == "mvs" else 0), "Wrong hardware identity"
    assert last["pad_status"] == 1, "Idle pad lost its connected status"
    assert last["message"] == 69 and last["increment"] == 0, "MESS_OUT output is incorrect"
    assert last["inline"] == 0x104B, "MESS_OUT inline command stream was not run"
    assert last["mess_point"] == 0x10FF00, "Message queue was not reset"
    assert last["dips"] == [1, 0x30, 0xFF, 0xFF, 3, 0xFF,
                            2, 2, 1, 0, 0, 0, 0, 0, 0, 0], "Soft-DIP defaults were not decoded"
    if platform == "mvs":
        assert last["coins"] == 10 and last["credit"] == 9, "BCD coin accounting failed"
        assert last["requests"] == 13, "MVS request flow must be INIT, DEMO, TITLE"
    elif logoflag == 1:
        assert last["coins"] == 0 and last["requests"] == 7, "AES cartridge eye-catcher flow failed"
    else:
        assert last["requests"] == 5, "AES system eye-catcher must skip request 1"
        first_demo = next(s["time"] for s in samples if s["request"] == 2)
        assert first_demo >= 2.5, "System eye-catcher was not shown before the demo"
    print("Firmware contract assertions passed")


def check_game(samples, platform):
    """An SDK cartridge must reach attract, then take a coin and a start."""
    # A console cartridge on free play may go straight into its game (mode 2).
    attract = [s for s in samples if s["request"] == 2 and s["mode"] in ((1,) if platform == "mvs" else (1, 2))]
    assert attract, "Cartridge never reached its attract mode"
    assert attract[0]["time"] <= 7.5, "Attract mode came up too late"
    assert all(s["mvs"] == (128 if platform == "mvs" else 0) for s in samples[1:]), "Hardware identity changed"
    if platform == "mvs":
        before = [s["credit"] for s in samples if 8.5 <= s["time"] < 13.0]
        assert before and max(before) >= 1, "Coin was not credited"
        # Credits are BCD; a start must have spent exactly one of them.
        spent = int(f"{max(before):x}") - int(f"{samples[-1]['credit']:x}")
        assert spent == 1, f"Start should spend one credit, spent {spent}"
        assert any(s["request"] == 3 for s in samples), "Coin did not bring up the title"
    print("Cartridge compatibility assertions passed")


def run(args, platform):
    out = args.output.resolve() / f"{args.game}-{platform}"
    out.mkdir(parents=True, exist_ok=True)
    prepare(args, out)
    env = dict(os.environ, DISPLAY="", SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy",
               EAGLE_TEST_DIR=str(out), EAGLE_TEST_GAME=args.game,
               EAGLE_TEST_PLATFORM=platform)
    machine = "neogeo" if platform == "mvs" else "aes"
    command = [args.mame, machine, "-noreadconfig", "-rompath", str(out / "roms"),
               "-hashpath", str(out / "hash"), "-bios", "euro" if platform == "mvs" else "asia",
               "-cart1", args.game, "-video", "none", "-nothrottle", "-skip_gameinfo",
               "-seconds_to_run", str(args.seconds), "-nonvram_save", "-autoboot_delay", "0",
               "-cfg_directory", str(out / "cfg"), "-nvram_directory", str(out / "nvram"),
               "-snapshot_directory", str(out), "-autoboot_script", str(BIOS / "tests/capture.lua"),
               "-wavwrite", str(out / "audio.wav")]
    with (out / "mame.log").open("w") as log:
        result = subprocess.run(command, env=env, cwd=out, stdout=log, stderr=subprocess.STDOUT,
                                timeout=max(120, args.seconds * 10))
    if result.returncode:
        raise SystemExit((out / "mame.log").read_text()[-6000:])
    samples = [json.loads(line) for line in (out / "state.jsonl").read_text().splitlines()]
    if not samples or "[LUA ERROR]" in (out / "mame.log").read_text():
        raise SystemExit(f"Capture failed; inspect {out / 'mame.log'}")
    print(f"{args.game} {platform}: {samples[-1]}")
    if args.game.startswith("probe"):
        check_probe(samples, platform, 0 if args.game == "probe0" else 1)
    else:
        check_game(samples, platform)
    with wave.open(str(out / "audio.wav")) as audio:
        import array
        pcm = array.array("h", audio.readframes(audio.getnframes()))
        print(f"Audio peak: {max(map(abs, pcm), default=0)}; captures: {out}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--game", default="probe",
                        help="probe, probe0 (system eye-catcher), ssideki, or an SDK game under roms/")
    parser.add_argument("--platform", choices=("mvs", "aes", "both"), default="both")
    parser.add_argument("--seconds", type=int, default=18)
    parser.add_argument("--output", type=Path, default=BIOS / "out/tests")
    parser.add_argument("--mame", default=shutil.which("mame") or "/usr/games/mame")
    parser.add_argument("--cartridge", type=Path)
    parser.add_argument("--software-list", type=Path,
                        default=Path("/usr/share/games/mame/hash/neogeo.xml"),
                        help="MAME software list used to verify retail ROMs and board layout")
    parser.add_argument("--p1", type=Path, help="alternate program ROM for an SDK game (an AES build)")
    parser.add_argument("--toolchain", default=os.getenv("TOOLCHAIN", str(ROOT.parent / "x-tools-v3/m68k-unknown-elf/bin")))
    args = parser.parse_args()
    if args.seconds < 18:
        parser.error("Use at least 18 seconds to exercise coin and start transitions")
    for platform in (("mvs", "aes") if args.platform == "both" else (args.platform,)):
        run(args, platform)


if __name__ == "__main__":
    main()
