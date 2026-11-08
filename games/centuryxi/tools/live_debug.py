#!/usr/bin/env python3
"""Century XI's play-test logger, for one build.

Writes, under a MAME launcher folder (C:\\mame\\neogeosdk), the live session's watch
for Century XI (live\\watch\\centuryxi_<mode>.lua: tools/live_debug.lua with this
build's addresses) and the launcher that plays with it (live\\live_centuryxi_<mode>.bat,
through bin\\ngsdk.bat live). Run it after building the platform that gets installed,
with that build's ELF:

    python3 games/centuryxi/tools/live_debug.py MVS_OR_AES DEST [--elf out/game] [--out FILE]

--out writes only the Lua, anywhere (the automated runs: tools/genre_qa.py --lua FILE).
"""
import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
GAME = HERE.parent
ROOT = GAME.parents[1]
SCENES = GAME / "scenes"

WORLD = ["ev", "frame", "clock", "fpm", "half", "phase", "score", "team", "ctrl", "human", "pass_to", "scorer",
         "goal_kind", "card_man", "card_red", "recv_dir", "ball"]
BALL = ["x", "y", "h", "vx", "vy", "vh", "owner", "last"]
MAN = ["x", "y", "vx", "vy", "state", "timer", "dir", "kick", "squad"]


def tool(name):
    for cand in (os.environ.get("M68K_PREFIX", "") + name, name,
                 str(Path.home() / "x-tools-v3" / "m68k-unknown-elf" / "bin" / name)):
        if cand and shutil.which(cand):
            return shutil.which(cand)
    raise SystemExit(f"live_debug: {name} not found (set M68K_PREFIX)")


def symbols(elf):
    out = subprocess.run([tool("m68k-unknown-elf-nm"), str(elf)], capture_output=True, text=True, check=True).stdout
    sym = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3:
            sym[parts[2]] = int(parts[0], 16)
    want = {"world": "s_w", "camx": "cx_camx", "camy": "cx_camy", "roster_name": "cx_roster_name", "nation_code": "cx_nation_code",
            "catch_run": "s_catch_run"}
    missing = [v for v in want.values() if v not in sym]
    if missing:
        raise SystemExit(f"live_debug: {elf} has no {', '.join(missing)}: is it Century XI's?")
    return {k: sym[v] for k, v in want.items()}


def offsets():
    """The fields' offsets as the 68000 build lays the structures out."""
    lines = ["#include <stddef.h>", '#include "cx_sim.h"', "void f(void) {"]
    for f in WORLD:
        lines.append(f'__asm__ volatile("#OFF {f} %0" :: "i"(offsetof(CxWorld, {f})));')
    for f in BALL:
        lines.append(f'__asm__ volatile("#OFF b_{f} %0" :: "i"(offsetof(CxBall, {f})));')
    for f in MAN:
        lines.append(f'__asm__ volatile("#OFF m_{f} %0" :: "i"(offsetof(CxMan, {f})));')
    lines.append('__asm__ volatile("#OFF mansize %0" :: "i"(sizeof(CxMan)));')
    lines.append("}")
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / "off.c"
        src.write_text("\n".join(lines) + "\n")
        asm = subprocess.run([tool("m68k-unknown-elf-gcc"), "-m68000", "-S", f"-I{SCENES}",
                              f"-I{ROOT / 'sdk'}", f"-I{ROOT / 'sdk' / '2d_engine'}", "-DCX_CODE=", str(src), "-o", "-"],
                             capture_output=True, text=True, check=True).stdout
    off = {}
    for m in re.finditer(r"#OFF (\w+) #?(\d+)", asm):
        off[m.group(1)] = int(m.group(2))
    return off


def enum(text, prefix):
    for body in re.findall(r"enum\s*\{([^}]*)\}", text):
        names = [n.strip().split("=")[0].strip() for n in re.sub(r"/\*.*?\*/", "", body, flags=re.S).split(",")]
        names = [n for n in names if n]
        if names and names[0].startswith(prefix):
            return [n[len(prefix):].lower().replace("_", " ") for n in names]
    raise SystemExit(f"live_debug: no enum {prefix}* in cx_sim.h")


