local config = __MAIYA_CONFIG__
local machine = manager.machine
local memory = machine.devices[':maincpu'].spaces['program']
local audio = machine.devices[':audiocpu'].spaces['program']
local screen = machine.screens[':screen']
local fields = config.fields
local log = assert(io.open(config.output .. '/state.csv', 'w'))
log:write('seconds,mode,player,demo,state,stage,x,y,camera,slot_wait,timer,protocol\n')
local next_capture, selected = 0, false
local function input(name, value)
    for _, port in pairs(machine.ioport.ports) do
        if port.fields[name] then port.fields[name]:set_value(value and 1 or 0) end
    end
end
local function signed16(address)
    local n = memory:read_u16(address)
    return n >= 32768 and n-65536 or n
end
emu.register_frame_done(function()
    local now = machine.time:as_double()
    input('Coin 1', now > 8 and now < 8.3)
    local start = now > 12 and now < 17 and ((now-12) % 2 < 0.15)
    input('1 Player Start', start)
    input('P1 Start', start)
    local player = fields.player and memory:read_u32(fields.player) or 0
    if player < 0x100000 or player > 0x10efff then player = 0 end
    local mode = memory:read_u8(0x10fdaf)
    local demo = fields.demo and memory:read_u8(fields.demo) or 0
    local state = fields.state and memory:read_u8(fields.state) or 255
    local playing = mode == 2 and player ~= 0 and demo == 0 and state == 1
    input('P1 A', player == 0 and now > 13 and ((now-13) % 0.75 < 0.15))
    input('P1 Right', (playing or config.baseline) and now > 22 and now < 35)
    input('P1 Left', (playing or config.baseline) and now > 36 and now < 42)
    input('P1 B', playing and now > 25 and (now % 3 < 0.12))
    if config.stage and playing and not selected then
        memory:write_u8(fields.next_stage, config.stage)
        memory:write_u8(fields.state, 8)
        memory:write_u16(fields.state_timer, 1)
        selected = true
    end
    if now >= next_capture then
        local time = string.format('%.3f', now):gsub(',', '.')
        local name = string.format('%05.1f', now):gsub(',', '.')
        screen:snapshot(config.output .. '/t' .. name .. '.png')
        local protocol = ''
        for i=0,3 do protocol = protocol .. string.format('%02x', audio:read_u8(0xfe44+i)) end
        log:write(string.format('%s,%d,%x,%d,%d,%d,%d,%d,%d,%d,%d,%s\n', time, mode,
            player, demo, state, fields.stage and memory:read_u8(fields.stage) or 255,
            player ~= 0 and signed16(player + fields.char_x) or 0,
            player ~= 0 and signed16(player + fields.char_y) or 0,
            fields.camera and signed16(fields.camera) or 0,
            audio:read_u8(0xfe42), audio:read_u8(0xf927), protocol))
        log:flush()
        next_capture = now+2
    end
end)
