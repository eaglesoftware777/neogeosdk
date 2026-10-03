-- One measurement run for tools/perf_report.py: boot, credit and start,
-- jump to the stage, play the scenario for 3,600 frames, write one line.
--
-- From the game's own counters (ng_perf, a PERF=1 build; sdk/ng_perf.h):
-- frames, overruns, scanlines of work, VRAM words and how many of them were
-- written while the screen was being drawn. From the sprite chip's video RAM
-- every 15 frames: the most sprites on any drawn line, counted the way the
-- hardware picks them (MAME src/mame/snk/neogeo_spr.cpp, parse_sprites: a
-- chained sprite takes its Y and height from the one before; height 0 is
-- skipped; height 32 or more is on every line; X doesn't matter).
local machine = manager.machine
local mem = machine.devices[':maincpu'].spaces['program']
local a = dofile(os.getenv('PERF_LAYOUT'))
local out = assert(io.open(os.getenv('PERF_OUT'), 'w'))
local STAGE = tonumber(os.getenv('PERF_STAGE'))
local MODE = os.getenv('PERF_MODE') or 'walk'
local AT = tonumber(os.getenv('PERF_AT') or '120')
local N = 3600
local vram = emu.item(machine.devices[':spritegen'].items['0/m_videoram'])

local function input(name, value)
    for _, port in pairs(machine.ioport.ports) do
        local f = port.fields[name]
        if f then f:set_value(value and 1 or 0) end
    end
end

local function strips()
    local diff, always, y, rows = {}, 0, 0, 0
    for i = 0, 512 do diff[i] = 0 end
    for n = 0, 380 do
        local yc = vram:read(0x8200 + n)
        if (yc & 0x40) == 0 then y = (0x200 - (yc >> 7)) & 0x1ff; rows = yc & 0x3f end
        if rows ~= 0 then
            if rows >= 0x20 then always = always + 1
            else
                local e = y + rows * 16
                diff[y] = diff[y] + 1
                if e <= 512 then diff[e] = diff[e] - 1
                else diff[512] = diff[512] - 1; diff[0] = diff[0] + 1; diff[e - 512] = diff[e - 512] - 1 end
            end
        end
    end
    local cur, peak, at = 0, 0, 0
    for l = 0, 511 do
        cur = cur + diff[l]
        if l >= 16 and l < 240 and cur + always > peak then peak = cur + always; at = l end
    end
    return peak, at
end

local function place(p, x, y)
    mem:write_u16(p + a.char_x, x & 0xFFFF); mem:write_u16(p + a.char_y, y & 0xFFFF)
    mem:write_u32(p + a.char_x_fp, (x * 256) & 0xFFFFFFFF); mem:write_u32(p + a.char_y_fp, (y * 256) & 0xFFFFFFFF)
end

local P = a.ng_perf
local phase, pf, start, speak, sat, over96 = 'boot', 0, nil, 0, 0, 0
local held = {}
local function press(name, on) held[name] = on end

local function play(p, MG)
    mem:write_u8(p + a.char_hp, 5)
    mem:write_u16(MG + a.veil, 300)                 -- she can't be hurt: the run lasts
    if pf == 20 then
        if MODE == 'gate' then mem:write_u8(MG + a.has_key, 1) end
        place(p, AT, 192)
    end
    if pf == 60 then for o = 0, 74, 2 do mem:write_u16(P + o, 0) end; start = pf end
    if MODE == 'gate' and pf < 200 then
        press('P1 Right', pf > 20); press('P1 Up', pf > 30 and pf < 40)
    elseif MODE == 'gate' then
        press('P1 Right', (pf % 120) < 50); press('P1 Left', (pf % 120) >= 60 and (pf % 120) < 110)
        press('P1 A', (pf % 70) < 10); press('P1 B', (pf % 12) < 2)
    else
        press('P1 Right', true); press('P1 A', (pf % 45) < 10); press('P1 B', (pf % 20) < 2)
    end
    if start and (pf - start) % 15 == 0 then
        local s, l = strips()
        if s > speak then speak = s; sat = l end
        if s > 96 then over96 = over96 + 1 end
    end
    if start and pf == start + N then
        local function u16(o) return mem:read_u16(P + o) end
        local function u32(o) return mem:read_u32(P + o) end
        out:write(string.format('frames=%d overruns=%d lines_sum=%d lines_peak=%d vram_sum=%d vram_peak=%d ' ..
            'active_sum=%d active_peak=%d strips_peak=%d strips_line=%d over96=%d state=%d ' ..
            'outside_sum=%d outside_active_sum=%d commit_late=%d commit_spill=%d commit_lines_sum=%d commit_lines_peak=%d groups_peak=%d groups_sum=%d fixlines_sum=%d fixcells_sum=%d\n',
            u32(0), u32(4), u32(8), u16(22), u32(12), u16(26), u32(16), u16(30), speak, sat, over96,
            mem:read_u8(a.state), u32(38), u32(42), u16(46), u16(48), u32(50), u16(54), u16(58), u32(60), u32(64), u32(68)))
        out:close()
        machine:exit()
    end
end

emu.register_frame_done(function()
    local t = machine.time:as_double()
    input('Coin 1', t > 8 and t < 8.25)
    local p = mem:read_u32(a.player)
    local ok = p >= 0x100000 and p <= 0x10efff and mem:read_u8(a.demo) == 0
    local state = mem:read_u8(a.state)
    if phase == 'boot' then
        input('P1 Start', t > 12 and t < 12.25); input('1 Player Start', t > 12 and t < 12.25)
        input('P1 A', t > 15.5 and ((t - 15.5) % 0.75) < 0.15)
        if ok and state == a.st_play then
            if STAGE > 0 then
                mem:write_u8(a.next_stage, STAGE); mem:write_u8(a.state, a.st_interlude); mem:write_u16(a.state_timer, 1)
            end
            phase = 'load'; pf = 0
        end
        if t > 120 then out:write('ERROR never reached play\n'); out:close(); machine:exit() end
        return
    end
    pf = pf + 1
    if phase == 'load' then
        input('P1 A', false); input('P1 B', (pf % 10) < 2)
        if mem:read_u8(a.stage) == STAGE and state == a.st_play then phase = 'play'; pf = 0; input('P1 B', false) end
        if pf > 4000 then out:write('ERROR never reached the stage\n'); out:close(); machine:exit() end
        return
    end
    held = {}
    if ok then play(p, a.mg) end
    for _, n in ipairs({'P1 Up', 'P1 Down', 'P1 Left', 'P1 Right', 'P1 A', 'P1 B', 'P1 C', 'P1 D'}) do
        input(n, held[n] or false)
    end
end)
