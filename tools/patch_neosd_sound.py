"""Apply the slot-switch driver to a release image without replacing graphics."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
from functools import lru_cache

HEADER_SIZE = 4096
REGIONS = ("p1", "s1", "m1", "v1", "v2", "c")
# Verified soundInit prologue in the v1.7.1 cartridge, decoded 68000 byte order.
INIT_SIGNATURE = bytes.fromhex("487800014ebaffb0588f4e714e75")
RESET_SIGNATURE = bytes.fromhex("487800034ebaffa2588f4e714e75")
READY_SIGNATURE = bytes.fromhex("13fc0000003000011039003200000c0000016600ffec")
COMMAND_READY_SIGNATURE = bytes.fromhex("207c0032000010100c0000016616")
M1_READY_SIGNATURE = bytes.fromhex("3e80d30c")
M1_SLOT_SIGNATURE = bytes.fromhex("fb3e01d30cc385ff")


def find_m68k_prefix():
    configured = os.environ.get("M68K_PREFIX")
    if configured:
        return configured
    for prefix in ("m68k-unknown-elf-", "m68k-elf-"):
        assembler = shutil.which(prefix + "as")
        if assembler:
            return assembler[:-2]
    parent = Path(__file__).resolve().parents[2]
    for folder in ("x-tools-v2", "x-tools-v2-win", "x-tools"):
        for triple in ("m68k-unknown-elf", "m68k-elf"):
            prefix = parent / folder / triple / "bin" / (triple + "-")
            if Path(str(prefix) + "as" + (".exe" if os.name == "nt" else "")).exists():
                return str(prefix)
    raise ValueError("Set M68K_PREFIX to an installed 68000 binutils prefix")


@lru_cache(maxsize=4)
def build_bootstrap(ready, command_tail, origin):
    prefix = find_m68k_prefix()
    suffix = ".exe" if os.name == "nt" else ""
    with tempfile.TemporaryDirectory() as temporary:
        tmp = Path(temporary)
        obj, elf, binary = (tmp / name for name in ("boot.o", "boot.elf", "boot.bin"))
        subprocess.run([prefix + "as" + suffix, "-m68000",
                        "--defsym", f"sound_ready={ready}",
                        "--defsym", f"sound_command_tail={command_tail}",
                        "-o", str(obj), str(Path(__file__).with_name("mvs_sound_bootstrap.s"))], check=True)
        subprocess.run([prefix + "ld" + suffix, f"-Ttext=0x{origin:x}",
                        "-e", "bootstrap_init", "-o", str(elf), str(obj)], check=True)
        subprocess.run([prefix + "objcopy" + suffix, "-O", "binary", "-j", ".text",
                        str(elf), str(binary)], check=True)
        return binary.read_bytes()


def patch_program(program):
    p = bytearray(swap_words(program))
    for signature in (INIT_SIGNATURE, RESET_SIGNATURE, READY_SIGNATURE,
                      COMMAND_READY_SIGNATURE):
        if p.count(signature) != 1:
            raise ValueError("Expected exactly one verified release sound protocol signature")
    init = p.index(INIT_SIGNATURE)
    reset = p.index(RESET_SIGNATURE)
    ready = p.index(READY_SIGNATURE)
    command_ready = p.index(COMMAND_READY_SIGNATURE)
    if reset != init + len(INIT_SIGNATURE):
        raise ValueError("Unexpected release sound control function layout")
    command = init + 6 + struct.unpack_from(">h", p, init + 6)[0]
    if not command <= command_ready < init:
        raise ValueError("Ready comparison is outside the verified soundCommand function")
    prologue = bytes.fromhex("598f4eba") + struct.pack(">h", ready - (command + 4))
    if p[command:command + 6] != prologue:
        raise ValueError("Unrecognized soundCommand prologue")
    trampoline = len(p) - 256
    if p[trampoline:] != b"\xff" * 256:
        raise ValueError("Bootstrap reservation is not erased program-ROM padding")
    # Send init without waiting on a BIOS slot reply. Allow NMI to lower the
    # stale ready latch before calling the verified ordinary-ready polling loop.
    code = build_bootstrap(ready, command + 6, trampoline)
    if len(code) > 256:
        raise ValueError("Sound protocol adapter exceeds reserved padding")
    p[init:init + 14] = bytes.fromhex("4ef9") + struct.pack(">I", trampoline) + b"\x4e\x71" * 4
    p[reset + 3] = 8
    p[ready + 17] = 0x80
    p[command_ready + 11] = 0x80
    p[command:command + 6] = bytes.fromhex("4ef9") + struct.pack(">I", trampoline + 0x40)
    p[trampoline:trampoline + len(code)] = code
    return swap_words(p)


def read_regions(image):
    if len(image) < HEADER_SIZE or image[:4] != b"NEO\x01":
        raise ValueError("Not a NEO v1 image")
    sizes = struct.unpack_from("<6I", image, 4)
    if HEADER_SIZE + sum(sizes) != len(image):
        raise ValueError("Header sizes do not match the image length")
    regions = {}
    offset = HEADER_SIZE
    for name, size in zip(REGIONS, sizes):
        regions[name] = image[offset:offset + size]
        offset += size
    return regions


def swap_words(data):
    if len(data) & 1:
        raise ValueError("P ROM must contain complete 68000 words")
    result = bytearray(len(data))
    result[0::2], result[1::2] = data[1::2], data[0::2]
    return bytes(result)


def patch_image(base, m1):
    regions = read_regions(base)
    if len(m1) != len(regions["m1"]):
        raise ValueError("Replacement M1 must retain the release region size")
    if (m1[0x66:0xC0].count(M1_READY_SIGNATURE) != 1 or
            m1.count(M1_SLOT_SIGNATURE) != 1 or m1[0xC0:0xC4] != b"NGP2"):
        raise ValueError("Replacement M1 does not implement the paired ready/slot protocol")
    if len(regions["v1"]) != 0x800000:
        raise ValueError("Use the previously aligned 8 MiB V1 release image")
    regions["p1"] = patch_program(regions["p1"])
    regions["m1"] = m1
    output = base[:HEADER_SIZE] + b"".join(regions[name] for name in REGIONS)
    after = read_regions(output)
    before = read_regions(base)
    for name in ("s1", "v1", "v2", "c"):
        if before[name] != after[name]:
            raise AssertionError(f"Protected {name} region changed")
    if regions["p1"] != patch_program(before["p1"]):
        raise AssertionError("P1 differs from the verified sound-only protocol patch")
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("base", type=Path)
    parser.add_argument("--m1", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if args.output.exists() or args.output.with_suffix(".json").exists():
        parser.error("Output already exists; preserve previous test images")
    base = args.base.read_bytes()
    output = patch_image(base, args.m1.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(output)
    before, after = read_regions(base), read_regions(output)
    manifest = {"base": args.base.name, "output": args.output.name,
                "sha256": hashlib.sha256(output).hexdigest(),
                "hardware_test": "pending", "regions": {},
                "protocol": {"ordinary_ready": "80", "slot_reply": "01",
                             "bios_reset": "03", "scene_reset": "08",
                             "game_init": "09", "parameter_escape": "FF",
                             "escaped_values": ["01", "02", "03", "09", "FF"],
                             "escape_transform": "xor 80", "revision": "NGP2"}}
    for name in REGIONS:
        manifest["regions"][name] = {
            "size": len(after[name]), "unchanged": before[name] == after[name],
            "sha256": hashlib.sha256(after[name]).hexdigest()}
    args.output.with_suffix(".json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
