#!/usr/bin/env python3
"""
Drive a built game in MAME from a timeline and keep pictures and RAM traces.

    python3 tools/genre_qa.py --game sevenwonders --seconds 40 \\
        --step 3:coin --step 5:start --step 9:press:A --step 12:hold:LEFT --step 13:release:LEFT \\
        --step 15:snap:board --watch s_p:16 --out /tmp/sw-qa

Timeline steps are TIME:ACTION, TIME in emulated seconds:
    coin | start | press:BUTTON (a tenth of a second) | hold:BUTTON | release:BUTTON | snap:NAME
BUTTON is A B C D UP DOWN LEFT RIGHT START. Snapshots are also taken every --every seconds.
--watch SYMBOL[+OFFSET][:BYTES] logs that many bytes at the symbol (from out/game, the last
build), OFFSET bytes in, once a frame into watch.csv, so a test can check what the game did.
--lua FILE adds a script of one's own (a probe of video memory, say) after the runner's.

The machine boots EagleBIOS from bios/test_roms (MVS: neogeo -bios euro; AES: aes -bios asia)
in its own cfg/nvram folders under --out, never the user's MAME set-up. It always writes
ports.txt (the input fields the driver offers) and mame.log.
"""

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

LUA = r"""
local cfg = __CONFIG__
local machine = manager.machine
local screen = machine.screens[':screen']
local mem = machine.devices[':maincpu'].spaces['program']
local ports = assert(io.open(cfg.out .. '/ports.txt', 'w'))
for tag, port in pairs(machine.ioport.ports) do
    for name, _ in pairs(port.fields) do ports:write(tag .. '\t' .. name .. '\n') end
end
ports:close()
local function field(port, name)
    local p = machine.ioport.ports[port]
    if p and p.fields[name] then return p.fields[name] end
    return nil
end
local aes = cfg.platform == 'aes'
local MAP = {}
if aes then
    MAP = { A = {':ctrl1:joy:JOY', 'P1 A'}, B = {':ctrl1:joy:JOY', 'P1 B'},
            C = {':ctrl1:joy:JOY', 'P1 C'}, D = {':ctrl1:joy:JOY', 'P1 D'},
            UP = {':ctrl1:joy:JOY', 'P1 Up'}, DOWN = {':ctrl1:joy:JOY', 'P1 Down'},
            LEFT = {':ctrl1:joy:JOY', 'P1 Left'}, RIGHT = {':ctrl1:joy:JOY', 'P1 Right'},
            START = {':ctrl1:joy:START_SELECT', 'P1 Start'} }
else
    MAP = { A = {':edge:joy:JOY1', 'P1 A'}, B = {':edge:joy:JOY1', 'P1 B'},
            C = {':edge:joy:JOY1', 'P1 C'}, D = {':edge:joy:JOY1', 'P1 D'},
            UP = {':edge:joy:JOY1', 'P1 Up'}, DOWN = {':edge:joy:JOY1', 'P1 Down'},
            LEFT = {':edge:joy:JOY1', 'P1 Left'}, RIGHT = {':edge:joy:JOY1', 'P1 Right'},
            START = {':edge:joy:START', '1 Player Start'}, COIN = {':AUDIO_COIN', 'Coin 1'} }
end
local function set(name, v)
    local m = MAP[name]
    if not m then return end
    local f = field(m[1], m[2])
    if f then f:set_value(v) end
end
local steps = cfg.steps
local done = {}
local releases = {}
local watch = nil
if #cfg.watch > 0 then
    watch = assert(io.open(cfg.out .. '/watch.csv', 'w'))
    local head = {'seconds', 'frame'}
    for _, w in ipairs(cfg.watch) do head[#head + 1] = w.name end
    watch:write(table.concat(head, ',') .. '\n')
end
local frame, next_snap = 0, cfg.every
emu.register_frame_done(function()
    frame = frame + 1
    local now = machine.time:as_double()
    for i, s in ipairs(steps) do
        if not done[i] and now >= s.t then
            done[i] = true
            if s.a == 'coin' then set('COIN', 1); releases['COIN'] = now + 0.15
            elseif s.a == 'start' then set('START', 1); releases['START'] = now + 0.15
            elseif s.a == 'press' then set(s.b, 1); releases[s.b] = now + 0.1
            elseif s.a == 'hold' then set(s.b, 1); releases[s.b] = nil
            elseif s.a == 'release' then set(s.b, 0); releases[s.b] = nil
            elseif s.a == 'snap' then screen:snapshot(string.format('%s/%s.png', cfg.out, s.b)) end
        end
    end
    for name, t in pairs(releases) do
        if now >= t then set(name, 0); releases[name] = nil end
    end
    if cfg.every > 0 and now >= next_snap then
        screen:snapshot(string.format('%s/t%06.1f.png', cfg.out, now))
        next_snap = now + cfg.every
    end
    if watch then
        local row = {string.format('%.3f', now), tostring(frame)}
        for _, w in ipairs(cfg.watch) do
            local hex = {}
            for k = 0, w.size - 1 do hex[#hex + 1] = string.format('%02x', mem:read_u8(w.addr + k)) end
            row[#row + 1] = table.concat(hex)
        end
        watch:write(table.concat(row, ',') .. '\n')
    end
end)
"""


