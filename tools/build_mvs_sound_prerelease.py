"""Build a sound-only MVS test image without regenerating game assets."""

import argparse
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists() or args.output.with_suffix(".json").exists():
        parser.error("Output already exists; preserve previous test images")
    root = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory() as temporary:
        temporary = Path(temporary)
        obj = temporary / "driver.o"
        link = temporary / "driver.link"
        binary = temporary / "m1.bin"
        subprocess.run(["wla-z80", "-I", str(root / "sound/driver"),
                        "-o", str(obj), str(root / "sound/driver/driver.asm")],
                       check=True)
        link.write_text(f"[objects]\n{obj}\n", encoding="ascii")
        subprocess.run(["wlalink", "-r", str(link), str(binary)], check=True)
        data = binary.read_bytes()
        if len(data) != 65536:
            raise ValueError("Expected the authoritative 64 KiB driver bank")
        binary.write_bytes(data + b"\xff" * 65536)
        subprocess.run([
            sys.executable, str(root / "tools/patch_neosd_sound.py"),
            str(root / "dist/release/Maiya-WIP-NeoSD_MVS_v1.neo"),
            "--m1", str(binary), "--output", str(args.output.resolve())], check=True)


if __name__ == "__main__":
    main()
