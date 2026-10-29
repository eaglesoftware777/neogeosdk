#!/usr/bin/env python3
"""Shot and cloud probe: an automated test for projectiles that hang on aircraft
and for weather that follows the plane, run on the Linux MAME in WSL.

The pilot flies to just under the nearest enemy (or the boss), where aimed
fire is point blank, firing and weaving, for --seconds of emulated time with
invulnerability held. Every frame the probe walks the character table and
records, as facts:

  stuck_shots        enemy shots (not seekers) that kept one screen position
                     for 30 frames or more
  still_frames       frames with a non-seeker enemy shot at velocity (0,0)
  stale_bolts        shots the game itself removed for hanging (new builds)
  cloud_frames, cloud_xs
                     frames with the overhead cloud up, and how many different
                     x positions it took while the camera panned (1 = it kept
                     a screen x, i.e. it rode along with the plane)
  slots              the hardware sprite range each kind actually used
  line_peak          the busiest scanline the game counted (new builds)

    python3 games/skylance/tools/shot_probe.py --roms dist/neo/skylance/mvs --elf out/game --out DIR [--boss]

--roms is a MAME set folder (roms/skylance/*, hash/neogeo.xml) and --elf the
68000 ELF of the same build, for its symbols. Exit status 1 when a shot hung.
"""

import argparse
import json
import os
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
CC = Path("/mnt/c/neogeo/x-tools-v3/m68k-unknown-elf/bin/m68k-unknown-elf-gcc")

