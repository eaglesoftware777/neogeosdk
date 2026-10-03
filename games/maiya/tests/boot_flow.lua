local machine = manager.machine
local mem = machine.devices[':maincpu'].spaces['program']
local screen = machine.screens[':screen']
local a = dofile(os.getenv('MAIYA_LAYOUT'))
local out = assert(io.open(os.getenv('MAIYA_RESULT'), 'w'))
local seen, next_log = false, 0
local trace_step = 0

local function input(name, value)
    for _, port in pairs(machine.ioport.ports) do
        local field = port.fields[name]
        if field then field:set_value(value and 1 or 0) end
    end
end

emu.register_frame_done(function()
    local t = machine.time:as_double()
    if os.getenv('MAIYA_TRACE') ~= '' then
        if t > 11.9 and trace_step == 0 then
            machine.debugger:command('trace ' .. os.getenv('MAIYA_TRACE') .. ',0')
            trace_step = 1
        elseif t > 14 and trace_step == 1 then
            machine.debugger:command('trace off,0')
            trace_step = 2
        end
    end
    local player = mem:read_u32(a.player)
    local playing = player >= 0x100000 and player < 0x10F000
        and mem:read_u8(a.demo) == 0 and mem:read_u8(a.state) == 1
        and mem:read_u8(0x10FDAF) == 2
    input('Coin 1', (t > 8 and t < 8.25) or (t > 9 and t < 9.25))
    -- Two presses: a system ROM that boots slower (the Japan console one,
    -- the UniBIOS) can still be handing over to the game at the first.
    local start = os.getenv('MAIYA_AUTO') ~= '1' and ((t > 12 and t < 12.25) or (t > 16 and t < 16.25))
    input('1 Player Start', start)
    input('P1 Start', start)
    input('Start', start)
    input('P1 A', not playing and t > 13 and ((t - 13) % 0.75) < 0.15)
    if t >= next_log then
        out:write(string.format('t=%.1f req=%d mode=%d demo=%d latch=%d\n', t,
            mem:read_u8(0x10FDAE), mem:read_u8(0x10FDAF),
            mem:read_u8(a.demo), mem:read_u8(a.latch)))
        next_log = t + 2
        out:flush()
    end
    if playing and not seen then
        seen = true
        out:write(string.format('PLAY lives=%d continues=%d credit=%d time=%.1f\n',
            mem:read_u8(a.lives), mem:read_u8(a.continues), mem:read_u8(0xD00034), t))
        out:flush()
        screen:snapshot(os.getenv('MAIYA_SNAPSHOT'))
    end
end)
emu.register_stop(function()
    screen:snapshot(os.getenv('MAIYA_FINAL'))
    out:close()
end)