def symbols():
    nm = None
    sdkhome = Path(os.environ.get("SDKHOME", ROOT.parent))
    for d in ("x-tools-v3", "x-tools-v2", "x-tools"):
        p = sdkhome / d / "m68k-unknown-elf/bin/m68k-unknown-elf-nm"
        if p.is_file():
            nm = str(p)
            break
    out = subprocess.run([nm or "m68k-unknown-elf-nm", str(ROOT / "out/game")], capture_output=True, text=True, check=True).stdout
    table = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3:
            table[parts[2]] = int(parts[0], 16)
    return table


def lua_value(v):
    if isinstance(v, bool):
        return "true" if v else "false"
    if isinstance(v, (int, float)):
        return repr(v)
    if isinstance(v, str):
        return json.dumps(v)
    if isinstance(v, list):
        return "{" + ",".join(lua_value(x) for x in v) + "}"
    if isinstance(v, dict):
        return "{" + ",".join(f"{k}={lua_value(x)}" for k, x in v.items()) + "}"
    raise TypeError(v)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", required=True)
    ap.add_argument("--platform", choices=("mvs", "aes"), default="mvs")
    ap.add_argument("--bios")
    ap.add_argument("--seconds", type=float, default=30)
    ap.add_argument("--every", type=float, default=0)
    ap.add_argument("--step", action="append", default=[])
    ap.add_argument("--watch", action="append", default=[])
    ap.add_argument("--out", required=True)
    ap.add_argument("--mame", default="mame")
    ap.add_argument("--rompath", help="system ROM folders first (default: EagleBIOS's bios/test_roms)")
    ap.add_argument("--lua", help="more Lua run after the timeline's (sees machine, mem, screen, cfg; cfg.out is --out)")
    args = ap.parse_args(argv)
    out = Path(args.out).resolve()
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)
    steps = []
    for s in args.step:
        t, rest = s.split(":", 1)
        a, _, b = rest.partition(":")
        steps.append({"t": float(t), "a": a, "b": b.upper() if a != "snap" else b})
    steps.sort(key=lambda s: s["t"])
    watch = []
    if args.watch:
        syms = symbols()
        for w in args.watch:
            label, _, size = w.partition(":")
            name, _, offset = label.partition("+")
            if name not in syms:
                raise SystemExit(f"genre_qa: no symbol {name} in out/game")
            watch.append({"name": label, "addr": syms[name] + int(offset or "0", 0), "size": int(size or 1)})
    cfg = {"out": str(out), "platform": args.platform, "steps": steps, "every": args.every, "watch": watch}
    script = out / "qa.lua"
    extra = Path(args.lua).read_text(encoding="utf-8") if args.lua else ""
    script.write_text(LUA.replace("__CONFIG__", lua_value(cfg)) + "\n" + extra, encoding="utf-8")
    system = "aes" if args.platform == "aes" else "neogeo"
    bios = args.bios or ("asia" if args.platform == "aes" else "euro")
    rompath = args.rompath or f"{ROOT / 'bios/test_roms'};{ROOT / 'roms'}"
    hashpath = f"{ROOT / 'hash_eagle' / args.game};{ROOT / 'hash_eagle'}"
    cmd = [args.mame, system, "-noreadconfig", "-rompath", rompath, "-hashpath", hashpath, "-bios", bios,
           "-cart1", args.game, "-video", "none", "-sound", "none", "-nothrottle", "-seconds_to_run",
           str(args.seconds), "-skip_gameinfo", "-cfg_directory", str(out / "cfg"), "-nvram_directory",
           str(out / "nvram"), "-snapshot_directory", str(out), "-autoboot_delay", "0", "-autoboot_script", str(script)]
    with (out / "mame.log").open("w", encoding="utf-8") as log:
        r = subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT)
    print(f"genre_qa: {args.game} {args.platform} {args.seconds:.0f}s -> {out} (mame exit {r.returncode})")
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