LUA = r"""
local cfg = __CONFIG__
local machine = manager.machine
local memory = machine.devices[':maincpu'].spaces['program']
local screen = machine.screens[':screen']
local L, S = cfg.layout, cfg.symbols
local joystick = machine.ioport.ports[':edge:joy:JOY1'] and ':edge:joy:JOY1' or ':ctrl1:joy:JOY'
local start_port = machine.ioport.ports[':edge:joy:START'] and ':edge:joy:START' or ':ctrl1:joy:START_SELECT'
local start_name = start_port == ':edge:joy:START' and '1 Player Start' or 'P1 Start'
-- MAME 0.289 lists the buttons twice, as "P1 A" and "%p A": press both.
local function input(port, name, v)
  local p = machine.ioport.ports[port]
  if not p then return end
  for _, n in ipairs({name, (name:gsub('^P1 ([ABCD])$', '%%p %1'))}) do if p.fields[n] then p.fields[n]:set_value(v) end end
end
local function s16(v) return v >= 32768 and v - 65536 or v end
local frames, shot = 0, 0
local track, stuck_shots, still_frames, max_eshots = {}, 0, 0, 0
local cloud_frames, cloud_xs, ncloud = 0, {}, 0
local slots, line_peak, play_frames = {}, 0, 0
local function slot(kind, first)
  local s = slots[kind] or {lo = 9999, hi = -1}; slots[kind] = s
  if first ~= 0xffff and first ~= 0 then if first < s.lo then s.lo = first end; if first > s.hi then s.hi = first end end
end
local function finish(now)
  local f = assert(io.open(cfg.out .. '/probe.json', 'w'))
  local parts = {}
  for k, s in pairs(slots) do parts[#parts + 1] = string.format('"%d":[%d,%d]', k, s.lo, s.hi) end
  f:write(string.format('{"seconds":%.1f,"frames":%d,"play_frames":%d,"stuck_shots":%d,"still_frames":%d,"max_eshots":%d,' ..
    '"stale_bolts":%s,"cloud_frames":%d,"cloud_xs":%d,"line_peak":%s,"slots":{%s}}\n',
    now, frames, play_frames, stuck_shots, still_frames, max_eshots,
    S.s_stale_bolts and tostring(memory:read_u16(S.s_stale_bolts)) or 'null', cloud_frames, ncloud,
    S.s_line_peak and tostring(line_peak) or 'null', table.concat(parts, ',')))
  f:close()
  machine:exit()
end
emu.register_frame_done(function()
  frames = frames + 1
  local now = machine.time:as_double()
  if now > cfg.seconds then finish(now); return end
  local player = memory:read_u32(S.s_player)
  -- Until a sortie starts: a coin every 4 s (one during the BIOS screen is
  -- lost), Start and A in between to pass the pilot select.
  local wait = player == 0 and now > 6
  input(':AUDIO_COIN', 'Coin 1', (wait and (now - 6) % 4 < 0.2) and 1 or 0)
  input(start_port, start_name, (wait and (now - 6) % 4 > 1 and (now - 6) % 4 < 1.15) and 1 or 0)
  if player == 0 then input(joystick, 'P1 A', (wait and (now - 6) % 4 > 2.5 and (now - 6) % 4 < 2.65) and 1 or 0); return end
  play_frames = play_frames + 1
  memory:write_u8(S.s_invuln, 120)
  if cfg.boss then
    memory:write_u8(S.s_waves_left, 0); memory:write_u8(S.s_squad_left, 0); memory:write_u16(S.s_wave_timer, 0)
  end
  local px, py = s16(memory:read_u16(player + L[5])), s16(memory:read_u16(player + L[6]))
  local tx, ty, best, eshots, ex, ey = px, 176, 99999, 0, nil, nil
  local live = {}
  for i = 0, L[2] - 1 do
    local c = S.ng_chars + i * L[1]
    if memory:read_u8(c + L[3]) ~= 0 then
      local kind = memory:read_u8(c + L[4])
      local x, y = s16(memory:read_u16(c + L[5])), s16(memory:read_u16(c + L[6]))
      slot(kind, memory:read_u16(c + L[10]))
      if kind == L[12] or kind == L[16] then
        local d = math.abs(x - px) + math.abs(y - py)
        if y > 24 and y < 150 and d < best then best, tx, ty, ex, ey = d, x, y + 34, x, y end
      elseif kind == L[13] and memory:read_u16(c + L[9]) ~= cfg.seeker then
        eshots = eshots + 1
        local vx, vy = memory:read_u32(c + L[7]), memory:read_u32(c + L[8])
        if vx == 0 and vy == 0 then still_frames = still_frames + 1 end
        local t = track[i]
        if t and t.x == x and t.y == y then
          t.n = t.n + 1
          if t.n == 30 then stuck_shots = stuck_shots + 1 end
        else track[i] = {x = x, y = y, n = 0} end
        live[i] = true
      elseif kind == L[15] then
        cloud_frames = cloud_frames + 1
        if not cloud_xs[x] then cloud_xs[x] = true; ncloud = ncloud + 1 end
      end
    end
  end
  for i in pairs(track) do if not live[i] then track[i] = nil end end
  if eshots > max_eshots then max_eshots = eshots end
  if S.s_line_peak then local p = memory:read_u16(S.s_line_peak); if p > line_peak then line_peak = p end end
  if cfg.point_blank and ex then
    -- Held 10 px under the nearest enemy, inside the 16 px where whole-pixel
    -- aiming gave (0,0); not firing, so the target lives to shoot.
    memory:write_u16(player + L[5], ex & 0xffff); memory:write_u16(player + L[6], (ey + 10) & 0xffff)
    memory:write_u32(player + L[17], (ex * 256) & 0xffffffff); memory:write_u32(player + L[18], ((ey + 10) * 256) & 0xffffffff)
    return
  end
  local weave = (math.floor(now / 3) % 2 == 0) and -20 or 20
  if cfg.boss then tx = tx + weave end
  input(joystick, 'P1 Left', px > tx + 3 and 1 or 0)
  input(joystick, 'P1 Right', px < tx - 3 and 1 or 0)
  input(joystick, 'P1 Up', py > ty + 3 and 1 or 0)
  input(joystick, 'P1 Down', py < ty - 3 and 1 or 0)
  input(joystick, 'P1 A', (math.floor(now * 4) % 2 == 0) and 1 or 0)
  if now >= shot then screen:snapshot(cfg.out .. string.format('/t%03d.png', math.floor(now))); shot = now + 10 end
end)
"""


