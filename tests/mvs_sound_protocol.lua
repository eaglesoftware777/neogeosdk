-- Check BIOS priority and all 256 parameter values through the patched P1.
local machine = manager.machine
local main = machine.devices[':maincpu']
local memory = main.spaces['program']
local z80 = machine.devices[':audiocpu']
local ram = z80.spaces['program']
local report = assert(io.open(assert(os.getenv('MVS_PROTOCOL_REPORT')), 'w'))
local phase, deadline, value, parameter_phase = 0, 0, 0, false
local finished = false
local a, b = 0, 0
local ym = {}
local wire = {}
local function pass(message) report:write('PASS ' .. message .. '\n'); report:flush() end
local tap = z80.spaces['io']:install_write_tap(0, 0xffff, 'protocol-ym', function(offset, data)
    local port = offset & 255
    if port == 4 then a = data end
    if port == 6 then b = data end
    if port == 5 then ym[a] = data end
    if port == 7 then ym[256 + b] = data end
end)
local readtap = z80.spaces['io']:install_read_tap(0, 0xffff, 'protocol-wire', function(offset, data)
    if offset & 255 == 0 then wire[#wire + 1] = data end
end)
local function send(command) memory:write_u8(0x320000, command) end
local function call_p1(address, argument)
    memory:write_u32(0x10ee00, 0x10f000)
    memory:write_u32(0x10ee04, argument or 0)
    main.state['SP'].value = 0x10ee00
    main.state['CURPC'].value = address
end
local function returned()
    assert(main.state['CURPC'].value == 0x10f000, 'P1 sound call did not return')
    assert(memory:read_u8(0x320000) == 0x80, 'Driver did not become ready')
end
local function step()
    local now = machine.time:as_double()
    memory:write_u8(0x300001, 0)
    if now < 15 or now < deadline then return end
    if phase == 0 then
        memory:write_u16(0x10f000, 0x60fe)
        main.state['SR'].value = 0x2700
        call_p1(0x1b3a)
        phase, deadline = 1, now + 0.2
    elseif phase == 1 then
        returned()
        ram:write_u8(0xfe43, 1)
        ram:write_u8(0xfe48, 1)
        send(1)
        phase, deadline = 2, now + 0.05
    elseif phase == 2 then
        assert(memory:read_u8(0x320000) == 1, 'Pending escape swallowed BIOS 01')
        assert(z80.state['PC'].value == 0xff85, 'Slot wait not in RAM')
        assert(ym[0x27] == 0x30 and ym[0x100] == 0xbf and ym[0x10] == 1,
            string.format('Slot silence: timer=%02X A=%02X B=%02X',
                ym[0x27] or 0, ym[0x100] or 0, ym[0x10] or 0))
        pass('BIOS 01 interrupts pending escape and silences chip')
        send(2)
        phase, deadline = 3, now + 0.05
    elseif phase == 3 then
        assert(memory:read_u8(0x320000) == 0x80, 'BIOS 02 did not initialize')
        assert(ram:read_u8(0xfe08) == 1 and ram:read_u8(0xfe43) == 0,
            'BIOS 02 did not start eyecatcher independently')
        pass('BIOS 02 restarts directly from slot wait and starts eyecatcher')
        ram:write_u8(0xfe43, 1)
        ram:write_u8(0xfe48, 1)
        ram:write_u8(0xfe0b, 255)
        send(3)
        phase, deadline = 4, now + 0.05
    elseif phase == 4 then
        assert(memory:read_u8(0x320000) == 0x80, 'Pending parameter swallowed BIOS 03')
        assert(ram:read_u8(0xfe43) == 0 and ram:read_u8(0xfe48) == 0
            and ram:read_u8(0xfe0b) == 0 and ram:read_u8(0xfe08) == 0,
            'BIOS 03 left stale parameter or playback state')
        pass('BIOS 03 interrupts pending parameter and completes under 50 ms')
        -- Repeat 02 with a live parser, not only with a parked driver.
        ram:write_u8(0xfe43, 1)
        ram:write_u8(0xfe48, 1)
        send(2)
        phase, deadline = 5, now + 0.05
    elseif phase == 5 then
        assert(ram:read_u8(0xfe08) == 1 and ram:read_u8(0xfe43) == 0,
            'Pending parameter swallowed BIOS 02')
        pass('BIOS 02 bypasses incomplete game transactions')
        call_p1(0x1b3a)
        phase, deadline = 6, now + 0.2
    elseif phase == 6 then
        returned()
        if not parameter_phase then
            call_p1(0x1af0, 6)
            parameter_phase = true
        else
            wire = {}
            call_p1(0x1af0, value)
            phase = 7
        end
        deadline = now + 0.02
    elseif phase == 7 then
        returned()
        assert(ram:read_u8(0xfe0d) == value, string.format('Parameter %02X corrupted', value))
        assert(ram:read_u8(0xfe42) == 0, 'Parameter triggered BIOS switch')
        if value == 1 or value == 2 or value == 3 or value == 9 or value == 255 then
            assert(#wire == 2 and wire[1] == 255 and wire[2] == (value ~ 128),
                'P1 did not escape reserved parameter')
        else
            assert(#wire == 1 and wire[1] == value, 'Ordinary parameter wire format changed')
        end
        value, parameter_phase, phase = value + 1, false, 6
        if value == 256 then
            pass('all 256 parameters round-trip through actual P1 adapter')
            finished = true
            report:close()
            tap:remove(); readtap:remove()
            machine:exit()
        end
    end
end
emu.register_frame_done(function()
    if finished then return end
    local ok, error = xpcall(step, debug.traceback)
    if not ok then
        finished = true
        report:write('FAIL ' .. tostring(error) .. '\n'); report:close()
        tap:remove(); readtap:remove()
        machine:exit()
    end
end)
