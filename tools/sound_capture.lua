-- Isolate the real Z80 driver, send its public commands, and record YM writes.
local machine = manager.machine
local main = machine.devices[':maincpu']
local memory = main.spaces['program']
local z80 = machine.devices[':audiocpu']
local output = assert(os.getenv('SOUND_CAPTURE_DIR'))
local log = assert(io.open(output .. '/ym.tsv', 'w'))
log:write('time\tport\tregister\tvalue\n')
local commands = assert(io.open(output .. '/commands.tsv', 'w'))
commands:write('time\tevent\tbyte\n')
local latch = {0, 0}
local isolated = false
local origin = 0
local tap = z80.spaces['io']:install_write_tap(0, 0xffff, 'ym-capture', function(offset, data)
    local port = offset & 255
    if port == 4 or port == 6 then
        latch[(port - 4) // 2 + 1] = data
    elseif isolated and (port == 5 or port == 7) then
        local bank = (port - 5) // 2
        log:write(string.format('%.9f\t%d\t%02x\t%02x\n',
            machine.time:as_double(), bank, latch[bank + 1], data))
    end
end)

local events = {
    {3, {1}, 'init'},
    {4, {4, 0x31, 3, 0x1e, 120}, 'fm_120'},
    {10, {0x1e, 90}, 'fm_90'},
    {16, {0x1e, 150}, 'fm_150'},
    {22, {4, 0x31, 6, 0x17, 0}, 'lfo_off'},
    {26, {0x17, 8}, 'lfo_0'},
    {30, {0x17, 12}, 'lfo_4'},
    {34, {0x17, 15}, 'lfo_7'},
    {38, {4, 0x32, 0, 0x1f, 120}, 'ssg_120'},
    {44, {0x07, 0}, 'ssg_mute'},
    {46, {4, 0x05, 0x3f, 0x40}, 'coin'},
    {48, {0x16, 92}, 'voice'},
    {51, {4, 0x06, 0xb8, 0x80}, 'adpcmb_32k'},
    {64, {4, 0x32, 4}, 'ssg_formant'},
    {69, {4}, 'stop'},
    {70, {0x31, 3, 0x13, 0}, 'fm_mute'},
    {72, {4, 0x05, 0x28, 0x16, 92, 0x16, 93, 0x16, 94,
           0x16, 95, 0x16, 96, 0x16, 97}, 'six_voices'},
    {75, {4}, 'final_stop'},
    {76, {4, 0x06, 0xa8, 0x18, 1, 0x80}, 'b_loop'},
    {89, {0x0e, 1, 0x0a, 0xfe}, 'b_fade'},
    {92, {4, 0x06, 0xb8, 0x80}, 'b_one_shot'},
    {106, {4}, 'end_stop'},
    {108, {1, 0x23}, 'mml_mix'},
    {141, {4, 0x20}, 'mml_inline_b'},
    {144, {4}, 'mml_stop'},
}
local event, byte = 1, 1
emu.register_frame_done(function()
    local now = machine.time:as_double()
    local function input(port, name, value)
        local p = machine.ioport.ports[port]
        if p and p.fields[name] then p.fields[name]:set_value(value) end
    end
    input(':AUDIO_COIN', 'Coin 1', (now >= 7 and now < 7.25) and 1 or 0)
    input(':edge:joy:START', '1 Player Start', (now >= 9 and now < 9.25) and 1 or 0)
    if now >= 15 and not isolated then
        -- Keep the 68k out of the command stream without changing ROM files.
        local rom = assert(io.open(output .. '/loaded-m1.bin', 'wb'))
        for address = 0, 0x7fff do
            rom:write(string.char(z80.spaces['program']:read_u8(address)))
        end
        rom:close()
        memory:write_u16(0x10f000, 0x60fe)
        main.state['SR'].value = 0x2700
        main.state['CURPC'].value = 0x10f000
        isolated = true
        origin = now - 2
    end
    if not isolated then return end
    memory:write_u8(0x300001, 0) -- service watchdog while the 68k is parked
    local e = events[event]
    if e and now - origin >= e[1] then
        commands:write(string.format('%.9f\t%s\t%02x\n', now, e[3], e[2][byte]))
        memory:write_u8(0x320000, e[2][byte])
        byte = byte + 1
        if byte > #e[2] then event, byte = event + 1, 1 end
    end
    log:flush()
    if now - origin >= 145 then
        tap:remove()
        log:close()
        commands:close()
        machine:exit()
    end
end)
