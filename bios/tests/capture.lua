local machine = manager.machine
local cpu = machine.devices[':maincpu']
local memory = cpu.spaces['program']
local screen = machine.screens[':screen']
local vram = emu.item(machine.devices[':spritegen'].items['0/m_videoram'])
local output = os.getenv('EAGLE_TEST_DIR')
local probe = os.getenv('EAGLE_TEST_GAME'):sub(1, 5) == 'probe'
local frame = 0
local log = assert(io.open(output .. '/state.jsonl', 'w'))

local function pc(device)
    local value = device.state['PC'] or device.state['CURPC']
    return value and value.value or 0
end

local function press(name, value)
    for _, port in pairs(machine.ioport.ports) do
        for _, field in pairs(port.fields) do
            if field.name == name then
                field:set_value(value)
            end
        end
    end
end

emu.register_frame_done(function()
    frame = frame + 1
    local t = machine.time:as_double()
    -- The power-on presentation owns the first three seconds.  Ten coin
    -- edges then exercise the BCD 09 -> 10 carry before one start.
    local coin = t >= 4 and t < 8 and ((t - 4) % 0.4) < 0.12
    press('Coin 1', coin and 1 or 0)
    press('1 Player Start', (t >= 9 and t < 9.2) and 1 or 0)
    press('P1 Start', (t >= 9 and t < 9.2) and 1 or 0)
    press('P1 A', (not probe and t >= 11 and (math.floor(t) % 3 == 0)) and 1 or 0)
    if frame % 60 == 1 then
        screen:snapshot(string.format('%s/frame-%04d.png', output, frame))
        log:write(string.format(
            '{"time":%.2f,"pc":%d,"z80_pc":%d,"request":%d,"mode":%d,"mvs":%d,' ..
            '"credit":%d,"sentinel":%d,"requests":%d,"starts":%d,"coins":%d,' ..
            '"entry_sr":%d,"pad_status":%d,"mess_point":%d,"message":%d,"increment":%d,"ticks":%d,"status":%d}\n',
            t, pc(cpu), pc(machine.devices[':audiocpu']),
            memory:read_u8(0x10fdae), memory:read_u8(0x10fdaf), memory:read_u8(0x10fd82),
            memory:read_u8(0xd00034), memory:read_u32(0x100000), memory:read_u16(0x100004),
            memory:read_u16(0x100006), memory:read_u16(0x100008), memory:read_u16(0x100010),
            memory:read_u8(0x10fd94), memory:read_u32(0x10fdbe), vram:read(0x720e), vram:read(0x7242),
            memory:read_u32(0x10000c), memory:read_u8(0x320001)))
        log:flush()
    end
end)

emu.register_stop(function() log:close() end)
