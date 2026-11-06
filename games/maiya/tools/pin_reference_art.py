"""Keep Maiya's painted art exactly as a reference build drew it.

    python3 games/maiya/tools/pin_reference_art.py --reference v1.7.3

The painted assets (valleys, arenas, landmarks, creatures, the title) pass
through resampling and floating-point palette fitting, and two machines can
round those differently: the same sources built on two PCs give the same
tile layout but different pixels and palettes in about 76 of the 494 assets.
Her index-map frames (walk, run, throws ...) come out the same everywhere.

So that a release built on one machine stays the art that was reviewed on
another, this script takes the reference revision's C-ROM pair and its
palette/map arrays (read from that revision's out/game and out/game0.rom),
and lays over them only what the source edits since then changed:

    final = reference + (this build - this machine's build of the reference)

The reference sources are built here once, in a temporary git worktree, and
cached under artbox/generated/pin-baseline-<commit>/. The script stops if
the tile layout or any #define differs, if out/game at the reference is not
the build of its 780-p1.p1, or if the edits touch an array the reference
build left out. games/maiya/tools/art.py runs it when MAIYA_ART_REFERENCE
names a revision.
"""

import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

import numpy as np

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
GENERATED = GAME / "artbox/generated"
NM = "m68k-unknown-elf-nm"
FILES = ("780-c1.c1", "780-c2.c2", "maiya_assets.h", "assets.json")
ARRAY = re.compile(r"static const (uint8_t|uint16_t) (mg_\w+)\[(\d+)\] = \{(.*?)\};", re.S)


def git(*args, binary=False):
    out = subprocess.run(["git", *args], cwd=ROOT, check=True, capture_output=True)
    return out.stdout if binary else out.stdout.decode().strip()


def nm_tool():
    for root in ("x-tools-v3", "x-tools-v2", "x-tools"):
        tool = ROOT.parent / root / "m68k-unknown-elf/bin" / NM
        if tool.is_file():
            return str(tool)
    found = shutil.which(NM)
    if not found:
        raise SystemExit(f"{NM} not found (SDK toolchain)")
    return found


def tiles(c1, c2):
    a = np.frombuffer(c1, np.uint8).reshape(-1, 64)
    b = np.frombuffer(c2, np.uint8).reshape(-1, 64)
    return np.concatenate([a, b], 1)


def arrays(header):
    """{name: (ctype, big-endian bytes)} for every palette/map array."""
    out = {}
    for ctype, name, count, body in ARRAY.findall(header):
        values = [int(v) for v in re.findall(r"\d+", body)]
        assert len(values) == int(count), name
        out[name] = (ctype, np.array(values, ">u2" if ctype == "uint16_t" else "u1").tobytes())
    return out


def defines(header):
    return [line for line in header.splitlines() if line.startswith("#define")]


def reference_arrays(rev):
    """The reference build's arrays, by symbol, from its ELF and ROM image."""
    with tempfile.TemporaryDirectory() as tmp:
        elf = Path(tmp) / "game"
        elf.write_bytes(git("show", f"{rev}:out/game", binary=True))
        listing = subprocess.run([nm_tool(), "-S", str(elf)], check=True, capture_output=True, text=True).stdout
    image = git("show", f"{rev}:out/game0.rom", binary=True)
    p1 = git("show", f"{rev}:out/780-p1.p1", binary=True)
    swapped = bytearray(len(image) & ~1)
    swapped[0::2], swapped[1::2] = image[1:len(swapped):2], image[0:len(swapped):2]
    if p1[:len(swapped)] != bytes(swapped):
        raise SystemExit(f"{rev}: out/game0.rom is not the image of out/780-p1.p1")
    found = {}
    for line in listing.splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[3].startswith("mg_"):
            address, size = int(parts[0], 16), int(parts[1], 16)
            found[parts[3]] = image[address:address + size]
    return found


def baseline(rev, python):
    """This machine's build of the reference sources (cached by commit)."""
    commit = git("rev-parse", f"{rev}^{{commit}}")
    cache = GENERATED / f"pin-baseline-{commit[:12]}"
    if all((cache / name).is_file() for name in FILES):
        return cache
    with tempfile.TemporaryDirectory() as tree:
        # what the builder reads: its own sources, the shared artbox modules,
        # the system font and the demo's character sheets
        archive = subprocess.Popen(["git", "archive", commit, "games/maiya", "games/demo/artbox/in",
                                    "artbox", "bios/tools"], cwd=ROOT, stdout=subprocess.PIPE)
        subprocess.run(["tar", "-x", "-C", tree], stdin=archive.stdout, check=True)
        if archive.wait():
            raise SystemExit(f"pin: git archive {commit} failed")
        (Path(tree) / "out").mkdir()
        print(f"pin: building the reference sources ({commit[:12]}) on this machine ...", flush=True)
        subprocess.run([python, str(Path(tree) / "games/maiya/tools/build_commercial_assets.py")],
                       check=True, stdout=subprocess.DEVNULL)
        cache.mkdir(parents=True, exist_ok=True)
        for name in FILES:
            shutil.copyfile(Path(tree) / "games/maiya/artbox/generated" / name, cache / name)
    return cache


