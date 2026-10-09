-- The Python capture runner supplies paths and compiled symbol addresses.
local config = __SKY_QA_CONFIG__
local machine = manager.machine
local memory = machine.devices[':maincpu'].spaces['program']
local audio = machine.devices[':audiocpu'].spaces['program']
local screen = machine.screens[':screen']
local fields = config.symbols
local log = assert(io.open(config.output .. '/state.csv', 'w'))
log:write('seconds,frame,main_pc,audio_pc,user_mode,credit,player,energy,stage,pose,scroll,slot_wait,timer,protocol\n')
local frames, next_capture = 0, 0
local ports = assert(io.open(config.output .. '/input-ports.txt', 'w'))
for tag, port in pairs(machine.ioport.ports) do
    for name, field in pairs(port.fields) do ports:write(tag .. ',' .. name .. '\n') end
end
ports:close()
local joystick = machine.ioport.ports[':edge:joy:JOY1'] and ':edge:joy:JOY1' or ':ctrl1:joy:JOY'
local start_port = machine.ioport.ports[':edge:joy:START'] and ':edge:joy:START' or ':ctrl1:joy:START_SELECT'
local start_name = start_port == ':edge:joy:START' and '1 Player Start' or 'P1 Start'

local function input(port, name, value)
    local p = machine.ioport.ports[port]
    if p and p.fields[name] then p.fields[name]:set_value(value) end
end

local function byte(name)
    return memory:read_u8(fields[name])
end

emu.register_frame_done(function()
    frames = frames + 1
    local now = machine.time:as_double()
    local player = memory:read_u32(fields.s_player)
    local mode = memory:read_u8(0x10fdaf)
    input(':AUDIO_COIN', 'Coin 1', (not config.attract_only and now > 3 and now < 3.2) and 1 or 0)
    local start = not config.attract_only and now > 5 and now < 18 and ((now - 5) % 2 < 0.15)
    input(start_port, start_name, start and 1 or 0)
    local phase = now % 6
    input(joystick, 'P1 Left', player ~= 0 and phase > 1 and phase < 2 and 1 or 0)
    input(joystick, 'P1 Right', player ~= 0 and phase > 4 and phase < 5 and 1 or 0)
    input(joystick, 'P1 A', not config.attract_only and now > 7 and (math.floor(now * 4) % 2 == 0) and 1 or 0)
    input(joystick, 'P1 D', player ~= 0 and now > 12 and now < 12.15 and 1 or 0)
    if config.stage and mode == 2 and player == 0 and now > 5 then
        memory:write_u8(fields.s_stage, config.stage)
    end
    if config.stage and player ~= 0 then
        -- Inspection runs keep the craft alive to reach each sector's boss.
        memory:write_u8(fields.s_invuln, 120)
    end
    if now >= next_capture then
        local name = config.output .. '/t' .. string.format('%05.1f', now):gsub(',', '.')
        screen:snapshot(name .. '.png')
        local protocol = ''
        for i = 0, 3 do protocol = protocol .. string.format('%02x', audio:read_u8(0xfe44 + i)) end
        log:write(string.format('%s,%d,%x,%x,%d,%d,%x,%d,%d,%d,%d,%d,%d,%s\n',
            string.format('%.3f', now):gsub(',', '.'), frames, machine.devices[':maincpu'].state['PC'].value,
            machine.devices[':audiocpu'].state['PC'].value, mode,
            memory:read_u8(0x10fd00), player, byte('s_energy'), byte('s_stage'),
            byte('s_plane_pose'), math.floor(memory:read_u32(fields.s_camera) / 256) % 512,
            audio:read_u8(0xfe42), audio:read_u8(0xf927), protocol))
        log:flush()
        next_capture = now + 2
    end
end)
