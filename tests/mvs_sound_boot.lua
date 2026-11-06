-- Exercise warm initialization and BIOS handoff through the real MAME Z80.
local machine = manager.machine
local main = machine.devices[':maincpu']
local memory = main.spaces['program']
local cpu = machine.devices[':audiocpu']
local ram = cpu.spaces['program']
local output = assert(os.getenv('MVS_BOOT_REPORT'))
local log = assert(io.open(output, 'w'))
local isolated, stage, waiting = false, 0, false
local parameters = {5, 255, 137, 5, 255, 129, 5, 255, 131, 5, 0x3f}
local parameter = 1
local banks, writes, ready, enabled = {}, {}, 0, false
local address = 0
local reset_started = nil
local wt = cpu.spaces['io']:install_write_tap(0, 0xffff, 'boot-writes', function(offset, data)
    local port = offset & 255
    if isolated and port == 12 and data == 1 then
        assert(stage == 3 or stage == 8, 'Reserved slot reply used for ordinary readiness')
    end
    if isolated and port == 12 and data == 0x80 and reset_started then
        local elapsed = machine.time:as_double() - reset_started
        assert(elapsed < 0.1, 'Restart exceeded 100 ms')
        log:write(string.format('PASS reset completed in %.3f ms\n', elapsed * 1000))
        reset_started = nil
    end
    if not waiting then return end
    if port == 4 then address = data end
    if port == 5 then writes[address] = data end
    if port == 8 then enabled = true end
    if port == 12 and data == 0x80 then
        assert(enabled, 'Ready reply preceded NMI enable')
        assert(writes[0x22] == 0, 'Warm init skipped LFO reset')
        ready = ready + 1
    end
end)
local rt = cpu.spaces['io']:install_read_tap(0, 0xffff, 'boot-banks', function(offset)
    local port = offset & 255
    if waiting and port >= 8 and port <= 11 then banks[port] = offset >> 8 end
end)
local function send(value) memory:write_u8(0x320000, value) end
emu.register_frame_done(function()
    local now = machine.time:as_double()
    memory:write_u8(0x300001, 0)
    if now >= 15 and not isolated then
        memory:write_u16(0x10f000, 0x60fe)
        main.state['SR'].value = 0x2700
        main.state['CURPC'].value = 0x10f000
        isolated = true
    end
    if not isolated then return end
    if stage == 0 and now >= 16 then
        local source = os.getenv('MVS_FOREIGN_RAM')
        if source then
            local file = assert(io.open(source, 'rb'))
            local data = file:read('*a')
            file:close()
            assert(#data == 2048, 'Expected complete Z80 work RAM')
            for i = 1, #data do ram:write_u8(0xf7ff + i, data:byte(i)) end
        else
            for at = 0xf800, 0xffff do ram:write_u8(at, 0xa5) end
        end
        -- Model a ROM handoff: new NMI vector, previous driver's RAM loop.
        ram:write_u8(0xfffd, 0xc3)
        ram:write_u8(0xfffe, 0xfd)
        ram:write_u8(0xffff, 0xff)
        cpu.state['PC'].value = 0xfffd
        cpu.state['SP'].value = 0xfffc
        reset_started = now
        send(3)
        stage = 1
    elseif stage == 1 and now >= 17 then
        assert(ram:read_u8(0xfe44) == 0x4e and ram:read_u8(0xfe47) == 0x31,
            'Foreign RAM was not replaced on BIOS restart')
        assert(memory:read_u8(0x320000) == 0x80, 'Foreign handoff did not become ready')
        log:write('PASS foreign-RAM BIOS handoff\n')
        for at = 0xf900, 0xfaff do ram:write_u8(at, 0) end
        banks, writes, ready, enabled = {}, {}, 0, false
        waiting = true
        reset_started = now
        send(9)
        stage = 2
    elseif stage == 2 and now >= 18 then
        waiting = false
        assert(ready == 1, 'Expected one completed-initialization reply')
        assert(banks[8] == 0x1e and banks[9] == 0x0e
            and banks[10] == 6 and banks[11] == 2, 'Incorrect ROM bank setup')
        assert(memory:read_u8(0x320000) == 0x80, 'Driver not ready')
        log:write('PASS warm init: cache rebuilt, banks restored, ready after NMI enable\n')
        send(1)
        stage = 3
    elseif stage == 3 and now >= 19 then
        assert(cpu.state['PC'].value >= 0xf800, 'Slot wait executed outside RAM')
        assert(memory:read_u8(0x320000) == 1, 'Slot wait not acknowledged')
        log:write('PASS slot wait: RAM execution and ready reply\n')
        reset_started = now
        send(3)
        stage = 4
    elseif stage == 4 and now >= 20 then
        assert(ram:read_u8(0xfe42) == 0, 'Slot wait did not restart')
        assert(memory:read_u8(0x320000) == 0x80, 'Restart did not finish')
        log:write('PASS BIOS restart\n')
        send(1)
        stage = 8
    elseif stage == 8 and now >= 20.1 then
        assert(memory:read_u8(0x320000) == 1, 'Bootstrap test did not enter slot wait')
        reset_started = now
        memory:write_u32(0x10ee00, 0x10f000)
        main.state['SP'].value = 0x10ee00
        main.state['CURPC'].value = 0x1b3a
        stage = 9
    elseif stage == 9 and now >= 20.4 then
        assert(main.state['CURPC'].value == 0x10f000, 'P1 bootstrap did not return')
        assert(memory:read_u8(0x320000) == 0x80, 'P1 bootstrap did not restart driver')
        log:write('PASS patched 68000 bootstrap from BIOS RAM wait\n')
        stage = 5
    elseif stage == 5 and parameter <= #parameters then
        if parameter == 4 then
            assert(ram:read_u8(0xfe0c) == 9, 'Parameter 09 was treated as reset')
        elseif parameter == 7 then
            assert(ram:read_u8(0xfe0c) == 1, 'Parameter 01 was treated as slot switch')
        elseif parameter == 10 then
            assert(ram:read_u8(0xfe0c) == 3, 'Parameter 03 was treated as BIOS reset')
        end
        send(parameters[parameter])
        parameter = parameter + 1
    elseif stage == 5 and now >= 21 then
        log:write('PASS reserved-byte parameters\n')
        send(0x40)
        stage = 6
    elseif stage == 6 and now >= 23 then
        send(0x80)
        stage = 7
    elseif stage == 7 and now >= 29 then
        log:write('PASS ADPCM commands delivered\n')
        log:close()
        wt:remove()
        rt:remove()
        machine:exit()
        return
    end
    log:flush()
end)
