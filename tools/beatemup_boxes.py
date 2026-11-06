#!/usr/bin/env python3
"""
See ng_beatemup's volumes in MAME: hurt boxes, strike boxes, depth.

    python3 tools/beatemup_boxes.py lua OUT.lua [--symbol s_b] [--every 0.5] [--elf out/game]
    python3 tools/genre_qa.py --game nosignal ... --lua OUT.lua --out DIR
    python3 tools/beatemup_boxes.py draw DIR

`lua` writes a script for tools/genre_qa.py (--lua) that finds the game's
NGBeatEmUp by its symbol in the ELF of the build being run and, every frame,
draws on MAME's screen:
  green   each fighter's hurt volume (half_w either side, height up from
          the feet; 16 high lying down), its foot line as wide as its depth;
  yellow  the strike box of a move winding up (where it will hit);
  red     the strike box while it can hit;
  white   a fighter held in hitstop.
MAME's snapshots don't keep drawings, so every --every seconds it also takes
DIR/box_<frame>.png and writes the boxes of that frame to DIR/boxes.csv;
`draw` paints them onto copies (DIR/box_<frame>_boxes.png).

The structure offsets come from the SDK's m68k GCC compiling ng_beatemup.h,
so the script follows the engine as it changes. Positions are the engine's:
screen x = (x - camera_x) / 256, the feet at (y - z) / 256 (No Signal draws
fighters there; --ground adds a game's own vertical offset).
"""
import argparse
import csv
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]

FIELDS = {
    "ctx_f": "offsetof(NGBeatEmUp, f)", "ctx_cam": "offsetof(NGBeatEmUp, camera_x)",
    "ctx_prop": "offsetof(NGBeatEmUp, prop)", "ctx_stage": "offsetof(NGBeatEmUp, stage)",
    "f_size": "sizeof(NGBFighter)", "f_x": "offsetof(NGBFighter, x)", "f_y": "offsetof(NGBFighter, y)",
    "f_z": "offsetof(NGBFighter, z)", "f_def": "offsetof(NGBFighter, def)", "f_alive": "offsetof(NGBFighter, alive)",
    "f_state": "offsetof(NGBFighter, state)", "f_facing": "offsetof(NGBFighter, facing)",
    "f_move": "offsetof(NGBFighter, move)", "f_move_frame": "offsetof(NGBFighter, move_frame)",
    "f_hitstop": "offsetof(NGBFighter, hitstop)", "f_weapon": "offsetof(NGBFighter, weapon)",
    "p_size": "sizeof(NGBProp)", "p_contents": "offsetof(NGBProp, contents)",
    "s_weapons": "offsetof(NGBStage, weapons)", "w_size": "sizeof(NGBWeaponDef)", "w_move": "offsetof(NGBWeaponDef, move)",
    "d_half_w": "offsetof(NGBFighterDef, half_w)", "d_height": "offsetof(NGBFighterDef, height)",
    "d_half_depth": "offsetof(NGBFighterDef, half_depth)", "d_moves": "offsetof(NGBFighterDef, moves)",
    "d_nmoves": "offsetof(NGBFighterDef, nmoves)",
    "m_size": "sizeof(NGBMove)", "m_startup": "offsetof(NGBMove, startup)", "m_active": "offsetof(NGBMove, active)",
    "m_box_x": "offsetof(NGBMove, box_x)", "m_box_w": "offsetof(NGBMove, box_w)", "m_box_z": "offsetof(NGBMove, box_z)",
    "m_box_h": "offsetof(NGBMove, box_h)", "m_box_depth": "offsetof(NGBMove, box_depth)",
    "fighters": "NGB_FIGHTERS", "st_down": "NGB_ST_DOWN", "st_attack": "NGB_ST_ATTACK",
}


def tool(name):
    for d in ("x-tools-v3", "x-tools-v2", "x-tools"):
        p = Path(os.environ.get("SDKHOME", ROOT.parent)) / d / "m68k-unknown-elf/bin" / f"m68k-unknown-elf-{name}"
        if p.is_file():
            return str(p)
    return f"m68k-unknown-elf-{name}"


