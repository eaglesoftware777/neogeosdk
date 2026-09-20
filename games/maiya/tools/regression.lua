-- Inputs and debugger-only scenario placement; no test hooks in the cartridge.
local machine = manager.machine
local screen = machine.screens[':screen']
local memory = machine.devices[':maincpu'].spaces['program']
local output = assert(os.getenv('MG_CAPTURE_DIR'))
local a = dofile(output .. '/layout.lua')
local scenario = os.getenv('MG_SCENARIO') or 'walk'
local log = assert(io.open(output .. '/telemetry.jsonl', 'w'))
local next_capture, placed, play_frame = 0, false, 0
local function u8(name) return memory:read_u8(a[name]) end
local function w8(name, value) memory:write_u8(a[name], value) end
local function s16(addr)
    local n = memory:read_u16(addr)
    return n >= 32768 and n - 65536 or n
end
local function input(name, value)
    for _, port in pairs(machine.ioport.ports) do
        local f = port.fields[name]
        if f then f:set_value(value and 1 or 0) end
    end
end
local function position(p, x, y)
    memory:write_u16(p + a.char_x, x)
    memory:write_u16(p + a.char_y, y)
    memory:write_u32(p + a.char_x_fp, x * 256)
    memory:write_u32(p + a.char_y_fp, y * 256)
end
emu.register_frame_done(function()
    local t = machine.time:as_double()
    input('Coin 1', t > 8 and t < 8.25)
    input('1 Player Start', t > 12 and t < 12.25)
    local p = memory:read_u32(a.player)
    if p < 0x100000 or p > 0x10efff then p = 0 end
    local mode = memory:read_u8(0x10fdaf)
    if p ~= 0 and mode == 2 and u8('demo') == 0 then
        play_frame = play_frame + 1
        if not placed and u8('state') == 1 then
            placed = true
            if scenario == 'climb' then position(p, 708, 192) end
            if scenario == 'boss' then
                position(p, 3560, 192)
                memory:write_u16(a.camera_x, 3400)
                w8('gate_unlocked', 1)
            end
            if scenario == 'bonus' then
                w8('stage', 1)
                w8('state', 2)
                memory:write_u16(a.state_timer, 1)
            end
        end
        input('P1 Right', scenario == 'walk' or (scenario == 'bonus' and play_frame % 240 < 120))
        input('P1 Left', scenario == 'boss' or (scenario == 'bonus' and play_frame % 240 >= 120))
        input('P1 Up', scenario == 'climb' and placed and play_frame < 210)
        input('P1 Down', scenario == 'climb' and play_frame >= 240 and play_frame < 390)
        input('P1 A', (scenario == 'walk' or scenario == 'bonus') and play_frame % 180 < 6)
        input('P1 B', (scenario == 'walk' or scenario == 'bonus') and play_frame % 24 < 6)
        -- Keep arena observation alive without changing movement or collisions.
        if scenario == 'boss' then memory:write_u8(p + a.char_hp, 5) end
    end
    if t >= next_capture then
        local b = memory:read_u32(a.boss)
        if b < 0x100000 or b > 0x10efff then b = 0 end
        screen:snapshot(string.format('%s/frame_%05d.png', output, math.floor(t * 10)))
        log:write(string.format('{"time":%.2f,"mode":%d,"state":%d,"stage":%d,"player":%d,"x":%d,"y":%d,"flip":%d,"climbing":%d,"camera_x":%d,"shake_x":%d,"boss_active":%d,"boss":%d,"boss_x":%d}\n',
            t, mode, u8('state'), u8('stage'), p, p ~= 0 and s16(p + a.char_x) or 0,
            p ~= 0 and s16(p + a.char_y) or 0, p ~= 0 and memory:read_u8(p + a.char_flip_x) or 0,
            u8('climbing'), s16(a.camera_x), s16(a.shake_x), u8('boss_active'), b, b ~= 0 and s16(b + a.char_x) or 0))
        log:flush()
        next_capture = t + 0.5
    end
end)
emu.register_stop(function() log:close() end)