def pin(rev, python=sys.executable, check_only=False):
    base = baseline(rev, python)
    ref = tiles(git("show", f"{rev}:games/maiya/artbox/780-c1.c1", binary=True),
                git("show", f"{rev}:games/maiya/artbox/780-c2.c2", binary=True))
    was = tiles((base / FILES[0]).read_bytes(), (base / FILES[1]).read_bytes())
    now = tiles((GENERATED / FILES[0]).read_bytes(), (GENERATED / FILES[1]).read_bytes())
    layout = [(a["name"], a["tile_base"], a["tile_last"]) for a in json.loads((GENERATED / FILES[3]).read_text())]
    if layout != [(a["name"], a["tile_base"], a["tile_last"]) for a in json.loads((base / FILES[3]).read_text())]:
        raise SystemExit("pin: the tile layout moved since the reference; rebuild the reference art instead")
    header_now, header_was = (GENERATED / FILES[2]).read_text(), (base / FILES[2]).read_text()
    if defines(header_now) != defines(header_was):
        raise SystemExit("pin: a #define changed since the reference; rebuild the reference art instead")

    owner = np.full(len(now), "", object)
    for name, first, last in layout:
        owner[first:last + 1] = name
    edited = (now != was).any(1)
    final = ref.copy()
    final[edited] = now[edited]
    machine = sorted(set(owner[(was != ref).any(1)]) - {""})

    a_now, a_was, a_ref = arrays(header_now), arrays(header_was), reference_arrays(rev)
    if a_now.keys() != a_was.keys():
        raise SystemExit("pin: the set of palette/map arrays changed since the reference")
    kept, pinned = [], []
    for name, (ctype, data) in a_now.items():
        if data != a_was[name][1]:
            if name not in a_ref:
                raise SystemExit(f"pin: {name} changed, but the reference build has no copy to compare")
            kept.append(name)
        elif name in a_ref and a_ref[name] != data:
            if len(a_ref[name]) != len(data):
                raise SystemExit(f"pin: {name} has a different size in the reference")
            pinned.append(name)

    report = dict(reference=git("rev-parse", f"{rev}^{{commit}}"),
                  rebuilt_assets=sorted(set(owner[edited]) - {""}), rebuilt_tiles=int(edited.sum()),
                  rebuilt_arrays=kept, machine_differences=dict(assets=machine, arrays=pinned))
    print(f"pin: {len(report['rebuilt_assets'])} assets rebuilt by the edits "
          f"({report['rebuilt_tiles']} tiles, {len(kept)} arrays); "
          f"{len(machine)} assets and {len(pinned)} arrays kept as the reference drew them")
    for name in report["rebuilt_assets"]:
        print(f"  rebuilt: {name}")
    if check_only:
        return report

    def write_array(match):
        ctype, name = match.group(1), match.group(2)
        if name not in pinned:
            return match.group(0)
        values = np.frombuffer(a_ref[name], ">u2" if ctype == "uint16_t" else "u1").tolist()
        lines = [f"static const {ctype} {name}[{len(values)}] = {{"]
        for i in range(0, len(values), 16):
            lines.append("    " + ", ".join(str(int(v)) for v in values[i:i + 16]) + ",")
        return "\n".join(lines + ["};"])

    (GENERATED / FILES[2]).write_text(ARRAY.sub(write_array, header_now), encoding="ascii")
    (GENERATED / FILES[0]).write_bytes(final[:, :64].tobytes())
    (GENERATED / FILES[1]).write_bytes(final[:, 64:].tobytes())
    (GENERATED / "pin-report.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--reference", required=True, help="git revision whose art is kept (e.g. v1.7.3)")
    ap.add_argument("--python", default=sys.executable, help="interpreter for the reference build")
    ap.add_argument("--check-only", action="store_true", help="report, write nothing")
    args = ap.parse_args()
    pin(args.reference, args.python, args.check_only)


if __name__ == "__main__":
    main()
