#!/usr/bin/env python3
"""
Check the genre engines' 68000 code (sdk/2d_engine/ng_<genre>.c).

    python3 tools/check_genre_codegen.py            every genre engine present
    python3 tools/check_genre_codegen.py puzzle rpg some of them

Each engine is compiled with the SDK's m68k GCC at -O0 and -O2 (the levels a
game builds with) and its disassembly is checked for:

  * a source post-increment or pre-decrement and an indexed destination on
    the same address register (move.w (a0)+,(0,a0,d0.l)): the 68000 works the
    destination out after the increment, so such a copy lands one element
    along. GCC -O2 makes it from plain C copy loops;
  * calls to the library's 32-bit divide and modulo (__divsi3, __udivsi3,
    __modsi3, __umodsi3), several hundred cycles each;
  * code outside the neogeo_user section, or any .data/.bss: the game's
    linker script only places neogeo_user code, and an engine keeps no
    hidden state of its own (contexts belong to the caller).

It prints the code size of each engine at both levels (the ROM cost), the
number of 32-bit multiply calls and the largest stack frames at -O2 (a frame
over STACK_LIMIT bytes is a finding: the 68000 stack shares the top of work
RAM with the game), and exits non-zero on any finding.
"""

import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GENRES = ("puzzle", "mahjong", "rpg", "beatemup")
FLAGS = ["-c", "-fomit-frame-pointer", "-Wall", "-Wextra", "-fno-zero-initialized-in-bss",
         "-march=68000", "-mcpu=68000", "-mtune=68000", "-m68000", "-ffreestanding", "-std=gnu99",
         "-I" + ROOT, "-I" + os.path.join(ROOT, "sdk"), "-I" + os.path.join(ROOT, "sdk", "2d_engine")]
STACK_LIMIT = 768
POSTINC = re.compile(r"%(a[0-7])@[+-],%\1@\(")
LIBCALL = re.compile(r"\b(__(?:u?div|u?mod|mul)si3)\b")


def toolchain():
    sdkhome = os.environ.get("SDKHOME", os.path.dirname(ROOT))
    for d in ("x-tools-v3", "x-tools-v2", "x-tools"):
        b = os.path.join(sdkhome, d, "m68k-unknown-elf", "bin")
        if os.path.isfile(os.path.join(b, "m68k-unknown-elf-gcc")):
            return os.path.join(b, "m68k-unknown-elf-")
    return "m68k-unknown-elf-"


def sections(prefix, obj):
    out = subprocess.run([prefix + "objdump", "-h", obj], capture_output=True, text=True, check=True).stdout
    sizes = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[0].isdigit():
            sizes[parts[1]] = int(parts[2], 16)
    return sizes


def check(genre, prefix, tmp):
    src = os.path.join(ROOT, "sdk", "2d_engine", f"ng_{genre}.c")
    problems, report = [], []
    for level in ("-O0", "-O2"):
        obj = os.path.join(tmp, f"ng_{genre}{level}.o")
        extra = ["-fstack-usage"] if level == "-O2" else []
        r = subprocess.run([prefix + "gcc"] + FLAGS + extra + [level, src, "-o", obj], capture_output=True, text=True,
                           cwd=tmp)
        if r.returncode:
            problems.append(f"{level}: does not compile:\n{r.stderr}")
            continue
        warn = [l for l in r.stderr.splitlines() if "warning:" in l]
        problems += [f"{level}: {w}" for w in warn]
        dis = subprocess.run([prefix + "objdump", "-dr", obj], capture_output=True, text=True, check=True).stdout
        fn = "?"
        calls = {}
        for line in dis.splitlines():
            m = re.match(r"^[0-9a-f]+ <([^>]+)>:", line)
            if m:
                fn = m.group(1)
                continue
            if POSTINC.search(line):
                problems.append(f"{level}: {fn}: increment and indexed store on one register: {line.strip()}")
            for call in LIBCALL.findall(line):
                calls[call] = calls.get(call, 0) + 1
                if "mul" not in call:
                    problems.append(f"{level}: {fn}: library divide {call}")
        sizes = sections(prefix, obj)
        for name, size in sizes.items():
            if size and (name in (".data", ".bss", ".text") or name.startswith((".data.", ".bss.", ".text."))):
                problems.append(f"{level}: {size} bytes in {name} (code must be NEOGEO_USER, no globals)")
        code = sizes.get("neogeo_user", 0)
        ro = sum(v for k, v in sizes.items() if k.startswith(".rodata"))
        muls = sum(v for k, v in calls.items() if "mul" in k)
        line = f"{level} code {code} bytes, rodata {ro}, 32-bit multiply calls {muls}"
        su = os.path.splitext(obj)[0] + ".su"
        if os.path.isfile(su):
            frames = []
            with open(su, encoding="utf-8") as fh:
                for row in fh:
                    parts = row.rstrip("\n").split("\t")
                    if len(parts) >= 2 and parts[1].isdigit():
                        frames.append((int(parts[1]), parts[0].split(":")[-1]))
            frames.sort(reverse=True)
            for size, name in frames:
                if size > STACK_LIMIT:
                    problems.append(f"{level}: {name}: {size}-byte stack frame (limit {STACK_LIMIT})")
            line += ", largest frames " + ", ".join(f"{n} {sz}" for sz, n in frames[:3])
        report.append(line)
    return problems, report


def main(argv):
    want = argv or [g for g in GENRES if os.path.isfile(os.path.join(ROOT, "sdk", "2d_engine", f"ng_{g}.c"))]
    prefix = toolchain()
    failed = 0
    with tempfile.TemporaryDirectory(prefix="ng-genre-") as tmp:
        for genre in want:
            problems, report = check(genre, prefix, tmp)
            print(f"ng_{genre}: " + "; ".join(report))
            for p in problems:
                print(f"  ng_{genre}: {p}", file=sys.stderr)
            failed += len(problems)
    if failed:
        print(f"check_genre_codegen: {failed} finding(s)", file=sys.stderr)
        return 1
    print("check_genre_codegen: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
