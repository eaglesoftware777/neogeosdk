-- Exercise the real cartridge startup and capture native frames and telemetry.
local machine = manager.machine
local screen = machine.screens[':screen']
local memory = machine.devices[':maincpu'].spaces['program']
local output = assert(os.getenv('MG_CAPTURE_DIR'))
local stage_addr = tonumber(os.getenv('MG_STAGE_ADDR'))
local state_addr = tonumber(os.getenv('MG_STATE_ADDR'))
local x_addr = tonumber(os.getenv('MG_X_ADDR'))
local y_addr = tonumber(os.getenv('MG_Y_ADDR'))
local boss_addr = tonumber(os.getenv('MG_BOSS_ADDR'))
local rescue_addr = tonumber(os.getenv('MG_RESCUE_ADDR'))
local next_capture = 0
local log = assert(io.open(output .. '/telemetry.csv', 'w'))
log:write('time,stage,state,x,y,boss_hp,rescues,bios_mode,start,credit,request,buttons\n')

local function input(port, name, value)
    local p = machine.ioport.ports[port]
    if p and p.fields[name] then p.fields[name]:set_value(value) end
end

emu.register_frame_done(function()
    local t = machine.time:as_double()
    input(':AUDIO_COIN', 'Coin 1', t > 5 and t < 5.25 and 1 or 0)
    input(':edge:joy:START', '1 Player Start', t > 7 and t < 7.25 and 1 or 0)
    if t > 9 then
        local active = os.getenv('MG_CAPTURE_IDLE') ~= '1'
        input(':edge:joy:JOY1', 'P1 Right', active and 1 or 0)
        input(':edge:joy:JOY1', 'P1 B', active and 1 or 0)
        input(':edge:joy:JOY1', 'P1 A', active and t % 4 > 3.5 and 1 or 0)
        input(':edge:joy:JOY1', 'P1 D', active and t % 12 < 0.05 and 1 or 0)
    end
    if t >= next_capture then
        screen:snapshot(string.format('%s/frame_%03d.png', output, math.floor(t)))
        log:write(string.format('%.2f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n', t,
            memory:read_u16(stage_addr), memory:read_u16(state_addr),
            memory:read_u16(x_addr), memory:read_u16(y_addr),
            memory:read_u16(boss_addr), memory:read_u16(rescue_addr),
            memory:read_u8(0x10fdaf), memory:read_u8(0xd00100), memory:read_u8(0xd00034),
            memory:read_u8(0x10fdae), memory:read_u8(0x10fd96)))
        log:flush()
        next_capture = t + 2
    end
end)

emu.register_stop(function() log:close() end)