def layout():
    """Field offsets and sizes as the 68000 build lays them out."""
    src = "#include <stddef.h>\n#include \"ng_beatemup.h\"\n" + "".join(
        f"const unsigned short k_{k} = (unsigned short)({v});\n" for k, v in FIELDS.items())
    with tempfile.TemporaryDirectory() as tmp:
        c = Path(tmp) / "layout.c"
        c.write_text(src)
        asm = subprocess.run([tool("gcc"), "-S", "-O0", "-m68000", "-I", str(ROOT / "sdk"), "-I", str(ROOT / "sdk/2d_engine"),
                              "-o", "-", str(c)], capture_output=True, text=True, check=True).stdout
    out, name = {}, None
    for line in asm.splitlines():
        m = re.match(r"^_?k_(\w+):", line.strip())
        if m:
            name = m.group(1)
            continue
        m = re.match(r"\s*\.(short|word)\s+(-?\d+)", line)
        if m and name:
            out[name] = int(m.group(2))
            name = None
        elif name and re.match(r"\s*\.(zero|skip)\s+2\b", line):
            out[name] = 0                     # a zero constant is laid out as two zero bytes
            name = None
    missing = set(FIELDS) - set(out)
    if missing:
        raise SystemExit(f"beatemup_boxes: no offsets for {sorted(missing)}")
    return out


def symbol(elf, name):
    for line in subprocess.run([tool("nm"), str(elf)], capture_output=True, text=True, check=True).stdout.splitlines():
        p = line.split()
        if len(p) == 3 and p[2] == name:
            return int(p[0], 16)
    raise SystemExit(f"beatemup_boxes: no symbol {name} in {elf}")


LUA = r"""
-- ng_beatemup volumes (tools/beatemup_boxes.py): drawn each frame, logged with a snapshot every EVERY s
do
local L = __LAYOUT__
local CTX, EVERY, GROUND = __CTX__, __EVERY__, __GROUND__
local bmem = manager.machine.devices[':maincpu'].spaces['program']
local bscreen = manager.machine.screens[':screen']
local log = assert(io.open(cfg.out .. '/boxes.csv', 'w'))
log:write('snap,kind,x0,y0,x1,y1\n')
local function s8(v) if v >= 128 then return v - 256 end return v end
local function s32(v) if v >= 0x80000000 then return v - 0x100000000 end return v end
local bframe, bnext = 0, EVERY
local function move_of(f, def)
    local mv = bmem:read_u8(f + L.f_move)
    if mv == 0xFF then return nil end
    if mv == 0xFE then
        local w = bmem:read_u8(f + L.f_weapon)
        if w == 0xFF then return nil end
        local contents = bmem:read_u8(CTX + L.ctx_prop + w * L.p_size + L.p_contents)
        local weapons = bmem:read_u32(bmem:read_u32(CTX + L.ctx_stage) + L.s_weapons)
        local m = bmem:read_u32(weapons + contents * L.w_size + L.w_move)
        if m == 0 then return nil end
        return m
    end
    if mv >= bmem:read_u8(def + L.d_nmoves) then return nil end
    return bmem:read_u32(def + L.d_moves) + mv * L.m_size
end
emu.register_frame_done(function()
    bframe = bframe + 1
    local now = manager.machine.time:as_double()
    local snap = nil
    if EVERY > 0 and now >= bnext then
        snap = string.format('box_%06d', bframe)
        bscreen:snapshot(cfg.out .. '/' .. snap .. '.png')
        bnext = now + EVERY
    end
    local cam = s32(bmem:read_u32(CTX + L.ctx_cam))
    local function box(kind, x0, y0, x1, y1, color)
        bscreen:draw_box(x0, y0, x1, y1, color, 0)
        if snap then log:write(string.format('%s,%s,%d,%d,%d,%d\n', snap, kind, x0, y0, x1, y1)) end
    end
    for i = 0, L.fighters - 1 do
        local f = CTX + L.ctx_f + i * L.f_size
        local def = bmem:read_u32(f + L.f_def)
        if bmem:read_u8(f + L.f_alive) ~= 0 and def ~= 0 then
            local x = (s32(bmem:read_u32(f + L.f_x)) - cam) / 256
            local y, z = s32(bmem:read_u32(f + L.f_y)) / 256, s32(bmem:read_u32(f + L.f_z)) / 256
            local feet = y - z + GROUND
            local hw, ht, hd = bmem:read_u8(def + L.d_half_w), bmem:read_u8(def + L.d_height), bmem:read_u8(def + L.d_half_depth)
            local state = bmem:read_u8(f + L.f_state)
            if state == L.st_down then ht = 16 end
            local held = bmem:read_u8(f + L.f_hitstop) ~= 0
            box(held and 'held' or 'hurt', x - hw, feet - ht, x + hw, feet, held and 0xffffffff or 0xff30ff60)
            box('depth', x - hd, y + GROUND - 1, x + hd, y + GROUND + 1, 0xff40a0ff)
            if state == L.st_attack then
                local m = move_of(f, def)
                if m then
                    local mf, su, ac = bmem:read_u8(f + L.f_move_frame), bmem:read_u8(m + L.m_startup), bmem:read_u8(m + L.m_active)
                    local bx, bw = s8(bmem:read_u8(m + L.m_box_x)), bmem:read_u8(m + L.m_box_w)
                    local bz, bh = bmem:read_u8(m + L.m_box_z), bmem:read_u8(m + L.m_box_h)
                    if bw > 0 and bh > 0 then
                        local dir = bmem:read_u8(f + L.f_facing) ~= 0 and -1 or 1
                        local a, b = x + dir * bx, x + dir * (bx + bw)
                        if a > b then a, b = b, a end
                        local live = mf >= su and mf < su + ac
                        if mf < su + ac then
                            box(live and 'strike' or 'windup', a, feet - bz - bh, b, feet - bz, live and 0xffff3030 or 0xffffe040)
                        end
                    end
                end
            end
        end
    end
    if snap then log:flush() end
end)
end
"""