def symbols(elf):
    out = subprocess.run([str(CC).replace("gcc", "nm"), str(elf)], capture_output=True, text=True, check=True).stdout
    return {p[2]: int(p[0], 16) for p in (line.split() for line in out.splitlines()) if len(p) == 3}


def layout():
    with tempfile.TemporaryDirectory(dir="/dev/shm") as d:
        obj, raw = Path(d) / "l.o", Path(d) / "l.bin"
        subprocess.run([str(CC), "-c", "-O2", "-ffreestanding", "-I", str(ROOT), "-I", str(ROOT / "sdk"),
                        str(GAME / "tools" / "shot_probe_layout.c"), "-o", str(obj)], check=True)
        subprocess.run([str(CC).replace("gcc", "objcopy"), "-O", "binary", "-j", ".rodata", str(obj), str(raw)], check=True)
        data = raw.read_bytes()
    return list(struct.unpack(f">{len(data) // 2}H", data))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--roms", type=Path, required=True)
    ap.add_argument("--elf", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--seconds", type=int, default=75)
    ap.add_argument("--boss", action="store_true")
    ap.add_argument("--point-blank", action="store_true", help="hold the plane 10 px under the nearest enemy, not firing")
    ap.add_argument("--mode", choices=("mvs", "aes"), default="mvs")
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    sym = symbols(args.elf)
    names = ("ng_chars", "s_player", "s_invuln", "s_waves_left", "s_squad_left", "s_wave_timer", "s_stale_bolts", "s_line_peak")
    seeker = int(re.search(r"#define SKY_SEEKER_ART\s+(\d+)", (GAME / "scenes" / "sky_stage.c").read_text()).group(1))
    cfg = ("{out=" + json.dumps(str(args.out)) + f",seconds={args.seconds},seeker={seeker},boss={'true' if args.boss else 'false'},point_blank={'true' if args.point_blank else 'false'},"
           "layout={" + ",".join(map(str, layout())) + "},symbols={" +
           ",".join(f"{n}={sym[n]}" for n in names if n in sym) + "}}")
    lua = args.out / "probe.lua"
    lua.write_text(LUA.replace("__CONFIG__", cfg))
    hold = Path("/mnt/c/mame/neogeosdk/roms")
    cmd = ["mame", "aes" if args.mode == "aes" else "neogeo", "-noreadconfig", "-skip_gameinfo",
           "-rompath", f"{args.roms / 'roms'};{hold}", "-hashpath", str(args.roms / "hash"),
           "-bios", "asia" if args.mode == "aes" else "euro", "-cart1", "skylance",
           "-cfg_directory", str(args.out / "cfg"), "-nvram_directory", str(args.out / "nvram"), "-noautosave",
           "-video", "none", "-sound", "none", "-nothrottle", "-seconds_to_run", str(args.seconds + 20),
           "-autoboot_script", str(lua)]
    env = dict(os.environ, DISPLAY=":0")    # WSLg; an inherited DISPLAY can stall MAME for minutes
    with open(args.out / "mame.log", "w") as mlog:
        p = subprocess.run(cmd, env=env, stdin=subprocess.DEVNULL, stdout=mlog, stderr=subprocess.STDOUT, timeout=900, cwd=str(Path.home()))
    res = json.loads((args.out / "probe.json").read_text())
    res["mame_exit"] = p.returncode
    (args.out / "probe.json").write_text(json.dumps(res, indent=1) + "\n")
    print(json.dumps(res))
    return 1 if res["stuck_shots"] or p.returncode else 0


if __name__ == "__main__":
    sys.exit(main())