def lua_text(elf):
    sim = (SCENES / "cx_sim.h").read_text()
    render = (SCENES / "cx_render.h").read_text()
    sym, off = symbols(elf), offsets()
    ev = dict(re.findall(r"#define CX_EV_(\w+)\s+(0x[0-9A-Fa-f]+)u", sim))
    spr = dict(re.findall(r"#define SPR_(\w+)\s+(\d+)u", render))
    num = lambda name: int(re.search(rf"#define {name}\s+(\d+)", sim).group(1))
    lst = lambda xs: "{" + ", ".join(f'"{x}"' for x in xs) + "}"
    p1 = hashlib.sha256(Path(elf).read_bytes()).hexdigest()[:12]
    version = (GAME / "VERSION").read_text().strip()
    o = {k: v for k, v in off.items() if not k.startswith(("b_", "m_"))}
    states = enum(sim, "CX_M_")
    q = ["local C = {",
         f'    build = "{version} {p1}",',
         f"    world = 0x{sym['world']:06X}, camx = 0x{sym['camx']:06X}, camy = 0x{sym['camy']:06X},",
         f"    roster_name = 0x{sym['roster_name']:06X}, nation_code = 0x{sym['nation_code']:06X},",
         f"    catch_run = 0x{sym['catch_run']:06X},   -- 1: the frame just run drew nothing (a late frame's owed tick)",
         "    frames = 0x100020,          -- waitVbl() counts the game's frames here",
         f"    len = {num('CX_LEN')}, mid_y = {num('CX_MID_Y')}, hold_state = {states.index('hold')},",
         "    off = {" + ", ".join(f"{k} = {v}" for k, v in sorted(o.items())) + "},",
         "    ev = {" + ", ".join(f"{k} = {v}" for k, v in sorted(ev.items())) + "},",
         "    spr = {" + f"bg = {spr['BG']}, bg_end = {int(spr['SHADOW']) - 1}, men = {spr['MEN']}, men_end = {int(spr['BALL']) - 1}, "
         f"ball = {spr['BALL']}, goal = {spr['GOAL']}, cursor = {spr['CURSOR']}" + "},",
         f"    phases = {lst(enum(sim, 'CX_P_'))},",
         f"    states = {lst(states)},",
         f"    kicks = {lst(enum(sim, 'CX_K_'))},",
         f"    goal_kinds = {lst(enum(sim, 'CX_G_'))},",
         "}"]
    # the ball's and a man's fields share names (x, y...): a table each, read through ball() and man()
    ball = ", ".join(f"{f} = {off['b_' + f]}" for f in BALL)
    man = ", ".join(f"{f} = {off['m_' + f]}" for f in MAN)
    q.append(f"local BALL_OFF = {{{ball}}}")
    q.append(f"local MAN_OFF = {{{man}}}")
    lua = (HERE / "live_debug.lua").read_text()
    return lua.replace("--@@CONSTANTS@@", "\n".join(q))


def crlf(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(text.replace("\r\n", "\n").replace("\n", "\r\n").encode("ascii"))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mode", choices=("mvs", "aes"))
    ap.add_argument("dest", nargs="?", help="the MAME launcher folder (C:\\mame\\neogeosdk)")
    ap.add_argument("--elf", default=str(ROOT / "out" / "game"))
    ap.add_argument("--out", help="write only the Lua, here")
    a = ap.parse_args()
    text = lua_text(a.elf)
    if a.out:
        Path(a.out).write_text(text)
        print(f"live_debug: {a.out}")
        return
    if not a.dest:
        raise SystemExit("live_debug: give the launcher folder, or --out")
    dest = Path(a.dest)
    if not (dest / "bin" / "ngsdk.bat").is_file():
        raise SystemExit(f"live_debug: {dest} has no bin\\ngsdk.bat (tools/mame_bats.py writes it)")
    crlf(dest / "live" / "watch" / f"centuryxi_{a.mode}.lua", text)
    crlf(dest / "live" / f"live_centuryxi_{a.mode}.bat",
         "@echo off\n"
         "rem Century XI play test: plays the installed build while the live log and Century XI's\n"
         "rem own logger (live\\watch\\centuryxi_" + a.mode + ".lua) record the session. Press M at anything wrong.\n"
         f'call "%~dp0..\\bin\\ngsdk.bat" live centuryxi {a.mode} %*\n')
    crlf(dest / "live" / "README-CENTURYXI.txt", README)
    print(f"live_debug: {dest / 'live'} live_centuryxi_{a.mode}.bat and watch\\centuryxi_{a.mode}.lua")


README = r"""Century XI play tests
=====================

live_centuryxi_mvs.bat and live_centuryxi_aes.bat play the installed Century XI as
usual (your settings, saves and controls) and record the session for debugging.
Press M on the keyboard the moment something looks or plays wrong: a snapshot, a
save state and a full picture of the match at that moment are kept ("Mark N saved").

Each session is a folder in sessions\ (date, time, centuryxi, mode). Besides the
live log's files (live.log, stats.csv, snap\, state\, session.inp) Century XI's
own logger writes:

  cx_debug.log    the match as it happened: phases, goals (with the scorer), shots,
                  saves, fouls, cards, the keepers, every button you pressed (with who
                  you had and where the ball was), every pass you played that did not
                  reach its man (who, how far, how close the ball came to him, whether
                  you steered him away), sprites lost to the hardware's 96 a line,
                  men or pitch columns that vanished for a frame (flicker, ghosts),
                  and slow seconds with what was on screen
  cx_seconds.csv  one row a second: clock, phase, score, ball, who has it, who you
                  control, camera, frames drawn, the busiest sprite line, flicker
  cx_passes.csv   every pass of the match and what became of it
  cx_mark_N.txt   at each M: every man (state, place, speed), the ball, the camera,
                  and the 40 log lines before it
  cx_summary.txt  at the end: goals, shots, passes reaching their man for each side,
                  flicker, sprite overflow, slow seconds, frame rates
  snap\cx_*.png   a few snapshots of those moments

Send the whole session folder: with session.inp and start\ the match can be
replayed exactly on the same build.
"""

if __name__ == "__main__":
    main()