COLORS = {"hurt": (48, 255, 96), "held": (255, 255, 255), "depth": (64, 160, 255), "windup": (255, 224, 64), "strike": (255, 48, 48)}


def write_lua(args):
    lay = layout()
    ctx = symbol(args.elf, args.symbol)
    table = "{" + ",".join(f"{k}={v}" for k, v in lay.items()) + "}"
    text = LUA.replace("__LAYOUT__", table).replace("__CTX__", str(ctx)).replace("__EVERY__", repr(args.every)) \
              .replace("__GROUND__", str(args.ground))
    Path(args.out).write_text(text)
    print(f"beatemup_boxes: {args.out} ({args.symbol} at {ctx:#x}, NGBFighter {lay['f_size']} bytes, NGBMove {lay['m_size']})")


def draw(args):
    from PIL import Image, ImageDraw
    d = Path(args.dir)
    rows = {}
    with (d / "boxes.csv").open() as f:
        for r in csv.DictReader(f):
            rows.setdefault(r["snap"], []).append(r)
    n = 0
    for snap, boxes in sorted(rows.items()):
        p = d / f"{snap}.png"
        if not p.is_file():
            continue
        im = Image.open(p).convert("RGB")
        dr = ImageDraw.Draw(im)
        for b in boxes:
            c = COLORS.get(b["kind"], (255, 0, 255))
            x0, y0, x1, y1 = (int(float(b[k])) for k in ("x0", "y0", "x1", "y1"))
            dr.rectangle([x0, y0, x1, y1], outline=c)
        im.save(d / f"{snap}_boxes.png")
        n += 1
    print(f"beatemup_boxes: {n} pictures with their boxes in {d}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    a = sub.add_parser("lua")
    a.add_argument("out")
    a.add_argument("--symbol", default="s_b")
    a.add_argument("--every", type=float, default=0.5)
    a.add_argument("--elf", default=str(ROOT / "out/game"))
    a.add_argument("--ground", type=int, default=0)
    b = sub.add_parser("draw")
    b.add_argument("dir")
    args = ap.parse_args()
    (write_lua if args.cmd == "lua" else draw)(args)


if __name__ == "__main__":
    sys.exit(main())
